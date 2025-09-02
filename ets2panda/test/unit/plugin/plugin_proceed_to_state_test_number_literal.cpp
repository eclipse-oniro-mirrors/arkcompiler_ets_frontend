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
#include "util.h"
#include "public/es2panda_lib.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

static auto source = std::string("function main() { arktest.assertEQ(5, 10); }");

static es2panda_AstNode *assertStatement = nullptr;
static es2panda_Context *ctx = nullptr;

static void FindAssert(es2panda_AstNode *ast)
{
    if (!IsAssertCall(ast)) {
        impl->AstNodeIterateConst(ctx, ast, FindAssert);
        return;
    }
    assertStatement = ast;
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

    auto Ast = impl->ProgramAst(context, impl->ContextProgram(context));
    ctx = context;
    FindAssert(Ast);
    if (assertStatement == nullptr) {
        return TEST_ERROR_CODE;
    }
    auto binExpr = AssertStatementTest(context, assertStatement);
    auto newNumLiteral = impl->CreateNumberLiteral(context, 10);
    impl->BinaryExpressionSetLeft(context, binExpr, newNumLiteral);
    impl->AstNodeSetParent(context, newNumLiteral, binExpr);
    std::cout << impl->AstNodeDumpJSONConst(context, binExpr) << std::endl;
    std::cout << impl->AstNodeDumpEtsSrcConst(context, Ast) << std::endl;

    impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", context);

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    impl->AstNodeRecheck(context, Ast);

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
