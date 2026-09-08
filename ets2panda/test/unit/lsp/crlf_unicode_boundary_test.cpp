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
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/lsp_utils.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/formatting/formatting_settings.h"
#include "lsp/include/types.h"
#include "public/es2panda_lib.h"

#include <gtest/gtest.h>
#include <algorithm>

namespace {

class LspCrlfUnicodeBoundaryTests : public LSPAPITests {};

// Named constants for code point / byte offset positions used in tests.
// "a中b": code points 'a'(0), '中'(1), 'b'(2); byte offsets 'a'=0, '中'=1, 'b'=4
constexpr size_t K_CP_IDX_B = 2;              // code point index of 'b' in "a中b"
constexpr size_t K_CP_PAST_END_AB = 3;        // one past last code point in "a中b"
constexpr size_t K_BYTE_OFF_B = 4;            // byte offset of 'b' in "a中b"
constexpr size_t K_BYTE_OFF_PAST_END_AB = 5;  // one past last byte in "a中b"

// "x😀y": code points 'x'(0), '😀'(1), 'y'(2); byte offsets 'x'=0, '😀'=1, 'y'=5
constexpr size_t K_CP_IDX_Y = 2;              // code point index of 'y' in "x😀y"
constexpr size_t K_CP_PAST_END_XY = 3;        // one past last code point in "x😀y"
constexpr size_t K_BYTE_OFF_Y = 5;            // byte offset of 'y' in "x😀y"
constexpr size_t K_BYTE_OFF_PAST_END_XY = 6;  // one past last byte in "x😀y"

// Test: CodePointOffsetToByteOffset returns identical offset for pure ASCII content
TEST_F(LspCrlfUnicodeBoundaryTests, CodePointOffsetToByteOffsetAsciiIsIdentity)
{
    const std::string content = "let x: number = 42;";
    // For pure ASCII, each code point is one byte, so offsets must match.
    for (size_t cp = 0; cp <= content.size(); ++cp) {
        EXPECT_EQ(ark::es2panda::lsp::CodePointOffsetToByteOffset(content, cp), cp);
    }
}

// Test: CodePointOffsetToByteOffset converts code point offset to byte offset for Chinese (3-byte UTF-8)
TEST_F(LspCrlfUnicodeBoundaryTests, CodePointOffsetToByteOffsetChineseThreeBytes)
{
    // "中" is U+4E2D, encoded as 3 bytes in UTF-8 (E4 B8 AD).
    const std::string content = "a中b";
    // code points: 'a'(0), '中'(1), 'b'(2)
    // byte offsets: 'a'=0, '中'=1, 'b'=4
    EXPECT_EQ(ark::es2panda::lsp::CodePointOffsetToByteOffset(content, 0), 0U);
    EXPECT_EQ(ark::es2panda::lsp::CodePointOffsetToByteOffset(content, 1), 1U);
    EXPECT_EQ(ark::es2panda::lsp::CodePointOffsetToByteOffset(content, K_CP_IDX_B), K_BYTE_OFF_B);
    EXPECT_EQ(ark::es2panda::lsp::CodePointOffsetToByteOffset(content, K_CP_PAST_END_AB), K_BYTE_OFF_PAST_END_AB);
}

// Test: CodePointOffsetToByteOffset converts code point offset to byte offset for emoji (4-byte UTF-8)
TEST_F(LspCrlfUnicodeBoundaryTests, CodePointOffsetToByteOffsetEmojiFourBytes)
{
    // "😀" is U+1F600, encoded as 4 bytes in UTF-8 (F0 9F 98 80).
    const std::string content = "x😀y";
    // code points: 'x'(0), '😀'(1), 'y'(2)
    // byte offsets: 'x'=0, '😀'=1, 'y'=5
    EXPECT_EQ(ark::es2panda::lsp::CodePointOffsetToByteOffset(content, 0), 0U);
    EXPECT_EQ(ark::es2panda::lsp::CodePointOffsetToByteOffset(content, 1), 1U);
    EXPECT_EQ(ark::es2panda::lsp::CodePointOffsetToByteOffset(content, K_CP_IDX_Y), K_BYTE_OFF_Y);
    EXPECT_EQ(ark::es2panda::lsp::CodePointOffsetToByteOffset(content, K_CP_PAST_END_XY), K_BYTE_OFF_PAST_END_XY);
}

// Test: CodePointOffsetToByteOffset handles offset past end of string safely
TEST_F(LspCrlfUnicodeBoundaryTests, CodePointOffsetToByteOffsetPastEndClamps)
{
    const std::string content = "ab中";
    // content has 3 code points, 5 bytes. Requesting offset 10 should not overflow.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t cpOffsetPastEnd = 10;
    auto byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(content, cpOffsetPastEnd);
    // Should not exceed content.size()
    ASSERT_LE(byteOffset, content.size());
    EXPECT_EQ(byteOffset, content.size());
}

// Test: ByteOffsetToCodePointOffset returns identical offset for pure ASCII content
TEST_F(LspCrlfUnicodeBoundaryTests, ByteOffsetToCodePointOffsetAsciiIsIdentity)
{
    const std::string content = "function f() {}";
    for (size_t b = 0; b <= content.size(); ++b) {
        EXPECT_EQ(ark::es2panda::lsp::ByteOffsetToCodePointOffset(content, b), b);
    }
}

// Test: ByteOffsetToCodePointOffset converts byte offset to code point offset for Chinese
TEST_F(LspCrlfUnicodeBoundaryTests, ByteOffsetToCodePointOffsetChinese)
{
    const std::string content = "a中b";
    // byte offsets: 'a'=0, '中'=1..3, 'b'=4
    // code point offsets: 'a'=0, '中'=1, 'b'=2
    EXPECT_EQ(ark::es2panda::lsp::ByteOffsetToCodePointOffset(content, 0), 0U);
    EXPECT_EQ(ark::es2panda::lsp::ByteOffsetToCodePointOffset(content, 1), 1U);
    EXPECT_EQ(ark::es2panda::lsp::ByteOffsetToCodePointOffset(content, K_BYTE_OFF_B), K_CP_IDX_B);
    EXPECT_EQ(ark::es2panda::lsp::ByteOffsetToCodePointOffset(content, K_BYTE_OFF_PAST_END_AB), K_CP_PAST_END_AB);
}

// Test: ByteOffsetToCodePointOffset converts byte offset to code point offset for emoji
TEST_F(LspCrlfUnicodeBoundaryTests, ByteOffsetToCodePointOffsetEmoji)
{
    const std::string content = "x😀y";
    // byte offsets: 'x'=0, '😀'=1..4, 'y'=5
    // code point offsets: 'x'=0, '😀'=1, 'y'=2
    EXPECT_EQ(ark::es2panda::lsp::ByteOffsetToCodePointOffset(content, 0), 0U);
    EXPECT_EQ(ark::es2panda::lsp::ByteOffsetToCodePointOffset(content, 1), 1U);
    EXPECT_EQ(ark::es2panda::lsp::ByteOffsetToCodePointOffset(content, K_BYTE_OFF_Y), K_CP_IDX_Y);
    EXPECT_EQ(ark::es2panda::lsp::ByteOffsetToCodePointOffset(content, K_BYTE_OFF_PAST_END_XY), K_CP_PAST_END_XY);
}

// Test: ByteOffsetToCodePointOffset handles mid-character byte offset gracefully
TEST_F(LspCrlfUnicodeBoundaryTests, ByteOffsetToCodePointOffsetMidCharacter)
{
    const std::string content = "a中b";
    // Byte offset 2 is in the middle of '中' (bytes 1,2,3).
    // The function should not advance past the partial character.
    auto cp = ark::es2panda::lsp::ByteOffsetToCodePointOffset(content, 2);
    // Should count 'a' only (1 code point), since '中' is not complete at byte 2.
    EXPECT_EQ(cp, 1U);
}

// Test: Round-trip conversion CodePoint -> Byte -> CodePoint is identity for mixed content
TEST_F(LspCrlfUnicodeBoundaryTests, RoundTripConversionMixedContent)
{
    const std::string content = "let 名前 = \"名前\"; 😀";
    // Compute the actual code point count by converting the full byte length back.
    const size_t totalCodePoints = ark::es2panda::lsp::ByteOffsetToCodePointOffset(content, content.size());
    // Round-trip should be identity for every valid code point offset within the content.
    for (size_t cp = 0; cp <= totalCodePoints; ++cp) {
        auto byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(content, cp);
        auto backToCp = ark::es2panda::lsp::ByteOffsetToCodePointOffset(content, byteOffset);
        EXPECT_EQ(backToCp, cp) << "Round-trip failed at code point offset " << cp;
    }
}

// Test: Formatting document with CRLF line endings produces valid edits
TEST_F(LspCrlfUnicodeBoundaryTests, FormatDocumentWithCrlfLineEndings)
{
    // Source with CRLF line endings and missing spaces around operators.
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = "function add(x:number,y:number):number{\r\nreturn x+y;\r\n}\r\n";

    auto tempFiles = CreateTempFile({"format_crlf.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);
    // Formatting should produce at least one edit (e.g. space after colon/comma).
    ASSERT_FALSE(changes.empty());

    // Apply changes and verify the result is still valid (contains the function signature).
    std::string result = testCode;
    std::vector<TextChange> sortedChanges = changes;
    std::sort(sortedChanges.begin(), sortedChanges.end(),
              [](const TextChange &a, const TextChange &b) { return a.span.start > b.span.start; });
    for (const auto &change : sortedChanges) {
        result.replace(change.span.start, change.span.length, change.newText);
    }
    // The formatted result should still contain the function name and return statement.
    EXPECT_NE(result.find("function add"), std::string::npos);
    EXPECT_NE(result.find("return"), std::string::npos);

    initializer.DestroyContext(ctx);
}

// Test: Formatting document with LF line endings produces valid edits
TEST_F(LspCrlfUnicodeBoundaryTests, FormatDocumentWithLfLineEndings)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = "function add(x:number,y:number):number{\nreturn x+y;\n}\n";

    auto tempFiles = CreateTempFile({"format_lf.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);
    ASSERT_FALSE(changes.empty());

    std::string result = testCode;
    std::vector<TextChange> sortedChanges = changes;
    std::sort(sortedChanges.begin(), sortedChanges.end(),
              [](const TextChange &a, const TextChange &b) { return a.span.start > b.span.start; });
    for (const auto &change : sortedChanges) {
        result.replace(change.span.start, change.span.length, change.newText);
    }
    EXPECT_NE(result.find("function add"), std::string::npos);
    EXPECT_NE(result.find("return"), std::string::npos);

    initializer.DestroyContext(ctx);
}

// Test: GetTouchingToken finds the correct identifier in source with Chinese string literals
TEST_F(LspCrlfUnicodeBoundaryTests, GetTouchingTokenWithChineseStringLiteral)
{
    // The identifier "foo" appears after a Chinese string literal.
    const std::string source = "let msg = \"中文测试\";\nlet foo = 1;\nfoo;\n";

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("unicode_token.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    // Position at the usage of "foo;" (the second occurrence).
    const auto fooUsage = source.find("foo;");
    ASSERT_NE(fooUsage, std::string::npos);
    auto token = ark::es2panda::lsp::GetTouchingToken(ctx, fooUsage, false);
    ASSERT_NE(token, nullptr);
    ASSERT_TRUE(token->IsIdentifier());
    EXPECT_EQ(token->AsIdentifier()->Name(), "foo");

    initializer.DestroyContext(ctx);
}

// Test: GetTouchingToken locates identifier after emoji content
TEST_F(LspCrlfUnicodeBoundaryTests, GetTouchingTokenAfterEmojiContent)
{
    // The identifier "bar" appears after an emoji in a string literal.
    const std::string source = "let emoji = \"😀\";\nlet bar = 2;\nbar;\n";

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("emoji_token.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto barUsage = source.find("bar;");
    ASSERT_NE(barUsage, std::string::npos);
    auto token = ark::es2panda::lsp::GetTouchingToken(ctx, barUsage, false);
    ASSERT_NE(token, nullptr);
    ASSERT_TRUE(token->IsIdentifier());
    EXPECT_EQ(token->AsIdentifier()->Name(), "bar");

    initializer.DestroyContext(ctx);
}

// Test: getDefinitionAtPosition works correctly when source contains Chinese comments.
// The API expects a code point offset, so we must convert the byte offset from source.find().
TEST_F(LspCrlfUnicodeBoundaryTests, GetDefinitionWithChineseCommentPreservesOffsets)
{
    const std::string source = "// 中文注释\nlet target: number = 1;\nfunction use(): number {\n  return target;\n}\n";

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("unicode_def.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    // Position at "target" usage inside the function body (byte offset).
    const auto targetUseByte = source.find("return target");
    ASSERT_NE(targetUseByte, std::string::npos);
    const auto targetPosByte = targetUseByte + std::string("return ").size();
    // Convert byte offset to code point offset for the API (Chinese comment shifts offsets).
    const auto targetPosCp = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, targetPosByte);

    auto def = lspApi->getDefinitionAtPosition(ctx, targetPosCp);
    initializer.DestroyContext(ctx);

    // The definition should point to the declaration of "target".
    ASSERT_EQ(def.length, std::string("target").size());
    // def.start is a code point offset; convert the declaration's byte offset to compare.
    const auto targetDeclByte = source.find("target: number");
    ASSERT_NE(targetDeclByte, std::string::npos);
    const auto targetDeclCp = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, targetDeclByte);
    EXPECT_EQ(def.start, targetDeclCp);
}

}  // namespace
