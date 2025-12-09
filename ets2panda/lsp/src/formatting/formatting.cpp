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
#include "lexer/token/token.h"

namespace ark::es2panda::lsp {

// NOLINTNEXTLINE
bool TokenMatch(TokenRange &tokenRange, lexer::TokenType tokenType)
{
    auto &tokens = tokenRange.GetTokens();
    return tokens.empty() || std::find(tokens.begin(), tokens.end(), tokenType) != tokens.end();
}

void ApplyInsertSpace(RuleAction action, const TextSpan &span, std::vector<TextChange> &changes)
{
    if ((static_cast<uint16_t>(RuleAction::INSERT_SPACE) & static_cast<uint16_t>(action)) != 0) {
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

void ApplyInsertNewline(RuleAction action, const TextSpan &span, std::vector<TextChange> &changes)
{
    if ((static_cast<uint16_t>(RuleAction::INSERT_NEWLINE) & static_cast<uint16_t>(action)) != 0) {
        changes.emplace_back(TextChange {span, "\n"});
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
    const auto &prevToken = context.GetPreviousToken();
    const auto &currentToken = context.GetCurrentToken();
    auto action = rule.GetRuleAction();

    auto prevLoc = prevToken.Loc();
    auto currLoc = currentToken.Loc();

    size_t start = prevLoc.end.index;
    size_t end = currLoc.start.index;

    if (start <= end) {
        TextSpan whitespaceSpan {start, end - start};
        ApplyInsertSpace(action, whitespaceSpan, changes);
        ApplyDeleteSpace(action, whitespaceSpan, changes);
        ApplyInsertNewline(action, whitespaceSpan, changes);
    }

    ApplyDeleteToken(action, currLoc, changes);
    ApplyInsertSemicolon(action, currLoc, changes);
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

std::vector<TextChange> FormatDocument(es2panda_Context *context, FormatContext formatContext)
{
    if (context == nullptr) {
        return {};
    }

    auto *publicContext = reinterpret_cast<public_lib::Context *>(context);
    if (publicContext == nullptr || publicContext->parserProgram == nullptr) {
        return {};
    }

    RulesMap &rulesMap = formatContext.GetRulesMap();
    [[maybe_unused]] const FormatCodeSettings &options = formatContext.GetFormatCodeSettings();

    parser::ParserContext parserCtx(publicContext->parserProgram, parser::ParserStatus::NO_OPTS);
    lexer::Lexer lexer(&parserCtx, *publicContext->diagnosticEngine);

    std::string sourceText(publicContext->parserProgram->SourceCode().Utf8());

    FormattingContext formattingContext(sourceText);
    std::vector<TextChange> changes;

    lexer::Token prevToken;
    prevToken.SetTokenType(lexer::TokenType::EOS);

    lexer.NextToken();
    lexer::Token currentToken = lexer.GetToken();
    if (currentToken.Type() == lexer::TokenType::EOS) {
        return {};
    }
    lexer.NextToken();
    lexer::Token nextToken = lexer.GetToken();

    while (currentToken.Type() != lexer::TokenType::EOS) {
        formattingContext.SetPreviousToken(prevToken);
        formattingContext.SetCurrentToken(currentToken);
        formattingContext.SetNextToken(nextToken);

        auto *currentTokenParent = GetTouchingToken(context, currentToken.Loc().start.index, false);
        formattingContext.SetCurrentTokenParent(currentTokenParent);

        ir::AstNode *nextTokenParent = nullptr;
        if (nextToken.Type() != lexer::TokenType::EOS) {
            nextTokenParent = GetTouchingToken(context, nextToken.Loc().start.index, false);
        }
        formattingContext.SetNextTokenParent(nextTokenParent);

        ApplyRulesOnRange(formattingContext, changes, rulesMap);

        prevToken = currentToken;
        currentToken = nextToken;

        if (currentToken.Type() != lexer::TokenType::EOS) {
            lexer.NextToken();
            nextToken = lexer.GetToken();
        } else {
            nextToken.SetTokenType(lexer::TokenType::EOS);
        }
    }
    return changes;
}

FormatContext GetFormatContext(FormatCodeSettings &options)
{
    RulesMap &rulesMap = RulesMapCache::Instance().GetRulesMap();
    return FormatContext(options, rulesMap);
}

}  // namespace ark::es2panda::lsp