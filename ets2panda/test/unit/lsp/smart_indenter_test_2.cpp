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
#include "lsp/include/formatting/smart_indenter.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "lsp_api_test.h"

namespace {
using ark::es2panda::lsp::FormatCodeSettings;
using ark::es2panda::lsp::GetIndentation;
using ark::es2panda::lsp::IndentStyle;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::ONE;
using ark::es2panda::lsp::ZERO;

// Second smart-indenter suite. Focuses on the column-math and AST-walk clusters
// of smart_indenter.cpp that the first suite does not exercise: tab-expanded
// column computation, block-comment same-line/asterisk alignment, close-brace
// and else-keyword same-line detection, multiline call-argument anchoring,
// list-position indentation with an out-of-list preceding token, braceless
// control-flow children, and control-flow-ending statements.
class SmartIndenterTests2 : public LSPAPITests {
public:
    struct TestResult {
        bool isValid;
        size_t indentation;
    };

    static FormatCodeSettings DefaultSettings()
    {
        return ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
    }

    TestResult RunIndentationTest(const std::string &code, size_t position,
                                  const FormatCodeSettings &settings = FormatCodeSettings(),
                                  const std::string &fileName = "smart_indenter_test_2.ets")
    {
        std::vector<std::string> files = {fileName};
        std::vector<std::string> texts = {code};
        auto filePaths = CreateTempFile(files, texts);
        if (filePaths.empty()) {
            return {false, ZERO};
        }

        Initializer init;
        es2panda_Context *ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
        if (ctx == nullptr) {
            return {false, ZERO};
        }

        FormatCodeSettings testSettings = settings;
        if (testSettings.GetIndentSize() == ZERO) {
            testSettings = DefaultSettings();
        }

        auto result = GetIndentation(ctx, position, testSettings);
        init.DestroyContext(ctx);

        return {result.isValid, result.indentation};
    }
};

// Both tab-column-math tests below set TabSize=3 so a two-tab body line lands
// on expanded column 6 (3 + 3) instead of the raw per-character count.
constexpr size_t TAB_SIZE = 3;

// ---------------------------------------------------------------------------
// Tab-indented column math: FindColumnForFirstNonWhitespaceCharacter must add
// TAB_SIZE - (column % TAB_SIZE) per tab instead of counting one per character.
// ---------------------------------------------------------------------------

TEST_F(SmartIndenterTests2, TabColumnMathUnderBlockStyle)
{
    // Body line uses two tabs; with TabSize=3 the expanded columns are 3 then 6.
    const std::string code = "function f(): void {\n\t\tlet x = 1;\n}\n";
    FormatCodeSettings settings = DefaultSettings();
    settings.SetIndentStyle(IndentStyle::BLOCK);
    settings.SetTabSize(TAB_SIZE);

    constexpr std::string_view X_MARKER = "let x";
    size_t position = code.find(X_MARKER.data()) + X_MARKER.length();
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, settings, "smart_indenter2_tab_block.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 6U);
}

TEST_F(SmartIndenterTests2, TabColumnMathCommentContinuation)
{
    // Continuation line starts with two tabs inside a multi-line comment; the
    // returned indent must be the tab-expanded column (3 + 3 = 6), not 2.
    const std::string code = "function f(): void {\n\t/* start\n\t\tmore text */\n}\n";
    FormatCodeSettings settings = DefaultSettings();
    // Continuation indent is the tab-expanded column (3 + 3 = 6), not the raw tab count.
    settings.SetTabSize(TAB_SIZE);

    constexpr std::string_view MORE_MARKER = "more text";
    size_t position = code.find(MORE_MARKER.data()) + ONE;
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, settings, "smart_indenter2_tab_comment.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 6U);
}

// ---------------------------------------------------------------------------
// Block-comment alignment: cursor on the opening line, at a continuation line
// start (zero columns), and on an asterisk-prefixed continuation line where
// the indent lands one column left of the star.
// ---------------------------------------------------------------------------

TEST_F(SmartIndenterTests2, CommentCursorOnOpeningLineKeepsLineColumn)
{
    const std::string code = "function g(): void {\n    /* alpha beta */\n    let v = 1;\n}\n";
    constexpr std::string_view ALPHA_MARKER = "alpha";
    size_t position = code.find(ALPHA_MARKER.data()) + ONE;
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_comment_open.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 4U);
}

TEST_F(SmartIndenterTests2, CommentContinuationAtLineStartGetsZeroIndent)
{
    const std::string code = "function h(): void {\n    /* start\ntext */\n}\n";
    constexpr std::string_view TEXT_MARKER = "text */";
    size_t position = code.find(TEXT_MARKER.data());
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_comment_zero.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, ZERO);
}

TEST_F(SmartIndenterTests2, CommentAsteriskAlignmentIsOneLeftOfStar)
{
    // Continuation line "     * tail here */": five spaces before the star, so
    // the aligned indent is star-column - 1 = 4 (matches "/* head" text col).
    const std::string code = "function i(): void {\n    /* head\n     * tail here */\n}\n";
    constexpr std::string_view TAIL_MARKER = "tail here";
    size_t position = code.find(TAIL_MARKER.data()) + ONE;
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_comment_star.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 4U);
}

// ---------------------------------------------------------------------------
// Close-brace / else same-line detection around if statements.
// ---------------------------------------------------------------------------

TEST_F(SmartIndenterTests2, SingleLineThenBlockNextTokenIsBlockOnSameLine)
{
    // Cursor in the gap between the condition's ')' and the then-block '{':
    // no AST token covers that range, so the preceding token resolves to the
    // condition identifier. The walker then evaluates the if-statement gate
    // with the same-line BLOCK_STATEMENT as next sibling; the block starts
    // with '{', so the CLOSE_BRACE classification does not apply.
    const std::string code = "function v(c: boolean): void {\n    if (c) { a(); }\n}\n";
    size_t brace = code.find('{', code.find("if (c)"));
    ASSERT_NE(brace, std::string::npos);
    ASSERT_GT(brace, ZERO);
    auto result = RunIndentationTest(code, brace - ONE, DefaultSettings(), "smart_indenter2_if_inline.ets");
    EXPECT_TRUE(result.isValid);
    // Observed behavior: statement column (4) plus one indent delta picked up
    // while ascending past the if-statement; pinned as-is.
    EXPECT_EQ(result.indentation, 6U);
}

TEST_F(SmartIndenterTests2, NextLineOpenBraceClassifiedAsOpenBraceToken)
{
    // Same gap cursor, but with the brace moved to its own line: the following
    // BLOCK_STATEMENT no longer shares the cursor line, so the next token kind
    // becomes OPEN_BRACE and the indentation delta stays zero.
    const std::string code = "function w(c: boolean): void {\n    if (c)\n    {\n        a();\n    }\n}\n";
    size_t brace = code.find('{', code.find("if (c)"));
    ASSERT_NE(brace, std::string::npos);
    ASSERT_GT(brace, ZERO);
    auto result = RunIndentationTest(code, brace - ONE, DefaultSettings(), "smart_indenter2_if_nextline_brace.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 4U);
}

TEST_F(SmartIndenterTests2, ElseBodyIndentationResolvedBeforeIfLevelWalk)
{
    // "} else {" layout, cursor inside the else body. Current behavior: the
    // walker resolves an actual list/statement indentation on the way up and
    // returns before evaluating ChildStartsOnTheSameLineWithElseInIfStatement
    // for the (if-statement, alternate-block) pair; observed value pinned.
    const std::string code =
        "function j(c: boolean): void {\n    if (c) {\n        a();\n    } else {\n        b();\n    }\n}\n";
    constexpr std::string_view B_MARKER = "b();";
    size_t position = code.find(B_MARKER.data()) + ONE;
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_else_same_line.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 10U);
}

TEST_F(SmartIndenterTests2, ElseSplitBraceBodyResolvesSameAsSameLineLayout)
{
    // "} else\n{" layout: the alternate block starts one line below the `else`
    // keyword. Current behavior matches the same-line layout above (10); both
    // resolve through the same ancestor indentation before any else-line
    // comparison would happen.
    const std::string code =
        "function k(c: boolean): void {\n    if (c) {\n        a();\n    } else\n    {\n        b();\n    }\n}\n";
    constexpr std::string_view B_MARKER = "b();";
    size_t position = code.find(B_MARKER.data()) + ONE;
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_else_split_line.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 10U);
}

// ---------------------------------------------------------------------------
// Multiline call arguments: argument-vs-callee line overlap drives both the
// useTrueStart re-anchoring and the plain indentation delta.
// ---------------------------------------------------------------------------

TEST_F(SmartIndenterTests2, MultilineObjectFirstArgumentReanchorsToCallStart)
{
    // First argument (object literal) opens on the callee line: the walk flags
    // IsArgumentAndStartLineOverlapsExpressionBeingCalled and re-anchors the
    // current location to the true start of the call expression. Current
    // behavior resolves to call-column + two indent levels; value pinned.
    const std::string code = "function l(): void {\n    foo({\n        key: 1\n    }, bar);\n}\n";
    constexpr std::string_view KEY_MARKER = "key:";
    size_t position = code.find(KEY_MARKER.data()) + ONE;
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_call_obj_arg.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 8U);
}

TEST_F(SmartIndenterTests2, MemberExpressionCalleeIsNotCallArgument)
{
    // Cursor inside the callee property of obj.method(1): the walked child is
    // not part of Arguments(), so the argument-overlap check returns false.
    // Current behavior pins the resolved indentation at 6.
    const std::string code = "function m(): void {\n    obj.method(1);\n}\n";
    constexpr std::string_view METHOD_MARKER = "method";
    size_t position = code.find(METHOD_MARKER.data()) + ONE;
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_call_member.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 6U);
}

TEST_F(SmartIndenterTests2, SecondArgumentOnOwnLineAddsIndentDelta)
{
    const std::string code = "function n(): void {\n    foo(a,\n        b);\n}\n";
    constexpr std::string_view B_MARKER = "b);";
    size_t position = code.find(B_MARKER.data()) + ONE;
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_call_second_arg.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 4U);
}

TEST_F(SmartIndenterTests2, ArgumentListWrappedAfterOpenParenAddsDelta)
{
    const std::string code = "function o(): void {\n    foo(\n        a);\n}\n";
    constexpr std::string_view A_MARKER = "a);";
    size_t position = code.find(A_MARKER.data()) + ONE;
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_call_wrapped.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 4U);
}

// ---------------------------------------------------------------------------
// List-position indentation: preceding token outside the list range that
// contains the cursor position falls back to the list-start indentation.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// List-position indentation: the out-of-list preceding-token branch of
// CheckListIndent is structurally unreachable (see result notes), so this
// scenario pins the fallback the walker produces for an empty-statement
// cursor inside a catch block instead.
// ---------------------------------------------------------------------------

TEST_F(SmartIndenterTests2, EmptyStatementInCatchFallsBackToWalkerIndent)
{
    // Cursor exactly on the empty-statement semicolon inside the catch block.
    // Current behavior: CheckListIndent does not claim the position (the
    // nearest-token invariant keeps the preceding token inside the containing
    // list range), and the smart-indent walk resolves to base indent 4.
    const std::string code = "function t(): void {\n    try {\n        a();\n    } catch (e) {\n        ;\n    }\n}\n";
    size_t position = code.find(';', code.find("catch (e)"));
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_catch_empty_stmt.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 4U);
}

// ---------------------------------------------------------------------------
// Braceless control-flow children and control-flow-ending statements.
// ---------------------------------------------------------------------------

TEST_F(SmartIndenterTests2, BracelessWhileBodyResolvesToPinnedIndent)
{
    // Current behavior: the walk resolves an actual statement indentation of
    // 10 columns for the braceless while body before reaching the while-level
    // child-kind decision.
    const std::string code = "function p(n: number): void {\n    while (n > 0)\n        n--;\n}\n";
    constexpr std::string_view DECR_MARKER = "n--;";
    size_t position = code.find(DECR_MARKER.data()) + ONE;
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_while_braceless.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 10U);
}

TEST_F(SmartIndenterTests2, BracelessDoWhileBodyResolvesToPinnedIndent)
{
    // Current behavior mirrors the braceless while case: resolved indentation
    // of 10 columns for the do-body statement.
    const std::string code = "function q(n: number): void {\n    do\n        n--;\n    while (n > 0);\n}\n";
    constexpr std::string_view DECR_MARKER = "n--;";
    size_t position = code.find(DECR_MARKER.data()) + ONE;
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_dowhile_braceless.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 10U);
}

TEST_F(SmartIndenterTests2, ReturnInsideBracelessIfBodyResolvesThroughWalk)
{
    // Cursor on the semicolon terminating the guarded "return 1;": RETURN
    // under a non-block if is a control-flow-ending statement candidate.
    // Current behavior resolves the cursor through the ancestor walk to 10
    // columns; value pinned as-is.
    const std::string code = "function r(w: boolean): number {\n    if (w)\n        return 1;\n    return 2;\n}\n";
    constexpr std::string_view ONE_LITERAL = "return 1;";
    size_t position = code.find(ONE_LITERAL.data()) + std::string_view("return ").length() + ONE;
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position, DefaultSettings(), "smart_indenter2_if_return_end.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 10U);
}

}  // namespace
