/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "formatting/formatting.h"
#include "formatting/formatting_context.h"
#include "formatting/rules_map.h"
#include "internal_api.h"
#include "public/public.h"
#include "lexer/lexer.h"
#include "lexer/ETSLexer.h"
#include "lexer/TSLexer.h"
#include "lexer/token/token.h"
#include "es2panda.h"

namespace ark::es2panda::lsp {

// NOLINTNEXTLINE
bool TokenMatch(TokenRange &tokenRange, lexer::TokenType tokenType)
{
    auto &tokens = tokenRange.GetTokens();
    return tokens.empty() || std::find(tokens.begin(), tokens.end(), tokenType) != tokens.end();
}

void ApplyInsertSpace(RuleAction action, const TextSpan &span, const std::string &sourceText,
                      std::vector<TextChange> &changes)
{
    if ((static_cast<uint16_t>(RuleAction::INSERT_SPACE) & static_cast<uint16_t>(action)) != 0) {
        if (span.length == 1 && span.start < sourceText.length() && sourceText[span.start] == ' ') {
            return;
        }
        changes.emplace_back(TextChange {span, " "});
    }
}

void ApplyDeleteSpace(RuleAction action, const TextSpan &span, std::vector<TextChange> &changes)
{
    if ((static_cast<uint16_t>(RuleAction::DELETE_SPACE) & static_cast<uint16_t>(action)) != 0) {
        if (span.length > 0) {
            changes.emplace_back(TextChange {span, ""});
        }
    }
}

void ApplyInsertNewline(RuleAction action, const TextSpan &span, const std::string &sourceText,
                        const std::string &newLineCharacter, std::vector<TextChange> &changes)
{
    if ((static_cast<uint16_t>(RuleAction::INSERT_NEWLINE) & static_cast<uint16_t>(action)) != 0) {
        auto nlLen = newLineCharacter.length();
        if (span.length == nlLen && span.start + nlLen <= sourceText.length() &&
            sourceText.compare(span.start, nlLen, newLineCharacter) == 0) {
            return;
        }
        changes.emplace_back(TextChange {span, newLineCharacter});
    }
}

void ApplyDeleteToken(RuleAction action, const lexer::SourceRange &tokenLoc, std::vector<TextChange> &changes)
{
    if ((static_cast<uint16_t>(RuleAction::DELETE_TOKEN) & static_cast<uint16_t>(action)) != 0) {
        TextSpan span {tokenLoc.start.index, tokenLoc.end.index - tokenLoc.start.index};
        changes.emplace_back(TextChange {span, ""});
    }
}

void ApplyInsertSemicolon(RuleAction action, const lexer::SourceRange &tokenLoc, std::vector<TextChange> &changes)
{
    if ((static_cast<uint16_t>(RuleAction::INSERT_TRAILING_SEMICOLON) & static_cast<uint16_t>(action)) != 0) {
        TextSpan span {tokenLoc.end.index, 0};
        changes.emplace_back(TextChange {span, ";"});
    }
}

void ExecuteRuleAction(FormattingContext &context, std::vector<TextChange> &changes, Rule &rule)
{
    const auto &currentToken = context.GetCurrentToken();
    const auto &nextToken = context.GetNextToken();
    const auto &sourceText = context.GetSourceText();
    const auto &newLineCharacter = context.GetOptions().GetNewLineCharacter();
    auto action = rule.GetRuleAction();

    auto currLoc = currentToken.Loc();
    auto nextLoc = nextToken.Loc();

    size_t start = currLoc.end.index;
    size_t end = nextLoc.start.index;

    bool tokensOnDifferentLines = currLoc.end.line != nextLoc.start.line;
    bool canDeleteNewlines = rule.GetRuleFlags() == RuleFlags::CAN_DELETE_NEWLINES;

    if (start <= end) {
        TextSpan whitespaceSpan {start, end - start};
        if (!tokensOnDifferentLines || canDeleteNewlines) {
            ApplyInsertSpace(action, whitespaceSpan, sourceText, changes);
            ApplyInsertNewline(action, whitespaceSpan, sourceText, newLineCharacter, changes);
        }
        ApplyDeleteSpace(action, whitespaceSpan, changes);
    }

    ApplyDeleteToken(action, nextLoc, changes);
    ApplyInsertSemicolon(action, nextLoc, changes);
}

void ApplyRulesOnRange(FormattingContext &context, std::vector<TextChange> &changes, RulesMap &rulesMap)
{
    const auto &currentToken = context.GetCurrentToken();
    const auto &nextToken = context.GetNextToken();

    if (currentToken.Type() == lexer::TokenType::EOS || nextToken.Type() == lexer::TokenType::EOS) {
        return;
    }

    auto allRules = rulesMap(context);

    for (auto &ruleSpec : allRules) {
        if (!TokenMatch(ruleSpec.GetLeftTokenRange(), currentToken.Type())) {
            continue;
        }

        if (!TokenMatch(ruleSpec.GetRightTokenRange(), nextToken.Type())) {
            continue;
        }

        bool predicatesMet = true;
        for (const auto &predicate : ruleSpec.GetRule().GetContext()) {
            if (!predicate(&context)) {
                predicatesMet = false;
                break;
            }
        }
        if (predicatesMet) {
            ExecuteRuleAction(context, changes, ruleSpec.GetRule());
            if ((static_cast<uint16_t>(ruleSpec.GetRule().GetRuleAction()) &
                 (static_cast<uint16_t>(static_cast<uint16_t>(RuleAction::STOP_PROCESSING_SPACE_ACTIONS) |
                                        static_cast<uint16_t>(RuleAction::STOP_PROCESSING_TOKEN_ACTIONS)))) != 0) {
                break;
            }
        }
    }
}

struct RangeFormat {
    size_t start;
    size_t end;
};

static void AdvanceTokens(lexer::Lexer &lex, lexer::Token &prev, lexer::Token &curr, lexer::Token &next)
{
    prev = curr;
    curr = next;
    if (curr.Type() != lexer::TokenType::EOS) {
        lex.NextToken();
        next = lex.GetToken();
    } else {
        next.SetTokenType(lexer::TokenType::EOS);
    }
}

static std::unique_ptr<lexer::Lexer> CreateLexer(parser::ParserContext *parserCtx, util::DiagnosticEngine &diagEngine,
                                                 ScriptExtension extension)
{
    if (extension == ScriptExtension::ETS) {
        return std::make_unique<lexer::ETSLexer>(parserCtx, diagEngine);
    }
    if (extension == ScriptExtension::TS) {
        return std::make_unique<lexer::TSLexer>(parserCtx, diagEngine);
    }
    return std::make_unique<lexer::Lexer>(parserCtx, diagEngine);
}

static std::vector<TextChange> FormatSpan(es2panda_Context *context, public_lib::Context *publicContext,
                                          FormatContext &formatContext, const std::string &sourceText,
                                          const RangeFormat &range)
{
    RulesMap &rulesMap = formatContext.GetRulesMap();
    parser::ParserContext parserCtx(publicContext->parserProgram, parser::ParserStatus::NO_OPTS);
    auto extension = publicContext->parserProgram->Extension();
    auto lex = CreateLexer(&parserCtx, *publicContext->diagnosticEngine, extension);
    FormattingContext fmtCtx(sourceText);
    fmtCtx.SetOptions(formatContext.GetFormatCodeSettings());
    std::vector<TextChange> changes;

    lexer::Token prev;
    lexer::Token curr;
    lexer::Token next;
    prev.SetTokenType(lexer::TokenType::EOS);
    lex->NextToken();
    curr = lex->GetToken();
    if (curr.Type() == lexer::TokenType::EOS) {
        return {};
    }
    lex->NextToken();
    next = lex->GetToken();

    while (curr.Type() != lexer::TokenType::EOS) {
        if (curr.Loc().end.index < range.start) {
            AdvanceTokens(*lex, prev, curr, next);
            continue;
        }
        if (curr.Loc().start.index > range.end) {
            break;
        }

        fmtCtx.SetPreviousToken(prev);
        fmtCtx.SetCurrentToken(curr);
        fmtCtx.SetNextToken(next);
        fmtCtx.SetCurrentTokenParent(GetTouchingToken(context, curr.Loc().start.index, false));
        auto *nextParent =
            next.Type() != lexer::TokenType::EOS ? GetTouchingToken(context, next.Loc().start.index, false) : nullptr;
        fmtCtx.SetNextTokenParent(nextParent);

        ApplyRulesOnRange(fmtCtx, changes, rulesMap);
        AdvanceTokens(*lex, prev, curr, next);
    }
    return changes;
}

std::vector<TextChange> FormatDocument(es2panda_Context *context, FormatContext formatContext)
{
    if (context == nullptr) {
        return {};
    }
    auto *publicContext = reinterpret_cast<public_lib::Context *>(context);
    if (publicContext == nullptr || publicContext->parserProgram == nullptr) {
        return {};
    }
    std::string sourceText(publicContext->parserProgram->SourceCode().Utf8());
    return FormatSpan(context, publicContext, formatContext, sourceText, {0, sourceText.length()});
}

std::vector<TextChange> FormatRange(es2panda_Context *context, FormatContext formatContext, const TextSpan &span)
{
    if (context == nullptr) {
        return {};
    }
    auto *publicContext = reinterpret_cast<public_lib::Context *>(context);
    if (publicContext == nullptr || publicContext->parserProgram == nullptr) {
        return {};
    }

    util::StringView sourceCode = publicContext->parserProgram->SourceCode();
    std::string sourceText(sourceCode.Utf8());
    size_t rangeEnd = span.start + span.length;
    if (rangeEnd > sourceText.length()) {
        return {};
    }

    lexer::LineIndex lineIndex(sourceCode);
    auto [line, col] = lineIndex.GetLocation(span.start);
    size_t rangeStart = lineIndex.GetOffset(lexer::SourceLocation(line, 1, nullptr));

    return FormatSpan(context, publicContext, formatContext, sourceText, {rangeStart, rangeEnd});
}

FormatContext GetFormatContext(FormatCodeSettings &options)
{
    RulesMap &rulesMap = RulesMapCache::Instance().GetRulesMap();
    return FormatContext(options, rulesMap);
}

}  // namespace ark::es2panda::lsp
