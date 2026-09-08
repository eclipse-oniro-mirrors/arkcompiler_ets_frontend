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

namespace {

// Test suite for module-key, external-program, package-module-path and
// definition-search edges of the symbol reference index that are not
// exercised by symbol_reference_index_test.cpp / _test_1.cpp.
class LSPSymbolReferenceIndexModuleKeyTests : public LSPAPITests {
public:
    LSPSymbolReferenceIndexModuleKeyTests() = default;
    ~LSPSymbolReferenceIndexModuleKeyTests() override = default;

    NO_COPY_SEMANTIC(LSPSymbolReferenceIndexModuleKeyTests);
    NO_MOVE_SEMANTIC(LSPSymbolReferenceIndexModuleKeyTests);

    void SetUp() override
    {
        LSPAPITests::SetUp();
        ark::es2panda::lsp::InitSymbolReferenceIndex();
    }

    void TearDown() override
    {
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
};

// Max-distance budgets for the LevenshteinDistance edge cases below; costs are
// scaled (insert/delete = 10 per character), so each budget pins one scenario.
constexpr int GENEROUS_MAX_DISTANCE = 5;
constexpr int REJECTING_MAX_DISTANCE = 2;
constexpr int IDENTICAL_PAIR_MAX_DISTANCE = 9;
constexpr int DISTANT_PAIR_MAX_DISTANCE = 3;
// Insert/delete cost is 10 per character, so "" <-> "abc" scores exactly this.
constexpr int THREE_CHAR_INSERT_DELETE_COST = 30;

// LevenshteinDistance: empty operands take the dedicated early-return branches.
// Costs are scaled: insert/delete = 10 per character; -1 signals "too far".
TEST_F(LSPSymbolReferenceIndexModuleKeyTests, LevenshteinDistanceEmptyOperandEdges)
{
    // NOLINTBEGIN(readability-magic-numbers)
    EXPECT_EQ(ark::es2panda::lsp::LevenshteinDistance("", "abc", GENEROUS_MAX_DISTANCE), THREE_CHAR_INSERT_DELETE_COST);
    EXPECT_EQ(ark::es2panda::lsp::LevenshteinDistance("abc", "", GENEROUS_MAX_DISTANCE), THREE_CHAR_INSERT_DELETE_COST);
    // Empty source with a target beyond maxDistance must be rejected as -1.
    EXPECT_EQ(ark::es2panda::lsp::LevenshteinDistance("", "abcdef", REJECTING_MAX_DISTANCE), -1);
    EXPECT_EQ(ark::es2panda::lsp::LevenshteinDistance("abc", "", 1), -1);
    // Identical strings cost nothing; an early-terminated far pair returns -1.
    EXPECT_EQ(ark::es2panda::lsp::LevenshteinDistance("same", "same", IDENTICAL_PAIR_MAX_DISTANCE), 0);
    EXPECT_EQ(ark::es2panda::lsp::LevenshteinDistance("kitten", "sitting", DISTANT_PAIR_MAX_DISTANCE), -1);
    // NOLINTEND(readability-magic-numbers)
}

// GetSpellingSuggestion skips candidate/name pairs where both are shorter than
// three characters and do not differ by case only. The index supplies real
// occurrences so FindSimilarSymbolNames runs its full pipeline.
TEST_F(LSPSymbolReferenceIndexModuleKeyTests, SpellingSuggestionSkipsShortNamesWithoutCaseMatch)
{
    std::vector<std::string> files = {"sri_short_names.ets"};
    std::vector<std::string> texts = {R"(let ab = 7;
console.log(ab);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    // "xy" vs "ab": both shorter than three chars, no case-only relation -> skipped.
    auto skipped = ark::es2panda::lsp::FindSimilarSymbolNames("xy", filePaths[0]);
    EXPECT_TRUE(skipped.empty());

    // "AB" vs "ab" is a case-only difference -> eligible despite short length.
    auto suggested = ark::es2panda::lsp::FindSimilarSymbolNames("AB", filePaths[0]);
    ASSERT_EQ(suggested.size(), 1U);
    EXPECT_EQ(suggested[0], "ab");

    initializer.DestroyContext(context);
}

// GetFileReferencesFromIndex with isPackageModule=true requires a path
// separator in searchFileName; without one it returns empty immediately, and
// with one it looks up the directory part (no import resolves to a bare
// directory here, so the lookup misses).
TEST_F(LSPSymbolReferenceIndexModuleKeyTests, PackageModuleFileReferencesRequirePathSeparator)
{
    std::vector<std::string> files = {"sri_pkg_base.ets", "sri_pkg_user.ets"};
    std::vector<std::string> texts = {R"(export let pkgValue: number = 1;)",
                                      R"(import { pkgValue } from './sri_pkg_base';
console.log(pkgValue);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *userCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(userCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(userCtx));

    // Non-package lookups keep resolving the import statement to the base file.
    const size_t literalStart = texts[1].find("'./sri_pkg_base'");
    ASSERT_NE(literalStart, std::string::npos);
    auto plainRefs = ark::es2panda::lsp::GetFileReferencesFromIndex(userCtx, filePaths[0], false);
    ASSERT_EQ(plainRefs.referenceInfos.size(), 1U);
    EXPECT_EQ(plainRefs.referenceInfos[0].fileName, filePaths[1]);
    EXPECT_EQ(plainRefs.referenceInfos[0].start, literalStart);
    EXPECT_EQ(plainRefs.referenceInfos[0].length, std::string("'./sri_pkg_base'").size());

    // Package-mode lookup of a separator-less name is rejected up front.
    auto noSeparator = ark::es2panda::lsp::GetFileReferencesFromIndex(userCtx, "plain-file-name.ets", true);
    EXPECT_TRUE(noSeparator.referenceInfos.empty());
    // Package-mode lookup strips the last path component; no import resolved to
    // the bare directory, so the directory key has no references.
    auto dirOnly = ark::es2panda::lsp::GetFileReferencesFromIndex(userCtx, filePaths[0], true);
    EXPECT_TRUE(dirOnly.referenceInfos.empty());

    initializer.DestroyContext(userCtx);
}

// An import whose module cannot be resolved must not corrupt the index: local
// symbols stay queryable and no entry is keyed by the dangling specifier.

TEST_F(LSPSymbolReferenceIndexModuleKeyTests, UnresolvedModuleImportContributesNoIndexEntry)
{
    std::vector<std::string> files = {"sri_missing_main.ets"};
    std::vector<std::string> texts = {R"(import { ghost } from './sri_missing_module_xyz';
let localOnly: number = 1;
console.log(localOnly);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(ctx));
    EXPECT_EQ(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]), texts[0]);

    // Local symbols unaffected by the unresolved import stay fully queryable.
    const auto defPos = texts[0].find("localOnly: number");
    auto localRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(ctx, defPos);
    ASSERT_EQ(localRefs.referenceInfos.size(), 1U);
    EXPECT_EQ(localRefs.definitionInfo.fileName, filePaths[0]);
    EXPECT_EQ(localRefs.definitionInfo.start, defPos);

    // The dangling module specifier contributes nothing: characterization -
    // an unresolved import is dropped before the file-reference stage, so no
    // index entry is keyed by its literal text.
    auto fileRefs = ark::es2panda::lsp::GetFileReferencesFromIndex(ctx, "./sri_missing_module_xyz", false);
    EXPECT_TRUE(fileRefs.referenceInfos.empty());
    auto byLiteralName = ark::es2panda::lsp::GetFileReferencesFromIndex(ctx, "sri_missing_main.ets", false);
    EXPECT_TRUE(byLiteralName.referenceInfos.empty());

    initializer.DestroyContext(ctx);
}

// Two modules exporting the same name keep their keys distinct: each aliased
// import forms its own module key scoped to the resolved module, so lookups
// never leak references across modules. Characterization: with aliased named
// imports the index tracks the alias spellings inside the importing file only;
// the exporting side keeps its own separate identity and no definitionInfo is
// attached to the alias key.
TEST_F(LSPSymbolReferenceIndexModuleKeyTests, SameNamedExportsFromDifferentModulesStayDistinct)
{
    // Index 2 selects sri_dup_main.ets, whose aliases must not leak across modules.
    constexpr size_t mainFileIndex = 2;

    std::vector<std::string> files = {"sri_dup_a.ets", "sri_dup_b.ets", "sri_dup_main.ets"};
    std::vector<std::string> texts = {R"(export let value = 10;
let localUse = value;)",
                                      R"(export let value = 20;
let localUseB = value;)",
                                      R"(import { value as valueA } from './sri_dup_a';
import { value as valueB } from './sri_dup_b';
console.log(valueA + valueB);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *mainCtx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(mainCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContextWithExternal(mainCtx));
    // Both modules are indexed through the external-program traversal.
    EXPECT_FALSE(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]).empty());
    EXPECT_FALSE(ark::es2panda::lsp::GetIndexedFileSource(filePaths[1]).empty());

    // Each alias occurs exactly twice in the main file: the import specifier
    // alias and the console.log usage.
    const auto aliasAPos = texts[2].find("valueA");
    const auto useAPos = texts[2].rfind("valueA");
    const auto aliasBPos = texts[2].find("valueB");
    const auto useBPos = texts[2].rfind("valueB");
    ASSERT_NE(aliasAPos, useAPos);
    ASSERT_NE(aliasBPos, useBPos);

    const auto posA = texts[2].find("console.log(valueA") + std::string("console.log(").size();
    auto refsA = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(mainCtx, posA);
    ASSERT_EQ(refsA.referenceInfos.size(), 2U);
    // Both occurrences live in the importing file; neither module leaks in.
    EXPECT_EQ(CountReferencesInFile(refsA, filePaths[mainFileIndex]), 2U);
    EXPECT_EQ(CountReferencesInFile(refsA, filePaths[0]), 0U);
    EXPECT_EQ(CountReferencesInFile(refsA, filePaths[1]), 0U);
    EXPECT_EQ(refsA.referenceInfos[0].fileName, filePaths[mainFileIndex]);
    EXPECT_EQ(refsA.referenceInfos[0].start, aliasAPos);
    EXPECT_EQ(refsA.referenceInfos[0].length, std::string("valueA").size());
    EXPECT_EQ(refsA.referenceInfos[1].fileName, filePaths[mainFileIndex]);
    EXPECT_EQ(refsA.referenceInfos[1].start, useAPos);
    EXPECT_EQ(refsA.referenceInfos[1].length, std::string("valueA").size());
    // The alias key carries no definition entry from the exporting module.
    EXPECT_TRUE(refsA.definitionInfo.fileName.empty());

    // Symmetric behaviour for the second same-named module.
    auto refsB = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(mainCtx, useBPos);
    ASSERT_EQ(refsB.referenceInfos.size(), 2U);
    EXPECT_EQ(CountReferencesInFile(refsB, filePaths[mainFileIndex]), 2U);
    EXPECT_EQ(CountReferencesInFile(refsB, filePaths[0]), 0U);
    EXPECT_EQ(CountReferencesInFile(refsB, filePaths[1]), 0U);
    EXPECT_EQ(refsB.referenceInfos[0].start, aliasBPos);
    EXPECT_EQ(refsB.referenceInfos[1].start, useBPos);
    EXPECT_TRUE(refsB.definitionInfo.fileName.empty());

    initializer.DestroyContext(mainCtx);
}

// A default import binds to the module's default export and resolves back to
// the originating declaration in the exporting file.
TEST_F(LSPSymbolReferenceIndexModuleKeyTests, DefaultImportResolvesToExportingDeclaration)
{
    std::vector<std::string> files = {"sri_def_mod.ets", "sri_def_main.ets"};
    std::vector<std::string> texts = {R"(export default class Shape {
    area(): number {
        return 1;
    }
})",
                                      R"(import Shape from './sri_def_mod';
let shape = new Shape();
console.log(shape.area());)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *mainCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(mainCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContextWithExternal(mainCtx));
    ASSERT_FALSE(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]).empty());

    const auto posUsage = texts[1].find("new Shape()") + std::string("new ").size();
    auto refs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(mainCtx, posUsage);
    ASSERT_GT(refs.referenceInfos.size(), 0U);
    ASSERT_FALSE(refs.definitionInfo.fileName.empty());
    EXPECT_EQ(refs.definitionInfo.fileName, filePaths[0]);
    EXPECT_EQ(refs.definitionInfo.start, texts[0].find("Shape"));
    EXPECT_EQ(refs.definitionInfo.length, std::string("Shape").size());

    initializer.DestroyContext(mainCtx);
}

// A namespace import contributes its own occurrence keyed by the "*" module
// key while named members still resolve into the exporting file.
TEST_F(LSPSymbolReferenceIndexModuleKeyTests, NamespaceImportResolvesMemberIntoOriginModule)
{
    std::vector<std::string> files = {"sri_ns_mod.ets", "sri_ns_main.ets"};
    std::vector<std::string> texts = {R"(export function nsHelper(): number {
    return 42;
})",
                                      R"(import * as helpers from './sri_ns_mod';
console.log(helpers.nsHelper());)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *mainCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(mainCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContextWithExternal(mainCtx));
    ASSERT_FALSE(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]).empty());

    // The namespaced call resolves to the helper declaration in the origin module.
    const auto posCall = texts[1].find("helpers.nsHelper()") + std::string("helpers.").size();
    auto refs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(mainCtx, posCall);
    ASSERT_GT(refs.referenceInfos.size(), 0U);
    EXPECT_EQ(refs.definitionInfo.fileName, filePaths[0]);
    EXPECT_EQ(refs.definitionInfo.start, texts[0].find("nsHelper"));

    initializer.DestroyContext(mainCtx);
}

// Exported classes/functions/properties surface declType and returnType through
// the definition-search APIs; functions without a return annotation fall back
// to the checker signature return type.
TEST_F(LSPSymbolReferenceIndexModuleKeyTests, ExportedMembersExposeDeclTypeAndReturnType)
{
    std::vector<std::string> files = {"sri_members_mod.ets", "sri_members_main.ets"};
    std::vector<std::string> texts = {R"(export class Repo {
    fetch(): string {
        return "data";
    }
    attempts: number = 0;
}
export function annotated(): number {
    return repoValue;
}
export function bare() {
    return repoValue;
}
let repoValue = 3;)",
                                      R"(import { Repo, annotated, bare } from './sri_members_mod';
let repo = new Repo();
repo.fetch();
annotated();
bare();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *mainCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(mainCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContextWithExternal(mainCtx));
    ASSERT_FALSE(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]).empty());

    auto byNameRepo = ark::es2panda::lsp::FindSymbolDefinitionsByName("Repo", "");
    ASSERT_EQ(byNameRepo.size(), 1U);
    EXPECT_EQ(byNameRepo[0].fileName, filePaths[0]);
    EXPECT_EQ(byNameRepo[0].symbolName, "Repo");
    EXPECT_FALSE(byNameRepo[0].isDefaultExport);

    // Top-level exported functions surface with their return types.
    auto annotatedEntries = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("annotated", "");
    ASSERT_EQ(annotatedEntries.size(), 1U);
    EXPECT_EQ(annotatedEntries[0].fileName, filePaths[0]);
    EXPECT_EQ(annotatedEntries[0].returnType, "number");

    // No explicit annotation: the signature fallback yields the checker's
    // inferred spelling of the return type.
    auto bareEntries = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("bare", "");
    ASSERT_EQ(bareEntries.size(), 1U);
    EXPECT_EQ(bareEntries[0].returnType, "Int");

    // Characterization: class MEMBERS (methods and properties) are not part of
    // the export definition search - only the class declaration itself is.
    auto fetchEntries = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("fetch", "");
    EXPECT_TRUE(fetchEntries.empty());
    auto attemptsEntries = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("attempts", "");
    EXPECT_TRUE(attemptsEntries.empty());

    initializer.DestroyContext(mainCtx);
}

// FindExportSymbolDefinitionsByPrefix honours excludeFile: entries whose
// definition lives in the excluded file are dropped, others survive.
TEST_F(LSPSymbolReferenceIndexModuleKeyTests, ExportPrefixSearchExcludesOwningFile)
{
    std::vector<std::string> files = {"sri_excl_mod.ets", "sri_excl_other.ets"};
    std::vector<std::string> texts = {R"(export let markerOne = 1;)", R"(export let markerTwo = 2;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *ctxA = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxA, nullptr);
    auto *ctxB = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxB, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(ctxA));
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(ctxB));

    // Without exclusion both exported markers are visible.
    auto all = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("marker", "");
    ASSERT_EQ(all.size(), 2U);

    // Excluding the owning file drops exactly that entry.
    auto withoutA = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("marker", filePaths[0]);
    ASSERT_EQ(withoutA.size(), 1U);
    EXPECT_EQ(withoutA[0].fileName, filePaths[1]);
    EXPECT_EQ(withoutA[0].symbolName, "markerTwo");

    auto withoutB = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("marker", filePaths[1]);
    ASSERT_EQ(withoutB.size(), 1U);
    EXPECT_EQ(withoutB[0].fileName, filePaths[0]);
    EXPECT_EQ(withoutB[0].symbolName, "markerOne");

    initializer.DestroyContext(ctxA);
    initializer.DestroyContext(ctxB);
}

// Remove followed by rebuild restores an identical reference set byte-for-byte.
TEST_F(LSPSymbolReferenceIndexModuleKeyTests, RemoveThenRebuildRestoresIdenticalReferenceSet)
{
    std::vector<std::string> files = {"sri_rr_base.ets", "sri_rr_user.ets"};
    std::vector<std::string> texts = {R"(export let rrValue: number = 1;
let rrLocal = rrValue;)",
                                      R"(import { rrValue } from './sri_rr_base';
console.log(rrValue);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *baseCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(baseCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(baseCtx));
    auto *userCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(userCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(userCtx));

    const auto defPos = texts[0].find("rrValue: number");
    auto before = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(baseCtx, defPos);
    ASSERT_EQ(before.referenceInfos.size(), 3U);

    // Removing both files empties the query results completely.
    ASSERT_TRUE(ark::es2panda::lsp::RemoveSymbolReferenceIndexForFile(filePaths[0]));
    ASSERT_TRUE(ark::es2panda::lsp::RemoveSymbolReferenceIndexForFile(filePaths[1]));
    auto emptied = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(baseCtx, defPos);
    EXPECT_TRUE(emptied.referenceInfos.empty());
    EXPECT_TRUE(emptied.definitionInfo.fileName.empty());
    // Removing again is a no-op reported as false.
    EXPECT_FALSE(ark::es2panda::lsp::RemoveSymbolReferenceIndexForFile(filePaths[1]));

    // Rebuilding in the same order reproduces the exact pre-remove reference set.
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(baseCtx));
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(userCtx));
    auto rebuilt = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(baseCtx, defPos);
    ASSERT_EQ(rebuilt.referenceInfos.size(), before.referenceInfos.size());
    for (size_t i = 0; i < before.referenceInfos.size(); i++) {
        EXPECT_EQ(rebuilt.referenceInfos[i].fileName, before.referenceInfos[i].fileName);
        EXPECT_EQ(rebuilt.referenceInfos[i].start, before.referenceInfos[i].start);
        EXPECT_EQ(rebuilt.referenceInfos[i].length, before.referenceInfos[i].length);
    }
    EXPECT_EQ(rebuilt.definitionInfo.fileName, before.definitionInfo.fileName);
    EXPECT_EQ(rebuilt.definitionInfo.start, before.definitionInfo.start);
    EXPECT_EQ(rebuilt.definitionInfo.length, before.definitionInfo.length);

    initializer.DestroyContext(baseCtx);
    initializer.DestroyContext(userCtx);
}
}  // namespace
