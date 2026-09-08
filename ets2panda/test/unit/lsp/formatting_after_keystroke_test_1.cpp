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
#include <algorithm>
#include <gtest/gtest.h>

namespace {

class LSPFormattingAfterKeystrokeTest1 : public LSPAPITests {};

std::string ApplyTextChanges1(const std::string &text, const std::vector<TextChange> &changes)
{
    auto sorted = changes;
    std::sort(sorted.begin(), sorted.end(),
              [](const TextChange &a, const TextChange &b) { return a.span.start > b.span.start; });
    std::string result = text;
    for (const auto &change : sorted) {
        result.replace(change.span.start, change.span.length, change.newText);
    }
    return result;
}

TEST_F(LSPFormattingAfterKeystrokeTest1, FormatAfterKeystroke_SemicolonFormatsConstDeclaration)
{
    std::string code = R"(
const s:string="a"+"b";
)";
    std::string expected = R"(
const s: string = "a" + "b";
)";
    const char *fileName = "keystroke_semicolon_const.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t semicolonPos = code.find(';');
    ASSERT_NE(semicolonPos, std::string::npos);

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, ';', TextSpan {semicolonPos + 1, 0});
    ASSERT_FALSE(changes.empty());

    auto formatted = ApplyTextChanges1(code, changes);
    EXPECT_EQ(formatted, expected);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfterKeystrokeTest1, FormatAfterKeystroke_SemicolonFormatsReturnStatement)
{
    std::string code = R"(
function foo() {
return 1+2;
}
)";
    std::string expected = R"(
function foo() {
return 1 + 2;
}
)";
    const char *fileName = "keystroke_semicolon_return.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t semicolonPos = code.find(';');
    ASSERT_NE(semicolonPos, std::string::npos);

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, ';', TextSpan {semicolonPos + 1, 0});
    ASSERT_FALSE(changes.empty());

    auto formatted = ApplyTextChanges1(code, changes);
    EXPECT_EQ(formatted, expected);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfterKeystrokeTest1, FormatAfterKeystroke_SemicolonNoChangesWhenWellFormatted)
{
    std::string code = R"(
let n: number = 42;
)";
    const char *fileName = "keystroke_semicolon_formatted.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t semicolonPos = code.find(';');
    ASSERT_NE(semicolonPos, std::string::npos);

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, ';', TextSpan {semicolonPos + 1, 0});
    EXPECT_TRUE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfterKeystrokeTest1, FormatAfterKeystroke_ClosingBraceFormatsNestedBlocks)
{
    std::string code = R"(
function outer() {
if (ready) {
let x=1+2;
}
}
)";
    std::string expected = R"(
function outer() {
if (ready) {
let x = 1 + 2;
}
}
)";
    const char *fileName = "keystroke_brace_nested.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t bracePos = code.rfind('}');
    ASSERT_NE(bracePos, std::string::npos);

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, '}', TextSpan {bracePos + 1, 0});
    ASSERT_FALSE(changes.empty());

    auto formatted = ApplyTextChanges1(code, changes);
    EXPECT_EQ(formatted, expected);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfterKeystrokeTest1, FormatAfterKeystroke_ClosingBraceFormatsInnerBlock)
{
    std::string code = R"(
function foo() {
while (running) {
let n=1;
}
}
)";
    std::string expected = R"(
function foo() {
while (running) {
let n = 1;
}
}
)";
    const char *fileName = "keystroke_brace_inner.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t innerBracePos = code.find('}', code.find("let n"));
    ASSERT_NE(innerBracePos, std::string::npos);

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, '}', TextSpan {innerBracePos + 1, 0});
    ASSERT_FALSE(changes.empty());

    auto formatted = ApplyTextChanges1(code, changes);
    EXPECT_EQ(formatted, expected);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfterKeystrokeTest1, FormatAfterKeystroke_ClosingBraceNoChangesWhenWellFormatted)
{
    std::string code = R"(
function foo() {
let a: number = 1;
}
)";
    const char *fileName = "keystroke_brace_formatted.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t bracePos = code.rfind('}');
    ASSERT_NE(bracePos, std::string::npos);

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, '}', TextSpan {bracePos + 1, 0});
    EXPECT_TRUE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfterKeystrokeTest1, FormatAfterKeystroke_NewlineFormatsPreviousLine)
{
    std::string code = R"(
function bar() {
let z=3*4;
}
)";
    std::string expected = R"(
function bar() {
let z = 3 * 4;
}
)";
    const char *fileName = "keystroke_newline_prev_line.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t letLineEnd = code.find('\n', code.find("let z"));
    ASSERT_NE(letLineEnd, std::string::npos);
    size_t caretPos = letLineEnd + 1;

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, '\n', TextSpan {caretPos, 0});
    ASSERT_FALSE(changes.empty());

    auto formatted = ApplyTextChanges1(code, changes);
    EXPECT_EQ(formatted, expected);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfterKeystrokeTest1, FormatAfterKeystroke_NewlineKeepsNestedBlockIndentation)
{
    std::string code = R"(
function foo() {
if (ready) {
let n=1;
}
}
)";
    std::string expected = R"(
function foo() {
if (ready) {
let n = 1;
}
}
)";
    const char *fileName = "keystroke_newline_nested.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t letLineEnd = code.find('\n', code.find("let n"));
    ASSERT_NE(letLineEnd, std::string::npos);
    size_t caretPos = letLineEnd + 1;

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, '\n', TextSpan {caretPos, 0});
    ASSERT_FALSE(changes.empty());

    auto formatted = ApplyTextChanges1(code, changes);
    EXPECT_EQ(formatted, expected);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfterKeystrokeTest1, FormatAfterKeystroke_NewlineOnFirstLineReturnsEmpty)
{
    std::string code = "let a:number=1;\n";
    const char *fileName = "keystroke_newline_first_line.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t caretPos = 0;
    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, '\n', TextSpan {caretPos, 0});
    EXPECT_TRUE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfterKeystrokeTest1, FormatAfterKeystroke_CloseParenReturnsEmpty)
{
    std::string code = R"(
function baz(a:number) {
return a;
}
)";
    const char *fileName = "keystroke_close_paren.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t parenPos = code.find(')');
    ASSERT_NE(parenPos, std::string::npos);

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, ')', TextSpan {parenPos + 1, 0});
    EXPECT_TRUE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfterKeystrokeTest1, FormatAfterKeystroke_CloseBracketReturnsEmpty)
{
    std::string code = R"(
let arr: number[] = [1, 2];
let v = arr[0];
)";
    const char *fileName = "keystroke_close_bracket.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t bracketPos = code.rfind(']');
    ASSERT_NE(bracketPos, std::string::npos);

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, ']', TextSpan {bracketPos + 1, 0});
    EXPECT_TRUE(changes.empty());
    initializer.DestroyContext(ctx);
}

}  // namespace
