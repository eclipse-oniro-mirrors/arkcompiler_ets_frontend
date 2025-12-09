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
#include <string>
#include <utility>

#include "libarkbase/os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

static auto source = std::string(
    "   function foo(lambda: (instance: string) => string):void {\n"
    "    console.log(lambda(\"ABC\"))\n"
    "   }\n"
    "   foo((instance: string) => { return instance })\n");

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

    auto *programNode = impl->ProgramAst(context, impl->ContextProgram(context));

    std::cout << impl->AstNodeDumpJSONConst(context, programNode) << std::endl;

    std::cout << impl->IsETSModule(programNode) << std::endl;

    std::size_t n = 0;
    es2panda_AstNode **statements = impl->BlockStatementStatements(context, programNode, &n);

    auto *expression = impl->ExpressionStatementGetExpression(context, statements[1]);

    statements[1] = impl->UpdateExpressionStatement(context, statements[1], expression);
    impl->AstNodeSetParent(context, expression, statements[1]);
    impl->BlockStatementSetStatements(context, programNode, statements, n);
    impl->AstNodeSetParent(context, statements[0], programNode);
    impl->AstNodeSetParent(context, statements[1], programNode);

    std::cout << impl->AstNodeDumpJSONConst(context, programNode) << std::endl;
    std::cout << impl->AstNodeDumpEtsSrcConst(context, programNode) << std::endl;

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
