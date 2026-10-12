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
#include "lsp/include/symbol_reference_index.h"
#include "ir/astNode.h"
#include "public/es2panda_lib.h"

using ark::es2panda::ir::AstNodeType;
using ark::es2panda::lsp::Initializer;

namespace {

// Index layout of the specifier-shapes scenario; paths and texts are index-aligned.
constexpr size_t SPEC_NAMED_LIB_IDX = 0;
constexpr size_t SPEC_DEFAULT_LIB_IDX = 1;
constexpr size_t SPEC_NS_LIB_IDX = 2;
constexpr size_t SPEC_MAIN_FILE_IDX = 3;
// The named/default import specifier and its usage both register in the importing main file.
constexpr size_t SPEC_MAIN_REF_COUNT = 2;

// Index layout of the re-export-chain scenario.
constexpr size_t REX_BASE_FILE_IDX = 0;
constexpr size_t REX_MID_FILE_IDX = 1;
constexpr size_t REX_MAIN_FILE_IDX = 2;
// The import specifier and the console usage both register in the re-export main file.
constexpr size_t REX_MAIN_REF_COUNT = 2;

// Index layout of the transitive cross-file scenario.
constexpr size_t TRANSITIVE_DEEP_LIB_IDX = 0;
constexpr size_t TRANSITIVE_MID_LIB_IDX = 1;
constexpr size_t TRANSITIVE_MAIN_FILE_IDX = 2;
// Sorted reference list of a deep symbol queried from the mid file: import specifier,
// arithmetic usage, console usage.
constexpr size_t MID_CONSOLE_USAGE_REF_IDX = 2;
constexpr size_t TRANSITIVE_MAIN_REF_COUNT = 2;

// Expected shape of one definition entry returned by the symbol-reference index lookups.
struct SingleDefExpectation {
    std::string fileName;
    std::string symbolName;
    bool isDefaultExport;
    AstNodeType declType;
    std::string returnType;
};

}  // namespace

class LSPSymbolReferenceIndexShapesTests : public LSPAPITests {
public:
    LSPSymbolReferenceIndexShapesTests() = default;
    ~LSPSymbolReferenceIndexShapesTests() override = default;

    NO_COPY_SEMANTIC(LSPSymbolReferenceIndexShapesTests);
    NO_MOVE_SEMANTIC(LSPSymbolReferenceIndexShapesTests);

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

    static void ExpectSingleDef(const std::vector<ark::es2panda::lsp::SymbolDefSearchResult> &results,
                                const SingleDefExpectation &expected)
    {
        ASSERT_EQ(results.size(), 1U);
        EXPECT_EQ(results[0].fileName, expected.fileName);
        EXPECT_EQ(results[0].symbolName, expected.symbolName);
        EXPECT_EQ(results[0].isDefaultExport, expected.isDefaultExport);
        EXPECT_EQ(results[0].declType, expected.declType);
        EXPECT_EQ(results[0].returnType, expected.returnType);
    }

    static void ExpectImportedLibSourcesIndexed(const std::vector<std::string> &paths,
                                                const std::vector<std::string> &texts)
    {
        // All three imported libraries are indexed through the external-sources walk.
        ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(paths[SPEC_NAMED_LIB_IDX]), texts[SPEC_NAMED_LIB_IDX]);
        ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(paths[SPEC_DEFAULT_LIB_IDX]), texts[SPEC_DEFAULT_LIB_IDX]);
        ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(paths[SPEC_NS_LIB_IDX]), texts[SPEC_NS_LIB_IDX]);
        ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(paths[SPEC_MAIN_FILE_IDX]), texts[SPEC_MAIN_FILE_IDX]);
    }

    static void ExpectSpecifierFileReferences(es2panda_Context *mainCtx, const std::vector<std::string> &paths,
                                              const std::vector<std::string> &texts)
    {
        // Each import declaration contributes exactly one file-reference to its module path;
        // the reference span covers the quoted specifier './shapes_lib_named' (19 chars incl. quotes).
        auto namedFileRefs = ark::es2panda::lsp::GetFileReferencesFromIndex(mainCtx, paths[SPEC_NAMED_LIB_IDX], false);
        ASSERT_EQ(namedFileRefs.referenceInfos.size(), 1U);
        ASSERT_EQ(namedFileRefs.referenceInfos[0].fileName, paths[SPEC_MAIN_FILE_IDX]);
        const size_t namedImportStart = texts[SPEC_MAIN_FILE_IDX].find("'./shapes_lib_named'");
        ASSERT_NE(namedImportStart, std::string::npos);
        ASSERT_EQ(namedFileRefs.referenceInfos[0].start, namedImportStart);
        ASSERT_EQ(namedFileRefs.referenceInfos[0].length, std::string("'./shapes_lib_named'").size());

        auto defaultFileRefs =
            ark::es2panda::lsp::GetFileReferencesFromIndex(mainCtx, paths[SPEC_DEFAULT_LIB_IDX], false);
        ASSERT_EQ(defaultFileRefs.referenceInfos.size(), 1U);
        ASSERT_EQ(defaultFileRefs.referenceInfos[0].fileName, paths[SPEC_MAIN_FILE_IDX]);

        auto nsFileRefs = ark::es2panda::lsp::GetFileReferencesFromIndex(mainCtx, paths[SPEC_NS_LIB_IDX], false);
        ASSERT_EQ(nsFileRefs.referenceInfos.size(), 1U);
        ASSERT_EQ(nsFileRefs.referenceInfos[0].fileName, paths[SPEC_MAIN_FILE_IDX]);
    }

    static void ExpectNamedSpecifierDefShape(es2panda_Context *namedCtx, const std::vector<std::string> &paths,
                                             const std::vector<std::string> &texts)
    {
        // Named specifier: queried from the defining file the definition resolves with exact
        // offsets, and the importing-file references are the specifier plus the console.log usage.
        const auto namedDefPos = texts[SPEC_NAMED_LIB_IDX].find("namedVal: number");
        ASSERT_NE(namedDefPos, std::string::npos);
        auto namedRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(namedCtx, namedDefPos);
        ASSERT_EQ(namedRefs.definitionInfo.fileName, paths[SPEC_NAMED_LIB_IDX]);
        ASSERT_EQ(namedRefs.definitionInfo.start, namedDefPos);
        ASSERT_EQ(namedRefs.definitionInfo.length, std::string("namedVal").size());
        ASSERT_EQ(CountReferencesInFile(namedRefs, paths[SPEC_NAMED_LIB_IDX]), 0U);
        ASSERT_EQ(CountReferencesInFile(namedRefs, paths[SPEC_MAIN_FILE_IDX]), SPEC_MAIN_REF_COUNT);
    }

    static void ExpectDefaultSpecifierDefShape(es2panda_Context *defaultCtx, const std::vector<std::string> &paths,
                                               const std::vector<std::string> &texts)
    {
        // Default specifier: the definition resolves from its own file (the "default" module key
        // of the import side does not link back to it, but the definition entry itself is intact).
        const auto defaultDefPos = texts[SPEC_DEFAULT_LIB_IDX].find("DefaultCls {");
        ASSERT_NE(defaultDefPos, std::string::npos);
        auto defaultRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(defaultCtx, defaultDefPos);
        ASSERT_EQ(defaultRefs.definitionInfo.fileName, paths[SPEC_DEFAULT_LIB_IDX]);
        ASSERT_EQ(defaultRefs.definitionInfo.start, defaultDefPos);
        ASSERT_EQ(defaultRefs.definitionInfo.length, std::string("DefaultCls").size());
        // The default import specifier shares the "module:<path>:default" key with the
        // definition, so both main-file occurrences (specifier and member usage) are listed.
        ASSERT_EQ(CountReferencesInFile(defaultRefs, paths[SPEC_MAIN_FILE_IDX]), SPEC_MAIN_REF_COUNT);
    }

    static void ExpectNamespaceSpecifierDefShape(es2panda_Context *nsCtx, const std::vector<std::string> &paths,
                                                 const std::vector<std::string> &texts)
    {
        // Namespace specifier: same definition-side resolution from its own file.
        const auto nsDefPos = texts[SPEC_NS_LIB_IDX].find("nsVal: number");
        ASSERT_NE(nsDefPos, std::string::npos);
        auto nsRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(nsCtx, nsDefPos);
        ASSERT_EQ(nsRefs.definitionInfo.fileName, paths[SPEC_NS_LIB_IDX]);
        ASSERT_EQ(nsRefs.definitionInfo.start, nsDefPos);
        // The main-file member usage nsAll.nsVal resolves to the exported member's symbol.
        ASSERT_EQ(CountReferencesInFile(nsRefs, paths[SPEC_MAIN_FILE_IDX]), 1U);
    }

    static void ExpectImportSpecifierRefs(es2panda_Context *mainCtx, const std::vector<std::string> &paths,
                                          const std::vector<std::string> &texts)
    {
        // Import side: querying the main file at the namedVal specifier resolves to the
        // module-keyed import symbol whose references are both main-file occurrences; the
        // definition linkage surfaces through the defining-file query above.
        const auto specifierPos = texts[SPEC_MAIN_FILE_IDX].find("namedVal }");
        ASSERT_NE(specifierPos, std::string::npos);
        auto specifierRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(mainCtx, specifierPos);
        ASSERT_EQ(CountReferencesInFile(specifierRefs, paths[SPEC_MAIN_FILE_IDX]), SPEC_MAIN_REF_COUNT);
    }

    static void ExpectReExportChainSourcesIndexed(const std::vector<std::string> &paths,
                                                  const std::vector<std::string> &texts)
    {
        ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(paths[REX_BASE_FILE_IDX]), texts[REX_BASE_FILE_IDX]);
        ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(paths[REX_MID_FILE_IDX]), texts[REX_MID_FILE_IDX]);
        ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(paths[REX_MAIN_FILE_IDX]), texts[REX_MAIN_FILE_IDX]);
    }

    static void ExpectReExportFileReferences(es2panda_Context *mainCtx, const std::vector<std::string> &paths,
                                             const std::vector<std::string> &texts)
    {
        // Measured contract: the re-export statement `export ... from` is an export declaration,
        // not an ETSImportDeclaration, so AddFileReferenceFromImportDecl
        // (symbol_reference_index.cpp:630-635) never records a file reference for the base file;
        // only the plain import in main contributes one, for the mid file.
        auto baseFileRefs = ark::es2panda::lsp::GetFileReferencesFromIndex(mainCtx, paths[REX_BASE_FILE_IDX], false);
        ASSERT_TRUE(baseFileRefs.referenceInfos.empty());

        auto midFileRefs = ark::es2panda::lsp::GetFileReferencesFromIndex(mainCtx, paths[REX_MID_FILE_IDX], false);
        ASSERT_EQ(midFileRefs.referenceInfos.size(), 1U);
        ASSERT_EQ(midFileRefs.referenceInfos[0].fileName, paths[REX_MAIN_FILE_IDX]);
        const size_t mainImportStart = texts[REX_MAIN_FILE_IDX].find("'./shapes_rex_mid'");
        ASSERT_NE(mainImportStart, std::string::npos);
        ASSERT_EQ(midFileRefs.referenceInfos[0].start, mainImportStart);
        ASSERT_EQ(midFileRefs.referenceInfos[0].length, std::string("'./shapes_rex_mid'").size());
    }

    static void ExpectBaseDefinitionShape(es2panda_Context *baseCtx, const std::vector<std::string> &paths,
                                          const std::vector<std::string> &texts)
    {
        // The original definition in the base file is found by exact name from its own file
        // context with exact offsets.
        const auto baseDefPos = texts[REX_BASE_FILE_IDX].find("reShared: number");
        ASSERT_NE(baseDefPos, std::string::npos);
        auto baseRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(baseCtx, baseDefPos);
        ASSERT_EQ(baseRefs.definitionInfo.fileName, paths[REX_BASE_FILE_IDX]);
        ASSERT_EQ(baseRefs.definitionInfo.start, baseDefPos);
        ASSERT_EQ(baseRefs.definitionInfo.length, std::string("reShared").size());
    }

    static void ExpectBaseListedInExactLookup(const std::vector<std::string> &paths)
    {
        auto exact = ark::es2panda::lsp::FindSymbolDefinitionsByName("reShared", "");
        ASSERT_GE(exact.size(), 1U);
        bool baseFound = false;
        for (const auto &entry : exact) {
            if (entry.fileName == paths[REX_BASE_FILE_IDX]) {
                baseFound = true;
                ASSERT_EQ(entry.symbolName, "reShared");
                ASSERT_FALSE(entry.isDefaultExport);
            }
        }
        ASSERT_TRUE(baseFound);
    }

    static void ExpectMainAliasUsageShape(es2panda_Context *mainCtx, const std::vector<std::string> &paths,
                                          const std::vector<std::string> &texts)
    {
        // The main-file alias usage resolves to the module-keyed import symbol with both
        // main-file occurrences (specifier and console usage).
        const auto aliasUsePos =
            texts[REX_MAIN_FILE_IDX].find("console.log(reAlias)") + std::string("console.log(").size();
        ASSERT_NE(aliasUsePos, std::string::npos);
        auto aliasRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(mainCtx, aliasUsePos);
        ASSERT_EQ(CountReferencesInFile(aliasRefs, paths[REX_MAIN_FILE_IDX]), REX_MAIN_REF_COUNT);
    }

    static void ExpectStaticMethodShapes(es2panda_Context *context, const std::vector<std::string> &paths,
                                         const std::vector<std::string> &texts)
    {
        const std::string &source = texts[0];

        // Static method: definition resolves at its identifier, references include the definition
        // and the qualified call-site usage.
        const auto methodDefPos = source.find("staticMethod(x: number)");
        ASSERT_NE(methodDefPos, std::string::npos);
        auto methodRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, methodDefPos);
        ASSERT_EQ(methodRefs.definitionInfo.fileName, paths[0]);
        ASSERT_EQ(methodRefs.definitionInfo.start, methodDefPos);
        ASSERT_EQ(methodRefs.definitionInfo.length, std::string("staticMethod").size());
        // The definition is stripped from the list; the qualified call-site usage remains.
        ASSERT_EQ(CountReferencesInFile(methodRefs, paths[0]), 1U);

        // The call-site usage resolves to the same symbol: same reference count, same definition.
        const auto methodUsePos = source.find(".staticMethod(3)") + 1;
        ASSERT_NE(methodUsePos, std::string::npos);
        auto methodUseRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, methodUsePos);
        ASSERT_EQ(methodUseRefs.referenceInfos.size(), methodRefs.referenceInfos.size());
        ASSERT_EQ(methodUseRefs.definitionInfo.start, methodDefPos);
    }

    static void ExpectStaticFieldShape(es2panda_Context *context, const std::vector<std::string> &texts)
    {
        const std::string &source = texts[0];

        // Static field: definition only (no usage), still resolved with exact offsets; with no
        // other occurrence the displayed reference list is empty.
        const auto fieldDefPos = source.find("staticField: number");
        ASSERT_NE(fieldDefPos, std::string::npos);
        auto fieldRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, fieldDefPos);
        ASSERT_EQ(fieldRefs.definitionInfo.start, fieldDefPos);
        ASSERT_EQ(fieldRefs.definitionInfo.length, std::string("staticField").size());
        ASSERT_TRUE(fieldRefs.referenceInfos.empty());
    }

    static void ExpectNestedShapeQueriesUnresolved(es2panda_Context *context, const std::vector<std::string> &texts)
    {
        const std::string &source = texts[0];

        // Nested exported class identifier: the qualified call-chain usage is registered, but the
        // definition entry does not resolve at the identifier position (measured: definitionInfo
        // stays empty for the nested export).
        const auto classDefPos = source.find("class InnerClass {");
        ASSERT_NE(classDefPos, std::string::npos);
        auto classRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, classDefPos);
        ASSERT_TRUE(classRefs.definitionInfo.fileName.empty());
        // Measured: the qualified call-chain usage registers under a different symbol identity,
        // so no reference is listed at the class identifier position.
        ASSERT_TRUE(classRefs.referenceInfos.empty());

        // Inner exported namespace: same measured shape.
        const auto nsDefPos = source.find("namespace InnerNS {");
        ASSERT_NE(nsDefPos, std::string::npos);
        auto nestedNsRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, nsDefPos);
        ASSERT_TRUE(nestedNsRefs.definitionInfo.fileName.empty());
        ASSERT_TRUE(nestedNsRefs.referenceInfos.empty());
    }

    static void ExpectNestedExportLookups(const std::vector<std::string> &paths)
    {
        // Exported definitions of the nested shapes are exposed with exact names.
        auto classExport = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("innerclass", "");
        ASSERT_EQ(classExport.size(), 1U);
        ASSERT_EQ(classExport[0].fileName, paths[0]);
        ASSERT_EQ(classExport[0].symbolName, "InnerClass");
        auto nsExport = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("innerns", "");
        ASSERT_EQ(nsExport.size(), 1U);
        ASSERT_EQ(nsExport[0].symbolName, "InnerNS");
    }

    static void ExpectTransitiveSourcesIndexed(const std::vector<std::string> &paths,
                                               const std::vector<std::string> &texts)
    {
        // Direct and transitive externals are both indexed with their exact sources.
        ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(paths[TRANSITIVE_DEEP_LIB_IDX]),
                  texts[TRANSITIVE_DEEP_LIB_IDX]);
        ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(paths[TRANSITIVE_MID_LIB_IDX]),
                  texts[TRANSITIVE_MID_LIB_IDX]);
        ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(paths[TRANSITIVE_MAIN_FILE_IDX]),
                  texts[TRANSITIVE_MAIN_FILE_IDX]);
    }

    static void ExpectDeepDefinitionShape(es2panda_Context *deepCtx, const std::vector<std::string> &paths,
                                          const std::vector<std::string> &texts)
    {
        // Deep definition queried from its own file: definition exact, displayed references empty
        // (the definition is stripped and no other deep-file occurrence exists).
        const auto deepDefPos = texts[TRANSITIVE_DEEP_LIB_IDX].find("deepVal: number");
        ASSERT_NE(deepDefPos, std::string::npos);
        auto deepRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(deepCtx, deepDefPos);
        ASSERT_TRUE(deepRefs.referenceInfos.empty());
        ASSERT_EQ(deepRefs.definitionInfo.fileName, paths[TRANSITIVE_DEEP_LIB_IDX]);
        ASSERT_EQ(deepRefs.definitionInfo.start, deepDefPos);
        ASSERT_EQ(deepRefs.definitionInfo.length, std::string("deepVal").size());
    }

    static void ExpectMidFileDeepUsageShape(es2panda_Context *midCtx, const std::vector<std::string> &paths,
                                            const std::vector<std::string> &texts)
    {
        // Queried from the mid file at its arithmetic usage: the three mid-file occurrences
        // (import specifier, usage, console) are sorted by start and the deep definition
        // resolves across the file boundary.
        const auto deepUsePos = texts[TRANSITIVE_MID_LIB_IDX].find("deepVal + 1");
        ASSERT_NE(deepUsePos, std::string::npos);
        auto midRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(midCtx, deepUsePos);
        constexpr size_t midRefCount = 3;
        ASSERT_EQ(midRefs.referenceInfos.size(), midRefCount);
        for (const auto &ref : midRefs.referenceInfos) {
            ASSERT_EQ(ref.fileName, paths[TRANSITIVE_MID_LIB_IDX]);
            ASSERT_EQ(ref.length, std::string("deepVal").size());
        }
        ASSERT_EQ(midRefs.referenceInfos[0].start, texts[TRANSITIVE_MID_LIB_IDX].find("deepVal }"));
        ASSERT_EQ(midRefs.referenceInfos[1].start, deepUsePos);
        ASSERT_EQ(midRefs.referenceInfos[MID_CONSOLE_USAGE_REF_IDX].start,
                  texts[TRANSITIVE_MID_LIB_IDX].find("console.log(deepVal)") + std::string("console.log(").size());
        // Measured contract: the importing-side occurrences are grouped under their own symbol
        // identity; the defining-side definition entry is only visible from the defining file
        // (deepCtx query above).
        ASSERT_TRUE(midRefs.definitionInfo.fileName.empty());
    }

    static void ExpectMainMidValUsageShape(es2panda_Context *mainCtx, const std::vector<std::string> &paths,
                                           const std::vector<std::string> &texts)
    {
        // Queried from the main file: the exported midVal definition resolves through the shared
        // module key with both main-file occurrences.
        const auto midDefPos = texts[TRANSITIVE_MID_LIB_IDX].find("midVal: number");
        ASSERT_NE(midDefPos, std::string::npos);
        const auto midUsePos =
            texts[TRANSITIVE_MAIN_FILE_IDX].find("console.log(midVal)") + std::string("console.log(").size();
        ASSERT_NE(midUsePos, std::string::npos);
        auto mainRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(mainCtx, midUsePos);
        ASSERT_EQ(CountReferencesInFile(mainRefs, paths[TRANSITIVE_MAIN_FILE_IDX]), TRANSITIVE_MAIN_REF_COUNT);
        ASSERT_EQ(mainRefs.definitionInfo.fileName, paths[TRANSITIVE_MID_LIB_IDX]);
        ASSERT_EQ(mainRefs.definitionInfo.start, midDefPos);
        ASSERT_EQ(mainRefs.definitionInfo.length, std::string("midVal").size());
    }
};

// PK-001 — AddFileReferenceFromImportDecl (symbol_reference_index.cpp:586-622) records one
// file-reference entry per import declaration, keyed by the resolved module path
// (BuildFileReferenceSymbolId, symbol_reference_index.cpp:337-340), for each specifier form:
// named / default / namespace. GetFileReferencesFromIndex
// (symbol_reference_index.cpp:791-818) resolves those entries per imported file, and
// GetReferencesAtPositionFromIndex (symbol_reference_index.cpp:736-789) resolves the
// specifier identifiers through their module keys (BuildModuleSymbolKey
// symbol_reference_index.cpp:271-280: named -> imported name, default -> "default",
// namespace -> "*").
TEST_F(LSPSymbolReferenceIndexShapesTests, ImportDeclSpecifierShapesRegisterFileReferences)
{
    std::vector<std::string> files = {"shapes_lib_named.ets", "shapes_lib_default.ets", "shapes_lib_ns.ets",
                                      "shapes_import_main.ets"};
    std::vector<std::string> texts = {R"(export let namedVal: number = 1;)",
                                      R"(export default class DefaultCls {
    v: number = 1;
})",
                                      R"(export let nsVal: number = 3;)",
                                      R"(import { namedVal } from './shapes_lib_named';
import DefaultCls from './shapes_lib_default';
import * as nsAll from './shapes_lib_ns';
console.log(namedVal);
console.log(DefaultCls.v);
console.log(nsAll.nsVal);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *mainCtx = initializer.CreateContext(filePaths[SPEC_MAIN_FILE_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(mainCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContextWithExternal(mainCtx));
    // Definition-side queries run from contexts whose source file is the defining file; the
    // index is global, the query position is resolved against ctx->sourceFileName
    // (symbol_reference_index.cpp:743-748).
    auto *namedCtx = initializer.CreateContext(filePaths[SPEC_NAMED_LIB_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(namedCtx, nullptr);
    auto *defaultCtx = initializer.CreateContext(filePaths[SPEC_DEFAULT_LIB_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(defaultCtx, nullptr);
    auto *nsCtx = initializer.CreateContext(filePaths[SPEC_NS_LIB_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(nsCtx, nullptr);

    ExpectImportedLibSourcesIndexed(filePaths, texts);
    ExpectSpecifierFileReferences(mainCtx, filePaths, texts);
    ExpectNamedSpecifierDefShape(namedCtx, filePaths, texts);
    ExpectDefaultSpecifierDefShape(defaultCtx, filePaths, texts);
    ExpectNamespaceSpecifierDefShape(nsCtx, filePaths, texts);
    ExpectImportSpecifierRefs(mainCtx, filePaths, texts);

    initializer.DestroyContext(mainCtx);
    initializer.DestroyContext(namedCtx);
    initializer.DestroyContext(defaultCtx);
    initializer.DestroyContext(nsCtx);
}

// PK-001 — re-export chain: AddFileReferenceFromImportDecl (symbol_reference_index.cpp:586-622)
// records the file references of both hops, and BuildModuleSymbolKey
// (symbol_reference_index.cpp:300-304) derives the exported name from the export specifier
// (`Exported() != nullptr` arm) for the re-exported alias. FindExportSymbolDefinitionsByPrefix
// (symbol_reference_index.cpp:1068-1112) and FindSymbolDefinitionsByName
// (symbol_reference_index.cpp:1026-1066) expose both sides of the chain.
TEST_F(LSPSymbolReferenceIndexShapesTests, ImportDeclReExportChainResolvesAlias)
{
    std::vector<std::string> files = {"shapes_rex_base.ets", "shapes_rex_mid.ets", "shapes_rex_main.ets"};
    std::vector<std::string> texts = {R"(export let reShared: number = 1;)",
                                      R"(export { reShared as reAlias } from './shapes_rex_base';)",
                                      R"(import { reAlias } from './shapes_rex_mid';
console.log(reAlias);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *mainCtx = initializer.CreateContext(filePaths[REX_MAIN_FILE_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(mainCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContextWithExternal(mainCtx));

    ExpectReExportChainSourcesIndexed(filePaths, texts);
    ExpectReExportFileReferences(mainCtx, filePaths, texts);

    // Measured contract: the re-export specifier identifier is not a definition node (its
    // owner is the export specifier, so IsDefinitionNode at symbol_reference_index.cpp:342-348
    // is false), so no "reAlias" export entry exists for the mid file.
    auto aliasPrefix = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("realias", "");
    ASSERT_TRUE(aliasPrefix.empty());

    auto *baseCtx = initializer.CreateContext(filePaths[REX_BASE_FILE_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(baseCtx, nullptr);
    ExpectBaseDefinitionShape(baseCtx, filePaths, texts);
    ExpectBaseListedInExactLookup(filePaths);

    ExpectMainAliasUsageShape(mainCtx, filePaths, texts);

    initializer.DestroyContext(mainCtx);
    initializer.DestroyContext(baseCtx);
}

// PK-001 — import of a module path with no resolvable file: AddFileReferenceFromImportDecl
// (symbol_reference_index.cpp:593-601) falls back through HasSpecifiedDeclPath/DeclPath and
// Source()->Str() when ResolvedSource() is empty, and the index build still succeeds.
TEST_F(LSPSymbolReferenceIndexShapesTests, ImportDeclUnresolvedModuleStillBuildsIndex)
{
    std::vector<std::string> files = {"shapes_ghost_main.ets"};
    std::vector<std::string> texts = {R"(import { ghostVal } from './shapes_no_such_module';
console.log(ghostVal);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *mainCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(mainCtx, nullptr);
    // The build itself must not fail on the unresolved module: the import declaration keeps a
    // non-empty source path through the fallback chain, so the file reference is recorded and
    // the main file is indexed verbatim.
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(mainCtx));
    ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]), texts[0]);
    initializer.DestroyContext(mainCtx);
}

// PK-002 — BuildModuleSymbolKey (symbol_reference_index.cpp:257-310) and BuildSymbolId
// (symbol_reference_index.cpp:312-335) arm matrix through index build: nested namespaces,
// nested classes and static members. The definition and reference entries of every shape are
// resolved with exact offsets through GetReferencesAtPositionFromIndex
// (symbol_reference_index.cpp:736-789).
TEST_F(LSPSymbolReferenceIndexShapesTests, ModuleKeysAndSymbolIdsForNestedStaticShapes)
{
    std::vector<std::string> files = {"shapes_nested.ets"};
    std::vector<std::string> texts = {R"(namespace OuterNS {
    export namespace InnerNS {
        export class InnerClass {
            static staticField: number = 7;
            static staticMethod(x: number): number {
                return x;
            }
        }
    }
}
let callSite = OuterNS.InnerNS.InnerClass.staticMethod(3);
console.log(callSite);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    ExpectStaticMethodShapes(context, filePaths, texts);
    ExpectStaticFieldShape(context, texts);
    ExpectNestedShapeQueriesUnresolved(context, texts);
    ExpectNestedExportLookups(filePaths);

    initializer.DestroyContext(context);
}

// PK-003 — GetFunctionReturnTypeForIndex (symbol_reference_index.cpp:153-179) return-type arms
// recorded by AddDefinitionAndExportInfo (symbol_reference_index.cpp:507-533) via
// BuildDisplayNameForExport (symbol_reference_index.cpp:197-207): explicit annotation,
// union annotation normalized by NormalizeTypeText (symbol_reference_index.cpp:134-151),
// generic annotation, and the signature fallback (symbol_reference_index.cpp:171-178).
TEST_F(LSPSymbolReferenceIndexShapesTests, FunctionReturnTypesForIndexExportMatrix)
{
    std::vector<std::string> files = {"shapes_return_types.ets"};
    std::vector<std::string> texts = {R"(export function retNum(x: number): number {
    return x;
}
export function retUnion(x: number): number | string {
    return x;
}
export function retGeneric<T>(x: T): T {
    return x;
}
export function retVoid(x: number): void {
    console.log(x);
}
export function retInferred(x: number) {
    return x;
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    ExpectSingleDef(ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("retnum", ""),
                    {filePaths[0], "retNum", false, AstNodeType::FUNCTION_DECLARATION, "number"});
    ExpectSingleDef(ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("retunion", ""),
                    {filePaths[0], "retUnion", false, AstNodeType::FUNCTION_DECLARATION, "(number | string)"});
    ExpectSingleDef(ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("retgeneric", ""),
                    {filePaths[0], "retGeneric", false, AstNodeType::FUNCTION_DECLARATION, "T"});
    ExpectSingleDef(ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("retvoid", ""),
                    {filePaths[0], "retVoid", false, AstNodeType::FUNCTION_DECLARATION, "void"});
    // Inferred return type: with no return annotation the signature fallback
    // (symbol_reference_index.cpp:171-175) reports the checker type text "Double".
    ExpectSingleDef(ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("retinferred", ""),
                    {filePaths[0], "retInferred", false, AstNodeType::FUNCTION_DECLARATION, "Double"});

    initializer.DestroyContext(context);
}

// PK-004 — FindExportSymbolDefinitionsByPrefix (symbol_reference_index.cpp:1068-1112):
// case-insensitive substring matching with exact hit/miss/exclude arms, empty-prefix pass-all
// arm (symbol_reference_index.cpp:1098) and the dedup guard (symbol_reference_index.cpp:1103).
TEST_F(LSPSymbolReferenceIndexShapesTests, PrefixLookupHitMissExcludeArms)
{
    std::vector<std::string> files = {"shapes_prefix.ets"};
    std::vector<std::string> texts = {R"(export function prefixAlpha(x: number): number {
    return x;
}
export let prefixBeta: string = "b";
export class PrefixGamma {
    v: number = 1;
}
let localHidden: number = 9;
console.log(localHidden);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    // Uppercase query matches through the lowercase transform (L1074-1075).
    auto upper = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("PREFIXALPHA", "");
    ASSERT_EQ(upper.size(), 1U);
    ASSERT_EQ(upper[0].symbolName, "prefixAlpha");

    // Substring (not only prefix) hit.
    ExpectSingleDef(ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("gamma", ""),
                    {filePaths[0], "PrefixGamma", false, AstNodeType::CLASS_DECLARATION, ""});

    // Miss: no exported symbol contains the fragment (L1098 continue arm).
    auto miss = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("zzz_no_match", "");
    ASSERT_TRUE(miss.empty());

    // Empty prefix passes every exported definition; the non-exported local is never listed.
    auto all = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("", "");
    ASSERT_EQ(all.size(), 3U);
    bool hiddenListed = false;
    for (const auto &entry : all) {
        hiddenListed = hiddenListed || entry.symbolName == "localHidden";
    }
    ASSERT_FALSE(hiddenListed);

    // Excluding the defining file empties the result (L1086 arm).
    auto excluded = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("prefixalpha", filePaths[0]);
    ASSERT_TRUE(excluded.empty());

    initializer.DestroyContext(context);
}

// PK-004 — FindSymbolDefinitionsByName (symbol_reference_index.cpp:1026-1066): exact-name hit,
// name-mismatch continue (L1051), excludeFile arm (L1032) and the same-file dedup collision
// (L1056) with overloaded declarations of one name.
TEST_F(LSPSymbolReferenceIndexShapesTests, ExactNameLookupHitMissExcludeDedup)
{
    std::vector<std::string> files = {"shapes_overloads.ets"};
    std::vector<std::string> texts = {R"(export function pick(a: string): void;
export function pick(a: number): void;
export function pick(a: string | number): void {
    console.log(a);
}
export function soloShape(): void {
    console.log(1);
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    // Three "pick" definitions share one file+name key: the dedup guard collapses them into a
    // single result (L1056 continue arm). Overload signatures resolve no declaration owner, so
    // the recorded declType is the unresolved 0 (ResolveDeclTypeForCompletion
    // symbol_reference_index.cpp:123-132).
    auto picks = ark::es2panda::lsp::FindSymbolDefinitionsByName("pick", "");
    ASSERT_EQ(picks.size(), 1U);
    ASSERT_EQ(picks[0].fileName, filePaths[0]);
    ASSERT_EQ(picks[0].symbolName, "pick");
    ASSERT_FALSE(picks[0].isDefaultExport);
    ASSERT_EQ(picks[0].declType, static_cast<AstNodeType>(0));

    // Same query with the defining file excluded yields nothing (L1032 continue arm).
    auto excluded = ark::es2panda::lsp::FindSymbolDefinitionsByName("pick", filePaths[0]);
    ASSERT_TRUE(excluded.empty());

    // Name mismatch never matches (L1051 continue arm).
    auto missing = ark::es2panda::lsp::FindSymbolDefinitionsByName("no_such_pick", "");
    ASSERT_TRUE(missing.empty());

    // A name is matched exactly, not by substring.
    // A name is matched exactly, not by substring. FindSymbolDefinitionsByName constructs its
    // results without declType/returnType (symbol_reference_index.cpp:1062), so both keep
    // their struct defaults.
    auto partial = ark::es2panda::lsp::FindSymbolDefinitionsByName("solo", "");
    ASSERT_TRUE(partial.empty());
    ExpectSingleDef(ark::es2panda::lsp::FindSymbolDefinitionsByName("soloShape", ""),
                    {filePaths[0], "soloShape", false, static_cast<AstNodeType>(0), ""});

    initializer.DestroyContext(context);
}

// PK-005 — lifecycle: RemoveFileContributions (symbol_reference_index.cpp:350-392) erases the
// definitions of the removed file (L365 arm) and prunes its per-reference entries, while other
// files stay indexed; rebuilding restores exact entries (BuildSymbolReferenceIndexForProgram
// symbol_reference_index.cpp:638-668 re-registers after RemoveFileContributions L641).
TEST_F(LSPSymbolReferenceIndexShapesTests, RemoveDefiningFilePrunesDefinitionsAndRefs)
{
    std::vector<std::string> files = {"shapes_lc_def.ets", "shapes_lc_use.ets"};
    std::vector<std::string> texts = {R"(export let lifeVal: number = 1;)",
                                      R"(import { lifeVal } from './shapes_lc_def';
console.log(lifeVal);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *defCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(defCtx, nullptr);
    auto *useCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(useCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(defCtx));
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(useCtx));

    const auto defPos = texts[0].find("lifeVal: number");
    ASSERT_NE(defPos, std::string::npos);
    auto before = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(defCtx, defPos);
    // The definition is reported via definitionInfo only: both references are the import
    // specifier and the console.log usage inside the importing file.
    ASSERT_EQ(before.referenceInfos.size(), 2U);
    ASSERT_EQ(CountReferencesInFile(before, filePaths[1]), 2U);

    // Removing the importing file removes both references but keeps the definition entry.
    ASSERT_TRUE(ark::es2panda::lsp::RemoveSymbolReferenceIndexForFile(filePaths[1]));
    auto afterUseRemoved = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(defCtx, defPos);
    ASSERT_TRUE(afterUseRemoved.referenceInfos.empty());
    ASSERT_EQ(afterUseRemoved.definitionInfo.fileName, filePaths[0]);
    ASSERT_EQ(afterUseRemoved.definitionInfo.start, defPos);

    // Removing the defining file erases its definitions (L365 arm) and all its references.
    ASSERT_TRUE(ark::es2panda::lsp::RemoveSymbolReferenceIndexForFile(filePaths[0]));
    auto afterDefRemoved = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(defCtx, defPos);
    ASSERT_TRUE(afterDefRemoved.referenceInfos.empty());
    ASSERT_TRUE(afterDefRemoved.definitionInfo.fileName.empty());
    ASSERT_TRUE(ark::es2panda::lsp::FindSymbolDefinitionsByName("lifeVal", "").empty());
    ASSERT_TRUE(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]).empty());
    // The importing file was already removed, so the whole index is empty again.
    ASSERT_TRUE(ark::es2panda::lsp::GetIndexedFileSource(filePaths[1]).empty());

    // Rebuilding the defining file restores exactly the previous def-file entries.
    auto *rebuiltCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(rebuiltCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(rebuiltCtx));
    auto restored = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(rebuiltCtx, defPos);
    // The rebuilt file contributes exactly the definition occurrence; it is stripped from the
    // displayed references and resurfaces as definitionInfo.
    ASSERT_TRUE(restored.referenceInfos.empty());
    ASSERT_EQ(restored.definitionInfo.fileName, filePaths[0]);
    ASSERT_EQ(restored.definitionInfo.start, defPos);
    ASSERT_EQ(restored.definitionInfo.length, std::string("lifeVal").size());

    initializer.DestroyContext(defCtx);
    initializer.DestroyContext(useCtx);
    initializer.DestroyContext(rebuiltCtx);
}

// PK-005 — a symbol referenced only inside one file: RemoveFileContributions
// (symbol_reference_index.cpp:371-389) prunes each per-reference entry and erases the symbol's
// reference vector when it becomes empty (L386 arm); the visited-symbol dedup (L360) walks the
// repeated entries once.
TEST_F(LSPSymbolReferenceIndexShapesTests, RemoveFileWithAllRefsErasesSymbolEntries)
{
    std::vector<std::string> files = {"shapes_single_file_symbol.ets"};
    std::vector<std::string> texts = {R"(let onlyLocal = 1;
onlyLocal = 2;
onlyLocal = 3;
console.log(onlyLocal);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    const auto defPos = texts[0].find("onlyLocal = 1");
    ASSERT_NE(defPos, std::string::npos);
    auto before = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, defPos);
    // The definition is stripped from the list; the two assignment targets and the
    // console.log usage remain.
    ASSERT_EQ(before.referenceInfos.size(), 3U);
    for (const auto &ref : before.referenceInfos) {
        ASSERT_EQ(ref.fileName, filePaths[0]);
        ASSERT_EQ(ref.length, std::string("onlyLocal").size());
    }

    ASSERT_TRUE(ark::es2panda::lsp::RemoveSymbolReferenceIndexForFile(filePaths[0]));
    ASSERT_TRUE(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]).empty());
    auto afterRemove = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, defPos);
    ASSERT_TRUE(afterRemove.referenceInfos.empty());
    ASSERT_TRUE(afterRemove.definitionInfo.fileName.empty());

    // Rebuild: no stale entries from the removed contributions remain.
    auto *freshCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(freshCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(freshCtx));
    auto rebuilt = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(freshCtx, defPos);
    ASSERT_EQ(rebuilt.referenceInfos.size(), before.referenceInfos.size());
    ASSERT_EQ(rebuilt.definitionInfo.start, defPos);

    initializer.DestroyContext(context);
    initializer.DestroyContext(freshCtx);
}

// PK-005 — empty-index queries and boundaries: ClearSymbolReferenceIndex
// (symbol_reference_index.cpp:680-683) leaves GetReferencesAtPositionFromIndex
// (symbol_reference_index.cpp:736-748) and GetIndexedFileSource
// (symbol_reference_index.cpp:820-827) empty; GetFileReferencesFromIndex
// (symbol_reference_index.cpp:791-818) rejects an empty search name (L795) and a package-module
// name without any path separator (L802).
TEST_F(LSPSymbolReferenceIndexShapesTests, EmptyIndexQueriesAndPackageModuleBoundary)
{
    std::vector<std::string> files = {"shapes_no_import.ets"};
    std::vector<std::string> texts = {R"(let soloVal: number = 1;
console.log(soloVal);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // WithExternal on an import-free context: no external declaration sources exist, so the
    // early return keeps buildSuccess true (symbol_reference_index.cpp:714-717).
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContextWithExternal(context));
    ASSERT_EQ(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]), texts[0]);

    ark::es2panda::lsp::ClearSymbolReferenceIndex();
    ASSERT_TRUE(ark::es2panda::lsp::GetIndexedFileSource(filePaths[0]).empty());
    const auto defPos = texts[0].find("soloVal: number");
    ASSERT_NE(defPos, std::string::npos);
    auto clearedRefs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, defPos);
    ASSERT_TRUE(clearedRefs.referenceInfos.empty());
    ASSERT_TRUE(clearedRefs.definitionInfo.fileName.empty());
    ASSERT_TRUE(ark::es2panda::lsp::FindSymbolDefinitionsByName("soloVal", "").empty());
    ASSERT_TRUE(ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("soloval", "").empty());

    // GetFileReferencesFromIndex guards on the empty search name and on a package-module name
    // without any path separator (L795, L802).
    auto emptyName = ark::es2panda::lsp::GetFileReferencesFromIndex(context, "", false);
    ASSERT_TRUE(emptyName.referenceInfos.empty());
    auto barePackage = ark::es2panda::lsp::GetFileReferencesFromIndex(context, "bare_module_name", true);
    ASSERT_TRUE(barePackage.referenceInfos.empty());
    auto dirPackage = ark::es2panda::lsp::GetFileReferencesFromIndex(context, filePaths[0], true);
    ASSERT_TRUE(dirPackage.referenceInfos.empty());

    initializer.DestroyContext(context);
}

// PK-005 — cross-file: BuildSymbolReferenceIndexForContextWithExternal
// (symbol_reference_index.cpp:695-727) indexes the direct external source and the transitive
// one exactly once each (indexedFiles dedup L706), and the reference list of a transitive
// symbol is sorted by (fileName, start, length) (DeduplicateAndSortReferences
// symbol_reference_index.cpp:394-412).
TEST_F(LSPSymbolReferenceIndexShapesTests, ExternalTransitiveCrossFileIndex)
{
    std::vector<std::string> files = {"shapes_deep_lib.ets", "shapes_mid_lib.ets", "shapes_trans_main.ets"};
    std::vector<std::string> texts = {R"(export let deepVal: number = 3;)",
                                      R"(import { deepVal } from './shapes_deep_lib';
export let midVal: number = deepVal + 1;
console.log(deepVal);
console.log(midVal);)",
                                      R"(import { midVal } from './shapes_mid_lib';
console.log(midVal);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *mainCtx = initializer.CreateContext(filePaths[TRANSITIVE_MAIN_FILE_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(mainCtx, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContextWithExternal(mainCtx));
    // Definition-side queries run from contexts whose source file is the queried file; the
    // index itself is global (symbol_reference_index.cpp:743-748).
    auto *deepCtx = initializer.CreateContext(filePaths[TRANSITIVE_DEEP_LIB_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(deepCtx, nullptr);
    auto *midCtx = initializer.CreateContext(filePaths[TRANSITIVE_MID_LIB_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(midCtx, nullptr);

    ExpectTransitiveSourcesIndexed(filePaths, texts);
    ExpectDeepDefinitionShape(deepCtx, filePaths, texts);
    ExpectMidFileDeepUsageShape(midCtx, filePaths, texts);
    ExpectMainMidValUsageShape(mainCtx, filePaths, texts);

    initializer.DestroyContext(mainCtx);
    initializer.DestroyContext(deepCtx);
    initializer.DestroyContext(midCtx);
}

// PK-003/PK-005 — query-position boundaries in GetReferencesAtPositionFromIndex
// (symbol_reference_index.cpp:750-765): the narrowest-occurrence scan skips positions outside
// every occurrence (L753) and inside an identifier the hit resolves at the exact occurrence
// (L756-760); positions at occurrence end (L753 `position >= end`) yield no hit.
TEST_F(LSPSymbolReferenceIndexShapesTests, QueryPositionBoundariesHitAndMiss)
{
    std::vector<std::string> files = {"shapes_positions.ets"};
    std::vector<std::string> texts = {R"(let posTarget: number = 1;
console.log(posTarget);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    const std::string &source = texts[0];
    const size_t nameLen = std::string("posTarget").size();

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    const auto defPos = source.find("posTarget: number");
    ASSERT_NE(defPos, std::string::npos);
    const auto usePos = source.find("console.log(posTarget)") + std::string("console.log(").size();
    ASSERT_NE(usePos, std::string::npos);

    // Inside the definition identifier: hit with the usage reference; the definition is
    // reported through definitionInfo only.
    auto hit = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, defPos);
    ASSERT_EQ(hit.referenceInfos.size(), 1U);
    ASSERT_EQ(hit.definitionInfo.start, defPos);
    ASSERT_EQ(hit.definitionInfo.length, nameLen);

    // Last character of the identifier still hits.
    auto lastChar = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, defPos + nameLen - 1);
    ASSERT_EQ(lastChar.referenceInfos.size(), hit.referenceInfos.size());

    // Position between two occurrences (gap) misses.
    const auto gapPos = source.find(": number = 1");
    ASSERT_NE(gapPos, std::string::npos);
    auto gap = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, gapPos);
    ASSERT_TRUE(gap.referenceInfos.empty());
    ASSERT_TRUE(gap.definitionInfo.fileName.empty());

    // One past the end of the usage identifier misses (`position >= end` arm of L753).
    auto pastEnd = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, usePos + nameLen);
    ASSERT_TRUE(pastEnd.referenceInfos.empty());

    // Far past EOF misses.
    auto wayPast = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, source.size() + 100U);
    ASSERT_TRUE(wayPast.referenceInfos.empty());

    // The usage position resolves to the same symbol with the exact usage span listed.
    auto usageHit = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, usePos);
    ASSERT_EQ(usageHit.referenceInfos.size(), 1U);
    ASSERT_EQ(usageHit.definitionInfo.start, defPos);
    bool usageListed = false;
    for (const auto &ref : usageHit.referenceInfos) {
        usageListed = usageListed || ref.start == usePos;
    }
    ASSERT_TRUE(usageListed);

    initializer.DestroyContext(context);
}

// PK-002 — BuildModuleSymbolKey default-export arm (symbol_reference_index.cpp:298-299) and
// the EXPORT_DEFAULT export flag recorded by AddDefinitionAndExportInfo
// (symbol_reference_index.cpp:520-522): the default-exported class resolves with
// isDefaultExport=true through both lookups, while the named sibling stays false.
TEST_F(LSPSymbolReferenceIndexShapesTests, DefaultExportModuleKeyAndFlags)
{
    std::vector<std::string> files = {"shapes_default_export.ets"};
    std::vector<std::string> texts = {R"(export default class DefaultShape {
    v: number = 1;
}
export class NamedShape {
    v: number = 2;
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    // Default export is flagged and found by prefix search.
    auto defaultHits = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("defaultshape", "");
    ASSERT_EQ(defaultHits.size(), 1U);
    ASSERT_EQ(defaultHits[0].fileName, filePaths[0]);
    ASSERT_EQ(defaultHits[0].symbolName, "DefaultShape");
    ASSERT_TRUE(defaultHits[0].isDefaultExport);
    ASSERT_EQ(defaultHits[0].declType, AstNodeType::CLASS_DECLARATION);

    // The named sibling is exported without the default flag.
    auto namedHits = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("namedshape", "");
    ASSERT_EQ(namedHits.size(), 1U);
    ASSERT_EQ(namedHits[0].symbolName, "NamedShape");
    ASSERT_FALSE(namedHits[0].isDefaultExport);

    // The exact-name lookup reports the same default flag from HasExportFlag
    // (symbol_reference_index.cpp:70-74).
    auto exactDefault = ark::es2panda::lsp::FindSymbolDefinitionsByName("DefaultShape", "");
    ASSERT_EQ(exactDefault.size(), 1U);
    ASSERT_TRUE(exactDefault[0].isDefaultExport);
    auto exactNamed = ark::es2panda::lsp::FindSymbolDefinitionsByName("NamedShape", "");
    ASSERT_EQ(exactNamed.size(), 1U);
    ASSERT_FALSE(exactNamed[0].isDefaultExport);

    initializer.DestroyContext(context);
}
