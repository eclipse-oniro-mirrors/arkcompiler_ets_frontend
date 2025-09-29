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
#include <gtest/gtest.h>
#include <cstdio>
#include <optional>
#include <string>
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/internal_api.h"
#include "ir/astNode.h"
#include "lsp/include/completions.h"

using ark::es2panda::lsp::Initializer;

class LSPAICTests : public LSPAPITests {};

TEST_F(LSPAICTests, GetAutoImportCompletionEntryUnresolved1)
{
    Initializer initializer = Initializer();

    const char *fileName = "file.ets";
    const char *fileSource = "import { BusinessError } from \"@ohos.base\"";
    auto ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, fileSource);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetImportSpecifier =
        ast->FindChild([](ark::es2panda::ir::AstNode *node) { return node->IsImportSpecifier(); });
    auto targetImportDeclaration =
        ast->FindChild([](ark::es2panda::ir::AstNode *node) { return node->IsETSImportDeclaration(); });

    auto importDecl = targetImportDeclaration->AsETSImportDeclaration()->Source()->ToString();
    auto importSpec = targetImportSpecifier->AsImportSpecifier()->Imported()->ToString();
    const auto &config = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->config->options->ArkTSConfig();

    auto data = ark::es2panda::lsp::CompletionEntryData(fileName, importSpec, importDecl, "");
    auto result = ark::es2panda::lsp::GetAutoImportCompletionEntry(&data, &config, "");
    ASSERT_EQ(ark::es2panda::lsp::ResolutionStatus::UNRESOLVED, result->GetStatus());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPAICTests, GetAutoImportCompletionEntryNotExistInConfigPath)
{
    Initializer initializer = Initializer();

    const char *fileName = "file.ets";
    const char *fileSource = "import { COLLATION } from \"native/core\"";
    auto ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, fileSource);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetImportSpecifier =
        ast->FindChild([](ark::es2panda::ir::AstNode *node) { return node->IsImportSpecifier(); });
    auto targetImportDeclaration =
        ast->FindChild([](ark::es2panda::ir::AstNode *node) { return node->IsETSImportDeclaration(); });

    auto importDecl = targetImportDeclaration->AsETSImportDeclaration()->Source()->ToString();
    auto importSpec = targetImportSpecifier->AsImportSpecifier()->Imported()->ToString();
    const auto &config = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->config->options->ArkTSConfig();

    auto data = ark::es2panda::lsp::CompletionEntryData(fileName, importSpec, importDecl, "");
    auto result = ark::es2panda::lsp::GetAutoImportCompletionEntry(&data, &config, "");
    ASSERT_EQ(std::nullopt, result);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPAICTests, GetAutoImportCompletionEntryResolved)
{
    Initializer initializer = Initializer();

    std::vector<std::string> files = {"get_auto_completion_test_main.ets", "get_auto_completion_test_helper.ets"};
    std::vector<std::string> texts = {
        R"(
         import { helper } from "./get_auto_completion_test_helper.ets";
         function main() {
             helper();
         }
         )",
        R"(
         export function helper() {
             return 2 * 2;
         }
         )"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    char const *fileName = filePaths[0].c_str();
    auto ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetImportSpecifier =
        ast->FindChild([](ark::es2panda::ir::AstNode *node) { return node->IsImportSpecifier(); });
    auto targetImportDeclaration =
        ast->FindChild([](ark::es2panda::ir::AstNode *node) { return node->IsETSImportDeclaration(); });

    auto importDecl = targetImportDeclaration->AsETSImportDeclaration()->Source()->ToString();
    auto importSpec = targetImportSpecifier->AsImportSpecifier()->Imported()->ToString();
    const auto &config = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->config->options->ArkTSConfig();

    auto data = ark::es2panda::lsp::CompletionEntryData(fileName, importSpec, importDecl, "");
    auto result = ark::es2panda::lsp::GetAutoImportCompletionEntry(&data, &config, "");
    ASSERT_EQ(ark::es2panda::lsp::ResolutionStatus::RESOLVED, result->GetStatus());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPAICTests, GetAutoImportCompletionEntryEmptyFile)
{
    auto data = ark::es2panda::lsp::CompletionEntryData("", "", "", "");
    auto result = ark::es2panda::lsp::GetAutoImportCompletionEntry(&data, nullptr, "");
    ASSERT_EQ(std::nullopt, result);
}

TEST_F(LSPAICTests, GetAutoImportCompletionEntryNullConfigNonEmptyFile)
{
    Initializer initializer = Initializer();

    const char *fileName = "file.ets";
    const char *fileSource = "import { PI } from \"std/math\"";
    auto ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, fileSource);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetImportSpecifier =
        ast->FindChild([](ark::es2panda::ir::AstNode *node) { return node->IsImportSpecifier(); });
    auto targetImportDeclaration =
        ast->FindChild([](ark::es2panda::ir::AstNode *node) { return node->IsETSImportDeclaration(); });

    auto importDecl = targetImportDeclaration->AsETSImportDeclaration()->Source()->ToString();
    auto importSpec = targetImportSpecifier->AsImportSpecifier()->Imported()->ToString();
    initializer.DestroyContext(ctx);

    auto data = ark::es2panda::lsp::CompletionEntryData(fileName, importSpec, importDecl, "");
    auto result = ark::es2panda::lsp::GetAutoImportCompletionEntry(&data, nullptr, "");
    ASSERT_EQ(std::nullopt, result);
}

TEST_F(LSPAICTests, GetAutoImportCompletionEntryEmptyImportDeclaration)
{
    Initializer initializer = Initializer();

    const char *fileName = "file.ets";
    auto ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, "");
    const auto &config = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->config->options->ArkTSConfig();

    auto data = ark::es2panda::lsp::CompletionEntryData(fileName, "", "", "");
    auto result = ark::es2panda::lsp::GetAutoImportCompletionEntry(&data, &config, "");
    ASSERT_EQ(std::nullopt, result);
    initializer.DestroyContext(ctx);
}
