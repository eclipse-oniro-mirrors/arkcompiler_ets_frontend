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
#include <filesystem>
#include <fstream>
#include <optional>

#include "generated/code_fix_register.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/symbol_reference_index.h"

namespace {
using ark::es2panda::lsp::Initializer;

constexpr auto ERROR_CODES = ark::es2panda::lsp::codefixes::FIX_IMPORT_SOURCE.GetSupportedCodeNumbers();
constexpr int DEFAULT_THROTTLE = 20;
constexpr size_t FIRST_FILE_INDEX = 0;
constexpr size_t SECOND_FILE_INDEX = 1;
constexpr size_t THIRD_FILE_INDEX = 2;
constexpr std::string_view FIX_IMPORT_SOURCE_NAME = "FixImportSource";

class FixImportSourceTest : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static std::string ApplyFirstChange(const std::string &source, const CodeFixActionInfo &action)
    {
        EXPECT_FALSE(action.changes_.empty());
        EXPECT_FALSE(action.changes_[0].textChanges.empty());
        const auto &change = action.changes_[0].textChanges[0];
        return source.substr(0, change.span.start) + change.newText +
               source.substr(change.span.start + change.span.length);
    }

    static std::optional<CodeFixActionInfo> FindFixImportSource(const std::vector<CodeFixActionInfo> &fixes)
    {
        auto it = std::find_if(fixes.begin(), fixes.end(),
                               [](const CodeFixActionInfo &fix) { return fix.fixName_ == FIX_IMPORT_SOURCE_NAME; });
        if (it == fixes.end()) {
            return std::nullopt;
        }
        return *it;
    }

    static void BuildSymbolIndex(Initializer &initializer, const std::string &filePath)
    {
        auto *context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
        ASSERT_NE(context, nullptr);
        ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));
        initializer.DestroyContext(context);
    }

    static std::vector<CodeFixActionInfo> GetImportSourceFixes(es2panda_Context *context, const std::string &source,
                                                               std::string_view unresolvedName)
    {
        const auto unresolvedPos = source.find(unresolvedName);
        EXPECT_NE(unresolvedPos, std::string::npos);

        std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
        CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(
            context, unresolvedPos, unresolvedPos + unresolvedName.size(), errorCodes, options);
    }

    static std::vector<std::string> ApplyImportSourceFixes(const std::vector<CodeFixActionInfo> &fixes,
                                                           const std::string &source)
    {
        std::vector<std::string> updatedSources;
        for (const auto &fix : fixes) {
            if (fix.fixName_ == FIX_IMPORT_SOURCE_NAME) {
                updatedSources.push_back(ApplyFirstChange(source, fix));
            }
        }
        std::sort(updatedSources.begin(), updatedSources.end());
        updatedSources.erase(std::unique(updatedSources.begin(), updatedSources.end()), updatedSources.end());
        return updatedSources;
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

TEST_F(FixImportSourceTest, ImportClassFromSourceFile)
{
    std::vector<std::string> fileNames = {"test1.ets", "test2.ets"};
    std::vector<std::string> fileContents = {
        R"(
export class Aaaa {};
)",
        R"(
let a = new Aaaa();
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    BuildSymbolIndex(initializer, filePaths[FIRST_FILE_INDEX]);

    auto *context = initializer.CreateContext(filePaths[SECOND_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportSourceFixes(context, fileContents[SECOND_FILE_INDEX], "Aaaa");
    auto importFix = FindFixImportSource(fixes);
    ASSERT_TRUE(importFix.has_value());

    const auto updated = ApplyFirstChange(fileContents[SECOND_FILE_INDEX], importFix.value());
    const std::string expected = R"(import { Aaaa } from './test1';
let a = new Aaaa();
)";
    ASSERT_EQ(updated, expected);

    initializer.DestroyContext(context);
}

TEST_F(FixImportSourceTest, ImportMultiClassFromSourceFile)
{
    std::vector<std::string> fileNames = {"ImportMultiClassFromSourceFile1.ets", "ImportMultiClassFromSourceFile2.ets",
                                          "ImportMultiClassFromSourceFile3.ets"};
    std::vector<std::string> fileContents = {
        R"(
'use static'
export class Bbbb {};
)",
        R"(
'use static'
export class Bbbb {};
)",
        R"(
'use static'
let a = new Bbbb();
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    BuildSymbolIndex(initializer, filePaths[FIRST_FILE_INDEX]);
    BuildSymbolIndex(initializer, filePaths[SECOND_FILE_INDEX]);

    auto *context = initializer.CreateContext(filePaths[THIRD_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportSourceFixes(context, fileContents[THIRD_FILE_INDEX], "Bbbb");
    auto updatedSources = ApplyImportSourceFixes(fixes, fileContents[THIRD_FILE_INDEX]);

    std::vector<std::string> expected = {
        R"(
'use static'
import { Bbbb } from './ImportMultiClassFromSourceFile1';
let a = new Bbbb();
)",
        R"(
'use static'
import { Bbbb } from './ImportMultiClassFromSourceFile2';
let a = new Bbbb();
)"};
    std::sort(expected.begin(), expected.end());
    ASSERT_EQ(updatedSources, expected);

    initializer.DestroyContext(context);
}

TEST_F(FixImportSourceTest, ImportFromSourceFile)
{
    std::vector<std::string> fileNames = {"ExportModule.ets", "Consumer.ets"};
    std::vector<std::string> fileContents = {
        R"(
export function foo(): void {}
)",
        R"(
function test(): void {
    foo();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    // First, index the export file by creating a context for it
    {
        Initializer exportInitializer;
        auto *exportCtx = exportInitializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
        ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(exportCtx);
        exportInitializer.DestroyContext(exportCtx);
    }

    // Now create the consumer context; the symbol index still has export file data
    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const auto unresolvedPos = fileContents[1].find("foo()");
    ASSERT_NE(unresolvedPos, std::string::npos);

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, unresolvedPos, unresolvedPos + 1, errorCodes, options);
    auto importSourceFix = FindFixImportSource(fixes);
    ASSERT_TRUE(importSourceFix.has_value());

    const auto updated = ApplyFirstChange(fileContents[1], importSourceFix.value());
    // The fix should add an import statement with foo
    ASSERT_NE(updated.find("import"), std::string::npos);
    ASSERT_NE(updated.find("foo"), std::string::npos);

    initializer.DestroyContext(context);
}

TEST_F(FixImportSourceTest, NoFixForOwnFileDefinition)
{
    std::vector<std::string> fileNames = {"SelfRef.ets"};
    std::vector<std::string> fileContents = {
        R"(
function bar(): void {
    bar();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // bar() is a recursive call and should be resolved, so no fix needed
    const auto pos = fileContents[0].rfind("bar()");
    ASSERT_NE(pos, std::string::npos);

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, pos, pos + 1, errorCodes, options);
    auto importSourceFix = FindFixImportSource(fixes);
    // bar is defined in the same file, so FixImportSource should not offer a fix
    ASSERT_FALSE(importSourceFix.has_value());

    initializer.DestroyContext(context);
}

TEST_F(FixImportSourceTest, ImportFromSiblingDirectory)
{
    // Create export file in a sibling directory: sibling/ExportModule.ets
    // Create consumer file in: consumer/Consumer.ets
    std::filesystem::path baseDir = std::filesystem::path(testing::TempDir()) / "sibling_import_test";
    std::filesystem::create_directories(baseDir / "sibling");
    std::filesystem::create_directories(baseDir / "consumer");

    std::string exportContent = R"(
export function baz(): void {}
)";
    std::string consumerContent = R"(
function test(): void {
    baz();
}
)";

    auto exportPath = baseDir / "sibling" / "ExportModule.ets";
    auto consumerPath = baseDir / "consumer" / "Consumer.ets";

    {
        std::ofstream out(exportPath);
        out << exportContent;
    }
    {
        std::ofstream out(consumerPath);
        out << consumerContent;
    }

    tempFiles_.push_back(baseDir);

    // Index the export file
    {
        Initializer exportInitializer;
        auto *exportCtx = exportInitializer.CreateContext(exportPath.c_str(), ES2PANDA_STATE_CHECKED);
        ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(exportCtx);
        exportInitializer.DestroyContext(exportCtx);
    }

    // Create consumer context and get code fix
    Initializer initializer;
    auto *context = initializer.CreateContext(consumerPath.c_str(), ES2PANDA_STATE_CHECKED);

    const auto unresolvedPos = consumerContent.find("baz()");
    ASSERT_NE(unresolvedPos, std::string::npos);

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, unresolvedPos, unresolvedPos + 1, errorCodes, options);
    auto importSourceFix = FindFixImportSource(fixes);
    ASSERT_TRUE(importSourceFix.has_value());

    const auto updated = ApplyFirstChange(consumerContent, importSourceFix.value());
    // The fix should produce a relative path with "../" for sibling directory
    ASSERT_NE(updated.find("import"), std::string::npos);
    ASSERT_NE(updated.find("baz"), std::string::npos);
    ASSERT_NE(updated.find("../sibling/ExportModule"), std::string::npos)
        << "Expected '../sibling/ExportModule' but got: " << updated;

    initializer.DestroyContext(context);
}
}  // namespace
