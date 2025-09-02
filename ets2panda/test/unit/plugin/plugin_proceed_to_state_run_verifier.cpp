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

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include "util.h"
#include "public/es2panda_lib.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

static auto source = std::string("function main() { \nlet a = 5;\n arktest.assertEQ(a, 5);\n  }");

static es2panda_AstNode *letStatement = nullptr;
static es2panda_AstNode *assertStatement = nullptr;
static es2panda_AstNode *mainScriptFunc = nullptr;
static es2panda_Context *ctx = nullptr;

es2panda_AstNode *parNode;
es2panda_Context *newCtx;

static void FindLet(es2panda_AstNode *ast)
{
    if (!impl->IsVariableDeclaration(ast)) {
        impl->AstNodeIterateConst(ctx, ast, FindLet);
        return;
    }
    letStatement = ast;
}

static void FindAssert(es2panda_AstNode *ast)
{
    if (!IsAssertCall(ast)) {
        impl->AstNodeIterateConst(ctx, ast, FindAssert);
        return;
    }
    assertStatement = ast;
}

static void FindMainDef(es2panda_AstNode *ast)
{
    if (!impl->IsMethodDefinition(ast)) {
        impl->AstNodeIterateConst(ctx, ast, FindMainDef);
        return;
    }
    auto scriptFunc = impl->MethodDefinitionFunction(ctx, ast);
    if (scriptFunc == nullptr ||
        std::string("main") != impl->IdentifierName(ctx, impl->ScriptFunctionId(ctx, scriptFunc))) {
        impl->AstNodeIterateConst(ctx, ast, FindMainDef);
        return;
    }
    mainScriptFunc = scriptFunc;
}

static void changeParent(es2panda_AstNode *child)
{
    impl->AstNodeSetParent(newCtx, child, parNode);
}

static void SetRightParent(es2panda_AstNode *node, void *arg)
{
    es2panda_Context *context = static_cast<es2panda_Context *>(arg);
    newCtx = context;
    parNode = node;
    impl->AstNodeIterateConst(context, node, changeParent);
}

bool Verify(es2panda_Context *context)
{
    std::string checkerPhaseName = std::string("CheckerPhase");
    auto *memForCheckerPhaseName = static_cast<char *>(impl->AllocMemory(context, checkerPhaseName.size() + 1, 1));
    std::copy_n(checkerPhaseName.c_str(), checkerPhaseName.size() + 1, memForCheckerPhaseName);

    std::string scopesPhaseName = std::string("ScopesInitPhase");
    auto *memForScopesPhaseName = static_cast<char *>(impl->AllocMemory(context, scopesPhaseName.size() + 1, 1));
    std::copy_n(scopesPhaseName.c_str(), scopesPhaseName.size() + 1, memForScopesPhaseName);

    auto verifier = impl->CreateASTVerifier(context, impl->ContextProgram(context));
    impl->ASTVerifierIntroduceNewInvariants(context, verifier, memForScopesPhaseName);
    impl->ASTVerifierVerify(context, verifier, memForScopesPhaseName);
    impl->ASTVerifierIntroduceNewInvariants(context, verifier, memForCheckerPhaseName);
    impl->ASTVerifierVerify(context, verifier, memForCheckerPhaseName);
    return !impl->ASTVerifierHasErrorsConst(context, verifier);
}

static bool CheckVerifierOnChangedAst(es2panda_Context *context, es2panda_AstNode *ast)
{
    std::cout << impl->AstNodeDumpJSONConst(context, ast) << std::endl;
    size_t n = 0;
    ctx = context;
    impl->AstNodeIterateConst(context, ast, FindLet);
    impl->AstNodeIterateConst(context, ast, FindAssert);
    impl->AstNodeIterateConst(context, ast, FindMainDef);
    if (mainScriptFunc == nullptr || letStatement == nullptr || assertStatement == nullptr) {
        return false;
    }
    auto mainFuncBody = impl->ScriptFunctionBody(context, mainScriptFunc);
    auto mainStatements = impl->BlockStatementStatements(context, mainFuncBody, &n);
    auto assertStatementTest = AssertStatementTest(context, mainStatements[1]);

    std::string className = std::string("b");
    auto *memForName = static_cast<char *>(impl->AllocMemory(context, className.size() + 1, 1));
    std::copy_n(className.c_str(), className.size() + 1, memForName);

    auto varIdent = impl->CreateIdentifier1(context, memForName);
    auto assertIdent = impl->CreateIdentifier1(context, memForName);
    auto declarator = impl->CreateVariableDeclarator1(
        context, Es2pandaVariableDeclaratorFlag::VARIABLE_DECLARATOR_FLAG_LET, varIdent,
        impl->VariableDeclaratorInit(context, impl->VariableDeclarationDeclaratorsConst(context, letStatement, &n)[0]));
    auto declaration = impl->CreateVariableDeclaration(
        context, Es2pandaVariableDeclarationKind::VARIABLE_DECLARATION_KIND_LET, &declarator, 1);

    impl->BinaryExpressionSetLeft(context, assertStatementTest, assertIdent);
    auto newAssertStatement = CreateAssertStatement(context, assertStatementTest, nullptr);

    es2panda_AstNode *newMainStatements[2] = {declaration, newAssertStatement};
    impl->BlockStatementSetStatements(context, mainFuncBody, newMainStatements, 2U);

    if (Verify(context)) {
        return false;
    }

    impl->AstNodeForEach(ast, SetRightParent, context);
    impl->AstNodeRecheck(context, declaration);
    impl->AstNodeRecheck(context, newAssertStatement);

    return Verify(context);
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
    std::cout << "LOAD SUCCESS" << std::endl;
    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    auto context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);

    impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", context);

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    auto ast = impl->ProgramAst(context, impl->ContextProgram(context));
    if (!CheckVerifierOnChangedAst(context, ast)) {
        return TEST_ERROR_CODE;
    }

    impl->AstNodeRecheck(context, ast);

    impl->ProceedToState(context, ES2PANDA_STATE_LOWERED);
    CheckForErrors("LOWERED", context);

    impl->ProceedToState(context, ES2PANDA_STATE_ASM_GENERATED);
    CheckForErrors("ASM", context);

    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", context);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
