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

#include <algorithm>
#include <optional>

#include "generated/code_fix_register.h"
#include "lsp/include/cancellation_token.h"

namespace {
using ark::es2panda::lsp::Initializer;

constexpr auto ERROR_CODES = {1005};
constexpr int DEFAULT_THROTTLE = 20;

class ImportFixesTest : public LSPAPITests {
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

    static std::optional<CodeFixActionInfo> FindImportFix(const std::vector<CodeFixActionInfo> &fixes)
    {
        auto it = std::find_if(fixes.begin(), fixes.end(),
                               [](const CodeFixActionInfo &fix) { return fix.fixName_ == "ImportFixes"; });
        if (it == fixes.end()) {
            return std::nullopt;
        }
        return *it;
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

TEST_F(ImportFixesTest, MergeIntoExistingNamedImportFromSameModule)
{
    std::vector<std::string> fileNames = {"@kit.ModuleNamed.ets", "@kit.ImportModuleNamed.ets", "MainNamed.ets"};
    std::vector<std::string> fileContents = {
        R"(
export function a(): void {}
export function b(): void {}
)",
        R"(
import { a, b } from './@kit.ModuleNamed'
export { a, b };
)",
        R"(
import { a } from './@kit.ImportModuleNamed';

function useIt(): void {
    b();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::CollectApiCompletionInfo(context);

    const auto unresolvedPos = fileContents[2].find("b();");
    ASSERT_NE(unresolvedPos, std::string::npos);

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, unresolvedPos, unresolvedPos + 1, errorCodes, options);
    auto importFix = FindImportFix(fixes);
    ASSERT_TRUE(importFix.has_value());

    const auto updated = ApplyFirstChange(fileContents[2], importFix.value());
    const std::string expected = R"(
import { a, b } from './@kit.ImportModuleNamed';

function useIt(): void {
    b();
}
)";
    ASSERT_EQ(updated, expected);

    initializer.DestroyContext(context);
}

TEST_F(ImportFixesTest, RewriteDefaultAndAddMissingAsNamedImports)
{
    std::vector<std::string> fileNames = {"@kit.ModuleDefault.ets", "@kit.ImportModuleDefault.ets", "MainDefault.ets"};
    std::vector<std::string> fileContents = {
        R"(
export default class A {}
export function b(): string{
    return "str"
}
)",
        R"(
import A from './@kit.ModuleDefault'
import { b } from './@kit.ModuleDefault'
export { A, b };
)",
        R"(
import A from './@kit.ImportModuleDefault';

function useIt(): void {
    b();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::CollectApiCompletionInfo(context);

    const auto unresolvedPos = fileContents[2].find("b();");
    ASSERT_NE(unresolvedPos, std::string::npos);

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, unresolvedPos, unresolvedPos + 1, errorCodes, options);
    auto importFix = FindImportFix(fixes);
    ASSERT_TRUE(importFix.has_value());

    const auto updated = ApplyFirstChange(fileContents[2], importFix.value());
    const std::string expected = R"(
import A, { b } from './@kit.ImportModuleDefault';

function useIt(): void {
    b();
}
)";
    ASSERT_EQ(updated, expected);

    initializer.DestroyContext(context);
}
}  // namespace
