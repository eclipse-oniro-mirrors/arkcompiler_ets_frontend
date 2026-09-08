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
#include "lsp/include/internal_api.h"
#include "lsp/include/references.h"
#include "lsp/include/symbol_reference_index.h"
#include "public/es2panda_lib.h"

using ark::es2panda::lsp::Initializer;

class LSPSymbolReferenceIndexGapTests : public LSPAPITests {
public:
    LSPSymbolReferenceIndexGapTests() = default;
    ~LSPSymbolReferenceIndexGapTests() override = default;

    NO_COPY_SEMANTIC(LSPSymbolReferenceIndexGapTests);
    NO_MOVE_SEMANTIC(LSPSymbolReferenceIndexGapTests);

    void SetUp() override
    {
        LSPAPITests::SetUp();
        // Ensure a clean global index state before every test case.
        ark::es2panda::lsp::InitSymbolReferenceIndex();
    }

    void TearDown() override
    {
        // Clean global state to avoid leaking index entries between test cases.
        ark::es2panda::lsp::ClearSymbolReferenceIndex();
        LSPAPITests::TearDown();
    }

    static size_t CountReferencesInFile(const References &result, const std::string &fileName)
    {
        size_t count = 0;
        for (const auto &ref : result.referenceInfos) {
            if (ref.fileName == fileName) {
                count++;
            }
        }
        return count;
    }

    static void AssertReferencesEqual(const std::vector<ReferenceInfo> &actual,
                                      const std::vector<ReferenceInfo> &expected)
    {
        ASSERT_EQ(actual.size(), expected.size());
        for (size_t i = 0; i < expected.size(); i++) {
            ASSERT_EQ(actual[i].fileName, expected[i].fileName);
            ASSERT_EQ(actual[i].start, expected[i].start);
            ASSERT_EQ(actual[i].length, expected[i].length);
        }
    }

    // Query references with the plain (non-index) implementation over the given files.
    static References PlainGetReferences(const std::string &queryFile, size_t position,
                                         const std::vector<std::string> &allFiles)
    {
        Initializer initializer = Initializer();
        auto queryContext = initializer.CreateContext(queryFile.c_str(), ES2PANDA_STATE_CHECKED);
        auto astNode = ark::es2panda::lsp::GetTouchingToken(queryContext, position, false);
        auto declInfo = ark::es2panda::lsp::GetDeclInfoImpl(astNode);
        initializer.DestroyContext(queryContext);

        References result {};
        for (auto const &file : allFiles) {
            auto fileContext = initializer.CreateContext(file.c_str(), ES2PANDA_STATE_CHECKED);
            auto refInfo = ark::es2panda::lsp::GetReferencesAtPositionImpl(fileContext, declInfo);
            result.referenceInfos.insert(result.referenceInfos.end(), refInfo.referenceInfos.begin(),
                                         refInfo.referenceInfos.end());
            initializer.DestroyContext(fileContext);
        }
        auto comp = [](const ReferenceInfo &lhs, const ReferenceInfo &rhs) {
            if (lhs.fileName != rhs.fileName) {
                return lhs.fileName < rhs.fileName;
            }
            if (lhs.start != rhs.start) {
                return lhs.start < rhs.start;
            }
            return lhs.length < rhs.length;
        };
        ark::es2panda::lsp::RemoveDuplicates(result.referenceInfos, comp);
        return result;
    }
};

// Verify the index state after removing one importing file: its references disappear,
// the definition stays intact, and the other importing file still resolves.
// The scenario is fixed by this fixture: filePaths[1] is the removed file, filePaths[2] is the
// surviving importing file, and each importing file contributes exactly two references.
static void AssertReferencesAfterFileRemoved(const References &after, const std::vector<std::string> &filePaths,
                                             const std::vector<std::string> &texts, es2panda_Context *ctxB,
                                             es2panda_Context *ctxC)
{
    // Index of gap_remove_c.ets in the file vectors and the reference count per importing file.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t fileCIndex = 2;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t refsPerImportingFile = 2;
    // Only the two references from the removed file disappear; definition and file C stay intact.
    ASSERT_EQ(after.referenceInfos.size(), refsPerImportingFile);
    ASSERT_EQ(LSPSymbolReferenceIndexGapTests::CountReferencesInFile(after, filePaths[1]), 0U);
    ASSERT_EQ(LSPSymbolReferenceIndexGapTests::CountReferencesInFile(after, filePaths[fileCIndex]),
              refsPerImportingFile);
    ASSERT_EQ(after.definitionInfo.fileName, filePaths[0]);
    ASSERT_EQ(after.definitionInfo.start, 11U);
    ASSERT_EQ(after.definitionInfo.length, 9U);
    for (const auto &ref : after.referenceInfos) {
        ASSERT_EQ(ref.fileName, filePaths[fileCIndex]);
        ASSERT_EQ(ref.length, 9U);
    }
    ASSERT_EQ(after.referenceInfos[0].start, 9U);
    ASSERT_EQ(after.referenceInfos[1].start, 59U);
    // Querying inside the removed file now returns empty, other files still resolve.
    const auto posInB = texts[1].find("console.log(gapShared)") + std::string("console.log(").size();
    auto fromRemoved = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(ctxB, posInB);
    ASSERT_TRUE(fromRemoved.referenceInfos.empty());
    const auto posInC = texts[fileCIndex].find("console.log(gapShared)") + std::string("console.log(").size();
    auto fromC = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(ctxC, posInC);
    ASSERT_EQ(fromC.referenceInfos.size(), refsPerImportingFile);
    ASSERT_EQ(fromC.definitionInfo.fileName, filePaths[0]);
}

// Test: index-based references are consistent with the plain references implementation
TEST_F(LSPSymbolReferenceIndexGapTests, SingleFileIndexConsistentWithPlainReferences)
{
    std::vector<std::string> files = {"gap_plain1.ets"};
    std::vector<std::string> texts = {R"(export let gapVar: number = 1;
function useGapVar(): number {
    return gapVar;
}
console.log(gapVar);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Query at a usage position inside the function body
    const auto queryPos = texts[0].find("return gapVar") + std::string("return ").size();
    auto plain = PlainGetReferences(filePaths[0], queryPos, filePaths);
    // The plain implementation reports usages only (definition goes through decl info separately).
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<ReferenceInfo> expectedPlain {{filePaths[0], 73, 6}, {filePaths[0], 95, 6}};
    // NOLINTEND(readability-magic-numbers)
    AssertReferencesEqual(plain.referenceInfos, expectedPlain);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));
    auto indexed = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, queryPos);
    initializer.DestroyContext(context);

    // The index returns the same usage list, and resolves the definition through definitionInfo.
    AssertReferencesEqual(indexed.referenceInfos, plain.referenceInfos);
    ASSERT_EQ(indexed.definitionInfo.fileName, filePaths[0]);
    ASSERT_EQ(indexed.definitionInfo.start, 11U);
    ASSERT_EQ(indexed.definitionInfo.length, 6U);
}

// Test: index-based cross-file references are consistent with the plain implementation
TEST_F(LSPSymbolReferenceIndexGapTests, MultiFileIndexConsistentWithPlainReferences)
{
    std::vector<std::string> files = {"gap_plain2.ets", "gap_plain3.ets"};
    std::vector<std::string> texts = {R"(export let gapShared: number = 1;
let gapLocal = gapShared;)",
                                      R"(import { gapShared } from './gap_plain2';
console.log(gapShared);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Query at a usage in the export file
    const auto queryPos = texts[0].find("gapLocal = gapShared") + std::string("gapLocal = ").size();
    auto plain = PlainGetReferences(filePaths[0], queryPos, filePaths);
    // The plain implementation reports usages only, including both occurrences in the importing file.
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<ReferenceInfo> expectedPlain {{filePaths[0], 49, 9}, {filePaths[1], 9, 9}, {filePaths[1], 54, 9}};
    // NOLINTEND(readability-magic-numbers)
    AssertReferencesEqual(plain.referenceInfos, expectedPlain);

    Initializer initializer;
    auto *exportCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(exportCtx, nullptr);
    auto *importCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(importCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(exportCtx));
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(importCtx));
    auto indexed = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(exportCtx, queryPos);
    initializer.DestroyContext(exportCtx);
    initializer.DestroyContext(importCtx);

    // The index returns the same cross-file usage list plus the resolved definition.
    AssertReferencesEqual(indexed.referenceInfos, plain.referenceInfos);
    ASSERT_EQ(indexed.definitionInfo.fileName, filePaths[0]);
    ASSERT_EQ(indexed.definitionInfo.start, 11U);
    ASSERT_EQ(indexed.definitionInfo.length, 9U);
    ASSERT_EQ(CountReferencesInFile(indexed, filePaths[0]), 1U);
    ASSERT_EQ(CountReferencesInFile(indexed, filePaths[1]), 2U);
}

// Test: after remove, references from the removed file disappear while other files stay indexed
TEST_F(LSPSymbolReferenceIndexGapTests, RemoveFileReferencesDisappearOthersUnaffected)
{
    std::vector<std::string> files = {"gap_remove_base.ets", "gap_remove_b.ets", "gap_remove_c.ets"};
    std::vector<std::string> texts = {R"(export let gapShared: number = 1;)",
                                      R"(import { gapShared } from './gap_remove_base';
console.log(gapShared);)",
                                      R"(import { gapShared } from './gap_remove_base';
console.log(gapShared);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Index of gap_remove_c.ets in the file vectors and the reference count per importing file.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t fileCIndex = 2;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t refsPerImportingFile = 2;

    Initializer initializer;
    auto *baseCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(baseCtx, nullptr);
    auto *ctxB = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxB, nullptr);
    auto *ctxC = initializer.CreateContext(filePaths[fileCIndex].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxC, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(baseCtx));
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(ctxB));
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(ctxC));

    const auto defPos = texts[0].find("gapShared: number");
    auto before = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(baseCtx, defPos);
    ASSERT_EQ(before.referenceInfos.size(), 4U);
    ASSERT_EQ(CountReferencesInFile(before, filePaths[1]), refsPerImportingFile);
    ASSERT_EQ(CountReferencesInFile(before, filePaths[fileCIndex]), refsPerImportingFile);

    ASSERT_TRUE(ark::es2panda::lsp::RemoveSymbolReferenceIndexForFile(filePaths[1]));
    auto after = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(baseCtx, defPos);
    AssertReferencesAfterFileRemoved(after, filePaths, texts, ctxB, ctxC);

    initializer.DestroyContext(baseCtx);
    initializer.DestroyContext(ctxB);
    initializer.DestroyContext(ctxC);
}

// Test: after clear, index queries return empty results (no stale fallback)
TEST_F(LSPSymbolReferenceIndexGapTests, ClearIndexQueriesReturnEmpty)
{
    std::vector<std::string> files = {"gap_clear_a.ets", "gap_clear_b.ets"};
    std::vector<std::string> texts = {R"(export let gapVar: number = 1;)",
                                      R"(import { gapVar } from './gap_clear_a';
console.log(gapVar);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *ctxA = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxA, nullptr);
    auto *ctxB = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxB, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(ctxA));
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(ctxB));

    const auto defPos = texts[0].find("gapVar: number");
    auto before = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(ctxA, defPos);
    ASSERT_EQ(before.referenceInfos.size(), 2U);

    ark::es2panda::lsp::ClearSymbolReferenceIndex();

    auto afterA = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(ctxA, defPos);
    ASSERT_TRUE(afterA.referenceInfos.empty());
    ASSERT_TRUE(afterA.definitionInfo.fileName.empty());
    const auto posInB = texts[1].find("console.log(gapVar)") + std::string("console.log(").size();
    auto afterB = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(ctxB, posInB);
    ASSERT_TRUE(afterB.referenceInfos.empty());
    ASSERT_TRUE(afterB.definitionInfo.fileName.empty());

    // Rebuilding after clear restores the exact same references as before.
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(ctxA));
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(ctxB));
    auto rebuilt = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(ctxA, defPos);
    ASSERT_EQ(rebuilt.referenceInfos.size(), before.referenceInfos.size());
    for (size_t i = 0; i < before.referenceInfos.size(); i++) {
        ASSERT_EQ(rebuilt.referenceInfos[i].fileName, before.referenceInfos[i].fileName);
        ASSERT_EQ(rebuilt.referenceInfos[i].start, before.referenceInfos[i].start);
        ASSERT_EQ(rebuilt.referenceInfos[i].length, before.referenceInfos[i].length);
    }

    initializer.DestroyContext(ctxA);
    initializer.DestroyContext(ctxB);
}

// Test: updating file content and rebuilding the index leaves no stale references
TEST_F(LSPSymbolReferenceIndexGapTests, RebuildAfterContentUpdateLeavesNoStaleReferences)
{
    std::vector<std::string> files = {"gap_update.ets"};
    std::vector<std::string> texts = {R"(let gapOld: number = 1;
console.log(gapOld);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *oldCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(oldCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(oldCtx));

    const auto oldDefPos = texts[0].find("gapOld: number");
    auto oldResult = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(oldCtx, oldDefPos);
    ASSERT_EQ(oldResult.referenceInfos.size(), 1U);
    ASSERT_EQ(oldResult.referenceInfos[0].start, 36U);
    ASSERT_EQ(oldResult.referenceInfos[0].length, 6U);
    initializer.DestroyContext(oldCtx);

    // Overwrite the file with a structurally different content, then rebuild from a fresh context.
    std::vector<std::string> newTexts = {R"(// updated header comment line one
// updated header comment line two
let gapNew: number = 2;
console.log(gapNew);)"};
    auto newFilePaths = CreateTempFile(files, newTexts);
    ASSERT_EQ(newFilePaths.size(), files.size());
    ASSERT_EQ(newFilePaths[0], filePaths[0]);

    auto *newCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(newCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(newCtx));

    // The indexed source is refreshed to the new content.
    ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]), newTexts[0]);

    // Querying at the old usage position no longer resolves to the removed symbol:
    // offset 36 now sits inside a comment, and the old gapOld reference set is gone.
    const auto oldUsePos = texts[0].find("console.log(gapOld)") + std::string("console.log(").size();
    auto stale = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(newCtx, oldUsePos);
    ASSERT_TRUE(stale.referenceInfos.empty());
    ASSERT_TRUE(stale.definitionInfo.fileName.empty());

    // The new symbol resolves with exact new offsets.
    const auto newDefPos = newTexts[0].find("gapNew: number");
    auto newResult = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(newCtx, newDefPos);
    ASSERT_EQ(newResult.referenceInfos.size(), 1U);
    ASSERT_EQ(newResult.referenceInfos[0].fileName, filePaths[0]);
    ASSERT_EQ(newResult.referenceInfos[0].start, 106U);
    ASSERT_EQ(newResult.referenceInfos[0].length, 6U);
    ASSERT_EQ(newResult.definitionInfo.fileName, filePaths[0]);
    ASSERT_EQ(newResult.definitionInfo.start, 74U);
    ASSERT_EQ(newResult.definitionInfo.length, 6U);

    initializer.DestroyContext(newCtx);
}

// Test: a file imported by the indexed context is treated as an external declaration source:
// BuildSymbolReferenceIndexForContext does not index it,
// BuildSymbolReferenceIndexForContextWithExternal does.
TEST_F(LSPSymbolReferenceIndexGapTests, DeclarationFileIndexingPolicy)
{
    std::vector<std::string> files = {"gap_decl_export.ets", "gap_decl_main.ets"};
    std::vector<std::string> texts = {R"(export let gapDecl: number = 1;)",
                                      R"(import { gapDecl } from './gap_decl_export';
console.log(gapDecl);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *mainCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(mainCtx, nullptr);

    // Building only the main context does not index the external declaration file.
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(mainCtx));
    ASSERT_FALSE(ark::es2panda::lsp::GetIndexedFileSource(filePaths[1]).empty());
    ASSERT_TRUE(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]).empty());

    // Building with external sources indexes the declaration file with its exact source text.
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContextWithExternal(mainCtx));
    ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]), texts[0]);

    initializer.DestroyContext(mainCtx);
}

// Cache/declaration-like files use the same explicit indexing policy as normal
// source contexts: once a context exists and is built into the index, its exact
// file source is retrievable from the index. This fixes the plan's cache-file
// participation policy instead of leaving it implicit.
TEST_F(LSPSymbolReferenceIndexGapTests, CacheFileIndexingPolicy)
{
    const std::string fileName = "gap_cache_policy.etscache";
    const std::string source = "export let cacheValue: number = 1;\nconsole.log(cacheValue);\n";

    Initializer initializer;
    auto *ctx = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    EXPECT_TRUE(ark::es2panda::lsp::GetIndexedFileSource(fileName).empty());
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(ctx));
    EXPECT_EQ(ark::es2panda::lsp::GetIndexedFileSource(fileName), source);

    const auto pos = source.find("cacheValue: number");
    ASSERT_NE(pos, std::string::npos);
    const auto refs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(ctx, pos);
    EXPECT_EQ(refs.definitionInfo.fileName, fileName);
    EXPECT_EQ(refs.referenceInfos.size(), 1U);

    initializer.DestroyContext(ctx);
}

// Test: contexts opened sequentially A then B - querying references in A does not depend on B
TEST_F(LSPSymbolReferenceIndexGapTests, SequentialContextsQueryADoesNotDependOnB)
{
    std::vector<std::string> files = {"gap_seq_a.ets", "gap_seq_b.ets"};
    std::vector<std::string> texts = {R"(let gapA: number = 1;
console.log(gapA);)",
                                      R"(let gapB: string = "hello";
console.log(gapB);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *ctxA = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxA, nullptr);
    auto *ctxB = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxB, nullptr);

    // Index file A only; file B's context exists but is never indexed.
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(ctxA));

    const auto defPosA = texts[0].find("gapA: number");
    auto resultA = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(ctxA, defPosA);
    ASSERT_EQ(resultA.referenceInfos.size(), 1U);
    ASSERT_EQ(resultA.referenceInfos[0].fileName, filePaths[0]);
    ASSERT_EQ(resultA.referenceInfos[0].start, 34U);
    ASSERT_EQ(resultA.referenceInfos[0].length, 4U);
    ASSERT_EQ(resultA.definitionInfo.fileName, filePaths[0]);
    ASSERT_EQ(resultA.definitionInfo.start, 4U);
    ASSERT_EQ(resultA.definitionInfo.length, 4U);

    // File B is not indexed, so querying it returns empty.
    const auto defPosB = texts[1].find("gapB: string");
    auto resultB = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(ctxB, defPosB);
    ASSERT_TRUE(resultB.referenceInfos.empty());

    // Destroying context B does not disturb queries against context A.
    initializer.DestroyContext(ctxB);
    auto resultAAfterBDestroyed = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(ctxA, defPosA);
    ASSERT_EQ(resultAAfterBDestroyed.referenceInfos.size(), 1U);
    ASSERT_EQ(resultAAfterBDestroyed.referenceInfos[0].start, 34U);
    ASSERT_EQ(resultAAfterBDestroyed.definitionInfo.start, 4U);

    initializer.DestroyContext(ctxA);
}

// Test: the external (imported) file contributes references when indexed via WithExternal
TEST_F(LSPSymbolReferenceIndexGapTests, ExternalFileContributesReferences)
{
    std::vector<std::string> files = {"gap_ext_export.ets", "gap_ext_main.ets"};
    std::vector<std::string> texts = {R"(export let gapExt: number = 1;
let gapExtUse = gapExt;)",
                                      R"(import { gapExt } from './gap_ext_export';
console.log(gapExt);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *mainCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(mainCtx, nullptr);

    // Without indexing the external file, only the main file's references are visible.
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(mainCtx));
    const auto posInMain = texts[1].find("console.log(gapExt)") + std::string("console.log(").size();
    auto withoutExternal = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(mainCtx, posInMain);
    // Only the main file's own occurrences are visible: the import specifier and the usage.
    ASSERT_EQ(withoutExternal.referenceInfos.size(), 2U);
    ASSERT_EQ(withoutExternal.referenceInfos[0].fileName, filePaths[1]);
    ASSERT_EQ(withoutExternal.referenceInfos[0].start, 9U);
    ASSERT_EQ(withoutExternal.referenceInfos[0].length, 6U);
    ASSERT_EQ(withoutExternal.referenceInfos[1].fileName, filePaths[1]);
    ASSERT_EQ(withoutExternal.referenceInfos[1].start, 55U);
    ASSERT_EQ(withoutExternal.referenceInfos[1].length, 6U);
    // The definition lives in the not-yet-indexed external file, so it is not resolved yet.
    ASSERT_TRUE(withoutExternal.definitionInfo.fileName.empty());

    // With external sources indexed, the external file participates in the lookup.
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContextWithExternal(mainCtx));
    auto withExternal = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(mainCtx, posInMain);
    // Index of the third entry (the second main-file reference) in the sorted reference list.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t secondMainFileRefIndex = 2;
    ASSERT_EQ(withExternal.referenceInfos.size(), 3U);
    ASSERT_EQ(CountReferencesInFile(withExternal, filePaths[0]), 1U);
    ASSERT_EQ(CountReferencesInFile(withExternal, filePaths[1]), 2U);
    // References are sorted by fileName then start; gap_ext_export.ets sorts before gap_ext_main.ets.
    ASSERT_EQ(withExternal.referenceInfos[0].fileName, filePaths[0]);
    ASSERT_EQ(withExternal.referenceInfos[0].start, 47U);
    ASSERT_EQ(withExternal.referenceInfos[0].length, 6U);
    ASSERT_EQ(withExternal.referenceInfos[1].fileName, filePaths[1]);
    ASSERT_EQ(withExternal.referenceInfos[1].start, 9U);
    ASSERT_EQ(withExternal.referenceInfos[1].length, 6U);
    ASSERT_EQ(withExternal.referenceInfos[secondMainFileRefIndex].fileName, filePaths[1]);
    ASSERT_EQ(withExternal.referenceInfos[secondMainFileRefIndex].start, 55U);
    ASSERT_EQ(withExternal.referenceInfos[secondMainFileRefIndex].length, 6U);
    ASSERT_EQ(withExternal.definitionInfo.fileName, filePaths[0]);
    ASSERT_EQ(withExternal.definitionInfo.start, 11U);
    ASSERT_EQ(withExternal.definitionInfo.length, 6U);

    initializer.DestroyContext(mainCtx);
}

// Test: GetFileReferencesFromIndex locates import statements that reference a file
TEST_F(LSPSymbolReferenceIndexGapTests, FileReferencesFromIndexForImportingFile)
{
    std::vector<std::string> files = {"gap_fileref_base.ets", "gap_fileref_user.ets"};
    std::vector<std::string> texts = {R"(export let gapBase: number = 1;)",
                                      R"(import { gapBase } from './gap_fileref_base';
console.log(gapBase);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *baseCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(baseCtx, nullptr);
    auto *userCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(userCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(userCtx));

    // The import statement in the user file resolves to the base file path.
    auto fileRefs = ark::es2panda::lsp::GetFileReferencesFromIndex(userCtx, filePaths[0], false);
    ASSERT_EQ(fileRefs.referenceInfos.size(), 1U);
    ASSERT_EQ(fileRefs.referenceInfos[0].fileName, filePaths[1]);
    ASSERT_EQ(fileRefs.referenceInfos[0].start, 24U);
    // The span covers the quoted module specifier './gap_fileref_base'.
    ASSERT_EQ(fileRefs.referenceInfos[0].length, 20U);

    // A file that nobody imports yields no file references.
    auto noRefs = ark::es2panda::lsp::GetFileReferencesFromIndex(userCtx, filePaths[1], false);
    ASSERT_TRUE(noRefs.referenceInfos.empty());

    initializer.DestroyContext(baseCtx);
    initializer.DestroyContext(userCtx);
}

// Wrapper-level test: the LSPAPI index wrappers Init -> build -> query -> source
// -> Clear round trip. GetReferencesAtPositionFromIndexWrapper converts spans to
// code-point offsets; GetIndexedFileSourceWrapper returns the indexed source.
TEST_F(LSPSymbolReferenceIndexGapTests, SymbolReferenceIndexWrappersRoundTrip)
{
    std::vector<std::string> files = {"gap_wrapper_roundtrip.ets"};
    std::vector<std::string> texts = {R"(let gapWrap: number = 1;
console.log(gapWrap);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    lspApi->initSymbolReferenceIndex();

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(context));

    // The indexed source must exactly match the file content.
    ASSERT_EQ(lspApi->getIndexedFileSource(filePaths[0]), texts[0]);

    const auto defPos = texts[0].find("gapWrap: number");
    ASSERT_NE(defPos, std::string::npos);
    auto refs = lspApi->getReferencesAtPositionFromIndex(context, defPos);
    ASSERT_EQ(refs.referenceInfos.size(), 1U);
    ASSERT_EQ(refs.referenceInfos[0].fileName, filePaths[0]);
    const auto usePos = texts[0].find("gapWrap);");
    ASSERT_NE(usePos, std::string::npos);
    ASSERT_EQ(refs.referenceInfos[0].start, usePos);
    ASSERT_EQ(refs.referenceInfos[0].length, 7U);
    ASSERT_EQ(refs.definitionInfo.fileName, filePaths[0]);
    ASSERT_EQ(refs.definitionInfo.start, defPos);
    ASSERT_EQ(refs.definitionInfo.length, 7U);

    lspApi->clearSymbolReferenceIndex();
    auto afterClear = lspApi->getIndexedFileSource(filePaths[0]);
    ASSERT_TRUE(afterClear.empty());
    auto afterClearRefs = lspApi->getReferencesAtPositionFromIndex(context, defPos);
    ASSERT_TRUE(afterClearRefs.referenceInfos.empty());

    initializer.DestroyContext(context);
}

// Wrapper-level test: a symbol referenced from two files resolves through the public
// getReferencesAtPositionFromIndex wrapper into a reference list that is sorted by
// (fileName, start, length) and contains no duplicates. The wrapper converts the
// indexed byte offsets to code-point offsets and then runs RemoveDuplicates, so this
// scenario exercises the compare lambda inside GetReferencesAtPositionFromIndexWrapper
// (api.cpp), which is only invoked when the wrapper returns at least two references.
TEST_F(LSPSymbolReferenceIndexGapTests, WrapperMultiFileReferencesSortedAndDeduplicated)
{
    std::vector<std::string> files = {"gap_wrapper_sort_a.ets", "gap_wrapper_sort_b.ets"};
    std::vector<std::string> texts = {R"(export let gapSort: number = 1;
let gapSortLocal = gapSort;)",
                                      R"(import { gapSort } from './gap_wrapper_sort_a';
console.log(gapSort);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    lspApi->initSymbolReferenceIndex();

    Initializer initializer;
    auto *ctxA = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxA, nullptr);
    auto *ctxB = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxB, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(ctxA));
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(ctxB));

    // Query at the definition of gapSort inside file A.
    const auto defPos = texts[0].find("gapSort: number");
    ASSERT_NE(defPos, std::string::npos);
    auto refs = lspApi->getReferencesAtPositionFromIndex(ctxA, defPos);

    // Three references: the local use in file A plus the import specifier and the
    // console.log usage in file B. The result must be sorted by fileName, then start,
    // then length, with each (fileName, start, length) triplet unique.
    // Index 2 addresses the console.log usage recorded inside importing file B.
    constexpr size_t importingFileUsageIndex = 2;
    // NOLINTBEGIN(readability-magic-numbers)
    ASSERT_EQ(refs.referenceInfos.size(), 3U);
    ASSERT_EQ(refs.referenceInfos[0].fileName, filePaths[0]);
    ASSERT_EQ(refs.referenceInfos[0].start, 51U);
    ASSERT_EQ(refs.referenceInfos[0].length, 7U);
    ASSERT_EQ(refs.referenceInfos[1].fileName, filePaths[1]);
    ASSERT_EQ(refs.referenceInfos[1].start, 9U);
    ASSERT_EQ(refs.referenceInfos[1].length, 7U);
    ASSERT_EQ(refs.referenceInfos[importingFileUsageIndex].fileName, filePaths[1]);
    ASSERT_EQ(refs.referenceInfos[importingFileUsageIndex].start, 60U);
    ASSERT_EQ(refs.referenceInfos[importingFileUsageIndex].length, 7U);
    // NOLINTEND(readability-magic-numbers)
    ASSERT_EQ(refs.definitionInfo.fileName, filePaths[0]);
    ASSERT_EQ(refs.definitionInfo.start, defPos);
    ASSERT_EQ(refs.definitionInfo.length, 7U);

    // Dedup invariant: no two consecutive references share the same triplet.
    for (size_t i = 1; i < refs.referenceInfos.size(); i++) {
        const auto &prev = refs.referenceInfos[i - 1];
        const auto &curr = refs.referenceInfos[i];
        ASSERT_FALSE(prev.fileName == curr.fileName && prev.start == curr.start && prev.length == curr.length);
    }

    initializer.DestroyContext(ctxA);
    initializer.DestroyContext(ctxB);
}

// Wrapper-level test: RemoveSymbolReferenceIndexForFileWrapper drops exactly the
// given file from the index while other files remain indexed.
TEST_F(LSPSymbolReferenceIndexGapTests, WrapperRemoveFileLeavesOthersIndexed)
{
    std::vector<std::string> files = {"gap_wrapper_remove_a.ets", "gap_wrapper_remove_b.ets"};
    std::vector<std::string> texts = {R"(let gapWrapA: number = 1;
console.log(gapWrapA);)",
                                      R"(let gapWrapB: string = "b";
console.log(gapWrapB);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    lspApi->initSymbolReferenceIndex();

    Initializer initializer;
    auto *ctxA = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxA, nullptr);
    auto *ctxB = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxB, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(ctxA));
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(ctxB));

    ASSERT_EQ(lspApi->getIndexedFileSource(filePaths[0]), texts[0]);
    ASSERT_EQ(lspApi->getIndexedFileSource(filePaths[1]), texts[1]);

    ASSERT_TRUE(lspApi->removeSymbolReferenceIndexForFile(filePaths[0].c_str()));
    ASSERT_TRUE(lspApi->getIndexedFileSource(filePaths[0]).empty());
    ASSERT_EQ(lspApi->getIndexedFileSource(filePaths[1]), texts[1]);

    // Removing a file that is not indexed returns false.
    ASSERT_FALSE(lspApi->removeSymbolReferenceIndexForFile(filePaths[0].c_str()));
    // A null file name must be handled safely and returns false.
    ASSERT_FALSE(lspApi->removeSymbolReferenceIndexForFile(nullptr));

    initializer.DestroyContext(ctxA);
    initializer.DestroyContext(ctxB);
}

// Wrapper-level test: DeleteProgramForFile removes the resolved program entry for
// the file from the import path manager; a freshly created context for the same
// file still parses and checks successfully afterwards.
TEST_F(LSPSymbolReferenceIndexGapTests, DeleteProgramForFileWrapper)
{
    std::vector<std::string> files = {"gap_delete_program.ets"};
    std::vector<std::string> texts = {R"(export let gapDeleted: number = 1;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Deleting the currently parsed program returns 0 (success).
    ASSERT_EQ(lspApi->DeleteProgramForFile(context, filePaths[0].c_str()), 0);
    // A null file name is a no-op and returns 0.
    ASSERT_EQ(lspApi->DeleteProgramForFile(context, nullptr), 0);

    initializer.DestroyContext(context);

    // After deletion a fresh context for the same file still works.
    auto *freshContext = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(freshContext, nullptr);
    LSPAPI const *lspApiFresh = GetImpl();
    auto refs = lspApiFresh->getDeclInfo(freshContext, texts[0].find("gapDeleted: number"));
    ASSERT_FALSE(refs.fileName.empty());
    initializer.DestroyContext(freshContext);
}
