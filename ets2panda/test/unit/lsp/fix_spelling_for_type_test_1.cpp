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

constexpr std::string_view EXPECTED_FIX_NAME = FIX_SPELLING_FOR_TYPE.GetFixId();
constexpr auto ERROR_CODES = FIX_SPELLING_FOR_TYPE.GetSupportedCodeNumbers();
// TYPE_NOT_FOUND: DiagnosticType::SEMANTIC * DIAGNOSTIC_CODE_MULTIPLIER + 371
constexpr int TYPE_NOT_FOUND_CODE = 2371;
constexpr int DEFAULT_THROTTLE = 20;
constexpr size_t EXPECTED_FIX_ALL_CHANGE_COUNT = 2;

class FixSpellingForTypeTests1 : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static std::vector<CodeFixActionInfo> GetFixes(es2panda_Context *context, size_t start, size_t length)
    {
        std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
        CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    }

    static const CodeFixActionInfo *FindSpellingFix(const std::vector<CodeFixActionInfo> &fixes)
    {
        for (const auto &fix : fixes) {
            if (fix.fixName_ == EXPECTED_FIX_NAME) {
                return &fix;
            }
        }
        return nullptr;
    }

    static void ValidateSingleReplacement(const CodeFixActionInfo &fix, const std::string &fileName,
                                          size_t expectedStart, size_t expectedLength, const std::string &expectedText)
    {
        ASSERT_EQ(fix.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(fix.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(fix.description_, "Did you mean '" + expectedText + "'?");
        ASSERT_EQ(fix.changes_.size(), 1U);
        ASSERT_EQ(fix.changes_[0].fileName, fileName);
        ASSERT_EQ(fix.changes_[0].textChanges.size(), 1U);
        EXPECT_EQ(fix.changes_[0].textChanges[0].span.start, expectedStart);
        EXPECT_EQ(fix.changes_[0].textChanges[0].span.length, expectedLength);
        EXPECT_EQ(fix.changes_[0].textChanges[0].newText, expectedText);
    }

    static bool HasReplacement(const CombinedCodeActionsInfo &fix, size_t expectedStart, size_t expectedLength,
                               const std::string &expectedText)
    {
        for (const auto &fileChange : fix.changes_) {
            for (const auto &textChange : fileChange.textChanges) {
                if (textChange.span.start == expectedStart && textChange.span.length == expectedLength &&
                    textChange.newText == expectedText) {
                    return true;
                }
            }
        }
        return false;
    }

    void SetUp() override
    {
        LSPAPITests::SetUp();
        ark::es2panda::lsp::ClearSymbolReferenceIndex();
    }

    void TearDown() override
    {
        ark::es2panda::lsp::ClearSymbolReferenceIndex();
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

TEST_F(FixSpellingForTypeTests1, SuggestsInterfaceAndEnumTypes)
{
    const std::string source = R"(
interface UserProfile {
    id: number;
}
enum ColorMode {
    LIGHT,
    DARK
}
let user: UserProfle;
let mode: ColorMod;
)";
    auto filePaths = CreateTempFile({"suggest_interface_enum.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));
    ASSERT_EQ(ERROR_CODES.size(), 1U);
    ASSERT_EQ(ERROR_CODES[0], TYPE_NOT_FOUND_CODE);

    const size_t interfaceStart = source.find("UserProfle");
    const size_t enumStart = source.find("ColorMod;");
    ASSERT_NE(interfaceStart, std::string::npos);
    ASSERT_NE(enumStart, std::string::npos);

    auto interfaceFixes = GetFixes(context, interfaceStart, std::string("UserProfle").size());
    auto enumFixes = GetFixes(context, enumStart, std::string("ColorMod").size());
    initializer.DestroyContext(context);

    const auto *interfaceFix = FindSpellingFix(interfaceFixes);
    const auto *enumFix = FindSpellingFix(enumFixes);
    ASSERT_NE(interfaceFix, nullptr);
    ASSERT_NE(enumFix, nullptr);
    ValidateSingleReplacement(*interfaceFix, filePaths[0], interfaceStart, std::string("UserProfle").size(),
                              "UserProfile");
    ValidateSingleReplacement(*enumFix, filePaths[0], enumStart, std::string("ColorMod").size(), "ColorMode");
}

TEST_F(FixSpellingForTypeTests1, SuggestsShortCaseOnlyTypeName)
{
    const std::string source = R"(
interface ID {
    value: number;
}
let item: id;
)";
    auto filePaths = CreateTempFile({"suggest_case_only.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    const size_t start = source.find("id;");
    ASSERT_NE(start, std::string::npos);
    auto fixes = GetFixes(context, start, std::string("id").size());
    initializer.DestroyContext(context);

    const auto *fix = FindSpellingFix(fixes);
    ASSERT_NE(fix, nullptr);
    ValidateSingleReplacement(*fix, filePaths[0], start, std::string("id").size(), "ID");
}

TEST_F(FixSpellingForTypeTests1, FixAllReplacesMultipleMisspelledTypes)
{
    const std::string source = R"(
interface AccountDetails {
    id: number;
}
class ProjectSettings {
    enabled: boolean = true;
}
let account: AcountDetails;
let settings: ProjectSetings;
)";
    auto filePaths = CreateTempFile({"fix_all_type_spelling.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    const size_t accountStart = source.find("AcountDetails");
    const size_t settingsStart = source.find("ProjectSetings");
    ASSERT_NE(accountStart, std::string::npos);
    ASSERT_NE(settingsStart, std::string::npos);

    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto combinedFix = ark::es2panda::lsp::GetCombinedCodeFixImpl(context, EXPECTED_FIX_NAME.data(), options);
    initializer.DestroyContext(context);

    ASSERT_EQ(combinedFix.changes_.size(), 1U);
    ASSERT_EQ(combinedFix.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(combinedFix.changes_[0].textChanges.size(), EXPECTED_FIX_ALL_CHANGE_COUNT);
    EXPECT_TRUE(HasReplacement(combinedFix, accountStart, std::string("AcountDetails").size(), "AccountDetails"));
    EXPECT_TRUE(HasReplacement(combinedFix, settingsStart, std::string("ProjectSetings").size(), "ProjectSettings"));
}

}  // namespace
