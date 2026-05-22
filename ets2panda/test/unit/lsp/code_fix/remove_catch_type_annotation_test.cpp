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

#include "gtest/gtest.h"
#include "../lsp_api_test.h"

#include "generated/code_fix_register.h"
#include "lsp/include/cancellation_token.h"

namespace {
using ark::es2panda::lsp::Initializer;

constexpr int DEFAULT_THROTTLE = 20;
constexpr auto ERROR_CODES = ark::es2panda::lsp::codefixes::REMOVE_CATCH_TYPE_ANNOTATION.GetSupportedCodeNumbers();

class RemoveCatchTypeAnnotationTest : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static std::string ApplyFirstChange(const std::string &source, const CodeFixActionInfo &action)
    {
        EXPECT_FALSE(action.changes_.empty());
        EXPECT_FALSE(action.changes_[0].textChanges.empty());
        const auto &change = action.changes_[0].textChanges[0];
        return source.substr(0, change.span.start) + change.newText +
               source.substr(change.span.start + change.span.length);
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

TEST_F(RemoveCatchTypeAnnotationTest, RemoveCatchTypeAnnotationBasic)
{
    std::vector<std::string> fileNames = {"RemoveCatchTypeAnnotation.ets"};
    std::vector<std::string> fileContents = {R"(
function test(): void {
    try {
        doSomething();
    } catch (e: Error) {
        console.log(e);
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const auto errorPos = fileContents[0].find("Error");
    ASSERT_NE(errorPos, std::string::npos);

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, errorPos, errorPos + 1, errorCodes, options);
    ASSERT_FALSE(fixes.empty());

    const auto updated = ApplyFirstChange(fileContents[0], fixes[0]);
    const std::string expected = R"(
function test(): void {
    try {
        doSomething();
    } catch (e) {
        console.log(e);
    }
}
)";
    ASSERT_EQ(updated, expected);

    initializer.DestroyContext(context);
}

TEST_F(RemoveCatchTypeAnnotationTest, RemoveCatchTypeAnnotationWithCustomType)
{
    std::vector<std::string> fileNames = {"RemoveCatchTypeAnnotationCustom.ets"};
    std::vector<std::string> fileContents = {R"(
function test(): void {
    try {
        doSomething();
    } catch (e: MyCustomError) {
        console.log(e);
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const auto errorPos = fileContents[0].find("MyCustomError");
    ASSERT_NE(errorPos, std::string::npos);

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, errorPos, errorPos + 1, errorCodes, options);
    ASSERT_FALSE(fixes.empty());

    const auto updated = ApplyFirstChange(fileContents[0], fixes[0]);
    const std::string expected = R"(
function test(): void {
    try {
        doSomething();
    } catch (e) {
        console.log(e);
    }
}
)";
    ASSERT_EQ(updated, expected);

    initializer.DestroyContext(context);
}
}  // namespace
