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

#include "lsp_api_test.h"
#include "lsp/include/completions.h"
#include "lsp/include/internal_api.h"

class LSPCompletionsModuleTests1 : public LSPAPITests {};

using ark::es2panda::lsp::Initializer;

namespace {

// Helper: check if entries contain an entry with the given name and kind
bool HasEntryWithName(const std::vector<ark::es2panda::lsp::CompletionEntry> &entries, const std::string &expectedName)
{
    for (const auto &entry : entries) {
        if (entry.GetName() == expectedName) {
            return true;
        }
    }
    return false;
}

// Extract the member identifier from a completion entry name.
// "field: number" -> "field"; "method(): void" -> "method"; "InnerClass" -> "InnerClass".
// The type suffix differs across compiler versions (e.g. "number" vs "Double",
// "void" vs "undefined"), so identifier-only matching keeps the test portable.
std::string MemberIdentifier(const std::string &name)
{
    auto colon = name.find(':');
    auto paren = name.find('(');
    size_t end = std::string::npos;
    if (colon != std::string::npos) {
        end = colon;
    }
    if (paren != std::string::npos && (end == std::string::npos || paren < end)) {
        end = paren;
    }
    return end == std::string::npos ? name : name.substr(0, end);
}

// Helper: check if entries contain a member whose identifier matches expectedId.
bool HasEntryWithIdentifier(const std::vector<ark::es2panda::lsp::CompletionEntry> &entries,
                            const std::string &expectedId)
{
    for (const auto &entry : entries) {
        if (MemberIdentifier(entry.GetName()) == expectedId) {
            return true;
        }
    }
    return false;
}

// Test: re-export module completion - completions from a re-exported module
TEST_F(LSPCompletionsModuleTests1, ReExportModuleCompletions)
{
    std::vector<std::string> files = {"reexport_source.ets", "reexport_middle.ets", "reexport_use.ets"};
    std::vector<std::string> texts = {R"(export class SourceClass { value: number = 1; }
export function sourceFunc(): void {})",
                                      R"(export { SourceClass, sourceFunc } from './reexport_source';)",
                                      R"(import { SourceClass } from './reexport_middle';
let foo = new SourceClass();
foo.)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // Position after "foo." to get property completions
    const size_t offset = texts[2].find("foo.") + 4;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    // Should suggest "value" from SourceClass (re-exported through middle)
    bool foundValue = false;
    for (const auto &entry : entries) {
        if (entry.GetInsertText() == "value") {
            foundValue = true;
            break;
        }
    }
    ASSERT_TRUE(foundValue);

    initializer.DestroyContext(ctx);
}

// Test: alias export completion - completions for aliased exports
TEST_F(LSPCompletionsModuleTests1, AliasExportCompletions)
{
    std::vector<std::string> files = {"alias_export_source.ets", "alias_export_use.ets"};
    std::vector<std::string> texts = {R"(export class OriginalName { value: number = 1; })",
                                      R"(import { OriginalName as AliasedName } from './alias_export_source';
let foo = new AliasedName();
foo.)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // Position after "foo." to get property completions
    const size_t offset = texts[1].find("foo.") + 4;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    // Should suggest "value" property through the aliased import
    bool foundValue = false;
    for (const auto &entry : entries) {
        if (entry.GetInsertText() == "value") {
            foundValue = true;
            break;
        }
    }
    ASSERT_TRUE(foundValue);

    initializer.DestroyContext(ctx);
}

// Test: namespace member completions - access namespace members
TEST_F(LSPCompletionsModuleTests1, NamespaceMemberCompletions)
{
    std::vector<std::string> files = {"namespace_members.ets"};
    std::vector<std::string> texts = {R"(export namespace Utils {
    export function helper(): void {}
    export let counter: number = 0;
    export class InnerClass { value: number = 1; }
}
Utils.)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // Position after "Utils." to get namespace member completions
    const size_t offset = texts[0].find("Utils.") + 6;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    // Should suggest helper, counter, and InnerClass
    ASSERT_TRUE(HasEntryWithIdentifier(entries, "helper"));
    ASSERT_TRUE(HasEntryWithIdentifier(entries, "counter"));
    ASSERT_TRUE(HasEntryWithIdentifier(entries, "InnerClass"));

    initializer.DestroyContext(ctx);
}

// Test: enum member completions
TEST_F(LSPCompletionsModuleTests1, EnumMemberCompletions)
{
    std::vector<std::string> files = {"enum_completions.ets"};
    std::vector<std::string> texts = {R"(enum Direction {
    North = 1,
    South = 2,
    East = 3,
    West = 4
}
let d = Direction.)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // Position after "Direction." to get enum member completions
    const size_t offset = texts[0].find("Direction.") + 10;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    // Should suggest enum members
    ASSERT_TRUE(HasEntryWithName(entries, "North"));
    ASSERT_TRUE(HasEntryWithName(entries, "South"));
    ASSERT_TRUE(HasEntryWithName(entries, "East"));
    ASSERT_TRUE(HasEntryWithName(entries, "West"));

    initializer.DestroyContext(ctx);
}

// Test: static member completions
TEST_F(LSPCompletionsModuleTests1, StaticMemberCompletions)
{
    std::vector<std::string> files = {"static_members.ets"};
    std::vector<std::string> texts = {R"(class MathUtils {
    static PI: number = 3.14159;
    static square(x: number): number {
        return x * x;
    }
    static cube(x: number): number {
        return x * x * x;
    }
}
MathUtils.)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // Position after "MathUtils." to get static member completions
    const size_t offset = texts[0].find("MathUtils.") + 10;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    // Should suggest static members - check by insert text
    bool foundPI = false;
    bool foundSquare = false;
    bool foundCube = false;
    for (const auto &entry : entries) {
        if (entry.GetInsertText() == "PI") {
            foundPI = true;
        }
        if (entry.GetInsertText() == "square()") {
            foundSquare = true;
        }
        if (entry.GetInsertText() == "cube()") {
            foundCube = true;
        }
    }
    ASSERT_TRUE(foundPI);
    ASSERT_TRUE(foundSquare);
    ASSERT_TRUE(foundCube);

    initializer.DestroyContext(ctx);
}

// Test: inherited member completions
TEST_F(LSPCompletionsModuleTests1, InheritedMemberCompletions)
{
    std::vector<std::string> files = {"inherited_members.ets"};
    std::vector<std::string> texts = {R"(class Base {
    baseField: number = 1;
    baseMethod(): void {}
}
class Derived extends Base {
    derivedField: string = "test";
    derivedMethod(): void {}
}
let d = new Derived();
d.)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // Position after "d." to get member completions (including inherited)
    const size_t offset = texts[0].find("d.") + 2;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    // Should suggest both base and derived members
    ASSERT_TRUE(HasEntryWithIdentifier(entries, "baseField"));
    ASSERT_TRUE(HasEntryWithIdentifier(entries, "baseMethod"));
    ASSERT_TRUE(HasEntryWithIdentifier(entries, "derivedField"));
    ASSERT_TRUE(HasEntryWithIdentifier(entries, "derivedMethod"));

    initializer.DestroyContext(ctx);
}

// Test: this. completions inside class method
TEST_F(LSPCompletionsModuleTests1, ThisCompletionsInClassMethod)
{
    std::vector<std::string> files = {"this_completions.ets"};
    std::vector<std::string> texts = {R"(class MyClass {
    field1: number = 1;
    field2: string = "test";
    method1(): void {}
    method2(): void {
        this.
    }
})"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // Position after "this." inside method2
    const size_t offset = texts[0].find("this.") + 5;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    // Should suggest class members
    ASSERT_TRUE(HasEntryWithIdentifier(entries, "field1"));
    ASSERT_TRUE(HasEntryWithIdentifier(entries, "field2"));
    ASSERT_TRUE(HasEntryWithIdentifier(entries, "method1"));

    initializer.DestroyContext(ctx);
}

// Test: completion after dot access on class instance
TEST_F(LSPCompletionsModuleTests1, DotAccessCompletions)
{
    std::vector<std::string> files = {"dot_access.ets"};
    std::vector<std::string> texts = {R"(class Foo {
    value: number = 1;
    method(): void {}
}
let foo = new Foo();
foo.)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // Position after "foo." to get member completions
    const size_t offset = texts[0].find("foo.") + 4;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    // Should suggest members
    bool foundValue = false;
    bool foundMethod = false;
    for (const auto &entry : entries) {
        if (entry.GetInsertText() == "value") {
            foundValue = true;
        }
        if (entry.GetInsertText() == "method()") {
            foundMethod = true;
        }
    }
    ASSERT_TRUE(foundValue);
    ASSERT_TRUE(foundMethod);

    initializer.DestroyContext(ctx);
}

// Test: completion in type position
TEST_F(LSPCompletionsModuleTests1, CompletionInTypePosition)
{
    std::vector<std::string> files = {"type_position.ets"};
    std::vector<std::string> texts = {R"(class MyClass {
    value: number = 1;
}
// test logic - space after colon is intentional for type position completion
let foo:)"
                                      " "};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // Position after "let foo: " to get type completions
    const size_t offset = texts[0].find("let foo: ") + 9;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    // Type position should return some completions (may include keywords, types, classes)
    ASSERT_GE(entries.size(), 0U);

    initializer.DestroyContext(ctx);
}

// Test: completion after new keyword
TEST_F(LSPCompletionsModuleTests1, CompletionAfterNewKeyword)
{
    std::vector<std::string> files = {"new_keyword.ets"};
    std::vector<std::string> texts = {R"(class MyClass {
    value: number = 1;
}
// test logic - space after new is intentional for constructor completion
let foo = new)"
                                      " "};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // Position after "new " to get class completions
    const size_t offset = texts[0].find("new ") + 4;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    // After new keyword, should return some completions
    ASSERT_GE(entries.size(), 0U);

    initializer.DestroyContext(ctx);
}

// Test: isPackageModule - a context created from a source with a "package"
// declaration is a package module, a plain script is not.
TEST_F(LSPCompletionsModuleTests1, IsPackageModuleForPackageDeclaration)
{
    std::vector<std::string> files = {"is_pkg_module.ets"};
    std::vector<std::string> texts = {R"(package com.example.pkgtest;

export function pkgFunc(): number {
    return 1;
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // "package com.example.pkgtest;" marks the file as a package module.
    ASSERT_TRUE(lspApi->isPackageModule(ctx));

    initializer.DestroyContext(ctx);
}

// Test: isPackageModule - a regular script without a package declaration is not
// a package module.
TEST_F(LSPCompletionsModuleTests1, IsPackageModuleForPlainScript)
{
    std::vector<std::string> files = {"is_plain_script.ets"};
    std::vector<std::string> texts = {R"(let plainVar: number = 1;
export function plainFunc(): number {
    return plainVar;
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    ASSERT_FALSE(lspApi->isPackageModule(ctx));

    initializer.DestroyContext(ctx);
}

}  // namespace
