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

#include <gtest/gtest.h>
#include <cstddef>
#include <string>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"

using ark::es2panda::lsp::Initializer;

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
    size_t const offset = 46;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 16;
    size_t const expectedLength = 1;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
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
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 16;
    size_t const expectedLength = 1;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
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
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 16;
    size_t const expectedLength = 1;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
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
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 11;
    size_t const expectedLength = 1;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
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
    std::string expectedFileName = "std.concurrency.etscache";
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
    size_t const offset = 100;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContextWithCache(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED, texts);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 18;
    size_t const expectedLength = 3;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
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
