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

    static void ExpectNoFixWithName(const std::vector<CodeFixActionInfo> &fixes, std::string_view fixName)
    {
        for (const auto &fix : fixes) {
            EXPECT_NE(fix.fixName_, fixName);
        }
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

// RHS bigint literal infers "BigInt" (AddLocalVariable::InferTypeFromLiteral BIGINT branch)
TEST_F(AddLocalVariableTests3, InfersBigIntTypeFromBigIntLiteralRhs)
{
    std::vector<std::string> fileNames = {"BigIntLiteralRhs.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    bigValue = 1n;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string_view varName = "bigValue";
    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 3, 5), varName.size());
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "  let bigValue: BigInt;");

    initializer.DestroyContext(context);
}

// RHS undefined literal falls back to "Object" (InferTypeFromLiteral UNDEFINED branch)
TEST_F(AddLocalVariableTests3, InfersObjectTypeFromUndefinedLiteralRhs)
{
    std::vector<std::string> fileNames = {"UndefinedLiteralRhs.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    unsetValue = undefined;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string_view varName4 = "unsetValue";
    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 3, 5), varName4.size());
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "  let unsetValue: Object;");

    initializer.DestroyContext(context);
}

// Object literal RHS infers "Object" (InferTypeFromOtherExpressions object-expression branch)
TEST_F(AddLocalVariableTests3, InfersObjectTypeFromObjectLiteralRhs)
{
    std::vector<std::string> fileNames = {"ObjectLiteralRhs.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    configBag = {};
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string_view varName6 = "configBag";
    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 3, 5), varName6.size());
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "  let configBag: Object;");

    initializer.DestroyContext(context);
}

// The receiver identifier of a member assignment is not the property itself, so the fix offers
// a declaration for the receiver instead (GetTypeFromMemberAssignment property guard).
TEST_F(AddLocalVariableTests3, ReceiverIdentifierOfMemberAssignmentGetsObjectDeclaration)
{
    std::vector<std::string> fileNames = {"ReceiverIdentifierMemberAssignment.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    cart.boxLabel = "x";
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // "cart" sits at line 3 column 5
    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 3, 5), 4);
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "  let cart: Object;");

    initializer.DestroyContext(context);
}

// A member assignment on an unresolved receiver has no in-file class to attach a field to,
// so no fix is offered at all (non-this property classification without class-field fallback).
TEST_F(AddLocalVariableTests3, NoFixWhenReceiverOfMemberAssignmentIsUnresolved)
{
    std::vector<std::string> fileNames = {"UnresolvedReceiverMemberAssignment.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    phantom.propField = 7;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string_view propName = "propField";
    // Point at the property name "propField" on line 3
    const size_t start = fileContents[0].find(propName);
    ASSERT_NE(start, std::string::npos);
    auto fixResult = GetClassFixes(context, start, propName.size());
    ExpectNoFixWithName(fixResult, CLASS_FIX_NAME);
    ExpectNoFixWithName(fixResult, FUNCTION_FIX_NAME);

    initializer.DestroyContext(context);
}

// Requesting the fix directly at a function-declaration name anchors the insertion inside that
// function's own body (FindFunctionInsertionPoint resolves the ScriptFunction body).
TEST_F(AddLocalVariableTests3, FunctionNamePositionInsertsIntoItsOwnBody)
{
    std::vector<std::string> fileNames = {"FunctionNamePosition.ets"};
    std::vector<std::string> fileContents = {R"(
function runnerFn(): void {
    runnerFn();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Point at the declaration name "runnerFn" on line 2
    const size_t start = fileContents[0].find("runnerFn");
    ASSERT_NE(start, std::string::npos);
    auto fixResult = GetFunctionFixes(context, start, 8);
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "runnerFn(): void {", "  let runnerFn: Object;");

    initializer.DestroyContext(context);
}
}  // namespace
