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
#include "lsp/include/symbol_reference_index.h"

namespace {
using ark::es2panda::lsp::Initializer;

constexpr auto ERROR_CODES = ark::es2panda::lsp::codefixes::FIX_IMPORT_SOURCE.GetSupportedCodeNumbers();
constexpr int DEFAULT_THROTTLE = 20;

class FixImportSourceTest1 : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static std::optional<CodeFixActionInfo> FindFixImportSource(const std::vector<CodeFixActionInfo> &fixes)
    {
        auto it = std::find_if(fixes.begin(), fixes.end(),
                               [](const CodeFixActionInfo &fix) { return fix.fixName_ == "FixImportSource"; });
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

// Test: the fix-all public wrapper (GetCombinedCodeFixImpl) dispatches to
// FixImportSource::GetAllCodeActions, which is currently an empty stub: even with a real
// unresolved-reference diagnostic the combined result carries no edits.
TEST_F(FixImportSourceTest1, FixAllEntryIsStubAndReturnsNoChanges)
{
    std::vector<std::string> fileNames = {"fix_all_consumer.ets"};
    std::vector<std::string> fileContents = {
        R"(function main(): void {
    let r = unknownSymbol(5);
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto combined = ark::es2panda::lsp::GetCombinedCodeFixImpl(
        context, ark::es2panda::lsp::codefixes::FIX_IMPORT_SOURCE.GetFixId().data(), options);

    // Current stub behavior: the fix-all entry produces no edits.
    ASSERT_TRUE(combined.changes_.empty());

    initializer.DestroyContext(context);
}

// Test: no fix should be offered when there is no indexed source for the unresolved symbol
TEST_F(FixImportSourceTest1, NoFixWhenNoIndexedSourceAvailable)
{
    std::vector<std::string> fileNames = {"no_source_consumer.ets"};
    std::vector<std::string> fileContents = {
        R"(function main(): void {
    let r = unknownSymbol(5);
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    // Do NOT index any export file - no source available for unknownSymbol
    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const auto unresolvedPos = fileContents[0].find("unknownSymbol(5)");
    ASSERT_NE(unresolvedPos, std::string::npos);

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, unresolvedPos, unresolvedPos + 1, errorCodes, options);
    auto importSourceFix = FindFixImportSource(fixes);
    // No indexed source contains unknownSymbol, so no FixImportSource should be offered
    ASSERT_FALSE(importSourceFix.has_value());

    initializer.DestroyContext(context);
}
}  // namespace
