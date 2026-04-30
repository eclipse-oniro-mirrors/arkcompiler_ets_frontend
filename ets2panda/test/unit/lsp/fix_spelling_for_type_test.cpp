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
#include "lsp/include/register_code_fix/fix_spelling_for_type.h"
#include "lsp/include/symbol_reference_index.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_SPELLING_FOR_TYPE;
using ark::es2panda::util::DiagnosticType;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_SPELLING_FOR_TYPE.GetFixId();
constexpr auto ERROR_CODES = FIX_SPELLING_FOR_TYPE.GetSupportedCodeNumbers();
// TYPE_NOT_FOUND: DiagnosticType::SEMANTIC * DIAGNOSTIC_CODE_MULTIPLIER + 371
constexpr int TYPE_NOT_FOUND_CODE = 2371;
constexpr int DEFAULT_THROTTLE = 20;

class FixSpellingForTypeTests : public LSPAPITests {
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

// Test: type name "MyClas" should suggest "MyClass" as correction
// Using a user-defined class so the symbol is indexed from source
TEST_F(FixSpellingForTypeTests, TestFixSpellingForTypeSuggestClass)
{
    std::vector<std::string> fileNames = {"TestFixSpellingForTypeSuggestClass.ets"};
    std::vector<std::string> fileContents = {R"(
class MyClass {
    myProp: string = "hello";
}
let x: MyClas = new MyClass();
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Build symbol index so that "MyClass" is indexed
    ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context);

    // Position at "MyClas" type name (line 5, col 8)
    const size_t start = LineColToPos(context, 5, 8);
    const size_t length = 6;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    // Verify target error code: TYPE_NOT_FOUND(2371)
    ASSERT_EQ(errorCodes.size(), 1U);
    ASSERT_EQ(errorCodes[0], TYPE_NOT_FOUND_CODE);
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_GE(fixResult.size(), 1U);

    // Find the fix action that suggests "MyClass"
    bool foundClassSuggestion = false;
    for (const auto &result : fixResult) {
        if (result.description_.find("MyClass") != std::string::npos) {
            ValidateCodeFixActionInfo(result, "MyClass", filePaths[0]);
            foundClassSuggestion = true;
            break;
        }
    }
    ASSERT_TRUE(foundClassSuggestion);

    initializer.DestroyContext(context);
}

// Test: no similar type found should return empty result
TEST_F(FixSpellingForTypeTests, TestFixSpellingForTypeNoMatch)
{
    std::vector<std::string> fileNames = {"TestFixSpellingForTypeNoMatch.ets"};
    std::vector<std::string> fileContents = {R"(
let x: xyzabc = 1;
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context);

    const size_t start = LineColToPos(context, 2, 8);
    const size_t length = 6;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    // Should return empty - no similar types
    ASSERT_EQ(fixResult.size(), 0U);

    initializer.DestroyContext(context);
}

}  // namespace
