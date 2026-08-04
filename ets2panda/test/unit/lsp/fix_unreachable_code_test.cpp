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

#include "lsp_api_test.h"
#include "util/diagnostic.h"
#include <gtest/gtest.h>
#include <iostream>
#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_unreachable_code.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_UNREACHABLE_CODE;
using ark::es2panda::util::DiagnosticType;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_UNREACHABLE_CODE.GetFixId();
constexpr auto ERROR_CODES = FIX_UNREACHABLE_CODE.GetSupportedCodeNumbers();
// UNREACHABLE_STMT: DiagnosticType::WARNING * DIAGNOSTIC_CODE_MULTIPLIER + 26
constexpr int UNREACHABLE_STMT_CODE = 3026;
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Remove unreachable code";
constexpr int DEFAULT_THROTTLE = 20;
constexpr size_t DEFAULT_FIX_SPAN_LENGTH = 1;
constexpr size_t EXPECTED_SINGLE_DIAGNOSTIC = 1;
constexpr size_t EXPECTED_SINGLE_FIX = 1;

class FixUnreachableCodeTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        return index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
    }

    static void ValidateCodeFixActionInfo(const CodeFixActionInfo &info, const size_t expectedTextChangeStart,
                                          const size_t expectedTextChangeLength, const std::string &expectedFileName,
                                          const std::string &expectedNewText = "")
    {
        ASSERT_EQ(info.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(info.changes_[0].fileName, expectedFileName);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.start, expectedTextChangeStart);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.length, expectedTextChangeLength);
        ASSERT_EQ(info.changes_[0].textChanges[0].newText, expectedNewText);
    }

    static std::vector<Diagnostic> GetUnreachableDiagnostics(es2panda_Context *context)
    {
        LSPAPI const *lspApi = GetImpl();
        auto semanticDiagnostics = lspApi->getSemanticDiagnostics(context);
        auto syntacticDiagnostics = lspApi->getSyntacticDiagnostics(context);
        auto suggestionDiagnostics = lspApi->getSuggestionDiagnostics(context);
        std::vector<Diagnostic> result;
        auto collectUnreachableDiagnostics = [&result](const DiagnosticReferences &diagnostics) {
            for (const auto &diagnostic : diagnostics.diagnostic) {
                if (std::get<int>(diagnostic.code_) == UNREACHABLE_STMT_CODE) {
                    result.push_back(diagnostic);
                }
            }
        };
        collectUnreachableDiagnostics(semanticDiagnostics);
        collectUnreachableDiagnostics(syntacticDiagnostics);
        collectUnreachableDiagnostics(suggestionDiagnostics);
        return result;
    }

    static void AssertNoUnreachableDiagnostic(es2panda_Context *context)
    {
        auto diagnostics = GetUnreachableDiagnostics(context);
        ASSERT_EQ(diagnostics.size(), 0U);
    }

    static size_t FindOffset(const std::string &source, const std::string &text)
    {
        const auto offset = source.find(text);
        EXPECT_NE(offset, std::string::npos);
        return offset;
    }

    static void ValidateUnreachableDiagnosticAndFix(es2panda_Context *context, const std::string &source,
                                                    const std::string &unreachableText,
                                                    const std::string &expectedFileName)
    {
        auto diagnostics = GetUnreachableDiagnostics(context);
        ASSERT_EQ(diagnostics.size(), EXPECTED_SINGLE_DIAGNOSTIC);
        ASSERT_EQ(std::get<int>(diagnostics[0].code_), UNREACHABLE_STMT_CODE);
        ASSERT_EQ(diagnostics[0].severity_, DiagnosticSeverity::Warning);
        ASSERT_EQ(std::get<std::string>(diagnostics[0].data_), "unusedSymbol");

        const auto start =
            LineColToPos(context, diagnostics[0].range_.start.line_, diagnostics[0].range_.start.character_);
        const auto expectedTextChangeStart = FindOffset(source, unreachableText);
        ASSERT_EQ(start, expectedTextChangeStart);

        std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
        CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + DEFAULT_FIX_SPAN_LENGTH,
                                                                        errorCodes, options);
        ASSERT_EQ(fixResult.size(), EXPECTED_SINGLE_FIX);

        ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, unreachableText.size(), expectedFileName);
    }

    static void ValidateCodeFixForText(es2panda_Context *context, const std::string &source, const std::string &posText,
                                       const std::string &deletedText, const std::string &expectedFileName)
    {
        const auto pos = FindOffset(source, posText);
        const auto expectedTextChangeStart = FindOffset(source, deletedText);
        std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
        CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, pos, pos + DEFAULT_FIX_SPAN_LENGTH,
                                                                        errorCodes, options);
        ASSERT_EQ(fixResult.size(), EXPECTED_SINGLE_FIX);

        ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, deletedText.size(), expectedFileName);
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

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterReturn1)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeAfterReturn1.ets"};
    std::vector<std::string> fileContents = {
        R"(
function func(): boolean {
return false;
console.log("log");
})"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 4, 1);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 42;
    const size_t expectedTextChangeLength = 19;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    // Verify target error code: UNREACHABLE_STMT(3026)
    ASSERT_EQ(errorCodes.size(), 1U);
    ASSERT_EQ(errorCodes[0], UNREACHABLE_STMT_CODE);
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableInIfFalseBlock)
{
    const std::string source = R"(
function case3(): void {
  if (false) {
    let unreach: number = 10;
  }
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeIfFalseBlock.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateUnreachableDiagnosticAndFix(context, source, "let unreach: number = 10;", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableInIfUndefinedBlock)
{
    const std::string source = R"(
function caseUndefined(): void {
  if (undefined) {
    let unreach: number = 10;
  }
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeIfUndefinedBlock.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateUnreachableDiagnosticAndFix(context, source, "let unreach: number = 10;", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableInResolvedVariableConditionBlock)
{
    const std::string source = R"(
function case3(): void {
  let num: number = 0;
  if (num > 0) {
    let unreach: number = 10;
  }
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeVariableConditionBlock.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateUnreachableDiagnosticAndFix(context, source, "let unreach: number = 10;", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableInBooleanVariableConditionBlock)
{
    const std::string source = R"(
function case3(): void {
  let flag: boolean = false;
  if (flag) {
    let unreach: number = 10;
  }
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeBooleanVariableConditionBlock.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateUnreachableDiagnosticAndFix(context, source, "let unreach: number = 10;", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableInBracelessIfFalse)
{
    const std::string source = R"(
function case3(): void {
  if (false) return;
  let reachable: number = 10;
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeBracelessIfFalse.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateCodeFixForText(context, source, "return;", "if (false) return;", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveBracelessIfFalseAndPreserveElseBody)
{
    const std::string source = R"(
function case3(): void {
  if (false) foo();
  else bar();
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeBracelessIfFalseWithElse.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const auto pos = FindOffset(source, "foo();");
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, pos, pos + DEFAULT_FIX_SPAN_LENGTH,
                                                                    errorCodes, options);
    ASSERT_EQ(fixResult.size(), EXPECTED_SINGLE_FIX);
    ValidateCodeFixActionInfo(fixResult[0], pos, std::string("foo();").size(), filePaths[0], "{}");

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableForShadowedBooleanVariableCondition)
{
    const std::string source = R"(
function case3(): void {
  let flag: boolean = true;
  if (flag) {
    let flag: boolean = false;
    if (flag) {
      let unreach: number = 10;
    }
  }
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeShadowedBooleanCondition.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateUnreachableDiagnosticAndFix(context, source, "let unreach: number = 10;", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestNoFalsePositiveAfterIncrementedNumericVariableCondition)
{
    const std::string source = R"(
function case3(): void {
  let num: number = 0;
  num++;
  if (num == 0) {
    return;
  }
  let reachable: number = 10;
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeIncrementedNumericCondition.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    AssertNoUnreachableDiagnostic(context);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestNoFalsePositiveAfterDecrementedNumericVariableCondition)
{
    const std::string source = R"(
function case3(): void {
  let num: number = 0;
  --num;
  if (num != 0) {
    let reachable: number = 10;
  }
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeDecrementedNumericCondition.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    AssertNoUnreachableDiagnostic(context);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestNoFalsePositiveAfterAssignmentInTrueCondition)
{
    const std::string source = R"(
function case3(flag: boolean): void {
  if (flag) {
    flag = false;
    if (flag) {
      return;
    }
    let reachable: number = 10;
  }
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeAssignmentInTrueCondition.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    AssertNoUnreachableDiagnostic(context);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterNestedSameConditionalReturn)
{
    const std::string source = R"(
function case3(flag: boolean): void {
  if (flag) {
    if (flag) {
        return;
    }
    let unreach: number = 10;
  }
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeNestedConditionalReturn.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateUnreachableDiagnosticAndFix(context, source, "let unreach: number = 10;", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterReturn2)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeAfterReturn2.ets"};
    std::vector<std::string> fileContents = {
        R"(
function func(): boolean {
return false;
if (true) {
console.log("log");
}
})"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 4, 1);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 42;
    const size_t expectedTextChangeLength = 33;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterReturn3)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeAfterReturn3.ets"};
    std::vector<std::string> fileContents = {
        R"(
function func(): void{
if (true) {
return;
console.log("log");
}
})"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 5, 1);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 44;
    const size_t expectedTextChangeLength = 19;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterReturn4)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeAfterReturn4.ets"};
    std::vector<std::string> fileContents = {
        R"(
function test() : void {
    do {
        return;
        console.log("log");
    } while (true)
}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 5, 9);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 59;
    const size_t expectedTextChangeLength = 19;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterReturn5)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeAfterReturn5.ets"};
    std::vector<std::string> fileContents = {
        R"(
function test() : void {
return;
for (; false ;) {
console.log("log");
return;
console.log("log2");
}
}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 4, 1);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 52;
    const size_t expectedTextChangeLength = 48;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterReturn6)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeAfterReturn6.ets"};
    std::vector<std::string> fileContents = {
        R"(
function test() : void {
return;
console.log("log");
console.log("log");
}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 4, 1);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 34;
    const size_t expectedTextChangeLength = 39;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterReturn7)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeAfterReturn7.ets"};
    std::vector<std::string> fileContents = {
        R"(
function test() : void {
return;
console.log("log");
console.log("log");
}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 5, 1);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 54;
    const size_t expectedTextChangeLength = 19;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterWhileFalse1)
{
    const std::string source = R"(
function test(): void{
while (false) {
console.log("log");
}
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeWhileFalse1.ets"};
    std::vector<std::string> fileContents = {source};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateUnreachableDiagnosticAndFix(context, source, "console.log(\"log\");", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableInBracelessWhileFalse)
{
    const std::string source = R"(
function case3(): void {
  while (false) return;
  let reachable: number = 10;
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeBracelessWhileFalse.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateCodeFixForText(context, source, "return;", "while (false) return;", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableInBooleanVariableWhileBlock)
{
    const std::string source = R"(
function case3(): void {
  let flag: boolean = false;
  while (flag) {
    let unreach: number = 10;
  }
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeBooleanVariableWhileBlock.ets"};
    std::vector<std::string> fileContents = {source};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateUnreachableDiagnosticAndFix(context, source, "let unreach: number = 10;", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterWhileFalse2)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeWhileFalse2.ets"};
    std::vector<std::string> fileContents = {
        R"(
function test(): void{
while (0) {
console.log("log");
}
}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 3, 11);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 36;
    const size_t expectedTextChangeLength = 19;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterWhileFalse3)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeWhileFalse4.ets"};
    std::vector<std::string> fileContents = {
        R"(
function test():void {
const x=5;
while (x!=5) {
console.log("log");
}}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 4, 12);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 50;
    const size_t expectedTextChangeLength = 19;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterWhileFalse4)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeWhileFalse5.ets"};
    std::vector<std::string> fileContents = {
        R"(
function test():void {
while (1!=1) {
console.log("log");
}}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 3, 12);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 39;
    const size_t expectedTextChangeLength = 19;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterForFalse)
{
    const std::string source = R"(
function test() : void {
for (; false ;) {
        console.log("log");
}
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeIfFalse.ets"};
    std::vector<std::string> fileContents = {source};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateUnreachableDiagnosticAndFix(context, source, "console.log(\"log\");", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableInBracelessForFalse)
{
    const std::string source = R"(
function case3(): void {
  for (; false ;) return;
  let reachable: number = 10;
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeBracelessForFalse.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateCodeFixForText(context, source, "return;", "for (; false ;) return;", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableInBooleanVariableForBlock)
{
    const std::string source = R"(
function case3(): void {
  let flag: boolean = false;
  for (; flag ;) {
    let unreach: number = 10;
  }
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeBooleanVariableForBlock.ets"};
    std::vector<std::string> fileContents = {source};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ValidateUnreachableDiagnosticAndFix(context, source, "let unreach: number = 10;", filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestNoFalsePositiveForShadowedNumericVariableCondition)
{
    const std::string source = R"(
function case3(): void {
  let x: number = 0;
  {
    let x: number = 1;
  }
  if (x == 0) {
    let unreach: number = 10;
  }
}
)";
    std::vector<std::string> fileNames = {"FixUnreachableCodeShadowedNumericCondition.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    AssertNoUnreachableDiagnostic(context);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterIfFalse)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeIfFalse.ets"};
    std::vector<std::string> fileContents = {
        R"(
function nested(): void {
if (false) {
console.log("log");
}
return;
})"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 4, 11);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 40;
    const size_t expectedTextChangeLength = 19;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestNoFixReachableStatement)
{
    std::vector<std::string> fileNames = {"NoFixReachableStatement.ets"};
    std::vector<std::string> fileContents = {
        R"(
function func() : void {
console.log("log");
}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 2, 1);
    const size_t length = 1;
    const int expectedFixResultSize = 0;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);
    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestCaseLetUsage)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeLetUsage.ets"};
    std::vector<std::string> fileContents = {
        R"(
let x = false;
function test() : void {
for (; x ;) {
console.log("no unreachable error");
}
})"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 4, 13);
    const size_t length = 1;
    const int expectedFixResultSize = 0;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    initializer.DestroyContext(context);
}

TEST_F(FixUnreachableCodeTests, TestFixRemoveUnreachableAfterThrowStmnt)
{
    std::vector<std::string> fileNames = {"FixUnreachableCodeAfterReturn6.ets"};
    std::vector<std::string> fileContents = {
        R"(
function test() : void {
throw Error();
console.log("log");
}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 4, 1);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 41;
    const size_t expectedTextChangeLength = 19;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    initializer.DestroyContext(context);
}

}  // namespace
