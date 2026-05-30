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
    size_t firstHighlightStart;
    size_t secondHighlightStart;
    size_t expectedLength;
};

class LspDocumentHighlights : public LSPAPITests {};

struct HighlightExpectation {
    size_t start;
    size_t length;
    HighlightSpanKind kind;
};

static void AssertHighlights(const DocumentHighlightsReferences &result, const std::string &fileName,
                             const std::vector<HighlightExpectation> &expected)
{
    ASSERT_EQ(result.documentHighlights_.size(), 1U);
    ASSERT_EQ(result.documentHighlights_[0].fileName_, fileName);
    ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), expected.size());
    for (size_t i = 0; i < expected.size(); i++) {
        const auto &actual = result.documentHighlights_[0].highlightSpans_[i];
        ASSERT_EQ(actual.textSpan_.start, expected[i].start);
        ASSERT_EQ(actual.textSpan_.length, expected[i].length);
        ASSERT_EQ(actual.kind_, expected[i].kind);
    }
}

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
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    AssertHighlights(result, filePaths[0],
                     {{5, 3, HighlightSpanKind::WRITTEN_REFERENCE},
                      {26, 3, HighlightSpanKind::REFERENCE},
                      {53, 3, HighlightSpanKind::REFERENCE}});
    initializer.DestroyContext(ctx);
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
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto firstResult = lspApi->getDocumentHighlights(ctx, 109);
    AssertHighlights(firstResult, filePaths[0],
                     {{20, 3, HighlightSpanKind::WRITTEN_REFERENCE},
                      {47, 3, HighlightSpanKind::REFERENCE},
                      {108, 3, HighlightSpanKind::REFERENCE}});

    auto secondResult = lspApi->getDocumentHighlights(ctx, 77);
    AssertHighlights(secondResult, filePaths[0],
                     {{76, 3, HighlightSpanKind::WRITTEN_REFERENCE}, {102, 3, HighlightSpanKind::REFERENCE}});
    initializer.DestroyContext(ctx);
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
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    AssertHighlights(result, filePaths[0],
                     {{7, 3, HighlightSpanKind::WRITTEN_REFERENCE},
                      {52, 3, HighlightSpanKind::REFERENCE},
                      {110, 3, HighlightSpanKind::REFERENCE}});
    initializer.DestroyContext(ctx);
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
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    AssertHighlights(result, filePaths[0],
                     {{17, 3, HighlightSpanKind::WRITTEN_REFERENCE},
                      {64, 3, HighlightSpanKind::REFERENCE},
                      {133, 3, HighlightSpanKind::REFERENCE},
                      {144, 3, HighlightSpanKind::REFERENCE}});
    initializer.DestroyContext(ctx);
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
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    AssertHighlights(result, filePaths[0],
                     {{10, 3, HighlightSpanKind::WRITTEN_REFERENCE},
                      {88, 3, HighlightSpanKind::REFERENCE},
                      {132, 3, HighlightSpanKind::REFERENCE}});
    initializer.DestroyContext(ctx);
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
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    AssertHighlights(result, filePaths[0],
                     {{7, 8, HighlightSpanKind::WRITTEN_REFERENCE}, {45, 8, HighlightSpanKind::REFERENCE}});
    initializer.DestroyContext(ctx);
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
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    AssertHighlights(result, filePaths[0],
                     {{10, 3, HighlightSpanKind::WRITTEN_REFERENCE},
                      {124, 3, HighlightSpanKind::REFERENCE},
                      {137, 3, HighlightSpanKind::REFERENCE}});
    initializer.DestroyContext(ctx);
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
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    AssertHighlights(result, filePaths[0],
                     {{12, 3, HighlightSpanKind::WRITTEN_REFERENCE},
                      {42, 3, HighlightSpanKind::REFERENCE},
                      {79, 3, HighlightSpanKind::REFERENCE}});
    initializer.DestroyContext(ctx);
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
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    AssertHighlights(result, filePaths[0],
                     {{17, 3, HighlightSpanKind::WRITTEN_REFERENCE},
                      {155, 3, HighlightSpanKind::REFERENCE},
                      {169, 3, HighlightSpanKind::REFERENCE}});
    initializer.DestroyContext(ctx);
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
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto firstResult = lspApi->getDocumentHighlights(ctx, firstPos);
    AssertHighlights(firstResult, filePaths[0],
                     {{25, 3, HighlightSpanKind::WRITTEN_REFERENCE}, {65, 3, HighlightSpanKind::REFERENCE}});
    auto const secondPos = 66;
    auto secondResult = lspApi->getDocumentHighlights(ctx, secondPos);
    AssertHighlights(secondResult, filePaths[0],
                     {{25, 3, HighlightSpanKind::WRITTEN_REFERENCE}, {65, 3, HighlightSpanKind::REFERENCE}});
    initializer.DestroyContext(ctx);
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
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<TypeCase> cases = {{9, 8, 211, 3},     {26, 25, 228, 5},   {47, 46, 249, 6},   {69, 68, 271, 5},
                                   {88, 87, 290, 4},   {116, 115, 318, 4}, {133, 132, 335, 4}, {153, 152, 355, 7},
                                   {176, 175, 379, 3}, {193, 192, 398, 6}};
    // NOLINTEND(readability-magic-numbers)

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto const firstExpectedHighlightCount = 2;
    for (auto const &testCase : cases) {
        auto result = lspApi->getDocumentHighlights(ctx, testCase.pos);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), firstExpectedHighlightCount);
        AssertHighlights(result, filePaths[0],
                         {{testCase.firstHighlightStart, testCase.expectedLength, HighlightSpanKind::REFERENCE},
                          {testCase.secondHighlightStart, testCase.expectedLength, HighlightSpanKind::REFERENCE}});
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
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<TypeCase> cases = {{9, 8, 175, 3},   {26, 25, 192, 5},   {47, 46, 213, 6},   {69, 68, 235, 5},
                                   {88, 87, 254, 4}, {116, 115, 282, 4}, {133, 132, 299, 4}, {153, 152, 319, 7}};
    // NOLINTEND(readability-magic-numbers)

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto const firstExpectedHighlightCount = 2;
    for (auto const &testCase : cases) {
        auto result = lspApi->getDocumentHighlights(ctx, testCase.pos);
        ASSERT_EQ(result.documentHighlights_[0].highlightSpans_.size(), firstExpectedHighlightCount);
        AssertHighlights(result, filePaths[0],
                         {{testCase.firstHighlightStart, testCase.expectedLength, HighlightSpanKind::REFERENCE},
                          {testCase.secondHighlightStart, testCase.expectedLength, HighlightSpanKind::REFERENCE}});
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspDocumentHighlights, getDocumentHighlights1111)
{
    std::vector<std::string> files = {"getDocumentHighlights1.ets"};
    std::vector<std::string> texts = {R"delimiter(
enum AAAA {
}
let a: AAAA;

)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    auto const pos = 24;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    lspApi->buildSymbolReferenceIndexForContext(ctx);
    auto result = lspApi->getDocumentHighlights(ctx, pos);
    AssertHighlights(result, filePaths[0],
                     {{6, 4, HighlightSpanKind::WRITTEN_REFERENCE}, {22, 4, HighlightSpanKind::REFERENCE}});
    initializer.DestroyContext(ctx);
}
