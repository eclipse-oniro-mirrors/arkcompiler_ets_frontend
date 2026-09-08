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
// DUPLICATE_EXPORT_ALIASES: DiagnosticType::WARNING * DIAGNOSTIC_CODE_MULTIPLIER + 73
constexpr int DUPLICATE_EXPORT_ALIASES_CODE = 3073;
// DUPLICATE_IMPORT: DiagnosticType::WARNING * DIAGNOSTIC_CODE_MULTIPLIER + 125428
constexpr int DUPLICATE_IMPORT_CODE = 128428;
constexpr int DEFAULT_THROTTLE = 20;

class FixRemoveDuplicateExportImportTests2 : public LSPAPITests {
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

    static Diagnostic FindDiagnosticByCode(es2panda_Context *context, int expectedCode, int occurrence = 0)
    {
        auto diagnostics = GetImpl()->getSemanticDiagnostics(context);
        int index = 0;
        for (const auto &diagnostic : diagnostics.diagnostic) {
            if (std::holds_alternative<int>(diagnostic.code_) && std::get<int>(diagnostic.code_) == expectedCode) {
                if (index == occurrence) {
                    return diagnostic;
                }
                ++index;
            }
        }
        return Diagnostic(Range(), {}, {}, DiagnosticSeverity::Warning, 0, "");
    }

    static void ValidateDeletionFix(const CodeFixActionInfo &fix, const std::string &expectedFileName,
                                    const size_t expectedStart, const size_t expectedLength)
    {
        ASSERT_EQ(fix.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(fix.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(fix.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(fix.changes_.size(), 1U);
        ASSERT_EQ(fix.changes_[0].fileName, expectedFileName);
        ASSERT_EQ(fix.changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(fix.changes_[0].textChanges[0].span.start, expectedStart);
        ASSERT_EQ(fix.changes_[0].textChanges[0].span.length, expectedLength);
        ASSERT_EQ(fix.changes_[0].textChanges[0].newText, "");
    }

    static const CodeFixActionInfo *FindExpectedFix(const std::vector<CodeFixActionInfo> &fixes)
    {
        for (const auto &fix : fixes) {
            if (fix.fixName_ == EXPECTED_FIX_NAME) {
                return &fix;
            }
        }
        return nullptr;
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

// Duplicate import specifiers inside the same import statement: the fix deletes only the repeated
// specifier token, so the rest of the statement (including the "from ..." clause) stays intact.
TEST_F(FixRemoveDuplicateExportImportTests2, RemovesDuplicateSpecifierInsideSingleImport)
{
    std::vector<std::string> fileNames = {"RemovesDuplicateSpecifierInsideSingleImport.ets", "same_import_module.ets"};
    std::vector<std::string> fileContents = {R"(import { A, A } from './same_import_module';
let a = new A();
a.value;)",
                                             R"(export class A { value: number = 1; })"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto diagnostic = FindDiagnosticByCode(context, DUPLICATE_IMPORT_CODE);
    ASSERT_EQ(std::get<int>(diagnostic.code_), DUPLICATE_IMPORT_CODE);
    ASSERT_EQ(diagnostic.message_, "'A' has already imported");
    // The diagnostic points at the second "A" (line 1, col 13, 1-based)
    const size_t start = LineColToPos(context, diagnostic.range_.start.line_, diagnostic.range_.start.character_);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t duplicateImportLine = 1;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t duplicateImportCol = 13;
    ASSERT_EQ(start, LineColToPos(context, duplicateImportLine, duplicateImportCol));

    std::vector<int> errorCodes = {DUPLICATE_IMPORT_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 1, errorCodes, options);
    initializer.DestroyContext(context);

    // Deletes only the repeated specifier token, keeping the from clause of the statement
    ASSERT_EQ(fixResult.size(), 1U);
    ValidateDeletionFix(fixResult[0], filePaths[0], start, 1U);
}

// Duplicate import specifier written on its own line of a multi-line import: the fix deletes only the
// repeated specifier token, so the rest of the multi-line import statement stays intact.
TEST_F(FixRemoveDuplicateExportImportTests2, RemovesWholeLineOfMultiLineImportDuplicate)
{
    std::vector<std::string> fileNames = {"RemovesWholeLineOfMultiLineImportDuplicate.ets", "ml_import_module.ets"};
    std::vector<std::string> fileContents = {R"(import {
    A,
    A
} from './ml_import_module';
let a = new A();
a.value;)",
                                             R"(export class A { value: number = 1; })"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto diagnostic = FindDiagnosticByCode(context, DUPLICATE_IMPORT_CODE);
    ASSERT_EQ(std::get<int>(diagnostic.code_), DUPLICATE_IMPORT_CODE);
    ASSERT_EQ(diagnostic.message_, "'A' has already imported");
    // The diagnostic points at the second "A" (line 3, col 5, 1-based)
    const size_t start = LineColToPos(context, diagnostic.range_.start.line_, diagnostic.range_.start.character_);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t duplicateImportLine = 3;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t duplicateImportCol = 5;
    ASSERT_EQ(start, LineColToPos(context, duplicateImportLine, duplicateImportCol));

    std::vector<int> errorCodes = {DUPLICATE_IMPORT_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 1, errorCodes, options);
    initializer.DestroyContext(context);

    // Deletes only the repeated specifier token on line 3
    ASSERT_EQ(fixResult.size(), 1U);
    ValidateDeletionFix(fixResult[0], filePaths[0], start, 1U);
}

// A duplicate whole import statement is removed together with its line break, so the deletion does not
// leave a residual "from ..." clause or an empty line behind.
TEST_F(FixRemoveDuplicateExportImportTests2, RemovesWholeDuplicateImportLine)
{
    std::vector<std::string> fileNames = {"RemovesWholeDuplicateImportLine.ets", "whole_import_module.ets"};
    std::vector<std::string> fileContents = {R"(import { A } from './whole_import_module';
import { A } from './whole_import_module';
let a = new A();
a.value;)",
                                             R"(export class A { value: number = 1; })"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto diagnostic = FindDiagnosticByCode(context, DUPLICATE_IMPORT_CODE);
    ASSERT_EQ(std::get<int>(diagnostic.code_), DUPLICATE_IMPORT_CODE);
    ASSERT_EQ(diagnostic.message_, "'A' has already imported");
    // The diagnostic points at the repeated "A" specifier of the second import (line 2, col 10, 1-based)
    const size_t start = LineColToPos(context, diagnostic.range_.start.line_, diagnostic.range_.start.character_);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t duplicateImportLine = 2;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t duplicateImportCol = 10;
    ASSERT_EQ(start, LineColToPos(context, duplicateImportLine, duplicateImportCol));

    std::vector<int> errorCodes = {DUPLICATE_IMPORT_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 1, errorCodes, options);

    const size_t lineStart = LineColToPos(context, 2, 1);
    const size_t nextLineStart = LineColToPos(context, 3, 1);
    initializer.DestroyContext(context);

    // Deletes the entire second import statement plus its trailing newline
    ASSERT_EQ(fixResult.size(), 1U);
    ValidateDeletionFix(fixResult[0], filePaths[0], lineStart, nextLineStart - lineStart);
}

// A duplicate import statement without a semicolon is removed up to the end of its line, newline included.
TEST_F(FixRemoveDuplicateExportImportTests2, RemovesDuplicateImportWithoutSemicolonToLineEnd)
{
    std::vector<std::string> fileNames = {"RemovesDuplicateImportWithoutSemicolonToLineEnd.ets", "no_semi_import.ets"};
    std::vector<std::string> fileContents = {R"(import { A } from './no_semi_import'
import { A } from './no_semi_import'
let a = new A();
a.value;)",
                                             R"(export class A { value: number = 1; })"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    auto diagnostic = FindDiagnosticByCode(context, DUPLICATE_IMPORT_CODE);
    ASSERT_EQ(std::get<int>(diagnostic.code_), DUPLICATE_IMPORT_CODE);
    ASSERT_EQ(diagnostic.message_, "'A' has already imported");
    // The diagnostic points at the repeated "A" specifier of the second import (line 2, col 10, 1-based)
    const size_t start = LineColToPos(context, diagnostic.range_.start.line_, diagnostic.range_.start.character_);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t duplicateImportLine = 2;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t duplicateImportCol = 10;
    ASSERT_EQ(start, LineColToPos(context, duplicateImportLine, duplicateImportCol));

    std::vector<int> errorCodes = {DUPLICATE_IMPORT_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 1, errorCodes, options);

    const size_t lineStart = LineColToPos(context, 2, 1);
    const size_t nextLineStart = LineColToPos(context, 3, 1);
    initializer.DestroyContext(context);

    // Deletes the second import statement from its line start to the end of the line
    ASSERT_EQ(fixResult.size(), 1U);
    ValidateDeletionFix(fixResult[0], filePaths[0], lineStart, nextLineStart - lineStart);
}

// Two duplicate exports on the same line: the fix deletes the exact diagnostic span of the repeated
// export statement without touching the preceding export on the same line.
TEST_F(FixRemoveDuplicateExportImportTests2, RemovesSameLineConsecutiveExportSpan)
{
    std::vector<std::string> fileNames = {"RemovesSameLineConsecutiveExportSpan.ets"};
    std::vector<std::string> fileContents = {R"(let a = 1;
let b = 2;
export { a };export { a };
export { b };
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const std::string expectedDuplicateExport = "export { a };";
    auto diagnostic = FindDiagnosticByCode(context, DUPLICATE_EXPORT_ALIASES_CODE);
    ASSERT_EQ(std::get<int>(diagnostic.code_), DUPLICATE_EXPORT_ALIASES_CODE);
    ASSERT_EQ(diagnostic.source_, expectedDuplicateExport);

    const size_t start = LineColToPos(context, diagnostic.range_.start.line_, diagnostic.range_.start.character_);
    const size_t end = LineColToPos(context, diagnostic.range_.end.line_, diagnostic.range_.end.character_);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t duplicateExportLine = 3;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t duplicateExportStartCol = 14;
    const size_t duplicateExportEndCol = duplicateExportStartCol + expectedDuplicateExport.size();
    ASSERT_EQ(start, LineColToPos(context, duplicateExportLine, duplicateExportStartCol));
    ASSERT_EQ(end, LineColToPos(context, duplicateExportLine, duplicateExportEndCol));

    std::vector<int> errorCodes = {DUPLICATE_EXPORT_ALIASES_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, end, errorCodes, options);
    initializer.DestroyContext(context);

    const auto *expectedFix = FindExpectedFix(fixResult);
    ASSERT_NE(expectedFix, nullptr);
    ValidateDeletionFix(*expectedFix, filePaths[0], start, end - start);
}

// A duplicate re-export without semicolon: the deletion covers the exact diagnostic span, which ends at
// the end of the statement line (the closing quote of the module path), not beyond the line.
TEST_F(FixRemoveDuplicateExportImportTests2, RemovesReExportWithoutSemicolonDiagnosticSpan)
{
    std::vector<std::string> fileNames = {"RemovesReExportWithoutSemicolonDiagnosticSpan.ets", "no_semi_reexport.ets"};
    std::vector<std::string> fileContents = {R"(export { Foo } from './no_semi_reexport';
export { Foo } from './no_semi_reexport'
)",
                                             R"(export class Foo { value: number = 1; })"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const std::string expectedDuplicateExport = "export { Foo } from './no_semi_reexport'";
    auto diagnostic = FindDiagnosticByCode(context, DUPLICATE_EXPORT_ALIASES_CODE);
    ASSERT_EQ(std::get<int>(diagnostic.code_), DUPLICATE_EXPORT_ALIASES_CODE);
    ASSERT_EQ(diagnostic.source_, expectedDuplicateExport);

    const size_t start = LineColToPos(context, diagnostic.range_.start.line_, diagnostic.range_.start.character_);
    const size_t end = LineColToPos(context, diagnostic.range_.end.line_, diagnostic.range_.end.character_);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t duplicateExportLine = 2;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t duplicateExportStartCol = 1;
    const size_t duplicateExportEndCol = duplicateExportStartCol + expectedDuplicateExport.size();
    ASSERT_EQ(start, LineColToPos(context, duplicateExportLine, duplicateExportStartCol));
    ASSERT_EQ(end, LineColToPos(context, duplicateExportLine, duplicateExportEndCol));

    std::vector<int> errorCodes = {DUPLICATE_EXPORT_ALIASES_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, end, errorCodes, options);
    initializer.DestroyContext(context);

    const auto *expectedFix = FindExpectedFix(fixResult);
    ASSERT_NE(expectedFix, nullptr);
    ValidateDeletionFix(*expectedFix, filePaths[0], start, end - start);
}

// Fix all with several duplicate imports: every duplicate statement line is deleted and the text changes
// are reported in ascending offset order so they can be applied sequentially.
TEST_F(FixRemoveDuplicateExportImportTests2, FixAllRemovesDuplicateImportsInAscendingOrder)
{
    std::vector<std::string> fileNames = {"FixAllRemovesDuplicateImportsInAscendingOrder.ets", "fix_all_import.ets"};
    std::vector<std::string> fileContents = {R"(import { A } from './fix_all_import';
import { B } from './fix_all_import';
import { A } from './fix_all_import';
import { B } from './fix_all_import';
let a = new A();
let b = new B();
a.value;
b.value;)",
                                             R"(export class A { value: number = 1; }
export class B { value: number = 2; })"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto combinedFix = ark::es2panda::lsp::GetCombinedCodeFixImpl(context, std::string(EXPECTED_FIX_NAME), options);

    const size_t thirdLineStart = LineColToPos(context, 3, 1);
    const size_t fourthLineStart = LineColToPos(context, 4, 1);
    const size_t fifthLineStart = LineColToPos(context, 5, 1);
    initializer.DestroyContext(context);

    // The third and fourth import lines are duplicates and are deleted, in ascending offset order
    ASSERT_EQ(combinedFix.changes_.size(), 1U);
    ASSERT_EQ(combinedFix.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(combinedFix.changes_[0].textChanges.size(), 2U);
    EXPECT_EQ(combinedFix.changes_[0].textChanges[0].span.start, thirdLineStart);
    EXPECT_EQ(combinedFix.changes_[0].textChanges[0].span.length, fourthLineStart - thirdLineStart);
    EXPECT_EQ(combinedFix.changes_[0].textChanges[0].newText, "");
    EXPECT_EQ(combinedFix.changes_[0].textChanges[1].span.start, fourthLineStart);
    EXPECT_EQ(combinedFix.changes_[0].textChanges[1].span.length, fifthLineStart - fourthLineStart);
    EXPECT_EQ(combinedFix.changes_[0].textChanges[1].newText, "");
    EXPECT_LT(combinedFix.changes_[0].textChanges[0].span.start, combinedFix.changes_[0].textChanges[1].span.start);
}

// Fix all with mixed duplicate exports and imports: every duplicate is removed. The combined fix reports
// the changes in ascending offset order, each deletion targeting the duplicate export statement span or
// the whole duplicate import line.
TEST_F(FixRemoveDuplicateExportImportTests2, FixAllRemovesMixedDuplicatesInAscendingOrder)
{
    std::vector<std::string> fileNames = {"FixAllRemovesMixedDuplicatesInAscendingOrder.ets", "fix_all_mixed.ets"};
    std::vector<std::string> fileContents = {R"(import { B } from './fix_all_mixed';
import { B } from './fix_all_mixed';
let a = 1;
let c = 3;
export { a };
export { a };
export { c };
let b = new B();
b.value;)",
                                             R"(export class B { value: number = 1; })"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto combinedFix = ark::es2panda::lsp::GetCombinedCodeFixImpl(context, std::string(EXPECTED_FIX_NAME), options);

    const size_t secondLineStart = LineColToPos(context, 2, 1);
    const size_t thirdLineStart = LineColToPos(context, 3, 1);
    const size_t sixthLineStart = LineColToPos(context, 6, 1);
    initializer.DestroyContext(context);

    // The duplicate import on line 2 (whole line) and the duplicate export on line 6 (statement span),
    // in ascending offset order.
    ASSERT_EQ(combinedFix.changes_.size(), 1U);
    ASSERT_EQ(combinedFix.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(combinedFix.changes_[0].textChanges.size(), 2U);
    EXPECT_EQ(combinedFix.changes_[0].textChanges[0].span.start, secondLineStart);
    EXPECT_EQ(combinedFix.changes_[0].textChanges[0].span.length, thirdLineStart - secondLineStart);
    EXPECT_EQ(combinedFix.changes_[0].textChanges[0].newText, "");
    EXPECT_EQ(combinedFix.changes_[0].textChanges[1].span.start, sixthLineStart);
    EXPECT_EQ(combinedFix.changes_[0].textChanges[1].span.length, std::string("export { a };").size());
    EXPECT_EQ(combinedFix.changes_[0].textChanges[1].newText, "");
}

}  // namespace
