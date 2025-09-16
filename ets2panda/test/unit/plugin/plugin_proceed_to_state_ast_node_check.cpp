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

static auto source = std::string("function main(): void { let v = 1; v + 2 }");

static es2panda_AstNode *binExpr = nullptr;
static es2panda_Context *ctx = nullptr;

es2panda_AstNode *parNode;
es2panda_Context *newCtx;

static void FindBinaryExpression(es2panda_AstNode *ast)
{
    if (!impl->IsBinaryExpression(ast)) {
        impl->AstNodeIterateConst(ctx, ast, FindBinaryExpression);
        return;
    }
    binExpr = ast;
}

static bool TestAstNodeCheck(es2panda_Context *context, es2panda_AstNode *root)
{
    std::cout << impl->AstNodeDumpJSONConst(context, root) << std::endl;
    ctx = context;
    impl->AstNodeIterateConst(context, root, FindBinaryExpression);
    if (binExpr == nullptr) {
        return false;
    }

    auto *mainType = impl->AstNodeCheck(context, binExpr);
    std::cout << impl->TypeToStringConst(context, mainType) << std::endl;
    return impl->TypeIsETSObjectType(mainType);  // boxed Int
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return 1;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    impl = GetImpl();
    std::cout << "LOAD SUCCESS" << std::endl;
    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    auto context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context != nullptr) {
        std::cout << "CREATE CONTEXT SUCCESS" << std::endl;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);

    impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", context);

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    auto root = impl->ProgramAst(context, impl->ContextProgram(context));
    if (!TestAstNodeCheck(context, root)) {
        return TEST_ERROR_CODE;
    }

    impl->AstNodeRecheck(context, root);

    impl->ProceedToState(context, ES2PANDA_STATE_LOWERED);
    CheckForErrors("LOWERED", context);

    impl->ProceedToState(context, ES2PANDA_STATE_ASM_GENERATED);
    CheckForErrors("ASM", context);

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
