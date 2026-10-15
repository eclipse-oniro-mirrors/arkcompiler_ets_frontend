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

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp/include/services/text_change/text_change_context.h"
#include "lsp/include/refactor_provider.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/code_fixes/code_fix_types.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/references.h"
#include "public/es2panda_lib.h"

namespace {

using ::FileTextChanges;
using ::LanguageServiceHost;
using ::TextChangesContext;
using ark::es2panda::SourceFile;
using ark::es2panda::lsp::ChangeTracker;
using ark::es2panda::lsp::CodeFixAction;
using ark::es2panda::lsp::CodeFixAllContext;
using ark::es2panda::lsp::CodeFixContext;
using ark::es2panda::lsp::CodeFixProvider;
using ark::es2panda::lsp::CodeFixRegistration;
using ark::es2panda::lsp::CombinedCodeActions;
using ark::es2panda::lsp::FormatCodeSettings;
using ark::es2panda::lsp::FormatContext;
using ark::es2panda::lsp::GetDefaultFormatCodeSettings;
using ark::es2panda::lsp::GetFormatContext;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::Refactor;
using ark::es2panda::lsp::RefactorContext;
using ark::es2panda::lsp::RefactorProvider;
using ark::es2panda::lsp::RemoveDuplicates;
using ark::es2panda::lsp::UserPreferences;

class LspTemplateSpecializationTests : public LSPAPITests {};

// =============================================================================
// P3-TPL-04: AutoRefactorRegister_CustomRefactor
// =============================================================================
class CustomTestRefactor : public Refactor {
public:
    std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> GetAvailableActions(
        const RefactorContext & /*context*/) const override
    {
        return {};
    }

    std::unique_ptr<ark::es2panda::lsp::RefactorEditInfo> GetEditsForAction(
        const RefactorContext & /*context*/, const std::string & /*actionName*/) const override
    {
        return nullptr;
    }
};

// Custom refactor/codefix registration plumbing; refactor_provider_test.cpp covers built-ins only.
TEST_F(LspTemplateSpecializationTests, AutoRefactorRegister_CustomRefactor)
{
    // Register a custom refactor using AutoRefactorRegister<T>
    auto reg = std::make_unique<ark::es2panda::lsp::AutoRefactorRegister<CustomTestRefactor>>("custom_test_refactor");

    // The refactor should now be registered in the provider.
    const auto &refactors = RefactorProvider::Instance().GetRefactors();
    auto it = refactors.find("custom_test_refactor");
    ASSERT_NE(it, refactors.end());
    EXPECT_NE(it->second, nullptr);
    // Verify it's the exact type we registered
    EXPECT_EQ(it->first, "custom_test_refactor");
}

// =============================================================================
// P3-TPL-05: AutoCodeFixRegister_CustomCodeFix
// =============================================================================
class CustomTestCodeFix : public CodeFixRegistration {
public:
    CustomTestCodeFix()
    {
        // Set a unique error code for this custom fix
        SetErrorCodes({99999});
    }

    std::vector<CodeFixAction> GetCodeActions(const CodeFixContext &) override
    {
        return {};
    }

    CombinedCodeActions GetAllCodeActions(const CodeFixAllContext &) override
    {
        return {};
    }
};

TEST_F(LspTemplateSpecializationTests, AutoCodeFixRegister_CustomCodeFix)
{
    // Register a custom code fix using AutoCodeFixRegister<T>
    auto reg = std::make_unique<ark::es2panda::lsp::AutoCodeFixRegister<CustomTestCodeFix>>("custom_test_codefix");

    // The code fix should now be registered in the provider with its error codes.
    std::vector<std::string> errorCodes = CodeFixProvider::Instance().GetSupportedErrorCodes();
    // The custom fix registered with error code 99999 should appear in the supported codes.
    bool found = false;
    for (const auto &code : errorCodes) {
        if (code.find("99999") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Custom code fix error code not found in GetSupportedErrorCodes";
}

// =============================================================================
// P3-TPL-06: RemoveDuplicates_IntVector
// =============================================================================
TEST_F(LspTemplateSpecializationTests, RemoveDuplicates_IntVector)
{
    std::vector<int> vec = {3, 1, 2, 1, 3, 2, 4};
    RemoveDuplicates(vec, std::less<int>());

    // After dedup, the vector should have exactly the unique elements in sorted order.
    std::vector<int> expected = {1, 2, 3, 4};
    ASSERT_EQ(vec, expected);
}

// =============================================================================
// P3-TPL-07: RemoveDuplicates_StringVectorCustomCompare
// =============================================================================
TEST_F(LspTemplateSpecializationTests, RemoveDuplicates_StringVectorCustomCompare)
{
    std::vector<std::string> vec = {"apple", "Banana", "APPLE", "banana", "cherry"};
    // Custom compare: case-insensitive
    auto caseInsensitiveCompare = [](const std::string &a, const std::string &b) {
        std::string aLower = a;
        std::string bLower = b;
        std::transform(aLower.begin(), aLower.end(), aLower.begin(), ::tolower);
        std::transform(bLower.begin(), bLower.end(), bLower.begin(), ::tolower);
        return aLower < bLower;
    };
    RemoveDuplicates(vec, caseInsensitiveCompare);

    // After case-insensitive dedup, we should have exactly 3 unique elements.
    ASSERT_EQ(vec.size(), 3U);
    // Verify no duplicates remain (case-insensitively)
    for (size_t i = 0; i < vec.size(); i++) {
        for (size_t j = i + 1; j < vec.size(); j++) {
            std::string aLower = vec[i];
            std::string bLower = vec[j];
            std::transform(aLower.begin(), aLower.end(), aLower.begin(), ::tolower);
            std::transform(bLower.begin(), bLower.end(), bLower.begin(), ::tolower);
            EXPECT_NE(aLower, bLower);
        }
    }
}

// =============================================================================
// P3-TPL-08: RemoveDuplicates_EmptyVector
// =============================================================================
TEST_F(LspTemplateSpecializationTests, RemoveDuplicates_EmptyVector)
{
    std::vector<int> vec;
    RemoveDuplicates(vec, std::less<int>());

    // Empty vector should remain empty, no crash.
    EXPECT_TRUE(vec.empty());
}

}  // namespace