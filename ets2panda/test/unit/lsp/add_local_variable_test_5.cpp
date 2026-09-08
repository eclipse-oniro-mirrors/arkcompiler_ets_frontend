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

#include "lsp_api_test.h"

#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/add_local_variable.h"
#include "lsp/include/register_code_fix/fix_spelling_for_property.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::ADD_LOCAL_VARIABLE;
using ark::es2panda::lsp::codefixes::ADD_LOCAL_VARIABLE_FOR_CLASS;
using ark::es2panda::lsp::codefixes::FIX_SPELLING_FOR_PROPERTY;

constexpr std::string_view EXPECTED_FUNCTION_FIX_NAME = ADD_LOCAL_VARIABLE.GetFixId();
constexpr std::string_view EXPECTED_CLASS_FIX_NAME = ADD_LOCAL_VARIABLE_FOR_CLASS.GetFixId();
constexpr std::string_view EXPECTED_SPELLING_FIX_NAME = FIX_SPELLING_FOR_PROPERTY.GetFixId();
constexpr auto FUNCTION_ERROR_CODES = ADD_LOCAL_VARIABLE.GetSupportedCodeNumbers();
constexpr auto CLASS_ERROR_CODES = ADD_LOCAL_VARIABLE_FOR_CLASS.GetSupportedCodeNumbers();
constexpr auto SPELLING_ERROR_CODES = FIX_SPELLING_FOR_PROPERTY.GetSupportedCodeNumbers();
constexpr std::string_view EXPECTED_FUNCTION_FIX_DESCRIPTION = "Add local variable declaration";
constexpr int DEFAULT_THROTTLE = 20;

class AddLocalVariableTests2 : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto pos = index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
        return pos;
    }

    static std::vector<CodeFixActionInfo> GetFunctionFixes(es2panda_Context *context, size_t start, size_t length)
    {
        std::vector<int> errorCodes(FUNCTION_ERROR_CODES.begin(), FUNCTION_ERROR_CODES.end());
        CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    }

    static size_t CountFixesWithName(const std::vector<CodeFixActionInfo> &fixes, std::string_view fixName)
    {
        size_t count = 0;
        for (const auto &fix : fixes) {
            if (fix.fixName_ == fixName) {
                count++;
            }
        }
        return count;
    }

    static bool HasFixWithName(const std::vector<CodeFixActionInfo> &fixes, std::string_view fixName)
    {
        return CountFixesWithName(fixes, fixName) > 0;
    }

    static const CodeFixActionInfo &RequireSingleFix(const std::vector<CodeFixActionInfo> &fixes,
                                                     std::string_view fixName)
    {
        std::vector<const CodeFixActionInfo *> matched;
        for (const auto &fix : fixes) {
            if (fix.fixName_ == fixName) {
                matched.push_back(&fix);
            }
        }
        EXPECT_EQ(matched.size(), 1U) << "Expected exactly one fix named " << fixName;
        EXPECT_TRUE(matched[0] != nullptr);
        return *matched[0];
    }

    static void ValidateLocalVariableFix(const CodeFixActionInfo &info, const std::string &expectedFileName,
                                         size_t expectedStart, size_t expectedLength,
                                         const std::string &expectedNewText)
    {
        ASSERT_EQ(info.fixName_, EXPECTED_FUNCTION_FIX_NAME);
        ASSERT_EQ(info.fixId_, EXPECTED_FUNCTION_FIX_NAME);
        ASSERT_EQ(info.description_, EXPECTED_FUNCTION_FIX_DESCRIPTION);
        ASSERT_EQ(info.changes_.size(), 1U);
        ASSERT_EQ(info.changes_[0].fileName, expectedFileName);
        ASSERT_EQ(info.changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.start, expectedStart);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.length, expectedLength);
        ASSERT_EQ(info.changes_[0].textChanges[0].newText, expectedNewText);
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
};

// Test: RHS type inference - null literal falls back to "Object"
TEST_F(AddLocalVariableTests2, InfersObjectTypeFromNullLiteral)
{
    std::vector<std::string> fileNames = {"InfersObjectTypeFromNullLiteral.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    value = null;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 3, 5), 5);
    const auto &fix = RequireSingleFix(fixResult, EXPECTED_FUNCTION_FIX_NAME);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t mainBodyInsertPos = 24;  // offset right after the "function main(): void {" line
    ValidateLocalVariableFix(fix, filePaths[0], mainBodyInsertPos, 0, "  let value: Object;");

    initializer.DestroyContext(context);
}

// Test: RHS type inference - call expression falls back to "Object"
TEST_F(AddLocalVariableTests2, InfersObjectTypeFromCallExpression)
{
    std::vector<std::string> fileNames = {"InfersObjectTypeFromCallExpression.ets"};
    std::vector<std::string> fileContents = {R"(
function compute(): number {
    return 42;
}
function main(): void {
    result = compute();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 6, 5), 6);
    const auto &fix = RequireSingleFix(fixResult, EXPECTED_FUNCTION_FIX_NAME);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t mainBodyInsertPos = 70;  // offset right after the "function main(): void {" line
    ValidateLocalVariableFix(fix, filePaths[0], mainBodyInsertPos, 0, "  let result: Object;");

    initializer.DestroyContext(context);
}

// Test: RHS type inference - numeric binary expression infers "Double"
TEST_F(AddLocalVariableTests2, InfersDoubleTypeFromBinaryExpression)
{
    std::vector<std::string> fileNames = {"InfersDoubleTypeFromBinaryExpression.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    total = 1 + 2;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 3, 5), 5);
    const auto &fix = RequireSingleFix(fixResult, EXPECTED_FUNCTION_FIX_NAME);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t mainBodyInsertPos = 24;  // offset right after the "function main(): void {" line
    ValidateLocalVariableFix(fix, filePaths[0], mainBodyInsertPos, 0, "  let total: Double;");

    initializer.DestroyContext(context);
}

// Test: RHS type inference - a binary expression with a string operand infers "String"
TEST_F(AddLocalVariableTests2, InfersStringTypeFromBinaryExpressionWithStringOperand)
{
    std::vector<std::string> fileNames = {"InfersStringTypeFromBinaryExpressionWithStringOperand.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    let name: string = "x";
    msg = name + "!";
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 4, 5), 3);
    const auto &fix = RequireSingleFix(fixResult, EXPECTED_FUNCTION_FIX_NAME);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t mainBodyInsertPos = 24;  // offset right after the "function main(): void {" line
    ValidateLocalVariableFix(fix, filePaths[0], mainBodyInsertPos, 0, "  let msg: String;");

    initializer.DestroyContext(context);
}

// Test: the fix-all public wrapper (GetCombinedCodeFixImpl) dispatches to
// AddLocalVariable::GetAllCodeActions: a real unresolved-reference diagnostic inserts the
// local variable declaration for the same source shape as the single-point case.
TEST_F(AddLocalVariableTests2, FixAllInsertsLocalVariableDeclaration)
{
    std::vector<std::string> fileNames = {"FixAllInsertsLocalVariableDeclaration.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    foo = 1;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto combined = ark::es2panda::lsp::GetCombinedCodeFixImpl(context, EXPECTED_FUNCTION_FIX_NAME.data(), options);

    ASSERT_EQ(combined.changes_.size(), 1U);
    ASSERT_EQ(combined.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(combined.changes_[0].textChanges.size(), 1U);
    const auto &change = combined.changes_[0].textChanges[0];
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t mainBodyInsertPos = 24;  // offset right after the "function main(): void {" line
    ASSERT_EQ(change.span.start, mainBodyInsertPos);
    ASSERT_EQ(change.span.length, 0U);
    ASSERT_EQ(change.newText, "  let foo: Double;");

    initializer.DestroyContext(context);
}

// Test: AddLocalVariableForClass and FixSpellingForProperty share PROPERTY_NONEXISTENT(87);
// a misspelled property read must return both fixes without polluting each other
TEST_F(AddLocalVariableTests2, SharedErrorCodeReturnsIndependentFixSets)
{
    std::vector<std::string> fileNames = {"SharedErrorCodeReturnsIndependentFixSets.ets"};
    std::vector<std::string> fileContents = {R"(
class MyClass {
    myField: number = 0;
}
function use(): void {
    let obj = new MyClass();
    obj.myFiel;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 7, 9);
    const size_t length = 6;

    // Both providers register PROPERTY_NONEXISTENT(87) and share the same error code list
    ASSERT_EQ(CLASS_ERROR_CODES.size(), SPELLING_ERROR_CODES.size());
    ASSERT_EQ(CLASS_ERROR_CODES.size(), 1U);
    ASSERT_EQ(CLASS_ERROR_CODES[0], SPELLING_ERROR_CODES[0]);

    std::vector<int> errorCodes(CLASS_ERROR_CODES.begin(), CLASS_ERROR_CODES.end());
    errorCodes.insert(errorCodes.end(), SPELLING_ERROR_CODES.begin(), SPELLING_ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    // FixSpellingForProperty suggests the correct member name.
    // The provider runs once per requested code (87 passed twice), so the fix is duplicated.
    ASSERT_EQ(CountFixesWithName(fixResult, EXPECTED_SPELLING_FIX_NAME), 2U);
    for (const auto &spellingFix : fixResult) {
        if (spellingFix.fixName_ == EXPECTED_SPELLING_FIX_NAME) {
            ASSERT_EQ(spellingFix.fixId_, EXPECTED_SPELLING_FIX_NAME);
            ASSERT_EQ(spellingFix.description_, "Did you mean 'myField'?");
            ASSERT_EQ(spellingFix.changes_[0].fileName, filePaths[0]);
            ASSERT_EQ(spellingFix.changes_[0].textChanges.size(), 1U);
            ASSERT_EQ(spellingFix.changes_[0].textChanges[0].newText, "myField");
        }
    }

    // AddLocalVariableForClass does not fire on a property read (no assignment), even under the shared code
    ASSERT_FALSE(HasFixWithName(fixResult, EXPECTED_CLASS_FIX_NAME));

    initializer.DestroyContext(context);
}
}  // namespace
