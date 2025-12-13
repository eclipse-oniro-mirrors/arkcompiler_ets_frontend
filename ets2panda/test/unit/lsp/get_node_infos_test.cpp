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
#include <cstddef>

namespace {
using ark::es2panda::lsp::Initializer;

class LspGetNodeInfosTests : public LSPAPITests {
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
        contexts_ = initializer.CreateContext("LspGetNodeInfosTests.ets", ES2PANDA_STATE_CHECKED, R"('use static'
declare class Foo {
    foo(): void;
    bar() {}
    enum Color {
        Red,
        Green,
        Blue
    };
})");
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetNodeInfosTests, GetNodeInfosTests1)
{
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getNodeInfosByDefinitionData(nullptr, 0);
    ASSERT_TRUE(result.empty());
}

TEST_F(LspGetNodeInfosTests, GetNodeInfosTests2)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t errorOffset = 150;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, errorOffset);
    ASSERT_TRUE(result.empty());
}

TEST_F(LspGetNodeInfosTests, GetNodeInfosTests3)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 27;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    const size_t expectedSize = 3;
    std::vector<NodeInfo> expectedResult = {{"Foo", ark::es2panda::ir::AstNodeType::CLASS_DECLARATION},
                                            {"Foo", ark::es2panda::ir::AstNodeType::CLASS_DEFINITION},
                                            {"Foo", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfosTests, GetMethodDefinitionInfo)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 54;
    const size_t expectedSize = 4;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"Foo", ark::es2panda::ir::AstNodeType::CLASS_DECLARATION},
                                            {"Foo", ark::es2panda::ir::AstNodeType::CLASS_DEFINITION},
                                            {"bar", ark::es2panda::ir::AstNodeType::METHOD_DEFINITION},
                                            {"bar", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfosTests, GetTsEnumDeclarationInfo)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 72;
    const size_t expectedSize = 4;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"Foo", ark::es2panda::ir::AstNodeType::CLASS_DECLARATION},
                                            {"Foo", ark::es2panda::ir::AstNodeType::CLASS_DEFINITION},
                                            {"Color", ark::es2panda::ir::AstNodeType::TS_ENUM_DECLARATION},
                                            {"Color", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfosTests, GetTsEnumMemberInfo)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 101;
    const size_t expectedSize = 5;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"Foo", ark::es2panda::ir::AstNodeType::CLASS_DECLARATION},
                                            {"Foo", ark::es2panda::ir::AstNodeType::CLASS_DEFINITION},
                                            {"Color", ark::es2panda::ir::AstNodeType::TS_ENUM_DECLARATION},
                                            {"Green", ark::es2panda::ir::AstNodeType::TS_ENUM_MEMBER},
                                            {"Green", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}
}  // namespace