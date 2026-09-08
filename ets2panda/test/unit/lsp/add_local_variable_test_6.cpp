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

#include <string>
#include <vector>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/add_local_variable.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::ADD_LOCAL_VARIABLE;
using ark::es2panda::lsp::codefixes::ADD_LOCAL_VARIABLE_FOR_CLASS;

constexpr std::string_view FUNCTION_FIX_NAME = ADD_LOCAL_VARIABLE.GetFixId();
constexpr std::string_view CLASS_FIX_NAME = ADD_LOCAL_VARIABLE_FOR_CLASS.GetFixId();
constexpr auto FUNCTION_ERROR_CODES = ADD_LOCAL_VARIABLE.GetSupportedCodeNumbers();
constexpr auto CLASS_ERROR_CODES = ADD_LOCAL_VARIABLE_FOR_CLASS.GetSupportedCodeNumbers();
constexpr int DEFAULT_THROTTLE = 20;

class AddLocalVariableTests3 : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        // line/column are 1-based
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        return index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
    }

    static std::vector<CodeFixActionInfo> GetFunctionFixes(es2panda_Context *context, size_t start, size_t length)
    {
        std::vector<int> errorCodes(FUNCTION_ERROR_CODES.begin(), FUNCTION_ERROR_CODES.end());
        CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    }

    static std::vector<CodeFixActionInfo> GetClassFixes(es2panda_Context *context, size_t start, size_t length)
    {
        std::vector<int> errorCodes(CLASS_ERROR_CODES.begin(), CLASS_ERROR_CODES.end());
        CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    }

    static const CodeFixActionInfo &RequireSingleFix(const std::vector<CodeFixActionInfo> &fixes,
                                                     std::string_view fixName)
    {
        std::vector<const CodeFixActionInfo *> matched;
        for (const auto &fix : fixes) {
            if (fix.fixName_ == fixName) {
                matched.push_back(&fix);
            }
        }
        EXPECT_EQ(matched.size(), 1U) << "Expected exactly one fix named " << fixName;
        EXPECT_TRUE(matched[0] != nullptr);
        return *matched[0];
    }

    static void ExpectInsertionAfterBrace(const CodeFixActionInfo &fix, const std::string &fileName,
                                          const std::string &source, const std::string &braceMarker,
                                          const std::string &expectedNewText)
    {
        ASSERT_EQ(fix.changes_.size(), 1U);
        ASSERT_EQ(fix.changes_[0].fileName, fileName);
        ASSERT_EQ(fix.changes_[0].textChanges.size(), 1U);
        const auto &change = fix.changes_[0].textChanges[0];
        // The provider inserts right after the first '{' found from the insertion point on
        const auto bracePos = source.find(braceMarker);
        ASSERT_NE(bracePos, std::string::npos);
        EXPECT_EQ(change.span.start, bracePos + braceMarker.size());
        EXPECT_EQ(change.span.length, 0U);
        EXPECT_EQ(change.newText, expectedNewText);
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

// A bigint-only binary expression is intentionally not folded by ConstantExpressionLowering,
// so the BinaryExpression reaches the checked AST and InferTypeFromBinaryExpression returns
// "BigInt" through its BIGINT operand branch.
TEST_F(AddLocalVariableTests3, InfersBigIntTypeFromNonFoldedBigIntBinary)
{
    std::vector<std::string> fileNames = {"BigIntBinaryRhs.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    bigSum = 1n + 2n;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string_view varName2 = "bigSum";
    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 3, 5), varName2.size());
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "  let bigSum: BigInt;");

    initializer.DestroyContext(context);
}

// Empty array literal infers "Object[]" (InferTypeFromOtherExpressions empty-elements branch)
TEST_F(AddLocalVariableTests3, InfersObjectTypeForEmptyArrayLiteralRhs)
{
    std::vector<std::string> fileNames = {"EmptyArrayRhs.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    emptyBag = [];
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string_view varName5 = "emptyBag";
    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 3, 5), varName5.size());
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "  let emptyBag: Object[];");

    initializer.DestroyContext(context);
}

// `grabbedField = this;` inside a method keeps the local-variable fix (the unresolved name is
// the assignment target, not a this-member) and infers "Object" via the this-expression branch
TEST_F(AddLocalVariableTests3, InfersObjectTypeFromThisRhs)
{
    std::vector<std::string> fileNames = {"ThisRhs.ets"};
    std::vector<std::string> fileContents = {R"(
class Holder {
    capture(): void {
        grabbedField = this;
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string_view fieldName = "grabbedField";
    const size_t fieldPos = fileContents[0].find(fieldName);
    ASSERT_NE(fieldPos, std::string::npos);
    auto fixResult = GetFunctionFixes(context, fieldPos, fieldName.size());
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "capture(): void {", "  let grabbedField: Object;");

    initializer.DestroyContext(context);
}

// A binary expression whose operands are both unresolved identifiers infers neither literal nor
// special type, so InferTypeFromBinaryExpression falls through to its OBJECT fallback.
TEST_F(AddLocalVariableTests3, InfersObjectFallbackFromIdentifierEqualityBinary)
{
    std::vector<std::string> fileNames = {"IdentifierEqualityBinary.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    eqResult = lhsOperand == rhsOperand;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 3, 5), 8);
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "  let eqResult: Object;");

    initializer.DestroyContext(context);
}

// An unresolved call argument is not an assignment target: both assignment-based inference
// helpers exit early and the identifier still gets an Object-typed local declaration.
TEST_F(AddLocalVariableTests3, CallArgumentIdentifierGetsObjectDeclaration)
{
    std::vector<std::string> fileNames = {"CallArgumentIdentifier.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    consume(passedValue);
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string_view argName = "passedValue";
    const size_t argPos = fileContents[0].find(argName);
    ASSERT_NE(argPos, std::string::npos);
    auto fixResult = GetFunctionFixes(context, argPos, argName.size());
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "  let passedValue: Object;");

    initializer.DestroyContext(context);
}

// A bare `this.member;` read still suggests adding the missing class field, typed Object via
// the member-assignment helper's enclosing-statement guard.
TEST_F(AddLocalVariableTests3, ClassFieldSuggestedForThisPropertyRead)
{
    std::vector<std::string> fileNames = {"ThisPropertyRead.ets"};
    std::vector<std::string> fileContents = {R"(
class Badge {
    render(): void {
        this.badgeLabel;
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string_view labelName = "badgeLabel";
    const size_t labelPos = fileContents[0].find(labelName);
    ASSERT_NE(labelPos, std::string::npos);
    auto fixResult = GetClassFixes(context, labelPos, labelName.size());
    const auto &fix = RequireSingleFix(fixResult, CLASS_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "\n  badgeLabel: Object;");

    initializer.DestroyContext(context);
}
}  // namespace
