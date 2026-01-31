/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
#include <vector>
#include "util.h"
#include "public/es2panda_lib.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

static bool CheckExternalSources(es2panda_Context *context, es2panda_Program *pgm)
{
    size_t n = 0;
    auto externalSources = impl->ProgramExternalSources(context, pgm, &n);
    if (externalSources == nullptr) {
        return false;
    }
    bool findImport = false;
    for (size_t i = 0; i < n; i++) {
        size_t j = 0;
        auto extPrograms = impl->ExternalSourcePrograms(externalSources[i], &j);
        for (size_t k = 0; k < j; k++) {
            if (impl->ProgramIsStdLibConst(context, extPrograms[k])) {
                continue;
            }
            findImport |= std::string(impl->ProgramFileNameConst(context, extPrograms[k])) == "export";
        }
    }
    return findImport;
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
    auto context = impl->CreateContextFromFile(config, argv[argc - 1]);
    if (context != nullptr) {
        std::cout << "CREATE CONTEXT SUCCESS" << std::endl;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);

    impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", context);

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    auto pgm = impl->ContextProgram(context);
    if (!CheckExternalSources(context, pgm)) {
        return TEST_ERROR_CODE;
    }

    impl->AstNodeRecheck(context, impl->ProgramAst(context, pgm));

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
