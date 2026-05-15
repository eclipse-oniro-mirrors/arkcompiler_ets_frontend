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

class LspExtrSymblGetEditsTestsConstant : public LSPAPITests {
public:
    ark::es2panda::lsp::RefactorContext *CreateExtractContext(Initializer *initializer, const std::string &code,
                                                              size_t start, size_t end)
    {
        std::vector<std::string> files = {"ExtractSymbolRefactorConstantTest.ets"};
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

protected:
    bool HasRefactorAction(const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable,
                           const std::string_view actionName)
    {
        return std::any_of(applicable.begin(), applicable.end(),
                           [&](const auto &info) { return info.action.name == actionName; });
    }

    std::string StripWhitespace(const std::string &s)
    {
        std::string result = s;
        result.erase(std::remove_if(result.begin(), result.end(), [](unsigned char c) { return std::isspace(c); }),
                     result.end());
        return result;
    }

    void VerifyExtractConstantRefactor(ark::es2panda::lsp::RefactorContext *context, const std::string &expected,
                                       const std::string &originalCode, const std::string &refactorName,
                                       const std::string &actionName)
    {
        auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*context, refactorName, actionName);

        ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
        const auto &fileEdit = edits->GetFileTextChanges().at(0);
        ASSERT_FALSE(fileEdit.textChanges.empty());

        const std::string actual = ApplyEdits(originalCode, fileEdit.textChanges);
        EXPECT_EQ(StripWhitespace(actual), StripWhitespace(expected));
    }

    std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> GetApplicableOrAssert(
        ark::es2panda::lsp::RefactorContext *context)
    {
        auto applicable = GetApplicableRefactorsImpl(context);
        EXPECT_FALSE(applicable.empty());
        return applicable;
    }

    void ExpectActionPresent(const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable,
                             const std::string &actionName, bool expected)
    {
        EXPECT_EQ(HasRefactorAction(applicable, actionName), expected);
    }

    bool HasAnyActionWithPrefix(const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable,
                                const std::string &actionName, const std::string &prefix)
    {
        return std::any_of(applicable.begin(), applicable.end(), [&](const auto &info) {
            return info.action.name == actionName || info.action.name.rfind(prefix, 0) == 0;
        });
    }
};

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant1)
{
    const std::string code = R"(
for (let i = 0; i < 10; i++) {
    for (let j = 0; j < 10; j++) {
        const x = i + 1;
    }
}
)";
    const std::string expected = R"(
for (let i = 0; i < 10; i++) {
    for (let j = 0; j < 10; j++) {
        const newLocal: Int = i + 1;
        const x = newLocal;
    }
}
)";
    const std::string target = "i + 1";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const std::string globalActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const bool hasConstantEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    const bool hasConstantGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == globalActionName; });
    EXPECT_TRUE(hasConstantEnclose);
    EXPECT_FALSE(hasConstantGlobal);

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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant2)
{
    const std::string code = R"(
switch (2) {
    case 1:
        break;
}
)";
    const std::string expected = R"(
const newLocal: Int = 1;
switch (2) {
    case newLocal:
        break;
}
)";
    const std::string target = " 1";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasConstantEnclose);

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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant3)
{
    const std::string code = R"(
function F() {
    let i = 0;
    i++;
}
)";
    const std::string expected = R"(
function F() {
    let i = 0;
    const newLocal: Int = i++;
}
)";
    const std::string target = "i++";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasConstantEnclose);

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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant4)
{
    const std::string code = R"(
let i = 0;
function F() {
    i++;
}
)";
    const std::string expected = R"(
let i = 0;
const newLocal: Int = i++;
function F() {
}
)";
    const std::string target = "i++";
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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant5)
{
    const std::string code = R"(
let i = 0;
function F() {
    i++;
}
)";
    const std::string expected = R"(
let i = 0;
function F() {
    const newLocal: Int = i++;
}
)";
    const std::string target = "i++";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasConstantEnclose);

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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant6)
{
    const std::string code = R"(
const i = 0;
for (let j = 0; j < 10; j++) {
    const x = i + 1;
}
)";
    const std::string expected = R"(
const i = 0;
for (let j = 0; j < 10; j++) {
    const newLocal: Int = i + 1;
    const x = newLocal;
}
)";
    const std::string target = "i + 1";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasConstantEnclose);

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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant7)
{
    const std::string code = R"(
const i = 0;
for (let j = 0; j < 10; j++) {
    const x = i + 1;
}
)";
    const std::string expected = R"(
const i = 0;
const newLocal: Int = i + 1;
for (let j = 0; j < 10; j++) {
    const x = newLocal;
}
)";
    const std::string target = "i + 1";
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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant8)
{
    const std::string code = R"(
class C {
    constructor() {
        /*start*/this.m2()/*end*/;
    }
    m2() {
        return 1;
    }
}
)";
    const std::string expected = R"(
class C {
    constructor() {
        /*start*/const newLocal = this.m2()/*end*/;
    }
    m2() {
        return 1;
    }
}
)";
    const std::string target = "this.m2()";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    const std::string globalActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const bool hasConstantGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == globalActionName; });
    EXPECT_TRUE(hasConstantEnclose);
    EXPECT_FALSE(hasConstantGlobal);

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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant9)
{
    const std::string code = R"(
interface I {
    a: number;
}

class A {
    i: I = { a: 1 };
}
)";
    const std::string expected = R"(
interface I {
    a: number;
}

class A {
    private readonly newProperty: I = { a: 1 };
    i: I = this.newProperty;
}
)";
    const std::string target = "{ a: 1 }";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    const bool hasConstantClass = std::any_of(applicable.begin(), applicable.end(),
                                              [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasConstantClass);

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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant10)
{
    const std::string code = R"(
namespace N { //Force this test to be TS-only
    class C {
        x = 1;
        y = this.x;
    }
}
)";
    const std::string expected = R"(
namespace N { //Force this test to be TS-only
    class C {
        x = 1;
        private readonly newProperty = this.x;

        y = this.newProperty;
    }
}
)";
    const std::string target = "this.x";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableOrAssert(refactorContext);

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    const std::string globalActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string namespaceEncloseActionName =
        std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantNamespaceEnclose =
        HasAnyActionWithPrefix(applicable, namespaceEncloseActionName, "extract_constant_scope_ns_");
    const std::string functionClassActionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);
    const std::string functionEncloseActionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    const std::string functionGlobalActionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const bool hasFunctionClass = HasRefactorAction(applicable, functionClassActionName);
    const bool hasFunctionNamespace =
        HasAnyActionWithPrefix(applicable, functionEncloseActionName, "extract_function_scope_ns_");
    const bool hasFunctionGlobal = HasRefactorAction(applicable, functionGlobalActionName);
    ExpectActionPresent(applicable, actionName, true);
    ExpectActionPresent(applicable, globalActionName, false);
    EXPECT_FALSE(hasConstantNamespaceEnclose);
    EXPECT_TRUE(hasFunctionClass);
    EXPECT_FALSE(hasFunctionNamespace);
    EXPECT_FALSE(hasFunctionGlobal);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    VerifyExtractConstantRefactor(refactorContext, expected, code, refactorName, actionName);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant11)
{
    const std::string code = R"(
namespace A {
    const i = 1;
    function f() {
        const x = i + 1;
    }
}
)";
    const std::string expected = R"(
namespace A {
    const i = 1;
    function f() {
        const newLocal: Int = i + 1;
        const x = newLocal;
    }
}
)";
    const std::string target = "i + 1";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string globalActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == globalActionName; });
    const bool hasConstantEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_FALSE(hasConstantGlobal);
    EXPECT_TRUE(hasConstantEnclose);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    VerifyExtractConstantRefactor(refactorContext, expected, code, refactorName, encloseActionName);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant12)
{
    const std::string code = R"(
namespace A {
    export const i = 1;
    function f() {
        const x = i + 1;
    }
}
)";
    const std::string expected = R"(
const newLocal: Int = A.i + 1;
namespace A {
    export const i = 1;
    function f() {
        const x = newLocal;
    }
}
)";
    const std::string target = "i + 1";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string globalActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == globalActionName; });
    const bool hasConstantEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_TRUE(hasConstantGlobal);
    EXPECT_TRUE(hasConstantEnclose);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    VerifyExtractConstantRefactor(refactorContext, expected, code, refactorName, globalActionName);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant13)
{
    const std::string code = R"(
namespace A {
    interface I {
        x: number;
    }
    function id<T>(value: T): T {
        return value;
    }
    function f() {
        const x = id<I>({ x: 1 });
    }
}
)";
    const std::string expected = R"(
namespace A {
    interface I {
        x: number;
    }
    function id<T>(value: T): T {
        return value;
    }
    function f() {
        const newLocal = id<I>({ x: 1 });
        const x = newLocal;
    }
}
)";
    const std::string target = "id<I>({ x: 1 })";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string globalActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == globalActionName; });
    const bool hasConstantEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_FALSE(hasConstantGlobal);
    EXPECT_TRUE(hasConstantEnclose);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    VerifyExtractConstantRefactor(refactorContext, expected, code, refactorName, encloseActionName);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant14)
{
    const std::string code = R"(
namespace A {
    export interface I {
        x: number;
    }
    export const i = 1;
    function f() {
        const x = i + 1;
    }
}
)";
    const std::string expected = R"(
const newLocal: Int = A.i + 1;
namespace A {
    export interface I {
        x: number;
    }
    export const i = 1;
    function f() {
        const x = newLocal;
    }
}
)";
    const std::string target = "i + 1";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string globalActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == globalActionName; });
    const bool hasConstantEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_TRUE(hasConstantGlobal);
    EXPECT_TRUE(hasConstantEnclose);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    VerifyExtractConstantRefactor(refactorContext, expected, code, refactorName, globalActionName);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant15)
{
    const std::string code = R"(
namespace A {
    export const i = 1;
    function f() {
        const x = i.toString();
    }
}
)";
    const std::string expected = R"(
const newLocal = A.i.toString();
namespace A {
    export const i = 1;
    function f() {
        const x = newLocal;
    }
}
)";
    const std::string target = "i.toString()";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string globalActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == globalActionName; });
    const bool hasConstantEnclose = std::any_of(
        applicable.begin(), applicable.end(), [&](const auto &info) { return info.action.name == encloseActionName; });
    EXPECT_TRUE(hasConstantGlobal);
    EXPECT_TRUE(hasConstantEnclose);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    VerifyExtractConstantRefactor(refactorContext, expected, code, refactorName, globalActionName);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant16)
{
    const std::string code = R"(
namespace A {
    export const i = 1;
    namespace B {
        function f() {
            const x = i.toString();
        }
    }
}
)";
    const std::string expected = R"(
namespace A {
    export const i = 1;
    const newLocal = i.toString();
    namespace B {
        function f() {
            const x = newLocal;
        }
    }
}
)";
    const std::string target = "i.toString()";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    std::string namespaceActionName;
    for (const auto &info : applicable) {
        if (info.action.name.rfind(std::string("extract_constant_scope_ns_"), 0) == 0) {
            namespaceActionName = info.action.name;
            break;
        }
    }
    ASSERT_FALSE(namespaceActionName.empty());

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    VerifyExtractConstantRefactor(refactorContext, expected, code, refactorName, namespaceActionName);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant17)
{
    const std::string code = R"(
class C {
    x = 1;
}
)";
    const std::string expected = R"(
class C {
    private readonly newProperty = 1;
    x = this.newProperty;
}
)";
    const std::string target = " 1";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    const std::string globalActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const bool hasConstantClass = std::any_of(applicable.begin(), applicable.end(),
                                              [&](const auto &info) { return info.action.name == actionName; });
    const bool hasConstantGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == globalActionName; });
    EXPECT_TRUE(hasConstantClass);
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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant18)
{
    const std::string code = R"(
'use static'
let a = "123";
let ChinaStr1: string = "他看她";
let b = "123";
)";
    const std::string expected = R"(
'use static'
let a = "123";
const newLocal: String = "他看她";
let ChinaStr1: string = newLocal;
let b = "123";
)";
    const std::string target = "\"他看她\"";
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
    const std::string classActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantClass = std::any_of(applicable.begin(), applicable.end(),
                                              [&](const auto &info) { return info.action.name == classActionName; });
    const bool hasNamespaceEnclose = std::any_of(applicable.begin(), applicable.end(), [&](const auto &info) {
        return info.action.name.rfind("extract_constant_scope_ns_", 0) == 0;
    });
    EXPECT_TRUE(hasConstantGlobal);
    EXPECT_FALSE(hasConstantClass);
    EXPECT_FALSE(hasNamespaceEnclose);

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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant19)
{
    const std::string code = R"(
switch (1) {
    case 123:
        break;
}
)";
    const std::string expected = R"(
const newLocal: Int = 123;
switch (1) {
    case newLocal:
        break;
}
)";
    const std::string target = "123";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableOrAssert(refactorContext);

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string classActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const std::string constantKind = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.kind);
    const bool hasConstantEnclose = std::any_of(applicable.begin(), applicable.end(), [&](const auto &info) {
        return info.action.name == encloseActionName && info.action.kind == constantKind;
    });
    const bool hasNamespaceEnclose = std::any_of(applicable.begin(), applicable.end(), [&](const auto &info) {
        return info.action.name.rfind("extract_constant_scope_ns_", 0) == 0;
    });
    ExpectActionPresent(applicable, actionName, true);
    EXPECT_FALSE(hasConstantEnclose);
    ExpectActionPresent(applicable, classActionName, false);
    EXPECT_FALSE(hasNamespaceEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    VerifyExtractConstantRefactor(refactorContext, expected, code, refactorName, actionName);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant20)
{
    const std::string code = R"(
"hello";
)";
    const std::string expected = R"(
const newLocal = "hello";
)";
    const std::string target = "\"hello\";";
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
    const std::string classActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const std::string constantKind = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.kind);
    const bool hasConstantEnclose = std::any_of(applicable.begin(), applicable.end(), [&](const auto &info) {
        return info.action.name == encloseActionName && info.action.kind == constantKind;
    });
    const bool hasConstantClass = std::any_of(applicable.begin(), applicable.end(),
                                              [&](const auto &info) { return info.action.name == classActionName; });
    const bool hasNamespaceEnclose = std::any_of(applicable.begin(), applicable.end(), [&](const auto &info) {
        return info.action.name.rfind("extract_constant_scope_ns_", 0) == 0;
    });
    EXPECT_TRUE(hasConstantGlobal);
    EXPECT_FALSE(hasConstantEnclose);
    EXPECT_FALSE(hasConstantClass);
    EXPECT_FALSE(hasNamespaceEnclose);

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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant21)
{
    const std::string code = R"(
let i = 0;
function F() {
    /*start*/i++/*end*/;
}
)";
    const std::string expected = R"(
let i = 0;
const newLocal: Int = i++;
function F() {
    /*start*//*end*/;
}
)";
    const std::string target = "i++";
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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant22)
{
    const std::string code = R"(
const i = 0;
for (let j = 0; j < 10; j++) {
    const x = i + 1;
}
)";
    const std::string expected = R"(
const i = 0;
const newLocal: Int = i + 1;
for (let j = 0; j < 10; j++) {
    const x = newLocal;
}
)";
    const std::string target = "i + 1";
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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant23)
{
    const std::string code = R"(
class C {
    constructor() {
        this.m2();
    }
    m2() { return 1; };
}
)";
    const std::string expected = R"(
class C {
    private readonly newProperty = this.m2();

    constructor() {
        this.newProperty;
    }
    m2() { return 1; };
}
)";
    const std::string target = "this.m2()";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    const bool hasConstantClass = std::any_of(applicable.begin(), applicable.end(),
                                              [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasConstantClass);

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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant24)
{
    const std::string code = R"(
const f = () => {
  return 2 + 1;
}
)";
    const std::string expected = R"(
const newLocal: Int = 2 + 1;

const f = () => {
  return newLocal;
}
)";
    const std::string target = "2 + 1";
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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant25)
{
    const std::string code = R"(
for (let i = 0; i < 20; i++) {
  for (let j = 0; j < 20; j++) {
    let x = /*start*/1/*end*/;
  }
}
)";
    const std::string expected = R"(
for (let i = 0; i < 20; i++) {
  for (let j = 0; j < 20; j++) {
    const newLocal: Int = 1;
    let x = /*start*/newLocal/*end*/;
  }
}
)";
    const std::string target = "1";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableOrAssert(refactorContext);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    ExpectActionPresent(applicable, actionName, true);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    VerifyExtractConstantRefactor(refactorContext, expected, code, refactorName, actionName);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant26)
{
    const std::string code = R"(
'use static'
interface I {a:int};
let i: I = {a:1};
)";
    const std::string expected = R"(
'use static'
interface I {a:int};
const newLocal: I = {a:1};
let i: I = newLocal;
)";
    const std::string target = "{a:1}";
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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant27)
{
    const std::string code = R"(
namespace N {
    let arr: Array<int|string|boolean> = [1, "2", true];
}
)";
    const std::string expected = R"(
const newLocal: Array<int|string|boolean> = [1, "2", true];

namespace N {
    let arr: Array<int|string|boolean> = newLocal;
}
)";
    const std::string target = "[1, \"2\", true]";
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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant28)
{
    const std::string code = R"(
namespace N {
    let a: double = 3 + 6 - 9 * 1 / 2;
}
)";
    const std::string expected = R"(
const newLocal: double = 3 + 6 - 9 * 1 / 2;

namespace N {
    let a: double = newLocal;
}
)";
    const std::string target = "3 + 6 - 9 * 1 / 2";
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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant29)
{
    const std::string code = R"(
namespace N {
    let emptyArr: Array<float> = [];
}
)";
    const std::string expected = R"(
namespace N {
    const newLocal: Array<float> = [];
    let emptyArr: Array<float> = newLocal;
}
)";
    const std::string target = "[]";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasConstantEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasConstantEnclose);

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

TEST_F(LspExtrSymblGetEditsTestsConstant, ExtractConstant30)
{
    const std::string code = R"(
'use static'
class A {}
let a = 1 + 1;
)";
    const std::string expected = R"(
'use static'
class A {}
const newLocal: Int = 1 + 1;

let a = newLocal;
)";
    const std::string target = "1 + 1";
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
    LSPAPI const *lspApi = GetImpl();
    auto edits = lspApi->getEditsForRefactor(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

}  // namespace
