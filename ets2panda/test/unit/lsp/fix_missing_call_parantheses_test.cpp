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
#include "lsp_api_test.h"
#include <algorithm>
#include <string>
#include <vector>
#include "lsp/include/cancellation_token.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/register_code_fix/fix_missing_call_parantheses.h"

namespace {
constexpr int DEFAULT_THROTTLE = 20;
// FixMissingCallParantheses is registered by its production constructor with a
// hard-coded placeholder error code (lsp/src/register_code_fix/
// fix_missing_call_parantheses.cpp). That code is not emitted by the compiler:
// no entry exists in util/diagnostic/{syntax,semantic,warning}.yaml for it, and
// it is absent from the generated code_fix_register.h.
constexpr int STUB_REGISTERED_ERROR_CODE = 1002;
// No code fix registers this code, so the provider lookup must not match.
constexpr int UNREGISTERED_ERROR_CODE = 424242;
constexpr std::string_view FIX_ID = "FixMissingCallParantheses";

class FixMissingCallParanthesesTests : public LSPAPITests {
public:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        static NullCancellationToken nullToken;
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &nullToken);
    }
};

// The production GetCodeActions of FixMissingCallParantheses is a stub that
// always returns an empty action list. This case drives the real provider entry
// (GetCodeFixesAtPositionImpl -> CodeFixProvider::GetFixes) for the error code
// registered by the fix and records that no action is produced.
TEST_F(FixMissingCallParanthesesTests, ProviderEntryReachesRegisteredStubAndReturnsNoActions)
{
    const std::string sourceCode = R"(
function getValue(): number {
    return 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("fix_missing_call_parantheses_entry.ets", ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto supportedCodes = ark::es2panda::lsp::CodeFixProvider::Instance().GetSupportedErrorCodes();
    ASSERT_TRUE(std::find(supportedCodes.begin(), supportedCodes.end(), std::to_string(STUB_REGISTERED_ERROR_CODE)) !=
                supportedCodes.end());

    const auto pos = sourceCode.find("getValue");
    ASSERT_NE(pos, std::string::npos);
    std::vector<int> errorCodes = {STUB_REGISTERED_ERROR_CODE};
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos, errorCodes, emptyOptions);

    EXPECT_TRUE(fixResult.empty());

    initializer.DestroyContext(ctx);
}

// Negative case: an error code with no registered code fix returns no actions.
TEST_F(FixMissingCallParanthesesTests, UnregisteredErrorCodeReturnsNoActions)
{
    const std::string sourceCode = R"(
function getValue(): number {
    return 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("fix_missing_call_parantheses_unregistered.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = sourceCode.find("getValue");
    ASSERT_NE(pos, std::string::npos);
    std::vector<int> errorCodes = {UNREGISTERED_ERROR_CODE};
    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos, errorCodes, emptyOptions);

    EXPECT_TRUE(fixResult.empty());

    initializer.DestroyContext(ctx);
}

// GetAllCodeActions of the fix is also a stub returning an empty result; this
// documents that through the public GetCombinedCodeFixImpl entry.
TEST_F(FixMissingCallParanthesesTests, CombinedFixAllForStubReturnsEmptyChanges)
{
    const std::string sourceCode = R"(
function getValue(): number {
    return 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("fix_missing_call_parantheses_fix_all.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    ASSERT_NE(ctx, nullptr);

    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    CombinedCodeActionsInfo combinedFixResult =
        ark::es2panda::lsp::GetCombinedCodeFixImpl(ctx, FIX_ID.data(), emptyOptions);

    EXPECT_TRUE(combinedFixResult.changes_.empty());
    EXPECT_TRUE(combinedFixResult.commands_.empty());

    initializer.DestroyContext(ctx);
}

}  // namespace
