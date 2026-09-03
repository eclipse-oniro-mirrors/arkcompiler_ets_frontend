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

#include <gtest/gtest.h>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"

using ark::es2panda::lsp::Initializer;

namespace {

constexpr size_t DYNAMIC_DEPENDENCY_INDEX = 0U;
constexpr size_t STATIC_IMPLEMENTATION_INDEX = 1U;
constexpr size_t STATIC_DECLARATION_INDEX = 2U;
constexpr size_t DEPENDENCY_USE_INDEX = 3U;
constexpr size_t DEPENDENCY_CONFIG_INDEX = 4U;
constexpr size_t DEPENDENCY_FILE_COUNT = 5U;
constexpr std::string_view DEFAULT_ORDER_STATUS_SOURCE = R"('use static'
export interface EnumOrderStatus {
    Pending: string;
    Shipped: string;
    Delivered: string;
    Cancelled: string;
}
export interface InterOrder {
    OrderStatus: EnumOrderStatus;
}
export default {
    OrderStatus: {
        Pending: 'PENDING',
        Shipped: 'SHIPPED',
        Delivered: 'DELIVERED',
        Cancelled: 'CANCELLED',
    } as EnumOrderStatus,
} as InterOrder;
)";
constexpr std::string_view INVALID_TYPE_ONLY_EXPORT_SOURCE = R"('use static'
let t_type_var_int: int = 0;
let type_var_int: int = 1;
export type { type_var_int };
)";
constexpr std::string_view INVALID_TYPE_ONLY_IMPORT_SOURCE = R"('use static'
import type { type_var_int as t_type_var_int } from './invalid_type_only_export';
let value: t_type_var_int;
)";
constexpr std::string_view INVALID_TYPE_ONLY_VALUE_IMPORT_SOURCE = R"('use static'
import { type_var_int } from './invalid_type_only_export';
)";
constexpr std::string_view AMBIGUOUS_EXPORT_SOURCE = R"(let type_var_int: int = 0;
export type { type_var_int };
export * from './ambiguous_export_a';
export * from './ambiguous_export_b';)";
constexpr std::string_view VALID_FUNCTION_ANNOTATION_EXPORT_SOURCE = R"('use static'
export function validFunction(): void {}
export @interface ValidAnnotation {}
)";
constexpr std::string_view VALID_FUNCTION_ANNOTATION_IMPORT_SOURCE = R"('use static'
import { validFunction, ValidAnnotation } from './valid_function_annotation_export';
validFunction();
@ValidAnnotation
class AnnotatedClass {}
)";
constexpr std::string_view INVALID_FUNCTION_ANNOTATION_EXPORT_SOURCE = R"('use static'
function invalidFunction(): void {}
@interface InvalidAnnotation {}
export type { invalidFunction, InvalidAnnotation };
)";
constexpr std::string_view INVALID_FUNCTION_ANNOTATION_IMPORT_SOURCE = R"('use static'
import type { invalidFunction as FunctionType, InvalidAnnotation as AnnotationType } from
    './invalid_function_annotation_export';
let functionValue: FunctionType;
let annotationValue: AnnotationType;
)";

struct DefinitionExpectation {
    std::string_view clickedName;
    const std::string &targetFile;
    size_t targetPosition;
    std::string_view targetName;
};

void ExpectDefinitionForToken(const LSPAPI *lspApi, es2panda_Context *ctx, size_t position,
                              const DefinitionExpectation &expected)
{
    for (size_t current = position; current <= position + expected.clickedName.size(); current++) {
        auto result = lspApi->getDefinitionAtPosition(ctx, current);
        EXPECT_EQ(result.fileName, expected.targetFile) << "name=" << expected.clickedName << ", position=" << current;
        EXPECT_EQ(result.start, expected.targetPosition) << "name=" << expected.clickedName << ", position=" << current;
        EXPECT_EQ(result.length, expected.targetName.size())
            << "name=" << expected.clickedName << ", position=" << current;
    }
}

void ExpectNoDefinitionForToken(const LSPAPI *lspApi, es2panda_Context *ctx, size_t position, std::string_view name)
{
    for (size_t current = position; current <= position + name.size(); current++) {
        auto result = lspApi->getDefinitionAtPosition(ctx, current);
        EXPECT_TRUE(result.fileName.empty()) << "name=" << name << ", position=" << current;
        EXPECT_EQ(result.start, 0U) << "name=" << name << ", position=" << current;
        EXPECT_EQ(result.length, 0U) << "name=" << name << ", position=" << current;
    }
}

std::string MakeDependencyConfig(const std::filesystem::path &tempDir, const std::vector<std::string> &files)
{
    const auto dynamicDependencyPath = (tempDir / files[DYNAMIC_DEPENDENCY_INDEX]).string();
    const auto staticImplementationPath = (tempDir / files[STATIC_IMPLEMENTATION_INDEX]).string();
    const auto staticDeclarationPath = (tempDir / files[STATIC_DECLARATION_INDEX]).string();
    return R"({"compilerOptions": {"baseUrl": ")" + tempDir.string() +
           R"(", "dependencies": {"js": {"language": "js", "path": ")" + dynamicDependencyPath +
           R"(", "ohmUrl": "js"}, "staticLib": {"language": "ets", "path": ")" + staticDeclarationPath +
           R"(", "sourceFilePath": ")" + staticImplementationPath + R"("}}}})";
}

}  // namespace

class LspGetDefTests : public LSPAPITests {};

TEST_F(LspGetDefTests, GetDefinitionAtPosition1)
{
    std::vector<std::string> files = {"getDefinitionAtPosition1.ets", "getDefinitionAtPosition2.ets"};
    std::vector<std::string> texts = {R"(export function A(a:number, b:number): number {
    return a + b;
})",
                                      R"(import {A} from './getDefinitionAtPosition1';
A(1, 2);)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 46;             // left of 'A'
    size_t const offsetEnd = 47;          // right of 'A'
    size_t const offsetLeftBracket = 48;  // right of '('
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    auto resultEnd = lspApi->getDefinitionAtPosition(ctx, offsetEnd);
    auto resultLeftBracket = lspApi->getDefinitionAtPosition(ctx, offsetLeftBracket);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 16;
    size_t const expectedLength = 1;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
    ASSERT_EQ(resultEnd.fileName, expectedFileName);
    ASSERT_EQ(resultEnd.start, expectedStart);
    ASSERT_EQ(resultEnd.length, expectedLength);
    ASSERT_EQ(resultLeftBracket.fileName, "");
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition2)
{
    std::vector<std::string> files = {"getDefinitionAtPosition3.ets"};
    std::vector<std::string> texts = {R"(
    function A(a:number, b:number) {
        return a + b;
    }
    A(1, 2);
    function A(a:number) {
        return a;
    }
    A(1);)"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 70;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 14;
    size_t const expectedLength = 1;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);

    size_t const offset1 = 134;
    auto ctx1 = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto result1 = lspApi->getDefinitionAtPosition(ctx1, offset1);
    initializer.DestroyContext(ctx1);
    std::string expectedFileName1 = filePaths[0];
    size_t const expectedStart1 = 92;
    size_t const expectedLength1 = 1;
    ASSERT_EQ(result1.fileName, expectedFileName1);
    ASSERT_EQ(result1.start, expectedStart1);
    ASSERT_EQ(result1.length, expectedLength1);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition3)
{
    std::vector<std::string> files = {"getDefinitionAtPosition4.ets", "getDefinitionAtPosition5.ets"};
    std::vector<std::string> texts = {R"(export function A(a:number, b:number): number {
    return a + b;
})",
                                      R"(import * as All from './getDefinitionAtPosition4';
All.A(1, 2);)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 55;
    size_t const offsetEnd = 56;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    auto resultEnd = lspApi->getDefinitionAtPosition(ctx, offsetEnd);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 16;
    size_t const expectedLength = 1;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
    ASSERT_EQ(resultEnd.fileName, expectedFileName);
    ASSERT_EQ(resultEnd.start, expectedStart);
    ASSERT_EQ(resultEnd.length, expectedLength);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition4)
{
    std::vector<std::string> files = {"getDefinitionAtPosition6.ets", "getDefinitionAtPosition7.ets"};
    std::vector<std::string> texts = {R"(export class A {
Foo(a:number, b:number): number {
    return a + b;
}})",
                                      R"(import * as All from './getDefinitionAtPosition6';
let a = new All.A();
a.Foo(1, 2);)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 74;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 17;
    size_t const expectedLength = 3;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition5)
{
    std::vector<std::string> files = {"getDefinitionAtPosition8.ets", "getDefinitionAtPosition9.ets"};
    std::vector<std::string> texts = {R"(export enum A {
a,
b})",
                                      R"(import * as All from './getDefinitionAtPosition8';
All.A.a;)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 57;
    size_t const offsetEnd = 58;  // right of 'a'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    auto resultEnd = lspApi->getDefinitionAtPosition(ctx, offsetEnd);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 16;
    size_t const expectedLength = 1;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
    ASSERT_EQ(resultEnd.fileName, expectedFileName);
    ASSERT_EQ(resultEnd.start, expectedStart);
    ASSERT_EQ(resultEnd.length, expectedLength);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition6)
{
    std::vector<std::string> files = {"getDefinitionAtPosition10.ets", "getDefinitionAtPosition11.ets"};
    std::vector<std::string> texts = {R"(export class A {
Foo(a:number, b:number): number {
    return a + b;
}};
)",
                                      R"(import {A} from './getDefinitionAtPosition10';
let a = new A();
a.Foo(1, 2);)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 66;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 17;
    size_t const expectedLength = 3;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition7)
{
    std::vector<std::string> files = {"getDefinitionAtPosition12.ets", "getDefinitionAtPosition13.ets"};
    std::vector<std::string> texts = {R"(export let a = 1;)",
                                      R"(import {a} from './getDefinitionAtPosition12';
let b = a;)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 55;
    size_t const offsetEnd = 56;  // right of 'a'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    auto resultEnd = lspApi->getDefinitionAtPosition(ctx, offsetEnd);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 11;
    size_t const expectedLength = 1;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
    ASSERT_EQ(resultEnd.fileName, expectedFileName);
    ASSERT_EQ(resultEnd.start, expectedStart);
    ASSERT_EQ(resultEnd.length, expectedLength);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition8)
{
    std::vector<std::string> files = {"getDefinitionAtPosition14.ets", "getDefinitionAtPosition15.ets"};
    std::vector<std::string> texts = {R"(export interface I {})",
                                      R"(import {I} from './getDefinitionAtPosition14';
import * as All from './getDefinitionAtPosition14';
class A implements All.I {};)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 8;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 17;
    size_t const expectedLength = 1;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);

    size_t const offset1 = 122;
    ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    result = lspApi->getDefinitionAtPosition(ctx, offset1);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition9)
{
    std::vector<std::string> files = {"getDefinitionAtPosition16.ets", "getDefinitionAtPosition17.ets"};
    std::vector<std::string> texts = {R"(export class Foo {
Foo(a:number, b:number): number {
    return a + b;
}})",
                                      R"(import * as All from './getDefinitionAtPosition16';
let a = new All.Foo();
a.Foo(1, 2);)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 68;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 13;
    size_t const expectedLength = 3;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);

    size_t const offset1 = 77;
    ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result1 = lspApi->getDefinitionAtPosition(ctx, offset1);
    initializer.DestroyContext(ctx);
    std::string expectedFileName1 = filePaths[0];
    size_t const expectedStart1 = 19;
    size_t const expectedLength1 = 3;
    ASSERT_EQ(result1.fileName, expectedFileName1);
    ASSERT_EQ(result1.start, expectedStart1);
    ASSERT_EQ(result1.length, expectedLength1);
}

TEST_F(LspGetDefTests, GetDefinitionAtPositionForSpecialCharacters)
{
    std::vector<std::string> files = {"getDefinitionAtPosition18.ets", "getDefinitionAtPosition19.ets"};
    std::vector<std::string> texts = {R"(
//中文测试
export class Foo {
//中文测试
Foo(a:number, b:number): number {
    return a + b;
}})",
                                      R"(import * as All from './getDefinitionAtPosition18';
//中文测试
let a = new All.Foo();
a.Foo(1, 2);)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 76;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 21;
    size_t const expectedLength = 3;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);

    size_t const offset1 = 85;
    ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result1 = lspApi->getDefinitionAtPosition(ctx, offset1);
    initializer.DestroyContext(ctx);
    std::string expectedFileName1 = filePaths[0];
    size_t const expectedStart1 = 34;
    size_t const expectedLength1 = 3;
    ASSERT_EQ(result1.fileName, expectedFileName1);
    ASSERT_EQ(result1.start, expectedStart1);
    ASSERT_EQ(result1.length, expectedLength1);
}

TEST_F(LspGetDefTests, GetDefinitionAtPositionForStdLibraryTaskPool)
{
    std::vector<std::string> files = {"getDefinitionAtPositionForStdLibraryTaskPool.ets"};
    std::vector<std::string> texts = {R"(const task = new taskpool.Task(()=>{});)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 21;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = "std/concurrency.etscache";
    size_t const expectedLength = 8;
    ASSERT_TRUE(result.fileName.find(expectedFileName) != std::string::npos);
    ASSERT_EQ(result.length, expectedLength);
}

TEST_F(LspGetDefTests, DisableLoweringTest1)
{
    std::vector<std::string> files = {"DefaultParameters.ets"};
    std::vector<std::string> texts = {R"(
    export class AAA {
        constructor(a : number, b : string = "a") {}
    }
    let a = new AAA(1);)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offsetEnd = 102;
    size_t const offset = 100;
    ark::es2panda::EHeap::Scope eheapScope;  // remove this line #32069
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContextWithCache(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED, texts);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    auto resultEnd = lspApi->getDefinitionAtPosition(ctx, offsetEnd);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 18;
    size_t const expectedLength = 3;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
    ASSERT_EQ(resultEnd.fileName, expectedFileName);
    ASSERT_EQ(resultEnd.start, expectedStart);
    ASSERT_EQ(resultEnd.length, expectedLength);
}

TEST_F(LspGetDefTests, DisableLoweringTest2)
{
    std::vector<std::string> files = {"RestTuple.ets"};
    std::vector<std::string> texts = {R"(
    function sum(a : int, ...numbers : [number, number]) {
        return a + numbers[0] + numbers[1];
    }
    let a = sum(1, 2, 3);)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 123;
    ark::es2panda::EHeap::Scope eheapScope;  // remove this line #32069
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContextWithCache(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED, texts);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 14;
    size_t const expectedLength = 3;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
}

TEST_F(LspGetDefTests, DisableLoweringTest3)
{
    std::vector<std::string> files = {"CapturedVariable.ets"};
    std::vector<std::string> texts = {R"(
    let aaa = 1;
    let lam : () => void = () => {
        aaa = 2
    };
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 62;
    ark::es2panda::EHeap::Scope eheapScope;  // remove this line #32069
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContextWithCache(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED, texts);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 9;
    size_t const expectedLength = 3;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
}

TEST_F(LspGetDefTests, DisableLoweringTest4)
{
    std::vector<std::string> files = {"SetJumpTarget.ets"};
    std::vector<std::string> texts = {R"(
    let iii : number = 0;
    do {
        iii++;
    } while (iii < 10);
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 45;
    ark::es2panda::EHeap::Scope eheapScope;  // remove this line #32069
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContextWithCache(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED, texts);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 9;
    size_t const expectedLength = 3;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_ImportNamespaceAliasDeclaration)
{
    std::vector<std::string> files = {"getDefinitionAtPositionNamespaceAliasDeclarationSource.ets",
                                      "getDefinitionAtPositionNamespaceAliasDeclarationUse.ets"};
    std::vector<std::string> texts = {
        R"('use static'
export interface GeneratedObjectLiteralInterface_1 {
    bool: boolean;
    bool1: boolean;
}
export const appConfig7: GeneratedObjectLiteralInterface_1 = {
    bool1: false,
    bool: true,
} as GeneratedObjectLiteralInterface_1;
)",
        R"(import * as appConfig8 from './getDefinitionAtPositionNamespaceAliasDeclarationSource';)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    const auto offset = texts[1].find("appConfig8");
    ASSERT_NE(offset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    for (size_t position = offset; position <= offset + std::string_view("appConfig8").size(); position++) {
        auto result = lspApi->getDefinitionAtPosition(ctx, position);
        EXPECT_EQ(result.fileName, filePaths[0]) << "position=" << position;
        EXPECT_EQ(result.start, 0U) << "position=" << position;
        EXPECT_EQ(result.length, 0U) << "position=" << position;
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_ImportNamespaceModulePath)
{
    std::vector<std::string> files = {"getDefinitionAtPositionNamespacePathSource.ets",
                                      "getDefinitionAtPositionNamespacePathUse.ets"};
    const std::string importPath = "./getDefinitionAtPositionNamespacePathSource";
    std::vector<std::string> texts = {R"(export class A {})",
                                      R"(import * as aaaa from './getDefinitionAtPositionNamespacePathSource';)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    const auto offset = texts[1].find(importPath);
    ASSERT_NE(offset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(result.fileName, filePaths[0]);
    ASSERT_EQ(result.start, 0U);
    ASSERT_EQ(result.length, 0U);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_ArktsconfigDependency)
{
    std::vector<std::string> files = {
        "getDefinitionAtPositionDynamicDependency.d.ets", "getDefinitionAtPositionStaticDependency.ets",
        "getDefinitionAtPositionStaticDependency.d.ets", "getDefinitionAtPositionDependencyUse.ets",
        "getDefinitionAtPositionDependencyArktsconfig.json"};
    const auto tempDir = std::filesystem::path(testing::TempDir()).append(GetExecutableName());
    const auto configText = MakeDependencyConfig(tempDir, files);
    std::vector<std::string> texts = {R"(export declare class DynamicA {})", R"(export class StaticA {})",
                                      R"(export declare class StaticA {})",
                                      R"('use static'

import * as jsModule from 'js';
import * as staticModule from 'staticLib';)",
                                      configText};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), DEPENDENCY_FILE_COUNT);

    const auto configOption = "--arktsconfig=" + filePaths[DEPENDENCY_CONFIG_INDEX];
    std::array<const char *, 2U> args = {filePaths[DEPENDENCY_USE_INDEX].c_str(), configOption.c_str()};
    auto *compilerImpl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    ASSERT_NE(compilerImpl, nullptr);

    auto configDeleter = [compilerImpl](es2panda_Config *config) { compilerImpl->DestroyConfigWithoutLog(config); };
    std::unique_ptr<es2panda_Config, decltype(configDeleter)> config(
        compilerImpl->CreateConfig(args.size(), args.data()), configDeleter);
    ASSERT_NE(config, nullptr);

    auto contextDeleter = [compilerImpl](es2panda_Context *context) { compilerImpl->DestroyContext(context); };
    std::unique_ptr<es2panda_Context, decltype(contextDeleter)> context(
        compilerImpl->CreateContextFromFile(config.get(), filePaths[DEPENDENCY_USE_INDEX].c_str()), contextDeleter);
    ASSERT_NE(context, nullptr);
    compilerImpl->ProceedToState(context.get(), ES2PANDA_STATE_BOUND);

    const auto dynamicAliasOffset = texts[DEPENDENCY_USE_INDEX].find("jsModule");
    const auto dynamicSourceOffset = texts[DEPENDENCY_USE_INDEX].rfind("js");
    const auto staticAliasOffset = texts[DEPENDENCY_USE_INDEX].find("staticModule");
    const auto staticSourceOffset = texts[DEPENDENCY_USE_INDEX].rfind("staticLib");
    ASSERT_NE(dynamicAliasOffset, std::string::npos);
    ASSERT_NE(dynamicSourceOffset, std::string::npos);
    ASSERT_NE(staticAliasOffset, std::string::npos);
    ASSERT_NE(staticSourceOffset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    const std::array<std::pair<size_t, std::string>, 4U> expectedDefinitions = {
        {{dynamicAliasOffset, filePaths[DYNAMIC_DEPENDENCY_INDEX]},
         {dynamicSourceOffset, filePaths[DYNAMIC_DEPENDENCY_INDEX]},
         {staticAliasOffset, filePaths[STATIC_DECLARATION_INDEX]},
         {staticSourceOffset, filePaths[STATIC_DECLARATION_INDEX]}}};
    for (const auto &[offset, expectedFile] : expectedDefinitions) {
        const auto result = lspApi->getDefinitionAtPosition(context.get(), offset);
        EXPECT_EQ(result.fileName, expectedFile);
        EXPECT_EQ(result.start, 0U);
        EXPECT_EQ(result.length, 0U);
    }
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_ImportNamespaceAliasUsage)
{
    std::vector<std::string> files = {"getDefinitionAtPositionNamespaceAliasUsageSource.ets",
                                      "getDefinitionAtPositionNamespaceAliasUsageUse.ets"};
    std::vector<std::string> texts = {R"(export class A {})",
                                      R"(import * as aaaa from './getDefinitionAtPositionNamespaceAliasUsageSource';
let instance = new aaaa.A();)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    const auto offset = texts[1].rfind("aaaa");
    ASSERT_NE(offset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(result.fileName, filePaths[0]);
    ASSERT_EQ(result.start, 0U);
    ASSERT_EQ(result.length, 0U);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_ReExportNamespaceAliasInDeclaration)
{
    std::vector<std::string> files = {"getDefinitionAtPositionReExportNamespaceAliasSource.ets",
                                      "getDefinitionAtPositionReExportNamespaceAliasUse.ets"};
    std::vector<std::string> texts = {
        R"(export class A {})",
        R"(export * as components from './getDefinitionAtPositionReExportNamespaceAliasSource';)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    const auto offset = texts[1].find("components");
    ASSERT_NE(offset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(result.fileName, filePaths[0]);
    ASSERT_EQ(result.start, 0U);
    ASSERT_EQ(result.length, 0U);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_ImportReExportNamespaceAlias)
{
    std::vector<std::string> files = {"EnumOrderStatus.ets", "UtilOrderStatus1.ets", "import_order_status.ets"};
    std::vector<std::string> texts = {
        R"('use static'
export enum EnumOrderStatus { Pending = 'PENDING' })",
        R"(export * as OrderStatus5 from './EnumOrderStatus';)",
        R"(import { OrderStatus5 } from './UtilOrderStatus1';)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto offset = texts[2].find("OrderStatus5");
    ASSERT_NE(offset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    for (size_t position = offset; position <= offset + std::string_view("OrderStatus5").size(); position++) {
        auto result = lspApi->getDefinitionAtPosition(ctx, position);
        EXPECT_EQ(result.fileName, filePaths[0]) << "position=" << position;
        EXPECT_EQ(result.start, 0U) << "position=" << position;
        EXPECT_EQ(result.length, 0U) << "position=" << position;
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_ImportExportAliases)
{
    std::vector<std::string> files = {"export_as_default_array.ets", "import_default_array.ets"};
    std::vector<std::string> texts = {R"('use static'
let const_array1: Array<number> = [1, 2, 3, 4, 5, 6];
let const_array2: Array<number> = [11, 12, 13, 14, 15, 16];
export { const_array1 as default, const_array2 as const_array };
)",
                                      R"('use static'
import default_const_array, { const_array } from "./export_as_default_array";
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto defaultImportOffset = texts[1].find("default_const_array");
    const auto namedImportOffset = texts[1].find("const_array }");
    const auto defaultDeclarationOffset = texts[0].find("const_array1");
    const auto namedDeclarationOffset = texts[0].find("const_array2");
    ASSERT_NE(defaultImportOffset, std::string::npos);
    ASSERT_NE(namedImportOffset, std::string::npos);
    ASSERT_NE(defaultDeclarationOffset, std::string::npos);
    ASSERT_NE(namedDeclarationOffset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto defaultResult = lspApi->getDefinitionAtPosition(ctx, defaultImportOffset);
    auto namedResult = lspApi->getDefinitionAtPosition(ctx, namedImportOffset);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(defaultResult.fileName, filePaths[0]);
    EXPECT_EQ(defaultResult.start, defaultDeclarationOffset);
    EXPECT_EQ(defaultResult.length, std::string_view("const_array1").size());
    EXPECT_EQ(namedResult.fileName, filePaths[0]);
    EXPECT_EQ(namedResult.start, namedDeclarationOffset);
    EXPECT_EQ(namedResult.length, std::string_view("const_array2").size());
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_MultiDeclaratorExportAlias)
{
    std::vector<std::string> files = {"multi_declarator_export.ets", "multi_declarator_import.ets"};
    std::vector<std::string> texts = {R"('use static'
let first = 1, target = 2;
export { target as alias };
)",
                                      R"('use static'
import { alias as localAlias } from "./multi_declarator_export";
localAlias;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto declarationOffset = texts[0].find("target");
    const auto importOffset = texts[1].find("localAlias");
    const auto useOffset = texts[1].rfind("localAlias");
    ASSERT_NE(declarationOffset, std::string::npos);
    ASSERT_NE(importOffset, std::string::npos);
    ASSERT_NE(useOffset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ExpectDefinitionForToken(lspApi, ctx, importOffset, {"localAlias", filePaths[0], declarationOffset, "target"});
    ExpectDefinitionForToken(lspApi, ctx, useOffset, {"localAlias", filePaths[0], declarationOffset, "target"});
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_DefaultExportIdentifierAndNamedAlias)
{
    std::vector<std::string> files = {"DefaultLetArray.ets", "import_default_let_array.ets"};
    std::vector<std::string> texts = {R"('use static'
let arr: Array<number> = [1, 2, 3]
export default arr
export type typeArr<T> = Array<T>
export let arr1: Array<number> = [2, 3, 4]
let arr2: Array<number> = [3, 4, 5]
export { arr2 }
export { arr2 as arr3 }
)",
                                      R"(import arr, { arr1, arr2, arr3, typeArr } from './DefaultLetArray';)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto defaultImportOffset = texts[1].find("arr,");
    const auto aliasImportOffset = texts[1].find("arr3");
    const auto defaultDeclarationOffset = texts[0].find("arr:");
    const auto aliasDeclarationOffset = texts[0].find("arr2:");
    ASSERT_NE(defaultImportOffset, std::string::npos);
    ASSERT_NE(aliasImportOffset, std::string::npos);
    ASSERT_NE(defaultDeclarationOffset, std::string::npos);
    ASSERT_NE(aliasDeclarationOffset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    for (size_t position = defaultImportOffset; position <= defaultImportOffset + std::string_view("arr").size();
         position++) {
        auto result = lspApi->getDefinitionAtPosition(ctx, position);
        EXPECT_EQ(result.fileName, filePaths[0]) << "position=" << position;
        EXPECT_EQ(result.start, defaultDeclarationOffset) << "position=" << position;
        EXPECT_EQ(result.length, std::string_view("arr").size()) << "position=" << position;
    }
    for (size_t position = aliasImportOffset; position <= aliasImportOffset + std::string_view("arr3").size();
         position++) {
        auto result = lspApi->getDefinitionAtPosition(ctx, position);
        EXPECT_EQ(result.fileName, filePaths[0]) << "position=" << position;
        EXPECT_EQ(result.start, aliasDeclarationOffset) << "position=" << position;
        EXPECT_EQ(result.length, std::string_view("arr2").size()) << "position=" << position;
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_DefaultExportTypeAlias)
{
    std::vector<std::string> files = {"DefaultTypeArray.ets", "import_default_type_array.ets"};
    std::vector<std::string> texts = {R"('use static'
type typeArr<T> = Array<T>
export default typeArr
)",
                                      R"(import defaultTypeArr from './DefaultTypeArray';)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto importOffset = texts[1].find("defaultTypeArr");
    const auto declarationOffset = texts[0].find("typeArr");
    ASSERT_NE(importOffset, std::string::npos);
    ASSERT_NE(declarationOffset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    for (size_t position = importOffset; position <= importOffset + std::string_view("defaultTypeArr").size();
         position++) {
        auto result = lspApi->getDefinitionAtPosition(ctx, position);
        EXPECT_EQ(result.fileName, filePaths[0]) << "position=" << position;
        EXPECT_EQ(result.start, declarationOffset) << "position=" << position;
        EXPECT_EQ(result.length, std::string_view("typeArr").size()) << "position=" << position;
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_DefaultExportAnonymousConstant)
{
    std::vector<std::string> files = {"BoolExport.ets", "import_bool_export.ets"};
    std::vector<std::string> texts = {R"('use static'
export default true;
)",
                                      R"(import appConfig from './BoolExport';)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto importOffset = texts[1].find("appConfig");
    const auto declarationOffset = texts[0].find("true");
    ASSERT_NE(importOffset, std::string::npos);
    ASSERT_NE(declarationOffset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    for (size_t position = importOffset; position <= importOffset + std::string_view("appConfig").size(); position++) {
        auto result = lspApi->getDefinitionAtPosition(ctx, position);
        EXPECT_EQ(result.fileName, filePaths[0]) << "position=" << position;
        EXPECT_EQ(result.start, declarationOffset) << "position=" << position;
        EXPECT_EQ(result.length, std::string_view("true").size()) << "position=" << position;
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_DefaultExportAnonymousObject)
{
    const std::string sourceText(DEFAULT_ORDER_STATUS_SOURCE);
    std::vector<std::string> files = {"DefaultObjOrderStatus1.ets", "DefaultOrderStatus.ets",
                                      "import_default_order_status.ets"};
    std::vector<std::string> texts = {sourceText, sourceText,
                                      R"(import DefaultObjOrderStatus1 from './DefaultObjOrderStatus1';
import DefaultOrderStatus from './DefaultOrderStatus';)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto exportOffset = sourceText.find("export default");
    const auto declarationOffset = sourceText.find("{", exportOffset);
    const auto declarationEnd = sourceText.find(";", declarationOffset);
    ASSERT_NE(exportOffset, std::string::npos);
    ASSERT_NE(declarationOffset, std::string::npos);
    ASSERT_NE(declarationEnd, std::string::npos);

    const std::array<std::pair<std::string_view, size_t>, 2U> imports = {
        std::pair {std::string_view("DefaultObjOrderStatus1"), 0U},
        std::pair {std::string_view("DefaultOrderStatus"), 1U},
    };
    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    for (const auto &[name, fileIndex] : imports) {
        const auto importOffset = texts[2].find(name);
        ASSERT_NE(importOffset, std::string::npos);
        for (size_t position = importOffset; position <= importOffset + name.size(); position++) {
            auto result = lspApi->getDefinitionAtPosition(ctx, position);
            EXPECT_EQ(result.fileName, filePaths[fileIndex]) << "name=" << name << ", position=" << position;
            EXPECT_EQ(result.start, declarationOffset) << "name=" << name << ", position=" << position;
            EXPECT_EQ(result.length, declarationEnd - declarationOffset)
                << "name=" << name << ", position=" << position;
        }
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_ValidFunctionAndAnnotation)
{
    std::vector<std::string> files = {"valid_function_annotation_export.ets", "valid_function_annotation_import.ets"};
    std::vector<std::string> texts = {std::string(VALID_FUNCTION_ANNOTATION_EXPORT_SOURCE),
                                      std::string(VALID_FUNCTION_ANNOTATION_IMPORT_SOURCE)};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto functionDeclaration = texts[0].find("validFunction");
    const auto annotationDeclaration = texts[0].find("ValidAnnotation");
    const auto functionImport = texts[1].find("validFunction");
    const auto annotationImport = texts[1].find("ValidAnnotation");
    const auto functionUse = texts[1].rfind("validFunction");
    const auto annotationUse = texts[1].rfind("ValidAnnotation");
    ASSERT_NE(functionDeclaration, std::string::npos);
    ASSERT_NE(annotationDeclaration, std::string::npos);
    ASSERT_NE(functionImport, std::string::npos);
    ASSERT_NE(annotationImport, std::string::npos);
    ASSERT_NE(functionUse, std::string::npos);
    ASSERT_NE(annotationUse, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ExpectDefinitionForToken(lspApi, ctx, functionImport,
                             {"validFunction", filePaths[0], functionDeclaration, "validFunction"});
    ExpectDefinitionForToken(lspApi, ctx, annotationImport,
                             {"ValidAnnotation", filePaths[0], annotationDeclaration, "ValidAnnotation"});
    ExpectDefinitionForToken(lspApi, ctx, functionUse,
                             {"validFunction", filePaths[0], functionDeclaration, "validFunction"});
    ExpectDefinitionForToken(lspApi, ctx, annotationUse,
                             {"ValidAnnotation", filePaths[0], annotationDeclaration, "ValidAnnotation"});
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_InvalidTypeOnlyFunctionAndAnnotationImportClause)
{
    std::vector<std::string> files = {"invalid_function_annotation_export.ets",
                                      "invalid_function_annotation_import.ets"};
    std::vector<std::string> texts = {std::string(INVALID_FUNCTION_ANNOTATION_EXPORT_SOURCE),
                                      std::string(INVALID_FUNCTION_ANNOTATION_IMPORT_SOURCE)};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto functionDeclaration = texts[0].find("invalidFunction");
    const auto annotationDeclaration = texts[0].find("InvalidAnnotation");
    const auto functionImport = texts[1].find("invalidFunction");
    const auto functionAlias = texts[1].find("FunctionType");
    const auto annotationImport = texts[1].find("InvalidAnnotation");
    const auto annotationAlias = texts[1].find("AnnotationType");
    ASSERT_NE(functionDeclaration, std::string::npos);
    ASSERT_NE(annotationDeclaration, std::string::npos);
    ASSERT_NE(functionImport, std::string::npos);
    ASSERT_NE(functionAlias, std::string::npos);
    ASSERT_NE(annotationImport, std::string::npos);
    ASSERT_NE(annotationAlias, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ExpectDefinitionForToken(lspApi, ctx, functionImport,
                             {"invalidFunction", filePaths[0], functionDeclaration, "invalidFunction"});
    ExpectDefinitionForToken(lspApi, ctx, functionAlias,
                             {"FunctionType", filePaths[0], functionDeclaration, "invalidFunction"});
    ExpectDefinitionForToken(lspApi, ctx, annotationImport,
                             {"InvalidAnnotation", filePaths[0], annotationDeclaration, "InvalidAnnotation"});
    ExpectDefinitionForToken(lspApi, ctx, annotationAlias,
                             {"AnnotationType", filePaths[0], annotationDeclaration, "InvalidAnnotation"});
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_InvalidTypeOnlyFunctionAndAnnotationTypeUse)
{
    std::vector<std::string> files = {"invalid_function_annotation_export.ets",
                                      "invalid_function_annotation_import.ets"};
    std::vector<std::string> texts = {std::string(INVALID_FUNCTION_ANNOTATION_EXPORT_SOURCE),
                                      std::string(INVALID_FUNCTION_ANNOTATION_IMPORT_SOURCE)};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto functionUse = texts[1].rfind("FunctionType");
    const auto annotationUse = texts[1].rfind("AnnotationType");
    ASSERT_NE(functionUse, std::string::npos);
    ASSERT_NE(annotationUse, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ExpectNoDefinitionForToken(lspApi, ctx, functionUse, "FunctionType");
    ExpectNoDefinitionForToken(lspApi, ctx, annotationUse, "AnnotationType");
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_InvalidTypeOnlyValueExportImportClause)
{
    std::vector<std::string> files = {"invalid_type_only_export.ets", "invalid_type_only_import.ets"};
    std::vector<std::string> texts = {std::string(INVALID_TYPE_ONLY_EXPORT_SOURCE),
                                      std::string(INVALID_TYPE_ONLY_IMPORT_SOURCE)};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto declarationOffset = texts[0].find("type_var_int: int = 1");
    const auto importedOffset = texts[1].find("type_var_int as");
    const auto localOffset = texts[1].find("t_type_var_int }");
    ASSERT_NE(declarationOffset, std::string::npos);
    ASSERT_NE(importedOffset, std::string::npos);
    ASSERT_NE(localOffset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    for (const auto &[name, offset] : std::array {std::pair {std::string_view("type_var_int"), importedOffset},
                                                  std::pair {std::string_view("t_type_var_int"), localOffset}}) {
        for (size_t position = offset; position <= offset + name.size(); position++) {
            auto result = lspApi->getDefinitionAtPosition(ctx, position);
            EXPECT_EQ(result.fileName, filePaths[0]) << "name=" << name << ", position=" << position;
            EXPECT_EQ(result.start, declarationOffset) << "name=" << name << ", position=" << position;
            EXPECT_EQ(result.length, std::string_view("type_var_int").size())
                << "name=" << name << ", position=" << position;
        }
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_InvalidTypeOnlyValueExportValueImportClause)
{
    std::vector<std::string> files = {"invalid_type_only_export.ets", "invalid_type_only_value_import.ets"};
    std::vector<std::string> texts = {std::string(INVALID_TYPE_ONLY_EXPORT_SOURCE),
                                      std::string(INVALID_TYPE_ONLY_VALUE_IMPORT_SOURCE)};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto declarationOffset = texts[0].find("type_var_int: int = 1");
    const auto importOffset = texts[1].find("type_var_int");
    ASSERT_NE(declarationOffset, std::string::npos);
    ASSERT_NE(importOffset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    for (size_t position = importOffset; position <= importOffset + std::string_view("type_var_int").size();
         position++) {
        auto result = lspApi->getDefinitionAtPosition(ctx, position);
        EXPECT_EQ(result.fileName, filePaths[0]) << "position=" << position;
        EXPECT_EQ(result.start, declarationOffset) << "position=" << position;
        EXPECT_EQ(result.length, std::string_view("type_var_int").size()) << "position=" << position;
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_InvalidTypeOnlyValueExportTypeUse)
{
    std::vector<std::string> files = {"invalid_type_only_export.ets", "invalid_type_only_import.ets"};
    std::vector<std::string> texts = {std::string(INVALID_TYPE_ONLY_EXPORT_SOURCE),
                                      std::string(INVALID_TYPE_ONLY_IMPORT_SOURCE)};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const auto typeUseOffset = texts[1].rfind("t_type_var_int");
    ASSERT_NE(typeUseOffset, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    for (size_t position = typeUseOffset; position <= typeUseOffset + std::string_view("t_type_var_int").size();
         position++) {
        auto result = lspApi->getDefinitionAtPosition(ctx, position);
        EXPECT_TRUE(result.fileName.empty()) << "position=" << position;
        EXPECT_EQ(result.start, 0U) << "position=" << position;
        EXPECT_EQ(result.length, 0U) << "position=" << position;
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetDefTests, GetDefinitionAtPosition_InvalidTypeOnlyValueExportAmbiguousImport)
{
    std::vector<std::string> files = {"ambiguous_export_a.ets", "ambiguous_export_b.ets", "ambiguous_export.ets",
                                      "ambiguous_type_import.ets", "ambiguous_value_import.ets"};
    std::vector<std::string> texts = {"export let type_var_int: int = 1;", "export let type_var_int: int = 2;",
                                      std::string(AMBIGUOUS_EXPORT_SOURCE),
                                      "import type { type_var_int as t_type_var_int } from './ambiguous_export';",
                                      "import { type_var_int } from './ambiguous_export';"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    for (size_t fileIndex : {3U, 4U}) {
        const auto importOffset = texts[fileIndex].find("type_var_int");
        ASSERT_NE(importOffset, std::string::npos);

        Initializer initializer;
        auto ctx = initializer.CreateContext(filePaths[fileIndex].c_str(), ES2PANDA_STATE_CHECKED);
        auto result = lspApi->getDefinitionAtPosition(ctx, importOffset);
        EXPECT_TRUE(result.fileName.empty()) << "fileIndex=" << fileIndex;
        EXPECT_EQ(result.start, 0U) << "fileIndex=" << fileIndex;
        EXPECT_EQ(result.length, 0U) << "fileIndex=" << fileIndex;
        initializer.DestroyContext(ctx);
    }
}

TEST_F(LspGetDefTests, negativeTest1)
{
    std::vector<std::string> files = {"SetJumpTarget.ets"};
    std::vector<std::string> texts = {R"(
    Animal
    {
        name: string;
        constructor(name:
        string
        )
        {
            this.name = name;
        }
        void {
        hilog
        .
        info
        (
        DOMAIN_NAME,
        TAG,
        'Hello world()

        export
        function
        AAA() {
        }
        }
    }
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 234;
    ark::es2panda::EHeap::Scope eheapScope;  // remove this line #32069
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContextWithCache(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED, texts);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    ASSERT_EQ(result.length, 0);
}
