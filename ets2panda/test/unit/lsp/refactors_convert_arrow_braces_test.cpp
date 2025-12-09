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

#include <gtest/gtest.h>
#include <cstddef>
#include <string>
#include <algorithm>
#include "lsp_api_test.h"
#include "lsp/include/applicable_refactors.h"
#include "lsp/include/refactors/convert_arrow_braces.h"
#include "lsp/include/services/text_change/text_change_context.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/user_preferences.h"
#include "lsp/include/get_edits_for_refactor.h"

namespace {
using ark::es2panda::lsp::ADD_BRACES_ACTION;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::REMOVE_BRACES_ACTION;

class LspConvertArrowBracesTests : public LSPAPITests {
protected:
    ark::es2panda::lsp::RefactorContext *CreateArrowBracesContext(Initializer &initializer, const std::string &fileName,
                                                                  const std::string &code, size_t startPos,
                                                                  size_t endPos)
    {
        std::vector<std::string> files = {fileName};
        std::vector<std::string> texts = {code};
        auto filePaths = CreateTempFile(files, texts);
        auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

        ark::es2panda::lsp::UserPreferences prefs = ark::es2panda::lsp::UserPreferences::GetDefaultUserPreferences();
        ark::es2panda::lsp::FormatCodeSettings settings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
        ark::es2panda::lsp::FormatContext fmt = ark::es2panda::lsp::GetFormatContext(settings);
        LanguageServiceHost host;
        auto *textChangesContext = new TextChangesContext {host, fmt, prefs};

        auto *refactorContext = new ark::es2panda::lsp::RefactorContext;
        refactorContext->context = ctx;
        refactorContext->textChangesContext = textChangesContext;
        refactorContext->span.pos = startPos;
        refactorContext->span.end = endPos;
        return refactorContext;
    }
};

TEST_F(LspConvertArrowBracesTests, AddBraces_SimpleLiteral)
{
    const std::string code = R"(const getValue = () => 42;)";
    size_t const startPos = 17;
    size_t const endPos = 23;

    Initializer initializer;
    auto *refactorContext = CreateArrowBracesContext(initializer, "refactor_add_braces1.ets", code, startPos, endPos);
    refactorContext->kind = std::string(ADD_BRACES_ACTION.kind);

    auto result = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(result.empty());
    const bool found =
        std::any_of(result.begin(), result.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == std::string(ADD_BRACES_ACTION.name);
        });
    ASSERT_TRUE(found);

    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::CONVERT_ARROW_BRACES_REFACTOR_NAME;
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName),
                                                              std::string(ADD_BRACES_ACTION.name));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "{ return 42; }";
    EXPECT_EQ(newText, expect);

    initializer.DestroyContext(refactorContext->context);
}

TEST_F(LspConvertArrowBracesTests, AddBraces_BinaryExpression)
{
    const std::string code = R"(const add = (a: number, b: number) => a + b;)";
    size_t const startPos = 35;
    size_t const endPos = 39;

    Initializer initializer;
    auto *refactorContext = CreateArrowBracesContext(initializer, "refactor_add_braces2.ets", code, startPos, endPos);
    refactorContext->kind = std::string(ADD_BRACES_ACTION.kind);

    auto result = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(result.empty());
    const bool found =
        std::any_of(result.begin(), result.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == std::string(ADD_BRACES_ACTION.name);
        });
    ASSERT_TRUE(found);

    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::CONVERT_ARROW_BRACES_REFACTOR_NAME;
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName),
                                                              std::string(ADD_BRACES_ACTION.name));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "{ return ((a) + (b)); }";
    EXPECT_EQ(newText, expect);

    initializer.DestroyContext(refactorContext->context);
}

TEST_F(LspConvertArrowBracesTests, RemoveBraces_SimpleLiteral)
{
    const std::string code = R"(const getValue = () => {
    return 42;
};)";
    size_t const startPos = 17;
    size_t const endPos = 23;

    Initializer initializer;
    auto *refactorContext =
        CreateArrowBracesContext(initializer, "refactor_remove_braces1.ets", code, startPos, endPos);
    refactorContext->kind = std::string(REMOVE_BRACES_ACTION.kind);

    auto result = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(result.empty());
    const bool found =
        std::any_of(result.begin(), result.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == std::string(REMOVE_BRACES_ACTION.name);
        });
    ASSERT_TRUE(found);

    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::CONVERT_ARROW_BRACES_REFACTOR_NAME;
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName),
                                                              std::string(REMOVE_BRACES_ACTION.name));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "42";
    EXPECT_EQ(newText, expect);

    initializer.DestroyContext(refactorContext->context);
}

TEST_F(LspConvertArrowBracesTests, RemoveBraces_BinaryExpression)
{
    const std::string code = R"(const add = (a: number, b: number) => {
    return a + b;
};)";
    size_t const startPos = 35;
    size_t const endPos = 39;

    Initializer initializer;
    auto *refactorContext =
        CreateArrowBracesContext(initializer, "refactor_remove_braces2.ets", code, startPos, endPos);
    refactorContext->kind = std::string(REMOVE_BRACES_ACTION.kind);

    auto result = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(result.empty());
    const bool found =
        std::any_of(result.begin(), result.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == std::string(REMOVE_BRACES_ACTION.name);
        });
    ASSERT_TRUE(found);

    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::CONVERT_ARROW_BRACES_REFACTOR_NAME;
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName),
                                                              std::string(REMOVE_BRACES_ACTION.name));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "((a) + (b))";
    EXPECT_EQ(newText, expect);

    initializer.DestroyContext(refactorContext->context);
}

TEST_F(LspConvertArrowBracesTests, RemoveBraces_EmptyReturnToUndefined)
{
    const std::string code = R"(const noop = () => {
    return;
};)";
    size_t const startPos = 13;
    size_t const endPos = 19;

    Initializer initializer;
    auto *refactorContext =
        CreateArrowBracesContext(initializer, "refactor_remove_braces4.ets", code, startPos, endPos);
    refactorContext->kind = std::string(REMOVE_BRACES_ACTION.kind);

    auto result = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(result.empty());
    const bool found =
        std::any_of(result.begin(), result.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == std::string(REMOVE_BRACES_ACTION.name);
        });
    ASSERT_TRUE(found);

    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::CONVERT_ARROW_BRACES_REFACTOR_NAME;
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName),
                                                              std::string(REMOVE_BRACES_ACTION.name));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "undefined";
    EXPECT_EQ(newText, expect);

    initializer.DestroyContext(refactorContext->context);
}

TEST_F(LspConvertArrowBracesTests, NotApplicable_MultipleStatements)
{
    const std::string code = R"(const compute = (x: number) => {
    const temp = x * 2;
    return temp + 1;
};)";
    size_t const startPos = 28;
    size_t const endPos = 34;

    Initializer initializer;
    auto *refactorContext =
        CreateArrowBracesContext(initializer, "refactor_not_applicable.ets", code, startPos, endPos);
    refactorContext->kind = std::string(REMOVE_BRACES_ACTION.kind);

    auto result = GetApplicableRefactorsImpl(refactorContext);
    initializer.DestroyContext(refactorContext->context);

    const bool found =
        std::any_of(result.begin(), result.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == std::string(REMOVE_BRACES_ACTION.name);
        });
    ASSERT_FALSE(found);
}

}  // namespace
