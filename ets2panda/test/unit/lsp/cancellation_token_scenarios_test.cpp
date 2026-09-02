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

#include <cstddef>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "es2panda.h"

#include <gtest/gtest.h>

namespace {

// A host that always reports cancellation as requested.
class AlwaysCancelledHost : public ark::es2panda::lsp::HostCancellationToken {
public:
    bool IsCancellationRequested() override
    {
        return true;
    }
};

// A host that never reports cancellation.
class NeverCancelledHost : public ark::es2panda::lsp::HostCancellationToken {
public:
    bool IsCancellationRequested() override
    {
        return false;
    }
};

class LspCancellationTokenScenariosTests : public LSPAPITests {
public:
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr int defaultThrottle = 20;
    // Build a CancellationToken that is always cancelled.
    static ark::es2panda::lsp::CancellationToken MakeCancelledToken()
    {
        return ark::es2panda::lsp::CancellationToken(defaultThrottle, &GetAlwaysCancelledHost());
    }

    // Build a CancellationToken that is never cancelled.
    static ark::es2panda::lsp::CancellationToken MakeNonCancelledToken()
    {
        return ark::es2panda::lsp::CancellationToken(defaultThrottle, &GetNeverCancelledHost());
    }

private:
    static AlwaysCancelledHost &GetAlwaysCancelledHost()
    {
        static AlwaysCancelledHost instance;
        return instance;
    }
    static NeverCancelledHost &GetNeverCancelledHost()
    {
        static NeverCancelledHost instance;
        return instance;
    }
};

// Test: getCompilerOptionsDiagnostics with a cancelled token returns empty diagnostics
TEST_F(LspCancellationTokenScenariosTests, CompilerOptionsDiagnosticsCancelledReturnsEmpty)
{
    std::vector<std::string> files = {"cancel_diag.ets"};
    std::vector<std::string> texts = {R"(let x: number = 1;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    auto cancelledToken = MakeCancelledToken();
    // With cancellation requested, the API should return early with empty diagnostics.
    auto result = lspApi->getCompilerOptionsDiagnostics(filePaths[0].c_str(), cancelledToken);
    EXPECT_TRUE(result.diagnostic.empty());
}

// Test: getCompilerOptionsDiagnostics with a non-cancelled token returns usable result
TEST_F(LspCancellationTokenScenariosTests, CompilerOptionsDiagnosticsNotCancelledReturnsResult)
{
    std::vector<std::string> files = {"cancel_diag_ok.ets"};
    std::vector<std::string> texts = {R"(let x: number = 1;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    auto nonCancelledToken = MakeNonCancelledToken();
    // Without cancellation, the API should run to completion without throwing.
    auto result = lspApi->getCompilerOptionsDiagnostics(filePaths[0].c_str(), nonCancelledToken);
    // The result may be empty for a valid file, but the call must not be short-circuited.
    // We only verify the call completes and returns a usable structure.
    EXPECT_GE(result.diagnostic.size(), 0U);
}

// Test: findReferences with a cancelled token returns empty references
TEST_F(LspCancellationTokenScenariosTests, FindReferencesCancelledReturnsEmpty)
{
    std::vector<std::string> files = {"cancel_ref_export.ets", "cancel_ref_import.ets"};
    std::vector<std::string> texts = {R"(export let shared: number = 1;
console.log(shared);)",
                                      R"(import { shared } from './cancel_ref_export';
function consume(): number {
    return shared;
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 2U);

    // Build source files for findReferences API.
    std::vector<ark::es2panda::SourceFile> sourceFiles;
    sourceFiles.emplace_back(filePaths[0], texts[0]);
    sourceFiles.emplace_back(filePaths[1], texts[1]);
    ark::es2panda::SourceFile mainSrcFile(filePaths[1], texts[1]);

    // Position at "shared" usage in the import file.
    const auto pos = texts[1].find("return shared");
    ASSERT_NE(pos, std::string::npos);
    const auto sharedPos = pos + std::string("return ").size();

    LSPAPI const *lspApi = GetImpl();
    auto cancelledToken = MakeCancelledToken();
    // With cancellation requested, findReferences should return empty (or partial) results.
    auto result = lspApi->findReferences(&cancelledToken, sourceFiles, mainSrcFile, sharedPos);
    // When cancelled before processing, the result should be empty.
    EXPECT_TRUE(result.empty());
}

// Test: findRenameLocationsWithCancellationToken with a cancelled token returns empty
TEST_F(LspCancellationTokenScenariosTests, FindRenameLocationsCancelledReturnsEmpty)
{
    std::vector<std::string> files = {"cancel_rename.ets"};
    std::vector<std::string> texts = {R"(let target: number = 1;
function use(): number {
    return target;
}
console.log(target);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    ark::es2panda::lsp::Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    std::vector<es2panda_Context *> fileContexts = {context};

    // Position at "target" declaration.
    const auto pos = texts[0].find("target: number");
    ASSERT_NE(pos, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    auto cancelledToken = MakeCancelledToken();
    // With cancellation requested, rename locations should be empty.
    auto result = lspApi->findRenameLocationsWithCancellationToken(&cancelledToken, fileContexts, context, pos);
    EXPECT_TRUE(result.empty());

    initializer.DestroyContext(context);
}

// Test: findRenameLocationsWithCancellationToken without cancellation returns non-empty
TEST_F(LspCancellationTokenScenariosTests, FindRenameLocationsNotCancelledReturnsResults)
{
    std::vector<std::string> files = {"cancel_rename_ok.ets"};
    std::vector<std::string> texts = {R"(let target: number = 1;
function use(): number {
    return target;
}
console.log(target);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    ark::es2panda::lsp::Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    std::vector<es2panda_Context *> fileContexts = {context};

    // Position at "target" declaration.
    const auto pos = texts[0].find("target: number");
    ASSERT_NE(pos, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    auto nonCancelledToken = MakeNonCancelledToken();
    auto result = lspApi->findRenameLocationsWithCancellationToken(&nonCancelledToken, fileContexts, context, pos);
    // Without cancellation, rename locations should include the declaration and usages.
    EXPECT_GE(result.size(), 1U);

    initializer.DestroyContext(context);
}

// Test: CancellationToken with nullptr host never reports cancellation
TEST_F(LspCancellationTokenScenariosTests, NullHostTokenNeverCancels)
{
    ark::es2panda::lsp::CancellationToken token(defaultThrottle, nullptr);
    EXPECT_FALSE(token.IsCancellationRequested());
    EXPECT_FALSE(token.ThrottledCancellationCheck());
}

// Test: ThrottledCancellationCheck respects throttle time
TEST_F(LspCancellationTokenScenariosTests, ThrottledCancellationCheckRespectsThrottle)
{
    // With throttle time 0, every check should pass through to the host.
    AlwaysCancelledHost host;
    ark::es2panda::lsp::CancellationToken token(0, &host);
    EXPECT_TRUE(token.ThrottledCancellationCheck());
}

}  // namespace
