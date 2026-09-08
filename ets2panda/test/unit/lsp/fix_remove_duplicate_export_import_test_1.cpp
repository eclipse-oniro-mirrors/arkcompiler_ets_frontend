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

#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_remove_duplicate_export_import.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_REMOVE_DUPLICATE_EXPORT_IMPORT;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_REMOVE_DUPLICATE_EXPORT_IMPORT.GetFixId();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Remove duplicate export/import";
constexpr auto ERROR_CODES = FIX_REMOVE_DUPLICATE_EXPORT_IMPORT.GetSupportedCodeNumbers();
constexpr int DEFAULT_THROTTLE = 20;

class FixRemoveDuplicateExportImportTests1 : public LSPAPITests {
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

// Test: multi-line re-export should delete the entire statement
TEST_F(FixRemoveDuplicateExportImportTests1, RemovesMultiLineReExportStatement)
{
    std::vector<std::string> fileNames = {"RemovesMultiLineReExportStatement.ets", "multi_line_module.ets"};
    std::vector<std::string> fileContents = {
        R"(export {
  Foo
} from './multi_line_module';
export { Foo } from './multi_line_module';)",
        R"(export class Foo { value: number = 1; })"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at second export "Foo" (line 4, col 10)
    const size_t start = LineColToPos(context, 4, 10);
    const size_t length = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    bool foundDuplicateFix = false;
    for (const auto &result : fixResult) {
        if (result.fixName_ == EXPECTED_FIX_NAME) {
            ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
            ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
            ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
            ASSERT_EQ(result.changes_[0].textChanges.size(), 1U);
            ASSERT_EQ(result.changes_[0].textChanges[0].newText, "");
            ASSERT_GT(result.changes_[0].textChanges[0].span.length, 0U);
            foundDuplicateFix = true;
        }
    }
    ASSERT_TRUE(foundDuplicateFix);

    initializer.DestroyContext(context);
}

// Test: re-export without semicolon should delete to end of line
TEST_F(FixRemoveDuplicateExportImportTests1, RemovesReExportWithoutSemicolon)
{
    std::vector<std::string> fileNames = {"RemovesReExportWithoutSemicolon.ets", "no_semicolon_module.ets"};
    std::vector<std::string> fileContents = {
        R"(export { Foo } from './no_semicolon_module'
export { Foo } from './no_semicolon_module')",
        R"(export class Foo { value: number = 1; })"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at second export "Foo" (line 2, col 10)
    const size_t start = LineColToPos(context, 2, 10);
    const size_t length = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    bool foundDuplicateFix = false;
    for (const auto &result : fixResult) {
        if (result.fixName_ == EXPECTED_FIX_NAME) {
            ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
            ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
            ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
            ASSERT_EQ(result.changes_[0].textChanges.size(), 1U);
            ASSERT_EQ(result.changes_[0].textChanges[0].newText, "");
            ASSERT_GT(result.changes_[0].textChanges[0].span.length, 0U);
            foundDuplicateFix = true;
        }
    }
    ASSERT_TRUE(foundDuplicateFix);

    initializer.DestroyContext(context);
}

// Test: multi-line import with duplicate specifier
TEST_F(FixRemoveDuplicateExportImportTests1, RemovesMultiLineImportDuplicate)
{
    std::vector<std::string> fileNames = {"RemovesMultiLineImportDuplicate.ets", "multiline_import_module.ets"};
    std::vector<std::string> fileContents = {
        R"(import {
  A,
  A
} from './multiline_import_module';)",
        R"(export class A { value: number = 1; })"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at second "A" (line 3, col 3)
    const size_t start = LineColToPos(context, 3, 3);
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

// Test: deletion should not leave residual "from ..." or commas
TEST_F(FixRemoveDuplicateExportImportTests1, DeletionDoesNotLeaveResidualFromKeyword)
{
    std::vector<std::string> fileNames = {"DeletionNoResidual.ets", "no_residual_module.ets"};
    std::vector<std::string> fileContents = {
        R"(import { A } from './no_residual_module';
import { A } from './no_residual_module';
import { B } from './no_residual_module';)",
        R"(export class A { value: number = 1; }
export class B { value: number = 2; })"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at second import "A" (line 2, col 10)
    const size_t start = LineColToPos(context, 2, 10);
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
        // The deleted span should cover the entire duplicate import statement
        ASSERT_GT(result.changes_[0].textChanges[0].span.length, 0U);
    }

    initializer.DestroyContext(context);
}

// Test: single specifier duplicate export { Foo } twice
TEST_F(FixRemoveDuplicateExportImportTests1, RemovesSingleSpecifierDuplicateExport)
{
    std::vector<std::string> fileNames = {"RemovesSingleSpecifierDuplicateExport.ets", "single_spec_module.ets"};
    std::vector<std::string> fileContents = {
        R"(export { Foo } from './single_spec_module';
export { Foo } from './single_spec_module';)",
        R"(export class Foo { value: number = 1; })"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at first export "Foo" (line 1, col 10)
    const size_t start = LineColToPos(context, 1, 10);
    const size_t length = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    bool foundDuplicateFix = false;
    for (const auto &result : fixResult) {
        if (result.fixName_ == EXPECTED_FIX_NAME) {
            ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
            ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
            ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
            ASSERT_EQ(result.changes_[0].textChanges.size(), 1U);
            ASSERT_EQ(result.changes_[0].textChanges[0].newText, "");
            ASSERT_GT(result.changes_[0].textChanges[0].span.length, 0U);
            foundDuplicateFix = true;
        }
    }
    ASSERT_TRUE(foundDuplicateFix);

    initializer.DestroyContext(context);
}

}  // namespace
