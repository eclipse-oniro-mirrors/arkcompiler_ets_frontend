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
#include "lsp/include/internal_api.h"

using ark::es2panda::lsp::Initializer;

struct TypeCase {
    size_t pos;
    int firstHighlightStart;
    int secondHighlightStart;
    int expectedLength;
};

class LspDocumentHighlights : public LSPAPITests {};

TEST_F(LspDocumentHighlights, getDocumentHighlights1)
{
    std::vector<std::string> files = {"getDocumentHighlights1.ets"};
    std::vector<std::string> texts = {R"delimiter(
let aaa = 123;
let bbb = aaa + 111;
let ccc = bbb + aaa + 234;
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    auto const pos = 6;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    initializer.DestroyContext(ctx);
    auto const expectedHighlightCount = 3;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), expectedHighlightCount);
    ASSERT_EQ(result.documentHighlights_[0].fileName_, filePaths[0]);
    auto const firstHighlightStart = 5;
    auto const secondHighlightStart = 26;
    auto const thirdHighlightStart = 53;
    auto const expectedLength = 3;
    auto const secondIndex = 2;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.start, firstHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.start, secondHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.start, thirdHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].kind_, HighlightSpanKind::REFERENCE);
}

TEST_F(LspDocumentHighlights, getDocumentHighlights2)
{
    std::vector<std::string> files = {"getDocumentHighlights2.ets"};
    std::vector<std::string> texts = {R"delimiter(
let aaa = 123;
let bbb = aaa + 111;
let ccc = bbb + aaa + 234;
function f1(aaa: number) {
    return aaa + bbb;})delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    LSPAPI const *lspApi = GetImpl();
    auto const firstPos = 109;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto firstResult = lspApi->getDocumentHighlights(ctx, firstPos);
    auto const firstExpectedHighlightCount = 3;
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_.size(), firstExpectedHighlightCount);
    ASSERT_EQ(firstResult.documentHighlights_[0].fileName_, filePaths[0]);
    auto const firstFirstHighlightStart = 20;
    auto const firstSecondHighlightStart = 47;
    auto const firstThirdHighlightStart = 108;
    auto const firstExpectedLength = 3;
    auto const secondIndex = 2;
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[0].textSpan_.start, firstFirstHighlightStart);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[1].textSpan_.start, firstSecondHighlightStart);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.start,
              firstThirdHighlightStart);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[0].textSpan_.length, firstExpectedLength);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[1].textSpan_.length, firstExpectedLength);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.length, firstExpectedLength);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[secondIndex].kind_, HighlightSpanKind::REFERENCE);
    auto const secondPos = 77;
    auto secondResult = lspApi->getDocumentHighlights(ctx, secondPos);
    initializer.DestroyContext(ctx);
    auto const secondExpectedHighlightCount = 2;
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_.size(), secondExpectedHighlightCount);
    ASSERT_EQ(secondResult.documentHighlights_[0].fileName_, filePaths[0]);
    auto const secondFirstHighlightStart = 76;
    auto const secondSecondHighlightStart = 102;
    auto const secondExpectedLength = 3;
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_[0].textSpan_.start, secondFirstHighlightStart);
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_[1].textSpan_.start, secondSecondHighlightStart);
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_[0].textSpan_.length, secondExpectedLength);
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_[1].textSpan_.length, secondExpectedLength);
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
}

TEST_F(LspDocumentHighlights, getDocumentHighlights3)
{
    std::vector<std::string> files = {"getDocumentHighlights3.ets"};
    std::vector<std::string> texts = {R"delimiter(
class Foo {
    aaa: number = 0;
}

let foo1 = new Foo();
foo1.aaa = 222

function f2() {
    let foo2 = new Foo();
    return foo2.aaa + foo1.aaa;
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    auto const pos = 8;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    initializer.DestroyContext(ctx);
    auto const expectedHighlightCount = 3;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), expectedHighlightCount);
    ASSERT_EQ(result.documentHighlights_[0].fileName_, filePaths[0]);
    auto const firstHighlightStart = 7;
    auto const secondHighlightStart = 52;
    auto const thirdHighlightStart = 110;
    auto const expectedLength = 3;
    auto const secondIndex = 2;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.start, firstHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.start, secondHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.start, thirdHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].kind_, HighlightSpanKind::REFERENCE);
}

TEST_F(LspDocumentHighlights, getDocumentHighlights4)
{
    std::vector<std::string> files = {"getDocumentHighlights3.ets"};
    std::vector<std::string> texts = {R"delimiter(
class Foo {
    aaa: number = 0;
}

let foo1 = new Foo();
foo1.aaa = 222

function f2() {
    let foo2 = new Foo();
    return foo2.aaa + foo1.aaa;
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    auto const pos = 18;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    initializer.DestroyContext(ctx);
    auto const expectedHighlightCount = 4;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), expectedHighlightCount);
    ASSERT_EQ(result.documentHighlights_[0].fileName_, filePaths[0]);
    auto const firstHighlightStart = 17;
    auto const secondHighlightStart = 64;
    auto const thirdHighlightStart = 133;
    auto const fourthHighlightStart = 144;
    auto const expectedLength = 3;
    auto const secondIndex = 2;
    auto const thirdIndex = 3;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.start, firstHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.start, secondHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.start, thirdHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[thirdIndex].textSpan_.start, fourthHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[thirdIndex].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].kind_, HighlightSpanKind::REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[thirdIndex].kind_, HighlightSpanKind::REFERENCE);
}

TEST_F(LspDocumentHighlights, getDocumentHighlights5)
{
    std::vector<std::string> files = {"getDocumentHighlights5.ets"};
    std::vector<std::string> texts = {R"delimiter(
function add(x: number, y: number) {
    return x + y;
}

function five() {
    return add(2, 3);
}

class Bar {
    six: number = add(2, 4);
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    auto const pos = 11;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    initializer.DestroyContext(ctx);
    auto const expectedHighlightCount = 3;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), expectedHighlightCount);
    ASSERT_EQ(result.documentHighlights_[0].fileName_, filePaths[0]);
    auto const firstHighlightStart = 10;
    auto const secondHighlightStart = 88;
    auto const thirdHighlightStart = 132;
    auto const expectedLength = 3;
    auto const secondIndex = 2;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.start, firstHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.start, secondHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.start, thirdHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].kind_, HighlightSpanKind::REFERENCE);
}

TEST_F(LspDocumentHighlights, getDocumentHighlights6)
{
    std::vector<std::string> files = {"getDocumentHighlights6.ets"};
    std::vector<std::string> texts = {R"delimiter(
class ListNode<T> {
    value: T;
    next: ListNode<T> | null = null;

    constructor(value: T) {
        this.value = value;
    }
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    auto const pos = 49;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    initializer.DestroyContext(ctx);
    auto const expectedHighlightCount = 2;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), expectedHighlightCount);
    ASSERT_EQ(result.documentHighlights_[0].fileName_, filePaths[0]);
    auto const firstHighlightStart = 7;
    auto const secondHighlightStart = 45;
    auto const expectedLength = 8;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.start, firstHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.start, secondHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
}

TEST_F(LspDocumentHighlights, getDocumentHighlights7)
{
    std::vector<std::string> files = {"getDocumentHighlights7.ets"};
    std::vector<std::string> texts = {R"delimiter(
function fib(n: number) {
    if (n === 0) {
        return 0;
    }
    if (n === 1) {
        return 1;
    }
    return fib(n - 1) + fib(n - 2);
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    auto const pos = 125;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    initializer.DestroyContext(ctx);
    auto const expectedHighlightCount = 3;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), expectedHighlightCount);
    ASSERT_EQ(result.documentHighlights_[0].fileName_, filePaths[0]);
    auto const firstHighlightStart = 10;
    auto const secondHighlightStart = 124;
    auto const thirdHighlightStart = 137;
    auto const expectedLength = 3;
    auto const secondIndex = 2;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.start, firstHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.start, secondHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.start, thirdHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].kind_, HighlightSpanKind::REFERENCE);
}

TEST_F(LspDocumentHighlights, getDocumentHighlights8)
{
    std::vector<std::string> files = {"getDocumentHighlights8.ets"};
    std::vector<std::string> texts = {R"delimiter(
//中文测试
let aaa = "123";
//中文测试
let bbb = aaa + "中文测试";
//中文测试
let ccc = bbb + aaa + "234";
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    auto const pos = 13;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    initializer.DestroyContext(ctx);
    auto const expectedHighlightCount = 3;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), expectedHighlightCount);
    ASSERT_EQ(result.documentHighlights_[0].fileName_, filePaths[0]);
    auto const firstHighlightStart = 12;
    auto const secondHighlightStart = 42;
    auto const thirdHighlightStart = 79;
    auto const expectedLength = 3;
    auto const secondIndex = 2;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.start, firstHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.start, secondHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.start, thirdHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].kind_, HighlightSpanKind::REFERENCE);
}

TEST_F(LspDocumentHighlights, getDocumentHighlights9)
{
    std::vector<std::string> files = {"getDocumentHighlights9.ets"};
    std::vector<std::string> texts = {R"delimiter(
//中文测试
function fib(n: string) {
    if (n === "0") {
        return "0";
    }
    if (n === "//中文测试") {
        return "1";
    }
    //中文测试
    return fib("中文测试") + fib(n + "2");
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    auto const pos = 156;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    initializer.DestroyContext(ctx);
    auto const expectedHighlightCount = 3;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), expectedHighlightCount);
    ASSERT_EQ(result.documentHighlights_[0].fileName_, filePaths[0]);
    auto const firstHighlightStart = 17;
    auto const secondHighlightStart = 155;
    auto const thirdHighlightStart = 169;
    auto const expectedLength = 3;
    auto const secondIndex = 2;
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.start, firstHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.start, secondHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.start, thirdHighlightStart);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].textSpan_.length, expectedLength);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[secondIndex].kind_, HighlightSpanKind::REFERENCE);
}

TEST_F(LspDocumentHighlights, getDocumentHighlights10)
{
    std::vector<std::string> files = {"getDocumentHighlights1.ets"};
    std::vector<std::string> texts = {R"delimiter(
function func() {}
enum aaa{
a = 1;
}
function func1() {}
bbb = aaa.a;
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    auto const firstPos = 26;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto firstResult = lspApi->getDocumentHighlights(ctx, firstPos);
    auto const firstExpectedHighlightCount = 2;
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_.size(), firstExpectedHighlightCount);
    ASSERT_EQ(firstResult.documentHighlights_[0].fileName_, filePaths[0]);
    auto const firstFirstHighlightStart = 25;
    auto const firstSecondHighlightStart = 65;
    auto const firstExpectedLength = 3;
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[0].textSpan_.start, firstFirstHighlightStart);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[1].textSpan_.start, firstSecondHighlightStart);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[0].textSpan_.length, firstExpectedLength);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[1].textSpan_.length, firstExpectedLength);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    ASSERT_EQ(firstResult.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
    auto const secondPos = 66;
    auto secondResult = lspApi->getDocumentHighlights(ctx, secondPos);
    initializer.DestroyContext(ctx);
    auto const secondExpectedHighlightCount = 2;
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_.size(), secondExpectedHighlightCount);
    ASSERT_EQ(secondResult.documentHighlights_[0].fileName_, filePaths[0]);
    auto const secondFirstHighlightStart = 25;
    auto const secondSecondHighlightStart = 65;
    auto const secondExpectedLength = 3;
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_[0].textSpan_.start, secondFirstHighlightStart);
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_[1].textSpan_.start, secondSecondHighlightStart);
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_[0].textSpan_.length, secondExpectedLength);
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_[1].textSpan_.length, secondExpectedLength);
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    ASSERT_EQ(secondResult.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
}

TEST_F(LspDocumentHighlights, getDocumentHighlightsBuiltInTypes)
{
    std::vector<std::string> files = {"getDocumentHighlightsBuiltInTypes.ets"};
    std::vector<std::string> texts = {R"delimiter(
let a: int = 10; let b: float = 2.2f;
let c: double = 3.14; let d: short = 20;
let e: long = 200000000000; let f: byte = 2;
let g: char = c'a'; let h: boolean = true;
let i: Any = 10; let j: Object = 1;
let k: int = 20; let l: float = 3.3f;
let m: double = 6.28; let n: short = 30;
let o: long = 300000000000; let p: byte = 3;
let q: char = c'b'; let r: boolean = false;
let s: Any = true; let t: Object = 2;
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1);
    std::vector<TypeCase> cases = {{9, 8, 211, 3},     {26, 25, 228, 5},   {47, 46, 249, 6},   {69, 68, 271, 5},
                                   {88, 87, 290, 4},   {116, 115, 318, 4}, {133, 132, 335, 4}, {153, 152, 355, 7},
                                   {176, 175, 379, 3}, {193, 192, 398, 6}};

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto const firstExpectedHighlightCount = 2;
    for (auto const &testCase : cases) {
        auto result = lspApi->getDocumentHighlights(ctx, testCase.pos);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), firstExpectedHighlightCount);
        ASSERT_EQ(result.documentHighlights_[0].fileName_, filePaths[0]);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.start, testCase.firstHighlightStart);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.start, testCase.secondHighlightStart);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.length, testCase.expectedLength);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.length, testCase.expectedLength);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::REFERENCE);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspDocumentHighlights, getDocumentHighlightsBoxedTypes)
{
    std::vector<std::string> files = {"getDocumentHighlightsBoxedTypes.ets"};
    std::vector<std::string> texts = {R"delimiter(
let a: Int = 10; let b: Float = 2.2f;
let c: Double = 3.14; let d: Short = 20;
let e: Long = 200000000000; let f: Byte = 2;
let g: Char = c'a'; let h: Boolean = true;
let i: Int = 20; let j: Float = 3.3f;
let k: Double = 6.28; let l: Short = 30;
let m: Long = 300000000000; let n: Byte = 3;
let o: Char = c'b'; let p: Boolean = false;
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1);
    std::vector<TypeCase> cases = {{9, 8, 175, 3},   {26, 25, 192, 5},   {47, 46, 213, 6},   {69, 68, 235, 5},
                                   {88, 87, 254, 4}, {116, 115, 282, 4}, {133, 132, 299, 4}, {153, 152, 319, 7}};

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto const firstExpectedHighlightCount = 2;
    for (auto const &testCase : cases) {
        auto result = lspApi->getDocumentHighlights(ctx, testCase.pos);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), firstExpectedHighlightCount);
        ASSERT_EQ(result.documentHighlights_[0].fileName_, filePaths[0]);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.start, testCase.firstHighlightStart);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.start, testCase.secondHighlightStart);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].textSpan_.length, testCase.expectedLength);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].textSpan_.length, testCase.expectedLength);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[0].kind_, HighlightSpanKind::REFERENCE);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
    }
    initializer.DestroyContext(ctx);
}
