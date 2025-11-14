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

#include "os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN
static es2panda_Impl *impl = nullptr;
static std::string g_source = R"(
type testAlias = () => void;
export interface I {
    testBuilder: testAlias;
}
function foo(testVar: I) {
    const cb: () => void = (): void => {
        try {
            testVar.testBuilder()
        } finally {}
    }
}
)";

static es2panda_AstNode *targetId = nullptr;
static void FindIdentifierOfTargetMemberExpr(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsCallExpression(ast)) {
        return;
    }

    auto maybeMemberExpr = impl->CallExpressionCallee(ctx, ast);
    if (!impl->IsMemberExpression(maybeMemberExpr)) {
        return;
    }

    auto prop = impl->MemberExpressionProperty(ctx, maybeMemberExpr);
    if (!impl->IsIdentifier(prop)) {
        return;
    }

    auto name = std::string(impl->IdentifierName(ctx, prop));
    if (name == "testBuilder") {
        targetId = prop;
    }
}

static void FindTargetAstAfterChecker(es2panda_Context *context, es2panda_AstNode *ast)
{
    impl->AstNodeForEach(ast, FindIdentifierOfTargetMemberExpr, context);
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    impl = GetImpl();
    if (impl == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }

    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    auto context = impl->CreateContextFromString(config, g_source.data(), argv[argc - 1]);
    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);
    auto *program = impl->ContextProgram(context);
    auto *entryAst = impl->ProgramAst(context, program);
    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECK", context);
    FindTargetAstAfterChecker(context, entryAst);
    ASSERT(impl->DeclarationFromIdentifier(context, targetId) != nullptr);
    return 0;
}

// NOLINTEND