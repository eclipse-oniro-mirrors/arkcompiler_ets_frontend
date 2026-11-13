/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

/*
 * Covered C API List:
 *   bool (*ProgramIsFromABC)(es2panda_Context *context, const es2panda_Program *program);
 *   es2panda_Program *(*ContextProgram)(es2panda_Context *context);
 *   es2panda_Context *(*ProceedToState)(es2panda_Context *context, es2panda_ContextState state);
 *   es2panda_ExternalSource **(*ProgramExternalSources)(es2panda_Context *context, es2panda_Program *program,
 *                                                        size_t *lenP);
 *   char const *(*ExternalSourceName)(es2panda_ExternalSource *e_source);
 *   es2panda_Program **(*ExternalSourcePrograms)(es2panda_ExternalSource *e_source, size_t *len_p);
 */

namespace {
// these FAIL_CODE does not share with other files.
constexpr int PROGRAM_IS_FROM_ABC_FAIL_CODE = 0x15;
constexpr int EXTERNAL_SOURCE_FROM_ABC_FAIL_CODE = 0x16;

es2panda_Impl *g_impl = nullptr;
}  // namespace

// 'ProgramIsFromABC' must tolerate nullptr arguments and return 'false' for them.
bool CheckProgramIsFromAbcNullArguments(es2panda_Context *ctx, es2panda_Program *program)
{
    return !g_impl->ProgramIsFromABC(nullptr, program) && !g_impl->ProgramIsFromABC(ctx, nullptr);
}

// The main program is compiled from a source file and its module is not listed in 'externalDeclPackages'.
bool CheckMainProgramIsNotFromAbc(es2panda_Context *ctx)
{
    g_impl->ProceedToState(ctx, ES2PANDA_STATE_PARSED);
    auto *program = g_impl->ContextProgram(ctx);
    if (program == nullptr) {
        return false;
    }
    return !g_impl->ProgramIsFromABC(ctx, program);
}

// External sources listed in 'externalDeclPackages' must be reported as programs from ABC,
// while all the others must not.
bool CheckExternalSourcesFromAbc(es2panda_Context *ctx)
{
    g_impl->ProceedToState(ctx, ES2PANDA_STATE_BOUND);
    auto *program = g_impl->ContextProgram(ctx);
    if (program == nullptr) {
        return false;
    }

    size_t externalSourceCnt {0};
    auto **externalSourceList = g_impl->ProgramExternalSources(ctx, program, &externalSourceCnt);
    if (externalSourceList == nullptr || externalSourceCnt == 0) {
        return false;
    }

    bool fromAbcSeen = false;
    bool notFromAbcSeen = false;
    for (size_t i = 0; i < externalSourceCnt; i++) {
        const char *externalSourceName = g_impl->ExternalSourceName(externalSourceList[i]);
        if (externalSourceName == nullptr) {
            return false;
        }
        size_t externalProgramCount {0};
        auto **programList = g_impl->ExternalSourcePrograms(externalSourceList[i], &externalProgramCount);
        if (programList == nullptr) {
            return false;
        }
        for (size_t j = 0; j < externalProgramCount; j++) {
            // the module name is dot-separated, the config key is slash-separated
            bool isListed = std::string(externalSourceName) == "libhar.Index";
            bool isFromAbc = g_impl->ProgramIsFromABC(ctx, programList[j]);
            if (isListed != isFromAbc) {
                return false;
            }
            fromAbcSeen |= isListed;
            notFromAbcSeen |= !isListed;
        }
    }
    // both the listed package and at least one unlisted source must have been observed
    return fromAbcSeen && notFromAbcSeen;
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }
    g_impl = GetImpl();
    if (g_impl == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    auto config = g_impl->CreateConfig(argc - 1, argv + 1);
    auto context = g_impl->CreateContextFromFile(config, argv[argc - 1]);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        g_impl->DestroyConfig(config);
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    auto *mainProgram = g_impl->ContextProgram(context);

    if (!CheckProgramIsFromAbcNullArguments(context, mainProgram)) {
        g_impl->DestroyContext(context);
        g_impl->DestroyConfig(config);
        return PROGRAM_IS_FROM_ABC_FAIL_CODE;
    }

    if (!CheckMainProgramIsNotFromAbc(context)) {
        g_impl->DestroyContext(context);
        g_impl->DestroyConfig(config);
        return PROGRAM_IS_FROM_ABC_FAIL_CODE;
    }

    if (!CheckExternalSourcesFromAbc(context)) {
        g_impl->DestroyContext(context);
        g_impl->DestroyConfig(config);
        return EXTERNAL_SOURCE_FROM_ABC_FAIL_CODE;
    }

    g_impl->DestroyContext(context);
    g_impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
