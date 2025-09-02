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
#include <cstring>

#include "os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

/*
 * Covered C API List:
 *   AstNodeDumpEtsSrcConst
 *   AstNodeIterateConst
 *   AstNodeSetParent
 *   BlockStatementSetStatements
 *   ContextProgram
 *   CreateConfig
 *   CreateContextFromFile
 *   CreateContextFromString
 *   CreateReturnStatement1
 *   CreateStringLiteral1
 *   DestroyConfig
 *   IdentifierName
 *   IsBlockStatement
 *   IsScriptFunction
 *   ProceedToState
 *   ProgramAst
 *   ScriptFunctionBody
 *   ScriptFunctionId
 */

// NOLINTBEGIN
static std::string source = R"(
function foo(): string {
}
let x = foo();
arktest.assertEQ(x, "hello");
)";

static es2panda_Impl *impl = nullptr;
static es2panda_Context *context = nullptr;
static es2panda_AstNode *fooFunction = nullptr;

static es2panda_AstNode *CreateReturnStatement()
{
    es2panda_AstNode *arg = impl->CreateStringLiteral1(context, const_cast<char *>("hello"));
    es2panda_AstNode *returnStat = impl->CreateReturnStatement1(context, arg);
    impl->AstNodeSetParent(context, arg, returnStat);
    return returnStat;
}

static void FindFooFunction(es2panda_AstNode *node)
{
    if (!impl->IsScriptFunction(node)) {
        impl->AstNodeIterateConst(context, node, FindFooFunction);
        return;
    }
    auto name = impl->ScriptFunctionId(context, node);
    if (strcmp(impl->IdentifierName(context, name), "foo") == 0) {
        fooFunction = node;
    }
    impl->AstNodeIterateConst(context, node, FindFooFunction);
}

static int CheckReturnStatement()
{
    es2panda_AstNode *returnStat = CreateReturnStatement();
    es2panda_AstNode *body = impl->ScriptFunctionBody(context, fooFunction);
    if (!impl->IsBlockStatement(body)) {
        return TEST_ERROR_CODE;
    }
    impl->AstNodeSetParent(context, returnStat, body);
    impl->BlockStatementSetStatements(context, body, &returnStat, 1);
    return 0;
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
    context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);

    es2panda_AstNode *programNode = impl->ProgramAst(context, impl->ContextProgram(context));

    FindFooFunction(programNode);

    int code = CheckReturnStatement();
    if (code != 0) {
        std::cerr << "FAILED TO CHECK RETURN STATEMENT" << std::endl;
        return code;
    }

    std::cout << "PROGRAM NODE: \n" << impl->AstNodeDumpEtsSrcConst(context, programNode) << std::endl;

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
