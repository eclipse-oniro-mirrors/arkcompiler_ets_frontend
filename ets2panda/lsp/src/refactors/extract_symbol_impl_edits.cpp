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

static bool IsSelectionInsideUnterminatedDeclarationInitializer(const RefactorContext &context, TextRange trimmed);
static bool IsLiteralSelectionInsideDeclarationInitializer(const RefactorContext &context, TextRange trimmed);
ir::AstNode *FindExactSelectionExpression(const RefactorContext &context, TextRange selection);
ir::AstNode *ResolveExpressionCoveringRange(const RefactorContext &context, TextRange initRange);

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
    if (state.wholeDeclSelectionNode != nullptr) {
        // A whole declaration selection is a valid statement-shape candidate for function extraction.
        state.canExtractFunctionBySelectionShape = true;
        state.declInitializerExpr = ResolveInitializerExpressionFromDeclarationSelection(context, state.trimmedSpan);
    }
    if (state.declInitializerExpr == nullptr) {
        if (auto *candidate = ResolveDeclarationInitializerByRange(context, state.trimmedSpan);
            candidate != nullptr && candidate->Start().index == state.trimmedSpan.pos &&
            candidate->End().index == state.trimmedSpan.end) {
            state.declInitializerExpr = candidate;
        }
    }
    if (state.declInitializerExpr == nullptr) {
        if (auto rhsRange = ResolveInitializerRhsRange(context, state.trimmedSpan);
            rhsRange.has_value() && rhsRange->pos == state.trimmedSpan.pos && rhsRange->end == state.trimmedSpan.end) {
            if (auto *exactExpr = FindExactSelectionExpression(context, rhsRange.value()); exactExpr != nullptr) {
                state.declInitializerExpr = exactExpr;
            }
        }
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
        if (hasNamespaceScope && scope.hasClassScope) {
            AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_ENCLOSE,
                              BuildNamedScopeDescription("constant", "namespace", scope.namespaceScopeNames.front()));
        } else {
            AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_ENCLOSE);
        }
    } else {
        AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_GLOBAL);
    }
    for (size_t namespaceDepth = 1; namespaceDepth < scope.namespaceScopeNames.size(); ++namespaceDepth) {
        AddRefactorAction(
            actions, EXTRACT_CONSTANT_ACTION_ENCLOSE,
            BuildNamedScopeDescription("constant", "namespace", scope.namespaceScopeNames[namespaceDepth]),
            BuildNamespaceScopedActionName(EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX, namespaceDepth),
            std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.kind));
    }
    if (scope.hasClassScope) {
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
    TextRange dependencySpan = state.trimmedSpan;
    if (state.declInitializerExpr != nullptr) {
        dependencySpan = {state.declInitializerExpr->Start().index, state.declInitializerExpr->End().index};
    }
    flags.hasNamespacePrivateDependency =
        HasNamespacePrivateSymbolDependencyForGlobalExtraction(context, dependencySpan);
    flags.hasLocalValueDependency = HasLocalValueDependencyInSelection(context, dependencySpan);
    const bool isTopLevelScriptSelection = FindScriptFunction(state.node) == nullptr && !scope.hasClassScope &&
                                           scope.namespaceScopeNames.empty() && !scope.hasEncloseScope;
    if (isTopLevelScriptSelection) {
        flags.hasLocalValueDependency = false;
    }
    if (!state.selectionHasNewline) {
        if (auto *exactExpr = FindExactSelectionExpression(context, state.trimmedSpan);
            exactExpr != nullptr && exactExpr->IsArrowFunctionExpression()) {
            flags.hasLocalValueDependency = false;
        }
    }
    flags.hasExternalLocalWriteDependency = HasExternalLocalWriteDependencyInSelection(context, state.trimmedSpan);
    const bool isInsideFunction = FindScriptFunction(state.node) != nullptr;
    const bool hasDeclarationLeadingExternalWriteUsage =
        HasDeclarationLeadingExternalWriteUsage(context, ctx, state.trimmedSpan);
    const bool disallowGlobalForFunctionMultilineDeclarationLeadingSelection =
        hasDeclarationLeadingExternalWriteUsage && isInsideFunction && state.selectionHasNewline &&
        state.wholeDeclSelectionNode == nullptr;
    flags.disallowGlobalFunctionForDeclarationLeadingExternalWrite =
        hasDeclarationLeadingExternalWriteUsage &&
        ((flags.hasExternalLocalWriteDependency && (!state.selectionHasNewline || isInsideFunction)) ||
         disallowGlobalForFunctionMultilineDeclarationLeadingSelection);
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
    EnsureArrowTopLevelGlobalConstantAction(actions, context, state, scope);
    EnsureVariableEncloseActionForScopedSelection(actions, context, state, scope, flags);
    ApplyInitializerLiteralSelectionRules(actions, context, state, scope);
    RebalanceVariableActionByScope(actions, context, state, scope, flags);
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
        return FindExtractedVals(context);
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
    auto *node = GetTouchingTokenByRange(context.context, context.span, false);
    auto *declarator = FindEnclosingVariableDeclarator(node);
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
    const size_t declaratorPos = declarators[*declaratorIndex]->Start().index;
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
    if (IsActionNameOrKind(inputs.actionName, EXTRACT_VARIABLE_ACTION_GLOBAL)) {
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

static size_t ResolveMultiDeclaratorFallbackInsertPos(std::string_view source, TextRange extractionSpan,
                                                      size_t insertPos)
{
    if (extractionSpan.pos >= source.size()) {
        return insertPos;
    }
    const size_t declarationStart = FindLineStart(source, extractionSpan.pos);
    return FindInsertionPosBeforeTightLeadingComment(source, declarationStart);
}

static size_t ResolveInsertionPosForVariableExtraction(const RefactorContext &context, public_lib::Context *ctx,
                                                       const std::string &actionName, size_t insertPos)
{
    const bool isVariableGlobal = IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_GLOBAL);
    const bool isVariableEnclose = IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_ENCLOSE);
    const bool isConstantEnclose = IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE);
    if (!(isVariableGlobal || isVariableEnclose || isConstantEnclose) || ctx == nullptr || ctx->sourceFile == nullptr) {
        return insertPos;
    }
    if (isVariableEnclose || isConstantEnclose) {
        if (auto keywordStart = FindVariableDeclKeywordStart(ctx->sourceFile->source, context.span.pos);
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

static void AppendTrailingNewLineForGlobalVariableInsert(const RefactorContext &context, const std::string &actionName,
                                                         size_t insertPos, std::string &generatedText)
{
    if (!(IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_GLOBAL) ||
          IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_ENCLOSE) ||
          IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL))) {
        return;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || generatedText.empty()) {
        return;
    }
    const auto &source = ctx->sourceFile->source;
    if (insertPos < source.size() && !IsLineBreakChar(source[insertPos]) && !IsLineBreakChar(generatedText.back())) {
        generatedText.append(context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter());
    }
}

static void AppendTrailingNewLineForConstantEncloseInsert(const RefactorContext &context, const std::string &actionName,
                                                          size_t insertPos, std::string_view source,
                                                          std::string &generatedText)
{
    if (!IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE) || generatedText.empty()) {
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
    const bool isQuotedLiteral = selected.size() >= 2 && ((selected.front() == '"' && selected.back() == '"') ||
                                                          (selected.front() == '\'' && selected.back() == '\''));
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
        selectedExpr = ResolveExpressionCoveringRange(refContext, trimmed);
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

static bool TryAddConstantActionForEmptyRefactors(const RefactorContext &refContext,
                                                  std::vector<RefactorAction> &actions)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(refContext.context);
    const TextRange trimmed = GetTrimmedSelectionSpan(refContext);
    if (ctx == nullptr || ctx->sourceFile == nullptr || trimmed.end <= trimmed.pos ||
        trimmed.end > ctx->sourceFile->source.size() || HasSelectionNewline(refContext, ctx->sourceFile->source)) {
        return false;
    }
    ir::AstNode *selectedExpr = FindExactSelectionExpression(refContext, trimmed);
    if (selectedExpr == nullptr) {
        selectedExpr = ResolveExpressionCoveringRange(refContext, trimmed);
    }
    if (selectedExpr == nullptr || !selectedExpr->IsExpression() || ContainsThisOrSuperExpression(selectedExpr) ||
        ContainsThisOrSuperInRange(ctx, trimmed)) {
        return false;
    }
    const auto scope = ResolveScopeContext(selectedExpr);
    if (!scope.hasEncloseScope && scope.namespaceScopeNames.empty()) {
        AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_GLOBAL);
        return true;
    }
    AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_ENCLOSE);
    AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_GLOBAL);
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

static std::vector<ApplicableRefactorInfo> BuildApplicableRefactorInfoList(
    const RefactorContext &refContext, const std::vector<RefactorAction> &refactoredNodeList)
{
    std::vector<ApplicableRefactorInfo> resList;
    for (const RefactorAction &ref : refactoredNodeList) {
        if (!refContext.kind.empty() && refContext.kind != ref.kind) {
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

static void EnsureVariableEncloseForTypeofSelection(std::vector<ApplicableRefactorInfo> &resList,
                                                    ir::AstNode *selectionNode)
{
    const bool isTypeofSelection = selectionNode != nullptr && selectionNode->IsTypeofExpression();
    if (!isTypeofSelection) {
        return;
    }
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

static void ApplyAvailableActionPostFilters(const RefactorContext &refContext,
                                            std::vector<ApplicableRefactorInfo> &resList)
{
    const TextRange trimmed = GetTrimmedSelectionSpan(refContext);
    ir::AstNode *selectionNode = ResolveSelectionNodeForApplicableActions(refContext, trimmed);
    EnsureVariableEncloseForTypeofSelection(resList, selectionNode);
    if (IsSelectionInsideTypeAnnotationContext(refContext, trimmed)) {
        RemoveValueAndFunctionActionsForTypeContext(resList);
    }
    if (IsSelectionWithinControlFlowTest(refContext)) {
        auto it = resList.begin();
        while (it != resList.end()) {
            const bool isEnclose = it->action.name == std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
            const bool isNamespace = it->action.name.rfind(EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX, 0) == 0;
            if (isEnclose || isNamespace) {
                it = resList.erase(it);
                continue;
            }
            ++it;
        }
    }
    if (IsSelectionInsideClassExpressionMethod(refContext)) {
        auto it = resList.begin();
        while (it != resList.end()) {
            if (it->action.name == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name)) {
                it = resList.erase(it);
                continue;
            }
            ++it;
        }
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
        const bool recoveredVariable = TryAddVariableActionForEmptyRefactors(refContext, refactoredNodeList);
        const bool recoveredConstant = TryAddConstantActionForEmptyRefactors(refContext, refactoredNodeList);
        if (!recoveredVariable && !recoveredConstant &&
            !TryAddFunctionActionForEmptyRefactors(refContext, refactoredNodeList)) {
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
    const auto rhsRange = ResolveInitializerRhsRange(context, trimmedSpan);
    const bool isFullDeclRhsSelection =
        rhsRange.has_value() && rhsRange->pos == trimmedSpan.pos && rhsRange->end == trimmedSpan.end;
    bool isExactExpressionSelection = FindExactSelectionExpression(context, trimmedSpan) != nullptr;
    if (!isExactExpressionSelection) {
        if (auto *touch = GetTouchingTokenByRange(context.context, trimmedSpan, false); touch != nullptr) {
            if (auto *opt = GetOptimumNodeByRange(touch, trimmedSpan); opt != nullptr && opt->IsExpression() &&
                                                                       opt->Start().index == trimmedSpan.pos &&
                                                                       opt->End().index == trimmedSpan.end) {
                isExactExpressionSelection = true;
            }
        }
    }
    if (isFullDeclRhsSelection || isExactExpressionSelection) {
        return true;
    }
    if (ResolveInitializerExpressionContainingSelection(context, trimmedSpan) != nullptr) {
        return false;
    }
    if (auto *coverExpr = ResolveExpressionCoveringRange(context, trimmedSpan);
        coverExpr != nullptr && coverExpr->IsExpression() && coverExpr->Start().index <= trimmedSpan.pos &&
        coverExpr->End().index >= trimmedSpan.end) {
        const bool isExactCover =
            coverExpr->Start().index == trimmedSpan.pos && coverExpr->End().index == trimmedSpan.end;
        if (!isExactCover && coverExpr->IsBinaryExpression()) {
            auto op = coverExpr->AsBinaryExpression()->OperatorType();
            const bool isArithmeticBinary =
                op == lexer::TokenType::PUNCTUATOR_PLUS || op == lexer::TokenType::PUNCTUATOR_MINUS ||
                op == lexer::TokenType::PUNCTUATOR_MULTIPLY || op == lexer::TokenType::PUNCTUATOR_DIVIDE ||
                op == lexer::TokenType::PUNCTUATOR_MOD || op == lexer::TokenType::PUNCTUATOR_EXPONENTIATION;
            if (isArithmeticBinary) {
                return false;
            }
        }
        return true;
    }
    return false;
}

static ir::AstNode *ResolveValueExtractionDeclarationNode(const RefactorContext &context, ir::AstNode *extractedText,
                                                          const std::string &actionName)
{
    if (IsConstantExtractionAction(actionName)) {
        const TextRange trimmedSpan = GetTrimmedSelectionSpan(context);
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
    state.insertPos = GetVarAndFunctionPosToWriteNode(context, actionName).pos;
    state.insertPos = ResolveInsertionPosForVariableExtraction(context, ctx, actionName, state.insertPos);
    if (IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
        state.insertPos = ResolveGlobalConstantInsertionPosFromSource(ctx->sourceFile->source, context.span.pos,
                                                                      state.insertPos, DetermineGlobalInsertPos(ctx));
    }
    auto inlineInsertionResult = TryBuildInlineInsertion({context, ctx, declarationNode, actionName, uniqueVarName});
    state.inlineInsertion = ApplyInlineInsertionResult(state.insertPos, state.generatedText, inlineInsertionResult);
    const bool multiDeclInsertionText = IsMultiDeclaratorInsertionText(state.generatedText, uniqueVarName);
    if (!state.inlineInsertion) {
        if (multiDeclInsertionText && IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
            state.insertPos =
                ResolveMultiDeclaratorFallbackInsertPos(ctx->sourceFile->source, context.span, state.insertPos);
        }
        if (!multiDeclInsertionText) {
            AdjustGeneratedTextForInsert(context, ctx, state.insertPos, uniqueVarName, state.generatedText);
            AppendTrailingNewLineForGlobalVariableInsert(context, actionName, state.insertPos, state.generatedText);
            AppendTrailingNewLineForConstantEncloseInsert(context, actionName, state.insertPos, ctx->sourceFile->source,
                                                          state.generatedText);
            MaybePrependNamespaceNewlinesForValueExtraction(context, state.insertPos, state.generatedText,
                                                            {&actionName, declarationNode});
        }
    }
    return state;
}

static ir::AstNode *ResolveExtractedNodeForValueAction(const RefactorContext &context, ir::AstNode *extractedText,
                                                       const std::string &actionName)
{
    const TextRange trimmed = GetTrimmedSelectionSpan(context);
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
