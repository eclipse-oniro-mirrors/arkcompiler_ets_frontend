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
#include <utility>
#include <unordered_map>
#include <unordered_set>
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

constexpr size_t HELPER_RESERVE_PADDING = 64;
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
            !node->IsBinaryExpression() && !node->IsObjectExpression() && !node->IsUpdateExpression());
}

bool IsImportSelectionNode(const ir::AstNode *node)
{
    if (node == nullptr) {
        return false;
    }
    if (node->IsImportDeclaration() || node->IsETSImportDeclaration()) {
        return true;
    }
    const auto *parent = node->Parent();
    return parent != nullptr && (parent->IsImportDeclaration() || parent->IsETSImportDeclaration());
}

bool HasImportDeclarationOverlap(const RefactorContext &context, TextRange range)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return false;
    }
    bool hasImportOverlap = false;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *child) {
        if (child == nullptr || (!child->IsImportDeclaration() && !child->IsETSImportDeclaration())) {
            return false;
        }
        if (child->Start().index < range.end && child->End().index > range.pos) {
            hasImportOverlap = true;
            return true;
        }
        return false;
    });
    return hasImportOverlap;
}

static bool SelectionMatchesNodeWithOptionalTrailingSemicolon(std::string_view source, const ir::AstNode *node,
                                                              TextRange selection)
{
    if (node == nullptr) {
        return false;
    }
    const size_t start = node->Start().index;
    const size_t end = node->End().index;
    if (start != selection.pos) {
        return false;
    }
    if (selection.end == end) {
        return true;
    }
    if (selection.end < end || selection.end > source.size() || end > source.size()) {
        return false;
    }
    for (size_t i = end; i < selection.end; ++i) {
        const char ch = source[i];
        if (ch == ';' || std::isspace(static_cast<unsigned char>(ch)) != 0) {
            continue;
        }
        return false;
    }
    return true;
}

ir::AstNode *FindWholeVariableDeclarationSelectionNode(const RefactorContext &context, TextRange selection)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr ||
        ctx->sourceFile == nullptr) {
        return nullptr;
    }
    ir::AstNode *matchedDecl = nullptr;
    std::string_view source = ctx->sourceFile->source;
    ctx->parserProgram->Ast()->FindChild([selection, &matchedDecl, source](ir::AstNode *node) {
        if (matchedDecl != nullptr || node == nullptr || !node->IsVariableDeclaration()) {
            return false;
        }
        if (!SelectionMatchesNodeWithOptionalTrailingSemicolon(source, node, selection)) {
            return false;
        }
        matchedDecl = node;
        return true;
    });
    return matchedDecl;
}

static std::optional<TextRange> ResolveDeclarationSelectionRhsRange(public_lib::Context *ctx, TextRange selection)
{
    const std::string_view selected(ctx->sourceFile->source.data() + selection.pos, selection.end - selection.pos);
    if (!(selected.rfind("let ", 0) == 0 || selected.rfind("const ", 0) == 0)) {
        return std::nullopt;
    }
    const size_t eqPos = selected.find('=');
    if (eqPos == std::string_view::npos) {
        return std::nullopt;
    }
    size_t rhsStart = eqPos + 1;
    while (rhsStart < selected.size() && std::isspace(static_cast<unsigned char>(selected[rhsStart])) != 0) {
        ++rhsStart;
    }
    size_t rhsEnd = selected.size();
    while (rhsEnd > rhsStart && std::isspace(static_cast<unsigned char>(selected[rhsEnd - 1])) != 0) {
        --rhsEnd;
    }
    while (rhsEnd > rhsStart && selected[rhsEnd - 1] == ';') {
        --rhsEnd;
    }
    while (rhsEnd > rhsStart && std::isspace(static_cast<unsigned char>(selected[rhsEnd - 1])) != 0) {
        --rhsEnd;
    }
    if (rhsEnd <= rhsStart) {
        return std::nullopt;
    }
    return TextRange {selection.pos + rhsStart, selection.pos + rhsEnd};
}

static ir::AstNode *ResolveExpressionNodeByRange(const RefactorContext &context, TextRange rhsRange)
{
    auto *node = GetTouchingTokenByRange(context.context, rhsRange, false);
    if (node != nullptr) {
        if (auto *optimum = GetOptimumNodeByRange(node, rhsRange); optimum != nullptr) {
            node = optimum;
        }
    }
    while (node != nullptr && !node->IsExpression()) {
        node = node->Parent();
    }
    return node;
}

ir::AstNode *ResolveInitializerExpressionFromDeclarationSelection(const RefactorContext &context, TextRange selection)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || selection.end <= selection.pos ||
        selection.end > ctx->sourceFile->source.size()) {
        return nullptr;
    }
    if (auto rhsRange = ResolveDeclarationSelectionRhsRange(ctx, selection); rhsRange.has_value()) {
        return ResolveExpressionNodeByRange(context, rhsRange.value());
    }
    return nullptr;
}

static void UpdateBestInitializerCandidate(ir::AstNode *candidate, TextRange selection, size_t &bestSpan,
                                           ir::AstNode *&best)
{
    if (candidate == nullptr || candidate->Start().index > selection.pos || candidate->End().index < selection.end) {
        return;
    }
    const size_t span = candidate->End().index - candidate->Start().index;
    if (span < bestSpan) {
        best = candidate;
        bestSpan = span;
    }
}

static ir::AstNode *ResolveInitializerFromTouchNode(ir::AstNode *touch, TextRange selection)
{
    for (auto *current = touch; current != nullptr; current = current->Parent()) {
        if (current->IsVariableDeclarator()) {
            auto *declarator = current->AsVariableDeclarator();
            auto *init = declarator == nullptr ? nullptr : declarator->Init();
            if (init != nullptr && init->Start().index <= selection.pos && init->End().index >= selection.end) {
                return init;
            }
            continue;
        }
        if (!current->IsClassProperty()) {
            continue;
        }
        auto *classProp = current->AsClassProperty();
        auto *value = classProp == nullptr ? nullptr : classProp->Value();
        if (value != nullptr && value->Start().index <= selection.pos && value->End().index >= selection.end) {
            return value;
        }
    }
    return nullptr;
}

static ir::AstNode *ResolveInitializerFromTouchPoints(const RefactorContext &context, TextRange selection)
{
    if (auto *touchByRange = GetTouchingTokenByRange(context.context, selection, false); touchByRange != nullptr) {
        if (auto *init = ResolveInitializerFromTouchNode(touchByRange, selection); init != nullptr) {
            return init;
        }
    }
    if (auto *touchAtStart = GetTouchingToken(context.context, selection.pos, false); touchAtStart != nullptr) {
        if (auto *init = ResolveInitializerFromTouchNode(touchAtStart, selection); init != nullptr) {
            return init;
        }
    }
    return nullptr;
}

ir::AstNode *ResolveInitializerExpressionContainingSelection(const RefactorContext &context, TextRange selection)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr ||
        selection.end <= selection.pos) {
        return nullptr;
    }

    ir::AstNode *best = nullptr;
    size_t bestSpan = std::numeric_limits<size_t>::max();
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (node == nullptr) {
            return false;
        }
        if (node->IsVariableDeclarator()) {
            auto *declarator = node->AsVariableDeclarator();
            UpdateBestInitializerCandidate(declarator == nullptr ? nullptr : declarator->Init(), selection, bestSpan,
                                           best);
            return false;
        }
        if (node->IsClassProperty()) {
            auto *classProp = node->AsClassProperty();
            UpdateBestInitializerCandidate(classProp == nullptr ? nullptr : classProp->Value(), selection, bestSpan,
                                           best);
        }
        return false;
    });
    if (best != nullptr) {
        return best;
    }
    return ResolveInitializerFromTouchPoints(context, selection);
}

static void TrimSpanWhitespace(std::string_view source, size_t &start, size_t &end)
{
    while (start < end && std::isspace(static_cast<unsigned char>(source[start])) != 0) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(source[end - 1])) != 0) {
        --end;
    }
}

constexpr size_t K_COMMENT_DELIMITER_LENGTH = 2;

static bool TrimLeadingBlockComment(std::string_view source, size_t &start, size_t &end)
{
    if (start + 1 >= end || source[start] != '/' || source[start + 1] != '*') {
        return false;
    }
    const size_t commentEnd = source.find("*/", start + K_COMMENT_DELIMITER_LENGTH);
    if (commentEnd == std::string::npos || commentEnd + K_COMMENT_DELIMITER_LENGTH > end) {
        return false;
    }
    start = commentEnd + K_COMMENT_DELIMITER_LENGTH;
    TrimSpanWhitespace(source, start, end);
    return true;
}

static bool TrimTrailingBlockComment(std::string_view source, size_t &start, size_t &end)
{
    if (end < K_COMMENT_DELIMITER_LENGTH || end <= start + 1 || source[end - K_COMMENT_DELIMITER_LENGTH] != '*' ||
        source[end - 1] != '/') {
        return false;
    }
    const size_t commentStart = source.rfind("/*", end - K_COMMENT_DELIMITER_LENGTH);
    if (commentStart == std::string::npos || commentStart < start) {
        return false;
    }
    end = commentStart;
    TrimSpanWhitespace(source, start, end);
    return true;
}

static bool TrimSlashWrappedCommentBoundary(std::string_view source, size_t &start, size_t &end)
{
    bool changed = false;
    if (start < end && source[start] == '/' && start > 0 && source[start - 1] == '*') {
        ++start;
        changed = true;
    }
    if (end > start && source[end - 1] == '/' && end < source.size() && source[end] == '*') {
        --end;
        changed = true;
    }
    if (changed) {
        TrimSpanWhitespace(source, start, end);
    }
    return changed;
}

TextRange GetTrimmedSelectionSpan(const RefactorContext &context)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return context.span;
    }

    const auto &source = ctx->sourceFile->source;
    size_t start = std::min(context.span.pos, source.size());
    size_t end = std::min(context.span.end, source.size());
    TrimSpanWhitespace(source, start, end);

    // Allow selections wrapped by comment markers (e.g. /*start*/expr/*end*/)
    // to behave the same as selecting the expression itself.
    bool changed = true;
    while (changed && start < end) {
        changed = false;
        changed = TrimLeadingBlockComment(source, start, end) || changed;
        changed = TrimTrailingBlockComment(source, start, end) || changed;

        // Support selecting "/expr/" across comment boundaries like "*/expr/*".
        // Example: x = /*start*/1/*end*/; with selection "/1/" should normalize to "1".
        changed = TrimSlashWrappedCommentBoundary(source, start, end) || changed;
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
    if (startedNode == nullptr) {
        return {start, end};
    }
    if (startedNode->Start().index < start) {
        start = startedNode->Start().index;
    }
    const auto endedNode = GetTouchingToken(context.context, end - 1, false);
    if (endedNode == nullptr) {
        return {start, end};
    }
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
        if (current->IsClassDefinition()) {
            return current->AsClassDefinition();
        }
        if (current->IsClassDeclaration()) {
            return current->AsClassDeclaration()->Definition();
        }
    }
    return nullptr;
}

bool IsSwitchCaseTestSelection(ir::AstNode *node, TextRange selection)
{
    for (ir::AstNode *current = node; current != nullptr; current = current->Parent()) {
        if (!current->IsSwitchCaseStatement()) {
            continue;
        }
        auto *testExpr = current->AsSwitchCaseStatement()->Test();
        if (testExpr == nullptr) {
            return false;
        }
        return testExpr->Start().index == selection.pos && testExpr->End().index == selection.end;
    }
    return false;
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

std::string IdentifierNameMutf8(const ir::Identifier *ident)
{
    return ident == nullptr ? "" : ident->Name().Mutf8();
}

template <typename ExistsPredicate>
std::string GenerateUniqueName(std::string_view baseName, ExistsPredicate exists)
{
    constexpr int suffixThreshold = 1000000;
    std::string name(baseName);
    int counter = 0;
    while (exists(name) && counter < suffixThreshold) {
        ++counter;
        name = std::string(baseName) + "_" + std::to_string(counter);
    }
    if (!exists(name)) {
        return name;
    }

    while (exists(name)) {
        const auto now = std::chrono::system_clock::now();
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        name = std::string(baseName) + "_" + std::to_string(millis);
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

std::string GetDeclaratorTextWithoutInitializer(public_lib::Context *ctx, const VariableBindingInfo &binding)
{
    if (binding.declarator == nullptr || binding.declarator->Id() == nullptr) {
        return "";
    }
    std::string declText = GetNodeText(ctx, binding.declarator);
    if (declText.empty()) {
        return GetDeclaratorIdText(ctx, binding);
    }
    const size_t eqPos = declText.find('=');
    if (eqPos == std::string::npos) {
        while (!declText.empty() && IsLineBreakChar(declText.back())) {
            declText.pop_back();
        }
        return declText;
    }
    size_t end = eqPos;
    while (end > 0 && std::isspace(static_cast<unsigned char>(declText[end - 1])) != 0) {
        --end;
    }
    return declText.substr(0, end);
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

bool IsMemberObjectIdentifier(const ir::Identifier *ident)
{
    if (ident == nullptr) {
        return false;
    }
    auto *parent = ident->Parent();
    if (parent == nullptr || !parent->IsMemberExpression()) {
        return false;
    }
    auto *member = parent->AsMemberExpression();
    return member->Object() == ident;
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

bool IsConstructorTypeReferenceIdentifier(const ir::Identifier *ident)
{
    if (ident == nullptr) {
        return false;
    }
    bool inTypeRef = false;
    for (auto *parent = ident->Parent(); parent != nullptr; parent = parent->Parent()) {
        if (parent->IsETSTypeReference() || parent->IsETSTypeReferencePart()) {
            inTypeRef = true;
            continue;
        }
        if (inTypeRef && parent->IsETSNewClassInstanceExpression()) {
            return true;
        }
    }
    return false;
}

bool IsTypeReferenceIdentifier(const ir::Identifier *ident)
{
    if (ident == nullptr) {
        return false;
    }
    for (auto *parent = ident->Parent(); parent != nullptr; parent = parent->Parent()) {
        if (parent->IsETSTypeReference() || parent->IsETSTypeReferencePart()) {
            return true;
        }
        if (parent->IsIdentifier()) {
            continue;
        }
        if (parent->IsETSParameterExpression() || parent->IsVariableDeclarator() || parent->IsClassProperty()) {
            continue;
        }
        break;
    }
    return false;
}

bool IsQualifiedTypeReferencePropertyIdentifier(const ir::Identifier *ident)
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

bool IsObjectPropertyKeyIdentifier(const ir::Identifier *ident)
{
    if (ident == nullptr) {
        return false;
    }
    auto *parent = ident->Parent();
    if (parent == nullptr || !parent->IsProperty()) {
        return false;
    }
    auto *property = parent->AsProperty();
    return !property->IsComputed() && property->Key() == ident;
}

std::string BuildNamespaceQualifiedNameForDecl(const ir::AstNode *declNode, std::string_view baseName)
{
    if (declNode == nullptr || baseName.empty()) {
        return "";
    }
    auto scopes = CollectEnclosingNamespaceScopes(const_cast<ir::AstNode *>(declNode));
    if (scopes.empty()) {
        return "";
    }
    std::string qualified;
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        const std::string nsName = IdentifierNameMutf8(*it == nullptr ? nullptr : (*it)->Ident());
        if (nsName.empty()) {
            continue;
        }
        if (!qualified.empty()) {
            qualified.push_back('.');
        }
        qualified.append(nsName);
    }
    if (qualified.empty()) {
        return "";
    }
    const std::string innermostNsName =
        IdentifierNameMutf8(scopes.front() == nullptr ? nullptr : scopes.front()->Ident());
    if (!innermostNsName.empty() && innermostNsName == baseName) {
        return qualified;
    }
    qualified.push_back('.');
    qualified.append(baseName);
    return qualified;
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

static bool IsNamespaceTopLevelDeclNode(const ir::AstNode *declNode);
static bool IsExportedBeforeNamespaceBoundary(const ir::AstNode *declNode);

bool HasLocalValueDependencyInSelection(const RefactorContext &context, TextRange range)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr ||
        range.end <= range.pos) {
        return false;
    }
    bool hasLocal = false;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (hasLocal || node == nullptr || !node->IsIdentifier() || node->Start().index < range.pos ||
            node->End().index > range.end) {
            return false;
        }
        auto *ident = node->AsIdentifier();
        if (IsMemberPropertyIdentifier(ident) || IsDeclarationIdentifier(ident) ||
            IsObjectPropertyKeyIdentifier(ident)) {
            return false;
        }
        auto *variable = ResolveIdentifier(ident);
        if (variable == nullptr || variable->Declaration() == nullptr) {
            return false;
        }
        auto *decl = variable->Declaration();
        auto *declNode = decl->Node();
        if (IsNamespaceTopLevelDeclNode(declNode) && IsExportedBeforeNamespaceBoundary(declNode)) {
            return false;
        }
        if (!IsDeclaredInGlobalScope(decl)) {
            hasLocal = true;
            return true;
        }
        return false;
    });
    return hasLocal;
}

static const ir::Expression *ResolveSelectionWriteTarget(const ir::AstNode *node)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (node->IsAssignmentExpression()) {
        return node->AsAssignmentExpression()->Left();
    }
    if (node->IsUpdateExpression()) {
        return node->AsUpdateExpression()->Argument();
    }
    return nullptr;
}

static bool IsExternalLocalWriteInSelection(const ir::AstNode *node, TextRange range, ir::ScriptFunction *enclosingFunc)
{
    const ir::Expression *target = ResolveSelectionWriteTarget(node);
    if (target == nullptr || !target->IsIdentifier()) {
        return false;
    }

    auto *ident = target->AsIdentifier();
    if (IsMemberPropertyIdentifier(ident) || IsDeclarationIdentifier(ident) || IsObjectPropertyKeyIdentifier(ident)) {
        return false;
    }
    auto *variable = ResolveIdentifier(ident);
    if (variable == nullptr || variable->Declaration() == nullptr || variable->Declaration()->Node() == nullptr ||
        !IsLocalToEnclosingFunction(enclosingFunc, variable)) {
        return false;
    }

    auto *declNode = variable->Declaration()->Node();
    return declNode->Start().index < range.pos || declNode->End().index > range.end;
}

bool HasExternalLocalWriteDependencyInSelection(const RefactorContext &context, TextRange range)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr ||
        range.end <= range.pos) {
        return false;
    }

    auto *touchNode = GetTouchingToken(context.context, range.pos, false);
    auto *enclosingFunc = FindScriptFunction(touchNode);
    if (enclosingFunc == nullptr) {
        return false;
    }

    bool hasExternalLocalWrite = false;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (hasExternalLocalWrite || node == nullptr || node->Start().index < range.pos ||
            node->End().index > range.end) {
            return false;
        }
        hasExternalLocalWrite = IsExternalLocalWriteInSelection(node, range, enclosingFunc);
        return hasExternalLocalWrite;
    });

    return hasExternalLocalWrite;
}

static bool IsNamespaceTopLevelDeclNode(const ir::AstNode *declNode);
static bool IsExportedBeforeNamespaceBoundary(const ir::AstNode *declNode);
static bool IsCallCalleeIdentifier(const ir::Identifier *ident);

struct IdentifierReplacement {
    size_t start;
    size_t end;
    std::string text;
};

static bool IsIdentifierQualifiableTypeRef(const ir::Identifier *ident)
{
    return IsTypeReferenceIdentifier(ident) && !IsQualifiedTypeReferencePropertyIdentifier(ident);
}

static bool IsIdentifierQualifiableValueRef(const ir::Identifier *ident)
{
    return !IsTypeReferenceIdentifier(ident) && !IsMemberPropertyIdentifier(ident) && !IsDeclarationIdentifier(ident) &&
           !IsObjectPropertyKeyIdentifier(ident);
}

static bool IsProtectedGlobalValueRef(bool isValueRef, const std::string &name,
                                      const std::unordered_set<std::string> *protectedValueNames)
{
    return isValueRef && protectedValueNames != nullptr &&
           protectedValueNames->find(name) != protectedValueNames->end();
}

static bool IsExportedNamespaceValueRef(const ir::Identifier *ident, const ir::AstNode *declNode)
{
    if (!IsNamespaceTopLevelDeclNode(declNode) || !IsExportedBeforeNamespaceBoundary(declNode)) {
        return false;
    }
    return IsCallCalleeIdentifier(ident) || IsMemberObjectIdentifier(ident) || !IdentifierNameMutf8(ident).empty();
}

static bool ShouldQualifyGlobalExtractedValueRef(const ir::Identifier *ident, varbinder::Variable *variable,
                                                 const varbinder::Decl *decl, const ir::AstNode *declNode)
{
    auto *enclosingFunc = FindScriptFunction(const_cast<ir::Identifier *>(ident));
    if (IsLocalToEnclosingFunction(enclosingFunc, variable)) {
        return false;
    }
    if (IsDeclaredInGlobalScope(decl) && !IsExportedNamespaceValueRef(ident, declNode)) {
        return false;
    }
    return IsExportedNamespaceValueRef(ident, declNode);
}

static std::vector<IdentifierReplacement> CollectGlobalExtractedBodyReplacements(
    public_lib::Context *ctx, TextRange range, bool qualifyValueRefs,
    const std::unordered_set<std::string> *protectedValueNames)
{
    std::vector<IdentifierReplacement> replacements;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (node == nullptr || !node->IsIdentifier() || node->Start().index < range.pos ||
            node->End().index > range.end) {
            return false;
        }
        auto *ident = node->AsIdentifier();
        const std::string name = IdentifierNameMutf8(ident);
        const bool isTypeRef = IsIdentifierQualifiableTypeRef(ident);
        const bool isValueRef = IsIdentifierQualifiableValueRef(ident);
        if ((!isTypeRef && !isValueRef) || (isValueRef && !qualifyValueRefs) ||
            IsProtectedGlobalValueRef(isValueRef, name, protectedValueNames)) {
            return false;
        }
        auto *variable = ResolveIdentifier(ident);
        auto *decl = variable == nullptr ? nullptr : variable->Declaration();
        auto *declNode = decl == nullptr ? nullptr : decl->Node();
        if (isValueRef && !ShouldQualifyGlobalExtractedValueRef(ident, variable, decl, declNode)) {
            return false;
        }
        const std::string qualified = BuildNamespaceQualifiedNameForDecl(declNode, name);
        if (qualified.empty() || qualified == name) {
            return false;
        }
        replacements.push_back({node->Start().index - range.pos, node->End().index - range.pos, qualified});
        return false;
    });
    return replacements;
}

std::string QualifyTypeReferencesForGlobalExtractedBody(public_lib::Context *ctx, TextRange range, std::string bodyText,
                                                        bool qualifyValueRefs,
                                                        const std::unordered_set<std::string> *protectedValueNames)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr || bodyText.empty() ||
        range.end <= range.pos) {
        return bodyText;
    }
    std::vector<IdentifierReplacement> replacements =
        CollectGlobalExtractedBodyReplacements(ctx, range, qualifyValueRefs, protectedValueNames);
    if (replacements.empty()) {
        return bodyText;
    }
    std::sort(replacements.begin(), replacements.end(),
              [](const IdentifierReplacement &lhs, const IdentifierReplacement &rhs) { return lhs.start < rhs.start; });

    std::string rewritten;
    size_t cursor = 0;
    for (const auto &item : replacements) {
        if (item.start < cursor || item.end > bodyText.size() || item.start >= item.end) {
            continue;
        }
        rewritten.append(bodyText.substr(cursor, item.start - cursor));
        rewritten.append(item.text);
        cursor = item.end;
    }
    rewritten.append(bodyText.substr(cursor));
    return rewritten;
}

bool IsContainedInRange(const ir::AstNode *node, TextRange span)
{
    return node != nullptr && node->Start().index >= span.pos && node->End().index <= span.end;
}

static bool IsNamespaceTopLevelDeclNode(const ir::AstNode *declNode);
static bool IsExportedBeforeNamespaceBoundary(const ir::AstNode *declNode);

static bool IsNamespaceVisibleFromTarget(ir::AstNode *declNode, ir::AstNode *insertAnchorNode)
{
    if (declNode == nullptr || insertAnchorNode == nullptr) {
        return false;
    }
    auto declScopes = CollectEnclosingNamespaceScopes(declNode);
    auto targetScopes = CollectEnclosingNamespaceScopes(insertAnchorNode);
    if (declScopes.empty() || targetScopes.empty()) {
        return false;
    }
    // Symbol declared in an ancestor namespace of target is visible in target namespace.
    if (declScopes.size() > targetScopes.size()) {
        return false;
    }
    const size_t offset = targetScopes.size() - declScopes.size();
    for (size_t i = 0; i < declScopes.size(); ++i) {
        if (declScopes[i] != targetScopes[i + offset]) {
            return false;
        }
    }
    return true;
}

void RecordDeclaredIdentifier(const ir::AstNode *node, std::unordered_set<std::string> &declaredInside)
{
    if (node == nullptr || !node->IsVariableDeclarator()) {
        return;
    }
    auto *decl = node->AsVariableDeclarator();
    if (decl->Id() != nullptr && decl->Id()->IsIdentifier()) {
        declaredInside.insert(IdentifierNameMutf8(decl->Id()->AsIdentifier()));
    }
}

void RecordAssignedIdentifier(const ir::AstNode *node, ir::ScriptFunction *enclosing,
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

static bool IsExactAssignmentTargetIdentifier(const ir::Identifier *ident, TextRange range)
{
    auto *parent = ident == nullptr ? nullptr : ident->Parent();
    if (parent == nullptr || !parent->IsAssignmentExpression()) {
        return false;
    }
    auto *assignment = parent->AsAssignmentExpression();
    return assignment != nullptr && assignment->Left() == ident && assignment->Start().index == range.pos &&
           assignment->End().index == range.end;
}

static bool IsClassQualifiedObjectRef(const ir::Identifier *ident, const varbinder::Decl *decl)
{
    auto *parent = ident == nullptr ? nullptr : ident->Parent();
    if (decl == nullptr || parent == nullptr || !parent->IsMemberExpression()) {
        return false;
    }
    auto *member = parent->AsMemberExpression();
    return !member->IsComputed() && member->Object() == ident && decl->IsClassDecl();
}

static bool CanUseQualifiedCallRef(const ir::Identifier *ident, const ir::AstNode *declNode)
{
    return ident != nullptr && declNode != nullptr && IsCallCalleeIdentifier(ident) &&
           IsNamespaceTopLevelDeclNode(declNode) && IsExportedBeforeNamespaceBoundary(declNode);
}

static bool ShouldSkipUsedIdentifierNode(const ir::Identifier *ident, TextRange range)
{
    return IsExactAssignmentTargetIdentifier(ident, range) || IsMemberPropertyIdentifier(ident) ||
           IsDeclarationIdentifier(ident) || IsConstructorTypeReferenceIdentifier(ident) ||
           IsTypeReferenceIdentifier(ident) || IsObjectPropertyKeyIdentifier(ident);
}

static bool ShouldSkipUsedIdentifierDecl(const ir::Identifier *ident, const varbinder::Variable *variable, bool isLocal,
                                         ir::AstNode *insertAnchorNode, bool preferQualifiedNamespaceRefs)
{
    auto *decl = variable == nullptr ? nullptr : variable->Declaration();
    auto *declNode = decl == nullptr ? nullptr : decl->Node();
    if (insertAnchorNode != nullptr && declNode != nullptr && !isLocal &&
        IsNamespaceVisibleFromTarget(const_cast<ir::AstNode *>(declNode), insertAnchorNode)) {
        return true;
    }
    if (IsClassQualifiedObjectRef(ident, decl) || CanUseQualifiedCallRef(ident, declNode)) {
        return true;
    }
    if (decl != nullptr && decl->IsClassDecl() && IsDeclaredInGlobalScope(decl)) {
        return true;
    }
    const bool isNamespaceTopLevel = declNode != nullptr && IsNamespaceTopLevelDeclNode(declNode);
    const bool isExportedNamespaceValueRef = isNamespaceTopLevel && IsExportedBeforeNamespaceBoundary(declNode);
    return preferQualifiedNamespaceRefs && isExportedNamespaceValueRef;
}

static bool IsNonGlobalUsedIdentifierDecl(bool includeNonGlobal, const varbinder::Decl *decl,
                                          const ir::AstNode *declNode)
{
    const bool isNamespaceTopLevel = declNode != nullptr && IsNamespaceTopLevelDeclNode(declNode);
    const bool isExportedNamespaceValueRef = isNamespaceTopLevel && IsExportedBeforeNamespaceBoundary(declNode);
    return includeNonGlobal &&
           (!IsDeclaredInGlobalScope(decl) || (isNamespaceTopLevel && !isExportedNamespaceValueRef));
}

struct ResolveUsedIdentifierOptions {
    bool includeNonGlobal {false};
    ir::ScriptFunction *enclosingFunc {nullptr};
    const std::unordered_set<std::string> &declaredInside;
    ir::AstNode *insertAnchorNode {nullptr};
    TextRange range;
    bool preferQualifiedNamespaceRefs {false};
};

std::optional<std::pair<std::string, ir::Identifier *>> ResolveUsedIdentifier(
    ir::AstNode *node, const ResolveUsedIdentifierOptions &options)
{
    if (node == nullptr || !node->IsIdentifier()) {
        return std::nullopt;
    }
    auto *ident = node->AsIdentifier();
    if (ShouldSkipUsedIdentifierNode(ident, options.range)) {
        return std::nullopt;
    }
    auto *variable = ResolveIdentifier(ident);
    if (variable == nullptr) {
        return std::nullopt;
    }
    const bool isLocal = IsLocalToEnclosingFunction(options.enclosingFunc, variable);
    auto *decl = variable->Declaration();
    auto *declNode = decl == nullptr ? nullptr : decl->Node();
    if (ShouldSkipUsedIdentifierDecl(ident, variable, isLocal, options.insertAnchorNode,
                                     options.preferQualifiedNamespaceRefs)) {
        return std::nullopt;
    }
    const bool isNonGlobal = IsNonGlobalUsedIdentifierDecl(options.includeNonGlobal, decl, declNode);
    if (!isLocal && !isNonGlobal) {
        return std::nullopt;
    }
    std::string name = IdentifierNameMutf8(ident);
    if (options.declaredInside.find(name) != options.declaredInside.end()) {
        return std::nullopt;
    }
    return std::make_pair(std::move(name), ident);
}

static void RecordParameterTextOverrides(const std::unordered_map<std::string, ir::Identifier *> &firstUse,
                                         FunctionIOInfo &info, public_lib::Context *ctx)
{
    for (size_t i = 0; i < info.callArgs.size() && i < info.paramDecls.size(); ++i) {
        auto it = firstUse.find(info.callArgs[i]);
        if (it == firstUse.end() || it->second == nullptr) {
            continue;
        }
        auto *parent = it->second->Parent();
        if (parent == nullptr || !parent->IsETSParameterExpression()) {
            continue;
        }
        const std::string paramText = GetNodeText(ctx, parent);
        if (!paramText.empty()) {
            info.paramDecls[i] = paramText;
        }
    }
}

template <class Handler>
void CollectFunctionIOUsage(ir::AstNode *ast, TextRange range, Handler &&handler)
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

void FinalizeFunctionIO(FunctionIOInfo &info, const std::unordered_set<std::string> &declaredInside,
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
}

static std::string BuildNamespaceQualifierFromScopes(const std::vector<ir::ClassDefinition *> &namespaceScopes)
{
    std::string qualifier;
    for (auto scopeIt = namespaceScopes.rbegin(); scopeIt != namespaceScopes.rend(); ++scopeIt) {
        const std::string nsName = IdentifierNameMutf8(*scopeIt == nullptr ? nullptr : (*scopeIt)->Ident());
        if (nsName.empty()) {
            continue;
        }
        if (!qualifier.empty()) {
            qualifier.push_back('.');
        }
        qualifier.append(nsName);
    }
    return qualifier;
}

static std::string NormalizeTypeForExtractedParam(std::string typeText)
{
    auto isIdentChar = [](char ch) {
        return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '$';
    };
    auto replaceAllToken = [&typeText, &isIdentChar](std::string_view from, std::string_view to) {
        size_t pos = 0;
        while ((pos = typeText.find(from.data(), pos, from.size())) != std::string::npos) {
            const bool leftOk = pos == 0 || !isIdentChar(typeText[pos - 1]);
            const size_t end = pos + from.size();
            const bool rightOk = end >= typeText.size() || !isIdentChar(typeText[end]);
            if (leftOk && rightOk) {
                typeText.replace(pos, from.size(), to.data(), to.size());
                pos += to.size();
            } else {
                pos = end;
            }
        }
    };
    replaceAllToken("Number", "number");
    replaceAllToken("Double", "number");
    replaceAllToken("Float", "number");
    replaceAllToken("Int", "int");
    replaceAllToken("Long", "long");
    replaceAllToken("Short", "short");
    replaceAllToken("Byte", "byte");
    return typeText;
}

static bool TryAppendOriginalParameterDecl(FunctionIOInfo &info, public_lib::Context *ctx, ir::Identifier *ident)
{
    auto *variable = ident == nullptr ? nullptr : ResolveIdentifier(ident);
    auto *declNode =
        variable != nullptr && variable->Declaration() != nullptr ? variable->Declaration()->Node() : nullptr;
    if (ctx == nullptr || declNode == nullptr || !declNode->IsETSParameterExpression()) {
        return false;
    }
    std::string text = GetNodeText(ctx, declNode);
    if (text.empty()) {
        return false;
    }
    info.paramDecls.push_back(text);
    return true;
}

static std::string QualifyNonGlobalParamTypeIfNeeded(std::string typeText, bool includeNonGlobal, ir::Identifier *ident)
{
    if (!includeNonGlobal || ident == nullptr || typeText != IdentifierNameMutf8(ident)) {
        return typeText;
    }
    auto namespaceScopes = CollectEnclosingNamespaceScopes(ident);
    if (namespaceScopes.empty()) {
        return typeText;
    }
    std::string qualifier = BuildNamespaceQualifierFromScopes(namespaceScopes);
    return qualifier.empty() ? typeText : qualifier + "." + typeText;
}

static std::string ResolveParamDeclTypeText(checker::ETSChecker *checker, bool includeNonGlobal, ir::Identifier *ident)
{
    auto type = GetTypeOfSymbolAtLocation(checker, ident);
    if (type == nullptr) {
        return "";
    }
    std::string typeText = NormalizeTypeForExtractedParam(type->ToString());
    return QualifyNonGlobalParamTypeIfNeeded(std::move(typeText), includeNonGlobal, ident);
}

struct ParamDeclResolveContext {
    const std::unordered_map<std::string, ir::Identifier *> &firstUse;
    checker::ETSChecker *checker {nullptr};
    bool includeNonGlobal {false};
    public_lib::Context *ctx {nullptr};
};

static void AppendParamDeclFromFirstUse(FunctionIOInfo &info, const std::string &name,
                                        const ParamDeclResolveContext &resolveContext)
{
    auto it = resolveContext.firstUse.find(name);
    if (resolveContext.checker == nullptr || it == resolveContext.firstUse.end()) {
        info.paramDecls.push_back(name);
        return;
    }
    if (TryAppendOriginalParameterDecl(info, resolveContext.ctx, it->second)) {
        return;
    }
    std::string typeText =
        ResolveParamDeclTypeText(resolveContext.checker, resolveContext.includeNonGlobal, it->second);
    if (typeText.empty()) {
        info.paramDecls.push_back(name);
        return;
    }
    info.paramDecls.push_back(std::string(name).append(": ").append(typeText));
}

static bool TryAssignReturnTypeFromIdent(FunctionIOInfo &info, public_lib::Context *ctx, ir::Identifier *ident)
{
    if (ctx == nullptr || ident == nullptr || ident->TypeAnnotation() == nullptr) {
        return false;
    }
    std::string text = GetNodeText(ctx, ident->TypeAnnotation());
    if (text.empty()) {
        text = ident->TypeAnnotation()->ToString();
    }
    if (text.empty()) {
        return false;
    }
    info.returnVarTypeAnnotation = ": " + text;
    return true;
}

static void AssignReturnVarTypeAnnotation(FunctionIOInfo &info,
                                          const std::unordered_map<std::string, ir::Identifier *> &firstUse,
                                          checker::ETSChecker *checker, public_lib::Context *ctx)
{
    if (!info.returnVar.has_value()) {
        return;
    }
    auto retIt = firstUse.find(info.returnVar.value());
    if (retIt == firstUse.end() || retIt->second == nullptr) {
        return;
    }
    auto *retIdent = retIt->second;
    if (TryAssignReturnTypeFromIdent(info, ctx, retIdent)) {
        return;
    }
    auto *variable = ResolveIdentifier(retIdent);
    auto *declNode =
        variable != nullptr && variable->Declaration() != nullptr ? variable->Declaration()->Node() : nullptr;
    if (declNode != nullptr && declNode->IsVariableDeclarator()) {
        auto *id = declNode->AsVariableDeclarator()->Id();
        if (id != nullptr && id->IsIdentifier() && TryAssignReturnTypeFromIdent(info, ctx, id->AsIdentifier())) {
            return;
        }
    }
    if (checker != nullptr) {
        auto type = GetTypeOfSymbolAtLocation(checker, retIdent);
        if (type != nullptr) {
            std::string text = type->ToString();
            if (!text.empty()) {
                info.returnVarTypeAnnotation = ": " + text;
            }
        }
    }
}

void BuildParamDecls(FunctionIOInfo &info, const std::unordered_map<std::string, ir::Identifier *> &firstUse,
                     checker::ETSChecker *checker, bool includeNonGlobal, public_lib::Context *ctx)
{
    ParamDeclResolveContext resolveContext {firstUse, checker, includeNonGlobal, ctx};
    for (const auto &name : info.callArgs) {
        AppendParamDeclFromFirstUse(info, name, resolveContext);
    }
    AssignReturnVarTypeAnnotation(info, firstUse, checker, ctx);
}

bool IsIdentifierContinuation(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '$';
}

bool HasUseStaticDirective(std::string_view source)
{
    size_t pos = 0;
    while (pos < source.size() && std::isspace(static_cast<unsigned char>(source[pos])) != 0) {
        ++pos;
    }
    constexpr std::string_view singleQuoteDirective = "'use static'";
    constexpr std::string_view doubleQuoteDirective = "\"use static\"";
    return source.substr(pos, singleQuoteDirective.size()) == singleQuoteDirective ||
           source.substr(pos, doubleQuoteDirective.size()) == doubleQuoteDirective;
}

bool ContainsIdentifierToken(std::string_view text, std::string_view token)
{
    if (token.empty() || text.size() < token.size()) {
        return false;
    }
    size_t pos = text.find(token);
    while (pos != std::string::npos) {
        const bool leftOk = pos == 0 || !IsIdentifierContinuation(text[pos - 1]);
        const size_t end = pos + token.size();
        const bool rightOk = end >= text.size() || !IsIdentifierContinuation(text[end]);
        if (leftOk && rightOk) {
            return true;
        }
        pos = text.find(token, pos + token.size());
    }
    return false;
}

static bool IsNamespaceTopLevelDeclNode(const ir::AstNode *declNode)
{
    if (declNode == nullptr) {
        return false;
    }
    bool hasNamespaceModuleAncestor = false;
    for (auto *current = const_cast<ir::AstNode *>(declNode); current != nullptr; current = current->Parent()) {
        if (current->IsFunctionDeclaration() || current->IsFunctionExpression() ||
            current->IsArrowFunctionExpression() || current->IsScriptFunction()) {
            return false;
        }
        if (current->IsETSModule()) {
            auto *module = current->AsETSModule();
            if (module != nullptr && module->IsNamespace()) {
                hasNamespaceModuleAncestor = true;
            }
        }
        if (current->IsClassDeclaration()) {
            auto *classDef = current->AsClassDeclaration()->Definition();
            if (classDef != nullptr && classDef->IsNamespaceTransformed()) {
                return true;
            }
        }
    }
    return hasNamespaceModuleAncestor;
}

static bool IsExportedBeforeNamespaceBoundary(const ir::AstNode *declNode)
{
    for (auto *current = const_cast<ir::AstNode *>(declNode); current != nullptr; current = current->Parent()) {
        if (util::Helpers::IsExported(current)) {
            return true;
        }
        if (current->IsClassDeclaration()) {
            auto *classDef = current->AsClassDeclaration()->Definition();
            if (classDef != nullptr && classDef->IsNamespaceTransformed()) {
                return false;
            }
        }
    }
    return false;
}

static bool IsUnexportedNamespaceTopLevelDeclNode(const ir::AstNode *declNode)
{
    return IsNamespaceTopLevelDeclNode(declNode) && !IsExportedBeforeNamespaceBoundary(declNode);
}

static bool IsCallCalleeIdentifier(const ir::Identifier *ident)
{
    if (ident == nullptr) {
        return false;
    }
    auto *parent = ident->Parent();
    return parent != nullptr && parent->IsCallExpression() && parent->AsCallExpression()->Callee() == ident;
}

static bool HasUnexportedNamespaceDependencyInSelection(const RefactorContext &context, TextRange range,
                                                        bool includeValueRefs, bool includeTypeRefs)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr ||
        range.pos >= range.end) {
        return false;
    }
    bool hasDependency = false;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (hasDependency || node == nullptr || !node->IsIdentifier() || node->Start().index < range.pos ||
            node->End().index > range.end) {
            return false;
        }
        auto *ident = node->AsIdentifier();
        const bool isTypeRef = IsTypeReferenceIdentifier(ident) && !IsQualifiedTypeReferencePropertyIdentifier(ident);
        const bool isValueRef = !IsTypeReferenceIdentifier(ident) && !IsMemberPropertyIdentifier(ident) &&
                                !IsDeclarationIdentifier(ident) && !IsObjectPropertyKeyIdentifier(ident);
        if ((isTypeRef && !includeTypeRefs) || (isValueRef && !includeValueRefs) || (!isTypeRef && !isValueRef)) {
            return false;
        }
        auto *variable = ResolveIdentifier(ident);
        auto *declNode =
            (variable != nullptr && variable->Declaration() != nullptr) ? variable->Declaration()->Node() : nullptr;
        if (IsUnexportedNamespaceTopLevelDeclNode(declNode)) {
            hasDependency = true;
            return true;
        }
        return false;
    });
    return hasDependency;
}

static bool HasUnexportedNamespaceTopLevelDependencyInSelection(const RefactorContext &context, TextRange range)
{
    return HasUnexportedNamespaceDependencyInSelection(context, range, true, false);
}

bool HasUnexportedNamespaceInterfaceDependencyInSelection(const RefactorContext &context, TextRange range)
{
    return HasUnexportedNamespaceDependencyInSelection(context, range, false, true);
}

bool HasNamespacePrivateSymbolDependencyForGlobalExtraction(const RefactorContext &context, TextRange range)
{
    return HasUnexportedNamespaceTopLevelDependencyInSelection(context, range);
}

std::string TrimAsciiWhitespace(std::string_view s)
{
    size_t begin = 0;
    while (begin < s.size() && std::isspace(static_cast<unsigned char>(s[begin])) != 0) {
        ++begin;
    }
    size_t end = s.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
        --end;
    }
    return std::string(s.substr(begin, end - begin));
}

static std::string NormalizeExtractedReturnTypeText(const std::string &typeText, bool normalizePrimitiveTypes)
{
    if (!normalizePrimitiveTypes) {
        return typeText;
    }
    if (typeText == "Number" || typeText == "Double" || typeText == "Int" || typeText == "Float" ||
        typeText == "Long" || typeText == "Short" || typeText == "Byte") {
        return "number";
    }
    if (typeText == "Boolean") {
        return "boolean";
    }
    if (typeText == "String") {
        return "string";
    }
    return typeText;
}

static std::string BuildTypeAnnotationText(const std::string &typeText, bool normalizePrimitiveTypes)
{
    const std::string normalized = NormalizeExtractedReturnTypeText(typeText, normalizePrimitiveTypes);
    return normalized.empty() ? "" : ": " + normalized;
}

static std::string ResolveTypeAnnotationFromChecker(checker::ETSChecker *checker, ir::AstNode *target,
                                                    bool normalizePrimitiveTypes)
{
    if (checker == nullptr || target == nullptr) {
        return "";
    }
    auto type = GetTypeOfSymbolAtLocation(checker, target);
    return type == nullptr ? "" : BuildTypeAnnotationText(type->ToString(), normalizePrimitiveTypes);
}

static ir::TypeNode *ResolveSelectionOwnerTypeAnnotation(ir::AstNode *node, TextRange range)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (node->IsVariableDeclarator()) {
        auto *id = node->AsVariableDeclarator()->Id();
        auto *init = node->AsVariableDeclarator()->Init();
        if (id != nullptr && id->IsIdentifier() && init != nullptr && init->Start().index <= range.pos &&
            init->End().index >= range.end) {
            return id->AsIdentifier()->TypeAnnotation();
        }
        return nullptr;
    }
    if (!node->IsClassProperty()) {
        return nullptr;
    }
    auto *prop = node->AsClassProperty();
    if (prop->Value() == nullptr || prop->Value()->Start().index > range.pos ||
        prop->Value()->End().index < range.end) {
        return nullptr;
    }
    if (prop->Key() != nullptr && prop->Key()->IsIdentifier()) {
        if (auto *typeAnno = prop->Key()->AsIdentifier()->TypeAnnotation(); typeAnno != nullptr) {
            return typeAnno;
        }
    }
    return prop->TypeAnnotation();
}

static bool HasUnexportedNamespaceTypeReference(ir::TypeNode *typeAnno)
{
    bool hasPrivateTypeDependency = false;
    if (typeAnno == nullptr) {
        return false;
    }
    typeAnno->FindChild([&](ir::AstNode *typeNode) {
        if (hasPrivateTypeDependency || typeNode == nullptr || !typeNode->IsIdentifier()) {
            return false;
        }
        auto *ident = typeNode->AsIdentifier();
        if (!IsTypeReferenceIdentifier(ident) || IsQualifiedTypeReferencePropertyIdentifier(ident)) {
            return false;
        }
        auto *variable = ResolveIdentifier(ident);
        auto *declNode =
            (variable != nullptr && variable->Declaration() != nullptr) ? variable->Declaration()->Node() : nullptr;
        if (IsUnexportedNamespaceTopLevelDeclNode(declNode)) {
            hasPrivateTypeDependency = true;
            return true;
        }
        return false;
    });
    return hasPrivateTypeDependency;
}

bool HasNamespacePrivateTypeAnnotationDependencyForExpression(const RefactorContext &context, TextRange range)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr ||
        range.pos >= range.end) {
        return false;
    }
    bool hasPrivateTypeDependency = false;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (hasPrivateTypeDependency || node == nullptr) {
            return false;
        }
        ir::TypeNode *typeAnno = ResolveSelectionOwnerTypeAnnotation(node, range);
        if (typeAnno == nullptr) {
            return false;
        }
        hasPrivateTypeDependency = HasUnexportedNamespaceTypeReference(typeAnno);
        return hasPrivateTypeDependency;
    });
    return hasPrivateTypeDependency;
}

static void RecordFunctionIOAssignmentFirstUse(ir::AstNode *node,
                                               std::unordered_map<std::string, ir::Identifier *> &firstUse)
{
    if (node == nullptr || !node->IsAssignmentExpression()) {
        return;
    }
    auto *lhs = node->AsAssignmentExpression()->Left();
    if (lhs == nullptr || !lhs->IsIdentifier()) {
        return;
    }
    auto *ident = lhs->AsIdentifier();
    const std::string name = IdentifierNameMutf8(ident);
    if (!name.empty()) {
        firstUse.emplace(name, ident);
    }
}

struct FunctionIONodeUsageContext {
    bool includeNonGlobal {false};
    ir::ScriptFunction *enclosingFunc {nullptr};
    std::unordered_set<std::string> &declaredInside;
    std::unordered_set<std::string> &assignedInside;
    std::unordered_set<std::string> &usedSet;
    std::vector<std::string> &usedOrder;
    std::unordered_map<std::string, ir::Identifier *> &firstUse;
    ir::AstNode *insertAnchorNode {nullptr};
    TextRange range;
    bool preferQualifiedNamespaceRefs {false};
};

static void RecordFunctionIOReturnUsage(FunctionIOInfo &info, ir::AstNode *node, ir::ScriptFunction *enclosingFunc)
{
    if (node->IsReturnStatement() && FindScriptFunction(node) == enclosingFunc) {
        info.hasReturnStatement = true;
    }
}

static void RecordFunctionIOResolvedIdentifierUsage(ir::AstNode *node, FunctionIONodeUsageContext &usageContext)
{
    auto used = ResolveUsedIdentifier(node, {usageContext.includeNonGlobal, usageContext.enclosingFunc,
                                             usageContext.declaredInside, usageContext.insertAnchorNode,
                                             usageContext.range, usageContext.preferQualifiedNamespaceRefs});
    if (used.has_value() && usageContext.usedSet.insert(used->first).second) {
        usageContext.usedOrder.push_back(used->first);
        usageContext.firstUse.emplace(used->first, used->second);
    }
}

static void RecordFunctionIONodeUsage(FunctionIOInfo &info, ir::AstNode *node, FunctionIONodeUsageContext &usageContext)
{
    RecordFunctionIOReturnUsage(info, node, usageContext.enclosingFunc);
    RecordDeclaredIdentifier(node, usageContext.declaredInside);
    RecordAssignedIdentifier(node, usageContext.enclosingFunc, usageContext.assignedInside);
    RecordFunctionIOAssignmentFirstUse(node, usageContext.firstUse);
    RecordFunctionIOResolvedIdentifierUsage(node, usageContext);
}

FunctionIOInfo AnalyzeFunctionIO(const RefactorContext &context, TextRange range, bool includeNonGlobal,
                                 ir::AstNode *insertAnchorNode, bool preferQualifiedNamespaceRefs)
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
    FunctionIONodeUsageContext usageContext {
        includeNonGlobal, enclosingFunc, declaredInside,   assignedInside, usedSet,
        usedOrder,        firstUse,      insertAnchorNode, range,          preferQualifiedNamespaceRefs};
    auto onNode = [&info, &usageContext](ir::AstNode *node) { RecordFunctionIONodeUsage(info, node, usageContext); };
    CollectFunctionIOUsage(ctx->parserProgram->Ast(), range, onNode);
    FinalizeFunctionIO(info, declaredInside, assignedInside, usedOrder);
    BuildParamDecls(info, firstUse, checker, includeNonGlobal, ctx);
    RecordParameterTextOverrides(firstUse, info, ctx);
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
    constexpr size_t DEC_BASE = 10;                                   // CC-OFF(G.NAM.03-CPP) project code style
    constexpr size_t MAX_VALUE = std::numeric_limits<size_t>::max();  // CC-OFF(G.NAM.03-CPP) project code style
    for (char ch : text) {
        if (ch < '0' || ch > '9') {
            return false;
        }
        auto digit = static_cast<size_t>(ch - '0');
        if (parsed > (MAX_VALUE - digit) / DEC_BASE) {
            return false;
        }
        parsed = (parsed * DEC_BASE) + digit;
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

std::string GetInsertionIndent(public_lib::Context *ctx, size_t insertPos);

size_t NormalizeInsertPos(std::string_view source, size_t pos)
{
    size_t adjusted = std::min(pos, source.size());
    while (adjusted < source.size() && IsLineBreakChar(source[adjusted])) {
        ++adjusted;
    }
    return adjusted;
}

void GetLineBounds(std::string_view source, size_t pos, size_t &lineStart, size_t &lineEnd)
{
    if (source.empty()) {
        lineStart = 0;
        lineEnd = 0;
        return;
    }

    size_t safePos = std::min(pos, source.size() - 1);
    lineStart = safePos;
    while (lineStart > 0 && !IsLineBreakChar(source[lineStart - 1])) {
        --lineStart;
    }

    lineEnd = safePos;
    while (lineEnd < source.size() && !IsLineBreakChar(source[lineEnd])) {
        ++lineEnd;
    }
    if (lineEnd >= source.size()) {
        lineEnd = source.size() - 1;
    }
}

bool IsBlankLine(std::string_view source, size_t lineStart, size_t lineEnd)
{
    if (source.empty() || lineStart >= source.size()) {
        return true;
    }
    size_t end = std::min(lineEnd, source.size() - 1);
    for (size_t i = lineStart; i <= end; ++i) {
        char ch = source[i];
        if (IsLineBreakChar(ch)) {
            continue;
        }
        if (ch != ' ' && ch != '\t') {
            return false;
        }
    }
    return true;
}

std::string FormatDeclarationForInsert(public_lib::Context *ctx, size_t insertPos, std::string declaration)
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

bool IsLineStartAtPosition(std::string_view source, size_t pos)
{
    if (pos == 0) {
        return true;
    }
    char prev = source[pos - 1];
    return prev == '\n' || prev == '\r';
}

ir::AstNode *ScanTouchingTokenForward(const RefactorContext &context, std::string_view source)
{
    size_t upper = std::min(context.span.end, source.size());
    for (size_t i = context.span.pos; i < upper; ++i) {
        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
        if (isspace(source[i])) {
            continue;
        }
        if (auto *node = GetTouchingToken(context.context, i, false); node != nullptr) {
            return node;
        }
    }
    return nullptr;
}

ir::AstNode *ScanTouchingTokenBackward(const RefactorContext &context, std::string_view source)
{
    size_t i = std::min(context.span.pos, source.size());
    while (i > 0) {
        --i;
        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
        if (isspace(source[i])) {
            continue;
        }
        if (auto *node = GetTouchingToken(context.context, i, false); node != nullptr) {
            return node;
        }
    }
    return nullptr;
}

ir::AstNode *FindTouchingTokenByScan(const RefactorContext &context, public_lib::Context *ctx)
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

bool IsNamespaceModule(const ir::AstNode *node)
{
    auto *module = node != nullptr ? node->AsETSModule() : nullptr;
    return module != nullptr && module->IsNamespace();
}

bool HasNamespaceModuleAncestor(const ir::AstNode *current)
{
    for (auto *parent = current != nullptr ? current->Parent() : nullptr; parent != nullptr;
         parent = parent->Parent()) {
        if (parent->IsETSModule() && IsNamespaceModule(parent)) {
            return true;
        }
    }
    return false;
}

bool IsInGlobalClassStaticBlock(const ir::AstNode *current)
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

bool IsSyntheticScriptFunctionUnderGlobalClass(const ir::AstNode *node)
{
    auto *parent = node != nullptr ? node->Parent() : nullptr;
    if (parent == nullptr || !parent->IsScriptFunction() || !compiler::HasGlobalClassParent(parent)) {
        return false;
    }
    auto *script = parent->AsScriptFunction();
    return script != nullptr && script->IsSynthetic();
}

bool IsNamespaceModuleParent(const ir::AstNode *node)
{
    auto *parent = node != nullptr ? node->Parent() : nullptr;
    return parent != nullptr && parent->IsETSModule() && IsNamespaceModule(parent);
}

bool IsProgramParent(const ir::AstNode *node)
{
    auto *parent = node != nullptr ? node->Parent() : nullptr;
    return parent != nullptr && parent->IsProgram();
}

bool ShouldIndentBlockStatement(const ir::AstNode *node)
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

bool ShouldIndentClassDefinition(const ir::AstNode *node)
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

bool IsIndentScopeNode(const ir::AstNode *node)
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

bool HasSelectionNewline(const RefactorContext &context, std::string_view source)
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

ir::AstNode *GetNodeForSpan(const RefactorContext &context)
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

static ir::AstNode *ResolveOptimumNodeForSelection(ir::AstNode *node, TextRange normalizedSpan)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (auto *optimum = GetOptimumNodeByRange(node, normalizedSpan); optimum != nullptr) {
        return optimum;
    }
    return node;
}

static bool ShouldPreferTouchingTokenForExpression(ir::AstNode *node, TextRange normalizedSpan)
{
    if (node == nullptr) {
        return false;
    }
    const bool nodeStrictlyContainsSpan =
        node->Start().index < normalizedSpan.pos || node->End().index > normalizedSpan.end;
    const bool selectionLooksLikeExpression =
        normalizedSpan.end > normalizedSpan.pos && (normalizedSpan.end - normalizedSpan.pos) <= 64;
    return nodeStrictlyContainsSpan && selectionLooksLikeExpression && !node->IsExpression() &&
           !node->IsBinaryExpression();
}

static ir::AstNode *ResolveTouchingTokenForSelection(const RefactorContext &context, TextRange normalizedSpan)
{
    auto *touchNode = GetTouchingToken(context.context, normalizedSpan.pos, false);
    return ResolveOptimumNodeForSelection(touchNode, normalizedSpan);
}

static ir::AstNode *NormalizeNodeToCoverSpan(ir::AstNode *node, TextRange normalizedSpan)
{
    while (node != nullptr && (node->Start().index > normalizedSpan.pos || node->End().index < normalizedSpan.end)) {
        node = node->Parent();
    }
    return node;
}

static ir::AstNode *TryResolveInitializerBySelection(ir::AstNode *seed, TextRange normalizedSpan)
{
    VariableBindingInfo binding;
    for (auto *current = seed; current != nullptr; current = current->Parent()) {
        if (!ResolveVariableBinding(current, binding) || binding.initializer == nullptr) {
            continue;
        }
        if (normalizedSpan.pos >= binding.initializer->Start().index &&
            normalizedSpan.end <= binding.initializer->End().index) {
            return binding.initializer;
        }
    }
    return nullptr;
}

ir::AstNode *ResolveNodeForSelection(const RefactorContext &context, public_lib::Context *ctx, bool selectionHasNewline,
                                     TextRange normalizedSpan)
{
    auto *node = GetNodeForSpan(context);
    if (node != nullptr) {
        node = NormalizeNodeToCoverSpan(node, normalizedSpan);
        if (auto *initializer = TryResolveInitializerBySelection(node, normalizedSpan); initializer != nullptr) {
            return initializer;
        }
        if (ShouldPreferTouchingTokenForExpression(node, normalizedSpan)) {
            if (auto *touchNode = ResolveTouchingTokenForSelection(context, normalizedSpan); touchNode != nullptr) {
                return touchNode;
            }
        }
        return node;
    }
    node = ResolveOptimumNodeForSelection(FindTouchingTokenByScan(context, ctx), normalizedSpan);
    if (node != nullptr) {
        node = NormalizeNodeToCoverSpan(node, normalizedSpan);
        return node;
    }
    if (!selectionHasNewline) {
        // For single-line selections, token probe can still fail for exact-span cases
        // (e.g. statement selections ending with semicolon). Fall back to touching token.
        return ResolveTouchingTokenForSelection(context, normalizedSpan);
    }
    return node;
}

bool IsStatementSelectionCandidate(const ir::AstNode *node)
{
    return node != nullptr && (node->IsStatement() || node->IsExpressionStatement() || node->IsVariableDeclaration()) &&
           !node->IsBlockStatement();
}

bool IsSelectionSuffixSkippable(std::string_view source, size_t start, size_t end)
{
    if (start > end || end > source.size()) {
        return false;
    }
    for (size_t i = start; i < end; ++i) {
        char ch = source[i];
        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
        if (ch != ';' && !isspace(ch)) {
            return false;
        }
    }
    return true;
}

struct StatementSelectionScanResult {
    std::vector<ir::AstNode *> containedStatements;
    bool hasPartialOverlap {false};
};

StatementSelectionScanResult ScanStatementSelectionCandidates(ir::AstNode *ast, TextRange span)
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

bool HasContainedStatementAncestorInSpan(const ir::AstNode *statement, TextRange span)
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

std::vector<ir::AstNode *> CollectTopLevelContainedStatements(const std::vector<ir::AstNode *> &containedStatements,
                                                              TextRange span)
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

void SortStatementsBySourceOrder(std::vector<ir::AstNode *> &statements)
{
    std::stable_sort(statements.begin(), statements.end(), [](const ir::AstNode *lhs, const ir::AstNode *rhs) {
        if (lhs->Start().index != rhs->Start().index) {
            return lhs->Start().index < rhs->Start().index;
        }
        return lhs->End().index < rhs->End().index;
    });
}

bool ValidateStatementSelectionBoundaries(public_lib::Context *ctx, TextRange span, const ir::AstNode *first,
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

bool AreTopLevelStatementsContinuousSiblings(const std::vector<ir::AstNode *> &topLevelStatements)
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

ir::AstNode *FindStatementOverlappingSelection(public_lib::Context *ctx, TextRange span)
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

ir::AstNode *FindBackwardNonWhitespaceToken(const RefactorContext &context, size_t pos)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return nullptr;
    }
    const auto &source = ctx->sourceFile->source;
    size_t probe = std::min(pos, source.size());
    while (probe > 0) {
        --probe;
        if (isspace(source[probe])) {
            continue;
        }
        return GetTouchingToken(context.context, probe, false);
    }
    return nullptr;
}

ir::AstNode *ResolveScopeDepthProbeNode(const RefactorContext &context, size_t pos)
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

size_t CountIndentScopeDepth(const ir::AstNode *node)
{
    size_t depth = 0;
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (IsIndentScopeNode(current)) {
            ++depth;
        }
    }
    return depth;
}

std::string GetInsertionIndent(public_lib::Context *ctx, size_t insertPos)
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
        if (found || node == nullptr) {
            return false;
        }
        if (node->IsFunctionDeclaration()) {
            auto *decl = node->AsFunctionDeclaration();
            auto *func = decl->Function();
            if (func != nullptr && func->Id() != nullptr && IdentifierNameMutf8(func->Id()) == name) {
                found = true;
                return true;
            }
            return false;
        }
        if (!node->IsScriptFunction()) {
            return false;
        }
        auto *func = node->AsScriptFunction();
        if (func == nullptr || func->Id() == nullptr) {
            return false;
        }
        if (!compiler::HasGlobalClassParent(node)) {
            return false;
        }
        if (IdentifierNameMutf8(func->Id()) == name) {
            found = true;
            return true;
        }
        return false;
    });
    return found;
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

bool ScopeHasName(ir::AstNode *scopeNode, const std::string &name)
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

static size_t ScanDirectivePrologueEnd(std::string_view src)
{
    size_t offset = 0;
    size_t lastDirectiveEnd = 0;
    while (offset < src.size()) {
        if (src[offset] == '\n' || src[offset] == '\r') {
            ++offset;
            continue;
        }
        size_t lineStart = offset;
        while (lineStart < src.size() && IsIndentChar(src[lineStart])) {
            ++lineStart;
        }
        if (lineStart >= src.size()) {
            break;
        }
        if (src[lineStart] == '\n' || src[lineStart] == '\r') {
            offset = lineStart + 1;
            continue;
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
            return src.size();
        }
        lastDirectiveEnd = newline + 1;
        offset = newline + 1;
    }
    return lastDirectiveEnd;
}

static size_t ExtendInsertPosPastLeadingTypeDecls(public_lib::Context *ctx, std::string_view src, size_t basePos)
{
    if (ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return basePos;
    }
    size_t insertPos = basePos;
    for (auto *stmt : ctx->parserProgram->Ast()->Statements()) {
        if (stmt == nullptr || stmt->Start().index < insertPos) {
            continue;
        }
        if (!stmt->IsTSInterfaceDeclaration() && !stmt->IsTSTypeAliasDeclaration()) {
            break;
        }
        insertPos = ExtendToLineEnd(util::StringView(src), stmt->End().index);
    }
    return insertPos;
}

size_t DetermineGlobalInsertPos(public_lib::Context *ctx)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return 0;
    }
    const auto &src = ctx->sourceFile->source;
    const size_t directiveEnd = ScanDirectivePrologueEnd(src);
    return ExtendInsertPosPastLeadingTypeDecls(ctx, src, directiveEnd);
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
    std::string declaratorText = GetDeclaratorTextWithoutInitializer(ctx, binding);
    if (declaratorText.empty()) {
        declaratorText = GetDeclaratorIdText(ctx, binding);
    }
    line.reserve(indent.size() + keyword.size() + declaratorText.size() + callExpr.size());
    if (needsLeadingBlank) {
        line.append(newLine);
    }

    line.append(indent);
    line.append(keyword);
    line.append(" ");
    line.append(declaratorText);
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
    FunctionIOInfo ioInfo = AnalyzeFunctionIO(context, initializerRange, includeNonGlobal, nullptr, false);
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

size_t ResolveIndentSize(const RefactorContext &context)
{
    const size_t globalDefaultIndentSize = FormatCodeSettings().GetIndentSize();
    if (context.textChangesContext == nullptr) {
        return globalDefaultIndentSize;
    }
    size_t indentSize = context.textChangesContext->formatContext.GetFormatCodeSettings().GetIndentSize();
    return indentSize == 0 ? globalDefaultIndentSize : indentSize;
}

std::string ResolveReturnTypeAnnotationForBinding(const RefactorContext &context, const VariableBindingInfo &binding,
                                                  bool normalizePrimitiveTypes, bool includeLiteralFallback)
{
    auto *pubCtx = reinterpret_cast<public_lib::Context *>(context.context);

    if (pubCtx != nullptr && binding.identifier != nullptr && binding.identifier->TypeAnnotation() != nullptr) {
        std::string annotated = GetNodeText(pubCtx, binding.identifier->TypeAnnotation());
        if (annotated.empty()) {
            annotated = binding.identifier->TypeAnnotation()->ToString();
        }
        if (std::string typeAnnotation = BuildTypeAnnotationText(annotated, normalizePrimitiveTypes);
            !typeAnnotation.empty()) {
            return typeAnnotation;
        }
    }

    auto *checker =
        pubCtx == nullptr || pubCtx->GetChecker() == nullptr ? nullptr : pubCtx->GetChecker()->AsETSChecker();
    if (std::string typeAnnotation =
            ResolveTypeAnnotationFromChecker(checker, binding.identifier, normalizePrimitiveTypes);
        !typeAnnotation.empty()) {
        return typeAnnotation;
    }
    if (std::string typeAnnotation =
            ResolveTypeAnnotationFromChecker(checker, binding.initializer, normalizePrimitiveTypes);
        !typeAnnotation.empty()) {
        return typeAnnotation;
    }

    if (includeLiteralFallback && binding.initializer != nullptr) {
        if (binding.initializer->IsNumberLiteral()) {
            return ": int";
        }
        if (binding.initializer->IsStringLiteral()) {
            return ": String";
        }
        if (binding.initializer->IsBooleanLiteral()) {
            return ": boolean";
        }
    }
    return "";
}

struct GlobalHelperBodyParts {
    std::string_view newLine;
    std::string_view helperName;
    std::string_view returnTypeAnnotation;
    std::string_view indentStep;
    std::string_view varName;
    std::string_view body;
};

static std::string BuildGlobalHelperFromSelectionBody(const GlobalHelperBodyParts &parts)
{
    std::string helper;
    helper.reserve(parts.body.size() + parts.helperName.size() + parts.varName.size() + K_HELPER_RESERVE_PADDING);
    helper.append(parts.newLine);
    helper.append("function ")
        .append(parts.helperName)
        .append("()")
        .append(parts.returnTypeAnnotation)
        .append(" {")
        .append(parts.newLine);
    std::istringstream lines {std::string(parts.body)};
    std::string line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        helper.append(parts.indentStep).append(line).append(parts.newLine);
    }
    helper.append(parts.indentStep).append("return ").append(parts.varName).append(";").append(parts.newLine);
    helper.append("}").append(parts.newLine);
    return helper;
}

std::string InferHelperReturnTypeAnnotationFromBinding(const RefactorContext &context,
                                                       const VariableBindingInfo &binding,
                                                       [[maybe_unused]] std::string_view paramsSig)
{
    return ResolveReturnTypeAnnotationForBinding(context, binding, true, false);
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

    auto *enclosingClass = FindEnclosingClassDefinition(binding.declaration);
    if (enclosingClass != nullptr && enclosingClass->IsGlobal()) {
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
        const std::string returnTypeAnnotation =
            InferHelperReturnTypeAnnotationFromBinding(context, binding, paramsSig);
        std::string helper;
        helper.reserve(paramsSig.size() + initBody.size() + HELPER_RESERVE_PADDING);
        helper.append(newLine);
        helper.append("function ")
            .append(helperName)
            .append("(")
            .append(paramsSig)
            .append(")")
            .append(returnTypeAnnotation)
            .append(" {")
            .append(newLine);
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

bool BuildGlobalPiecesFromDeclarationSelection(const RefactorContext &context, const VariableBindingInfo &binding,
                                               TextRange selectionSpan, HelperPieces &out)
{
    auto *pubCtx = reinterpret_cast<public_lib::Context *>(context.context);
    if (pubCtx == nullptr || pubCtx->sourceFile == nullptr || binding.declaration == nullptr ||
        binding.identifier == nullptr || selectionSpan.end <= selectionSpan.pos ||
        selectionSpan.end > pubCtx->sourceFile->source.size()) {
        return false;
    }
    // Only handle module-top declaration-leading multi-statement selection.
    if (IsNamespaceContext(binding.declaration)) {
        return false;
    }
    auto *enclosingClass = FindEnclosingClassDefinition(binding.declaration);
    if (enclosingClass != nullptr && !enclosingClass->IsGlobal()) {
        return false;
    }

    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    std::string helperName =
        GenerateUniqueFuncName(context, "newFunction", std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name));
    const auto &source = pubCtx->sourceFile->source;
    auto [lineStart, indentEnd] = ComputeLineIndent(util::StringView(source), binding.declaration->Start().index);
    std::string indent(source.substr(lineStart, indentEnd - lineStart));
    const std::string varName = IdentifierNameMutf8(binding.identifier);
    if (varName.empty()) {
        return false;
    }

    std::string replacement = BuildAssignmentLine(pubCtx, binding, indent, std::string(helperName) + "()", newLine);

    out.insertHelper = false;
    if (!ProgramHasFunction(pubCtx, std::string(helperName))) {
        std::string body(source.substr(selectionSpan.pos, selectionSpan.end - selectionSpan.pos));
        TrimTrailingNewlines(body);
        const std::string indentStep(ResolveIndentSize(context), SPACE_CHAR);
        const std::string returnTypeAnnotation = InferHelperReturnTypeAnnotationFromBinding(context, binding, "");
        out.insertHelper = true;
        out.insertPos = DetermineGlobalInsertPos(pubCtx);
        out.helperText =
            BuildGlobalHelperFromSelectionBody({newLine, helperName, returnTypeAnnotation, indentStep, varName, body});
    }

    out.replacementText = std::move(replacement);
    out.replaceRange = {lineStart, ExtendToLineEnd(util::StringView(source), selectionSpan.end)};
    return true;
}

size_t FindClassHelperInsertPos(public_lib::Context *ctx, ir::ClassDefinition *classDef)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || classDef == nullptr) {
        return 0;
    }

    const auto &source = ctx->sourceFile->source;
    size_t pos = std::min(classDef->End().index, source.size());
    while (pos > 0 && isspace(source[pos - 1])) {
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

std::string ResolveClassIndent(std::string_view methodIndent, size_t indentSize)
{
    if (methodIndent.size() < indentSize) {
        return "";
    }
    return std::string(methodIndent.substr(0, methodIndent.size() - indentSize));
}

struct ClassHelperSignatureParts {
    std::string_view classIndent;
    std::string_view helperName;
    std::string_view paramsSig;
    std::string_view returnTypeAnnotation;
    std::string_view newLine;
};

void AppendClassHelperSignature(std::string &helper, const ClassHelperSignatureParts &parts)
{
    helper.append(parts.newLine);
    helper.append(parts.classIndent)
        .append("private ")
        .append(parts.helperName)
        .append("(")
        .append(parts.paramsSig)
        .append(")")
        .append(parts.returnTypeAnnotation)
        .append(" {")
        .append(parts.newLine);
}

void AppendClassHelperBodyLine(std::string &helper, std::string_view classIndent, std::string_view indentStep,
                               std::string_view body, std::string_view newLine)
{
    helper.append(classIndent).append(indentStep).append(body).append(newLine);
}

void AppendClassHelperReturnLine(std::string &helper, std::string_view classIndent, std::string_view indentStep,
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
    const std::string returnTypeAnnotation = InferHelperReturnTypeAnnotationFromBinding(context, binding, paramsSig);

    std::string helper;
    helper.reserve(body.size() + paramsSig.size() + HELPER_RESERVE_PADDING);
    AppendClassHelperSignature(helper, {classIndent, helperName, paramsSig, returnTypeAnnotation, newLine});
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

static bool TryResolveBindingFromSelectionTokens(const RefactorContext &context, TextRange trimmedSpan,
                                                 VariableBindingInfo &binding)
{
    auto tryResolveFromToken = [&context, &binding](size_t pos) -> bool {
        auto *touch = GetTouchingToken(context.context, pos, false);
        for (auto *current = touch; current != nullptr; current = current->Parent()) {
            if (ResolveVariableBinding(current, binding)) {
                return true;
            }
        }
        return false;
    };
    const bool resolvedFromStart = tryResolveFromToken(trimmedSpan.pos);
    if (resolvedFromStart) {
        return true;
    }
    return trimmedSpan.end > trimmedSpan.pos && tryResolveFromToken(trimmedSpan.end - 1);
}

static bool SelectionMatchesNodeWithTrailingSemicolon(const public_lib::Context *pubCtx, const ir::AstNode *node,
                                                      TextRange trimmedSpan)
{
    if (node == nullptr || pubCtx == nullptr || pubCtx->sourceFile == nullptr) {
        return false;
    }
    const size_t start = node->Start().index;
    const size_t end = node->End().index;
    if (start != trimmedSpan.pos || end < trimmedSpan.end) {
        return false;
    }
    if (end == trimmedSpan.end) {
        return true;
    }
    const auto &source = pubCtx->sourceFile->source;
    if (trimmedSpan.end >= source.size() || end > source.size()) {
        return false;
    }
    bool hasSemicolon = false;
    for (size_t i = trimmedSpan.end; i < end; ++i) {
        const char ch = source[i];
        if (ch == ';') {
            hasSemicolon = true;
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            continue;
        }
        return false;
    }
    return hasSemicolon;
}

static bool TryResolveDeclarationLeadingSelectionForGlobal(public_lib::Context *pubCtx, TextRange trimmedSpan,
                                                           VariableBindingInfo &binding)
{
    bool declarationLeadingSelection = binding.declaration != nullptr &&
                                       binding.declaration->Start().index == trimmedSpan.pos &&
                                       trimmedSpan.end > binding.declaration->End().index;
    if (declarationLeadingSelection || pubCtx == nullptr || pubCtx->parserProgram == nullptr ||
        pubCtx->parserProgram->Ast() == nullptr) {
        return declarationLeadingSelection;
    }
    pubCtx->parserProgram->Ast()->FindChild([&declarationLeadingSelection, &binding, trimmedSpan](ir::AstNode *node) {
        if (declarationLeadingSelection || node == nullptr || !node->IsVariableDeclaration()) {
            return false;
        }
        VariableBindingInfo astBinding;
        if (!ResolveVariableBinding(node, astBinding) || astBinding.declaration == nullptr ||
            astBinding.initializer == nullptr || astBinding.identifier == nullptr) {
            return false;
        }
        if (astBinding.declaration->Start().index != trimmedSpan.pos ||
            trimmedSpan.end <= astBinding.declaration->End().index) {
            return false;
        }
        binding = astBinding;
        declarationLeadingSelection = true;
        return true;
    });
    return declarationLeadingSelection;
}

struct HelperBuildActionInputs {
    const RefactorContext &context;
    const std::string &actionName;
    const VariableBindingInfo &binding;
    TextRange trimmedSpan {};
    HelperPieces &pieces;
    bool declarationLeadingSelection {false};
};

static bool BuildHelperPiecesForAction(const HelperBuildActionInputs &inputs)
{
    if (inputs.actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name)) {
        if (inputs.declarationLeadingSelection) {
            return BuildGlobalPiecesFromDeclarationSelection(inputs.context, inputs.binding, inputs.trimmedSpan,
                                                             inputs.pieces);
        }
        return BuildGlobalPieces(inputs.context, inputs.binding, inputs.pieces);
    }
    if (inputs.actionName == std::string(EXTRACT_FUNCTION_ACTION_CLASS.name)) {
        return BuildClassPieces(inputs.context, inputs.binding, inputs.pieces);
    }
    return false;
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
        const TextRange trimmedSpan = GetTrimmedSelectionSpan(context);
        if (!TryResolveBindingFromSelectionTokens(context, trimmedSpan, binding)) {
            return false;
        }
    }
    // Helper extraction is valid for variable declaration extraction or direct initializer extraction.
    if (binding.initializer == nullptr || extractedNode == nullptr) {
        return false;
    }
    const TextRange trimmedSpan = GetTrimmedSelectionSpan(context);
    const bool initializerSelected =
        SelectionMatchesNodeWithTrailingSemicolon(pubCtx, binding.initializer, trimmedSpan);
    const bool declarationSelected =
        SelectionMatchesNodeWithTrailingSemicolon(pubCtx, binding.declaration, trimmedSpan);
    bool declarationLeadingSelection = false;
    if (actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name)) {
        declarationLeadingSelection = TryResolveDeclarationLeadingSelectionForGlobal(pubCtx, trimmedSpan, binding);
    }
    if (!initializerSelected && !declarationSelected && !declarationLeadingSelection) {
        return false;
    }

    HelperPieces pieces;
    if (!BuildHelperPiecesForAction({context, actionName, binding, trimmedSpan, pieces, declarationLeadingSelection})) {
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

bool IsMultiDecl(ir::AstNode *node, public_lib::Context *context)
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
        return tail < source.size() && isspace(source[tail]);
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
    if (afterKeyword >= source.size() || !isspace(source[afterKeyword])) {
        return false;
    }
    cursor = afterKeyword;
    return true;
}

size_t SkipWhitespace(std::string_view source, size_t cursor, size_t limit)
{
    while (cursor < limit && isspace(source[cursor])) {
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
    while (eqPos > lineStart && isspace(source[eqPos - 1])) {
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
    while (start < typeText.size() && isspace(typeText[start])) {
        ++start;
    }
    size_t end = typeText.size();
    while (end > start && isspace(typeText[end - 1])) {
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

std::optional<std::string> ExtractClassPropertyDeclaredTypeFromInitializer(std::string_view source,
                                                                           size_t initializerStart)
{
    if (initializerStart == 0 || initializerStart > source.size()) {
        return std::nullopt;
    }
    const size_t lineStart = FindLineStart(source, initializerStart);
    size_t cursor = lineStart;
    while (cursor < initializerStart && IsIndentChar(source[cursor])) {
        ++cursor;
    }

    constexpr std::array<std::string_view, 5> modifiers = {"public", "private", "protected", "readonly", "static"};
    bool consumedModifier = true;
    while (consumedModifier) {
        consumedModifier = false;
        for (auto modifier : modifiers) {
            size_t before = cursor;
            if (ConsumeKeyword(source, cursor, modifier)) {
                cursor = SkipWhitespace(source, cursor, initializerStart);
                consumedModifier = true;
                break;
            }
            cursor = before;
        }
    }

    cursor = SkipIdentifier(source, cursor, initializerStart);
    if (cursor < initializerStart && source[cursor] == '?') {
        ++cursor;
    }
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
    constexpr size_t BLOCK_COMMENT_SUFFIX_LEN = 2;
    constexpr size_t BLOCK_COMMENT_STAR_OFFSET = 2;
    constexpr size_t BLOCK_COMMENT_SLASH_OFFSET = 1;
    if (scan < BLOCK_COMMENT_SUFFIX_LEN || source[scan - BLOCK_COMMENT_STAR_OFFSET] != '*' ||
        source[scan - BLOCK_COMMENT_SLASH_OFFSET] != '/') {
        return std::nullopt;
    }
    const size_t commentStart = source.rfind("/*", scan - BLOCK_COMMENT_SUFFIX_LEN);
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
               namespaceDepth.has_value()) {  // CC-OFF(G.FMT.02-CPP) project code style
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
    auto overlaps = [range](ir::AstNode *candidate) {
        return candidate->Start().index < range.end && candidate->End().index > range.pos;
    };
    auto *exprNode =
        originNode->FindChild([&overlaps](ir::AstNode *child) { return child->IsExpression() && overlaps(child); });
    while (exprNode != nullptr) {
        auto *nested =
            exprNode->FindChild([&overlaps](ir::AstNode *child) { return child->IsExpression() && overlaps(child); });
        if (nested == nullptr) {
            break;
        }
        exprNode = nested;
    }
    return exprNode;
}

static ir::AstNode *FindExactExpressionByRange(public_lib::Context *ctx, TextRange rangeToExtract)
{
    ir::AstNode *exactExpression = nullptr;
    ctx->parserProgram->Ast()->FindChild([rangeToExtract, &exactExpression](ir::AstNode *child) {
        if (exactExpression != nullptr || child == nullptr || !child->IsExpression()) {
            return false;
        }
        if (child->Start().index == rangeToExtract.pos && child->End().index == rangeToExtract.end) {
            exactExpression = child;
            return true;
        }
        return false;
    });
    return exactExpression;
}

static ir::AstNode *LiftNodeToCoverRange(ir::AstNode *candidate, TextRange rangeToExtract)
{
    while (candidate != nullptr &&
           (candidate->Start().index > rangeToExtract.pos || candidate->End().index < rangeToExtract.end)) {
        candidate = candidate->Parent();
    }
    return candidate;
}

static ir::AstNode *ResolveExtractedFromPrimaryNode(ir::AstNode *node, TextRange rangeToExtract)
{
    node = FindExpressionOrVarAncestor(node);
    if (node == nullptr) {
        return nullptr;
    }
    node = LiftNodeToCoverRange(node, rangeToExtract);
    VariableBindingInfo binding;
    if (ResolveVariableBinding(node, binding) && binding.initializer != nullptr &&
        rangeToExtract.pos == binding.initializer->Start().index &&
        rangeToExtract.end == binding.initializer->End().index) {
        return binding.initializer;
    }
    if (node->Start().index <= rangeToExtract.pos && node->End().index >= rangeToExtract.end) {
        return node;
    }
    return FindDeepestOverlappingExpression(node, rangeToExtract);
}

ir::AstNode *FindExtractedVals(const RefactorContext &context)
{
    const auto rangeToExtract = GetTrimmedSelectionSpan(context);
    if (rangeToExtract.pos >= rangeToExtract.end) {
        return nullptr;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return nullptr;
    }
    if (auto *exactExpression = FindExactExpressionByRange(ctx, rangeToExtract); exactExpression != nullptr) {
        return exactExpression;
    }
    auto *originNode = FindTouchingTokenNearSpan(context);
    if (originNode == nullptr) {
        originNode = FindTouchingTokenByScan(context, ctx);
    }
    if (originNode == nullptr) {
        return nullptr;
    }
    if (auto *resolved = ResolveExtractedFromPrimaryNode(originNode, rangeToExtract); resolved != nullptr) {
        return resolved;
    }
    auto *overlap = FindDeepestOverlappingExpression(originNode, rangeToExtract);
    return overlap == nullptr ? nullptr : LiftNodeToCoverRange(overlap, rangeToExtract);
}

static ir::AstNode *ResolveFunctionExtractionTouchNode(const RefactorContext &context, public_lib::Context *ctx,
                                                       TextRange rangeToExtract)
{
    auto *node = GetTouchingToken(context.context, rangeToExtract.pos, false);
    if (node == nullptr) {
        node = FindTouchingTokenNearSpan(context);
    }
    if (node == nullptr) {
        node = FindTouchingTokenByScan(context, ctx);
    }
    return node;
}

static ir::AstNode *LiftFunctionExtractionCandidate(ir::AstNode *node)
{
    while (node != nullptr && !node->IsExpression() && !node->IsFunctionExpression() &&
           !node->IsArrowFunctionExpression() && !node->IsStatement() && !node->IsVariableDeclaration()) {
        node = node->Parent();
    }
    return node;
}

static ir::AstNode *ResolveThrowSelectionArgument(ir::AstNode *node, TextRange rangeToExtract)
{
    if (node == nullptr || !node->IsThrowStatement()) {
        return nullptr;
    }
    auto *argument = const_cast<ir::Expression *>(node->AsThrowStatement()->Argument());
    if (argument != nullptr && argument->Start().index <= rangeToExtract.pos &&
        argument->End().index >= rangeToExtract.end) {
        return argument;
    }
    return nullptr;
}

ir::AstNode *FindExtractedFunction(const RefactorContext &context)
{
    const auto rangeToExtract = GetTrimmedSelectionSpan(context);
    if (rangeToExtract.pos >= rangeToExtract.end) {
        return nullptr;
    }
    if (HasImportDeclarationOverlap(context, rangeToExtract)) {
        return nullptr;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr) {
        return nullptr;
    }
    if (ctx->parserProgram->Ast() == nullptr) {
        return nullptr;
    }
    auto *node = ResolveFunctionExtractionTouchNode(context, ctx, rangeToExtract);
    if (node == nullptr) {
        return nullptr;
    }
    if (auto *declNode = FindWholeVariableDeclarationSelectionNode(context, rangeToExtract); declNode != nullptr) {
        return declNode;
    }
    if (IsImportSelectionNode(node)) {
        return nullptr;
    }
    node = LiftFunctionExtractionCandidate(node);
    if (auto *throwArgument = ResolveThrowSelectionArgument(node, rangeToExtract); throwArgument != nullptr) {
        return throwArgument;
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

ir::AstNode *IsReplaceRangeRequired(const RefactorContext &context, ir::AstNode *extractedText);

bool IsInsideFinallyBlock(ir::AstNode *node)
{
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        auto *parent = current->Parent();
        if (parent == nullptr || !parent->IsTryStatement()) {
            continue;
        }
        auto *tryStmt = parent->AsTryStatement();
        if (tryStmt != nullptr && tryStmt->FinallyBlock() == current) {
            return true;
        }
    }
    return false;
}
}  // namespace ark::es2panda::lsp
