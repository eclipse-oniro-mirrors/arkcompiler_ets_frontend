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

#include "ir/astNode.h"
#include "lsp/include/api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include <gtest/gtest.h>

namespace {
using ark::es2panda::lsp::Initializer;

class LspGetNodeInfosExpressionTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        GenerateContexts(*initializer_);
    }

    static void TearDownTestSuite()
    {
        initializer_->DestroyContext(contexts_);
        delete initializer_;
        initializer_ = nullptr;
    }
    static void GenerateContexts(Initializer &initializer)
    {
        contexts_ = initializer.CreateContext("GetNodeInfosExpresion.ts", ES2PANDA_STATE_PARSED, R"(
class Foo {
    bar() {}
}
let foo = new Foo();
foo.bar();

let obj: Record<string, string> = {
    prop: "value"
};
let propName = "prop";
obj[propName];

function a() {
    return "hello";
}
a();

class Parent {
  name: string;
  
  constructor(name: string) {
    this.name = name;
  }
}

class Child extends Parent {
  age: number;
  
  constructor(name: string, age: number) {
    super(name);
    this.age = age;
  }
}
)");
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetNodeInfosExpressionTests, GetMemberExpressionInfo_Error)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t errorOffset = 450;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, errorOffset);
    ASSERT_TRUE(result.empty());
}

TEST_F(LspGetNodeInfosExpressionTests, GetMemberExpressionInfo_PROPERTY)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 53;
    const size_t expectedSize = 3;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"bar", ark::es2panda::ir::AstNodeType::CALL_EXPRESSION},
                                            {"bar", ark::es2panda::ir::AstNodeType::MEMBER_EXPRESSION},
                                            {"bar", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfosExpressionTests, GetMemberExpressionInfo_ELEMENT)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 145;
    const size_t expectedSize = 2;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"propName", ark::es2panda::ir::AstNodeType::MEMBER_EXPRESSION},
                                            {"propName", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfosExpressionTests, GetCallExpression_IdentifierInfo)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 194;
    const size_t expectedSize = 2;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"a", ark::es2panda::ir::AstNodeType::CALL_EXPRESSION},
                                            {"a", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}
}  // namespace