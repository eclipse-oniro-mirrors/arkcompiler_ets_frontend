/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <memory>
#include "lsp/include/refactors/extract_symbol.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/get_edits_for_refactor.h"
#include "lsp/include/types.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/user_preferences.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "lsp_api_test.h"
#include "public/public.h"

namespace {
using ark::es2panda::lsp::Initializer;
std::string ApplyEdits(const std::string &original, const std::vector<::TextChange> &edits)
{
    if (edits.empty()) {
        return original;
    }

    std::vector<const ::TextChange *> ordered;
    ordered.reserve(edits.size());
    for (const auto &change : edits) {
        ordered.push_back(&change);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const ::TextChange *lhs, const ::TextChange *rhs) { return lhs->span.start < rhs->span.start; });

    std::string result;
    result.reserve(original.size());
    size_t cursor = 0;
    for (const auto *change : ordered) {
        size_t start = std::min(change->span.start, original.size());
        if (start < cursor) {
            start = cursor;
        }
        size_t end = std::min(start + change->span.length, static_cast<size_t>(original.size()));
        if (cursor < start) {
            result.append(original, cursor, start - cursor);
        }
        result.append(change->newText);
        cursor = end;
    }

    if (cursor < original.size()) {
        result.append(original, cursor, original.size() - cursor);
    }
    return result;
}

class LspExtrSymblGetEditsTestsFunction : public LSPAPITests {
public:
    ark::es2panda::lsp::RefactorContext *CreateExtractContext(Initializer *initializer, const std::string &code,
                                                              size_t start, size_t end)
    {
        std::vector<std::string> files = {"ExtractSymbolRefactorFunctionTest.ets"};
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

void ExpectExtractionApplies(const std::string &source, ark::es2panda::lsp::RefactorContext *refactorContext,
                             const std::string &refactorName, const std::string &actionName,
                             const std::string &expected)
{
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);

    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().front();
    ASSERT_FALSE(fileEdit.textChanges.empty());

    const std::string result = ApplyEdits(source, fileEdit.textChanges);
    EXPECT_EQ(result, expected);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction)
{
    const std::string code = R"('use static'

class MyClass {

    MyMethod(a: number, b: number) {
        let c = a + b;
        let d = c * c;
        return d;
    }
}
)";
    const std::string expected = R"('use static'

function newFunction(a: number, b: number) {
    return a + b;
}

class MyClass {

    MyMethod(a: number, b: number) {

        let c = newFunction(a, b);
        let d = c * c;
        return d;
    }
}
)";

    const std::string target = "let c = a + b;";
    const size_t spanStart = code.find(target, code.find("MyMethod"));
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const bool hasGlobal = std::any_of(applicable.begin(), applicable.end(),
                                       [&](const auto &info) { return info.action.name == globalScopeAction; });
    EXPECT_TRUE(hasGlobal);

    ExpectExtractionApplies(code, refactorContext, refactorName, globalScopeAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction1)
{
    const std::string code = R"('use static'

class MyClass {

    MyMethod(a: number, b: number) {
        let c = a + b;
        let d = c * c;
        return d;
    }
}
)";
    const std::string expected = R"('use static'

class MyClass {

    MyMethod(a: number, b: number) {

        let c = this.newMethod(a, b);
        let d = c * c;
        return d;
    }

    private newMethod(a: number, b: number) {
        let c = a + b;
        return c;
    }
}
)";

    const std::string target = "let c = a + b;";
    const size_t spanStart = code.find(target, code.find("MyMethod"));
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const bool hasGlobal = std::any_of(applicable.begin(), applicable.end(),
                                       [&](const auto &info) { return info.action.name == globalScopeAction; });
    EXPECT_TRUE(hasGlobal);

    ExpectExtractionApplies(code, refactorContext, refactorName, globalScopeAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction2)
{
    const std::string code = R"('use static'

class MyClass {

    MyMethod(a: number, b: number) {
        let c = a + b;
        let d = c * c;
        return d;
    }

    private newMethod(a: number, b: number) {
        let d = a + b;
        return d;
    }
}
)";
    const std::string expected = R"('use static'

class MyClass {

    MyMethod(a: number, b: number) {

        let c = this.newMethod_1(a, b);
        let d = c * c;
        return d;
    }

    private newMethod(a: number, b: number) {
        let d = a + b;
        return d;
    }

    private newMethod_1(a: number, b: number) {
        let c = a + b;
        return c;
    }
}
)";

    const std::string target = "let c = a + b;";
    const size_t spanStart = code.find(target, code.find("MyMethod"));
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const bool hasGlobal = std::any_of(applicable.begin(), applicable.end(),
                                       [&](const auto &info) { return info.action.name == globalScopeAction; });
    EXPECT_TRUE(hasGlobal);

    ExpectExtractionApplies(code, refactorContext, refactorName, globalScopeAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

}  // namespace
