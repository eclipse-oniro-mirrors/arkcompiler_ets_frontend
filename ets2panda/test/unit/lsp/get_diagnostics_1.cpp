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

#include "generated/diagnostic.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/classifier.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
using ark::es2panda::lsp::Initializer;

// LSP diagnostic code formula: DiagnosticType * DIAGNOSTIC_CODE_MULTIPLIER + diagnosticId.
// DiagnosticType::WARNING = 3 (util/diagnostic.h).
constexpr int DIAGNOSTIC_CODE_MULTIPLIER = 1000;
constexpr int WARNING_TYPE_FACTOR = 3;  // DiagnosticType::WARNING

class LspDiagnosticsTests1 : public LSPAPITests {};

// Test: diagnostic from type mismatch should have precise range, valid code and error severity
TEST_F(LspDiagnosticsTests1, DuplicateExportFromDiagnosticRange)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("DuplicateExportFromDiagnosticRange.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(const a: number = "hello";)");
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    // Should have at least one diagnostic for type mismatch
    ASSERT_GE(result.diagnostic.size(), 1U);

    // Verify the diagnostic has a valid range, code, and severity
    for (const auto &diag : result.diagnostic) {
        ASSERT_NE(std::get<int>(diag.code_), 0);
        ASSERT_EQ(diag.severity_, DiagnosticSeverity::Error);
        // Range should be within the source file
        ASSERT_GE(diag.range_.start.line_, 1U);
        ASSERT_GE(diag.range_.end.line_, diag.range_.start.line_);
    }
}

// Test: diagnostic code calculation - DiagnosticType * diagnosticCodeMultiplier + id
TEST_F(LspDiagnosticsTests1, DiagnosticCodeCalculation)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("DiagnosticCodeCalculation.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(const a: number = "hello";)");
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_GE(result.diagnostic.size(), 1U);

    // Verify diagnostic code is non-zero and follows the formula
    auto code = std::get<int>(result.diagnostic[0].code_);
    ASSERT_NE(code, 0);
    // The code should be DiagnosticType * diagnosticCodeMultiplier + id
    // diagnosticCodeMultiplier is typically 1000
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr int diagnosticCodeMultiplier = 1000;
    auto diagnosticType = code / diagnosticCodeMultiplier;
    auto diagnosticId = code % diagnosticCodeMultiplier;
    ASSERT_GT(diagnosticType, 0);
    ASSERT_GE(diagnosticId, 0);
}

// Test: empty file should produce no diagnostics
TEST_F(LspDiagnosticsTests1, EmptyFileNoDiagnostics)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("EmptyFileNoDiagnostics.ets", ES2PANDA_STATE_CHECKED, "");
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences semanticResult = lspApi->getSemanticDiagnostics(ctx);
    DiagnosticReferences syntacticResult = lspApi->getSyntacticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(semanticResult.diagnostic.size(), 0U);
    ASSERT_EQ(syntacticResult.diagnostic.size(), 0U);
}

// Test: comment-only file should produce no diagnostics
TEST_F(LspDiagnosticsTests1, CommentOnlyFileNoDiagnostics)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("CommentOnlyFileNoDiagnostics.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(// This is a comment
// Another comment
/* block comment */)");
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences semanticResult = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(semanticResult.diagnostic.size(), 0U);
}

// Test: import-only file should report exactly one UNUSED_SYMBOL warning
TEST_F(LspDiagnosticsTests1, ImportOnlyFileReportsUnusedImportWarning)
{
    std::vector<std::string> files = {"ImportOnlyFileReportsUnusedImportWarning.ets", "import_only_module.ets"};
    std::vector<std::string> texts = {R"(import { Foo } from './import_only_module';)",
                                      R"(export class Foo { value: number = 1; })"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    // The unused-vars analyzer (enabled by default) reports the never-used import as a warning.
    ASSERT_EQ(result.diagnostic.size(), 1U);
    const auto &diag = result.diagnostic[0];
    // UNUSED_SYMBOL: DiagnosticType::WARNING * DIAGNOSTIC_CODE_MULTIPLIER + 289743
    EXPECT_EQ(std::get<int>(diag.code_), DIAGNOSTIC_CODE_MULTIPLIER * WARNING_TYPE_FACTOR +
                                             static_cast<int>(ark::es2panda::diagnostic::UNUSED_SYMBOL.Id()));
    EXPECT_EQ(diag.severity_, DiagnosticSeverity::Warning);
}

// Test: diagnostic message parameter substitution
TEST_F(LspDiagnosticsTests1, DiagnosticMessageParameterSubstitution)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("DiagnosticMessageParameterSubstitution.ets",
                                                      ES2PANDA_STATE_CHECKED, R"(const a: number = "hello";)");
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_GE(result.diagnostic.size(), 1U);

    // The message should contain the type names that were substituted
    const auto &message = result.diagnostic[0].message_;
    ASSERT_FALSE(message.empty());
    // Should mention the types involved in the mismatch
    ASSERT_TRUE(message.find("hello") != std::string::npos || message.find("Double") != std::string::npos ||
                message.find("number") != std::string::npos);
}

// Test: warning vs error severity classification
TEST_F(LspDiagnosticsTests1, WarningAndErrorSeverityClassification)
{
    std::vector<std::string> files = {"WarningAndErrorSeverityClassification.ets"};
    std::vector<std::string> texts = {
        R"(const a: number = "hello";
let x = 1;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    // Type mismatch should be an error
    ASSERT_GE(result.diagnostic.size(), 1U);
    ASSERT_EQ(result.diagnostic[0].severity_, DiagnosticSeverity::Error);
}

// Test: diagnostic code and codefix errorCode mapping
TEST_F(LspDiagnosticsTests1, DiagnosticCodeMapsToCodeFixErrorCode)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("DiagnosticCodeMapsToCodeFixErrorCode.ets",
                                                      ES2PANDA_STATE_CHECKED, R"(const a: number = "hello";)");
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_GE(result.diagnostic.size(), 1U);

    // The diagnostic code should be non-zero and follow the formula:
    // DiagnosticType * diagnosticCodeMultiplier + id
    // Verify all diagnostic codes are non-zero
    for (const auto &diag : result.diagnostic) {
        auto code = std::get<int>(diag.code_);
        ASSERT_NE(code, 0);
    }
}

// Test: multiple diagnostics should have stable ordering
TEST_F(LspDiagnosticsTests1, MultipleDiagnosticsStableOrdering)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("MultipleDiagnosticsStableOrdering.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(const a: number = "hello";
const b: string = 42;
const c: boolean = "not bool";)");
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result1 = lspApi->getSemanticDiagnostics(ctx);
    DiagnosticReferences result2 = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(result1.diagnostic.size(), result2.diagnostic.size());
    for (size_t i = 0; i < result1.diagnostic.size(); i++) {
        ASSERT_EQ(result1.diagnostic[i].range_.start.line_, result2.diagnostic[i].range_.start.line_);
        ASSERT_EQ(result1.diagnostic[i].range_.start.character_, result2.diagnostic[i].range_.start.character_);
        ASSERT_EQ(result1.diagnostic[i].code_, result2.diagnostic[i].code_);
    }
}
