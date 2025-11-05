/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
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
using ark::es2panda::lsp::RenameLocation;
class LspFindRenameLocationsTests : public LSPAPITests {};

TEST_F(LspFindRenameLocationsTests, LocalNameNeedsCrossFileRename)
{
    const std::string fileContent = R"(function add(a:number, b:number): number {
  let sum = a + b;
  return sum;
}
add(123, 456);
class Foo {name: string = "john";}
)";
    Initializer initializer = Initializer();
    auto context =
        initializer.CreateContext("find_rename_locations_cross_test.ets", ES2PANDA_STATE_CHECKED, fileContent.c_str());
    ASSERT_FALSE(NeedsCrossFileRename(context, fileContent.find("sum =")));   // sum
    ASSERT_FALSE(NeedsCrossFileRename(context, fileContent.find("Foo {")));   // Foo
    ASSERT_FALSE(NeedsCrossFileRename(context, fileContent.find("name: ")));  // Foo name
    initializer.DestroyContext(context);
}

TEST_F(LspFindRenameLocationsTests, ImportNeedsCrossFileRename)
{
    std::vector<std::string> files = {"rename_cross_check_export.ets", "rename_cross_check_import.ets"};
    std::vector<std::string> texts = {R"(export class Foo {name: string = "john"}
export function add(): number {return 123;}
)",
                                      R"(import { Foo, add } from './rename_cross_check_export.ets';
let foo: Foo = new Foo();
console.log(foo.name);
console.log(add());
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();

    auto context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_FALSE(NeedsCrossFileRename(context, texts[1].find("Foo()")));  // Foo
    ASSERT_TRUE(NeedsCrossFileRename(context, texts[1].find("name)")));   // name
    ASSERT_FALSE(NeedsCrossFileRename(context, texts[1].find("add()")));  // add
    initializer.DestroyContext(context);
}

TEST_F(LspFindRenameLocationsTests, ExportNeedsCrossFileRename)
{
    Initializer initializer = Initializer();
    std::string fileContent = R"(
export class Foo {
  name: string = "john";
  private age: number = 20;
  public getAge(): number {
    return this.age;
  }
  private privateF1(): number {
    const num: number = 1;
    return num;
  }
}
export function add(a: number, b: number) {
  const sum = a + b;
  return sum;
}
export const arrowFunc = () => "arrow";
export let xxx = 123;
export const yyy = 456;
export type Status = "pending" | "completed" | "failed";
export enum Color {
  Red = "red",
  Green = "green",
  Blue = "blue"
}
)";
    auto context =
        initializer.CreateContext("find_rename_locations_cross_test1.ets", ES2PANDA_STATE_CHECKED, fileContent.c_str());
    ASSERT_TRUE(NeedsCrossFileRename(context, fileContent.find("Foo {")));         // Foo
    ASSERT_TRUE(NeedsCrossFileRename(context, fileContent.find("name: string")));  // Foo name
    ASSERT_FALSE(NeedsCrossFileRename(context, fileContent.find("age: number")));  // Foo age
    ASSERT_TRUE(NeedsCrossFileRename(context, fileContent.find("getAge()")));      // Foo getAge
    ASSERT_FALSE(NeedsCrossFileRename(context, fileContent.find("privateF1()")));  // Foo privateF1
    ASSERT_TRUE(NeedsCrossFileRename(context, fileContent.find("add(a:")));        // add
    ASSERT_FALSE(NeedsCrossFileRename(context, fileContent.find("sum = a + b")));  // add sum
    ASSERT_TRUE(NeedsCrossFileRename(context, fileContent.find("arrowFunc")));     // arrowFunc
    ASSERT_TRUE(NeedsCrossFileRename(context, fileContent.find("xxx")));           // xxx
    ASSERT_TRUE(NeedsCrossFileRename(context, fileContent.find("yyy")));           // yyy
    ASSERT_TRUE(NeedsCrossFileRename(context, fileContent.find("Status")));        // Status
    ASSERT_TRUE(NeedsCrossFileRename(context, fileContent.find("Color")));         // Color
    ASSERT_TRUE(NeedsCrossFileRename(context, fileContent.find("Red")));           // Color Red
    initializer.DestroyContext(context);
}

TEST_F(LspFindRenameLocationsTests, ClassAlias)
{
    std::vector<std::string> files = {"rename_cross_check_export1.ets", "rename_cross_check_import1.ets"};
    std::vector<std::string> texts = {R"(export class Foo {name: string = "john"})",
                                      R"(import { Foo as Fooo } from './rename_cross_check_export1.ets';
let foo: Fooo = new Fooo();
console.log(foo.name);
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();

    auto context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_FALSE(NeedsCrossFileRename(context, texts[1].find("Fooo()")));  // Fooo
    auto res = FindRenameLocationsInCurrentFile(context, texts[1].find("Fooo()"));
    const auto expected = std::vector<RenameLocation> {
        {filePaths[1], 16, 20, 0},
        {filePaths[1], 73, 77, 1},
        {filePaths[1], 84, 88, 1},
    };
    const auto actual = std::vector<RenameLocation>(res.begin(), res.end());
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        ASSERT_EQ(actual[i].fileName, expected[i].fileName);
        ASSERT_EQ(actual[i].start, expected[i].start);
        ASSERT_EQ(actual[i].end, expected[i].end);
        ASSERT_EQ(actual[i].line, expected[i].line);
    }
    initializer.DestroyContext(context);
}
}  // namespace
