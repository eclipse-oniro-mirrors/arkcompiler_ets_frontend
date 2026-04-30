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

#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_remove_illegal_await.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_REMOVE_ILLEGAL_AWAIT;
using ark::es2panda::util::DiagnosticType;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_REMOVE_ILLEGAL_AWAIT.GetFixId();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Remove illegal 'await' keyword";
constexpr auto ERROR_CODES = FIX_REMOVE_ILLEGAL_AWAIT.GetSupportedCodeNumbers();
// AWAIT_IN_ARROW_FUN_PARAM: DiagnosticType::SYNTAX * DIAGNOSTIC_CODE_MULTIPLIER + 46
constexpr int AWAIT_IN_ARROW_FUN_PARAM_CODE = 1046;
// AWAIT_IN_NON_ASYNC_DEPRECATED: DiagnosticType::SEMANTIC * DIAGNOSTIC_CODE_MULTIPLIER + 173979
constexpr int AWAIT_IN_NON_ASYNC_DEPRECATED_CODE = 175979;
constexpr int DEFAULT_THROTTLE = 20;

class FixRemoveIllegalAwaitTests : public LSPAPITests {
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
        ASSERT_EQ(info.changes_[0].textChanges.size(), 1U);
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

// Test: await in non-async function should suggest removal
TEST_F(FixRemoveIllegalAwaitTests, TestFixRemoveIllegalAwait01)
{
    std::vector<std::string> fileNames = {"TestFixRemoveIllegalAwait01.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(): void {
    let p = new Promise<void>(() => {});
    await p;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "await" keyword (line 4, col 5)
    const size_t start = LineColToPos(context, 4, 5);
    const size_t length = 5;
    const int expectedFixResultSize = 2;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    // Verify target error codes: AWAIT_IN_ARROW_FUN_PARAM(1046), AWAIT_IN_NON_ASYNC_DEPRECATED(175979)
    ASSERT_EQ(errorCodes.size(), 2U);
    ASSERT_EQ(errorCodes[0], AWAIT_IN_ARROW_FUN_PARAM_CODE);
    ASSERT_EQ(errorCodes[1], AWAIT_IN_NON_ASYNC_DEPRECATED_CODE);
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    // Validate all results have correct fixName and description
    for (const auto &result : fixResult) {
        ASSERT_EQ(result.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
        ASSERT_EQ(result.changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(result.changes_[0].textChanges[0].newText, "");
        // The "await" keyword + trailing space should be deleted
        ASSERT_GE(result.changes_[0].textChanges[0].span.length, 5U);
    }

    initializer.DestroyContext(context);
}

// Test: await in non-async function with Promise should suggest removal
TEST_F(FixRemoveIllegalAwaitTests, TestFixRemoveIllegalAwait02)
{
    std::vector<std::string> fileNames = {"TestFixRemoveIllegalAwait02.ets"};
    std::vector<std::string> fileContents = {R"(
function bar(): void {
    await foo();
}
function foo(): Promise<void> { return Promise.resolve(); }
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "await" keyword (line 3, col 5)
    const size_t start = LineColToPos(context, 3, 5);
    const size_t length = 5;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);
    for (const auto &result : fixResult) {
        ASSERT_EQ(result.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
        ASSERT_EQ(result.changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(result.changes_[0].textChanges[0].newText, "");
        ASSERT_GE(result.changes_[0].textChanges[0].span.length, 5U);
    }

    initializer.DestroyContext(context);
}

}  // namespace
