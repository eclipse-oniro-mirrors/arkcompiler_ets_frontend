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

#include <cstddef>
#include <cstdio>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/get_name_or_dotted_name_span.h"

namespace {
const size_t IDENTIFIER_ONLY_IDX = 24;
const size_t PROPERTY_ACCESS_IDX = 36;
const size_t FULL_DOTTED_EXPR_IDX = 56;
const size_t INVALID_TOKEN_IDX = 31;
const size_t MODULE_DECLARATION_QUALIFIED_NAME_IDX = 72;

std::string ExtractSpanText(const std::string &source, const TextSpan *span)
{
    if (span == nullptr || span->start + span->length > source.size()) {
        return {};
    }

    return source.substr(span->start, span->length);
}

class GetNameOrDottedNameSpanTests : public LSPAPITests {};
TEST_F(GetNameOrDottedNameSpanTests, GetNameSpanFromSimpleIdentifier)
{
    std::vector<std::string> fileNames = {"get_name_span_from_simple_identifier.ets"};
    std::vector<std::string> fileContents = {
        R"(
function myMethod() {}
myMethod();
        )"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), 1);

    ark::es2panda::lsp::Initializer initializer;
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto content = fileContents[0];
    auto span = ark::es2panda::lsp::GetNameOrDottedNameSpanImpl(context, IDENTIFIER_ONLY_IDX);
    ASSERT_NE(span, nullptr);
    EXPECT_EQ(ExtractSpanText(content, span.get()), "myMethod");
    initializer.DestroyContext(context);
}

TEST_F(GetNameOrDottedNameSpanTests, GetNameSpanFromPropertyAccess)
{
    std::vector<std::string> fileNames = {"get_name_span_from_property_access.ets"};
    std::vector<std::string> fileContents = {
        R"(
class A {
static myMethod() {}
}
A.myMethod();
        )"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), 1);

    ark::es2panda::lsp::Initializer initializer;
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto content = fileContents[0];
    auto span = ark::es2panda::lsp::GetNameOrDottedNameSpanImpl(context, PROPERTY_ACCESS_IDX);
    ASSERT_NE(span, nullptr);
    EXPECT_EQ(ExtractSpanText(content, span.get()), "A.myMethod");
    initializer.DestroyContext(context);
}

TEST_F(GetNameOrDottedNameSpanTests, GetNameSpanFromNestedNamespaceClassCall)
{
    std::vector<std::string> fileNames = {"get_name_span_from_nested_namespace_class_call.ets"};
    std::vector<std::string> fileContents = {
        R"(
namespace A {
export class B {
static foo() {}
}
}
A.B.foo();
        )"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), 1);

    ark::es2panda::lsp::Initializer initializer;
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto content = fileContents[0];
    auto span = ark::es2panda::lsp::GetNameOrDottedNameSpanImpl(context, FULL_DOTTED_EXPR_IDX);
    ASSERT_NE(span, nullptr);
    EXPECT_EQ(ExtractSpanText(content, span.get()), "A.B.foo");
    initializer.DestroyContext(context);
}

TEST_F(GetNameOrDottedNameSpanTests, GetNameSpanFromInvalidToken)
{
    std::vector<std::string> fileNames = {"get_name_span_from_invalid_token.ets"};
    std::vector<std::string> fileContents = {
        R"(
let message = "hello";
console.log(message);
        )"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), 1);

    ark::es2panda::lsp::Initializer initializer;
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto content = fileContents[0];
    auto span = ark::es2panda::lsp::GetNameOrDottedNameSpanImpl(context, INVALID_TOKEN_IDX);
    EXPECT_EQ(span, nullptr);
    initializer.DestroyContext(context);
}

TEST_F(GetNameOrDottedNameSpanTests, GetNameSpanFromQualifiedNamespaceFunction)
{
    std::vector<std::string> fileNames = {"get_name_span_from_qualified_namespace_function.ets"};
    std::vector<std::string> fileContents = {
        R"(
namespace Outer.Inner {
    export function greet() {}
}

Outer.Inner.greet();
        )"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), 1);
    ark::es2panda::lsp::Initializer initializer;
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto content = fileContents[0];
    auto span = ark::es2panda::lsp::GetNameOrDottedNameSpanImpl(context, MODULE_DECLARATION_QUALIFIED_NAME_IDX);
    ASSERT_NE(span, nullptr);
    EXPECT_EQ(ExtractSpanText(content, span.get()), "Outer.Inner.greet");
    initializer.DestroyContext(context);
}
}  // namespace