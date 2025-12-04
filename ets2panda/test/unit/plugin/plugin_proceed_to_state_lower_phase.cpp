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
#include "util.h"
#include "public/es2panda_lib.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;
static std::string source = R"(
interface A{
    name: Record<string, string>
    age: Record<string, number>
}
let a: A = {
    name: {
        'name': 'aaa'
    },
    age: {
        'age': '90'
    }
})";

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return 1;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    impl = GetImpl();
    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    auto context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    impl->ProceedToState(context, ES2PANDA_STATE_LOWERED);

    CheckForErrors("LOWERED", context);
    impl->DestroyContext(context);
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
