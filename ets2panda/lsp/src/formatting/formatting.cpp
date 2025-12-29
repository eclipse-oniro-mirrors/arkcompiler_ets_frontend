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
#include <algorithm>
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

struct FormatInputs {
    es2panda_Context *ctx;
    public_lib::Context *publicCtx;
    FormatContext &formatCtx;
    const std::string &sourceText;
    ir::AstNode *ast;
    ArenaAllocator *allocator;
};

enum class KeystrokeType {
    OPEN_BRACE,
    CLOSE_BRACE,
    SEMICOLON,
    NEWLINE,
    OTHER,
};

static KeystrokeType ClassifyKeystroke(char key)
{
    switch (key) {
        case '{':
            return KeystrokeType::OPEN_BRACE;
        case '}':
            return KeystrokeType::CLOSE_BRACE;
        case ';':
            return KeystrokeType::SEMICOLON;
        case '\n':
            return KeystrokeType::NEWLINE;
        default:
            return KeystrokeType::OTHER;
    }
}

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

static bool IsCommentPosition(es2panda_Context *context, size_t position)
{
    CommentRange commentRange {};
    commentRange.pos_ = 0;
    commentRange.end_ = 0;
    commentRange.kind_ = CommentKind::SINGLE_LINE;
    GetRangeOfEnclosingComment(context, position, &commentRange);
    return commentRange.end_ != 0 || commentRange.pos_ != 0;
}

static bool IsListElement(ir::AstNode *parent, ir::AstNode *node, ArenaAllocator *allocator)
{
    if (parent == nullptr || node == nullptr) {
        return false;
    }
    auto children = GetChildren(parent, allocator);
    if (children.size() <= 1) {
        return false;
    }
    return std::find(children.begin(), children.end(), node) != children.end();
}

static ir::AstNode *FindOutermostNodeWithinListLevel(ir::AstNode *node, ArenaAllocator *allocator)
{
    auto *current = node;
    while (current != nullptr) {
        auto *parent = current->Parent();
        if (parent == nullptr) {
            break;
        }
        if (parent->End().index != current->End().index) {
            break;
        }
        if (IsListElement(parent, current, allocator)) {
            break;
        }
        current = parent;
    }
    return current;
}

static std::vector<TextChange> FormatNodeLines(const FormatInputs &inputs, ir::AstNode *node, size_t endExclusive)
{
    if (node == nullptr) {
        return {};
    }

    util::StringView sourceCode = inputs.publicCtx->parserProgram->SourceCode();
    lexer::LineIndex lineIndex(sourceCode);
    size_t startLineOffset = lineIndex.GetOffset(lexer::SourceLocation(node->Start().line, 1, nullptr));
    if (startLineOffset > endExclusive) {
        return {};
    }
    return FormatSpan(inputs.ctx, inputs.publicCtx, inputs.formatCtx, inputs.sourceText,
                      {startLineOffset, endExclusive});
}

static std::vector<TextChange> FormatAfterOpenBrace(const FormatInputs &inputs, size_t position)
{
    auto *precedingToken = FindPrecedingToken(position, inputs.ast, inputs.allocator);
    if (precedingToken == nullptr && position > 0) {
        precedingToken = FindPrecedingToken(position - 1, inputs.ast, inputs.allocator);
    }
    auto *outermostNode = FindOutermostNodeWithinListLevel(precedingToken, inputs.allocator);
    return FormatNodeLines(inputs, outermostNode, position);
}

static std::vector<TextChange> FormatAfterCloseBraceOrSemicolon(const FormatInputs &inputs, size_t position)
{
    size_t searchPos = position > 0 ? position - 1 : position;
    auto *precedingToken = FindPrecedingToken(searchPos, inputs.ast, inputs.allocator);
    if (precedingToken == nullptr) {
        precedingToken = GetTouchingToken(inputs.ctx, searchPos, false);
    }
    if (precedingToken == nullptr) {
        return {};
    }
    auto *outermostNode = FindOutermostNodeWithinListLevel(precedingToken, inputs.allocator);
    if (outermostNode == nullptr) {
        return {};
    }
    size_t endExclusive = std::min(inputs.sourceText.length(), outermostNode->End().index + 1);
    return FormatNodeLines(inputs, outermostNode, endExclusive);
}

static std::vector<TextChange> FormatAfterNewline(const FormatInputs &inputs, lexer::LineIndex &lineIndex,
                                                  size_t position)
{
    auto [line, col] = lineIndex.GetLocation(position);
    if (line <= 1) {
        return {};
    }

    size_t startCurrentLine = lineIndex.GetOffset(lexer::SourceLocation(line, 1, nullptr));
    size_t startPrevLine = lineIndex.GetOffset(lexer::SourceLocation(line - 1, 1, nullptr));
    size_t startNextLine = inputs.sourceText.length();
    if (position < inputs.sourceText.length()) {
        startNextLine = std::min(startNextLine, lineIndex.GetOffset(lexer::SourceLocation(line + 1, 1, nullptr)));
    }
    size_t endOfFormatSpan = startNextLine > 0 ? startNextLine - 1 : inputs.sourceText.length();
    while (endOfFormatSpan > startCurrentLine &&
           (inputs.sourceText[endOfFormatSpan] == '\n' || inputs.sourceText[endOfFormatSpan] == '\r')) {
        endOfFormatSpan--;
    }
    while (endOfFormatSpan > startCurrentLine &&
           (inputs.sourceText[endOfFormatSpan] == ' ' || inputs.sourceText[endOfFormatSpan] == '\t')) {
        endOfFormatSpan--;
    }
    size_t endExclusive = endOfFormatSpan + 1;
    return FormatSpan(inputs.ctx, inputs.publicCtx, inputs.formatCtx, inputs.sourceText, {startPrevLine, endExclusive});
}

std::vector<TextChange> FormatAfterKeystroke(es2panda_Context *context, FormatContext formatContext, char key,
                                             const TextSpan &span)
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
    auto *ast = publicContext->parserProgram->Ast();
    auto *allocator = publicContext->allocator;
    FormatInputs inputs {context, publicContext, formatContext, sourceText, ast, allocator};

    if (span.start > sourceText.length()) {
        return {};
    }

    size_t position = span.start;
    if (IsCommentPosition(context, position)) {
        return {};
    }

    lexer::LineIndex lineIndex(sourceCode);
    switch (ClassifyKeystroke(key)) {
        case KeystrokeType::OPEN_BRACE:
            return FormatAfterOpenBrace(inputs, position);
        case KeystrokeType::CLOSE_BRACE:
        case KeystrokeType::SEMICOLON:
            return FormatAfterCloseBraceOrSemicolon(inputs, position);
        case KeystrokeType::NEWLINE:
            return FormatAfterNewline(inputs, lineIndex, position);
        default:
            return {};
    }
}

FormatContext GetFormatContext(FormatCodeSettings &options)
{
    RulesMap &rulesMap = RulesMapCache::Instance().GetRulesMap();
    return FormatContext(options, rulesMap);
}

}  // namespace ark::es2panda::lsp
