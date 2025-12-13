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

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/add_missing_new_operator.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_ADD_MISSING_NEW_OPERATOR;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_ADD_MISSING_NEW_OPERATOR.GetFixId();
constexpr auto ERROR_CODES = FIX_ADD_MISSING_NEW_OPERATOR.GetSupportedCodeNumbers();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Add missing 'new' operator to constructor call";

// ---- Test 1: Adds 'new' to constructor call ----
constexpr size_t T1_ERROR_LINE = 3;     // "let a:Foo = Foo();"
constexpr size_t T1_ERROR_COLUMN = 13;  // column of 'F' in the call "Foo()"
constexpr size_t T4_ERROR_COLUMN = 9;
constexpr size_t T1_ERROR_LENGTH = 0;

constexpr std::string_view T1_EXPECTED_REPLACEMENT = "new Foo()";
constexpr std::string_view T4_EXPECTED_REPLACEMENT = "new Bar()";

// ---- Test 2: Already has 'new' -> no action ----
constexpr size_t T2_ERROR_LINE = 5;     // "const obj = new Foo();"
constexpr size_t T2_ERROR_COLUMN = 17;  // column of 'F' in "Foo()" (after 'new ')
constexpr size_t T2_ERROR_LENGTH = 0;

// ---- Test 3: Non-class function call -> no action ----
constexpr size_t T3_ERROR_LINE = 3;    // "foo();"
constexpr size_t T3_ERROR_COLUMN = 1;  // start of "foo()"
constexpr size_t T3_ERROR_LENGTH = 0;

constexpr int DEFAULT_THROTTLE = 20;

// ---- Test 4: Multiple calls ----
constexpr size_t T6_ERROR_LINE = 4;    // "let x = Foo();
constexpr size_t T6_ERROR_COLUMN = 9;  // column of 'F' in the first call "Foo()"
constexpr size_t T6_ERROR_LENGTH = 0;
constexpr std::string_view T6_EXPECTED_REPLACEMENT = "new Foo()";

class FixAddMissingNewOperatorTests : public LSPAPITests {
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

TEST_F(FixAddMissingNewOperatorTests, AddsNewKeywordToConstructorCall)
{
    std::vector<std::string> fileNames = {"AddNew_ToConstructorCall.ets"};
    std::vector<std::string> fileContents = {R"(
class Foo {}
let a:Foo = Foo();
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());
    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    size_t start = LineColToPos(context, T1_ERROR_LINE, T1_ERROR_COLUMN);
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + T1_ERROR_LENGTH, errorCodes, options);
    const size_t c1 = 1;
    ASSERT_EQ(fixes.size(), c1);
    ASSERT_EQ(fixes[0].fixName_, EXPECTED_FIX_NAME);
    ASSERT_EQ(fixes[0].description_, EXPECTED_FIX_DESCRIPTION);
    ASSERT_EQ(fixes[0].changes_[0].fileName, filePaths[0]);
    const auto &textChanges = fixes[0].changes_[0].textChanges;
    ASSERT_EQ(textChanges.size(), c1);
    const auto &tc = textChanges[0];
    EXPECT_EQ(tc.newText, T1_EXPECTED_REPLACEMENT);
    initializer.DestroyContext(context);
}

TEST_F(FixAddMissingNewOperatorTests, AddsNew_WhenClassDeclarationUsedWithoutNew)
{
    std::vector<std::string> fileNames = {"AddNew_ToClassDeclarationCall.ets"};
    std::vector<std::string> fileContents = {R"(
class Bar {}
let b = Bar();
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());
    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    size_t start = LineColToPos(context, T1_ERROR_LINE, T4_ERROR_COLUMN);
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + T1_ERROR_LENGTH, errorCodes, options);

    const size_t expectedCount = 1;
    ASSERT_EQ(fixes.size(), expectedCount);
    ASSERT_EQ(fixes[0].fixName_, EXPECTED_FIX_NAME);
    ASSERT_EQ(fixes[0].description_, EXPECTED_FIX_DESCRIPTION);
    ASSERT_EQ(fixes[0].changes_[0].fileName, filePaths[0]);

    const auto &textChanges = fixes[0].changes_[0].textChanges;
    ASSERT_EQ(textChanges.size(), expectedCount);
    const auto &tc = textChanges[0];
    EXPECT_EQ(tc.newText, T4_EXPECTED_REPLACEMENT);
    initializer.DestroyContext(context);
}

TEST_F(FixAddMissingNewOperatorTests, AddsNew_ForMultipleCalls)
{
    std::vector<std::string> fileNames = {"AddNew_MultipleCalls.ets"};
    std::vector<std::string> fileContents = {R"(
class Foo {}
let x = Foo();
let y = Foo();
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());
    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    size_t start = LineColToPos(context, T6_ERROR_LINE, T6_ERROR_COLUMN);
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + T6_ERROR_LENGTH, errorCodes, options);

    const size_t expectedCount = 1;
    ASSERT_EQ(fixes.size(), expectedCount);
    ASSERT_EQ(fixes[0].fixName_, EXPECTED_FIX_NAME);
    ASSERT_EQ(fixes[0].description_, EXPECTED_FIX_DESCRIPTION);
    ASSERT_EQ(fixes[0].changes_[0].fileName, filePaths[0]);

    const auto &textChanges = fixes[0].changes_[0].textChanges;
    ASSERT_EQ(textChanges.size(), expectedCount);
    const auto &tc = textChanges[0];
    EXPECT_EQ(tc.newText, T6_EXPECTED_REPLACEMENT);
    initializer.DestroyContext(context);
}

TEST_F(FixAddMissingNewOperatorTests, SkipsValidNewCall)
{
    std::vector<std::string> fileNames = {"Skip_WhenAlreadyHasNew.ets"};
    std::vector<std::string> fileContents = {R"(
class Foo {
    constructor() {}
}
const obj = new Foo();
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    size_t start = LineColToPos(context, T2_ERROR_LINE, T2_ERROR_COLUMN);
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + T2_ERROR_LENGTH, errorCodes, options);
    EXPECT_TRUE(fixes.empty());

    initializer.DestroyContext(context);
}

TEST_F(FixAddMissingNewOperatorTests, SkipsNonClassFunctionCalls)
{
    std::vector<std::string> fileNames = {"Skip_NonClassFunctionCalls.ets"};
    std::vector<std::string> fileContents = {R"(
function foo() {}
foo();
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    size_t start = LineColToPos(context, T3_ERROR_LINE, T3_ERROR_COLUMN);
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + T3_ERROR_LENGTH, errorCodes, options);
    EXPECT_TRUE(fixes.empty());

    initializer.DestroyContext(context);
}
}  // namespace