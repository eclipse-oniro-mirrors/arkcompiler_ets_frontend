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
#include "lsp/include/register_code_fix/convert_const_to_let.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_CONVERT_CONST_TO_LET;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_CONVERT_CONST_TO_LET.GetFixId();
constexpr auto ERROR_CODES = FIX_CONVERT_CONST_TO_LET.GetSupportedCodeNumbers();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Convert const to let";
constexpr std::string_view EXPECTED_TEXT_CHANGE_NEW_TEXT = "let";
constexpr int DEFAULT_THROTTLE = 20;

class FixConvertConstToLetTests : public LSPAPITests {
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

    static void ValidateCodeFixActionInfo(const CodeFixActionInfo &info, const size_t expectedTextChangeStart,
                                          const size_t expectedTextChangeLength, const std::string &expectedFileName)
    {
        ASSERT_EQ(info.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(info.changes_[0].fileName, expectedFileName);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.start, expectedTextChangeStart);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.length, expectedTextChangeLength);
        ASSERT_EQ(info.changes_[0].textChanges[0].newText, EXPECTED_TEXT_CHANGE_NEW_TEXT);
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

TEST_F(FixConvertConstToLetTests, TestFixConvertConstToLet01)
{
    std::vector<std::string> fileNames = {"TestFixConvertConstToLet01.ets"};
    std::vector<std::string> fileContents = {R"(
const a:Int = 0;
a = 1;
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 3, 1);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 1;
    const size_t expectedTextChangeLength = 5;
    const int expectedFixResultSize = 2;
    const int expectedCombinedFixResultSize = 1;
    const int expectedCombinedTextChangesSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    CombinedCodeActionsInfo combinedFixResult =
        ark::es2panda::lsp::GetCombinedCodeFixImpl(context, EXPECTED_FIX_NAME.data(), emptyOptions);
    ASSERT_EQ(combinedFixResult.changes_.size(), expectedCombinedFixResultSize);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges.size(), expectedCombinedTextChangesSize);
    ASSERT_EQ(combinedFixResult.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[0].span.start, expectedTextChangeStart);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[0].span.length, expectedTextChangeLength);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[0].newText, EXPECTED_TEXT_CHANGE_NEW_TEXT.data());
    initializer.DestroyContext(context);
}

TEST_F(FixConvertConstToLetTests, TestFixConvertConstToLet02)
{
    std::vector<std::string> fileNames = {"TestFixConvertConstToLet02.ets"};
    std::vector<std::string> fileContents = {R"(
const a = 0;
const b = 1;
a = 3;
b = 2;
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start1 = LineColToPos(context, 4, 1);
    const size_t length1 = 1;
    const size_t start2 = LineColToPos(context, 5, 1);
    const size_t length2 = 1;
    const size_t expectedTextChangeStart1 = 1;
    const size_t expectedTextChangeLength1 = 5;
    const size_t expectedTextChangeStart2 = 14;
    const size_t expectedTextChangeLength2 = 5;
    const int expectedFixResultSize = 2;
    const int expectedCombinedFixResultSize = 1;
    const int expectedCombinedTextChangesSize = 2;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult1 =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start1, start1 + length1, errorCodes, emptyOptions);
    auto fixResult2 =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start2, start2 + length2, errorCodes, emptyOptions);
    ASSERT_EQ(fixResult1.size(), expectedFixResultSize);
    ValidateCodeFixActionInfo(fixResult1[0], expectedTextChangeStart1, expectedTextChangeLength1, filePaths[0]);

    ASSERT_EQ(fixResult2.size(), expectedFixResultSize);
    ValidateCodeFixActionInfo(fixResult2[0], expectedTextChangeStart2, expectedTextChangeLength2, filePaths[0]);

    CombinedCodeActionsInfo combinedFixResult =
        ark::es2panda::lsp::GetCombinedCodeFixImpl(context, EXPECTED_FIX_NAME.data(), emptyOptions);
    ASSERT_EQ(combinedFixResult.changes_.size(), expectedCombinedFixResultSize);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges.size(), expectedCombinedTextChangesSize);
    ASSERT_EQ(combinedFixResult.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[0].span.start, expectedTextChangeStart1);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[0].span.length, expectedTextChangeLength1);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[0].newText, EXPECTED_TEXT_CHANGE_NEW_TEXT.data());
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[1].span.start, expectedTextChangeStart2);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[1].span.length, expectedTextChangeLength2);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[1].newText, EXPECTED_TEXT_CHANGE_NEW_TEXT.data());
    initializer.DestroyContext(context);
}

TEST_F(FixConvertConstToLetTests, TestFixConvertConstToLet03)
{
    std::vector<std::string> fileNames = {"TestFixConvertConstToLet03.ets"};
    std::vector<std::string> fileContents = {R"(
function f() {
const a = 1;
a = 2;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 4, 1);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 16;
    const size_t expectedTextChangeLength = 5;
    const int expectedFixResultSize = 2;
    const int expectedCombinedFixResultSize = 1;
    const int expectedCombinedTextChangesSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);
    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    CombinedCodeActionsInfo combinedFixResult =
        ark::es2panda::lsp::GetCombinedCodeFixImpl(context, EXPECTED_FIX_NAME.data(), emptyOptions);
    ASSERT_EQ(combinedFixResult.changes_.size(), expectedCombinedFixResultSize);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges.size(), expectedCombinedTextChangesSize);
    ASSERT_EQ(combinedFixResult.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[0].span.start, expectedTextChangeStart);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[0].span.length, expectedTextChangeLength);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[0].newText, EXPECTED_TEXT_CHANGE_NEW_TEXT.data());
    initializer.DestroyContext(context);
}

}  // namespace
