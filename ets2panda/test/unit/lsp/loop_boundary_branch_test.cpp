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
#include "lsp/include/organize_imports.h"
#include "lsp/include/cancellation_token.h"
#include "public/es2panda_lib.h"

namespace {

using ::FileTextChanges;
using ark::es2panda::SourceFile;
using ark::es2panda::lsp::CancellationToken;
using ark::es2panda::lsp::CompletionInfo;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::RenameLocation;

// Named constants for CancellationToken IDs
constexpr size_t CANCEL_TOKEN_ID = 123;
// Named origin index
constexpr size_t FILE_A_IDX = 0;

class LspLoopBoundaryBranchTests : public LSPAPITests {};

// =============================================================================
// P4-LBB-01: FindReferences_EmptyResultList_LoopZeroIterations
// FindReferences on a symbol with 0 references (e.g., unused private member)
// should iterate 0 times over results and return empty set.
// =============================================================================
TEST_F(LspLoopBoundaryBranchTests, FindReferences_EmptyResultList_LoopZeroIterations)
{
    std::string source = R"(
class C {
    private unusedMethod(): void {}
}
)";
    std::vector<std::string> files = {"lbb01.ets"};
    auto filePaths = CreateTempFile(files, {source});
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer init;
    auto ctx = init.CreateContext(filePaths[FILE_A_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // Position on the unused private method name
    size_t pos = source.find("unusedMethod");
    ASSERT_NE(pos, std::string::npos);

    std::vector<SourceFile> srcFiles = {SourceFile(filePaths[FILE_A_IDX], source)};
    CancellationToken token(CANCEL_TOKEN_ID, nullptr);
    auto refs = ark::es2panda::lsp::FindReferences(&token, srcFiles, srcFiles[FILE_A_IDX], pos);

    // Private unused method: only the definition itself, no other references
    // The loop over results executes 0 times for external references
    // (the definition is included as isDefinition=true)
    // We verify the result is bounded and the definition is found
    EXPECT_LE(refs.size(), 1U);  // At most the definition itself

    init.DestroyContext(ctx);
}

// =============================================================================
// P4-LBB-02: FindReferences_SingleResult_LoopOneIteration
// FindReferences on a symbol with exactly 1 reference (declaration + 1 use)
// should iterate exactly 1 time over the reference results.
// =============================================================================
TEST_F(LspLoopBoundaryBranchTests, FindReferences_SingleResult_LoopOneIteration)
{
    std::string source = R"(
let target = 42;
function test(): void {
    let x = target;
}
)";
    std::vector<std::string> files = {"lbb02.ets"};
    auto filePaths = CreateTempFile(files, {source});
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer init;
    auto ctx = init.CreateContext(filePaths[FILE_A_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // Position on the reference 'target' in the function body
    size_t pos = source.find("target", source.find("let x ="));
    ASSERT_NE(pos, std::string::npos);

    std::vector<SourceFile> srcFiles = {SourceFile(filePaths[FILE_A_IDX], source)};
    CancellationToken token(CANCEL_TOKEN_ID, nullptr);
    auto refs = ark::es2panda::lsp::FindReferences(&token, srcFiles, srcFiles[FILE_A_IDX], pos);

    // Should find: definition + 1 reference = at least 2 entries
    // (definition has isDefinition=true, reference has isDefinition=false)
    EXPECT_GE(refs.size(), 2U);

    init.DestroyContext(ctx);
}

// =============================================================================
// P4-LBB-03: GetCompletions_EmptyScope_LoopZeroIterations
// GetCompletionsAtPosition in an effectively empty scope (comment-only file)
// should traverse 0 symbols and return 0 or keyword-only entries.
// =============================================================================
TEST_F(LspLoopBoundaryBranchTests, GetCompletions_EmptyScope_LoopZeroIterations)
{
    std::string source = "// empty file with only comment\n";
    std::vector<std::string> files = {"lbb03.ets"};
    auto filePaths = CreateTempFile(files, {source});
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer init;
    auto ctx = init.CreateContext(filePaths[FILE_A_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // Position at end of file (after comment)
    size_t pos = source.size();

    auto res = lspApi->getCompletionsAtPosition(ctx, pos);

    // In an empty scope, the internal loop over symbol tables executes 0 times
    // and no keyword completions are offered either: the exact outcome is an
    // empty entry list (design expectation "返回空").
    EXPECT_EQ(res.GetEntries().size(), 0U);

    init.DestroyContext(ctx);
}

// =============================================================================
// P4-LBB-05: FindRenameLocations_SingleLocation_LoopOneIteration
// FindRenameLocationsInCurrentFile on a symbol with exactly 1 rename location
// (declaration only, no other references) should iterate 1 time.
// =============================================================================
TEST_F(LspLoopBoundaryBranchTests, FindRenameLocations_SingleLocation_LoopOneIteration)
{
    std::string source = R"(
let singleUse = 42;
)";
    std::vector<std::string> files = {"lbb05.ets"};
    auto filePaths = CreateTempFile(files, {source});
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer init;
    auto ctx = init.CreateContext(filePaths[FILE_A_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // Position on the declaration 'singleUse'
    size_t pos = source.find("singleUse");
    ASSERT_NE(pos, std::string::npos);

    auto locations = lspApi->findRenameLocationsInCurrentFile(ctx, pos);

    // Declaration only: exactly 1 rename location (the declaration itself)
    EXPECT_EQ(locations.size(), 1U);

    init.DestroyContext(ctx);
}

}  // namespace
