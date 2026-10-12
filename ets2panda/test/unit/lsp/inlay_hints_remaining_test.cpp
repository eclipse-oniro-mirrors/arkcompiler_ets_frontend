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
#include "lsp/include/inlay_hints.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/types.h"
#include "lsp/include/user_preferences.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::AddEnumMemberValueHints;
using ark::es2panda::lsp::AddParameterHints;
using ark::es2panda::lsp::AddTypeHints;
using ark::es2panda::lsp::DecodedTextSpanIntersectsWith;
using ark::es2panda::lsp::GetFullWidth;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::IsExpressionWithTypeArguments;
using ark::es2panda::lsp::SaveToList;
using ark::es2panda::lsp::ShouldShowParameterNameHints;
using ark::es2panda::lsp::TextSpanIntersectsWith;
using ark::es2panda::lsp::UserPreferences;

class LspInlayHintsRemainingTests : public LSPAPITests {
protected:
    AstNode *FindNode(AstNode *root, std::function<bool(AstNode *)> pred)
    {
        AstNode *result = nullptr;
        root->FindChild([&result, &pred](AstNode *n) {
            if (result == nullptr && pred(n)) {
                result = n;
            }
            return false;
        });
        return result;
    }
};

// =============================================================================
// P2D-IH-01: GetFullWidth_ReturnsWidth
// =============================================================================
TEST_F(LspInlayHintsRemainingTests, GetFullWidth_ReturnsWidth)
{
    const std::string source = "let x = 42;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ih01.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the NumberLiteral "42" node
    auto *numberLit =
        FindNode(ast, [](AstNode *n) { return n->IsNumberLiteral() && n->AsNumberLiteral()->Number().IsInt(); });
    ASSERT_NE(numberLit, nullptr);

    // "42" occupies 2 bytes: pin the literal width instead of mirroring the
    // production End().index - Start().index computation.
    const int expectedLiteralWidth = 2;
    EXPECT_EQ(GetFullWidth(numberLit), expectedLiteralWidth);
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-IH-02: DecodedTextSpanIntersectsWith_True
// =============================================================================
TEST_F(LspInlayHintsRemainingTests, DecodedTextSpanIntersectsWith_True)
{
    // Overlapping spans: [0, 5) and [3, 8) → true
    EXPECT_TRUE(DecodedTextSpanIntersectsWith(0, 5, 3, 8));

    // One contains the other: [0, 10) and [2, 5) → true
    EXPECT_TRUE(DecodedTextSpanIntersectsWith(0, 10, 2, 5));

    // Touching at boundary: [0, 5) and [5, 10) → true (start2 == end1: inclusive)
    EXPECT_TRUE(DecodedTextSpanIntersectsWith(0, 5, 5, 5));

    // Exact same span: [3, 7) and [3, 7) → true
    const int identicalStart = 3;
    const int identicalLength = 4;
    EXPECT_TRUE(DecodedTextSpanIntersectsWith(identicalStart, identicalLength, identicalStart, identicalLength));
}

// =============================================================================
// P2D-IH-03: DecodedTextSpanIntersectsWith_False
// =============================================================================
TEST_F(LspInlayHintsRemainingTests, DecodedTextSpanIntersectsWith_False)
{
    // Non-overlapping: [0, 5) and [7, 10) → false
    const int firstSpanLength = 5;
    const int nonOverlapStart = 7;
    const int nonOverlapLen = 3;
    EXPECT_FALSE(DecodedTextSpanIntersectsWith(0, firstSpanLength, nonOverlapStart, nonOverlapLen));

    // Non-overlapping: [10, 5) and [0, 8) → false (start2=0, end1=15, end2=8, start2=0 <= end1=15 true, but end2=8 >=
    // start1=10 false) Actually: start1=10, length1=5 → end1=15; start2=0, length2=8 → end2=8 start2 <= end1: 0 <= 15 →
    // true end2 >= start1: 8 >= 10 → false
    const int reversedLen = 5;
    const int reversedStart = 10;
    const int reversedEnd = 8;
    EXPECT_FALSE(DecodedTextSpanIntersectsWith(reversedStart, reversedLen, 0, reversedEnd));
}

// =============================================================================
// P2D-IH-04: TextSpanIntersectsWith_True
// =============================================================================
TEST_F(LspInlayHintsRemainingTests, TextSpanIntersectsWith_True)
{
    // TextSpan at 0 length 10, position 3 size 5 → true
    TextSpan span(0, 10);
    const int intersectPos = 3;
    const int intersectSize = 5;
    EXPECT_TRUE(TextSpanIntersectsWith(span, intersectPos, intersectSize));

    // TextSpan at 5 length 5, position 5 size 5 → true (touching)
    TextSpan span2(5, 5);
    const int touchPos = 5;
    const int touchSize = 5;
    EXPECT_TRUE(TextSpanIntersectsWith(span2, touchPos, touchSize));
}

// =============================================================================
// P2D-IH-05: TextSpanIntersectsWith_False
// =============================================================================
TEST_F(LspInlayHintsRemainingTests, TextSpanIntersectsWith_False)
{
    // TextSpan at 0 length 5, position 7 size 3 → false
    TextSpan ts(0, 5);
    const int nonOverlapPos = 7;
    const int nonOverlapSize = 3;
    EXPECT_FALSE(TextSpanIntersectsWith(ts, nonOverlapPos, nonOverlapSize));
}

// =============================================================================
// P2D-IH-06: IsExpressionWithTypeArguments_True
// =============================================================================
// True branch already exercised end-to-end by inlay_hints_test_1
// ParameterNameHintsViaPublicApi; this isolates the predicate directly
// (intentional overlap, unit seam vs pipeline seam).
TEST_F(LspInlayHintsRemainingTests, IsExpressionWithTypeArguments_True)
{
    const std::string source = R"(
function foo(): void {}
let x = foo();
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ih06.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find a CallExpression
    auto *callExpr = FindNode(ast, [](AstNode *n) { return n->IsCallExpression(); });
    ASSERT_NE(callExpr, nullptr);

    EXPECT_TRUE(IsExpressionWithTypeArguments(callExpr));
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-IH-07: IsExpressionWithTypeArguments_False
// =============================================================================
TEST_F(LspInlayHintsRemainingTests, IsExpressionWithTypeArguments_False)
{
    const std::string source = "let x = 42;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ih07.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find a NumberLiteral (not a CallExpression or NewExpression)
    auto *numLit = FindNode(ast, [](AstNode *n) { return n->IsNumberLiteral(); });
    ASSERT_NE(numLit, nullptr);

    EXPECT_FALSE(IsExpressionWithTypeArguments(numLit));
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-IH-08: ShouldShowParameterNameHints_True
// =============================================================================
// NONE vs LITERALS/ALL preference branches are proven via hint output in
// inlay_hints_test_1; this pins the 1-line predicate decision directly.
TEST_F(LspInlayHintsRemainingTests, ShouldShowParameterNameHints_True)
{
    UserPreferences prefs;

    // With LITERALS setting
    prefs.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::LITERALS);
    EXPECT_TRUE(ShouldShowParameterNameHints(prefs));

    // With ALL setting
    prefs.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::ALL);
    EXPECT_TRUE(ShouldShowParameterNameHints(prefs));
}

// =============================================================================
// P2D-IH-09: ShouldShowParameterNameHints_False
// =============================================================================
TEST_F(LspInlayHintsRemainingTests, ShouldShowParameterNameHints_False)
{
    UserPreferences prefs;

    // Default is NONE
    prefs.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::NONE);
    EXPECT_FALSE(ShouldShowParameterNameHints(prefs));
}

// =============================================================================
// P2D-IH-10: AddEnumMemberValueHints_AddsHint
// =============================================================================
TEST_F(LspInlayHintsRemainingTests, AddEnumMemberValueHints_AddsHint)
{
    InlayHintList result;
    std::string text = "myValue";
    int position = 42;

    AddEnumMemberValueHints(text, position, &result);

    ASSERT_EQ(result.hints.size(), 1U);
    EXPECT_EQ(result.hints[0].text, "myValue");
    EXPECT_EQ(result.hints[0].number, position);
    EXPECT_EQ(result.hints[0].kind, InlayHintKind::ENUM);
    EXPECT_TRUE(result.hints[0].whitespaceBefore);
    EXPECT_FALSE(result.hints[0].whitespaceAfter);
}

// =============================================================================
// P2D-IH-11: AddTypeHints_AddsHint
// =============================================================================
// TYPE hint convention asserted end-to-end in inlay_hints_test_1
// TypeHintsForVariableParameterAndReturn; this pins the helper in isolation.
TEST_F(LspInlayHintsRemainingTests, AddTypeHints_AddsHint)
{
    InlayHintList result;
    std::string text = "number";
    int position = 100;

    AddTypeHints(text, position, &result);

    ASSERT_EQ(result.hints.size(), 1U);
    EXPECT_EQ(result.hints[0].text, "number");
    EXPECT_EQ(result.hints[0].number, position);
    EXPECT_EQ(result.hints[0].kind, InlayHintKind::TYPE);
    EXPECT_TRUE(result.hints[0].whitespaceBefore);
    EXPECT_FALSE(result.hints[0].whitespaceAfter);
}

// =============================================================================
// P2D-IH-12: AddParameterHints_AddsHint
// =============================================================================
TEST_F(LspInlayHintsRemainingTests, AddParameterHints_AddsHint)
{
    // Non-variadic argument
    InlayHintList result;
    std::string text = "x";
    int position = 200;

    AddParameterHints(text, position, false, &result);

    ASSERT_EQ(result.hints.size(), 1U);
    EXPECT_EQ(result.hints[0].text, "x");
    EXPECT_EQ(result.hints[0].number, position);
    EXPECT_EQ(result.hints[0].kind, InlayHintKind::PARAMETER);
    EXPECT_FALSE(result.hints[0].whitespaceBefore);
    EXPECT_TRUE(result.hints[0].whitespaceAfter);

    // Variadic argument
    InlayHintList result2;
    AddParameterHints(text, position, true, &result2);

    ASSERT_EQ(result2.hints.size(), 1U);
    EXPECT_EQ(result2.hints[0].text, "...x");
    EXPECT_EQ(result2.hints[0].number, position);
    EXPECT_EQ(result2.hints[0].kind, InlayHintKind::PARAMETER);
}

// =============================================================================
// P2D-IH-13: SaveToList_SavesHint
// =============================================================================
TEST_F(LspInlayHintsRemainingTests, SaveToList_SavesHint)
{
    const std::string source = "let x = 42;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ih13.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the NumberLiteral "42" (expression for the value)
    auto *numberLit =
        FindNode(ast, [](AstNode *n) { return n->IsNumberLiteral() && n->AsNumberLiteral()->Number().IsInt(); });
    ASSERT_NE(numberLit, nullptr);

    // Find the VariableDeclarator (used as the assign node for End().index)
    auto *varDeclarator = FindNode(ast, [](AstNode *n) { return n->IsVariableDeclarator(); });
    ASSERT_NE(varDeclarator, nullptr);

    // SaveToList should add an enum member value hint with the number text
    InlayHintList list;
    SaveToList(numberLit->AsExpression(), varDeclarator, &list);

    // The function should add a hint with the number value "42" at the
    // variable declarator's end position
    ASSERT_EQ(list.hints.size(), 1U);
    // The number 42 should produce "42" as text
    EXPECT_EQ(list.hints[0].text, "42");
    EXPECT_EQ(list.hints[0].kind, InlayHintKind::ENUM);
    EXPECT_EQ(list.hints[0].number, static_cast<int>(varDeclarator->End().index));
    initializer.DestroyContext(ctx);
}

}  // namespace
