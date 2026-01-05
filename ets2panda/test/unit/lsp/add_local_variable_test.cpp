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

class AddLocalVariableTests : public LSPAPITests {
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

    struct ExpectedCodeFixInfo {
        size_t textChangeStart;
        size_t textChangeLength;
        std::string fileName;
        std::string newText;
        std::string_view fixName;
        std::string_view description;
    };

    static void ValidateCodeFixActionInfo(const CodeFixActionInfo &info, const ExpectedCodeFixInfo &expected)
    {
        ASSERT_EQ(info.fixName_, expected.fixName);
        ASSERT_EQ(info.fixId_, expected.fixName);
        ASSERT_EQ(info.description_, expected.description);
        ASSERT_EQ(info.changes_[0].fileName, expected.fileName);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.start, expected.textChangeStart);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.length, expected.textChangeLength);
        ASSERT_EQ(info.changes_[0].textChanges[0].newText, expected.newText);
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

TEST_F(AddLocalVariableTests, TestAddLocalVariableInFunction)
{
    std::vector<std::string> fileNames = {"TestAddLocalVariableInFunction.ets"};
    std::vector<std::string> fileContents = {R"(
function calculate() {
    result = 10 + 5;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 3, 5);
    const size_t length = 6;
    const size_t expectedTextChangeStart = 23;
    const size_t expectedTextChangeLength = 0;
    const std::string expectedNewText = "  let result: Double;";
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(FUNCTION_ERROR_CODES.begin(), FUNCTION_ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ExpectedCodeFixInfo expected = {
        expectedTextChangeStart, expectedTextChangeLength,   filePaths[0],
        expectedNewText,         EXPECTED_FUNCTION_FIX_NAME, EXPECTED_FUNCTION_FIX_DESCRIPTION};
    ValidateCodeFixActionInfo(fixResult[0], expected);

    initializer.DestroyContext(context);
}

TEST_F(AddLocalVariableTests, TestAddLocalVariableInEmptyClass)
{
    std::vector<std::string> fileNames = {"TestAddLocalVariableInEmptyClass.ets"};
    std::vector<std::string> fileContents = {R"(
class MyComponent {
   build() {
        this.title = "Hello";
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 4, 14);
    const size_t length = 5;
    const size_t expectedTextChangeStart = 20;
    const size_t expectedTextChangeLength = 0;
    const std::string expectedNewText = "  title: String;";
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(CLASS_ERROR_CODES.begin(), CLASS_ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ExpectedCodeFixInfo expected = {expectedTextChangeStart, expectedTextChangeLength, filePaths[0],
                                    expectedNewText,         EXPECTED_CLASS_FIX_NAME,  EXPECTED_CLASS_FIX_DESCRIPTION};
    ValidateCodeFixActionInfo(fixResult[0], expected);

    initializer.DestroyContext(context);
}

TEST_F(AddLocalVariableTests, TestAddLocalVariableInClassWithMembers)
{
    std::vector<std::string> fileNames = {"TestAddLocalVariableInClassWithMembers.ets"};
    std::vector<std::string> fileContents = {R"(
class MyComponent {
    name: String = "test";
    age: Double = 25;
     
    constructor() {
    }
     
    build() {
        this.title = "Hello";
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 10, 14);
    const size_t length = 5;
    const size_t expectedTextChangeStart = 20;
    const size_t expectedTextChangeLength = 0;
    const std::string expectedNewText = "  title: String;";
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(CLASS_ERROR_CODES.begin(), CLASS_ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ExpectedCodeFixInfo expected = {expectedTextChangeStart, expectedTextChangeLength, filePaths[0],
                                    expectedNewText,         EXPECTED_CLASS_FIX_NAME,  EXPECTED_CLASS_FIX_DESCRIPTION};
    ValidateCodeFixActionInfo(fixResult[0], expected);

    initializer.DestroyContext(context);
}

TEST_F(AddLocalVariableTests, TestAddMultipleLocalVariables)
{
    std::vector<std::string> fileNames = {"TestAddMultipleLocalVariables.ets"};
    std::vector<std::string> fileContents = {R"(
function process() {
    count = 5;
    message = "done";
    return count + message.length;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start1 = LineColToPos(context, 3, 5);
    const size_t length1 = 5;
    const size_t start2 = LineColToPos(context, 4, 5);
    const size_t length2 = 7;
    const size_t expectedTextChangeStart1 = 21;
    const size_t expectedTextChangeLength1 = 0;
    const std::string expectedNewText1 = "  let count: Double;";
    const size_t expectedTextChangeStart2 = 21;
    const size_t expectedTextChangeLength2 = 0;
    const std::string expectedNewText2 = "  let message: String;";
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(FUNCTION_ERROR_CODES.begin(), FUNCTION_ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult1 =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start1, start1 + length1, errorCodes, emptyOptions);
    auto fixResult2 =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start2, start2 + length2, errorCodes, emptyOptions);

    ASSERT_EQ(fixResult1.size(), expectedFixResultSize);
    ASSERT_EQ(fixResult2.size(), expectedFixResultSize);

    ExpectedCodeFixInfo expected1 = {
        expectedTextChangeStart1, expectedTextChangeLength1,  filePaths[0],
        expectedNewText1,         EXPECTED_FUNCTION_FIX_NAME, EXPECTED_FUNCTION_FIX_DESCRIPTION};
    ExpectedCodeFixInfo expected2 = {
        expectedTextChangeStart2, expectedTextChangeLength2,  filePaths[0],
        expectedNewText2,         EXPECTED_FUNCTION_FIX_NAME, EXPECTED_FUNCTION_FIX_DESCRIPTION};
    ValidateCodeFixActionInfo(fixResult1[0], expected1);
    ValidateCodeFixActionInfo(fixResult2[0], expected2);

    initializer.DestroyContext(context);
}

TEST_F(AddLocalVariableTests, TestAddLocalForSpecialCharacters)
{
    std::vector<std::string> fileNames = {"TestAddLocalForSpecialCharacters.ets"};
    std::vector<std::string> fileContents = {R"(
//中文测试
class MyComponent {
    name: String = "中文测试";
    age: Double = 25;
     
    constructor() {
    }
     
    build() {
        //中文测试
        this.title = "中文测试";
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = 157;
    const size_t length = 5;
    const size_t expectedTextChangeStart = 27;
    const size_t expectedTextChangeLength = 0;
    const std::string expectedNewText = "  title: String;";
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(CLASS_ERROR_CODES.begin(), CLASS_ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    LSPAPI const *lspApi = GetImpl();
    auto fixResult = lspApi->getCodeFixesAtPosition(context, start, start + length, errorCodes, emptyOptions);

    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ExpectedCodeFixInfo expected = {expectedTextChangeStart, expectedTextChangeLength, filePaths[0],
                                    expectedNewText,         EXPECTED_CLASS_FIX_NAME,  EXPECTED_CLASS_FIX_DESCRIPTION};
    ValidateCodeFixActionInfo(fixResult[0], expected);

    initializer.DestroyContext(context);
}

}  // namespace