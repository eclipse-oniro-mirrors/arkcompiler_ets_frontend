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
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <memory>
#include "lsp/include/refactors/extract_symbol.h"
#include "lsp/include/refactors/extract_type.h"
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
        std::vector<std::string> files = {"ExtractSymbolRefactorFunctionTest2.ets"};
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

template <class T>
static bool HasAction(const std::vector<T> &applicable, const std::string &name)
{
    return std::any_of(applicable.begin(), applicable.end(),
                       [&](const auto &info) { return info.action.name == name; });
}

static std::string StripWs(std::string s)
{
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
    return s;
}

static std::string FindConstantNamespaceActionName(
    const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable, const std::string &encloseScopeAction,
    const std::string &namespaceDesc)
{
    for (const auto &info : applicable) {
        if ((info.action.name == encloseScopeAction ||
             info.action.name.rfind(std::string("extract_constant_scope_ns_"), 0) == 0) &&
            info.action.description.find(namespaceDesc) != std::string::npos) {
            return info.action.name;
        }
    }
    return "";
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol1)
{
    const std::string code = R"(
let name: string = "World";
let message = /*start*/"Hello, " + name + "!"/*end*/;
)";
    const std::string expected = R"(
let name: string = "World";
let newLocal: String = "Hello, " + name + "!";
let message = /*start*/newLocal/*end*/;
)";
    const std::string target = R"("Hello, " + name + "!")";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    const bool hasVariableGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableGlobal);

    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const bool hasVariableEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_FALSE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto stripWs = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    };
    const std::string appliedAction = hasVariableGlobal ? actionName : encloseActionName;
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, appliedAction);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol2)
{
    const std::string code = R"(
let name: string = "World";
let message = /*start*/"Hello, " + name + "!"/*end*/;
)";
    const std::string expected = R"(
let newLocal: string = "World";
let name: string = newLocal;
let message = /*start*/"Hello, " + name + "!"/*end*/;
)";
    const std::string target = R"("World")";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    const bool hasVariableGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_FALSE(hasVariableGlobal);

    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const bool hasVariableEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto stripWs = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    };
    const std::string appliedAction = hasVariableGlobal ? actionName : encloseActionName;
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, appliedAction);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol3)
{
    const std::string code = R"(
let a: number = 5;
let b: number = 50;
if (/*start*/a > 0 && b < 100/*end*/) {
}
)";
    const std::string expected = R"(
let a: number = 5;
let b: number = 50;
let newLocal: Boolean = a > 0 && b < 100;
if (/*start*/newLocal/*end*/) {
}
)";
    const std::string target = R"(a > 0 && b < 100)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    const bool hasVariableGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_FALSE(hasVariableGlobal);

    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const bool hasVariableEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    const std::string appliedAction = hasVariableGlobal ? actionName : encloseActionName;
    auto stripWs = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    };
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, appliedAction);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol4)
{
    const std::string code = R"(
let isActive: boolean = true;
let label = /*start*/isActive ? "Active" : "Inactive"/*end*/;
)";
    const std::string expected = R"(
let isActive: boolean = true;
let newLocal = isActive ? "Active" : "Inactive";
let label = /*start*/newLocal/*end*/;
)";
    const std::string target = R"(isActive ? "Active" : "Inactive")";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    const bool hasVariableGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableGlobal);

    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const bool hasVariableEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_FALSE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    const std::string appliedAction = hasVariableGlobal ? actionName : encloseActionName;
    auto stripWs = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    };
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, appliedAction);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol5)
{
    const std::string code = R"(
class User {
  isValid: boolean = true;
}
let user: User | null = new User();
if (/*start*/user != null && user.isValid/*end*/) {
}
)";
    const std::string expected = R"(
class User {
  isValid: boolean = true;
}
let user: User | null = new User();
let newLocal: Boolean = user != null && user.isValid;
if (/*start*/newLocal/*end*/) {
}
)";
    const std::string target = R"(user != null && user.isValid)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    const bool hasVariableGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_FALSE(hasVariableGlobal);

    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const bool hasVariableEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    const std::string appliedAction = hasVariableGlobal ? actionName : encloseActionName;
    auto stripWs = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    };
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, appliedAction);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol6)
{
    const std::string code = R"(
let handler = /*start*/(x: int): int => x * 2/*end*/;
)";
    const std::string expected = R"(
let newLocal = (x: int): int => x * 2;
let handler = /*start*/newLocal/*end*/;
)";
    const std::string target = R"((x: int): int => x * 2)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    const bool hasVariableGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableGlobal);

    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const bool hasVariableEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_FALSE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    const std::string appliedAction = hasVariableGlobal ? actionName : encloseActionName;
    auto stripWs = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    };
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, appliedAction);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol7)
{
    const std::string code = R"(
let handler = /*start*/(x: int): int => x * 2/*end*/;
)";
    const std::string expected = R"(
const newLocal = (x: int): int => x * 2;
let handler = /*start*/newLocal/*end*/;
)";
    const std::string target = R"((x: int): int => x * 2)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const bool hasConstantGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasConstantGlobal);

    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_FALSE(hasConstantEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    const std::string appliedAction = hasConstantGlobal ? actionName : encloseActionName;
    auto stripWs = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    };
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, appliedAction);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol8)
{
    const std::string code = R"(
interface NestedB {
  c: int; d: int;
}
interface NestedA {
  a: int; b: NestedB;
}
let result: NestedA = /*start*/{ a: 1, b: { c: 2, d: 3 } }/*end*/;
)";
    const std::string expected = R"(
interface NestedB {
  c: int; d: int;
}
interface NestedA {
  a: int; b: NestedB;
}
const newLocal: NestedA = { a: 1, b: { c: 2, d: 3 } };
let result: NestedA = /*start*/newLocal/*end*/;
)";
    const std::string target = R"({ a: 1, b: { c: 2, d: 3 } })";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_TRUE(hasConstantEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto stripWs = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    };
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, encloseActionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol9)
{
    const std::string code = R"(
class Person {
  age: int = 20;
  isActive: boolean = true;
}
let person: Person = new Person();
if (/*start*/person.age >= 18 && person.isActive/*end*/) {
  console.log("test 1", /*start*/person.age >= 18 && person.isActive/*end*/);
}
)";
    const std::string expected = R"(
class Person {
  age: int = 20;
  isActive: boolean = true;
}
let person: Person = new Person();
let newLocal: Boolean = person.age >= 18 && person.isActive;
if (/*start*/newLocal/*end*/) {
  console.log("test 1", /*start*/person.age >= 18 && person.isActive/*end*/);
}
)";
    const std::string target = R"(person.age >= 18 && person.isActive)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const bool hasVariableEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto stripWs = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    };
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, encloseActionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol10)
{
    const std::string code = R"(
class Person {
  age: int = 20;
  isActive: boolean = true;
}
let person: Person = new Person();
if (/*start*/person.age >= 18 && person.isActive/*end*/) {
  console.log("test 1", /*start*/person.age >= 18 && person.isActive/*end*/);
}
)";
    const std::string expected = R"(
class Person {
  age: int = 20;
  isActive: boolean = true;
}
let person: Person = new Person();
const newLocal: Boolean = person.age >= 18 && person.isActive;
if (/*start*/newLocal/*end*/) {
  console.log("test 1", /*start*/person.age >= 18 && person.isActive/*end*/);
}
)";
    const std::string target = R"(person.age >= 18 && person.isActive)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const bool hasConstantGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasConstantGlobal);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
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

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol11)
{
    const std::string code = R"(
let handler = /*start*/(x: int): int => x * 2/*end*/;
)";
    const std::string expected = R"(
function newFunction() {
  return (x: int): int => x * 2;
}

let handler = /*start*/newFunction()/*end*/;
)";
    const std::string target = R"((x: int): int => x * 2)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = "extract_function_scope_2";
    ASSERT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol12)
{
    const std::string code = R"(
let items: int[] = [1, 2, 3, 4, 5];
let first = /*start*/items.filter(x => x > 0)/*end*/.length;
let second = /*start*/items.filter(x => x > 0)/*end*/.toString();
)";
    const std::string expected = R"(
let items: int[] = [1, 2, 3, 4, 5];
const newLocal = items.filter(x => x > 0);
let first = /*start*/newLocal/*end*/.length;
let second = /*start*/items.filter(x => x > 0)/*end*/.toString();
)";
    const std::string target = R"(items.filter(x => x > 0))";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string globalName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string encloseName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    EXPECT_TRUE(HasAction(applicable, globalName));
    EXPECT_FALSE(HasAction(applicable, encloseName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, globalName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol13)
{
    const std::string code = R"(
class Person {
  age: int = 20;
  isActive: boolean = true;
}
let person: Person = new Person();
if (/*start*/person.age == 18 && person.isActive/*end*/) {
  console.log("test 1", /*start*/person.age >= 18 && person.isActive/*end*/);
}
)";
    const std::string expected = R"(
class Person {
  age: int = 20;
  isActive: boolean = true;
}
let person: Person = new Person();
if (/*start*/person.age == 18 && person.isActive/*end*/) {
  let newLocal: Boolean = person.age >= 18 && person.isActive;
  console.log("test 1", /*start*/newLocal/*end*/);
}
)";
    const std::string target = R"(person.age >= 18 && person.isActive)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string globalName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    const std::string encloseName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    EXPECT_FALSE(HasAction(applicable, globalName));
    EXPECT_TRUE(HasAction(applicable, encloseName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, encloseName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol14)
{
    const std::string code = R"(
class Person {
  age: int = 20;
  isActive: boolean = true;
}
let person: Person = new Person();
if (/*start*/person.age == 18 && person.isActive/*end*/) {
  console.log("test 1", /*start*/person.age >= 18 && person.isActive/*end*/);
}
)";
    const std::string expected = R"(
class Person {
  age: int = 20;
  isActive: boolean = true;
}
let person: Person = new Person();
const newLocal: Boolean = person.age == 18 && person.isActive;
if (/*start*/newLocal/*end*/) {
  console.log("test 1", /*start*/person.age >= 18 && person.isActive/*end*/);
}
)";
    const std::string target = R"(person.age == 18 && person.isActive)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string globalName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string encloseName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    EXPECT_TRUE(HasAction(applicable, globalName));
    EXPECT_FALSE(HasAction(applicable, encloseName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, globalName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol15)
{
    const std::string code = R"(
class Person {
  age: int = 20;
  isActive: boolean = true;
}
let person: Person = new Person();
if (/*start*/person.age == 18 && person.isActive/*end*/) {
  console.log("test 1", /*start*/person.age >= 18 && person.isActive/*end*/);
}
)";
    const std::string expected = R"(
class Person {
  age: int = 20;
  isActive: boolean = true;
}
let person: Person = new Person();
const newLocal = person.age >= 18 && person.isActive;
if (/*start*/person.age == 18 && person.isActive/*end*/) {
  console.log("test 1", /*start*/newLocal/*end*/);
}
)";
    const std::string target = R"(person.age >= 18 && person.isActive)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string globalName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, globalName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, globalName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol16)
{
    const std::string code = R"(
class Person {
  age: int = 20;
  isActive: boolean = true;
}
let person: Person = new Person();
if (/*start*/person.age == 18 && person.isActive/*end*/) {
  console.log("test 1", /*start*/person.age >= 18 && person.isActive/*end*/);
}
)";
    const std::string expected = R"(
class Person {
  age: int = 20;
  isActive: boolean = true;
}
let person: Person = new Person();
if (/*start*/person.age == 18 && person.isActive/*end*/) {
  const newLocal: Boolean = person.age >= 18 && person.isActive;
  console.log("test 1", /*start*/newLocal/*end*/);
}
)";
    const std::string target = R"(person.age >= 18 && person.isActive)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string encloseName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    EXPECT_TRUE(HasAction(applicable, encloseName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, encloseName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol17)
{
    const std::string code = R"(
function calculate(): int {
  return 42;
}
let value: int = /*start*/calculate()/*end*/;
)";
    const std::string expected = R"(
function calculate(): int {
  return 42;
}
const newLocal: int = calculate();
let value: int = /*start*/newLocal/*end*/;
)";
    const std::string target = R"(calculate())";
    const std::string lineAnchor = R"(let value: int = )";
    const size_t linePos = code.find(lineAnchor);
    ASSERT_NE(linePos, std::string::npos);
    const size_t spanStart = code.find(target, linePos + lineAnchor.size());
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string globalName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, globalName));

    const std::string encloseName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    EXPECT_TRUE(HasAction(applicable, encloseName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, encloseName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol18)
{
    const std::string code = R"(
let value: int = 42;
let typeStr = /*start*/typeof value/*end*/;
)";
    const std::string expected = R"(
let value: int = 42;
let newLocal = typeof value;
let typeStr = /*start*/newLocal/*end*/;
)";
    const std::string target = R"(typeof value)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string encloseName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    EXPECT_TRUE(HasAction(applicable, encloseName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, encloseName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol19)
{
    const std::string code = R"(
class MyClass {
  value: int = 0;
}
let obj: Object = new MyClass();
let isInstance = /*start*/obj instanceof MyClass/*end*/;
)";
    const std::string expected = R"(
class MyClass {
  value: int = 0;
}
let obj: Object = new MyClass();
const newLocal = obj instanceof MyClass;
let isInstance = /*start*/newLocal/*end*/;
)";
    const std::string target = R"(obj instanceof MyClass)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string globalName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, globalName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, globalName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol20)
{
    const std::string code = R"(
function getPair(): /*start*/[number, string]/*end*/ {
  return [1, "a"];
}
)";
    const std::string target = R"([number, string])";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);

    const std::string varGlobal = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    const std::string varEnclose = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const std::string constGlobal = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string constEnclose = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const std::string funcGlobal = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const std::string funcEnclose = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    EXPECT_FALSE(HasAction(applicable, varGlobal));
    EXPECT_FALSE(HasAction(applicable, varEnclose));
    EXPECT_FALSE(HasAction(applicable, constGlobal));
    EXPECT_FALSE(HasAction(applicable, constEnclose));
    EXPECT_FALSE(HasAction(applicable, funcGlobal));
    EXPECT_FALSE(HasAction(applicable, funcEnclose));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol21)
{
    const std::string code = R"(
function process(value: /*start*/string | number | boolean/*end*/) {
  return value;
}
)";
    const std::string target = R"(string | number | boolean)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);

    const std::string varGlobal = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    const std::string varEnclose = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const std::string constGlobal = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string constEnclose = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const std::string funcGlobal = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const std::string funcEnclose = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    EXPECT_FALSE(HasAction(applicable, varGlobal));
    EXPECT_FALSE(HasAction(applicable, varEnclose));
    EXPECT_FALSE(HasAction(applicable, constGlobal));
    EXPECT_FALSE(HasAction(applicable, constEnclose));
    EXPECT_FALSE(HasAction(applicable, funcGlobal));
    EXPECT_FALSE(HasAction(applicable, funcEnclose));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol22)
{
    const std::string code = R"(
interface GeneratedTypeLiteralInterface_2 {
  value: number;
}

interface GeneratedTypeLiteralInterface_1 {
  value: number;
  children: GeneratedTypeLiteralInterface_2[];
}

class Tree {
  getNode(): /*start*/GeneratedTypeLiteralInterface_1/*end*/ {
    return { value: 1, children: [] };
  }
}
)";
    const std::string target = R"(GeneratedTypeLiteralInterface_1)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);

    const std::string varGlobal = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    const std::string varEnclose = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const std::string constGlobal = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string constEnclose = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const std::string funcGlobal = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const std::string funcEnclose = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    EXPECT_FALSE(HasAction(applicable, varGlobal));
    EXPECT_FALSE(HasAction(applicable, varEnclose));
    EXPECT_FALSE(HasAction(applicable, constGlobal));
    EXPECT_FALSE(HasAction(applicable, constEnclose));
    EXPECT_FALSE(HasAction(applicable, funcGlobal));
    EXPECT_FALSE(HasAction(applicable, funcEnclose));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol23)
{
    const std::string code = R"(
function validate(age: number) {
  if (age < 0) {
    throw /*start*/new Error("Invalid age")/*end*/;
  }
}
)";
    const std::string expected = R"(
function newFunction(): Error {
  return new Error("Invalid age");
}

function validate(age: number) {
  if (age < 0) {
    throw /*start*/newFunction()/*end*/;
  }
}
)";
    const std::string target = R"(new Error("Invalid age"))";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = "extract_function_scope_2";
    ASSERT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol24)
{
    const std::string code = R"(
const _ = class {
  a() {
    /*start*/let a1 = { x: 1 };
    return a1.x + 10;/*end*/
  }
}
)";
    const std::string target = R"(let a1 = { x: 1 };
    return a1.x + 10;)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = "extract_function_scope_2";
    EXPECT_FALSE(HasAction(applicable, actionName));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol25)
{
    const std::string code = R"(
namespace A {
  namespace B {
    let arr: Array<float> = [];
  }
}
)";
    const std::string expected = R"(
namespace A {
  const newLocal: Array<float> = [];
  namespace B {
    let arr: Array<float> = newLocal;
  }
}
)";
    const std::string target = R"([])";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string encloseScopeAction = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    std::string namespaceAName = FindConstantNamespaceActionName(applicable, encloseScopeAction, "namespace 'A'");
    EXPECT_TRUE(HasAction(applicable, namespaceAName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, namespaceAName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

}  // namespace
