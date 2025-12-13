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

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>

#include "libarkbase/os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static const std::string funcName = "foo";

static es2panda_Impl *impl = nullptr;

es2panda_AstNode *parNode;
es2panda_Context *newCtx;

static void changeParent(es2panda_AstNode *child)
{
    impl->AstNodeSetParent(newCtx, child, parNode);
}

static void SetRightParent(es2panda_AstNode *node, void *arg)
{
    es2panda_Context *ctx = static_cast<es2panda_Context *>(arg);
    newCtx = ctx;
    parNode = node;
    impl->AstNodeIterateConst(ctx, node, changeParent);
}

static void UpdateCall(es2panda_AstNode *ast, es2panda_Context *ctx)
{
    size_t n = 0;
    auto **statements = impl->BlockStatementStatementsConst(ctx, ast, &n);
    auto *exprStatement = statements[1];
    auto *callExpr = const_cast<es2panda_AstNode *>(impl->ExpressionStatementGetExpressionConst(ctx, exprStatement));
    auto *arrowFunc = impl->CallExpressionArgumentsConst(ctx, callExpr, &n)[0];
    auto *body = const_cast<es2panda_AstNode *>(
        impl->ScriptFunctionBodyConst(ctx, impl->ArrowFunctionExpressionFunction(ctx, arrowFunc)));
    auto funcSignature = impl->CreateFunctionSignature(
        ctx, nullptr, nullptr, 0, impl->CreateETSPrimitiveType(ctx, Es2pandaPrimitiveType::PRIMITIVE_TYPE_VOID), false);
    auto *scriptFunc = impl->CreateScriptFunction(ctx, body, std::move(funcSignature),
                                                  Es2pandaScriptFunctionFlags::SCRIPT_FUNCTION_FLAGS_ARROW, 0);
    impl->AstNodeSetParent(ctx, body, scriptFunc);
    auto *newArrowFunc = impl->CreateArrowFunctionExpression(ctx, scriptFunc);
    auto *newId =
        impl->UpdateIdentifier1(ctx, impl->CallExpressionCallee(ctx, callExpr), const_cast<char *>(funcName.c_str()));
    auto *newCallExpr = impl->UpdateCallExpression(ctx, callExpr, newId, &newArrowFunc, 1, nullptr, false, false);
    impl->AstNodeSetParent(ctx, newId, newCallExpr);
    impl->AstNodeSetParent(ctx, newArrowFunc, newCallExpr);
    auto *newStatement = impl->UpdateExpressionStatement(ctx, exprStatement, newCallExpr);
    impl->AstNodeSetParent(ctx, newCallExpr, newStatement);
    es2panda_AstNode *newStatements[] = {statements[0], newStatement};
    const int numOfNewStatements = 2;
    impl->BlockStatementSetStatements(ctx, ast, newStatements, numOfNewStatements);
    impl->AstNodeForEach(ast, SetRightParent, ctx);
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
    auto src = std::string("function foo(builder: () => void) {}\nfoo(() => {})");
    auto context = impl->CreateContextFromString(config, src.c_str(), argv[argc - 1]);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);

    auto *programNode = impl->ProgramAst(context, impl->ContextProgram(context));
    std::cout << std::endl << "SRC DUMP BEFORE CHANGE CALL:" << std::endl;
    std::cout << impl->AstNodeDumpEtsSrcConst(context, programNode) << std::endl << std::endl;
    UpdateCall(programNode, context);
    std::cout << std::endl << "SRC DUMP AFTER CHANGE CALL:" << std::endl;
    std::cout << impl->AstNodeDumpEtsSrcConst(context, programNode) << std::endl << std::endl;

    impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", context);

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    impl->AstNodeRecheck(context, programNode);

    impl->ProceedToState(context, ES2PANDA_STATE_LOWERED);
    CheckForErrors("LOWERED", context);

    impl->ProceedToState(context, ES2PANDA_STATE_ASM_GENERATED);
    CheckForErrors("ASM", context);

    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", context);
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
