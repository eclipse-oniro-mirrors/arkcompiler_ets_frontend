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
#include <optional>
#include <string>
#include <vector>

#include "lsp/include/cancellation_token.h"
#include "lsp/include/symbol_reference_index.h"

namespace {
using ark::es2panda::lsp::Initializer;

constexpr std::string_view FIX_IMPORT_SOURCE_NAME = "FixImportSource";
constexpr int DEFAULT_THROTTLE = 20;
// LSP diagnostic codes dispatched to FixImportSource (type multiplier: SYNTAX 1000 / SEMANTIC 2000
// / WARNING 3000 on top of the generated per-diagnostic numbers).
constexpr int UNRESOLVED_REFERENCE_LSP_CODE = 2143;      // diagnostic::UNRESOLVED_REFERENCE (SEMANTIC 143)
constexpr int REDEFINITION_LSP_CODE = 2349;              // diagnostic::REDEFINITION (SEMANTIC 349)
constexpr int AMBIGUOUS_EXPORT_LSP_CODE = 2362;          // diagnostic::AMBIGUOUS_EXPORT (SEMANTIC 362)
constexpr int DUPLICATE_EXPORT_ALIASES_LSP_CODE = 3073;  // diagnostic::DUPLICATE_EXPORT_ALIASES (WARNING 73)

class FixImportSourceTest2 : public LSPAPITests {
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

    static std::vector<CodeFixActionInfo> GetFixes(es2panda_Context *context, size_t start, size_t length,
                                                   int errorCode)
    {
        std::vector<int> errorCodes {errorCode};
        CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    }

    static std::vector<CodeFixActionInfo> GetFixesUnresolved(es2panda_Context *context, size_t start, size_t length)
    {
        return GetFixes(context, start, length, UNRESOLVED_REFERENCE_LSP_CODE);
    }

    static std::vector<const CodeFixActionInfo *> CollectImportSourceFixes(const std::vector<CodeFixActionInfo> &fixes)
    {
        std::vector<const CodeFixActionInfo *> result;
        for (const auto &fix : fixes) {
            if (fix.fixName_ == FIX_IMPORT_SOURCE_NAME) {
                result.push_back(&fix);
            }
        }
        return result;
    }

    static const CodeFixActionInfo *RequireSingleImportSourceFix(const std::vector<CodeFixActionInfo> &fixes)
    {
        std::vector<const CodeFixActionInfo *> collected = CollectImportSourceFixes(fixes);
        EXPECT_EQ(collected.size(), 1U);
        return collected.size() == 1U ? collected[0] : nullptr;
    }

    static std::string ApplyFirstChange(const std::string &source, const CodeFixActionInfo &action)
    {
        EXPECT_FALSE(action.changes_.empty());
        EXPECT_FALSE(action.changes_[0].textChanges.empty());
        const auto &change = action.changes_[0].textChanges[0];
        return source.substr(0, change.span.start) + change.newText +
               source.substr(change.span.start + change.span.length);
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

// An unresolved symbol whose module is already imported gets merged into the existing import
// statement: the declaration range is replaced by the extended specifier list.
TEST_F(FixImportSourceTest2, MergesIntoExistingNamedImportFromSameSource)
{
    std::vector<std::string> fileNames = {"FisMergeSrc.ets", "FisMergeConsumer.ets"};
    std::vector<std::string> fileContents = {
        R"(export function alphaFn(): void {}
export function betaFn(): void {}
)",
        R"(import { alphaFn } from './FisMergeSrc';
function use(): void {
    betaFn();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[0]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto pos = fileContents[1].find("betaFn");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = GetFixesUnresolved(context, pos, 6);
    initializer.DestroyContext(context);

    const auto *fix = RequireSingleImportSourceFix(fixes);
    ASSERT_NE(fix, nullptr);
    EXPECT_EQ(fix->description_, "Add import {betaFn} from './FisMergeSrc'");

    const auto &consumer = fileContents[1];
    const auto importStart = consumer.find("import { alphaFn }");
    ASSERT_NE(importStart, std::string::npos);
    const auto importEnd = consumer.find(';', importStart) + 1;
    ASSERT_EQ(fix->changes_.size(), 1U);
    ASSERT_EQ(fix->changes_[0].textChanges.size(), 1U);
    const auto &change = fix->changes_[0].textChanges[0];
    EXPECT_EQ(change.span.start, importStart);
    EXPECT_EQ(change.span.length, importEnd - importStart);
    EXPECT_EQ(change.newText, "import { alphaFn, betaFn } from './FisMergeSrc';");
    EXPECT_EQ(ApplyFirstChange(consumer, *fix),
              "import { alphaFn, betaFn } from './FisMergeSrc';" + consumer.substr(importEnd));
}

// A namespace-only import from the target module is not mergeable, so the fix falls back to
// inserting a fresh import statement after the existing one.
TEST_F(FixImportSourceTest2, NamespaceOnlyImportFallsBackToFreshImportStatement)
{
    std::vector<std::string> fileNames = {"FisNsSrc.ets", "FisNsConsumer.ets"};
    std::vector<std::string> fileContents = {
        R"(export function nsTarget(): void {}
)",
        R"(import * as kitAll from './FisNsSrc';
function use(): void {
    nsTarget();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[0]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto pos = fileContents[1].find("nsTarget()");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = GetFixesUnresolved(context, pos, 8);
    initializer.DestroyContext(context);

    const auto *fix = RequireSingleImportSourceFix(fixes);
    ASSERT_NE(fix, nullptr);
    ASSERT_EQ(fix->changes_.size(), 1U);
    ASSERT_EQ(fix->changes_[0].textChanges.size(), 1U);
    const auto &change = fix->changes_[0].textChanges[0];
    const auto &consumer = fileContents[1];
    const auto insertPos = consumer.find(';') + 1;
    EXPECT_EQ(change.span.start, insertPos);
    EXPECT_EQ(change.span.length, 0U);
    EXPECT_EQ(change.newText, "\nimport { nsTarget } from './FisNsSrc';");
    EXPECT_EQ(ApplyFirstChange(consumer, *fix),
              consumer.substr(0, insertPos) + "\nimport { nsTarget } from './FisNsSrc';" + consumer.substr(insertPos));
}

// Requesting the fix on an already-imported specifier name must not offer another import.
TEST_F(FixImportSourceTest2, NoReimportWhenRequestedAtImportedSpecifierName)
{
    std::vector<std::string> fileNames = {"FisOwnSrc.ets", "FisOwnConsumer.ets"};
    std::vector<std::string> fileContents = {
        R"(export function alphaFn(): void {}
)",
        R"(import { alphaFn } from './FisOwnSrc';
function use(): void {
    alphaFn();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[0]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Point at "alphaFn" inside the import specifier itself
    const auto pos = fileContents[1].find("alphaFn");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = GetFixesUnresolved(context, pos, 7);
    initializer.DestroyContext(context);

    EXPECT_TRUE(CollectImportSourceFixes(fixes).empty());
}

// A span touching a non-identifier token (string literal content) yields no action.
TEST_F(FixImportSourceTest2, NonIdentifierSpanYieldsNoAction)
{
    std::vector<std::string> fileNames = {"FisStrConsumer.ets"};
    std::vector<std::string> fileContents = {
        R"(function use(): void {
    let msg = "ghostStr";
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Point inside the string literal content
    const auto pos = fileContents[0].find("ghostStr");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = GetFixesUnresolved(context, pos, 8);
    initializer.DestroyContext(context);

    EXPECT_TRUE(CollectImportSourceFixes(fixes).empty());
}

// An export-conflict error on a module-level declaration that does not start with the export
// keyword cannot build a lowered-export delete range, so no action is offered.
TEST_F(FixImportSourceTest2, ExportConflictWithoutExportKeywordOffersNoAction)
{
    std::vector<std::string> fileNames = {"FisPlainDecl.ets"};
    std::vector<std::string> fileContents = {
        R"(function plainHandler(): void {
    return;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Point at the declaration name; the enclosing declaration starts with "function", not "export"
    const std::string_view declName = "plainHandler";
    const auto pos = fileContents[0].find(declName);
    ASSERT_NE(pos, std::string::npos);
    auto fixes = GetFixes(context, pos, declName.size(), DUPLICATE_EXPORT_ALIASES_LSP_CODE);
    initializer.DestroyContext(context);

    EXPECT_TRUE(CollectImportSourceFixes(fixes).empty());
}

// A lowered export conflict (`export default` twice) removes the whole offending statement,
// including its CRLF line terminator and any trailing blanks before the line break.
TEST_F(FixImportSourceTest2, LoweredDefaultExportConflictRemovesStatementWithCrlf)
{
    const std::string content = "export default class CrlfA {}\r\nexport default class CrlfB {};  \r\n";
    std::vector<std::string> fileNames = {"FisCrlfDup.ets"};
    std::vector<std::string> fileContents = {content};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Point at the second default-declaration name
    const auto pos = content.rfind("CrlfB");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = GetFixes(context, pos, 5, AMBIGUOUS_EXPORT_LSP_CODE);
    initializer.DestroyContext(context);

    const auto *fix = RequireSingleImportSourceFix(fixes);
    ASSERT_NE(fix, nullptr);
    EXPECT_EQ(fix->description_, "Remove ambiguous export");
    ASSERT_EQ(fix->changes_.size(), 1U);
    ASSERT_EQ(fix->changes_[0].textChanges.size(), 1U);
    const auto &change = fix->changes_[0].textChanges[0];
    EXPECT_EQ(change.span.start, content.rfind("export"));
    EXPECT_EQ(change.span.length, content.size() - content.rfind("export"));
    EXPECT_EQ(ApplyFirstChange(content, *fix), "export default class CrlfA {}\r\n");
}

// An out-of-range span under an export-conflict error resolves no touching token; the
// lowered-export fallback anchors on the literal "export" keyword inside a leading comment
// (not at line start) and the delete range runs to end of file.
TEST_F(FixImportSourceTest2, OutOfRangeSpanAnchorsLoweredExportOnCommentedKeyword)
{
    const std::string content = "// export marker\nfunction tail(): void {\n    return;\n}\n";
    std::vector<std::string> fileNames = {"FisOobAnchor.ets"};
    std::vector<std::string> fileContents = {content};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position beyond end-of-file: no token touches it, forcing the keyword-search fallback
    const size_t oobPastEnd = 64;
    const size_t oob = content.size() + oobPastEnd;
    auto fixes = GetFixes(context, oob, 0, DUPLICATE_EXPORT_ALIASES_LSP_CODE);
    initializer.DestroyContext(context);

    const auto *fix = RequireSingleImportSourceFix(fixes);
    ASSERT_NE(fix, nullptr);
    EXPECT_EQ(fix->description_, "Remove ambiguous export");
    ASSERT_EQ(fix->changes_.size(), 1U);
    ASSERT_EQ(fix->changes_[0].textChanges.size(), 1U);
    const auto &change = fix->changes_[0].textChanges[0];
    const auto anchorPos = content.find("export");
    EXPECT_EQ(change.span.start, anchorPos);
    EXPECT_EQ(change.span.length, content.size() - anchorPos);
    EXPECT_EQ(change.newText, "");
    EXPECT_EQ(ApplyFirstChange(content, *fix), "// ");
}

// Same out-of-range setup but without any "export" text in the file: the keyword search fails,
// no delete range can be built and the provider stays silent.
TEST_F(FixImportSourceTest2, OutOfRangeSpanWithoutExportTextOffersNoAction)
{
    std::vector<std::string> fileNames = {"FisOobNoExport.ets"};
    std::vector<std::string> fileContents = {"function tail2(): void {\n    return;\n}\n"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const size_t oobPastEnd = 64;
    const size_t oob = fileContents[0].size() + oobPastEnd;
    auto fixes = GetFixes(context, oob, 0, DUPLICATE_EXPORT_ALIASES_LSP_CODE);
    initializer.DestroyContext(context);

    EXPECT_TRUE(CollectImportSourceFixes(fixes).empty());
}

// A redefinition between two aliased imports is resolved by deleting the conflicting import
// line even when the file uses CRLF line terminators.
TEST_F(FixImportSourceTest2, RedefinitionConflictRemovesConflictingImportLineWithLf)
{
    const std::string content =
        "import { Aaaa as Xxxx } from './FisCrlfOne';\r\nimport { Aaaa as Xxxx } from './FisCrlfTwo';\r\n";
    std::vector<std::string> fileNames = {"FisCrlfOne.ets", "FisCrlfTwo.ets", "FisCrlfConsumer.ets"};
    std::vector<std::string> fileContents = {"export type Aaaa = int;\n", "export type Bbbb = string;\n", content};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Point at the second alias occurrence
    const auto pos = content.rfind("Xxxx");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = GetFixes(context, pos, 4, REDEFINITION_LSP_CODE);
    initializer.DestroyContext(context);

    const auto *fix = RequireSingleImportSourceFix(fixes);
    ASSERT_NE(fix, nullptr);
    EXPECT_EQ(fix->description_, "Remove conflicting import");
    // ChangeTracker::DeleteNode removes the declaration text only; the line terminator of the
    // deleted statement stays behind as an empty line (same shape as the LF variant upstream).
    EXPECT_EQ(ApplyFirstChange(content, *fix), "import { Aaaa as Xxxx } from './FisCrlfOne';\r\n\r\n");
}

}  // namespace
