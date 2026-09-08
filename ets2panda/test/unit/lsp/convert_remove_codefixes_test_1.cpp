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

#include "generated/code_fix_register.h"
#include "lsp_api_test.h"
#include "util/diagnostic.h"

#include <algorithm>
#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/register_code_fix/constructor_for_derived_need_super_call.h"
#include "lsp/include/register_code_fix/convert_const_to_let.h"
#include "lsp/include/register_code_fix/fix_remove_override_modifier.h"
#include "lsp/include/register_code_fix/fix_unreachable_code.h"
#include "lsp/include/register_code_fix/remove_accidental_call_parentheses.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::CONSTRUCTOR_DERIVED_NEED_SUPER;
using ark::es2panda::lsp::codefixes::FIX_CONVERT_CONST_TO_LET;
using ark::es2panda::lsp::codefixes::FIX_UNREACHABLE_CODE;
using ark::es2panda::lsp::codefixes::REMOVE_ACCIDENTAL_CALL_PARENTHESES;
using ark::es2panda::lsp::codefixes::REMOVE_OVERRIDE_MODIFIER;

constexpr std::string_view CONST_TO_LET_FIX_NAME = FIX_CONVERT_CONST_TO_LET.GetFixId();
constexpr std::string_view CONST_TO_LET_FIX_DESCRIPTION = "Convert const to let";
constexpr std::string_view CONST_TO_LET_NEW_TEXT = "let";
constexpr auto CONST_TO_LET_ERROR_CODES = FIX_CONVERT_CONST_TO_LET.GetSupportedCodeNumbers();

constexpr std::string_view OVERRIDE_FIX_NAME = REMOVE_OVERRIDE_MODIFIER.GetFixId();
constexpr std::string_view OVERRIDE_FIX_DESCRIPTION = "Remove override modifier";
constexpr auto OVERRIDE_ERROR_CODES = REMOVE_OVERRIDE_MODIFIER.GetSupportedCodeNumbers();

constexpr std::string_view PARENS_FIX_NAME = REMOVE_ACCIDENTAL_CALL_PARENTHESES.GetFixId();
constexpr std::string_view PARENS_FIX_DESCRIPTION = "Remove parentheses from accessor call";
constexpr auto PARENS_ERROR_CODES = REMOVE_ACCIDENTAL_CALL_PARENTHESES.GetSupportedCodeNumbers();

constexpr std::string_view SUPER_FIX_NAME = CONSTRUCTOR_DERIVED_NEED_SUPER.GetFixId();
constexpr std::string_view SUPER_FIX_DESCRIPTION = "Add missing 'super()' call to derived constructor";
constexpr auto SUPER_ERROR_CODES = CONSTRUCTOR_DERIVED_NEED_SUPER.GetSupportedCodeNumbers();

constexpr std::string_view UNREACHABLE_FIX_NAME = FIX_UNREACHABLE_CODE.GetFixId();
// UNREACHABLE_STMT: DiagnosticType::WARNING * DIAGNOSTIC_CODE_MULTIPLIER + 26
constexpr int UNREACHABLE_STMT_CODE = 3026;
constexpr auto UNREACHABLE_ERROR_CODES = FIX_UNREACHABLE_CODE.GetSupportedCodeNumbers();

constexpr int DEFAULT_THROTTLE = 20;
constexpr size_t CONST_KEYWORD_LENGTH = 5;

class ConvertRemoveCodeFixTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static CodeFixOptions CreateCodeFixOptions()
    {
        return {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    }

    template <size_t N>
    static std::vector<int> ToErrorCodeVector(const std::array<int, N> &codes)
    {
        return std::vector<int>(codes.begin(), codes.end());
    }

    static std::vector<CodeFixActionInfo> GetFixesAt(es2panda_Context *context, std::string_view marker,
                                                     std::vector<int> &errorCodes, CodeFixOptions &options)
    {
        const size_t start = currentSource_.find(marker);
        EXPECT_NE(start, std::string::npos);
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 1, errorCodes, options);
    }

    static std::string ApplyTextChanges(std::string text, std::vector<TextChange> changes)
    {
        std::sort(changes.begin(), changes.end(),
                  [](const TextChange &left, const TextChange &right) { return left.span.start > right.span.start; });
        for (const auto &change : changes) {
            text.replace(change.span.start, change.span.length, change.newText);
        }
        return text;
    }

    static void SetCurrentSource(const std::string &source)
    {
        currentSource_ = source;
    }

private:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    static NullCancellationToken &GetNullHost()
    {
        static NullCancellationToken instance;
        return instance;
    }

    // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
    static inline std::string currentSource_;
};

// Convert const to let: shadowing scenario. The cursor is on the assignment to the
// function-local const, so only the local declaration must be rewritten and the
// global const with the same name must stay untouched.
TEST_F(ConvertRemoveCodeFixTests, ConvertConstToLetShadowingOnlyChangesLocalDeclaration)
{
    const std::string source = R"(
const value = 0;
function updateValue(): void {
    const value = 1;
    value = 2;
}
)";
    SetCurrentSource(source);
    Initializer initializer = Initializer();
    auto *context =
        initializer.CreateContext("convert_const_to_let_shadowing.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    const size_t globalConstPos = source.find("const value = 0");
    const size_t localConstPos = source.find("const value = 1");
    ASSERT_NE(globalConstPos, std::string::npos);
    ASSERT_NE(localConstPos, std::string::npos);

    auto errorCodes = ToErrorCodeVector(CONST_TO_LET_ERROR_CODES);
    CodeFixOptions options = CreateCodeFixOptions();
    auto fixResult = GetFixesAt(context, "value = 2", errorCodes, options);
    initializer.DestroyContext(context);

    ASSERT_EQ(fixResult.size(), 2U);
    const auto &fix = fixResult[0];
    ASSERT_EQ(fix.fixName_, CONST_TO_LET_FIX_NAME);
    ASSERT_EQ(fix.fixId_, CONST_TO_LET_FIX_NAME);
    ASSERT_EQ(fix.description_, CONST_TO_LET_FIX_DESCRIPTION);
    ASSERT_EQ(fix.changes_.size(), 1U);
    ASSERT_EQ(fix.changes_[0].textChanges.size(), 1U);
    const auto &change = fix.changes_[0].textChanges[0];
    EXPECT_EQ(change.span.start, localConstPos);
    EXPECT_EQ(change.span.length, CONST_KEYWORD_LENGTH);
    EXPECT_EQ(change.newText, CONST_TO_LET_NEW_TEXT);
    EXPECT_NE(change.span.start, globalConstPos);
}

// Convert const to let: destructuring const declaration reassigned later.
// The fix rewrites the const keyword of the destructuring declaration.
TEST_F(ConvertRemoveCodeFixTests, ConvertConstToLetArrayDestructuring)
{
    const std::string source = R"(
const [first, second] = [1, 2];
first = 3;
)";
    SetCurrentSource(source);
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("convert_const_to_let_array_destructuring.ets", ES2PANDA_STATE_CHECKED,
                                              source.c_str());
    ASSERT_NE(context, nullptr);

    auto errorCodes = ToErrorCodeVector(CONST_TO_LET_ERROR_CODES);
    CodeFixOptions options = CreateCodeFixOptions();
    auto fixResult = GetFixesAt(context, "first = 3", errorCodes, options);
    initializer.DestroyContext(context);

    const size_t constPos = source.find("const [first");
    ASSERT_NE(constPos, std::string::npos);

    ASSERT_EQ(fixResult.size(), 2U);
    const auto &fix = fixResult[0];
    ASSERT_EQ(fix.fixName_, CONST_TO_LET_FIX_NAME);
    ASSERT_EQ(fix.fixId_, CONST_TO_LET_FIX_NAME);
    ASSERT_EQ(fix.description_, CONST_TO_LET_FIX_DESCRIPTION);
    ASSERT_EQ(fix.changes_.size(), 1U);
    ASSERT_EQ(fix.changes_[0].textChanges.size(), 1U);
    const auto &change = fix.changes_[0].textChanges[0];
    EXPECT_EQ(change.span.start, constPos);
    EXPECT_EQ(change.span.length, CONST_KEYWORD_LENGTH);
    EXPECT_EQ(change.newText, CONST_TO_LET_NEW_TEXT);
}

// Remove override modifier: deleting the token must also delete the trailing
// space so that the member line keeps correct spacing.
TEST_F(ConvertRemoveCodeFixTests, DISABLED_RemoveOverrideModifierDeletesTrailingSpace)
{
    const std::string source = R"(
class Animal {
}
class Dog extends Animal {
    override foo() {}
}
)";
    SetCurrentSource(source);
    Initializer initializer = Initializer();
    auto *context =
        initializer.CreateContext("remove_override_modifier_spacing.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    auto errorCodes = ToErrorCodeVector(OVERRIDE_ERROR_CODES);
    CodeFixOptions options = CreateCodeFixOptions();
    auto fixResult = GetFixesAt(context, "foo()", errorCodes, options);
    initializer.DestroyContext(context);

    const size_t overridePos = source.find("override");
    ASSERT_NE(overridePos, std::string::npos);
    // Delete the keyword and its horizontal separator so the method keeps the
    // original indentation without introducing an extra space.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t overrideWithSpaceLength = 9;

    ASSERT_EQ(fixResult.size(), 1U);
    const auto &fix = fixResult[0];
    ASSERT_EQ(fix.fixName_, OVERRIDE_FIX_NAME);
    ASSERT_EQ(fix.fixId_, OVERRIDE_FIX_NAME);
    ASSERT_EQ(fix.description_, OVERRIDE_FIX_DESCRIPTION);
    ASSERT_EQ(fix.changes_.size(), 1U);
    ASSERT_EQ(fix.changes_[0].textChanges.size(), 1U);
    const auto &change = fix.changes_[0].textChanges[0];
    EXPECT_EQ(change.span.start, overridePos);
    EXPECT_EQ(change.span.length, overrideWithSpaceLength);
    EXPECT_EQ(change.newText, "");
    EXPECT_EQ(ApplyTextChanges(source, fix.changes_[0].textChanges), R"(
class Animal {
}
class Dog extends Animal {
    foo() {}
}
)");
}

// Remove override modifier: applied to a line with several modifiers, only the
// override token and its trailing space are removed, other modifiers survive.
TEST_F(ConvertRemoveCodeFixTests, DISABLED_RemoveOverrideModifierKeepsOtherModifiers)
{
    const std::string source = R"(
class Animal {
}
class Dog extends Animal {
    public override foo() {}
}
)";
    SetCurrentSource(source);
    Initializer initializer = Initializer();
    auto *context =
        initializer.CreateContext("remove_override_modifier_keep_others.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    auto errorCodes = ToErrorCodeVector(OVERRIDE_ERROR_CODES);
    CodeFixOptions options = CreateCodeFixOptions();
    auto fixResult = GetFixesAt(context, "foo()", errorCodes, options);
    initializer.DestroyContext(context);

    const size_t overridePos = source.find("override");
    ASSERT_NE(overridePos, std::string::npos);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t overrideWithSpaceLength = 9;

    ASSERT_EQ(fixResult.size(), 1U);
    const auto &fix = fixResult[0];
    ASSERT_EQ(fix.fixName_, OVERRIDE_FIX_NAME);
    ASSERT_EQ(fix.description_, OVERRIDE_FIX_DESCRIPTION);
    ASSERT_EQ(fix.changes_.size(), 1U);
    ASSERT_EQ(fix.changes_[0].textChanges.size(), 1U);
    const auto &change = fix.changes_[0].textChanges[0];
    EXPECT_EQ(change.span.start, overridePos);
    EXPECT_EQ(change.span.length, overrideWithSpaceLength);
    EXPECT_EQ(change.newText, "");
    EXPECT_EQ(ApplyTextChanges(source, fix.changes_[0].textChanges), R"(
class Animal {
}
class Dog extends Animal {
    public foo() {}
}
)");
}

// Fix all: several override diagnostics in one file must be removed in order and
// the result must keep the line structure intact.
TEST_F(ConvertRemoveCodeFixTests, DISABLED_RemoveOverrideModifierFixAllOrder)
{
    const std::string source = R"(
class Animal {
}
class Dog extends Animal {
    override foo() {}
    override bar() {}
}
)";
    SetCurrentSource(source);
    Initializer initializer = Initializer();
    auto *context =
        initializer.CreateContext("remove_override_modifier_fix_all.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    CodeFixOptions options = CreateCodeFixOptions();
    CombinedCodeActionsInfo combinedFixResult =
        ark::es2panda::lsp::GetCombinedCodeFixImpl(context, OVERRIDE_FIX_NAME.data(), options);
    initializer.DestroyContext(context);

    const size_t firstOverridePos = source.find("override");
    const size_t secondOverridePos = source.find("override", firstOverridePos + 1);
    ASSERT_NE(firstOverridePos, std::string::npos);
    ASSERT_NE(secondOverridePos, std::string::npos);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t overrideWithSpaceLength = 9;

    ASSERT_EQ(combinedFixResult.changes_.size(), 1U);
    const auto &fileChanges = combinedFixResult.changes_[0];
    ASSERT_EQ(fileChanges.textChanges.size(), 2U);
    EXPECT_EQ(fileChanges.textChanges[0].span.start, firstOverridePos);
    EXPECT_EQ(fileChanges.textChanges[0].span.length, overrideWithSpaceLength);
    EXPECT_EQ(fileChanges.textChanges[0].newText, "");
    EXPECT_EQ(fileChanges.textChanges[1].span.start, secondOverridePos);
    EXPECT_EQ(fileChanges.textChanges[1].span.length, overrideWithSpaceLength);
    EXPECT_EQ(fileChanges.textChanges[1].newText, "");
    EXPECT_LT(fileChanges.textChanges[0].span.start, fileChanges.textChanges[1].span.start);
    EXPECT_EQ(ApplyTextChanges(source, fileChanges.textChanges), R"(
class Animal {
}
class Dog extends Animal {
    foo() {}
    bar() {}
}
)");
}

// Remove accidental call parentheses: in a nested call expression only the
// parentheses of the inner non-callable property call are removed, the outer
// call node is not deleted.
TEST_F(ConvertRemoveCodeFixTests, RemoveAccidentalCallParenthesesNestedExpression)
{
    const std::string source = R"(
function identity(n: number): number {
    return n;
}
class Box {
    value: number = 5;
}
const box = new Box();
const result = identity(box.value());
)";
    SetCurrentSource(source);
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("remove_accidental_call_parentheses_nested.ets", ES2PANDA_STATE_CHECKED,
                                              source.c_str());
    ASSERT_NE(context, nullptr);

    auto errorCodes = ToErrorCodeVector(PARENS_ERROR_CODES);
    CodeFixOptions options = CreateCodeFixOptions();
    auto fixResult = GetFixesAt(context, "box.value()", errorCodes, options);
    initializer.DestroyContext(context);

    const size_t innerParenPos = source.find('(', source.find("box.value"));
    ASSERT_NE(innerParenPos, std::string::npos);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t emptyParenPairLength = 2;

    ASSERT_EQ(fixResult.size(), 1U);
    const auto &fix = fixResult[0];
    ASSERT_EQ(fix.fixName_, PARENS_FIX_NAME);
    ASSERT_EQ(fix.fixId_, PARENS_FIX_NAME);
    ASSERT_EQ(fix.description_, PARENS_FIX_DESCRIPTION);
    ASSERT_EQ(fix.changes_.size(), 1U);
    ASSERT_EQ(fix.changes_[0].textChanges.size(), 1U);
    const auto &change = fix.changes_[0].textChanges[0];
    EXPECT_EQ(change.span.start, innerParenPos);
    EXPECT_EQ(change.span.length, emptyParenPairLength);
    EXPECT_EQ(change.newText, "");
    EXPECT_EQ(ApplyTextChanges(source, fix.changes_[0].textChanges), R"(
function identity(n: number): number {
    return n;
}
class Box {
    value: number = 5;
}
const box = new Box();
const result = identity(box.value);
)");
}

// Remove accidental call parentheses: only the innermost accidental parentheses
// are removed, the surrounding call chain nodes are preserved.
TEST_F(ConvertRemoveCodeFixTests, DISABLED_RemoveAccidentalCallParenthesesDoesNotDeleteOuterNode)
{
    const std::string source = R"(
class Box {
    value: number = 5;
}
const box = new Box();
const result = (box.value()) + 1;
)";
    SetCurrentSource(source);
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("remove_accidental_call_parentheses_outer.ets", ES2PANDA_STATE_CHECKED,
                                              source.c_str());
    ASSERT_NE(context, nullptr);

    auto errorCodes = ToErrorCodeVector(PARENS_ERROR_CODES);
    CodeFixOptions options = CreateCodeFixOptions();
    auto fixResult = GetFixesAt(context, "box.value()", errorCodes, options);
    initializer.DestroyContext(context);

    const size_t innerParenPos = source.find('(', source.find("box.value"));
    ASSERT_NE(innerParenPos, std::string::npos);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t emptyParenPairLength = 2;

    ASSERT_EQ(fixResult.size(), 1U);
    const auto &fix = fixResult[0];
    ASSERT_EQ(fix.fixName_, PARENS_FIX_NAME);
    ASSERT_EQ(fix.changes_.size(), 1U);
    ASSERT_EQ(fix.changes_[0].textChanges.size(), 1U);
    const auto &change = fix.changes_[0].textChanges[0];
    EXPECT_EQ(change.span.start, innerParenPos);
    EXPECT_EQ(change.span.length, emptyParenPairLength);
    EXPECT_EQ(change.newText, "");
    EXPECT_EQ(ApplyTextChanges(source, fix.changes_[0].textChanges), R"(
class Box {
    value: number = 5;
}
const box = new Box();
const result = (box.value) + 1;
)");
}

// Add missing super call: cursor on a nested expression inside the constructor
// body, the fix must only insert super() and must not remove the statement.
TEST_F(ConvertRemoveCodeFixTests, AddSuperCallFromNestedExpressionKeepsOuterStatements)
{
    const std::string source = R"(
class Animal {
    constructor(public name: string) {}
}
class Dog extends Animal {
    constructor(name: string) {
        console.log(name);
    }
}
)";
    SetCurrentSource(source);
    Initializer initializer = Initializer();
    auto *context =
        initializer.CreateContext("add_super_call_nested_expression.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    auto errorCodes = ToErrorCodeVector(SUPER_ERROR_CODES);
    CodeFixOptions options = CreateCodeFixOptions();
    auto fixResult = GetFixesAt(context, "name);", errorCodes, options);
    initializer.DestroyContext(context);

    ASSERT_EQ(fixResult.size(), 1U);
    const auto &fix = fixResult[0];
    ASSERT_EQ(fix.fixName_, SUPER_FIX_NAME);
    ASSERT_EQ(fix.fixId_, SUPER_FIX_NAME);
    ASSERT_EQ(fix.description_, SUPER_FIX_DESCRIPTION);
    ASSERT_EQ(fix.changes_.size(), 1U);
    ASSERT_EQ(fix.changes_[0].textChanges.size(), 1U);
    const auto &change = fix.changes_[0].textChanges[0];
    EXPECT_EQ(change.span.length, 0U);
    EXPECT_EQ(change.newText, "super()");
    const std::string applied = ApplyTextChanges(source, fix.changes_[0].textChanges);
    EXPECT_NE(applied.find("super()"), std::string::npos);
    EXPECT_NE(applied.find("console.log(name);"), std::string::npos);
}

// Unreachable code: fix all must remove several unreachable blocks in order.
TEST_F(ConvertRemoveCodeFixTests, UnreachableCodeFixAllOrder)
{
    const std::string source = R"(
function first(): void {
    return;
    console.log("a");
}
function second(): void {
    return;
    console.log("b");
}
)";
    SetCurrentSource(source);
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("unreachable_code_fix_all.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    ASSERT_EQ(UNREACHABLE_ERROR_CODES.size(), 1U);
    ASSERT_EQ(UNREACHABLE_ERROR_CODES[0], UNREACHABLE_STMT_CODE);

    CodeFixOptions options = CreateCodeFixOptions();
    CombinedCodeActionsInfo combinedFixResult =
        ark::es2panda::lsp::GetCombinedCodeFixImpl(context, UNREACHABLE_FIX_NAME.data(), options);
    initializer.DestroyContext(context);

    const size_t firstLogPos = source.find("console.log(\"a\")");
    const size_t secondLogPos = source.find("console.log(\"b\")");
    ASSERT_NE(firstLogPos, std::string::npos);
    ASSERT_NE(secondLogPos, std::string::npos);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t consoleLogStatementLength = 17;  // console.log("a"); or console.log("b");

    ASSERT_EQ(combinedFixResult.changes_.size(), 1U);
    const auto &fileChanges = combinedFixResult.changes_[0];
    ASSERT_EQ(fileChanges.textChanges.size(), 2U);
    EXPECT_EQ(fileChanges.textChanges[0].span.start, firstLogPos);
    EXPECT_EQ(fileChanges.textChanges[0].span.length, consoleLogStatementLength);
    EXPECT_EQ(fileChanges.textChanges[0].newText, "");
    EXPECT_EQ(fileChanges.textChanges[1].span.start, secondLogPos);
    EXPECT_EQ(fileChanges.textChanges[1].span.length, consoleLogStatementLength);
    EXPECT_EQ(fileChanges.textChanges[1].newText, "");
    EXPECT_LT(fileChanges.textChanges[0].span.start, fileChanges.textChanges[1].span.start);
}

}  // namespace
