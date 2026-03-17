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
#include <string>

#include "public/es2panda_lib.h"
#include "util.h"

static es2panda_Impl *g_impl = nullptr;

static constexpr auto SOURCE =
    "function add(base: int, extra: int = 1): int {\n"
    "    return base + extra;\n"
    "}\n"
    "\n"
    "function main(): void {\n"
    "    let applyExtra: (base: int) => int = (base: int): int => {\n"
    "        return add(base);\n"
    "    };\n"
    "    let value: int = applyExtra(10);\n"
    "    if (value > 0) {\n"
    "        return;\n"
    "    }\n"
    "}\n";

static bool CreateLoweredDump(es2panda_Config *config, const char *fileName, std::string *dumpedSource)
{
    auto *context = g_impl->CreateContextFromString(config, SOURCE, fileName);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return false;
    }

    g_impl->ProceedToState(context, ES2PANDA_STATE_LOWERED);
    CheckForErrors("LOWERED", context);
    if (g_impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        g_impl->DestroyContext(context);
        return false;
    }

    auto *program = g_impl->ProgramAst(context, g_impl->ContextProgram(context));
    const char *lowered = g_impl->AstNodeDumpEtsSrcConst(context, program);
    if (lowered == nullptr) {
        g_impl->DestroyContext(context);
        return false;
    }

    *dumpedSource = lowered;
    g_impl->DestroyContext(context);
    return true;
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

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char **args = const_cast<const char **>(&(argv[1]));
    auto *config = g_impl->CreateConfig(argc - 1, args);

    std::string firstLowered;
    std::string secondLowered;
    std::string loweredAfterReset;

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (!CreateLoweredDump(config, argv[argc - 1], &firstLowered) ||
        !CreateLoweredDump(config, argv[argc - 1], &secondLowered)) {
        g_impl->DestroyConfig(config);
        return TEST_ERROR_CODE;
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    if (firstLowered == secondLowered) {
        std::cerr << "Lowered sources are unexpectedly equal before ResetCounters" << std::endl;
        g_impl->DestroyConfig(config);
        return TEST_ERROR_CODE;
    }

    g_impl->ResetCounters();

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (!CreateLoweredDump(config, argv[argc - 1], &loweredAfterReset)) {
        g_impl->DestroyConfig(config);
        return TEST_ERROR_CODE;
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    if (firstLowered != loweredAfterReset) {
        std::cerr << "Lowered source after ResetCounters does not match the initial lowered source" << std::endl;
        g_impl->DestroyConfig(config);
        return TEST_ERROR_CODE;
    }

    g_impl->DestroyConfig(config);
    return 0;
}
