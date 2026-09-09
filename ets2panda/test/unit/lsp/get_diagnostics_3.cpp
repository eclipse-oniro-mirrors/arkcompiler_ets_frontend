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

#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"

namespace {

using ark::es2panda::lsp::Initializer;

// LSP diagnostic code formula: DiagnosticType * DIAGNOSTIC_CODE_MULTIPLIER + diagnosticId.
// DiagnosticType::SYNTAX = 1, SEMANTIC = 2, WARNING = 3 (util/diagnostic.h).
constexpr int DIAGNOSTIC_CODE_MULTIPLIER = 1000;
constexpr int DUPLICATE_EXPORT_ALIASES_CODE = 3073;  // WARNING * 1000 + DUPLICATE_EXPORT_ALIASES(73)
constexpr int FORBIDDEN_ANY_DIAGNOSTIC_CODE =
    DIAGNOSTIC_CODE_MULTIPLIER * 2 + 153545;    // SEMANTIC * 1000 + ANY_TYPE_ANNOTATION_FORBIDDEN(153545)
constexpr int UNEXPECTED_TOKEN_ID_CODE = 1112;  // SYNTAX * 1000 + UNEXPECTED_TOKEN_ID(112)
constexpr int UNRESOLVABLE_ARRAY_CODE = 2301;   // SEMANTIC * 1000 + UNRESOLVABLE_ARRAY(301)
// End columns (1-based, inclusive) of the two boundary ranges asserted below.
constexpr size_t NEWLINE_STOPPED_RANGE_END_CHAR = 13;   // last char of "export { a }"
constexpr size_t SPACED_SEMICOLON_RANGE_END_CHAR = 17;  // last char of "export { a }   ;"

// The duplicate-export scanner (internal_api.cpp FindExportRangeEnd and its helpers) walks the
// raw source text from the second "export" keyword to the statement boundary. These tests pin
// the exact reported range and source text for comment/string/semicolon shapes that only exist
// inside a duplicate export statement without a from-clause: with a from-clause the checker does
// not report DUPLICATE_EXPORT_ALIASES at all.
class LspDiagnosticsTests3 : public LSPAPITests {};

// Expected shape of the single duplicate-export warning: reported range plus raw source text.
struct ExpectedDiagnostic {
    size_t startLine;
    size_t startCharacter;
    size_t endLine;
    size_t endCharacter;
    std::string source;
};

void AssertSingleDuplicateExportDiagnostic(const DiagnosticReferences &result, const ExpectedDiagnostic &expected)
{
    ASSERT_EQ(result.diagnostic.size(), 1U);
    const auto &diag = result.diagnostic[0];
    EXPECT_EQ(std::get<int>(diag.code_), DUPLICATE_EXPORT_ALIASES_CODE);
    EXPECT_EQ(diag.severity_, DiagnosticSeverity::Warning);
    EXPECT_EQ(diag.message_, "Duplicated export aliases for 'a'.");
    EXPECT_EQ(diag.range_.start.line_, expected.startLine);
    EXPECT_EQ(diag.range_.start.character_, expected.startCharacter);
    EXPECT_EQ(diag.range_.end.line_, expected.endLine);
    EXPECT_EQ(diag.range_.end.character_, expected.endCharacter);
    EXPECT_EQ(diag.source_, expected.source);
}

DiagnosticReferences QueryDuplicateExport(Initializer &initializer, const std::string &path)
{
    auto *context = initializer.CreateContext(path.c_str(), ES2PANDA_STATE_CHECKED);
    EXPECT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);
    return result;
}

// A line comment inside the braces consumes "{", ";" and quote characters until the newline; the
// scan then resumes on the next line and absorbs " ;" after the closing brace.
TEST_F(LspDiagnosticsTests3, DuplicateExportRangeLineCommentInsideBraces)
{
    std::vector<std::string> files = {"DuplicateExportRangeLineCommentInsideBraces.ets"};
    std::vector<std::string> texts = {R"(let a: number = 1;
export { a };
export {
    // line comment with ; { " inside
    a,
} ;
console.log(a);
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto result = QueryDuplicateExport(initializer, filePaths[0]);
    AssertSingleDuplicateExportDiagnostic(result, {3U, 1U, 6U, 4U, R"(export {
    // line comment with ; { " inside
    a,
} ;)"});
}

// A block comment inside the braces swallows brace/semicolon/quote delimiters and the trailing
// newline until "*/" closes it; none of those characters may terminate or split the export range.
TEST_F(LspDiagnosticsTests3, DuplicateExportRangeBlockCommentConsumesDelimiters)
{
    std::vector<std::string> files = {"DuplicateExportRangeBlockCommentConsumesDelimiters.ets"};
    std::vector<std::string> texts = {
        "let a: number = 1;\nexport { a };\nexport { /* { ; ' \"\n*/ a };\nconsole.log(a);\n"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto result = QueryDuplicateExport(initializer, filePaths[0]);
    AssertSingleDuplicateExportDiagnostic(result, {3U, 1U, 4U, 8U, "export { /* { ; ' \"\n*/ a };"});
}

// Inside a line comment the state ends at '\r' as well as at '\n', so CRLF sources keep the whole
// statement in one range.
TEST_F(LspDiagnosticsTests3, DuplicateExportRangeCrLfTerminatesLineComment)
{
    std::vector<std::string> files = {"DuplicateExportRangeCrLfTerminatesLineComment.ets"};
    std::vector<std::string> texts = {
        "let a: number = 1;\nexport { a };\nexport { // { ; \" x\r\na };\nconsole.log(a);\n"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto result = QueryDuplicateExport(initializer, filePaths[0]);
    AssertSingleDuplicateExportDiagnostic(result, {3U, 1U, 4U, 5U, "export { // { ; \" x\r\na };"});
}

// The parser supports escaped quotes inside string export aliases. The scanner must keep the
// escaped quote inside the double-quoted alias region instead of ending the string state early,
// so source_ still contains the full raw statement text including the backslash.
TEST_F(LspDiagnosticsTests3, DuplicateExportEscapedQuoteAliasKeepsRawSourceText)
{
    std::vector<std::string> files = {"DuplicateExportEscapedQuoteAliasKeepsRawSourceText.ets"};
    std::vector<std::string> texts = {R"(let a: number = 1;
export { a as "esc\"q" };
export { a as "esc\"q" };
console.log(a);
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences syntacticResult = lspApi->getSyntacticDiagnostics(context);
    DiagnosticReferences semanticResult = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    // Error recovery from the alias parse emits two syntax errors.
    ASSERT_EQ(syntacticResult.diagnostic.size(), 2U);
    EXPECT_EQ(std::get<int>(syntacticResult.diagnostic[0].code_), UNEXPECTED_TOKEN_ID_CODE);
    EXPECT_EQ(syntacticResult.diagnostic[0].severity_, DiagnosticSeverity::Error);
    EXPECT_EQ(std::get<int>(syntacticResult.diagnostic[1].code_), UNEXPECTED_TOKEN_ID_CODE);
    EXPECT_EQ(syntacticResult.diagnostic[1].severity_, DiagnosticSeverity::Error);

    // The duplicate-export warning is reported by the semantic query in source order.
    ASSERT_EQ(semanticResult.diagnostic.size(), 1U);

    const auto &diag = semanticResult.diagnostic[0];
    EXPECT_EQ(std::get<int>(diag.code_), DUPLICATE_EXPORT_ALIASES_CODE);
    EXPECT_EQ(diag.severity_, DiagnosticSeverity::Warning);
    EXPECT_EQ(diag.range_.start.line_, 3U);
    EXPECT_EQ(diag.range_.start.character_, 1U);
    EXPECT_EQ(diag.range_.end.line_, 3U);
    EXPECT_EQ(diag.range_.end.character_, 26U);
    EXPECT_EQ(diag.source_, R"(export { a as "esc\"q" };)");
}

// A comment that starts while the brace depth is zero stops the scan right after the keyword:
// neither the comment nor anything behind it becomes part of the range.
TEST_F(LspDiagnosticsTests3, DuplicateExportRangeStopsAtDepthZeroComment)
{
    std::vector<std::string> files = {"DuplicateExportRangeStopsAtDepthZeroComment.ets"};
    std::vector<std::string> texts = {R"(let a: number = 1;
export { a };
export // { ; ' "
{ a };
console.log(a);
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto result = QueryDuplicateExport(initializer, filePaths[0]);
    AssertSingleDuplicateExportDiagnostic(result, {3U, 1U, 3U, 7U, "export"});
}

// A semicolon on the next line belongs to no statement part of this export: the scan stops at the
// newline and must not absorb it.
TEST_F(LspDiagnosticsTests3, DuplicateExportNewlineSemicolonNotAbsorbed)
{
    std::vector<std::string> files = {"DuplicateExportNewlineSemicolonNotAbsorbed.ets"};
    std::vector<std::string> texts = {R"(let a: number = 1;
export { a };
export { a }
;
console.log(a);
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto result = QueryDuplicateExport(initializer, filePaths[0]);
    AssertSingleDuplicateExportDiagnostic(result, {3U, 1U, 3U, NEWLINE_STOPPED_RANGE_END_CHAR, "export { a }"});
}

// Horizontal spaces between the closing brace and the semicolon are absorbed together with the
// semicolon itself.
TEST_F(LspDiagnosticsTests3, DuplicateExportSemicolonAfterSpacesAbsorbed)
{
    std::vector<std::string> files = {"DuplicateExportSemicolonAfterSpacesAbsorbed.ets"};
    std::vector<std::string> texts = {"let a: number = 1;\nexport { a }  ;\nexport { a }   ;\nconsole.log(a);\n"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto result = QueryDuplicateExport(initializer, filePaths[0]);
    AssertSingleDuplicateExportDiagnostic(result, {3U, 1U, 3U, SPACED_SEMICOLON_RANGE_END_CHAR, "export { a }   ;"});
}

// Type-alias annotation shape: exactly one forbidden-'any' error whose range covers the original
// 'any' token.
TEST_F(LspDiagnosticsTests3, ForbiddenAnyTypeAliasArrayExactSpan)
{
    Initializer initializer;
    auto *context = initializer.CreateContext("ForbiddenAnyTypeAliasArrayExactSpan.ets", ES2PANDA_STATE_CHECKED,
                                              R"(type T = any[]
)");
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    ASSERT_EQ(result.diagnostic.size(), 1U);
    const auto &diag = result.diagnostic[0];
    EXPECT_EQ(std::get<int>(diag.code_), FORBIDDEN_ANY_DIAGNOSTIC_CODE);
    EXPECT_EQ(diag.severity_, DiagnosticSeverity::Error);
    EXPECT_EQ(diag.message_, "'any' type annotation is forbidden");
    EXPECT_EQ(diag.source_, "any");
    EXPECT_EQ(diag.range_.start.line_, 1U);
    EXPECT_EQ(diag.range_.start.character_, 10U);
    EXPECT_EQ(diag.range_.end.line_, 1U);
    EXPECT_EQ(diag.range_.end.character_, 13U);
}

// Class property shape: the forbidden-'any' error keeps covering the original 'any' token even
// when the resizable-array lowering coexists with an array-resolve error for the same property.
TEST_F(LspDiagnosticsTests3, ForbiddenAnyClassPropertyWithCoexistingResolveError)
{
    Initializer initializer;
    auto *context = initializer.CreateContext("ForbiddenAnyClassPropertyWithCoexistingResolveError.ets",
                                              ES2PANDA_STATE_CHECKED, R"(class C {
    p: any[] = []
}
)");
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    ASSERT_EQ(result.diagnostic.size(), 2U);
    const auto &forbiddenAny = result.diagnostic[0];
    EXPECT_EQ(std::get<int>(forbiddenAny.code_), FORBIDDEN_ANY_DIAGNOSTIC_CODE);
    EXPECT_EQ(forbiddenAny.severity_, DiagnosticSeverity::Error);
    EXPECT_EQ(forbiddenAny.source_, "any");
    EXPECT_EQ(forbiddenAny.range_.start.line_, 2U);
    EXPECT_EQ(forbiddenAny.range_.start.character_, 8U);
    EXPECT_EQ(forbiddenAny.range_.end.line_, 2U);
    EXPECT_EQ(forbiddenAny.range_.end.character_, 11U);

    const auto &resolveError = result.diagnostic[1];
    EXPECT_EQ(std::get<int>(resolveError.code_), UNRESOLVABLE_ARRAY_CODE);
    EXPECT_EQ(resolveError.severity_, DiagnosticSeverity::Error);
}

// Function parameter shape: exactly one forbidden-'any' error whose range covers the original
// 'any' token.
TEST_F(LspDiagnosticsTests3, ForbiddenAnyFunctionParameterExactSpan)
{
    Initializer initializer;
    auto *context = initializer.CreateContext("ForbiddenAnyFunctionParameterExactSpan.ets", ES2PANDA_STATE_CHECKED,
                                              R"(function f(p: any[]): void {}
)");
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    ASSERT_EQ(result.diagnostic.size(), 1U);
    const auto &diag = result.diagnostic[0];
    EXPECT_EQ(std::get<int>(diag.code_), FORBIDDEN_ANY_DIAGNOSTIC_CODE);
    EXPECT_EQ(diag.severity_, DiagnosticSeverity::Error);
    EXPECT_EQ(diag.source_, "any");
    EXPECT_EQ(diag.range_.start.line_, 1U);
    EXPECT_EQ(diag.range_.start.character_, 15U);
    EXPECT_EQ(diag.range_.end.line_, 1U);
    EXPECT_EQ(diag.range_.end.character_, 18U);
}

}  // namespace
