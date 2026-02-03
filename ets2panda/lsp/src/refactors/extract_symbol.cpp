/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

/**
 * @file extract_symbol.h/cpp
 * @brief Extract symbol refactor implementation for the ES2Panda LSP.
 *
 * @details
 * This file implements the ExtractSymbolRefactor used by the language server
 * to offer extraction refactorings (extract constant, extract variable,
 * extract function) from a selected source range. It provides:
 *  - ExtractSymbolRefactor class which registers available extract actions
 *    and creates the edits for the selected action.
 *  - Helper routines to locate and validate the AST nodes to extract,
 *    compute insertion positions, build extracted function text, and
 *    generate the corresponding text edits via ChangeTracker.
 *
 * Main components:
 *  - class ExtractSymbolRefactor
 *      Registers extract refactor kinds and exposes:
 *        - GetAvailableActions(const RefactorContext&)
 *        - GetEditsForAction(const RefactorContext&, const std::string&)
 *
 *  - Utility functions:
 *      - std::vector<ir::AstNode *> IsRightNodeInRange(ir::AstNode*, TextRange)
 *          Collects right-side binary-expression nodes that lie fully inside
 *          the provided span (used when combining binary-expr parts).
 *
 *      - bool IsNodeInScope(ir::AstNode*)
 *          Predicate to determine whether a node is unsuitable for extraction
 *          (e.g. literals, member/call expressions, variable declarations).
 *
 *      - TextRange GetCallPositionOfExtraction(const RefactorContext &)
 *          Compute the textual call-site range that should be replaced with
 *          an invocation to the newly-extracted symbol/function.
 *
 *      - TextRange GetVarAndFunctionPosToWriteNode(const RefactorContext &)
 *          Determine a suitable insertion position for generated variable or
 *          function declarations (walks parent nodes to find the block scope).
 *
 *      - ir::AstNode *FindExtractedVals(const RefactorContext &)
 *        ir::AstNode *FindExtractedFunction(const RefactorContext &)
 *          Find the expression/statement/function node corresponding to the
 *          user-selected span to be extracted.
 *
 *      - std::vector<FunctionExtraction> GetPossibleFunctionExtractions(const RefactorContext &)
 *          Enumerate candidate statements/blocks that can be turned into a function.
 *
 *      - void CollectFunctionParameters(FunctionExtraction &)
 *          Collect function parameters from function nodes to build call-site arguments.
 *
 *      - std::string BuildFunctionText(
 *          const FunctionExtraction &, const RefactorContext &, const std::string &, const FunctionIOInfo *)
 *          Build the source text for the extracted function (name, params, body).
 *
 *      - std::string ReplaceWithFunctionCall(const FunctionExtraction &, const std::string &)
 *          Build the call-site text that replaces the original code after extraction.
 *
 *      - std::string GenerateInlineEdits(const RefactorContext &, ir::AstNode *)
 *          Generate the declaration (const/let) text for extracted values.
 *
 *      - RefactorEditInfo GetRefactorEditsToExtractVals(const RefactorContext &, ir::AstNode *)
 *        RefactorEditInfo GetRefactorEditsToExtractFunction(const RefactorContext &, ir::AstNode *)
 *          Create and return actual FileTextChanges via ChangeTracker for each refactor.
 *
 * Usage / Notes:
 *  - This module relies on the ES2Panda public context (public_lib::Context),
 *    the parser AST, and the ChangeTracker service to build edits.
 *  - The refactor logic assumes a valid RefactorContext describing the
 *    selection span and editor context; callers should validate that the
 *    context and parser AST are available before calling the entry points.
 *
 *
 * @see refactors/extract_symbol.h
 * @see refactors/refactor_types.h
 * @see services/text_change/change_tracker.h
 */

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "checker/ETSchecker.h"
#include "refactors/extract_symbol.h"
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

constexpr size_t HELPER_RESERVE_PADDING = 64;
constexpr std::string_view EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX = "extract_function_scope_ns_";
constexpr std::string_view EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX = "extract_constant_scope_ns_";

ExtractSymbolRefactor::ExtractSymbolRefactor()
{
    AddKind(std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.kind));
    AddKind(std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.kind));
    AddKind(std::string(EXTRACT_VARIABLE_ACTION_GLOBAL.kind));

    AddKind(std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.kind));
    AddKind(std::string(EXTRACT_VARIABLE_ACTION_ENCLOSE.kind));
    AddKind(std::string(EXTRACT_FUNCTION_ACTION_CLASS.kind));
}

std::vector<ir::AstNode *> IsRightNodeInRange(ir::AstNode *node, TextRange span)
{
    std::vector<ir::AstNode *> nodeList;
    if (!node->IsBinaryExpression()) {
        return nodeList;
    }
    ir::AstNode *right = node;
    while (right->AsBinaryExpression()->Right() != nullptr && right->Start().index > span.pos &&
           right->End().index < span.end) {
        nodeList.push_back(right);
        right = right->AsBinaryExpression()->Right();
    }
    return nodeList;
}

bool IsNodeInScope(ir::AstNode *node)
{
    return (node != nullptr && !node->IsVariableDeclaration() && !node->IsCallExpression() &&
            !node->IsMemberExpression() && !node->IsExpressionStatement() && !node->IsStringLiteral() &&
            !node->IsNumberLiteral() && !node->IsBooleanLiteral() && !node->IsNullLiteral() && !node->IsCharLiteral() &&
            !node->IsBinaryExpression() && !node->IsObjectExpression());
}

static TextRange GetTrimmedSelectionSpan(const RefactorContext &context)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return context.span;
    }

    const auto &source = ctx->sourceFile->source;
    size_t start = std::min(context.span.pos, source.size());
    size_t end = std::min(context.span.end, source.size());
    while (start < end && std::isspace(static_cast<unsigned char>(source[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(source[end - 1]))) {
        --end;
    }
    if (start == end) {
        return {std::min(context.span.pos, source.size()), std::min(context.span.end, source.size())};
    }
    return {start, end};
}

TextRange GetCallPositionOfExtraction(const RefactorContext &context)
{
    const auto normalizedSpan = GetTrimmedSelectionSpan(context);
    auto start = normalizedSpan.pos;
    auto end = normalizedSpan.end;
    const auto startedNode = GetTouchingToken(context.context, start, false);
    if (startedNode->Start().index < start) {
        start = startedNode->Start().index;
    }
    const auto endedNode = GetTouchingToken(context.context, end - 1, false);
    if (endedNode->End().index > end) {
        end = endedNode->End().index;
    }

    return {start, end};
}

static size_t LineToPos(public_lib::Context *context, const size_t line)
{
    auto index = ark::es2panda::lexer::LineIndex(context->parserProgram->SourceCode());
    return index.GetOffsetOfLine(line);
}

namespace {

struct VariableBindingInfo {
    ir::VariableDeclaration *declaration {nullptr};
    ir::VariableDeclarator *declarator {nullptr};
    ir::Identifier *identifier {nullptr};
    ir::Expression *initializer {nullptr};
};

struct HelperPieces {
    bool insertHelper {false};
    size_t insertPos {0};
    std::string helperText;
    TextRange replaceRange {};
    std::string replacementText;
};

struct FunctionIOInfo {
    std::vector<std::string> paramDecls;
    std::vector<std::string> callArgs;
    std::optional<std::string> returnVar;
    bool hasReturnStatement {false};
};

struct FunctionBodyOptions {
    std::string newLine;
    std::string indent;
    bool addLeadingNewLine {false};
    bool returnEachLine {false};
    std::optional<std::string> returnVar;
    size_t trimIndent {0};
    size_t indentSize {FormatCodeSettings().GetIndentSize()};
};

struct ScopeContext {
    bool hasEncloseScope {false};
    bool hasClassScope {false};
    std::string classScopeName;
    std::vector<std::string> namespaceScopeNames;
};

constexpr char LINE_FEED = '\n';
constexpr char CARRIAGE_RETURN = '\r';
constexpr char SPACE_CHAR = ' ';
constexpr char TAB_CHAR = '\t';
constexpr size_t CRLF_LENGTH = 2;
#ifdef _WIN32
constexpr std::string_view WINDOWS_LINE_BREAK = "\r\n";
#endif

inline bool IsLineBreakChar(char ch)
{
    return ch == LINE_FEED || ch == CARRIAGE_RETURN;
}

inline bool IsIndentChar(char ch)
{
    return ch == SPACE_CHAR || ch == TAB_CHAR;
}

bool ResolveVariableBinding(ir::AstNode *node, VariableBindingInfo &out)
{
    for (ir::AstNode *current = node; current != nullptr; current = current->Parent()) {
        if (!current->IsVariableDeclaration()) {
            continue;
        }
        auto *decl = current->AsVariableDeclaration();
        auto &items = decl->Declarators();
        if (items.size() != 1) {
            return false;
        }
        auto *item = items.front();
        if (item == nullptr || item->Id() == nullptr || !item->Id()->IsIdentifier() || item->Init() == nullptr ||
            !item->Init()->IsExpression()) {
            return false;
        }
        out.declaration = decl;
        out.declarator = item;
        out.identifier = item->Id()->AsIdentifier();
        out.initializer = item->Init()->AsExpression();
        return true;
    }
    return false;
}

std::pair<size_t, size_t> ComputeLineIndent(util::StringView source, size_t pos)
{
    auto sv = source.Utf8();
    if (sv.empty()) {
        return {0, 0};
    }
    size_t cursor = std::min(pos, sv.size());
    while (cursor > 0) {
        char ch = sv[cursor - 1];
        if (IsLineBreakChar(ch)) {
            break;
        }
        --cursor;
    }
    size_t indentEnd = cursor;
    while (indentEnd < sv.size()) {
        char ch = sv[indentEnd];
        if (IsIndentChar(ch)) {
            ++indentEnd;
            continue;
        }
        break;
    }
    return {cursor, indentEnd};
}

ir::ScriptFunction *FindScriptFunction(ir::AstNode *node)
{
    for (ir::AstNode *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsFunctionDeclaration()) {
            return current->AsFunctionDeclaration()->Function();
        }
        if (current->IsFunctionExpression()) {
            return current->AsFunctionExpression()->Function();
        }
        if (current->IsArrowFunctionExpression()) {
            return current->AsArrowFunctionExpression()->Function();
        }
    }
    return nullptr;
}

ir::ClassDefinition *FindEnclosingClassDefinition(ir::AstNode *node)
{
    for (ir::AstNode *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsClassDeclaration()) {
            return current->AsClassDeclaration()->Definition();
        }
    }
    return nullptr;
}

bool IsNamespaceScope(const ir::ClassDefinition *classDef)
{
    return classDef != nullptr && classDef->IsNamespaceTransformed();
}

bool IsNamespaceContext(ir::AstNode *node)
{
    return IsNamespaceScope(FindEnclosingClassDefinition(node));
}

std::vector<ir::ClassDefinition *> CollectEnclosingNamespaceScopes(ir::AstNode *node)
{
    std::vector<ir::ClassDefinition *> scopes;
    for (ir::AstNode *current = node; current != nullptr; current = current->Parent()) {
        ir::ClassDefinition *classDef = nullptr;
        if (current->IsClassDefinition()) {
            classDef = current->AsClassDefinition();
        } else if (current->IsClassDeclaration()) {
            classDef = current->AsClassDeclaration()->Definition();
        }
        if (!IsNamespaceScope(classDef) || classDef->IsGlobal()) {
            continue;
        }
        if (scopes.empty() || scopes.back() != classDef) {
            scopes.push_back(classDef);
        }
    }
    return scopes;
}

static std::string IdentifierNameMutf8(const ir::Identifier *ident)
{
    return ident == nullptr ? "" : ident->Name().Mutf8();
}

template <typename ExistsPredicate>
std::string GenerateUniqueName(std::string_view baseName, ExistsPredicate exists)
{
    std::string name(baseName);
    int counter = 0;
    while (exists(name)) {
        ++counter;
        name = std::string(baseName) + "_" + std::to_string(counter);
    }
    return name;
}

ir::ClassDefinition *FindNamespaceScopeByDepth(ir::AstNode *node, size_t namespaceDepth)
{
    auto scopes = CollectEnclosingNamespaceScopes(node);
    if (namespaceDepth >= scopes.size()) {
        return nullptr;
    }
    return scopes[namespaceDepth];
}

std::string GetNodeText(public_lib::Context *ctx, const ir::AstNode *node)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || node == nullptr) {
        return "";
    }
    return GetSourceTextOfNodeFromSourceFile(ctx->sourceFile->source, const_cast<ir::AstNode *>(node));
}

std::unordered_map<std::string, std::string> CollectParameterText(public_lib::Context *ctx, ir::ScriptFunction *func)
{
    std::unordered_map<std::string, std::string> result;
    if (ctx == nullptr || func == nullptr) {
        return result;
    }
    for (auto *paramExpr : func->Params()) {
        if (paramExpr == nullptr || !paramExpr->IsETSParameterExpression()) {
            continue;
        }
        auto *param = paramExpr->AsETSParameterExpression();
        auto *ident = param->Ident();
        if (ident == nullptr) {
            continue;
        }
        std::string name = IdentifierNameMutf8(ident);
        std::string text = GetNodeText(ctx, param);
        if (text.empty()) {
            text = name;
        }
        result.emplace(std::move(name), std::move(text));
    }
    return result;
}

std::string GetKeyword(ir::VariableDeclaration::VariableDeclarationKind kind)
{
    if (kind == ir::VariableDeclaration::VariableDeclarationKind::CONST) {
        return "const";
    }
    return "let";
}

std::string GetDeclaratorIdText(public_lib::Context *ctx, const VariableBindingInfo &binding)
{
    std::string text = GetNodeText(ctx, binding.declarator->Id());
    if (text.empty()) {
        return IdentifierNameMutf8(binding.identifier);
    }
    while (!text.empty() && IsLineBreakChar(text.back())) {
        text.pop_back();
    }
    return text;
}

std::vector<std::string> CollectIdentifierNames(ir::Expression *expr, const std::string &skip)
{
    std::vector<std::string> names;
    if (expr == nullptr) {
        return names;
    }
    std::unordered_set<std::string> seen;
    expr->FindChild([&](ir::AstNode *node) {
        if (!node->IsIdentifier()) {
            return false;
        }
        auto *ident = node->AsIdentifier();
        auto *parent = ident->Parent();
        if (parent != nullptr && parent->IsMemberExpression()) {
            auto *member = parent->AsMemberExpression();
            if (!member->IsComputed() && member->Property() == ident) {
                return false;
            }
        }
        std::string name = IdentifierNameMutf8(ident);
        if (name == skip) {
            return false;
        }
        if (seen.insert(name).second) {
            names.emplace_back(std::move(name));
        }
        return false;
    });
    return names;
}

bool HasNewlineInRange(std::string_view source, TextRange range)
{
    size_t upper = std::min(range.end, source.size());
    for (size_t i = range.pos; i < upper; ++i) {
        char ch = source[i];
        if (ch == LINE_FEED || ch == CARRIAGE_RETURN) {
            return true;
        }
    }
    return false;
}

bool IsMemberPropertyIdentifier(const ir::Identifier *ident)
{
    if (ident == nullptr) {
        return false;
    }
    auto *parent = ident->Parent();
    if (parent == nullptr || !parent->IsMemberExpression()) {
        return false;
    }
    auto *member = parent->AsMemberExpression();
    return !member->IsComputed() && member->Property() == ident;
}

bool IsDeclarationIdentifier(const ir::Identifier *ident)
{
    if (ident == nullptr) {
        return false;
    }
    auto *parent = ident->Parent();
    if (parent == nullptr) {
        return false;
    }
    if (parent->IsVariableDeclarator()) {
        return parent->AsVariableDeclarator()->Id() == ident;
    }
    if (parent->IsFunctionDeclaration()) {
        auto *func = parent->AsFunctionDeclaration()->Function();
        return func != nullptr && func->Id() == ident;
    }
    return false;
}

bool IsLocalToEnclosingFunction(const ir::ScriptFunction *enclosingFunc, varbinder::Variable *variable)
{
    if (enclosingFunc == nullptr || variable == nullptr) {
        return false;
    }
    auto *decl = variable->Declaration();
    if (decl == nullptr) {
        return false;
    }
    auto *node = decl->Node();
    if (node == nullptr) {
        return false;
    }
    return node->Start().index >= enclosingFunc->Start().index && node->End().index <= enclosingFunc->End().index;
}

bool IsDeclaredInGlobalScope(const varbinder::Decl *decl)
{
    if (decl == nullptr) {
        return true;
    }
    auto *node = decl->Node();
    if (node == nullptr) {
        return true;
    }
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsClassDefinition()) {
            auto *classDef = current->AsClassDefinition();
            if (classDef != nullptr && !classDef->IsGlobal()) {
                return false;
            }
        }
        if (current->IsFunctionDeclaration() || current->IsFunctionExpression() ||
            current->IsArrowFunctionExpression() || current->IsScriptFunction()) {
            return false;
        }
    }
    return true;
}

static bool IsContainedInRange(const ir::AstNode *node, TextRange span)
{
    return node != nullptr && node->Start().index >= span.pos && node->End().index <= span.end;
}

static void RecordDeclaredIdentifier(const ir::AstNode *node, std::unordered_set<std::string> &declaredInside)
{
    if (node == nullptr || !node->IsVariableDeclarator()) {
        return;
    }
    auto *decl = node->AsVariableDeclarator();
    if (decl->Id() != nullptr && decl->Id()->IsIdentifier()) {
        declaredInside.insert(IdentifierNameMutf8(decl->Id()->AsIdentifier()));
    }
}

static void RecordAssignedIdentifier(const ir::AstNode *node, ir::ScriptFunction *enclosing,
                                     std::unordered_set<std::string> &assignedInside)
{
    const ir::Expression *target = nullptr;
    if (node != nullptr && node->IsAssignmentExpression()) {
        target = node->AsAssignmentExpression()->Left();
    } else if (node != nullptr && node->IsUpdateExpression()) {
        target = node->AsUpdateExpression()->Argument();
    }
    if (target == nullptr || !target->IsIdentifier()) {
        return;
    }
    auto *ident = target->AsIdentifier();
    if (IsMemberPropertyIdentifier(ident) || IsDeclarationIdentifier(ident)) {
        return;
    }
    auto *variable = ResolveIdentifier(ident);
    if (IsLocalToEnclosingFunction(enclosing, variable)) {
        assignedInside.insert(IdentifierNameMutf8(ident));
    }
}

static std::optional<std::pair<std::string, ir::Identifier *>> ResolveUsedIdentifier(
    ir::AstNode *node, bool includeNonGlobal, ir::ScriptFunction *enclosing,
    const std::unordered_set<std::string> &declaredInside)
{
    if (node == nullptr || !node->IsIdentifier()) {
        return std::nullopt;
    }
    auto *ident = node->AsIdentifier();
    if (IsMemberPropertyIdentifier(ident) || IsDeclarationIdentifier(ident)) {
        return std::nullopt;
    }
    auto *variable = ResolveIdentifier(ident);
    if (variable == nullptr) {
        return std::nullopt;
    }
    const bool isLocal = IsLocalToEnclosingFunction(enclosing, variable);
    const bool isNonGlobal = includeNonGlobal && !IsDeclaredInGlobalScope(variable->Declaration());
    if (!isLocal && !isNonGlobal) {
        return std::nullopt;
    }
    std::string name = IdentifierNameMutf8(ident);
    if (declaredInside.find(name) != declaredInside.end()) {
        return std::nullopt;
    }
    return std::make_pair(std::move(name), ident);
}

template <class Handler>
static void CollectFunctionIOUsage(ir::AstNode *ast, TextRange range, Handler &&handler)
{
    if (ast == nullptr) {
        return;
    }
    ast->FindChild([range, &handler](ir::AstNode *node) {
        if (!IsContainedInRange(node, range)) {
            return false;
        }
        handler(node);
        return false;
    });
}

static void FinalizeFunctionIO(FunctionIOInfo &info, const std::unordered_set<std::string> &declaredInside,
                               const std::unordered_set<std::string> &assignedInside,
                               const std::vector<std::string> &usedOrder)
{
    std::vector<std::string> assignedOutside;
    assignedOutside.reserve(assignedInside.size());
    for (const auto &name : assignedInside) {
        if (declaredInside.find(name) == declaredInside.end()) {
            assignedOutside.push_back(name);
        }
    }
    if (!info.hasReturnStatement && assignedOutside.size() == 1U) {
        info.returnVar = assignedOutside.front();
    }
    for (const auto &name : usedOrder) {
        if (declaredInside.find(name) != declaredInside.end()) {
            continue;
        }
        info.callArgs.push_back(name);
    }
    if (info.returnVar.has_value() &&
        std::find(info.callArgs.begin(), info.callArgs.end(), info.returnVar.value()) == info.callArgs.end()) {
        info.callArgs.push_back(info.returnVar.value());
    }
}

static void BuildParamDecls(FunctionIOInfo &info, const std::unordered_map<std::string, ir::Identifier *> &firstUse,
                            checker::ETSChecker *checker)
{
    auto normalizeTypeForExtractedParam = [](const std::string &typeText) -> std::string {
        if (typeText == "Number") {
            return "number";
        }
        if (typeText == "Boolean") {
            return "boolean";
        }
        if (typeText == "String") {
            return "string";
        }
        return typeText;
    };

    for (const auto &name : info.callArgs) {
        auto it = firstUse.find(name);
        if (checker == nullptr || it == firstUse.end()) {
            info.paramDecls.push_back(name);
            continue;
        }
        auto type = GetTypeOfSymbolAtLocation(checker, it->second);
        if (type == nullptr) {
            info.paramDecls.push_back(name);
            continue;
        }
        std::string typeText = type->ToString();
        typeText = normalizeTypeForExtractedParam(typeText);
        if (typeText.empty()) {
            info.paramDecls.push_back(name);
            continue;
        }
        info.paramDecls.push_back(name + ": " + typeText);
    }
}

FunctionIOInfo AnalyzeFunctionIO(const RefactorContext &context, TextRange range, bool includeNonGlobal)
{
    FunctionIOInfo info;
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return info;
    }
    auto *checker = ctx->GetChecker() == nullptr ? nullptr : ctx->GetChecker()->AsETSChecker();

    auto *touchNode = GetTouchingToken(context.context, range.pos, false);
    auto *enclosingFunc = FindScriptFunction(touchNode);

    std::unordered_set<std::string> declaredInside;
    std::unordered_set<std::string> assignedInside;
    std::unordered_set<std::string> usedSet;
    std::vector<std::string> usedOrder;
    std::unordered_map<std::string, ir::Identifier *> firstUse;
    auto onNode = [&info, &declaredInside, &assignedInside, includeNonGlobal, enclosingFunc, &usedSet, &usedOrder,
                   &firstUse](ir::AstNode *node) {
        if (node->IsReturnStatement()) {
            info.hasReturnStatement = true;
        }
        RecordDeclaredIdentifier(node, declaredInside);
        RecordAssignedIdentifier(node, enclosingFunc, assignedInside);
        auto used = ResolveUsedIdentifier(node, includeNonGlobal, enclosingFunc, declaredInside);
        if (used.has_value() && usedSet.insert(used->first).second) {
            usedOrder.push_back(used->first);
            firstUse.emplace(used->first, used->second);
        }
    };
    CollectFunctionIOUsage(ctx->parserProgram->Ast(), range, onNode);
    FinalizeFunctionIO(info, declaredInside, assignedInside, usedOrder);
    BuildParamDecls(info, firstUse, checker);
    return info;
}

std::string JoinWithComma(const std::vector<std::string> &items)
{
    if (items.empty()) {
        return "";
    }
    std::string joined;
    size_t total = (items.size() - 1) * 2;  // comma + space
    for (const auto &item : items) {
        total += item.size();
    }
    joined.reserve(total);
    for (size_t i = 0; i < items.size(); ++i) {
        if (i != 0) {
            joined.append(", ");
        }
        joined.append(items[i]);
    }
    return joined;
}

bool SourceContainsFunctionDefinition(public_lib::Context *ctx, const std::string &name)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return false;
    }

    const auto &src = ctx->sourceFile->source;
    if (src.empty()) {
        return false;
    }

    const std::string needle = "function " + name;
    size_t pos = src.find(needle);
    while (pos != std::string::npos) {
        size_t next = pos + needle.size();
        while (next < src.size() && (std::isspace(static_cast<unsigned char>(src[next])) != 0)) {
            ++next;
        }
        if (next < src.size() && (src[next] == '(' || src[next] == '<')) {
            return true;
        }
        pos = src.find(needle, pos + 1);
    }

    return false;
}

bool IsActionNameOrKind(std::string_view actionName, const RefactorActionView &action)
{
    return actionName == action.name || actionName == action.kind;
}

bool IsVariableExtractionAction(const std::string &actionName)
{
    return IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_ENCLOSE) ||
           IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_GLOBAL);
}

bool ParseUnsignedIndex(std::string_view text, size_t &value)
{
    if (text.empty()) {
        return false;
    }
    size_t parsed = 0;
    constexpr size_t decBase = 10;
    constexpr size_t maxValue = std::numeric_limits<size_t>::max();
    for (char ch : text) {
        if (ch < '0' || ch > '9') {
            return false;
        }
        size_t digit = static_cast<size_t>(ch - '0');
        if (parsed > (maxValue - digit) / decBase) {
            return false;
        }
        parsed = (parsed * decBase) + digit;
    }
    value = parsed;
    return true;
}

std::optional<size_t> ParseNamespaceActionDepth(std::string_view actionName, std::string_view prefix)
{
    if (actionName.size() <= prefix.size() || actionName.rfind(prefix, 0) != 0) {
        return std::nullopt;
    }
    size_t parsed = 0;
    if (!ParseUnsignedIndex(actionName.substr(prefix.size()), parsed)) {
        return std::nullopt;
    }
    return parsed;
}

std::optional<size_t> GetNamespaceActionDepth(std::string_view actionName, std::string_view encloseName,
                                              std::string_view prefix)
{
    if (actionName == encloseName) {
        return 0;
    }
    return ParseNamespaceActionDepth(actionName, prefix);
}

bool IsNamespaceAction(std::string_view actionName, std::string_view encloseName, std::string_view prefix)
{
    return GetNamespaceActionDepth(actionName, encloseName, prefix).has_value();
}

bool IsConstantExtractionInClassAction(const std::string &actionName)
{
    return IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_CLASS);
}

bool IsConstantExtractionAction(const std::string &actionName)
{
    return IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL) ||
           IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE) ||
           IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_CLASS) ||
           IsNamespaceAction(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE.name,
                             EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX);
}

std::string GetIndentAtPosition(public_lib::Context *ctx, size_t pos)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return "";
    }
    const auto &source = ctx->sourceFile->source;
    auto [lineStart, indentEnd] = ComputeLineIndent(util::StringView(source), pos);
    if (indentEnd <= lineStart) {
        return "";
    }
    return std::string(source.substr(lineStart, indentEnd - lineStart));
}

static bool IsLineBreak(char ch)
{
    return ch == '\n' || ch == '\r';
}

static std::string GetInsertionIndent(public_lib::Context *ctx, size_t insertPos);

static size_t NormalizeInsertPos(std::string_view source, size_t pos)
{
    size_t adjusted = std::min(pos, source.size());
    while (adjusted < source.size() && IsLineBreak(source[adjusted])) {
        ++adjusted;
    }
    return adjusted;
}

static void GetLineBounds(std::string_view source, size_t pos, size_t &lineStart, size_t &lineEnd)
{
    if (source.empty()) {
        lineStart = 0;
        lineEnd = 0;
        return;
    }

    size_t safePos = std::min(pos, source.size() - 1);
    lineStart = safePos;
    while (lineStart > 0 && !IsLineBreak(source[lineStart - 1])) {
        --lineStart;
    }

    lineEnd = safePos;
    while (lineEnd < source.size() && !IsLineBreak(source[lineEnd])) {
        ++lineEnd;
    }
    if (lineEnd >= source.size()) {
        lineEnd = source.size() - 1;
    }
}

static bool IsBlankLine(std::string_view source, size_t lineStart, size_t lineEnd)
{
    if (source.empty() || lineStart >= source.size()) {
        return true;
    }
    size_t end = std::min(lineEnd, source.size() - 1);
    for (size_t i = lineStart; i <= end; ++i) {
        char ch = source[i];
        if (IsLineBreak(ch)) {
            continue;
        }
        if (ch != ' ' && ch != '\t') {
            return false;
        }
    }
    return true;
}

static std::string FormatDeclarationForInsert(public_lib::Context *ctx, size_t insertPos, std::string declaration)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || declaration.empty()) {
        return declaration;
    }
    std::string_view source = ctx->sourceFile->source;
    if (source.empty() || insertPos >= source.size()) {
        return declaration;
    }

    auto [indentStart, indentEnd] = ComputeLineIndent(util::StringView(source), insertPos);
    if (insertPos == indentStart && indentEnd > indentStart) {
        declaration.insert(0, source.substr(indentStart, indentEnd - indentStart));
    }
    return declaration;
}

static bool IsLineStartAtPosition(std::string_view source, size_t pos)
{
    if (pos == 0) {
        return true;
    }
    char prev = source[pos - 1];
    return prev == '\n' || prev == '\r';
}

static ir::AstNode *ScanTouchingTokenForward(const RefactorContext &context, std::string_view source)
{
    size_t upper = std::min(context.span.end, source.size());
    for (size_t i = context.span.pos; i < upper; ++i) {
        if (std::isspace(static_cast<unsigned char>(source[i]))) {
            continue;
        }
        if (auto *node = GetTouchingToken(context.context, i, false); node != nullptr) {
            return node;
        }
    }
    return nullptr;
}

static ir::AstNode *ScanTouchingTokenBackward(const RefactorContext &context, std::string_view source)
{
    size_t i = std::min(context.span.pos, source.size());
    while (i > 0) {
        --i;
        if (std::isspace(static_cast<unsigned char>(source[i]))) {
            continue;
        }
        if (auto *node = GetTouchingToken(context.context, i, false); node != nullptr) {
            return node;
        }
    }
    return nullptr;
}

static ir::AstNode *FindTouchingTokenByScan(const RefactorContext &context, public_lib::Context *ctx)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return nullptr;
    }
    std::string_view source = ctx->sourceFile->source;
    if (auto *node = ScanTouchingTokenForward(context, source); node != nullptr) {
        return node;
    }
    if (context.span.pos == 0) {
        return nullptr;
    }
    return ScanTouchingTokenBackward(context, source);
}

static bool IsNamespaceModule(const ir::AstNode *node)
{
    auto *module = node != nullptr ? node->AsETSModule() : nullptr;
    return module != nullptr && module->IsNamespace();
}

static bool HasNamespaceModuleAncestor(const ir::AstNode *current)
{
    for (auto *parent = current != nullptr ? current->Parent() : nullptr; parent != nullptr;
         parent = parent->Parent()) {
        if (parent->IsETSModule() && IsNamespaceModule(parent)) {
            return true;
        }
    }
    return false;
}

static bool IsInGlobalClassStaticBlock(const ir::AstNode *current)
{
    for (auto *cursor = current; cursor != nullptr; cursor = cursor->Parent()) {
        if (!cursor->IsClassStaticBlock()) {
            continue;
        }
        for (auto *parent = cursor->Parent(); parent != nullptr; parent = parent->Parent()) {
            if (!parent->IsClassDefinition()) {
                continue;
            }
            auto *classDef = parent->AsClassDefinition();
            return classDef != nullptr && (classDef->IsGlobal() || classDef->IsNamespaceTransformed());
        }
    }
    return false;
}

static bool IsSyntheticScriptFunctionUnderGlobalClass(const ir::AstNode *node)
{
    auto *parent = node != nullptr ? node->Parent() : nullptr;
    if (parent == nullptr || !parent->IsScriptFunction() || !compiler::HasGlobalClassParent(parent)) {
        return false;
    }
    auto *script = parent->AsScriptFunction();
    return script != nullptr && script->IsSynthetic();
}

static bool IsNamespaceModuleParent(const ir::AstNode *node)
{
    auto *parent = node != nullptr ? node->Parent() : nullptr;
    return parent != nullptr && parent->IsETSModule() && IsNamespaceModule(parent);
}

static bool IsProgramParent(const ir::AstNode *node)
{
    auto *parent = node != nullptr ? node->Parent() : nullptr;
    return parent != nullptr && parent->IsProgram();
}

static bool ShouldIndentBlockStatement(const ir::AstNode *node)
{
    if (node == nullptr) {
        return false;
    }
    if (IsInGlobalClassStaticBlock(node)) {
        return false;
    }
    if (IsSyntheticScriptFunctionUnderGlobalClass(node)) {
        return false;
    }
    if (IsNamespaceModuleParent(node)) {
        return false;
    }
    if (IsProgramParent(node)) {
        return false;
    }
    return true;
}

static bool ShouldIndentClassDefinition(const ir::AstNode *node)
{
    auto *classDef = node != nullptr ? node->AsClassDefinition() : nullptr;
    if (classDef == nullptr) {
        return false;
    }
    if (classDef->IsGlobal()) {
        return false;
    }
    if (classDef->IsNamespaceTransformed()) {
        return !HasNamespaceModuleAncestor(node);
    }
    return true;
}

static bool IsIndentScopeNode(const ir::AstNode *node)
{
    if (node == nullptr) {
        return false;
    }
    if (node->IsETSModule()) {
        return IsNamespaceModule(node);
    }
    if (node->IsBlockStatement()) {
        return ShouldIndentBlockStatement(node);
    }
    if (node->IsClassDefinition()) {
        return ShouldIndentClassDefinition(node);
    }
    return node->IsSwitchStatement();
}

static bool HasSelectionNewline(const RefactorContext &context, std::string_view source)
{
    size_t upper = std::min(context.span.end, source.size());
    for (size_t i = context.span.pos; i < upper; ++i) {
        char ch = source[i];
        if (ch == '\n' || ch == '\r') {
            return true;
        }
    }
    return false;
}

static TextRange TrimSpanWhitespace(TextRange span, std::string_view source)
{
    size_t trimStart = std::min(span.pos, source.size());
    size_t trimEnd = std::min(span.end, source.size());
    while (trimStart < trimEnd && std::isspace(static_cast<unsigned char>(source[trimStart]))) {
        ++trimStart;
    }
    while (trimEnd > trimStart && std::isspace(static_cast<unsigned char>(source[trimEnd - 1]))) {
        --trimEnd;
    }
    return {trimStart, trimEnd};
}

static ir::AstNode *GetNodeForSpan(const RefactorContext &context)
{
    const auto normalizedSpan = GetTrimmedSelectionSpan(context);
    if (normalizedSpan.pos == normalizedSpan.end) {
        return GetTouchingToken(context.context, normalizedSpan.pos, false);
    }
    auto *node = GetTouchingTokenByRange(context.context, normalizedSpan, false);
    if (node != nullptr) {
        if (auto *optimum = GetOptimumNodeByRange(node, normalizedSpan); optimum != nullptr) {
            node = optimum;
        }
    }
    return node;
}

static ir::AstNode *ResolveNodeForSelection(const RefactorContext &context, public_lib::Context *ctx,
                                            bool selectionHasNewline)
{
    auto *node = GetNodeForSpan(context);
    if (node != nullptr || !selectionHasNewline) {
        return node;
    }
    node = FindTouchingTokenByScan(context, ctx);
    if (node != nullptr) {
        if (auto *optimum = GetOptimumNodeByRange(node, context.span); optimum != nullptr) {
            node = optimum;
        }
    }
    return node;
}

static bool IsStatementSelectionCandidate(const ir::AstNode *node)
{
    return node != nullptr && (node->IsStatement() || node->IsExpressionStatement() || node->IsVariableDeclaration()) &&
           !node->IsBlockStatement();
}

static bool IsSelectionSuffixSkippable(std::string_view source, size_t start, size_t end)
{
    if (start > end || end > source.size()) {
        return false;
    }
    for (size_t i = start; i < end; ++i) {
        char ch = source[i];
        if (ch != ';' && !std::isspace(static_cast<unsigned char>(ch))) {
            return false;
        }
    }
    return true;
}

struct StatementSelectionScanResult {
    std::vector<ir::AstNode *> containedStatements;
    bool hasPartialOverlap {false};
};

static StatementSelectionScanResult ScanStatementSelectionCandidates(ir::AstNode *ast, TextRange span)
{
    StatementSelectionScanResult result;
    if (ast == nullptr) {
        return result;
    }
    ast->FindChild([&](ir::AstNode *child) {
        if (!IsStatementSelectionCandidate(child)) {
            return false;
        }
        const size_t start = child->Start().index;
        const size_t end = child->End().index;
        const bool overlapsSelection = start < span.end && end > span.pos;
        if (!overlapsSelection) {
            return false;
        }
        const bool isContainedBySelection = start >= span.pos && end <= span.end;
        if (isContainedBySelection) {
            result.containedStatements.push_back(child);
            return false;
        }
        const bool fullyContainsSelection = start <= span.pos && end >= span.end;
        if (!fullyContainsSelection) {
            result.hasPartialOverlap = true;
        }
        return false;
    });
    return result;
}

static bool HasContainedStatementAncestorInSpan(const ir::AstNode *statement, TextRange span)
{
    for (auto *ancestor = statement == nullptr ? nullptr : statement->Parent(); ancestor != nullptr;
         ancestor = ancestor->Parent()) {
        if (ancestor->Start().index < span.pos || ancestor->End().index > span.end) {
            continue;
        }
        if (IsStatementSelectionCandidate(ancestor)) {
            return true;
        }
    }
    return false;
}

static std::vector<ir::AstNode *> CollectTopLevelContainedStatements(
    const std::vector<ir::AstNode *> &containedStatements, TextRange span)
{
    std::vector<ir::AstNode *> topLevelStatements;
    topLevelStatements.reserve(containedStatements.size());
    for (auto *statement : containedStatements) {
        if (!HasContainedStatementAncestorInSpan(statement, span)) {
            topLevelStatements.push_back(statement);
        }
    }
    return topLevelStatements;
}

static void SortStatementsBySourceOrder(std::vector<ir::AstNode *> &statements)
{
    std::stable_sort(statements.begin(), statements.end(), [](const ir::AstNode *lhs, const ir::AstNode *rhs) {
        if (lhs->Start().index != rhs->Start().index) {
            return lhs->Start().index < rhs->Start().index;
        }
        return lhs->End().index < rhs->End().index;
    });
}

static bool ValidateStatementSelectionBoundaries(public_lib::Context *ctx, TextRange span, const ir::AstNode *first,
                                                 const ir::AstNode *last)
{
    if (first == nullptr || last == nullptr || first->Start().index != span.pos) {
        return false;
    }
    if (last->End().index > span.end) {
        return false;
    }
    if (last->End().index == span.end) {
        return true;
    }
    auto *sourceFile = ctx == nullptr ? nullptr : ctx->sourceFile;
    return sourceFile != nullptr && IsSelectionSuffixSkippable(sourceFile->source, last->End().index, span.end);
}

static bool AreTopLevelStatementsContinuousSiblings(const std::vector<ir::AstNode *> &topLevelStatements)
{
    if (topLevelStatements.empty()) {
        return false;
    }
    auto *first = topLevelStatements.front();
    auto *last = topLevelStatements.back();
    auto *parent = first == nullptr ? nullptr : first->Parent();
    if (parent == nullptr || last == nullptr) {
        return false;
    }

    std::unordered_set<const ir::AstNode *> containedSet;
    containedSet.reserve(topLevelStatements.size());
    for (auto *statement : topLevelStatements) {
        if (statement == nullptr || statement->Parent() != parent) {
            return false;
        }
        containedSet.insert(statement);
    }

    std::vector<ir::AstNode *> siblings;
    parent->Iterate([&siblings](ir::AstNode *child) {
        if (child != nullptr) {
            siblings.push_back(child);
        }
    });

    bool seenFirst = false;
    for (auto *sibling : siblings) {
        if (sibling == first) {
            seenFirst = true;
        }
        if (!seenFirst) {
            continue;
        }
        if (IsStatementSelectionCandidate(sibling) && containedSet.count(sibling) == 0) {
            return false;
        }
        if (sibling == last) {
            return true;
        }
    }
    return false;
}

static ir::AstNode *FindStatementOverlappingSelection(public_lib::Context *ctx, TextRange span)
{
    auto *ast = ctx == nullptr ? nullptr : ctx->parserProgram->Ast();
    if (ast == nullptr || span.pos >= span.end) {
        return nullptr;
    }
    const auto scanResult = ScanStatementSelectionCandidates(ast, span);
    if (scanResult.hasPartialOverlap || scanResult.containedStatements.empty()) {
        return nullptr;
    }

    auto topLevelStatements = CollectTopLevelContainedStatements(scanResult.containedStatements, span);
    if (topLevelStatements.empty()) {
        return nullptr;
    }

    SortStatementsBySourceOrder(topLevelStatements);

    auto *first = topLevelStatements.front();
    auto *last = topLevelStatements.back();
    if (!ValidateStatementSelectionBoundaries(ctx, span, first, last)) {
        return nullptr;
    }
    if (!AreTopLevelStatementsContinuousSiblings(topLevelStatements)) {
        return nullptr;
    }
    return first;
}

static ir::AstNode *FindBackwardNonWhitespaceToken(const RefactorContext &context, size_t pos)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return nullptr;
    }
    const auto &source = ctx->sourceFile->source;
    size_t probe = std::min(pos, source.size());
    while (probe > 0) {
        --probe;
        if (std::isspace(static_cast<unsigned char>(source[probe]))) {
            continue;
        }
        return GetTouchingToken(context.context, probe, false);
    }
    return nullptr;
}

static ir::AstNode *ResolveScopeDepthProbeNode(const RefactorContext &context, size_t pos)
{
    if (auto *node = GetTouchingToken(context.context, pos, false); node != nullptr) {
        return node;
    }
    if (pos > 0) {
        if (auto *node = GetTouchingToken(context.context, pos - 1, false); node != nullptr) {
            return node;
        }
    }
    return FindBackwardNonWhitespaceToken(context, pos);
}

static size_t CountIndentScopeDepth(const ir::AstNode *node)
{
    size_t depth = 0;
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (IsIndentScopeNode(current)) {
            ++depth;
        }
    }
    return depth;
}

static std::string GetInsertionIndent(public_lib::Context *ctx, size_t insertPos)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return "";
    }
    const auto &source = ctx->sourceFile->source;
    if (insertPos == 0 || source.empty()) {
        return "";
    }

    size_t anchor = insertPos;
    if (!IsLineStartAtPosition(source, insertPos)) {
        while (anchor < source.size() && source[anchor] != '\n' && source[anchor] != '\r') {
            ++anchor;
        }
        while (anchor < source.size() && (source[anchor] == '\n' || source[anchor] == '\r')) {
            ++anchor;
        }
    }
    return GetIndentAtPosition(ctx, anchor);
}

bool ProgramHasFunction(public_lib::Context *ctx, const std::string &name)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr) {
        return false;
    }
    bool found = false;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (found || node == nullptr || !node->IsFunctionDeclaration()) {
            return false;
        }
        auto *decl = node->AsFunctionDeclaration();
        auto *func = decl->Function();
        if (func != nullptr && func->Id() != nullptr && IdentifierNameMutf8(func->Id()) == name) {
            found = true;
            return true;
        }
        auto text = GetNodeText(ctx, decl);
        std::string prefix = "function " + name;
        auto it = std::search(text.begin(), text.end(), prefix.begin(), prefix.end());
        if (it != text.end()) {
            found = true;
            return true;
        }
        return false;
    });
    if (found) {
        return true;
    }

    return SourceContainsFunctionDefinition(ctx, name);
}

bool ClassHasProperty(ir::ClassDefinition *classDef, const std::string &name)
{
    if (classDef == nullptr) {
        return false;
    }
    for (auto *member : classDef->Body()) {
        if (!member->IsClassProperty()) {
            continue;
        }
        auto *prop = member->AsClassProperty();
        if (prop->Key()->IsIdentifier() && IdentifierNameMutf8(prop->Key()->AsIdentifier()) == name) {
            return true;
        }
    }
    return false;
}

bool ClassHasMethod(ir::ClassDefinition *classDef, const std::string &name)
{
    if (classDef == nullptr) {
        return false;
    }
    for (auto *method : classDef->Body()) {
        if (method->Type() != ir::AstNodeType::METHOD_DEFINITION) {
            continue;
        }
        auto *func = method->AsMethodDefinition()->Function();
        if (func != nullptr && (func->Id() != nullptr) && IdentifierNameMutf8(func->Id()) == name) {
            return true;
        }
    }
    return false;
}

bool ScopeHasVar(ir::AstNode *scopeNode, const std::string &name)
{
    if (scopeNode == nullptr) {
        return false;
    }
    bool found = false;
    scopeNode->Iterate([&](ir::AstNode *child) {
        if (!child->IsVariableDeclaration()) {
            return;
        }
        for (auto *decl : child->AsVariableDeclaration()->Declarators()) {
            if ((decl->Id() != nullptr) && decl->Id()->IsIdentifier() &&
                IdentifierNameMutf8(decl->Id()->AsIdentifier()) == name) {
                found = true;
            }
        }
    });
    return found;
}

static bool ScopeHasName(ir::AstNode *scopeNode, const std::string &name)
{
    if (scopeNode == nullptr) {
        return false;
    }
    if (scopeNode->IsClassDefinition() && scopeNode->AsClassDefinition()->IsNamespaceTransformed()) {
        auto *classDef = scopeNode->AsClassDefinition();
        return ClassHasProperty(classDef, name) || ClassHasMethod(classDef, name);
    }
    return ScopeHasVar(scopeNode, name);
}

bool ProgramHasGlobalVar(public_lib::Context *ctx, const std::string &name)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr) {
        return false;
    }
    auto *ast = ctx->parserProgram->Ast();
    for (auto *stmt : ast->Statements()) {
        if (!stmt->IsClassDeclaration()) {
            continue;
        }
        auto *clsDef = stmt->AsClassDeclaration()->Definition();
        if (clsDef->IsGlobal() && ClassHasProperty(clsDef, name)) {
            return true;
        }
    }
    return false;
}

std::string GenerateUniqueFuncName(const RefactorContext &context, const std::string &baseName,
                                   const std::string &actionName)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr) {
        return baseName;
    }

    if (actionName == EXTRACT_FUNCTION_ACTION_GLOBAL.name) {
        return GenerateUniqueName(baseName, [ctx](const std::string &name) { return ProgramHasFunction(ctx, name); });
    }

    if (actionName == EXTRACT_FUNCTION_ACTION_CLASS.name) {
        auto *node = GetTouchingToken(context.context, context.span.pos, false);
        auto *classDef = FindEnclosingClassDefinition(node);
        if (classDef == nullptr) {
            return baseName;
        }
        return GenerateUniqueName(baseName,
                                  [classDef](const std::string &name) { return ClassHasMethod(classDef, name); });
    }

    if (const auto namespaceDepth = GetNamespaceActionDepth(actionName, EXTRACT_FUNCTION_ACTION_ENCLOSE.name,
                                                            EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX);
        namespaceDepth.has_value()) {
        auto *node = GetTouchingToken(context.context, context.span.pos, false);
        auto *classDef = FindNamespaceScopeByDepth(node, namespaceDepth.value());
        if (classDef == nullptr) {
            return baseName;
        }
        return GenerateUniqueName(baseName,
                                  [classDef](const std::string &name) { return ClassHasMethod(classDef, name); });
    }

    return baseName;
}

std::string GenerateUniqueClassPropertyName(const RefactorContext &context)
{
    std::string baseName = "newProperty";
    auto *node = GetTouchingToken(context.context, context.span.pos, false);
    auto *classDef = FindEnclosingClassDefinition(node);
    if (classDef == nullptr) {
        return baseName;
    }
    auto name = GenerateUniqueName(baseName,
                                   [classDef](const std::string &value) { return ClassHasProperty(classDef, value); });
    return "this." + name;
}

std::string GenerateUniqueGlobalVarName(const RefactorContext &context)
{
    std::string baseName = "newLocal";
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    return GenerateUniqueName(baseName, [ctx](const std::string &name) { return ProgramHasGlobalVar(ctx, name); });
}

std::string GenerateUniqueEncloseVarName(const RefactorContext &context)
{
    std::string baseName = "newLocal";
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    auto *node = GetTouchingToken(context.context, context.span.pos, false);
    ir::AstNode *scopeNode = nullptr;
    ir::AstNode *namespaceScopeNode = nullptr;
    while (node != nullptr) {
        if (node->IsClassDefinition() && node->AsClassDefinition()->IsNamespaceTransformed()) {
            if (namespaceScopeNode == nullptr) {
                namespaceScopeNode = node;
            }
        }
        if (node->IsProgram()) {
            scopeNode = namespaceScopeNode != nullptr ? namespaceScopeNode : node;
            break;
        }
        if (node->IsBlockStatement() || node->IsFunctionDeclaration() || node->IsFunctionExpression() ||
            node->IsArrowFunctionExpression()) {
            scopeNode = node;
            break;
        }
        node = node->Parent();
    }
    if (scopeNode == nullptr) {
        scopeNode = namespaceScopeNode;
    }
    if (scopeNode == nullptr) {
        scopeNode = ctx->parserProgram->Ast();
    }
    return GenerateUniqueName(baseName, [scopeNode](const std::string &name) { return ScopeHasName(scopeNode, name); });
}

std::string GenerateUniqueNamespaceVarName(const RefactorContext &context, size_t namespaceDepth)
{
    std::string baseName = "newLocal";
    auto *node = GetTouchingToken(context.context, context.span.pos, false);
    auto *namespaceScope = FindNamespaceScopeByDepth(node, namespaceDepth);
    if (namespaceScope == nullptr) {
        return GenerateUniqueEncloseVarName(context);
    }
    return GenerateUniqueName(baseName,
                              [namespaceScope](const std::string &name) { return ScopeHasName(namespaceScope, name); });
}

std::string GenerateUniqueExtractedVarName(const RefactorContext &context, const std::string &actionName)
{
    if (IsConstantExtractionInClassAction(actionName)) {
        auto *node = GetTouchingToken(context.context, context.span.pos, false);
        if (IsNamespaceContext(node)) {
            return GenerateUniqueNamespaceVarName(context, 0);
        }
        return GenerateUniqueClassPropertyName(context);
    }
    if (IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL) ||
        IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_GLOBAL)) {
        return GenerateUniqueGlobalVarName(context);
    }
    if (IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_ENCLOSE)) {
        return GenerateUniqueEncloseVarName(context);
    }
    if (IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE)) {
        auto *node = GetTouchingToken(context.context, context.span.pos, false);
        if (IsNamespaceContext(node)) {
            return GenerateUniqueNamespaceVarName(context, 0);
        }
        return GenerateUniqueEncloseVarName(context);
    }
    if (const auto namespaceDepth = GetNamespaceActionDepth(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE.name,
                                                            EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX);
        namespaceDepth.has_value() && !IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE)) {
        return GenerateUniqueNamespaceVarName(context, namespaceDepth.value());
    }
    return "";
}

size_t DetermineGlobalInsertPos(public_lib::Context *ctx)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return 0;
    }
    const auto &src = ctx->sourceFile->source;
    size_t offset = 0;
    size_t lastDirectiveEnd = 0;
    while (offset < src.size()) {
        size_t lineStart = offset;
        while (lineStart < src.size() && IsIndentChar(src[lineStart])) {
            ++lineStart;
        }
        if (lineStart >= src.size()) {
            break;
        }
        if (src[lineStart] != '\'' && src[lineStart] != '"') {
            break;
        }
#ifdef _WIN32
        size_t newline = src.find(WINDOWS_LINE_BREAK, lineStart);
#else
        size_t newline = src.find(LINE_FEED, lineStart);
#endif
        if (newline == std::string::npos) {
            lastDirectiveEnd = src.size();
            break;
        }
        lastDirectiveEnd = newline + 1;
        offset = newline + 1;
    }
    return lastDirectiveEnd;
}

size_t ExtendToLineEnd(util::StringView source, size_t index)
{
    auto sv = source.Utf8();
    size_t pos = std::min(index, sv.size());

    while (pos < sv.size() && !IsLineBreakChar(sv[pos])) {
        ++pos;
    }
    while (pos < sv.size() && IsLineBreakChar(sv[pos])) {
        ++pos;
    }
    return pos;
}

void TrimTrailingNewlines(std::string &text)
{
    while (!text.empty() && IsLineBreakChar(text.back())) {
        text.pop_back();
    }
}

void StripLeadingIndent(std::string &text, const std::string &indent)
{
    if (text.rfind(indent, 0) == 0) {
        text.erase(0, indent.size());
    }
}

std::string BuildAssignmentLine(public_lib::Context *ctx, const VariableBindingInfo &binding, const std::string &indent,
                                const std::string &callExpr, const std::string &newLine)
{
    std::string line;
    auto *parent = binding.declaration != nullptr ? binding.declaration->Parent() : nullptr;
    bool needsLeadingBlank = parent != nullptr && !parent->IsProgram();
    auto keyword = GetKeyword(binding.declaration->Kind());
    auto declaratorId = GetDeclaratorIdText(ctx, binding);
    line.reserve(indent.size() + keyword.size() + declaratorId.size() + callExpr.size());
    if (needsLeadingBlank) {
        line.append(newLine);
    }

    line.append(indent);
    line.append(keyword);
    line.append(" ");
    line.append(declaratorId);
    line.append(" = ");
    line.append(callExpr);
    if (line.back() != ';') {
        line.append(";");
    }
    line.append(newLine);
    return line;
}

bool PrepareBindingLayout(public_lib::Context *ctx, const VariableBindingInfo &binding, size_t &lineStart,
                          std::string &indent, std::string &trimmedBody)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return false;
    }
    const auto &source = ctx->sourceFile->source;
    auto [start, indentEnd] = ComputeLineIndent(util::StringView(source), binding.declaration->Start().index);
    lineStart = start;
    indent = source.substr(start, indentEnd - start);

    trimmedBody = GetNodeText(ctx, binding.declaration);
    if (trimmedBody.empty()) {
        return false;
    }
    TrimTrailingNewlines(trimmedBody);
    StripLeadingIndent(trimmedBody, indent);
    return true;
}

std::pair<std::string, std::string> BuildParamSignature(const RefactorContext &context, public_lib::Context *ctx,
                                                        const VariableBindingInfo &binding, bool includeNonGlobal)
{
    auto *enclosingFunc = FindScriptFunction(binding.declaration);
    auto paramText = CollectParameterText(ctx, enclosingFunc);
    TextRange initializerRange {binding.initializer->Start().index, binding.initializer->End().index};
    FunctionIOInfo ioInfo = AnalyzeFunctionIO(context, initializerRange, includeNonGlobal);
    if (!ioInfo.callArgs.empty()) {
        std::vector<std::string> paramDecls;
        paramDecls.reserve(ioInfo.callArgs.size());
        for (size_t i = 0; i < ioInfo.callArgs.size(); ++i) {
            const std::string &name = ioInfo.callArgs[i];
            auto paramIt = paramText.find(name);
            if (paramIt != paramText.end()) {
                // Keep declared function parameter text stable (e.g. `a: number`) for compatibility.
                paramDecls.push_back(paramIt->second);
                continue;
            }
            const std::string ioDecl = i < ioInfo.paramDecls.size() ? ioInfo.paramDecls[i] : name;
            paramDecls.push_back(ioDecl.empty() ? name : ioDecl);
        }
        return {JoinWithComma(paramDecls), JoinWithComma(ioInfo.callArgs)};
    }

    std::vector<std::string> freeVars =
        CollectIdentifierNames(binding.initializer, IdentifierNameMutf8(binding.identifier));

    std::vector<std::string> paramDecls;
    paramDecls.reserve(freeVars.size());
    for (const auto &name : freeVars) {
        auto it = paramText.find(name);
        paramDecls.push_back(it == paramText.end() ? name : it->second);
    }
    return {JoinWithComma(paramDecls), JoinWithComma(freeVars)};
}

static size_t ResolveIndentSize(const RefactorContext &context)
{
    const size_t globalDefaultIndentSize = FormatCodeSettings().GetIndentSize();
    if (context.textChangesContext == nullptr) {
        return globalDefaultIndentSize;
    }
    size_t indentSize = context.textChangesContext->formatContext.GetFormatCodeSettings().GetIndentSize();
    return indentSize == 0 ? globalDefaultIndentSize : indentSize;
}

bool BuildGlobalPieces(const RefactorContext &context, const VariableBindingInfo &binding, HelperPieces &out)
{
    auto *pubCtx = reinterpret_cast<public_lib::Context *>(context.context);
    if (pubCtx == nullptr || pubCtx->sourceFile == nullptr) {
        return false;
    }
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    // NOLINTNEXTLINE(readability-identifier-naming)
    std::string helperName =
        GenerateUniqueFuncName(context, "newFunction", std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name));

    if (FindEnclosingClassDefinition(binding.declaration) == nullptr) {
        return false;
    }

    const auto &source = pubCtx->sourceFile->source;
    auto [lineStart, indentEnd] = ComputeLineIndent(util::StringView(source), binding.declaration->Start().index);
    std::string indent(source.substr(lineStart, indentEnd - lineStart));

    auto [paramsSig, callArgs] = BuildParamSignature(context, pubCtx, binding, true);

    std::string callExpr = std::string(helperName) + "(" + callArgs + ")";
    std::string replacement = BuildAssignmentLine(pubCtx, binding, indent, callExpr, newLine);

    out.insertHelper = false;

    if (!ProgramHasFunction(pubCtx, std::string(helperName))) {
        std::string initBody = GetNodeText(pubCtx, binding.initializer);
        TrimTrailingNewlines(initBody);
        const std::string indentStep(ResolveIndentSize(context), SPACE_CHAR);
        std::string helper;
        helper.reserve(paramsSig.size() + initBody.size() + HELPER_RESERVE_PADDING);
        helper.append(newLine);
        helper.append("function ").append(helperName).append("(").append(paramsSig).append(") {").append(newLine);
        helper.append(indentStep).append("return ").append(initBody);
        if (!initBody.empty() && helper.back() != ';') {
            helper.append(";");
        }
        helper.append(newLine).append("}").append(newLine);
        out.insertHelper = true;
        out.insertPos = DetermineGlobalInsertPos(pubCtx);
        out.helperText = std::move(helper);
    }

    out.replacementText = std::move(replacement);
    out.replaceRange = {lineStart, ExtendToLineEnd(util::StringView(source), binding.declaration->End().index)};
    return true;
}

size_t FindClassHelperInsertPos(public_lib::Context *ctx, ir::ClassDefinition *classDef)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || classDef == nullptr) {
        return 0;
    }

    const auto &source = ctx->sourceFile->source;
    size_t pos = std::min(classDef->End().index, source.size());
    while (pos > 0 && (std::isspace(static_cast<unsigned char>(source[pos - 1])) != 0)) {
        --pos;
    }
    if (pos == 0) {
        return 0;
    }

    size_t bracePos = pos;
    while (bracePos > 0 && source[bracePos - 1] != '}') {
        --bracePos;
    }
    if (bracePos == 0 && (source[bracePos] != '}')) {
        return pos;
    }

    size_t lineStart = bracePos;
    while (lineStart > 0 && !IsLineBreakChar(source[lineStart - 1])) {
        --lineStart;
    }
    return lineStart;
}

static std::string ResolveClassIndent(std::string_view methodIndent, size_t indentSize)
{
    if (methodIndent.size() < indentSize) {
        return "";
    }
    return std::string(methodIndent.substr(0, methodIndent.size() - indentSize));
}

static void AppendClassHelperSignature(std::string &helper, std::string_view classIndent, std::string_view helperName,
                                       std::string_view paramsSig, std::string_view newLine)
{
    helper.append(newLine);
    helper.append(classIndent)
        .append("private ")
        .append(helperName)
        .append("(")
        .append(paramsSig)
        .append(") {")
        .append(newLine);
}

static void AppendClassHelperBodyLine(std::string &helper, std::string_view classIndent, std::string_view indentStep,
                                      std::string_view body, std::string_view newLine)
{
    helper.append(classIndent).append(indentStep).append(body).append(newLine);
}

static void AppendClassHelperReturnLine(std::string &helper, std::string_view classIndent, std::string_view indentStep,
                                        std::string_view returnName, std::string_view newLine)
{
    helper.append(classIndent).append(indentStep).append("return ").append(returnName).append(";").append(newLine);
}

bool BuildClassPieces(const RefactorContext &context, const VariableBindingInfo &binding, HelperPieces &out)
{
    auto *pubCtx = reinterpret_cast<public_lib::Context *>(context.context);
    if (pubCtx == nullptr || pubCtx->sourceFile == nullptr) {
        return false;
    }
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    // NOLINTNEXTLINE(readability-identifier-naming)
    std::string helperName =
        GenerateUniqueFuncName(context, "newMethod", std::string(EXTRACT_FUNCTION_ACTION_CLASS.name));

    auto *classDef = FindEnclosingClassDefinition(binding.declaration);
    if (classDef == nullptr) {
        return false;
    }

    size_t lineStart = 0;
    std::string methodIndent;
    std::string body;
    if (!PrepareBindingLayout(pubCtx, binding, lineStart, methodIndent, body)) {
        return false;
    }
    const size_t indentSize = ResolveIndentSize(context);
    const std::string indentStep(indentSize, SPACE_CHAR);
    std::string classIndent = ResolveClassIndent(methodIndent, indentSize);

    auto [paramsSig, callArgs] = BuildParamSignature(context, pubCtx, binding, false);
    const std::string returnName = IdentifierNameMutf8(binding.identifier);

    std::string helper;
    helper.reserve(body.size() + paramsSig.size() + HELPER_RESERVE_PADDING);
    AppendClassHelperSignature(helper, classIndent, helperName, paramsSig, newLine);
    AppendClassHelperBodyLine(helper, classIndent, indentStep, body, newLine);
    AppendClassHelperReturnLine(helper, classIndent, indentStep, returnName, newLine);
    helper.append(classIndent).append("}").append(newLine);

    std::string callExpr = "this." + std::string(helperName) + "(" + callArgs + ")";
    std::string replacement = BuildAssignmentLine(pubCtx, binding, methodIndent, callExpr, newLine);

    out.insertHelper = true;
    size_t insertPos = FindClassHelperInsertPos(pubCtx, classDef);
    out.insertPos = insertPos == 0 ? classDef->End().index : insertPos;
    out.helperText = std::move(helper);
    out.replacementText = std::move(replacement);
    const auto &source = pubCtx->sourceFile->source;
    out.replaceRange = {lineStart, ExtendToLineEnd(util::StringView(source), binding.declaration->End().index)};
    return true;
}

size_t FindRenameIndex(HelperPieces &pieces)
{
    size_t index = 0;
    while (index < pieces.replacementText.size() && pieces.replacementText[index] == ' ') {
        ++index;
    }

    size_t newMethodPos = pieces.replacementText.find("newMethod", index);
    size_t extractedPos = pieces.replacementText.find("extractedFunction", index);
    size_t newFunctionPos = pieces.replacementText.find("newFunction", index);
    if (newMethodPos != std::string::npos) {
        index = newMethodPos;
    }
    if (extractedPos != std::string::npos) {
        index = extractedPos;
    }
    if (newFunctionPos != std::string::npos) {
        index = newFunctionPos;
    }
    size_t shift = 0;
    if (pieces.insertHelper && pieces.insertPos <= pieces.replaceRange.pos) {
        shift = pieces.helperText.length();
    }
    return index + pieces.replaceRange.pos + shift + 1;
}

bool TryBuildHelperExtraction(const RefactorContext &context, ir::AstNode *extractedNode, const std::string &actionName,
                              RefactorEditInfo &outEdits)
{
    if (actionName != std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name) &&
        actionName != std::string(EXTRACT_FUNCTION_ACTION_CLASS.name)) {
        return false;
    }

    auto *pubCtx = reinterpret_cast<public_lib::Context *>(context.context);
    if (pubCtx == nullptr || context.textChangesContext == nullptr || pubCtx->sourceFile == nullptr) {
        return false;
    }
    const auto fileName = pubCtx->sourceFile->filePath;
    VariableBindingInfo binding;
    if (!ResolveVariableBinding(extractedNode, binding)) {
        return false;
    }

    HelperPieces pieces;
    bool success = false;
    if (actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name)) {
        success = BuildGlobalPieces(context, binding, pieces);
    } else if (actionName == std::string(EXTRACT_FUNCTION_ACTION_CLASS.name)) {
        success = BuildClassPieces(context, binding, pieces);
    }

    if (!success) {
        return false;
    }

    TextChangesContext textChangesContext = *context.textChangesContext;
    auto edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        if (pieces.insertHelper && !pieces.helperText.empty()) {
            tracker.InsertText(pubCtx->sourceFile, pieces.insertPos, pieces.helperText);
        }
        tracker.ReplaceRangeWithText(pubCtx->sourceFile, pieces.replaceRange, pieces.replacementText);
    });

    outEdits = RefactorEditInfo(std::move(edits), std::optional<std::string>(fileName),
                                std::optional<size_t>(FindRenameIndex(pieces)));
    return true;
}

bool IsClassContext(ir::AstNode *node)
{
    auto *cls = FindEnclosingClassDefinition(node);
    auto *classDef = cls == nullptr ? nullptr : cls->AsClassDefinition();
    return classDef != nullptr && classDef->Ident() != nullptr && !classDef->IsGlobal() &&
           !classDef->IsNamespaceTransformed();
}

}  // namespace

static bool IsEncloseVarConstBreak(ir::AstNode *parent)
{
    return parent != nullptr && parent->IsBlockStatement();
}

static bool IsGlobalBreak(ir::AstNode *parent)
{
    return parent != nullptr &&
           (parent->IsETSModule() || (parent->IsClassDefinition() && parent->AsClassDefinition()->IsGlobal()));
}

static bool IsClassBreak(ir::AstNode *parent)
{
    return parent != nullptr && parent->IsClassDeclaration();
}

static bool IsNamespaceBreak(ir::AstNode *parent)
{
    if (parent == nullptr || !parent->IsClassDeclaration()) {
        return false;
    }
    auto *classDef = parent->AsClassDeclaration()->Definition();
    return classDef != nullptr && classDef->IsNamespaceTransformed();
}

static bool AreNodesCommaSeparated(public_lib::Context *ctx, ir::AstNode *first, ir::AstNode *second)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || first == nullptr || second == nullptr) {
        return false;
    }
    if (first->End().index >= second->Start().index) {
        return false;
    }

    const std::string_view &source = ctx->sourceFile->source;

    bool hasComma = false;
    bool hasSemicolon = false;
    for (size_t i = first->End().index; i < second->Start().index && i < source.length(); i++) {
        if (source[i] == ',') {
            hasComma = true;
        } else if (source[i] == ';') {
            hasSemicolon = true;
            break;
        }
    }

    return hasComma && !hasSemicolon;
}

static std::vector<ir::AstNode *> GetSiblings(ir::AstNode *parent)
{
    std::vector<ir::AstNode *> siblings;
    if (parent == nullptr) {
        return siblings;
    }

    parent->Iterate([&siblings](ir::AstNode *child) {
        if (child != nullptr) {
            siblings.push_back(child);
        }
    });

    return siblings;
}

static bool IsMultiDecl(ir::AstNode *node, public_lib::Context *context)
{
    if (node == nullptr) {
        return false;
    }
    if (context == nullptr || context->sourceFile == nullptr) {
        return false;
    }
    ir::ClassProperty *targetClassProperty = nullptr;
    for (ir::AstNode *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsClassProperty()) {
            targetClassProperty = current->AsClassProperty();
            break;
        }

        if (current->IsBlockStatement() || current->IsScriptFunction() || current->IsProgram()) {
            break;
        }
    }
    if (targetClassProperty == nullptr) {
        return false;
    }

    ir::AstNode *parent = targetClassProperty->Parent();
    if (parent == nullptr) {
        return false;
    }

    ir::ClassProperty *prevClassProperty = nullptr;
    ir::ClassProperty *nextClassProperty = nullptr;

    auto siblings = GetSiblings(parent);
    for (size_t i = 0; i < siblings.size(); i++) {
        if (siblings[i] != targetClassProperty) {
            continue;
        }

        if (i > 0 && siblings[i - 1]->IsClassProperty()) {
            prevClassProperty = siblings[i - 1]->AsClassProperty();
        }

        if (i < siblings.size() - 1 && siblings[i + 1]->IsClassProperty()) {
            nextClassProperty = siblings[i + 1]->AsClassProperty();
        }
        break;
    }

    return (prevClassProperty != nullptr && (AreNodesCommaSeparated(context, prevClassProperty, targetClassProperty) ||
                                             AreNodesCommaSeparated(context, targetClassProperty, nextClassProperty)));
}

static void AdjustStatementForGlobalIfClass(ir::AstNode *&statement, ir::AstNode *node, size_t startPos)
{
    if (node == nullptr || !node->IsClassDefinition()) {
        return;
    }

    auto *cls = node->AsClassDefinition();
    if (!cls->Body().empty()) {
        statement = cls->Body().at(0);
    }

    for (auto ndx : cls->Body()) {
        if (ndx->Start().index > startPos || ndx->Start().index == ndx->End().index) {
            break;
        }
        statement = ndx;
        if (!ndx->IsClassProperty()) {
            break;
        }
    }
}

static ir::AstNode *FindBreakPosition(ir::AstNode *target, bool (*breakCondition)(ir::AstNode *))
{
    for (ir::AstNode *node = target; node != nullptr; node = node->Parent()) {
        if (breakCondition(node->Parent())) {
            return node;
        }
    }
    return nullptr;
}

static ir::AstNode *FindEncloseInsertPositionForClass(ir::AstNode *target, ir::AstNode *classNode, size_t startPos)
{
    for (ir::AstNode *ancestor = target; ancestor != nullptr && ancestor != classNode; ancestor = ancestor->Parent()) {
        if (ancestor->Parent() == classNode) {
            return ancestor;
        }
    }
    ir::AstNode *insertPosNode = classNode;
    AdjustStatementForGlobalIfClass(insertPosNode, classNode, startPos);
    return insertPosNode;
}

static ir::AstNode *FindEncloseBreakPosition(ir::AstNode *target, size_t startPos)
{
    for (ir::AstNode *node = target; node != nullptr; node = node->Parent()) {
        auto *parent = node->Parent();
        if (IsEncloseVarConstBreak(parent)) {
            return node;
        }
        if (!IsNamespaceBreak(parent)) {
            continue;
        }
        if (node->IsClassDefinition()) {
            return FindEncloseInsertPositionForClass(target, node, startPos);
        }
        ir::AstNode *insertPosNode = node;
        AdjustStatementForGlobalIfClass(insertPosNode, node, startPos);
        return insertPosNode;
    }
    return nullptr;
}

static ir::AstNode *FindNamespaceBreakPositionImpl(ir::AstNode *target, size_t namespaceDepth, size_t startPos)
{
    size_t currentDepth = 0;
    ir::AstNode *previousNode = nullptr;
    for (ir::AstNode *node = target; node != nullptr; previousNode = node, node = node->Parent()) {
        if (!IsNamespaceBreak(node->Parent())) {
            continue;
        }
        if (currentDepth != namespaceDepth) {
            ++currentDepth;
            continue;
        }
        if (namespaceDepth > 0 && previousNode != nullptr) {
            return previousNode;
        }
        if (namespaceDepth == 0 && node->IsClassDefinition()) {
            return FindEncloseInsertPositionForClass(target, node, startPos);
        }
        ir::AstNode *insertPosNode = node;
        AdjustStatementForGlobalIfClass(insertPosNode, node, startPos);
        return insertPosNode;
    }
    return nullptr;
}

static ir::AstNode *FindNamespaceBreakPosition(ir::AstNode *target, size_t namespaceDepth, size_t startPos)
{
    return FindNamespaceBreakPositionImpl(target, namespaceDepth, startPos);
}

static ir::AstNode *FindClassBreakPosition(ir::AstNode *target, size_t startPos)
{
    for (ir::AstNode *node = target; node != nullptr; node = node->Parent()) {
        if (IsClassBreak(node->Parent())) {
            ir::AstNode *insertPosNode = node;
            AdjustStatementForGlobalIfClass(insertPosNode, node, startPos);
            return insertPosNode;
        }
    }
    return nullptr;
}

namespace {

size_t SkipSpacesBackward(std::string_view source, size_t pos)
{
    while (pos > 0 && IsIndentChar(source[pos - 1])) {
        --pos;
    }
    return pos;
}

std::pair<size_t, size_t> CountAndSkipLineBreaksBackward(std::string_view source, size_t pos)
{
    size_t lineBreaks = 0;
    while (pos > 0 && IsLineBreakChar(source[pos - 1])) {
        if (source[pos - 1] == LINE_FEED) {
            ++lineBreaks;
        }
        --pos;
    }
    return {pos, lineBreaks};
}

size_t FindLineStart(std::string_view source, size_t pos)
{
    while (pos > 0 && !IsLineBreakChar(source[pos - 1])) {
        --pos;
    }
    return pos;
}

std::optional<size_t> FindVariableDeclKeywordStart(std::string_view source, size_t nodeStart)
{
    if (nodeStart >= source.size()) {
        return std::nullopt;
    }
    size_t lineStart = FindLineStart(source, nodeStart);
    size_t cursor = lineStart;
    while (cursor < nodeStart && IsIndentChar(source[cursor])) {
        ++cursor;
    }
    auto matchesKeyword = [source, cursor](std::string_view keyword) {
        if (cursor + keyword.size() > source.size()) {
            return false;
        }
        if (source.compare(cursor, keyword.size(), keyword) != 0) {
            return false;
        }
        size_t tail = cursor + keyword.size();
        return tail < source.size() && std::isspace(static_cast<unsigned char>(source[tail])) != 0;
    };
    if (matchesKeyword("let") || matchesKeyword("const") || matchesKeyword("var")) {
        return cursor;
    }
    return std::nullopt;
}

bool ConsumeKeyword(std::string_view source, size_t &cursor, std::string_view keyword)
{
    if (cursor + keyword.size() > source.size() || source.compare(cursor, keyword.size(), keyword) != 0) {
        return false;
    }
    size_t afterKeyword = cursor + keyword.size();
    if (afterKeyword >= source.size() || std::isspace(static_cast<unsigned char>(source[afterKeyword])) == 0) {
        return false;
    }
    cursor = afterKeyword;
    return true;
}

size_t SkipWhitespace(std::string_view source, size_t cursor, size_t limit)
{
    while (cursor < limit && std::isspace(static_cast<unsigned char>(source[cursor])) != 0) {
        ++cursor;
    }
    return cursor;
}

size_t SkipIdentifier(std::string_view source, size_t cursor, size_t limit)
{
    while (cursor < limit && (std::isalnum(static_cast<unsigned char>(source[cursor])) != 0 || source[cursor] == '_' ||
                              source[cursor] == '$')) {
        ++cursor;
    }
    return cursor;
}

std::optional<size_t> FindAssignPosBeforeInitializer(std::string_view source, size_t lineStart, size_t initializerStart)
{
    size_t eqPos = initializerStart;
    while (eqPos > lineStart && std::isspace(static_cast<unsigned char>(source[eqPos - 1])) != 0) {
        --eqPos;
    }
    if (eqPos == 0 || source[eqPos - 1] != '=') {
        return std::nullopt;
    }
    return eqPos - 1;
}

std::optional<std::string> ExtractTypeText(std::string_view source, size_t startPos, size_t endPos)
{
    if (startPos >= endPos || endPos > source.size()) {
        return std::nullopt;
    }
    std::string typeText(source.substr(startPos, endPos - startPos));
    size_t start = 0;
    while (start < typeText.size() && std::isspace(static_cast<unsigned char>(typeText[start])) != 0) {
        ++start;
    }
    size_t end = typeText.size();
    while (end > start && std::isspace(static_cast<unsigned char>(typeText[end - 1])) != 0) {
        --end;
    }
    if (start == end) {
        return std::nullopt;
    }
    return typeText.substr(start, end - start);
}

std::optional<std::string> ExtractVariableDeclaredTypeFromInitializer(std::string_view source, size_t initializerStart)
{
    if (initializerStart == 0 || initializerStart > source.size()) {
        return std::nullopt;
    }
    const size_t lineStart = FindLineStart(source, initializerStart);
    size_t cursor = lineStart;
    while (cursor < initializerStart && IsIndentChar(source[cursor])) {
        ++cursor;
    }
    if (!ConsumeKeyword(source, cursor, "let") && !ConsumeKeyword(source, cursor, "const") &&
        !ConsumeKeyword(source, cursor, "var")) {
        return std::nullopt;
    }
    cursor = SkipWhitespace(source, cursor, initializerStart);
    cursor = SkipIdentifier(source, cursor, initializerStart);

    auto assignPos = FindAssignPosBeforeInitializer(source, lineStart, initializerStart);
    if (!assignPos.has_value()) {
        return std::nullopt;
    }
    const size_t colonPos = source.find(':', cursor);
    if (colonPos == std::string::npos || colonPos >= assignPos.value()) {
        return std::nullopt;
    }
    return ExtractTypeText(source, colonPos + 1, assignPos.value());
}

size_t SkipSpacesForward(std::string_view source, size_t pos, size_t limit)
{
    while (pos < limit && IsIndentChar(source[pos])) {
        ++pos;
    }
    return pos;
}

std::optional<size_t> FindTightBlockCommentStart(std::string_view source, size_t scan)
{
    constexpr size_t blockCommentSuffixLen = 2;
    constexpr size_t blockCommentStarOffset = 2;
    constexpr size_t blockCommentSlashOffset = 1;
    if (scan < blockCommentSuffixLen || source[scan - blockCommentStarOffset] != '*' ||
        source[scan - blockCommentSlashOffset] != '/') {
        return std::nullopt;
    }
    const size_t commentStart = source.rfind("/*", scan - blockCommentSuffixLen);
    if (commentStart == std::string::npos) {
        return std::nullopt;
    }
    return commentStart;
}

std::optional<size_t> FindTightLineCommentStart(std::string_view source, size_t scan)
{
    size_t lineStart = FindLineStart(source, scan);
    lineStart = SkipSpacesForward(source, lineStart, scan);
    if (lineStart + 1 < scan && source[lineStart] == '/' && source[lineStart + 1] == '/') {
        return lineStart;
    }
    return std::nullopt;
}

}  // namespace

size_t FindInsertionPosBeforeTightLeadingComment(std::string_view source, size_t declarationStart)
{
    if (declarationStart == 0 || declarationStart > source.size()) {
        return declarationStart;
    }

    size_t scan = SkipSpacesBackward(source, declarationStart);
    auto [afterBreaks, lineBreaks] = CountAndSkipLineBreaksBackward(source, scan);
    if (lineBreaks > 1) {
        return declarationStart;
    }
    scan = SkipSpacesBackward(source, afterBreaks);
    auto blockStart = FindTightBlockCommentStart(source, scan);
    if (blockStart.has_value()) {
        return *blockStart;
    }
    auto lineStart = FindTightLineCommentStart(source, scan);
    if (lineStart.has_value()) {
        return *lineStart;
    }

    return declarationStart;
}

size_t FindPreviousNonCommentLine(const public_lib::Context *context, size_t startLine /* 0-based */)
{
    lexer::LineIndex lineIndex(context->parserProgram->SourceCode());
    std::string_view sourceCode = context->parserProgram->SourceCode();
    size_t currentLine = startLine;
    bool lookingForBlockStart = false;

    auto isCommentOnlyBlockLine = [](std::string_view lineText) -> bool {
        const size_t firstNonSpace = lineText.find_first_not_of(" \t");
        if (firstNonSpace == std::string_view::npos) {
            return false;
        }
        if (lineText.compare(firstNonSpace, 2, "/*") != 0) {
            return false;
        }
        const size_t blockEnd = lineText.find("*/", firstNonSpace + 2);
        if (blockEnd == std::string_view::npos) {
            return false;
        }
        return lineText.find_first_not_of(" \t", blockEnd + 2) == std::string_view::npos;
    };

    for (; currentLine > 0; currentLine--) {
        size_t lineStart = (currentLine == 0) ? 0 : lineIndex.GetOffsetOfLine(currentLine - 1) + 1;
        size_t lineEnd = lineIndex.GetOffsetOfLine(currentLine);
        std::string lineText(sourceCode.begin() + lineStart, sourceCode.begin() + lineEnd);

        if (lookingForBlockStart) {
            if (lineText.find("/*") != std::string::npos) {
                lookingForBlockStart = false;
            }
        } else {
            if (isCommentOnlyBlockLine(lineText)) {
                continue;
            }
            if (lineText.find("*/") != std::string::npos) {
                lookingForBlockStart = true;
            } else if (lineText.find("//") == std::string::npos) {
                break;
            }
        }
    }

    return currentLine;
}

static bool IsFunctionExtractionActionName(const std::string &actionName)
{
    return actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name) ||
           actionName == std::string(EXTRACT_FUNCTION_ACTION_CLASS.name) ||
           IsNamespaceAction(actionName, EXTRACT_FUNCTION_ACTION_ENCLOSE.name,
                             EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX);
}

static ir::AstNode *ResolveInsertPosNode(ir::AstNode *target, const std::string &actionName, size_t startPos,
                                         const std::optional<size_t> &functionNamespaceDepth)
{
    ir::AstNode *insertPosNode = nullptr;
    if (actionName == std::string(EXTRACT_VARIABLE_ACTION_ENCLOSE.name) ||
        actionName == std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.name)) {
        insertPosNode = FindEncloseBreakPosition(target, startPos);
    } else if (const auto namespaceDepth = GetNamespaceActionDepth(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE.name,
                                                                   EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX);
               namespaceDepth.has_value()) {
        insertPosNode = FindNamespaceBreakPosition(target, namespaceDepth.value(), startPos);
    }
    if (functionNamespaceDepth.has_value()) {
        insertPosNode = FindNamespaceBreakPosition(target, functionNamespaceDepth.value(), startPos);
        if (insertPosNode == nullptr && actionName == std::string(EXTRACT_FUNCTION_ACTION_ENCLOSE.name)) {
            insertPosNode = FindEncloseBreakPosition(target, startPos);
        }
    }
    if (actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name) ||
        actionName == std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.name) ||
        actionName == std::string(EXTRACT_VARIABLE_ACTION_GLOBAL.name)) {
        insertPosNode = FindBreakPosition(target, IsGlobalBreak);
    }
    if (actionName == std::string(EXTRACT_FUNCTION_ACTION_CLASS.name) ||
        actionName == std::string(EXTRACT_CONSTANT_ACTION_CLASS.name)) {
        insertPosNode = FindClassBreakPosition(target, startPos);
    }
    return insertPosNode;
}

static size_t ComputeInsertPosAfterHost(public_lib::Context *context, ir::AstNode *insertPosNode,
                                        const std::optional<size_t> &functionNamespaceDepth)
{
    if (functionNamespaceDepth.has_value() && functionNamespaceDepth.value() > 0 &&
        insertPosNode->Parent() != nullptr) {
        const auto &source = context->sourceFile->source;
        const size_t parentEnd = std::min(insertPosNode->Parent()->End().index, source.size());
        return FindLineStart(source, parentEnd);
    }
    return insertPosNode->End().index;
}

static size_t ComputeInsertPosBeforeNode(public_lib::Context *context, ir::AstNode *target, ir::AstNode *insertPosNode)
{
    if (IsMultiDecl(target, context)) {
        if (context == nullptr || context->sourceFile == nullptr) {
            return insertPosNode->Start().index;
        }
        return FindInsertionPosBeforeTightLeadingComment(context->sourceFile->source, insertPosNode->Start().index);
    }
    size_t line = insertPosNode->Start().line == 0 ? 0 : insertPosNode->Start().line - 1;
    line = FindPreviousNonCommentLine(context, line);
    size_t lineEnd = LineToPos(context, line);
    return lineEnd < insertPosNode->Parent()->Start().index ? insertPosNode->Start().index : lineEnd;
}

size_t FindInsertionPos(public_lib::Context *context, ir::AstNode *target, const std::string &actionName,
                        size_t startPos, size_t endPos)
{
    const auto functionNamespaceDepth = GetNamespaceActionDepth(actionName, EXTRACT_FUNCTION_ACTION_ENCLOSE.name,
                                                                EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX);
    ir::AstNode *insertPosNode = ResolveInsertPosNode(target, actionName, startPos, functionNamespaceDepth);
    if (insertPosNode == nullptr) {
        return 0;
    }
    const bool shouldInsertAfterHost = IsFunctionExtractionActionName(actionName) && context != nullptr &&
                                       context->sourceFile != nullptr &&
                                       HasNewlineInRange(context->sourceFile->source, {startPos, endPos});
    if (shouldInsertAfterHost) {
        return ComputeInsertPosAfterHost(context, insertPosNode, functionNamespaceDepth);
    }
    return ComputeInsertPosBeforeNode(context, target, insertPosNode);
}

TextRange GetVarAndFunctionPosToWriteNode(const RefactorContext &context, const std::string &actionName)
{
    auto startedNode = GetTouchingTokenByRange(context.context, context.span, false);
    auto ctx = reinterpret_cast<public_lib::Context *>(context.context);
    const auto startPos = FindInsertionPos(ctx, startedNode, actionName, context.span.pos, context.span.end);
    return {startPos, startPos};
}

namespace {
ir::AstNode *FindTouchingTokenNearSpan(const RefactorContext &context)
{
    auto node = GetTouchingTokenByRange(context.context, context.span, false);
    if (node == nullptr) {
        node = GetTouchingToken(context.context, context.span.pos, false);
        if (node == nullptr && context.span.end > 0) {
            node = GetTouchingToken(context.context, context.span.end - 1, false);
        }
    }
    return node;
}

ir::AstNode *FindExpressionOrVarAncestor(ir::AstNode *node)
{
    while (node != nullptr && (!node->IsExpression() && !node->IsVariableDeclaration())) {
        node = node->Parent();
    }
    return node;
}

ir::AstNode *FindDeepestOverlappingExpression(ir::AstNode *originNode, TextRange range)
{
    if (originNode == nullptr) {
        return nullptr;
    }
    auto overlaps = [&range](ir::AstNode *candidate) {
        return candidate->Start().index < range.end && candidate->End().index > range.pos;
    };
    auto *exprNode =
        originNode->FindChild([&](ir::AstNode *child) { return child->IsExpression() && overlaps(child); });
    while (exprNode != nullptr) {
        auto *nested =
            exprNode->FindChild([&](ir::AstNode *child) { return child->IsExpression() && overlaps(child); });
        if (nested == nullptr) {
            break;
        }
        exprNode = nested;
    }
    return exprNode;
}
}  // namespace

ir::AstNode *FindExtractedVals(const RefactorContext &context)
{
    const auto rangeToExtract = GetTrimmedSelectionSpan(context);
    if (rangeToExtract.pos >= rangeToExtract.end) {
        return nullptr;
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr) {
        return nullptr;
    }
    if (ctx->parserProgram == nullptr) {
        return nullptr;
    }
    if (ctx->parserProgram->Ast() == nullptr) {
        return nullptr;
    }

    auto node = FindTouchingTokenNearSpan(context);
    if (node == nullptr) {
        node = FindTouchingTokenByScan(context, ctx);
    }
    if (node == nullptr) {
        return nullptr;
    }
    auto *originNode = node;
    node = FindExpressionOrVarAncestor(node);
    if (node != nullptr) {
        return node;
    }

    auto *exprNode = FindDeepestOverlappingExpression(originNode, rangeToExtract);
    if (exprNode != nullptr) {
        return exprNode;
    }

    return nullptr;
}
ir::AstNode *FindExtractedFunction(const RefactorContext &context)
{
    const auto rangeToExtract = GetTrimmedSelectionSpan(context);
    if (rangeToExtract.pos >= rangeToExtract.end) {
        return nullptr;
    }
    const auto ast = reinterpret_cast<public_lib::Context *>(context.context)->parserProgram->Ast();
    if (ast == nullptr) {
        return nullptr;
    }
    auto node = GetTouchingToken(context.context, rangeToExtract.pos, false);
    if (node == nullptr) {
        return nullptr;
    }
    while (node != nullptr && (!node->IsExpression() && !node->IsFunctionExpression() &&
                               !node->IsArrowFunctionExpression() && !node->IsStatement())) {
        node = node->Parent();
    }
    if (node != nullptr) {
        return node;
    }
    return nullptr;
}

std::vector<FunctionExtraction> GetPossibleFunctionExtractions(const RefactorContext &context)
{
    std::vector<FunctionExtraction> res;

    if (context.span.pos >= context.span.end) {
        return res;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr) {
        return res;
    }

    const auto ast = ctx->parserProgram->Ast();
    if (ast == nullptr) {
        return res;
    }
    ast->FindChild([&res](ir::AstNode *node) {
        if (node->IsStatement() || node->IsExpressionStatement() || node->IsBlockStatement()) {
            FunctionExtraction fe;
            fe.node = node;
            fe.targetRange = TextRange {node->Start().index, node->End().index};
            fe.description = "Extract statement(s) to function";
            res.push_back(std::move(fe));
        }
        return false;
    });

    return res;
}

void CollectFunctionParameters(FunctionExtraction &funExt)
{
    const auto node = funExt.node;
    if (node == nullptr) {
        return;
    }
    if (node->IsFunctionDeclaration()) {
        auto *func = node->AsFunctionDeclaration();
        for (auto *param : func->Function()->Params()) {
            if (param->IsETSParameterExpression()) {
                funExt.parameters.push_back(param->AsETSParameterExpression());
            }
        }
    } else if (node->IsFunctionExpression() || node->IsArrowFunctionExpression()) {
        auto *func = node->AsFunctionExpression();
        for (auto *param : func->Function()->Params()) {
            if (param->IsETSParameterExpression()) {
                funExt.parameters.push_back(param->AsETSParameterExpression());
            }
        }
    }
}

bool IsNodeInParamList(ir::Identifier *ident, const std::vector<ir::Identifier *> &list)
{
    if (list.empty()) {
        return false;
    }
    for (auto param : list) {
        if (ident->ToString() == param->ToString()) {
            return true;
        }
    }
    return false;
}
std::string GetParamsText(const FunctionExtraction &candidate, const std::vector<ir::Identifier *> &functionParams)
{
    std::string params;
    for (size_t i = 0; i < candidate.parameters.size(); i++) {
        bool funcParamInParams = IsNodeInParamList(candidate.parameters[i]->Ident(), functionParams);
        if (!candidate.parameters.empty() && candidate.parameters.size() > i && funcParamInParams) {
            if (i != 0) {
                params += ", ";
            }
            params += candidate.parameters[i]->Ident()->ToString() + " : " +
                      candidate.parameters[i]->TypeAnnotation()->AsETSTypeReference()->BaseName()->Name().Mutf8();
        }
    }
    return params;
}

std::vector<ir::Identifier *> CollectFunctionParams(ir::AstNode *ast, size_t start, size_t end, bool &needParams)
{
    std::vector<ir::Identifier *> params;
    ast->FindChild([&](ir::AstNode *child) {
        if ((child->Start().index >= start && child->End().index <= end) && !child->IsStringLiteral() &&
            !child->IsNumberLiteral() && !child->IsBooleanLiteral() && !child->IsNullLiteral() &&
            !child->IsCharLiteral()) {
            if (child->IsIdentifier()) {
                needParams = true;
                params.push_back(child->AsIdentifier());
            }
        }
        return false;
    });
    return params;
}

std::string BuildFunctionBody(const std::string &body, const FunctionBodyOptions &options)
{
    std::ostringstream oss;
    std::istringstream lines(body);
    std::string line;
    std::string indentStep(options.indentSize, ' ');
    if (options.returnEachLine) {
        while (std::getline(lines, line)) {
            oss << options.indent << indentStep << "return " << line
                << (std::strchr(line.c_str(), ';') != nullptr ? "" : ";") << options.newLine;
        }
        return oss.str();
    }

    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        size_t count = 0;
        while (count < line.size() && IsIndentChar(line[count])) {
            ++count;
        }
        size_t toStrip = std::min(count, options.trimIndent);
        std::string trimmed = line.size() >= toStrip ? line.substr(toStrip) : line;
        if (trimmed.empty()) {
            oss << options.newLine;
            continue;
        }
        oss << options.indent << indentStep << trimmed << options.newLine;
    }
    if (options.returnVar.has_value()) {
        oss << options.indent << indentStep << "return " << options.returnVar.value() << ";" << options.newLine;
    }
    return oss.str();
}
std::string GenerateExtractedFunctionCode(const std::string &bodyText, const std::string &params,
                                          const RefactorContext &context, std::string_view actionName,
                                          const FunctionBodyOptions &baseBodyOptions)
{
    std::string baseName = (actionName == EXTRACT_FUNCTION_ACTION_CLASS.name) ? "newMethod" : "newFunction";
    std::string functionName = GenerateUniqueFuncName(context, baseName, std::string(actionName));
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();

    std::ostringstream oss;
    if (baseBodyOptions.addLeadingNewLine) {
        oss << newLine;
    }
    FunctionBodyOptions bodyOptions = baseBodyOptions;
    bodyOptions.newLine = newLine;
    oss << bodyOptions.indent << "function " << functionName << "(" << params << ") {" << newLine
        << BuildFunctionBody(bodyText, bodyOptions) << bodyOptions.indent << "}" << newLine << newLine;
    return oss.str();
}

static bool TryResolveFunctionExtractionRange(const RefactorContext &context, public_lib::Context *ctx, size_t &start,
                                              size_t &end)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || ctx->sourceFile->source.empty()) {
        return false;
    }
    const auto extractionPos = GetCallPositionOfExtraction(context);
    start = extractionPos.pos;
    end = extractionPos.end;
    const auto &source = ctx->sourceFile->source;
    return start < source.size() && end <= source.size() && start < end;
}

static size_t ResolveFunctionTrimIndent(std::string_view source, size_t start, bool treatAsStatements)
{
    if (!treatAsStatements) {
        return 0;
    }
    auto [lineStart, indentEnd] = ComputeLineIndent(util::StringView(source), start);
    return indentEnd >= lineStart ? indentEnd - lineStart : 0;
}

std::string BuildFunctionText(const FunctionExtraction &candidate, const RefactorContext &context,
                              const std::string &actionName, const FunctionIOInfo *ioInfo,
                              const std::vector<std::string> *capturedParams)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    size_t start = 0;
    size_t end = 0;
    if (!TryResolveFunctionExtractionRange(context, ctx, start, end)) {
        return "";
    }

    const auto &src = ctx->sourceFile->source;
    const auto ast = ctx->parserProgram->Ast();
    const bool treatAsStatements = ioInfo != nullptr;
    std::string params;
    if (ioInfo != nullptr) {
        params = JoinWithComma(ioInfo->paramDecls);
    } else if (capturedParams != nullptr && !capturedParams->empty()) {
        params = JoinWithComma(*capturedParams);
    } else {
        bool needParams = false;
        auto functionParams = CollectFunctionParams(ast, start, end, needParams);
        params = needParams ? GetParamsText(candidate, functionParams) : "";
    }

    std::string bodyText(src.begin() + start, src.begin() + end);
    size_t insertPos = GetVarAndFunctionPosToWriteNode(context, actionName).pos;
    insertPos = NormalizeInsertPos(src, insertPos);
    const std::string indent = GetInsertionIndent(ctx, insertPos);
    const bool addLeadingNewLine = !IsLineStartAtPosition(src, insertPos);
    size_t trimIndent = ResolveFunctionTrimIndent(src, start, treatAsStatements);
    FunctionBodyOptions bodyOptions {"",
                                     indent,
                                     addLeadingNewLine,
                                     !treatAsStatements,
                                     ioInfo == nullptr ? std::nullopt : ioInfo->returnVar,
                                     trimIndent,
                                     ResolveIndentSize(context)};
    return GenerateExtractedFunctionCode(bodyText, params, context, actionName, bodyOptions);
}

std::string ReplaceWithFunctionCall(const std::string &functionText, const std::vector<std::string> &callArgs,
                                    const std::optional<std::string> &returnVar, bool needsStatement)
{
    std::string functionName = "newFunction";
    {
        auto pos = functionText.find("function ");
        if (pos != std::string::npos) {
            pos += strlen("function ");
            auto paren = functionText.find('(', pos);
            if (paren != std::string::npos) {
                functionName = functionText.substr(pos, paren - pos);
            }
        }
    }
    std::string callArgsText = JoinWithComma(callArgs);
    std::string callText = functionName + "(" + callArgsText + ")";
    if (returnVar.has_value()) {
        callText = returnVar.value() + " = " + callText;
    }
    if (needsStatement && !callText.empty() && callText.back() != ';') {
        callText.push_back(';');
    }
    return callText;
}
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

static bool HasEncloseScope(ir::AstNode *node)
{
    if (IsInGlobalClassStaticBlock(node)) {
        return false;
    }
    for (; node != nullptr; node = node->Parent()) {
        if (node->IsFunctionDeclaration() || node->IsFunctionExpression()) {
            return true;
        }
        if (node->IsArrowFunctionExpression()) {
            return HasBlockEnclosing(node);
        }
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

static void AddExtractVariableActions(std::vector<RefactorAction> &actions, const ScopeContext &scope)
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
    AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_GLOBAL);
}

std::vector<RefactorAction> FindAvailableRefactors(const RefactorContext &context)
{
    std::vector<RefactorAction> actions;

    ir::AstNode *node = nullptr;
    bool allowVariableActions = true;
    bool selectionHasNewline = false;
    TextRange trimmedSpan = context.span;
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx != nullptr && ctx->sourceFile != nullptr) {
        std::string_view source = ctx->sourceFile->source;
        selectionHasNewline = HasSelectionNewline(context, source);
        trimmedSpan = TrimSpanWhitespace(trimmedSpan, source);
    }

    node = ResolveNodeForSelection(context, ctx, selectionHasNewline);
    if (node == nullptr) {
        return actions;
    }

    const auto positions = GetCallPositionOfExtraction(context);
    if (!IsInsideExtractionRange(node, positions)) {
        if (!selectionHasNewline) {
            return actions;
        }
        auto *statementInRange = FindStatementOverlappingSelection(ctx, positions);
        if (statementInRange == nullptr) {
            return actions;
        }
        node = statementInRange;
        allowVariableActions = false;
    }

    if (context.span.pos != context.span.end) {
        if (trimmedSpan.pos == trimmedSpan.end) {
            allowVariableActions = false;
        } else if (node->Start().index != trimmedSpan.pos || node->End().index != trimmedSpan.end) {
            allowVariableActions = false;
        }
    }

    const auto scope = ResolveScopeContext(node);

    if (node->IsExpression() || node->IsFunctionExpression() || node->IsArrowFunctionExpression() ||
        node->IsStatement()) {
        AddExtractFunctionActions(actions, scope);
    }

    if (allowVariableActions && (!node->IsStatement() || node->IsVariableDeclaration() || node->IsBinaryExpression())) {
        AddExtractVariableActions(actions, scope);
    }

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

std::string GetConstantString(std::string_view &src, ir::AstNode *extractedText)
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

static bool HasNewlineBetween(std::string_view source, size_t start, size_t end)
{
    if (start >= end || start >= source.size()) {
        return false;
    }
    const size_t upper = std::min(end, source.size());
    for (size_t i = start; i < upper; ++i) {
        if (source[i] == '\n' || source[i] == '\r') {
            return true;
        }
    }
    return false;
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

static bool NeedsContinuationIndentForInline(std::string_view source, const ir::VariableDeclaration *declaration,
                                             size_t probeStart)
{
    if (declaration->Start().line != declaration->End().line ||
        HasNewlineBetween(source, declaration->Start().index, declaration->End().index)) {
        return true;
    }
    bool sawNewline = false;
    for (size_t probe = probeStart; probe > 0; --probe) {
        char ch = source[probe - 1];
        if (ch == '\n' || ch == '\r') {
            sawNewline = true;
            continue;
        }
        if (ch == ' ' || ch == '\t') {
            continue;
        }
        if (ch == ',') {
            return sawNewline;
        }
        break;
    }
    return false;
}

static std::optional<std::pair<size_t, std::string>> TryBuildInlineMultiDeclInsertion(const RefactorContext &context,
                                                                                      public_lib::Context *ctx,
                                                                                      ir::AstNode *extractedText,
                                                                                      const std::string &varName)
{
    auto nodes = ResolveInlineMultiDeclNodes(ctx, extractedText);
    if (!nodes.has_value()) {
        return std::nullopt;
    }
    auto *declarator = nodes->first;
    auto *declaration = nodes->second;
    auto declaratorIndex = FindDeclaratorIndex(declaration, declarator);
    if (!declaratorIndex.has_value()) {
        return std::nullopt;
    }
    auto &declarators = declaration->Declarators();
    auto *prev = declarators[declaratorIndex.value() - 1];
    if (!AreNodesCommaSeparated(ctx, prev, declarator)) {
        return std::nullopt;
    }
    const auto &source = ctx->sourceFile->source;
    std::string_view srcView(source);
    std::string placeholder = GetConstantString(srcView, extractedText);
    if (placeholder.empty()) {
        return std::nullopt;
    }

    const size_t declaratorPos = declarator->Start().index;
    std::string indent = GetIndentAtPosition(ctx, declaratorPos);
    if (indent.empty() && NeedsContinuationIndentForInline(source, declaration, declaratorPos)) {
        indent.assign(ResolveIndentSize(context), ' ');
    }

    return std::make_pair(declaratorPos, indent + varName + " = " + placeholder + ", ");
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

static std::optional<std::pair<size_t, std::string>> TryBuildInlineInsertion(const RefactorContext &context,
                                                                             public_lib::Context *ctx,
                                                                             ir::AstNode *extractedText,
                                                                             const std::string &uniqueVarName)
{
    auto inlineInsertionResult = TryBuildInlineMultiDeclInsertion(context, ctx, extractedText, uniqueVarName);
    if (!inlineInsertionResult.has_value()) {
        return std::nullopt;
    }
    auto [inlinePos, inlineText] = std::move(inlineInsertionResult.value());
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return std::make_pair(inlinePos, std::move(inlineText));
    }
    std::string_view source = ctx->sourceFile->source;
    const size_t namePos = inlineText.find(uniqueVarName);
    const bool hasIndent = namePos != std::string::npos && namePos > 0;
    const size_t extractedStart = extractedText == nullptr ? inlinePos : extractedText->Start().index;
    if (!hasIndent && HasNewlineBetween(source, inlinePos, extractedStart)) {
        inlineText.insert(0, ResolveIndentSize(context), ' ');
    }
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

static size_t ResolveInsertionPosForVariableExtraction(const RefactorContext &context, public_lib::Context *ctx,
                                                       const std::string &actionName, size_t insertPos)
{
    if (!IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_GLOBAL) || ctx == nullptr ||
        ctx->sourceFile == nullptr) {
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
    if (!IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_GLOBAL)) {
        return;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || generatedText.empty()) {
        return;
    }
    const auto &source = ctx->sourceFile->source;
    if (insertPos < source.size() && !IsLineBreak(source[insertPos]) && !IsLineBreak(generatedText.back())) {
        generatedText.append(context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter());
    }
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
    const bool atLineEnd = insertPos == lineEnd && IsLineBreak(source[lineEnd]);
    if (!atLineEnd) {
        return "";
    }
    const bool blankLine = IsBlankLine(source, lineStart, lineEnd);
    const bool hasLeadingBreak = !generatedText.empty() && IsLineBreak(generatedText[0]);
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
    if (insertPos >= source.size() || !IsLineBreakChar(source[insertPos])) {
        return false;
    }
    return true;
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

static std::vector<const TextChange *> CollectOrderedTextChanges(const std::vector<FileTextChanges> &edits,
                                                                 size_t insertPos, const TextChange *&insertChange,
                                                                 size_t &insertShift)
{
    std::vector<const TextChange *> ordered;
    insertChange = nullptr;
    insertShift = 0;
    if (edits.empty() || edits[0].textChanges.empty()) {
        return ordered;
    }
    ordered.reserve(edits[0].textChanges.size());
    for (const auto &change : edits[0].textChanges) {
        ordered.push_back(&change);
        if (insertChange == nullptr && change.span.length == 0 && change.span.start == insertPos) {
            insertChange = &change;
        }
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const TextChange *lhs, const TextChange *rhs) { return lhs->span.start < rhs->span.start; });
    for (const auto *change : ordered) {
        if (change->span.start + change->span.length <= insertPos) {
            if (change->span.start == insertPos && change->span.length == 0) {
                continue;
            }
            insertShift += change->newText.length() - change->span.length;
        }
    }
    return ordered;
}

static void AdjustRenameLocFromChanges(const std::vector<const TextChange *> &ordered, bool renameLocIsFinal,
                                       size_t &renameLoc)
{
    if (renameLocIsFinal || ordered.empty()) {
        return;
    }
    size_t shift = 0;
    bool resolved = false;
    for (const auto *change : ordered) {
        if (change->span.start > renameLoc) {
            break;
        }
        if (change->span.start + change->span.length <= renameLoc) {
            shift += change->newText.length() - change->span.length;
            continue;
        }
        renameLoc = change->span.start + shift;
        resolved = true;
        break;
    }
    if (!resolved) {
        renameLoc += shift;
    }
}

static std::string BuildFinalText(std::string_view source, const std::vector<const TextChange *> &ordered)
{
    std::string finalText;
    finalText.reserve(source.size());
    size_t cursor = 0;
    for (const auto *change : ordered) {
        size_t start = std::min(change->span.start, source.size());
        if (start < cursor) {
            start = cursor;
        }
        size_t end = std::min(start + change->span.length, static_cast<size_t>(source.size()));
        if (cursor < start) {
            finalText.append(source.substr(cursor, start - cursor));
        }
        finalText.append(change->newText);
        cursor = end;
    }
    if (cursor < source.size()) {
        finalText.append(source.substr(cursor, source.size() - cursor));
    }
    return finalText;
}

static std::optional<size_t> FindBestRenameLoc(const std::string &finalText, const std::string &uniqueVarName,
                                               size_t renameLoc, const std::optional<TextRange> &insertedRange)
{
    size_t bestPos = std::string::npos;
    size_t bestDist = std::numeric_limits<size_t>::max();
    size_t bestPosAny = std::string::npos;
    size_t bestDistAny = std::numeric_limits<size_t>::max();
    size_t pos = finalText.find(uniqueVarName);
    while (pos != std::string::npos) {
        size_t end = pos + uniqueVarName.size();
        size_t dist = 0;
        if (renameLoc < pos) {
            dist = pos - renameLoc;
        } else if (renameLoc > end) {
            dist = renameLoc - end;
        }
        if (dist < bestDistAny) {
            bestDistAny = dist;
            bestPosAny = pos;
        }
        const bool insideInserted = insertedRange.has_value() && pos >= insertedRange->pos && pos < insertedRange->end;
        if (!insideInserted && dist < bestDist) {
            bestDist = dist;
            bestPos = pos;
        }
        pos = finalText.find(uniqueVarName, pos + 1);
    }
    if (bestPos == std::string::npos) {
        bestPos = bestPosAny;
    }
    if (bestPos == std::string::npos) {
        return std::nullopt;
    }
    size_t renameOffset = (uniqueVarName.size() > 1) ? 1 : 0;
    if (uniqueVarName.rfind("this.", 0) == 0) {
        renameOffset = std::string("this.").size() + 1;
    }
    return bestPos + renameOffset;
}

static void ApplyImplicitPrefix(std::string &finalText, size_t adjustedInsertPos, const std::string &implicitPrefix)
{
    if (implicitPrefix.empty() || adjustedInsertPos > finalText.size()) {
        return;
    }
    if (implicitPrefix[0] == '\n' && adjustedInsertPos < finalText.size() &&
        IsLineBreak(finalText[adjustedInsertPos])) {
        finalText.erase(adjustedInsertPos, 1);
    }
    finalText.insert(adjustedInsertPos, implicitPrefix);
}

static std::optional<TextRange> ComputeInsertedRange(size_t adjustedInsertPos, const TextChange *insertChange,
                                                     const std::string &implicitPrefix)
{
    if (insertChange == nullptr) {
        return std::nullopt;
    }
    size_t insertedStart = adjustedInsertPos + (!implicitPrefix.empty() ? implicitPrefix.size() : 0);
    size_t insertedEnd = insertedStart + insertChange->newText.size();
    return TextRange {insertedStart, insertedEnd};
}

static std::optional<size_t> RecomputeRenameLoc(const std::string &finalText, const std::string &uniqueVarName,
                                                size_t renameLoc, const std::optional<TextRange> &insertedRange)
{
    if (finalText.empty()) {
        return std::nullopt;
    }
    return FindBestRenameLoc(finalText, uniqueVarName, renameLoc, insertedRange);
}

static bool HasSourceNewlineInRange(public_lib::Context *ctx, TextRange range)
{
    return ctx != nullptr && ctx->sourceFile != nullptr && HasNewlineInRange(ctx->sourceFile->source, range);
}

static size_t NormalizeFunctionInsertPos(const RefactorContext &context, public_lib::Context *ctx,
                                         const std::string &actionName)
{
    size_t insertPos = GetVarAndFunctionPosToWriteNode(context, actionName).pos;
    if (ctx != nullptr && ctx->sourceFile != nullptr) {
        insertPos = NormalizeInsertPos(ctx->sourceFile->source, insertPos);
    }
    return insertPos;
}

static std::vector<std::string> BuildFunctionCallArgs(const FunctionExtraction &candidate, bool treatAsStatements,
                                                      const FunctionIOInfo &ioInfo,
                                                      const std::vector<std::string> *capturedArgs)
{
    if (treatAsStatements) {
        return ioInfo.callArgs;
    }
    if (capturedArgs != nullptr) {
        return *capturedArgs;
    }
    std::vector<std::string> callArgs;
    callArgs.reserve(candidate.parameters.size());
    for (auto *param : candidate.parameters) {
        if (param != nullptr && param->Ident() != nullptr) {
            callArgs.emplace_back(IdentifierNameMutf8(param->Ident()));
        }
    }
    return callArgs;
}

static size_t ComputeRenameLocFromEdits(const std::vector<FileTextChanges> &edits, size_t renameLoc)
{
    if (edits.empty() || edits[0].textChanges.empty()) {
        return renameLoc;
    }
    std::vector<const TextChange *> ordered;
    ordered.reserve(edits[0].textChanges.size());
    for (const auto &change : edits[0].textChanges) {
        ordered.push_back(&change);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const TextChange *lhs, const TextChange *rhs) { return lhs->span.start < rhs->span.start; });
    bool renameLocIsFinal = false;
    AdjustRenameLocFromChanges(ordered, renameLocIsFinal, renameLoc);
    return renameLoc;
}

static bool IsAsciiIdentifierChar(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '$';
}

static std::optional<size_t> FindCallCalleeOffset(std::string_view callText)
{
    const size_t parenPos = callText.find('(');
    if (parenPos == std::string::npos || parenPos == 0) {
        return std::nullopt;
    }
    size_t tokenEnd = parenPos;
    while (tokenEnd > 0 && std::isspace(static_cast<unsigned char>(callText[tokenEnd - 1])) != 0) {
        --tokenEnd;
    }
    if (tokenEnd == 0) {
        return std::nullopt;
    }
    size_t tokenStart = tokenEnd;
    while (tokenStart > 0 && IsAsciiIdentifierChar(callText[tokenStart - 1])) {
        --tokenStart;
    }
    if (tokenStart == tokenEnd) {
        return std::nullopt;
    }
    return tokenStart;
}

static size_t CountIdentifierLength(std::string_view text, size_t start)
{
    size_t end = start;
    while (end < text.size() && IsAsciiIdentifierChar(text[end])) {
        ++end;
    }
    return end - start;
}

static std::optional<size_t> ComputeFunctionCallRenameLocFromEdits(const std::vector<FileTextChanges> &edits,
                                                                   TextRange extractionPos)
{
    if (edits.empty() || edits[0].textChanges.empty()) {
        return std::nullopt;
    }
    const size_t extractionLen = extractionPos.end - extractionPos.pos;
    const TextChange *replaceChange = nullptr;
    for (const auto &change : edits[0].textChanges) {
        if (change.span.start == extractionPos.pos && change.span.length == extractionLen) {
            replaceChange = &change;
            break;
        }
    }
    if (replaceChange == nullptr) {
        for (const auto &change : edits[0].textChanges) {
            if (change.span.length == 0) {
                continue;
            }
            if (FindCallCalleeOffset(change.newText).has_value()) {
                replaceChange = &change;
                break;
            }
        }
    }
    if (replaceChange == nullptr) {
        return std::nullopt;
    }
    const auto calleeOffset = FindCallCalleeOffset(replaceChange->newText);
    if (!calleeOffset.has_value()) {
        return std::nullopt;
    }
    const size_t calleeLen = CountIdentifierLength(replaceChange->newText, calleeOffset.value());
    const size_t renameOffset = calleeLen > 1 ? 1 : 0;

    std::vector<const TextChange *> ordered;
    ordered.reserve(edits[0].textChanges.size());
    for (const auto &change : edits[0].textChanges) {
        ordered.push_back(&change);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const TextChange *lhs, const TextChange *rhs) { return lhs->span.start < rhs->span.start; });

    size_t shiftBeforeReplace = 0;
    for (const auto *change : ordered) {
        if (change == replaceChange) {
            break;
        }
        if (change->span.start + change->span.length <= replaceChange->span.start) {
            shiftBeforeReplace += change->newText.length() - change->span.length;
        }
    }

    return replaceChange->span.start + shiftBeforeReplace + calleeOffset.value() + renameOffset;
}

static std::optional<size_t> ComputeRenameLocForExprStmt(ir::AstNode *exprStmt, const std::string &generatedText,
                                                         const std::string &uniqueVarName, size_t insertPos)
{
    if (exprStmt == nullptr) {
        return std::nullopt;
    }
    std::string renameToken = uniqueVarName;
    size_t nameOffset = generatedText.find(renameToken);
    if (nameOffset == std::string::npos && renameToken.rfind("this.", 0) == 0) {
        renameToken = renameToken.substr(std::string("this.").size());
        nameOffset = generatedText.find(renameToken);
    }
    if (nameOffset == std::string::npos) {
        return std::nullopt;
    }
    size_t renameOffset = (renameToken.size() > 1) ? 1 : 0;
    return insertPos + nameOffset + renameOffset;
}

static std::string ResolveVariableTypeAnnotation(public_lib::Context *ctx, const RefactorContext &context,
                                                 ir::AstNode *extractedText)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || extractedText == nullptr) {
        return "";
    }
    auto resolveVarTypeText = [ctx](ir::VariableDeclarator *declarator) -> std::string {
        if (declarator == nullptr || declarator->Id() == nullptr || !declarator->Id()->IsIdentifier()) {
            return "";
        }
        auto *ident = declarator->Id()->AsIdentifier();
        if (auto *typeAnnotation = ident->TypeAnnotation(); typeAnnotation != nullptr) {
            std::string typeText = GetNodeText(ctx, typeAnnotation);
            if (typeText.empty()) {
                typeText = typeAnnotation->ToString();
            }
            if (!typeText.empty()) {
                return typeText;
            }
        }
        auto *checker = ctx->GetChecker() == nullptr ? nullptr : ctx->GetChecker()->AsETSChecker();
        auto type = GetTypeOfSymbolAtLocation(checker, ident);
        return type == nullptr ? "" : type->ToString();
    };

    const auto extractionPos = GetCallPositionOfExtraction(context);
    for (ir::AstNode *current = extractedText; current != nullptr; current = current->Parent()) {
        if (!current->IsVariableDeclarator()) {
            continue;
        }
        auto *declarator = current->AsVariableDeclarator();
        auto *init = declarator->Init();
        if (init == nullptr || init->Start().index != extractionPos.pos || init->End().index != extractionPos.end) {
            continue;
        }
        std::string typeText = resolveVarTypeText(declarator);
        if (!typeText.empty()) {
            return ": " + typeText;
        }
        break;
    }
    if (auto typeText = ExtractVariableDeclaredTypeFromInitializer(ctx->sourceFile->source, extractionPos.pos);
        typeText.has_value()) {
        return ": " + typeText.value();
    }
    return "";
}

static std::pair<std::string, bool> BuildClassConstantPrefix(const std::string &varName, ir::AstNode *startedNode)
{
    std::string prefix;
    bool append = false;
    if (IsNamespaceContext(startedNode)) {
        prefix.append("const ").append(varName).append(" = ");
    } else {
        prefix.append("private readonly ").append(varName.substr(std::string("this.").size())).append(" = ");
        append = true;
    }
    return {std::move(prefix), append};
}

static std::string BuildMultiDeclPrefix(const std::string &varName)
{
    return varName + " = ";
}

static std::string BuildStandardDeclPrefix(const std::string &varName, bool isConstantExtraction,
                                           const std::string &typeAnnotation)
{
    std::string prefix;
    prefix.append(isConstantExtraction ? "const " : "let ").append(varName);
    if (!typeAnnotation.empty()) {
        prefix.append(typeAnnotation);
    }
    prefix.append(" = ");
    return prefix;
}

std::string BuildExtractionDeclaration(const RefactorContext &context, ir::AstNode *extractedText,
                                       const std::string &actionName, const std::string &varName)
{
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    const bool isVariableExtraction = IsVariableExtractionAction(actionName);
    const bool isConstantExtraction = IsConstantExtractionAction(actionName);
    if (!isConstantExtraction && !isVariableExtraction) {
        return "";
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return "";
    }

    std::string_view srcView(ctx->sourceFile->source);
    std::string placeholder = GetConstantString(srcView, extractedText);
    if (placeholder.empty()) {
        return "";
    }
    const std::string extractedVarTypeAnnotation =
        isVariableExtraction ? ResolveVariableTypeAnnotation(ctx, context, extractedText) : "";

    auto startedNode = GetTouchingTokenByRange(context.context, context.span, false);
    const bool isMultiDecl = IsMultiDecl(startedNode, ctx);
    bool isAppend = false;
    std::string declaration;
    if (IsConstantExtractionInClassAction(actionName)) {
        auto prefixResult = BuildClassConstantPrefix(varName, startedNode);
        declaration = std::move(prefixResult.first);
        isAppend = prefixResult.second;
    } else if (isMultiDecl) {
        declaration = BuildMultiDeclPrefix(varName);
    } else {
        declaration = BuildStandardDeclPrefix(varName, isConstantExtraction, extractedVarTypeAnnotation);
    }
    declaration.append(placeholder);
    if (isMultiDecl && declaration.find(',') == std::string::npos) {
        declaration.append(", ");
    } else if (declaration.find(';') == std::string::npos) {
        declaration.append(";");
    }
    if (isAppend) {
        declaration.append(newLine);
    }
    return declaration;
}

void ApplyVariableFormatting(const RefactorContext &context, public_lib::Context *ctx, const std::string &actionName,
                             std::string &declaration)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return;
    }
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    size_t insertPos = GetVarAndFunctionPosToWriteNode(context, actionName).pos;
    std::string insertionIndent = GetIndentAtPosition(ctx, insertPos);
    TextRange callRange = GetCallPositionOfExtraction(context);
    std::string statementIndent = GetIndentAtPosition(ctx, callRange.pos);
    const std::string &indentToUse = statementIndent.empty() ? insertionIndent : statementIndent;

    declaration = newLine + indentToUse + declaration;
    declaration.append(newLine).append(newLine);
}

std::string GenerateInlineEdits(const RefactorContext &context, ir::AstNode *&extractedText,
                                const std::string &actionName, const std::string &varName)
{
    if (extractedText == nullptr) {
        return "";
    }
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (impl == nullptr) {
        return "";
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    extractedText = GetOptimumNodeByRange(extractedText, context.span);
    if (extractedText == nullptr || IsNodeInScope(extractedText) || ctx->sourceFile == nullptr ||
        ctx->sourceFile->source.empty()) {
        return "";
    }

    std::string declaration = BuildExtractionDeclaration(context, extractedText, actionName, varName);
    if (declaration.empty()) {
        return "";
    }
    return declaration;
}

std::vector<ApplicableRefactorInfo> ExtractSymbolRefactor::GetAvailableActions(const RefactorContext &refContext) const
{
    const auto rangeToExtract = refContext.span;
    if (rangeToExtract.pos >= rangeToExtract.end) {
        return {};
    }
    const auto refactoredNodeList = FindAvailableRefactors(refContext);
    if (refactoredNodeList.empty()) {
        return {};
    }

    std::vector<ApplicableRefactorInfo> resList;

    for (const RefactorAction &ref : refactoredNodeList) {
        if (!refContext.kind.empty()) {
            if (refContext.kind != ref.kind) {
                continue;
            }
        }
        ApplicableRefactorInfo res;
        res.name = REFACTOR_NAME;
        res.description = REFACTOR_DESCRIPTION;
        res.action = ref;
        resList.push_back(res);
    }
    return resList;
}

ir::AstNode *IsReplaceRangeRequired(const RefactorContext &context, ir::AstNode *extractedText)
{
    if (extractedText == nullptr) {
        return nullptr;
    }
    ir::AstNode *exprStmt = nullptr;
    for (ir::AstNode *parent = extractedText->Parent(); parent != nullptr; parent = parent->Parent()) {
        if (parent->IsExpressionStatement() &&
            (parent->Start().index <= context.span.pos && parent->End().index >= context.span.end)) {
            exprStmt = parent;
        }
    }
    for (ir::AstNode *parent = extractedText->Parent(); parent != nullptr; parent = parent->Parent()) {
        if (!parent->IsCallExpression()) {
            continue;
        }
        if ((parent->Start().index != context.span.pos && parent->End().index != context.span.end) ||
            exprStmt == nullptr) {
            return nullptr;
        }
    }
    return exprStmt;
}

static std::pair<std::vector<FileTextChanges>, ir::AstNode *> BuildValueExtractionChanges(
    const RefactorContext &context, ir::AstNode *extractedText, const std::pair<size_t, std::string> &insertionData,
    const std::string &uniqueVarName)
{
    TextChangesContext textChangesContext = *context.textChangesContext;
    const auto src = reinterpret_cast<public_lib::Context *>(context.context)->sourceFile;
    TextRange extractedRange {extractedText->Start().index, extractedText->End().index};
    auto *exprStmt = IsReplaceRangeRequired(context, extractedText);
    auto edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(src, insertionData.first, insertionData.second);
        if (exprStmt != nullptr) {
            tracker.DeleteRange(src, TextRange {exprStmt->Start().index, exprStmt->End().index});
            return;
        }
        tracker.ReplaceRangeWithText(src, extractedRange, uniqueVarName);
    });
    return {std::move(edits), exprStmt};
}

static void UpdateRenameLocFromFinalText(const std::string &finalText, const std::string &uniqueVarName,
                                         const std::optional<TextRange> &insertedRange, size_t &renameLoc)
{
    if (auto updatedLoc = RecomputeRenameLoc(finalText, uniqueVarName, renameLoc, insertedRange);
        updatedLoc.has_value()) {
        renameLoc = updatedLoc.value();
    }
}

RefactorEditInfo GetRefactorEditsToExtractVals(const RefactorContext &context, ir::AstNode *extractedText,
                                               const std::string &actionName)
{
    std::string uniqueVarName = GenerateUniqueExtractedVarName(context, actionName);
    std::string generatedText = GenerateInlineEdits(context, extractedText, actionName, uniqueVarName);
    if (generatedText.empty()) {
        return RefactorEditInfo {};
    }
    size_t insertPos = GetVarAndFunctionPosToWriteNode(context, actionName).pos;
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return RefactorEditInfo {};
    }
    const auto src = ctx->sourceFile;
    insertPos = ResolveInsertionPosForVariableExtraction(context, ctx, actionName, insertPos);
    auto inlineInsertionResult = TryBuildInlineInsertion(context, ctx, extractedText, uniqueVarName);
    const bool inlineInsertion = ApplyInlineInsertionResult(insertPos, generatedText, inlineInsertionResult);
    if (!inlineInsertion) {
        AdjustGeneratedTextForInsert(context, ctx, insertPos, uniqueVarName, generatedText);
        AppendTrailingNewLineForGlobalVariableInsert(context, actionName, insertPos, generatedText);
        MaybePrependNamespaceNewlinesForValueExtraction(context, insertPos, generatedText,
                                                        {&actionName, extractedText});
    }
    std::string implicitPrefix = BuildImplicitPrefix(context, ctx, insertPos, generatedText, inlineInsertion);
    auto [edits, exprStmt] =
        BuildValueExtractionChanges(context, extractedText, {insertPos, generatedText}, uniqueVarName);
    size_t renameLoc = extractedText->Start().index;
    bool renameLocIsFinal = false;
    if (auto renameLocForExpr = ComputeRenameLocForExprStmt(exprStmt, generatedText, uniqueVarName, insertPos);
        renameLocForExpr.has_value()) {
        renameLoc = renameLocForExpr.value();
        renameLocIsFinal = true;
    }
    const TextChange *insertChange = nullptr;
    size_t insertShift = 0;
    auto orderedChanges = CollectOrderedTextChanges(edits, insertPos, insertChange, insertShift);
    AdjustRenameLocFromChanges(orderedChanges, renameLocIsFinal, renameLoc);
    const size_t adjustedInsertPos = insertPos + insertShift;
    if (!implicitPrefix.empty() && adjustedInsertPos <= renameLoc) {
        renameLoc += implicitPrefix.size();
    }
    if (!renameLocIsFinal && !orderedChanges.empty()) {
        std::string finalText = BuildFinalText(src->source, orderedChanges);
        ApplyImplicitPrefix(finalText, adjustedInsertPos, implicitPrefix);
        auto insertedRange = adjustedInsertPos <= finalText.size()
                                 ? ComputeInsertedRange(adjustedInsertPos, insertChange, implicitPrefix)
                                 : std::optional<TextRange> {};
        UpdateRenameLocFromFinalText(finalText, uniqueVarName, insertedRange, renameLoc);
    }
    return RefactorEditInfo(std::move(edits), std::optional<std::string>(src->filePath),
                            std::optional<size_t>(renameLoc));
}

static bool TryGetFunctionExtractionCandidate(const RefactorContext &context, FunctionExtraction &candidate)
{
    auto candidates = GetPossibleFunctionExtractions(context);
    if (candidates.empty()) {
        return false;
    }
    candidate = candidates.front();
    CollectFunctionParameters(candidate);
    return true;
}

static std::vector<FileTextChanges> BuildFunctionExtractionTextChanges(const RefactorContext &context,
                                                                       const std::string &functionText,
                                                                       size_t insertPos, TextRange extractionPos,
                                                                       const std::string &funcCallText)
{
    TextChangesContext textChangesContext = *context.textChangesContext;
    const auto src = reinterpret_cast<public_lib::Context *>(context.context)->sourceFile;
    return ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(src, insertPos, functionText);
        tracker.ReplaceRangeWithText(src, extractionPos, funcCallText);
    });
}

static size_t ComputeFunctionRenameLoc(const std::vector<FileTextChanges> &edits, TextRange extractionPos)
{
    size_t renameLoc = ComputeRenameLocFromEdits(edits, extractionPos.pos);
    if (auto renameLocOnCall = ComputeFunctionCallRenameLocFromEdits(edits, extractionPos);
        renameLocOnCall.has_value()) {
        renameLoc = renameLocOnCall.value();
    }
    return renameLoc;
}

RefactorEditInfo GetRefactorEditsToExtractFunction(const RefactorContext &context, const std::string &actionName)
{
    auto *extractedNode = FindExtractedFunction(context);
    if (extractedNode == nullptr) {
        return RefactorEditInfo();
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    TextRange extractionPos = GetCallPositionOfExtraction(context);
    bool selectionHasNewline = HasSourceNewlineInRange(ctx, extractionPos);
    RefactorEditInfo helperEdits;
    if (!selectionHasNewline && TryBuildHelperExtraction(context, extractedNode, actionName, helperEdits)) {
        return helperEdits;
    }
    FunctionExtraction candidate;
    if (!TryGetFunctionExtractionCandidate(context, candidate)) {
        return RefactorEditInfo();
    }

    FunctionIOInfo statementIo;
    FunctionIOInfo expressionIo;
    const FunctionIOInfo *ioInfoPtr = nullptr;
    const std::vector<std::string> *capturedParams = nullptr;
    const std::vector<std::string> *capturedArgs = nullptr;
    const bool treatAsStatements = selectionHasNewline;
    const bool includeNonGlobal = actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    if (treatAsStatements) {
        statementIo = AnalyzeFunctionIO(context, extractionPos, includeNonGlobal);
        ioInfoPtr = &statementIo;
    } else {
        expressionIo = AnalyzeFunctionIO(context, extractionPos, includeNonGlobal);
        if (!expressionIo.callArgs.empty()) {
            capturedParams = &expressionIo.paramDecls;
            capturedArgs = &expressionIo.callArgs;
        }
    }

    size_t insertPos = NormalizeFunctionInsertPos(context, ctx, actionName);

    std::string functionText = BuildFunctionText(candidate, context, actionName, ioInfoPtr, capturedParams);
    std::vector<std::string> callArgs = BuildFunctionCallArgs(candidate, treatAsStatements, statementIo, capturedArgs);
    auto funcCallText = ReplaceWithFunctionCall(
        functionText, callArgs, treatAsStatements ? statementIo.returnVar : std::nullopt, treatAsStatements);
    std::vector<FileTextChanges> edits =
        BuildFunctionExtractionTextChanges(context, functionText, insertPos, extractionPos, funcCallText);
    size_t renameLoc = ComputeFunctionRenameLoc(edits, extractionPos);
    const auto src = reinterpret_cast<public_lib::Context *>(context.context)->sourceFile;
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
