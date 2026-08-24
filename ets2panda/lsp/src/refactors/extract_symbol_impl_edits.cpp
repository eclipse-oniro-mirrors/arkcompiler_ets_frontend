/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "checker/ETSchecker.h"
#include "refactors/extract_symbol.h"
#include "refactors/extract_symbol_internal.h"
#include "ir/astNode.h"
#include "ir/base/scriptFunction.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/memberExpression.h"
#include "ir/expressions/updateExpression.h"
#include "ir/ets/etsParameterExpression.h"
#include "ir/ets/etsModule.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/expressionStatement.h"
#include "ir/statements/functionDeclaration.h"
#include "ir/statements/returnStatement.h"
#include "ir/statements/throwStatement.h"
#include "ir/statements/variableDeclaration.h"
#include "ir/statements/variableDeclarator.h"
#include "compiler/lowering/util.h"
#include "lsp/include/internal_api.h"
#include "util/helpers.h"
#include "util/ustring.h"
#include "lexer/token/sourceLocation.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "varbinder/declaration.h"
#include "refactor_provider.h"
#include "internal_api.h"
#include "services/text_change/change_tracker.h"
#include "refactors/refactor_types.h"
#include "rename.h"
#include "types.h"

namespace ark::es2panda::lsp {

constexpr size_t MIN_QUOTED_LITERAL_LENGTH = 2;
constexpr size_t MIN_INLINE_DECLARATORS = 2;

struct RefactorSelectionState;
struct RefactorAvailabilityFlags;
struct InlineGlobalConstantMultiDeclaratorCandidate {
    TextRange trimmed {};
    std::string_view source;
    std::string uniqueVarName;
    std::string placeholder;
    ir::VariableDeclarator *declarator {nullptr};
    ir::VariableDeclaration *declaration {nullptr};
};

static bool IsSelectionInsideUnterminatedDeclarationInitializer(const RefactorContext &context, TextRange trimmed);
static bool IsLiteralSelectionInsideDeclarationInitializer(const RefactorContext &context, TextRange trimmed);
ir::AstNode *FindExactSelectionExpression(const RefactorContext &context, TextRange selection);
ir::AstNode *ResolveExpressionCoveringRange(const RefactorContext &context, TextRange initRange);

static ir::ClassProperty *FindContainingClassPropertyByRange(const RefactorContext &context, TextRange selection);
static bool IsSelectionInsideTypeAnnotationContext(const RefactorContext &refContext, TextRange span);
static bool IsQuotedLiteralSelectionText(const RefactorContext &refContext, TextRange span);
static bool CanRecoverEmptyConstantRefactor(const public_lib::Context *ctx, const RefactorContext &refContext,
                                            TextRange trimmed);
static ir::AstNode *ResolveFallbackConstantSelectionExpr(const RefactorContext &refContext, TextRange trimmed);
static bool IsRecoverableConstantSelection(public_lib::Context *ctx, const RefactorContext &refContext,
                                           TextRange trimmed, ir::AstNode *selectedExpr);
static bool AddRecoveredConstantActions(std::vector<RefactorAction> &actions, const ScopeContext &scope);
static bool IsDeclarationBoundaryChar(char ch);
static bool IsUnmatchedClosingDelimiter(char ch, int parenDepth, int bracketDepth);
static void UpdateDelimiterDepths(char ch, int &parenDepth, int &bracketDepth);
static std::optional<size_t> ResolveDeclarationCommaInsertCandidate(std::string_view source, size_t commaPos,
                                                                    size_t selectionPos);
static bool IsExpressionLikeRepeatedOccurrence(std::string_view source, std::string_view selectedText, size_t pos);

static std::optional<size_t> FindDeclarationInsertPosFromNode(std::string_view source, ir::AstNode *node);
static ir::AstNode *ResolveGlobalConstantSelectionExpression(const RefactorContext &context, TextRange trimmedSpan);
static std::optional<TextRange> FindLaterMatchingLiteralInitializerSpan(const RefactorContext &context,
                                                                        TextRange trimmedSpan);
static bool IsValidGlobalConstantSelection(const RefactorContext &context, const std::string &actionName);
static TextRange ResolveAvailabilityDependencySpan(const RefactorSelectionState &state);
static void ApplyAvailabilityLocalDependencyAdjustments(RefactorAvailabilityFlags &flags,
                                                        const RefactorContext &context,
                                                        const RefactorSelectionState &state, const ScopeContext &scope);
static bool ResolveGlobalFunctionExternalWriteDisallowFlag(const RefactorAvailabilityFlags &flags,
                                                           const RefactorContext &context, public_lib::Context *ctx,
                                                           const RefactorSelectionState &state);
static std::optional<InlineGlobalConstantMultiDeclaratorCandidate> ResolveInlineGlobalConstantMultiDeclaratorCandidate(
    const RefactorContext &context, public_lib::Context *ctx, const std::string &actionName);
static std::optional<RefactorEditInfo> BuildSingleDeclaratorInlineGlobalConstantEdits(
    const RefactorContext &context, public_lib::Context *ctx,
    const InlineGlobalConstantMultiDeclaratorCandidate &candidate);
static std::optional<RefactorEditInfo> BuildMultiDeclaratorInlineGlobalConstantEdits(
    const RefactorContext &context, public_lib::Context *ctx,
    const InlineGlobalConstantMultiDeclaratorCandidate &candidate);
static bool IsFullInitializerRhsSelection(const RefactorContext &context, TextRange trimmedSpan);
static bool HasExactGlobalConstantExpressionSelection(const RefactorContext &context, TextRange trimmedSpan);
static bool HasExactClassPropertyInitializerSelection(const RefactorContext &context, TextRange trimmedSpan);
static bool IsNonArithmeticCoveringExpressionSelection(const RefactorContext &context, TextRange trimmedSpan);

static std::string BuildNamedScopeDescription(std::string_view symbolType, std::string_view scopeType,
                                              const std::string &scopeName)
{
    std::string description = "Extract to ";
    description.append(symbolType);
    description.append(" in ");
    description.append(scopeType);
    if (scopeName.empty()) {
        description.append(" scope");
    } else {
        description.append(" '");
        description.append(scopeName);
        description.push_back('\'');
    }
    return description;
}

static void AddRefactorAction(std::vector<RefactorAction> &list, const RefactorActionView &info,
                              std::string description = "", std::string actionName = "", std::string actionKind = "")
{
    RefactorAction action;
    action.name = actionName.empty() ? std::string(info.name) : std::move(actionName);
    action.description = description.empty() ? std::string(info.description) : std::move(description);
    action.kind = actionKind.empty() ? std::string(info.kind) : std::move(actionKind);
    list.push_back(std::move(action));
}

static std::string BuildNamespaceScopedActionName(std::string_view prefix, size_t namespaceDepth)
{
    return std::string(prefix) + std::to_string(namespaceDepth);
}

static size_t FindNamespaceBodyInsertPos(std::string_view source, const ir::ClassDefinition *namespaceScope)
{
    if (namespaceScope == nullptr || namespaceScope->Start().index >= source.size()) {
        return 0;
    }
    size_t bracePos = source.find('{', namespaceScope->Start().index);
    if (bracePos == std::string_view::npos) {
        return 0;
    }
    ++bracePos;
    while (bracePos < source.size() && (source[bracePos] == '\r' || source[bracePos] == '\n' ||
                                        source[bracePos] == ' ' || source[bracePos] == '\t')) {
        if (source[bracePos] == '\r' && bracePos + 1 < source.size() && source[bracePos + 1] == '\n') {
            bracePos += CRLF_LENGTH;
            continue;
        }
        ++bracePos;
    }
    return bracePos;
}

static bool IsInsideExtractionRange(const ir::AstNode *node, TextRange positions)
{
    return node->Start().index >= positions.pos && node->End().index <= positions.end;
}

static bool HasBlockEnclosing(ir::AstNode *node)
{
    auto *block = node->AsArrowFunctionExpression()->Function()->Body()->AsBlockStatement();
    return !(block == nullptr || (block->Start().index == block->End().index));
}

static bool IsControlFlowEncloseScopeNode(const ir::AstNode *node)
{
    return node != nullptr && (node->IsForUpdateStatement() || node->IsForInStatement() || node->IsForOfStatement() ||
                               node->IsWhileStatement() || node->IsDoWhileStatement() || node->IsIfStatement() ||
                               node->IsSwitchStatement() || node->IsTryStatement() || node->IsCatchClause());
}

static bool IsInsideLoopStatement(ir::AstNode *node)
{
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsForUpdateStatement() || current->IsForInStatement() || current->IsForOfStatement() ||
            current->IsWhileStatement() || current->IsDoWhileStatement()) {
            return true;
        }
    }
    return false;
}

static bool IsIdentifierTokenAt(std::string_view text, size_t pos, std::string_view token)
{
    if (token.empty() || pos + token.size() > text.size() || text.substr(pos, token.size()) != token) {
        return false;
    }
    const bool leftOk = pos == 0 || !IsIdentifierContinuation(text[pos - 1]);
    const size_t end = pos + token.size();
    const bool rightOk = end >= text.size() || !IsIdentifierContinuation(text[end]);
    return leftOk && rightOk;
}

static bool ContainsIdentifierTokenInText(std::string_view text, std::string_view token)
{
    size_t pos = text.find(token);
    while (pos != std::string_view::npos) {
        if (IsIdentifierTokenAt(text, pos, token)) {
            return true;
        }
        pos = text.find(token, pos + token.size());
    }
    return false;
}

static bool IsThrowKeywordLinePrefix(std::string_view prefix)
{
    size_t pos = 0;
    while (pos < prefix.size() && std::isspace(static_cast<unsigned char>(prefix[pos])) != 0) {
        ++pos;
    }
    if (!IsIdentifierTokenAt(prefix, pos, "throw")) {
        return false;
    }
    pos += std::string_view("throw").size();
    while (pos < prefix.size() && std::isspace(static_cast<unsigned char>(prefix[pos])) != 0) {
        ++pos;
    }
    return pos == prefix.size();
}

static bool IsTypeAliasLikeLineForTypeSelection(std::string_view line, size_t selectionOffset)
{
    size_t pos = 0;
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])) != 0) {
        ++pos;
    }
    if (!IsIdentifierTokenAt(line, pos, "type")) {
        return false;
    }
    const size_t eqPos = line.find('=', pos + std::string_view("type").size());
    if (eqPos == std::string_view::npos || eqPos >= selectionOffset) {
        return false;
    }
    const std::string_view rhs = line.substr(eqPos + 1);
    return rhs.find('|') != std::string_view::npos ||
           (ContainsIdentifierTokenInText(rhs, "extends") && rhs.find('?') != std::string_view::npos &&
            rhs.find(':') != std::string_view::npos);
}

static bool IsSelectionInsideThrowArgument(const RefactorContext &context, TextRange trimmed)
{
    auto *node = GetTouchingTokenByRange(context.context, trimmed, false);
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (!current->IsThrowStatement()) {
            continue;
        }
        auto *argument = current->AsThrowStatement()->Argument();
        return argument != nullptr && argument->Start().index <= trimmed.pos && argument->End().index >= trimmed.end;
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx != nullptr && ctx->sourceFile != nullptr && trimmed.pos <= ctx->sourceFile->source.size()) {
        const auto &source = ctx->sourceFile->source;
        const size_t lineStartPos = source.rfind('\n', trimmed.pos);
        const size_t lineStart = lineStartPos == std::string_view::npos ? 0 : lineStartPos + 1;
        const std::string_view prefix(source.data() + lineStart, trimmed.pos - lineStart);
        if (IsThrowKeywordLinePrefix(prefix)) {
            return true;
        }
    }
    return false;
}

static bool IsSelectionInsideTsConditionalType(const RefactorContext &context, TextRange trimmed)
{
    auto *node = GetTouchingTokenByRange(context.context, trimmed, false);
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsTSTypeAliasDeclaration() && current->Start().index <= trimmed.pos &&
            current->End().index >= trimmed.end) {
            return true;
        }
        if (current->IsTSConditionalType() && current->Start().index <= trimmed.pos &&
            current->End().index >= trimmed.end) {
            return true;
        }
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx != nullptr && ctx->sourceFile != nullptr && trimmed.pos <= ctx->sourceFile->source.size()) {
        const auto &source = ctx->sourceFile->source;
        const size_t lineStartPos = source.rfind('\n', trimmed.pos);
        const size_t lineStart = lineStartPos == std::string_view::npos ? 0 : lineStartPos + 1;
        const size_t lineEndPos = source.find('\n', trimmed.end);
        const size_t lineEnd = lineEndPos == std::string_view::npos ? source.size() : lineEndPos;
        const std::string_view line(source.data() + lineStart, lineEnd - lineStart);
        if (IsTypeAliasLikeLineForTypeSelection(line, trimmed.pos - lineStart)) {
            return true;
        }
    }
    return false;
}

static std::optional<size_t> FindBlockStatementInsertPos(std::string_view source, ir::AstNode *node, TextRange trimmed)
{
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        auto *parent = current->Parent();
        if (parent == nullptr || !parent->IsBlockStatement()) {
            continue;
        }
        if (parent->Start().index <= trimmed.pos && parent->End().index >= trimmed.end) {
            return FindLineStart(source, current->Start().index);
        }
    }
    return std::nullopt;
}

static bool IsObjectLiteralSelectionNode(const RefactorContext &context, ir::AstNode *selectionNode, TextRange trimmed)
{
    if (IsObjectLiteralInitializerExtraction(selectionNode)) {
        return true;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx != nullptr && ctx->sourceFile != nullptr && trimmed.pos < trimmed.end &&
        trimmed.end <= ctx->sourceFile->source.size() && ctx->sourceFile->source[trimmed.pos] == '{' &&
        ctx->sourceFile->source[trimmed.end - 1] == '}') {
        return true;
    }
    auto *coverExpr = ResolveExpressionCoveringRange(context, trimmed);
    return IsObjectLiteralInitializerExtraction(coverExpr);
}

static bool IsGlobalStaticInitializerBody(const ir::AstNode *node)
{
    if (node == nullptr || !node->IsBlockStatement()) {
        return false;
    }
    auto *owner = node->Parent();
    if (owner == nullptr || !owner->IsClassStaticBlock()) {
        return false;
    }
    for (auto *parent = owner->Parent(); parent != nullptr; parent = parent->Parent()) {
        if (!parent->IsClassDefinition()) {
            continue;
        }
        auto *classDef = parent->AsClassDefinition();
        return classDef != nullptr && (classDef->IsGlobal() || classDef->IsNamespaceTransformed());
    }
    return false;
}

static bool HasEncloseScope(ir::AstNode *node)
{
    for (; node != nullptr; node = node->Parent()) {
        auto *parent = node->Parent();
        if (parent != nullptr && parent->IsBlockStatement() && !IsGlobalStaticInitializerBody(parent) &&
            !IsProgramParent(parent) && !IsNamespaceModuleParent(parent) &&
            !IsSyntheticScriptFunctionUnderGlobalClass(parent)) {
            return true;
        }
        if (node->IsFunctionDeclaration() || node->IsFunctionExpression()) {
            return true;
        }
        if (node->IsArrowFunctionExpression()) {
            return HasBlockEnclosing(node);
        }
        if (IsControlFlowEncloseScopeNode(node)) {
            return true;
        }
    }
    return false;
}

static std::string_view TrimOuterWhitespace(std::string_view text)
{
    size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return begin >= end ? std::string_view {} : text.substr(begin, end - begin);
}

static std::string_view TrimTrailingSemicolonAndWhitespace(std::string_view value)
{
    if (!value.empty() && value.back() == ';') {
        value.remove_suffix(1);
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
            value.remove_suffix(1);
        }
    }
    return value;
}

static bool IsQuotedLiteralText(std::string_view value)
{
    constexpr size_t kQuotedLiteralMinLength = 2;
    constexpr char kDoubleQuote = '"';
    return value.size() >= kQuotedLiteralMinLength && value.front() == kDoubleQuote && value.back() == kDoubleQuote;
}

static bool IsKeywordLiteralText(std::string_view value)
{
    return value == "true" || value == "false" || value == "null";
}

static bool IsNumericLiteralText(std::string_view value)
{
    bool hasDigit = false;
    for (size_t i = 0; i < value.size(); ++i) {
        const char ch = value[i];
        if (i == 0 && (ch == '+' || ch == '-')) {
            continue;
        }
        if (ch == '.') {
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(ch)) == 0) {
            return false;
        }
        hasDigit = true;
    }
    return hasDigit;
}

static bool IsTopLevelLiteralSelectionText(const RefactorContext &context, TextRange span)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || span.pos >= span.end ||
        span.end > ctx->sourceFile->source.size()) {
        return false;
    }
    std::string_view text(ctx->sourceFile->source.data() + span.pos, span.end - span.pos);
    std::string_view trimmed = TrimOuterWhitespace(text);
    if (trimmed.empty()) {
        return false;
    }
    trimmed = TrimTrailingSemicolonAndWhitespace(trimmed);
    return IsQuotedLiteralText(trimmed) || IsKeywordLiteralText(trimmed) || IsNumericLiteralText(trimmed);
}

static bool IsWithinSwitchCaseTest(const ir::AstNode *node)
{
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (!current->IsSwitchCaseStatement()) {
            continue;
        }
        auto *test = current->AsSwitchCaseStatement()->Test();
        if (test == nullptr) {
            return false;
        }
        const size_t nodeStart = node->Start().index;
        const size_t nodeEnd = node->End().index;
        return test->Start().index <= nodeStart && nodeEnd <= test->End().index;
    }
    return false;
}

static bool IsCoveredByControlFlowTest(const ir::AstNode *test, TextRange trimmed)
{
    if (test == nullptr) {
        return false;
    }
    if (test->Start().index <= trimmed.pos && test->End().index >= trimmed.end) {
        return true;
    }
    return trimmed.pos < test->Start().index && trimmed.end <= test->End().index && trimmed.end > test->Start().index;
}

static bool IsCoveredByControlFlowTestNode(const ir::AstNode *test, const ir::AstNode *selectedNode)
{
    return test != nullptr && selectedNode != nullptr && test->Start().index <= selectedNode->Start().index &&
           test->End().index >= selectedNode->End().index;
}

static ir::AstNode *ResolveSelectedExpressionNodeForControlFlowTest(const RefactorContext &context, TextRange trimmed)
{
    ir::AstNode *selectedNode = FindExactSelectionExpression(context, trimmed);
    if (selectedNode == nullptr) {
        selectedNode = ResolveExpressionCoveringRange(context, trimmed);
    }
    if (selectedNode == nullptr) {
        selectedNode = GetTouchingTokenByRange(context.context, trimmed, false);
    }
    for (auto *current = selectedNode; current != nullptr; current = current->Parent()) {
        if (current->IsExpression()) {
            return current;
        }
    }
    return selectedNode;
}

static std::string NormalizeControlFlowCompareText(std::string_view text)
{
    std::string out;
    out.reserve(text.size());
    for (char ch : text) {
        if (std::isspace(static_cast<unsigned char>(ch)) == 0) {
            out.push_back(ch);
        }
    }
    return out;
}

static bool HasSelectionTextInBodyForControlFlow(public_lib::Context *ctx, TextRange trimmed, const ir::AstNode *body)
{
    if (body == nullptr || ctx == nullptr || ctx->sourceFile == nullptr || trimmed.end <= trimmed.pos ||
        trimmed.end > ctx->sourceFile->source.size()) {
        return false;
    }
    const std::string selectedText = NormalizeControlFlowCompareText(
        std::string_view(ctx->sourceFile->source.data() + trimmed.pos, trimmed.end - trimmed.pos));
    if (selectedText.empty()) {
        return false;
    }
    const std::string bodyText = NormalizeControlFlowCompareText(
        GetSourceTextOfNodeFromSourceFile(ctx->sourceFile->source, const_cast<ir::AstNode *>(body)));
    return !bodyText.empty() && bodyText.find(selectedText) != std::string::npos;
}

static bool HasEquivalentExprInBodyForControlFlow(public_lib::Context *ctx, const ir::AstNode *body,
                                                  const ir::AstNode *selectedExpr)
{
    if (body == nullptr || selectedExpr == nullptr || !selectedExpr->IsExpression() || ctx == nullptr ||
        ctx->sourceFile == nullptr) {
        return false;
    }
    const std::string selectedText = NormalizeControlFlowCompareText(
        GetSourceTextOfNodeFromSourceFile(ctx->sourceFile->source, const_cast<ir::AstNode *>(selectedExpr)));
    if (selectedText.empty()) {
        return false;
    }
    bool found = false;
    body->FindChild([&](ir::AstNode *node) {
        if (found || node == nullptr || !node->IsExpression()) {
            return false;
        }
        if (node->Start().index == selectedExpr->Start().index && node->End().index == selectedExpr->End().index) {
            return false;
        }
        std::string candidate =
            NormalizeControlFlowCompareText(GetSourceTextOfNodeFromSourceFile(ctx->sourceFile->source, node));
        if (candidate == selectedText) {
            found = true;
            return true;
        }
        return false;
    });
    return found;
}

static bool ShouldDropForControlFlowStmt(public_lib::Context *ctx, TextRange trimmed, const ir::AstNode *node,
                                         const ir::AstNode *test, const ir::AstNode *selectedNode)
{
    const bool coveredBySpan = IsCoveredByControlFlowTest(test, trimmed);
    const bool coveredByNode = IsCoveredByControlFlowTestNode(test, selectedNode);
    if (!(coveredBySpan || coveredByNode)) {
        return false;
    }
    const ir::AstNode *probeExpr = selectedNode;
    if (test != nullptr &&
        (!coveredByNode || probeExpr == nullptr || !probeExpr->IsExpression() ||
         probeExpr->Start().index < test->Start().index || probeExpr->End().index > test->End().index)) {
        probeExpr = test;
    }
    if (probeExpr == nullptr || !probeExpr->IsExpression()) {
        return true;
    }
    if (node->IsIfStatement()) {
        auto *ifStmt = node->AsIfStatement();
        const bool hasReuseInBody = HasEquivalentExprInBodyForControlFlow(ctx, ifStmt->Consequent(), probeExpr) ||
                                    HasEquivalentExprInBodyForControlFlow(ctx, ifStmt->Alternate(), probeExpr);
        if (!coveredByNode) {
            const bool hasSelectionTextReuse =
                HasSelectionTextInBodyForControlFlow(ctx, trimmed, ifStmt->Consequent()) ||
                HasSelectionTextInBodyForControlFlow(ctx, trimmed, ifStmt->Alternate());
            return !hasSelectionTextReuse;
        }
        return !hasReuseInBody;
    }
    if (node->IsWhileStatement()) {
        return !HasEquivalentExprInBodyForControlFlow(ctx, node->AsWhileStatement()->Body(), probeExpr);
    }
    if (node->IsDoWhileStatement()) {
        return !HasEquivalentExprInBodyForControlFlow(ctx, node->AsDoWhileStatement()->Body(), probeExpr);
    }
    if (node->IsForUpdateStatement()) {
        return !HasEquivalentExprInBodyForControlFlow(ctx, node->AsForUpdateStatement()->Body(), probeExpr);
    }
    return true;
}

static bool IsControlFlowTestNode(public_lib::Context *ctx, TextRange trimmed, const ir::AstNode *node,
                                  const ir::AstNode *selectedNode)
{
    if (node == nullptr) {
        return false;
    }
    if (node->IsIfStatement()) {
        return ShouldDropForControlFlowStmt(ctx, trimmed, node, node->AsIfStatement()->Test(), selectedNode);
    }
    if (node->IsWhileStatement()) {
        return ShouldDropForControlFlowStmt(ctx, trimmed, node, node->AsWhileStatement()->Test(), selectedNode);
    }
    if (node->IsDoWhileStatement()) {
        return ShouldDropForControlFlowStmt(ctx, trimmed, node, node->AsDoWhileStatement()->Test(), selectedNode);
    }
    if (node->IsForUpdateStatement()) {
        return ShouldDropForControlFlowStmt(ctx, trimmed, node, node->AsForUpdateStatement()->Test(), selectedNode);
    }
    return false;
}

static bool IsExactControlFlowTestSelection(public_lib::Context *ctx, const ir::AstNode *current,
                                            const ir::AstNode *selectedNode)
{
    if (current == nullptr || selectedNode == nullptr) {
        return false;
    }
    const ir::AstNode *test = nullptr;
    const ir::AstNode *body = nullptr;
    if (current->IsIfStatement()) {
        auto *stmt = current->AsIfStatement();
        test = stmt == nullptr ? nullptr : stmt->Test();
        if (test != nullptr && test->Start().index == selectedNode->Start().index &&
            test->End().index == selectedNode->End().index) {
            return !(HasEquivalentExprInBodyForControlFlow(ctx, stmt->Consequent(), selectedNode) ||
                     HasEquivalentExprInBodyForControlFlow(ctx, stmt->Alternate(), selectedNode));
        }
        return false;
    }
    if (current->IsWhileStatement()) {
        auto *stmt = current->AsWhileStatement();
        test = stmt == nullptr ? nullptr : stmt->Test();
        body = stmt == nullptr ? nullptr : stmt->Body();
    } else if (current->IsDoWhileStatement()) {
        auto *stmt = current->AsDoWhileStatement();
        test = stmt == nullptr ? nullptr : stmt->Test();
        body = stmt == nullptr ? nullptr : stmt->Body();
    } else if (current->IsForUpdateStatement()) {
        auto *stmt = current->AsForUpdateStatement();
        test = stmt == nullptr ? nullptr : stmt->Test();
        body = stmt == nullptr ? nullptr : stmt->Body();
    } else {
        return false;
    }
    return test != nullptr && test->Start().index == selectedNode->Start().index &&
           test->End().index == selectedNode->End().index &&
           !HasEquivalentExprInBodyForControlFlow(ctx, body, selectedNode);
}

static bool FindMatchedControlFlowTestInAst(public_lib::Context *ctx, TextRange trimmed,
                                            const ir::AstNode *selectedNode)
{
    bool matched = false;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (matched || node == nullptr) {
            return false;
        }
        if (IsControlFlowTestNode(ctx, trimmed, node, selectedNode)) {
            matched = true;
            return true;
        }
        return false;
    });
    return matched;
}

static bool HasAncestorControlFlowTestSelection(public_lib::Context *ctx, TextRange trimmed, ir::AstNode *selectedNode)
{
    for (auto *current = selectedNode; current != nullptr; current = current->Parent()) {
        if (IsExactControlFlowTestSelection(ctx, current, selectedNode) ||
            IsControlFlowTestNode(ctx, trimmed, current, selectedNode)) {
            return true;
        }
    }
    return false;
}

static bool IsSelectionWithinControlFlowTest(const RefactorContext &context)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    const TextRange trimmed = GetTrimmedSelectionSpan(context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr ||
        trimmed.end <= trimmed.pos) {
        return false;
    }
    ir::AstNode *selectedNode = ResolveSelectedExpressionNodeForControlFlowTest(context, trimmed);
    if (HasAncestorControlFlowTestSelection(ctx, trimmed, selectedNode)) {
        return true;
    }
    return FindMatchedControlFlowTestInAst(ctx, trimmed, selectedNode);
}

static bool IsSelectionOnControlFlowJumpStatement(const RefactorContext &context, TextRange trimmed)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || trimmed.end <= trimmed.pos ||
        trimmed.end > ctx->sourceFile->source.size()) {
        return false;
    }
    auto *selectedNode = GetTouchingTokenByRange(context.context, trimmed, false);
    for (auto *current = selectedNode; current != nullptr; current = current->Parent()) {
        if (!current->IsBreakStatement() && !current->IsContinueStatement()) {
            continue;
        }
        if (current->Start().index != trimmed.pos || current->End().index < trimmed.end) {
            continue;
        }
        for (size_t pos = trimmed.end; pos < current->End().index; ++pos) {
            const char ch = ctx->sourceFile->source[pos];
            if (ch != ';' && std::isspace(static_cast<unsigned char>(ch)) == 0) {
                return false;
            }
        }
        return true;
    }
    return false;
}

static bool IsClassDefinitionNamed(const ir::ClassDefinition *classDef, const std::string &name)
{
    return classDef != nullptr && IdentifierNameMutf8(classDef->Ident()) == name;
}

static bool NamespaceScopeHasClassValue(const ir::ClassDefinition *namespaceScope, const std::string &name)
{
    if (namespaceScope == nullptr || name.empty()) {
        return false;
    }
    bool found = false;
    namespaceScope->FindChild([&found, &name](ir::AstNode *node) {
        if (found || node == nullptr) {
            return found;
        }
        if (node->IsClassDeclaration() && IsClassDefinitionNamed(node->AsClassDeclaration()->Definition(), name)) {
            found = true;
            return true;
        }
        if (node->IsClassDefinition() && IsClassDefinitionNamed(node->AsClassDefinition(), name)) {
            found = true;
            return true;
        }
        return false;
    });
    return found;
}

static ir::Identifier *FindIdentifierAtSelection(const RefactorContext &context, TextRange trimmed)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return nullptr;
    }
    ir::Identifier *result = nullptr;
    ctx->parserProgram->Ast()->FindChild([&result, trimmed](ir::AstNode *node) {
        if (result != nullptr || node == nullptr || !node->IsIdentifier()) {
            return result != nullptr;
        }
        if (node->Start().index == trimmed.pos && node->End().index == trimmed.end) {
            result = node->AsIdentifier();
            return true;
        }
        return false;
    });
    return result;
}

static bool IsNamespaceClassValueReferenceSelection(const RefactorContext &context, TextRange trimmed)
{
    if (trimmed.end <= trimmed.pos) {
        return false;
    }
    auto *ident = FindIdentifierAtSelection(context, trimmed);
    if (ident == nullptr || IsDeclarationIdentifier(ident) || IsMemberPropertyIdentifier(ident)) {
        return false;
    }
    const std::string name = IdentifierNameMutf8(ident);
    for (auto *namespaceScope : CollectEnclosingNamespaceScopes(ident)) {
        if (NamespaceScopeHasClassValue(namespaceScope, name)) {
            return true;
        }
    }
    auto *classDef = FindEnclosingClassDefinition(ident);
    if (classDef != nullptr && classDef->IsNamespaceTransformed() && !classDef->IsGlobal()) {
        return NamespaceScopeHasClassValue(classDef, name);
    }
    return false;
}

static bool ContainsThisOrSuperExpression(const ir::AstNode *node)
{
    if (node == nullptr) {
        return false;
    }
    if (node->IsThisExpression() || node->IsSuperExpression()) {
        return true;
    }
    bool found = false;
    node->Iterate([&](ir::AstNode *child) {
        if (found || child == nullptr) {
            return;
        }
        if (ContainsThisOrSuperExpression(child)) {
            found = true;
        }
    });
    return found;
}

static bool ContainsThisOrSuperInRange(public_lib::Context *ctx, TextRange range)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr ||
        range.end <= range.pos) {
        return false;
    }
    bool found = false;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (found || node == nullptr) {
            return false;
        }
        if ((node->IsThisExpression() || node->IsSuperExpression()) && node->Start().index >= range.pos &&
            node->End().index <= range.end) {
            found = true;
            return true;
        }
        return false;
    });
    return found;
}

bool IsObjectLiteralInitializerExtraction(const ir::AstNode *node)
{
    if (node == nullptr) {
        return false;
    }
    if (node->IsObjectExpression()) {
        auto *parent = node->Parent();
        return parent != nullptr && parent->IsVariableDeclarator();
    }
    if (node->IsVariableDeclarator()) {
        auto *init = node->AsVariableDeclarator()->Init();
        return init != nullptr && init->IsObjectExpression();
    }
    if (node->IsVariableDeclaration()) {
        for (auto *declarator : node->AsVariableDeclaration()->Declarators()) {
            if (declarator == nullptr) {
                continue;
            }
            auto *init = declarator->Init();
            if (init != nullptr && init->IsObjectExpression()) {
                return true;
            }
        }
        return false;
    }
    if (node->IsExpressionStatement()) {
        auto *expr = node->AsExpressionStatement()->GetExpression();
        return expr != nullptr && expr->IsObjectExpression();
    }
    return false;
}

static ScopeContext ResolveScopeContext(ir::AstNode *node)
{
    ScopeContext scope;
    scope.hasEncloseScope = HasEncloseScope(node);
    scope.hasClassScope = IsClassContext(node);
    if (scope.hasClassScope) {
        auto *classDef = FindEnclosingClassDefinition(node);
        scope.classScopeName = IdentifierNameMutf8(classDef == nullptr ? nullptr : classDef->Ident());
    }
    for (auto *namespaceScope : CollectEnclosingNamespaceScopes(node)) {
        scope.namespaceScopeNames.push_back(
            IdentifierNameMutf8(namespaceScope == nullptr ? nullptr : namespaceScope->Ident()));
    }
    return scope;
}

static void AddExtractFunctionActions(std::vector<RefactorAction> &actions, const ScopeContext &scope)
{
    const bool hasNamespaceScope = !scope.namespaceScopeNames.empty();
    if (scope.hasClassScope) {
        AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_CLASS,
                          BuildNamedScopeDescription("function", "class", scope.classScopeName));
    }
    if (hasNamespaceScope) {
        AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_ENCLOSE,
                          BuildNamedScopeDescription("function", "namespace", scope.namespaceScopeNames.front()));
        for (size_t namespaceDepth = 1; namespaceDepth < scope.namespaceScopeNames.size(); ++namespaceDepth) {
            AddRefactorAction(
                actions, EXTRACT_FUNCTION_ACTION_ENCLOSE,
                BuildNamedScopeDescription("function", "namespace", scope.namespaceScopeNames[namespaceDepth]),
                BuildNamespaceScopedActionName(EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX, namespaceDepth),
                std::string(EXTRACT_FUNCTION_ACTION_ENCLOSE.kind));
        }
    }
    AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_GLOBAL);
}

static bool HasValidFunctionExtractionCandidate(const RefactorContext &context)
{
    auto candidates = GetPossibleFunctionExtractions(context);
    return std::any_of(candidates.begin(), candidates.end(),
                       [](const FunctionExtraction &candidate) { return candidate.node != nullptr; });
}

static bool HasDeclarationLeadingExternalWriteUsage(const RefactorContext &context, public_lib::Context *ctx,
                                                    TextRange trimmedSpan)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || trimmedSpan.pos >= trimmedSpan.end ||
        trimmedSpan.end > ctx->sourceFile->source.size()) {
        return false;
    }
    std::string_view selected(ctx->sourceFile->source.data() + trimmedSpan.pos, trimmedSpan.end - trimmedSpan.pos);
    const bool startsWithDecl = selected.rfind("const ", 0) == 0 || selected.rfind("let ", 0) == 0;
    if (!startsWithDecl) {
        return false;
    }
    size_t keywordLen =
        selected.rfind("const ", 0) == 0 ? std::string_view("const ").size() : std::string_view("let ").size();
    size_t i = keywordLen;
    while (i < selected.size() && std::isspace(static_cast<unsigned char>(selected[i])) != 0) {
        ++i;
    }
    size_t nameBegin = i;
    while (i < selected.size()) {
        const char ch = selected[i];
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0 && ch != '_' && ch != '$') {
            break;
        }
        ++i;
    }
    const std::string declaredName = i > nameBegin ? std::string(selected.substr(nameBegin, i - nameBegin)) : "";
    if (declaredName.empty() || trimmedSpan.end >= ctx->sourceFile->source.size()) {
        return false;
    }
    const std::string_view suffix(ctx->sourceFile->source.data() + trimmedSpan.end,
                                  ctx->sourceFile->source.size() - trimmedSpan.end);
    const bool declaredVarUsedAfterSelection = ContainsIdentifierToken(suffix, declaredName);
    FunctionIOInfo ioInfo = AnalyzeFunctionIO(context, trimmedSpan, true, nullptr, false);
    return !ioInfo.hasReturnStatement && declaredVarUsedAfterSelection;
}

static void RemoveActionByName(std::vector<RefactorAction> &actions, std::string_view name)
{
    auto it = actions.begin();
    while (it != actions.end()) {
        if (it->name == std::string(name)) {
            it = actions.erase(it);
            continue;
        }
        ++it;
    }
}

static void RemoveFunctionGlobalActions(std::vector<RefactorAction> &actions)
{
    RemoveActionByName(actions, EXTRACT_FUNCTION_ACTION_GLOBAL.name);
}

static void RemoveFunctionEncloseAndNamespaceAndGlobalActions(std::vector<RefactorAction> &actions)
{
    std::vector<RefactorAction> kept;
    kept.reserve(actions.size());

    for (const auto &action : actions) {
        const bool isEnclose = action.name == EXTRACT_FUNCTION_ACTION_ENCLOSE.name;
        const bool isGlobal = action.name == EXTRACT_FUNCTION_ACTION_GLOBAL.name;
        const bool isNamespace = action.name.rfind(EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX, 0) == 0;
        if (!isEnclose && !isGlobal && !isNamespace) {
            kept.push_back(action);
        }
    }

    actions.swap(kept);
}

static void RemoveConstantEncloseAndNamespaceActions(std::vector<RefactorAction> &actions)
{
    std::vector<RefactorAction> kept;
    kept.reserve(actions.size());

    for (const auto &action : actions) {
        const bool isEnclose = action.name == EXTRACT_CONSTANT_ACTION_ENCLOSE.name;
        const bool isNamespace = action.name.rfind(EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX, 0) == 0;
        if (!isEnclose && !isNamespace) {
            kept.push_back(action);
        }
    }

    actions.swap(kept);
}

static void AddExtractVariableActions(std::vector<RefactorAction> &actions, const ScopeContext &scope,
                                      bool disallowGlobalConstant);

struct FunctionRefactorRestrictions {
    bool canExtractFunctionBySelectionShape {false};
    bool containsThisOrSuper {false};
    bool hasNamespacePrivateFunctionDependency {false};
    bool hasNamespacePrivateTypeAnnotationDependency {false};
    bool disallowGlobalFunctionForDeclarationLeadingExternalWrite {false};
    bool disallowGlobalFunctionForNamespaceObjectLiteral {false};
};

static void AddAvailableFunctionRefactors(std::vector<RefactorAction> &actions, const RefactorContext &context,
                                          const ScopeContext &scope, ir::AstNode *node,
                                          const FunctionRefactorRestrictions &restrictions)
{
    if (!(node->IsExpression() || node->IsFunctionExpression() || node->IsArrowFunctionExpression() ||
          node->IsStatement() || node->IsVariableDeclaration())) {
        return;
    }
    if (!restrictions.canExtractFunctionBySelectionShape || !HasValidFunctionExtractionCandidate(context)) {
        return;
    }
    AddExtractFunctionActions(actions, scope);
    if (restrictions.hasNamespacePrivateFunctionDependency ||
        restrictions.hasNamespacePrivateTypeAnnotationDependency ||
        restrictions.disallowGlobalFunctionForDeclarationLeadingExternalWrite ||
        restrictions.disallowGlobalFunctionForNamespaceObjectLiteral) {
        RemoveFunctionGlobalActions(actions);
    }
    if (restrictions.containsThisOrSuper && scope.hasClassScope) {
        RemoveFunctionEncloseAndNamespaceAndGlobalActions(actions);
    }
}

struct VariableRefactorRestrictions {
    bool disallowGlobalConstant {false};
    bool disallowNamespaceConstant {false};
    bool hasUseStaticDirective {false};
    bool disallowGlobalConstantForNamespaceObjectLiteral {false};
};

struct VariableRefactorRestrictionInputs {
    const ScopeContext &scope;
    ir::AstNode *node {nullptr};
    bool containsThisOrSuper {false};
    bool hasUseStaticDirective {false};
    bool disallowGlobalConstantForNamespaceObjectLiteral {false};
    bool hasNamespacePrivateDependency {false};
    bool hasLocalValueDependency {false};
    bool hasNamespacePrivateFunctionDependency {false};
};

static VariableRefactorRestrictions ResolveVariableRefactorRestrictions(const VariableRefactorRestrictionInputs &inputs)
{
    const bool isUseStaticObjectLiteralInitializer =
        inputs.hasUseStaticDirective && IsObjectLiteralInitializerExtraction(inputs.node);
    const bool disallowGlobalConstant =
        inputs.containsThisOrSuper ||
        (IsObjectLiteralInitializerExtraction(inputs.node) && !inputs.hasUseStaticDirective) ||
        inputs.disallowGlobalConstantForNamespaceObjectLiteral ||
        (!isUseStaticObjectLiteralInitializer && inputs.hasNamespacePrivateDependency) ||
        inputs.hasLocalValueDependency || inputs.hasNamespacePrivateFunctionDependency;
    const bool disallowNamespaceConstant =
        inputs.containsThisOrSuper && inputs.scope.hasClassScope && !inputs.scope.namespaceScopeNames.empty();
    return {disallowGlobalConstant, disallowNamespaceConstant, inputs.hasUseStaticDirective,
            inputs.disallowGlobalConstantForNamespaceObjectLiteral};
}

static void ApplyVariableRefactorRestrictions(std::vector<RefactorAction> &actions, const ScopeContext &scope,
                                              const VariableRefactorRestrictions &restrictions, ir::AstNode *node)
{
    AddExtractVariableActions(actions, scope, restrictions.disallowGlobalConstant);
    if (restrictions.disallowNamespaceConstant) {
        RemoveConstantEncloseAndNamespaceActions(actions);
    }
    if (restrictions.disallowGlobalConstantForNamespaceObjectLiteral) {
        RemoveActionByName(actions, EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    }
    if (restrictions.hasUseStaticDirective && IsObjectLiteralInitializerExtraction(node)) {
        const bool hasGlobalConstant = std::any_of(actions.begin(), actions.end(), [](const RefactorAction &action) {
            return action.name == std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.name);
        });
        if (!hasGlobalConstant) {
            AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_GLOBAL);
        }
    }
}

struct VariableRefactorAvailability {
    bool allowVariableActions {false};
    bool containsThisOrSuper {false};
    bool hasUseStaticDirective {false};
    bool disallowGlobalConstantForNamespaceObjectLiteral {false};
    bool hasNamespacePrivateDependency {false};
    bool hasLocalValueDependency {false};
    bool hasNamespacePrivateFunctionDependency {false};
};

static void AddAvailableVariableRefactors(std::vector<RefactorAction> &actions, const ScopeContext &scope,
                                          ir::AstNode *node, const VariableRefactorAvailability &availability)
{
    if (!availability.allowVariableActions ||
        (node->IsStatement() && !node->IsVariableDeclaration() && !node->IsBinaryExpression() &&
         !node->IsExpressionStatement() && !IsControlFlowEncloseScopeNode(node))) {
        return;
    }
    const VariableRefactorRestrictions restrictions = ResolveVariableRefactorRestrictions(
        {scope, node, availability.containsThisOrSuper, availability.hasUseStaticDirective,
         availability.disallowGlobalConstantForNamespaceObjectLiteral, availability.hasNamespacePrivateDependency,
         availability.hasLocalValueDependency, availability.hasNamespacePrivateFunctionDependency});
    ApplyVariableRefactorRestrictions(actions, scope, restrictions, node);
}

static void ApplyObjectLiteralGlobalConstantRule(std::vector<RefactorAction> &actions, bool isObjectLiteralSelection,
                                                 bool hasUseStaticDirective)
{
    if (isObjectLiteralSelection && !hasUseStaticDirective) {
        RemoveActionByName(actions, EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    }
}

static void ApplySwitchCaseConstantRules(std::vector<RefactorAction> &actions, ir::AstNode *node, TextRange span)
{
    if (!IsSwitchCaseTestSelection(node, span)) {
        return;
    }

    std::vector<RefactorAction> kept;
    kept.reserve(actions.size());
    for (const auto &action : actions) {
        const bool isEnclose = action.name == EXTRACT_CONSTANT_ACTION_ENCLOSE.name;
        const bool isClass = action.name == EXTRACT_CONSTANT_ACTION_CLASS.name;
        const bool isNamespace = action.name.rfind(EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX, 0) == 0;
        if (!isEnclose && !isClass && !isNamespace) {
            kept.push_back(action);
        }
    }
    actions.swap(kept);

    const bool hasGlobalConstant = std::any_of(actions.begin(), actions.end(), [](const RefactorAction &action) {
        return action.name == EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    });
    if (!hasGlobalConstant) {
        AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_GLOBAL);
    }
}

static void ApplyTopLevelLiteralRule(std::vector<RefactorAction> &actions, const RefactorContext &context,
                                     TextRange trimmedSpan, ir::AstNode *node)
{
    const bool isCompleteStatementSelection = node != nullptr && node->IsExpressionStatement() &&
                                              node->Start().index == trimmedSpan.pos &&
                                              node->End().index <= trimmedSpan.end;
    const bool isTopLevelLiteral = IsTopLevelLiteralSelectionText(context, trimmedSpan);
    const bool isSwitchCaseTest = IsWithinSwitchCaseTest(node);
    if (!isTopLevelLiteral || isSwitchCaseTest || !isCompleteStatementSelection) {
        return;
    }

    std::vector<RefactorAction> kept;
    kept.reserve(actions.size());
    for (const auto &action : actions) {
        const bool isEncloseAction = action.name == EXTRACT_CONSTANT_ACTION_ENCLOSE.name;
        const bool isEncloseKind = action.kind == EXTRACT_CONSTANT_ACTION_ENCLOSE.kind;
        if (!(isEncloseAction && isEncloseKind)) {
            kept.push_back(action);
        }
    }
    actions.swap(kept);
}

struct RefactorSelectionState {
    ir::AstNode *node {nullptr};
    bool allowVariableActions {true};
    bool selectionHasNewline {false};
    TextRange trimmedSpan {};
    const ir::AstNode *wholeDeclSelectionNode {nullptr};
    ir::AstNode *declInitializerExpr {nullptr};
    bool canExtractFunctionBySelectionShape {true};
};

static ir::AstNode *FindExactSelectionExpressionInAst(public_lib::Context *ctx, TextRange selection)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return nullptr;
    }
    ir::AstNode *matched = nullptr;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (matched != nullptr || node == nullptr || !node->IsExpression()) {
            return false;
        }
        if (node->Start().index == selection.pos && node->End().index == selection.end) {
            matched = node;
            return true;
        }
        return false;
    });
    return matched;
}

static ir::AstNode *FindExactSelectionExpressionFromTouch(const RefactorContext &context, TextRange selection)
{
    if (auto *touch = GetTouchingTokenByRange(context.context, selection, false); touch != nullptr) {
        if (auto *optimum = GetOptimumNodeByRange(touch, selection); optimum != nullptr) {
            touch = optimum;
        }
        for (auto *current = touch; current != nullptr; current = current->Parent()) {
            if (current->IsExpression() && current->Start().index == selection.pos &&
                current->End().index == selection.end) {
                return current;
            }
        }
    }
    return nullptr;
}

ir::AstNode *FindExactSelectionExpression(const RefactorContext &context, TextRange selection)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ir::AstNode *matched = FindExactSelectionExpressionInAst(ctx, selection); matched != nullptr) {
        return matched;
    }
    return FindExactSelectionExpressionFromTouch(context, selection);
}

bool IsArrowFunctionSelection(const RefactorContext &context)
{
    const TextRange trimmedSpan = GetTrimmedSelectionSpan(context);
    auto *exactExpr = FindExactSelectionExpression(context, trimmedSpan);
    return exactExpr != nullptr && exactExpr->IsArrowFunctionExpression();
}

ir::VariableDeclarator *FindContainingDeclaratorByRange(const RefactorContext &context, TextRange selection)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return nullptr;
    }
    ir::VariableDeclarator *best = nullptr;
    size_t bestSpan = std::numeric_limits<size_t>::max();
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (node == nullptr || !node->IsVariableDeclarator()) {
            return false;
        }
        auto *decl = node->AsVariableDeclarator();
        auto *init = decl == nullptr ? nullptr : decl->Init();
        if (init == nullptr) {
            return false;
        }
        if (init->Start().index > selection.pos || init->End().index < selection.end) {
            return false;
        }
        const size_t span = init->End().index - init->Start().index;
        if (span < bestSpan) {
            best = decl;
            bestSpan = span;
        }
        return false;
    });
    return best;
}

static ir::ClassProperty *FindContainingClassPropertyByRange(const RefactorContext &context, TextRange selection)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return nullptr;
    }
    ir::ClassProperty *best = nullptr;
    size_t bestSpan = std::numeric_limits<size_t>::max();
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (node == nullptr || !node->IsClassProperty()) {
            return false;
        }
        auto *prop = node->AsClassProperty();
        auto *value = prop == nullptr ? nullptr : prop->Value();
        if (value == nullptr) {
            return false;
        }
        if (value->Start().index > selection.pos || value->End().index < selection.end) {
            return false;
        }
        const size_t span = value->End().index - value->Start().index;
        if (span < bestSpan) {
            best = prop;
            bestSpan = span;
        }
        return false;
    });
    return best;
}

static ir::AstNode *FindClassPropertyValueBySourceRange(const RefactorContext &context, TextRange selection)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || selection.end <= selection.pos ||
        selection.end > ctx->sourceFile->source.size()) {
        return nullptr;
    }
    std::string_view source = ctx->sourceFile->source;
    const size_t lineStart = FindLineStart(source, selection.pos);
    const size_t eqPos = source.rfind('=', selection.pos);
    if (eqPos == std::string_view::npos || eqPos < lineStart || eqPos >= selection.pos) {
        return nullptr;
    }
    if (auto *prop = FindContainingClassPropertyByRange(context, {eqPos + 1, selection.end});
        prop != nullptr && prop->Value() != nullptr) {
        return prop->Value();
    }
    return nullptr;
}

static bool IsClassPropertyInitializerRhsSelection(const RefactorContext &context, TextRange selection)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || selection.end <= selection.pos ||
        selection.end > ctx->sourceFile->source.size()) {
        return false;
    }
    std::string_view source = ctx->sourceFile->source;
    const size_t lineStart = FindLineStart(source, selection.pos);
    const size_t eqPos = source.rfind('=', selection.pos);
    if (eqPos == std::string_view::npos || eqPos < lineStart || eqPos >= selection.pos) {
        return false;
    }
    const std::string_view head = source.substr(lineStart, eqPos - lineStart);
    if (head.find("let ") != std::string_view::npos || head.find("const ") != std::string_view::npos) {
        return false;
    }
    const size_t semiPos = source.find(';', selection.end);
    if (semiPos == std::string_view::npos) {
        return false;
    }
    size_t rhsStart = eqPos + 1;
    while (rhsStart < semiPos && std::isspace(static_cast<unsigned char>(source[rhsStart])) != 0) {
        ++rhsStart;
    }
    size_t rhsEnd = semiPos;
    while (rhsEnd > rhsStart && std::isspace(static_cast<unsigned char>(source[rhsEnd - 1])) != 0) {
        --rhsEnd;
    }
    if (rhsStart != selection.pos || rhsEnd != selection.end) {
        return false;
    }
    return true;
}

static std::optional<size_t> FindDeclarationInsertPosFromNode(std::string_view source, ir::AstNode *node)
{
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsVariableDeclarator()) {
            if (auto keywordStart = FindVariableDeclKeywordStart(source, current->Start().index);
                keywordStart.has_value()) {
                return FindLineStart(source, keywordStart.value());
            }
            return FindLineStart(source, current->Start().index);
        }
        if (current->IsClassProperty()) {
            return FindLineStart(source, current->Start().index);
        }
    }
    return std::nullopt;
}

static std::optional<size_t> FindLastTopLevelVarDeclEndBefore(std::string_view source, size_t limit)
{
    size_t cursor = 0;
    std::optional<size_t> lastDeclEnd;
    int braceDepth = 0;
    bool inTopLevelVarDecl = false;
    const size_t selectionLineStart = FindLineStart(source, limit);
    while (cursor < limit) {
        size_t lineEnd = source.find('\n', cursor);
        if (lineEnd == std::string_view::npos || lineEnd > limit) {
            lineEnd = limit;
        }
        std::string_view line = source.substr(cursor, lineEnd - cursor);
        size_t begin = 0;
        while (begin < line.size() && std::isspace(static_cast<unsigned char>(line[begin])) != 0) {
            ++begin;
        }
        std::string_view trimmed = line.substr(begin);
        const bool isVarDecl = trimmed.rfind("let ", 0) == 0 || trimmed.rfind("const ", 0) == 0;
        if (braceDepth == 0 && cursor < selectionLineStart && isVarDecl &&
            trimmed.find('=') != std::string_view::npos && trimmed.find('{') == std::string_view::npos) {
            inTopLevelVarDecl = true;
        }
        if (braceDepth == 0 && inTopLevelVarDecl && cursor < selectionLineStart &&
            trimmed.find(';') != std::string_view::npos) {
            lastDeclEnd = lineEnd;
            inTopLevelVarDecl = false;
        }
        for (char ch : line) {
            if (ch == '{') {
                ++braceDepth;
            } else if (ch == '}') {
                --braceDepth;
            }
        }
        if (braceDepth != 0) {
            inTopLevelVarDecl = false;
        }
        if (lineEnd >= limit) {
            break;
        }
        cursor = lineEnd + 1;
    }
    return lastDeclEnd;
}

static std::string PrepareValueDeclarationInsertText(const RefactorContext &context, std::string_view source,
                                                     size_t insertPos, std::string generatedText)
{
    if (generatedText.empty() || insertPos >= source.size()) {
        return generatedText;
    }
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    if (IsLineBreakChar(source[insertPos])) {
        if (!IsLineBreakChar(generatedText.front())) {
            generatedText.insert(0, newLine);
        }
        return generatedText;
    }
    if (!IsLineBreakChar(generatedText.back())) {
        generatedText.append(newLine);
    }
    return generatedText;
}

static size_t AdvanceInsertPosToBlankLineStart(std::string_view source, size_t insertPos)
{
    if (insertPos + 1U >= source.size() || !IsLineBreakChar(source[insertPos]) ||
        !IsLineBreakChar(source[insertPos + 1U])) {
        return insertPos;
    }
    return insertPos + 1U;
}

static std::optional<size_t> FindLastTopLevelClassEndBefore(std::string_view source, size_t limit)
{
    size_t cursor = 0;
    std::optional<size_t> lastClassEnd;
    int braceDepth = 0;
    bool inClass = false;
    while (cursor < limit) {
        size_t lineEnd = source.find('\n', cursor);
        if (lineEnd == std::string_view::npos || lineEnd > limit) {
            lineEnd = limit;
        }
        std::string_view line = source.substr(cursor, lineEnd - cursor);
        size_t begin = 0;
        while (begin < line.size() && std::isspace(static_cast<unsigned char>(line[begin])) != 0) {
            ++begin;
        }
        if (braceDepth == 0 && line.substr(begin).rfind("class ", 0) == 0) {
            inClass = true;
        }
        for (char ch : line) {
            if (ch == '{') {
                ++braceDepth;
            } else if (ch == '}') {
                --braceDepth;
            }
        }
        if (inClass && braceDepth == 0) {
            lastClassEnd = lineEnd < source.size() ? lineEnd + 1 : lineEnd;
            inClass = false;
        }
        if (lineEnd >= limit) {
            break;
        }
        cursor = lineEnd + 1;
    }
    return lastClassEnd;
}

static std::optional<size_t> FindFileHeaderBlockCommentEnd(std::string_view source)
{
    size_t pos = 0;
    while (pos < source.size() && IsLineBreakChar(source[pos])) {
        ++pos;
    }
    if (pos + 1 >= source.size() || source[pos] != '/' || source[pos + 1] != '*') {
        return std::nullopt;
    }
    const size_t commentEnd = source.find("*/", pos + 2);
    if (commentEnd == std::string_view::npos) {
        return std::nullopt;
    }
    return ExtendToLineEnd(util::StringView(source), commentEnd + std::string_view("*/").size());
}

static std::optional<size_t> FindLastVarDeclEndInRange(std::string_view source, size_t begin, size_t limit)
{
    size_t cursor = begin;
    std::optional<size_t> lastDeclEnd;
    int braceDepth = 0;
    const size_t selectionLineStart = FindLineStart(source, limit);
    while (cursor < limit) {
        size_t lineEnd = source.find('\n', cursor);
        if (lineEnd == std::string_view::npos || lineEnd > limit) {
            lineEnd = limit;
        }
        std::string_view line = source.substr(cursor, lineEnd - cursor);
        size_t textBegin = 0;
        while (textBegin < line.size() && std::isspace(static_cast<unsigned char>(line[textBegin])) != 0) {
            ++textBegin;
        }
        std::string_view trimmed = line.substr(textBegin);
        const bool isVarDecl = trimmed.rfind("let ", 0) == 0 || trimmed.rfind("const ", 0) == 0 ||
                               trimmed.rfind("export const ", 0) == 0 || trimmed.rfind("export let ", 0) == 0;
        if (braceDepth == 0 && cursor < selectionLineStart && isVarDecl &&
            trimmed.find('=') != std::string_view::npos) {
            lastDeclEnd = lineEnd < source.size() ? lineEnd + 1 : lineEnd;
        }
        for (char ch : line) {
            if (ch == '{') {
                ++braceDepth;
            } else if (ch == '}') {
                --braceDepth;
            }
        }
        if (lineEnd >= limit) {
            break;
        }
        cursor = lineEnd + 1;
    }
    return lastDeclEnd;
}

static ir::AstNode *ResolveGlobalConstantSelectionExpression(const RefactorContext &context, TextRange trimmedSpan)
{
    if (IsSelectionInsideTypeAnnotationContext(context, trimmedSpan) &&
        IsQuotedLiteralSelectionText(context, trimmedSpan)) {
        if (auto laterSpan = FindLaterMatchingLiteralInitializerSpan(context, trimmedSpan); laterSpan.has_value()) {
            if (auto *initializerExpr = ResolveInitializerExpressionContainingSelection(context, laterSpan.value());
                initializerExpr != nullptr) {
                return initializerExpr;
            }
        }
    }
    if (auto *initializerExpr = ResolveInitializerExpressionContainingSelection(context, trimmedSpan);
        initializerExpr != nullptr) {
        return initializerExpr;
    }
    if (auto *prop = FindContainingClassPropertyByRange(context, trimmedSpan);
        prop != nullptr && prop->Value() != nullptr) {
        return prop->Value();
    }
    if (auto *propValue = FindClassPropertyValueBySourceRange(context, trimmedSpan); propValue != nullptr) {
        return propValue;
    }
    if (auto *coverExpr = ResolveExpressionCoveringRange(context, trimmedSpan);
        coverExpr != nullptr && coverExpr->IsExpression() && coverExpr->Start().index <= trimmedSpan.pos &&
        coverExpr->End().index >= trimmedSpan.end) {
        return coverExpr;
    }
    return FindExactSelectionExpression(context, trimmedSpan);
}

static std::optional<TextRange> FindLaterMatchingLiteralInitializerSpan(const RefactorContext &context,
                                                                        TextRange trimmedSpan)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || trimmedSpan.end <= trimmedSpan.pos ||
        trimmedSpan.end > ctx->sourceFile->source.size()) {
        return std::nullopt;
    }
    const std::string_view source = ctx->sourceFile->source;
    const std::string_view literal = source.substr(trimmedSpan.pos, trimmedSpan.end - trimmedSpan.pos);
    size_t searchPos = trimmedSpan.end;
    while (searchPos < source.size()) {
        size_t nextPos = source.find(literal, searchPos);
        if (nextPos == std::string_view::npos) {
            return std::nullopt;
        }
        TextRange candidate {nextPos, nextPos + literal.size()};
        if (ResolveInitializerExpressionContainingSelection(context, candidate) != nullptr ||
            FindContainingDeclaratorByRange(context, candidate) != nullptr ||
            FindContainingClassPropertyByRange(context, candidate) != nullptr) {
            return candidate;
        }
        searchPos = nextPos + 1;
    }
    return std::nullopt;
}

static std::optional<TextRange> ResolveDeclarationInitializerRangeFromText(std::string_view sourceText,
                                                                           TextRange selection)
{
    const size_t stmtStart = sourceText.rfind('\n', selection.pos);
    const size_t probeStart = (stmtStart == std::string_view::npos) ? 0 : (stmtStart + 1);
    const size_t eqPos = sourceText.rfind('=', selection.pos);
    if (eqPos == std::string_view::npos || eqPos < probeStart) {
        return std::nullopt;
    }
    const std::string_view head = sourceText.substr(probeStart, eqPos - probeStart);
    if (head.find("let ") == std::string_view::npos && head.find("const ") == std::string_view::npos) {
        return std::nullopt;
    }
    const size_t semicolonPos = sourceText.find(';', selection.end);
    if (semicolonPos == std::string_view::npos || semicolonPos <= eqPos + 1) {
        return std::nullopt;
    }
    size_t initStart = eqPos + 1;
    while (initStart < semicolonPos && std::isspace(static_cast<unsigned char>(sourceText[initStart])) != 0) {
        ++initStart;
    }
    size_t initEnd = semicolonPos;
    while (initEnd > initStart && std::isspace(static_cast<unsigned char>(sourceText[initEnd - 1])) != 0) {
        --initEnd;
    }
    if (initStart >= initEnd || initStart > selection.pos || initEnd < selection.end) {
        return std::nullopt;
    }
    return TextRange {initStart, initEnd};
}

ir::AstNode *ResolveExpressionCoveringRange(const RefactorContext &context, TextRange initRange)
{
    if (auto *exactExpr = FindExactSelectionExpression(context, initRange); exactExpr != nullptr) {
        return exactExpr;
    }
    auto *node = GetTouchingTokenByRange(context.context, initRange, false);
    if (node != nullptr) {
        if (auto *optimum = GetOptimumNodeByRange(node, initRange); optimum != nullptr) {
            node = optimum;
        }
    }
    while (node != nullptr &&
           (!node->IsExpression() || node->Start().index > initRange.pos || node->End().index < initRange.end)) {
        node = node->Parent();
    }
    return node;
}

static ir::AstNode *ResolveDeclarationInitializerByRange(const RefactorContext &context, TextRange selection)
{
    if (auto *decl = FindContainingDeclaratorByRange(context, selection); decl != nullptr && decl->Init() != nullptr) {
        return decl->Init();
    }
    if (auto *initializerExpr = ResolveInitializerExpressionContainingSelection(context, selection);
        initializerExpr != nullptr) {
        return initializerExpr;
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || selection.end <= selection.pos ||
        selection.end > ctx->sourceFile->source.size()) {
        return nullptr;
    }

    std::string_view source = ctx->sourceFile->source;
    const auto initRangeOpt = ResolveDeclarationInitializerRangeFromText(source, selection);
    if (!initRangeOpt.has_value()) {
        return nullptr;
    }
    return ResolveExpressionCoveringRange(context, initRangeOpt.value());
}

static ir::AstNode *NormalizeGlobalConstantExtractedNode(const RefactorContext &context, ir::AstNode *node)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (node->IsExpression()) {
        return node;
    }
    if (node->IsVariableDeclarator()) {
        auto *init = node->AsVariableDeclarator()->Init();
        return init == nullptr ? node : init;
    }
    if (node->IsVariableDeclaration()) {
        auto &declarators = node->AsVariableDeclaration()->Declarators();
        if (!declarators.empty() && declarators.front() != nullptr && declarators.front()->Init() != nullptr) {
            return declarators.front()->Init();
        }
        return node;
    }
    if (node->IsClassProperty()) {
        auto *value = node->AsClassProperty()->Value();
        return value == nullptr ? node : value;
    }
    if (auto *initializerExpr = ResolveDeclarationInitializerByRange(context, GetTrimmedSelectionSpan(context));
        initializerExpr != nullptr) {
        return initializerExpr;
    }
    return node;
}

std::optional<TextRange> ResolveInitializerRhsRange(const RefactorContext &context, TextRange hint)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || hint.end <= hint.pos ||
        hint.end > ctx->sourceFile->source.size()) {
        return std::nullopt;
    }
    const std::string_view source = ctx->sourceFile->source;
    auto findInitializerAssign = [source, hint]() -> std::optional<size_t> {
        size_t eqPos = source.rfind('=', hint.pos);
        while (eqPos != std::string_view::npos && eqPos + 1 < source.size() && source[eqPos + 1] == '>') {
            if (eqPos == 0) {
                return std::nullopt;
            }
            eqPos = source.rfind('=', eqPos - 1);
        }
        return eqPos == std::string_view::npos ? std::nullopt : std::optional<size_t>(eqPos);
    };
    auto hasDeclarationKeywordBeforeEq = [source, hint](size_t eqPos) -> bool {
        const size_t stmtStart = source.rfind('\n', hint.pos);
        const size_t probeStart = (stmtStart == std::string_view::npos) ? 0 : (stmtStart + 1);
        const std::string_view head = source.substr(probeStart, eqPos - probeStart);
        return head.find("let ") != std::string_view::npos || head.find("const ") != std::string_view::npos;
    };
    auto buildRhsRange = [source, hint](size_t eqPos) -> std::optional<TextRange> {
        const size_t semiPos = source.find(';', hint.end);
        if (semiPos == std::string_view::npos || semiPos <= eqPos + 1) {
            return std::nullopt;
        }
        size_t rhsStart = eqPos + 1;
        while (rhsStart < semiPos && std::isspace(static_cast<unsigned char>(source[rhsStart])) != 0) {
            ++rhsStart;
        }
        size_t rhsEnd = semiPos;
        while (rhsEnd > rhsStart && std::isspace(static_cast<unsigned char>(source[rhsEnd - 1])) != 0) {
            --rhsEnd;
        }
        if (rhsStart >= rhsEnd) {
            return std::nullopt;
        }
        return TextRange {rhsStart, rhsEnd};
    };

    auto eqPos = findInitializerAssign();
    if (!eqPos.has_value() || !hasDeclarationKeywordBeforeEq(eqPos.value())) {
        return std::nullopt;
    }
    return buildRhsRange(eqPos.value());
}

static ir::AstNode *ResolveInitializerExpressionForSelectionState(const RefactorContext &context, TextRange trimmedSpan,
                                                                  const ir::AstNode *wholeDeclSelectionNode)
{
    if (wholeDeclSelectionNode != nullptr) {
        return ResolveInitializerExpressionFromDeclarationSelection(context, trimmedSpan);
    }
    if (auto *candidate = ResolveDeclarationInitializerByRange(context, trimmedSpan);
        candidate != nullptr && candidate->Start().index == trimmedSpan.pos &&
        candidate->End().index == trimmedSpan.end) {
        return candidate;
    }
    if (auto rhsRange = ResolveInitializerRhsRange(context, trimmedSpan);
        rhsRange.has_value() && rhsRange->pos == trimmedSpan.pos && rhsRange->end == trimmedSpan.end) {
        return FindExactSelectionExpression(context, rhsRange.value());
    }
    auto *initializer = ResolveInitializerExpressionContainingSelection(context, trimmedSpan);
    auto *selectionExpr = FindExactSelectionExpression(context, trimmedSpan);
    if (selectionExpr == nullptr) {
        selectionExpr = ResolveExpressionCoveringRange(context, trimmedSpan);
    }
    if (initializer != nullptr && selectionExpr != nullptr && initializer->Start().index <= trimmedSpan.pos &&
        initializer->End().index >= trimmedSpan.end) {
        return selectionExpr;
    }
    return nullptr;
}

static void InitializeRefactorSelectionState(const RefactorContext &context, public_lib::Context *ctx,
                                             RefactorSelectionState &state)
{
    state.trimmedSpan = context.span;
    if (ctx != nullptr && ctx->sourceFile != nullptr) {
        std::string_view source = ctx->sourceFile->source;
        state.selectionHasNewline = HasSelectionNewline(context, source);
        state.trimmedSpan = GetTrimmedSelectionSpan(context);
    }
    state.wholeDeclSelectionNode = FindWholeVariableDeclarationSelectionNode(context, state.trimmedSpan);
    state.declInitializerExpr =
        ResolveInitializerExpressionForSelectionState(context, state.trimmedSpan, state.wholeDeclSelectionNode);
    if (state.wholeDeclSelectionNode != nullptr ||
        (state.declInitializerExpr != nullptr && state.declInitializerExpr->IsExpression())) {
        state.canExtractFunctionBySelectionShape = true;
    }
    if (state.selectionHasNewline && state.wholeDeclSelectionNode == nullptr && state.declInitializerExpr == nullptr) {
        state.canExtractFunctionBySelectionShape = FindStatementOverlappingSelection(ctx, state.trimmedSpan) != nullptr;
    }
}

static bool ResolveRefactorSelectionNode(const RefactorContext &context, public_lib::Context *ctx,
                                         RefactorSelectionState &state)
{
    state.node = ResolveNodeForSelection(context, ctx, state.selectionHasNewline, state.trimmedSpan);
    if (state.node == nullptr) {
        return false;
    }
    if (!state.canExtractFunctionBySelectionShape && state.selectionHasNewline && state.node->IsExpression()) {
        const bool spanWithinExpression =
            state.node->Start().index <= state.trimmedSpan.pos && state.node->End().index >= state.trimmedSpan.end;
        if (spanWithinExpression) {
            state.canExtractFunctionBySelectionShape = true;
        }
    }
    if (state.declInitializerExpr != nullptr) {
        state.node = state.declInitializerExpr;
    } else if (state.wholeDeclSelectionNode != nullptr) {
        state.node = const_cast<ir::AstNode *>(state.wholeDeclSelectionNode);
    }
    return !IsImportSelectionNode(state.node) && !HasImportDeclarationOverlap(context, state.trimmedSpan);
}

static bool AdjustSelectionNodeToExtractionRange(const RefactorContext &context, public_lib::Context *ctx,
                                                 RefactorSelectionState &state)
{
    const auto positions = GetCallPositionOfExtraction(context);
    if (IsInsideExtractionRange(state.node, positions)) {
        return true;
    }
    if (state.wholeDeclSelectionNode != nullptr) {
        state.node = const_cast<ir::AstNode *>(state.wholeDeclSelectionNode);
        return true;
    }
    if (!state.selectionHasNewline) {
        return false;
    }
    auto *statementInRange = FindStatementOverlappingSelection(ctx, positions);
    if (statementInRange == nullptr) {
        return false;
    }
    state.node = statementInRange;
    state.allowVariableActions = false;
    return true;
}

static bool IsContainedLiteralSelectionNode(ir::AstNode *node, const RefactorContext &context, TextRange trimmedSpan)
{
    if (node == nullptr) {
        return false;
    }
    const bool isLiteralNode = node->IsNumberLiteral() || node->IsStringLiteral() || node->IsBooleanLiteral() ||
                               node->IsNullLiteral() || node->IsCharLiteral();
    if (!isLiteralNode) {
        return false;
    }
    return node->Start().index <= trimmedSpan.pos && node->End().index >= trimmedSpan.end &&
           IsTopLevelLiteralSelectionText(context, trimmedSpan);
}

static bool HasCoveringExpressionFromRange(const RefactorContext &context, const RefactorSelectionState &state)
{
    if (state.selectionHasNewline) {
        return false;
    }
    auto *rangeTouch = GetTouchingTokenByRange(context.context, state.trimmedSpan, false);
    for (auto *current = rangeTouch; current != nullptr; current = current->Parent()) {
        if (current->IsExpression() && current->Start().index <= state.trimmedSpan.pos &&
            current->End().index >= state.trimmedSpan.end) {
            return true;
        }
    }
    return false;
}

static void UpdateVariableActionAvailabilityForSelection(const RefactorContext &context, RefactorSelectionState &state)
{
    if (state.declInitializerExpr != nullptr) {
        // Selection is inside a declaration initializer; keep variable/constant actions available.
        return;
    }
    if (context.span.pos == context.span.end) {
        return;
    }
    if (state.trimmedSpan.pos == state.trimmedSpan.end) {
        state.allowVariableActions = false;
        return;
    }
    if (FindExactSelectionExpression(context, state.trimmedSpan) != nullptr) {
        return;
    }
    const bool matchesTrimmedSpan =
        state.node->Start().index == state.trimmedSpan.pos && state.node->End().index == state.trimmedSpan.end;
    const bool isContainedLiteralSelection = IsContainedLiteralSelectionNode(state.node, context, state.trimmedSpan);
    const bool hasExactExpressionSelection = FindExactSelectionExpression(context, state.trimmedSpan) != nullptr;
    const bool hasContainingInitializer =
        ResolveInitializerExpressionContainingSelection(context, state.trimmedSpan) != nullptr;
    const bool isSingleLineContainedSelection = !state.selectionHasNewline &&
                                                state.node->Start().index <= state.trimmedSpan.pos &&
                                                state.node->End().index >= state.trimmedSpan.end;
    const bool selectionExtendsBeyondNode =
        state.node->Start().index > state.trimmedSpan.pos || state.node->End().index < state.trimmedSpan.end;
    const bool isSingleLineWiderExpressionSelection =
        !state.selectionHasNewline && state.node->IsExpression() && selectionExtendsBeyondNode;
    const bool hasCoveringExpressionFromRange = HasCoveringExpressionFromRange(context, state);
    if (!matchesTrimmedSpan && !isContainedLiteralSelection && !hasExactExpressionSelection &&
        !hasContainingInitializer && !isSingleLineContainedSelection && !isSingleLineWiderExpressionSelection &&
        !hasCoveringExpressionFromRange && state.wholeDeclSelectionNode == nullptr) {
        state.allowVariableActions = false;
    }
}

static bool TryResolveRefactorSelectionState(const RefactorContext &context, public_lib::Context *ctx,
                                             RefactorSelectionState &state)
{
    InitializeRefactorSelectionState(context, ctx, state);
    if (!ResolveRefactorSelectionNode(context, ctx, state)) {
        return false;
    }
    if (!AdjustSelectionNodeToExtractionRange(context, ctx, state)) {
        return false;
    }
    UpdateVariableActionAvailabilityForSelection(context, state);
    return true;
}

static void AddExtractVariableActions(std::vector<RefactorAction> &actions, const ScopeContext &scope,
                                      bool disallowGlobalConstant)
{
    bool hasNamespaceScope = !scope.namespaceScopeNames.empty();
    bool isEncloseScopeAvailable = scope.hasEncloseScope || hasNamespaceScope;
    if (isEncloseScopeAvailable) {
        AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE);
        if (hasNamespaceScope) {
            if (scope.hasClassScope && !scope.namespaceScopeNames.empty()) {
                AddRefactorAction(
                    actions, EXTRACT_CONSTANT_ACTION_ENCLOSE,
                    BuildNamedScopeDescription("constant", "namespace", scope.namespaceScopeNames.front()));
            } else {
                AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_ENCLOSE);
            }
            for (size_t namespaceDepth = 1; namespaceDepth < scope.namespaceScopeNames.size(); ++namespaceDepth) {
                AddRefactorAction(
                    actions, EXTRACT_CONSTANT_ACTION_ENCLOSE,
                    BuildNamedScopeDescription("constant", "namespace", scope.namespaceScopeNames[namespaceDepth]),
                    BuildNamespaceScopedActionName(EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX, namespaceDepth),
                    std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.kind));
            }
        } else {
            AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_ENCLOSE);
        }
    } else {
        AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_GLOBAL);
    }
    if (scope.hasClassScope) {
        AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_CLASS,
                          BuildNamedScopeDescription("variable", "class", scope.classScopeName));
        AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_CLASS,
                          BuildNamedScopeDescription("constant", "class", scope.classScopeName));
    }
    if (!disallowGlobalConstant) {
        AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_GLOBAL);
    }
}

static bool IsObjectLiteralSelection(public_lib::Context *ctx, TextRange trimmedSpan)
{
    return ctx != nullptr && ctx->sourceFile != nullptr && trimmedSpan.pos < trimmedSpan.end &&
           trimmedSpan.end <= ctx->sourceFile->source.size() && ctx->sourceFile->source[trimmedSpan.pos] == '{' &&
           ctx->sourceFile->source[trimmedSpan.end - 1] == '}';
}

struct RefactorAvailabilityFlags {
    bool containsThisOrSuper {false};
    bool hasNamespacePrivateFunctionDependency {false};
    bool hasNamespacePrivateTypeAnnotationDependency {false};
    bool hasNamespacePrivateDependency {false};
    bool hasLocalValueDependency {false};
    bool hasExternalLocalWriteDependency {false};
    bool disallowGlobalFunctionForDeclarationLeadingExternalWrite {false};
    bool disallowGlobalFunctionForLocalClassExpression {false};
    bool isObjectLiteralSelection {false};
    bool disallowGlobalFunctionForNamespaceObjectLiteral {false};
    bool hasUseStaticDirective {false};
    bool disallowGlobalConstantForNamespaceObjectLiteral {false};
};

static bool IsInsideLocalClassExpression(ir::AstNode *node)
{
    auto *classDef = FindEnclosingClassDefinition(node);
    if (classDef == nullptr || classDef->IsGlobal() || classDef->IsNamespaceTransformed()) {
        return false;
    }
    if (classDef->Parent() != nullptr && classDef->Parent()->IsClassExpression()) {
        return true;
    }
    return classDef->IsAnonymous() || classDef->IsLocal();
}

static bool HasClassExpressionAncestor(const ir::AstNode *node)
{
    for (auto *current = node == nullptr ? nullptr : node->Parent(); current != nullptr; current = current->Parent()) {
        if (current->IsClassExpression()) {
            return true;
        }
    }
    return false;
}

static bool MethodBodyContainsSelection(ir::AstNode *node, TextRange trimmed)
{
    if (node == nullptr || !node->IsMethodDefinition()) {
        return false;
    }
    auto *method = node->AsMethodDefinition();
    auto *func = method == nullptr ? nullptr : method->Function();
    auto *body = func == nullptr ? nullptr : func->Body();
    return body != nullptr && body->Start().index <= trimmed.pos && body->End().index >= trimmed.end;
}

static bool IsSelectionInsideClassExpressionMethodByAst(public_lib::Context *ctx, TextRange trimmed)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return false;
    }
    bool found = false;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (found || !MethodBodyContainsSelection(node, trimmed)) {
            return false;
        }
        found = HasClassExpressionAncestor(node);
        return found;
    });
    return found;
}

static bool IsAnonymousClassAssignmentSelection(std::string_view source, TextRange trimmed)
{
    const size_t classPos = source.rfind("class", trimmed.pos);
    if (classPos == std::string_view::npos) {
        return false;
    }
    const size_t bracePos = source.find('{', classPos);
    if (bracePos == std::string_view::npos || bracePos >= trimmed.pos) {
        return false;
    }
    const std::string_view between = source.substr(classPos + std::string_view("class").size(),
                                                   bracePos - classPos - std::string_view("class").size());
    if (!TrimAsciiWhitespace(between).empty()) {
        return false;
    }
    const size_t lineStartPos = source.rfind('\n', classPos);
    const size_t lineStart = (lineStartPos == std::string_view::npos) ? 0 : (lineStartPos + 1);
    const size_t eqPos = source.rfind('=', classPos);
    return eqPos != std::string_view::npos && eqPos >= lineStart;
}

static bool IsSelectionInsideClassExpressionMethod(const RefactorContext &context)
{
    const TextRange trimmed = GetTrimmedSelectionSpan(context);
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return false;
    }
    if (IsSelectionInsideClassExpressionMethodByAst(ctx, trimmed)) {
        return true;
    }
    return IsAnonymousClassAssignmentSelection(ctx->sourceFile->source, trimmed);
}

static RefactorAvailabilityFlags ResolveRefactorAvailabilityFlags(const RefactorContext &context,
                                                                  public_lib::Context *ctx,
                                                                  const RefactorSelectionState &state,
                                                                  const ScopeContext &scope)
{
    RefactorAvailabilityFlags flags;
    flags.containsThisOrSuper =
        ContainsThisOrSuperExpression(state.node) || ContainsThisOrSuperInRange(ctx, state.trimmedSpan);
    flags.hasNamespacePrivateFunctionDependency =
        HasUnexportedNamespaceInterfaceDependencyInSelection(context, state.trimmedSpan);
    flags.hasNamespacePrivateTypeAnnotationDependency =
        HasNamespacePrivateTypeAnnotationDependencyForExpression(context, state.trimmedSpan);
    const TextRange dependencySpan = ResolveAvailabilityDependencySpan(state);
    flags.hasNamespacePrivateDependency =
        HasNamespacePrivateSymbolDependencyForGlobalExtraction(context, dependencySpan);
    flags.hasLocalValueDependency = HasLocalValueDependencyInSelection(context, dependencySpan);
    ApplyAvailabilityLocalDependencyAdjustments(flags, context, state, scope);
    flags.hasExternalLocalWriteDependency = HasExternalLocalWriteDependencyInSelection(context, state.trimmedSpan);
    flags.disallowGlobalFunctionForDeclarationLeadingExternalWrite =
        ResolveGlobalFunctionExternalWriteDisallowFlag(flags, context, ctx, state);
    flags.disallowGlobalFunctionForLocalClassExpression = IsInsideLocalClassExpression(state.node);
    flags.isObjectLiteralSelection = IsObjectLiteralSelection(ctx, state.trimmedSpan);
    flags.disallowGlobalFunctionForNamespaceObjectLiteral =
        flags.isObjectLiteralSelection && !scope.namespaceScopeNames.empty();
    flags.hasUseStaticDirective =
        ctx != nullptr && ctx->sourceFile != nullptr && HasUseStaticDirective(ctx->sourceFile->source);
    flags.disallowGlobalConstantForNamespaceObjectLiteral =
        flags.isObjectLiteralSelection && !flags.hasUseStaticDirective;
    return flags;
}

static TextRange ResolveAvailabilityDependencySpan(const RefactorSelectionState &state)
{
    if (state.declInitializerExpr == nullptr) {
        return state.trimmedSpan;
    }
    return {state.declInitializerExpr->Start().index, state.declInitializerExpr->End().index};
}

static void ApplyAvailabilityLocalDependencyAdjustments(RefactorAvailabilityFlags &flags,
                                                        const RefactorContext &context,
                                                        const RefactorSelectionState &state, const ScopeContext &scope)
{
    const bool isModuleLevelSelection =
        FindScriptFunction(state.node) == nullptr && !scope.hasClassScope && scope.namespaceScopeNames.empty();
    const bool isTopLevelScriptSelection = isModuleLevelSelection && !scope.hasEncloseScope;
    if (isTopLevelScriptSelection) {
        flags.hasLocalValueDependency = false;
    }
    const bool isSimpleModuleExpr =
        isModuleLevelSelection && state.node != nullptr &&
        (state.node->IsBinaryExpression() || state.node->IsMemberExpression() || state.node->IsCallExpression());
    if (isSimpleModuleExpr) {
        flags.hasLocalValueDependency = false;
    }
    if (!state.selectionHasNewline) {
        if (auto *exactExpr = FindExactSelectionExpression(context, state.trimmedSpan);
            exactExpr != nullptr && exactExpr->IsArrowFunctionExpression()) {
            flags.hasLocalValueDependency = false;
        }
    }
}

static bool ResolveGlobalFunctionExternalWriteDisallowFlag(const RefactorAvailabilityFlags &flags,
                                                           const RefactorContext &context, public_lib::Context *ctx,
                                                           const RefactorSelectionState &state)
{
    const bool isInsideFunction = FindScriptFunction(state.node) != nullptr;
    const bool hasDeclarationLeadingExternalWriteUsage =
        HasDeclarationLeadingExternalWriteUsage(context, ctx, state.trimmedSpan);
    const bool disallowMultilineDeclarationLeadingSelection = hasDeclarationLeadingExternalWriteUsage &&
                                                              isInsideFunction && state.selectionHasNewline &&
                                                              state.wholeDeclSelectionNode == nullptr;
    return hasDeclarationLeadingExternalWriteUsage &&
           ((flags.hasExternalLocalWriteDependency && (!state.selectionHasNewline || isInsideFunction)) ||
            disallowMultilineDeclarationLeadingSelection);
}

static bool HasActionNamed(const std::vector<RefactorAction> &actions, std::string_view name)
{
    return std::any_of(actions.begin(), actions.end(),
                       [name](const RefactorAction &action) { return action.name == std::string(name); });
}

static bool HasSelectionExpressionByRange(const RefactorContext &context, TextRange span)
{
    return FindExactSelectionExpression(context, span) != nullptr ||
           ResolveExpressionCoveringRange(context, span) != nullptr;
}

static bool CanExtractVariableByRange(const RefactorContext &context, TextRange span)
{
    if (HasSelectionExpressionByRange(context, span)) {
        return true;
    }
    if (auto *touch = GetTouchingTokenByRange(context.context, span, false); touch != nullptr) {
        for (auto *current = touch; current != nullptr; current = current->Parent()) {
            if (current->IsExpression() && current->Start().index <= span.pos && current->End().index >= span.end) {
                return true;
            }
        }
    }
    return ResolveInitializerExpressionContainingSelection(context, span) != nullptr;
}

static void RemoveValueActionsForDeclaratorTypeSelection(std::vector<RefactorAction> &actions,
                                                         const RefactorContext &context, TextRange span)
{
    auto *decl = FindContainingDeclaratorByRange(context, span);
    if (decl == nullptr || decl->Id() == nullptr) {
        return;
    }
    ir::TypeNode *typeAnno = TypeAnnoFromDeclaratorId(decl->Id());
    if (typeAnno == nullptr || typeAnno->Start().index > span.pos || typeAnno->End().index < span.end) {
        return;
    }
    RemoveActionByName(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    RemoveActionByName(actions, EXTRACT_VARIABLE_ACTION_CLASS.name);
    RemoveActionByName(actions, EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    RemoveActionByName(actions, EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    RemoveActionByName(actions, EXTRACT_CONSTANT_ACTION_GLOBAL.name);
}

static void EnsureArrowTopLevelGlobalConstantAction(std::vector<RefactorAction> &actions,
                                                    const RefactorContext &context, const RefactorSelectionState &state,
                                                    const ScopeContext &scope)
{
    if (state.selectionHasNewline) {
        return;
    }
    auto *exactExpr = FindExactSelectionExpression(context, state.trimmedSpan);
    if (exactExpr == nullptr || !exactExpr->IsArrowFunctionExpression() || FindScriptFunction(state.node) != nullptr ||
        scope.hasEncloseScope || !scope.namespaceScopeNames.empty()) {
        return;
    }
    if (!HasActionNamed(actions, EXTRACT_CONSTANT_ACTION_GLOBAL.name)) {
        AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_GLOBAL);
    }
}

static void EnsureVariableEncloseActionForScopedSelection(std::vector<RefactorAction> &actions,
                                                          const RefactorContext &context,
                                                          const RefactorSelectionState &state,
                                                          const ScopeContext &scope,
                                                          const RefactorAvailabilityFlags &flags)
{
    const bool hasVariableEncloseScope = scope.hasEncloseScope || !scope.namespaceScopeNames.empty();
    if (state.selectionHasNewline || !hasVariableEncloseScope ||
        HasActionNamed(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE.name)) {
        return;
    }
    if (CanExtractVariableByRange(context, state.trimmedSpan) && !flags.containsThisOrSuper) {
        AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE);
    }
}

static bool IsVariableEncloseScopeUsableInClass(const RefactorSelectionState &state)
{
    for (auto *current = state.node; current != nullptr; current = current->Parent()) {
        auto *parent = current->Parent();
        if (parent == nullptr || !parent->IsBlockStatement()) {
            continue;
        }
        if (parent->Start().index > state.trimmedSpan.pos || parent->End().index < state.trimmedSpan.end) {
            continue;
        }
        auto *owner = parent->Parent();
        return owner != nullptr && (owner->IsScriptFunction() || IsControlFlowEncloseScopeNode(owner));
    }
    return false;
}

static void ApplyClassVariableScopeRules(std::vector<RefactorAction> &actions, const RefactorSelectionState &state,
                                         const ScopeContext &scope)
{
    if (!scope.hasClassScope || IsVariableEncloseScopeUsableInClass(state)) {
        return;
    }
    RemoveActionByName(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    if (!HasActionNamed(actions, EXTRACT_VARIABLE_ACTION_CLASS.name)) {
        AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_CLASS,
                          BuildNamedScopeDescription("variable", "class", scope.classScopeName));
    }
}

static void ApplyInitializerLiteralSelectionRules(std::vector<RefactorAction> &actions, const RefactorContext &context,
                                                  const RefactorSelectionState &state, const ScopeContext &scope)
{
    if (state.selectionHasNewline) {
        return;
    }
    if (IsSelectionInsideUnterminatedDeclarationInitializer(context, state.trimmedSpan)) {
        RemoveActionByName(actions, EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    }
    if (!IsLiteralSelectionInsideDeclarationInitializer(context, state.trimmedSpan)) {
        return;
    }
    const bool hasEncloseScope = scope.hasEncloseScope || !scope.namespaceScopeNames.empty();
    if (hasEncloseScope) {
        return;
    }
    RemoveActionByName(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    if (!HasActionNamed(actions, EXTRACT_VARIABLE_ACTION_GLOBAL.name)) {
        AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_GLOBAL);
    }
}

static void RebalanceVariableActionByScope(std::vector<RefactorAction> &actions, const RefactorContext &context,
                                           const RefactorSelectionState &state, const ScopeContext &scope,
                                           const RefactorAvailabilityFlags &flags)
{
    const bool hasEncloseScope = scope.hasEncloseScope || !scope.namespaceScopeNames.empty();
    if (!state.selectionHasNewline && !hasEncloseScope) {
        RemoveActionByName(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
        if (!HasActionNamed(actions, EXTRACT_VARIABLE_ACTION_GLOBAL.name)) {
            AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_GLOBAL);
        }
    }
    if (state.selectionHasNewline || flags.containsThisOrSuper ||
        !HasSelectionExpressionByRange(context, state.trimmedSpan)) {
        return;
    }
    if (hasEncloseScope) {
        RemoveActionByName(actions, EXTRACT_VARIABLE_ACTION_GLOBAL.name);
        if (!HasActionNamed(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE.name)) {
            AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE);
        }
        return;
    }
    RemoveActionByName(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    if (!HasActionNamed(actions, EXTRACT_VARIABLE_ACTION_GLOBAL.name)) {
        AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_GLOBAL);
    }
}

static void AddBaseAvailableRefactors(std::vector<RefactorAction> &actions, const RefactorContext &context,
                                      const RefactorSelectionState &state, const ScopeContext &scope,
                                      const RefactorAvailabilityFlags &flags)
{
    AddAvailableFunctionRefactors(actions, context, scope, state.node,
                                  {state.canExtractFunctionBySelectionShape, flags.containsThisOrSuper,
                                   flags.hasNamespacePrivateFunctionDependency,
                                   flags.hasNamespacePrivateTypeAnnotationDependency,
                                   flags.disallowGlobalFunctionForDeclarationLeadingExternalWrite,
                                   flags.disallowGlobalFunctionForNamespaceObjectLiteral});
    if (flags.disallowGlobalFunctionForLocalClassExpression) {
        RemoveActionByName(actions, EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    }
    AddAvailableVariableRefactors(actions, scope, state.node,
                                  {state.allowVariableActions, flags.containsThisOrSuper, flags.hasUseStaticDirective,
                                   flags.disallowGlobalConstantForNamespaceObjectLiteral,
                                   flags.hasNamespacePrivateDependency, flags.hasLocalValueDependency,
                                   flags.hasNamespacePrivateFunctionDependency});
    ApplyObjectLiteralGlobalConstantRule(actions, flags.isObjectLiteralSelection, flags.hasUseStaticDirective);
    ApplySwitchCaseConstantRules(actions, state.node, context.span);
    ApplyTopLevelLiteralRule(actions, context, state.trimmedSpan, state.node);
}

static void ApplyAvailableRefactorPostRules(std::vector<RefactorAction> &actions, const RefactorContext &context,
                                            const RefactorSelectionState &state, const ScopeContext &scope,
                                            const RefactorAvailabilityFlags &flags)
{
    RemoveValueActionsForDeclaratorTypeSelection(actions, context, state.trimmedSpan);
    if (IsSelectionInsideTypeAnnotationContext(context, state.trimmedSpan) ||
        IsSelectionInsideTsConditionalType(context, state.trimmedSpan)) {
        RemoveActionByName(actions, EXTRACT_FUNCTION_ACTION_GLOBAL.name);
        RemoveActionByName(actions, EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
        RemoveActionByName(actions, EXTRACT_CONSTANT_ACTION_GLOBAL.name);
        RemoveActionByName(actions, EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
        RemoveActionByName(actions, EXTRACT_VARIABLE_ACTION_GLOBAL.name);
        RemoveActionByName(actions, EXTRACT_VARIABLE_ACTION_CLASS.name);
        RemoveActionByName(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
        return;
    }
    EnsureArrowTopLevelGlobalConstantAction(actions, context, state, scope);
    EnsureVariableEncloseActionForScopedSelection(actions, context, state, scope, flags);
    ApplyInitializerLiteralSelectionRules(actions, context, state, scope);
    RebalanceVariableActionByScope(actions, context, state, scope, flags);
    ApplyClassVariableScopeRules(actions, state, scope);
}

struct AvailableRefactorContext {
    RefactorSelectionState state {};
    ScopeContext scope {};
    RefactorAvailabilityFlags flags {};
};

static std::optional<AvailableRefactorContext> PrepareAvailableRefactorContext(const RefactorContext &context,
                                                                               public_lib::Context *ctx)
{
    AvailableRefactorContext prepared;
    if (!TryResolveRefactorSelectionState(context, ctx, prepared.state)) {
        return std::nullopt;
    }
    prepared.scope = ResolveScopeContext(prepared.state.node);
    prepared.flags = ResolveRefactorAvailabilityFlags(context, ctx, prepared.state, prepared.scope);
    return prepared;
}

std::vector<RefactorAction> FindAvailableRefactors(const RefactorContext &context)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    auto prepared = PrepareAvailableRefactorContext(context, ctx);
    if (!prepared.has_value()) {
        return {};
    }
    std::vector<RefactorAction> actions;
    AddBaseAvailableRefactors(actions, context, prepared->state, prepared->scope, prepared->flags);
    ApplyAvailableRefactorPostRules(actions, context, prepared->state, prepared->scope, prepared->flags);
    return actions;
}

ir::AstNode *FindRefactor(const RefactorContext &context, const std::string &actionName)
{
    if (IsConstantExtractionAction(actionName) || IsVariableExtractionAction(actionName)) {
        auto *extractedVals = FindExtractedVals(context);
        if (extractedVals != nullptr || !IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
            return extractedVals;
        }
        return ResolveGlobalConstantSelectionExpression(context, GetTrimmedSelectionSpan(context));
    }

    if (actionName == EXTRACT_FUNCTION_ACTION_GLOBAL.name || actionName == EXTRACT_FUNCTION_ACTION_CLASS.name ||
        IsNamespaceAction(actionName, EXTRACT_FUNCTION_ACTION_ENCLOSE.name, EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX)) {
        return FindExtractedFunction(context);
    }

    return nullptr;
}

std::string GetConstantString(std::string_view src, ir::AstNode *extractedText)
{
    if (extractedText == nullptr) {
        return "";
    }
    if (extractedText->IsVariableDeclaration()) {
        auto declarators = extractedText->AsVariableDeclaration()->Declarators();
        if (declarators.empty()) {
            return "";
        }
        auto init = declarators.front()->Init();
        if (init == nullptr || !init->IsExpression()) {
            return "";
        }
        return GetSourceTextOfNodeFromSourceFile(src, init);
    }
    if (extractedText->IsExpressionStatement()) {
        auto expression = extractedText->AsExpressionStatement()->GetExpression();
        if (expression == nullptr || !expression->IsExpression()) {
            return "";
        }
        return GetSourceTextOfNodeFromSourceFile(src, expression);
    }
    if (extractedText->IsMemberExpression()) {
        const size_t sizeOfPuncht = 2;
        size_t endPos = extractedText->AsMemberExpression()->End().index;
        if (extractedText->AsMemberExpression()->Object()->IsETSNewClassInstanceExpression()) {
            endPos = extractedText->AsMemberExpression()->End().index + sizeOfPuncht;
        }
        return std::string(src).substr(extractedText->AsMemberExpression()->Start().index,
                                       endPos - extractedText->AsMemberExpression()->Start().index);
    }
    if (extractedText != nullptr) {
        std::string strNow = GetSourceTextOfNodeFromSourceFile(src, extractedText);
        return strNow;
    }
    return "";
}

static ir::VariableDeclarator *FindEnclosingVariableDeclarator(ir::AstNode *node)
{
    for (ir::AstNode *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsVariableDeclarator()) {
            return current->AsVariableDeclarator();
        }
    }
    return nullptr;
}

static std::optional<std::pair<ir::VariableDeclarator *, ir::VariableDeclaration *>> ResolveInlineMultiDeclNodes(
    public_lib::Context *ctx, ir::AstNode *extractedText)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || extractedText == nullptr) {
        return std::nullopt;
    }
    auto *declarator = FindEnclosingVariableDeclarator(extractedText);
    if (declarator == nullptr) {
        return std::nullopt;
    }
    auto *parent = declarator->Parent();
    if (parent == nullptr || !parent->IsVariableDeclaration()) {
        return std::nullopt;
    }
    return std::make_pair(declarator, parent->AsVariableDeclaration());
}

static std::optional<std::pair<ir::VariableDeclarator *, ir::VariableDeclaration *>> ResolveInlineMultiDeclNodesBySpan(
    const RefactorContext &context, public_lib::Context *ctx)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return std::nullopt;
    }
    auto *declarator = FindContainingDeclaratorByRange(context, GetTrimmedSelectionSpan(context));
    if (declarator == nullptr) {
        auto *node = GetTouchingTokenByRange(context.context, context.span, false);
        declarator = FindEnclosingVariableDeclarator(node);
    }
    if (declarator == nullptr || declarator->Parent() == nullptr || !declarator->Parent()->IsVariableDeclaration()) {
        return std::nullopt;
    }
    return std::make_pair(declarator, declarator->Parent()->AsVariableDeclaration());
}

static std::optional<size_t> FindDeclaratorIndex(const ir::VariableDeclaration *declaration,
                                                 const ir::VariableDeclarator *declarator)
{
    constexpr size_t kMinDeclarators = 2;
    const auto &declarators = declaration->Declarators();
    if (declarators.size() < kMinDeclarators) {
        return std::nullopt;
    }
    for (size_t i = 0; i < declarators.size(); ++i) {
        if (declarators[i] == declarator) {
            return i == 0 ? std::nullopt : std::optional<size_t> {i};
        }
    }
    return std::nullopt;
}

static ir::VariableDeclarator *FindDeclaratorContainingSpan(const ir::VariableDeclaration *declaration, TextRange span)
{
    if (declaration == nullptr) {
        return nullptr;
    }
    for (auto *declarator : declaration->Declarators()) {
        if (declarator == nullptr || declarator->Init() == nullptr) {
            continue;
        }
        auto *init = declarator->Init();
        const size_t initStart = init->Start().index;
        const size_t initEnd = init->End().index;
        const bool contains = initStart <= span.pos && initEnd >= span.end;
        const bool overlaps = !(span.end <= initStart || span.pos >= initEnd);
        if (contains || overlaps) {
            return declarator;
        }
    }
    return nullptr;
}

static bool ResolveInlineInsertionTarget(const RefactorContext &context, public_lib::Context *ctx,
                                         ir::AstNode *extractedText, ir::VariableDeclarator *&declarator,
                                         ir::VariableDeclaration *&declaration)
{
    if (auto bySpan = ResolveInlineMultiDeclNodesBySpan(context, ctx); bySpan.has_value()) {
        declarator = bySpan->first;
        declaration = bySpan->second;
    }
    if (declarator != nullptr && declaration != nullptr) {
        return true;
    }
    if (auto nodes = ResolveInlineMultiDeclNodes(ctx, extractedText); nodes.has_value()) {
        declarator = nodes->first;
        declaration = nodes->second;
    }
    if (declarator == nullptr || declaration == nullptr) {
        if (extractedText != nullptr && extractedText->IsVariableDeclaration()) {
            declaration = extractedText->AsVariableDeclaration();
            declarator = FindDeclaratorContainingSpan(declaration, context.span);
        }
    }
    return declarator != nullptr && declaration != nullptr;
}

static std::optional<size_t> ResolveInlineDeclaratorIndex(ir::VariableDeclaration *declaration,
                                                          ir::VariableDeclarator *&declarator,
                                                          ir::AstNode *extractedText, TextRange span)
{
    auto declaratorIndex = FindDeclaratorIndex(declaration, declarator);
    if (declaratorIndex.has_value()) {
        return declaratorIndex;
    }
    if (extractedText != nullptr) {
        TextRange extractedTextRange {extractedText->Start().index, extractedText->End().index};
        if (auto *fallbackDeclarator = FindDeclaratorContainingSpan(declaration, extractedTextRange);
            fallbackDeclarator != nullptr) {
            declarator = fallbackDeclarator;
            declaratorIndex = FindDeclaratorIndex(declaration, declarator);
            if (declaratorIndex.has_value()) {
                return declaratorIndex;
            }
        }
    }
    if (auto *fallbackDeclarator = FindDeclaratorContainingSpan(declaration, span); fallbackDeclarator != nullptr) {
        declarator = fallbackDeclarator;
        return FindDeclaratorIndex(declaration, declarator);
    }
    return std::nullopt;
}

static std::optional<std::pair<size_t, std::string>> TryBuildInlineMultiDeclInsertion(const RefactorContext &context,
                                                                                      public_lib::Context *ctx,
                                                                                      ir::AstNode *extractedText,
                                                                                      const std::string &varName)
{
    ir::VariableDeclarator *declarator = nullptr;
    ir::VariableDeclaration *declaration = nullptr;
    if (!ResolveInlineInsertionTarget(context, ctx, extractedText, declarator, declaration)) {
        return std::nullopt;
    }
    auto declaratorIndex = ResolveInlineDeclaratorIndex(declaration, declarator, extractedText, context.span);
    if (!declaratorIndex.has_value()) {
        return std::nullopt;
    }
    const auto &source = ctx->sourceFile->source;
    std::string_view srcView(source);
    std::string placeholder = GetConstantString(srcView, extractedText);
    if (placeholder.empty()) {
        return std::nullopt;
    }

    const auto &declarators = declaration->Declarators();
    if (*declaratorIndex >= declarators.size() || declarators[*declaratorIndex] == nullptr) {
        return std::nullopt;
    }
    auto *targetDeclarator = declarators[*declaratorIndex];
    const size_t declaratorPos =
        targetDeclarator->Id() != nullptr ? targetDeclarator->Id()->Start().index : targetDeclarator->Start().index;
    std::string inlineText = varName + " = " + placeholder + ", ";
    if (!inlineText.empty() && std::isspace(static_cast<unsigned char>(inlineText.front())) != 0) {
        inlineText.erase(0, 1);
    }
    if (!inlineText.empty() && inlineText.front() == '\n') {
        inlineText.erase(0, 1);
    }
    return std::make_pair(declaratorPos, std::move(inlineText));
}

static bool GeneratedTextStartsWithVar(const std::string &generatedText, const std::string &uniqueVarName)
{
    if (generatedText.rfind(uniqueVarName, 0) == 0) {
        return true;
    }
    if (uniqueVarName.rfind("this.", 0) == 0) {
        const std::string suffix = uniqueVarName.substr(std::string("this.").size());
        return generatedText.rfind(suffix, 0) == 0;
    }
    return false;
}

static bool HasCommaBeforeWithNewline(std::string_view source, size_t insertPos)
{
    size_t probe = insertPos;
    bool sawNewline = false;
    bool commaBefore = false;
    while (probe > 0) {
        char ch = source[probe - 1];
        if (ch == '\n' || ch == '\r') {
            sawNewline = true;
            --probe;
            continue;
        }
        if (ch == ' ' || ch == '\t') {
            --probe;
            continue;
        }
        if (ch == ',') {
            commaBefore = sawNewline;
        }
        break;
    }
    return commaBefore;
}

struct InlineInsertionInputs {
    const RefactorContext &context;
    public_lib::Context *ctx {nullptr};
    ir::AstNode *extractedText {nullptr};
    const std::string &actionName;
    const std::string &uniqueVarName;
};

static std::optional<std::pair<size_t, std::string>> TryBuildInlineInsertion(const InlineInsertionInputs &inputs)
{
    if (IsActionNameOrKind(inputs.actionName, EXTRACT_VARIABLE_ACTION_GLOBAL) ||
        IsActionNameOrKind(inputs.actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
        return std::nullopt;
    }
    auto inlineInsertionResult =
        TryBuildInlineMultiDeclInsertion(inputs.context, inputs.ctx, inputs.extractedText, inputs.uniqueVarName);
    if (!inlineInsertionResult.has_value()) {
        return std::nullopt;
    }
    auto [inlinePos, inlineText] = std::move(inlineInsertionResult.value());
    return std::make_pair(inlinePos, std::move(inlineText));
}

static std::optional<size_t> FindCurrentDeclarationCommaInsertPos(std::string_view source, TextRange extractionSpan)
{
    if (extractionSpan.pos == 0 || extractionSpan.pos > source.size()) {
        return std::nullopt;
    }
    int parenDepth = 0;
    int bracketDepth = 0;
    for (size_t pos = extractionSpan.pos; pos > 0; --pos) {
        const char ch = source[pos - 1];
        if (IsDeclarationBoundaryChar(ch) || IsUnmatchedClosingDelimiter(ch, parenDepth, bracketDepth)) {
            return std::nullopt;
        }
        UpdateDelimiterDepths(ch, parenDepth, bracketDepth);
        if (ch == ',' && parenDepth == 0 && bracketDepth == 0) {
            return ResolveDeclarationCommaInsertCandidate(source, pos, extractionSpan.pos);
        }
    }
    return std::nullopt;
}

static bool IsDeclarationBoundaryChar(char ch)
{
    return ch == ';' || ch == '{' || ch == '}';
}

static bool IsUnmatchedClosingDelimiter(char ch, int parenDepth, int bracketDepth)
{
    return (ch == ')' && parenDepth == 0) || (ch == ']' && bracketDepth == 0);
}

static void UpdateDelimiterDepths(char ch, int &parenDepth, int &bracketDepth)
{
    if (ch == ')') {
        --parenDepth;
    } else if (ch == ']') {
        --bracketDepth;
    } else if (ch == '(') {
        ++parenDepth;
    } else if (ch == '[') {
        ++bracketDepth;
    }
}

static std::optional<size_t> ResolveDeclarationCommaInsertCandidate(std::string_view source, size_t commaPos,
                                                                    size_t selectionPos)
{
    const size_t lineStart = FindLineStart(source, commaPos - 1U);
    const std::string_view prefix(source.data() + lineStart, commaPos - lineStart);
    const bool hasDeclarationPrefix = prefix.find("const ") != std::string_view::npos ||
                                      prefix.find("let ") != std::string_view::npos ||
                                      prefix.find("var ") != std::string_view::npos;
    if (!hasDeclarationPrefix) {
        return std::nullopt;
    }
    size_t candidate = commaPos;
    while (candidate < selectionPos && std::isspace(static_cast<unsigned char>(source[candidate])) != 0) {
        ++candidate;
    }
    return candidate;
}

static void RemoveTrailingLineBreaks(std::string &text)
{
    while (!text.empty() && IsLineBreakChar(text.back())) {
        text.pop_back();
    }
}

static std::optional<RefactorEditInfo> TryBuildGlobalConstantMultiDeclaratorEdits(const RefactorContext &context,
                                                                                  public_lib::Context *ctx,
                                                                                  const std::string &actionName)
{
    auto inlineCandidate = ResolveInlineGlobalConstantMultiDeclaratorCandidate(context, ctx, actionName);
    if (!inlineCandidate.has_value()) {
        return std::nullopt;
    }
    if (inlineCandidate->declaration == nullptr ||
        inlineCandidate->declaration->Declarators().size() < MIN_INLINE_DECLARATORS) {
        return BuildSingleDeclaratorInlineGlobalConstantEdits(context, ctx, inlineCandidate.value());
    }
    return BuildMultiDeclaratorInlineGlobalConstantEdits(context, ctx, inlineCandidate.value());
}

static std::optional<InlineGlobalConstantMultiDeclaratorCandidate> ResolveInlineGlobalConstantMultiDeclaratorCandidate(
    const RefactorContext &context, public_lib::Context *ctx, const std::string &actionName)
{
    if (!IsConstantExtractionAction(actionName) || ctx == nullptr || ctx->sourceFile == nullptr) {
        return std::nullopt;
    }
    InlineGlobalConstantMultiDeclaratorCandidate candidate;
    candidate.trimmed = GetTrimmedSelectionSpan(context);
    candidate.source = ctx->sourceFile->source;
    if (candidate.trimmed.end <= candidate.trimmed.pos || candidate.trimmed.end > candidate.source.size()) {
        return std::nullopt;
    }
    candidate.uniqueVarName = GenerateUniqueExtractedVarName(context, actionName);
    candidate.placeholder.assign(
        candidate.source.substr(candidate.trimmed.pos, candidate.trimmed.end - candidate.trimmed.pos));
    if (candidate.placeholder.empty()) {
        return std::nullopt;
    }
    candidate.declarator = FindContainingDeclaratorByRange(context, candidate.trimmed);
    candidate.declaration = candidate.declarator != nullptr && candidate.declarator->Parent() != nullptr &&
                                    candidate.declarator->Parent()->IsVariableDeclaration()
                                ? candidate.declarator->Parent()->AsVariableDeclaration()
                                : nullptr;
    return candidate;
}

static std::optional<RefactorEditInfo> BuildGlobalConstantInlineEdits(
    const RefactorContext &context, public_lib::Context *ctx,
    const InlineGlobalConstantMultiDeclaratorCandidate &candidate, size_t insertPos)
{
    const std::string insertText = candidate.uniqueVarName + " = " + candidate.placeholder + ", ";
    TextChangesContext textChangesContext = *context.textChangesContext;
    auto edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(ctx->sourceFile, insertPos, insertText);
        tracker.ReplaceRangeWithText(ctx->sourceFile, candidate.trimmed, candidate.uniqueVarName);
    });
    const size_t renameLoc = candidate.trimmed.pos + insertText.size() + (candidate.uniqueVarName.size() > 1 ? 1 : 0);
    return RefactorEditInfo(std::move(edits), std::optional<std::string>(ctx->sourceFile->filePath),
                            std::optional<size_t>(renameLoc));
}

static std::optional<RefactorEditInfo> BuildSingleDeclaratorInlineGlobalConstantEdits(
    const RefactorContext &context, public_lib::Context *ctx,
    const InlineGlobalConstantMultiDeclaratorCandidate &candidate)
{
    if (auto insertPos = FindCurrentDeclarationCommaInsertPos(candidate.source, candidate.trimmed);
        insertPos.has_value()) {
        return BuildGlobalConstantInlineEdits(context, ctx, candidate, insertPos.value());
    }
    return std::nullopt;
}

static bool IsInlineMultiDeclaratorDeclarationPrefixValid(const InlineGlobalConstantMultiDeclaratorCandidate &candidate)
{
    if (candidate.declaration == nullptr || candidate.declaration->Start().index >= candidate.trimmed.pos) {
        return true;
    }
    const std::string_view prefix(candidate.source.data() + candidate.declaration->Start().index,
                                  candidate.trimmed.pos - candidate.declaration->Start().index);
    return prefix.find(';') == std::string_view::npos && prefix.find('{') == std::string_view::npos &&
           prefix.find('}') == std::string_view::npos;
}

static bool IsInlineMultiDeclaratorLineDeclaration(const InlineGlobalConstantMultiDeclaratorCandidate &candidate)
{
    const size_t lineStart = FindLineStart(candidate.source, candidate.trimmed.pos);
    size_t lineEnd = candidate.trimmed.end;
    while (lineEnd < candidate.source.size() && candidate.source[lineEnd] != ';' && candidate.source[lineEnd] != '\n' &&
           candidate.source[lineEnd] != '\r') {
        ++lineEnd;
    }
    const std::string_view line = candidate.source.substr(lineStart, lineEnd - lineStart);
    return line.find("const ") != std::string_view::npos || line.find("let ") != std::string_view::npos;
}

static std::optional<size_t> ResolveInlineMultiDeclaratorInsertPos(
    const InlineGlobalConstantMultiDeclaratorCandidate &candidate)
{
    if (candidate.declaration == nullptr || candidate.declarator == nullptr || candidate.declarator->Id() == nullptr) {
        return std::nullopt;
    }
    size_t declaratorIndex = 0;
    bool foundDeclarator = false;
    for (size_t i = 0; i < candidate.declaration->Declarators().size(); ++i) {
        if (candidate.declaration->Declarators()[i] == candidate.declarator) {
            declaratorIndex = i;
            foundDeclarator = true;
            break;
        }
    }
    if (!foundDeclarator || declaratorIndex == 0) {
        return std::nullopt;
    }
    const size_t lineStart = FindLineStart(candidate.source, candidate.trimmed.pos);
    const size_t insertPos = candidate.declarator->Id()->Start().index;
    if (insertPos <= lineStart || insertPos > candidate.trimmed.pos) {
        return std::nullopt;
    }
    return insertPos;
}

static std::optional<RefactorEditInfo> BuildMultiDeclaratorInlineGlobalConstantEdits(
    const RefactorContext &context, public_lib::Context *ctx,
    const InlineGlobalConstantMultiDeclaratorCandidate &candidate)
{
    if (!IsInlineMultiDeclaratorDeclarationPrefixValid(candidate) ||
        !IsInlineMultiDeclaratorLineDeclaration(candidate)) {
        return std::nullopt;
    }
    auto insertPos = ResolveInlineMultiDeclaratorInsertPos(candidate);
    return insertPos.has_value() ? BuildGlobalConstantInlineEdits(context, ctx, candidate, insertPos.value())
                                 : std::nullopt;
}

static void AdjustGeneratedTextForInsert(const RefactorContext &context, public_lib::Context *ctx, size_t insertPos,
                                         const std::string &uniqueVarName, std::string &generatedText)
{
    generatedText = FormatDeclarationForInsert(ctx, insertPos, generatedText);
    if (ctx == nullptr || ctx->sourceFile == nullptr || !GeneratedTextStartsWithVar(generatedText, uniqueVarName)) {
        return;
    }
    std::string indent = GetIndentAtPosition(ctx, insertPos);
    if (!indent.empty()) {
        return;
    }
    std::string_view source = ctx->sourceFile->source;
    if (HasCommaBeforeWithNewline(source, insertPos)) {
        generatedText.insert(0, ResolveIndentSize(context), ' ');
    }
}

static bool ApplyInlineInsertionResult(size_t &insertPos, std::string &generatedText,
                                       const std::optional<std::pair<size_t, std::string>> &inlineInsertionResult)
{
    if (!inlineInsertionResult.has_value()) {
        return false;
    }
    insertPos = inlineInsertionResult->first;
    generatedText = inlineInsertionResult->second;
    return true;
}

static bool IsMultiDeclaratorInsertionText(const std::string &generatedText, const std::string &uniqueVarName)
{
    if (generatedText.empty() || uniqueVarName.empty()) {
        return false;
    }
    if (generatedText.rfind(uniqueVarName + " = ", 0) != 0) {
        return false;
    }
    return generatedText.find(", ") != std::string::npos;
}

static size_t ResolveInsertionPosForVariableExtraction(const RefactorContext &context, public_lib::Context *ctx,
                                                       const std::string &actionName, size_t insertPos,
                                                       TextRange selectionSpan)
{
    if (IsNamespaceAction(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE.name, EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX)) {
        return insertPos;
    }
    const bool isVariableGlobal = IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_GLOBAL);
    const bool isVariableEnclose = IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_ENCLOSE);
    const bool isConstantEnclose =
        IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE) ||
        IsNamespaceAction(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE.name, EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX);
    if (!(isVariableGlobal || isVariableEnclose || isConstantEnclose) || ctx == nullptr || ctx->sourceFile == nullptr) {
        return insertPos;
    }
    if (isVariableEnclose || isConstantEnclose) {
        const TextRange trimmedSpan = selectionSpan;
        if (isConstantEnclose && IsClassPropertyInitializerRhsSelection(context, trimmedSpan)) {
            return FindLineStart(ctx->sourceFile->source, selectionSpan.pos);
        }
        auto *touchNode = GetTouchingTokenByRange(context.context, trimmedSpan, false);
        if (auto blockInsertPos = FindBlockStatementInsertPos(ctx->sourceFile->source, touchNode, trimmedSpan);
            blockInsertPos.has_value()) {
            return blockInsertPos.value();
        }
        if (auto *initializerExpr = ResolveInitializerExpressionContainingSelection(context, trimmedSpan);
            initializerExpr != nullptr) {
            if (auto keywordStart =
                    FindVariableDeclKeywordStart(ctx->sourceFile->source, initializerExpr->Start().index);
                keywordStart.has_value()) {
                return FindLineStart(ctx->sourceFile->source, keywordStart.value());
            }
            return FindLineStart(ctx->sourceFile->source, initializerExpr->Start().index);
        }
        if (auto *decl = FindContainingDeclaratorByRange(context, trimmedSpan); decl != nullptr) {
            return FindLineStart(ctx->sourceFile->source, decl->Start().index);
        }
        if (auto *prop = FindContainingClassPropertyByRange(context, trimmedSpan); prop != nullptr) {
            return FindLineStart(ctx->sourceFile->source, prop->Start().index);
        }
        if (auto keywordStart = FindVariableDeclKeywordStart(ctx->sourceFile->source, selectionSpan.pos);
            keywordStart.has_value()) {
            return FindLineStart(ctx->sourceFile->source, keywordStart.value());
        }
        return insertPos;
    }
    const auto extractionPos = GetCallPositionOfExtraction(context);
    if (auto keywordStart = FindVariableDeclKeywordStart(ctx->sourceFile->source, extractionPos.pos);
        keywordStart.has_value()) {
        return keywordStart.value();
    }
    return insertPos;
}

static TextRange NodeTextRange(const ir::AstNode *node)
{
    return TextRange {node->Start().index, node->End().index};
}

static bool ContainsTextRange(const ir::AstNode *node, TextRange range)
{
    return node != nullptr && node->Start().index <= range.pos && node->End().index >= range.end;
}

static bool IsRepeatedValueGlobalAction(const std::string &actionName)
{
    return IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL) ||
           IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_GLOBAL);
}

static bool IsRepeatedValueEncloseAction(const std::string &actionName)
{
    return IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE) ||
           IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_ENCLOSE);
}

static bool IsRepeatedValueNamespaceAction(const std::string &actionName)
{
    return actionName.rfind(std::string(EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX), 0) == 0;
}

static bool IsRepeatedValueBlockScope(const ir::AstNode *node, TextRange range)
{
    return node != nullptr && node->IsBlockStatement() && !IsGlobalStaticInitializerBody(node) &&
           !IsProgramParent(node) && !IsNamespaceModuleParent(node) && ContainsTextRange(node, range);
}

static std::optional<TextRange> GetRepeatedValueNamespaceSearchRange(ir::AstNode *touchNode,
                                                                     const std::string &actionName)
{
    const auto namespaceDepth = GetNamespaceActionDepth(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE.name,
                                                        EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX);
    if (!namespaceDepth.has_value()) {
        return std::nullopt;
    }

    auto *namespaceScope = FindNamespaceScopeByDepth(touchNode, namespaceDepth.value());
    if (namespaceScope == nullptr) {
        return std::nullopt;
    }
    return NodeTextRange(namespaceScope);
}

static std::optional<TextRange> GetRepeatedValueBlockSearchRange(ir::AstNode *blockNode, TextRange range)
{
    if (!IsRepeatedValueBlockScope(blockNode, range)) {
        return std::nullopt;
    }

    auto *controlFlowParent = blockNode->Parent();
    if (IsControlFlowEncloseScopeNode(controlFlowParent) && ContainsTextRange(controlFlowParent, range)) {
        return NodeTextRange(controlFlowParent);
    }
    return NodeTextRange(blockNode);
}

static std::optional<TextRange> GetRepeatedValueEncloseSearchRange(ir::AstNode *touchNode, TextRange range)
{
    for (auto *current = touchNode; current != nullptr; current = current->Parent()) {
        auto blockRange = GetRepeatedValueBlockSearchRange(current->Parent(), range);
        if (blockRange.has_value()) {
            return blockRange;
        }
        if (IsControlFlowEncloseScopeNode(current) && ContainsTextRange(current, range)) {
            return NodeTextRange(current);
        }
    }
    return std::nullopt;
}

static std::optional<TextRange> GetRepeatedValueExtractionSearchRange(const RefactorContext &context,
                                                                      public_lib::Context *ctx,
                                                                      const std::string &actionName)
{
    const TextRange fullRange {0, ctx->sourceFile->source.size()};
    if (IsRepeatedValueGlobalAction(actionName)) {
        return fullRange;
    }

    const TextRange trimmed = GetTrimmedSelectionSpan(context);
    auto *touchNode = GetTouchingTokenByRange(context.context, trimmed, false);
    if (IsRepeatedValueNamespaceAction(actionName)) {
        return GetRepeatedValueNamespaceSearchRange(touchNode, actionName);
    }

    if (!IsRepeatedValueEncloseAction(actionName)) {
        return fullRange;
    }
    return GetRepeatedValueEncloseSearchRange(touchNode, trimmed);
}

static std::vector<TextRange> CollectRepeatedValueExtractionOccurrences(const RefactorContext &context,
                                                                        public_lib::Context *ctx,
                                                                        const std::string &actionName)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return {};
    }
    const TextRange trimmed = GetTrimmedSelectionSpan(context);
    if (trimmed.end <= trimmed.pos || trimmed.end > ctx->sourceFile->source.size()) {
        return {};
    }
    const std::string_view source = ctx->sourceFile->source;
    const std::string selectedText(source.substr(trimmed.pos, trimmed.end - trimmed.pos));
    if (selectedText.empty()) {
        return {};
    }
    if (selectedText.size() >= 2U && ((selectedText.front() == '\'' && selectedText.back() == '\'') ||
                                      (selectedText.front() == '"' && selectedText.back() == '"') ||
                                      (selectedText.front() == '`' && selectedText.back() == '`'))) {
        return {};
    }

    const auto searchRange = GetRepeatedValueExtractionSearchRange(context, ctx, actionName);
    if (!searchRange.has_value()) {
        return {};
    }

    std::vector<TextRange> occurrences;
    const size_t searchEnd = std::min(searchRange->end, source.size());
    size_t searchPos = std::min(searchRange->pos, searchEnd);
    while (searchPos < searchEnd) {
        const size_t found = source.find(selectedText, searchPos);
        if (found == std::string_view::npos || found + selectedText.size() > searchEnd) {
            break;
        }
        if (!IsExpressionLikeRepeatedOccurrence(source, selectedText, found)) {
            searchPos = found + selectedText.size();
            continue;
        }
        occurrences.push_back({found, found + selectedText.size()});
        searchPos = found + selectedText.size();
    }
    return occurrences;
}

static void AppendTrailingNewLineForGlobalVariableInsert(const RefactorContext &context, const std::string &actionName,
                                                         size_t insertPos, std::string &generatedText)
{
    const bool isGlobalConstant = IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL);
    const bool isGlobalOrEncloseVariable = IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_GLOBAL) ||
                                           IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_CLASS) ||
                                           IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_ENCLOSE);
    if (!isGlobalConstant && !isGlobalOrEncloseVariable) {
        return;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || generatedText.empty()) {
        return;
    }
    const auto &source = ctx->sourceFile->source;
    if (isGlobalConstant && insertPos < source.size() && IsLineBreakChar(source[insertPos])) {
        return;
    }
    if (insertPos < source.size() && !IsLineBreakChar(source[insertPos]) && !IsLineBreakChar(generatedText.back())) {
        generatedText.append(context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter());
    }
}

static void AppendTrailingNewLineForConstantEncloseInsert(const RefactorContext &context, const std::string &actionName,
                                                          size_t insertPos, std::string_view source,
                                                          std::string &generatedText)
{
    if (!(IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE) ||
          IsNamespaceAction(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE.name,
                            EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX)) ||
        generatedText.empty()) {
        return;
    }
    if (insertPos >= source.size() || IsLineBreakChar(source[insertPos]) || IsLineBreakChar(generatedText.back())) {
        return;
    }
    generatedText.append(context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter());
}

static std::string BuildImplicitPrefix(const RefactorContext &context, public_lib::Context *ctx, size_t insertPos,
                                       const std::string &generatedText, bool inlineInsertion)
{
    if (inlineInsertion || ctx == nullptr || ctx->sourceFile == nullptr ||
        ctx->sourceFile->source.size() <= insertPos) {
        return "";
    }
    std::string_view source = ctx->sourceFile->source;
    size_t lineStart = 0;
    size_t lineEnd = 0;
    GetLineBounds(source, insertPos, lineStart, lineEnd);
    const bool atLineEnd = insertPos == lineEnd && IsLineBreakChar(source[lineEnd]);
    if (!atLineEnd) {
        return "";
    }
    const bool blankLine = IsBlankLine(source, lineStart, lineEnd);
    const bool hasLeadingBreak = !generatedText.empty() && IsLineBreakChar(generatedText[0]);
    if (hasLeadingBreak) {
        return "";
    }
    const size_t scopeDepth = CountIndentScopeDepth(ResolveScopeDepthProbeNode(context, insertPos));
    size_t lineIndent = 0;
    for (size_t i = lineStart; i <= lineEnd && i < source.size(); ++i) {
        const char ch = source[i];
        if (ch == ' ' || ch == '\t') {
            ++lineIndent;
            continue;
        }
        break;
    }
    size_t indentLen = 0;
    const size_t scopeIndent = scopeDepth * ResolveIndentSize(context);
    if (blankLine) {
        indentLen = lineIndent > 0 ? lineIndent : scopeIndent;
    } else {
        indentLen = scopeIndent;
    }
    if (!blankLine) {
        std::string prefix = "\n";
        if (indentLen > 0) {
            prefix.append(indentLen, ' ');
        }
        return prefix;
    }
    if (indentLen > 0) {
        return std::string(indentLen, ' ');
    }
    return "";
}

static bool IsNamespaceContextAtAnchor(const RefactorContext &context, ir::AstNode *anchor)
{
    if (anchor == nullptr) {
        anchor = GetTouchingToken(context.context, context.span.pos, false);
    }
    return IsNamespaceContext(anchor);
}

static bool IsNamespaceInsertionTarget(const RefactorContext &context, const std::string &actionName,
                                       ir::AstNode *anchor)
{
    if (const auto depth = GetNamespaceActionDepth(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE.name,
                                                   EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX);
        depth.has_value()) {
        if (depth.value() > 0) {
            return true;
        }
        return IsNamespaceContextAtAnchor(context, anchor);
    }
    if (IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_ENCLOSE) ||
        IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE) ||
        IsConstantExtractionInClassAction(actionName)) {
        return IsNamespaceContextAtAnchor(context, anchor);
    }
    return false;
}

static bool ShouldPrependNamespaceNewline(public_lib::Context *ctx, size_t insertPos, std::string_view generatedText)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || generatedText.empty()) {
        return false;
    }
    if (generatedText.front() == LINE_FEED || generatedText.front() == CARRIAGE_RETURN) {
        return false;
    }
    const auto &source = ctx->sourceFile->source;
    return insertPos < source.size() && IsLineBreakChar(source[insertPos]);
}

static std::string_view TrimHorizontalWhitespace(std::string_view text)
{
    size_t begin = 0;
    while (begin < text.size() && IsIndentChar(text[begin])) {
        ++begin;
    }
    size_t end = text.size();
    while (end > begin && IsIndentChar(text[end - 1])) {
        --end;
    }
    return text.substr(begin, end - begin);
}

static bool PreviousLineIsTerminatedExport(std::string_view source, size_t insertPos)
{
    if (insertPos == 0 || insertPos > source.size()) {
        return false;
    }
    size_t lineEnd = insertPos;
    while (lineEnd > 0 && IsLineBreakChar(source[lineEnd - 1])) {
        --lineEnd;
    }
    if (lineEnd == 0) {
        return false;
    }
    size_t lineStart = lineEnd;
    while (lineStart > 0 && !IsLineBreakChar(source[lineStart - 1])) {
        --lineStart;
    }
    const auto prevLine = TrimHorizontalWhitespace(source.substr(lineStart, lineEnd - lineStart));
    if (prevLine.empty() || prevLine.back() != ';') {
        return false;
    }
    return prevLine.rfind("export ", 0) == 0;
}

static bool IsValueDeclarationText(std::string_view generatedText)
{
    const auto trimmed = TrimHorizontalWhitespace(generatedText);
    return trimmed.rfind("let ", 0) == 0 || trimmed.rfind("const ", 0) == 0;
}

static bool ShouldPrependAdditionalNamespaceBlankLine(public_lib::Context *ctx, size_t insertPos,
                                                      std::string_view generatedText)
{
    if (!ShouldPrependNamespaceNewline(ctx, insertPos, generatedText)) {
        return false;
    }
    if (!IsValueDeclarationText(generatedText)) {
        return false;
    }
    const auto &source = ctx->sourceFile->source;
    return PreviousLineIsTerminatedExport(source, insertPos);
}

static size_t SkipLineBreak(std::string_view source, size_t pos);

static void MaybePrependNamespaceNewlinesForValueExtraction(
    const RefactorContext &context, size_t insertPos, std::string &generatedText,
    const std::pair<const std::string *, ir::AstNode *> &targetInfo)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || targetInfo.first == nullptr) {
        return;
    }
    const std::string &actionName = *targetInfo.first;
    if (!IsNamespaceInsertionTarget(context, actionName, targetInfo.second) ||
        !ShouldPrependNamespaceNewline(ctx, insertPos, generatedText)) {
        return;
    }
    const bool isExtractVariableOrConstant =
        IsVariableExtractionAction(actionName) || IsConstantExtractionAction(actionName);
    const bool prependAdditionalBlankLine =
        isExtractVariableOrConstant && ShouldPrependAdditionalNamespaceBlankLine(ctx, insertPos, generatedText);
    size_t nextLinePos = SkipLineBreak(ctx->sourceFile->source, insertPos);
    std::string indent = GetIndentAtPosition(ctx, nextLinePos);
    if (indent.empty()) {
        indent = GetIndentAtPosition(ctx, insertPos);
    }
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    generatedText.insert(0, indent);
    generatedText.insert(0, newLine);
    if (prependAdditionalBlankLine) {
        generatedText.insert(0, newLine);
    }
}

static size_t SkipLineBreak(std::string_view source, size_t pos)
{
    if (pos >= source.size() || !IsLineBreakChar(source[pos])) {
        return pos;
    }
    if (source[pos] == CARRIAGE_RETURN && pos + 1 < source.size() && source[pos + 1] == LINE_FEED) {
        return pos + CRLF_LENGTH;
    }
    return pos + 1;
}

struct PlaceholderBuildInfo;

static bool IsDeclarationTextSelection(const RefactorContext &refContext, TextRange trimmed)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(refContext.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || trimmed.end <= trimmed.pos ||
        trimmed.end > ctx->sourceFile->source.size()) {
        return false;
    }
    const std::string_view selected(ctx->sourceFile->source.data() + trimmed.pos, trimmed.end - trimmed.pos);
    const bool startsWithDecl = selected.rfind("let ", 0) == 0 || selected.rfind("const ", 0) == 0;
    if (!startsWithDecl || selected.find('\n') != std::string_view::npos) {
        return false;
    }
    size_t end = selected.size();
    while (end > 0 && std::isspace(static_cast<unsigned char>(selected[end - 1])) != 0) {
        --end;
    }
    return end > 0 && selected[end - 1] == ';';
}

static bool IsSelectionInsideUnterminatedDeclarationInitializer(const RefactorContext &context, TextRange trimmed)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || trimmed.end <= trimmed.pos ||
        trimmed.end > ctx->sourceFile->source.size()) {
        return false;
    }
    auto *initializerExpr = ResolveInitializerExpressionContainingSelection(context, trimmed);
    if (initializerExpr == nullptr || initializerExpr->Start().index > trimmed.pos ||
        initializerExpr->End().index < trimmed.end) {
        return false;
    }
    std::string_view source = ctx->sourceFile->source;
    const size_t stmtStart = source.rfind('\n', trimmed.pos);
    const size_t probeStart = (stmtStart == std::string::npos) ? 0 : (stmtStart + 1);
    const size_t eqPos = source.rfind('=', trimmed.pos);
    if (eqPos == std::string::npos || eqPos < probeStart) {
        return false;
    }
    const std::string_view head(source.data() + probeStart, eqPos - probeStart);
    const bool startsWithDecl =
        head.find("let ") != std::string_view::npos || head.find("const ") != std::string_view::npos;
    if (!startsWithDecl) {
        return false;
    }
    const size_t lineEndPos = source.find('\n', initializerExpr->End().index);
    const size_t lineEnd = (lineEndPos == std::string::npos) ? source.size() : lineEndPos;
    for (size_t i = lineEnd; i > probeStart; --i) {
        const unsigned char ch = static_cast<unsigned char>(source[i - 1]);
        if (std::isspace(ch) != 0) {
            continue;
        }
        return source[i - 1] != ';';
    }
    return false;
}

static bool IsLiteralSelectionInsideDeclarationInitializer(const RefactorContext &context, TextRange trimmed)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || trimmed.end <= trimmed.pos ||
        trimmed.end > ctx->sourceFile->source.size()) {
        return false;
    }
    const std::string_view source = ctx->sourceFile->source;
    const std::string_view selected(source.data() + trimmed.pos, trimmed.end - trimmed.pos);
    const bool isQuotedLiteral =
        selected.size() >= MIN_QUOTED_LITERAL_LENGTH &&
        ((selected.front() == '"' && selected.back() == '"') || (selected.front() == '\'' && selected.back() == '\''));
    const bool isKeywordLiteral = selected == "true" || selected == "false" || selected == "null";
    if (!isQuotedLiteral && !isKeywordLiteral) {
        if (auto *exactExpr = FindExactSelectionExpression(context, trimmed);
            exactExpr == nullptr ||
            (!exactExpr->IsNumberLiteral() && !exactExpr->IsStringLiteral() && !exactExpr->IsBooleanLiteral() &&
             !exactExpr->IsNullLiteral() && !exactExpr->IsCharLiteral())) {
            return false;
        }
    }
    const size_t lineStartPos = source.rfind('\n', trimmed.pos);
    const size_t lineStart = (lineStartPos == std::string::npos) ? 0 : (lineStartPos + 1);
    const size_t eqPos = source.rfind('=', trimmed.pos);
    if (eqPos == std::string::npos || eqPos < lineStart) {
        return false;
    }
    const std::string_view prefix(source.data() + lineStart, eqPos - lineStart);
    return prefix.find("let ") != std::string_view::npos || prefix.find("const ") != std::string_view::npos;
}

static bool HasGlobalFunctionAction(const std::vector<RefactorAction> &actions)
{
    return std::any_of(actions.begin(), actions.end(), [](const RefactorAction &action) {
        return action.kind == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.kind);
    });
}

static void AddWholeDeclFunctionActionIfMissing(const RefactorContext &refContext, std::vector<RefactorAction> &actions)
{
    if (HasGlobalFunctionAction(actions)) {
        return;
    }
    if (IsSelectionInsideClassExpressionMethod(refContext)) {
        return;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(refContext.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return;
    }
    const TextRange trimmed = GetTrimmedSelectionSpan(refContext);
    auto *resolvedNode =
        ResolveNodeForSelection(refContext, ctx, HasSelectionNewline(refContext, ctx->sourceFile->source), trimmed);
    if (FindScriptFunction(resolvedNode) != nullptr) {
        return;
    }
    if (FindWholeVariableDeclarationSelectionNode(refContext, trimmed) == nullptr &&
        !IsDeclarationTextSelection(refContext, trimmed)) {
        return;
    }
    auto scope = ResolveScopeContext(resolvedNode);
    if (!scope.namespaceScopeNames.empty()) {
        AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_ENCLOSE,
                          BuildNamedScopeDescription("function", "namespace", scope.namespaceScopeNames.front()));
        return;
    }
    AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_GLOBAL);
}

static bool TryAddFunctionActionForInitializerExpression(const RefactorContext &refContext,
                                                         std::vector<RefactorAction> &actions)
{
    if (IsSelectionInsideClassExpressionMethod(refContext)) {
        return false;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(refContext.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return false;
    }
    const TextRange trimmed = GetTrimmedSelectionSpan(refContext);
    auto *initializer = ResolveInitializerExpressionContainingSelection(refContext, trimmed);
    auto *selectionExpr = FindExactSelectionExpression(refContext, trimmed);
    if (selectionExpr == nullptr) {
        selectionExpr = ResolveExpressionCoveringRange(refContext, trimmed);
    }
    if (initializer == nullptr || selectionExpr == nullptr || !selectionExpr->IsExpression() ||
        initializer->Start().index > trimmed.pos || initializer->End().index < trimmed.end ||
        ContainsThisOrSuperExpression(selectionExpr) || ContainsThisOrSuperInRange(ctx, trimmed) ||
        IsSelectionInsideTypeAnnotationContext(refContext, trimmed) ||
        IsSelectionInsideTsConditionalType(refContext, trimmed)) {
        return false;
    }
    auto *resolvedNode = ResolveNodeForSelection(refContext, ctx, false, trimmed);
    auto scope = ResolveScopeContext(resolvedNode == nullptr ? selectionExpr : resolvedNode);
    if (scope.hasClassScope || !scope.namespaceScopeNames.empty() || !HasValidFunctionExtractionCandidate(refContext)) {
        return false;
    }
    AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_GLOBAL);
    return true;
}

static bool TryAddFunctionActionForEmptyRefactors(const RefactorContext &refContext,
                                                  std::vector<RefactorAction> &actions)
{
    if (IsSelectionInsideClassExpressionMethod(refContext)) {
        return false;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(refContext.context);
    const TextRange trimmed = GetTrimmedSelectionSpan(refContext);
    const bool topLevelMultiline =
        ctx != nullptr && ctx->sourceFile != nullptr && HasSelectionNewline(refContext, ctx->sourceFile->source) &&
        FindScriptFunction(ResolveNodeForSelection(refContext, ctx, true, trimmed)) == nullptr &&
        FindWholeVariableDeclarationSelectionNode(refContext, trimmed) == nullptr;
    const bool wholeDeclSelection = FindWholeVariableDeclarationSelectionNode(refContext, trimmed) != nullptr ||
                                    IsDeclarationTextSelection(refContext, trimmed);
    if (topLevelMultiline && HasValidFunctionExtractionCandidate(refContext)) {
        AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_GLOBAL);
        return true;
    }
    if (!wholeDeclSelection && TryAddFunctionActionForInitializerExpression(refContext, actions)) {
        return true;
    }
    if (!wholeDeclSelection || ctx == nullptr || ctx->sourceFile == nullptr) {
        return false;
    }
    auto *resolvedNode =
        ResolveNodeForSelection(refContext, ctx, HasSelectionNewline(refContext, ctx->sourceFile->source), trimmed);
    if (FindScriptFunction(resolvedNode) != nullptr) {
        return false;
    }
    auto scope = ResolveScopeContext(resolvedNode);
    if (!scope.namespaceScopeNames.empty()) {
        AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_ENCLOSE,
                          BuildNamedScopeDescription("function", "namespace", scope.namespaceScopeNames.front()));
    } else {
        AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_GLOBAL);
    }
    return true;
}

static bool TryAddVariableActionForEmptyRefactors(const RefactorContext &refContext,
                                                  std::vector<RefactorAction> &actions)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(refContext.context);
    const TextRange trimmed = GetTrimmedSelectionSpan(refContext);
    if (ctx == nullptr || ctx->sourceFile == nullptr || trimmed.end <= trimmed.pos ||
        trimmed.end > ctx->sourceFile->source.size() || HasSelectionNewline(refContext, ctx->sourceFile->source)) {
        return false;
    }
    if (FindExactSelectionExpression(refContext, trimmed) == nullptr &&
        ResolveExpressionCoveringRange(refContext, trimmed) == nullptr) {
        return false;
    }
    ir::AstNode *selectedExpr = FindExactSelectionExpression(refContext, trimmed);
    if (selectedExpr == nullptr) {
        selectedExpr = ResolveGlobalConstantSelectionExpression(refContext, trimmed);
    }
    if (selectedExpr != nullptr && selectedExpr->IsTypeofExpression()) {
        AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE);
        return true;
    }
    auto *resolvedNode = ResolveNodeForSelection(refContext, ctx, false, trimmed);
    if (resolvedNode == nullptr || ContainsThisOrSuperExpression(resolvedNode) ||
        ContainsThisOrSuperInRange(ctx, trimmed)) {
        return false;
    }
    const auto scope = ResolveScopeContext(resolvedNode);
    const bool hasNamespaceScope = !scope.namespaceScopeNames.empty();
    // Only recover enclose action when there is a true enclosing block/function scope.
    // Namespace-only/top-level contexts should keep global variable behavior.
    if (!scope.hasEncloseScope || hasNamespaceScope) {
        return false;
    }
    AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE);
    return true;
}

static bool HasValueIdentifierInSelection(ir::AstNode *node, TextRange span)
{
    bool hasIdentifier = false;
    if (node == nullptr) {
        return false;
    }
    node->FindChild([&](ir::AstNode *child) {
        if (hasIdentifier || child == nullptr || !child->IsIdentifier() || child->Start().index < span.pos ||
            child->End().index > span.end) {
            return false;
        }
        auto *ident = child->AsIdentifier();
        if (IsDeclarationIdentifier(ident) || IsMemberPropertyIdentifier(ident)) {
            return false;
        }
        const std::string name = IdentifierNameMutf8(ident);
        hasIdentifier = !name.empty() && name != "this" && name != "super";
        return hasIdentifier;
    });
    return hasIdentifier;
}

static bool IsArithmeticBinaryExpression(const ir::AstNode *node)
{
    if (node == nullptr || !node->IsBinaryExpression()) {
        return false;
    }
    auto op = node->AsBinaryExpression()->OperatorType();
    return op == lexer::TokenType::PUNCTUATOR_PLUS || op == lexer::TokenType::PUNCTUATOR_MINUS ||
           op == lexer::TokenType::PUNCTUATOR_MULTIPLY || op == lexer::TokenType::PUNCTUATOR_DIVIDE ||
           op == lexer::TokenType::PUNCTUATOR_MOD || op == lexer::TokenType::PUNCTUATOR_EXPONENTIATION;
}

static bool HasDeclaratorSeparatorBeforeSelection(const RefactorContext &refContext, TextRange trimmed)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(refContext.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || trimmed.pos > ctx->sourceFile->source.size()) {
        return false;
    }
    const auto &source = ctx->sourceFile->source;
    const size_t stmtStartPos = source.rfind(';', trimmed.pos);
    const size_t stmtStart = stmtStartPos == std::string::npos ? 0 : stmtStartPos + 1;
    const std::string_view prefix(source.data() + stmtStart, trimmed.pos - stmtStart);
    const bool hasDeclaration = prefix.find("const ") != std::string_view::npos ||
                                prefix.find("let ") != std::string_view::npos ||
                                prefix.find("var ") != std::string_view::npos;
    return hasDeclaration && prefix.find(',') != std::string_view::npos;
}

static bool ShouldPreferTopLevelFunctionExtraction(const RefactorContext &refContext, ir::AstNode *selectedExpr,
                                                   TextRange trimmed)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(refContext.context);
    if (selectedExpr == nullptr || FindScriptFunction(selectedExpr) != nullptr ||
        !IsArithmeticBinaryExpression(selectedExpr) || ResolveInlineMultiDeclNodesBySpan(refContext, ctx).has_value() ||
        HasDeclaratorSeparatorBeforeSelection(refContext, trimmed) ||
        !HasValidFunctionExtractionCandidate(refContext)) {
        return false;
    }
    const auto scope = ResolveScopeContext(selectedExpr);
    return !scope.hasEncloseScope && scope.namespaceScopeNames.empty() &&
           HasValueIdentifierInSelection(selectedExpr, trimmed);
}

static bool TryAddConstantActionForEmptyRefactors(const RefactorContext &refContext,
                                                  std::vector<RefactorAction> &actions)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(refContext.context);
    const TextRange trimmed = GetTrimmedSelectionSpan(refContext);
    if (!CanRecoverEmptyConstantRefactor(ctx, refContext, trimmed)) {
        return false;
    }
    ir::AstNode *selectedExpr = ResolveFallbackConstantSelectionExpr(refContext, trimmed);
    if (!IsRecoverableConstantSelection(ctx, refContext, trimmed, selectedExpr)) {
        return false;
    }
    return AddRecoveredConstantActions(actions, ResolveScopeContext(selectedExpr));
}

static bool CanRecoverEmptyConstantRefactor(const public_lib::Context *ctx, const RefactorContext &refContext,
                                            TextRange trimmed)
{
    return ctx != nullptr && ctx->sourceFile != nullptr && trimmed.end > trimmed.pos &&
           trimmed.end <= ctx->sourceFile->source.size() && !HasSelectionNewline(refContext, ctx->sourceFile->source);
}

static ir::AstNode *ResolveFallbackConstantSelectionExpr(const RefactorContext &refContext, TextRange trimmed)
{
    ir::AstNode *selectedExpr = ResolveGlobalConstantSelectionExpression(refContext, trimmed);
    if (selectedExpr != nullptr || !IsSelectionInsideTypeAnnotationContext(refContext, trimmed)) {
        return selectedExpr;
    }
    auto *touch = GetTouchingTokenByRange(refContext.context, trimmed, false);
    if (touch != nullptr && (touch->IsStringLiteral() || touch->IsTemplateLiteral() || touch->IsCharLiteral())) {
        return touch;
    }
    return nullptr;
}

static bool IsRecoverableConstantSelection(public_lib::Context *ctx, const RefactorContext &refContext,
                                           TextRange trimmed, ir::AstNode *selectedExpr)
{
    return selectedExpr != nullptr && selectedExpr->IsExpression() && !ContainsThisOrSuperExpression(selectedExpr) &&
           !ContainsThisOrSuperInRange(ctx, trimmed) &&
           !ShouldPreferTopLevelFunctionExtraction(refContext, selectedExpr, trimmed);
}

static bool AddRecoveredConstantActions(std::vector<RefactorAction> &actions, const ScopeContext &scope)
{
    if (!scope.hasEncloseScope && scope.namespaceScopeNames.empty()) {
        AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_GLOBAL);
        return true;
    }
    if (scope.namespaceScopeNames.empty()) {
        AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_ENCLOSE);
        return true;
    }
    AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_GLOBAL);
    AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_ENCLOSE);
    for (size_t namespaceDepth = 1; namespaceDepth < scope.namespaceScopeNames.size(); ++namespaceDepth) {
        AddRefactorAction(
            actions, EXTRACT_CONSTANT_ACTION_ENCLOSE,
            BuildNamedScopeDescription("constant", "namespace", scope.namespaceScopeNames[namespaceDepth]),
            BuildNamespaceScopedActionName(EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX, namespaceDepth),
            std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.kind));
    }
    return true;
}

static bool IsSelectionInsideTypeNode(const RefactorContext &refContext, TextRange span)
{
    ir::AstNode *node = FindExactSelectionExpression(refContext, span);
    if (node == nullptr) {
        node = ResolveExpressionCoveringRange(refContext, span);
    }
    if (node == nullptr) {
        node = GetTouchingTokenByRange(refContext.context, span, false);
    }
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (current->Start().index > span.pos || current->End().index < span.end) {
            continue;
        }
        if (current->IsExpression() && current->AsExpression()->IsTypeNode()) {
            return true;
        }
    }
    return false;
}

enum class SourceScanState {
    CODE,
    SINGLE_QUOTE,
    DOUBLE_QUOTE,
    TEMPLATE,
    LINE_COMMENT,
    BLOCK_COMMENT,
};

static void AdvanceCodeScanState(char ch, char next, SourceScanState &state, bool &escaped, size_t &i)
{
    if (ch == '/' && next == '/') {
        state = SourceScanState::LINE_COMMENT;
        ++i;
    } else if (ch == '/' && next == '*') {
        state = SourceScanState::BLOCK_COMMENT;
        ++i;
    } else if (ch == '\'') {
        state = SourceScanState::SINGLE_QUOTE;
        escaped = false;
    } else if (ch == '"') {
        state = SourceScanState::DOUBLE_QUOTE;
        escaped = false;
    } else if (ch == '`') {
        state = SourceScanState::TEMPLATE;
        escaped = false;
    }
}

static void AdvanceQuotedScanState(char ch, char quote, SourceScanState &state, bool &escaped)
{
    if (!escaped && ch == quote) {
        state = SourceScanState::CODE;
    }
    escaped = !escaped && ch == '\\';
}

static void AdvanceLineCommentScanState(char ch, SourceScanState &state)
{
    if (ch == '\n' || ch == '\r') {
        state = SourceScanState::CODE;
    }
}

static void AdvanceBlockCommentScanState(char ch, char next, SourceScanState &state, size_t &i)
{
    if (ch == '*' && next == '/') {
        state = SourceScanState::CODE;
        ++i;
    }
}

static bool IsSourcePositionInCode(std::string_view source, size_t pos)
{
    SourceScanState state = SourceScanState::CODE;
    bool escaped = false;
    for (size_t i = 0; i < pos; i++) {
        const char ch = source[i];
        const char next = i + 1 < source.size() ? source[i + 1] : '\0';
        switch (state) {
            case SourceScanState::CODE:
                AdvanceCodeScanState(ch, next, state, escaped, i);
                break;
            case SourceScanState::SINGLE_QUOTE:
                AdvanceQuotedScanState(ch, '\'', state, escaped);
                break;
            case SourceScanState::DOUBLE_QUOTE:
                AdvanceQuotedScanState(ch, '"', state, escaped);
                break;
            case SourceScanState::TEMPLATE:
                AdvanceQuotedScanState(ch, '`', state, escaped);
                break;
            case SourceScanState::LINE_COMMENT:
                AdvanceLineCommentScanState(ch, state);
                break;
            case SourceScanState::BLOCK_COMMENT:
                AdvanceBlockCommentScanState(ch, next, state, i);
                break;
        }
    }
    return state == SourceScanState::CODE;
}

static bool IsRepeatedOccurrenceBoundary(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) == 0 && ch != '_' && ch != '$' && ch != '.';
}

static bool HasRepeatedOccurrenceLeftBoundary(std::string_view source, size_t pos)
{
    return pos == 0 || IsRepeatedOccurrenceBoundary(source[pos - 1]) || source[pos - 1] == '(' ||
           source[pos - 1] == '[' || source[pos - 1] == '{' || source[pos - 1] == ',';
}

static bool HasRepeatedOccurrenceRightBoundary(std::string_view source, size_t end)
{
    return end >= source.size() || IsRepeatedOccurrenceBoundary(source[end]) || source[end] == ')' ||
           source[end] == ']' || source[end] == '}' || source[end] == ';' || source[end] == ',' || source[end] == '&' ||
           source[end] == '|';
}

static bool HasRepeatedOccurrenceAllowedPrefix(std::string_view source, size_t pos)
{
    const size_t lineStart = FindLineStart(source, pos);
    const std::string_view linePrefix(source.data() + lineStart, pos - lineStart);
    return linePrefix.find("function ") == std::string_view::npos;
}

static bool IsExpressionLikeRepeatedOccurrence(std::string_view source, std::string_view selectedText, size_t pos)
{
    if (selectedText.empty() || pos > source.size() || pos + selectedText.size() > source.size()) {
        return false;
    }
    if (!IsSourcePositionInCode(source, pos)) {
        return false;
    }
    if (!HasRepeatedOccurrenceAllowedPrefix(source, pos)) {
        return false;
    }
    if (!HasRepeatedOccurrenceLeftBoundary(source, pos)) {
        return false;
    }
    const size_t end = pos + selectedText.size();
    if (!HasRepeatedOccurrenceRightBoundary(source, end)) {
        return false;
    }
    return true;
}

static bool IsSelectionInsideDeclaratorTypeAnnotation(const RefactorContext &refContext, TextRange span)
{
    auto *decl = FindContainingDeclaratorByRange(refContext, span);
    if (decl == nullptr || decl->Id() == nullptr) {
        return false;
    }
    ir::TypeNode *typeAnno = TypeAnnoFromDeclaratorId(decl->Id());
    return typeAnno != nullptr && typeAnno->Start().index <= span.pos && typeAnno->End().index >= span.end;
}

static bool IsSelectionInsideFunctionReturnTypeAnnotation(const RefactorContext &refContext, TextRange span)
{
    ir::AstNode *node = GetTouchingTokenByRange(refContext.context, span, false);
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsScriptFunction()) {
            ir::TypeNode *returnType = current->AsScriptFunction()->ReturnTypeAnnotation();
            if (returnType != nullptr && returnType->Start().index <= span.pos && returnType->End().index >= span.end) {
                return true;
            }
        }
        if (current->IsMethodDefinition()) {
            auto *func = current->AsMethodDefinition()->Function();
            ir::TypeNode *returnType = func == nullptr ? nullptr : func->ReturnTypeAnnotation();
            if (returnType != nullptr && returnType->Start().index <= span.pos && returnType->End().index >= span.end) {
                return true;
            }
        }
    }
    return false;
}

static bool IsSelectionOnTypeDeclarationName(const RefactorContext &refContext, TextRange span)
{
    ir::AstNode *node = GetTouchingTokenByRange(refContext.context, span, false);
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsTSInterfaceDeclaration()) {
            auto *id = current->AsTSInterfaceDeclaration()->Id();
            return id != nullptr && id->Start().index <= span.pos && id->End().index >= span.end;
        }
        if (current->IsTSTypeAliasDeclaration()) {
            auto *id = current->AsTSTypeAliasDeclaration()->Id();
            return id != nullptr && id->Start().index <= span.pos && id->End().index >= span.end;
        }
    }
    return false;
}

static bool IsSelectionInsideTypeAnnotationContext(const RefactorContext &refContext, TextRange span)
{
    return IsSelectionInsideTypeNode(refContext, span) || IsSelectionInsideDeclaratorTypeAnnotation(refContext, span) ||
           IsSelectionInsideFunctionReturnTypeAnnotation(refContext, span) ||
           IsSelectionOnTypeDeclarationName(refContext, span);
}

static void RemoveControlFlowConstantActions(std::vector<RefactorAction> &actions)
{
    auto it = actions.begin();
    while (it != actions.end()) {
        const bool isEnclose = it->name == std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
        const bool isNamespace = it->name.rfind(EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX, 0) == 0;
        if (isEnclose || isNamespace) {
            it = actions.erase(it);
            continue;
        }
        ++it;
    }
}

static bool MatchesRequestedKind(const RefactorContext &refContext, const RefactorAction &action)
{
    return refContext.kind.empty() || refContext.kind == action.kind;
}

static std::vector<ApplicableRefactorInfo> BuildApplicableRefactorInfoList(
    const RefactorContext &refContext, const std::vector<RefactorAction> &refactoredNodeList)
{
    std::vector<ApplicableRefactorInfo> resList;
    for (const RefactorAction &ref : refactoredNodeList) {
        if (!MatchesRequestedKind(refContext, ref)) {
            continue;
        }
        ApplicableRefactorInfo res;
        res.name = REFACTOR_NAME;
        res.description = REFACTOR_DESCRIPTION;
        res.action = ref;
        resList.push_back(res);
    }
    return resList;
}

static ir::AstNode *ResolveSelectionNodeForApplicableActions(const RefactorContext &refContext, TextRange trimmed)
{
    if (ir::AstNode *selectionNode = FindExactSelectionExpression(refContext, trimmed); selectionNode != nullptr) {
        return selectionNode;
    }
    if (ir::AstNode *selectionNode = ResolveExpressionCoveringRange(refContext, trimmed); selectionNode != nullptr) {
        return selectionNode;
    }
    return GetTouchingTokenByRange(refContext.context, trimmed, false);
}

static void EnsureVariableEncloseForTypeofSelection(const RefactorContext &refContext,
                                                    std::vector<ApplicableRefactorInfo> &resList,
                                                    ir::AstNode *selectionNode)
{
    const bool isTypeofSelection = selectionNode != nullptr && selectionNode->IsTypeofExpression();
    if (!isTypeofSelection || !refContext.kind.empty()) {
        return;
    }
    const auto scope = ResolveScopeContext(selectionNode);
    bool hasVarEnclose = std::any_of(resList.begin(), resList.end(), [](const ApplicableRefactorInfo &info) {
        return info.action.name == std::string(EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    });
    if (hasVarEnclose) {
        return;
    }
    ApplicableRefactorInfo res;
    res.name = REFACTOR_NAME;
    res.description = REFACTOR_DESCRIPTION;
    res.action = {std::string(EXTRACT_VARIABLE_ACTION_ENCLOSE.name),
                  std::string(EXTRACT_VARIABLE_ACTION_ENCLOSE.description),
                  std::string(EXTRACT_VARIABLE_ACTION_ENCLOSE.kind)};
    if (!MatchesRequestedKind(refContext, res.action)) {
        return;
    }
    resList.push_back(res);
}

static void RemoveValueAndFunctionActionsForTypeContext(std::vector<ApplicableRefactorInfo> &resList)
{
    auto it = resList.begin();
    while (it != resList.end()) {
        const bool isVariableKind = it->action.kind == EXTRACT_VARIABLE_ACTION_GLOBAL.kind ||
                                    it->action.kind == EXTRACT_VARIABLE_ACTION_ENCLOSE.kind;
        const bool isConstantKind = it->action.kind == EXTRACT_CONSTANT_ACTION_GLOBAL.kind ||
                                    it->action.kind == EXTRACT_CONSTANT_ACTION_ENCLOSE.kind;
        const bool isFunctionKind = it->action.kind == EXTRACT_FUNCTION_ACTION_GLOBAL.kind ||
                                    it->action.kind == EXTRACT_FUNCTION_ACTION_ENCLOSE.kind;
        if (isVariableKind || isConstantKind || isFunctionKind) {
            it = resList.erase(it);
            continue;
        }
        ++it;
    }
}

template <typename Predicate>
static void RemoveApplicableActionsIf(std::vector<ApplicableRefactorInfo> &resList, Predicate predicate)
{
    auto it = resList.begin();
    while (it != resList.end()) {
        if (predicate(it->action)) {
            it = resList.erase(it);
            continue;
        }
        ++it;
    }
}

static bool IsLiteralTypeContextSelection(ir::AstNode *selectionNode)
{
    return selectionNode != nullptr &&
           (selectionNode->IsStringLiteral() || selectionNode->IsTemplateLiteral() || selectionNode->IsCharLiteral());
}

static bool IsQuotedLiteralSelectionText(const RefactorContext &refContext, TextRange span)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(refContext.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || span.end <= span.pos ||
        span.end > ctx->sourceFile->source.size()) {
        return false;
    }
    const std::string_view selected(ctx->sourceFile->source.data() + span.pos, span.end - span.pos);
    return selected.size() >= MIN_QUOTED_LITERAL_LENGTH && ((selected.front() == '\'' && selected.back() == '\'') ||
                                                            (selected.front() == '"' && selected.back() == '"'));
}

static bool IsSelectionInControlFlowBodyWithReusableTest(public_lib::Context *ctx, TextRange trimmed,
                                                         const ir::AstNode *selectedNode)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr ||
        ctx->sourceFile == nullptr || trimmed.end <= trimmed.pos || trimmed.end > ctx->sourceFile->source.size()) {
        return false;
    }
    if (selectedNode == nullptr || ContainsThisOrSuperExpression(selectedNode)) {
        return false;
    }
    bool matched = false;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (matched || node == nullptr || !node->IsIfStatement()) {
            return false;
        }
        auto *ifStmt = node->AsIfStatement();
        if (ifStmt->Consequent() == nullptr || ifStmt->Consequent()->Start().index > trimmed.pos ||
            ifStmt->Consequent()->End().index < trimmed.end) {
            return false;
        }
        matched = true;
        return matched;
    });
    return matched;
}

static void ApplyTypeContextPostFilters(const RefactorContext &refContext, std::vector<ApplicableRefactorInfo> &resList,
                                        ir::AstNode *selectionNode, TextRange trimmed)
{
    EnsureVariableEncloseForTypeofSelection(refContext, resList, selectionNode);
    if (IsSelectionInsideTypeAnnotationContext(refContext, trimmed) && !IsLiteralTypeContextSelection(selectionNode) &&
        !IsQuotedLiteralSelectionText(refContext, trimmed)) {
        RemoveValueAndFunctionActionsForTypeContext(resList);
    }
}

static void ApplyContextSpecificAvailableActionRemovals(const RefactorContext &refContext,
                                                        std::vector<ApplicableRefactorInfo> &resList,
                                                        ir::AstNode *selectionNode, TextRange trimmed)
{
    if (IsSelectionWithinControlFlowTest(refContext)) {
        RemoveApplicableActionsIf(resList, [](const RefactorAction &action) {
            const bool isEnclose = action.name == std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
            const bool isNamespace = action.name.rfind(EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX, 0) == 0;
            return isEnclose || isNamespace;
        });
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(refContext.context);
    const bool hasUseStaticDirective =
        ctx != nullptr && ctx->sourceFile != nullptr && HasUseStaticDirective(ctx->sourceFile->source);
    if (IsObjectLiteralSelectionNode(refContext, selectionNode, trimmed) && !hasUseStaticDirective) {
        RemoveApplicableActionsIf(resList, [](const RefactorAction &action) {
            return action.name == std::string(EXTRACT_VARIABLE_ACTION_GLOBAL.name) ||
                   action.name == std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.name);
        });
    }
    if (IsSelectionInsideClassExpressionMethod(refContext)) {
        RemoveApplicableActionsIf(resList, [](const RefactorAction &action) {
            return action.name == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name);
        });
    }
    if (IsSelectionInsideThrowArgument(refContext, trimmed) ||
        ShouldPreferTopLevelFunctionExtraction(refContext, selectionNode, trimmed)) {
        RemoveApplicableActionsIf(resList, [](const RefactorAction &action) {
            return action.name == std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.name);
        });
    }
    if (IsSelectionInsideTsConditionalType(refContext, trimmed)) {
        RemoveApplicableActionsIf(resList, [](const RefactorAction &action) {
            return action.name == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name) ||
                   action.name == std::string(EXTRACT_FUNCTION_ACTION_ENCLOSE.name) ||
                   action.name.rfind(EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX, 0) == 0;
        });
    }
}

static bool HasApplicableAction(const std::vector<ApplicableRefactorInfo> &resList, std::string_view actionName)
{
    return std::any_of(resList.begin(), resList.end(),
                       [actionName](const ApplicableRefactorInfo &info) { return info.action.name == actionName; });
}

static void AddApplicableActionIfMatched(const RefactorContext &refContext,
                                         std::vector<ApplicableRefactorInfo> &resList,
                                         const RefactorActionView &actionView)
{
    ApplicableRefactorInfo res;
    res.name = REFACTOR_NAME;
    res.description = REFACTOR_DESCRIPTION;
    res.action = {std::string(actionView.name), std::string(actionView.description), std::string(actionView.kind)};
    if (MatchesRequestedKind(refContext, res.action)) {
        resList.push_back(std::move(res));
    }
}

static ir::AstNode *ResolveApplicableGlobalConstantNode(const RefactorContext &refContext, TextRange trimmed,
                                                        ir::AstNode *selectionNode)
{
    if (auto *globalConstantNode = ResolveGlobalConstantSelectionExpression(refContext, trimmed);
        globalConstantNode != nullptr) {
        return globalConstantNode;
    }
    return selectionNode;
}

static bool CanAddGlobalConstantAction(const RefactorContext &refContext, public_lib::Context *ctx,
                                       ir::AstNode *globalConstantNode, TextRange trimmed)
{
    const bool allowControlFlowBodyGlobal =
        IsSelectionInControlFlowBodyWithReusableTest(ctx, trimmed, globalConstantNode);
    const bool allowGlobalExpressionConstant =
        globalConstantNode != nullptr && globalConstantNode->IsExpression() &&
        !ContainsThisOrSuperExpression(globalConstantNode) && !ContainsThisOrSuperInRange(ctx, trimmed) &&
        !HasLocalValueDependencyInSelection(refContext, trimmed) &&
        !ShouldPreferTopLevelFunctionExtraction(refContext, globalConstantNode, trimmed) &&
        IsValidGlobalConstantSelection(refContext, std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.name));
    return !IsSelectionInsideThrowArgument(refContext, trimmed) &&
           !IsSelectionInsideTypeAnnotationContext(refContext, trimmed) && globalConstantNode != nullptr &&
           !IsInsideLoopStatement(globalConstantNode) &&
           !IsObjectLiteralSelectionNode(refContext, globalConstantNode, trimmed) &&
           !IsNamespaceContext(globalConstantNode) &&
           ((FindScriptFunction(globalConstantNode) == nullptr &&
             FindEnclosingClassDefinition(globalConstantNode) == nullptr) ||
            allowControlFlowBodyGlobal || allowGlobalExpressionConstant);
}

static void MaybeAddGlobalConstantActions(const RefactorContext &refContext,
                                          std::vector<ApplicableRefactorInfo> &resList, public_lib::Context *ctx,
                                          ir::AstNode *selectionNode, TextRange trimmed)
{
    if (HasApplicableAction(resList, EXTRACT_CONSTANT_ACTION_GLOBAL.name)) {
        return;
    }
    ir::AstNode *globalConstantNode = ResolveApplicableGlobalConstantNode(refContext, trimmed, selectionNode);
    if (CanAddGlobalConstantAction(refContext, ctx, globalConstantNode, trimmed)) {
        AddApplicableActionIfMatched(refContext, resList, EXTRACT_CONSTANT_ACTION_GLOBAL);
        return;
    }
    if (globalConstantNode != nullptr && IsQuotedLiteralSelectionText(refContext, trimmed) &&
        !ContainsThisOrSuperExpression(globalConstantNode) && !ContainsThisOrSuperInRange(ctx, trimmed) &&
        !CollectEnclosingNamespaceScopes(globalConstantNode).empty()) {
        AddApplicableActionIfMatched(refContext, resList, EXTRACT_CONSTANT_ACTION_GLOBAL);
    }
}

static void ApplyAvailableActionPostFilters(const RefactorContext &refContext,
                                            std::vector<ApplicableRefactorInfo> &resList)
{
    const TextRange trimmed = GetTrimmedSelectionSpan(refContext);
    ir::AstNode *selectionNode = ResolveSelectionNodeForApplicableActions(refContext, trimmed);
    auto *ctx = reinterpret_cast<public_lib::Context *>(refContext.context);
    if (IsNamespaceClassValueReferenceSelection(refContext, trimmed)) {
        resList.clear();
        return;
    }
    const bool isClassPropertyInitializerSelection = IsClassPropertyInitializerRhsSelection(refContext, trimmed);
    if (isClassPropertyInitializerSelection) {
        RemoveApplicableActionsIf(resList, [](const RefactorAction &action) {
            return action.name == std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
        });
    }
    ApplyTypeContextPostFilters(refContext, resList, selectionNode, trimmed);
    ApplyContextSpecificAvailableActionRemovals(refContext, resList, selectionNode, trimmed);
    MaybeAddGlobalConstantActions(refContext, resList, ctx, selectionNode, trimmed);
    if (IsSelectionOnControlFlowJumpStatement(refContext, trimmed)) {
        RemoveApplicableActionsIf(resList, [](const RefactorAction &action) {
            return action.name == std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.name);
        });
    }
}

std::vector<ApplicableRefactorInfo> ExtractSymbolRefactor::GetAvailableActions(const RefactorContext &refContext) const
{
    const auto rangeToExtract = refContext.span;
    if (rangeToExtract.pos >= rangeToExtract.end) {
        return {};
    }
    if (HasImportDeclarationOverlap(refContext, GetTrimmedSelectionSpan(refContext))) {
        return {};
    }
    auto refactoredNodeList = FindAvailableRefactors(refContext);
    if (refactoredNodeList.empty()) {
        const bool recoveredFunction = TryAddFunctionActionForEmptyRefactors(refContext, refactoredNodeList);
        const bool recoveredVariable = TryAddVariableActionForEmptyRefactors(refContext, refactoredNodeList);
        const bool recoveredConstant = TryAddConstantActionForEmptyRefactors(refContext, refactoredNodeList);
        if (!recoveredFunction && !recoveredVariable && !recoveredConstant) {
            return {};
        }
    }
    if (IsSelectionWithinControlFlowTest(refContext)) {
        RemoveControlFlowConstantActions(refactoredNodeList);
    }
    AddWholeDeclFunctionActionIfMissing(refContext, refactoredNodeList);
    auto resList = BuildApplicableRefactorInfoList(refContext, refactoredNodeList);
    ApplyAvailableActionPostFilters(refContext, resList);
    return resList;
}

ir::AstNode *IsReplaceRangeRequired(const RefactorContext &context, ir::AstNode *extractedText)
{
    if (extractedText == nullptr) {
        return nullptr;
    }
    const auto trimmedSpan = GetTrimmedSelectionSpan(context);
    const bool isTopLevelLiteralSelection = IsTopLevelLiteralSelectionText(context, trimmedSpan);
    if (extractedText->IsExpressionStatement() && extractedText->Start().index <= context.span.pos &&
        extractedText->End().index >= context.span.end && isTopLevelLiteralSelection) {
        return extractedText;
    }
    return nullptr;
}

struct GlobalConstExtractionAdjustResult {
    ir::AstNode *extractedText {nullptr};
    TextRange extractedRange {};
};

static ir::AstNode *ResolveDeclInitializerForGlobalConst(const RefactorContext &context)
{
    const TextRange trimmed = GetTrimmedSelectionSpan(context);
    if (auto *decl = FindContainingDeclaratorByRange(context, trimmed); decl != nullptr && decl->Init() != nullptr) {
        return decl->Init();
    }
    return ResolveDeclarationInitializerByRange(context, trimmed);
}

static TextRange ComputeExtractedRangeForGlobalConst(const RefactorContext &context, ir::AstNode *extractedText,
                                                     TextRange currentRange, const std::optional<TextRange> &rhsRange)
{
    if (extractedText != nullptr && !extractedText->IsExpression()) {
        return rhsRange.has_value() ? rhsRange.value() : currentRange;
    }
    if (rhsRange.has_value() && extractedText != nullptr && extractedText->Start().index <= rhsRange->pos &&
        extractedText->End().index >= rhsRange->end) {
        return rhsRange.value();
    }
    if (auto *initializerExpr = ResolveDeclarationInitializerByRange(context, GetTrimmedSelectionSpan(context));
        initializerExpr != nullptr) {
        return {initializerExpr->Start().index, initializerExpr->End().index};
    }
    return currentRange;
}

static GlobalConstExtractionAdjustResult AdjustGlobalConstDeclarationSelection(const RefactorContext &context,
                                                                               ir::AstNode *extractedText,
                                                                               TextRange extractedRange)
{
    const TextRange trimmed = GetTrimmedSelectionSpan(context);
    const auto rhsRange = ResolveInitializerRhsRange(context, trimmed);
    const bool isDeclRhsSelection =
        rhsRange.has_value() && rhsRange->pos == trimmed.pos && rhsRange->end == trimmed.end;
    if (!isDeclRhsSelection) {
        return {extractedText, extractedRange};
    }
    if (auto *initializerExpr = ResolveDeclInitializerForGlobalConst(context); initializerExpr != nullptr) {
        extractedText = initializerExpr;
    }
    extractedText = NormalizeGlobalConstantExtractedNode(context, extractedText);
    extractedRange = ComputeExtractedRangeForGlobalConst(context, extractedText, extractedRange, rhsRange);
    return {extractedText, extractedRange};
}

static std::pair<std::vector<FileTextChanges>, ir::AstNode *> BuildValueExtractionChanges(
    const RefactorContext &context, ir::AstNode *extractedText, const std::pair<size_t, std::string> &insertionData,
    const std::string &uniqueVarName, const std::string &actionName)
{
    TextChangesContext textChangesContext = *context.textChangesContext;
    const auto src = reinterpret_cast<public_lib::Context *>(context.context)->sourceFile;
    TextRange extractedRange {extractedText->Start().index, extractedText->End().index};
    const TextRange trimmedSpan = GetTrimmedSelectionSpan(context);
    if ((IsVariableExtractionAction(actionName) || IsConstantExtractionAction(actionName)) &&
        trimmedSpan.pos < trimmedSpan.end) {
        extractedRange = trimmedSpan;
    }
    if (IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
        auto adjusted = AdjustGlobalConstDeclarationSelection(context, extractedText, extractedRange);
        extractedText = adjusted.extractedText;
        extractedRange = adjusted.extractedRange;
        if (IsSelectionInsideTypeAnnotationContext(context, trimmedSpan) &&
            IsQuotedLiteralSelectionText(context, trimmedSpan)) {
            if (auto laterSpan = FindLaterMatchingLiteralInitializerSpan(context, trimmedSpan); laterSpan.has_value()) {
                extractedRange = laterSpan.value();
            }
        }
        if (trimmedSpan.end > trimmedSpan.pos && extractedRange.pos <= trimmedSpan.pos &&
            extractedRange.end >= trimmedSpan.end &&
            (extractedRange.pos != trimmedSpan.pos || extractedRange.end != trimmedSpan.end)) {
            extractedRange = trimmedSpan;
        }
    }
    auto *exprStmt = IsReplaceRangeRequired(context, extractedText);
    exprStmt = ResolveExprStmtForValueExtraction(context, extractedText, actionName, exprStmt, src);
    auto applyDirectReplace = [src, &insertionData, extractedRange, &uniqueVarName](ChangeTracker &tracker) {
        tracker.InsertText(src, insertionData.first, insertionData.second);
        tracker.ReplaceRangeWithText(src, extractedRange, uniqueVarName);
    };
    auto edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        const bool applied =
            TryApplyExprStmtExtractionEdit(tracker, {src, exprStmt, insertionData, extractedRange, actionName});
        if (!applied) {
            applyDirectReplace(tracker);
        }
    });
    return {std::move(edits), exprStmt};
}

static bool IsValidGlobalConstantSelection(const RefactorContext &context, const std::string &actionName)
{
    if (!IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
        return true;
    }
    const TextRange trimmedSpan = GetTrimmedSelectionSpan(context);
    if (IsFullInitializerRhsSelection(context, trimmedSpan) ||
        HasExactGlobalConstantExpressionSelection(context, trimmedSpan)) {
        return true;
    }
    if (ResolveInitializerExpressionContainingSelection(context, trimmedSpan) != nullptr) {
        return false;
    }
    if (HasExactClassPropertyInitializerSelection(context, trimmedSpan)) {
        return true;
    }
    return IsNonArithmeticCoveringExpressionSelection(context, trimmedSpan);
}

static bool IsFullInitializerRhsSelection(const RefactorContext &context, TextRange trimmedSpan)
{
    const auto rhsRange = ResolveInitializerRhsRange(context, trimmedSpan);
    return rhsRange.has_value() && rhsRange->pos == trimmedSpan.pos && rhsRange->end == trimmedSpan.end;
}

static bool HasExactGlobalConstantExpressionSelection(const RefactorContext &context, TextRange trimmedSpan)
{
    if (auto *selectedExpr = FindExactSelectionExpression(context, trimmedSpan);
        selectedExpr != nullptr && selectedExpr->IsExpression()) {
        return true;
    }
    if (auto *resolvedExpr = ResolveGlobalConstantSelectionExpression(context, trimmedSpan);
        resolvedExpr != nullptr && resolvedExpr->IsExpression() && resolvedExpr->Start().index == trimmedSpan.pos &&
        resolvedExpr->End().index == trimmedSpan.end) {
        return true;
    }
    if (auto *touch = GetTouchingTokenByRange(context.context, trimmedSpan, false); touch != nullptr) {
        if (auto *opt = GetOptimumNodeByRange(touch, trimmedSpan); opt != nullptr && opt->IsExpression() &&
                                                                   opt->Start().index == trimmedSpan.pos &&
                                                                   opt->End().index == trimmedSpan.end) {
            return true;
        }
    }
    return false;
}

static bool HasExactClassPropertyInitializerSelection(const RefactorContext &context, TextRange trimmedSpan)
{
    if (IsClassPropertyInitializerRhsSelection(context, trimmedSpan)) {
        return true;
    }
    if (auto *prop = FindContainingClassPropertyByRange(context, trimmedSpan); prop != nullptr) {
        auto *value = prop->Value();
        return value != nullptr && value->Start().index == trimmedSpan.pos && value->End().index == trimmedSpan.end;
    }
    return false;
}

static bool IsNonArithmeticCoveringExpressionSelection(const RefactorContext &context, TextRange trimmedSpan)
{
    auto *coverExpr = ResolveExpressionCoveringRange(context, trimmedSpan);
    if (coverExpr == nullptr || !coverExpr->IsExpression() || coverExpr->Start().index > trimmedSpan.pos ||
        coverExpr->End().index < trimmedSpan.end) {
        return false;
    }
    const bool isExactCover = coverExpr->Start().index == trimmedSpan.pos && coverExpr->End().index == trimmedSpan.end;
    return isExactCover || !IsArithmeticBinaryExpression(coverExpr);
}

static ir::AstNode *ResolveValueExtractionDeclarationNode(const RefactorContext &context, ir::AstNode *extractedText,
                                                          const std::string &actionName)
{
    if (IsConstantExtractionAction(actionName)) {
        const TextRange trimmedSpan = GetTrimmedSelectionSpan(context);
        if (IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
            if (auto *selectedExpr = ResolveGlobalConstantSelectionExpression(context, trimmedSpan);
                selectedExpr != nullptr) {
                return selectedExpr;
            }
        }
        if (auto *exactExpr = FindExactSelectionExpression(context, trimmedSpan); exactExpr != nullptr) {
            return exactExpr;
        }
        if (auto *coverExpr = ResolveExpressionCoveringRange(context, trimmedSpan);
            coverExpr != nullptr && coverExpr->IsExpression() && coverExpr->Start().index <= trimmedSpan.pos &&
            coverExpr->End().index >= trimmedSpan.end) {
            return coverExpr;
        }
    }
    ir::AstNode *declarationNode = extractedText;
    if (!IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
        return declarationNode;
    }
    TextRange trimmedSpan = GetTrimmedSelectionSpan(context);
    if (auto *initializerExpr = ResolveDeclarationInitializerByRange(context, trimmedSpan);
        initializerExpr != nullptr && initializerExpr->Start().index == trimmedSpan.pos &&
        initializerExpr->End().index == trimmedSpan.end) {
        declarationNode = initializerExpr;
    }
    return NormalizeGlobalConstantExtractedNode(context, declarationNode);
}

struct ValueExtractionInsertState {
    size_t insertPos {0};
    std::string generatedText;
    bool inlineInsertion {false};
};

struct ValueExtractionInsertInputs {
    const RefactorContext &context;
    public_lib::Context *ctx {nullptr};
    ir::AstNode *declarationNode {nullptr};
    const std::string &actionName;
    TextRange trimmedSpan {};
};

static void ResolveBaseValueExtractionInsertPos(const ValueExtractionInsertInputs &inputs,
                                                ValueExtractionInsertState &state)
{
    state.insertPos = GetVarAndFunctionPosToWriteNode(inputs.context, inputs.actionName).pos;
    if (IsActionNameOrKind(inputs.actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE) &&
        !IsNamespaceAction(inputs.actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE.name,
                           EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX)) {
        if (auto insertPosFromNode =
                FindDeclarationInsertPosFromNode(inputs.ctx->sourceFile->source, inputs.declarationNode);
            insertPosFromNode.has_value()) {
            state.insertPos = insertPosFromNode.value();
        } else if (auto *decl = FindContainingDeclaratorByRange(inputs.context, inputs.trimmedSpan); decl != nullptr) {
            state.insertPos = FindLineStart(inputs.ctx->sourceFile->source, decl->Start().index);
        } else if (auto *prop = FindContainingClassPropertyByRange(inputs.context, inputs.trimmedSpan);
                   prop != nullptr) {
            state.insertPos = FindLineStart(inputs.ctx->sourceFile->source, prop->Start().index);
        }
    }
    if (const auto namespaceDepth = GetNamespaceActionDepth(inputs.actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE.name,
                                                            EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX);
        namespaceDepth.has_value() && namespaceDepth.value() > 0) {
        auto *touchNode = GetTouchingToken(inputs.context.context, inputs.context.span.pos, false);
        if (auto *namespaceScope = FindNamespaceScopeByDepth(touchNode, namespaceDepth.value());
            namespaceScope != nullptr) {
            state.insertPos = FindNamespaceBodyInsertPos(inputs.ctx->sourceFile->source, namespaceScope);
            if (auto declEnd =
                    FindLastVarDeclEndInRange(inputs.ctx->sourceFile->source, state.insertPos, inputs.context.span.pos);
                declEnd.has_value()) {
                state.insertPos = declEnd.value();
            }
        }
    }
    state.insertPos = ResolveInsertionPosForVariableExtraction(inputs.context, inputs.ctx, inputs.actionName,
                                                               state.insertPos, inputs.trimmedSpan);
    if (IsActionNameOrKind(inputs.actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE) &&
        FindEnclosingClassDefinition(inputs.declarationNode) != nullptr &&
        FindScriptFunction(inputs.declarationNode) == nullptr &&
        CollectEnclosingNamespaceScopes(inputs.declarationNode).empty()) {
        state.insertPos = FindLineStart(inputs.ctx->sourceFile->source, inputs.context.span.pos);
    }
}

static void ResolveGlobalValueExtractionInsertPos(const RefactorContext &context, public_lib::Context *ctx,
                                                  const std::string &actionName, TextRange trimmedSpan,
                                                  ValueExtractionInsertState &state)
{
    if (!IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL) ||
        IsNamespaceAction(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE.name, EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX)) {
        return;
    }
    const size_t globalInsertPos = DetermineGlobalInsertPos(ctx);
    state.insertPos = ResolveGlobalConstantInsertionPosFromSource(ctx->sourceFile->source, context.span.pos,
                                                                  globalInsertPos, globalInsertPos);
    const size_t selectionLineStart = FindLineStart(ctx->sourceFile->source, context.span.pos);
    const auto classEnd = FindLastTopLevelClassEndBefore(ctx->sourceFile->source, selectionLineStart);
    if (auto topLevelDeclEnd = FindLastTopLevelVarDeclEndBefore(ctx->sourceFile->source, context.span.pos);
        topLevelDeclEnd.has_value() && topLevelDeclEnd.value() >= globalInsertPos &&
        (topLevelDeclEnd.value() < selectionLineStart || classEnd.has_value())) {
        state.insertPos = topLevelDeclEnd.value();
    } else if (classEnd.has_value()) {
        state.insertPos = classEnd.value();
    }
    if (state.insertPos == 0) {
        if (auto headerEnd = FindFileHeaderBlockCommentEnd(ctx->sourceFile->source); headerEnd.has_value()) {
            state.insertPos = headerEnd.value();
        }
    }
    if (IsClassPropertyInitializerRhsSelection(context, trimmedSpan)) {
        state.insertPos = globalInsertPos;
    }
    state.insertPos = AdvanceInsertPosToBlankLineStart(ctx->sourceFile->source, state.insertPos);
}

struct FinalizeValueExtractionTextInputs {
    const RefactorContext &context;
    public_lib::Context *ctx {nullptr};
    ir::AstNode *declarationNode {nullptr};
    const std::string &actionName;
    const std::string &uniqueVarName;
};

static void FinalizeValueExtractionGeneratedText(const FinalizeValueExtractionTextInputs &inputs,
                                                 ValueExtractionInsertState &state)
{
    const bool multiDeclInsertionText = IsMultiDeclaratorInsertionText(state.generatedText, inputs.uniqueVarName);
    if (state.inlineInsertion) {
        return;
    }
    if (IsActionNameOrKind(inputs.actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE)) {
        if (auto keywordStart = FindVariableDeclKeywordStart(inputs.ctx->sourceFile->source, inputs.context.span.pos);
            keywordStart.has_value()) {
            state.insertPos = FindLineStart(inputs.ctx->sourceFile->source, keywordStart.value());
        }
    }
    bool keepGlobalConstantInlineText = false;
    if (multiDeclInsertionText && IsActionNameOrKind(inputs.actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
        if (auto commaInsertPos =
                FindCurrentDeclarationCommaInsertPos(inputs.ctx->sourceFile->source, inputs.context.span);
            commaInsertPos.has_value()) {
            state.insertPos = commaInsertPos.value();
            keepGlobalConstantInlineText = true;
        } else {
            RemoveTrailingLineBreaks(state.generatedText);
        }
    }
    if (multiDeclInsertionText && !IsActionNameOrKind(inputs.actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
        return;
    }
    if (IsActionNameOrKind(inputs.actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
        if (!keepGlobalConstantInlineText) {
            RemoveTrailingLineBreaks(state.generatedText);
        }
    } else {
        AdjustGeneratedTextForInsert(inputs.context, inputs.ctx, state.insertPos, inputs.uniqueVarName,
                                     state.generatedText);
    }
    AppendTrailingNewLineForGlobalVariableInsert(inputs.context, inputs.actionName, state.insertPos,
                                                 state.generatedText);
    AppendTrailingNewLineForConstantEncloseInsert(inputs.context, inputs.actionName, state.insertPos,
                                                  inputs.ctx->sourceFile->source, state.generatedText);
    MaybePrependNamespaceNewlinesForValueExtraction(inputs.context, state.insertPos, state.generatedText,
                                                    {&inputs.actionName, inputs.declarationNode});
}

static ValueExtractionInsertState BuildValueExtractionInsertState(const RefactorContext &context,
                                                                  public_lib::Context *ctx,
                                                                  ir::AstNode *declarationNode,
                                                                  const std::string &actionName,
                                                                  const std::string &uniqueVarName)
{
    ValueExtractionInsertState state {};
    state.generatedText = GenerateInlineEdits(context, declarationNode, actionName, uniqueVarName);
    if (state.generatedText.empty()) {
        return state;
    }
    const TextRange trimmedSpan = GetTrimmedSelectionSpan(context);
    ResolveBaseValueExtractionInsertPos({context, ctx, declarationNode, actionName, trimmedSpan}, state);
    ResolveGlobalValueExtractionInsertPos(context, ctx, actionName, trimmedSpan, state);
    auto inlineInsertionResult = TryBuildInlineInsertion({context, ctx, declarationNode, actionName, uniqueVarName});
    state.inlineInsertion = ApplyInlineInsertionResult(state.insertPos, state.generatedText, inlineInsertionResult);
    FinalizeValueExtractionGeneratedText({context, ctx, declarationNode, actionName, uniqueVarName}, state);
    return state;
}

static ir::AstNode *ResolveExtractedNodeForValueAction(const RefactorContext &context, ir::AstNode *extractedText,
                                                       const std::string &actionName)
{
    const TextRange trimmed = GetTrimmedSelectionSpan(context);
    if (IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
        if (auto *selectedExpr = ResolveGlobalConstantSelectionExpression(context, trimmed); selectedExpr != nullptr) {
            return selectedExpr;
        }
    }
    if (IsVariableExtractionAction(actionName)) {
        if (auto *exactExpr = FindExactSelectionExpression(context, trimmed); exactExpr != nullptr) {
            return exactExpr;
        }
        if (auto *coverExpr = ResolveExpressionCoveringRange(context, trimmed); coverExpr != nullptr) {
            return coverExpr;
        }
        if (auto *initializerExpr = ResolveInitializerExpressionContainingSelection(context, trimmed);
            initializerExpr != nullptr && initializerExpr->Start().index <= trimmed.pos &&
            initializerExpr->End().index >= trimmed.end) {
            return initializerExpr;
        }
        return extractedText;
    }
    if (IsConstantExtractionAction(actionName)) {
        if (auto *exactExpr = FindExactSelectionExpression(context, trimmed); exactExpr != nullptr) {
            return exactExpr;
        }
        if (auto *coverExpr = ResolveExpressionCoveringRange(context, trimmed);
            coverExpr != nullptr && coverExpr->Start().index <= trimmed.pos && coverExpr->End().index >= trimmed.end) {
            return coverExpr;
        }
    }
    return extractedText;
}

static ir::AstNode *AdjustExtractedNodeForVariableDeclarationNode(const RefactorContext &context,
                                                                  ir::AstNode *extractedText,
                                                                  ir::AstNode *declarationNode,
                                                                  const std::string &actionName)
{
    if (!IsVariableExtractionAction(actionName)) {
        return extractedText;
    }
    const TextRange trimmed = GetTrimmedSelectionSpan(context);
    if (declarationNode != nullptr && declarationNode->Start().index <= trimmed.pos &&
        declarationNode->End().index >= trimmed.end) {
        return declarationNode;
    }
    return extractedText;
}

static bool IsRepeatedOccurrenceInClassPropertyInitializer(const RefactorContext &context, TextRange range)
{
    auto *node = GetTouchingTokenByRange(context.context, range, false);
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (!current->IsClassProperty()) {
            continue;
        }
        auto *prop = current->AsClassProperty();
        auto *value = prop == nullptr ? nullptr : prop->Value();
        return value != nullptr && value->Start().index <= range.pos && value->End().index >= range.end;
    }
    return false;
}

static bool HasUseStaticDirectiveInSourceFile(public_lib::Context *ctx)
{
    return ctx != nullptr && ctx->sourceFile != nullptr && HasUseStaticDirective(ctx->sourceFile->source);
}

struct RepeatedValueExtractionEditInput {
    const RefactorContext &context;
    public_lib::Context *ctx {nullptr};
    std::string_view generatedText;
    const std::string &uniqueVarName;
    const std::string &actionName;
    size_t baseInsertPos {0};
};

static size_t ResolveRepeatedValueExtractionInsertPos(const RepeatedValueExtractionEditInput &input,
                                                      std::string_view source,
                                                      const std::vector<TextRange> &occurrences)
{
    if (occurrences.empty()) {
        return std::min(input.baseInsertPos, source.size());
    }
    if (IsActionNameOrKind(input.actionName, EXTRACT_CONSTANT_ACTION_GLOBAL) &&
        HasUseStaticDirectiveInSourceFile(input.ctx) &&
        IsRepeatedOccurrenceInClassPropertyInitializer(input.context, occurrences.front())) {
        return std::min(input.baseInsertPos, source.size());
    }
    return FindLineStart(source, occurrences.front().pos);
}

static std::optional<RefactorEditInfo> BuildRepeatedValueExtractionEdits(const RepeatedValueExtractionEditInput &input)
{
    auto *ctx = input.ctx;
    if (ctx == nullptr || ctx->sourceFile == nullptr || input.generatedText.empty()) {
        return std::nullopt;
    }
    const std::vector<TextRange> occurrences =
        CollectRepeatedValueExtractionOccurrences(input.context, ctx, input.actionName);
    if (occurrences.size() < 2U) {
        return std::nullopt;
    }

    const std::string_view source = ctx->sourceFile->source;
    const size_t insertPos = ResolveRepeatedValueExtractionInsertPos(input, source, occurrences);
    std::string repeatedGeneratedText =
        PrepareValueDeclarationInsertText(input.context, source, insertPos, std::string(input.generatedText));
    std::vector<TextChange> textChanges;
    textChanges.emplace_back(TextSpan {insertPos, 0}, std::move(repeatedGeneratedText));
    for (const auto &range : occurrences) {
        textChanges.emplace_back(TextSpan {range.pos, range.end - range.pos}, input.uniqueVarName);
    }

    FileTextChanges fileChange;
    fileChange.fileName = std::string(ctx->sourceFile->filePath);
    fileChange.textChanges = std::move(textChanges);
    return RefactorEditInfo(std::vector<FileTextChanges> {std::move(fileChange)},
                            std::optional<std::string>(ctx->sourceFile->filePath), std::nullopt);
}

RefactorEditInfo GetRefactorEditsToExtractVals(const RefactorContext &context, ir::AstNode *extractedText,
                                               const std::string &actionName)
{
    if (!IsValidGlobalConstantSelection(context, actionName)) {
        return RefactorEditInfo {};
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return RefactorEditInfo {};
    }
    if (auto edits = TryBuildGlobalConstantMultiDeclaratorEdits(context, ctx, actionName); edits.has_value()) {
        return std::move(edits.value());
    }
    extractedText = ResolveExtractedNodeForValueAction(context, extractedText, actionName);
    std::string uniqueVarName = GenerateUniqueExtractedVarName(context, actionName);
    ir::AstNode *declarationNode = ResolveValueExtractionDeclarationNode(context, extractedText, actionName);
    extractedText = AdjustExtractedNodeForVariableDeclarationNode(context, extractedText, declarationNode, actionName);
    ValueExtractionInsertState insertState =
        BuildValueExtractionInsertState(context, ctx, declarationNode, actionName, uniqueVarName);
    if (insertState.generatedText.empty()) {
        return RefactorEditInfo {};
    }
    const auto src = ctx->sourceFile;
    std::string implicitPrefix = BuildImplicitPrefix(context, ctx, insertState.insertPos, insertState.generatedText,
                                                     insertState.inlineInsertion);
    auto [edits, exprStmt] = BuildValueExtractionChanges(
        context, extractedText, {insertState.insertPos, insertState.generatedText}, uniqueVarName, actionName);
    size_t renameLoc =
        ResolveValueExtractionRenameLoc({actionName, src->source, extractedText, edits, exprStmt, insertState.insertPos,
                                         insertState.generatedText, uniqueVarName, implicitPrefix});
    if (IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_ENCLOSE) ||
        IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_GLOBAL) ||
        IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_CLASS) ||
        IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE) ||
        IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
        RepeatedValueExtractionEditInput repeatedInput {context,       ctx,        insertState.generatedText,
                                                        uniqueVarName, actionName, insertState.insertPos};
        if (auto repeatedEdits = BuildRepeatedValueExtractionEdits(repeatedInput); repeatedEdits.has_value()) {
            auto repeatedFileTextChanges = repeatedEdits->GetFileTextChanges();
            edits.insert(edits.end(), repeatedFileTextChanges.begin(), repeatedFileTextChanges.end());
        }
    }
    return RefactorEditInfo(std::move(edits), std::optional<std::string>(src->filePath),
                            std::optional<size_t>(renameLoc));
}

std::unique_ptr<RefactorEditInfo> ExtractSymbolRefactor::GetEditsForAction(const RefactorContext &context,
                                                                           const std::string &actionName) const
{
    const auto ctx = context.context;
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (ctx == nullptr || impl == nullptr) {
        return nullptr;
    }
    const auto rangeToExtract = context.span;
    if (rangeToExtract.pos >= rangeToExtract.end) {
        return nullptr;
    }

    const auto extractedText = FindRefactor(context, actionName);
    if (extractedText == nullptr) {
        return nullptr;
    }
    RefactorEditInfo refactor;
    if (IsConstantExtractionAction(actionName) || IsVariableExtractionAction(actionName)) {
        refactor = GetRefactorEditsToExtractVals(context, extractedText, actionName);
    } else if (actionName == EXTRACT_FUNCTION_ACTION_GLOBAL.name || actionName == EXTRACT_FUNCTION_ACTION_CLASS.name ||
               IsNamespaceAction(actionName, EXTRACT_FUNCTION_ACTION_ENCLOSE.name,
                                 EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX)) {
        refactor = GetRefactorEditsToExtractFunction(context, actionName);
    }

    return std::make_unique<RefactorEditInfo>(refactor);
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ExtractSymbolRefactor> g_extractSymbolRefactorRegister("ExtractSymbolRefactor");

}  // namespace ark::es2panda::lsp
