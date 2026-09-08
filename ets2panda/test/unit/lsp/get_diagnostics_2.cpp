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

#include "lsp_api_test.h"

#include <gtest/gtest.h>

#include "generated/diagnostic.h"
#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "util/arktsconfig.h"
#include "util/diagnostic.h"

namespace {

using ark::es2panda::lsp::CancellationToken;
using ark::es2panda::lsp::HostCancellationToken;
using ark::es2panda::lsp::Initializer;

// LSP diagnostic code formula: DiagnosticType * DIAGNOSTIC_CODE_MULTIPLIER + diagnosticId.
// DiagnosticType::FATAL = 0, SYNTAX = 1, SEMANTIC = 2, WARNING = 3 (util/diagnostic.h).
constexpr int DIAGNOSTIC_CODE_MULTIPLIER = 1000;
constexpr int DUPLICATE_EXPORT_ALIASES_CODE = 3073;  // WARNING * 1000 + 73
constexpr int DUPLICATE_IMPORT_CODE = 128428;        // WARNING * 1000 + 125428
constexpr int SYNTAX_TYPE_FACTOR = 1;                // DiagnosticType::SYNTAX
constexpr int SEMANTIC_TYPE_FACTOR = 2;              // DiagnosticType::SEMANTIC
constexpr int DEFAULT_THROTTLE = 20;

class LspDiagnosticsTests2 : public LSPAPITests {
public:
    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        return index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
    }

    static CancellationToken CreateNonCancellationToken()
    {
        return CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static CancellationToken CreateCancelledToken()
    {
        return CancellationToken(DEFAULT_THROTTLE, &GetCancelledHost());
    }

private:
    class NullCancellationToken : public HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    class AlwaysCancelledToken : public HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return true;
        }
    };

    static NullCancellationToken &GetNullHost()
    {
        static NullCancellationToken instance;
        return instance;
    }

    static AlwaysCancelledToken &GetCancelledHost()
    {
        static AlwaysCancelledToken instance;
        return instance;
    }
};

// Diagnostic code must follow the formula DiagnosticType * 1000 + id for every severity class.
TEST_F(LspDiagnosticsTests2, DiagnosticCodeCalculationBySeverityClass)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("DiagnosticCodeCalculationBySeverityClass.ets",
                                                      ES2PANDA_STATE_CHECKED, R"(const a: number = "hello";)");
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto semanticResult = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(semanticResult.diagnostic.size(), 1U);
    const auto &diag = semanticResult.diagnostic[0];
    ASSERT_EQ(std::get<int>(diag.code_), DIAGNOSTIC_CODE_MULTIPLIER * SEMANTIC_TYPE_FACTOR +
                                             static_cast<int>(ark::es2panda::diagnostic::INVALID_ASSIGNMNENT.Id()));
    EXPECT_EQ(diag.severity_, DiagnosticSeverity::Error);

    Initializer initializer2 = Initializer();
    es2panda_Context *ctx2 =
        initializer2.CreateContext("DiagnosticCodeCalculationSyntax.ets", ES2PANDA_STATE_CHECKED, R"(let x = ;)");
    ASSERT_NE(ctx2, nullptr);
    auto syntacticResult = lspApi->getSyntacticDiagnostics(ctx2);
    initializer2.DestroyContext(ctx2);

    ASSERT_GE(syntacticResult.diagnostic.size(), 1U);
    for (const auto &syntaxDiag : syntacticResult.diagnostic) {
        auto code = std::get<int>(syntaxDiag.code_);
        EXPECT_EQ(code / DIAGNOSTIC_CODE_MULTIPLIER, SYNTAX_TYPE_FACTOR);
        EXPECT_GE(code % DIAGNOSTIC_CODE_MULTIPLIER, 0);
        EXPECT_EQ(syntaxDiag.severity_, DiagnosticSeverity::Error);
    }
}

// Warning diagnostics must use DiagnosticType::WARNING * 1000 + id and Warning severity.
TEST_F(LspDiagnosticsTests2, WarningDiagnosticCodeCalculation)
{
    std::vector<std::string> files = {"WarningDiagnosticCodeCalculation.ets"};
    std::vector<std::string> texts = {R"(
let a: number = 1;
export { a };
export { a };
console.log(a);
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    ASSERT_EQ(result.diagnostic.size(), 1U);
    const auto &diag = result.diagnostic[0];
    // DUPLICATE_EXPORT_ALIASES: DiagnosticType::WARNING * DIAGNOSTIC_CODE_MULTIPLIER + 73
    EXPECT_EQ(std::get<int>(diag.code_), DUPLICATE_EXPORT_ALIASES_CODE);
    EXPECT_EQ(diag.severity_, DiagnosticSeverity::Warning);
    EXPECT_EQ(diag.message_, "Duplicated export aliases for 'a'.");
}

// Message parameters {0}/{1}/... must be substituted with the concrete type arguments.
TEST_F(LspDiagnosticsTests2, MessageParameterSubstitution)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("MessageParameterSubstitution.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(const a: number = "hello";)");
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(result.diagnostic.size(), 1U);
    EXPECT_EQ(result.diagnostic[0].message_, R"(Type '"hello"' cannot be assigned to type 'Double')");
}

// Error severity for semantic and syntax diagnostics, Warning severity for warnings.
TEST_F(LspDiagnosticsTests2, SeverityClassification)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("SeverityClassification.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(const a: number = "hello";
let x = ;)");
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto semanticResult = lspApi->getSemanticDiagnostics(ctx);
    auto syntacticResult = lspApi->getSyntacticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(semanticResult.diagnostic.size(), 1U);
    EXPECT_EQ(semanticResult.diagnostic[0].severity_, DiagnosticSeverity::Error);
    ASSERT_GE(syntacticResult.diagnostic.size(), 1U);
    EXPECT_EQ(syntacticResult.diagnostic[0].severity_, DiagnosticSeverity::Error);

    std::vector<std::string> files = {"SeverityClassificationWarning.ets"};
    std::vector<std::string> texts = {R"(
let a: number = 1;
export { a };
export { a };
console.log(a);
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    Initializer initializer2 = Initializer();
    auto *context = initializer2.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    auto warningResult = lspApi->getSemanticDiagnostics(context);
    initializer2.DestroyContext(context);

    ASSERT_EQ(warningResult.diagnostic.size(), 1U);
    EXPECT_EQ(warningResult.diagnostic[0].severity_, DiagnosticSeverity::Warning);
}

// Fatal diagnostics are surfaced to LSP clients as Error severity.  Exercise the
// conversion directly with a file-less fatal diagnostic so this path does not
// depend on a particular parser/checker failure being able to manufacture one.
TEST_F(LspDiagnosticsTests2, FatalDiagnosticIsClassifiedAsError)
{
    ark::es2panda::util::ThrowableDiagnostic fatalDiagnostic(ark::es2panda::util::DiagnosticType::FATAL,
                                                             "fatal diagnostic for lsp severity test");

    auto result = ark::es2panda::lsp::CreateDiagnosticWithoutFile(fatalDiagnostic);

    EXPECT_EQ(result.severity_, DiagnosticSeverity::Error);
    EXPECT_EQ(result.message_, "fatal diagnostic for lsp severity test");
}

// Diagnostic range must exactly cover the offending token.
TEST_F(LspDiagnosticsTests2, RangeCoversOffendingTokenExactly)
{
    const std::string source = R"(const a: number = "hello";)";
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("RangeCoversOffendingTokenExactly.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(result.diagnostic.size(), 1U);
    const auto &range = result.diagnostic[0].range_;
    EXPECT_EQ(range.start.line_, 1U);
    EXPECT_EQ(range.start.character_, 19U);
    EXPECT_EQ(range.end.line_, 1U);
    EXPECT_EQ(range.end.character_, 26U);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t tokenStart = 18;  // 0-based offset of the string literal in the source
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t tokenLength = 7;  // length of "hello" including quotes
    const std::string token = source.substr(tokenStart, tokenLength);
    EXPECT_EQ(token, R"("hello")");
}

// Diagnostic range must exactly cover the duplicate export statement and match the codefix span.
TEST_F(LspDiagnosticsTests2, RangeCoversDuplicateExportStatement)
{
    std::vector<std::string> files = {"RangeCoversDuplicateExportStatement.ets"};
    std::vector<std::string> texts = {R"(
let a: number = 1;
export { a };
export { a };
console.log(a);
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(context);

    ASSERT_EQ(result.diagnostic.size(), 1U);
    const auto &diag = result.diagnostic[0];
    EXPECT_EQ(diag.range_.start.line_, 4U);
    EXPECT_EQ(diag.range_.start.character_, 1U);
    EXPECT_EQ(diag.range_.end.line_, 4U);
    EXPECT_EQ(diag.range_.end.character_, 14U);
    EXPECT_EQ(diag.source_, "export { a };");

    // The codefix span must remove exactly the same range as the diagnostic range.
    const size_t start = LineColToPos(context, diag.range_.start.line_, diag.range_.start.character_);
    const size_t end = LineColToPos(context, diag.range_.end.line_, diag.range_.end.character_);
    std::vector<int> errorCodes {DUPLICATE_EXPORT_ALIASES_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, end, errorCodes, options);
    initializer.DestroyContext(context);

    ASSERT_GE(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 1U);
    EXPECT_EQ(fixResult[0].fixName_, "FixRemoveDuplicateExportImport");
    EXPECT_EQ(fixResult[0].changes_[0].fileName, filePaths[0]);
    EXPECT_EQ(fixResult[0].changes_[0].textChanges[0].span.start, start);
    EXPECT_EQ(fixResult[0].changes_[0].textChanges[0].span.length, end - start);
    EXPECT_EQ(fixResult[0].changes_[0].textChanges[0].newText, "");
}

// source_ must contain the full source text of the token the diagnostic points to.
TEST_F(LspDiagnosticsTests2, SourceFieldContainsTokenText)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("SourceFieldContainsTokenText.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(const a: number = "hello";)");
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(result.diagnostic.size(), 1U);
    EXPECT_EQ(result.diagnostic[0].source_, R"("hello")");
}

// Multiple diagnostics must come in a stable, position-ordered sequence on repeated queries.
TEST_F(LspDiagnosticsTests2, MultipleDiagnosticsStableOrdering)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("MultipleDiagnosticsStableOrdering.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(const a: number = "hello";
const b: string = 42;
const c: boolean = "not bool";)");
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto result1 = lspApi->getSemanticDiagnostics(ctx);
    auto result2 = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(result1.diagnostic.size(), 3U);
    ASSERT_EQ(result2.diagnostic.size(), 3U);
    for (size_t i = 0; i < result1.diagnostic.size(); i++) {
        EXPECT_EQ(result1.diagnostic[i].range_.start.line_, i + 1);
        EXPECT_EQ(result1.diagnostic[i].range_.start.line_, result2.diagnostic[i].range_.start.line_);
        EXPECT_EQ(result1.diagnostic[i].range_.start.character_, result2.diagnostic[i].range_.start.character_);
        EXPECT_EQ(result1.diagnostic[i].code_, result2.diagnostic[i].code_);
        EXPECT_EQ(result1.diagnostic[i].message_, result2.diagnostic[i].message_);
    }
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t thirdDiagIndex = 2;
    // 1-based columns of the offending literals on source lines 1-2 and line 3
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t firstSecondDiagChar = 19;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t thirdDiagChar = 20;
    EXPECT_EQ(result1.diagnostic[0].range_.start.character_, firstSecondDiagChar);
    EXPECT_EQ(result1.diagnostic[1].range_.start.character_, firstSecondDiagChar);
    EXPECT_EQ(result1.diagnostic[thirdDiagIndex].range_.start.character_, thirdDiagChar);
}

// Incomplete syntax must not crash the diagnostics queries.
TEST_F(LspDiagnosticsTests2, IncompleteSyntaxDoesNotCrash)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("IncompleteSyntaxDoesNotCrash.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(function add(a: number, b: number {
    return a + b;
)");
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto syntacticResult = lspApi->getSyntacticDiagnostics(ctx);
    auto semanticResult = lspApi->getSemanticDiagnostics(ctx);
    auto suggestionResult = lspApi->getSuggestionDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_GE(syntacticResult.diagnostic.size(), 1U);
    for (const auto &diag : syntacticResult.diagnostic) {
        EXPECT_NE(std::get<int>(diag.code_), 0);
        EXPECT_EQ(diag.severity_, DiagnosticSeverity::Error);
    }
    SUCCEED() << "semantic diagnostics count: " << semanticResult.diagnostic.size()
              << ", suggestion diagnostics count: " << suggestionResult.diagnostic.size();
}

// Diagnostic ranges near EOF must stay within the source bounds.
TEST_F(LspDiagnosticsTests2, RangeNearEofDoesNotOverflow)
{
    const std::string source = "let x: number = \"hello\"";
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("RangeNearEofDoesNotOverflow.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(result.diagnostic.size(), 1U);
    const auto &diag = result.diagnostic[0];
    EXPECT_EQ(diag.range_.start.line_, 1U);
    EXPECT_EQ(diag.range_.start.character_, 17U);
    EXPECT_EQ(diag.range_.end.line_, 1U);
    EXPECT_EQ(diag.range_.end.character_, 24U);
    EXPECT_LE(diag.range_.end.character_, source.size() + 1);
    EXPECT_EQ(diag.source_, R"("hello")");
}

// An export-only file must not produce semantic or syntactic diagnostics.
TEST_F(LspDiagnosticsTests2, ExportOnlyFileNoDiagnostics)
{
    std::vector<std::string> files = {"ExportOnlyModule.ets", "export_only_consumer.ets"};
    std::vector<std::string> texts = {R"(export function foo(): number {
    return 1;
}
export const bar: string = 'bar';)",
                                      R"(import { foo } from './ExportOnlyModule';
foo();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto semanticResult = lspApi->getSemanticDiagnostics(context);
    auto syntacticResult = lspApi->getSyntacticDiagnostics(context);
    initializer.DestroyContext(context);

    EXPECT_EQ(semanticResult.diagnostic.size(), 0U);
    EXPECT_EQ(syntacticResult.diagnostic.size(), 0U);
}

// Duplicate import diagnostic must carry the token range and the codefix must remove that token.
TEST_F(LspDiagnosticsTests2, DuplicateImportDiagnosticRangeMatchesCodeFix)
{
    std::vector<std::string> files = {"DuplicateImportDiagnosticRangeMatchesCodeFix.ets", "module1.ets"};
    std::vector<std::string> texts = {R"(
import { A, A } from './module1';
console.log(A);
)",
                                      R"(export class A { value: number = 1; })"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(context);

    ASSERT_EQ(result.diagnostic.size(), 1U);
    const auto &diag = result.diagnostic[0];
    // DUPLICATE_IMPORT: DiagnosticType::WARNING * DIAGNOSTIC_CODE_MULTIPLIER + 125428
    EXPECT_EQ(std::get<int>(diag.code_), DUPLICATE_IMPORT_CODE);
    EXPECT_EQ(diag.severity_, DiagnosticSeverity::Warning);
    EXPECT_EQ(diag.message_, "'A' has already imported");
    EXPECT_EQ(diag.range_.start.line_, 2U);
    EXPECT_EQ(diag.range_.start.character_, 13U);
    EXPECT_EQ(diag.range_.end.line_, 2U);
    EXPECT_EQ(diag.range_.end.character_, 14U);
    EXPECT_EQ(diag.source_, "A");

    const size_t start = LineColToPos(context, diag.range_.start.line_, diag.range_.start.character_);
    const size_t end = LineColToPos(context, diag.range_.end.line_, diag.range_.end.character_);
    std::vector<int> errorCodes {DUPLICATE_IMPORT_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, end, errorCodes, options);
    initializer.DestroyContext(context);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 1U);
    EXPECT_EQ(fixResult[0].changes_[0].fileName, filePaths[0]);
    EXPECT_EQ(fixResult[0].changes_[0].textChanges[0].span.start, start);
    EXPECT_EQ(fixResult[0].changes_[0].textChanges[0].span.length, end - start);
    EXPECT_EQ(fixResult[0].changes_[0].textChanges[0].newText, "");
}

// Compiler options diagnostics must report invalid fields of arktsconfig.json.
TEST_F(LspDiagnosticsTests2, CompilerOptionsDiagnosticsInvalidField)
{
    std::vector<std::string> files = {"compiler_options_invalid_field_main.ets", "arktsconfig.json"};
    std::vector<std::string> texts = {R"(
function A(a: number, b: number) {
    return a + b;
}
)",
                                      R"({
    "compilerOptions": {
        "paths": {
            "std": []
        }
    }
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    DiagnosticReferences diagnostics;
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    ark::es2panda::util::DiagnosticEngine *diagnosticEngine =
        reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->diagnosticEngine;
    auto config = ark::es2panda::ArkTsConfig {filePaths[1], *diagnosticEngine};
    config.Parse();
    ark::es2panda::lsp::GetOptionDiagnostics(ctx, diagnostics);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(diagnostics.diagnostic.size(), 1U);
    EXPECT_EQ(diagnostics.diagnostic[0].severity_, DiagnosticSeverity::Error);
    EXPECT_NE(diagnostics.diagnostic[0].message_.find("Substitutions for pattern 'std' shouldn't be an empty array"),
              std::string::npos);
}

// Missing arktsconfig.json must be surfaced as a compiler-options diagnostic.
TEST_F(LspDiagnosticsTests2, CompilerOptionsDiagnosticsMissingArkTsConfig)
{
    std::vector<std::string> files = {"compiler_options_missing_config_main.ets"};
    std::vector<std::string> texts = {R"(function main(): void {})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    DiagnosticReferences diagnostics;
    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    auto *diagnosticEngine = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->diagnosticEngine;
    ASSERT_NE(diagnosticEngine, nullptr);

    const std::string missingConfig = filePaths[0] + ".missing.arktsconfig.json";
    auto config = ark::es2panda::ArkTsConfig {missingConfig, *diagnosticEngine};
    EXPECT_FALSE(config.Parse());
    ark::es2panda::lsp::GetOptionDiagnostics(ctx, diagnostics);
    initializer.DestroyContext(ctx);

    auto missingConfigDiagnostic = std::find_if(
        diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(), [&missingConfig](const Diagnostic &diagnostic) {
            return diagnostic.message_.find("Can't resolve config path") != std::string::npos &&
                   diagnostic.message_.find(missingConfig) != std::string::npos;
        });
    ASSERT_NE(missingConfigDiagnostic, diagnostics.diagnostic.end());
    EXPECT_EQ(missingConfigDiagnostic->severity_, DiagnosticSeverity::Error);
}

// Compiler options diagnostics must report invalid paths of arktsconfig.json.
TEST_F(LspDiagnosticsTests2, CompilerOptionsDiagnosticsInvalidPath)
{
    std::vector<std::string> files = {"compiler_options_invalid_path_main.ets", "arktsconfig.json"};
    std::vector<std::string> texts = {R"(
function A(a: number, b: number) {
    return a + b;
}
)",
                                      R"({
    "compilerOptions": {
        "baseUrl": "./temp",
        "paths": {
            "std": ["./path1"]
        },
        "dependencies": {
            "dynamic_import_tests": {
                "language": "ts",
                "path": "path.d.ets"
            }
        }
    }
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    DiagnosticReferences diagnostics;
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    ark::es2panda::util::DiagnosticEngine *diagnosticEngine =
        reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->diagnosticEngine;
    auto config = ark::es2panda::ArkTsConfig {filePaths[1], *diagnosticEngine};
    config.Parse();
    ark::es2panda::lsp::GetOptionDiagnostics(ctx, diagnostics);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(diagnostics.diagnostic.size(), 1U);
    EXPECT_EQ(diagnostics.diagnostic[0].severity_, DiagnosticSeverity::Error);
    EXPECT_NE(diagnostics.diagnostic[0].message_.find("Interoperability with language 'ts' is not supported"),
              std::string::npos);
}

// A cancellation-requested token must make getCompilerOptionsDiagnostics return no diagnostics.
TEST_F(LspDiagnosticsTests2, CompilerOptionsDiagnosticsCancellation)
{
    std::vector<std::string> files = {"compiler_options_cancellation_main.ets", "arktsconfig.json"};
    std::vector<std::string> texts = {R"(
function A(a: number, b: number) {
    return a + b;
}
)",
                                      R"({
    "compilerOptions": {
        "paths": {
            "std": []
        }
    }
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // The invalid config does produce an option diagnostic when queried directly.
    DiagnosticReferences diagnostics;
    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    ark::es2panda::util::DiagnosticEngine *diagnosticEngine =
        reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->diagnosticEngine;
    auto config = ark::es2panda::ArkTsConfig {filePaths[1], *diagnosticEngine};
    config.Parse();
    ark::es2panda::lsp::GetOptionDiagnostics(ctx, diagnostics);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(diagnostics.diagnostic.size(), 1U);

    // A cancellation-requested token short-circuits the API and returns no diagnostics.
    LSPAPI const *lspApi = GetImpl();
    auto cancelledResult = lspApi->getCompilerOptionsDiagnostics(filePaths[0].c_str(), CreateCancelledToken());
    EXPECT_EQ(cancelledResult.diagnostic.size(), 0U);
}

}  // namespace
