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
#include <string>
#include <algorithm>
#include "lsp/include/refactors/extract_symbol.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/get_edits_for_refactor.h"
#include "lsp/include/types.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/user_preferences.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "lsp_api_test.h"

namespace {
constexpr int START_POS = 45;
constexpr int START_POS_CONSTANT = 49;
using ark::es2panda::lsp::Initializer;

class LspExtrSymblGetEditsTests : public LSPAPITests {
public:
    ark::es2panda::lsp::RefactorContext *CreateExtractContext(Initializer *initializer, const std::string &code,
                                                              size_t start, size_t end)
    {
        std::vector<std::string> files = {"ExtractSymbolRefactorTest.ets"};
        std::vector<std::string> texts = {code};
        auto filePaths = CreateTempFile(files, texts);
        auto ctx = initializer->CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

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
// TEST 1: GetEditsForRefactorsImpl - Extract Constant 1
// -----------------------------------------------------------------------------
TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaPublicAPI1)
{
    const std::string code = R"(
    const kkmm = 1 + 1;

    const kks = kkmm + 1;
)";

    const size_t spanStart = 18;
    const size_t spanEnd = 23;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);
    const auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(refactorContext->context);
    ASSERT_NE(ctx, nullptr);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(),
                    [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) { return info.action.name == target; });

    EXPECT_TRUE(found);
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const EXTRACTED_VAL = 1 + 1;";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;
    EXPECT_EQ(startPos1, 0);
    EXPECT_EQ(newText, expect);

    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// TEST 1: GetEditsForRefactorsImpl - Extract Constant
// -----------------------------------------------------------------------------
TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaPublicAPI)
{
    const std::string code = R"(function main() {
    let x = 10;
    let y = 20;
    console.log("x + y = " + (x + y));
})";

    const size_t spanStart = 67;
    const size_t spanEnd = 75;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
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

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const EXTRACTED_VAL = \"x + y = \";";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;
    EXPECT_EQ(startPos1, START_POS_CONSTANT);
    EXPECT_EQ(newText, expect);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaGlobalPublicAPI)
{
    const std::string code = R"(
    import {{ something }} from 'somewhere';
    const a = 42;
    function main() {
    let x = 10;
    let y = 20;
    console.log("x + y = " + (x + y));
})";

    const size_t spanStart = 134;
    const size_t spanEnd = 144;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);
    const auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(refactorContext->context);
    ASSERT_NE(ctx, nullptr);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
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

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const EXTRACTED_VAL = \"x + y = \";";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;

    EXPECT_EQ(startPos1, START_POS);
    EXPECT_EQ(newText, expect);

    initializer->DestroyContext(refactorContext->context);
}
// -----------------------------------------------------------------------------
// TEST 2: GetEditsForRefactorsImpl - Extract Function
// -----------------------------------------------------------------------------
TEST_F(LspExtrSymblGetEditsTests, ExtractFunctionViaPublicAPI)
{
    const std::string code = R"(
    const kkmm = 1 + 1;

    const kks = kkmm + 1;
})";

    const size_t spanStart = 42;
    const size_t spanEnd = 50;

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());
    const std::string_view target =
        ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name;  // extract_function_scope_0"
    const std::string_view refactorName =
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME;  // ExtractSymbolRefactor

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

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "function extractedFunction1() {\n    return kkmm + 1;\n}\n\n";
    EXPECT_EQ(newText, expect);

    initializer->DestroyContext(refactorContext->context);
}

}  // namespace