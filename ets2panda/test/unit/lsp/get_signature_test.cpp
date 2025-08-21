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

#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/get_signature.h"
#include <gtest/gtest.h>
#include <cstddef>
#include "test/unit/lsp/lsp_api_test.h"
namespace {

using ark::es2panda::lsp::Initializer;

class LspGetSignatureTests : public LSPAPITests {};

TEST_F(LspGetSignatureTests, LspTests)
{
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext("get_signature_test1.ets", ES2PANDA_STATE_CHECKED,
                                         R"(function add(a:number, b:Double): double {
  return a + b;
}
add())");
    const LSPAPI *lspApi = GetImpl();
    const size_t offset = 65;
    lspApi->getSignatureHelpItems(ctx, offset);
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedCount = 2;
    const size_t expectedIndex = 0;
    const size_t expectedSpanLength = 0;
    const size_t expectedSpanStart = 65;
    const size_t typeIndex = 3;
    const size_t returnTypeIndex = 4;
    ASSERT_EQ(result.GetArgumentCount(), expectedCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedIndex);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetItems().at(0).GetPrefixDisplayParts().at(0).GetText(), "add");
    ASSERT_EQ(result.GetItems().at(0).GetPrefixDisplayParts().at(0).GetKind(), "functionName");
    ASSERT_EQ(result.GetItems().at(0).GetSuffixDisplayParts().at(returnTypeIndex).GetText(), "double");
    ASSERT_EQ(result.GetItems().at(0).GetSuffixDisplayParts().at(returnTypeIndex).GetKind(), "keyword");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(0).GetDisplayParts().at(0).GetText(), "a");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(0).GetDisplayParts().at(typeIndex).GetText(), "number");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(1).GetDisplayParts().at(0).GetText(), "b");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(1).GetDisplayParts().at(typeIndex).GetText(), "Double");
}

TEST_F(LspGetSignatureTests, LspTests2)
{
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext("get_signature_test2.ets", ES2PANDA_STATE_CHECKED,
                                         R"(function add(a:number, b:number): number {
  return a + b;
}
add(11,))");
    const LSPAPI *lspApi = GetImpl();
    const size_t offset = 68;
    lspApi->getSignatureHelpItems(ctx, offset);
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedCount = 2;
    const size_t expectedIndex = 1;
    const size_t expectedSpanLength = 3;
    const size_t expectedSpanStart = 65;
    ASSERT_EQ(result.GetArgumentCount(), expectedCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedIndex);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
}

TEST_F(LspGetSignatureTests, LspTests3)
{
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext("get_signature_test3.ets", ES2PANDA_STATE_CHECKED,
                                         R"(function add(a, b) {
  return a + b;
}
add(11,))");
    const LSPAPI *lspApi = GetImpl();
    const size_t offset = 46;
    lspApi->getSignatureHelpItems(ctx, offset);
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedCount = 2;
    const size_t expectedIndex = 1;
    const size_t expectedSpanLength = 3;
    const size_t expectedSpanStart = 43;
    const size_t typeIndex = 3;
    const size_t returnTypeIndex = 4;
    ASSERT_EQ(result.GetArgumentCount(), expectedCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedIndex);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(0).GetDisplayParts().at(typeIndex).GetText(), "*ERROR_TYPE*");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(1).GetDisplayParts().at(typeIndex).GetText(), "*ERROR_TYPE*");
    ASSERT_EQ(result.GetItems().at(0).GetSuffixDisplayParts().at(returnTypeIndex).GetText(), "");
}

TEST_F(LspGetSignatureTests, LspTests4)
{
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext("get_signature_test4.ets", ES2PANDA_STATE_CHECKED,
                                         R"(declare function Foo(a: number, b:string):void;
declare function Foo(a: number):void;
Foo())");
    const LSPAPI *lspApi = GetImpl();
    const size_t offset = 90;
    lspApi->getSignatureHelpItems(ctx, offset);
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedCount = 2;
    const size_t expectedIndex = 0;
    const size_t expectedSpanLength = 0;
    const size_t expectedSpanStart = 90;
    const size_t typeIndex = 3;
    ASSERT_EQ(result.GetArgumentCount(), expectedCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedIndex);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(0).GetDisplayParts().at(typeIndex).GetText(), "number");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(1).GetDisplayParts().at(typeIndex).GetText(), "string");
    ASSERT_EQ(result.GetItems().at(1).GetParameters().at(0).GetDisplayParts().at(typeIndex).GetText(), "number");
}

TEST_F(LspGetSignatureTests, LspTests5)
{
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext("get_signature_test5.ets", ES2PANDA_STATE_CHECKED,
                                         R"(class A{
    Foo(a: number) {}
}
let a = new A();
a.Foo())");
    const LSPAPI *lspApi = GetImpl();
    const size_t offset = 56;
    lspApi->getSignatureHelpItems(ctx, offset);
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedCount = 1;
    const size_t expectedIndex = 0;
    const size_t expectedSpanLength = 0;
    const size_t expectedSpanStart = 56;
    const size_t typeIndex = 3;
    ASSERT_EQ(result.GetArgumentCount(), expectedCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedIndex);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(0).GetDisplayParts().at(typeIndex).GetText(), "number");
}

TEST_F(LspGetSignatureTests, LspTests6)
{
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext("get_signature_test4.ets", ES2PANDA_STATE_CHECKED,
                                         R"(declare function Foo(a: number, b:string):void;
declare function Foo(a: number):void;
Foo(1,))");
    const LSPAPI *lspApi = GetImpl();
    const size_t offset = 92;
    lspApi->getSignatureHelpItems(ctx, offset);
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedCount = 2;
    const size_t expectedIndex = 1;
    const size_t expectedSpanLength = 2;
    const size_t expectedSpanStart = 90;
    const size_t typeIndex = 3;
    ASSERT_EQ(result.GetArgumentCount(), expectedCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedIndex);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(0).GetDisplayParts().at(typeIndex).GetText(), "number");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(1).GetDisplayParts().at(typeIndex).GetText(), "string");
    ASSERT_EQ(result.GetItems().at(1).GetParameters().at(0).GetDisplayParts().at(typeIndex).GetText(), "number");
}

TEST_F(LspGetSignatureTests, LspTests7)
{
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext("get_signature_test4.ets", ES2PANDA_STATE_CHECKED,
                                         R"(class Person {
  name: string = '';
  age: name = 0;
  constructor(name: string, age: number) {
    this.name = name;
    this.age = age;
  }
  introduce(name: string, age: number): void {}
}
let p: Person = new Person("张三", 18);
p.introduce())");
    const LSPAPI *lspApi = GetImpl();
    const size_t offset = 246;
    lspApi->getSignatureHelpItems(ctx, offset);
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedCount = 2;
    const size_t expectedIndex = 0;
    const size_t expectedSpanLength = 0;
    const size_t expectedSpanStart = 246;
    const size_t typeIndex = 3;
    ASSERT_EQ(result.GetArgumentCount(), expectedCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedIndex);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(0).GetDisplayParts().at(typeIndex).GetText(), "string");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(1).GetDisplayParts().at(typeIndex).GetText(), "number");
}

TEST_F(LspGetSignatureTests, LspTests8)
{
    const std::string testString = "testNoLeftBracketOrNoDot";
    const size_t testPos = 100;
    size_t result = ark::es2panda::lsp::FindFunctionNameStartNested(testString, testPos);
    ASSERT_EQ(result, 0);
    result = ark::es2panda::lsp::FindClassNameStart(testString, testPos);
    ASSERT_EQ(result, 0);
    const size_t testPos1 = 10;
    result = ark::es2panda::lsp::FindFunctionNameStartNested(testString, testPos1);
    ASSERT_EQ(result, 0);
    result = ark::es2panda::lsp::FindClassNameStart(testString, testPos1);
    ASSERT_EQ(result, 0);

    const std::string testString1 = "(testNoFuncName";
    const size_t testPos2 = 10;
    result = ark::es2panda::lsp::FindFunctionNameStartNested(testString1, testPos2);
    ASSERT_EQ(result, 0);

    const std::string testString2 = ".testNoClassName";
    result = ark::es2panda::lsp::FindClassNameStart(testString2, testPos2);
    ASSERT_EQ(result, 0);
}

}  // namespace