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
#include <iostream>
#include <ostream>
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

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const newLocal = 1 + 1;";
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

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const newLocal = \"x + y = \";";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedInsertPos = 49;
    EXPECT_EQ(startPos1, expectedInsertPos);
    EXPECT_EQ(newText, expect);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaGlobalPublicAPI)
{
    const std::string code = R"(
    import hilog from '@ohos.hilog'
    const a = 42;
    function main() {
    let x = 10;
    let y = 20;
    console.log("x + y = " + (x + y));
})";

    const size_t spanStart = 134;
    const size_t spanEnd = 144;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const newLocal = \"x + y = \";";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedInsertPos = 0;
    EXPECT_EQ(startPos1, expectedInsertPos);
    EXPECT_EQ(newText, expect);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableFromMethodEncloseScope)
{
    const std::string code = R"('use static'

class AccountingDepartment {
    name: string = '';

    printName(): void {
        console.log('Department name:' + this.name);
    }
}
)";
    const std::string expected = R"('use static'

class AccountingDepartment {
    name: string = '';

    printName(): void {
        let newLocal = 'Department name:';

        console.log(newLocal + this.name);
    }
}
)";
    const std::string target = "'Department name:'";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    refactorContext->kind = "refactor.extract.variable";

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const bool hasVariableEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto stripWs = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    };
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractFunctionViaPublicAPI)
{
    const std::string code = R"(
    const kkmm = 1 + 1;

    const kks = kkmm + 1;
})";

    const size_t spanStart = 42;
    const size_t spanEnd = 50;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target =
        ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name;  // extract_function_scope_0"
    const std::string_view refactorName =
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME;  // ExtractSymbolRefactor
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "function extractedFunction1() {\n    return kkmm + 1;\n}\n\n";
    EXPECT_EQ(newText, expect);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodGlobal)
{
    const std::string code = R"('use static'

function newFunction(a: number, b: number) {
    let c = a + b;
    return c;
}

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
    let c = a + b;
    return c;
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

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodClass)
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
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string classScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const bool hasClass = std::any_of(applicable.begin(), applicable.end(),
                                      [&](const auto &info) { return info.action.name == classScopeAction; });
    EXPECT_TRUE(hasClass);

    ExpectExtractionApplies(code, refactorContext, refactorName, classScopeAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

}  // namespace
