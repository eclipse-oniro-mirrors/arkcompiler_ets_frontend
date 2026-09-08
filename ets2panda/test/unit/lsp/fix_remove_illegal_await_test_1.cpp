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

#include <algorithm>
#include <gtest/gtest.h>

#include "lsp/include/cancellation_token.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/register_code_fix/fix_remove_illegal_await.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_REMOVE_ILLEGAL_AWAIT;
using ark::es2panda::lsp::codefixes::REMOVE_ILLEGAL_AWAIT_KEYWORD;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_REMOVE_ILLEGAL_AWAIT.GetFixId();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Add async modifier to containing function";
constexpr std::string_view EXPECTED_REMOVE_AWAIT_FIX_NAME = REMOVE_ILLEGAL_AWAIT_KEYWORD.GetFixId();
constexpr std::string_view EXPECTED_REMOVE_AWAIT_FIX_DESCRIPTION = "Remove illegal 'await' keyword";
// AWAIT_IN_NON_ASYNC_DEPRECATED: DiagnosticType::SEMANTIC * DIAGNOSTIC_CODE_MULTIPLIER + 173979
constexpr int AWAIT_IN_NON_ASYNC_DEPRECATED_CODE = 175979;
constexpr std::string_view ASYNC_KEYWORD_NEW_TEXT = "async ";
constexpr std::string_view AWAIT_KEYWORD_TEXT = "await";
// WrapReturnTypeInPromise replaces only the type-name node; the ": " separator stays.
constexpr std::string_view ORIGINAL_RETURN_TYPE_TEXT = "void";
constexpr std::string_view PROMISE_VOID_TYPE_TEXT = "Promise<void>";
constexpr int DEFAULT_THROTTLE = 20;

// Function-local instances are created inside each test body: no static-storage object
// carries dynamic initialization (cert-err58-cpp / fuchsia-statically-constructed-objects),
// and the stack lifetime covers every CancellationToken built from it.
class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
public:
    bool IsCancellationRequested() override
    {
        return false;
    }
};

class FixRemoveIllegalAwaitFallbackTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken(
        ark::es2panda::lsp::HostCancellationToken *host)
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, host);
    }

    static std::string ApplyTextChanges(std::string text, std::vector<TextChange> changes)
    {
        std::sort(changes.begin(), changes.end(),
                  [](const TextChange &left, const TextChange &right) { return left.span.start > right.span.start; });
        for (const auto &change : changes) {
            text.replace(change.span.start, change.span.length, change.newText);
        }
        return text;
    }

    static void ExpectAsyncInsertAndPromiseReturnType(const std::string &fileContent,
                                                      const std::vector<CodeFixActionInfo> &fixResult,
                                                      const std::string &expectedFileName, size_t asyncInsertPos,
                                                      size_t awaitPos)
    {
        ASSERT_EQ(fixResult.size(), 2U);

        // The first action adds the async modifier and wraps the return type in Promise.
        const auto &result = fixResult[0];
        EXPECT_EQ(result.fixName_, EXPECTED_FIX_NAME);
        EXPECT_EQ(result.fixId_, EXPECTED_FIX_NAME);
        EXPECT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(result.changes_.size(), 1U);
        EXPECT_EQ(result.changes_[0].fileName, expectedFileName);
        ASSERT_EQ(result.changes_[0].textChanges.size(), 2U);

        // Ascending span order: first the "async " insertion, then the Promise<void>
        // replacement of the declared return type.
        EXPECT_EQ(result.changes_[0].textChanges[0].span.start, asyncInsertPos);
        EXPECT_EQ(result.changes_[0].textChanges[0].span.length, 0U);
        EXPECT_EQ(result.changes_[0].textChanges[0].newText, ASYNC_KEYWORD_NEW_TEXT);
        const size_t returnTypePos = fileContent.find(ORIGINAL_RETURN_TYPE_TEXT);
        ASSERT_NE(returnTypePos, std::string::npos);
        EXPECT_EQ(result.changes_[0].textChanges[1].span.start, returnTypePos);
        EXPECT_EQ(result.changes_[0].textChanges[1].span.length, ORIGINAL_RETURN_TYPE_TEXT.size());
        EXPECT_EQ(result.changes_[0].textChanges[1].newText, PROMISE_VOID_TYPE_TEXT);

        // The second action removes the illegal 'await' keyword together with the trailing spaces.
        const auto &removeAwaitResult = fixResult[1];
        EXPECT_EQ(removeAwaitResult.fixName_, EXPECTED_REMOVE_AWAIT_FIX_NAME);
        EXPECT_EQ(removeAwaitResult.fixId_, EXPECTED_REMOVE_AWAIT_FIX_NAME);
        EXPECT_EQ(removeAwaitResult.description_, EXPECTED_REMOVE_AWAIT_FIX_DESCRIPTION);
        ASSERT_EQ(removeAwaitResult.changes_.size(), 1U);
        EXPECT_EQ(removeAwaitResult.changes_[0].fileName, expectedFileName);
        ASSERT_EQ(removeAwaitResult.changes_[0].textChanges.size(), 1U);
        EXPECT_EQ(removeAwaitResult.changes_[0].textChanges[0].span.start, awaitPos);
        size_t awaitEnd = awaitPos + AWAIT_KEYWORD_TEXT.size();
        while (awaitEnd < fileContent.size() && fileContent[awaitEnd] == ' ') {
            ++awaitEnd;
        }
        EXPECT_EQ(removeAwaitResult.changes_[0].textChanges[0].span.length, awaitEnd - awaitPos);
        EXPECT_EQ(removeAwaitResult.changes_[0].textChanges[0].newText, "");
    }
};

// Checked-state behavior for a keyword without trailing space: at ES2PANDA_STATE_CHECKED the
// top-level function is owned by the global class as a method definition, so the method branch of
// GetAsyncInsertPosition runs. Its spaced-keyword lookup ("function ") fails for
// "function/*c*/foo", and the fallback inserts "async " directly before the method name.
TEST_F(FixRemoveIllegalAwaitFallbackTests, TestCheckedNoSpaceAfterKeywordInsertsBeforeName)
{
    const std::string noSpaceSource = R"(function/* keep */ foo(): void {
    await Promise.resolve();
}
)";
    NullCancellationToken nullHost;
    std::vector<std::string> fileNames = {"TestCheckedNoSpaceAfterKeywordInsertsBeforeName.ets"};
    std::vector<std::string> fileContents = {noSpaceSource};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = noSpaceSource.find("await");
    ASSERT_NE(start, std::string::npos);
    const size_t methodNamePos = noSpaceSource.find("foo");
    ASSERT_NE(methodNamePos, std::string::npos);

    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(&nullHost), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);
    initializer.DestroyContext(context);

    ExpectAsyncInsertAndPromiseReturnType(noSpaceSource, fixResult, filePaths[0], methodNamePos, start);

    EXPECT_EQ(ApplyTextChanges(noSpaceSource, fixResult[0].changes_[0].textChanges),
              R"(function/* keep */ async foo(): Promise<void> {
    await Promise.resolve();
}
)");
}

// The generic declaration/expression branch of GetAsyncInsertPosition and its StartsWith fallback
// are only enterable through the public fix API before binding/checker rewrites the program shape:
// at CHECKED state top-level functions surface as global-class method definitions, function
// expressions are rejected by the parser (ESY0320 "use arrow functions instead"), nested functions
// are rejected (ESY0135), and arrows return earlier. Quick fixes on not-yet-checked sources are a
// regular editor flow, so this pins the PARSED-state contract: with the spaced-keyword search
// failing ("function/*c*/foo" has no "function "), the StartsWith(source, <start>, "function")
// fallback decides the insert position at the keyword itself.
TEST_F(FixRemoveIllegalAwaitFallbackTests, TestParsedNoSpaceAfterKeywordInsertsAtKeyword)
{
    const std::string noSpaceSource = R"(function/* keep */ foo(): void {
    await Promise.resolve();
}
)";
    NullCancellationToken nullHost;
    std::vector<std::string> fileNames = {"TestParsedNoSpaceAfterKeywordInsertsAtKeyword.ets"};
    std::vector<std::string> fileContents = {noSpaceSource};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    const size_t start = noSpaceSource.find("await");
    ASSERT_NE(start, std::string::npos);
    const size_t functionKeywordPos = noSpaceSource.find("function");
    ASSERT_NE(functionKeywordPos, std::string::npos);

    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(&nullHost), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);
    initializer.DestroyContext(context);

    ExpectAsyncInsertAndPromiseReturnType(noSpaceSource, fixResult, filePaths[0], functionKeywordPos, start);

    EXPECT_EQ(ApplyTextChanges(noSpaceSource, fixResult[0].changes_[0].textChanges),
              R"(async function/* keep */ foo(): Promise<void> {
    await Promise.resolve();
}
)");
}

}  // namespace
