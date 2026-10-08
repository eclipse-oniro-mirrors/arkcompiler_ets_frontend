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
#include "lsp/include/symbol_reference_index.h"

#include <algorithm>

class LSPCompletionsModuleTests : public LSPAPITests {};

using ark::es2panda::lsp::CompletionEntryKind;
using ark::es2panda::lsp::Initializer;

namespace {

bool HasCompletion(const std::vector<ark::es2panda::lsp::CompletionEntry> &entries, const std::string &insertText)
{
    return std::any_of(entries.begin(), entries.end(),
                       [&insertText](const auto &entry) { return entry.GetInsertText() == insertText; });
}

const ark::es2panda::lsp::CompletionEntry *FindCompletion(
    const std::vector<ark::es2panda::lsp::CompletionEntry> &entries, const std::string &insertText)
{
    auto found = std::find_if(entries.begin(), entries.end(),
                              [&insertText](const auto &entry) { return entry.GetInsertText() == insertText; });
    return found == entries.end() ? nullptr : &*found;
}

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

TEST_F(LSPCompletionsModuleTests, NamespaceImportCompletesVisibleDirectExports)
{
    std::vector<std::string> files = {"namespace_direct_source.ets", "namespace_direct_user.ets"};
    std::vector<std::string> texts = {R"(
export const VALUE: number = 1;
export function run(input: number): number { return input; }
export class ExportedClass {}
export interface ExportedInterface {}
export type ExportedAlias = number;
export enum ExportedEnum { VALUE }
export @interface ExportedAnnotation {}
export namespace Nested {}
export default class HiddenDefault {}
)",
                                      R"(
import * as M1 from './namespace_direct_source';
M1.
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    auto offset = texts[1].find("M1.") + 3;
    auto entries = GetImpl()->getCompletionsAtPosition(ctx, offset).GetEntries();

    const auto assertKind = [&entries](const std::string &insertText, CompletionEntryKind expectedKind) {
        auto *entry = FindCompletion(entries, insertText);
        ASSERT_NE(entry, nullptr) << insertText;
        EXPECT_EQ(entry->GetCompletionKind(), expectedKind) << insertText;
    };
    assertKind("VALUE", CompletionEntryKind::PROPERTY);
    assertKind("run()", CompletionEntryKind::METHOD);
    assertKind("ExportedClass", CompletionEntryKind::CLASS);
    assertKind("ExportedInterface", CompletionEntryKind::INTERFACE);
    assertKind("ExportedAlias", CompletionEntryKind::INTERFACE);
    assertKind("ExportedEnum", CompletionEntryKind::ENUM);
    assertKind("ExportedAnnotation", CompletionEntryKind::ANNOTATION);
    assertKind("Nested", CompletionEntryKind::MODULE);
    EXPECT_FALSE(HasCompletion(entries, "HiddenDefault"));
    EXPECT_FALSE(HasCompletion(entries, "default"));
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsModuleTests, NamespaceImportUsesResolvedReExportClosure)
{
    std::vector<std::string> files = {"namespace_leaf_a.ets", "namespace_leaf_b.ets", "namespace_leaf_c.ets",
                                      "namespace_barrel.ets", "namespace_reexport_user.ets"};
    std::vector<std::string> texts = {R"(
export const A: number = 1;
)",
                                      R"(
export const B: number = 2;
export const Shared: number = 3;
)",
                                      R"(
export const Shared: number = 4;
)",
                                      R"(
export { A as Renamed } from './namespace_leaf_a';
export * from './namespace_leaf_b';
export * from './namespace_leaf_c';
export * as Nested from './namespace_leaf_a';
)",
                                      R"(
import * as M1 from './namespace_barrel';
M1.
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[4].c_str(), ES2PANDA_STATE_CHECKED);

    auto offset = texts[4].find("M1.") + 3;
    auto entries = GetImpl()->getCompletionsAtPosition(ctx, offset).GetEntries();

    EXPECT_TRUE(HasCompletion(entries, "Renamed"));
    EXPECT_TRUE(HasCompletion(entries, "B"));
    EXPECT_TRUE(HasCompletion(entries, "Nested"));
    EXPECT_FALSE(HasCompletion(entries, "A"));
    EXPECT_FALSE(HasCompletion(entries, "Shared"));
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsModuleTests, NamespaceImportCompletesCyclicStarExports)
{
    std::vector<std::string> files = {"namespace_cycle_a.ets", "namespace_cycle_b.ets", "namespace_cycle_user.ets"};
    std::vector<std::string> texts = {R"(
export const A: number = 1;
export * from './namespace_cycle_b';
)",
                                      R"(
export const B: number = 2;
export * from './namespace_cycle_a';
)",
                                      R"(
import * as M1 from './namespace_cycle_a';
M1.
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);

    auto offset = texts[2].find("M1.") + 3;
    auto entries = GetImpl()->getCompletionsAtPosition(ctx, offset).GetEntries();

    EXPECT_TRUE(HasCompletion(entries, "A"));
    EXPECT_TRUE(HasCompletion(entries, "B"));
    initializer.DestroyContext(ctx);
}

// Auto-import completion for re-exported enum and namespace must have ENUM and MODULE kinds.
TEST_F(LSPCompletionsModuleTests, AutoImportReexportedEnumAndNamespaceKind)
{
    std::vector<std::string> files = {"ai_origin.ets", "ai_barrel.ets", "ai_user.ets"};
    std::vector<std::string> texts = {R"(
export enum Color { Red, Green, Blue }
export namespace MyNs { export const V = 1; }
)",
                                      R"(
export { Color as PublicColor, MyNs as PublicNs } from './ai_origin';
)",
                                      R"(
Pub
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *originCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(originCtx, nullptr);
    auto *barrelCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(barrelCtx, nullptr);

    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(originCtx));
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(barrelCtx));

    auto *userCtx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(userCtx, nullptr);

    auto offset = texts[2].find("Pub") + 3;
    auto entries = GetImpl()->getCompletionsAtPosition(userCtx, offset).GetEntries();

    auto *colorEntry = FindCompletion(entries, "PublicColor");
    ASSERT_NE(colorEntry, nullptr);
    EXPECT_EQ(colorEntry->GetCompletionKind(), CompletionEntryKind::ENUM);

    auto *nsEntry = FindCompletion(entries, "PublicNs");
    ASSERT_NE(nsEntry, nullptr);
    EXPECT_EQ(nsEntry->GetCompletionKind(), CompletionEntryKind::MODULE);

    initializer.DestroyContext(originCtx);
    initializer.DestroyContext(barrelCtx);
    initializer.DestroyContext(userCtx);
}

// Auto-import completion should not suggest re-exported symbols when origin index is removed.
TEST_F(LSPCompletionsModuleTests, AutoImportReexportInvalidatedOnOriginRemoval)
{
    std::vector<std::string> files = {"c_origin.ets", "b_barrel.ets", "a_user.ets"};
    std::vector<std::string> texts = {R"(
export type MyUserId = string;
)",
                                      R"(
export * from './c_origin';
)",
                                      R"(
MyU
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *originCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(originCtx, nullptr);
    auto *barrelCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(barrelCtx, nullptr);

    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(originCtx));
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(barrelCtx));

    auto *userCtx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(userCtx, nullptr);

    auto offset = texts[2].find("MyU") + 3;
    auto entriesBefore = GetImpl()->getCompletionsAtPosition(userCtx, offset).GetEntries();
    EXPECT_TRUE(HasCompletion(entriesBefore, "MyUserId"));

    // Remove c_origin.ets index
    ASSERT_TRUE(ark::es2panda::lsp::RemoveSymbolReferenceIndexForFile(filePaths[0]));

    auto entriesAfter = GetImpl()->getCompletionsAtPosition(userCtx, offset).GetEntries();
    EXPECT_FALSE(HasCompletion(entriesAfter, "MyUserId"));

    initializer.DestroyContext(originCtx);
    initializer.DestroyContext(barrelCtx);
    initializer.DestroyContext(userCtx);
}

}  // namespace
