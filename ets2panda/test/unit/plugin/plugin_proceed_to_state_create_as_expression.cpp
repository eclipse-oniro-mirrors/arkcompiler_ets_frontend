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

// Test case: run plugin in .ts mode should not crash with Segfault.

#include <iostream>
#include <cstring>

#include "public/es2panda_lib.h"
#include "util.h"

/*
 * Found unique words after 'impl->' and before '(':
 * - AstNodeDumpEtsSrcConst
 * - AstNodeDumpJSONConst
 * - AstNodeIterateConst
 * - AstNodeRecheck
 * - AstNodeSetParent
 * - ContextProgram
 * - ContextState
 * - CreateConfig
 * - CreateContextFromString
 * - CreateETSPrimitiveType
 * - CreateNumberLiteral
 * - CreateTSAsExpression
 * - DestroyConfig
 * - DestroyContext
 * - IdentifierName
 * - IsMethodDefinition
 * - IsVariableDeclaration
 * - IsVariableDeclarator
 * - MethodDefinitionFunction
 * - ProceedToState
 * - ProgramAst
 * - ScriptFunctionBody
 * - ScriptFunctionId
 * - VariableDeclaratorId
 * - VariableDeclaratorSetInit
 *
 * Total unique words found: 25
 */

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;
static es2panda_AstNode *program = nullptr;
static es2panda_AstNode *scriptFunc = nullptr;
static es2panda_Context *context = nullptr;
static const char *funcName = "main";
static const char *varName = "a";
static es2panda_AstNode *variableDeclarator = nullptr;
static es2panda_AstNode *letStatement = nullptr;

static std::string originalSourceCode = R"(
function main() : void {
    let a = 1
}
)";
static void FindLet(es2panda_AstNode *node)
{
    if (!impl->IsVariableDeclaration(node)) {
        impl->AstNodeIterateConst(context, node, FindLet);
        return;
    }
    letStatement = node;
}

static void FindFuncDef(es2panda_AstNode *ast)
{
    if (!impl->IsMethodDefinition(ast)) {
        impl->AstNodeIterateConst(context, ast, FindFuncDef);
        return;
    }
    auto tmpScriptFunc = impl->MethodDefinitionFunction(context, ast);
    if (tmpScriptFunc == nullptr ||
        std::string(funcName) != impl->IdentifierName(context, impl->ScriptFunctionId(context, tmpScriptFunc))) {
        impl->AstNodeIterateConst(context, ast, FindFuncDef);
        return;
    }
    scriptFunc = tmpScriptFunc;
}

static void FindVariableDeclarator(es2panda_AstNode *ast)
{
    if (!impl->IsVariableDeclarator(ast)) {
        impl->AstNodeIterateConst(context, ast, FindVariableDeclarator);
        return;
    }
    if (std::string(varName) != impl->IdentifierName(context, impl->VariableDeclaratorId(context, ast))) {
        impl->AstNodeIterateConst(context, ast, FindVariableDeclarator);
        return;
    }
    variableDeclarator = ast;
}

void CreateAsExpression()
{
    impl->AstNodeIterateConst(context, program, FindFuncDef);
    if (scriptFunc == nullptr) {
        return;
    }
    auto funcBody = impl->ScriptFunctionBody(context, scriptFunc);
    impl->AstNodeIterateConst(context, funcBody, FindLet);
    if (letStatement == nullptr) {
        return;
    }
    impl->AstNodeIterateConst(context, funcBody, FindVariableDeclarator);
    if (variableDeclarator == nullptr) {
        return;
    }
    auto numberLiteral = impl->CreateNumberLiteral(context, 1);
    Es2pandaPrimitiveType primitiveType = PRIMITIVE_TYPE_SHORT;
    auto typeAnnotation = impl->CreateETSPrimitiveType(context, primitiveType);
    auto *asExpr = impl->CreateTSAsExpression(context, numberLiteral, typeAnnotation, true);
    impl->VariableDeclaratorSetInit(context, variableDeclarator, asExpr);
    impl->AstNodeSetParent(context, numberLiteral, asExpr);
    impl->AstNodeSetParent(context, typeAnnotation, asExpr);
    impl->AstNodeSetParent(context, asExpr, variableDeclarator);
    impl->AstNodeSetParent(context, variableDeclarator, letStatement);
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    impl = GetImpl();

    auto config = impl->CreateConfig(argc - 1, argv + 1);
    context = impl->CreateContextFromString(config, originalSourceCode.c_str(), argv[argc - 1]);

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    program = impl->ProgramAst(context, impl->ContextProgram(context));

    CreateAsExpression();

    const char *src = impl->AstNodeDumpEtsSrcConst(context, program);
    const char *expected = "1 as short";
    if (strstr(src, expected) == nullptr) {
        return 1;
    }

    impl->AstNodeRecheck(context, program);

    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", context);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }
    impl->DestroyContext(context);
    impl->DestroyConfig(config);
    return 0;
}

// NOLINTEND
