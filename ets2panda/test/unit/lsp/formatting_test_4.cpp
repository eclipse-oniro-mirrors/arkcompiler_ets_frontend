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

#include "lsp/include/formatting/formatting.h"
#include "lsp/include/formatting/formatting_settings.h"
#include "lsp/include/types.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>
#include <algorithm>

namespace {

class LSPFormattingTests4 : public LSPAPITests {};

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

TEST_F(LSPFormattingTests4, FormatRange_EditsStayWithinRangeSpan)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let a:number=10+20;
let b:number=30-40;
let c:number=50*60;)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let a:number=10+20;
let b: number = 30 - 40;
let c:number=50*60;)";

    const size_t rangeStart = testCode.find("let b");
    ASSERT_NE(rangeStart, std::string::npos);
    const std::string middleLine = "let b:number=30-40;";
    const size_t rangeLength = middleLine.size();

    auto tempFiles = CreateTempFile({"lsp_format_range_contained.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    auto changes = ark::es2panda::lsp::FormatRange(ctx, formatContext, span);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    const size_t rangeEnd = rangeStart + rangeLength;
    for (const auto &change : changes) {
        EXPECT_GE(change.span.start, rangeStart);
        EXPECT_LE(change.span.start + change.span.length, rangeEnd);
    }
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

TEST_F(LSPFormattingTests4, FormatDocument_LineCommentContentUnchanged)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(// comment   with   weird:spacing,and   kept
let a:number=1;)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(// comment   with   weird:spacing,and   kept
let a: number = 1;)";

    auto tempFiles = CreateTempFile({"lsp_format_line_comment.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

TEST_F(LSPFormattingTests4, FormatDocument_BlockCommentContentUnchanged)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(/* block   comment:with   weird,spacing   kept */
let a:number=1;)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(/* block   comment:with   weird,spacing   kept */
let a: number = 1;)";

    auto tempFiles = CreateTempFile({"lsp_format_block_comment.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

TEST_F(LSPFormattingTests4, FormatDocument_StringLiteralContentUnchanged)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let s = "value:with   weird, spacing inside";
let a:number=1;)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let s = "value:with   weird, spacing inside";
let a: number = 1;)";

    auto tempFiles = CreateTempFile({"lsp_format_string_literal.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

TEST_F(LSPFormattingTests4, FormatDocument_TemplateLiteralExpressionUnchanged)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let t = `value is ${x}`;
let a:number=1;)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let t = `value is ${x}`;
let a: number = 1;)";

    auto tempFiles = CreateTempFile({"lsp_format_template_literal.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

TEST_F(LSPFormattingTests4, FormatDocument_ImportStatementSpacing)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(import {a, b} from './dep';
let c:number=1;
let d:number=2;
export {c, d};)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(import {a, b} from './dep';
let c: number = 1;
let d: number = 2;
export { c, d };)";

    auto tempFiles = CreateTempFile({"lsp_format_import_export.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

TEST_F(LSPFormattingTests4, FormatDocument_ClassFunctionAndLambdaBodies)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(class A{
method():void{
let fn = (x)=>x+1;
fn(2);
}
})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(class A {
method(): void {
let fn = (x) => x + 1;
fn(2);
}
})";

    auto tempFiles = CreateTempFile({"lsp_format_bodies.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

TEST_F(LSPFormattingTests4, FormatDocument_ObjectLiteralSpacing)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let obj = {a:1,b:2};)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let obj = { a: 1, b: 2 };)";

    auto tempFiles = CreateTempFile({"lsp_format_object_literal.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

TEST_F(LSPFormattingTests4, FormatDocument_NestedArrayLiteralSpacing)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let arr = [[1,2],[3,4]];)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let arr = [[1, 2], [3, 4]];)";

    auto tempFiles = CreateTempFile({"lsp_format_nested_array.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

TEST_F(LSPFormattingTests4, FormatDocument_ChainedCallWithArrowFunctions)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let r = arr.filter((x)=>x>0).map((x)=>x*2);)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let r = arr.filter((x) => x > 0).map((x) => x * 2);)";

    auto tempFiles = CreateTempFile({"lsp_format_chained_call.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

TEST_F(LSPFormattingTests4, FormatDocument_BlankLinesPreserved)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = "let a:number=1;\n\n\nlet b:number=2;\n";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = "let a: number = 1;\n\n\nlet b: number = 2;\n";

    auto tempFiles = CreateTempFile({"lsp_format_blank_lines.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

TEST_F(LSPFormattingTests4, FormatDocument_CRLFLineEndingsPreserved)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = "let a:number=1;\r\nlet b:number=2;\r\n";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = "let a: number = 1;\r\nlet b: number = 2;\r\n";

    auto tempFiles = CreateTempFile({"lsp_format_crlf.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

TEST_F(LSPFormattingTests4, GetFormattingEditsForRange_UnicodeCodePointOffsets)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let s = "中文字符";
let a:number=1;)";
    // first line: 16 code points + newline; "中文字符" is 4 code points but 12 bytes
    // CC-OFFNXT(G.NAM.03-CPP) project code style
    const size_t rangeStart = 17;
    // CC-OFFNXT(G.NAM.03-CPP) project code style
    const size_t rangeLength = 15;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t thirdChangeIndex = 2;

    auto tempFiles = CreateTempFile({"lsp_format_unicode_offsets.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    TextSpan span(rangeStart, rangeLength);
    LSPAPI const *lspApi = GetImpl();
    auto changes = lspApi->getFormattingEditsForRange(ctx, formatContext.GetFormatCodeSettings(), span);

    initializer.DestroyContext(ctx);

    // edits are reported in code point offsets: line 2 starts at code point 17
    ASSERT_EQ(changes.size(), 3U);
    EXPECT_EQ(changes[0].span.start, 22U);  // before 'n' of 'number'
    EXPECT_EQ(changes[0].span.length, 0U);
    EXPECT_EQ(changes[0].newText, " ");
    EXPECT_EQ(changes[1].span.start, 28U);  // before '='
    EXPECT_EQ(changes[1].span.length, 0U);
    EXPECT_EQ(changes[1].newText, " ");
    EXPECT_EQ(changes[thirdChangeIndex].span.start, 29U);  // before '1'
    EXPECT_EQ(changes[thirdChangeIndex].span.length, 0U);
    EXPECT_EQ(changes[thirdChangeIndex].newText, " ");
}

TEST_F(LSPFormattingTests4, FormatDocument_NullContext)
{
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(nullptr, formatContext);

    EXPECT_TRUE(changes.empty());
}

// Wrapper-level test: GetFormattingEditsForDocument(es2panda_Context*,
// FormatCodeSettings&) returns the same document-wide edits as FormatDocument.
// The wrapper does not convert spans, so ASCII-only source keeps byte and
// code-point offsets identical.
TEST_F(LSPFormattingTests4, GetFormattingEditsForDocumentWrapper)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let a:number=10+20;
let b:number=30-40;)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let a: number = 10 + 20;
let b: number = 30 - 40;)";

    auto tempFiles = CreateTempFile({"lsp_format_document_wrapper.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    LSPAPI const *lspApi = GetImpl();
    auto changes = lspApi->getFormattingEditsForDocument(ctx, settings);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    // Precise span assertion: the formatter inserts a space right after the type
    // annotation colon. In "let a:number=10+20;" the colon is at code point 5 and
    // 'n' of "number" starts at code point 6.
    auto it =
        std::find_if(changes.begin(), changes.end(), [](const TextChange &change) { return change.span.start == 6; });
    ASSERT_NE(it, changes.end());
    EXPECT_EQ(it->span.length, 0U);
    EXPECT_EQ(it->newText, " ");
}

// The keystroke entry point reaches FormatAfterOpenBrace (formatting.cpp), which
// re-formats the lines starting from the outermost enclosing node up to the
// freshly typed '{'. Typing '{' right after 'class A' inserts the space between
// the class name and the opening brace, leaving the rest of the file untouched.
TEST_F(LSPFormattingTests4, DISABLED_FormatAfterKeystroke_OpenBraceAfterClassDeclaration)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(class A{
method():void{
let a:number=1;
}
})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(class A {
method():void{
let a:number=1;
}
})";

    auto tempFiles = CreateTempFile({"lsp_format_keystroke_open_brace.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t bracePos = testCode.find('{');
    ASSERT_NE(bracePos, std::string::npos);
    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, '{', TextSpan {bracePos + 1, 0});

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    // The single edit inserts a space right before the class-body '{'.
    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].span.start, bracePos);
    EXPECT_EQ(changes[0].span.length, 0U);
    EXPECT_EQ(changes[0].newText, " ");
}

// Optional property contexts reach the IsOptionalPropertyContext predicate in
// rules.cpp through the '?' token rule. Optional properties in an interface
// (TS_PROPERTY_SIGNATURE), in a class (CLASS_PROPERTY) and constructor
// parameter properties (TS_PARAMETER_PROPERTY) all carry the OPTIONAL modifier.
TEST_F(LSPFormattingTests4, FormatDocument_OptionalPropertyTypeAnnotations)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(interface I {
a?:number;
b:string;
}
class C {
public a?:number;
b?:string;
}
class D {
constructor(public c?:number) {}
})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(interface I {
a?: number;
b: string;
}
class C {
public a?: number;
b?: string;
}
class D {
constructor(public c?: number) {}
})";

    auto tempFiles = CreateTempFile({"lsp_format_optional_property.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

// A '?' in an optional function parameter is not attached to a property node,
// so IsOptionalPropertyContext walks past the parameter expression and returns
// false. The '?' rule then deletes the space between '?' and ':'.
TEST_F(LSPFormattingTests4, FormatDocument_OptionalParameterRemovesSpaceBeforeColon)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(function f(x? :number):void{
})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(function f(x?: number): void {})";

    auto tempFiles = CreateTempFile({"lsp_format_optional_param.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

// The non-null assertion '!' is parsed as TS_NON_NULL_EXPRESSION by the ETS
// parser (ParsePotentialNonNullExpression). The decorator rule that fires on
// '<any> !' evaluates IsNonNullAssertionContext and removes spaces around '!'.
TEST_F(LSPFormattingTests4, FormatDocument_NonNullAssertionNormalizesSpacing)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(function f(str: string | null): number {
return str ! .length;
})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(function f(str: string | null): number {
return str!.length;
})";

    auto tempFiles = CreateTempFile({"lsp_format_non_null.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

// A generic call 'id<number> (1)' exercises the '> (' pair. The space removal
// is performed by the function-call rule through IsFunctionCallOrNewContext
// (the '>' belongs to a CALL_EXPRESSION). The dedicated generic rule whose
// predicates are IsNotFunctionDeclContext / IsNonTypeAssertionContext is
// masked in this bucket, so those two predicates remain unreachable from ETS
// sources: ETS always wraps generic angle brackets in TSTypeParameterInstantiation,
// which is not part of IsTypeArgumentOrParameterOrAssertion's node set.
TEST_F(LSPFormattingTests4, FormatDocument_GenericCallRemovesSpaceBeforeArgumentList)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(function id<T>(x: T): T {
return x;
}
let v = id<number> (1);)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(function id<T>(x: T): T {
return x;
}
let v = id<number>(1);)";

    auto tempFiles = CreateTempFile({"lsp_format_generic_call.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

// A function whose return type is an array literal type ('number[]') followed
// by a body block reaches the '] {' token pair. The observed spacing change is
// produced by the function-open-brace INSERT_SPACE rule; the
// IsNotBeforeBlockInFunctionDeclarationContext predicate (guarding the
// no-space-after-close-bracket rule) is masked by that earlier rule in the
// same bucket, so it is never evaluated for '] {' (confirmed by gcov).
TEST_F(LSPFormattingTests4, FormatDocument_ArrayReturnTypeFunctionBodySpacing)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(function f():number[]{
return [1, 2];
})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(function f(): number[] {
return [1, 2];
})";

    auto tempFiles = CreateTempFile({"lsp_format_array_ret_type.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

// SemicolonPreference::REMOVE triggers the DELETE_TOKEN action
// (ApplyDeleteToken in formatting.cpp). Note: the current implementation
// deletes the token *following* the semicolon instead of the semicolon itself,
// which corrupts the statement start; this behavior mismatch is recorded, not
// papered over, and the test asserts the observed output.
TEST_F(LSPFormattingTests4, FormatDocument_SemicolonPreferenceRemoveDeletesFollowingToken)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = "let a:number=1;\nlet b:number=2;\n";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = "let a: number = 1;\n b: number = 2;\n";

    auto tempFiles = CreateTempFile({"lsp_format_semicolon_remove.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetSemicolons(ark::es2panda::lsp::SemicolonPreference::REMOVE);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

// An async arrow function is parsed by the ETS parser (ParseAsyncExpression)
// into an ArrowFunctionExpression, but the 'async' keyword lies outside the
// node's source range (the node starts at the '('). Formatting an async arrow
// does evaluate IsArrowFunctionContext (via the async/'(' rule), but the
// predicate always sees a VARIABLE_DECLARATOR in the parent chain and returns
// false, so no formatting edit is ever emitted for the async keyword. The
// stable (no-change) assertion pins down that current behavior.
TEST_F(LSPFormattingTests4, FormatDocument_AsyncArrowFunctionSpacingIsStable)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let f = async (x: number): Promise<number> => x + 1;)";

    auto tempFiles = CreateTempFile({"lsp_format_async_arrow.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    EXPECT_TRUE(changes.empty());
}

// A generic call with an array type argument ('id<number[] >(1)') produces the
// ']' + '>' token pair. In the '] >' rules bucket the earlier candidates all
// fail: AddGenericRules rule6 (any -> '>') requires IsTypeArgumentOrParameter-
// OrAssertionContext, which never holds for ETS because the '>' is always a
// direct child of TSTypeParameterInstantiation; the user-config binary-operator
// rules require IsBinaryOpContext, which also fails for the array type. The
// no-space-after-close-bracket rule is therefore reached and its
// IsNotBeforeBlockInFunctionDeclarationContext predicate is evaluated: the
// next token '>' is not a BLOCK_STATEMENT, so the predicate returns true and
// DELETE_SPACE removes the space between ']' and '>'.
TEST_F(LSPFormattingTests4, FormatDocument_ArrayTypeArgumentRemovesSpaceAfterCloseBracket)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let v = id<number[] >(1);)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(let v = id<number[]>(1);)";

    auto tempFiles = CreateTempFile({"lsp_format_array_type_arg.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    // The only edit deletes the space between ']' (code point 18) and '>'
    // (code point 20); the replaced span covers the single space at 19.
    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].span.start, 19U);
    EXPECT_EQ(changes[0].span.length, 1U);
    EXPECT_TRUE(changes[0].newText.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

// 'new(B)()' exercises the 'new' + '(' token pair. In that rules bucket the
// yield/return/keyword rule (spaceAfterKeywords -> any, INSERT_SPACE,
// {IsOnSameLineContext}) is registered before the decorator rule
// (new -> '(', DELETE_SPACE, {IsOnSameLineContext, IsConstructorSignatureContext}).
// For same-line pairs the keyword rule always matches first and its
// MODIFY_SPACE action masks the later rule, so IsConstructorSignatureContext
// is never evaluated through this public entry point (confirmed by gcov). The
// observable edit is the keyword rule inserting a space after 'new'.
TEST_F(LSPFormattingTests4, FormatDocument_NewWithGroupedCalleeInsertsSpace)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(class A {
constructor() {
let x = new(B)();
}
})";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(class A {
constructor() {
let x = new (B)();
}
})";

    auto tempFiles = CreateTempFile({"lsp_format_new_grouped.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    // The single edit inserts a space right before the '(' of the grouped
    // callee; the 'new' + '(' pair is consumed by the keyword rule, keeping
    // IsConstructorSignatureContext unreachable.
    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].span.start, 37U);
    EXPECT_EQ(changes[0].span.length, 0U);
    EXPECT_EQ(changes[0].newText, " ");
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
}

// ETS rejects the TS-style angle-bracket type assertion '<Foo> (1)' at parse
// time (diagnostic TS_TYPE_ASSERTION, "Type cast syntax '<type>' is not
// supported"); the parser returns an AllocBrokenExpression instead of a
// TSTypeAssertion node. As a result the '>' token is never a direct child of
// a TS_TYPE_ASSERTION, IsTypeArgumentOrParameterOrAssertionContext stays
// false, and the generic rule whose predicates are IsNotFunctionDeclContext /
// IsNonTypeAssertionContext is never evaluated (rule7 in AddGenericRules,
// masked by its own IsTypeArgumentOrParameterOrAssertionContext guard). The
// source is left untouched apart from the (already canonical) spacing.
TEST_F(LSPFormattingTests4, FormatDocument_AngleBracketAssertionRejectedByETSParser)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(let a = <Foo> (1);)";

    auto tempFiles = CreateTempFile({"lsp_format_angle_assertion.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    initializer.DestroyContext(ctx);

    EXPECT_TRUE(changes.empty());
}

}  // namespace
