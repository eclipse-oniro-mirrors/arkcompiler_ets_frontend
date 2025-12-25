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

#include "lsp/include/formatting/formatting.h"
#include "lsp/include/formatting/formatting_settings.h"
#include "lsp/include/types.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>

namespace {

class LSPFormattingTests3 : public LSPAPITests {};

std::string ApplyChanges(const std::string &source, const std::vector<TextChange> &changes)
{
    std::string result = source;
    std::vector<TextChange> sortedChanges = changes;
    std::sort(sortedChanges.begin(), sortedChanges.end(),
              [](const TextChange &a, const TextChange &b) { return a.span.start > b.span.start; });
    for (const auto &change : sortedChanges) {
        result.replace(change.span.start, change.span.length, change.newText);
    }
    return result;
}

TEST_F(LSPFormattingTests3, FormatRange_CommaSpacingInRange)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let arr1 = [1,2,3];
let arr2 = [4,5,6];
let arr3 = [7,8,9];)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let arr1 = [1, 2, 3];
let arr2 = [4,5,6];
let arr3 = [7,8,9];)";

    constexpr size_t rangeStart = 0;
    constexpr size_t rangeLength = 20;

    auto tempFiles = CreateTempFile({"lsp_format_range_comma.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterCommaDelimiter(true);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_TypeAnnotationSpacingInRange)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(function test1(x:number):string { return ""; }
function test2(y:number):number { return 0; }
function test3(z:boolean):void {})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(function test1(x: number): string { return ""; }
function test2(y:number):number { return 0; }
function test3(z:boolean):void {})";

    constexpr size_t rangeStart = 0;
    constexpr size_t rangeLength = 50;

    auto tempFiles = CreateTempFile({"lsp_format_range_type.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_BinaryOperatorsInRange)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let a:number=10+20;
let b:number=30-40;
let c:number=50*60;
let d:number=70/80;)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let a: number = 10 + 20;
let b: number=30-40;
let c:number=50*60;
let d:number=70/80;)";

    constexpr size_t rangeStart = 4;
    constexpr size_t rangeLength = 21;

    auto tempFiles = CreateTempFile({"lsp_format_range_binary.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_ArrowFunctionSpacingInRange)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let fn1 = (x)=>x * 2;
let fn2 = (y)=>y * 3;
let fn3 = (z)=>z * 4;)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let fn1 = (x) => x * 2;
let fn2 = (y)=>y * 3;
let fn3 = (z)=>z * 4;)";

    constexpr size_t rangeStart = 10;
    constexpr size_t rangeLength = 15;

    auto tempFiles = CreateTempFile({"lsp_format_range_arrow.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_ClassBraceSpacingInRange)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(class A{
constructor(x:number){}
}
class B{
method():void{}
})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(class A {
constructor(x:number){}
}
class B{
method():void{}
})";

    constexpr size_t rangeStart = 0;
    constexpr size_t rangeLength = 8;

    auto tempFiles = CreateTempFile({"lsp_format_range_class.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_MiddleLineFormatting)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let a:number=10;
let b:number=20+30;
let c:number=40;
let d:number=50;)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let a:number=10;
let b: number = 20 + 30;
let c: number =40;
let d:number=50;)";

    constexpr size_t rangeStart = 18;
    constexpr size_t rangeLength = 25;

    auto tempFiles = CreateTempFile({"lsp_format_range_middle.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_MultipleLinesInRange)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let a:number=10;
let b:number=20;
let c:number=30;
let d:number=40;
let e:number=50;)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let a:number=10;
let b: number = 20;
let c: number = 30;
let d:number=40;
let e:number=50;)";

    constexpr size_t rangeStart = 18;
    constexpr size_t rangeLength = 30;

    auto tempFiles = CreateTempFile({"lsp_format_range_multiple.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_InterfaceBraceSpacingInRange)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(interface A{
prop:number;
}
interface B{
method():void;
})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(interface A {
prop:number;
}
interface B{
method():void;
})";

    constexpr size_t rangeStart = 0;
    constexpr size_t rangeLength = 12;

    auto tempFiles = CreateTempFile({"lsp_format_range_interface.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_EmptyBracesInRange)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(class A{}
class B{}
class C{})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(class A { }
class B{}
class C{})";

    constexpr size_t rangeStart = 0;
    constexpr size_t rangeLength = 9;

    auto tempFiles = CreateTempFile({"lsp_format_range_empty_braces.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces(true);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_TernaryOperatorInRange)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let x:number=value>0?value:-value;
let y:number=value>0?value:-value;
let z:number=value>0?value:-value;)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let x: number = value > 0 ? value : -value;
let y: number=value>0?value:-value;
let z:number=value>0?value:-value;)";

    constexpr size_t rangeStart = 4;
    constexpr size_t rangeLength = 36;

    auto tempFiles = CreateTempFile({"lsp_format_range_ternary.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_NoChangeWhenAlreadyFormatted)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let obj = { a: 1 };
let arr = [1, 2, 3];
let x: number = 10;)";

    constexpr size_t rangeStart = 0;
    constexpr size_t rangeLength = 100;

    auto tempFiles = CreateTempFile({"lsp_format_range_no_change.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    EXPECT_TRUE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_NoChangeWhenRangeIsEmpty)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let x:number=10;
let y:number=20;)";

    constexpr size_t rangeStart = 0;
    constexpr size_t rangeLength = 0;

    auto tempFiles = CreateTempFile({"lsp_format_range_empty_range.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    EXPECT_TRUE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_InvalidRangeBeyondFileLength)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let x = 1;)";

    constexpr size_t rangeStart = 0;
    constexpr size_t rangeLength = 10000;

    auto tempFiles = CreateTempFile({"lsp_format_range_invalid.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    EXPECT_TRUE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_NullContext)
{
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    constexpr size_t rangeStart = 0;
    constexpr size_t rangeLength = 10;

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(nullptr, formatContext, span);

    EXPECT_TRUE(changes.empty());
}

TEST_F(LSPFormattingTests3, FormatRange_ComplexFunctionInRange)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(function test1(x:number,y:number):number{return x+y;}
function test2(a:number,b:number):number{return a*b;}
function test3(p:number,q:number):number{return p-q;})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(function test1(x: number, y: number): number { return x + y;}
function test2(a:number,b:number):number{return a*b;}
function test3(p:number,q:number):number{return p-q;})";

    constexpr size_t rangeStart = 9;
    constexpr size_t rangeLength = 41;

    auto tempFiles = CreateTempFile({"lsp_format_range_complex.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterCommaDelimiter(true);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_NamespaceSpacingInRange)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(namespace A{
export class B{}
}
namespace C{
export class D{}
})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(namespace A{
export class B { }
}
namespace C{
export class D{}
})";

    constexpr size_t rangeStart = 0;
    constexpr size_t rangeLength = 30;

    auto tempFiles = CreateTempFile({"lsp_format_range_namespace.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces(true);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests3, FormatRange_ControlFlowSpacingInRange)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(if(condition){
doSomething();
}
if(condition){
doSomethingElse();
})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(if (condition) {
doSomething();
}
if(condition){
doSomethingElse();
})";

    constexpr size_t rangeStart = 0;
    constexpr size_t rangeLength = 15;

    auto tempFiles = CreateTempFile({"lsp_format_range_control.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

}  // namespace
