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

    static std::string ApplyFirstChange(const std::string &source, const CodeFixActionInfo &action)
    {
        EXPECT_FALSE(action.changes_.empty());
        EXPECT_FALSE(action.changes_[0].textChanges.empty());
        const auto &change = action.changes_[0].textChanges[0];
        return source.substr(0, change.span.start) + change.newText +
               source.substr(change.span.start + change.span.length);
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

// Test: barrel file re-exporting a function should be discoverable as an import source
TEST_F(FixImportSourceTest1, BarrelReExportFunctionAsImportSource)
{
    std::vector<std::string> fileNames = {"func_origin.ets", "func_barrel.ets", "func_consumer.ets"};
    std::vector<std::string> fileContents = {
        R"(export function compute(x: number): number {
    return x * 2;
})",
        R"(export { compute } from './func_origin';)",
        R"(function main(): void {
    let r = compute(5);
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    // Index the origin and barrel files
    {
        Initializer indexInitializer;
        auto *originCtx = indexInitializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
        ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(originCtx);
        indexInitializer.DestroyContext(originCtx);
    }
    {
        Initializer indexInitializer;
        auto *barrelCtx = indexInitializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
        ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(barrelCtx);
        indexInitializer.DestroyContext(barrelCtx);
    }

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);

    const auto unresolvedPos = fileContents[2].find("compute(5)");
    ASSERT_NE(unresolvedPos, std::string::npos);

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, unresolvedPos, unresolvedPos + 1, errorCodes, options);
    auto importSourceFix = FindFixImportSource(fixes);
    ASSERT_TRUE(importSourceFix.has_value());

    const auto updated = ApplyFirstChange(fileContents[2], importSourceFix.value());
    ASSERT_NE(updated.find("import"), std::string::npos);
    ASSERT_NE(updated.find("compute"), std::string::npos);

    initializer.DestroyContext(context);
}
}  // namespace
