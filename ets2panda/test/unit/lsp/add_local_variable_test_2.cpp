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

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::ADD_LOCAL_VARIABLE;
using ark::es2panda::lsp::codefixes::ADD_LOCAL_VARIABLE_FOR_CLASS;

constexpr std::string_view EXPECTED_FUNCTION_FIX_NAME = ADD_LOCAL_VARIABLE.GetFixId();
constexpr std::string_view EXPECTED_CLASS_FIX_NAME = ADD_LOCAL_VARIABLE_FOR_CLASS.GetFixId();
constexpr auto FUNCTION_ERROR_CODES = ADD_LOCAL_VARIABLE.GetSupportedCodeNumbers();
constexpr auto CLASS_ERROR_CODES = ADD_LOCAL_VARIABLE_FOR_CLASS.GetSupportedCodeNumbers();
constexpr std::string_view EXPECTED_FUNCTION_FIX_DESCRIPTION = "Add local variable declaration";
constexpr std::string_view EXPECTED_CLASS_FIX_DESCRIPTION = "Add class field declaration";
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

    static std::vector<CodeFixActionInfo> GetClassFixes(es2panda_Context *context, size_t start, size_t length)
    {
        std::vector<int> errorCodes(CLASS_ERROR_CODES.begin(), CLASS_ERROR_CODES.end());
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

    static void ValidateClassFieldFix(const CodeFixActionInfo &info, const std::string &expectedFileName,
                                      size_t expectedStart, size_t expectedLength, const std::string &expectedNewText)
    {
        ASSERT_EQ(info.fixName_, EXPECTED_CLASS_FIX_NAME);
        ASSERT_EQ(info.fixId_, EXPECTED_CLASS_FIX_NAME);
        ASSERT_EQ(info.description_, EXPECTED_CLASS_FIX_DESCRIPTION);
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

// Test: "foo = 1" in a function body should suggest "let foo: Double;" with exact position and text
TEST_F(AddLocalVariableTests2, GeneratesLetDeclarationForSimpleAssignment)
{
    std::vector<std::string> fileNames = {"GeneratesLetDeclarationForSimpleAssignment.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    foo = 1;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 3, 5);
    const size_t length = 3;

    auto fixResult = GetFunctionFixes(context, start, length);
    const auto &fix = RequireSingleFix(fixResult, EXPECTED_FUNCTION_FIX_NAME);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t mainBodyInsertPos = 24;  // offset right after the "function main(): void {" line
    ValidateLocalVariableFix(fix, filePaths[0], mainBodyInsertPos, 0, "  let foo: Double;");

    initializer.DestroyContext(context);
}

// Test: insertion point is the enclosing block/function body for nested blocks and class methods
TEST_F(AddLocalVariableTests2, InsertionPointInBlockAndClassMethod)
{
    std::vector<std::string> fileNames = {"InsertionPointInBlockAndClassMethod.ets"};
    std::vector<std::string> fileContents = {R"(
function outer(): void {
    if (true) {
        inner = 1;
    }
}

class Worker {
    doWork(): void {
        count = 2;
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // "inner" in the nested if-block: insertion happens right after the if-block opening brace
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t ifBlockInsertPos = 41;  // offset right after the "if (true) {" line
    // NOLINTNEXTLINE(readability-magic-numbers)
    auto fixResult1 = GetFunctionFixes(context, LineColToPos(context, 4, 9), 5);
    const auto &fix1 = RequireSingleFix(fixResult1, EXPECTED_FUNCTION_FIX_NAME);
    ValidateLocalVariableFix(fix1, filePaths[0], ifBlockInsertPos, 0, "  let inner: Double;");

    // "count" in the class method body: insertion happens at the start of the first statement,
    // because the insertion offset found after "class Worker {" overlaps the existing whitespace
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t methodBodyInsertPos = 105;  // offset right after the "class Worker {" line
    // NOLINTNEXTLINE(readability-magic-numbers)
    auto fixResult2 = GetFunctionFixes(context, LineColToPos(context, 10, 9), 5);
    const auto &fix2 = RequireSingleFix(fixResult2, EXPECTED_FUNCTION_FIX_NAME);
    ValidateLocalVariableFix(fix2, filePaths[0], methodBodyInsertPos, 0, "  let count: Double;");

    initializer.DestroyContext(context);
}

// Test: "this.foo += 1" should still generate the class field with type inferred from the RHS literal
TEST_F(AddLocalVariableTests2, CompoundThisAssignmentGeneratesClassField)
{
    std::vector<std::string> fileNames = {"CompoundThisAssignmentGeneratesClassField.ets"};
    std::vector<std::string> fileContents = {R"(
class Bar {
    method(): void {
        this.foo += 1;
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 4, 14);
    const size_t length = 3;

    auto fixResult = GetClassFixes(context, start, length);
    const auto &fix = RequireSingleFix(fixResult, EXPECTED_CLASS_FIX_NAME);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t classFieldInsertPos = 12;  // offset right after the "class Bar {" line
    ValidateClassFieldFix(fix, filePaths[0], classFieldInsertPos, 0, "\n  foo: number;");

    initializer.DestroyContext(context);
}

// Test: "obj.foo = 1" must not generate a local variable declaration for "foo"
TEST_F(AddLocalVariableTests2, ObjPropertyAssignmentDoesNotGenerateLocalVariable)
{
    std::vector<std::string> fileNames = {"ObjPropertyAssignmentDoesNotGenerateLocalVariable.ets"};
    std::vector<std::string> fileContents = {R"(
class MyClass {
    myField: number = 0;
}
function use(): void {
    let obj = new MyClass();
    obj.foo = 1;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 7, 9);
    const size_t length = 3;

    std::vector<int> errorCodes(FUNCTION_ERROR_CODES.begin(), FUNCTION_ERROR_CODES.end());
    errorCodes.insert(errorCodes.end(), CLASS_ERROR_CODES.begin(), CLASS_ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    // No "let foo" local variable fix for a member expression property
    ASSERT_FALSE(HasFixWithName(fixResult, EXPECTED_FUNCTION_FIX_NAME));
    // A non-this property assignment on a class of the same file gets a class field fix instead.
    // The provider is invoked once per requested code (87 passed twice), so the fix is duplicated.
    ASSERT_EQ(CountFixesWithName(fixResult, EXPECTED_CLASS_FIX_NAME), 2U);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t classFieldInsertPos = 16;  // offset right after the "class MyClass {" line
    for (const auto &fix : fixResult) {
        if (fix.fixName_ == EXPECTED_CLASS_FIX_NAME) {
            ValidateClassFieldFix(fix, filePaths[0], classFieldInsertPos, 0, "\n  foo: number;");
        }
    }

    initializer.DestroyContext(context);
}

// Test: RHS type inference - new expression falls back to "Object"
TEST_F(AddLocalVariableTests2, InfersObjectTypeFromNewExpression)
{
    std::vector<std::string> fileNames = {"InfersObjectTypeFromNewExpression.ets"};
    std::vector<std::string> fileContents = {R"(
class MyClass {}
function main(): void {
    inst = new MyClass();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 4, 5), 4);
    const auto &fix = RequireSingleFix(fixResult, EXPECTED_FUNCTION_FIX_NAME);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t mainBodyInsertPos = 41;  // offset right after the "function main(): void {" line
    ValidateLocalVariableFix(fix, filePaths[0], mainBodyInsertPos, 0, "  let inst: Object;");

    initializer.DestroyContext(context);
}
}  // namespace
