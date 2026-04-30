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

#include "generated/code_fix_register.h"
#include "lsp_api_test.h"
#include "util/diagnostic.h"

#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_remove_duplicate_export_import.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_REMOVE_DUPLICATE_EXPORT_IMPORT;
using ark::es2panda::util::DiagnosticType;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_REMOVE_DUPLICATE_EXPORT_IMPORT.GetFixId();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Remove duplicate export/import";
constexpr auto ERROR_CODES = FIX_REMOVE_DUPLICATE_EXPORT_IMPORT.GetSupportedCodeNumbers();
// DUPLICATE_EXPORT_ALIASES: DiagnosticType::WARNING * DIAGNOSTIC_CODE_MULTIPLIER + 73
constexpr int DUPLICATE_EXPORT_ALIASES_CODE = 3073;
// DUPLICATE_IMPORT: DiagnosticType::WARNING * DIAGNOSTIC_CODE_MULTIPLIER + 125428
constexpr int DUPLICATE_IMPORT_CODE = 128428;
constexpr int DEFAULT_THROTTLE = 20;

class FixRemoveDuplicateExportImportTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto pos = index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
        return pos;
    }

    static void ValidateCodeFixActionInfo(const CodeFixActionInfo &info, const size_t expectedTextChangeStart,
                                          const size_t expectedTextChangeLength, const std::string &expectedFileName)
    {
        ASSERT_EQ(info.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(info.changes_[0].fileName, expectedFileName);
        ASSERT_EQ(info.changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.start, expectedTextChangeStart);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.length, expectedTextChangeLength);
        ASSERT_EQ(info.changes_[0].textChanges[0].newText, "");
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

// Test: duplicate import with different sources should suggest removal
TEST_F(FixRemoveDuplicateExportImportTests, TestFixRemoveDuplicateExport01)
{
    std::vector<std::string> fileNames = {"TestFixRemoveDuplicateExport02.ets"};
    std::vector<std::string> fileContents = {R"(
import { A } from './module1';
import { B } from './module1';
import { A } from './module2';
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at the third import "A" (line 4, col 11)
    const size_t start = LineColToPos(context, 4, 11);
    const size_t length = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    // Verify target error codes: DUPLICATE_EXPORT_ALIASES(3073), DUPLICATE_IMPORT(128428)
    ASSERT_EQ(errorCodes.size(), 2U);
    ASSERT_EQ(errorCodes[0], DUPLICATE_EXPORT_ALIASES_CODE);
    ASSERT_EQ(errorCodes.back(), DUPLICATE_IMPORT_CODE);
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    // Validate fixName, description, and that changes contain deletion
    for (const auto &result : fixResult) {
        ASSERT_EQ(result.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
        ASSERT_EQ(result.changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(result.changes_[0].textChanges[0].newText, "");
        // The duplicate import statement should be deleted, length must be positive
        ASSERT_GT(result.changes_[0].textChanges[0].span.length, 0U);
    }

    initializer.DestroyContext(context);
}

// Test: duplicate import should suggest removal
TEST_F(FixRemoveDuplicateExportImportTests, TestFixRemoveDuplicateImport01)
{
    std::vector<std::string> fileNames = {"TestFixRemoveDuplicateImport01.ets"};
    std::vector<std::string> fileContents = {R"(
import { A } from './module1';
import { A } from './module1';
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at the second import line (line 3, col 1)
    const size_t start = LineColToPos(context, 3, 1);
    const size_t length = 6;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    for (const auto &result : fixResult) {
        ASSERT_EQ(result.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
        ASSERT_EQ(result.changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(result.changes_[0].textChanges[0].newText, "");
        ASSERT_GT(result.changes_[0].textChanges[0].span.length, 0U);
    }

    initializer.DestroyContext(context);
}

// Test: duplicate export with same name should suggest removal
TEST_F(FixRemoveDuplicateExportImportTests, TestFixRemoveDuplicateExportName)
{
    std::vector<std::string> fileNames = {"TestFixRemoveDuplicateExportName.ets"};
    std::vector<std::string> fileContents = {R"(
let a = 1;
export { a };
export { a };
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at second export "a" (line 4, col 10)
    const size_t start = LineColToPos(context, 4, 10);
    const size_t length = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 2U);

    for (const auto &result : fixResult) {
        ASSERT_EQ(result.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
        ASSERT_EQ(result.changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(result.changes_[0].textChanges[0].newText, "");
        ASSERT_GT(result.changes_[0].textChanges[0].span.length, 0U);
    }

    initializer.DestroyContext(context);
}

// Test: duplicate export aliases should suggest removal
TEST_F(FixRemoveDuplicateExportImportTests, TestFixRemoveDuplicateExportAliases)
{
    std::vector<std::string> fileNames = {"TestFixRemoveDuplicateExportAliases.ets"};
    std::vector<std::string> fileContents = {R"(
let x = 1;
let y = 2;
export { x as a };
export { y as a };
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at second export alias "a" (line 5, col 15)
    const size_t start = LineColToPos(context, 5, 15);
    const size_t length = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 2U);

    for (const auto &result : fixResult) {
        ASSERT_EQ(result.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
        ASSERT_EQ(result.changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(result.changes_[0].textChanges[0].newText, "");
        ASSERT_GT(result.changes_[0].textChanges[0].span.length, 0U);
    }

    initializer.DestroyContext(context);
}

// Test: duplicate export on same line separated by semicolons
TEST_F(FixRemoveDuplicateExportImportTests, TestFixRemoveDuplicateExportSameLineSemicolon)
{
    std::vector<std::string> fileNames = {"TestFixRemoveDuplicateExportSameLineSemicolon.ets"};
    std::vector<std::string> fileContents = {R"(
let a = 1;
export { a };export { a };
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at second export "a" (line 3, col 23)
    const size_t start = LineColToPos(context, 3, 23);
    const size_t length = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    for (const auto &result : fixResult) {
        ASSERT_EQ(result.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
        ASSERT_EQ(result.changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(result.changes_[0].textChanges[0].newText, "");
        ASSERT_GT(result.changes_[0].textChanges[0].span.length, 0U);
    }

    initializer.DestroyContext(context);
}

// Test: duplicate export on separate lines (normal case)
TEST_F(FixRemoveDuplicateExportImportTests, TestFixRemoveDuplicateExportSeparateLines)
{
    std::vector<std::string> fileNames = {"TestFixRemoveDuplicateExportSeparateLines.ets"};
    std::vector<std::string> fileContents = {R"(
let a = 1;
export { a };
export { a };
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at second export "a" (line 4, col 10)
    const size_t start = LineColToPos(context, 4, 10);
    const size_t length = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    for (const auto &result : fixResult) {
        ASSERT_EQ(result.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
        ASSERT_EQ(result.changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(result.changes_[0].textChanges[0].newText, "");
        ASSERT_GT(result.changes_[0].textChanges[0].span.length, 0U);
    }

    initializer.DestroyContext(context);
}

}  // namespace
