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

#include "lsp_api_test.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"
#include "public/public.h"

namespace {

using ark::es2panda::lsp::Initializer;

class LspApiWrapperPathsTests : public LSPAPITests {};

// Query offset pushed past the end of the source to exercise the null-token defensive path.
constexpr size_t BEYOND_EOF_POSITION_PADDING = 100;

// Ordinal index of the third element: the last file of the three-file fixtures below
// and the third entry of the sorted node-info / reference vectors.
constexpr size_t THIRD_ELEMENT_INDEX = 2;

// getNodeInfosByDefinitionData resolves fileName against the main program first: the exact main
// file path matches it, a nullptr view and an unknown name both fall back to the main program, so
// all three variants produce identical node-info chains.
TEST_F(LspApiWrapperPathsTests, GetNodeInfosByDefinitionDataResolvesMainProgramByFileName)
{
    std::vector<std::string> files = {"WrapperInfosExport.ets", "WrapperInfosImport.ets"};
    std::vector<std::string> texts = {"export class Alpha {\n    method(): void {}\n}\n",
                                      R"(import { Alpha } from './WrapperInfosExport';

let al: Alpha = new Alpha();
al.method();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();

    const std::string &source = texts[1];
    const auto queryPos = source.find("al.method");
    ASSERT_NE(queryPos, std::string::npos);

    auto byNullName = lspApi->getNodeInfosByDefinitionData(context, nullptr, queryPos);
    auto byMainName = lspApi->getNodeInfosByDefinitionData(context, filePaths[1].c_str(), queryPos);
    auto byUnknownName = lspApi->getNodeInfosByDefinitionData(context, "unknown_wrapper_file.ets", queryPos);

    auto nullContextResult = lspApi->getNodeInfosByDefinitionData(nullptr, filePaths[1].c_str(), queryPos);
    auto beyondEof = lspApi->getNodeInfosByDefinitionData(context, filePaths[1].c_str(),
                                                          source.size() + BEYOND_EOF_POSITION_PADDING);
    initializer.DestroyContext(context);

    ASSERT_EQ(byNullName.size(), 3U);
    EXPECT_EQ(byNullName[0].name, "ETSGLOBAL");
    EXPECT_EQ(byNullName[1].name, "method");
    EXPECT_EQ(byNullName[THIRD_ELEMENT_INDEX].name, "method");

    ASSERT_EQ(byMainName.size(), byNullName.size());
    for (size_t i = 0; i < byNullName.size(); i++) {
        EXPECT_EQ(byMainName[i].name, byNullName[i].name);
        EXPECT_EQ(byMainName[i].kind, byNullName[i].kind);
    }
    ASSERT_EQ(byUnknownName.size(), byNullName.size());
    for (size_t i = 0; i < byNullName.size(); i++) {
        EXPECT_EQ(byUnknownName[i].name, byNullName[i].name);
        EXPECT_EQ(byUnknownName[i].kind, byNullName[i].kind);
    }

    EXPECT_TRUE(nullContextResult.empty());
    EXPECT_TRUE(beyondEof.empty());
}

// A declaration (.d.ets) import is registered as a SOURCE_DECL external program; passing its
// absolute path makes GetNodeInfosByDefinitionData search the touching token inside that
// external program AST instead of the importer.
TEST_F(LspApiWrapperPathsTests, GetNodeInfosByDefinitionDataReadsExternalDeclarationProgram)
{
    std::vector<std::string> files = {"WrapperDeclExternal.d.ets", "WrapperDeclImporter.ets"};
    std::vector<std::string> texts = {"export class Delta {\n    run(): void {}\n}\n",
                                      R"(import { Delta } from './WrapperDeclExternal';

let d: Delta = new Delta();
d.run();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // The import must really be registered as a SOURCE_DECL external program.
    auto *parserProgram = reinterpret_cast<ark::es2panda::public_lib::Context *>(context)->parserProgram;
    const auto &externalSources =
        parserProgram->GetExternalDecls()->Get<ark::es2panda::util::ModuleKind::SOURCE_DECL>();
    ASSERT_EQ(externalSources.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    const auto queryPos = texts[0].find("run");
    ASSERT_NE(queryPos, std::string::npos);
    auto byExternal = lspApi->getNodeInfosByDefinitionData(context, filePaths[0].c_str(), queryPos);
    initializer.DestroyContext(context);

    ASSERT_EQ(byExternal.size(), 2U);
    EXPECT_EQ(byExternal[0].name, "Delta");
    EXPECT_EQ(byExternal[1].name, "run");
}

// The full public chain getNodeInfosByDefinitionData -> getDefinitionDataFromNode /
// findRenameLocationsFromNode recovers the member-expression property span of `al.method`.
TEST_F(LspApiWrapperPathsTests, NodeInfoChainRoundTripsToDefinitionAndRenameLocation)
{
    std::vector<std::string> files = {"WrapperChainExport.ets", "WrapperChainImport.ets"};
    std::vector<std::string> texts = {"export class Alpha {\n    method(): void {}\n}\n",
                                      R"(import { Alpha } from './WrapperChainExport';

let al: Alpha = new Alpha();
al.method();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();

    const std::string &source = texts[1];
    const auto queryPos = source.find("al.method");
    ASSERT_NE(queryPos, std::string::npos);

    auto nodeInfos = lspApi->getNodeInfosByDefinitionData(context, nullptr, queryPos);
    ASSERT_FALSE(nodeInfos.empty());

    std::vector<NodeInfo> mutableInfos = nodeInfos;
    std::vector<NodeInfo *> infoPointers;
    infoPointers.reserve(mutableInfos.size());
    for (auto &nodeInfo : mutableInfos) {
        infoPointers.push_back(&nodeInfo);
    }

    const auto definition = lspApi->getDefinitionDataFromNode(context, infoPointers);
    const auto renameLocation = lspApi->findRenameLocationsFromNode(context, infoPointers);
    initializer.DestroyContext(context);

    const size_t methodStart = source.find(".method") + 1U;
    const size_t methodLength = std::string("method").size();
    EXPECT_TRUE(definition.fileName.empty());
    EXPECT_EQ(definition.start, methodStart);
    EXPECT_EQ(definition.length, methodLength);
    EXPECT_TRUE(renameLocation.fileName.empty());
    EXPECT_EQ(renameLocation.start, methodStart);
    EXPECT_EQ(renameLocation.end, methodStart + methodLength);
    EXPECT_EQ(renameLocation.line, methodLength);
}

// Defensive paths of findRenameLocationsFromNode: null context, empty chain, and a chain whose
// kind has no node matcher must all return the zeroed location without crashing.
TEST_F(LspApiWrapperPathsTests, FindRenameLocationsFromNodeDefensivePaths)
{
    Initializer initializer;
    auto *context = initializer.CreateContext("FindRenameLocationsFromNodeDefensivePaths.ets", ES2PANDA_STATE_PARSED,
                                              "let value = 1;\n");
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();

    const ark::es2panda::lsp::RenameLocation zeroed {"", 0U, 0U, 0U};

    auto fromNullContext = lspApi->findRenameLocationsFromNode(nullptr, {});
    EXPECT_EQ(fromNullContext.fileName, zeroed.fileName);
    EXPECT_EQ(fromNullContext.start, zeroed.start);
    EXPECT_EQ(fromNullContext.end, zeroed.end);
    EXPECT_EQ(fromNullContext.line, zeroed.line);

    auto fromEmptyInfos = lspApi->findRenameLocationsFromNode(context, {});
    EXPECT_EQ(fromEmptyInfos.start, zeroed.start);
    EXPECT_EQ(fromEmptyInfos.end, zeroed.end);
    EXPECT_EQ(fromEmptyInfos.line, zeroed.line);

    // TS_ANY_KEYWORD has no entry in the node matcher table, so no child can match.
    std::vector<NodeInfo> unmatchedInfos {NodeInfo {"value", ark::es2panda::ir::AstNodeType::TS_ANY_KEYWORD}};
    std::vector<NodeInfo *> unmatchedPointers {&unmatchedInfos[0]};
    auto fromUnmatchedKind = lspApi->findRenameLocationsFromNode(context, unmatchedPointers);
    EXPECT_EQ(fromUnmatchedKind.start, zeroed.start);
    EXPECT_EQ(fromUnmatchedKind.end, zeroed.end);
    EXPECT_EQ(fromUnmatchedKind.line, zeroed.line);

    // A valid first link followed by an unmatchable second link aborts mid-chain with zeros too.
    std::vector<NodeInfo> mixedInfos {NodeInfo {"value", ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATOR},
                                      NodeInfo {"value", ark::es2panda::ir::AstNodeType::TS_ANY_KEYWORD}};
    std::vector<NodeInfo *> mixedPointers {&mixedInfos[0], &mixedInfos[1]};
    auto fromMixedChain = lspApi->findRenameLocationsFromNode(context, mixedPointers);
    initializer.DestroyContext(context);
    EXPECT_EQ(fromMixedChain.start, zeroed.start);
    EXPECT_EQ(fromMixedChain.end, zeroed.end);
    EXPECT_EQ(fromMixedChain.line, zeroed.line);
}

// getFileReferences aggregates the string-literal spans of every import statement that resolves
// to the searched file across all indexed importers, deduplicated and sorted by file name then
// offset. Null arguments return empty results instead of crashing.
TEST_F(LspApiWrapperPathsTests, GetFileReferencesAggregatesCrossFileImportsSorted)
{
    std::vector<std::string> files = {"FileRefsTarget.ets", "FileRefsImporterA.ets", "FileRefsImporterB.ets"};
    std::vector<std::string> texts = {"export class Sigma {}\n",
                                      R"(import { Sigma } from './FileRefsTarget';
let s: Sigma = new Sigma();)",
                                      R"(import { Sigma } from './FileRefsTarget';
import { Sigma as Tau } from './FileRefsTarget';
let t: Tau = new Tau();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *contextA = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(contextA, nullptr);
    auto *contextB = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(contextB, nullptr);
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(contextA));
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(contextB));

    auto references = lspApi->getFileReferences(filePaths[0].c_str(), contextA, false);

    auto nullNameReferences = lspApi->getFileReferences(nullptr, contextA, false);
    auto indexReferencesNullContext = lspApi->getReferencesAtPositionFromIndex(nullptr, 0U);
    const bool removedNullFile = lspApi->removeSymbolReferenceIndexForFile(nullptr);
    initializer.DestroyContext(contextA);
    initializer.DestroyContext(contextB);

    ASSERT_EQ(references.referenceInfos.size(), 3U);
    EXPECT_EQ(references.referenceInfos[0].fileName, filePaths[1]);
    EXPECT_EQ(references.referenceInfos[1].fileName, filePaths[THIRD_ELEMENT_INDEX]);
    EXPECT_EQ(references.referenceInfos[THIRD_ELEMENT_INDEX].fileName, filePaths[THIRD_ELEMENT_INDEX]);
    // Sorted by start inside the same importing file.
    EXPECT_LT(references.referenceInfos[1].start, references.referenceInfos[THIRD_ELEMENT_INDEX].start);
    for (const auto &ref : references.referenceInfos) {
        // Each span covers the quoted module specifier './FileRefsTarget'.
        EXPECT_EQ(ref.length, std::string("'./FileRefsTarget'").size());
    }
    EXPECT_EQ(references.referenceInfos[1].start, texts[THIRD_ELEMENT_INDEX].find("'./FileRefsTarget'"));
    EXPECT_EQ(references.referenceInfos[THIRD_ELEMENT_INDEX].start,
              texts[THIRD_ELEMENT_INDEX].find("'./FileRefsTarget'", references.referenceInfos[1].start + 1U));
    EXPECT_EQ(references.referenceInfos[0].start, texts[1].find("'./FileRefsTarget'"));

    EXPECT_TRUE(nullNameReferences.referenceInfos.empty());
    EXPECT_TRUE(indexReferencesNullContext.referenceInfos.empty());
    EXPECT_FALSE(removedNullFile);
}

}  // namespace
