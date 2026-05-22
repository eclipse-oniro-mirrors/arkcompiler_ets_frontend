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
                               [](const CodeFixActionInfo &fix) { return fix.fixName_ == "FixImportSource"; });
        if (it == fixes.end()) {
            return std::nullopt;
        }
        return *it;
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
