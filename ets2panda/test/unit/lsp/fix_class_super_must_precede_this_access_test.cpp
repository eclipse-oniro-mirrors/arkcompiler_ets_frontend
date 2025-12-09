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
#include "lsp/include/register_code_fix/fix_class_super_must_precede_this_access.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS;

constexpr std::string_view EXPECTED_FIX_NAME = CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS.GetFixId();
constexpr auto ERROR_CODES = CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS.GetSupportedCodeNumbers();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Fix 'super' access before 'this'";
constexpr size_t ERROR_LINE = 8;
constexpr size_t ERROR_COLUMN = 9;
constexpr size_t ERROR_LENGTH = 4;
constexpr std::string_view EXPECTED_INSERTED_TEXT = "super(name)";
constexpr size_t EXPECTED_INSERT_POS = 126;
constexpr size_t EXPECTED_INSERT_LENGTH = 0;

constexpr size_t EXPECTED_DELETE_POS = 147;
constexpr size_t EXPECTED_DELETE_LENGTH = 5;
constexpr int DEFAULT_THROTTLE = 20;

class FixClassSuperMustPrecedeThisAccessTests : public LSPAPITests {
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

TEST_F(FixClassSuperMustPrecedeThisAccessTests, TestSuperMustPrecedeThisAccess)
{
    std::vector<std::string> fileNames = {"TestSuperMustPrecedeThisAccess.ets"};
    std::vector<std::string> fileContents = {R"(
class Animal {
    constructor(public name: string) {}
}

class Dog extends Animal {
    constructor(name: string) {
        this.bark();
        super(name);
    }
    bark() { console.log("Woof!"); }
}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    size_t start = LineColToPos(context, ERROR_LINE, ERROR_COLUMN);
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + ERROR_LENGTH, errorCodes, emptyOptions);
    int resSize = 1;
    int changeSize = 2;
    ASSERT_EQ(fixResult.size(), resSize);
    const auto &changes = fixResult[0].changes_[0].textChanges;
    ASSERT_EQ(changes.size(), changeSize);
    auto insertIt = std::find_if(changes.begin(), changes.end(), [](const auto &tc) { return !tc.newText.empty(); });
    auto deleteIt = std::find_if(changes.begin(), changes.end(), [](const auto &tc) { return tc.newText.empty(); });

    ASSERT_NE(insertIt, changes.end());
    ASSERT_NE(deleteIt, changes.end());
    EXPECT_EQ(insertIt->newText, EXPECTED_INSERTED_TEXT);
    EXPECT_EQ(insertIt->span.start, EXPECTED_INSERT_POS);
    EXPECT_EQ(insertIt->span.length, EXPECTED_INSERT_LENGTH);
    EXPECT_EQ(deleteIt->newText, "");
    EXPECT_EQ(deleteIt->span.start, EXPECTED_DELETE_POS);
    EXPECT_EQ(deleteIt->span.length, EXPECTED_DELETE_LENGTH);
    ASSERT_EQ(fixResult[0].fixName_, EXPECTED_FIX_NAME);
    ASSERT_EQ(fixResult[0].description_, EXPECTED_FIX_DESCRIPTION);
    ASSERT_EQ(fixResult[0].changes_[0].fileName, filePaths[0]);

    initializer.DestroyContext(context);
}
}  // namespace