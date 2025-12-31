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
/**
 * @file extract_symbol_refactor.cpp
 * @brief Implements extract-symbol refactoring logic for LSP.
 *
 * This file provides the implementation of the ExtractSymbolRefactor, which
 * supports extracting variables, constants, and functions from selected AST
 * nodes. It analyzes the AST range selected by the user and determines valid
 * refactor actions, generates replacement text, and produces text edits.
 *
 * Supported refactors:
 *  - Extract Variable (enclosed scope)
 *  - Extract Constant (enclosed / class / global)
 *  - Extract Function (class / global)
 *
 * The implementation relies on AST traversal, scope analysis, and ChangeTracker
 * utilities to safely generate refactor edits.
 */

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iterator>
#include <ostream>
#include <string>
#include <vector>
#include "refactors/extract_symbol.h"
#include "internal_api.h"
#include "ir/astNode.h"
#include "ir/ets/etsParameterExpression.h"
#include "ir/expressions/identifier.h"
#include "public/public.h"
#include "rename.h"
#include "refactor_provider.h"
#include "refactors/refactor_types.h"
#include "services/text_change/change_tracker.h"

namespace ark::es2panda::lsp {

ExtractSymbolRefactor::ExtractSymbolRefactor()
{
    AddKind(std::string(EXTRACT_VARIABLE_ACTION_ENCLOSE.kind));
    AddKind(std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.kind));
    AddKind(std::string(EXTRACT_CONSTANT_ACTION_CLASS.kind));
    AddKind(std::string(EXTRACT_FUNCTION_ACTION_CLASS.kind));
    AddKind(std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.kind));
    AddKind(std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.kind));
}

bool IsNodeInRange(const TextRange &range, ir::AstNode *node)
{
    const auto nodeStart = node->Start().index;
    const auto nodeEnd = node->End().index;
    auto flag = false;
    if (nodeStart >= range.pos && nodeEnd <= range.end) {
        flag = true;
    }
    return flag;
}

bool IsConstantExtractionInClassAction(const std::string &actionName)
{
    return actionName == EXTRACT_CONSTANT_ACTION_CLASS.name || actionName == EXTRACT_CONSTANT_ACTION_CLASS.kind;
}

bool IsNodeInExtractScope(ir::AstNode *node)
{
    bool flag = false;
    if (!node->IsVariableDeclarator() &&
        (node->IsStatement() || node->IsVariableDeclaration() || node->IsClassProperty() || node->IsCallExpression() ||
         node->IsBinaryExpression() || node->IsAssignmentExpression() || node->IsFunctionExpression() ||
         node->IsArrowFunctionExpression() || node->IsReturnStatement() || node->IsClassProperty() ||
         node->IsNumberLiteral() || node->IsStringLiteral() || node->IsBooleanLiteral() || node->IsTemplateLiteral())) {
        flag = true;
    }
    return flag;
}

std::vector<ir::AstNode *> FindNodesInRange(const RefactorContext &context, const TextRange &range)
{
    std::vector<ir::AstNode *> resList;
    const auto &ctx = reinterpret_cast<public_lib::Context *>(context.context);
    const auto astRoot = ctx->parserProgram->Ast();
    if (astRoot == nullptr) {
        return resList;
    }
    astRoot->FindChild([range, &resList](ir::AstNode *node) -> bool {
        if (IsNodeInRange(range, node) && IsNodeInExtractScope(node)) {
            if (!resList.empty() && (resList[resList.size() - 1]->Start().index <= node->Start().index &&
                                     resList[resList.size() - 1]->End().index >= node->End().index)) {
                return false;
            }
            resList.push_back(node);
        }
        return false;
    });
    return resList;
}

TextRange GetParentRange(const RefactorContext &context)
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
static void AddRefactorAction(std::vector<RefactorAction> &list, const RefactorActionView &info,
                              const std::string &className)
{
    RefactorAction action;
    action.name = info.name;
    if (info.name == EXTRACT_CONSTANT_ACTION_CLASS.name || info.name == EXTRACT_FUNCTION_ACTION_CLASS.name) {
        if (!className.empty()) {
            std::string desc = std::string(info.description) + className;
            action.description = desc;
        } else {
            action.description = std::string(info.description) + "Class Scope";
        }
    } else {
        action.description = info.description;
    }
    action.kind = info.kind;
    list.push_back(action);
}

static bool HasEnclosingFunction(ir::AstNode *node)
{
    while (node != nullptr) {
        if (node->IsFunctionDeclaration() || node->IsFunctionExpression() || node->IsArrowFunctionExpression() ||
            node->IsBlockStatement()) {
            return true;
        }
        node = node->Parent();
    }
    return false;
}

ir::ScriptFunction *FindEnclosingScriptFunction(ir::AstNode *node)
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
        if (current->IsMethodDefinition()) {
            return current->AsMethodDefinition()->Function();
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

bool IsClassMethodContext(ir::AstNode *node)
{
    auto *func = FindEnclosingScriptFunction(node);
    if (func == nullptr) {
        return false;
    }
    return FindEnclosingClassDefinition(node) != nullptr;
}

static void AddExtractFunctionActions(std::vector<RefactorAction> &actions, bool hasClassScope,
                                      const std::string &className)
{
    if (hasClassScope) {
        AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_CLASS, className);
    }
    AddRefactorAction(actions, EXTRACT_FUNCTION_ACTION_GLOBAL, className);
}

static void AddExtractVariableActions(std::vector<RefactorAction> &actions, bool isEncloseScopeAvailable,
                                      bool hasClassScope, const std::string &className)
{
    if (isEncloseScopeAvailable) {
        AddRefactorAction(actions, EXTRACT_VARIABLE_ACTION_ENCLOSE, className);
        AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_ENCLOSE, className);
    }
    if (hasClassScope) {
        AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_CLASS, className);
    }
    AddRefactorAction(actions, EXTRACT_CONSTANT_ACTION_GLOBAL, className);
}

std::vector<std::string> CollectFreeVariables(ir::AstNode *node)
{
    std::vector<std::string> freeVars;
    node->FindChild([&freeVars](ir::AstNode *child) -> bool {
        if (child->IsIdentifier()) {
            std::string varName = child->AsIdentifier()->Name().Mutf8();
            freeVars.push_back(varName);
        }
        return false;
    });
    return freeVars;
}
bool IsValueProducing(ir::AstNode *n)
{
    bool flag = false;
    if (n == nullptr) {
        return flag;
    }
    if (n->IsBinaryExpression() || n->IsBooleanLiteral() || n->IsNumberLiteral() || n->IsStringLiteral() ||
        n->IsTemplateLiteral() || n->IsCharLiteral() || n->IsVariableDeclaration() || n->IsCallExpression() ||
        n->IsClassProperty() || n->IsAssignmentExpression()) {
        flag = true;
    }
    return flag;
}

ir::AstNode *FindReturnValueNode(const std::vector<ir::AstNode *> &nodes)
{
    for (int i = nodes.size() - 1; i >= 0; --i) {
        ir::AstNode *n = nodes[i];
        if (IsValueProducing(n)) {
            return n;
        }
    }
    return nullptr;
}

ir::AstNode *FindParentNode(ir::AstNode *node, ir::AstNode *parent)
{
    for (ir::AstNode *current = node; current != nullptr; current = current->Parent()) {
        if (current->Parent() == parent) {
            return current;
        }
    }
    return nullptr;
}

bool IsNodesCanExtract(const std::vector<ir::AstNode *> &nodeList)
{
    const auto parent = nodeList[0]->Parent();
    std::vector<std::string> freeVars;
    for (ir::AstNode *node : nodeList) {
        if (FindParentNode(node, parent) == nullptr) {
            return false;
        }
        if (!node->IsVariableDeclaration() && !node->IsExpression() && !node->IsStatement() &&
            !node->IsClassProperty() && !node->IsVariableDeclarator() && !node->IsStringLiteral() &&
            !node->IsNumberLiteral()) {
            return false;
        }
        if (node->IsReturnStatement() || node->IsBreakStatement() || node->IsContinueStatement()) {
            return false;
        }
    }
    return FindReturnValueNode(nodeList) != nullptr;
}

std::vector<RefactorAction> FindAvailableRefactors(const RefactorContext &context)
{
    std::vector<RefactorAction> resList;
    const auto rangeToExtract = context.span;
    if (rangeToExtract.pos >= rangeToExtract.end) {
        return resList;
    }
    auto nodeList = FindNodesInRange(context, rangeToExtract);
    if (nodeList.empty()) {
        nodeList = FindNodesInRange(context, GetParentRange(context));
        if (nodeList.empty()) {
            return resList;
        }
    }
    if (!IsNodesCanExtract(nodeList)) {
        return resList;
    }
    bool hasEnclosingFunction = HasEnclosingFunction(nodeList[0]);
    bool isClassMethodContext = IsClassMethodContext(nodeList[0]);
    std::string className;
    auto classNode = FindEnclosingClassDefinition(nodeList[0]);
    className = classNode->AsClassDefinition()->Ident()->Name().Mutf8();
    if (!nodeList[0]->IsStatement() || nodeList[0]->IsVariableDeclaration() || nodeList[0]->IsExpression() ||
        nodeList[0]->IsStringLiteral() || nodeList[0]->IsNumberLiteral()) {
        AddExtractVariableActions(resList, hasEnclosingFunction, isClassMethodContext, className);
    }
    if (nodeList[0]->IsExpression() || nodeList[0]->IsFunctionExpression() || nodeList[0]->IsClassProperty() ||
        nodeList[0]->IsArrowFunctionExpression() || nodeList[0]->IsStatement()) {
        AddExtractFunctionActions(resList, isClassMethodContext, className);
    }
    return resList;
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
        tryName = baseName + std::to_string(counter);
    }
    return tryName;
}

static std::string GenerateUniqueGlobalVarName(const RefactorContext &context)
{
    std::string baseName = "newLocal";
    int counter = 0;
    std::string tryName = baseName;
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    while (ProgramHasGlobalVar(ctx, tryName)) {
        ++counter;
        tryName = baseName + std::to_string(counter);
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
        tryName = baseName + std::to_string(counter);
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

std::string CreateVariableRefactorText(const RefactorContext &context, const std::string &actionName)
{
    std::string resText;
    auto nodeList = FindNodesInRange(context, context.span);
    if (nodeList.empty()) {
        nodeList = FindNodesInRange(context, GetParentRange(context));
        if (nodeList.empty()) {
            return resText;
        }
    }
    const auto &ctx = reinterpret_cast<public_lib::Context *>(context.context);
    const auto firstNode = nodeList[0];
    const auto freeVars = CollectFreeVariables(firstNode);
    if (actionName == std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.name) ||
        actionName == std::string(EXTRACT_CONSTANT_ACTION_CLASS.name)) {
        resText += "\nconst ";
    } else if (actionName == std::string(EXTRACT_VARIABLE_ACTION_ENCLOSE.name)) {
        resText += "\nlet ";
    }
    std::string extractedName = GenerateUniqueExtractedVarName(context, actionName);
    resText = resText + extractedName + " = ";
    for (ir::AstNode *node : nodeList) {
        if (node->IsStatement() && !node->IsVariableDeclaration() && !node->IsExpression()) {
            continue;
        }
        resText += GetSourceTextOfNodeFromSourceFile(ctx->sourceFile->source, node);
        if (resText.find_last_of(';') != resText.length() - 1) {
            resText += ";";
        }
        resText += "\n";
    }
    return resText;
}
struct ParamTexts {
    std::string paramNames;
    std::string paramTypes;
};

std::vector<ParamTexts> CollectFunctionParams(ir::AstNode *ast, size_t start, size_t end, bool &needParams)
{
    std::vector<ParamTexts> params;
    ast->FindChild([&](ir::AstNode *child) {
        if ((child->Start().index >= start && child->End().index <= end) && !child->IsStringLiteral() &&
            !child->IsNumberLiteral() && !child->IsBooleanLiteral() && !child->IsNullLiteral() &&
            !child->IsCharLiteral()) {
            if (child->IsIdentifier()) {
                needParams = true;
                ParamTexts param;
                param.paramNames = child->AsIdentifier()->Name().Mutf8();
                std::string typeText;
                param.paramTypes = typeText;
                params.push_back(param);
            }
        }
        return false;
    });
    return params;
}

void GetParamTypeIfTypeReferance(const ir::ETSParameterExpression *paramExpr, std::vector<ParamTexts> &params)
{
    for (size_t i = 0; i < params.size(); ++i) {
        auto param = params[i];
        if (param.paramNames == paramExpr->Name().Mutf8()) {
            continue;
        }
        auto typeText = paramExpr->TypeAnnotation()->AsETSTypeReference()->Part()->Name()->ToString();
        param.paramTypes = typeText;
        ParamTexts paramRef;
        paramRef.paramNames = param.paramNames;
        paramRef.paramTypes = typeText;
        params.erase(std::remove_if(params.begin(), params.end(),
                                    [&](const ParamTexts &p) { return p.paramNames == param.paramNames; }),
                     params.end());
        params.insert(params.begin() + i, paramRef);
    }
}
std::string GetReturnTypeText(const ir::AstNode *retNode)
{
    std::string returnTypeText;
    switch (retNode->Type()) {
        case ir::AstNodeType::NUMBER_LITERAL:
            returnTypeText = "number";
            break;
        case ir::AstNodeType::STRING_LITERAL:
            returnTypeText = "string";
            break;
        case ir::AstNodeType::BOOLEAN_LITERAL:
            returnTypeText = "boolean";
            break;
        case ir::AstNodeType::BINARY_EXPRESSION:
            returnTypeText = "number";
            break;
        default:
            break;
    }
    return returnTypeText;
}

void GetParamTypeIfVariableDecl(const ir::VariableDeclarator *varDecl, std::vector<ParamTexts> &params)
{
    for (auto &param : params) {
        if (param.paramNames != varDecl->Id()->AsIdentifier()->Name().Mutf8()) {
            continue;
        }
        std::string typeText = GetReturnTypeText(varDecl->Init());
        param.paramTypes = typeText;
    }
}
std::vector<ParamTexts> FindExactFunctionParams(const RefactorContext &context,
                                                const std::vector<ir::AstNode *> &nodeList, bool &needParams)
{
    std::vector<ParamTexts> params;
    for (auto node : nodeList) {
        if (node->IsStatement() && !node->IsVariableDeclaration() && !node->IsExpression()) {
            continue;
        }
        if (!node->IsVariableDeclaration()) {
            auto paramsFromNode = CollectFunctionParams(node, context.span.pos, context.span.end, needParams);
            params.insert(params.end(), paramsFromNode.begin(), paramsFromNode.end());
            continue;
        }
        auto varDecl = node->AsVariableDeclaration();
        auto paramsFromDecl = CollectFunctionParams(varDecl, context.span.pos, context.span.end, needParams);
        params.insert(params.end(), paramsFromDecl.begin(), paramsFromDecl.end());
    }

    params.erase(std::unique(params.begin(), params.end(),
                             [](const ParamTexts &a, const ParamTexts &b) { return a.paramNames == b.paramNames; }),
                 params.end());

    for (auto declIdent : nodeList) {
        std::string varName;
        if (declIdent->IsVariableDeclaration()) {
            varName = declIdent->AsVariableDeclaration()->Declarators()[0]->Id()->Variable()->Name().Mutf8();
        } else if (declIdent->IsClassProperty()) {
            varName = declIdent->AsClassProperty()->Key()->AsIdentifier()->Name().Mutf8();
        }
        if (!varName.empty()) {
            params.erase(std::remove_if(params.begin(), params.end(),
                                        [&](const ParamTexts &p) { return p.paramNames == varName; }),
                         params.end());
        }
    }
    const auto ctx = reinterpret_cast<public_lib::Context *>(context.context);
    ctx->parserProgram->Ast()->FindChild([&params](ir::AstNode *child) {
        if (child->IsETSParameterExpression()) {
            auto paramExpr = child->AsETSParameterExpression();
            GetParamTypeIfTypeReferance(paramExpr, params);
        } else if (child->IsVariableDeclarator()) {
            auto varDecl = child->AsVariableDeclarator();
            if (varDecl->Id()->IsIdentifier() && varDecl->Init() != nullptr) {
                GetParamTypeIfVariableDecl(varDecl, params);
            }
        }
        return false;
    });
    return params;
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
            tryName = baseName + std::to_string(counter);
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
            tryName = baseName + std::to_string(counter);
        }
        return tryName;
    }

    return baseName;
}

std::string GetFunctionNameAndType(const RefactorContext &context, std::string refactorName)
{
    std::string resText;
    std::string helperName;
    if (refactorName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name)) {
        helperName = GenerateUniqueFuncName(context, "newFunction", refactorName);
        resText += "function " + helperName + "(";
    } else if (refactorName == std::string(EXTRACT_FUNCTION_ACTION_CLASS.name)) {
        helperName = GenerateUniqueFuncName(context, "newMethod", refactorName);
        resText += "private " + helperName + "(";
    }
    return resText;
}

std::string WriteFunctionBody(const std::vector<ir::AstNode *> nodeList, const ir::AstNode *returnNode,
                              const public_lib::Context *ctx)
{
    std::string resText;
    for (ir::AstNode *node : nodeList) {
        if (node->IsStatement() && !node->IsVariableDeclaration() && !node->IsExpression() &&
            !node->IsClassProperty()) {
            continue;
        }
        resText += "    ";
        if (node == returnNode) {
            continue;
        }
        auto sText = GetSourceTextOfNodeFromSourceFile(ctx->sourceFile->source, node);
        resText += sText;
        if (sText.find_last_of(';') != sText.length() - 1) {
            resText += ";";
        }
        resText += "\n";
    }
    return resText;
}

std::string GetFunctionParamsText(std::vector<ParamTexts> &params)
{
    std::string resText;
    for (size_t i = 0; i < params.size(); ++i) {
        resText += params[i].paramNames;
        if (!params[i].paramTypes.empty()) {
            resText += ": ";
            resText += params[i].paramTypes;
        }
        if (i < params.size() - 1) {
            resText += ", ";
        }
    }
    return resText;
}

std::string CreateFunctionRefactorText(const RefactorContext &context, const std::string &refactorName)
{
    std::string resText;
    auto nodeList = FindNodesInRange(context, context.span);
    if (nodeList.empty()) {
        nodeList = FindNodesInRange(context, GetParentRange(context));
        if (nodeList.empty()) {
            return resText;
        }
    }
    const auto &ctx = reinterpret_cast<public_lib::Context *>(context.context);
    bool needParams = false;
    std::vector<ParamTexts> params = FindExactFunctionParams(context, nodeList, needParams);
    resText += "\n";
    resText += GetFunctionNameAndType(context, refactorName);
    resText += GetFunctionParamsText(params);
    auto returnNode = FindReturnValueNode(nodeList);
    auto retNode = returnNode;
    if (returnNode->IsVariableDeclaration()) {
        returnNode = returnNode->AsVariableDeclaration()->Declarators()[0]->Id();
    } else if (returnNode->IsClassProperty()) {
        retNode = retNode->AsClassProperty()->Value();
        if (nodeList.size() > 1) {
            returnNode = returnNode->AsClassProperty()->Key();
        } else {
            returnNode = returnNode->AsClassProperty()->Value();
        }
    }
    auto returnText = GetSourceTextOfNodeFromSourceFile(ctx->sourceFile->source, returnNode);
    resText += ")";
    std::string returnTypeText = GetReturnTypeText(retNode);
    if (!returnTypeText.empty()) {
        resText += ": ";
        resText += returnTypeText;
    }
    resText += " {\n";
    resText += WriteFunctionBody(nodeList, returnNode, ctx);
    resText += "    return ";
    resText += returnText;
    resText += ";\n";
    resText += "}\n";
    return resText;
}
static bool IsEncloseVarConstBreak(ir::AstNode *parent)
{
    return parent != nullptr && (parent->IsBlockStatement() || parent->IsProgram() || parent->IsClassDeclaration());
}

static bool IsGlobalBreakForExtractSymbol(ir::AstNode *parent)
{
    return parent != nullptr && parent->IsProgram();
}

ir::AstNode *AdjustStatementForGlobalIfClass(ir::AstNode *node)
{
    if (node != nullptr && node->IsClassDeclaration()) {
        auto *cls = node->AsClassDeclaration();
        if (!cls->Definition()->Body().empty()) {
            return cls->Definition()->Body().at(0);
        }
    }
    return node;
}
ir::AstNode *GetFirstNodeWithoutImport(ir::AstNode *parent)
{
    ir::AstNode *node = nullptr;
    if (!parent->IsClassDeclaration()) {
        return nullptr;
    }
    auto nodeListToFirsElement = parent->AsClassDeclaration()->Definition()->Body();
    for (auto ndx : nodeListToFirsElement) {
        if (!ndx->IsImportSpecifier() && !ndx->IsImportDeclaration()) {
            node = ndx;
            break;
        }
    }
    return node;
}
static size_t LineColToPos(public_lib::Context *context, const size_t line, const size_t col)
{
    auto index = ark::es2panda::lexer::LineIndex(context->parserProgram->SourceCode());
    auto pos = index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, context->parserProgram));
    return pos;
}

size_t FindTopLevelInsertionPos(public_lib::Context *context, ir::AstNode *target, const std::string &actionName)
{
    ir::AstNode *statement = nullptr;
    for (ir::AstNode *node = target; node != nullptr; node = node->Parent()) {
        if (!node->IsStatement()) {
            continue;
        }
        statement = node;
        ir::AstNode *parent = node->Parent();
        if (actionName == std::string(EXTRACT_VARIABLE_ACTION_ENCLOSE.name) ||
            actionName == std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.name)) {
            if (IsEncloseVarConstBreak(parent)) {
                break;
            }
        }
        if (actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name) ||
            actionName == std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.name)) {
            if (IsGlobalBreakForExtractSymbol(parent)) {
                statement = GetFirstNodeWithoutImport(parent) ? GetFirstNodeWithoutImport(parent) : statement;
                break;
            }
        }
        if (actionName == std::string(EXTRACT_FUNCTION_ACTION_CLASS.name) ||
            actionName == std::string(EXTRACT_CONSTANT_ACTION_CLASS.name)) {
            if (IsGlobalBreakForExtractSymbol(parent)) {
                statement = AdjustStatementForGlobalIfClass(node);
                break;
            }
        }
    }
    if (statement == nullptr) {
        return 0;
    }
    if (statement->Start().line == 0 && statement->Start().index == 0) {
        return statement->Start().index;
    }
    return LineColToPos(context, statement->Start().line, statement->Start().index);
}

TextRange GetVarAndFunctionPosToWriteNode(const RefactorContext &context, const std::string &actionName,
                                          const size_t textSize)
{
    auto start = context.span.pos;
    auto startedNode = GetTouchingToken(context.context, start, false);
    auto ctx = reinterpret_cast<public_lib::Context *>(context.context);
    const auto startPos = FindTopLevelInsertionPos(ctx, startedNode, actionName);
    return {startPos, startPos + textSize};
}
TextRange GetCallPositionOfExtraction(const RefactorContext &context)
{
    auto nodeList = FindNodesInRange(context, context.span);
    if (nodeList.empty()) {
        nodeList = FindNodesInRange(context, GetParentRange(context));
        if (nodeList.empty()) {
            return {};
        }
    }
    size_t startIndex = nodeList[0]->Start().index;
    size_t endIndex = startIndex + context.span.end;
    for (int nodeIndex = nodeList.size() - 1; nodeIndex == 0; nodeIndex--) {
        if (nodeList[nodeIndex]->IsVariableDeclaration()) {
            startIndex = nodeList[nodeIndex]->AsVariableDeclaration()->Declarators().front()->Init()->Start().index;
            endIndex = startIndex + context.span.end;
        }
    }
    return {startIndex, endIndex};
}

std::string GetCallTextForFunction(const RefactorContext &context, const std::string &actionName)
{
    std::string funcCallText;
    auto nodeList = FindNodesInRange(context, context.span);
    if (nodeList.empty()) {
        nodeList = FindNodesInRange(context, GetParentRange(context));
        if (nodeList.empty()) {
            return funcCallText;
        }
    }
    bool needParams = false;
    std::vector<ParamTexts> params = FindExactFunctionParams(context, nodeList, needParams);
    std::string helperName;
    if (actionName == EXTRACT_FUNCTION_ACTION_CLASS.name) {
        helperName = GenerateUniqueFuncName(context, "newMethod", actionName);
        funcCallText = "this." + helperName + "(";
    } else {
        helperName = GenerateUniqueFuncName(context, "newFunction", actionName);
        funcCallText = helperName + "(";
    }
    if (needParams && !params.empty()) {
        for (auto &param : params) {
            funcCallText += param.paramNames;
            if (params.size() > 1 && param.paramNames != params[params.size() - 1].paramNames) {
                funcCallText += ", ";
            }
        }
    }
    funcCallText += ");";
    return funcCallText;
}

RefactorEditInfo GetRefactorEditsToExtractFunction(const RefactorContext &context, const std::string &extractedText,
                                                   const std::string &actionName)
{
    std::vector<FileTextChanges> edits;
    auto funcCallText = GetCallTextForFunction(context, actionName);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    TextChangesContext textChangesContext = *context.textChangesContext;
    const auto src = reinterpret_cast<public_lib::Context *>(context.context)->sourceFile;
    edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(src, GetVarAndFunctionPosToWriteNode(context, actionName, extractedText.size()).pos,
                           extractedText);
        tracker.ReplaceRangeWithText(src, GetCallPositionOfExtraction(context), funcCallText);
    });
    RefactorEditInfo refactorEdits(std::move(edits));
    return refactorEdits;
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

    auto node = GetTouchingToken(context.context, rangeToExtract.pos, false);
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

ir::AstNode *IsReplaceRangeRequired(const RefactorContext &context, ir::AstNode *ExtractedValsNode)
{
    if (ExtractedValsNode == nullptr) {
        return nullptr;
    }
    ir::AstNode *exprStmt = nullptr;
    for (ir::AstNode *parent = ExtractedValsNode->Parent(); parent != nullptr; parent = parent->Parent()) {
        if (parent->IsExpressionStatement() &&
            (parent->Start().index <= context.span.pos && parent->End().index >= context.span.end)) {
            exprStmt = parent;
        }
    }
    for (ir::AstNode *parent = ExtractedValsNode->Parent(); parent != nullptr; parent = parent->Parent()) {
        if (!(parent->IsCallExpression() || parent->IsAssignmentExpression())) {
            continue;
        }
        if ((parent->Start().index != context.span.pos || parent->End().index != context.span.end) ||
            exprStmt == nullptr) {
            return nullptr;
        }
    }
    return exprStmt;
}

RefactorEditInfo GetRefactorEditsToExtractVals(const RefactorContext &context, const std::string &extractedText,
                                               const std::string &actionName)
{
    std::vector<FileTextChanges> edits;
    TextChangesContext textChangesContext = *context.textChangesContext;
    const auto src = reinterpret_cast<public_lib::Context *>(context.context)->sourceFile;
    std::string extractedName = GenerateUniqueExtractedVarName(context, actionName);
    edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(src, GetVarAndFunctionPosToWriteNode(context, actionName, extractedText.size()).pos,
                           extractedText);
        auto ExtractedValsNode = FindExtractedVals(context);
        auto exprStmt = IsReplaceRangeRequired(context, ExtractedValsNode);
        if (exprStmt != nullptr) {
            tracker.DeleteRange(src, TextRange {exprStmt->Start().index, exprStmt->End().index});
            return;
        }
        tracker.ReplaceRangeWithText(src, GetCallPositionOfExtraction(context), extractedName);
    });
    RefactorEditInfo refactorEdits(std::move(edits));
    return refactorEdits;
}

std::string FindRefactor(const RefactorContext &context, const std::string &actionName)
{
    if (actionName == std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.name) ||
        actionName == std::string(EXTRACT_VARIABLE_ACTION_ENCLOSE.name) ||
        actionName == std::string(EXTRACT_CONSTANT_ACTION_CLASS.name)) {
        return CreateVariableRefactorText(context, actionName);
    }
    if (actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name) ||
        actionName == std::string(EXTRACT_FUNCTION_ACTION_CLASS.name)) {
        return CreateFunctionRefactorText(context, actionName);
    }
    return "";
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
    if (extractedText.empty()) {
        return nullptr;
    }
    RefactorEditInfo refactor;
    if (actionName == std::string(EXTRACT_CONSTANT_ACTION_GLOBAL.name) ||
        actionName == std::string(EXTRACT_CONSTANT_ACTION_ENCLOSE.name) ||
        actionName == std::string(EXTRACT_CONSTANT_ACTION_CLASS.name) ||
        actionName == std::string(EXTRACT_VARIABLE_ACTION_ENCLOSE.name)) {
        refactor = GetRefactorEditsToExtractVals(context, extractedText, actionName);
    } else if (actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name) ||
               actionName == std::string(EXTRACT_FUNCTION_ACTION_CLASS.name)) {
        refactor = GetRefactorEditsToExtractFunction(context, extractedText, actionName);
    }
    return std::make_unique<RefactorEditInfo>(refactor);
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ExtractSymbolRefactor> g_extractSymbolRefactorRegister("ExtractSymbolRefactor");

}  // namespace ark::es2panda::lsp
