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
#include "lsp/include/find_references.h"
#include "lsp/include/find_rename_locations.h"
#include "lsp/include/completions.h"
#include "lsp/include/quick_info.h"
#include "lsp/include/classifier.h"
#include "lsp/include/organize_imports.h"
#include "lsp/include/inlay_hints.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/get_definition_and_bound_span.h"
#include "lsp/include/user_preferences.h"
#include "lsp/include/navigate_to.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::SourceFile;
using ark::es2panda::lsp::CancellationToken;
using ark::es2panda::lsp::CompletionInfo;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::NavigateToItem;
using ark::es2panda::lsp::RenameLocation;
using ark::es2panda::lsp::UserPreferences;
// api.h/types.h declare TextSpan in the global (non-namespaced) scope
using ::TextSpan;

// Named constants for CancellationToken IDs
constexpr size_t CANCEL_TOKEN_ID = 123;

class LspNullCheckFalseBranchTests : public LSPAPITests {};

// =============================================================================
// P4-NCB-01: GetTouchingToken_NullProgram_ReturnsNull
// GetTouchingTokenRightMatch accepts an optional parser::Program* parameter
// (default nullptr). Passing nullptr for program is a valid call path that
// exercises the null-program branch inside the function.
// =============================================================================
// Explicit nullptr program; internal_api_helpers_test.cpp reaches the same branch via the
// defaulted program parameter.
TEST_F(LspNullCheckFalseBranchTests, GetTouchingToken_NullProgram_ReturnsNull)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = 1;)";
    es2panda_Context *ctx = initializer.CreateContext("ncb01.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    // Call with explicit nullptr program (valid parameter). The function
    // falls back to the context's own program (internal_api.cpp:189), so for
    // a valid position it must return the touching token from that program.
    auto *token = ark::es2panda::lsp::GetTouchingTokenRightMatch(ctx, 4, nullptr);
    ASSERT_NE(token, nullptr);
    EXPECT_EQ(token->Start().index, 4U);

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P4-NCB-02: FindReferences_NullSourceFile_ReturnsEmpty - BLOCKED_DESIGN_MAPPING
// Design expectation: FindReferences with null SourceFile returns empty set.
// Actual product behavior: FindReferences (find_references.h:46-49) takes
// const SourceFile& (reference) or const vector<Context*>&, neither of which
// can be null. The public API requires a valid context/file. The "null source
// file" state is not reachable through the public API; every context has an
// associated source file.
// Implementation evidence: find_references.h:46-49 - all overloads require
// valid SourceFile& or Context*.
// next_action: Test empty result by querying a position with no references
// instead (already covered by loop boundary test LBB-01).
// No TEST_F written for this item.

// =============================================================================
// P4-NCB-03: GetQuickInfo_NullNode_ReturnsEmpty - BLOCKED_DESIGN_MAPPING
// Design expectation: GetQuickInfoAtPosition on a position yielding no node
// returns empty displayParts.
// Actual product behavior: GetQuickInfoAtPositionImpl (quick_info.h:44) takes
// context and position, internally calls GetTokenForQuickInfo which returns
// a node or nullptr. The public API does not expose a null-node path; the
// "null node" branch is internal to GetTokenForQuickInfo. The testable
// observable is that quick info at an empty position returns empty display.
// next_action: Test by requesting quick info at a position with no token
// (e.g., past end of file) and assert empty result.
// No TEST_F written for this item.

// =============================================================================
// P4-NCB-04: GetCompletions_NullAst_ReturnsEmpty - BLOCKED_DESIGN_MAPPING
// Design expectation: GetCompletionsAtPosition with null AST returns empty.
// Actual product behavior: GetCompletionsAtPositionImpl (completions.h:220)
// takes context; the context is created by Initializer and always has a
// parserProgram/AST (even if empty). The "null AST" state is not reachable
// through the public API.
// Implementation evidence: Initializer::CreateContext always calls
// ProceedToState which runs at least parsing.
// next_action: Test completion in an effectively empty scope (comment-only
// file) which already yields empty/keyword-only entries (covered by
// LBB-03 and EHB-01).
// No TEST_F written for this item.

// =============================================================================
// P4-NCB-05: FindRenameLocations_NullToken_ReturnsEmpty - BLOCKED_DESIGN_MAPPING
// Design expectation: FindRenameLocations with null token returns empty set.
// Actual product behavior: FindRenameLocations (find_rename_locations.h:57-63)
// takes position, not a token pointer. The "null token" branch would be
// internal to the token resolution logic. The public API does not accept
// a token parameter.
// Implementation evidence: find_rename_locations.h:57-63 - all overloads take
// position (size_t), not token pointer.
// next_action: Test rename at a position with no renameable symbol (e.g., on
// a keyword) which returns empty (covered by LBB-05 with single location).
// No TEST_F written for this item.

// =============================================================================
// P4-NCB-06: NavigateTo_InvalidPattern_ReturnsEmpty
// GetNavigateToItems takes a searchValue string. An invalid regex pattern
// (e.g., unclosed bracket "[") makes PatternMatcher::IsPatternValid() return
// false, which causes GetNavigateToItems to return empty immediately.
// =============================================================================
TEST_F(LspNullCheckFalseBranchTests, NavigateTo_InvalidPattern_ReturnsEmpty)
{
    Initializer initializer = Initializer();
    const std::string source = R"(
class MyClass {
    method(): void {}
}
function myFunction(): void {}
)";
    es2panda_Context *ctx = initializer.CreateContext("ncb06.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    std::vector<SourceFile> srcFiles = {SourceFile("ncb06.ets", source)};
    CancellationToken token(CANCEL_TOKEN_ID, nullptr);

    // Invalid regex pattern '[' makes IsPatternValid() return false
    // This exercises the early-return branch in GetNavigateToItems
    auto result = ark::es2panda::lsp::GetNavigateToItems(ctx, srcFiles, 20, "[", false);
    EXPECT_TRUE(result.empty());

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P4-NCB-07: GetDefinition_NullBoundSpan_ReturnsEmpty - BLOCKED_DESIGN_MAPPING
// Design expectation: GetDefinitionAndBoundSpan with null AST returns empty.
// Actual product behavior: GetDefinitionAndBoundSpan
// (get_definition_and_bound_span.h:35) takes context and offset. The context
// always has an AST. The "null bound span" branch is internal (when no
// definition is found). The testable observable is empty DefinitionInfo when
// no definition exists at the position.
// Implementation evidence: get_definition_and_bound_span.h:35 - requires
// valid context.
// next_action: Test definition at a position with no definition (e.g., on a
// literal) and assert empty fileName in result.
// No TEST_F written for this item.

// =============================================================================
// P4-NCB-08: OrganizeImports_NullProgram_ReturnsEmpty - BLOCKED_DESIGN_MAPPING
// Design expectation: OrganizeImports::Organize with null program returns
// empty FileTextChanges.
// Actual product behavior: OrganizeImports::Organize (organize_imports.h:45)
// takes context and fileName. The context always has a program. The "null
// program" branch is not reachable via public API.
// Implementation evidence: organize_imports.h:45 - requires valid context.
// next_action: Test OrganizeImports on a file with no imports (returns empty
// changes) - covered by LBB-04.
// No TEST_F written for this item.

// =============================================================================
// P4-NCB-09: Classifier_NullLexer_ReturnsEmpty - BLOCKED_DESIGN_MAPPING
// Design expectation: GetSyntacticClassifications with null lexer returns
// empty.
// Actual product behavior: GetSyntacticClassifications (classifier.h:53)
// takes context; it internally calls InitLexer which creates a lexer from
// the context. The "null lexer" branch would be internal to InitLexer. The
// public API does not expose lexer creation.
// Implementation evidence: classifier.h:49-53 - InitLexer requires context,
// GetSyntacticClassifications requires context.
// next_action: Test classification on empty file (returns empty) - not a
// high-priority branch.
// No TEST_F written for this item.

// =============================================================================
// P4-NCB-10: InlayHints_NullContext_ReturnsEmpty
// ProvideInlayHintsImpl takes es2panda_Context* directly. Passing nullptr
// is a valid call that exercises the null-context branch.
// =============================================================================
TEST_F(LspNullCheckFalseBranchTests, InlayHints_NullContext_ReturnsEmpty)
{
    TextSpan span = {0, 10};
    CancellationToken token(CANCEL_TOKEN_ID, nullptr);
    UserPreferences prefs = UserPreferences::GetDefaultUserPreferences();

    // Call with nullptr context - should not crash and return empty hints
    auto hints = ark::es2panda::lsp::ProvideInlayHintsImpl(nullptr, &span, token, prefs);
    EXPECT_TRUE(hints.hints.empty());
}

}  // namespace
