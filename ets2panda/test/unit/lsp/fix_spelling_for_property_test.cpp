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

#include "generated/code_fix_register.h"
#include "lsp_api_test.h"
#include "util/diagnostic.h"

#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_spelling_for_property.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_SPELLING_FOR_PROPERTY;
using ark::es2panda::util::DiagnosticType;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_SPELLING_FOR_PROPERTY.GetFixId();
constexpr auto ERROR_CODES = FIX_SPELLING_FOR_PROPERTY.GetSupportedCodeNumbers();
// PROPERTY_NONEXISTENT: DiagnosticType::SEMANTIC * DIAGNOSTIC_CODE_MULTIPLIER + 87
constexpr int PROPERTY_NONEXISTENT_CODE = 2087;
constexpr int DEFAULT_THROTTLE = 20;

class FixSpellingForPropertyTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto pos = index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
        return pos;
    }

    static void ValidateCodeFixActionInfo(const CodeFixActionInfo &info, const std::string &expectedNewText,
                                          const std::string &expectedFileName)
    {
        ASSERT_EQ(info.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info.fixId_, EXPECTED_FIX_NAME);
        ASSERT_FALSE(info.description_.empty());
        ASSERT_EQ(info.changes_[0].fileName, expectedFileName);
        ASSERT_EQ(info.changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(info.changes_[0].textChanges[0].newText, expectedNewText);
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

// Test: misspelled field name "nam" should suggest "name"
TEST_F(FixSpellingForPropertyTests, TestFixSpellingForPropertySuggestField)
{
    std::vector<std::string> fileNames = {"TestFixSpellingForPropertySuggestField.ets"};
    std::vector<std::string> fileContents = {R"(
class MyClass {
    name: string = "hello";
}
function foo(): void {
    let obj = new MyClass();
    obj.nam;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "nam" property name (line 7, col 9)
    const size_t start = LineColToPos(context, 7, 9);
    const size_t length = 3;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    // Verify target error code: PROPERTY_NONEXISTENT(2087)
    ASSERT_EQ(errorCodes.size(), 1U);
    ASSERT_EQ(errorCodes[0], PROPERTY_NONEXISTENT_CODE);
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    // Find the fix action that suggests "name"
    bool foundSuggestion = false;
    for (const auto &result : fixResult) {
        if (result.description_.find("name") != std::string::npos) {
            ValidateCodeFixActionInfo(result, "name", filePaths[0]);
            foundSuggestion = true;
            break;
        }
    }
    ASSERT_TRUE(foundSuggestion);

    initializer.DestroyContext(context);
}

// Test: misspelled method name "myMetho" should suggest "myMethod"
TEST_F(FixSpellingForPropertyTests, TestFixSpellingForPropertySuggestMethod)
{
    std::vector<std::string> fileNames = {"TestFixSpellingForPropertySuggestMethod.ets"};
    std::vector<std::string> fileContents = {R"(
class MyClass {
    myMethod(): void {}
}
function foo(): void {
    let obj = new MyClass();
    obj.myMetho;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 7, 9);
    const size_t length = 7;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    // Verify target error code: PROPERTY_NONEXISTENT(2087)
    ASSERT_EQ(errorCodes.size(), 1U);
    ASSERT_EQ(errorCodes[0], PROPERTY_NONEXISTENT_CODE);
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    // Find the fix action that suggests "myMethod"
    bool foundSuggestion = false;
    for (const auto &result : fixResult) {
        if (result.description_.find("myMethod") != std::string::npos) {
            ValidateCodeFixActionInfo(result, "myMethod", filePaths[0]);
            foundSuggestion = true;
            break;
        }
    }
    ASSERT_TRUE(foundSuggestion);

    initializer.DestroyContext(context);
}

// Test: no similar property found should return empty result
TEST_F(FixSpellingForPropertyTests, TestFixSpellingForPropertyNoMatch)
{
    std::vector<std::string> fileNames = {"TestFixSpellingForPropertyNoMatch.ets"};
    std::vector<std::string> fileContents = {R"(
class MyClass {
    name: string = "hello";
}
function foo(): void {
    let obj = new MyClass();
    obj.xyzabc;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 7, 9);
    const size_t length = 6;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    // Verify target error code: PROPERTY_NONEXISTENT(2087)
    ASSERT_EQ(errorCodes.size(), 1U);
    ASSERT_EQ(errorCodes[0], PROPERTY_NONEXISTENT_CODE);
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    // FixSpellingForProperty returns no match, but AddLocalVariable may also produce a result
    // for the same error code (PROPERTY_NONEXISTENT), so we only verify no spelling suggestion is present.
    for (const auto &result : fixResult) {
        ASSERT_TRUE(result.description_.find("Did you mean") == std::string::npos);
    }

    initializer.DestroyContext(context);
}

// Test: inherited property from base class should be suggested
TEST_F(FixSpellingForPropertyTests, TestFixSpellingForPropertyInherited)
{
    std::vector<std::string> fileNames = {"TestFixSpellingForPropertyInherited.ets"};
    std::vector<std::string> fileContents = {R"(
class Base {
    baseValue: number = 42;
}
class Derived extends Base {
    derivedProp: string = "test";
}
function foo(): void {
    let obj = new Derived();
    obj.baseValu;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 10, 9);
    const size_t length = 8;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    // Verify target error code: PROPERTY_NONEXISTENT(2087)
    ASSERT_EQ(errorCodes.size(), 1U);
    ASSERT_EQ(errorCodes[0], PROPERTY_NONEXISTENT_CODE);
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    // Find the fix action that suggests "baseValue"
    bool foundSuggestion = false;
    for (const auto &result : fixResult) {
        if (result.description_.find("baseValue") != std::string::npos) {
            ValidateCodeFixActionInfo(result, "baseValue", filePaths[0]);
            foundSuggestion = true;
            break;
        }
    }
    ASSERT_TRUE(foundSuggestion);

    initializer.DestroyContext(context);
}

// Test: no similar property found on assignment target should not suggest spelling fix
TEST_F(FixSpellingForPropertyTests, TestFixSpellingForPropertyAssignmentNoMatch)
{
    std::vector<std::string> fileNames = {"TestFixSpellingForPropertyAssignmentNoMatch.ets"};
    std::vector<std::string> fileContents = {R"(
class MyClass {
    myField: number = 0;
}
let obj = new MyClass();
obj.xyzabc = 1;
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 6, 5);
    const size_t length = 6;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    // Verify target error code: PROPERTY_NONEXISTENT(2087)
    ASSERT_EQ(errorCodes.size(), 1U);
    ASSERT_EQ(errorCodes[0], PROPERTY_NONEXISTENT_CODE);
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    // Other code fixes can share PROPERTY_NONEXISTENT, but spelling should not suggest unrelated "myField".
    for (const auto &result : fixResult) {
        ASSERT_TRUE(result.description_.find("Did you mean") == std::string::npos);
        ASSERT_TRUE(result.description_.find("myField") == std::string::npos);
    }

    initializer.DestroyContext(context);
}

}  // namespace
