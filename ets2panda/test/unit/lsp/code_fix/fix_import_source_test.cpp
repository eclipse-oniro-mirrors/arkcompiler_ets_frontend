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
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

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
constexpr std::string_view FIX_REMOVE_DUPLICATE_EXPORT_IMPORT_NAME = "FixRemoveDuplicateExportImport";

struct ImportRenameTestCase {
    std::string name;
    std::string firstDeclaration;
    std::string secondDeclaration;
    int errorCode;
};

class FixImportSourceTest : public LSPAPITests {
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

    std::vector<std::string> CreateTempFile(std::vector<std::string> files, std::vector<std::string> texts)
    {
        std::vector<std::string> result {};
        auto tempDir = GetTestTempDir();
        for (size_t i = 0; i < files.size(); i++) {
            std::filesystem::path outPath = tempDir / files[i];
            std::filesystem::create_directories(outPath.parent_path());
            std::ofstream outStream(outPath);
            if (outStream.fail()) {
                std::cerr << "Failed to open file: " << outPath << std::endl;
                return result;
            }
            outStream << texts[i];
            outStream.close();
            result.push_back(outPath.string());
        }
        return result;
    }

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

    static bool HasSemanticDiagnostic(es2panda_Context *context, int errorCode)
    {
        const auto diagnostics = GetImpl()->getSemanticDiagnostics(context);
        return std::any_of(
            diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(), [errorCode](const Diagnostic &diagnostic) {
                return std::holds_alternative<int>(diagnostic.code_) && std::get<int>(diagnostic.code_) == errorCode;
            });
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

protected:
    static std::string SanitizePathPart(std::string value)
    {
        for (auto &ch : value) {
            if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_' && ch != '-') {
                ch = '_';
            }
        }
        return value;
    }

    std::filesystem::path GetTestTempDir()
    {
        if (!testTempDir_.empty()) {
            return testTempDir_;
        }

        const auto *testInfo = testing::UnitTest::GetInstance()->current_test_info();
        std::string testName = testInfo == nullptr ? "unknown" : testInfo->test_suite_name();
        testName += "_";
        testName += testInfo == nullptr ? "unknown" : testInfo->name();
        testName += "_";
        testName += std::to_string(getpid());

        testTempDir_ = std::filesystem::path(testing::TempDir()) / GetExecutableName() / SanitizePathPart(testName);
        std::filesystem::remove_all(testTempDir_);
        std::filesystem::create_directories(testTempDir_);
        tempFiles_.push_back(testTempDir_);
        return testTempDir_;
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

    std::filesystem::path testTempDir_;
};

class ImportRenameDeclarationKindsTest : public FixImportSourceTest,
                                         public testing::WithParamInterface<ImportRenameTestCase> {};

TEST_F(FixImportSourceTest, ReImportFromDifSourceFile)
{
    std::vector<std::string> fileNames = {"ReImportFromDifSourceFile1.ets", "ReImportFromDifSourceFile2.ets",
                                          "ReImportFromDifSourceFile3.ets"};
    std::vector<std::string> fileContents = {
        R"(
export class Aaaa {};
)",
        R"(
export class Aaaa {};
)",
        R"(
import { Aaaa } from './ReImportFromDifSourceFile1';
import {
    Aaaa
} from './ReImportFromDifSourceFile2';
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto redefinedPos = fileContents[2].rfind("Aaaa");
    ASSERT_NE(redefinedPos, std::string::npos);

    std::vector<int> errorCodes {2349};
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, redefinedPos, redefinedPos + 4, errorCodes, options);
    auto importFix = FindFixImportSource(fixes);
    ASSERT_TRUE(importFix.has_value());

    const auto updated = ApplyFirstChange(fileContents[2], importFix.value());
    const std::string expected = R"(
import { Aaaa } from './ReImportFromDifSourceFile1';

)";
    ASSERT_EQ(updated, expected);

    initializer.DestroyContext(context);
}

TEST_F(FixImportSourceTest, DupExportFromSourceFile1)
{
    std::vector<std::string> fileNames = {"DupExportFromSourceFile1.ets"};
    std::vector<std::string> fileContents = {
        R"(
function fvvv(): void {}
export { fvvv }
export { fvvv };
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto redefinedPos = 35;

    std::vector<int> errorCodes {3073};
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, redefinedPos, redefinedPos + 1, errorCodes, options);
    auto importFix = FindFixImportSource(fixes);
    ASSERT_TRUE(importFix.has_value());

    const auto updated = ApplyFirstChange(fileContents[0], importFix.value());
    const std::string expected = R"(
function fvvv(): void {}
export { fvvv };
)";
    ASSERT_EQ(updated, expected);

    initializer.DestroyContext(context);
}

TEST_F(FixImportSourceTest, DupExportFromSourceFile)
{
    std::vector<std::string> fileNames = {"DupExportFromSourceFile.ets"};
    std::vector<std::string> fileContents = {
        R"(
/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 */

function x(): void {}
function y(): void {}
export { x as Xxxx }
export { y as Xxxx };
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto redefinedPos = fileContents[0].rfind("Xxxx");
    ASSERT_NE(redefinedPos, std::string::npos);

    std::vector<int> errorCodes {1344};
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, redefinedPos, redefinedPos + 4, errorCodes, options);
    auto importFix = FindFixImportSource(fixes);
    ASSERT_TRUE(importFix.has_value());

    const auto updated = ApplyFirstChange(fileContents[0], importFix.value());
    const std::string expected = R"(
/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 */

function x(): void {}
function y(): void {}
export { x as Xxxx }
)";
    ASSERT_EQ(updated, expected);

    initializer.DestroyContext(context);
}

TEST_F(FixImportSourceTest, ImportRenameFunc)
{
    std::vector<std::string> fileNames = {"ImportRenameFunc1.ets", "ImportRenameFunc2.ets", "ImportRenameFunc3.ets"};
    std::vector<std::string> fileContents = {
        R"(
export function Aaaa(): void {};
)",
        R"(
export function Bbbb(): void {};
)",
        R"(
import { Aaaa as Xxxx } from './ImportRenameFunc1';
import {
    Bbbb as Xxxx
} from './ImportRenameFunc2';
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto redefinedPos = fileContents[2].rfind("Xxxx");
    ASSERT_NE(redefinedPos, std::string::npos);

    std::vector<int> errorCodes {505573};
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, redefinedPos, redefinedPos + 1, errorCodes, options);
    auto importFix = FindFixImportSource(fixes);
    ASSERT_TRUE(importFix.has_value());

    const auto updated = ApplyFirstChange(fileContents[2], importFix.value());
    const std::string expected = R"(
import { Aaaa as Xxxx } from './ImportRenameFunc1';

)";
    ASSERT_EQ(updated, expected);

    initializer.DestroyContext(context);
}

TEST_F(FixImportSourceTest, NoFixForOverloadedFunctionOutsideImport)
{
    std::vector<std::string> fileNames = {"OverloadedFunction.ets"};
    std::vector<std::string> fileContents = {
        R"(
class A {
    static foo(): void {}
}
overload bar { A.foo }
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto overloadTargetPos = fileContents[0].find("A.foo");
    ASSERT_NE(overloadTargetPos, std::string::npos);

    std::vector<int> errorCodes {505573};
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, overloadTargetPos, overloadTargetPos + 1,
                                                                errorCodes, options);
    ASSERT_FALSE(FindFixImportSource(fixes).has_value());

    initializer.DestroyContext(context);
}

TEST_F(FixImportSourceTest, ImportRenameClass)
{
    std::vector<std::string> fileNames = {"ImportRenameClass1.ets", "ImportRenameClass2.ets", "ImportRenameClass3.ets"};
    std::vector<std::string> fileContents = {
        R"(
export class Aaaa {};
)",
        R"(
export class Bbbb {};
)",
        R"(
import { Aaaa as Xxxx } from './ImportRenameClass1';
import {
    Bbbb as Xxxx
} from './ImportRenameClass2';
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto redefinedPos = 75;

    std::vector<int> errorCodes {2349};
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, redefinedPos, redefinedPos + 4, errorCodes, options);
    auto importFix = FindFixImportSource(fixes);
    ASSERT_TRUE(importFix.has_value());

    const auto updated = ApplyFirstChange(fileContents[2], importFix.value());
    const std::string expected = R"(
import { Aaaa as Xxxx } from './ImportRenameClass1';

)";
    ASSERT_EQ(updated, expected);

    initializer.DestroyContext(context);
}

TEST_P(ImportRenameDeclarationKindsTest, RemovesConflictingImport)
{
    const auto &testCase = GetParam();
    const std::string baseName = "ImportRename" + testCase.name;
    const std::vector<std::string> fileNames = {baseName + "1.ets", baseName + "2.ets", baseName + "3.ets"};
    const std::vector<std::string> fileContents = {testCase.firstDeclaration + "\n", testCase.secondDeclaration + "\n",
                                                   "import { Aaaa as Xxxx } from './" + baseName +
                                                       "1';\n"
                                                       "import { Bbbb as Xxxx } from './" +
                                                       baseName + "2';\n"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[THIRD_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(HasSemanticDiagnostic(context, testCase.errorCode));

    const auto redefinedPos = fileContents[THIRD_FILE_INDEX].rfind("Xxxx");
    ASSERT_NE(redefinedPos, std::string::npos);
    std::vector<int> errorCodes {testCase.errorCode};
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, redefinedPos, redefinedPos + 4, errorCodes, options);
    auto importFix = FindFixImportSource(fixes);
    ASSERT_TRUE(importFix.has_value());

    const auto updated = ApplyFirstChange(fileContents[THIRD_FILE_INDEX], importFix.value());
    const std::string expected = "import { Aaaa as Xxxx } from './" + baseName + "1';\n\n";
    ASSERT_EQ(updated, expected);
    initializer.DestroyContext(context);
}

INSTANTIATE_TEST_SUITE_P(
    ImportKinds, ImportRenameDeclarationKindsTest,
    testing::Values(ImportRenameTestCase {"Type", "export type Aaaa = int;", "export type Bbbb = string;", 2349},
                    ImportRenameTestCase {"Variable", "export let Aaaa: int = 1;", "export let Bbbb: int = 2;", 2349},
                    ImportRenameTestCase {"Const", "export const Aaaa: int = 1;", "export const Bbbb: int = 2;", 2349},
                    ImportRenameTestCase {"ClassFunction", "export class Aaaa {};", "export function Bbbb(): void {};",
                                          2350},
                    ImportRenameTestCase {"FunctionVariable", "export function Aaaa(): void {};",
                                          "export let Bbbb: int = 1;", 2350}),
    [](const testing::TestParamInfo<ImportRenameTestCase> &info) { return info.param.name; });

TEST_F(FixImportSourceTest, DuplicateImport)
{
    std::vector<std::string> fileNames = {"DuplicateImport1.ets", "DuplicateImport2.ets"};
    std::vector<std::string> fileContents = {
        R"(
export class Aaaa {};
)",
        R"(
import { Aaaa } from './DuplicateImport1';
import { Aaaa } from './DuplicateImport1';
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto duplicateImportPos = fileContents[SECOND_FILE_INDEX].rfind("Aaaa");
    ASSERT_NE(duplicateImportPos, std::string::npos);

    std::vector<int> errorCodes {128428};
    CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, duplicateImportPos, duplicateImportPos + 4,
                                                                errorCodes, options);
    const auto duplicateImportFix = std::find_if(fixes.begin(), fixes.end(), [](const CodeFixActionInfo &fix) {
        return fix.fixName_ == FIX_REMOVE_DUPLICATE_EXPORT_IMPORT_NAME;
    });
    ASSERT_NE(duplicateImportFix, fixes.end());

    const auto updated = ApplyFirstChange(fileContents[SECOND_FILE_INDEX], *duplicateImportFix);
    const std::string expected = R"(
import { Aaaa } from './DuplicateImport1';
)";
    ASSERT_EQ(updated, expected);

    initializer.DestroyContext(context);
}

TEST_F(FixImportSourceTest, ImportClassFromSourceFile)
{
    std::vector<std::string> fileNames = {"test1.ets", "test2.ets"};
    std::vector<std::string> fileContents = {
        R"(
export class classA {};
)",
        R"(
let a = new classA();
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    BuildSymbolIndex(initializer, filePaths[FIRST_FILE_INDEX]);

    auto *context = initializer.CreateContext(filePaths[SECOND_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportSourceFixes(context, fileContents[SECOND_FILE_INDEX], "classA");
    auto importFix = FindFixImportSource(fixes);
    ASSERT_TRUE(importFix.has_value());

    const auto updated = ApplyFirstChange(fileContents[SECOND_FILE_INDEX], importFix.value());
    const std::string expected = R"(import { classA } from './test1';
let a = new classA();
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
export class classB {};
)",
        R"(
'use static'
export class classB {};
)",
        R"(
'use static'
let a = new classB();
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    BuildSymbolIndex(initializer, filePaths[FIRST_FILE_INDEX]);
    BuildSymbolIndex(initializer, filePaths[SECOND_FILE_INDEX]);

    auto *context = initializer.CreateContext(filePaths[THIRD_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto fixes = GetImportSourceFixes(context, fileContents[THIRD_FILE_INDEX], "classB");
    auto updatedSources = ApplyImportSourceFixes(fixes, fileContents[THIRD_FILE_INDEX]);

    std::vector<std::string> expected = {
        R"(
'use static'
import { classB } from './ImportMultiClassFromSourceFile1';
let a = new classB();
)",
        R"(
'use static'
import { classB } from './ImportMultiClassFromSourceFile2';
let a = new classB();
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
    std::filesystem::path baseDir = GetTestTempDir() / "sibling_import";
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
