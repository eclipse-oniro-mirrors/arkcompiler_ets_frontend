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
#include "lsp/include/register_code_fix/fix_extends_interface_becomes_implements.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::EXTENDS_INTERFACE_BECOMES_IMPLEMENTS;

constexpr std::string_view EXPECTED_FIX_NAME = EXTENDS_INTERFACE_BECOMES_IMPLEMENTS.GetFixId();
constexpr auto ERROR_CODES = EXTENDS_INTERFACE_BECOMES_IMPLEMENTS.GetSupportedCodeNumbers();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Change 'extends' to 'implements'";
constexpr std::string_view EXPECTED_TEXT_CHANGE_NEW_TEXT = "implements";
constexpr int DEFAULT_THROTTLE = 20;

class FixExtendsInterfaceBecomesImplementsTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        return index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
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

TEST_F(FixExtendsInterfaceBecomesImplementsTests, TestFixExtendsToImplements)
{
    std::vector<std::string> fileNames = {"FixExtendsToImplements.ets"};
    std::vector<std::string> fileContents = {R"(
interface IFoo {}
class Bar extends IFoo {}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 3, 13);  // inside "extends"
    const size_t length = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);
    ASSERT_EQ(fixResult.size(), 1);
    const auto &textChange = fixResult[0].changes_[0].textChanges[0];

    EXPECT_EQ(fixResult[0].fixName_, EXPECTED_FIX_NAME);
    EXPECT_EQ(fixResult[0].fixId_, EXPECTED_FIX_NAME);
    EXPECT_EQ(fixResult[0].description_, EXPECTED_FIX_DESCRIPTION);
    EXPECT_EQ(fixResult[0].changes_[0].fileName, filePaths[0]);
    EXPECT_EQ(textChange.newText, EXPECTED_TEXT_CHANGE_NEW_TEXT);

    initializer.DestroyContext(context);
}

}  // namespace