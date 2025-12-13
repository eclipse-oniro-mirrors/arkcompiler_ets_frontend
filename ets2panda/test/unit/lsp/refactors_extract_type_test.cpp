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
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "lsp/include/get_edits_for_refactor.h"
#include "lsp/include/refactors/extract_type.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/services/text_change/text_change_context.h"
#include "lsp/include/types.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/user_preferences.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "lsp_api_test.h"

namespace {
using ark::es2panda::lsp::Initializer;

class LspExtrTypeGetEditsTests : public LSPAPITests {
public:
    ark::es2panda::lsp::RefactorContext *CreateExtractContext(Initializer *initializer, const std::string &code,
                                                              size_t start, size_t end)
    {
        std::vector<std::string> files = {"ExtractTypeRefactorTest.ets"};
        std::vector<std::string> texts = {code};
        auto filePaths = CreateTempFile(files, texts);
        auto ctx = initializer->CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

        ark::es2panda::lsp::UserPreferences prefs = ark::es2panda::lsp::UserPreferences::GetDefaultUserPreferences();
        ark::es2panda::lsp::FormatCodeSettings settings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
        ark::es2panda::lsp::FormatContext fmt = ark::es2panda::lsp::GetFormatContext(settings);
        LanguageServiceHost host;
        auto *textChangesContext = new TextChangesContext {host, fmt, prefs};

        auto *refactorContext = new ark::es2panda::lsp::RefactorContext;
        refactorContext->context = ctx;
        refactorContext->textChangesContext = textChangesContext;
        refactorContext->span.pos = start;
        refactorContext->span.end = end;
        return refactorContext;
    }
};

// -----------------------------------------------------------------------------
// CASE 1: Object parameter extraction
TEST_F(LspExtrTypeGetEditsTests, ExtractInterfaceViaPublicAPI)
{
    const std::string code = R"(function test() {
    let obj: { a: number; b: string } = { a: 10, b: "hi" };
})";

    const size_t spanStart = 31;
    const size_t spanEnd = 55;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "interface ExtractedInterface { a: number; b: string }\n\n";
    EXPECT_EQ(newText, expect);

    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// CASE 2: Array alias extraction
TEST_F(LspExtrTypeGetEditsTests, ExtractTypeAliasViaPublicAPI)
{
    const std::string code = R"(const value: Array<string> = ["x"];)";
    const size_t spanStart = 13;
    const size_t spanEnd = 26;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_TYPE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);

    const auto &insertChange = fileEdit.textChanges.at(0);
    EXPECT_EQ(insertChange.span.length, 0U);
    EXPECT_EQ(insertChange.newText, "type ExtractedType = Array<string>;\n\n");

    const auto &replaceChange = fileEdit.textChanges.at(1);
    EXPECT_GT(replaceChange.span.length, 0U);
    EXPECT_EQ(replaceChange.newText, "ExtractedType");

    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// CASE 3: Inline object -> interface
TEST_F(LspExtrTypeGetEditsTests, ExtractInterfaceForInlineObjectVariable)
{
    const std::string code = R"(let a: { n: number; s: string } = { n: 1, s: "value" };)";
    const size_t spanStart = 7;
    const size_t spanEnd = 31;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);

    const auto &insertChange = fileEdit.textChanges.at(0);
    EXPECT_EQ(insertChange.span.length, 0U);
    EXPECT_EQ(insertChange.newText, "interface ExtractedInterface { n: number; s: string }\n\n");

    const auto &replaceChange = fileEdit.textChanges.at(1);
    EXPECT_GT(replaceChange.span.length, 0U);
    EXPECT_EQ(replaceChange.newText, "ExtractedInterface");

    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// CASE 4: Function parameter interface
TEST_F(LspExtrTypeGetEditsTests, ExtractInterfaceForObjectParameter)
{
    const std::string code = R"(function f(p: { x: number; y: number }): void { console.log(p.x + p.y); })";
    const size_t spanStart = 14;
    const size_t spanEnd = 38;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);

    const auto &insertChange = fileEdit.textChanges.at(0);
    EXPECT_EQ(insertChange.span.length, 0U);
    EXPECT_EQ(insertChange.newText, "interface ExtractedInterface { x: number; y: number }\n\n");

    const auto &replaceChange = fileEdit.textChanges.at(1);
    EXPECT_GT(replaceChange.span.length, 0U);
    EXPECT_EQ(replaceChange.newText, "ExtractedInterface");

    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// CASE 5: Promise return alias
TEST_F(LspExtrTypeGetEditsTests, ExtractTypeForPromiseReturn)
{
    const std::string code = R"(function g(): Promise<{ ok: boolean }> { return Promise.resolve({ ok: true }); })";
    const size_t spanStart = 14;
    const size_t spanEnd = 38;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_TYPE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);

    const auto &insertChange = fileEdit.textChanges.at(0);
    EXPECT_EQ(insertChange.span.length, 0U);
    EXPECT_EQ(insertChange.newText, "type ExtractedType = Promise<{ ok: boolean }>;\n\n");

    const auto &replaceChange = fileEdit.textChanges.at(1);
    EXPECT_GT(replaceChange.span.length, 0U);
    EXPECT_EQ(replaceChange.newText, "ExtractedType");

    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// CASE 6: Class property interface
TEST_F(LspExtrTypeGetEditsTests, ExtractInterfaceForClassProperty)
{
    const std::string code = R"(class C { info: { id: number; active: boolean } = { id: 1, active: true }; })";
    const size_t spanStart = 16;
    const size_t spanEnd = 47;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);

    const auto &insertChange = fileEdit.textChanges.at(0);
    EXPECT_EQ(insertChange.span.length, 0U);
    EXPECT_EQ(insertChange.newText, "interface ExtractedInterface { id: number; active: boolean }\n\n");

    const auto &replaceChange = fileEdit.textChanges.at(1);
    EXPECT_GT(replaceChange.span.length, 0U);
    EXPECT_EQ(replaceChange.newText, "ExtractedInterface");

    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// CASE 7: Union alias extraction
TEST_F(LspExtrTypeGetEditsTests, ExtractTypeForUnionType)
{
    const std::string code = R"(type Combo = number | string | boolean;)";
    const size_t spanStart = 13;
    const size_t spanEnd = 38;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_TYPE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);

    const auto &insertChange = fileEdit.textChanges.at(0);
    EXPECT_EQ(insertChange.span.length, 0U);
    EXPECT_EQ(insertChange.newText, "type ExtractedType = number | string | boolean;\n\n");

    const auto &replaceChange = fileEdit.textChanges.at(1);
    EXPECT_GT(replaceChange.span.length, 0U);
    EXPECT_EQ(replaceChange.newText, "ExtractedType");

    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// CASE 8: Example variable interface
TEST_F(LspExtrTypeGetEditsTests, ExtractInterfaceForExampleVariable)
{
    const std::string code = R"(let example: { v: number; active: boolean } = { v: 1, active: true };)";
    const size_t spanStart = 13;
    const size_t spanEnd = 43;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);

    const auto &insertChange = fileEdit.textChanges.at(0);
    EXPECT_EQ(insertChange.span.length, 0U);
    EXPECT_EQ(insertChange.newText, "interface ExtractedInterface { v: number; active: boolean }\n\n");

    const auto &replaceChange = fileEdit.textChanges.at(1);
    EXPECT_GT(replaceChange.span.length, 0U);
    EXPECT_EQ(replaceChange.newText, "ExtractedInterface");

    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// CASE 9: Function type alias
TEST_F(LspExtrTypeGetEditsTests, ExtractTypeForFunctionVariable)
{
    const std::string code = R"(const handler: (value: string) => number = (value) => value.length;)";
    const size_t spanStart = 15;
    const size_t spanEnd = 40;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_TYPE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);

    const auto &insertChange = fileEdit.textChanges.at(0);
    EXPECT_EQ(insertChange.span.length, 0U);
    EXPECT_EQ(insertChange.newText, "type ExtractedType = (value: string) => number;\n\n");

    const auto &replaceChange = fileEdit.textChanges.at(1);
    EXPECT_GT(replaceChange.span.length, 0U);
    EXPECT_EQ(replaceChange.newText, "ExtractedType");

    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// CASE 10: Type assertion interface
TEST_F(LspExtrTypeGetEditsTests, ExtractInterfaceForTypeAssertionObject)
{
    const std::string code = R"(const typed = (value as { n: number; s: string }).s.length;)";
    const size_t spanStart = 24;
    const size_t spanEnd = 48;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);
    refactorContext->kind = "refactor.extract.interface";
    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);

    const auto &insertChange = fileEdit.textChanges.at(0);
    EXPECT_EQ(insertChange.span.length, 0U);
    EXPECT_EQ(insertChange.newText, "interface ExtractedInterface { n: number; s: string }\n\n");

    const auto &replaceChange = fileEdit.textChanges.at(1);
    EXPECT_GT(replaceChange.span.length, 0U);
    EXPECT_EQ(replaceChange.newText, "ExtractedInterface");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrTypeGetEditsTests, ExtractInterfaceForClassMethodReturn)
{
    const std::string code = R"('use static'
class Circle {
    radius: number;
    constructor(radius: number) {
        this.radius = radius;
    }
    
    getBoundingBox(): { width: number; height: number } {
        return {
            width: this.radius * 2,
            height: this.radius * 2
        }
    }
})";
    const size_t spanStart = 145;
    const size_t spanEnd = 178;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);

    const auto &insertChange = fileEdit.textChanges.at(0);
    EXPECT_EQ(insertChange.span.length, 0U);
    EXPECT_EQ(insertChange.newText, "interface ExtractedInterface { width: number; height: number }\n\n");

    const auto &replaceChange = fileEdit.textChanges.at(1);
    EXPECT_GT(replaceChange.span.length, 0U);
    EXPECT_EQ(replaceChange.newText, "ExtractedInterface");

    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// CASE 11: First array alias
TEST_F(LspExtrTypeGetEditsTests, ExtractTypeForFirstArrayVariable)
{
    const std::string code = R"(const first: Array<string> = [];)";
    const size_t spanStart = 13;
    const size_t spanEnd = 26;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_TYPE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);

    const auto &insertChange = fileEdit.textChanges.at(0);
    EXPECT_EQ(insertChange.span.length, 0U);
    EXPECT_EQ(insertChange.newText, "type ExtractedType = Array<string>;\n\n");

    const auto &replaceChange = fileEdit.textChanges.at(1);
    EXPECT_GT(replaceChange.span.length, 0U);
    EXPECT_EQ(replaceChange.newText, "ExtractedType");

    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// CASE 12: Sequential array alias
TEST_F(LspExtrTypeGetEditsTests, ExtractTypeGeneratesUniqueNameForSecondArrayVariable)
{
    const std::string code = R"(type ExtractedType = Array<string>;
const second: Array<number> = [];)";
    const size_t spanStart = 50;
    const size_t spanEnd = 63;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    ASSERT_FALSE(applicable.empty());

    const std::string_view target = ark::es2panda::lsp::EXTRACT_TYPE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });
    ASSERT_TRUE(found);

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);

    const auto &insertChange = fileEdit.textChanges.at(0);
    EXPECT_EQ(insertChange.span.length, 0U);
    EXPECT_EQ(insertChange.newText, "type ExtractedType1 = Array<number>;\n\n");

    const auto &replaceChange = fileEdit.textChanges.at(1);
    EXPECT_GT(replaceChange.span.length, 0U);
    EXPECT_EQ(replaceChange.newText, "ExtractedType1");

    initializer->DestroyContext(refactorContext->context);
}

}  // namespace
