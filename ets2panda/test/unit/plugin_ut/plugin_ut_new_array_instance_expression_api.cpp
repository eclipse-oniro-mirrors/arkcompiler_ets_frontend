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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or g_implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <ostream>
#include <string>
#include "test/unit/plugin/util.h"
#include "public/es2panda_lib.h"

// NOLINTBEGIN
/*
 * Covered C API List:
 *   AstNodeDumpEtsSrcConst
 *   AstNodeDumpJSONConst
 *   AstNodeForEach
 *   AstNodeIterateConst
 *   AstNodeRebind
 *   AstNodeRecheck
 *   AstNodeSetParent
 *   BlockStatementSetStatements
 *   ContextProgram
 *   ContextState
 *   CreateBlockStatement
 *   CreateConfig
 *   CreateContextFromString
 *   CreateDoubleType1
 *   CreateETSNewArrayInstanceExpression
 *   CreateETSPrimitiveType
 *   CreateETSTypeReference
 *   CreateETSTypeReferencePart1
 *   CreateExpressionStatement
 *   CreateFunctionSignature
 *   CreateIdentifier1
 *   CreateNumberLiteral
 *   CreateScriptFunction
 *   CreateSignature
 *   DestroyConfig
 *   DestroyContext
 *   ETSNewArrayInstanceExpressionDimension
 *   ETSNewArrayInstanceExpressionDimensionConst
 *   ETSNewArrayInstanceExpressionSetDimension
 *   ETSNewArrayInstanceExpressionSetSignature
 *   ETSNewArrayInstanceExpressionSignature
 *   ETSNewArrayInstanceExpressionSignatureConst
 *   ETSNewArrayInstanceExpressionTypeReference
 *   ETSNewArrayInstanceExpressionTypeReferenceConst
 *   FunctionDeclarationFunction
 *   IdentifierName
 *   IsETSNewArrayInstanceExpression
 *   IsFunctionDeclaration
 *   ProceedToState
 *   ProgramAst
 *   ScriptFunctionBody
 *   ScriptFunctionId
 *   UpdateETSNewArrayInstanceExpression
 */

static std::string strInit {""};

static std::string g_source = R"(
function main() {
    new number[5](0);
}
)";

static es2panda_Impl *g_impl = nullptr;
static es2panda_AstNode *arrayExpr = nullptr;
static es2panda_AstNode *mainScriptFunc = nullptr;
static es2panda_Context *g_ctx = nullptr;

es2panda_AstNode *parNode = nullptr;
es2panda_Context *newCtx = nullptr;

static void FindArrayExpression(es2panda_AstNode *ast)
{
    if (!g_impl->IsETSNewArrayInstanceExpression(ast)) {
        g_impl->AstNodeIterateConst(g_ctx, ast, FindArrayExpression);
        return;
    }
    arrayExpr = ast;
}

static void FindMainDef(es2panda_AstNode *ast)
{
    if (!g_impl->IsFunctionDeclaration(ast)) {
        g_impl->AstNodeIterateConst(g_ctx, ast, FindMainDef);
        return;
    }
    auto scriptFunc = g_impl->FunctionDeclarationFunction(g_ctx, ast);
    if (scriptFunc == nullptr ||
        std::string("main") != g_impl->IdentifierName(g_ctx, g_impl->ScriptFunctionId(g_ctx, scriptFunc))) {
        return;
    }
    mainScriptFunc = scriptFunc;
}

static void changeParent(es2panda_AstNode *child)
{
    g_impl->AstNodeSetParent(newCtx, child, parNode);
}

static void SetRightParent(es2panda_AstNode *node, void *arg)
{
    es2panda_Context *context = static_cast<es2panda_Context *>(arg);
    newCtx = context;
    parNode = node;
    g_impl->AstNodeIterateConst(context, node, changeParent);
}

static bool SetAndGetETSArrayExpressionValue(es2panda_Context *context)
{
    es2panda_AstNode *dimension = g_impl->CreateNumberLiteral(context, 8);
    g_impl->ETSNewArrayInstanceExpressionSetDimension(context, arrayExpr, dimension);
    es2panda_SignatureInfo *signatureInfo = nullptr;
    es2panda_Type *doubleType1 = g_impl->CreateDoubleType1(context, 6);
    es2panda_AstNode *voidType = g_impl->CreateETSPrimitiveType(context, PRIMITIVE_TYPE_VOID);
    es2panda_FunctionSignature *sig = g_impl->CreateFunctionSignature(context, nullptr, nullptr, 0, voidType, false);
    es2panda_AstNode *body = g_impl->CreateBlockStatement(context, nullptr, 0);
    es2panda_AstNode *func = g_impl->CreateScriptFunction(context, body, sig, 0, 0);
    es2panda_Signature *signatureInit = g_impl->CreateSignature(context, signatureInfo, doubleType1, func);
    g_impl->ETSNewArrayInstanceExpressionSetSignature(context, arrayExpr, signatureInit);
    es2panda_AstNode *type = g_impl->ETSNewArrayInstanceExpressionTypeReference(context, arrayExpr);
    if (!(strcmp(g_impl->AstNodeDumpEtsSrcConst(context, type), "string") == 0)) {
        return false;
    }
    const es2panda_AstNode *typeConst = g_impl->ETSNewArrayInstanceExpressionTypeReferenceConst(context, arrayExpr);
    if (!(strcmp(g_impl->AstNodeDumpEtsSrcConst(context, const_cast<es2panda_AstNode *>(typeConst)), "string") == 0)) {
        return false;
    }
    es2panda_AstNode *resultDimension = g_impl->ETSNewArrayInstanceExpressionDimension(context, arrayExpr);
    if (!(strcmp(g_impl->AstNodeDumpEtsSrcConst(context, resultDimension), "8") == 0)) {
        return false;
    }
    const es2panda_AstNode *dimensionConst = g_impl->ETSNewArrayInstanceExpressionDimensionConst(context, arrayExpr);
    if (!(strcmp(g_impl->AstNodeDumpEtsSrcConst(context, const_cast<es2panda_AstNode *>(dimensionConst)), "8") == 0)) {
        return false;
    }
    es2panda_Signature *signature = g_impl->ETSNewArrayInstanceExpressionSignature(context, arrayExpr);
    if (signature != signatureInit) {
        return false;
    }
    const es2panda_Signature *signatureConst = g_impl->ETSNewArrayInstanceExpressionSignatureConst(context, arrayExpr);
    if (signatureConst != signatureInit) {
        return false;
    }
    return true;
}

/*
 *  new number[5](0)
 *  is changed to
 *  new string[10]("")
 */
static bool ETSArrayExpressionHandle(es2panda_Context *context, es2panda_AstNode *ast)
{
    es2panda_AstNode *arg1 = g_impl->CreateIdentifier1(context, const_cast<char *>("string"));
    es2panda_AstNode *part1 = g_impl->CreateETSTypeReferencePart1(context, arg1);
    es2panda_AstNode *typeReference = g_impl->CreateETSTypeReference(context, part1);
    es2panda_AstNode *dimensionInit = g_impl->CreateNumberLiteral(context, 1);
    es2panda_AstNode *initializer = g_impl->CreateStringLiteral1(context, strInit.data());
    es2panda_AstNode *newArrayExpression =
        g_impl->CreateETSNewArrayInstanceExpression(context, typeReference, dimensionInit, initializer);
    if (!g_impl->IsETSNewArrayInstanceExpression(newArrayExpression)) {
        return false;
    }
    g_impl->AstNodeIterateConst(context, ast, FindArrayExpression);
    g_impl->AstNodeIterateConst(context, ast, FindMainDef);
    if (mainScriptFunc == nullptr || arrayExpr == nullptr) {
        return false;
    }

    auto mainFuncBody = g_impl->ScriptFunctionBody(context, mainScriptFunc);
    es2panda_AstNode *exprStatement = g_impl->CreateExpressionStatement(context, newArrayExpression);
    es2panda_AstNode *newMainStatements[1] = {exprStatement};
    g_impl->BlockStatementSetStatements(context, mainFuncBody, newMainStatements, 1U);
    g_impl->AstNodeForEach(ast, SetRightParent, context);
    g_impl->AstNodeIterateConst(context, ast, FindArrayExpression);

    if (!SetAndGetETSArrayExpressionValue(context)) {
        return false;
    }
    es2panda_AstNode *argUpdate = g_impl->CreateIdentifier1(context, const_cast<char *>("string"));
    es2panda_AstNode *partUpdate = g_impl->CreateETSTypeReferencePart1(context, argUpdate);
    es2panda_AstNode *typeReferenceUpdate = g_impl->CreateETSTypeReference(context, partUpdate);
    es2panda_AstNode *dimensionUpdate = g_impl->CreateNumberLiteral(context, 10);
    es2panda_AstNode *initializerUpdate = g_impl->CreateStringLiteral1(context, strInit.data());
    es2panda_AstNode *newArrayExpressionUpdate = g_impl->UpdateETSNewArrayInstanceExpression(
        context, arrayExpr, typeReferenceUpdate, dimensionUpdate, initializerUpdate);

    exprStatement = g_impl->CreateExpressionStatement(context, newArrayExpressionUpdate);
    newMainStatements[0] = {exprStatement};
    g_impl->BlockStatementSetStatements(context, mainFuncBody, newMainStatements, 1U);
    g_impl->AstNodeForEach(ast, SetRightParent, context);
    std::string str(g_impl->AstNodeDumpEtsSrcConst(context, ast));
    if (str.find("new string[10](\"\");") == std::string::npos) {
        std::cout << "Fail to find update array value." << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }

    g_impl = GetImpl();

    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = g_impl->CreateConfig(argc - 1, args);

    g_ctx = g_impl->CreateContextFromString(config, g_source.data(), argv[argc - 1]);
    if (g_ctx == nullptr) {
        std::cerr << "Fail to create context." << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    int retCode = Test(g_ctx, g_impl, ES2PANDA_STATE_PARSED, ETSArrayExpressionHandle);
    if (retCode != 0) {
        g_impl->DestroyConfig(config);
        return retCode;
    }

    g_impl->DestroyConfig(config);

    return 0;
}
// NOLINTEND
