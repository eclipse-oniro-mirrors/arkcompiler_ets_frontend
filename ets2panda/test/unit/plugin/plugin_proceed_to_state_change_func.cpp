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
#include <iostream>
#include <ostream>
#include <string>

#include "libarkbase/os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

const int CANT_FIND_FUNC_ERROR = 3;
static const std::string mainName = "main";
static const std::string funcName1 = "foo";
static const std::string funcName2 = "goo";

static es2panda_Impl *impl = nullptr;

static es2panda_AstNode *FindMain(es2panda_AstNode *ast, es2panda_Context *ctx)
{
    size_t n = 0;
    auto **statements = impl->BlockStatementStatementsConst(ctx, ast, &n);
    es2panda_AstNode *mainScriptFunc = nullptr;
    for (size_t i = 0; i < n; i++) {
        if (!impl->IsFunctionDeclaration(statements[i])) {
            continue;
        }
        auto *scriptFunc = impl->FunctionDeclarationFunction(ctx, statements[i]);
        if (scriptFunc != nullptr && impl->ScriptFunctionId(ctx, scriptFunc) != nullptr &&
            impl->IdentifierName(ctx, impl->ScriptFunctionId(ctx, scriptFunc)) == mainName) {
            mainScriptFunc = scriptFunc;
            break;
        }
    }
    return mainScriptFunc;
}

static void ChangeCall(es2panda_AstNode *node, void *arg)
{
    es2panda_Context *ctx = static_cast<es2panda_Context *>(arg);
    if (impl->IsCallExpression(node)) {
        auto *callee = impl->CallExpressionCallee(ctx, node);
        if (callee == nullptr || !impl->IsIdentifier(impl->CallExpressionCallee(ctx, node))) {
            return;
        }
        std::string name = impl->IdentifierName(ctx, callee);
        if (name == funcName1) {
            auto *memForIdent = static_cast<char *>(impl->AllocMemory(ctx, funcName2.size() + 1, 1));
            std::copy_n(funcName2.c_str(), funcName2.size() + 1, memForIdent);
            auto *newIdent = impl->CreateIdentifier1(ctx, memForIdent);
            impl->CallExpressionSetCallee(ctx, node, newIdent);
        }
    }
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
    auto context = impl->CreateContextFromFile(config, argv[argc - 1]);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);

    auto *programNode = impl->ProgramAst(context, impl->ContextProgram(context));
    std::cout << std::endl << "SRC DUMP BEFORE CHANGE CALL:" << std::endl;
    std::cout << impl->AstNodeDumpEtsSrcConst(context, programNode) << std::endl << std::endl;
    auto *mainScriptFunc = FindMain(programNode, context);
    if (mainScriptFunc == nullptr) {
        std::cout << "CANT FIND MAIN FUNC" << std::endl;
        return CANT_FIND_FUNC_ERROR;
    }
    impl->AstNodeForEach(mainScriptFunc, ChangeCall, context);
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
