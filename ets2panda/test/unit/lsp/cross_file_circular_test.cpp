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
#include <filesystem>

#include "lsp_api_test.h"
#include "lsp/include/find_references.h"
#include "lsp/include/find_rename_locations.h"
#include "lsp/include/get_definition_and_bound_span.h"
#include "lsp/include/cancellation_token.h"

namespace {

using ark::es2panda::SourceFile;
using ark::es2panda::lsp::CancellationToken;
using ark::es2panda::lsp::DefinitionInfoAndBoundSpan;
using ark::es2panda::lsp::GetDefinitionAndBoundSpan;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::ReferencedNode;
using ark::es2panda::lsp::RenameLocation;

// Named constants for CancellationToken IDs
constexpr size_t CANCEL_TOKEN_ID = 123;

// Named origin indices
constexpr size_t FILE_A_IDX = 0;
constexpr size_t FILE_B_IDX = 1;
constexpr size_t FILE_C_IDX = 2;
constexpr size_t FILE_D_IDX = 3;

// Named string constants for symbol lengths
const std::string ALPHA_NAME = "alpha";
const std::string SHARED_FUNC_NAME = "sharedFunc";
const std::string SELF_FUNC_NAME = "selfFunc";
const std::string ORIGIN_CLASS_NAME = "OriginClass";
const std::string TARGET_NAME = "Target";
const std::string CONFLICT_NAME = "conflict";

// Helper to compute line number from source and position
static size_t GetLine(const std::string &source, size_t pos)
{
    size_t line = 0;
    for (size_t i = 0; i < pos && i < source.size(); ++i) {
        if (source[i] == '\n') {
            ++line;
        }
    }
    return line;
}

// Helper to create expected ReferencedNode with computed line
static ReferencedNode makeRef(const std::string &filePath, size_t start, size_t end, bool isDef,
                              const std::string &source)
{
    return {filePath, start, end, GetLine(source, start), isDef};
}

// Helper to create expected RenameLocation with computed line
static RenameLocation makeRename(const std::string &fileName, size_t start, size_t end, const std::string &source)
{
    return {fileName, start, end, GetLine(source, start), std::nullopt, std::nullopt};
}

void VerifyRenameLocations(const std::set<RenameLocation> &locations, const std::set<RenameLocation> &expected,
                           const std::string &expectedFile)
{
    ASSERT_EQ(locations.size(), expected.size());
    for (const auto &location : locations) {
        EXPECT_NE(expected.find(location), expected.end());
        EXPECT_EQ(location.fileName, expectedFile);
    }
}

class LspCrossFileCircularTests : public LSPAPITests {};

/* P1D1-CIRC-01: FindReferences_CircularImport_NotDeadlock
   A↔B circular import: A imports from B, B imports from A. FindReferences on a symbol
   should not deadlock and return correct references from both files. */
// Circular A<->B import topology; upstream find_references tests use linear topologies.
// The self-import (A->A) case below is unique in the tree.
TEST_F(LspCrossFileCircularTests, FindReferences_CircularImport_NotDeadlock)
{
    const std::vector<std::string> files = {"circularA.ets", "circularB.ets"};
    const std::vector<std::string> texts = {
        R"(
import { sharedFunc } from "./circularB.ets";

export function alpha(x: number): void {}
sharedFunc(1);
)",
        R"(
import { alpha } from "./circularA.ets";

export function sharedFunc(x: number): void {}
alpha(2);
)"};

    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    std::vector<SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(SourceFile {filePaths[i], texts[i]});
    }

    CancellationToken token {CANCEL_TOKEN_ID, nullptr};

    // Test 1: FindReferences on 'alpha' definition in circularA.ets (offset at 'alpha' in export)
    size_t offsetAlphaDef = texts[FILE_A_IDX].find(ALPHA_NAME);
    ASSERT_NE(offsetAlphaDef, std::string::npos);

    auto refsAlpha = FindReferences(&token, sourceFiles, sourceFiles[FILE_A_IDX], offsetAlphaDef);

    // Expected: definition in A, call in B (import specifier + call)
    std::set<ReferencedNode> expectedAlpha = {
        makeRef(filePaths[FILE_A_IDX], offsetAlphaDef, offsetAlphaDef + ALPHA_NAME.length(), true, texts[FILE_A_IDX]),
        makeRef(filePaths[FILE_B_IDX], texts[FILE_B_IDX].find(ALPHA_NAME),
                texts[FILE_B_IDX].find(ALPHA_NAME) + ALPHA_NAME.length(), false, texts[FILE_B_IDX]),
        makeRef(filePaths[FILE_B_IDX], texts[FILE_B_IDX].rfind(ALPHA_NAME),
                texts[FILE_B_IDX].rfind(ALPHA_NAME) + ALPHA_NAME.length(), false, texts[FILE_B_IDX])};
    ASSERT_EQ(refsAlpha.size(), expectedAlpha.size());
    for (const auto &ref : refsAlpha) {
        ASSERT_NE(expectedAlpha.find(ref), expectedAlpha.end());
    }

    // Test 2: FindReferences on 'sharedFunc' definition in circularB.ets
    size_t offsetSharedDef = texts[FILE_B_IDX].find(SHARED_FUNC_NAME);
    ASSERT_NE(offsetSharedDef, std::string::npos);

    auto refsShared = FindReferences(&token, sourceFiles, sourceFiles[FILE_B_IDX], offsetSharedDef);

    // Expected: definition in B, import+call in A
    std::set<ReferencedNode> expectedShared = {
        makeRef(filePaths[FILE_B_IDX], offsetSharedDef, offsetSharedDef + SHARED_FUNC_NAME.length(), true,
                texts[FILE_B_IDX]),
        makeRef(filePaths[FILE_A_IDX], texts[FILE_A_IDX].find(SHARED_FUNC_NAME),
                texts[FILE_A_IDX].find(SHARED_FUNC_NAME) + SHARED_FUNC_NAME.length(), false, texts[FILE_A_IDX]),
        makeRef(filePaths[FILE_A_IDX], texts[FILE_A_IDX].rfind(SHARED_FUNC_NAME),
                texts[FILE_A_IDX].rfind(SHARED_FUNC_NAME) + SHARED_FUNC_NAME.length(), false, texts[FILE_A_IDX])};
    ASSERT_EQ(refsShared.size(), expectedShared.size());
    for (const auto &ref : refsShared) {
        ASSERT_NE(expectedShared.find(ref), expectedShared.end());
    }
}

/* P1D1-CIRC-02: FindReferences_SelfImportingFile
   A→A self-import. FindReferences should not infinite-loop and return finite references. */
TEST_F(LspCrossFileCircularTests, FindReferences_SelfImportingFile)
{
    const std::vector<std::string> files = {"selfImport.ets"};
    const std::vector<std::string> texts = {
        R"(
import { selfFunc } from "./selfImport.ets";

export function selfFunc(x: number): void {}
selfFunc(1);
)"};

    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    std::vector<SourceFile> sourceFiles;
    sourceFiles.emplace_back(SourceFile {filePaths[FILE_A_IDX], texts[FILE_A_IDX]});

    CancellationToken token {CANCEL_TOKEN_ID, nullptr};

    // FindReferences on 'selfFunc' definition
    size_t offsetDef = texts[FILE_A_IDX].find(SELF_FUNC_NAME);
    ASSERT_NE(offsetDef, std::string::npos);

    auto refs = FindReferences(&token, sourceFiles, sourceFiles[FILE_A_IDX], offsetDef);

    // Expected finite references: definition + import specifier + call (3 references)
    const size_t importPos = texts[FILE_A_IDX].find(SELF_FUNC_NAME);
    const std::string defMarker = "export function ";
    const size_t defPos = texts[FILE_A_IDX].find(defMarker) + defMarker.size();
    const size_t callPos = texts[FILE_A_IDX].rfind(SELF_FUNC_NAME);
    std::set<ReferencedNode> expected = {
        makeRef(filePaths[FILE_A_IDX], defPos, defPos + SELF_FUNC_NAME.length(), true, texts[FILE_A_IDX]),
        makeRef(filePaths[FILE_A_IDX], importPos, importPos + SELF_FUNC_NAME.length(), false, texts[FILE_A_IDX]),
        makeRef(filePaths[FILE_A_IDX], callPos, callPos + SELF_FUNC_NAME.length(), false, texts[FILE_A_IDX])};
    ASSERT_EQ(refs.size(), expected.size());
    for (const auto &ref : refs) {
        ASSERT_NE(expected.find(ref), expected.end());
    }
}

/* P1D1-CIRC-03: FindRenameLocations_CircularImport_NoInfiniteLoop
   A↔B circular import. FindRenameLocations should not infinite-loop and return
   exact rename locations across both files. */
// Circular-import dimension for rename with exact 3-location sets.
TEST_F(LspCrossFileCircularTests, FindRenameLocations_CircularImport_NoInfiniteLoop)
{
    const std::vector<std::string> files = {"renameA.ets", "renameB.ets"};
    const std::vector<std::string> texts = {
        R"(
import { sharedFunc } from "./renameB.ets";

export function alpha(x: number): void {}
sharedFunc(1);
)",
        R"(
import { alpha } from "./renameA.ets";

export function sharedFunc(x: number): void {}
alpha(2);
)"};

    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer init;
    std::vector<es2panda_Context *> fileContexts;
    for (const auto &path : filePaths) {
        auto ctx = init.CreateContext(path.c_str(), ES2PANDA_STATE_CHECKED);
        ASSERT_NE(ctx, nullptr);
        fileContexts.push_back(ctx);
    }

    CancellationToken token {CANCEL_TOKEN_ID, nullptr};

    // Rename 'alpha' from its definition in A
    size_t offsetAlphaDef = texts[FILE_A_IDX].find(ALPHA_NAME);
    ASSERT_NE(offsetAlphaDef, std::string::npos);

    auto locations = FindRenameLocations(&token, fileContexts, fileContexts[FILE_A_IDX], offsetAlphaDef);

    // Expected: definition in A, import + call in B (3 locations)
    std::set<RenameLocation> expected = {
        makeRename(filePaths[FILE_A_IDX], offsetAlphaDef, offsetAlphaDef + ALPHA_NAME.length(), texts[FILE_A_IDX]),
        makeRename(filePaths[FILE_B_IDX], texts[FILE_B_IDX].find(ALPHA_NAME),
                   texts[FILE_B_IDX].find(ALPHA_NAME) + ALPHA_NAME.length(), texts[FILE_B_IDX]),
        makeRename(filePaths[FILE_B_IDX], texts[FILE_B_IDX].rfind(ALPHA_NAME),
                   texts[FILE_B_IDX].rfind(ALPHA_NAME) + ALPHA_NAME.length(), texts[FILE_B_IDX])};
    ASSERT_EQ(locations.size(), expected.size());
    for (const auto &loc : locations) {
        ASSERT_NE(expected.find(loc), expected.end());
    }

    for (auto ctx : fileContexts) {
        init.DestroyContext(ctx);
    }
}

/* P1D1-CIRC-04: GetDefinition_CircularReExport_FindsOrigin
   Circular re-export: A re-exports from B, B re-exports from A, but original
   definition is in A. GetDefinition should find the actual origin (A), not loop. */
// Circular re-export graph vs upstream linear ReExportChainDefinition.
TEST_F(LspCrossFileCircularTests, GetDefinition_CircularReExport_FindsOrigin)
{
    const std::vector<std::string> files = {"reexportA.ets", "reexportB.ets", "reexportC.ets"};
    const std::vector<std::string> texts = {
        R"(
export class OriginClass {
    value: number = 42;
}

export { sharedFunc } from "./reexportB.ets";
)",
        R"(
export { OriginClass } from "./reexportA.ets";

export function sharedFunc(x: number): void {}
)",
        R"(
import { OriginClass } from "./reexportA.ets";

let instance = new OriginClass();
)"};

    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer init;
    // Create context for consumer file C
    auto ctxC = init.CreateContext(filePaths[FILE_C_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxC, nullptr);

    // Position at 'OriginClass' in 'new OriginClass()' in file C
    size_t offset = texts[FILE_C_IDX].rfind(ORIGIN_CLASS_NAME);
    ASSERT_NE(offset, std::string::npos);

    auto result = GetDefinitionAndBoundSpan(ctxC, offset);

    // Should resolve to origin in reexportA.ets (definition of OriginClass)
    EXPECT_FALSE(result.definitionInfo.fileName.empty());
    EXPECT_EQ(result.definitionInfo.fileName, filePaths[FILE_A_IDX]);
    // Definition at 'class OriginClass' - find 'OriginClass' in file A
    size_t expectedDefStart = texts[FILE_A_IDX].find(ORIGIN_CLASS_NAME);
    ASSERT_NE(expectedDefStart, std::string::npos);
    EXPECT_EQ(result.definitionInfo.start, expectedDefStart);
    EXPECT_EQ(result.definitionInfo.length, ORIGIN_CLASS_NAME.length());

    // Bound span should cover the usage in file C
    EXPECT_EQ(result.boundSpan.start, offset);
    EXPECT_EQ(result.boundSpan.length, ORIGIN_CLASS_NAME.length());

    init.DestroyContext(ctxC);
}

/* P1D1-CIRC-07: GetDefinition_BarrelFileChain
   A → index → B → C chain. Construct exactly and assert origin file/span and bound span. */
// 3-hop named `export {X} from` chain vs upstream 2-hop star re-export.
TEST_F(LspCrossFileCircularTests, GetDefinition_BarrelFileChain)
{
    const std::vector<std::string> files = {"barrelA.ets", "barrelIndex.ets", "barrelB.ets", "barrelC.ets"};
    const std::vector<std::string> texts = {R"(import { Target } from "./barrelIndex.ets"; let a = new Target();)",
                                            R"(export { Target } from "./barrelB.ets";)",
                                            R"(export { Target } from "./barrelC.ets";)",
                                            R"(export class Target { value: number = 100; })"};

    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer init;
    // Context for file A (consumer)
    auto ctxA = init.CreateContext(filePaths[FILE_A_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxA, nullptr);

    // Position at 'Target' in 'new Target()' in file A
    size_t offset = texts[FILE_A_IDX].rfind(TARGET_NAME);
    ASSERT_NE(offset, std::string::npos);

    auto result = GetDefinitionAndBoundSpan(ctxA, offset);

    // Should resolve to origin in barrelC.ets (actual class definition)
    EXPECT_FALSE(result.definitionInfo.fileName.empty());
    EXPECT_EQ(result.definitionInfo.fileName, filePaths[FILE_D_IDX]);

    size_t expectedDefStart = texts[FILE_D_IDX].find(TARGET_NAME);
    ASSERT_NE(expectedDefStart, std::string::npos);
    EXPECT_EQ(result.definitionInfo.start, expectedDefStart);
    EXPECT_EQ(result.definitionInfo.length, TARGET_NAME.length());

    // Bound span at usage in file A
    EXPECT_EQ(result.boundSpan.start, offset);
    EXPECT_EQ(result.boundSpan.length, TARGET_NAME.length());

    init.DestroyContext(ctxA);
}

/* P1D1-CIRC-08: FindRenameLocations_ConflictingNamesAcrossFiles
   Two independent files with same-named declarations. Rename locations should
   be restricted to the file containing the selected symbol. */
// Rename scoping across independent same-name files; the references-only variant
// (MultiFileSameNameSymbolsDoNotCross) does not cover FindRenameLocations.
TEST_F(LspCrossFileCircularTests, FindRenameLocations_ConflictingNamesAcrossFiles)
{
    const std::vector<std::string> files = {"conflictA.ets", "conflictB.ets"};
    const std::vector<std::string> texts = {
        R"(
export function conflict(x: number): number { return x + 1; }
conflict(1);
conflict(2);
)",
        R"(
export function conflict(x: string): string { return x + "!"; }
conflict("a");
)"};

    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer init;
    std::vector<es2panda_Context *> fileContexts;
    for (const auto &path : filePaths) {
        auto ctx = init.CreateContext(path.c_str(), ES2PANDA_STATE_CHECKED);
        ASSERT_NE(ctx, nullptr);
        fileContexts.push_back(ctx);
    }

    CancellationToken token {CANCEL_TOKEN_ID, nullptr};

    // Rename 'conflict' in file A at its definition
    size_t offsetA = texts[FILE_A_IDX].find(CONFLICT_NAME);
    ASSERT_NE(offsetA, std::string::npos);

    auto locations = FindRenameLocations(&token, fileContexts, fileContexts[FILE_A_IDX], offsetA);

    // Expected: 3 locations in file A only (def + 2 calls), none in file B
    std::set<RenameLocation> expectedA = {
        makeRename(filePaths[FILE_A_IDX], offsetA, offsetA + CONFLICT_NAME.length(), texts[FILE_A_IDX]),
        makeRename(filePaths[FILE_A_IDX], texts[FILE_A_IDX].find(CONFLICT_NAME, offsetA + CONFLICT_NAME.length()),
                   texts[FILE_A_IDX].find(CONFLICT_NAME, offsetA + CONFLICT_NAME.length()) + CONFLICT_NAME.length(),
                   texts[FILE_A_IDX]),
        makeRename(filePaths[FILE_A_IDX], texts[FILE_A_IDX].rfind(CONFLICT_NAME),
                   texts[FILE_A_IDX].rfind(CONFLICT_NAME) + CONFLICT_NAME.length(), texts[FILE_A_IDX])};
    VerifyRenameLocations(locations, expectedA, filePaths[FILE_A_IDX]);

    // Now rename in file B - should get only B's locations
    size_t offsetB = texts[FILE_B_IDX].find(CONFLICT_NAME);
    ASSERT_NE(offsetB, std::string::npos);

    auto locationsB = FindRenameLocations(&token, fileContexts, fileContexts[FILE_B_IDX], offsetB);

    std::set<RenameLocation> expectedB = {
        makeRename(filePaths[FILE_B_IDX], offsetB, offsetB + CONFLICT_NAME.length(), texts[FILE_B_IDX]),
        makeRename(filePaths[FILE_B_IDX], texts[FILE_B_IDX].rfind(CONFLICT_NAME),
                   texts[FILE_B_IDX].rfind(CONFLICT_NAME) + CONFLICT_NAME.length(), texts[FILE_B_IDX])};
    VerifyRenameLocations(locationsB, expectedB, filePaths[FILE_B_IDX]);

    for (auto ctx : fileContexts) {
        init.DestroyContext(ctx);
    }
}

}  // namespace
