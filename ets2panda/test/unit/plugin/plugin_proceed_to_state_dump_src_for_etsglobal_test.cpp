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
let a: int = 11;
const b: int = 666;
a = 20;
foo()
function foo() {}
)";

static std::string expected = R"(
let a: int;

const b: int = 666;

function main() {}

function foo() {}

a = 20;
foo();
)";

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
    auto *program = impl->ContextProgram(context);
    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    auto *entryAst = impl->ProgramAst(context, program);
    [[maybe_unused]] std::string actual = impl->AstNodeDumpEtsSrcConst(context, entryAst);
    ASSERT(expected == actual);
    return 0;
}

// NOLINTEND