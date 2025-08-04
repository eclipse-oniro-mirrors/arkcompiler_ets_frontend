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

#include "../lsp_api_test.h"

#include <gtest/gtest.h>

#include "ir/astNode.h"
#include "lsp/include/register_code_fix/ui_plugin_suggest.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "public/public.h"

namespace {

class LspUISuggestionTests : public LSPAPITests {};

using ark::es2panda::lsp::Initializer;

void AssertDiagnosticContainsCodeAndMessage(const DiagnosticReferences &suggest, const int code, const char *dmessage)
{
    bool found = false;
    for (auto diag : suggest.diagnostic) {
        if (std::get<int>(diag.code_) == code) {
            if (diag.message_ == dmessage) {
                found = true;
            }
        }
    }
    ASSERT_TRUE(found) << "Expected code: " << code << " not found";
}

TEST_F(LspUISuggestionTests, UIPluginsErrorTest1)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;
    Initializer initializer = Initializer();
    std::vector<std::string> files = {"ui_error1.ets"};
    std::vector<std::string> texts = {R"delimiter(function main() {})delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    int const offset = 11;
    // NOLINTNEXTLINE
    const char *params[] = {
        "a",
    };
    const char *dmessage1 = "origin {}";
    const size_t argc1 = 1;
    const char *substitutionCode = "replace b";
    const char *title = "Title:";
    const char *dmessage2 = "error";
    const size_t argc0 = 0;
    const size_t index1 = 1;
    const size_t line1 = 0;
    const size_t index2 = 15;
    const size_t line2 = 0;
    const int code = 4000;
    auto suggestionkind = initializer.CreateDiagnosticKind(ctx, dmessage1, ES2PANDA_PLUGIN_SUGGESTION);
    auto diagnostikind = initializer.CreateDiagnosticKind(ctx, dmessage2, ES2PANDA_PLUGIN_ERROR);
    es2panda_SourcePosition *left = initializer.CreateSourcePosition(ctx, index1, line1);
    es2panda_SourcePosition *right = initializer.CreateSourcePosition(ctx, index2, line2);
    es2panda_SourceRange *range = initializer.CreateSourceRange(ctx, left, right);
    auto suggestionInfo =
        initializer.CreateSuggestionInfo(ctx, suggestionkind, params, argc1, substitutionCode, title, range);
    auto diagnosticInfo = initializer.CreateDiagnosticInfo(ctx, diagnostikind, nullptr, argc0, left);
    initializer.LogDiagnosticWithSuggestion(ctx, diagnosticInfo, suggestionInfo);
    auto suggest = lspApi->getSyntacticDiagnostics(ctx);
    AssertDiagnosticContainsCodeAndMessage(suggest, code, dmessage2);
    auto result = ark::es2panda::lsp::UIPluginSuggest::GetUIPluginCodeFixes(ctx, offset, false);
    ASSERT_EQ(result.at(0).textChanges.at(0).newText, substitutionCode);
    std::vector<int> codes;
    codes.emplace_back(code);
    CodeFixOptions emptyOptions;
    auto fix = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, index1, index2, codes, emptyOptions);
    ASSERT_EQ(fix.at(0).changes_.at(0).textChanges.at(0).newText, substitutionCode);
    ASSERT_EQ(fix.at(0).changes_.at(0).textChanges.at(0).span.length, index2 - index1);

    initializer.DestroyContext(ctx);
}

TEST_F(LspUISuggestionTests, UIPluginsErrorTest2)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;
    Initializer initializer = Initializer();
    std::vector<std::string> files = {"ui_error2.ets"};
    std::vector<std::string> texts = {R"delimiter(function main() {})delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    // NOLINTNEXTLINE
    const char *params[] = {
        "a",
    };
    const size_t argc1 = 1;
    const char *dmessage2 = "origin {}";
    const char *dmessage1 = "origin a";
    const size_t index1 = 0;
    const size_t line1 = 0;
    const int code = 4000;
    auto diagnostikind = initializer.CreateDiagnosticKind(ctx, dmessage2, ES2PANDA_PLUGIN_ERROR);
    es2panda_SourcePosition *left = initializer.CreateSourcePosition(ctx, index1, line1);

    initializer.LogDiagnostic(ctx, diagnostikind, params, argc1, left);
    auto suggest = lspApi->getSyntacticDiagnostics(ctx);
    AssertDiagnosticContainsCodeAndMessage(suggest, code, dmessage1);
    std::vector<int> codes;
    codes.emplace_back(code);
    CodeFixOptions emptyOptions;
    auto fix = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, index1, index1, codes, emptyOptions);
    ASSERT_TRUE(fix.at(0).changes_.at(0).textChanges.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LspUISuggestionTests, UIPluginsErrorTest3)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;
    Initializer initializer = Initializer();
    std::vector<std::string> files = {"ui_error3.ets"};
    std::vector<std::string> texts = {R"delimiter(function main() {})delimiter"};
    auto ctx = initializer.CreateContext(CreateTempFile(files, texts)[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const char *dmessage1 = "origin a";
    const char *substitutionCode = "replace b";
    const char *title1 = "Title1:";
    const char *dmessage2 = "error";
    const size_t argc0 = 0;
    const size_t index1 = 0;
    const size_t line1 = 0;
    const size_t index2 = 15;
    const size_t line2 = 0;
    const int code = 4000;
    auto suggestionkind = initializer.CreateDiagnosticKind(ctx, dmessage1, ES2PANDA_PLUGIN_SUGGESTION);
    auto diagnostikind = initializer.CreateDiagnosticKind(ctx, dmessage2, ES2PANDA_PLUGIN_ERROR);
    es2panda_SourcePosition *left = initializer.CreateSourcePosition(ctx, index1, line1);
    es2panda_SourcePosition *right = initializer.CreateSourcePosition(ctx, index2, line2);
    es2panda_SourceRange *range = initializer.CreateSourceRange(ctx, left, right);
    auto suggestionInfo =
        initializer.CreateSuggestionInfo(ctx, suggestionkind, nullptr, argc0, substitutionCode, title1, range);
    auto diagnosticInfo = initializer.CreateDiagnosticInfo(ctx, diagnostikind, nullptr, argc0, left);
    initializer.LogDiagnosticWithSuggestion(ctx, diagnosticInfo, suggestionInfo);

    const char *substitutionCode2 = "replace c";
    const char *title2 = "Title2:";
    auto suggestionkind2 = initializer.CreateDiagnosticKind(ctx, dmessage1, ES2PANDA_PLUGIN_SUGGESTION);
    auto diagnostikind2 = initializer.CreateDiagnosticKind(ctx, dmessage2, ES2PANDA_PLUGIN_ERROR);
    es2panda_SourcePosition *left2 = initializer.CreateSourcePosition(ctx, index1, line1);
    es2panda_SourcePosition *right2 = initializer.CreateSourcePosition(ctx, index2, line2);
    es2panda_SourceRange *range2 = initializer.CreateSourceRange(ctx, left2, right2);
    auto suggestionInfo2 =
        initializer.CreateSuggestionInfo(ctx, suggestionkind2, nullptr, argc0, substitutionCode2, title2, range2);
    auto diagnosticInfo2 = initializer.CreateDiagnosticInfo(ctx, diagnostikind2, nullptr, argc0, left2);
    initializer.LogDiagnosticWithSuggestion(ctx, diagnosticInfo2, suggestionInfo2);

    auto suggest = lspApi->getSyntacticDiagnostics(ctx);
    AssertDiagnosticContainsCodeAndMessage(suggest, code, dmessage2);
    AssertDiagnosticContainsCodeAndMessage(suggest, code, dmessage2);
    std::vector<int> codes;
    codes.emplace_back(code);
    CodeFixOptions emptyOptions;
    const std::string fixId = "UIPluginSuggest";
    auto fix = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, index1, index1, codes, emptyOptions);
    ASSERT_FALSE(fix.at(0).changes_.at(0).textChanges.empty());
    auto fixAll = ark::es2panda::lsp::GetCombinedCodeFixImpl(ctx, fixId, emptyOptions);
    const size_t expectedCount = 2;
    ASSERT_EQ(fixAll.changes_.size(), expectedCount);

    initializer.DestroyContext(ctx);
}

TEST_F(LspUISuggestionTests, UIPluginsErrorTest4)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;
    Initializer initializer = Initializer();
    std::vector<std::string> files = {"ui_error1.ets"};
    std::vector<std::string> texts = {R"delimiter(function main() { return 1 })delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    int const offset = 9;
    // NOLINTNEXTLINE
    const char *params[] = {
        "a",
    };
    const char *dmessage1 = "origin {}";
    const size_t argc1 = 1;
    const char *substitutionCode = "replace b";
    const char *title = "Title:";
    const char *dmessage2 = "error";
    const size_t argc0 = 0;
    const size_t index1 = 16;  // suggestion left
    const size_t line1 = 0;
    const size_t index2 = 27;  // suggestion right
    const size_t line2 = 0;
    const size_t index3 = 9;  // diag pos
    const size_t line3 = 0;
    const size_t index4 = 9;  // diag right virtual
    const int code = 4000;
    auto suggestionkind = initializer.CreateDiagnosticKind(ctx, dmessage1, ES2PANDA_PLUGIN_SUGGESTION);
    auto diagnostikind = initializer.CreateDiagnosticKind(ctx, dmessage2, ES2PANDA_PLUGIN_ERROR);
    es2panda_SourcePosition *left = initializer.CreateSourcePosition(ctx, index1, line1);
    es2panda_SourcePosition *right = initializer.CreateSourcePosition(ctx, index2, line2);
    es2panda_SourcePosition *diagPos = initializer.CreateSourcePosition(ctx, index3, line3);
    es2panda_SourceRange *range = initializer.CreateSourceRange(ctx, left, right);
    auto suggestionInfo =
        initializer.CreateSuggestionInfo(ctx, suggestionkind, params, argc1, substitutionCode, title, range);
    auto diagnosticInfo = initializer.CreateDiagnosticInfo(ctx, diagnostikind, nullptr, argc0, diagPos);
    initializer.LogDiagnosticWithSuggestion(ctx, diagnosticInfo, suggestionInfo);
    auto suggest = lspApi->getSyntacticDiagnostics(ctx);
    AssertDiagnosticContainsCodeAndMessage(suggest, code, dmessage2);
    auto result = ark::es2panda::lsp::UIPluginSuggest::GetUIPluginCodeFixes(ctx, offset, false);
    ASSERT_EQ(result.at(0).textChanges.at(0).newText, substitutionCode);
    std::vector<int> codes;
    codes.emplace_back(code);
    CodeFixOptions emptyOptions;
    auto fix = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, index3, index4, codes, emptyOptions);
    ASSERT_EQ(fix.at(0).changes_.at(0).textChanges.at(0).newText, substitutionCode);
    ASSERT_EQ(fix.at(0).changes_.at(0).textChanges.at(0).span.length, index2 - index1);

    initializer.DestroyContext(ctx);
}

}  // namespace
