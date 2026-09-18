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

static const std::string EXTRACT_SYMBOL32_CODE = R"(
interface I {x: int};
namespace A {let y = 1;
  class C {b() {}
    a() {let z = 1;
      /*start*/let a1: I = { x: 1 };
      y = 10;
      z = 42;
      this.b();
      return a1.x + 10;/*end*/
    }
  }
}
)";

static const std::string EXTRACT_SYMBOL32_EXPECTED = R"(
interface I {x: int};
namespace A {let y = 1;
  class C {b() {}
    private newMethod(z: int): Int {
      let a1: I = { x: 1 };
      y = 10;
      z = 42;
      this.b();
      return a1.x + 10;
    }
    a() {
      let z = 1;
      /*start*/return this.newMethod(z);/*end*/
    }
  }
}
)";

static const std::string EXTRACT_SYMBOL32_TARGET = R"(let a1: I = { x: 1 };
      y = 10;
      z = 42;
      this.b();
      return a1.x + 10;)";

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol1)
{
    const std::string code = R"(
namespace A {
  export namespace B {
    let B_undefinedValue:undefined = undefined; // Constant undefined : error
    type B_directVars = 'Up' | 'Down' | 'Left' | 'Right'; // Method 'Up' | 'Down' | 'Left' | 'Right' : error
    const B_skipVar: B_directVars = 'Down'; // Constant 'Down' -> global scope
  }
}
)";

    const std::string target = R"('Up' | 'Down' | 'Left' | 'Right')";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    refactorContext->kind = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.kind);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(applicable.empty());

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol2)
{
    const std::string code = R"(
'use static'
namespace A {
  let A_intvar_plus1 = 1, A_mixVar = /**/ A_intvar_plus1 + 2; // Constant A_intvar_plus1 + 2 : auto
  let A_partWord = "ArkTS"; let Words = `Hello, ${A_partWord}!`; // Method `Hello, ${A_partWord}!` -> namespace 'A'
}
)";
    const std::string expected = R"(
'use static'
namespace A {
  let A_intvar_plus1 = 1, A_mixVar = /**/ A_intvar_plus1 + 2; // Constant A_intvar_plus1 + 2 : auto
  function newFunction(A_partWord: String): String {
    return `Hello, ${A_partWord}!`;
  }
  let A_partWord = "ArkTS"; let Words = newFunction(A_partWord); // Method `Hello, ${A_partWord}!` -> namespace 'A'
}
)";
    const std::string target = R"(`Hello, ${A_partWord}!`)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    EXPECT_TRUE(HasAction(applicable, actionName));
    const std::string actionName1 = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_FALSE(HasAction(applicable, actionName1));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol3)
{
    const std::string code = R"(
const longVar: long = 203; // Method 203 : auto
let lxl = (longVar - 1) * longVar; // Method (longVar - 1) * longVar : auto

function ffff() {
  let B_ChinaStr2: string = '它看她'; // Method '它看她' -> global scope
}
)";
    const std::string expected = R"(
const longVar: long = 203; // Method 203 : auto
let lxl = (longVar - 1) * longVar; // Method (longVar - 1) * longVar : auto
const newLocal: String = '它看她';
function ffff() {
  let B_ChinaStr2: string = newLocal; // Method '它看她' -> global scope
}
)";
    const std::string target = R"('它看她')";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &textChanges = edits->GetFileTextChanges().at(0).textChanges;
    const auto insertChange = std::find_if(textChanges.begin(), textChanges.end(), [](const auto &change) {
        return change.span.length == 0 && change.newText.find("const newLocal") != std::string::npos;
    });
    ASSERT_NE(insertChange, textChanges.end());
    EXPECT_EQ(insertChange->span.start, 125U);
    const std::string result = ApplyEdits(code, textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol4)
{
    const std::string code = R"(
const /*A*/Npro = 1,
  /*B*/Nbak = /*start*/Npro + 1/*end*/; // Constant start-end : auto

namespace A {
  type B_directVars = 'Up' | 'Down' | 'Left' | 'Right'; // Method 'Up' | 'Down' | 'Left' | 'Right' : error
  const B_skipVar: B_directVars = 'Down'; // Constant 'Down' -> global scope
}
)";
    const std::string expected = R"(
const /*A*/Npro = 1,
  /*B*/Nbak = /*start*/Npro + 1/*end*/; // Constant start-end : auto
const newLocal: 'Up' | 'Down' | 'Left' | 'Right' = 'Down';
namespace A {
  type B_directVars = 'Up' | 'Down' | 'Left' | 'Right'; // Method 'Up' | 'Down' | 'Left' | 'Right' : error
  const B_skipVar: B_directVars = newLocal; // Constant 'Down' -> global scope
}
)";
    const size_t spanStart = 247;
    const size_t spanEnd = 253;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol5)
{
    const std::string code = R"(
type IsString<T> = T extends string ? /*start*/true/*end*/ : false;
)";
    const std::string target = R"(true)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    refactorContext->kind = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.kind);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(applicable.empty());

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol6)
{
    const std::string code = R"(
'use static'

'use static'
/// <reference path="path.js"/>
const x = /*start*/2 + 1/*end*/;
)";
    const std::string expected = R"(
'use static'

'use static'
/// <reference path="path.js"/>
const newLocal: Int = 2 + 1;
const x = /*start*/newLocal/*end*/;
)";
    const std::string target = R"(2 + 1)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol7)
{
    const std::string code = R"(
function calculateSum(a: int, b: int): int {
return a + b;
}
let result = /*start*/calculateSum(1, 2)/*end*/;
)";
    const std::string expected = R"(
function calculateSum(a: int, b: int): int {
return a + b;
}
function newFunction(): Int {
  return calculateSum(1, 2);
}
let result = /*start*/newFunction()/*end*/;
)";
    const std::string target = R"(calculateSum(1, 2))";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    refactorContext->kind = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.kind);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));
    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    EXPECT_FALSE(HasAction(applicable, encloseActionName));
    const std::string actionName1 = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_FALSE(HasAction(applicable, actionName1));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol8)
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
    refactorContext->kind = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.kind);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));
    const std::string encloseActionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    EXPECT_FALSE(HasAction(applicable, encloseActionName));
    const std::string actionName1 = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_FALSE(HasAction(applicable, actionName1));
    const std::string actionName2 = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_FALSE(HasAction(applicable, actionName2));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol9)
{
    const std::string code = R"(
const aaa: long = 1 + 1;
const b = aaa * (aaa + 1);
)";
    const std::string expected = R"(
const aaa: long = 1 + 1;
function newFunction(): long {
  return aaa * (aaa + 1);
}
const b = newFunction();
)";
    const std::string target = R"(aaa * (aaa + 1))";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));
    const std::string actionName1 = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_FALSE(HasAction(applicable, actionName1));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol10)
{
    const std::string code = R"(
'use static'
/*! dsa */
const x = 2 + 1;
)";
    const std::string expected = R"(
'use static'
/*! dsa */
const newLocal: Int = 2 + 1;
const x = newLocal;
)";
    const std::string target = R"(2 + 1)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol11)
{
    const std::string code = R"(
export class A {
  C_sysNum:int = 0b1110 + 0o765 - 0xFE;
}
)";
    const std::string expected = R"(
export class A {
  private readonly newProperty: int = 0b1110 + 0o765 - 0xFE;
  C_sysNum:int = this.newProperty;
}
)";
    const std::string target = R"(0b1110 + 0o765 - 0xFE)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName1 = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    EXPECT_FALSE(HasAction(applicable, actionName1));

    const std::string actionName2 = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName2));

    const std::string actionName3 = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    EXPECT_TRUE(HasAction(applicable, actionName3));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName3);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol12)
{
    const std::string code = R"(
export function fff(): void {
  let TestLambda  = () => {
    let FF_doubleVar: double = 3 + 6 - 9 * 1 / 2;
  }
}
)";
    const std::string expected = R"(
const newLocal: double = 3 + 6 - 9 * 1 / 2;
export function fff(): void {
  let TestLambda  = () => {
    let FF_doubleVar: double = newLocal;
  }
}
)";
    const std::string target = R"(3 + 6 - 9 * 1 / 2)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string encloseAction = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, encloseAction));
    EXPECT_TRUE(HasAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, globalAction);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol13)
{
    const std::string code = R"(
export class A {
  C_ChinaChar: char = c'她';
}
)";
    const std::string expected = R"(
export class A {
  private readonly newProperty: Int = c'她';
  C_ChinaChar: char = this.newProperty;
}
)";
    const std::string target = R"(c'她')";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string classAction = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    const std::string encloseAction = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    EXPECT_TRUE(HasAction(applicable, globalAction));
    EXPECT_TRUE(HasAction(applicable, classAction));
    EXPECT_FALSE(HasAction(applicable, encloseAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, classAction);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol14)
{
    const std::string code = R"(
let fn = (x: int): int => {
  return /*start*/x + 1/*end*/;
};
)";
    const std::string expected = R"(
let fn = (x: int): int => {
  let newLocal: Int = x + 1;
  return /*start*/newLocal/*end*/;
};
)";
    const std::string target = R"(x + 1)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol15)
{
    const std::string code = R"(
function validate(age: number) {
  if (age < 0) {
    throw /*start*/new Error("Invalid age")/*end*/;
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

    const std::string functionGlobal = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const std::string constantGlobal = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, functionGlobal));
    EXPECT_FALSE(HasAction(applicable, constantGlobal));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol16)
{
    const std::string code = R"(
'use static'

class A{};

const f = () => {
  return /*start*/2 + 1/*end*/;
};
)";
    const std::string expected = R"(
'use static'

class A{};
const newLocal: Int = 2 + 1;

const f = () => {
  return /*start*/newLocal/*end*/;
};
)";
    const std::string target = R"(2 + 1)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol17)
{
    const std::string code = R"(
function F() {
  let i = 0;
  /*start*/i++/*end*/;
}
)";
    const std::string expected = R"(
function F() {
  let i = 0;
  /*start*/const newLocal: Int = i++/*end*/;
}
)";
    const std::string target = R"(i++)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));

    const size_t expectedRenameLoc = result.find("newLocal") - std::string("/*start*/").size();
    ASSERT_NE(expectedRenameLoc, std::string::npos);
    ASSERT_TRUE(edits->GetRenameLocation().has_value());
    EXPECT_EQ(edits->GetRenameLocation().value(), expectedRenameLoc);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol18)
{
    const std::string code = R"(
type IsString<T> = T extends string ? /*start*/true/*end*/ : false;
)";
    const std::string target = R"(true)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);

    const std::string functionGlobal = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const std::string functionEnclose = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    EXPECT_FALSE(HasAction(applicable, functionGlobal));
    EXPECT_FALSE(HasAction(applicable, functionEnclose));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol19)
{
    const std::string code = R"(
function getDayType(day: number): string {
  /*start*/switch (day) {
    case 1: return "Weekday";
    case 2: return "Weekday";
    default: return "Weekend";
  }/*end*/
}
)";
    const std::string expected = R"(
function getDayType(day: number): string {
  /*start*/return newFunction(day);/*end*/
}

function newFunction(day: number): String {
  switch (day) {
    case 1: return "Weekday";
    case 2: return "Weekday";
    default: return "Weekend";
  }
}
)";
    const std::string target = R"(switch (day) {
    case 1: return "Weekday";
    case 2: return "Weekday";
    default: return "Weekend";
  })";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol20)
{
    const std::string code = R"(
namespace A {
  export namespace B {
    type B_directVars = 'Up' | 'Down' | 'Left' | 'Right';
    const B_skipVar:B_directVars = 'Down'; // Constant 'Down' -> global scope
  }
}
)";
    const std::string expected = R"(
const newLocal: 'Up' | 'Down' | 'Left' | 'Right' = 'Down';
namespace A {
  export namespace B {
    type B_directVars = 'Up' | 'Down' | 'Left' | 'Right';
    const B_skipVar: B_directVars = newLocal; // Constant 'Down' -> global scope
  }
}
)";
    const std::string target = R"(B_skipVar:B_directVars = )";
    const size_t targetPos = code.find(target);
    ASSERT_NE(targetPos, std::string::npos);
    const size_t spanStart = targetPos + target.size();
    const size_t spanEnd = spanStart + std::string(R"('Down')").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol21)
{
    const std::string code = R"(
const longVar: long = 203;
let lxl = (longVar - 1) * longVar;
)";
    const std::string expected = R"(
const longVar: long = 203;
function newFunction(): long {
  return (longVar - 1) * longVar;
}
let lxl = newFunction();
)";
    const std::string target = R"((longVar - 1) * longVar)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const std::string result = ApplyEdits(code, edits->GetFileTextChanges().at(0).textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol22)
{
    const std::string code = R"(
class A {
  TestMethod(): void {
    const C_intVar: int = 201;
    let C_intxint = C_intVar * C_intVar;
  }
}
)";
    const std::string expected = R"(
class A {
  TestMethod(): void {
    const C_intVar: int = 201;
    let C_intxint = this.newMethod(C_intVar);
  }
  private newMethod(C_intVar: int): number {
    return C_intVar * C_intVar;
  }
}
)";
    const std::string target = R"(C_intVar * C_intVar)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol23)
{
    const std::string code = R"(
'use static'
const longVar: long = 203;
let lxl = (longVar - 1) * longVar;
)";
    const std::string expected = R"(
'use static'
const longVar: long = 203;
function newFunction(): long {
  return (longVar - 1) * longVar;
}
let lxl = newFunction();
)";
    const std::string target = R"((longVar - 1) * longVar)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol24)
{
    const std::string code = R"(
namespace A {
  function A_Func1() {}
  let A_shortVar: short = 202;
  let a = /**/ 1;
  let A_fdf = A_shortVar / (A_shortVar + 1);
}
)";
    const std::string expected = R"(
namespace A {
  function A_Func1() {}
  let A_shortVar: short = 202;
  let a = /**/ 1;
  function newFunction(A_shortVar: short): Int {
    return A_shortVar / (A_shortVar + 1);
  }
  let A_fdf = newFunction(A_shortVar);
}
)";
    const std::string target = R"(A_shortVar / (A_shortVar + 1))";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol25)
{
    const std::string code = R"(
export class A {
  C_char: char = c'她';
}
)";
    const std::string expected = R"(
const newLocal: char = c'她';
export class A {
  C_char: char = newLocal;
}
)";
    const std::string target = R"(c'她')";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol26)
{
    const std::string code = R"(
export class A {
  C_char: char = c'她';
}
)";
    const std::string target = R"(c'她')";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    EXPECT_FALSE(HasAction(applicable, actionName));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol27)
{
    const std::string code = R"(
export class A {
  C_sysNum:int = 0b1110 + 0o765 - 0xFE;
}
)";
    const std::string expected = R"(
const newLocal: int = 0b1110 + 0o765 - 0xFE;
export class A {
  C_sysNum:int = newLocal;
}
)";
    const std::string target = R"(0b1110 + 0o765 - 0xFE)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const bool hasactionName = std::any_of(applicable.begin(), applicable.end(),
                                           [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasactionName);

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

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol28)
{
    const std::string code = R"(
export function fff(): void {
  let TestLambda  = () => {
    let FF_doubleVar: double = 3 + 6 - 9 * 1 / 2;
  }
}
)";
    const std::string expected = R"(
export function fff(): void {
  let TestLambda  = () => {
    const newLocal: double = 3 + 6 - 9 * 1 / 2;
    let FF_doubleVar: double = newLocal;
  }
}
)";
    const std::string target = R"(3 + 6 - 9 * 1 / 2)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasactionName = std::any_of(applicable.begin(), applicable.end(),
                                           [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasactionName);

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

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol29)
{
    const std::string code = R"(
namespace A {
  export namespace B {
    type B_directVars = 'Up' | 'Down' | 'Left' | 'Right';
    const B_skipVar: B_directVars = 'Down'; // Constant 'Down' -> global scope
  }
}
)";
    const std::string expected = R"(
namespace A {
  export namespace B {
    type B_directVars = 'Up' | 'Down' | 'Left' | 'Right';
    const newLocal: 'Up' | 'Down' | 'Left' | 'Right' = 'Down';
    const B_skipVar: B_directVars = newLocal; // Constant 'Down' -> global scope
  }
    }
)";
    const std::string target = R"(B_skipVar: B_directVars = )";
    const size_t targetPos = code.find(target);
    EXPECT_NE(targetPos, std::string::npos);
    const size_t spanStart = targetPos + target.size();
    const size_t spanEnd = spanStart + std::string(R"('Down')").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    EXPECT_TRUE(HasAction(applicable, actionName));

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

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol30)
{
    const std::string code = R"(
namespace A {
  export namespace B {
    type B_directVars = 'Up' | 'Down' | 'Left' | 'Right';
    const B_skipVar: B_directVars = 'Down'; // Constant 'Down' -> global scope
  }
}
)";
    const std::string expected = R"(
namespace A {
  const newLocal: 'Up' | 'Down' | 'Left' | 'Right' = 'Down';
  export namespace B {
    type B_directVars = 'Up' | 'Down' | 'Left' | 'Right';
    const B_skipVar: B_directVars = newLocal; // Constant 'Down' -> global scope
  }
    }
)";
    const std::string target = R"(B_skipVar: B_directVars = )";
    const size_t targetPos = code.find(target);
    EXPECT_NE(targetPos, std::string::npos);
    const size_t spanStart = targetPos + target.size();
    const size_t spanEnd = spanStart + std::string(R"('Down')").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string encloseScopeAction = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const std::string actionName = FindConstantNamespaceActionName(applicable, encloseScopeAction, "namespace 'A'");
    EXPECT_TRUE(HasAction(applicable, actionName));

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

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol31)
{
    const std::string code = R"(
namespace A {
  export namespace B {
    type B_directVars = 'Up' | 'Down' | 'Left' | 'Right';
    const B_skipVar: B_directVars = 'Down'; // Constant 'Down' -> global scope
  }
}
)";
    const std::string expected = R"(
const newLocal: 'Up' | 'Down' | 'Left' | 'Right' = 'Down';
namespace A {
  export namespace B {
    type B_directVars = 'Up' | 'Down' | 'Left' | 'Right';
    const B_skipVar: B_directVars = newLocal; // Constant 'Down' -> global scope
  }
}
)";
    const std::string target = R"('Down')";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const bool hasactionName = std::any_of(applicable.begin(), applicable.end(),
                                           [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasactionName);

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

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol32)
{
    const size_t spanStart = EXTRACT_SYMBOL32_CODE.find(EXTRACT_SYMBOL32_TARGET);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + EXTRACT_SYMBOL32_TARGET.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), EXTRACT_SYMBOL32_CODE, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);
    const bool hasactionName = std::any_of(applicable.begin(), applicable.end(),
                                           [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasactionName);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto stripWs = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    };
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(EXTRACT_SYMBOL32_CODE, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(EXTRACT_SYMBOL32_EXPECTED));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol33)
{
    const std::string code = R"(
function F() {
  let i = 0;
  /*start*/i++/*end*/;
}
)";
    const std::string expected = R"(
function F() {
  let i = 0;
  /*start*/const newLocal: Int = i++/*end*/;
}
)";
    const std::string target = R"(i++)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const bool hasactionName = std::any_of(applicable.begin(), applicable.end(),
                                           [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasactionName);

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

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol34)
{
    const std::string code = R"(
const a = 1, b = /*start*/a + 1/*end*/;
)";
    const std::string expected = R"(
const a = 1, newLocal = a + 1, b = /*start*/newLocal/*end*/;
)";
    const std::string target = R"(a + 1)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const bool hasFunctionGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasFunctionGlobal);

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

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol35)
{
    const std::string code = R"(
interface MyObject {
  value: int;
}
let obj: MyObject = { value: 1 };
let result: string = /*start*/JSON.stringify(obj)/*end*/;
)";
    const std::string expected = R"(
interface MyObject {
  value: int;
}
function newFunction(obj: MyObject): String {
  return JSON.stringify(obj);
}
let obj: MyObject = { value: 1 };
let result: string = /*start*/newFunction(obj)/*end*/;
)";
    const std::string target = R"(JSON.stringify(obj))";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const bool hasFunctionGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasFunctionGlobal);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
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

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol36)
{
    const std::string code = R"(
function calculateSum(a: int, b: int): int {
  return a + b;
}
let result = /*start*/calculateSum(1, 2)/*end*/;
)";
    const std::string expected = R"(
function calculateSum(a: int, b: int): int {
  return a + b;
}
function newFunction(): Int {
  return calculateSum(1, 2);
}

let result = /*start*/newFunction()/*end*/;
)";
    const std::string target = R"(calculateSum(1, 2))";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const bool hasFunctionGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasFunctionGlobal);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
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

TEST_F(LspExtrSymblGetEditsTests, ExtractSymbol37)
{
    const std::string code = R"(
function merge(...arrays: number[][]) {
  return /*start*/[...arrays[0], ...arrays[1]]/*end*/;
}
)";
    const std::string expected = R"(
function newFunction(...arrays: number[][]): Array<Double> {
  return [...arrays[0], ...arrays[1]];
}

function merge(...arrays: number[][]) {
  return /*start*/newFunction(...arrays)/*end*/;
}
)";
    const std::string target = R"([...arrays[0], ...arrays[1]])";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const bool hasFunctionGlobal = std::any_of(applicable.begin(), applicable.end(),
                                               [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasFunctionGlobal);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
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

}  // namespace
