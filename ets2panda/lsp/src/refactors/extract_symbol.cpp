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
 *      - std::string BuildFunctionText(const FunctionExtraction &, const RefactorContext &)
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
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "refactors/extract_symbol.h"
#include "ir/astNode.h"
#include "ir/base/scriptFunction.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/memberExpression.h"
#include "ir/ets/etsParameterExpression.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/expressionStatement.h"
#include "ir/statements/functionDeclaration.h"
#include "ir/statements/variableDeclaration.h"
#include "util/ustring.h"
#include "lexer/token/sourceLocation.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
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
            !node->IsBinaryExpression() && !node->IsObjectExpression());
}

TextRange GetCallPositionOfExtraction(const RefactorContext &context)
{
    auto start = context.span.pos;
    auto end = context.span.end;
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
        if (ch == '\n' || ch == '\r') {
            break;
        }
        --cursor;
    }
    size_t indentEnd = cursor;
    while (indentEnd < sv.size()) {
        char ch = sv[indentEnd];
        if (ch == ' ' || ch == '\t') {
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
        std::string name = ident->Name().Mutf8();
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
        return binding.identifier->Name().Mutf8();
    }
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
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
        std::string name = ident->Name().Mutf8();
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

bool IsVariableExtractionAction(const std::string &actionName)
{
    return actionName == EXTRACT_VARIABLE_ACTION_ENCLOSE.name || actionName == EXTRACT_VARIABLE_ACTION_GLOBAL.name;
}

bool IsConstantExtractionInClassAction(const std::string &actionName)
{
    return actionName == EXTRACT_CONSTANT_ACTION_CLASS.name || actionName == EXTRACT_CONSTANT_ACTION_CLASS.kind;
}

bool IsConstantExtractionAction(const std::string &actionName)
{
    return actionName == EXTRACT_CONSTANT_ACTION_GLOBAL.name || actionName == EXTRACT_CONSTANT_ACTION_ENCLOSE.name ||
           actionName == EXTRACT_CONSTANT_ACTION_CLASS.name || actionName == EXTRACT_CONSTANT_ACTION_GLOBAL.kind ||
           actionName == EXTRACT_CONSTANT_ACTION_ENCLOSE.kind || actionName == EXTRACT_CONSTANT_ACTION_CLASS.kind;
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
        if (func != nullptr && func->Id() != nullptr && func->Id()->Name().Mutf8() == name) {
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

static bool ClassHasProperty(ir::ClassDefinition *classDef, const std::string &name)
{
    if (classDef == nullptr) {
        return false;
    }
    for (auto *member : classDef->Body()) {
        if (!member->IsClassProperty()) {
            continue;
        }
        auto *prop = member->AsClassProperty();
        if (prop->Key()->IsIdentifier() && prop->Key()->AsIdentifier()->Name().Mutf8() == name) {
            return true;
        }
    }
    return false;
}

static bool ClassHasMethod(ir::ClassDefinition *classDef, const std::string &name)
{
    if (classDef == nullptr) {
        return false;
    }
    for (auto *method : classDef->Body()) {
        if (method->Type() != ir::AstNodeType::METHOD_DEFINITION) {
            continue;
        }
        auto *func = method->AsMethodDefinition()->Function();
        if (func && func->Id() && func->Id()->Name().Mutf8() == name) {
            return true;
        }
    }
    return false;
}

static bool ScopeHasVar(ir::AstNode *scopeNode, const std::string &name)
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
            if (decl->Id() && decl->Id()->IsIdentifier() && decl->Id()->AsIdentifier()->Name().Mutf8() == name) {
                found = true;
            }
        }
    });
    return found;
}

static bool ProgramHasGlobalVar(public_lib::Context *ctx, const std::string &name)
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

ir::ClassDefinition *FindClassDefinitionFromNode(ir::AstNode *node)
{
    while (node != nullptr) {
        if (node->IsClassDeclaration()) {
            return node->AsClassDeclaration()->Definition();
        }
        node = node->Parent();
    }
    return nullptr;
}

std::string GenerateUniqueFuncName(const RefactorContext &context, const std::string &baseName,
                                   const std::string &actionName)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr) {
        return baseName;
    }
    int counter = 0;
    std::string tryName = baseName;

    if (actionName == EXTRACT_FUNCTION_ACTION_GLOBAL.name) {
        while (ProgramHasFunction(ctx, tryName)) {
            ++counter;
            tryName = baseName + "_" + std::to_string(counter);
        }
        return tryName;
    }

    if (actionName == EXTRACT_FUNCTION_ACTION_CLASS.name) {
        auto *node = GetTouchingToken(context.context, context.span.pos, false);
        auto *classDef = FindClassDefinitionFromNode(node);
        if (classDef == nullptr) {
            return baseName;
        }
        while (ClassHasMethod(classDef, tryName)) {
            ++counter;
            tryName = baseName + "_" + std::to_string(counter);
        }
        return tryName;
    }

    return baseName;
}

static std::string GenerateUniqueClassPropertyName(const RefactorContext &context)
{
    std::string baseName = "newProperty";
    int counter = 0;
    std::string tryName = baseName;
    auto *node = GetTouchingToken(context.context, context.span.pos, false);
    auto *classDef = FindClassDefinitionFromNode(node);
    if (classDef == nullptr) {
        return baseName;
    }
    while (ClassHasProperty(classDef, tryName)) {
        ++counter;
        tryName = baseName + "_" + std::to_string(counter);
    }
    return "this." + tryName;
}

static std::string GenerateUniqueGlobalVarName(const RefactorContext &context)
{
    std::string baseName = "newLocal";
    int counter = 0;
    std::string tryName = baseName;
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    while (ProgramHasGlobalVar(ctx, tryName)) {
        ++counter;
        tryName = baseName + "_" + std::to_string(counter);
    }
    return tryName;
}

static std::string GenerateUniqueEncloseVarName(const RefactorContext &context)
{
    std::string baseName = "newLocal";
    int counter = 0;
    std::string tryName = baseName;
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    auto *node = GetTouchingToken(context.context, context.span.pos, false);
    ir::AstNode *scopeNode = nullptr;
    while (node != nullptr) {
        if (node->IsBlockStatement() || node->IsFunctionDeclaration() || node->IsFunctionExpression() ||
            node->IsArrowFunctionExpression() || node->IsProgram()) {
            scopeNode = node;
            break;
        }
        node = node->Parent();
    }
    if (scopeNode == nullptr) {
        scopeNode = ctx->parserProgram->Ast();
    }
    while (ScopeHasVar(scopeNode, tryName)) {
        ++counter;
        tryName = baseName + "_" + std::to_string(counter);
    }
    return tryName;
}

std::string GenerateUniqueExtractedVarName(const RefactorContext &context, const std::string &actionName)
{
    if (IsConstantExtractionInClassAction(actionName)) {
        return GenerateUniqueClassPropertyName(context);
    }
    if (actionName == EXTRACT_CONSTANT_ACTION_GLOBAL.name || actionName == EXTRACT_VARIABLE_ACTION_GLOBAL.name) {
        return GenerateUniqueGlobalVarName(context);
    }
    if (actionName == EXTRACT_VARIABLE_ACTION_ENCLOSE.name || actionName == EXTRACT_CONSTANT_ACTION_ENCLOSE.name) {
        return GenerateUniqueEncloseVarName(context);
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
        while (lineStart < src.size() && (src[lineStart] == ' ' || src[lineStart] == '\t')) {
            ++lineStart;
        }
        if (lineStart >= src.size()) {
            break;
        }
        if (src[lineStart] != '\'' && src[lineStart] != '"') {
            break;
        }
#ifdef _WIN32
        size_t newline = src.find("\r\n", lineStart);
#else
        size_t newline = src.find('\n', lineStart);
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

    while (pos < sv.size() && sv[pos] != '\n' && sv[pos] != '\r') {
        ++pos;
    }
    while (pos < sv.size() && (sv[pos] == '\n' || sv[pos] == '\r')) {
        ++pos;
    }
    return pos;
}

void TrimTrailingNewlines(std::string &text)
{
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
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

std::pair<std::string, std::string> BuildParamSignature(public_lib::Context *ctx, const VariableBindingInfo &binding)
{
    std::vector<std::string> freeVars = CollectIdentifierNames(binding.initializer, binding.identifier->Name().Mutf8());
    auto *enclosingFunc = FindScriptFunction(binding.declaration);
    auto paramText = CollectParameterText(ctx, enclosingFunc);

    std::vector<std::string> paramDecls;
    paramDecls.reserve(freeVars.size());
    for (const auto &name : freeVars) {
        auto it = paramText.find(name);
        paramDecls.push_back(it == paramText.end() ? name : it->second);
    }
    return {JoinWithComma(paramDecls), JoinWithComma(freeVars)};
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

    auto [paramsSig, callArgs] = BuildParamSignature(pubCtx, binding);

    std::string callExpr = std::string(helperName) + "(" + callArgs + ")";
    std::string replacement = BuildAssignmentLine(pubCtx, binding, indent, callExpr, newLine);

    out.insertHelper = false;

    if (!ProgramHasFunction(pubCtx, std::string(helperName))) {
        std::string initBody = GetNodeText(pubCtx, binding.initializer);
        TrimTrailingNewlines(initBody);
        std::string helper;
        helper.reserve(paramsSig.size() + initBody.size() + HELPER_RESERVE_PADDING);
        helper.append(newLine);
        helper.append("function ").append(helperName).append("(").append(paramsSig).append(") {").append(newLine);
        helper.append("    return ").append(initBody);
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
    while (lineStart > 0 && source[lineStart - 1] != '\n' && source[lineStart - 1] != '\r') {
        --lineStart;
    }
    return lineStart;
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
    std::string classIndent = methodIndent.size() >= 4 ? methodIndent.substr(0, methodIndent.size() - 4) : "";

    auto [paramsSig, callArgs] = BuildParamSignature(pubCtx, binding);

    std::string helper;
    helper.reserve(body.size() + paramsSig.size() + HELPER_RESERVE_PADDING);
    helper.append(newLine);
    helper.append(classIndent)
        .append("private ")
        .append(helperName)
        .append("(")
        .append(paramsSig)
        .append(") {")
        .append(newLine);
    helper.append(classIndent).append("    ").append(body).append(newLine);
    helper.append(classIndent)
        .append("    return ")
        .append(binding.identifier->Name().Mutf8())
        .append(";")
        .append(newLine);
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
    return index + pieces.replaceRange.pos + pieces.helperText.length() + 1;
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
    if (cls == nullptr || cls->AsClassDefinition()->Ident() == nullptr || cls->AsClassDefinition()->IsGlobal()) {
        return false;
    }
    return true;
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

    if (prevClassProperty != nullptr && (AreNodesCommaSeparated(context, prevClassProperty, targetClassProperty) ||
                                         AreNodesCommaSeparated(context, targetClassProperty, nextClassProperty))) {
        return true;
    }

    return false;
}

static void AdjustStatementForGlobalIfClass(ir::AstNode *&statement, ir::AstNode *node, size_t startLine)
{
    if (node == nullptr || !node->IsClassDefinition()) {
        return;
    }

    auto *cls = node->AsClassDefinition();
    if (!cls->Body().empty()) {
        statement = cls->Body().at(0);
    }

    for (auto ndx : cls->Body()) {
        if (ndx->Start().line > startLine || ndx->Start().index == ndx->End().index) {
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

static ir::AstNode *FindClassBreakPosition(ir::AstNode *target)
{
    for (ir::AstNode *node = target; node != nullptr; node = node->Parent()) {
        if (IsClassBreak(node->Parent())) {
            ir::AstNode *insertPosNode = node;
            AdjustStatementForGlobalIfClass(insertPosNode, node, target->Start().line);
            return insertPosNode;
        }
    }
    return nullptr;
}

size_t FindPreviousNonCommentLine(const public_lib::Context *context, size_t startLine /* 0-based */)
{
    lexer::LineIndex lineIndex(context->parserProgram->SourceCode());
    const auto &sourceCode = context->parserProgram->SourceCode().Mutf8();
    size_t currentLine = startLine;
    bool lookingForBlockStart = false;

    for (; currentLine > 0; currentLine--) {
        size_t lineStart = (currentLine == 0) ? 0 : lineIndex.GetOffsetOfLine(currentLine - 1) + 1;
        size_t lineEnd = lineIndex.GetOffsetOfLine(currentLine);
        std::string lineText(sourceCode.begin() + lineStart, sourceCode.begin() + lineEnd);

        if (lookingForBlockStart) {
            if (lineText.find("/*") != std::string::npos) {
                lookingForBlockStart = false;
            }
        } else {
            if (lineText.find("*/") != std::string::npos) {
                lookingForBlockStart = true;
            } else if (lineText.find("//") == std::string::npos) {
                break;
            }
        }
    }

    return currentLine;
}

size_t FindInsertionPos(public_lib::Context *context, ir::AstNode *target, const std::string &actionName)
{
    ir::AstNode *insertPosNode = nullptr;
    if (actionName == std::string(EXTRACT_VARIABLE_ACTION_ENCLOSE.name) ||
        actionName == std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.name)) {
        insertPosNode = FindBreakPosition(target, IsEncloseVarConstBreak);
    }
    if (actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name) ||
        actionName == std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.name) ||
        actionName == std::string(EXTRACT_VARIABLE_ACTION_GLOBAL.name)) {
        insertPosNode = FindBreakPosition(target, IsGlobalBreak);
    }
    if (actionName == std::string(EXTRACT_FUNCTION_ACTION_CLASS.name) ||
        actionName == std::string(EXTRACT_CONSTANT_ACTION_CLASS.name)) {
        insertPosNode = FindClassBreakPosition(target);
    }
    if (insertPosNode == nullptr) {
        return 0;
    }
    if (IsMultiDecl(target, context)) {
        return insertPosNode->Start().index;
    }
    size_t line = insertPosNode->Start().line - 1;
    line = FindPreviousNonCommentLine(context, line);
    size_t lineEnd = LineToPos(context, line);

    return lineEnd < insertPosNode->Parent()->Start().index ? insertPosNode->Start().index : lineEnd;
}

TextRange GetVarAndFunctionPosToWriteNode(const RefactorContext &context, const std::string &actionName)
{
    auto startedNode = GetTouchingTokenByRange(context.context, context.span, false);
    auto ctx = reinterpret_cast<public_lib::Context *>(context.context);
    const auto startPos = FindInsertionPos(ctx, startedNode, actionName);
    return {startPos, startPos};
}

ir::AstNode *FindExtractedVals(const RefactorContext &context)
{
    const auto rangeToExtract = context.span;
    if (rangeToExtract.pos >= rangeToExtract.end) {
        return nullptr;
    }

    const auto ast = reinterpret_cast<public_lib::Context *>(context.context)->parserProgram->Ast();
    if (ast == nullptr) {
        return nullptr;
    }

    auto node = GetTouchingTokenByRange(context.context, context.span, false);
    if (node == nullptr) {
        return nullptr;
    }
    while (node != nullptr && (!node->IsExpression() && !node->IsVariableDeclaration())) {
        node = node->Parent();
    }

    if (node != nullptr) {
        return node;
    }
    return nullptr;
}
ir::AstNode *FindExtractedFunction(const RefactorContext &context)
{
    const auto rangeToExtract = context.span;
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

std::string BuildFunctionBody(const std::string &body, const std::string &newLine)
{
    std::ostringstream oss;
    std::istringstream lines(body);
    std::string line;
    while (std::getline(lines, line)) {
        oss << "    return " << line << (std::strchr(line.c_str(), ';') != nullptr ? "" : ";") << newLine;
    }
    return oss.str();
}
std::string GenerateExtractedFunctionCode(const std::string &bodyText, const std::string &params,
                                          const RefactorContext &context, const std::string &actionName)
{
    std::string baseName = (actionName == EXTRACT_FUNCTION_ACTION_CLASS.name) ? "newMethod" : "newFunction";
    std::string functionName = GenerateUniqueFuncName(context, baseName, actionName);
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();

    std::ostringstream oss;
    oss << newLine << "function " << functionName << "(" << params << ") {" << newLine
        << BuildFunctionBody(bodyText, newLine) << "}" << newLine;
    return oss.str();
}

std::string BuildFunctionText(const FunctionExtraction &candidate, const RefactorContext &context,
                              const std::string &actionName)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr || ctx->sourceFile->source.empty()) {
        return "";
    }

    const auto &src = ctx->sourceFile->source;
    const auto ast = ctx->parserProgram->Ast();
    auto extractionPos = GetCallPositionOfExtraction(context);

    size_t start = extractionPos.pos;
    size_t end = extractionPos.end;
    if (start >= src.size() || end > src.size() || start >= end) {
        return "";
    }

    bool needParams = false;
    auto functionParams = CollectFunctionParams(ast, start, end, needParams);
    std::string params = needParams ? GetParamsText(candidate, functionParams) : "";

    std::string bodyText(src.begin() + start, src.begin() + end);
    return GenerateExtractedFunctionCode(bodyText, params, context, actionName);
}

std::string ReplaceWithFunctionCall(const FunctionExtraction &candidate, const std::string &functionText)
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
    std::string callArgs;
    for (size_t i = 0; i < candidate.parameters.size(); ++i) {
        if (i != 0) {
            callArgs += ", ";
        }
        callArgs += candidate.parameters[i]->Ident()->Name().Mutf8();
    }
    std::string callText = functionName + "(" + callArgs + ")";
    return callText;
}
static void AddRefactorAction(std::vector<RefactorAction> &list, const RefactorActionView &info)
{
    RefactorAction action;
    action.name = info.name;
    action.description = info.description;
    action.kind = info.kind;
    list.push_back(action);
}

static bool IsInsideExtractionRange(const ir::AstNode *node, TextRange positions)
{
    return node->Start().index >= positions.pos && node->End().index <= positions.end;
}

static bool HasBlockEnclosing(ir::AstNode *node)
{
    auto *block = node->AsArrowFunctionExpression()->Function()->Body()->AsBlockStatement();
    if (block == nullptr || (block->Start().index == block->End().index)) {
        return false;
    }
    return true;
}

static bool HasEncloseScope(ir::AstNode *node)
{
    for (; node != nullptr; node = node->Parent()) {
        if (node->IsFunctionDeclaration() || node->IsFunctionExpression()) {
            return true;
        } else if (node->IsArrowFunctionExpression()) {
            return HasBlockEnclosing(node);
        }
    }
    return false;
}

static void AddExtractFunctionActions(std::vector<RefactorAction> &actions, bool hasClassScope)
{
    if (hasClassScope) {
        AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_CLASS);
    }
    AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_GLOBAL);
}

static void AddExtractVariableActions(std::vector<RefactorAction> &actions, bool isEncloseScopeAvailable,
                                      bool hasClassScope)
{
    if (isEncloseScopeAvailable) {
        AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE);
        AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_ENCLOSE);
    } else {
        AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_GLOBAL);
    }
    if (hasClassScope) {
        AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_CLASS);
    }
    AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_GLOBAL);
}

std::vector<RefactorAction> FindAvailableRefactors(const RefactorContext &context)
{
    std::vector<RefactorAction> actions;

    ir::AstNode *node = nullptr;
    if (context.span.pos == context.span.end) {
        node = GetTouchingToken(context.context, context.span.pos, false);
    } else {
        node = GetTouchingTokenByRange(context.context, context.span, false);
        if (node != nullptr) {
            node = GetOptimumNodeByRange(node, context.span);
        }
    }

    if (node == nullptr) {
        return actions;
    }

    const auto positions = GetCallPositionOfExtraction(context);
    if (!IsInsideExtractionRange(node, positions)) {
        return actions;
    }

    const bool hasScope = HasEncloseScope(node);
    const bool hasClassScope = IsClassContext(node);

    if (node->IsExpression() || node->IsFunctionExpression() || node->IsArrowFunctionExpression() ||
        node->IsStatement()) {
        AddExtractFunctionActions(actions, hasClassScope);
    }

    if (!node->IsStatement() || node->IsVariableDeclaration() || node->IsBinaryExpression()) {
        AddExtractVariableActions(actions, hasScope, hasClassScope);
    }

    return actions;
}

ir::AstNode *FindRefactor(const RefactorContext &context, const std::string &actionName)
{
    if (actionName == EXTRACT_CONSTANT_ACTION_GLOBAL.name || actionName == EXTRACT_CONSTANT_ACTION_ENCLOSE.name ||
        actionName == EXTRACT_VARIABLE_ACTION_ENCLOSE.name || actionName == EXTRACT_CONSTANT_ACTION_CLASS.name ||
        actionName == EXTRACT_VARIABLE_ACTION_GLOBAL.name) {
        return FindExtractedVals(context);
    }

    if (actionName == EXTRACT_FUNCTION_ACTION_GLOBAL.name || actionName == EXTRACT_FUNCTION_ACTION_CLASS.name) {
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

std::string BuildExtractionDeclaration(const RefactorContext &context, public_lib::Context *ctx,
                                       ir::AstNode *extractedText, const std::string &actionName,
                                       bool &isVariableExtraction, const std::string &varName)
{
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    bool isAppend = false;
    isVariableExtraction = IsVariableExtractionAction(actionName);
    const bool isConstantExtraction = IsConstantExtractionAction(actionName);
    if (!isConstantExtraction && !isVariableExtraction) {
        return "";
    }
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return "";
    }

    std::string_view srcView(ctx->sourceFile->source);
    std::string placeholder = GetConstantString(srcView, extractedText);
    if (placeholder.empty()) {
        return "";
    }

    auto startedNode = GetTouchingTokenByRange(context.context, context.span, false);
    std::string declaration = "";
    bool isMutiDecl = IsMultiDecl(startedNode, ctx);
    if (IsConstantExtractionInClassAction(actionName)) {
        declaration.append("private readonly ").append(varName.substr(std::string("this.").size())).append(" = ");
        isAppend = true;
    } else if (isMutiDecl) {
        declaration.append(varName).append(" = ");
    } else {
        declaration.append(isConstantExtraction ? "const " : "let ").append(varName).append(" = ");
    }
    declaration.append(placeholder);
    if (isMutiDecl && declaration.find(',') == std::string::npos) {
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

    bool isVariableExtraction = false;
    std::string declaration =
        BuildExtractionDeclaration(context, ctx, extractedText, actionName, isVariableExtraction, varName);
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

RefactorEditInfo GetRefactorEditsToExtractVals(const RefactorContext &context, ir::AstNode *extractedText,
                                               const std::string &actionName)
{
    std::string uniqueVarName = GenerateUniqueExtractedVarName(context, actionName);
    std::string generatedText = GenerateInlineEdits(context, extractedText, actionName, uniqueVarName);
    if (generatedText.empty()) {
        return RefactorEditInfo {};
    }
    size_t insertPos = GetVarAndFunctionPosToWriteNode(context, actionName).pos;
    TextRange extractedRange {extractedText->Start().index, extractedText->End().index};
    size_t startPos = extractedText->Start().index + 1;
    TextChangesContext textChangesContext = *context.textChangesContext;
    const auto src = reinterpret_cast<public_lib::Context *>(context.context)->sourceFile;
    const auto fileName = src->filePath;
    std::vector<FileTextChanges> edits;
    std::string extractedName = "";

    extractedName.append(uniqueVarName);
    edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(src, insertPos, generatedText);
        auto exprStmt = IsReplaceRangeRequired(context, extractedText);
        if (exprStmt != nullptr) {
            tracker.DeleteRange(src, TextRange {exprStmt->Start().index, exprStmt->End().index});
            return;
        }
        tracker.ReplaceRangeWithText(src, extractedRange, uniqueVarName);
    });

    size_t renameLoc = startPos;
    if (!edits.empty() && !edits[0].textChanges.empty()) {
        renameLoc += edits[0].textChanges[0].newText.length() + 1;
    }

    RefactorEditInfo refactorEdits(std::move(edits), std::optional<std::string>(fileName),
                                   std::optional<size_t>(renameLoc));
    return refactorEdits;
}

RefactorEditInfo GetRefactorEditsToExtractFunction(const RefactorContext &context, const std::string &actionName)
{
    std::vector<FileTextChanges> edits;

    auto *extractedNode = FindExtractedFunction(context);
    if (extractedNode == nullptr) {
        return RefactorEditInfo();
    }

    RefactorEditInfo helperEdits;
    if (TryBuildHelperExtraction(context, extractedNode, actionName, helperEdits)) {
        return helperEdits;
    }
    auto candidates = GetPossibleFunctionExtractions(context);
    if (candidates.empty()) {
        return RefactorEditInfo();
    }

    FunctionExtraction candidate = candidates.front();

    CollectFunctionParameters(candidate);
    std::string functionText = BuildFunctionText(candidate, context, actionName);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    auto funcCallText = ReplaceWithFunctionCall(candidate, functionText);

    TextChangesContext textChangesContext = *context.textChangesContext;
    const auto src = reinterpret_cast<public_lib::Context *>(context.context)->sourceFile;
    edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(src, GetVarAndFunctionPosToWriteNode(context, actionName).pos, functionText);
        tracker.ReplaceRangeWithText(src, GetCallPositionOfExtraction(context), funcCallText);
    });
    size_t renameLoc = GetCallPositionOfExtraction(context).pos + 1;
    if (!edits.empty() && !edits[0].textChanges.empty()) {
        renameLoc += edits[0].textChanges[0].newText.length() + 1;
    }
    RefactorEditInfo refactorEdits(std::move(edits), std::optional<std::string>(src->filePath),
                                   std::optional<size_t>(renameLoc));
    return refactorEdits;
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
    if (actionName == EXTRACT_CONSTANT_ACTION_GLOBAL.name || actionName == EXTRACT_CONSTANT_ACTION_ENCLOSE.name ||
        actionName == EXTRACT_CONSTANT_ACTION_CLASS.name || actionName == EXTRACT_VARIABLE_ACTION_ENCLOSE.name ||
        actionName == EXTRACT_VARIABLE_ACTION_GLOBAL.name) {
        refactor = GetRefactorEditsToExtractVals(context, extractedText, actionName);
    } else if (actionName == EXTRACT_FUNCTION_ACTION_GLOBAL.name || actionName == EXTRACT_FUNCTION_ACTION_CLASS.name) {
        refactor = GetRefactorEditsToExtractFunction(context, actionName);
    }

    return std::make_unique<RefactorEditInfo>(refactor);
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ExtractSymbolRefactor> g_extractSymbolRefactorRegister("ExtractSymbolRefactor");

}  // namespace ark::es2panda::lsp
