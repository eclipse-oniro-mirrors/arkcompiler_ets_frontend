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

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "public/es2panda_lib.h"

namespace ark::es2panda::staexpr2abc {

static constexpr char BASE64_INPUT_PREFIX[] = "--base64Input=";
static constexpr char EXTENSION_ARG[] = "--extension=ets";

static void PrintUsage()
{
    std::cerr << "Usage: ark_staexpr2abc [es2panda compiler options] --base64Input=<expression base64>\n";
    std::cerr << "  Compiles a base64-encoded ArkTS expression to abc bytecode via the\n";
    std::cerr << "  evaluate pipeline and prints base64 abc to stdout (single line).\n";
    std::cerr << "  The expression must be the base64 of the raw expression text.\n";
    std::cerr << "  Exit code: 0 on success, 1 on failure (diagnostics on stderr).\n";
}

// Extracts the --base64Input value, filters tool-only arguments out of the
// CreateConfig passthrough list and injects --extension=ets when missing.
// Returns false when --help was requested or no expression was given.
static bool FilterArgs(int argc, const char *const *argv, std::string &base64Value, std::vector<const char *> &filtered)
{
    bool hasExtension = false;
    filtered.push_back(argv[0]);
    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], BASE64_INPUT_PREFIX, std::strlen(BASE64_INPUT_PREFIX)) == 0) {
            base64Value = argv[i] + std::strlen(BASE64_INPUT_PREFIX);
            continue;
        }
        if (std::strcmp(argv[i], "--help") == 0) {
            return false;
        }
        if (std::strncmp(argv[i], "--extension", std::strlen("--extension")) == 0) {
            hasExtension = true;
        }
        filtered.push_back(argv[i]);
    }
    if (base64Value.empty()) {
        return false;
    }
    if (!hasExtension) {
        filtered.push_back(EXTENSION_ARG);
    }
    return true;
}

static int Run(int argc, const char *const *argv)
{
    std::string base64Value;
    std::vector<const char *> filtered;
    if (!FilterArgs(argc, argv, base64Value, filtered)) {
        PrintUsage();
        return 1;
    }

    const auto *impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (impl == nullptr) {
        std::cerr << "Error: failed to obtain es2panda implementation" << std::endl;
        return 1;
    }

    auto *config = impl->CreateConfig(static_cast<int>(filtered.size()), filtered.data());
    if (config == nullptr) {
        std::cerr << "Error: failed to create es2panda config" << std::endl;
        impl->MemFinalize();
        return 1;
    }

    char *result = impl->EvaluateExpression(config, base64Value.c_str());
    if (result == nullptr) {
        // Single-line failure reason is printed by the lib (first error
        // diagnostic, or a fallback when nothing was recorded).
        impl->DestroyConfigWithoutLog(config);
        impl->MemFinalize();
        return 1;
    }

    std::cout << result << std::endl;
    impl->FreeExpressionResult(result);
    impl->DestroyConfigWithoutLog(config);
    impl->MemFinalize();
    return 0;
}
}  // namespace ark::es2panda::staexpr2abc

int main(int argc, const char *argv[])
{
    return ark::es2panda::staexpr2abc::Run(argc, argv);
}
