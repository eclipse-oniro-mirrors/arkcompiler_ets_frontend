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

// RHS char literal infers "Char" (InferTypeFromLiteral CHAR branch)
TEST_F(AddLocalVariableTests3, InfersCharTypeFromCharLiteralRhs)
{
    std::vector<std::string> fileNames = {"CharLiteralRhs.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    gradeMark = c'a';
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string_view varName3 = "gradeMark";
    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 3, 5), varName3.size());
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "  let gradeMark: Char;");

    initializer.DestroyContext(context);
}

// A non-constant binary with a char literal operand survives folding and hits the CHAR branch
// of InferTypeFromBinaryExpression (the identifier operand infers Object, which does not mask CHAR).
TEST_F(AddLocalVariableTests3, InfersCharTypeFromNonConstantBinaryWithCharOperand)
{
    std::vector<std::string> fileNames = {"CharBinaryRhs.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    let flagOn: boolean = true;
    charEq = c'z' == flagOn;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string_view eqName = "charEq";
    auto fixResult = GetFunctionFixes(context, LineColToPos(context, 4, 5), eqName.size());
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "  let charEq: Char;");

    initializer.DestroyContext(context);
}

// Requesting the fix on the RHS identifier of an assignment: GetTypeFromDirectAssignment exits
// through its left-side guard and the identifier falls back to an Object-typed declaration.
TEST_F(AddLocalVariableTests3, RhsIdentifierPositionGetsObjectDeclaration)
{
    std::vector<std::string> fileNames = {"RhsIdentifierPosition.ets"};
    std::vector<std::string> fileContents = {R"(
function main(): void {
    stored = provided;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Point at the RHS identifier "provided" on line 3
    const size_t start = fileContents[0].find("provided");
    ASSERT_NE(start, std::string::npos);
    auto fixResult = GetFunctionFixes(context, start, 8);
    const auto &fix = RequireSingleFix(fixResult, FUNCTION_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "  let provided: Object;");

    initializer.DestroyContext(context);
}

// `holderVar = this.lockTag;`: the this-member appears on the RHS of an outer assignment, so the
// member-assignment helper exits through its left-side guard and the class field stays Object.
TEST_F(AddLocalVariableTests3, ClassFieldSuggestedWhenThisMemberIsRhsOfOuterAssignment)
{
    std::vector<std::string> fileNames = {"ThisMemberAsRhs.ets"};
    std::vector<std::string> fileContents = {R"(
class Locker {
    open(): void {
        keeperVar = this.lockTag;
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Point at "lockTag" inside `this.lockTag` on line 4
    const size_t lockTagStart = fileContents[0].rfind("lockTag");
    ASSERT_NE(lockTagStart, std::string::npos);
    auto fixResult = GetClassFixes(context, lockTagStart, 7);
    const auto &fix = RequireSingleFix(fixResult, CLASS_FIX_NAME);
    ExpectInsertionAfterBrace(fix, filePaths[0], fileContents[0], "{", "\n  lockTag: Object;");

    initializer.DestroyContext(context);
}

// A non-this member read assigned to another name cannot host a class field: the class-field
// helper requires the member access itself to be the assignment target.
TEST_F(AddLocalVariableTests3, NoClassFieldWhenMemberReadIsRhsOfOuterAssignment)
{
    std::vector<std::string> fileNames = {"MemberReadAsRhs.ets"};
    std::vector<std::string> fileContents = {R"(
class Device {
    serialNo: string = "";
}
function main(): void {
    let dev = new Device();
    mirrored = dev.serialNo;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Point at "serialNo" inside `dev.serialNo` on line 8
    std::vector<int> errorCodes(CLASS_ERROR_CODES.begin(), CLASS_ERROR_CODES.end());
    errorCodes.insert(errorCodes.end(), FUNCTION_ERROR_CODES.begin(), FUNCTION_ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    const size_t start = fileContents[0].rfind("serialNo");
    ASSERT_NE(start, std::string::npos);
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 8, errorCodes, options);
    ExpectNoFixWithName(fixResult, CLASS_FIX_NAME);
    ExpectNoFixWithName(fixResult, FUNCTION_FIX_NAME);

    initializer.DestroyContext(context);
}

// Abstract methods have no function body, so walking up from the method-name position never
// finds an insertion point and no fix is offered.
TEST_F(AddLocalVariableTests3, AbstractMethodNamePositionOffersNoFix)
{
    std::vector<std::string> fileNames = {"AbstractMethodPosition.ets"};
    std::vector<std::string> fileContents = {R"(
abstract class Shape {
    abstract drawShape(): void;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string_view methodName = "drawShape";
    // Point at the method name "drawShape" on line 3
    const size_t start = fileContents[0].find(methodName);
    ASSERT_NE(start, std::string::npos);
    auto fixResult = GetClassFixes(context, start, methodName.size());
    ExpectNoFixWithName(fixResult, CLASS_FIX_NAME);
    ExpectNoFixWithName(fixResult, FUNCTION_FIX_NAME);

    initializer.DestroyContext(context);
}
}  // namespace
