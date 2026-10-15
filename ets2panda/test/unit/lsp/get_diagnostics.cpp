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

#include "lsp/include/internal_api.h"
#include "lsp/include/classifier.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

using ark::es2panda::lsp::Initializer;

class LspDiagnosticsTests : public LSPAPITests {};

namespace {
constexpr char FORBIDDEN_ANY_MESSAGE[] = "'any' type annotation is forbidden";

struct ExpectedDiagnosticRange {
    size_t startLine;
    size_t startCharacter;
    size_t endLine;
    size_t endCharacter;
};

void AssertDiagnosticRange(const Diagnostic &diagnostic, const ExpectedDiagnosticRange &expected)
{
    ASSERT_EQ(diagnostic.message_, FORBIDDEN_ANY_MESSAGE);
    ASSERT_EQ(diagnostic.range_.start.line_, expected.startLine);
    ASSERT_EQ(diagnostic.range_.start.character_, expected.startCharacter);
    ASSERT_EQ(diagnostic.range_.end.line_, expected.endLine);
    ASSERT_EQ(diagnostic.range_.end.character_, expected.endCharacter);
}

// Fails only the current test (instead of aborting the whole binary via
// std::bad_variant_access) when the diagnostic code variant stops holding an int.
int GetDiagnosticCode(const Diagnostic &diagnostic)
{
    const auto *code = std::get_if<int>(&diagnostic.code_);
    if (code == nullptr) {
        ADD_FAILURE() << "diagnostic code_ does not hold an int variant";
        return -1;
    }
    return *code;
}
}  // namespace

TEST_F(LspDiagnosticsTests, GetSemanticDiagnostics1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("GetSemanticDiagnosticsNoError1.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(function add(a: number, b: number) {
    return a + b;
}
let n = 333;
let res = add(n, n);)");
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(result.diagnostic.size(), 1);
    ASSERT_EQ(result.diagnostic[0].severity_, DiagnosticSeverity::Warning);
    ASSERT_EQ(result.diagnostic[0].message_, "'res' is never used");
}

TEST_F(LspDiagnosticsTests, GetSemanticDiagnostics2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("GetSemanticDiagnosticsNoError1.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(const a: number = "hello";
function add(a: number, b: number): number {
    return a + b;
}
add("1", 2);)");
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);
    auto const expectedErrorCount = 3;
    ASSERT_EQ(result.diagnostic.size(), expectedErrorCount);
    auto const firstIndex = 0;
    auto const thirdIndex = 2;
    auto const expectedFirstStartLine = 1;
    auto const expectedFirstStartCharacter = 19;
    auto const expectedFirstEndLine = 1;
    auto const expectedFirstEndCharacter = 26;
    ASSERT_EQ(result.diagnostic[firstIndex].range_.start.line_, expectedFirstStartLine);
    ASSERT_EQ(result.diagnostic[firstIndex].range_.start.character_, expectedFirstStartCharacter);
    ASSERT_EQ(result.diagnostic[firstIndex].range_.end.line_, expectedFirstEndLine);
    ASSERT_EQ(result.diagnostic[firstIndex].range_.end.character_, expectedFirstEndCharacter);
    ASSERT_EQ(result.diagnostic[firstIndex].severity_, DiagnosticSeverity::Error);
    ASSERT_NE(GetDiagnosticCode(result.diagnostic[firstIndex]), 0);
    ASSERT_EQ(result.diagnostic[firstIndex].message_, R"(Type '"hello"' cannot be assigned to type 'Double')");
    ASSERT_EQ(result.diagnostic[firstIndex].codeDescription_.href_, "test code description");
    auto const expectedSecondStartLine = 5;
    auto const expectedSecondStartCharacter = 5;
    auto const expectedSecondEndLine = 5;
    auto const expectedSecondEndCharacter = 8;
    ASSERT_EQ(result.diagnostic[thirdIndex].range_.start.line_, expectedSecondStartLine);
    ASSERT_EQ(result.diagnostic[thirdIndex].range_.start.character_, expectedSecondStartCharacter);
    ASSERT_EQ(result.diagnostic[thirdIndex].range_.end.line_, expectedSecondEndLine);
    ASSERT_EQ(result.diagnostic[thirdIndex].range_.end.character_, expectedSecondEndCharacter);
    ASSERT_EQ(result.diagnostic[thirdIndex].severity_, DiagnosticSeverity::Error);
    ASSERT_NE(GetDiagnosticCode(result.diagnostic[thirdIndex]), 0);
    ASSERT_EQ(result.diagnostic[thirdIndex].message_, R"(Type '"1"' is not compatible with type 'Double' at index 1)");
    ASSERT_EQ(result.diagnostic[thirdIndex].codeDescription_.href_, "test code description");
}

TEST_F(LspDiagnosticsTests, GetSemanticDiagnosticsAnyArrayRange)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("GetSemanticDiagnosticsAnyArrayRange.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(let arr: any[] = [1, null, "hello", true, null]
let arr2: any[] = arr)");
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    constexpr size_t expectedErrorCount = 2;
    constexpr size_t firstErrorIndex = 0;
    constexpr size_t secondErrorIndex = 1;
    constexpr size_t firstAnyLine = 1;
    constexpr size_t firstAnyStartCharacter = 10;
    constexpr size_t firstAnyEndCharacter = 13;
    constexpr size_t secondAnyLine = 2;
    constexpr size_t secondAnyStartCharacter = 11;
    constexpr size_t secondAnyEndCharacter = 14;
    constexpr ExpectedDiagnosticRange firstAnyRange {firstAnyLine, firstAnyStartCharacter, firstAnyLine,
                                                     firstAnyEndCharacter};
    constexpr ExpectedDiagnosticRange secondAnyRange {secondAnyLine, secondAnyStartCharacter, secondAnyLine,
                                                      secondAnyEndCharacter};

    ASSERT_EQ(result.diagnostic.size(), expectedErrorCount);
    AssertDiagnosticRange(result.diagnostic[firstErrorIndex], firstAnyRange);
    AssertDiagnosticRange(result.diagnostic[secondErrorIndex], secondAnyRange);
}

// PF-001..PF-005 — forbidden-any array-context fallback chain via the public
// getSemanticDiagnostics API. On `interface I { p: any[] }` the checker emits two
// 'any' type annotation is forbidden diagnostics and the first one is recorded on a
// lowering-cloned identifier whose offset maps back to the property name, so
// FindAnyAtPosition (internal_api.cpp:1228) misses at that offset and the fallback
// chain GetOriginalArrayElementType -> FindOriginalArrayElementTypeAround ->
// FindOriginalArrayElementTypeInSubtree -> IsArrayTypeSearchBoundary -> GetTypeRangeNode
// (internal_api.cpp:1142-1226) reconstructs the real `any` token range for both entries.
TEST_F(LspDiagnosticsTests, GetSemanticDiagnosticsAnyInterfacePropertyRunsArrayFallback)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("GetSemanticDiagnosticsAnyInterfaceProperty.ets", ES2PANDA_STATE_CHECKED,
                                  R"(interface I {
    p: any[]
})");
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    constexpr size_t expectedErrorCount = 2;
    constexpr int expectedErrorCode = 155545;
    constexpr size_t anyLine = 2;
    constexpr size_t anyStartCharacter = 8;
    constexpr size_t anyEndCharacter = 11;
    constexpr ExpectedDiagnosticRange anyRange {anyLine, anyStartCharacter, anyLine, anyEndCharacter};

    ASSERT_EQ(result.diagnostic.size(), expectedErrorCount);
    for (const auto &diagnostic : result.diagnostic) {
        AssertDiagnosticRange(diagnostic, anyRange);
        ASSERT_EQ(diagnostic.severity_, DiagnosticSeverity::Error);
        ASSERT_EQ(GetDiagnosticCode(diagnostic), expectedErrorCode);
        ASSERT_EQ(diagnostic.source_, "any");
    }
}

// PF-005 negative contract: `interface I { p: FixedArray<any> }` has no array-lowering
// original element node, so the fallback breaks on the CLASS_PROPERTY search boundary
// (IsArrayTypeSearchBoundary, internal_api.cpp:1195-1212) and returns nullopt; the
// diagnostic degrades to the touching-token span and text, i.e. the property name `p`.
TEST_F(LspDiagnosticsTests, GetSemanticDiagnosticsForbiddenAnyWithoutArrayElementDegradesToTouchingToken)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("GetSemanticDiagnosticsFixedArrayAny.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(interface I { p: FixedArray<any> })");
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);

    constexpr size_t expectedErrorCount = 1;
    constexpr int expectedErrorCode = 155545;
    constexpr size_t propertyNameLine = 1;
    constexpr size_t propertyNameStartCharacter = 15;
    constexpr size_t propertyNameEndCharacter = 16;
    constexpr ExpectedDiagnosticRange propertyNameRange {propertyNameLine, propertyNameStartCharacter, propertyNameLine,
                                                         propertyNameEndCharacter};

    ASSERT_EQ(result.diagnostic.size(), expectedErrorCount);
    AssertDiagnosticRange(result.diagnostic[0], propertyNameRange);
    ASSERT_EQ(result.diagnostic[0].severity_, DiagnosticSeverity::Error);
    ASSERT_EQ(GetDiagnosticCode(result.diagnostic[0]), expectedErrorCode);
    ASSERT_EQ(result.diagnostic[0].source_, "p");
}

TEST_F(LspDiagnosticsTests, GetSyntacticDiagnostics1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("GetSemanticDiagnosticsNoError1.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(function add(a: number, b: number) {
    return a + b;
}
let n = 333;
let res = add(n, n);)");
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSyntacticDiagnostics(ctx);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(result.diagnostic.size(), 0);
}

TEST_F(LspDiagnosticsTests, GetSyntacticDiagnostics2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("GetSemanticDiagnosticsNoError1.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(functon add(a: number, b: number) {
    return a + b;
}
let n = 333;
let res = add(n, n);)");
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSyntacticDiagnostics(ctx);
    initializer.DestroyContext(ctx);
    auto const expectedErrorCount = 10;
    ASSERT_EQ(result.diagnostic.size(), expectedErrorCount);
    auto const expectedFirstStartLine = 1;
    auto const expectedFirstStartCharacter = 9;
    auto const expectedFirstEndLine = 1;
    auto const expectedFirstEndCharacter = 12;
    ASSERT_EQ(result.diagnostic[0].range_.start.line_, expectedFirstStartLine);
    ASSERT_EQ(result.diagnostic[0].range_.start.character_, expectedFirstStartCharacter);
    ASSERT_EQ(result.diagnostic[0].range_.end.line_, expectedFirstEndLine);
    ASSERT_EQ(result.diagnostic[0].range_.end.character_, expectedFirstEndCharacter);
    ASSERT_EQ(result.diagnostic[0].severity_, DiagnosticSeverity::Error);
    ASSERT_NE(GetDiagnosticCode(result.diagnostic[0]), 0);
    auto const expectedSecondStartLine = 1;
    auto const expectedSecondStartCharacter = 9;
    auto const expectedSecondEndLine = 1;
    auto const expectedSecondEndCharacter = 15;
    ASSERT_EQ(result.diagnostic[1].range_.start.line_, expectedSecondStartLine);
    ASSERT_EQ(result.diagnostic[1].range_.start.character_, expectedSecondStartCharacter);
    ASSERT_EQ(result.diagnostic[1].range_.end.line_, expectedSecondEndLine);
    ASSERT_EQ(result.diagnostic[1].range_.end.character_, expectedSecondEndCharacter);
    ASSERT_EQ(result.diagnostic[1].severity_, DiagnosticSeverity::Error);
    ASSERT_NE(GetDiagnosticCode(result.diagnostic[1]), 0);
    auto const thirdIndex = 2;
    auto const expectedThirdStartLine = 1;
    auto const expectedThirdStartCharacter = 9;
    auto const expectedThirdEndLine = 1;
    auto const expectedThirdEndCharacter = 15;
    ASSERT_EQ(result.diagnostic[thirdIndex].range_.start.line_, expectedThirdStartLine);
    ASSERT_EQ(result.diagnostic[thirdIndex].range_.start.character_, expectedThirdStartCharacter);
    ASSERT_EQ(result.diagnostic[thirdIndex].range_.end.line_, expectedThirdEndLine);
    ASSERT_EQ(result.diagnostic[thirdIndex].range_.end.character_, expectedThirdEndCharacter);
    ASSERT_EQ(result.diagnostic[thirdIndex].severity_, DiagnosticSeverity::Error);
    ASSERT_NE(GetDiagnosticCode(result.diagnostic[thirdIndex]), 0);
    ASSERT_EQ(result.diagnostic[thirdIndex].message_, R"(Unexpected token, expected ',' or ')'.)");
}

TEST_F(LspDiagnosticsTests, GetSyntacticDiagnostics3)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("GetSemanticDiagnosticsNoError1.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(functon add(a: number, b: number) {
    return a + b;
}
let n = 333;
let res = add(n, n);)");
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSyntacticDiagnostics(ctx);
    initializer.DestroyContext(ctx);
    auto const forthIndex = 4;
    auto const expectedForthStartLine = 1;
    auto const expectedForthStartCharacter = 22;
    auto const expectedForthEndLine = 1;
    auto const expectedForthEndCharacter = 23;
    ASSERT_EQ(result.diagnostic[forthIndex].range_.start.line_, expectedForthStartLine);
    ASSERT_EQ(result.diagnostic[forthIndex].range_.start.character_, expectedForthStartCharacter);
    ASSERT_EQ(result.diagnostic[forthIndex].range_.end.line_, expectedForthEndLine);
    ASSERT_EQ(result.diagnostic[forthIndex].range_.end.character_, expectedForthEndCharacter);
    ASSERT_EQ(result.diagnostic[forthIndex].severity_, DiagnosticSeverity::Error);
    ASSERT_NE(GetDiagnosticCode(result.diagnostic[forthIndex]), 0);
    ASSERT_EQ(result.diagnostic[forthIndex].message_, R"(Unexpected token ','.)");
    ASSERT_EQ(result.diagnostic[forthIndex].codeDescription_.href_, "test code description");
    auto const fifthIndex = 6;
    auto const expectedFifththStartLine = 1;
    auto const expectedFifthStartCharacter = 27;
    auto const expectedFifthEndLine = 1;
    auto const expectedFifthEndCharacter = 33;
    ASSERT_EQ(result.diagnostic[fifthIndex].range_.start.line_, expectedFifththStartLine);
    ASSERT_EQ(result.diagnostic[fifthIndex].range_.start.character_, expectedFifthStartCharacter);
    ASSERT_EQ(result.diagnostic[fifthIndex].range_.end.line_, expectedFifthEndLine);
    ASSERT_EQ(result.diagnostic[fifthIndex].range_.end.character_, expectedFifthEndCharacter);
    ASSERT_EQ(result.diagnostic[fifthIndex].severity_, DiagnosticSeverity::Error);
    ASSERT_NE(GetDiagnosticCode(result.diagnostic[fifthIndex]), 0);
    ASSERT_EQ(result.diagnostic[fifthIndex].message_, R"(Label must be followed by a loop statement.)");
    ASSERT_EQ(result.diagnostic[fifthIndex].codeDescription_.href_, "test code description");
}

TEST_F(LspDiagnosticsTests, GetSyntacticDiagnosticsForFile4)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("GetSemanticDiagnosticsNoError1.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(functon add(a: number, b: number) {
    return a + b;
}
let n = 333;
let res = add(n, n);)");
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSyntacticDiagnostics(ctx);
    initializer.DestroyContext(ctx);
    auto const sixthIndex = 7;
    auto const expectedSixthStartLine = 1;
    auto const expectedSixthStartCharacter = 33;
    auto const expectedSixthEndLine = 1;
    auto const expectedSixthEndCharacter = 34;
    ASSERT_EQ(result.diagnostic[sixthIndex].range_.start.line_, expectedSixthStartLine);
    ASSERT_EQ(result.diagnostic[sixthIndex].range_.start.character_, expectedSixthStartCharacter);
    ASSERT_EQ(result.diagnostic[sixthIndex].range_.end.line_, expectedSixthEndLine);
    ASSERT_EQ(result.diagnostic[sixthIndex].range_.end.character_, expectedSixthEndCharacter);
    ASSERT_EQ(result.diagnostic[sixthIndex].severity_, DiagnosticSeverity::Error);
    ASSERT_NE(GetDiagnosticCode(result.diagnostic[sixthIndex]), 0);
    ASSERT_EQ(result.diagnostic[sixthIndex].message_, R"(Unexpected token ')'.)");
    ASSERT_EQ(result.diagnostic[sixthIndex].codeDescription_.href_, "test code description");
    auto const sevenIndex = 9;
    auto const expectedSeventhStartLine = 2;
    auto const expectedSeventhStartCharacter = 5;
    auto const expectedSeventhEndLine = 2;
    auto const expectedSeventhEndCharacter = 18;
    ASSERT_EQ(result.diagnostic[sevenIndex].range_.start.line_, expectedSeventhStartLine);
    ASSERT_EQ(result.diagnostic[sevenIndex].range_.start.character_, expectedSeventhStartCharacter);
    ASSERT_EQ(result.diagnostic[sevenIndex].range_.end.line_, expectedSeventhEndLine);
    ASSERT_EQ(result.diagnostic[sevenIndex].range_.end.character_, expectedSeventhEndCharacter);
    ASSERT_EQ(result.diagnostic[sevenIndex].severity_, DiagnosticSeverity::Error);
    ASSERT_NE(GetDiagnosticCode(result.diagnostic[sevenIndex]), 0);
    ASSERT_EQ(result.diagnostic[sevenIndex].message_, R"(return keyword should be used in function body.)");
    ASSERT_EQ(result.diagnostic[sevenIndex].codeDescription_.href_, "test code description");
}

TEST_F(LspDiagnosticsTests, GetSemanticDiagnostics3)
{
    std::vector<std::string> files = {"getSemanticDiagnostics1.ets", "getSemanticDiagnostics2.ets"};
    std::vector<std::string> texts = {R"(import {foo2} from './getSemanticDiagnostics2')",
                                      R"(import {foo1} from './getSemanticDiagnostics1';
export function foo2(){})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    DiagnosticReferences result = lspApi->getSemanticDiagnostics(ctx);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(result.diagnostic.size(), 1);
    ASSERT_EQ(result.diagnostic[0].severity_, DiagnosticSeverity::Warning);
    ASSERT_EQ(result.diagnostic[0].message_, "'foo2' is never used");
    ASSERT_TRUE(std::holds_alternative<std::string>(result.diagnostic[0].data_));
    ASSERT_EQ(std::get<std::string>(result.diagnostic[0].data_), "unusedSymbol");
}
