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

#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/get_signature.h"
#include <gtest/gtest.h>
#include <cstddef>
#include "test/unit/lsp/lsp_api_test.h"
namespace {

using ark::es2panda::lsp::Initializer;

class LspGetSignatureTests1 : public LSPAPITests {};

TEST_F(LspGetSignatureTests1, OverloadActiveItemEmptyArgs)
{
    const std::string source = R"(declare function greet(a: number, b: string): void;
declare function greet(a: number): void;
greet())";
    Initializer initializer = Initializer();
    auto ctx =
        initializer.CreateContext("get_signature_test_1_overload_empty.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    const size_t offset = source.find("greet()") + 6;  // right of '('
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedItems = 2;
    const size_t expectedArgCount = 2;
    const size_t expectedArgIndex = 0;
    const size_t expectedSpanStart = 99;
    const size_t expectedSpanLength = 0;
    const size_t typeIndex = 3;
    ASSERT_EQ(result.GetItems().size(), expectedItems);
    ASSERT_EQ(result.GetArgumentCount(), expectedArgCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedArgIndex);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
    // first overload: greet(a: number, b: string)
    ASSERT_EQ(result.GetItems().at(0).GetPrefixDisplayParts().at(0).GetText(), "greet");
    ASSERT_EQ(result.GetItems().at(0).GetPrefixDisplayParts().at(0).GetKind(), "functionName");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().size(), 2U);
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(0).GetDisplayParts().at(0).GetText(), "a");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(0).GetDisplayParts().at(typeIndex).GetText(), "number");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(1).GetDisplayParts().at(0).GetText(), "b");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(1).GetDisplayParts().at(typeIndex).GetText(), "string");
    // second overload: greet(a: number)
    ASSERT_EQ(result.GetItems().at(1).GetPrefixDisplayParts().at(0).GetText(), "greet");
    ASSERT_EQ(result.GetItems().at(1).GetParameters().size(), 1U);
    ASSERT_EQ(result.GetItems().at(1).GetParameters().at(0).GetDisplayParts().at(typeIndex).GetText(), "number");
}

TEST_F(LspGetSignatureTests1, OverloadActiveItemSecondParam)
{
    const std::string source = R"(declare function greet(a: number, b: string): void;
declare function greet(a: number): void;
greet(1,))";
    Initializer initializer = Initializer();
    auto ctx =
        initializer.CreateContext("get_signature_test_1_overload_second.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    const size_t offset = source.find("greet(1,") + 8;  // right of ','
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedItems = 2;
    const size_t expectedArgCount = 2;
    const size_t expectedArgIndex = 1;
    const size_t expectedSpanStart = 99;
    const size_t expectedSpanLength = 2;
    ASSERT_EQ(result.GetItems().size(), expectedItems);
    ASSERT_EQ(result.GetArgumentCount(), expectedArgCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedArgIndex);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
}

TEST_F(LspGetSignatureTests1, NestedCallActiveParameter)
{
    const std::string source = R"(function outer(x: number): number {
  return x;
}
function inner(a: number, b: number): number {
  return a + b;
}
outer(inner(1,)))";
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext("get_signature_test_1_nested.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    const size_t offset = source.find("inner(1,") + 8;  // right of ',' inside inner call
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedItems = 1;
    const size_t expectedArgCount = 2;
    const size_t expectedArgIndex = 1;
    const size_t expectedSpanStart = 127;
    const size_t expectedSpanLength = 2;
    const size_t typeIndex = 3;
    ASSERT_EQ(result.GetItems().size(), expectedItems);
    ASSERT_EQ(result.GetArgumentCount(), expectedArgCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedArgIndex);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
    ASSERT_EQ(result.GetItems().at(0).GetPrefixDisplayParts().at(0).GetText(), "inner");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().size(), 2U);
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(0).GetDisplayParts().at(typeIndex).GetText(), "number");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(1).GetDisplayParts().at(typeIndex).GetText(), "number");
}

TEST_F(LspGetSignatureTests1, NestedCallOuterParameter)
{
    const std::string source = R"(function outer(x: number, y: number): number {
  return x + y;
}
function inner(a: number): number {
  return a;
}
outer(inner(1),))";
    Initializer initializer = Initializer();
    auto ctx =
        initializer.CreateContext("get_signature_test_1_nested_outer.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    const size_t offset = source.find("outer(inner(1),") + 14;  // right of ',' inside outer call
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedItems = 1;
    const size_t expectedArgCount = 2;
    const size_t expectedArgIndex = 1;
    const size_t expectedSpanStart = 121;
    const size_t expectedSpanLength = 8;
    ASSERT_EQ(result.GetItems().size(), expectedItems);
    ASSERT_EQ(result.GetArgumentCount(), expectedArgCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedArgIndex);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
    ASSERT_EQ(result.GetItems().at(0).GetPrefixDisplayParts().at(0).GetText(), "outer");
}

TEST_F(LspGetSignatureTests1, TrailingCommaSecondArgument)
{
    const std::string source = R"(function add(a: number, b: number, c: number): number {
  return a + b + c;
}
add(1, 2,))";
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext("get_signature_test_1_trailing.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    const size_t offset = source.find("add(1, 2,") + 9;  // right of trailing ','
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedItems = 1;
    const size_t expectedArgCount = 3;
    const size_t expectedArgIndex = 2;
    const size_t expectedSpanStart = 82;
    const size_t expectedSpanLength = 5;
    ASSERT_EQ(result.GetItems().size(), expectedItems);
    ASSERT_EQ(result.GetArgumentCount(), expectedArgCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedArgIndex);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
}

TEST_F(LspGetSignatureTests1, IncompleteCallEmptyArgs)
{
    const std::string source = R"ETS(function add(a: number, b: number): number {
  return a + b;
}
add(
)ETS";
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext("get_signature_test_1_incomplete.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    const size_t offset = source.find("add(\n") + 4;  // right of '('
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedItems = 1;
    const size_t expectedArgCount = 2;
    const size_t expectedArgIndex = 0;
    const size_t expectedSpanStart = 67;
    const size_t expectedSpanLength = 0;
    const size_t typeIndex = 3;
    ASSERT_EQ(result.GetItems().size(), expectedItems);
    ASSERT_EQ(result.GetArgumentCount(), expectedArgCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedArgIndex);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
    ASSERT_EQ(result.GetItems().at(0).GetPrefixDisplayParts().at(0).GetText(), "add");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().size(), 2U);
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(0).GetDisplayParts().at(typeIndex).GetText(), "number");
    ASSERT_EQ(result.GetItems().at(0).GetParameters().at(1).GetDisplayParts().at(typeIndex).GetText(), "number");
}

TEST_F(LspGetSignatureTests1, IncompleteCallFirstArgument)
{
    const std::string source = R"ETS(function add(a: number, b: number): number {
  return a + b;
}
add(1,
)ETS";
    Initializer initializer = Initializer();
    auto ctx =
        initializer.CreateContext("get_signature_test_1_incomplete_arg.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    const size_t offset = source.find("add(1,\n") + 6;  // right of ','
    auto result = ark::es2panda::lsp::GetSignature(ctx, offset);
    initializer.DestroyContext(ctx);
    const size_t expectedItems = 1;
    const size_t expectedArgCount = 2;
    const size_t expectedArgIndex = 1;
    const size_t expectedSpanStart = 67;
    const size_t expectedSpanLength = 2;
    ASSERT_EQ(result.GetItems().size(), expectedItems);
    ASSERT_EQ(result.GetArgumentCount(), expectedArgCount);
    ASSERT_EQ(result.GetArgumentIndex(), expectedArgIndex);
    ASSERT_EQ(result.GetApplicableSpan().start, expectedSpanStart);
    ASSERT_EQ(result.GetApplicableSpan().length, expectedSpanLength);
}

}  // namespace
