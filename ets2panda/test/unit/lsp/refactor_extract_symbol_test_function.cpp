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
    ASSERT_NE(edits, nullptr);

    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().front();
    ASSERT_FALSE(fileEdit.textChanges.empty());

    auto stripWs = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    };
    const std::string result = ApplyEdits(source, fileEdit.textChanges);
    EXPECT_EQ(stripWs(result), stripWs(expected));
}

bool HasApplicableAction(const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable,
                         const std::string &actionName)
{
    return std::any_of(applicable.begin(), applicable.end(),
                       [&](const auto &info) { return info.action.name == actionName; });
}

std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> GetApplicableRefactorsOrAssert(
    ark::es2panda::lsp::RefactorContext *refactorContext)
{
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());
    return applicable;
}

void ExpectApplicableAction(ark::es2panda::lsp::RefactorContext *refactorContext, const std::string &actionName)
{
    auto applicable = GetApplicableRefactorsOrAssert(refactorContext);
    EXPECT_TRUE(HasApplicableAction(applicable, actionName));
}

void ExpectApplicableAndApply(const std::string &code, ark::es2panda::lsp::RefactorContext *refactorContext,
                              const std::string &refactorName, const std::string &actionName,
                              const std::string &expected)
{
    ExpectApplicableAction(refactorContext, actionName);
    ExpectExtractionApplies(code, refactorContext, refactorName, actionName, expected);
}

void AssertExtractFunction3ApplicableActions(const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable,
                                             const std::string &classActionName, const std::string &globalActionName)
{
    const auto hasAction = [&applicable](const std::string &name) {
        return std::any_of(applicable.begin(), applicable.end(),
                           [&](const auto &info) { return info.action.name == name; });
    };
    const auto hasNamespaceEnclose = [&applicable]() {
        const std::string namespaceEncloseActionName =
            std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
        return std::any_of(applicable.begin(), applicable.end(), [&](const auto &info) {
            return info.action.name == namespaceEncloseActionName ||
                   info.action.name.rfind(std::string("extract_function_scope_ns_"), 0) == 0;
        });
    };

    EXPECT_TRUE(hasAction(classActionName));
    EXPECT_FALSE(hasAction(globalActionName));
    EXPECT_FALSE(hasNamespaceEnclose());
}

struct ExtractionCaseData {
    const char *code;
    const char *expected;
    const char *target;
};

struct NamespaceActionCaseData {
    ExtractionCaseData extraction;
    const char *namespaceDesc;
};

ExtractionCaseData GetExtractFunction4Case()
{
    return {R"(
namespace A {
  interface I {
    x: number;
  }
  let y = 1;
  class C {
    a() {
      let z = 1;
      let a1: I = { x : 1 };
      y = 10;
      z = 42;
      return a1.x + 10;
    }
  }
}
)",
            R"(
namespace A {
  interface I {
    x: number;
  }
  let y = 1;
  class C {
    a() {
      let z = 1;
      return this.newMethod(z);
    }
    private newMethod(z: int): Double {
      let a1: I = { x : 1 };
      y = 10;
      z = 42;
      return a1.x + 10;
    }
  }
}
)",
            R"(let a1: I = { x : 1 };
      y = 10;
      z = 42;
      return a1.x + 10;)"};
}

ExtractionCaseData GetExtractFunction5Case()
{
    return {R"(
namespace A {
  interface I {
    x: number;
  }
  let y = 1;
  class C {
    a() {
      let z = 1;
      let a1: I = { x : 1 };
      y = 10;
      z = 42;
      return a1.x + 10;
    }
  }
}
)",
            R"(
namespace A {
  interface I {
    x: number;
  }
  let y = 1;
  class C {
    a() {
      let z = 1;
      return newFunction(z);
    }
  }
  function newFunction(z: int): Double {
    let a1: I = { x : 1 };
    y = 10;
    z = 42;
    return a1.x + 10;
  }
}
)",
            R"(let a1: I = { x : 1 };
      y = 10;
      z = 42;
      return a1.x + 10;)"};
}

ExtractionCaseData GetExtractFunction7Case()
{
    return {R"(
namespace A {
  export interface I {
    x: number;
  }
  export let y = 1;
  class C {
    a() {
      let z = 1;
      let a1: I = { x : 1 };
      y = 10;
      z = 42;
      return a1.x + 10;
    }
  }
}
)",
            R"(
namespace A {
  export interface I {
    x: number;
  }
  export let y = 1;
  class C {
    a() {
      let z = 1;
      return newFunction(y, z);
    }
  }
}
function newFunction(y: int, z: int): Double {
  let a1: A.I = { x : 1 };
  y = 10;
  z = 42;
  return a1.x + 10;
}

)",
            R"(let a1: I = { x : 1 };
      y = 10;
      z = 42;
      return a1.x + 10;)"};
}

NamespaceActionCaseData GetExtractFunction23Case()
{
    return {{R"(
namespace A {
  function foo():int {
    return 5;
  }
  namespace B {
    async function a(z: number) {
      let y = 5;
      await Promise.resolve(z);
      return foo();
    }
  }
}
)",
             R"(
namespace A {
  function foo():int {
    return 5;
  }
  namespace B {
    async function a(z: number) {
      return await newFunction(z);
    }
  }
  async function newFunction(z: number): Promise<Int> {
    let y = 5;
    await Promise.resolve(z);
    return foo();
  }
}
)",
             R"(let y = 5;
      await Promise.resolve(z);
      return foo();)"},
            "namespace 'A'"};
}

ExtractionCaseData GetExtractFunction6Case()
{
    return {R"(
namespace A {
  interface I {
    x: number;
  }
  let y = 1;
  class C {
    a() {
      let z = 1;
      let a1: I = { x : 1 };
      y = 10;
      z = 42;
      return a1.x + 10;
    }
  }
}
)",
            "", R"(let a1: I = { x : 1 };
      y = 10;
      z = 42;
      return a1.x + 10;)"};
}

ExtractionCaseData GetExtractFunction22Case()
{
    return {R"(
namespace A {
  function foo():int {
    return 5;
  }
  namespace B {
    async function a(z: number) {
      let y = 5;
      await Promise.resolve(z);
      return foo();
    }
  }
}
)",
            R"(
namespace A {
  function foo():int {
    return 5;
  }
  namespace B {
    async function a(z: number) {
      return await newFunction(z, foo);
    }
  }
}
async function newFunction(z: number, foo: (() => int)): Promise<Int> {
  let y = 5;
  await Promise.resolve(z);
  return foo();
}

)",
            R"(let y = 5;
      await Promise.resolve(z);
      return foo();)"};
}

std::string FindNamespaceActionName(const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable,
                                    const std::string &encloseScopeAction, const std::string &namespaceDesc)
{
    for (const auto &info : applicable) {
        if ((info.action.name == encloseScopeAction ||
             info.action.name.rfind(std::string("extract_function_scope_ns_"), 0) == 0) &&
            info.action.description.find(namespaceDesc) != std::string::npos) {
            return info.action.name;
        }
    }
    return "";
}

const std::string &GetExtractFunction20Code()
{
    static const std::string code = R"(
interface UnaryExpression {
  kind: "Unary";
  operator: string;
  operand: Error;
}

function parseUnaryExpression(operator: string): UnaryExpression {
  return {
    kind: "Unary",
    operator,
    operand: parsePrimaryExpression(),
  };
}

function parsePrimaryExpression(): Error {
  throw Error("Not implemented");
}
)";
    return code;
}

ExtractionCaseData GetSimpleGlobalExtractFunctionCase()
{
    return {R"('use static'

class MyClass {

  MyMethod(a: number, b: number) {
    let c = a + b;
    let d = c * c;
    return d;
  }
}
)",
            R"('use static'

function newFunction(a: number, b: number): number {
  return a + b;
}

class MyClass {

  MyMethod(a: number, b: number) {

    let c = newFunction(a, b);
    let d = c * c;
    return d;
  }
}
)",
            "let c = a + b;"};
}

ExtractionCaseData GetSimpleClassExtractFunctionCase()
{
    return {R"('use static'

class MyClass {

  MyMethod(a: number, b: number) {
    let c = a + b;
    let d = c * c;
    return d;
  }
}
)",
            R"('use static'

class MyClass {

  MyMethod(a: number, b: number) {

    let c = this.newMethod(a, b);
    let d = c * c;
    return d;
  }

  private newMethod(a: number, b: number): number {
    let c = a + b;
    return c;
  }
}
)",
            "let c = a + b;"};
}

ExtractionCaseData GetExtractFunction20Case()
{
    return {GetExtractFunction20Code().c_str(), R"(
interface UnaryExpression {
  kind: "Unary";
  operator: string;
  operand: Error;
}

function parseUnaryExpression(operator: string): UnaryExpression {
  return newFunction(operator);
}

function newFunction(operator: string): UnaryExpression {
  return {
    kind: "Unary",
    operator,
    operand: parsePrimaryExpression(),
  };
}

function parsePrimaryExpression(): Error {
  throw Error("Not implemented");
}
)",
            R"(return {
    kind: "Unary",
    operator,
    operand: parsePrimaryExpression(),
  };)"};
}

const std::string &GetAsyncNamespaceFunctionCode()
{
    static const std::string code = R"(
namespace A {
  function foo() {
  }
  namespace B {
    async function a(z: number, z1: number) {
      /*start*/
      let y = 5;
      if (z) {
        await z1;
      }
      return foo();/*end*/
    }
  }
}
)";
    return code;
}

const std::string &GetExportedNamespaceFunctionCode()
{
    static const std::string code = R"(
namespace A {
  let x = 1;
  export namespace C {
    export function foo() {
    }
  }
  namespace B {
    function a() {
      let a = 1;
      let y = 5;
      let z = x;
      a = y;
      return C.foo();
    }
  }
}
)";
    return code;
}

const std::string &GetGlobalExportFunctionCode()
{
    static const std::string code = R"(
namespace A {
  let x = 1;
  export function foo() {
  }
  namespace B {
    function a() {
      let a = 1;
      let y = 5;
      let z = x;
      a = y;
      foo();
    }
  }
}
)";
    return code;
}

void ExpectSimpleFunctionExtraction(const ExtractionCaseData &testCase, const std::string &actionName,
                                    LspExtrSymblGetEditsTestsFunction &fixture)
{
    const std::string code = testCase.code;
    const std::string target = testCase.target;
    const size_t spanStart = code.find(target, code.find("MyMethod"));
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = fixture.CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    ExpectApplicableAndApply(code, refactorContext, refactorName, actionName, testCase.expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction)
{
    ExpectSimpleFunctionExtraction(GetSimpleGlobalExtractFunctionCase(),
                                   std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name), *this);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction1)
{
    ExpectSimpleFunctionExtraction(GetSimpleClassExtractFunctionCase(),
                                   std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name), *this);
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

  private newMethod(a: number, b: number): number {
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

  private newMethod(a: number, b: number): number {
    let d = a + b;
    return d;
  }

  private newMethod_1(a: number, b: number): number {
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction3)
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
        private newMethod(): C {
            return this.x;
        }
        y = this.newMethod();
    }
}
)";
    const std::string target = "this.x";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);
    const std::string globalActionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    AssertExtractFunction3ApplicableActions(applicable, actionName, globalActionName);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    ExpectExtractionApplies(code, refactorContext, refactorName, actionName, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction4)
{
    const auto testCase = GetExtractFunction4Case();
    const std::string code = testCase.code;
    const size_t spanStart = code.find(testCase.target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::char_traits<char>::length(testCase.target);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string classScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);
    ExpectApplicableAndApply(code, refactorContext, refactorName, classScopeAction, testCase.expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction5)
{
    const auto testCase = GetExtractFunction5Case();
    const std::string code = testCase.code;
    const size_t spanStart = code.find(testCase.target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::char_traits<char>::length(testCase.target);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string encloseScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    ExpectApplicableAndApply(code, refactorContext, refactorName, encloseScopeAction, testCase.expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction6)
{
    const auto testCase = GetExtractFunction6Case();
    const std::string code = testCase.code;
    const size_t spanStart = code.find(testCase.target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::char_traits<char>::length(testCase.target);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const std::string encloseScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const bool hasGlobal = std::any_of(applicable.begin(), applicable.end(),
                                       [&](const auto &info) { return info.action.name == globalScopeAction; });
    const bool hasEnclose = std::any_of(applicable.begin(), applicable.end(),
                                        [&](const auto &info) { return info.action.name == encloseScopeAction; });
    EXPECT_FALSE(hasGlobal);
    EXPECT_TRUE(hasEnclose);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction7)
{
    const auto testCase = GetExtractFunction7Case();
    const std::string code = testCase.code;
    const size_t spanStart = code.find(testCase.target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::char_traits<char>::length(testCase.target);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    ExpectApplicableAndApply(code, refactorContext, refactorName, globalScopeAction, testCase.expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction8)
{
    const std::string code = R"(
function F<T>() {
  const array: T[] = [];
}
)";
    const std::string expected = R"(
function newFunction<T>(): T[] {
  return [];
}

function F<T>() {
  const array: T[] = newFunction<T>();
}
)";

    const std::string anchor = "const array: T[] = [];";
    const size_t anchorPos = code.find(anchor);
    EXPECT_NE(anchorPos, std::string::npos);
    const size_t assignPos = code.find("= []", anchorPos);
    EXPECT_NE(assignPos, std::string::npos);
    const std::string target = "[]";
    const size_t spanStart = assignPos + 2;  // skip "= "
    EXPECT_LT(spanStart, code.size());
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    ExpectApplicableAndApply(code, refactorContext, refactorName, globalScopeAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction9)
{
    const std::string code = R"(
class C<T1 extends object, T2> {
  M(t1: T1, t2: T2) {
    t1.toString();
  }
}
)";
    const std::string expected = R"(
function newFunction<T1 extends object>(t1: T1) {
  t1.toString();
}

class C<T1 extends object, T2> {
  M(t1: T1, t2: T2) {
    newFunction<T1>(t1);
  }
}
)";

    const std::string target = "t1.toString()";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    ExpectApplicableAndApply(code, refactorContext, refactorName, globalScopeAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction10)
{
    const std::string code = R"(
class C<T1 extends object, T2> {
  M(t1: T1, t2: T2) {
    t1.toString();
  }
}
)";
    const std::string expected = R"(
class C<T1 extends object, T2> {
  private newMethod(t1: T1) {
    t1.toString();
  }
  M(t1: T1, t2: T2) {
    this.newMethod(t1);
  }
}
)";

    const std::string target = "t1.toString()";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string classScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const bool hasGlobal = std::any_of(applicable.begin(), applicable.end(),
                                       [&](const auto &info) { return info.action.name == classScopeAction; });
    EXPECT_TRUE(hasGlobal);

    ExpectExtractionApplies(code, refactorContext, refactorName, classScopeAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction11)
{
    const std::string code = R"(
class C {
  M<T1 extends object, T2>(t1: T1, t2: T2) {
    t1.toString();
  }
}
)";
    const std::string expected = R"(
function newFunction<T1 extends object>(t1: T1) {
  t1.toString();
}

class C {
  M<T1 extends object, T2>(t1: T1, t2: T2) {
    newFunction<T1>(t1);
  }
}
)";

    const std::string target = "t1.toString()";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction12)
{
    const std::string code = R"(
class C {
  M<T1 extends object, T2>(t1: T1, t2: T2) {
    t1.toString();
  }
}
)";
    const std::string expected = R"(
class C {
  private newMethod<T1 extends object>(t1: T1) {
    t1.toString();
  }
  M<T1 extends object, T2>(t1: T1, t2: T2) {
    this.newMethod<T1>(t1);
  }
}
)";

    const std::string target = "t1.toString()";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string classScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const bool hasGlobal = std::any_of(applicable.begin(), applicable.end(),
                                       [&](const auto &info) { return info.action.name == classScopeAction; });
    EXPECT_TRUE(hasGlobal);

    ExpectExtractionApplies(code, refactorContext, refactorName, classScopeAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction13)
{
    const std::string code = R"(
function F<T, U extends T[], V extends U[]>(v: V) {
  v.toString();
}
)";
    const std::string expected = R"(
function newFunction<V extends U[]>(v: V) {
  v.toString();
}

function F<T, U extends T[], V extends U[]>(v: V) {
  newFunction<V>(v);
}
)";

    const std::string target = "v.toString()";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction14)
{
    const std::string code = R"(
function F<T, U extends T[], V extends U[]>(v: V) {
  return v.toString();
}
)";
    const std::string expected = R"(
function newFunction<V extends U[]>(v: V): V {
  return v.toString();
}

function F<T, U extends T[], V extends U[]>(v: V) {
  return newFunction<V>(v);
}
)";

    const std::string target = "v.toString()";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction15)
{
    const std::string code = R"(
namespace A {
  let x = 1;
  function foo() {}
  namespace B {
    function a() {
      let y = 5;
      let z = x;
      return foo();
    }
  }
}
)";
    const std::string expected = R"(
namespace A {
  let x = 1;
  function foo() {}
  namespace B {
    function a() {
      return newFunction(x, foo);
    }
  }
}
function newFunction(x: int, foo: (() => undefined)): undefined {
  let y = 5;
  let z = x;
  return foo();
}

)";

    const std::string target = R"(let y = 5;
      let z = x;
      return foo();)";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction16)
{
    const std::string code = R"(
function test() {
  try {}
  finally {
    return 1;
  }
}
)";
    const std::string expected = R"(
function newFunction(): Int {
  return 1;
}

function test() {
  try {}
  finally {
    return newFunction();
  }
}
)";

    const std::string target = "return 1";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction17)
{
    const std::string code = R"(
function F<T, U extends T[], V extends U[]>(v: V) {
  return v.toString();
}
)";
    const std::string expected = R"(
function newFunction<V extends U[]>(v: V): V {
  return v.toString();
}

function F<T, U extends T[], V extends U[]>(v: V) {
  return newFunction<V>(v);
}
)";

    const std::string target = "v.toString()";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction18)
{
    const std::string code = R"(
let s = "123";
console.log(s + "456");
)";
    const std::string expected = R"(
function newFunction(): "456" {
  return "456";
}

let s = "123";
console.log(s + newFunction());
)";

    const std::string target = "\"456\"";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction19)
{
    const std::string code = R"(
class A {}
let a: A = new A();
)";
    const std::string expected = R"(
class A {}
function newFunction(): A {
  return new A();
}

let a: A = newFunction();
)";

    const std::string target = "new A()";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction20)
{
    const auto testCase = GetExtractFunction20Case();
    const std::string code = testCase.code;
    const std::string target = testCase.target;
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    ExpectApplicableAndApply(code, refactorContext, refactorName, globalScopeAction, testCase.expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction21)
{
    const std::string code = R"(
namespace A {
  function foo():String {
    return "123";
  }
  namespace B {
    async function a(z: number) {
      let y = 5;
      await Promise.resolve(z);
      return foo();
    }
  }
}
)";
    const std::string expected = R"(
namespace A {
  function foo():String {
    return "123";
  }
  namespace B {
    async function a(z: number) {
      return await newFunction(z, foo);
    }
  }
}
async function newFunction(z: number, foo: (() => String)): Promise<String> {
  let y = 5;
  await Promise.resolve(z);
  return foo();
}

)";

    const std::string target = R"(let y = 5;
      await Promise.resolve(z);
      return foo();)";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction22)
{
    const auto testCase = GetExtractFunction22Case();
    const std::string code = testCase.code;
    const size_t spanStart = code.find(testCase.target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::char_traits<char>::length(testCase.target);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const bool hasGlobal = std::any_of(applicable.begin(), applicable.end(),
                                       [&](const auto &info) { return info.action.name == globalScopeAction; });
    EXPECT_TRUE(hasGlobal);

    ExpectExtractionApplies(code, refactorContext, refactorName, globalScopeAction, testCase.expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction23)
{
    const auto testCase = GetExtractFunction23Case();
    const std::string code = testCase.extraction.code;
    const size_t spanStart = code.find(testCase.extraction.target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::char_traits<char>::length(testCase.extraction.target);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string encloseScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    std::string namespaceAAction = FindNamespaceActionName(applicable, encloseScopeAction, testCase.namespaceDesc);
    EXPECT_FALSE(namespaceAAction.empty());

    ExpectExtractionApplies(code, refactorContext, refactorName, namespaceAAction, testCase.extraction.expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction24)
{
    const std::string code = R"(
namespace A {
  function foo():int {
    return 5;
  }
  namespace B {
    async function a(z: number) {
      let y = 5;
      await Promise.resolve(z);
      return foo();
    }
  }
}
)";
    const std::string target = R"(let y = 5;
      await Promise.resolve(z);
      return foo();)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string encloseScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    auto applicable = GetApplicableRefactorsOrAssert(refactorContext);
    std::string namespaceBAction = FindNamespaceActionName(applicable, encloseScopeAction, "namespace 'B'");
    EXPECT_FALSE(namespaceBAction.empty());
    ExpectExtractionApplies(code, refactorContext, refactorName, namespaceBAction, R"(
namespace A {
  function foo():int {
    return 5;
  }
  namespace B {
    async function a(z: number) {
      return await newFunction(z);
    }
    async function newFunction(z: number): Promise<Int> {
      let y = 5;
      await Promise.resolve(z);
      return foo();
    }
  }
}
)");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction25)
{
    const std::string code = R"(
function F<T>() {
  let t: T;
}
)";
    const std::string expected = R"(
function newFunction<T>() {
  let t: T;
}

function F<T>() {
  newFunction<T>();
}
)";

    const std::string target = "let t: T;";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction26)
{
    const std::string code = R"(
function F() {
  let a: string | number = "123";
}
)";
    const std::string expected = R"(
function newFunction(): string | number {
  return "123";
}

function F() {
  let a: string | number = newFunction();
}
)";

    const std::string target = "\"123\"";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction27)
{
    const std::string code = R"(
function F<T extends object>(v: T) {
  let out: T = v;
}
)";
    const std::string expected = R"(
function newFunction<T extends object>(v: T): T {
  return v;
}

function F<T extends object>(v: T) {
  let out: T = newFunction<T>(v);
}
)";

    const size_t markerPos = code.find("let out: T = v;");
    ASSERT_NE(markerPos, std::string::npos);
    const size_t spanStart = code.find("v", markerPos);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + 1;

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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction28)
{
    const std::string code = R"(
namespace A {
  interface Hidden {
    x: number;
  }
  class C {
    m() {
      let value: Hidden = { x: 1 };
      return value;
    }
  }
}
)";

    const std::string target = "{ x: 1 }";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const std::string encloseScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    const std::string classScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto applicable = GetApplicableRefactorsOrAssert(refactorContext);
    const bool hasGlobal = HasApplicableAction(applicable, globalScopeAction);
    EXPECT_FALSE(hasGlobal);
    const bool hasEnclose = HasApplicableAction(applicable, encloseScopeAction);
    EXPECT_TRUE(hasEnclose);
    const bool hasClass = HasApplicableAction(applicable, classScopeAction);
    EXPECT_TRUE(hasClass);
    ExpectExtractionApplies(code, refactorContext, refactorName, encloseScopeAction, R"(
namespace A {
  interface Hidden {
    x: number;
  }
  function newFunction(): Hidden {
    return { x: 1 };
  }

  class C {
    m() {
      let value: Hidden = newFunction();
      return value;
    }
  }
}
)");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction29)
{
    const std::string code = R"(
let a = 1;
namespace A {
  let b = 2;
  namespace B {
    let d = 3;
    let e = b + d;
  }
}
)";
    const std::string expected = R"(
let a = 1;
namespace A {
  let b = 2;
  function newFunction(d: int): Int {
    return b + d;
  }

  namespace B {
    let d = 3;
    let e = newFunction(d);
  }
}
)";

    const std::string target = "b + d";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string encloseScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    std::string namespaceAAction;
    for (const auto &info : applicable) {
        if ((info.action.name == encloseScopeAction ||
             info.action.name.rfind(std::string("extract_function_scope_ns_"), 0) == 0) &&
            info.action.description.find("namespace 'A'") != std::string::npos) {
            namespaceAAction = info.action.name;
            break;
        }
    }
    EXPECT_FALSE(namespaceAAction.empty());

    ExpectExtractionApplies(code, refactorContext, refactorName, namespaceAAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction30)
{
    const std::string code = R"(
namespace A {
  export const value: number = 1;
  () => {
    let f: () => number;
    f = (): number => {
      return value;
    }
  }
}
)";
    const std::string expected = R"(
namespace A {
  export const value: number = 1;
  () => {
    let f: () => number;
    f = newFunction(value);
  }
}
function newFunction(value: number): (() => Double) {
  return (): number => {
    return value;
  };
}

)";

    const std::string target = R"(f = (): number => {
      return value;
    })";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction31)
{
    const std::string code = R"(
let a = 1;
let b = a + 1;
)";
    const std::string expected = R"(
function newFunction(): Int {
  return a + 1;
}

let a = 1;
let b = newFunction();
)";

    const std::string target = R"(let b = a + 1)";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction32)
{
    const std::string code = R"(
let a = 1;
let b = a + 1;
)";
    const std::string expected = R"(
function newFunction(): Int {
  return a + 1;
}

let a = 1;
let b = newFunction();
)";

    const std::string target = R"(let b = a + 1;)";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction33)
{
    const std::string code = GetAsyncNamespaceFunctionCode();

    const std::string target = R"(let y = 5;
      if (z) {
        await z1;
      }
      return foo();)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const std::string encloseScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    auto applicable = GetApplicableRefactorsOrAssert(refactorContext);
    const bool hasGlobal = HasApplicableAction(applicable, globalScopeAction);
    const bool hasEnclose = HasApplicableAction(applicable, encloseScopeAction);
    EXPECT_TRUE(hasGlobal);
    EXPECT_TRUE(hasEnclose);
    ExpectExtractionApplies(code, refactorContext, refactorName, encloseScopeAction, R"(
namespace A {
  function foo() {
  }
  namespace B {
    async function a(z: number, z1: number) {
      /*start*/
      return await newFunction(z, z1);/*end*/
    }
    async function newFunction(z: number, z1: number): Promise<undefined> {
      let y = 5;
      if (z) {
        await z1;
      }
      return foo();
    }
  }
}
)");
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction34)
{
    const std::string code = GetAsyncNamespaceFunctionCode();

    const std::string target = R"(let y = 5;
      if (z) {
        await z1;
      }
      return foo();)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string encloseScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto applicable = GetApplicableRefactorsOrAssert(refactorContext);
    std::string namespaceAAction = FindNamespaceActionName(applicable, encloseScopeAction, "namespace 'A'");
    EXPECT_FALSE(namespaceAAction.empty());
    ExpectExtractionApplies(code, refactorContext, refactorName, namespaceAAction, R"(
namespace A {
  function foo() {
  }
  namespace B {
    async function a(z: number, z1: number) {
      /*start*/
      return await newFunction(z, z1);/*end*/
    }
  }
  async function newFunction(z: number, z1: number): Promise<undefined> {
    let y = 5;
    if (z) {
      await z1;
    }
    return foo();
  }
}
)");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction35)
{
    const std::string code = GetAsyncNamespaceFunctionCode();

    const std::string target = R"(let y = 5;
      if (z) {
        await z1;
      }
      return foo();)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    ExpectApplicableAndApply(code, refactorContext, refactorName, globalScopeAction, R"(
namespace A {
  function foo() {
  }
  namespace B {
    async function a(z: number, z1: number) {
      /*start*/
      return await newFunction(z, z1, foo);/*end*/
    }
  }
}
async function newFunction(z: number, z1: number, foo: (() => undefined)): Promise<undefined> {
  let y = 5;
  if (z) {
    await z1;
  }
  return foo();
}

)");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction36)
{
    const std::string code = GetGlobalExportFunctionCode();

    const std::string target = R"(let y = 5;
      let z = x;
      a = y;
      foo();)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    ExpectApplicableAndApply(code, refactorContext, refactorName, globalScopeAction, R"(
namespace A {
  let x = 1;
  export function foo() {
  }
  namespace B {
    function a() {
      let a = 1;
      a = newFunction(x, a);
    }
  }
}
function newFunction(x: int, a: int): Int {
  let y = 5;
  let z = x;
  a = y;
  A.foo();
  return a;
}

)");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction37)
{
    const std::string code = GetExportedNamespaceFunctionCode();

    const std::string target = R"(let y = 5;
      let z = x;
      a = y;
      return C.foo();)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    ExpectApplicableAndApply(code, refactorContext, refactorName, globalScopeAction, R"(
namespace A {
  let x = 1;
  export namespace C {
    export function foo() {
    }
  }
  namespace B {
    function a() {
      let a = 1;
      return newFunction(x, a);
    }
  }
}
function newFunction(x: int, a: int): undefined {
  let y = 5;
  let z = x;
  a = y;
  return A.C.foo();
}

)");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction38)
{
    const std::string code = R"(
// a
let q = /*b*/ //c
  /*d*/ /*start*/1 /*e*/ //f
    /*g*/ + /*h*/ //i
    /*j*/ 2/*end*/ /*k*/ //l
/*m*/; /*n*/ //o
)";
    const std::string expected = R"(
// a
let q = /*b*/ //c
  /*d*/ /*start*/newFunction()/*end*/ /*k*/ //l
/*m*/; /*n*/ //o

function newFunction(): Int {
  return 1 /*e*/ //f
    /*g*/ + /*h*/ //i
    /*j*/ 2;
}

)";

    const std::string target = R"(1 /*e*/ //f
    /*g*/ + /*h*/ //i
    /*j*/ 2)";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction39)
{
    const std::string code = R"(
namespace X {
  export const j: int = 10;
  export const y: int = /*start*/j * j/*end*/;
}
)";
    const std::string expected = R"(
function newFunction(): int {
  return X.j * X.j;
}

namespace X {
  export const j: int = 10;
  export const y: int = /*start*/newFunction()/*end*/;
}
)";

    const std::string target = "j * j";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction40)
{
    const std::string code = R"(
const x = 1;
"hello";
x;
)";
    const std::string expected = R"(
function newFunction(): int {
  const x = 1;
  "hello";
  return x;
}

const x = newFunction();
x;
)";
    const std::string target = R"(const x = 1;
"hello";)";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction41)
{
    const std::string code = R"(
const x: number = 1;
"hello";
x;
)";
    const std::string expected = R"(
function newFunction(): number {
  const x: number = 1;
  "hello";
  return x;
}

const x: number = newFunction();
x;
)";
    const std::string target = R"(const x: number = 1;
"hello";)";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction42)
{
    const std::string code = R"(
function f() {
  let a = 1;
  const x = 1;
  let y = 2;
  a++;
  a; x; y;
}
)";
    const std::string target = R"(const x = 1;
  let y = 2;
  a++;)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const bool hasGlobal = std::any_of(applicable.begin(), applicable.end(),
                                       [&](const auto &info) { return info.action.name == globalScopeAction; });
    EXPECT_FALSE(hasGlobal);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction43)
{
    const std::string code = R"(
namespace A {
  let a = 1;
  let x = a + 1;
}
)";
    const std::string expected = R"(
function newFunction(a: int): Int {
  return a + 1;
}

namespace A {
  let a = 1;
  let x = newFunction(a);
}
)";
    const std::string target = R"(a + 1)";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction44)
{
    const std::string code = R"(
import {A} from './a';
)";
    const std::string target = R"(import {A} from './a';)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const std::string encloseScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    const std::string classScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const bool hasGlobal = std::any_of(applicable.begin(), applicable.end(),
                                       [&](const auto &info) { return info.action.name == globalScopeAction; });
    const bool hasEnclose = std::any_of(applicable.begin(), applicable.end(),
                                        [&](const auto &info) { return info.action.name == encloseScopeAction; });
    const bool hasClass = std::any_of(applicable.begin(), applicable.end(),
                                      [&](const auto &info) { return info.action.name == classScopeAction; });
    const bool hasNamespaceEnclose = std::any_of(applicable.begin(), applicable.end(), [&](const auto &info) {
        return info.action.name.rfind("extract_function_scope_ns_", 0) == 0;
    });
    EXPECT_FALSE(hasGlobal);
    EXPECT_FALSE(hasEnclose);
    EXPECT_FALSE(hasClass);
    EXPECT_FALSE(hasNamespaceEnclose);

    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, globalScopeAction);
    EXPECT_EQ(edits, nullptr);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction45)
{
    const std::string code = R"(
namespace A {
  let intVar: int = 10;
}
)";
    const std::string expected = R"(
namespace A {
  function newFunction(): int {
    return 10;
  }

  let intVar: int = newFunction();
}
)";
    const std::string target = R"(let intVar: int = 10;)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string encloseScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    std::string namespaceAAction;
    for (const auto &info : applicable) {
        if ((info.action.name == encloseScopeAction ||
             info.action.name.rfind(std::string("extract_function_scope_ns_"), 0) == 0) &&
            info.action.description.find("namespace 'A'") != std::string::npos) {
            namespaceAAction = info.action.name;
            break;
        }
    }
    EXPECT_FALSE(namespaceAAction.empty());

    ExpectExtractionApplies(code, refactorContext, refactorName, namespaceAAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction46)
{
    const std::string code = R"(
interface I {x: int};
namespace A {
  let y = 1;
  class C {
    b() {}
    a() {
      let z = 1;
      /*start*/let a1: I = { x: 1 };
      y = 10;
      z = 42;
      this.b();
      return a1.x + 10;/*end*/
    }
  }
}
)";
    const std::string target = R"(let a1: I = { x: 1 };
      y = 10;
      z = 42;
      this.b();
      return a1.x + 10;)";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const bool hasGlobal = std::any_of(applicable.begin(), applicable.end(),
                                       [&](const auto &info) { return info.action.name == globalScopeAction; });
    EXPECT_FALSE(hasGlobal);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction47)
{
    const std::string code = R"(
'use static'

/*123*/const x = 1;
hello;
x;
)";
    const std::string expected = R"(
'use static'

function newFunction(): int {
  const x = 1;
  hello;
  return x;
}

/*123*/const x = newFunction();
x;
)";
    const std::string target = R"(const x = 1;
hello;)";
    const size_t spanStart = code.find(target);
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

TEST_F(LspExtrSymblGetEditsTestsFunction, ExtractFunction48)
{
    const std::string code = R"(
let str: string = 123 - 23 + 'abc';
)";
    const std::string expected = R"(
function newFunction(): String {
  return 123 - 23 + 'abc';
}

let str: string = newFunction();
)";
    const std::string target = R"(123 - 23 + 'abc')";
    const size_t spanStart = code.find(target);
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

}  // namespace
