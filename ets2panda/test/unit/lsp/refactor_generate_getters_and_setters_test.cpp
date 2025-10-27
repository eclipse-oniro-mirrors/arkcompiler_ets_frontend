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
#include <algorithm>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <string_view>
#include "lsp/include/refactors/refactor_types.h"
#include "lsp_api_test.h"
#include "lsp/include/get_edits_for_refactor.h"
#include "lsp/include/types.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/user_preferences.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::GetApplicableRefactorsImpl;
using ark::es2panda::lsp::GetEditsForRefactorsImpl;
using ark::es2panda::lsp::Initializer;

class LspGenerateGS : public LSPAPITests {
public:
    static constexpr std::string_view kind = "refactor.rewrite.property.generateAccessors";
    static constexpr std::string_view name = "GenerateGettersAndSettersRefactor";
    static constexpr size_t ageFieldPos = 34;   // caret at "_age"
    static constexpr size_t nameFieldPos = 27;  // caret at "name"
    static constexpr size_t homeFieldPos = 27;  // caret at "_home"/"home"

    // Create a temporary ArkTS file with given class body
    std::vector<std::string> CreateFiles(const std::string &classBody, std::string_view fileName)
    {
        std::vector<std::string> names = {std::string(fileName)};
        std::string src = "export class Person {\n" + classBody + "\n}\n";
        std::vector<std::string> contents = {src};
        return CreateTempFile(names, contents);
    }

    // Check that generated edits contain the expected getter and setter
    static void AssertInsertBeforeClassClosePtr(const std::unique_ptr<ark::es2panda::lsp::RefactorEditInfo> &editsPtr,
                                                const std::string &mustContain1, const std::string &mustContain2)
    {
        ASSERT_NE(editsPtr, nullptr);
        const auto &edits = editsPtr->GetFileTextChanges();
        ASSERT_EQ(1U, edits.size());
        ASSERT_FALSE(edits[0].textChanges.empty());

        bool ok = false;
        for (auto &tc : edits[0].textChanges) {
            if (tc.newText.find(mustContain1) != std::string::npos &&
                tc.newText.find(mustContain2) != std::string::npos) {
                ok = true;
                ASSERT_EQ(tc.span.length, 0);  // insertion check
            }
        }
        ASSERT_TRUE(ok);
    }

    static const ark::es2panda::lsp::ApplicableRefactorInfo *FindGeneratedAccessors(
        const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable)
    {
        auto it = std::find_if(applicable.begin(), applicable.end(),
                               [](const auto &info) { return info.action.name == name && info.action.kind == kind; });
        return it != applicable.end() ? &(*it) : nullptr;
    }
};

TEST_F(LspGenerateGS, OfferOnPrivateUnderscoredFieldAndGenerateTypedAccessors)
{
    const std::string classBody = "  private _age: number;";
    auto files = CreateFiles(classBody, "GenGSPrivate.ets");
    ASSERT_EQ(1, static_cast<int>(files.size()));

    Initializer init;
    auto ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings codeSettings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
    ark::es2panda::lsp::FormatContext fmt = ark::es2panda::lsp::GetFormatContext(codeSettings);
    ark::es2panda::lsp::UserPreferences prefs = ark::es2panda::lsp::UserPreferences::GetDefaultUserPreferences();
    LanguageServiceHost host;
    auto tcc = std::make_unique<TextChangesContext>(TextChangesContext {host, fmt, prefs});

    ark::es2panda::lsp::RefactorContext rc;
    rc.textChangesContext = tcc.get();
    rc.context = ctx;
    rc.kind = std::string(LspGenerateGS::kind);

    // Hardcoded position (pick start of "_age")
    rc.span.pos = ageFieldPos;

    auto applicable = GetApplicableRefactorsImpl(&rc);
    ASSERT_FALSE(applicable.empty());
    const auto *generateAction = FindGeneratedAccessors(applicable);
    ASSERT_NE(generateAction, nullptr);

    auto edits = GetEditsForRefactorsImpl(rc, generateAction->name, generateAction->action.name);
    init.DestroyContext(ctx);

    AssertInsertBeforeClassClosePtr(edits, "get age(): number", "set age(value: number)");
}

TEST_F(LspGenerateGS, OfferOnPlainFieldAndUseNonClashingAccessorNames)
{
    const std::string classBody = "  name: string;";
    auto files = CreateFiles(classBody, "GenGSPlain.ets");
    ASSERT_EQ(1, static_cast<int>(files.size()));

    Initializer init;
    auto ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings codeSettings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
    ark::es2panda::lsp::FormatContext fmt = ark::es2panda::lsp::GetFormatContext(codeSettings);
    ark::es2panda::lsp::UserPreferences prefs = ark::es2panda::lsp::UserPreferences::GetDefaultUserPreferences();
    LanguageServiceHost host;
    auto tcc = std::make_unique<TextChangesContext>(TextChangesContext {host, fmt, prefs});

    ark::es2panda::lsp::RefactorContext rc;
    rc.textChangesContext = tcc.get();
    rc.context = ctx;
    rc.kind = std::string(LspGenerateGS::kind);

    // Hardcoded position (pick start of "name")
    rc.span.pos = nameFieldPos;

    auto applicable = GetApplicableRefactorsImpl(&rc);
    ASSERT_FALSE(applicable.empty());
    const auto *generateAction = FindGeneratedAccessors(applicable);
    ASSERT_NE(generateAction, nullptr);

    auto edits = GetEditsForRefactorsImpl(rc, generateAction->name, generateAction->action.name);
    init.DestroyContext(ctx);

    AssertInsertBeforeClassClosePtr(edits, "get get_name()", "set set_name(value");
}

TEST_F(LspGenerateGS, OfferOnCustomClassFieldAndGenerateTypedAccessors)
{
    const std::string classBody = "  _home: Address = new Address();\n}\n\nclass Address {\n  street: string = \"\";\n";
    auto files = CreateFiles(classBody, "GenGSCustom.ets");

    ASSERT_EQ(1, static_cast<int>(files.size()));

    Initializer init;
    auto ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings codeSettings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
    ark::es2panda::lsp::FormatContext fmt = ark::es2panda::lsp::GetFormatContext(codeSettings);
    ark::es2panda::lsp::UserPreferences prefs = ark::es2panda::lsp::UserPreferences::GetDefaultUserPreferences();
    LanguageServiceHost host;
    auto tcc = std::make_unique<TextChangesContext>(TextChangesContext {host, fmt, prefs});

    ark::es2panda::lsp::RefactorContext rc;
    rc.textChangesContext = tcc.get();
    rc.context = ctx;
    rc.kind = std::string(LspGenerateGS::kind);

    // Hardcoded position (pick start of "home")
    rc.span.pos = homeFieldPos;

    auto applicable = GetApplicableRefactorsImpl(&rc);
    ASSERT_FALSE(applicable.empty());
    const auto *generateAction = FindGeneratedAccessors(applicable);
    ASSERT_NE(generateAction, nullptr);

    auto edits = GetEditsForRefactorsImpl(rc, generateAction->name, generateAction->action.name);
    init.DestroyContext(ctx);

    AssertInsertBeforeClassClosePtr(edits, "get home(): Address", "set home(value: Address)");
}

}  // namespace
