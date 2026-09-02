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
#include "../lsp_api_test.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "generated/code_fix_register.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/symbol_reference_index.h"

namespace {
using ark::es2panda::lsp::Initializer;

constexpr std::string_view IMPORT_FIXES_ID = "ImportFixes";
// G_IMPORT_FIXES_CODE in import_fixes.cpp plus the unresolved-name error codes
constexpr int UNRESOLVED_ERROR_CODE = 1005;
constexpr int DEFAULT_THROTTLE = 20;
constexpr size_t SOURCE_FILE_INDEX = 0;
constexpr size_t CONSUMER_FILE_INDEX = 1;
// three-file scenarios: index of the consumer file, after the two source/barrel files
constexpr size_t THIRD_FILE_INDEX = 2;

class ImportFixesTest1 : public LSPAPITests {
public:
    void SetUp() override
    {
        LSPAPITests::SetUp();
        ark::es2panda::lsp::ClearSymbolReferenceIndex();
    }

    void TearDown() override
    {
        ark::es2panda::lsp::ClearSymbolReferenceIndex();
    }

    static ark::es2panda::lsp::CancellationToken CreateToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static void BuildSymbolIndex(Initializer &initializer, const std::string &filePath)
    {
        auto *context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
        ASSERT_NE(context, nullptr);
        ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));
        initializer.DestroyContext(context);
    }

    static std::vector<CodeFixActionInfo> GetImportFixes(es2panda_Context *context, const std::string &source,
                                                         std::string_view unresolvedName)
    {
        const auto pos = source.find(unresolvedName);
        EXPECT_NE(pos, std::string::npos);
        std::vector<int> errorCodes {UNRESOLVED_ERROR_CODE};
        CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, pos, pos + unresolvedName.size(), errorCodes,
                                                              options);
    }

    static std::vector<const CodeFixActionInfo *> CollectImportFixes(const std::vector<CodeFixActionInfo> &fixes)
    {
        std::vector<const CodeFixActionInfo *> result;
        for (const auto &fix : fixes) {
            if (fix.fixName_ == IMPORT_FIXES_ID) {
                result.push_back(&fix);
            }
        }
        return result;
    }

    static std::string ApplyFirstChange(const std::string &source, const CodeFixActionInfo &action)
    {
        EXPECT_FALSE(action.changes_.empty());
        EXPECT_FALSE(action.changes_[0].textChanges.empty());
        const auto &change = action.changes_[0].textChanges[0];
        return source.substr(0, change.span.start) + change.newText +
               source.substr(change.span.start + change.span.length);
    }

    static std::vector<std::string> ApplyAllImportFixes(const std::string &source,
                                                        const std::vector<CodeFixActionInfo> &fixes)
    {
        std::vector<std::string> updated;
        for (const auto *fix : CollectImportFixes(fixes)) {
            updated.push_back(ApplyFirstChange(source, *fix));
        }
        std::sort(updated.begin(), updated.end());
        updated.erase(std::unique(updated.begin(), updated.end()), updated.end());
        return updated;
    }

private:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    static NullCancellationToken &GetNullHost()
    {
        static NullCancellationToken instance;
        return instance;
    }
};

// named export: unresolved symbol gets a new named import statement inserted
TEST_F(ImportFixesTest1, InsertImportForNamedExport)
{
    std::vector<std::string> fileNames = {"IfNamedSource.ets", "IfNamedConsumer.ets"};
    std::vector<std::string> fileContents = {R"(export function helperFn(): void {})",
                                             R"(function use(): void {
    helperFn();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[SOURCE_FILE_INDEX]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[CONSUMER_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportFixes(context, fileContents[CONSUMER_FILE_INDEX], "helperFn");
    initializer.DestroyContext(context);

    auto importFixes = CollectImportFixes(fixes);
    ASSERT_EQ(importFixes.size(), 1U);
    EXPECT_EQ(importFixes[0]->fixName_, IMPORT_FIXES_ID);
    EXPECT_EQ(importFixes[0]->description_, "Add import {helperFn} from './IfNamedSource'");
    const auto &consumer = fileContents[CONSUMER_FILE_INDEX];
    ASSERT_EQ(importFixes[0]->changes_.size(), 1U);
    EXPECT_EQ(importFixes[0]->changes_[0].fileName, filePaths[CONSUMER_FILE_INDEX]);
    ASSERT_EQ(importFixes[0]->changes_[0].textChanges.size(), 1U);
    const auto &change = importFixes[0]->changes_[0].textChanges[0];
    EXPECT_EQ(change.span.start, 0U);
    EXPECT_EQ(change.span.length, 0U);
    EXPECT_EQ(change.newText, "import { helperFn } from './IfNamedSource';\n");
    EXPECT_EQ(ApplyFirstChange(consumer, *importFixes[0]), "import { helperFn } from './IfNamedSource';\n" + consumer);
}

// default export: unresolved symbol gets a default import statement inserted
TEST_F(ImportFixesTest1, InsertImportForDefaultExport)
{
    std::vector<std::string> fileNames = {"IfDefaultSource.ets", "IfDefaultConsumer.ets"};
    std::vector<std::string> fileContents = {R"(export default class MainWidget {})",
                                             R"(function build(): void {
    let w = new MainWidget();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[SOURCE_FILE_INDEX]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[CONSUMER_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportFixes(context, fileContents[CONSUMER_FILE_INDEX], "MainWidget");
    initializer.DestroyContext(context);

    auto importFixes = CollectImportFixes(fixes);
    ASSERT_EQ(importFixes.size(), 1U);
    EXPECT_EQ(importFixes[0]->fixName_, IMPORT_FIXES_ID);
    EXPECT_EQ(importFixes[0]->description_, "Add import MainWidget from './IfDefaultSource'");
    const auto &consumer = fileContents[CONSUMER_FILE_INDEX];
    ASSERT_EQ(importFixes[0]->changes_.size(), 1U);
    EXPECT_EQ(importFixes[0]->changes_[0].fileName, filePaths[CONSUMER_FILE_INDEX]);
    ASSERT_EQ(importFixes[0]->changes_[0].textChanges.size(), 1U);
    const auto &change = importFixes[0]->changes_[0].textChanges[0];
    EXPECT_EQ(change.span.start, 0U);
    EXPECT_EQ(change.span.length, 0U);
    EXPECT_EQ(change.newText, "import MainWidget from './IfDefaultSource';\n");
    EXPECT_EQ(ApplyFirstChange(consumer, *importFixes[0]), "import MainWidget from './IfDefaultSource';\n" + consumer);
}

// local alias export `export { X as Y }`: the symbol reference index does not register either
// the local name X or the alias name Y as an export source, so no import fix can be offered
// (documented behavior; alias exported via import `import { X as Y } from ...; export { Y };` is indexed)
TEST_F(ImportFixesTest1, NoFixForLocalAliasExport)
{
    std::vector<std::string> fileNames = {"IfAliasSource.ets", "IfAliasConsumer.ets"};
    std::vector<std::string> fileContents = {
        R"(function internalFn(): void {}
export { internalFn as publicFn };)",
        R"(function use(): void {
    publicFn();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[SOURCE_FILE_INDEX]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[CONSUMER_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // verified: neither "publicFn" nor "internalFn" is registered in the export index
    EXPECT_TRUE(ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("publicFn", "").empty());
    EXPECT_TRUE(ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("internalFn", "").empty());

    auto fixes = GetImportFixes(context, fileContents[CONSUMER_FILE_INDEX], "publicFn");
    initializer.DestroyContext(context);

    EXPECT_TRUE(CollectImportFixes(fixes).empty());
}

// alias export via import `import { X as Y } from ...; export { Y };`: the alias name Y is
// not registered in the export index either, so no import fix can be offered (documented behavior)
TEST_F(ImportFixesTest1, NoFixForAliasExportViaImport)
{
    std::vector<std::string> fileNames = {"IfAliasOrigin.ets", "IfAliasBarrel.ets", "IfAliasViaConsumer.ets"};
    std::vector<std::string> fileContents = {R"(export function rawFn(): void {})",
                                             R"(import { rawFn as renamedFn } from './IfAliasOrigin';
export { renamedFn };)",
                                             R"(function use(): void {
    renamedFn();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[0]);
    BuildSymbolIndex(indexInitializer, filePaths[1]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[THIRD_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // verified: the aliased import name is not registered as an export of the barrel file
    EXPECT_TRUE(ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("renamedFn", "").empty());

    auto fixes = GetImportFixes(context, fileContents[THIRD_FILE_INDEX], "renamedFn");
    initializer.DestroyContext(context);

    EXPECT_TRUE(CollectImportFixes(fixes).empty());
}

// barrel file `import { A } from './a'; export { A };`: the re-exported symbol is not
// registered for the barrel file itself; the fix resolves to the defining origin file
TEST_F(ImportFixesTest1, InsertImportThroughBarrelReExport)
{
    std::vector<std::string> fileNames = {"IfOrigin.ets", "IfBarrel.ets", "IfBarrelConsumer.ets"};
    std::vector<std::string> fileContents = {R"(export class OriginItem {})",
                                             R"(import { OriginItem } from './IfOrigin';
export { OriginItem };)",
                                             R"(function use(): void {
    let i = new OriginItem();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[0]);
    BuildSymbolIndex(indexInitializer, filePaths[1]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[THIRD_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportFixes(context, fileContents[THIRD_FILE_INDEX], "OriginItem");
    initializer.DestroyContext(context);

    auto importFixes = CollectImportFixes(fixes);
    ASSERT_EQ(importFixes.size(), 1U);
    EXPECT_EQ(importFixes[0]->fixName_, IMPORT_FIXES_ID);
    EXPECT_EQ(importFixes[0]->description_, "Add import {OriginItem} from './IfOrigin'");
    const std::vector<std::string> expected = {"import { OriginItem } from './IfOrigin';\n" +
                                               fileContents[THIRD_FILE_INDEX]};
    EXPECT_EQ(ApplyAllImportFixes(fileContents[THIRD_FILE_INDEX], fixes), expected);
}

// direct `export { A } from './a'` is not indexed as an export source by the symbol reference
// index, so no import fix can be offered for the re-exported symbol (documented behavior)
TEST_F(ImportFixesTest1, NoFixForDirectReExportWithoutImport)
{
    std::vector<std::string> fileNames = {"IfReExportOrigin.ets", "IfDirectBarrel.ets", "IfReExportConsumer.ets"};
    std::vector<std::string> fileContents = {R"(export class ReExportItem {})",
                                             R"(export { ReExportItem } from './IfReExportOrigin';)",
                                             R"(function use(): void {
    let i = new ReExportItem();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    // index only the barrel file: its direct re-export is not registered in the export index
    BuildSymbolIndex(indexInitializer, filePaths[1]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[THIRD_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportFixes(context, fileContents[THIRD_FILE_INDEX], "ReExportItem");
    initializer.DestroyContext(context);

    EXPECT_TRUE(CollectImportFixes(fixes).empty());
}

// multiple files export the same symbol name: one fix per source file is offered
TEST_F(ImportFixesTest1, MultipleSourcesWithSameExportName)
{
    std::vector<std::string> fileNames = {"IfDupA.ets", "IfDupB.ets", "IfDupConsumer.ets"};
    std::vector<std::string> fileContents = {R"(export class DupItem {})", R"(export class DupItem {})",
                                             R"(function use(): void {
    let i = new DupItem();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[0]);
    BuildSymbolIndex(indexInitializer, filePaths[1]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[THIRD_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportFixes(context, fileContents[THIRD_FILE_INDEX], "DupItem");
    initializer.DestroyContext(context);

    auto importFixes = CollectImportFixes(fixes);
    ASSERT_EQ(importFixes.size(), 2U);
    std::vector<std::string> descriptions;
    descriptions.reserve(importFixes.size());
    for (const auto *fix : importFixes) {
        descriptions.push_back(fix->description_);
    }
    std::sort(descriptions.begin(), descriptions.end());
    const std::vector<std::string> expectedDescriptions = {"Add import {DupItem} from './IfDupA'",
                                                           "Add import {DupItem} from './IfDupB'"};
    EXPECT_EQ(descriptions, expectedDescriptions);

    const std::vector<std::string> expected = {"import { DupItem } from './IfDupA';\n" + fileContents[THIRD_FILE_INDEX],
                                               "import { DupItem } from './IfDupB';\n" +
                                                   fileContents[THIRD_FILE_INDEX]};
    EXPECT_EQ(ApplyAllImportFixes(fileContents[THIRD_FILE_INDEX], fixes), expected);
}

// existing import from the same module: the fix merges into it instead of adding a duplicate import
TEST_F(ImportFixesTest1, MergeIntoExistingNamedImport)
{
    std::vector<std::string> fileNames = {"IfMergeSource.ets", "IfMergeConsumer.ets"};
    std::vector<std::string> fileContents = {
        R"(export function alphaFn(): void {}
export function betaFn(): void {})",
        R"(import { alphaFn } from './IfMergeSource';
function use(): void {
    alphaFn();
    betaFn();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[SOURCE_FILE_INDEX]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[CONSUMER_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportFixes(context, fileContents[CONSUMER_FILE_INDEX], "betaFn");
    initializer.DestroyContext(context);

    auto importFixes = CollectImportFixes(fixes);
    ASSERT_EQ(importFixes.size(), 1U);
    EXPECT_EQ(importFixes[0]->fixName_, IMPORT_FIXES_ID);
    EXPECT_EQ(importFixes[0]->description_, "Add import {betaFn} from './IfMergeSource'");
    ASSERT_EQ(importFixes[0]->changes_.size(), 1U);
    EXPECT_EQ(importFixes[0]->changes_[0].fileName, filePaths[CONSUMER_FILE_INDEX]);
    ASSERT_EQ(importFixes[0]->changes_[0].textChanges.size(), 1U);
    const auto &change = importFixes[0]->changes_[0].textChanges[0];
    const auto &consumer = fileContents[CONSUMER_FILE_INDEX];
    const auto importDeclStart = consumer.find("import { alphaFn }");
    ASSERT_NE(importDeclStart, std::string::npos);
    const auto importDeclEnd = consumer.find(';', importDeclStart) + 1;
    EXPECT_EQ(change.span.start, importDeclStart);
    EXPECT_EQ(change.span.length, importDeclEnd - importDeclStart);
    EXPECT_EQ(change.newText, "import { alphaFn, betaFn } from './IfMergeSource';");
    EXPECT_EQ(ApplyFirstChange(consumer, *importFixes[0]),
              "import { alphaFn, betaFn } from './IfMergeSource';" + consumer.substr(importDeclEnd));
}

// symbol already imported: no fix is offered and no duplicate import is created
TEST_F(ImportFixesTest1, NoFixWhenSymbolAlreadyImported)
{
    std::vector<std::string> fileNames = {"IfReadySource.ets", "IfReadyConsumer.ets"};
    std::vector<std::string> fileContents = {R"(export function readyFn(): void {})",
                                             R"(import { readyFn } from './IfReadySource';
function use(): void {
    readyFn();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[SOURCE_FILE_INDEX]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[CONSUMER_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportFixes(context, fileContents[CONSUMER_FILE_INDEX], "readyFn();");
    initializer.DestroyContext(context);

    EXPECT_TRUE(CollectImportFixes(fixes).empty());
}

// 'use static' directive: the new import is inserted after the directive, not before it
TEST_F(ImportFixesTest1, InsertImportAfterUseStaticDirective)
{
    std::vector<std::string> fileNames = {"IfStaticSource.ets", "IfStaticConsumer.ets"};
    std::vector<std::string> fileContents = {
        R"('use static'
export class StaticItem {})",
        R"('use static'
function use(): void {
    let s = new StaticItem();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[SOURCE_FILE_INDEX]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[CONSUMER_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportFixes(context, fileContents[CONSUMER_FILE_INDEX], "StaticItem");
    initializer.DestroyContext(context);

    auto importFixes = CollectImportFixes(fixes);
    ASSERT_EQ(importFixes.size(), 1U);
    EXPECT_EQ(importFixes[0]->description_, "Add import {StaticItem} from './IfStaticSource'");
    const auto &consumer = fileContents[CONSUMER_FILE_INDEX];
    ASSERT_EQ(importFixes[0]->changes_.size(), 1U);
    ASSERT_EQ(importFixes[0]->changes_[0].textChanges.size(), 1U);
    const auto &change = importFixes[0]->changes_[0].textChanges[0];
    const auto directiveEnd = consumer.find('\n') + 1;
    EXPECT_EQ(change.span.start, directiveEnd);
    EXPECT_EQ(change.span.length, 0U);
    EXPECT_EQ(change.newText, "import { StaticItem } from './IfStaticSource';\n");
    EXPECT_EQ(ApplyFirstChange(consumer, *importFixes[0]),
              "'use static'\nimport { StaticItem } from './IfStaticSource';\n" + consumer.substr(directiveEnd));
}

// merged import keeps the existing declaration's single-quote module path style
TEST_F(ImportFixesTest1, MergedImportKeepsSingleQuoteStyle)
{
    std::vector<std::string> fileNames = {"IfQuoteSource.ets", "IfQuoteConsumer.ets"};
    std::vector<std::string> fileContents = {
        R"(export function firstFn(): void {}
export function secondFn(): void {})",
        R"(import { firstFn } from './IfQuoteSource';
function use(): void {
    secondFn();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[SOURCE_FILE_INDEX]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[CONSUMER_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportFixes(context, fileContents[CONSUMER_FILE_INDEX], "secondFn");
    initializer.DestroyContext(context);

    auto importFixes = CollectImportFixes(fixes);
    ASSERT_EQ(importFixes.size(), 1U);
    ASSERT_EQ(importFixes[0]->changes_.size(), 1U);
    ASSERT_EQ(importFixes[0]->changes_[0].textChanges.size(), 1U);
    const auto &change = importFixes[0]->changes_[0].textChanges[0];
    // the module path is reused verbatim from the existing declaration, single quotes kept
    EXPECT_EQ(change.newText, "import { firstFn, secondFn } from './IfQuoteSource';");
    EXPECT_EQ(change.newText.find('"'), std::string::npos);
}

// The fix-all public wrapper (GetCombinedCodeFixImpl) dispatches to ImportFixes::GetAllCodeActions,
// which is currently an empty stub: even with a real unresolved-reference diagnostic the combined
// result carries no edits.
TEST_F(ImportFixesTest1, FixAllEntryIsStubAndReturnsNoChanges)
{
    std::vector<std::string> fileNames = {"IfFixAllStub.ets"};
    std::vector<std::string> fileContents = {
        R"(function use(): void {
    unknownFn();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto combined = ark::es2panda::lsp::GetCombinedCodeFixImpl(context, IMPORT_FIXES_ID.data(), options);

    // Current stub behavior: the fix-all entry produces no edits.
    ASSERT_TRUE(combined.changes_.empty());

    initializer.DestroyContext(context);
}

TEST_F(ImportFixesTest1, DISABLED_MergedImportSortsSpecifiersAndPreservesDoubleQuoteStyle)
{
    std::vector<std::string> fileNames = {"IfOrderSource.ets", "IfOrderConsumer.ets"};
    std::vector<std::string> fileContents = {
        R"(export function alphaFn(): void {}
export function zetaFn(): void {})",
        R"(import { zetaFn } from "./IfOrderSource";
function use(): void {
    zetaFn();
    alphaFn();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[SOURCE_FILE_INDEX]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[CONSUMER_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportFixes(context, fileContents[CONSUMER_FILE_INDEX], "alphaFn");
    initializer.DestroyContext(context);

    auto importFixes = CollectImportFixes(fixes);
    ASSERT_EQ(importFixes.size(), 1U);
    ASSERT_EQ(importFixes[0]->changes_.size(), 1U);
    ASSERT_EQ(importFixes[0]->changes_[0].textChanges.size(), 1U);
    const auto &change = importFixes[0]->changes_[0].textChanges[0];
    EXPECT_EQ(change.newText, "import { alphaFn, zetaFn } from \"./IfOrderSource\";");
}

}  // namespace
