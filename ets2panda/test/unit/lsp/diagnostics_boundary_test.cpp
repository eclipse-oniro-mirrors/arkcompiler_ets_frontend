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
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace {

class LspDiagnosticsBoundaryTests : public LSPAPITests {
public:
};

// Test: Empty source file produces no diagnostics (or at least no out-of-range diagnostics)
TEST_F(LspDiagnosticsBoundaryTests, EmptyFileProducesNoDiagnostics)
{
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("empty.ets", ES2PANDA_STATE_CHECKED, "");
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto semanticResult = lspApi->getSemanticDiagnostics(ctx);
    auto syntacticResult = lspApi->getSyntacticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    // An empty file should not produce any diagnostics.
    EXPECT_EQ(semanticResult.diagnostic.size(), 0U);
    EXPECT_EQ(syntacticResult.diagnostic.size(), 0U);
}

// Test: Comment-only file produces no diagnostics
TEST_F(LspDiagnosticsBoundaryTests, CommentOnlyFileProducesNoDiagnostics)
{
    const std::string source = R"(// just a comment
// another comment
/* multi-line
   comment */
)";

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("comments.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto semanticResult = lspApi->getSemanticDiagnostics(ctx);
    auto syntacticResult = lspApi->getSyntacticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    // A comment-only file should not produce any diagnostics.
    EXPECT_EQ(semanticResult.diagnostic.size(), 0U);
    EXPECT_EQ(syntacticResult.diagnostic.size(), 0U);
}

// Test: Diagnostic ranges near EOF do not crash and produce valid structure
TEST_F(LspDiagnosticsBoundaryTests, DiagnosticRangesNearEofAreValid)
{
    // Source with a syntax error at the very end (missing closing brace).
    const std::string source = "function foo(): void {\n  console.log('hi');\n";

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("eof_error.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSyntacticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    // For each diagnostic, verify the range structure is valid (start <= end on same line).
    // LSP conventions allow end positions to be exclusive, so they may extend one past source length.
    for (const auto &d : result.diagnostic) {
        // Line numbers should be reasonable (within a small multiple of source line count).
        size_t lineCount = 1;
        for (char c : source) {
            if (c == '\n') {
                lineCount++;
            }
        }
        EXPECT_LE(d.range_.start.line_, lineCount + 1);
        EXPECT_LE(d.range_.end.line_, lineCount + 1);
        // On the same line, end should be >= start.
        if (d.range_.start.line_ == d.range_.end.line_) {
            EXPECT_GE(d.range_.end.character_, d.range_.start.character_);
        }
    }
}

// Test: Single-character file does not crash and produces valid diagnostics
TEST_F(LspDiagnosticsBoundaryTests, SingleCharacterFileDoesNotCrash)
{
    const std::string source = "x";

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("single_char.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto semanticResult = lspApi->getSemanticDiagnostics(ctx);
    auto syntacticResult = lspApi->getSyntacticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    // The call should not crash. Diagnostics may or may not be present, but ranges must be structurally valid.
    // LSP end positions are exclusive and may extend one past source length.
    for (const auto &d : semanticResult.diagnostic) {
        EXPECT_LE(d.range_.start.character_, source.size() + 1);
        EXPECT_LE(d.range_.end.character_, source.size() + 1);
    }
    for (const auto &d : syntacticResult.diagnostic) {
        EXPECT_LE(d.range_.start.character_, source.size() + 1);
        EXPECT_LE(d.range_.end.character_, source.size() + 1);
    }
}

// Test: File with only whitespace produces no diagnostics
TEST_F(LspDiagnosticsBoundaryTests, WhitespaceOnlyFileProducesNoDiagnostics)
{
    const std::string source = "   \n  \t  \n";

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("whitespace.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto semanticResult = lspApi->getSemanticDiagnostics(ctx);
    auto syntacticResult = lspApi->getSyntacticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    // A whitespace-only file should not produce meaningful diagnostics.
    EXPECT_EQ(semanticResult.diagnostic.size(), 0U);
    EXPECT_EQ(syntacticResult.diagnostic.size(), 0U);
}

// Test: Diagnostic ranges for a multi-line error stay within bounds
TEST_F(LspDiagnosticsBoundaryTests, MultiLineErrorRangeStaysWithinBounds)
{
    // Source with a type error spanning multiple lines.
    const std::string source = "let x: number = \"hello\";\nlet y: string = 42;\n";

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("multi_line_err.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    // Verify each diagnostic's range is within the source bounds.
    for (const auto &d : result.diagnostic) {
        // Line numbers should be valid (1-based in this LSP implementation, source has 2 lines).
        EXPECT_GE(d.range_.start.line_, 1U);
        EXPECT_LE(d.range_.start.line_, 2U);
        EXPECT_GE(d.range_.end.line_, 1U);
        EXPECT_LE(d.range_.end.line_, 2U);
        // End position should not be before start position.
        if (d.range_.start.line_ == d.range_.end.line_) {
            EXPECT_GE(d.range_.end.character_, d.range_.start.character_);
        }
    }
}

// Test: getSuggestionDiagnostics on empty file returns no diagnostics
TEST_F(LspDiagnosticsBoundaryTests, SuggestionDiagnosticsOnEmptyFileReturnsEmpty)
{
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("empty_suggest.ets", ES2PANDA_STATE_CHECKED, "");
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSuggestionDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    // An empty file should not produce any suggestion diagnostics.
    EXPECT_EQ(result.diagnostic.size(), 0U);
}

// Test: Diagnostics on file with trailing newline do not produce out-of-bounds ranges
TEST_F(LspDiagnosticsBoundaryTests, TrailingNewlineDoesNotProduceOutOfBoundsRanges)
{
    // Source with a syntax error and trailing newline.
    const std::string source = "let x = ;\n";

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("trailing_nl.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSyntacticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    // The source has 1 line of content (plus trailing newline).
    // Diagnostic ranges should be reasonable (LSP end positions are exclusive).
    for (const auto &d : result.diagnostic) {
        // Line should be 1 (the only content line) or within valid range.
        EXPECT_GE(d.range_.start.line_, 1U);
        EXPECT_LE(d.range_.end.line_, 2U);
        // Character on line 1 should be reasonable (allow exclusive end position).
        if (d.range_.end.line_ == 1U) {
            EXPECT_LE(d.range_.end.character_, 10U);
        }
    }
}

}  // namespace
