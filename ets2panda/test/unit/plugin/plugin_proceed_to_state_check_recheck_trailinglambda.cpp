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

#include <iostream>
#include <ostream>
#include <string>
#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN
static std::string source = R"(
function foo(cb: ()=>void) {}
function foo2(p1?: string, p2?: ()=>void) {}

foo() {}
foo2() {}
)";

static es2panda_Impl *impl = nullptr;
es2panda_Context *context = nullptr;
static es2panda_AstNode *targetLambdaForFoo = nullptr;
static es2panda_AstNode *transferredTrailingLambdaForFoo = nullptr;
static void FindTransferredTrailingLambda(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsCallExpression(ast)) {
        return;
    }
    auto *callee = impl->CallExpressionCallee(context, ast);
    if (callee == nullptr || !impl->IsIdentifier(callee)) {
        return;
    }

    auto name = std::string(impl->IdentifierName(context, callee));
    if (name != "foo") {
        return;
    }

    size_t len = 0;
    auto argumentList = impl->CallExpressionArgumentsConst(context, ast, &len);
    targetLambdaForFoo = argumentList[0];
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

    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }
    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    auto *program = impl->ContextProgram(context);
    es2panda_AstNode *programNode = impl->ProgramAst(context, program);
    impl->AstNodeForEach(programNode, FindTransferredTrailingLambda, context);
    if (targetLambdaForFoo == nullptr) {
        return TEST_ERROR_CODE;
    }

    // save the result before recheck.
    transferredTrailingLambdaForFoo = targetLambdaForFoo;
    targetLambdaForFoo = nullptr;

    impl->AstNodeRecheck(context, programNode);
    CheckForErrors("RECHECKED", context);
    impl->AstNodeForEach(programNode, FindTransferredTrailingLambda, context);
    if (targetLambdaForFoo != transferredTrailingLambdaForFoo) {
        return TEST_ERROR_CODE;
    }

    impl->DestroyConfig(config);
    return 0;
}

// NOLINTEND