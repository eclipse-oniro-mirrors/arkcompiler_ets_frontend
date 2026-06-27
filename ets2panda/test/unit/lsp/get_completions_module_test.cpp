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

#include "lsp_api_test.h"
#include "lsp/include/completions.h"
#include "lsp/include/internal_api.h"

class LSPCompletionsModuleTests : public LSPAPITests {};

using ark::es2panda::lsp::CompletionEntryKind;
using ark::es2panda::lsp::Initializer;

namespace {

TEST_F(LSPCompletionsModuleTests, ModuleCompletionsVariable)
{
    std::vector<std::string> files = {"module_var.ets"};
    std::vector<std::string> texts = {R"(
export namespace MyModule {
    export let myVar: number = 10;
}
let a = MyModule.
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const size_t offset = 83;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    std::string insertName = "myVar";
    std::string name = "myVar: number";
    bool found = false;
    for (const auto &entry : entries) {
        if (entry.GetCompletionKind() == CompletionEntryKind::PROPERTY && entry.GetInsertText() == insertName &&
            (entry.GetName() == name)) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsModuleTests, ModuleCompletionsFunction)
{
    std::vector<std::string> files = {"module_func.ets"};
    std::vector<std::string> texts = {R"(
export namespace MyModule {
    export function myFunc(): void {}
}
let a = MyModule.
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const size_t offset = 86;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    bool found = false;
    for (const auto &entry : entries) {
        if (entry.GetCompletionKind() == CompletionEntryKind::METHOD && entry.GetInsertText() == "myFunc()" &&
            entry.GetName() == "myFunc(): void") {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsModuleTests, ModuleCompletionsClass)
{
    std::vector<std::string> files = {"module_class.ets"};
    std::vector<std::string> texts = {R"(
export namespace MyModule {
    export class MyClass {}
}
let a = MyModule.
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const size_t offset = 76;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    std::string expectedName = "MyClass";
    bool found = false;
    for (const auto &entry : entries) {
        if (entry.GetName() == expectedName && entry.GetCompletionKind() == CompletionEntryKind::CLASS) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsModuleTests, ModuleCompletionsInterface)
{
    std::vector<std::string> files = {"module_interface.ets"};
    std::vector<std::string> texts = {R"(
export namespace MyModule {
    export interface MyInterface {}
}
let a = MyModule.
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const size_t offset = 81;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    std::string expectedName = "MyInterface";
    bool found = false;
    for (const auto &entry : entries) {
        if (entry.GetName() == expectedName && entry.GetCompletionKind() == CompletionEntryKind::INTERFACE) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsModuleTests, ModuleCompletionsNestedModule)
{
    std::vector<std::string> files = {"module_nested.ets"};
    std::vector<std::string> texts = {R"(
export namespace MyModule {
    export namespace NestedModule {}
}
let a = MyModule.
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const size_t offset = 85;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    std::string expectedName = "NestedModule";
    bool found = false;
    for (const auto &entry : entries) {
        if (entry.GetName() == expectedName && entry.GetCompletionKind() == CompletionEntryKind::MODULE) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
    initializer.DestroyContext(ctx);
}

}  // namespace
