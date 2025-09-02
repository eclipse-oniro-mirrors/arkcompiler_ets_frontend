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
#include <vector>
#include "util.h"
#include "public/es2panda_lib.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

static std::string source = R"(
interface I {
    p: number;
}

interface J {
    p: number;
}

class A implements I {
    private prop: number = 4.0;

    get p(): number {
        return this.prop;
    }

    set p(value: number) {
        this.prop = value;
    }
}

class B implements J {
    private prop: number = 4.0;
    public p: number = 5.0;
}
)";

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

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    std::string dump = impl->AstNodeDumpEtsSrcConst(context, impl->ProgramAst(context, impl->ContextProgram(context)));
    std::cout << dump << std::endl;

    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        impl->DestroyContext(context);
        impl->DestroyConfig(config);
        return PROCEED_ERROR_CODE;
    }
    impl->DestroyContext(context);
    impl->DestroyConfig(config);

    /* Note (oeotvos) issue: #21301
    This causes massive problems for name mangling, because it runs es2panda on the dumped source
    code, which contains mangled names. When we chose the special character for mangling, the point was for it to not be
    parsable, so the user cannot use it in their code. Because of this I commented out the remaining part of the
    test.

    //  Rerun es2panda on dumped source
    config = impl->CreateConfig(argc - 1, args);
    context = impl->CreateContextFromString(config, dump.data(), argv[argc - 1]);
    if (context != nullptr) {
        std::cout << "CREATE CONTEXT SUCCESS" << std::endl;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        impl->DestroyContext(context);
        impl->DestroyConfig(config);
        return PROCEED_ERROR_CODE;
    }
    impl->DestroyContext(context);
    impl->DestroyConfig(config); */

    return 0;
}

// NOLINTEND
