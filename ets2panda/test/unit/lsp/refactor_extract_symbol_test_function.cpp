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
#include <iostream>
#include <iterator>
#include <ostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <memory>
#include <vector>
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
    const std::string expected = R"(
private newMethod(a: number, b: number) {
    let c = a + b;
    return c;
}
)";

    auto initializer = std::make_unique<Initializer>();
    const size_t startPoint = 74;
    const size_t endPoint = 88;
    auto *refactorContext = CreateExtractContext(initializer.get(), code, startPoint, endPoint);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    std::vector<ark::es2panda::lsp::RefactorAction> applicableList;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name ||
                   info.action.name == ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name;
        });
    ASSERT_TRUE(found);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName),
                                                              std::string(globalScopeAction));
    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {28, 0};
    const TextSpan expCallTextSpan {82, 88};
    const std::string expCallText = "this.newMethod(a, b);";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
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
    const std::string expected = R"(
function newFunction(a: number, b: number) {
    let c = a + b;
    return c;
}
)";
    auto initializer = std::make_unique<Initializer>();
    const size_t startPoint = 74;
    const size_t endPoint = 88;
    auto *refactorContext = CreateExtractContext(initializer.get(), code, startPoint, endPoint);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    std::vector<ark::es2panda::lsp::RefactorAction> applicableList;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name ||
                   info.action.name == ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name;
        });
    ASSERT_TRUE(found);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName),
                                                              std::string(globalScopeAction));
    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {12, 0};
    const TextSpan expCallTextSpan {82, 88};
    const std::string expCallText = "newFunction(a, b);";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction2)
{
    const std::string code = R"(class MyClass {
    newMethod(a: number, b: number) {let c = a + b;return c;}})";
    const std::string expected = R"(
private newMethod1(a: number, b: number) {
    let c = a + b;
    return c;
}
)";
    auto initializer = std::make_unique<Initializer>();
    const size_t startPoint = 53;
    const size_t endPoint = 67;
    auto *refactorContext = CreateExtractContext(initializer.get(), code, startPoint, endPoint);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    std::vector<ark::es2panda::lsp::RefactorAction> applicableList;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name ||
                   info.action.name == ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name;
        });
    ASSERT_TRUE(found);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName),
                                                              std::string(globalScopeAction));
    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {15, 0};
    const TextSpan expCallTextSpan {61, 67};
    const std::string expCallText = "this.newMethod1(a, b);";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction3)
{
    const std::string code = R"('use static'
class MyClass {MyMethod(a: number, b: number) {let c = a + b;let d = c * c;return d;}}
function newFunction(a: number, b: number) {let d = a + b;return d;})";
    const std::string expected = R"(
function newFunction1(a: number, b: number) {
    let c = a + b;
    return c;
}
)";
    auto initializer = std::make_unique<Initializer>();
    const size_t startPoint = 60;
    const size_t endPoint = 74;
    auto *refactorContext = CreateExtractContext(initializer.get(), code, startPoint, endPoint);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    std::vector<ark::es2panda::lsp::RefactorAction> applicableList;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name ||
                   info.action.name == ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name;
        });
    ASSERT_TRUE(found);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName),
                                                              std::string(globalScopeAction));
    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {12, 0};
    const TextSpan expCallTextSpan {68, 74};
    const std::string expCallText = "newFunction1(a, b);";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction4)
{
    const std::string code = R"(
class C {
    constructor() {
        this.foo(1);
    }
    foo(a: number) {return 1;}
}
)";
    const std::string expected = R"(
let newLocal = this.foo(1);
)";
    const std::string target = "this.foo(1)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const bool hasVariableEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    ASSERT_NE(edits, nullptr);

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {30, 0};
    const TextSpan expCallTextSpan {39, 12};
    const std::string expCallText = "";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractVariable1)
{
    const std::string code = R"('use static'

class AccountingDepartment {
    name: string = '';

    printName(): void {
        console.log('Department name:' + this.name);
    }
}
)";
    const std::string expected = R"(
let newLocal = 'Department name:';
)";
    const std::string target = "'Department name:'";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const bool hasVariableEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    ASSERT_NE(edits, nullptr);

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {90, 0};
    const TextSpan expCallTextSpan {111, 129};
    const std::string expCallText = "newLocal";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractVariable2)
{
    const std::string code = R"('use static'

class AccountingDepartment {
    name: string = '';

    printName(): void {
        console.log('Department name:' + this.name);
    }
}
)";
    const std::string expected = R"(
const newProperty = 'Department name:';
)";
    const std::string target = "'Department name:'";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    const bool hasVariableEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    ASSERT_NE(edits, nullptr);

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {42, 0};
    const TextSpan expCallTextSpan {111, 129};
    const std::string expCallText = "newProperty";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
    initializer->DestroyContext(refactorContext->context);
}
TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractVariable3)
{
    const std::string code = R"('use static'

class AccountingDepartment {
    name: string = '';

    printName(): void {
        console.log('Department name:' + this.name);
    }
}
)";
    const std::string expected = R"(
const newProperty = 'Department name:';
)";
    const std::string target = "'Department name:'";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    const bool hasVariableEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableEnclose);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    ASSERT_NE(edits, nullptr);
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {42, 0};
    const TextSpan expCallTextSpan {111, 129};
    const std::string expCallText = "newProperty";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractVariable4)
{
    const std::string code = R"('use static'

class AccountingDepartment {
    name: string = '';

    printName(): void {
        console.log('Department name:' + this.name);
    }
}
)";
    const std::string expected = R"(
const newLocal = 'Department name:';
)";
    const std::string target = "'Department name:'";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const bool hasVariableEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    ASSERT_NE(edits, nullptr);

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {13, 0};
    const TextSpan expCallTextSpan {111, 129};
    const std::string expCallText = "newLocal";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractVariable5)
{
    const std::string code = R"('use static'

class AccountingDepartment {
    name: string = '';

    printName(): void {
        console.log('Department name:' + this.name);
    }
    const newProperty = '';
}
)";
    const std::string expected = R"(
const newProperty1 = 'Department name:';
)";
    const std::string target = "'Department name:'";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    const bool hasVariableEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableEnclose);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    ASSERT_NE(edits, nullptr);
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {42, 0};
    const TextSpan expCallTextSpan {111, 129};
    const std::string expCallText = "newProperty1";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractVariable6)
{
    const std::string code = R"('use static'

class AccountingDepartment {
    name: string = '';

    printName(): void {
        console.log('Department name:' + this.name);
        let newLocal = '';
    }
}
)";
    const std::string expected = R"(
let newLocal1 = 'Department name:';
)";
    const std::string target = "'Department name:'";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const bool hasVariableEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    ASSERT_NE(edits, nullptr);

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {90, 0};
    const TextSpan expCallTextSpan {111, 129};
    const std::string expCallText = "newLocal1";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractVariable7)
{
    const std::string code = R"('use static'

class AccountingDepartment {
    name: string = '';

    printName(): void {
        console.log('Department name:' + this.name);
    }
}
const newLocal = '';
)";
    const std::string expected = R"(
const newLocal1 = 'Department name:';
)";
    const std::string target = "'Department name:'";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const bool hasVariableEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    ASSERT_NE(edits, nullptr);

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {13, 0};
    const TextSpan expCallTextSpan {111, 129};
    const std::string expCallText = "newLocal1";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractVariableBinaryExpression)
{
    const std::string code = R"('use static'
km = 1 + 2;
kkmm = km * 3 - 4 / 5;
c = kkmm + km;
)";
    const std::string expected = R"(
const newLocal = 1 + 2;
)";
    const std::string target = "1 + 2";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const bool hasVariableEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    ASSERT_NE(edits, nullptr);

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view callText = fileEdit.textChanges.at(1).newText;
    EXPECT_EQ(newText, expected);
    TextSpan ts = fileEdit.textChanges.at(0).span;
    TextSpan callTextTextSpan = fileEdit.textChanges.at(1).span;
    const TextSpan expTextSpan {0, 0};
    const TextSpan expCallTextSpan {18, 23};
    const std::string expCallText = "newLocal";
    EXPECT_EQ(ts, expTextSpan);
    EXPECT_EQ(callText, expCallText);
    EXPECT_EQ(callTextTextSpan, expCallTextSpan);
    initializer->DestroyContext(refactorContext->context);
}

}  // namespace
