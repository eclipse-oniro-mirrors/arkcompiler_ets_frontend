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

#include "formatting/smart_indenter.h"
#include "internal_api.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "ir/astNode.h"
#include "ir/expressions/callExpression.h"
#include "ir/statements/ifStatement.h"
#include "ir/statements/switchStatement.h"
#include "lexer/token/letters.h"
#include "lexer/token/sourceLocation.h"
#include <cctype>

namespace ark::es2panda::lsp {

struct IndentContext {
    const lexer::LineIndex *index;
    const std::string *sourceCode;
    const parser::Program *program;
    const FormatCodeSettings *settings;
    public_lib::Context *context;
};

struct NodeLocationInfo {
    ir::AstNode *node;
    size_t line;
    size_t column;
    size_t position;
};

enum class NextTokenKind { UNKNOWN, OPEN_BRACE, CLOSE_BRACE };

constexpr size_t UNKNOWN_INDENT = static_cast<size_t>(-1);

static std::vector<ir::AstNode *> GetListByPosition(size_t position, ir::AstNode *parent, public_lib::Context *ctx);
static size_t DeriveActualIndentationFromList(const IndentContext &ctx, const std::vector<ir::AstNode *> &list,
                                              size_t index);

static size_t GetBaseIndentation(const FormatCodeSettings &settings)
{
    return settings.GetBaseIndentSize();
}

static size_t GetLineStartPosition(const IndentContext &ctx, size_t line)
{
    return ctx.index->GetOffset(lexer::SourceLocation(line, ONE, ctx.program));
}

static size_t GetLineEndPosition(const IndentContext &ctx, size_t line)
{
    if (ctx.index == nullptr) {
        return 0;
    }
    size_t nextLineStart = ctx.index->GetOffset(lexer::SourceLocation(line + ONE, ONE, ctx.program));
    if (nextLineStart == ZERO || nextLineStart > ctx.sourceCode->length()) {
        return ctx.sourceCode->length();
    }
    return nextLineStart - ONE;
}

static bool IsWhiteSpaceCharacter(char ch)
{
    auto ch32 = static_cast<char32_t>(static_cast<unsigned char>(ch));

    return ch32 == lexer::LEX_CHAR_SP || ch32 == lexer::LEX_CHAR_TAB || ch32 == lexer::LEX_CHAR_VT ||
           ch32 == lexer::LEX_CHAR_FF || ch32 == lexer::LEX_CHAR_NBSP || ch32 == lexer::LEX_CHAR_NEXT_LINE ||
           ch32 == lexer::LEX_CHAR_OGHAM || (ch32 >= lexer::LEX_CHAR_ENQUAD && ch32 <= lexer::LEX_CHAR_ZERO_WIDTH_SP) ||
           ch32 == lexer::LEX_CHAR_NARROW_NO_BREAK_SP || ch32 == lexer::LEX_CHAR_MATHEMATICAL_SP ||
           ch32 == lexer::LEX_CHAR_IDEOGRAPHIC_SP || ch32 == lexer::LEX_CHAR_ZWNBSP;
}

static bool IsStringOrRegularExpressionOrTemplateLiteral(ir::AstNodeType type)
{
    return type == ir::AstNodeType::STRING_LITERAL || type == ir::AstNodeType::TEMPLATE_LITERAL ||
           type == ir::AstNodeType::REGEXP_LITERAL;
}

static size_t FindColumnForFirstNonWhitespaceCharacter(const IndentContext &ctx, size_t lineStart, size_t lineEnd)
{
    size_t column = ZERO;
    size_t tabSize = ctx.settings->GetTabSize();
    if (tabSize == 0) {
        tabSize = ONE;
    }

    for (size_t i = lineStart; i < lineEnd && i < ctx.sourceCode->length(); ++i) {
        char ch = (*ctx.sourceCode)[i];
        if (!IsWhiteSpaceCharacter(ch)) {
            break;
        }

        auto ch32 = static_cast<char32_t>(static_cast<unsigned char>(ch));
        if (ch32 == lexer::LEX_CHAR_TAB) {
            column += tabSize - (column % tabSize);
        } else {
            ++column;
        }
    }
    return column;
}

static size_t GetCommentIndent(const IndentContext &ctx, size_t position, const CommentRange &commentRange)
{
    auto [currentLine, currentCol] = ctx.index->GetLocation(position);
    auto [commentStartLine, commentStartCol] = ctx.index->GetLocation(commentRange.pos_);
    if (currentLine <= commentStartLine) {
        size_t lineStart = GetLineStartPosition(ctx, commentStartLine);
        return FindColumnForFirstNonWhitespaceCharacter(ctx, lineStart, position);
    }

    size_t startPosOfLine = GetLineStartPosition(ctx, currentLine);
    size_t column = FindColumnForFirstNonWhitespaceCharacter(ctx, startPosOfLine, position);
    if (column == ZERO) {
        return ZERO;
    }

    for (size_t i = startPosOfLine; i < ctx.sourceCode->length() && i < position; ++i) {
        if (!IsWhiteSpaceCharacter((*ctx.sourceCode)[i])) {
            if (static_cast<char32_t>(static_cast<unsigned char>((*ctx.sourceCode)[i])) == lexer::LEX_CHAR_ASTERISK) {
                return column > ZERO ? column - ONE : ZERO;
            }
            break;
        }
    }

    return column;
}

static bool PositionBelongsToNode(ir::AstNode *node, size_t position)
{
    if (node == nullptr) {
        return false;
    }

    size_t nodeStart = node->Start().index;
    size_t nodeEnd = node->End().index;

    return position >= nodeStart && position <= nodeEnd;
}

static ir::AstNode *FindNextToken(ir::AstNode *precedingNode, ir::AstNode *parent, public_lib::Context *ctx)
{
    if (precedingNode == nullptr || parent == nullptr || ctx == nullptr || ctx->allocator == nullptr) {
        return nullptr;
    }

    auto children = GetChildren(parent, ctx->allocator);
    if (children.empty()) {
        return nullptr;
    }

    bool foundPreceding = false;
    for (auto *child : children) {
        if (foundPreceding && child != nullptr) {
            return child;
        }
        if (child == precedingNode) {
            foundPreceding = true;
        }
    }

    return nullptr;
}

static NextTokenKind NextTokenIsCurlyBraceOnSameLineAsCursor(ir::AstNode *precedingNode, ir::AstNode *current,
                                                             size_t lineAtPosition, public_lib::Context *ctx)
{
    if (ctx == nullptr) {
        return NextTokenKind::UNKNOWN;
    }

    ir::AstNode *nextToken = FindNextToken(precedingNode, current, ctx);
    if (nextToken == nullptr) {
        return NextTokenKind::UNKNOWN;
    }

    auto nextType = nextToken->Type();
    size_t nextTokenLine = nextToken->Start().line;
    if (nextType == ir::AstNodeType::BLOCK_STATEMENT) {
        if (nextTokenLine == lineAtPosition) {
            const auto &sourceCode = ctx->parserProgram->SourceCode().Mutf8();
            size_t pos = nextToken->Start().index;
            if (pos < sourceCode.length() &&
                static_cast<char32_t>(static_cast<unsigned char>(sourceCode[pos])) == lexer::LEX_CHAR_RIGHT_BRACE) {
                return NextTokenKind::CLOSE_BRACE;
            }
        } else {
            return NextTokenKind::OPEN_BRACE;
        }
    }

    return NextTokenKind::UNKNOWN;
}

static bool IsControlFlowEndingStatement(ir::AstNodeType kind, ir::AstNode *parent)
{
    if (parent == nullptr || parent->Type() == ir::AstNodeType::BLOCK_STATEMENT) {
        return false;
    }

    switch (kind) {
        case ir::AstNodeType::RETURN_STATEMENT:
        case ir::AstNodeType::THROW_STATEMENT:
        case ir::AstNodeType::CONTINUE_STATEMENT:
        case ir::AstNodeType::BREAK_STATEMENT:
            return true;
        default:
            return false;
    }
}

static bool RangeIsOnOneLine(const IndentContext &ctx, ir::AstNode *node)
{
    if (node == nullptr || ctx.index == nullptr) {
        return false;
    }

    auto [startLine, startCol] = ctx.index->GetLocation(node->Start().index);
    auto [endLine, endCol] = ctx.index->GetLocation(node->End().index);

    return startLine == endLine;
}

static bool IsAlwaysIndentingParent(ir::AstNodeType parentType)
{
    switch (parentType) {
        case ir::AstNodeType::EXPRESSION_STATEMENT:
        case ir::AstNodeType::CLASS_DEFINITION:
        case ir::AstNodeType::CLASS_DECLARATION:
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
        case ir::AstNodeType::TS_ENUM_DECLARATION:
        case ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION:
        case ir::AstNodeType::ARRAY_EXPRESSION:
        case ir::AstNodeType::BLOCK_STATEMENT:
        case ir::AstNodeType::OBJECT_EXPRESSION:
        case ir::AstNodeType::CALL_EXPRESSION:
        case ir::AstNodeType::NEW_EXPRESSION:
        case ir::AstNodeType::RETURN_STATEMENT:
        case ir::AstNodeType::CONDITIONAL_EXPRESSION:
        case ir::AstNodeType::MEMBER_EXPRESSION:
        case ir::AstNodeType::TAGGED_TEMPLATE_EXPRESSION:
        case ir::AstNodeType::AWAIT_EXPRESSION:
        case ir::AstNodeType::CLASS_PROPERTY:
        case ir::AstNodeType::SWITCH_CASE_STATEMENT:
        case ir::AstNodeType::ETS_IMPORT_DECLARATION:
        case ir::AstNodeType::IMPORT_DECLARATION:
        case ir::AstNodeType::SWITCH_STATEMENT:
            return true;
        default:
            return false;
    }
}

static bool ShouldIndentForObjectLiteral(const IndentContext &ctx, const FormatCodeSettings &settings,
                                         ir::AstNode *child)
{
    if (!settings.GetIndentMultiLineObjectLiteralBeginningOnBlankLine() && child != nullptr &&
        child->Type() == ir::AstNodeType::OBJECT_EXPRESSION) {
        return RangeIsOnOneLine(ctx, child);
    }
    return true;
}

static bool ShouldIndentForBinaryExpression(const IndentContext &ctx, ir::AstNode *parent, ir::AstNode *child)
{
    if (child != nullptr) {
        auto [parentLine, parentCol] = ctx.index->GetLocation(parent->Start().index);
        auto [childLine, childCol] = ctx.index->GetLocation(child->Start().index);
        if (parentLine != childLine) {
            return true;
        }
    }
    return false;
}

static bool NodeWillIndentChild(const IndentContext &ctx, const FormatCodeSettings &settings, ir::AstNode *parent,
                                ir::AstNode *child, bool indentByDefault)
{
    if (parent == nullptr) {
        return false;
    }
    auto parentType = parent->Type();
    if (IsAlwaysIndentingParent(parentType)) {
        return true;
    }
    switch (parentType) {
        case ir::AstNodeType::VARIABLE_DECLARATION:
        case ir::AstNodeType::PROPERTY:
        case ir::AstNodeType::ASSIGNMENT_EXPRESSION:
        case ir::AstNodeType::BINARY_EXPRESSION:
            if (!ShouldIndentForObjectLiteral(ctx, settings, child)) {
                return false;
            }
            if (parentType == ir::AstNodeType::BINARY_EXPRESSION &&
                ShouldIndentForBinaryExpression(ctx, parent, child)) {
                return true;
            }
            if (parentType != ir::AstNodeType::BINARY_EXPRESSION) {
                return true;
            }
            break;
        case ir::AstNodeType::DO_WHILE_STATEMENT:
        case ir::AstNodeType::WHILE_STATEMENT:
        case ir::AstNodeType::FOR_IN_STATEMENT:
        case ir::AstNodeType::FOR_OF_STATEMENT:
        case ir::AstNodeType::FOR_UPDATE_STATEMENT:
        case ir::AstNodeType::IF_STATEMENT:
        case ir::AstNodeType::FUNCTION_DECLARATION:
        case ir::AstNodeType::FUNCTION_EXPRESSION:
        case ir::AstNodeType::METHOD_DEFINITION:
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
        case ir::AstNodeType::TRY_STATEMENT:
            return child == nullptr || child->Type() != ir::AstNodeType::BLOCK_STATEMENT;
        default:
            return indentByDefault;
    }
    return indentByDefault;
}

static bool ShouldIndentChildNode(const IndentContext &ctx, const FormatCodeSettings &settings, ir::AstNode *parent,
                                  ir::AstNode *child, bool isNextChild)
{
    if (parent == nullptr) {
        return false;
    }

    bool willIndent = NodeWillIndentChild(ctx, settings, parent, child, false);
    if (!willIndent) {
        return false;
    }

    if (isNextChild && child != nullptr) {
        auto childType = child->Type();
        if (IsControlFlowEndingStatement(childType, parent)) {
            return false;
        }
    }

    return true;
}

static std::vector<ir::AstNode *> GetContainingList(ir::AstNode *node, public_lib::Context *ctx)
{
    if (node == nullptr || node->Parent() == nullptr) {
        return {};
    }

    auto *parent = node->Parent();
    if (ctx == nullptr || ctx->allocator == nullptr) {
        return {};
    }

    return GetListByPosition(node->Start().index, parent, ctx);
}

static std::vector<ir::AstNode *> GetListByPosition(size_t position, ir::AstNode *parent, public_lib::Context *ctx)
{
    if (parent == nullptr) {
        return {};
    }

    if (ctx == nullptr || ctx->allocator == nullptr) {
        return {};
    }

    auto children = GetChildren(parent, ctx->allocator);
    std::vector<ir::AstNode *> result;

    for (auto *child : children) {
        if (child != nullptr && child->Start().index <= position && position <= child->End().index) {
            result.push_back(child);
        }
    }

    return result;
}

static size_t GetIndentationForNode(const IndentContext &ctx, ir::AstNode *node)
{
    if (node == nullptr) {
        return GetBaseIndentation(*ctx.settings);
    }

    size_t nodeStart = node->Start().index;
    auto [line, col] = ctx.index->GetLocation(nodeStart);
    size_t lineStart = GetLineStartPosition(ctx, line);
    size_t lineEnd = GetLineEndPosition(ctx, line);

    return FindColumnForFirstNonWhitespaceCharacter(ctx, lineStart, lineEnd);
}

static size_t GetBlockIndent(const IndentContext &ctx, size_t position)
{
    size_t current = position;

    while (current > ZERO) {
        char ch = (*ctx.sourceCode)[current];
        if (!IsWhiteSpaceCharacter(ch)) {
            break;
        }
        --current;
    }

    auto [line, col] = ctx.index->GetLocation(current);
    size_t lineStart = GetLineStartPosition(ctx, line);
    return FindColumnForFirstNonWhitespaceCharacter(ctx, lineStart, current);
}

static size_t GetActualIndentationForListStartLine(const IndentContext &ctx, const std::vector<ir::AstNode *> &list)
{
    if (list.empty()) {
        return UNKNOWN_INDENT;
    }

    return GetIndentationForNode(ctx, list[0]);
}

static size_t GetActualIndentationForListItem(const IndentContext &ctx, ir::AstNode *node)
{
    if (node == nullptr) {
        return UNKNOWN_INDENT;
    }

    auto list = GetContainingList(node, ctx.context);
    if (list.empty()) {
        return UNKNOWN_INDENT;
    }

    for (size_t i = ZERO; i < list.size(); ++i) {
        if (list[i] == node && i > ZERO) {
            return DeriveActualIndentationFromList(ctx, list, i - ONE);
        }
    }

    return UNKNOWN_INDENT;
}

static size_t GetActualIndentationForListItemBeforeComma(const IndentContext &ctx, ir::AstNode *commaNode)
{
    if (commaNode == nullptr || commaNode->Parent() == nullptr) {
        return UNKNOWN_INDENT;
    }

    auto *parent = commaNode->Parent();
    if (ctx.context == nullptr || ctx.context->allocator == nullptr) {
        return UNKNOWN_INDENT;
    }

    auto children = GetChildren(parent, ctx.context->allocator);
    for (size_t i = ZERO; i < children.size(); ++i) {
        if (children[i] == commaNode && i > ZERO) {
            return DeriveActualIndentationFromList(ctx, children, i - ONE);
        }
    }

    return UNKNOWN_INDENT;
}

static size_t DeriveActualIndentationFromList(const IndentContext &ctx, const std::vector<ir::AstNode *> &list,
                                              size_t index)
{
    if (index >= list.size()) {
        return UNKNOWN_INDENT;
    }

    auto *node = list[index];
    if (node == nullptr) {
        return UNKNOWN_INDENT;
    }

    return GetIndentationForNode(ctx, node);
}

static std::pair<size_t, size_t> GetContainingListOrParentStart(const IndentContext &ctx, ir::AstNode *parent,
                                                                ir::AstNode *child)
{
    if (parent == nullptr || ctx.context == nullptr) {
        return {0, 0};
    }

    size_t targetStart = parent->Start().index;
    if (child != nullptr) {
        auto list = GetContainingList(child, ctx.context);
        if (!list.empty()) {
            ir::AstNode *listParent = child->Parent();
            if (listParent != nullptr) {
                targetStart = listParent->Start().index;
            } else {
                targetStart = 0;
            }
        }
    }

    auto [line, col] = ctx.index->GetLocation(targetStart);
    size_t lineStart = GetLineStartPosition(ctx, line);
    size_t column = FindColumnForFirstNonWhitespaceCharacter(ctx, lineStart, targetStart);
    return {line, column};
}

static bool ChildStartsOnTheSameLineWithElseInIfStatement(ir::AstNode *parent, ir::AstNode *child,
                                                          size_t childStartLine, public_lib::Context *ctx)
{
    if (parent == nullptr || child == nullptr || parent->Type() != ir::AstNodeType::IF_STATEMENT) {
        return false;
    }

    auto *ifStmt = reinterpret_cast<ir::IfStatement *>(parent);
    if (ifStmt->Alternate() != child) {
        return false;
    }

    if (ctx == nullptr || ctx->parserProgram == nullptr) {
        return false;
    }

    const auto &sourceCode = ctx->parserProgram->SourceCode().Mutf8();
    size_t consequentEnd = ifStmt->Consequent()->End().index;

    lexer::LineIndex lineIndex(ctx->parserProgram->SourceCode());

    constexpr std::string_view ELSE_KEYWORD = "else";
    for (size_t i = consequentEnd; i < sourceCode.length() && i < child->Start().index; ++i) {
        if (sourceCode.substr(i, ELSE_KEYWORD.length()) == ELSE_KEYWORD) {
            auto [elseLine, elseCol] = lineIndex.GetLocation(i);
            return elseLine == childStartLine;
        }
    }

    return false;
}

static bool IsArgumentAndStartLineOverlapsExpressionBeingCalled(ir::AstNode *parent, ir::AstNode *child,
                                                                size_t childStartLine, public_lib::Context *ctx)
{
    if (parent == nullptr || child == nullptr || parent->Type() != ir::AstNodeType::CALL_EXPRESSION) {
        return false;
    }

    auto *callExpr = reinterpret_cast<ir::CallExpression *>(parent);
    auto &args = callExpr->Arguments();

    bool isArgument = false;
    for (auto *arg : args) {
        if (arg == child) {
            isArgument = true;
            break;
        }
    }

    if (!isArgument) {
        return false;
    }

    if (ctx == nullptr || ctx->parserProgram == nullptr) {
        return false;
    }

    lexer::LineIndex lineIndex(ctx->parserProgram->SourceCode());
    size_t calleeEnd = callExpr->Callee()->End().index;
    auto [calleeEndLine, calleeEndCol] = lineIndex.GetLocation(calleeEnd);

    return calleeEndLine == childStartLine;
}

static size_t GetActualIndentationForNode(const IndentContext &ctx, const NodeLocationInfo &currentInfo,
                                          ir::AstNode *parent, bool parentAndChildShareLine)
{
    if (currentInfo.node == nullptr || parent == nullptr) {
        return UNKNOWN_INDENT;
    }

    bool isDeclarationOrStatement = currentInfo.node->IsStatement();
    if (!isDeclarationOrStatement) {
        return UNKNOWN_INDENT;
    }

    bool isParentSourceFile = (parent->Parent() == nullptr);
    bool useActualIndentation = isParentSourceFile || !parentAndChildShareLine;
    if (!useActualIndentation) {
        return UNKNOWN_INDENT;
    }

    auto [line, col] = ctx.index->GetLocation(currentInfo.node->Start().index);
    size_t lineStart = GetLineStartPosition(ctx, line);
    size_t lineEnd = currentInfo.node->Start().index;

    return FindColumnForFirstNonWhitespaceCharacter(ctx, lineStart, lineEnd);
}

static size_t TryGetActualIndentation(const IndentContext &ctx, const NodeLocationInfo &currentInfo,
                                      ir::AstNode *parent, bool parentAndChildShareLine, size_t indentationDelta)
{
    size_t actualIndentation = GetActualIndentationForListItem(ctx, currentInfo.node);
    if (actualIndentation != UNKNOWN_INDENT) {
        return actualIndentation + indentationDelta;
    }

    actualIndentation = GetActualIndentationForNode(ctx, currentInfo, parent, parentAndChildShareLine);
    if (actualIndentation != UNKNOWN_INDENT) {
        return actualIndentation + indentationDelta;
    }

    return UNKNOWN_INDENT;
}

static std::pair<size_t, size_t> UpdateCurrentLocation(const IndentContext &ctx, ir::AstNode *current,
                                                       bool useTrueStart,
                                                       std::pair<size_t, size_t> containingListOrParent)
{
    if (useTrueStart) {
        auto [line, col] = ctx.index->GetLocation(current->Start().index);
        size_t lineStart = GetLineStartPosition(ctx, line);
        size_t column = FindColumnForFirstNonWhitespaceCharacter(ctx, lineStart, current->Start().index);
        return {line, column};
    }
    return containingListOrParent;
}

static size_t GetIndentationForNodeWorker(const IndentContext &ctx, const NodeLocationInfo &nodeInfo,
                                          size_t indentationDelta, bool isNextChild)
{
    ir::AstNode *current = nodeInfo.node;
    size_t currentLine = nodeInfo.line;
    size_t currentColumn = nodeInfo.column;
    ir::AstNode *parent = current->Parent();

    while (parent != nullptr) {
        auto [containingListOrParentLine, containingListOrParentColumn] =
            GetContainingListOrParentStart(ctx, parent, current);

        bool parentAndChildShareLine =
            (containingListOrParentLine == currentLine) ||
            ChildStartsOnTheSameLineWithElseInIfStatement(parent, current, currentLine, ctx.context);

        NodeLocationInfo currentInfo {current, currentLine, currentColumn, nodeInfo.position};
        size_t result = TryGetActualIndentation(ctx, currentInfo, parent, parentAndChildShareLine, indentationDelta);
        if (result != UNKNOWN_INDENT) {
            return result;
        }

        if (ShouldIndentChildNode(ctx, *ctx.settings, parent, current, isNextChild) && !parentAndChildShareLine) {
            indentationDelta += ctx.settings->GetIndentSize();
        }

        bool useTrueStart =
            IsArgumentAndStartLineOverlapsExpressionBeingCalled(parent, current, currentLine, ctx.context);

        current = parent;
        parent = current->Parent();

        std::pair<size_t, size_t> containingListOrParent {containingListOrParentLine, containingListOrParentColumn};
        auto [line, column] = UpdateCurrentLocation(ctx, current, useTrueStart, containingListOrParent);
        currentLine = line;
        currentColumn = column;
    }

    return indentationDelta + GetBaseIndentation(*ctx.settings);
}

static size_t CalculateIndentationDelta(const IndentContext &ctx, const NodeLocationInfo &nodeInfo,
                                        ir::AstNode *current, size_t currentLine, bool assumeNewLineBeforeCloseBrace)
{
    NextTokenKind nextTokenKind =
        NextTokenIsCurlyBraceOnSameLineAsCursor(nodeInfo.node, current, nodeInfo.line, ctx.context);
    if (nextTokenKind != NextTokenKind::UNKNOWN) {
        if (assumeNewLineBeforeCloseBrace && nextTokenKind == NextTokenKind::CLOSE_BRACE) {
            return ctx.settings->GetIndentSize();
        }
        return ZERO;
    }

    if (nodeInfo.line != currentLine) {
        return ctx.settings->GetIndentSize();
    }
    return ZERO;
}

static size_t GetSmartIndent(const IndentContext &ctx, const NodeLocationInfo &nodeInfo,
                             bool assumeNewLineBeforeCloseBrace)
{
    if (nodeInfo.node == nullptr) {
        return GetBaseIndentation(*ctx.settings);
    }

    if (ctx.context == nullptr || ctx.context->parserProgram == nullptr) {
        return GetBaseIndentation(*ctx.settings);
    }

    ir::AstNode *previous = nullptr;
    ir::AstNode *current = nodeInfo.node;

    while (current != nullptr) {
        if (PositionBelongsToNode(current, nodeInfo.position) &&
            ShouldIndentChildNode(ctx, *ctx.settings, current, previous, true)) {
            auto [currentLine, currentCol] = ctx.index->GetLocation(current->Start().index);
            size_t lineStart = GetLineStartPosition(ctx, currentLine);
            size_t currentColumn = FindColumnForFirstNonWhitespaceCharacter(ctx, lineStart, current->Start().index);

            size_t indentationDelta =
                CalculateIndentationDelta(ctx, nodeInfo, current, currentLine, assumeNewLineBeforeCloseBrace);

            NodeLocationInfo currentInfo {current, currentLine, currentColumn, nodeInfo.position};
            return GetIndentationForNodeWorker(ctx, currentInfo, indentationDelta, true);
        }

        size_t actualIndentation = GetActualIndentationForListItem(ctx, current);
        if (actualIndentation != UNKNOWN_INDENT) {
            return actualIndentation;
        }

        previous = current;
        current = current->Parent();
    }

    return GetBaseIndentation(*ctx.settings);
}

static bool CheckCommentIndent(es2panda_Context *context, const IndentContext &indentCtx, size_t position,
                               IndentationResult &result)
{
    CommentRange commentRange;
    GetRangeOfEnclosingComment(context, position, &commentRange);
    if (commentRange.pos_ != ZERO || commentRange.end_ != ZERO) {
        if (commentRange.kind_ == CommentKind::MULTI_LINE) {
            result.indentation = GetCommentIndent(indentCtx, position, commentRange);
            result.isValid = true;
            return true;
        }
    }
    return false;
}

static bool CheckStringOrLiteralIndent(ir::AstNode *precedingNode, size_t position, IndentationResult &result)
{
    auto precedingType = precedingNode->Type();
    if (IsStringOrRegularExpressionOrTemplateLiteral(precedingType)) {
        size_t precedingStart = precedingNode->Start().index;
        size_t precedingEnd = precedingNode->End().index;
        if (position > precedingStart && position < precedingEnd) {
            result.indentation = ZERO;
            result.isValid = false;
            return true;
        }
    }
    return false;
}

static bool CheckObjectLiteral(ir::AstNode *currentNode, const std::string &sourceCode)
{
    if (currentNode != nullptr && currentNode->Parent() != nullptr) {
        if (currentNode->Parent()->Type() == ir::AstNodeType::OBJECT_EXPRESSION) {
            size_t currentStart = currentNode->Start().index;
            if (currentStart < sourceCode.length() && static_cast<char32_t>(static_cast<unsigned char>(
                                                          sourceCode[currentStart])) == lexer::LEX_CHAR_LEFT_BRACE) {
                return true;
            }
        }
    }
    return false;
}

static bool CheckCommaIndent(const IndentContext &indentCtx, ir::AstNode *precedingNode, const std::string &sourceCode,
                             IndentationResult &result)
{
    size_t precedingEnd = precedingNode->End().index;
    if (precedingEnd < sourceCode.length() &&
        static_cast<char32_t>(static_cast<unsigned char>(sourceCode[precedingEnd])) == lexer::LEX_CHAR_COMMA) {
        if (precedingNode->Parent() == nullptr ||
            precedingNode->Parent()->Type() != ir::AstNodeType::BINARY_EXPRESSION) {
            size_t actualIndentation = GetActualIndentationForListItemBeforeComma(indentCtx, precedingNode);
            if (actualIndentation != UNKNOWN_INDENT) {
                result.indentation = actualIndentation;
                result.isValid = true;
                return true;
            }
        }
    }
    return false;
}

static bool CheckListIndent(const IndentContext &indentCtx, ir::AstNode *precedingNode, ir::AstNode *currentNode,
                            size_t position, IndentationResult &result)
{
    if (precedingNode->Parent() == nullptr) {
        return false;
    }

    auto list = GetListByPosition(position, precedingNode->Parent(), indentCtx.context);
    if (list.empty()) {
        return false;
    }

    bool rangeContainsPreceding = false;
    size_t listStart = list[0]->Start().index;
    size_t listEnd = list[list.size() - 1]->End().index;
    size_t precedingStart = precedingNode->Start().index;
    size_t precedingEndPos = precedingNode->End().index;
    if (listStart <= precedingStart && precedingEndPos <= listEnd) {
        rangeContainsPreceding = true;
    }

    if (!rangeContainsPreceding) {
        bool useTheSameBaseIndentation = false;
        if (currentNode != nullptr && currentNode->Parent() != nullptr) {
            auto parentType = currentNode->Parent()->Type();
            useTheSameBaseIndentation = (parentType == ir::AstNodeType::FUNCTION_EXPRESSION ||
                                         parentType == ir::AstNodeType::ARROW_FUNCTION_EXPRESSION);
        }

        size_t indentSize = useTheSameBaseIndentation ? 0 : indentCtx.settings->GetIndentSize();
        size_t listStartIndent = GetActualIndentationForListStartLine(indentCtx, list);
        result.indentation = listStartIndent + indentSize;
        result.isValid = true;
        return true;
    }

    return false;
}

static public_lib::Context *ValidateContextAndPosition(es2panda_Context *context, size_t position,
                                                       const FormatCodeSettings &settings, IndentationResult &result)
{
    if (context == nullptr) {
        return nullptr;
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx == nullptr || ctx->parserProgram == nullptr) {
        return nullptr;
    }

    const auto &sourceCode = ctx->parserProgram->SourceCode().Mutf8();
    if (position >= sourceCode.length()) {
        result.indentation = GetBaseIndentation(settings);
        result.isValid = true;
        return nullptr;
    }

    if (settings.GetIndentStyle() == IndentStyle::NONE) {
        result.indentation = ZERO;
        result.isValid = true;
        return nullptr;
    }

    return ctx;
}

static bool CheckPrecedingNull(const IndentContext &indentCtx, ir::AstNode *precedingNode, IndentationResult &result)
{
    if (precedingNode == nullptr) {
        result.indentation = GetBaseIndentation(*indentCtx.settings);
        result.isValid = true;
        return true;
    }
    return false;
}

static bool ProcessSpecialCases(const IndentContext &indentCtx, ir::AstNode *precedingNode, ir::AstNode *currentNode,
                                size_t position, IndentationResult &result)
{
    auto *context = reinterpret_cast<es2panda_Context *>(indentCtx.context);
    if (CheckCommentIndent(context, indentCtx, position, result)) {
        return true;
    }

    if (CheckPrecedingNull(indentCtx, precedingNode, result)) {
        return true;
    }

    if (CheckStringOrLiteralIndent(precedingNode, position, result)) {
        return true;
    }

    bool isObjectLiteral = CheckObjectLiteral(currentNode, *indentCtx.sourceCode);
    if (indentCtx.settings->GetIndentStyle() == IndentStyle::BLOCK || isObjectLiteral) {
        result.indentation = GetBlockIndent(indentCtx, position);
        result.isValid = true;
        return true;
    }

    return CheckCommaIndent(indentCtx, precedingNode, *indentCtx.sourceCode, result);
}

IndentationResult GetIndentation(es2panda_Context *context, size_t position, const FormatCodeSettings &settings,
                                 bool assumeNewLineBeforeCloseBrace)
{
    IndentationResult result;
    result.isValid = false;
    result.indentation = ZERO;

    auto *ctx = ValidateContextAndPosition(context, position, settings, result);
    if (ctx == nullptr) {
        return result;
    }

    const auto &sourceCode = ctx->parserProgram->SourceCode().Mutf8();
    lexer::LineIndex index(ctx->parserProgram->SourceCode());
    IndentContext indentCtx {&index, &sourceCode, ctx->parserProgram, &settings, ctx};

    auto *ast = ctx->parserProgram->Ast();
    ir::AstNode *precedingNode = FindPrecedingToken(position, ast, ctx->allocator);
    size_t lineAtPosition = (precedingNode != nullptr) ? precedingNode->Start().line : ZERO;

    ir::AstNode *currentNode = GetTouchingToken(context, position, false);
    if (ProcessSpecialCases(indentCtx, precedingNode, currentNode, position, result)) {
        return result;
    }

    if (CheckListIndent(indentCtx, precedingNode, currentNode, position, result)) {
        return result;
    }

    NodeLocationInfo nodeInfo {precedingNode, lineAtPosition, ZERO, position};
    size_t smartIndent = GetSmartIndent(indentCtx, nodeInfo, assumeNewLineBeforeCloseBrace);
    if (smartIndent == UNKNOWN_INDENT) {
        result.indentation = GetBlockIndent(indentCtx, position);
        result.isValid = true;
        return result;
    }

    result.indentation = smartIndent;
    result.isValid = true;
    return result;
}

}  // namespace ark::es2panda::lsp
