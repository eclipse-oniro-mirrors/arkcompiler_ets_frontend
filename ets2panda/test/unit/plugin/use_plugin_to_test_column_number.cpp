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

#include <cstdint>
#include <memory>
#include "libarkbase/macros.h"
#include "libarkbase/os/file.h"
#include "public/es2panda_lib.h"
#include "util.h"
#include "parser/program/program.h"
#include "ir/statements/blockStatement.h"
#include "ir/astNode.h"
#include "ir/statements/classDeclaration.h"
#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "util/options.h"
#include "ir/statements/functionDeclaration.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

namespace {
constexpr std::size_t COL_EXPECT = 13;

const std::string SOURCE_CODE =
    "   function foo():void {\n"
    "   }\n"
    "   class A{}\n";

}  // namespace

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
    auto context = impl->CreateContextFromString(config, SOURCE_CODE.data(), argv[argc - 1]);
    if (context == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);

    auto program = impl->ContextProgram(context);
    auto programPtr = reinterpret_cast<ark::es2panda::parser::Program *>(program);

    auto range = programPtr->Ast()->Statements()[0]->AsFunctionDeclaration()->Function()->Id()->Range();
    int res = 0;
    if (COL_EXPECT != impl->SourcePositionCol(context, (es2panda_SourcePosition *)(&range.start))) {
        res = TEST_ERROR_CODE;
    }

    impl->DestroyConfig(config);
    return res;
}

// NOLINTEND