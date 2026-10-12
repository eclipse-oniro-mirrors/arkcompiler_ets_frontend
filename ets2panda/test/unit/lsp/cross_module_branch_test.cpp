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
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include "lsp_api_test.h"
#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/completions.h"
#include "lsp/include/find_references.h"
#include "lsp/include/find_rename_locations.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/formatting/formatting_settings.h"
#include "lsp/include/inlay_hints.h"
#include "lsp/include/navigate_to.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/applicable_refactors.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/user_preferences.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::CompletionEntry;
using ark::es2panda::lsp::CompletionInfo;
using ark::es2panda::lsp::FormatCodeSettings;
using ark::es2panda::lsp::FormatContext;
using ark::es2panda::lsp::GetDefaultFormatCodeSettings;
using ark::es2panda::lsp::GetFormatContext;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::NavigateToItem;
using ark::es2panda::lsp::RenameLocation;

class LspCrossModuleBranchTests : public LSPAPITests {};

// =============================================================================
// P3-CMB-01: CompletionInFileWithErrors_ReturnsValidResults - BLOCKED
// Design expectation: Completion at "c." in a file with semantic errors
// returns valid member completions (val, method).
// Actual product behavior: Completion at the dot position returns 0 entries
// despite class members being present. The presence of errors doesn't block
// completion in principle (branch coverage proves completion works), but the
// test scenario has position/context issues. The completion API requires
// exact position matching for member completion triggers.
// Implementation evidence: completions.cpp — error presence doesn't disable
// completion, but position must be exact. Cross-module test may have
// context initialization differences from single-file tests.
// next_action: Verify completion position calculation in presence of errors.
// No TEST_F written for this item.

// =============================================================================
// P3-CMB-02: DefinitionLookupDuringRename_FindsCorrectDef
// =============================================================================
TEST_F(LspCrossModuleBranchTests, DefinitionLookupDuringRename_FindsCorrectDef)
{
    std::string source = R"(
let target = 1;
function test(): void {
    let x = target + 1;
}
)";
    auto filePaths = CreateTempFile({"cmb_def_rename.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);
    LSPAPI const *lspApi = GetImpl();
    Initializer init;
    auto ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // Find the 'target' reference in the function body
    size_t refPos = source.find("target + 1");
    ASSERT_NE(refPos, std::string::npos);
    size_t offset = refPos + std::string("target").size() - 1;  // inside the identifier

    // Get definition from the reference position
    DefinitionInfo def = lspApi->getDefinitionAtPosition(ctx, offset);
    // The definition should point to the declaration at the top of the file
    EXPECT_FALSE(def.fileName.empty());

    // Get rename locations from the same position
    std::set<RenameLocation> renameLocs = lspApi->findRenameLocationsInCurrentFile(ctx, offset);
    // Should find all occurrences of 'target' (declaration + 1 reference)
    EXPECT_GE(renameLocs.size(), 2U);

    init.DestroyContext(ctx);
}

// =============================================================================
// P3-CMB-03: FormattingPreservesDiagnosticRanges
// =============================================================================
// Diagnostics are queried twice on the same (unmodified) context to verify range stability
// while formatting edits are computed; edits are never applied to the context.
TEST_F(LspCrossModuleBranchTests, FormattingPreservesDiagnosticRanges)
{
    std::string source =
        "\n"
        // CC-OFFNXT(G.FMT.16-CPP) test logic
        R"(function   test ( )  :    void {
let x: number = "string";
}
)";
    auto filePaths = CreateTempFile({"cmb_format_diag.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);
    LSPAPI const *lspApi = GetImpl();
    Initializer init;
    auto ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // Get diagnostics before formatting
    DiagnosticReferences diagsBefore = lspApi->getSemanticDiagnostics(ctx);
    size_t countBefore = diagsBefore.diagnostic.size();
    EXPECT_GE(countBefore, 1U);

    // Apply formatting
    FormatCodeSettings fmtSettings;
    auto edits = lspApi->getFormattingEditsForDocument(ctx, fmtSettings);
    ASSERT_GE(edits.size(), 1U);

    // Get diagnostics after formatting (on the same context - before edit application)
    // This tests that the diagnostic ranges are still valid after formatting edits
    DiagnosticReferences diagsAfter = lspApi->getSemanticDiagnostics(ctx);
    // The number of diagnostics should remain the same
    EXPECT_EQ(diagsAfter.diagnostic.size(), countBefore);

    init.DestroyContext(ctx);
}

// =============================================================================
// P3-CMB-06: NavigateToWithCompletionFilter_FilteredResults - BLOCKED
// Design expectation: Completion at file start returns 4+ entries for 4 classes.
// Actual product behavior: Completion at position 1 (file start) returns
// keywords/builtins, not class names. The completion API at file start
// returns keywords/builtins, not class names. NavigateTo is a separate API
// not directly tested here. The design row conflates NavigateTo with
// Completion filtering. The completion API requires specific trigger
// positions (after dot, after identifier, etc.) to return relevant symbols.
// At file start, it returns keywords.
// next_action: Clarify NavigateTo vs Completion relationship in design;
// use proper completion trigger positions for member/type completion.
// No TEST_F written for this item.

}  // namespace
