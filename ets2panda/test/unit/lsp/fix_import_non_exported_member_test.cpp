/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "lsp_api_test.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_import_non_exported_member.h"
#include "generated/code_fix_register.h"

namespace {
using ark::es2panda::lsp::codefixes::FIX_IMPORT_NON_EXPORTED_MEMBER;
constexpr auto ERROR_CODES = FIX_IMPORT_NON_EXPORTED_MEMBER.GetSupportedCodeNumbers();
constexpr int DEFAULT_THROTTLE = 20;
constexpr int EXPECTED_START = 39;

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

    // Converts line and column positions into an offset position in the source code
    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto pos = index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
        return pos;
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
        ASSERT_EQ(info.changes_[0].textChanges[0].newText, "export");
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

    const size_t start = LineColToPos(context, 3, 36);
    const size_t length = 6;

    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);

    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], EXPECTED_START, filePaths[0]);

    // Clean up the context after the test
    initializer.DestroyContext(context);
}
}  // namespace
