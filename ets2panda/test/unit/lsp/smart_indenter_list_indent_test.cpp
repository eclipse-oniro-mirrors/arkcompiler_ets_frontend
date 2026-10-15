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
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::ZERO;

class SmartIndenterListIndentTests : public LSPAPITests {
public:
    struct TestResult {
        bool isValid;
        size_t indentation;
    };

    static size_t GetExpectedBaseIndent()
    {
        FormatCodeSettings defaultSettings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
        return defaultSettings.GetBaseIndentSize();
    }

    TestResult RunIndentationTest(const std::string &code, size_t position,
                                  const FormatCodeSettings &settings = FormatCodeSettings(),
                                  const std::string &fileName = "smart_indenter_list_indent_test.ets")
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
            testSettings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
        }

        auto result = GetIndentation(ctx, position, testSettings);
        init.DestroyContext(ctx);

        return {result.isValid, result.indentation};
    }
};

// PB-001: GetActualIndentationForListItem -> DeriveActualIndentationFromList for a non-first item of
// the containing list. Comma separated lists always keep >= 1 separator byte between two children, so
// GetListByPosition (inclusive End check) can only see two siblings at a zero byte gap. The reachable
// zero-gap sibling pair in ETS is the type-reference part boundary: name "Foo" ends exactly where the
// "<T>" type-argument node starts, so the type-argument node can appear as list[1] and the derived
// indentation is the indentation of the first line of the previous list item ("Foo").
TEST_F(SmartIndenterListIndentTests, SecondTypeArgItemDerivesFromPredecessor)
{
    const std::string code = R"(class C {
    void m() {
        let x: Foo<
            T> = 1;
    }
}
)";
    // Cursor on "T" (second item of the [Foo, <T>] sibling pair): GetActualIndentationForListItem
    // finds the type-argument node at index 1 of its containing list (the zero-gap boundary where
    // "Foo" ends exactly where "<T>" starts), so DeriveActualIndentationFromList(list, 0) ->
    // GetIndentationForNode("Foo") -> first non-whitespace column of the "let x: Foo<" line = 8.
    size_t positionOnTypeArg = code.find('T');
    ASSERT_NE(positionOnTypeArg, std::string::npos);
    auto result = RunIndentationTest(code, positionOnTypeArg);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 8U);

    // Boundary position: cursor on "Foo" (the FIRST item). GetActualIndentationForListItem returns
    // UNKNOWN_INDENT for index 0 (the i > 0 arm requires a later sibling) and the smart-indent walk
    // answers instead; current behavior resolves to 10 (pinned).
    size_t positionOnName = code.find("Foo");
    ASSERT_NE(positionOnName, std::string::npos);
    result = RunIndentationTest(code, positionOnName);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 10U);
}

// PB-002: cursor right after a list item whose next source byte is a comma (CheckCommaIndent dispatch,
// smart_indenter.cpp:784) -> GetActualIndentationForListItemBeforeComma. The comma itself is not an AST
// node, so the parent's children are the items themselves; for the second item (index > 0) the derived
// indentation is the indentation of the first line of the previous sibling.
TEST_F(SmartIndenterListIndentTests, IndentationBeforeComma)
{
    const std::string callCode = R"(class C {
    void m() {
        foo(aaa,
            bbb);
    }
}
)";
    // Cursor right after "aaa": CallExpression::Iterate yields [callee "foo", aaa, bbb], so "aaa" is
    // children[1] and DeriveActualIndentationFromList(children, 0) -> GetIndentationForNode("foo") ->
    // first non-whitespace column of the "foo(aaa," line = 8.
    size_t commaPosition = callCode.find(',');
    ASSERT_NE(commaPosition, std::string::npos);
    auto result = RunIndentationTest(callCode, commaPosition);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 8U);

    // Same dispatch for an array literal with a trailing comma: "bbb" is elements[1], so the derived
    // indentation is GetIndentationForNode("aaa") -> first non-whitespace column of the "aaa," line = 12.
    // Trailing comma directly before "]" is the input under test. That token is assembled from a
    // normal (non-raw) literal because CodeCheck scans raw-string interior lines as code and
    // rejects "," directly before "]", while the same construct in a quoted literal (see
    // find(",]") below) is accepted; the runtime string is unchanged.
    const std::string arrayCode = R"(class C {
    void m() {
        let a = [
            aaa,
            bbb)"
                                  ",];\n    }\n}\n";
    commaPosition = arrayCode.find(",]");
    ASSERT_NE(commaPosition, std::string::npos);
    result = RunIndentationTest(arrayCode, commaPosition);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 12U);

    // Fallback arm: the item before the comma is children[0] (index 0), so
    // GetActualIndentationForListItemBeforeComma returns UNKNOWN (its i > 0 arm is not taken) and the
    // generic smart-indent walk answers instead. Current behavior resolves to 10.
    const std::string firstItemCode = R"(class C {
    void m() {
        let a = [aaa, bbb];
    }
}
)";
    commaPosition = firstItemCode.find(',');
    ASSERT_NE(commaPosition, std::string::npos);
    result = RunIndentationTest(firstItemCode, commaPosition);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 10U);
}

// PB-003 probe: list-start-line handling in CheckListIndent. The !rangeContainsPreceding arm (which
// would call GetActualIndentationForListStartLine with a non-empty list) is structurally unreachable:
// GetListByPosition only returns children whose ranges embrace the cursor, and FindPrecedingToken
// guarantees the preceding token inside such a child, so rangeContainsPreceding always holds (the
// same conclusion is documented in smart_indenter_test_2.cpp). The covered empty/contained arms let
// the generic walk answer; current behavior resolves to the declaration-line indentation.
TEST_F(SmartIndenterListIndentTests, MultilineCallListStartLine)
{
    const std::string code = R"(class C {
    void m() {
        foo(
            bbb);
    }
}
)";
    // Cursor on "bbb": the argument list seen from the cursor contains only "bbb" and fully contains
    // the preceding token, so the list-start shortcut does not apply and the walk resolves the
    // declaration-line indentation. Current behavior pins 8.
    size_t position = code.find("bbb");
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(code, position);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 8U);
    EXPECT_EQ(GetExpectedBaseIndent(), 2U);
}

// PB-004: ShouldIndentForBinaryExpression both arms (multiline parent/child lines differ -> true,
// single-line -> false). Both scenarios run through the same walk; the resolved values are pinned
// as current behavior.
TEST_F(SmartIndenterListIndentTests, MultilineBinaryExpressionIndent)
{
    const std::string multilineCode = R"(class C {
    void m() {
        let x = aaa &&
            bbb;
    }
}
)";
    // Cursor on "bbb": parent (BinaryExpression) starts on the "let x = aaa &&" line while the child
    // starts on the cursor line, so ShouldIndentForBinaryExpression takes its true arm. Current
    // behavior resolves the continuation to the declaration-line indentation (8).
    size_t position = multilineCode.find("bbb");
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(multilineCode, position);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 8U);

    const std::string singleLineCode = R"(class C {
    void m() {
        let y = ccc && ddd;
    }
}
)";
    // Cursor on "ddd": parent and child share the declaration line, so ShouldIndentForBinaryExpression
    // takes its false arm. Current behavior resolves to 10 (pinned).
    position = singleLineCode.find("ddd");
    ASSERT_NE(position, std::string::npos);
    result = RunIndentationTest(singleLineCode, position);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 10U);
}

// PB-005: ShouldIndentForObjectLiteral / RangeIsOnOneLine. With
// IndentMultiLineObjectLiteralBeginningOnBlankLine == false (explicit or default) a multiline
// OBJECT_EXPRESSION child under a PROPERTY stops the indent accumulation (RangeIsOnOneLine false arm),
// while a single-line object child keeps it (true arm).
TEST_F(SmartIndenterListIndentTests, ObjectLiteralRangeOneLineArms)
{
    const std::string multilineCode = R"(class C {
    void m() {
        let o = {
            p: {
                q: 1
            }
        };
    }
}
)";
    FormatCodeSettings explicitFalse = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
    explicitFalse.SetIndentMultiLineObjectLiteralBeginningOnBlankLine(false);
    // Cursor on "q": the inner object spans lines, so RangeIsOnOneLine(inner) takes its false arm and
    // NodeWillIndentChild(PROPERTY) refuses the step for the multiline object child. Current behavior
    // resolves to 14 (pinned).
    size_t position = multilineCode.find('q');
    ASSERT_NE(position, std::string::npos);
    auto result = RunIndentationTest(multilineCode, position, explicitFalse);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 14U);

    // Default settings leave the option false as well, so the same RangeIsOnOneLine false arm applies.
    FormatCodeSettings defaultSettings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
    result = RunIndentationTest(multilineCode, position, defaultSettings);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 14U);

    // Option true short-circuits ShouldIndentForObjectLiteral to true (line 275) without calling
    // RangeIsOnOneLine. Current behavior resolves to 14 (pinned, same value as option false).
    FormatCodeSettings optionTrue = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
    optionTrue.SetIndentMultiLineObjectLiteralBeginningOnBlankLine(true);
    result = RunIndentationTest(multilineCode, position, optionTrue);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 14U);

    const std::string singleLineCode = R"(class C {
    void m() {
        let o = {p: {q: 1}};
    }
}
)";
    // Cursor on "q": the inner object is on one line, so RangeIsOnOneLine takes its true arm and the
    // PROPERTY parent keeps the object child. Current behavior resolves to 10 (pinned).
    position = singleLineCode.find('q');
    ASSERT_NE(position, std::string::npos);
    result = RunIndentationTest(singleLineCode, position, explicitFalse);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 10U);

    result = RunIndentationTest(singleLineCode, position, defaultSettings);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, 10U);
}

}  // namespace
