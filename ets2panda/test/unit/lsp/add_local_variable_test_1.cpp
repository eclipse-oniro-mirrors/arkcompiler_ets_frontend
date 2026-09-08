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
constexpr int DEFAULT_THROTTLE = 20;

class AddLocalVariableTests1 : public LSPAPITests {
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

    static bool HasFixWithName(const std::vector<CodeFixActionInfo> &fixes, std::string_view fixName)
    {
        for (const auto &fix : fixes) {
            if (fix.fixName_ == fixName) {
                return true;
            }
        }
        return false;
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

// Test: undefined variable in function should generate AddLocalVariable fix
TEST_F(AddLocalVariableTests1, SuggestsLocalVariableForUndefinedVariableInFunction)
{
    std::vector<std::string> fileNames = {"SuggestsLocalVariableForUndefinedVariableInFunction.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(): void {
    count = 1;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "count" (line 3, col 5)
    const size_t start = LineColToPos(context, 3, 5);
    const size_t length = 5;

    std::vector<int> errorCodes(FUNCTION_ERROR_CODES.begin(), FUNCTION_ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_TRUE(HasFixWithName(fixResult, EXPECTED_FUNCTION_FIX_NAME));

    initializer.DestroyContext(context);
}

// Test: undefined variable in class method should generate AddLocalVariableForClass fix
TEST_F(AddLocalVariableTests1, SuggestsLocalVariableForUndefinedVariableInClassMethod)
{
    std::vector<std::string> fileNames = {"SuggestsLocalVariableForUndefinedVariableInClassMethod.ets"};
    std::vector<std::string> fileContents = {R"(
class MyClass {
    foo(): void {
        count = 1;
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "count" (line 4, col 9)
    const size_t start = LineColToPos(context, 4, 9);
    const size_t length = 5;

    // Use both function and class error codes since the fix may come from either provider
    std::vector<int> errorCodes(FUNCTION_ERROR_CODES.begin(), FUNCTION_ERROR_CODES.end());
    errorCodes.insert(errorCodes.end(), CLASS_ERROR_CODES.begin(), CLASS_ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    // At least one of the two fix providers should return a fix
    bool hasFunctionFix = HasFixWithName(fixResult, EXPECTED_FUNCTION_FIX_NAME);
    bool hasClassFix = HasFixWithName(fixResult, EXPECTED_CLASS_FIX_NAME);
    ASSERT_TRUE(hasFunctionFix || hasClassFix) << "Expected either function or class local variable fix";

    initializer.DestroyContext(context);
}

// Test: compound assignment on undefined variable should generate fix
TEST_F(AddLocalVariableTests1, CompoundAssignmentOnUndefinedVariable)
{
    std::vector<std::string> fileNames = {"CompoundAssignmentOnUndefinedVariable.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(): void {
    count += 1;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "count" in "count += 1" (line 3, col 5)
    const size_t start = LineColToPos(context, 3, 5);
    const size_t length = 5;

    std::vector<int> errorCodes(FUNCTION_ERROR_CODES.begin(), FUNCTION_ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    // count is undefined, AddLocalVariable should be suggested
    ASSERT_TRUE(HasFixWithName(fixResult, EXPECTED_FUNCTION_FIX_NAME));

    initializer.DestroyContext(context);
}

// Test: RHS type inference - string type should be suggested in the fix
TEST_F(AddLocalVariableTests1, InfersStringTypeFromRHS)
{
    std::vector<std::string> fileNames = {"InfersStringTypeFromRHS.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(): void {
    message = "hello world";
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "message" (line 3, col 5)
    const size_t start = LineColToPos(context, 3, 5);
    const size_t length = 7;

    std::vector<int> errorCodes(FUNCTION_ERROR_CODES.begin(), FUNCTION_ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_TRUE(HasFixWithName(fixResult, EXPECTED_FUNCTION_FIX_NAME));

    // Verify the suggested type contains "String" (case-insensitive check)
    for (const auto &fix : fixResult) {
        if (fix.fixName_ == EXPECTED_FUNCTION_FIX_NAME) {
            ASSERT_NE(fix.changes_[0].textChanges[0].newText.find("String"), std::string::npos)
                << "Expected 'String' in suggested type, got: " << fix.changes_[0].textChanges[0].newText;
            break;
        }
    }

    initializer.DestroyContext(context);
}

// Test: RHS type inference - boolean type
TEST_F(AddLocalVariableTests1, InfersBooleanTypeFromRHS)
{
    std::vector<std::string> fileNames = {"InfersBooleanTypeFromRHS.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(): void {
    flag = true;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "flag" (line 3, col 5)
    const size_t start = LineColToPos(context, 3, 5);
    const size_t length = 4;

    std::vector<int> errorCodes(FUNCTION_ERROR_CODES.begin(), FUNCTION_ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_TRUE(HasFixWithName(fixResult, EXPECTED_FUNCTION_FIX_NAME));

    // Verify the suggested type contains "Boolean" (ETS uses capitalized type names)
    for (const auto &fix : fixResult) {
        if (fix.fixName_ == EXPECTED_FUNCTION_FIX_NAME) {
            ASSERT_NE(fix.changes_[0].textChanges[0].newText.find("Boolean"), std::string::npos)
                << "Expected 'Boolean' in suggested type, got: " << fix.changes_[0].textChanges[0].newText;
            break;
        }
    }

    initializer.DestroyContext(context);
}

// Test: RHS type inference - number type
TEST_F(AddLocalVariableTests1, InfersNumberTypeFromRHS)
{
    std::vector<std::string> fileNames = {"InfersNumberTypeFromRHS.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(): void {
    count = 42;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "count" (line 3, col 5)
    const size_t start = LineColToPos(context, 3, 5);
    const size_t length = 5;

    std::vector<int> errorCodes(FUNCTION_ERROR_CODES.begin(), FUNCTION_ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_TRUE(HasFixWithName(fixResult, EXPECTED_FUNCTION_FIX_NAME));

    // Verify the suggested type contains "Double" (ETS uses Double for number type)
    for (const auto &fix : fixResult) {
        if (fix.fixName_ == EXPECTED_FUNCTION_FIX_NAME) {
            ASSERT_NE(fix.changes_[0].textChanges[0].newText.find("Double"), std::string::npos)
                << "Expected 'Double' in suggested type, got: " << fix.changes_[0].textChanges[0].newText;
            break;
        }
    }

    initializer.DestroyContext(context);
}

}  // namespace
