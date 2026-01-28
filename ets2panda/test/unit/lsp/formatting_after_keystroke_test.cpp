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

class LSPFormattingAfteyKeystrokeTest : public LSPAPITests {};

std::string ApplyTextChanges(const std::string &text, const std::vector<TextChange> &changes)
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

TEST_F(LSPFormattingAfteyKeystrokeTest, FormatAfterKeystroke_SemicolonFormatsStatement)
{
    std::string code = R"(
let b:number=20+30;
)";

    std::string expected = R"(
let b: number = 20 + 30;
)";

    const char *fileName = "keystroke_semicolon.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t semicolonPos = code.find(';');
    ASSERT_NE(semicolonPos, std::string::npos);

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, ';', TextSpan {semicolonPos + 1, 0});
    ASSERT_FALSE(changes.empty());

    auto formatted = ApplyTextChanges(code, changes);
    EXPECT_EQ(formatted, expected);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfteyKeystrokeTest, FormatAfterKeystroke_NewlineIndentsNextLine)
{
    std::string code = R"(
function foo() {
let obj = { a: 1, b: 2 };
}
)";
    std::string expected = R"(
function foo() {
let obj = {a: 1, b: 2};
}
)";
    const char *fileName = "keystroke_newline.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces(false);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t letLineEnd = code.find('\n', code.find("let obj"));
    ASSERT_NE(letLineEnd, std::string::npos);
    size_t caretPos = letLineEnd + 1;

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, '\n', TextSpan {caretPos, 0});
    ASSERT_FALSE(changes.empty());

    auto formatted = ApplyTextChanges(code, changes);
    EXPECT_EQ(formatted, expected);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfteyKeystrokeTest, FormatAfterKeystroke_ClosingBraceFixesIndentation)
{
    std::string code = R"(
function foo() {
let obj = { a: 1, b: 2 };
}
)";
    std::string expected = R"(
function foo() {
let obj = {a: 1, b: 2 };
}
)";
    const char *fileName = "keystroke_closing_brace.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces(false);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t bracePos = code.find('}');
    ASSERT_NE(bracePos, std::string::npos);

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, '}', TextSpan {bracePos + 1, 0});
    ASSERT_FALSE(changes.empty());

    auto formatted = ApplyTextChanges(code, changes);
    EXPECT_EQ(formatted, expected);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfteyKeystrokeTest, FormatAfterKeystroke_SkipsInsideComment)
{
    std::string code = R"(// comment {
)";
    const char *fileName = "keystroke_comment.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    size_t bracePos = code.find('{');
    ASSERT_NE(bracePos, std::string::npos);

    auto changes = ark::es2panda::lsp::FormatAfterKeystroke(ctx, formatContext, '{', TextSpan {bracePos + 1, 0});
    ASSERT_TRUE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingAfteyKeystrokeTest, FormatAfterKeystroke_NewlineIndentsNextLine00)
{
    std::string code = R"(
function foo() {
//中文测试
let obj = { a: 1, b: 2 };
}
)";
    std::string expected = R"(
function foo() {
//中文测试
let obj = {a: 1, b: 2};
}
)";
    const char *fileName = "keystroke_newline.ets";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, code.c_str());

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces(false);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    // CC-OFFNXT(G.NAM.03-CPP) project code style
    constexpr size_t CARET_POS = 26;

    LSPAPI const *lspApi = GetImpl();
    auto changes = lspApi->getFormattingEditsAfterKeystroke(ctx, formatContext.GetFormatCodeSettings(), '\n',
                                                            TextSpan {CARET_POS, 0});
    ASSERT_FALSE(changes.empty());
    ASSERT_EQ(changes.size(), 2U);
    EXPECT_EQ(changes[0].span.start, 36U);
    EXPECT_EQ(changes[0].span.length, 1U);
    EXPECT_EQ(changes[0].newText, "");
    EXPECT_EQ(changes[1].span.start, 47U);
    EXPECT_EQ(changes[1].span.length, 1U);
    EXPECT_EQ(changes[1].newText, "");

    initializer.DestroyContext(ctx);
}

}  // namespace
