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

#include "generated/code_fix_register.h"
#include "lsp_api_test.h"

#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_remove_override_modifier.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::REMOVE_OVERRIDE_MODIFIER;

constexpr std::string_view EXPECTED_FIX_NAME = REMOVE_OVERRIDE_MODIFIER.GetFixId();
constexpr auto ERROR_CODES = REMOVE_OVERRIDE_MODIFIER.GetSupportedCodeNumbers();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Remove override modifier";
constexpr int DEFAULT_THROTTLE = 20;

class FixRemoveOverrideModifierTests : public LSPAPITests {
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
        ASSERT_EQ(info.changes_[0].textChanges[0].newText, "");
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

TEST_F(FixRemoveOverrideModifierTests, TestFixRemoveOverrideModifier01)
{
    std::vector<std::string> fileNames = {"TestFixRemoveOverrideModifier01.ets"};
    std::vector<std::string> fileContents = {R"(
class Animal {
}
class Dog extends Animal {
    override foo() {}
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 5, 17);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 49;
    const size_t expectedTextChangeLength = 8;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

}  // namespace