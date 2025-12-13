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

#include "lsp_api_test.h"
#include <gtest/gtest.h>
#include <iostream>
#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_unreachable_code.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_UNREACHABLE_CODE;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_UNREACHABLE_CODE.GetFixId();
constexpr auto ERROR_CODES = FIX_UNREACHABLE_CODE.GetSupportedCodeNumbers();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Remove unreachable code";
constexpr int DEFAULT_THROTTLE = 20;

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
                                          const size_t expectedTextChangeLength, const std::string &expectedFileName)
    {
        ASSERT_EQ(info.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(info.changes_[0].fileName, expectedFileName);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.start, expectedTextChangeStart);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.length, expectedTextChangeLength);
        ASSERT_EQ(info.changes_[0].textChanges[0].newText, "");
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
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

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
    const size_t expectedTextChangeStart = 34;
    const size_t expectedTextChangeLength = 68;
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
    std::vector<std::string> fileNames = {"FixUnreachableCodeWhileFalse1.ets"};
    std::vector<std::string> fileContents = {
        R"(
function test(): void{
while (false) {
console.log("log");
}
}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 3, 13);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 24;
    const size_t expectedTextChangeLength = 37;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

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
    const size_t expectedTextChangeStart = 24;
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
    const size_t expectedTextChangeStart = 35;
    const size_t expectedTextChangeLength = 36;
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
    const size_t expectedTextChangeStart = 24;
    const size_t expectedTextChangeLength = 36;
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
    std::vector<std::string> fileNames = {"FixUnreachableCodeIfFalse.ets"};
    std::vector<std::string> fileContents = {
        R"(
function test() : void {
for (; false ;) {
        console.log("log");
}
}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    const size_t start = LineColToPos(context, 3, 17);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 26;
    const size_t expectedTextChangeLength = 47;
    const int expectedFixResultSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

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
    const size_t expectedTextChangeStart = 27;
    const size_t expectedTextChangeLength = 34;
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