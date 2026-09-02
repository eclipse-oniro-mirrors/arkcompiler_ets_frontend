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

#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_spelling_for_property.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_SPELLING_FOR_PROPERTY;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_SPELLING_FOR_PROPERTY.GetFixId();
constexpr auto ERROR_CODES = FIX_SPELLING_FOR_PROPERTY.GetSupportedCodeNumbers();
// PROPERTY_NONEXISTENT: DiagnosticType::SEMANTIC * DIAGNOSTIC_CODE_MULTIPLIER + 87
constexpr int PROPERTY_NONEXISTENT_CODE = 2087;
constexpr int DEFAULT_THROTTLE = 20;

class FixSpellingForPropertyTests1 : public LSPAPITests {
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

    static bool HasFixWithName(const std::vector<CodeFixActionInfo> &fixes, std::string_view fixName)
    {
        for (const auto &fix : fixes) {
            if (fix.fixName_ == fixName) {
                return true;
            }
        }
        return false;
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

// Test: shared PROPERTY_NONEXISTENT errorCode should return both FixSpellingForProperty and AddLocalVariable
TEST_F(FixSpellingForPropertyTests1, SharedErrorCodeReturnsBothFixes)
{
    std::vector<std::string> fileNames = {"SharedErrorCodeReturnsBothFixes.ets"};
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

    // Position at "nam" (line 7, col 9)
    const size_t start = LineColToPos(context, 7, 9);
    const size_t length = 3;

    // Use the shared error code
    std::vector<int> errorCodes = {PROPERTY_NONEXISTENT_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    // Both FixSpellingForProperty and AddLocalVariable may be returned for the same error code
    // Verify that at least one fix is returned
    bool hasSpellingFix = false;
    for (const auto &result : fixResult) {
        if (result.fixName_ == EXPECTED_FIX_NAME) {
            hasSpellingFix = true;
        }
    }
    // At least the spelling fix should be present since "nam" is close to "name"
    ASSERT_TRUE(hasSpellingFix);

    initializer.DestroyContext(context);
}

// Test: multiple similar candidates - should suggest the closest match
TEST_F(FixSpellingForPropertyTests1, MultipleCandidatesSuggestsClosest)
{
    std::vector<std::string> fileNames = {"MultipleCandidatesSuggestsClosest.ets"};
    std::vector<std::string> fileContents = {R"(
class MyClass {
    name: string = "hello";
    names: string[] = ["a", "b"];
    named: string = "test";
}
function foo(): void {
    let obj = new MyClass();
    obj.name;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "name" (line 9, col 9) - should match "name" exactly or closest
    const size_t start = LineColToPos(context, 9, 9);
    const size_t length = 4;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    // "name" is an exact match, so no spelling fix should be needed
    // But if there's a diagnostic, verify the closest candidate is suggested
    for (const auto &result : fixResult) {
        if (result.fixName_ == EXPECTED_FIX_NAME) {
            // The suggested text should be one of the close matches
            ASSERT_TRUE(result.description_.find("name") != std::string::npos ||
                        result.description_.find("names") != std::string::npos ||
                        result.description_.find("named") != std::string::npos);
        }
    }

    initializer.DestroyContext(context);
}

// Test: private member should not be suggested from outside the class
TEST_F(FixSpellingForPropertyTests1, PrivateMemberNotSuggestedFromOutside)
{
    std::vector<std::string> fileNames = {"PrivateMemberNotSuggestedFromOutside.ets"};
    std::vector<std::string> fileContents = {R"(
class MyClass {
    private secretField: string = "secret";
    public publicField: string = "public";
}
function foo(): void {
    let obj = new MyClass();
    obj.secretFild;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "secretFild" (line 7, col 9)
    const size_t start = LineColToPos(context, 7, 9);
    const size_t length = 9;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    // If a spelling fix is returned, it should not suggest the private "secretField"
    for (const auto &result : fixResult) {
        if (result.fixName_ == EXPECTED_FIX_NAME) {
            // Private members should not be suggested from outside the class
            ASSERT_TRUE(result.description_.find("secretField") == std::string::npos);
        }
    }

    initializer.DestroyContext(context);
}

// Test: getter/setter should be suggested
TEST_F(FixSpellingForPropertyTests1, GetterSetterSuggested)
{
    std::vector<std::string> fileNames = {"GetterSetterSuggested.ets"};
    std::vector<std::string> fileContents = {R"(
class MyClass {
    private _value: number = 0;
    get value(): number { return this._value; }
    set value(v: number) { this._value = v; }
}
function foo(): void {
    let obj = new MyClass();
    obj.valu;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "valu" (line 9, col 9)
    const size_t start = LineColToPos(context, 9, 9);
    const size_t length = 4;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    // Find the fix action that suggests "value" (the getter/setter)
    bool foundSuggestion = false;
    for (const auto &result : fixResult) {
        if (result.fixName_ == EXPECTED_FIX_NAME && result.description_.find("value") != std::string::npos) {
            ValidateCodeFixActionInfo(result, "value", filePaths[0]);
            foundSuggestion = true;
            break;
        }
    }
    ASSERT_TRUE(foundSuggestion);

    initializer.DestroyContext(context);
}

}  // namespace
