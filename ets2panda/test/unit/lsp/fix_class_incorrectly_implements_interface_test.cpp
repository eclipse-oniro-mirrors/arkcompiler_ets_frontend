/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "lsp/include/register_code_fix/fix_class_incorrectly_implements_interface.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER;
using ark::es2panda::lsp::codefixes::FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER;

constexpr std::string_view EXPECTED_GETTER_FIX_NAME = FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER.GetFixId();
constexpr std::string_view EXPECTED_SETTER_FIX_NAME = FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER.GetFixId();

constexpr auto GETTER_ERROR_CODES = FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER.GetSupportedCodeNumbers();
constexpr auto SETTER_ERROR_CODES = FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER.GetSupportedCodeNumbers();

constexpr int DEFAULT_THROTTLE = 20;

class FixClassIncorrectlyImplementsInterfaceTests : public LSPAPITests {
public:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    static ark::es2panda::lsp::CancellationToken CreateToken()
    {
        static NullCancellationToken nullToken;
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &nullToken);
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto pos = index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
        return pos;
    }
};

TEST_F(FixClassIncorrectlyImplementsInterfaceTests, TestImplementMissingGetter)
{
    std::vector<std::string> fileNames = {"TestImplementMissingGetter.ets"};
    std::vector<std::string> fileContents = {R"(
interface User {
    get name(): String;
}

class Person implements User {
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 6, 30);
    const size_t length = 1;

    std::vector<int> errorCodes(GETTER_ERROR_CODES.begin(), GETTER_ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    ASSERT_GT(fixResult.size(), 0);
    ASSERT_EQ(fixResult[0].fixName_, EXPECTED_GETTER_FIX_NAME);
    ASSERT_EQ(fixResult[0].fixId_, EXPECTED_GETTER_FIX_NAME);
    ASSERT_EQ(fixResult[0].description_, "Add missing interface getter implementations");
    ASSERT_TRUE(fixResult[0].changes_[0].textChanges[0].newText.find("get name(): String") != std::string::npos);
    ASSERT_TRUE(fixResult[0].changes_[0].textChanges[0].newText.find("return null;") != std::string::npos);

    initializer.DestroyContext(context);
}

TEST_F(FixClassIncorrectlyImplementsInterfaceTests, TestImplementMissingSetter)
{
    std::vector<std::string> fileNames = {"TestImplementMissingSetter.ets"};
    std::vector<std::string> fileContents = {R"(
interface Writable {
    set value(data: String);
}

class Writer implements Writable {
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 6, 30);
    const size_t length = 1;

    std::vector<int> errorCodes(SETTER_ERROR_CODES.begin(), SETTER_ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    ASSERT_GT(fixResult.size(), 0);
    ASSERT_EQ(fixResult[0].fixName_, EXPECTED_SETTER_FIX_NAME);
    ASSERT_EQ(fixResult[0].fixId_, EXPECTED_SETTER_FIX_NAME);
    ASSERT_EQ(fixResult[0].description_, "Add missing interface setter implementations");
    ASSERT_TRUE(fixResult[0].changes_[0].textChanges[0].newText.find("set value(data: String)") != std::string::npos);

    initializer.DestroyContext(context);
}

TEST_F(FixClassIncorrectlyImplementsInterfaceTests, TestCombinedCodeFixAll)
{
    std::vector<std::string> fileNames = {"TestCombinedCodeFixAll.ets"};
    std::vector<std::string> fileContents = {R"(
interface PropertyInterface {
    get name(): String;
    set age(value: Double);
}

class Implementation implements PropertyInterface {
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    CodeFixOptions emptyOptions = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    CombinedCodeActionsInfo combinedFixResult =
        ark::es2panda::lsp::GetCombinedCodeFixImpl(context, EXPECTED_GETTER_FIX_NAME.data(), emptyOptions);

    ASSERT_GT(combinedFixResult.changes_.size(), 0);

    ASSERT_EQ(combinedFixResult.changes_[0].fileName, filePaths[0]);

    initializer.DestroyContext(context);
}

}  // namespace