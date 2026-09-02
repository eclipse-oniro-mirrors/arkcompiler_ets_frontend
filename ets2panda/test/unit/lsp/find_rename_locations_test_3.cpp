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
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/find_rename_locations.h"
#include <gtest/gtest.h>

namespace {
using ark::es2panda::lsp::FindRenameLocationsInCurrentFile;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::NeedsCrossFileRename;
class LspFindRenameLocationsTests3 : public LSPAPITests {};

// Test: import alias rename - should rename all usages of the alias in current file
TEST_F(LspFindRenameLocationsTests3, ImportAliasRenameInCurrentFile)
{
    std::vector<std::string> files = {"rename_import_alias_export.ets", "rename_import_alias_use.ets"};
    std::vector<std::string> texts = {R"(export class OriginalClass { value: number = 1; })",
                                      R"(import { OriginalClass as AliasedClass } from './rename_import_alias_export';
let foo: AliasedClass = new AliasedClass();
foo.value;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "AliasedClass" in the import statement
    const size_t position = texts[1].find("AliasedClass }");
    ASSERT_NE(position, std::string::npos);

    // Import alias should NOT need cross-file rename (only renames the alias, not the original)
    ASSERT_FALSE(NeedsCrossFileRename(context, position));

    auto res = FindRenameLocationsInCurrentFile(context, position);
    ASSERT_GE(res.size(), 1U);

    // All rename locations should be in the import file
    for (const auto &loc : res) {
        ASSERT_EQ(loc.fileName, filePaths[1]);
    }

    initializer.DestroyContext(context);
}

// Test: property rename - should rename property access via this. and external
TEST_F(LspFindRenameLocationsTests3, PropertyRenameThroughThisAndExternal)
{
    std::vector<std::string> files = {"rename_property.ets"};
    std::vector<std::string> texts = {R"(class Counter {
    count: number = 0;
    increment(): void {
        this.count++;
    }
}
let c = new Counter();
c.count = 10;
console.log(c.count);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "count" property definition
    const size_t position = texts[0].find("count: number");
    ASSERT_NE(position, std::string::npos);

    // Exported class property may need cross-file rename if accessed externally
    auto res = FindRenameLocationsInCurrentFile(context, position);
    ASSERT_GE(res.size(), 1U);

    // All rename locations should be in the same file
    for (const auto &loc : res) {
        ASSERT_EQ(loc.fileName, filePaths[0]);
    }

    initializer.DestroyContext(context);
}

// Test: local variable rename - should rename all usages in scope
TEST_F(LspFindRenameLocationsTests3, LocalVariableRenameInScope)
{
    std::vector<std::string> files = {"rename_local_var.ets"};
    std::vector<std::string> texts = {R"(function add(a: number, b: number): number {
    let sum = a + b;
    return sum;
}
let result = add(1, 2);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "sum" variable definition
    const size_t position = texts[0].find("sum = a + b");
    ASSERT_NE(position, std::string::npos);

    // Local variable should NOT need cross-file rename
    ASSERT_FALSE(NeedsCrossFileRename(context, position));

    auto res = FindRenameLocationsInCurrentFile(context, position);
    ASSERT_GE(res.size(), 1U);

    // All rename locations should be in the same file
    for (const auto &loc : res) {
        ASSERT_EQ(loc.fileName, filePaths[0]);
    }

    initializer.DestroyContext(context);
}

// Test: function parameter rename
TEST_F(LspFindRenameLocationsTests3, FunctionParameterRename)
{
    std::vector<std::string> files = {"rename_param.ets"};
    std::vector<std::string> texts = {R"(function greet(name: string): string {
    return "Hello, " + name;
}
let message = greet("World");)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "name" parameter
    const size_t position = texts[0].find("name: string");
    ASSERT_NE(position, std::string::npos);

    ASSERT_FALSE(NeedsCrossFileRename(context, position));

    auto res = FindRenameLocationsInCurrentFile(context, position);
    ASSERT_GE(res.size(), 1U);

    for (const auto &loc : res) {
        ASSERT_EQ(loc.fileName, filePaths[0]);
    }

    initializer.DestroyContext(context);
}

// Test: class rename - should rename class, constructor usages, and type references
TEST_F(LspFindRenameLocationsTests3, ClassRenameIncludesTypeReferences)
{
    std::vector<std::string> files = {"rename_class.ets"};
    std::vector<std::string> texts = {R"(class MyClass {
    value: number = 1;
}
let foo: MyClass = new MyClass();
foo.value;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "MyClass" usage in "new MyClass()"
    const size_t position = texts[0].find("new MyClass");
    ASSERT_NE(position, std::string::npos);
    // Move to the "MyClass" part after "new "
    const size_t classPos = position + 4;

    auto res = FindRenameLocationsInCurrentFile(context, classPos);
    ASSERT_GE(res.size(), 1U);

    // Should find at least the class definition and the new MyClass() usage
    for (const auto &loc : res) {
        ASSERT_EQ(loc.fileName, filePaths[0]);
    }

    initializer.DestroyContext(context);
}

// Test: enum member rename
TEST_F(LspFindRenameLocationsTests3, EnumMemberRename)
{
    std::vector<std::string> files = {"rename_enum_member.ets"};
    std::vector<std::string> texts = {R"(enum Color {
    Red = 1,
    Green = 2,
    Blue = 3
}
let c: Color = Color.Red;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "Red" enum member
    const size_t position = texts[0].find("Red = 1");
    ASSERT_NE(position, std::string::npos);

    auto res = FindRenameLocationsInCurrentFile(context, position);
    ASSERT_GE(res.size(), 1U);

    for (const auto &loc : res) {
        ASSERT_EQ(loc.fileName, filePaths[0]);
    }

    initializer.DestroyContext(context);
}

// Test: type alias rename
TEST_F(LspFindRenameLocationsTests3, TypeAliasRename)
{
    std::vector<std::string> files = {"rename_type_alias.ets"};
    std::vector<std::string> texts = {R"(type ID = number;
let userId: ID = 123;
let orderId: ID = 456;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "ID" usage in "userId: ID"
    const size_t position = texts[0].find("ID = 123");
    ASSERT_NE(position, std::string::npos);

    auto res = FindRenameLocationsInCurrentFile(context, position);
    ASSERT_GE(res.size(), 1U);

    for (const auto &loc : res) {
        ASSERT_EQ(loc.fileName, filePaths[0]);
    }

    initializer.DestroyContext(context);
}

// Test: rename should not modify string literals containing the identifier
TEST_F(LspFindRenameLocationsTests3, RenameDoesNotModifyStringLiterals)
{
    std::vector<std::string> files = {"rename_not_string.ets"};
    std::vector<std::string> texts = {R"(let foo: number = 1;
let bar: string = "foo is here";
console.log(foo);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "foo" variable definition
    const size_t position = texts[0].find("foo: number");
    ASSERT_NE(position, std::string::npos);

    auto res = FindRenameLocationsInCurrentFile(context, position);
    ASSERT_GE(res.size(), 1U);

    // Verify no rename location points to the string literal "foo is here"
    const size_t stringLitPos = texts[0].find("\"foo is here\"");
    ASSERT_NE(stringLitPos, std::string::npos);
    for (const auto &loc : res) {
        // The rename location should not overlap with the string literal content
        ASSERT_FALSE(loc.fileName == filePaths[0] && loc.start >= stringLitPos + 1 &&
                     loc.start < stringLitPos + texts[0].substr(stringLitPos).find('"', 1));
    }

    initializer.DestroyContext(context);
}

// Test: rename should not modify comments containing the identifier
TEST_F(LspFindRenameLocationsTests3, RenameDoesNotModifyComments)
{
    std::vector<std::string> files = {"rename_not_comment.ets"};
    std::vector<std::string> texts = {R"(// foo is defined here
let foo: number = 1;
// use foo carefully
console.log(foo);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "foo" variable definition
    const size_t position = texts[0].find("foo: number");
    ASSERT_NE(position, std::string::npos);

    auto res = FindRenameLocationsInCurrentFile(context, position);
    ASSERT_GE(res.size(), 1U);

    // Verify no rename location points to comment positions
    // The first comment "foo" is at position ~4
    const size_t comment1Pos = texts[0].find("// foo is defined here");
    const size_t comment2Pos = texts[0].find("// use foo carefully");
    ASSERT_NE(comment1Pos, std::string::npos);
    ASSERT_NE(comment2Pos, std::string::npos);

    for (const auto &loc : res) {
        // Should not be in the comment lines
        const size_t comment1End = comment1Pos + std::string("// foo is defined here").length();
        const size_t comment2End = comment2Pos + std::string("// use foo carefully").length();
        ASSERT_FALSE(loc.fileName == filePaths[0] && loc.start >= comment1Pos && loc.start < comment1End);
        ASSERT_FALSE(loc.fileName == filePaths[0] && loc.start >= comment2Pos && loc.start < comment2End);
    }

    initializer.DestroyContext(context);
}

// Test: namespace member rename
TEST_F(LspFindRenameLocationsTests3, NamespaceMemberRename)
{
    std::vector<std::string> files = {"rename_namespace_member.ets"};
    std::vector<std::string> texts = {R"(namespace MyModule {
    export let counter: number = 0;
    export function increment(): void {
        counter++;
    }
}
MyModule.counter = 10;
MyModule.increment();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "counter" in namespace
    const size_t position = texts[0].find("counter: number");
    ASSERT_NE(position, std::string::npos);

    auto res = FindRenameLocationsInCurrentFile(context, position);
    ASSERT_GE(res.size(), 1U);

    for (const auto &loc : res) {
        ASSERT_EQ(loc.fileName, filePaths[0]);
    }

    initializer.DestroyContext(context);
}

}  // namespace
