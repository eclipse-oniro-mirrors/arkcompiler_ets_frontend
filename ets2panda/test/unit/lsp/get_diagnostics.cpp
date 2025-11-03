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

#include "lsp/include/internal_api.h"
#include "lsp/include/classifier.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

using ark::es2panda::lsp::Initializer;

class LspDiagnosticsTests : public LSPAPITests {};

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
    ASSERT_EQ(result.diagnostic.size(), 0);
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
    ASSERT_NE(std::get<int>(result.diagnostic[firstIndex].code_), 0);
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
    ASSERT_NE(std::get<int>(result.diagnostic[thirdIndex].code_), 0);
    ASSERT_EQ(result.diagnostic[thirdIndex].message_, R"(Type '"1"' is not compatible with type 'Double' at index 1)");
    ASSERT_EQ(result.diagnostic[thirdIndex].codeDescription_.href_, "test code description");
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
    ASSERT_NE(std::get<int>(result.diagnostic[0].code_), 0);
    auto const expectedSecondStartLine = 1;
    auto const expectedSecondStartCharacter = 14;
    auto const expectedSecondEndLine = 1;
    auto const expectedSecondEndCharacter = 15;
    ASSERT_EQ(result.diagnostic[1].range_.start.line_, expectedSecondStartLine);
    ASSERT_EQ(result.diagnostic[1].range_.start.character_, expectedSecondStartCharacter);
    ASSERT_EQ(result.diagnostic[1].range_.end.line_, expectedSecondEndLine);
    ASSERT_EQ(result.diagnostic[1].range_.end.character_, expectedSecondEndCharacter);
    ASSERT_EQ(result.diagnostic[1].severity_, DiagnosticSeverity::Error);
    ASSERT_NE(std::get<int>(result.diagnostic[1].code_), 0);
    auto const thirdIndex = 2;
    auto const expectedThirdStartLine = 1;
    auto const expectedThirdStartCharacter = 14;
    auto const expectedThirdEndLine = 1;
    auto const expectedThirdEndCharacter = 15;
    ASSERT_EQ(result.diagnostic[thirdIndex].range_.start.line_, expectedThirdStartLine);
    ASSERT_EQ(result.diagnostic[thirdIndex].range_.start.character_, expectedThirdStartCharacter);
    ASSERT_EQ(result.diagnostic[thirdIndex].range_.end.line_, expectedThirdEndLine);
    ASSERT_EQ(result.diagnostic[thirdIndex].range_.end.character_, expectedThirdEndCharacter);
    ASSERT_EQ(result.diagnostic[thirdIndex].severity_, DiagnosticSeverity::Error);
    ASSERT_NE(std::get<int>(result.diagnostic[thirdIndex].code_), 0);
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
    ASSERT_NE(std::get<int>(result.diagnostic[forthIndex].code_), 0);
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
    ASSERT_NE(std::get<int>(result.diagnostic[fifthIndex].code_), 0);
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
    ASSERT_NE(std::get<int>(result.diagnostic[sixthIndex].code_), 0);
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
    ASSERT_NE(std::get<int>(result.diagnostic[sevenIndex].code_), 0);
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
    ASSERT_EQ(result.diagnostic.size(), 0);
}
