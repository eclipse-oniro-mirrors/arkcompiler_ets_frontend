/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include <algorithm>
#include "gtest/gtest.h"
#include "lsp_api_test.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_import_non_exported_member.h"
#include "generated/code_fix_register.h"

namespace {
using ark::es2panda::lsp::codefixes::FIX_IMPORT_NON_EXPORTED_MEMBER;
constexpr auto ERROR_CODES = FIX_IMPORT_NON_EXPORTED_MEMBER.GetSupportedCodeNumbers();
constexpr int DEFAULT_THROTTLE = 20;

class FixImportNonExportedMemberTest : public LSPAPITests {
protected:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    // Returns a static instance of NullCancellationToken for testing
    static NullCancellationToken &GetNullHost()
    {
        static NullCancellationToken instance;
        return instance;
    }

    // Validates the code fix action info by comparing it with expected values
    static void ValidateCodeFixActionInfo(const CodeFixActionInfo &info, const size_t expectedTextChangeStart,
                                          const std::string &expectedFileName)
    {
        ASSERT_EQ(info.fixName_, "FixImportNonExportedMember");
        ASSERT_EQ(info.fixId_, "FixImportNonExportedMember");
        ASSERT_EQ(info.description_, "Fix Import Non Exported Member");
        ASSERT_EQ(info.changes_[0].fileName, expectedFileName);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.start, expectedTextChangeStart);
        ASSERT_EQ(info.changes_[0].textChanges[0].newText, "export ");
    }
};

TEST_F(FixImportNonExportedMemberTest, TestImportNonExportedMemberFix)
{
    // Create test files and their contents
    std::vector<std::string> fileNames = {"MainModuleDone.ets", "TestImportNonExportedMember.ets"};
    std::vector<std::string> fileContents = {
        R"(
        // MainModuleDone.ets
        function myFunction() {
            console.log("Hello World!");
        }
        )",
        R"(
        // TestImportNonExportedMember.ets
        import { myFunction } from './MainModuleDone';  // Error: `myFunction` is not exported

        myFunction();
        )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string importLine = "import { myFunction } from './MainModuleDone';";
    const std::string targetName = "myFunction";
    const auto importLineStart = fileContents[1].find(importLine);
    ASSERT_NE(importLineStart, std::string::npos);
    const size_t start = importLineStart + importLine.find(targetName);
    const size_t length = targetName.size();

    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    const auto expectedStart = fileContents[0].find("function myFunction()");
    ASSERT_NE(expectedStart, std::string::npos);
    ValidateCodeFixActionInfo(fixResult[0], expectedStart, filePaths[0]);

    // Clean up the context after the test
    initializer.DestroyContext(context);
}

TEST_F(FixImportNonExportedMemberTest, TestImportNonExportedMemberFixNotOnSpecifierSingle)
{
    std::vector<std::string> fileNames = {"MainModuleSingle.ets", "TestImportNonExportedMemberSingle.ets"};
    std::vector<std::string> fileContents = {
        R"(
        // MainModuleSingle.ets
        function myFunction() {
            console.log("Hello World!");
        }
        )",
        R"(
        // TestImportNonExportedMemberSingle.ets
        import { myFunction } from './MainModuleSingle';

        myFunction();
        )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string importLine = "import { myFunction } from './MainModuleSingle';";
    const auto importLineStart = fileContents[1].find(importLine);
    ASSERT_NE(importLineStart, std::string::npos);
    const size_t start = importLineStart;
    const size_t length = std::string("import").size();

    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    const auto expectedStart = fileContents[0].find("function myFunction()");
    ASSERT_NE(expectedStart, std::string::npos);
    ValidateCodeFixActionInfo(fixResult[0], expectedStart, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixImportNonExportedMemberTest, TestImportNonExportedMemberFixWithAlias)
{
    std::vector<std::string> fileNames = {"MainModuleAlias.ets", "TestImportNonExportedMemberAlias.ets"};
    std::vector<std::string> fileContents = {
        R"(
        // MainModuleAlias.ets
        function myFunction() {
            console.log("Hello Alias!");
        }
        )",
        R"(
        // TestImportNonExportedMemberAlias.ets
        import { myFunction as myAlias } from './MainModuleAlias';

        myAlias();
        )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string importLine = "import { myFunction as myAlias }";
    const std::string aliasName = "myAlias";
    const auto importLineStart = fileContents[1].find(importLine);
    ASSERT_NE(importLineStart, std::string::npos);
    const size_t start = importLineStart + importLine.find(aliasName);
    const size_t length = aliasName.size();

    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    const auto expectedStart = fileContents[0].find("function myFunction()");
    ASSERT_NE(expectedStart, std::string::npos);
    ValidateCodeFixActionInfo(fixResult[0], expectedStart, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixImportNonExportedMemberTest, TestImportNonExportedMemberFixWithAliasClass)
{
    std::vector<std::string> fileNames = {"MainModuleClassAlias.ets", "TestImportNonExportedMemberClassAlias.ets"};
    std::vector<std::string> fileContents = {
        R"(
        // MainModuleClassAlias.ets
        class MyClass {
            Print(): void {
                console.log("Hello Alias Class!");
            }
        }
        )",
        R"(
        // TestImportNonExportedMemberClassAlias.ets
        import { MyClass as MyAlias } from './MainModuleClassAlias';

        let instance = new MyAlias();
        instance.Print();
        )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string importLine = "import { MyClass as MyAlias } from './MainModuleClassAlias';";
    const std::string aliasName = "MyAlias";
    const auto importLineStart = fileContents[1].find(importLine);
    ASSERT_NE(importLineStart, std::string::npos);
    const size_t start = importLineStart + importLine.find(aliasName);
    const size_t length = aliasName.size();

    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    const auto expectedStart = fileContents[0].find("class MyClass");
    ASSERT_NE(expectedStart, std::string::npos);
    ValidateCodeFixActionInfo(fixResult[0], expectedStart, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixImportNonExportedMemberTest, TestImportNonExportedMemberFixForClass)
{
    std::vector<std::string> fileNames = {"MainModuleClass.ets", "TestImportNonExportedMemberClass.ets"};
    std::vector<std::string> fileContents = {
        R"(
        // MainModuleClass.ets
        class MyClass {
            Print(): void {
                console.log("Hello Class!");
            }
        }
        )",
        R"(
        // TestImportNonExportedMemberClass.ets
        import { MyClass } from './MainModuleClass';

        let instance = new MyClass();
        instance.Print();
        )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string importLine = "import { MyClass } from './MainModuleClass';";
    const std::string className = "MyClass";
    const auto importLineStart = fileContents[1].find(importLine);
    ASSERT_NE(importLineStart, std::string::npos);
    const size_t start = importLineStart + importLine.find(className);
    const size_t length = className.size();

    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    const auto expectedStart = fileContents[0].find("class MyClass");
    ASSERT_NE(expectedStart, std::string::npos);
    ValidateCodeFixActionInfo(fixResult[0], expectedStart, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixImportNonExportedMemberTest, TestImportNonExportedMemberFixMultipleSpecifiers)
{
    std::vector<std::string> fileNames = {"MainModuleMulti.ets", "TestImportNonExportedMemberMulti.ets"};
    std::vector<std::string> fileContents = {
        R"(
        // MainModuleMulti.ets
        function foo() {
            console.log("Hello Foo!");
        }

        function bar() {
            console.log("Hello Bar!");
        }
        )",
        R"(
        // TestImportNonExportedMemberMulti.ets
        import { foo, bar } from './MainModuleMulti';

        bar();
        )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string importLine = "import { foo, bar } from './MainModuleMulti';";
    const std::string targetName = "bar";
    const auto importLineStart = fileContents[1].find(importLine);
    ASSERT_NE(importLineStart, std::string::npos);
    const size_t start = importLineStart + importLine.find(targetName);
    const size_t length = targetName.size();

    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    const auto expectedStart = fileContents[0].find("function bar()");
    ASSERT_NE(expectedStart, std::string::npos);
    ValidateCodeFixActionInfo(fixResult[0], expectedStart, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixImportNonExportedMemberTest, TestImportNonExportedMemberFixNotOnSpecifierMultiple)
{
    std::vector<std::string> fileNames = {"MainModuleMultiAll.ets", "TestImportNonExportedMemberMultiAll.ets"};
    std::vector<std::string> fileContents = {
        R"(
        // MainModuleMultiAll.ets
        function foo() {
            console.log("Hello Foo!");
        }

        function bar() {
            console.log("Hello Bar!");
        }
        )",
        R"(
        // TestImportNonExportedMemberMultiAll.ets
        import { foo, bar } from './MainModuleMultiAll';

        bar();
        )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string importLine = "import { foo, bar } from './MainModuleMultiAll';";
    const auto importLineStart = fileContents[1].find(importLine);
    ASSERT_NE(importLineStart, std::string::npos);
    const size_t start = importLineStart;
    const size_t length = std::string("import").size();

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_.size(), 1U);
    const auto &textChanges = fixResult[0].changes_[0].textChanges;
    ASSERT_EQ(textChanges.size(), 2U);

    const auto expectedFooStart = fileContents[0].find("function foo()");
    const auto expectedBarStart = fileContents[0].find("function bar()");
    ASSERT_NE(expectedFooStart, std::string::npos);
    ASSERT_NE(expectedBarStart, std::string::npos);

    std::vector<size_t> starts = {textChanges[0].span.start, textChanges[1].span.start};
    ASSERT_NE(std::find(starts.begin(), starts.end(), expectedFooStart), starts.end());
    ASSERT_NE(std::find(starts.begin(), starts.end(), expectedBarStart), starts.end());
    ASSERT_EQ(textChanges[0].newText, "export ");
    ASSERT_EQ(textChanges[1].newText, "export ");

    initializer.DestroyContext(context);
}

TEST_F(FixImportNonExportedMemberTest, TestImportNonExportedMemberFixNotApplicableWhenNotOnSpecifier)
{
    std::vector<std::string> fileNames = {"MainModuleNotOn.ets", "TestImportNonExportedMemberNotOn.ets"};
    std::vector<std::string> fileContents = {
        R"(
        // MainModuleNotOn.ets
        export function foo() {
            console.log("Hello Foo!");
        }

        export function bar() {
            console.log("Hello Bar!");
        }
        )",
        R"(
        // TestImportNonExportedMemberNotOn.ets
        import { foo, bar } from './MainModuleNotOn';

        bar();
        )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string importLine = "import { foo, bar } from './MainModuleNotOn';";
    const auto importLineStart = fileContents[1].find(importLine);
    ASSERT_NE(importLineStart, std::string::npos);
    const size_t start = importLineStart;
    const size_t length = std::string("import").size();

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    const bool hasTargetFix = std::any_of(fixResult.begin(), fixResult.end(), [](const CodeFixActionInfo &fix) {
        return fix.fixId_ == "FixImportNonExportedMember";
    });
    ASSERT_FALSE(hasTargetFix);

    initializer.DestroyContext(context);
}

TEST_F(FixImportNonExportedMemberTest, TestImportNonExportedMemberFixImportType)
{
    std::vector<std::string> fileNames = {"MainModuleType.ets", "TestImportNonExportedMemberType.ets"};
    std::vector<std::string> fileContents = {
        R"(
        // MainModuleType.ets
        type MyType = number;
        )",
        R"(
        // TestImportNonExportedMemberType.ets
        import type { MyType } from './MainModuleType';

        let value: MyType;
        )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string importLine = "import type { MyType } from './MainModuleType';";
    const std::string targetName = "MyType";
    const auto importLineStart = fileContents[1].find(importLine);
    ASSERT_NE(importLineStart, std::string::npos);
    const size_t start = importLineStart + importLine.find(targetName);
    const size_t length = targetName.size();

    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    const auto expectedStart = fileContents[0].find("type MyType");
    ASSERT_NE(expectedStart, std::string::npos);
    ValidateCodeFixActionInfo(fixResult[0], expectedStart, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixImportNonExportedMemberTest, TestImportNonExportedMemberFixNotApplicableForDefaultImport)
{
    std::vector<std::string> fileNames = {"MainModuleDefault.ets", "TestImportNonExportedMemberDefault.ets"};
    std::vector<std::string> fileContents = {
        R"(
        // MainModuleDefault.ets
        class DefaultClass {
            Name(): string {
                return "Default";
            }
        }
        )",
        R"(
        // TestImportNonExportedMemberDefault.ets
        import DefaultClass from './MainModuleDefault';

        let instance = new DefaultClass();
        )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string importLine = "import DefaultClass from './MainModuleDefault';";
    const std::string targetName = "DefaultClass";
    const auto importLineStart = fileContents[1].find(importLine);
    ASSERT_NE(importLineStart, std::string::npos);
    const size_t start = importLineStart + importLine.find(targetName);
    const size_t length = targetName.size();

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    const bool hasTargetFix = std::any_of(fixResult.begin(), fixResult.end(), [](const CodeFixActionInfo &fix) {
        return fix.fixId_ == "FixImportNonExportedMember";
    });
    ASSERT_FALSE(hasTargetFix);

    initializer.DestroyContext(context);
}

TEST_F(FixImportNonExportedMemberTest, TestImportNonExportedMemberFixNotApplicableForReExport)
{
    std::vector<std::string> fileNames = {"MainModuleReExport.ets", "TestImportNonExportedMemberReExport.ets"};
    std::vector<std::string> fileContents = {
        R"(
        // MainModuleReExport.ets
        function reExported() {
            console.log("Hello ReExport!");
        }
        )",
        R"(
        // TestImportNonExportedMemberReExport.ets
        export { reExported } from './MainModuleReExport';
        )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string exportLine = "export { reExported } from './MainModuleReExport';";
    const std::string targetName = "reExported";
    const auto exportLineStart = fileContents[1].find(exportLine);
    ASSERT_NE(exportLineStart, std::string::npos);
    const size_t start = exportLineStart + exportLine.find(targetName);
    const size_t length = targetName.size();

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    const bool hasTargetFix = std::any_of(fixResult.begin(), fixResult.end(), [](const CodeFixActionInfo &fix) {
        return fix.fixId_ == "FixImportNonExportedMember";
    });
    ASSERT_FALSE(hasTargetFix);

    initializer.DestroyContext(context);
}

TEST_F(FixImportNonExportedMemberTest, TestImportNonExportedMemberFixNotFoundInTargetFile)
{
    std::vector<std::string> fileNames = {"MainModuleDone_NotFound.ets", "TestImportNonExportedMember_NotFound.ets"};
    std::vector<std::string> fileContents = {
        R"(
        // MainModuleDone_NotFound.ets
        function anotherFunction() {
            console.log("Hello World!");
        }
        )",
        R"(
        // TestImportNonExportedMember_NotFound.ets
        import { myFunction } from './MainModuleDone_NotFound';

        myFunction();
        )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string importLine = "import { myFunction } from './MainModuleDone_NotFound';";
    const std::string targetName = "myFunction";
    const auto importLineStart = fileContents[1].find(importLine);
    ASSERT_NE(importLineStart, std::string::npos);
    const size_t start = importLineStart + importLine.find(targetName);
    const size_t length = targetName.size();

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    const bool hasTargetFix = std::any_of(fixResult.begin(), fixResult.end(), [](const CodeFixActionInfo &fix) {
        return fix.fixId_ == "FixImportNonExportedMember";
    });
    ASSERT_FALSE(hasTargetFix);

    initializer.DestroyContext(context);
}
}  // namespace
