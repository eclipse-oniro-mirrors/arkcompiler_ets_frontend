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

#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "util/arktsconfig.h"
#include "public/public.h"

using ark::es2panda::lsp::Initializer;

class LSPGetCompilerOptionsDiagnosticsTests : public LSPAPITests {
public:
    LSPGetCompilerOptionsDiagnosticsTests() = default;
    ~LSPGetCompilerOptionsDiagnosticsTests() override = default;

    NO_COPY_SEMANTIC(LSPGetCompilerOptionsDiagnosticsTests);
    NO_MOVE_SEMANTIC(LSPGetCompilerOptionsDiagnosticsTests);

    DiagnosticReferences MockGetGlobalDiagnostics(std::vector<std::string> &files, std::vector<std::string> &texts)
    {
        auto filePaths = CreateTempFile(files, texts);
        Initializer initializer;
        DiagnosticReferences diagnostics;

        for (auto const &file : filePaths) {
            auto ctx = initializer.CreateContext(file.c_str(), ES2PANDA_STATE_CHECKED);
            ark::es2panda::lsp::GetGlobalDiagnostics(ctx, diagnostics);
            initializer.DestroyContext(ctx);
        }

        return diagnostics;
    }

    DiagnosticReferences MockGetOptionDiagnostics(std::vector<std::string> &files, std::vector<std::string> &texts)
    {
        auto filePaths = CreateTempFile(files, texts);

        DiagnosticReferences diagnostics;
        int const expectedFileCount = 1;

        if (filePaths.size() < expectedFileCount) {
            return diagnostics;
        }

        Initializer initializer;
        auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
        ark::es2panda::util::DiagnosticEngine *diagnosticEngine =
            reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->diagnosticEngine;
        auto config = ark::es2panda::ArkTsConfig {filePaths[1], *diagnosticEngine};
        config.Parse();
        ark::es2panda::lsp::GetOptionDiagnostics(ctx, diagnostics);
        initializer.DestroyContext(ctx);

        return diagnostics;
    }
};

TEST_F(LSPGetCompilerOptionsDiagnosticsTests, GetGlobalDiagnostics1)
{
    std::vector<std::string> files = {"get_compiler_options_diagnostics_test_main.ets"};
    std::vector<std::string> texts = {
        R"(
import { test } from "";
)"};

    DiagnosticReferences diagnostics = MockGetGlobalDiagnostics(files, texts);

    int const defaultDiagnosticsSize = 0;
    ASSERT_GT(diagnostics.diagnostic.size(), defaultDiagnosticsSize);

    const auto &diag = diagnostics.diagnostic[0];
    ASSERT_EQ(diag.severity_, DiagnosticSeverity::Error);
    ASSERT_NE(diag.message_.find("Import path cannot be empty"), std::string::npos);
}

TEST_F(LSPGetCompilerOptionsDiagnosticsTests, GetGlobalDiagnostics2)
{
    std::vector<std::string> files = {"get_compiler_options_diagnostics_test_main.ets"};
    std::vector<std::string> texts = {
        R"(
import { test } from "@unknown/package";
)"};

    DiagnosticReferences diagnostics = MockGetGlobalDiagnostics(files, texts);

    int const defaultDiagnosticsSize = 0;
    ASSERT_GT(diagnostics.diagnostic.size(), defaultDiagnosticsSize);

    const auto &diag = diagnostics.diagnostic[0];
    ASSERT_EQ(diag.severity_, DiagnosticSeverity::Error);
    ASSERT_NE(diag.message_.find("Can't find prefix for"), std::string::npos);
}

TEST_F(LSPGetCompilerOptionsDiagnosticsTests, GetGlobalDiagnostics3)
{
    std::vector<std::string> files = {"get_compiler_options_diagnostics_test_main.ets"};
    std::vector<std::string> texts = {
        R"(
import { test2 } from "./non_existent.ets";
)"};

    DiagnosticReferences diagnostics = MockGetGlobalDiagnostics(files, texts);

    int const defaultDiagnosticsSize = 0;
    ASSERT_GT(diagnostics.diagnostic.size(), defaultDiagnosticsSize);

    const auto &diag = diagnostics.diagnostic[0];
    ASSERT_EQ(diag.severity_, DiagnosticSeverity::Error);

    bool isPathError = (diag.message_.find("Not supported path:") != std::string::npos) ||
                       (diag.message_.find("Not an available source path:") != std::string::npos);
    ASSERT_TRUE(isPathError);
}

TEST_F(LSPGetCompilerOptionsDiagnosticsTests, GetGlobalDiagnostics4)
{
    std::vector<std::string> files = {"get_compiler_options_diagnostics_test_main.ets",
                                      "get_compiler_options_diagnostics_test_helper.ets",
                                      "get_compiler_options_diagnostics_test_utils.ets"};
    std::vector<std::string> texts = {
        R"(
import { helper } from "./get_compiler_options_diagnostics_test_helper.ets";
import { utils } from "./get_compiler_options_diagnostics_test_utils.ets";
import { missing } from "./missing.ets";
 
function main() {
    helper(utils.getValue());
}
)",
        R"(
import { test1 } from "";
export function helper(value: number) {
    return value * 2;
}
)",
        R"(
import { test2 } from "./non_existent.ets";
export const utils = {
    getValue: () => helper(5)
})"};

    DiagnosticReferences diagnostics = MockGetGlobalDiagnostics(files, texts);

    int const defaultDiagnosticsSize = 0;
    ASSERT_GT(diagnostics.diagnostic.size(), defaultDiagnosticsSize);

    bool foundEmptyPath = false;
    bool foundPathError = false;

    for (const auto &diag : diagnostics.diagnostic) {
        ASSERT_EQ(diag.severity_, DiagnosticSeverity::Error);
        if (diag.message_.find("Import path cannot be empty") != std::string::npos) {
            foundEmptyPath = true;
        }
        if (diag.message_.find("Not supported path:") != std::string::npos) {
            foundPathError = true;
        }
    }

    ASSERT_TRUE(foundEmptyPath);
    ASSERT_TRUE(foundPathError);
}

TEST_F(LSPGetCompilerOptionsDiagnosticsTests, GetOptionDiagnostics1)
{
    std::vector<std::string> files = {"get_compiler_options_diagnostics_test_main.ets", "arktsconfig.json"};
    std::vector<std::string> texts = {
        R"(
function A(a:number, b:number) {
    return a + b;
}
)",
        R"({
    "invalid json"
})"};

    DiagnosticReferences diagnostics = MockGetOptionDiagnostics(files, texts);

    int const defaultDiagnosticsSize = 0;
    ASSERT_GT(diagnostics.diagnostic.size(), defaultDiagnosticsSize);
    bool found = false;
    for (const auto &diag : diagnostics.diagnostic) {
        if (diag.message_.find("ArkTsConfig is not valid json") != std::string::npos) {
            found = true;
        }
    }

    ASSERT_TRUE(found);
}

TEST_F(LSPGetCompilerOptionsDiagnosticsTests, GetOptionDiagnostics2)
{
    std::vector<std::string> files = {"get_compiler_options_diagnostics_test_main.ets", "arktsconfig.json"};
    std::vector<std::string> texts = {
        R"(
function A(a:number, b:number) {
    return a + b;
}
)",
        R"({
    "extends": "./arktsconfig.json",
    "compilerOptions": {
        "baseUrl": "./temp"
    }
})"};

    DiagnosticReferences diagnostics = MockGetOptionDiagnostics(files, texts);

    int const defaultDiagnosticsSize = 0;
    ASSERT_GT(diagnostics.diagnostic.size(), defaultDiagnosticsSize);
    bool found = false;
    for (const auto &diag : diagnostics.diagnostic) {
        if (diag.message_.find("Encountered cyclic import in") != std::string::npos) {
            found = true;
        }
    }

    ASSERT_TRUE(found);
}

TEST_F(LSPGetCompilerOptionsDiagnosticsTests, GetOptionDiagnostics3)
{
    std::vector<std::string> files = {"get_compiler_options_diagnostics_test_main.ets", "arktsconfig.json"};
    std::vector<std::string> texts = {
        R"(
function A(a:number, b:number) {
    return a + b;
}
)",
        R"({
    "compilerOptions": {
        "paths": {
            "std": []
        }
    }
})"};

    DiagnosticReferences diagnostics = MockGetOptionDiagnostics(files, texts);

    int const defaultDiagnosticsSize = 0;
    ASSERT_GT(diagnostics.diagnostic.size(), defaultDiagnosticsSize);
    bool found = false;
    for (const auto &diag : diagnostics.diagnostic) {
        if (diag.message_.find("Substitutions for pattern") != std::string::npos) {
            found = true;
        }
    }

    ASSERT_TRUE(found);
}

TEST_F(LSPGetCompilerOptionsDiagnosticsTests, GetOptionDiagnostics4)
{
    std::vector<std::string> files = {"get_compiler_options_diagnostics_test_main.ets", "arktsconfig.json"};
    std::vector<std::string> texts = {
        R"(
function A(a:number, b:number) {
    return a + b;
}
)",
        R"({
    "compilerOptions": {
        "baseUrl": "/path",
        "paths": {
            "std": ["./path1"]
        },
        "dependencies": {
            "dynamic_import_tests": {
                "language": "invalid_lang",
                "path": "path.d.ets"
            }
        }
    }
})"};

    DiagnosticReferences diagnostics = MockGetOptionDiagnostics(files, texts);

    int const defaultDiagnosticsSize = 0;
    ASSERT_GT(diagnostics.diagnostic.size(), defaultDiagnosticsSize);
    bool found = false;
    for (const auto &diag : diagnostics.diagnostic) {
        if (diag.message_.find("value for dynamic path with key") != std::string::npos) {
            found = true;
        }
    }

    ASSERT_TRUE(found);
}

TEST_F(LSPGetCompilerOptionsDiagnosticsTests, GetOptionDiagnostics5)
{
    std::vector<std::string> files = {"get_compiler_options_diagnostics_test_main.ets", "arktsconfig.json"};
    std::vector<std::string> texts = {
        R"(
function A(a:number, b:number) {
    return a + b;
}
)",
        R"({
    "compilerOptions": {
        "baseUrl": "./temp",
        "paths": {
            "std": ["./path1"]
        },
        "dependencies": {
            "dynamic_import_tests": {
                "language": "ts",
                "path": "path.d.ets"
            }
        }
    }
})"};

    DiagnosticReferences diagnostics = MockGetOptionDiagnostics(files, texts);

    int const defaultDiagnosticsSize = 0;
    ASSERT_GT(diagnostics.diagnostic.size(), defaultDiagnosticsSize);
    bool found = false;
    for (const auto &diag : diagnostics.diagnostic) {
        if (diag.message_.find("Interoperability with language") != std::string::npos) {
            found = true;
        }
    }

    ASSERT_TRUE(found);
}