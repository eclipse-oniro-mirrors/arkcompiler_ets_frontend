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

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <set>
#include "lsp_api_test.h"
#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/completions.h"
#include "lsp/include/quick_info.h"
#include "lsp/include/signature_help.h"
#include "lsp/include/find_rename_locations.h"
#include "lsp/include/inlay_hints.h"
#include "lsp/include/user_preferences.h"
#include "lsp/include/cancellation_token.h"
#include "public/es2panda_lib.h"

namespace {

using ::InlayHintList;
using ::SignatureHelpItems;
using ::TextSpan;
using ark::es2panda::lsp::CancellationToken;
using ark::es2panda::lsp::CompletionEntryKind;
using ark::es2panda::lsp::CompletionInfo;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::RenameLocation;
using ark::es2panda::lsp::SignatureHelpInvokedReason;
using ark::es2panda::lsp::UserPreferences;

// Named constants for CancellationToken IDs
constexpr size_t CANCEL_TOKEN_ID = 123;

class LspTernaryFalseBranchTests : public LSPAPITests {};

// =============================================================================
// P4-TFB-03: SignatureHelp_NonCallExpression_TernaryFalseBranch
// Scenario: Request signature help at a position that is NOT inside a call
// expression (e.g., on a variable). The signature help logic has a ternary:
// `isCall ? callSig : empty`. We test the false branch: empty result.
// =============================================================================
TEST_F(LspTernaryFalseBranchTests, SignatureHelp_NonCallExpression_TernaryFalseBranch)
{
    Initializer initializer = Initializer();
    const std::string source = R"(
function foo(x: number): void {}
let y = 1;
)";
    es2panda_Context *ctx = initializer.CreateContext("tfb03.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    // Position on 'y' (not inside a call expression)
    size_t pos = source.rfind("y");
    ASSERT_NE(pos, std::string::npos);

    SignatureHelpInvokedReason reason;
    CancellationToken token(CANCEL_TOKEN_ID, nullptr);
    auto res = ark::es2panda::lsp::GetSignatureHelpItems(ctx, pos, reason, token);

    // The ternary false branch: not a call expression -> empty
    EXPECT_EQ(res.GetItems().size(), 0U) << "Expected empty signature help for non-call-expression position";

    initializer.DestroyContext(ctx);
}

}  // namespace
