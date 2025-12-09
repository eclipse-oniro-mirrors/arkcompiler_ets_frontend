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

class LspGetNodeInfoTypeAliasDeclarationTests : public LSPAPITests {
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
        contexts_ = initializer.CreateContext("LspGetNodeInfosTypeAliasDeclarationTest.ets", ES2PANDA_STATE_PARSED,
                                              R"('use static'
type ID = string | number;
type Status = "active" | "inactive" | "pending";
type List<T> = T[];
namespace Models {
    namespace Utils {
        type Formatter<T> = (input: T) => string;
    }
}
)");
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetNodeInfoTypeAliasDeclarationTests, GetNodeInfoTypeAliasDeclarationTest1)
{
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getNodeInfosByDefinitionData(nullptr, 0);
    ASSERT_TRUE(result.empty());
}

TEST_F(LspGetNodeInfoTypeAliasDeclarationTests, GetNodeInfoTypeAliasDeclarationTest2)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t errorOffset = 220;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, errorOffset);
    ASSERT_TRUE(result.empty());
}

TEST_F(LspGetNodeInfoTypeAliasDeclarationTests, GetNodeInfoTypeAliasDeclarationTest3)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 19;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    const size_t expectedSize = 2;
    std::vector<NodeInfo> expectedResult = {{"ID", ark::es2panda::ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION},
                                            {"ID", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfoTypeAliasDeclarationTests, GetNodeInfoTypeAliasDeclarationTest4)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 47;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    const size_t expectedSize = 2;
    std::vector<NodeInfo> expectedResult = {{"Status", ark::es2panda::ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION},
                                            {"Status", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfoTypeAliasDeclarationTests, GetNodeInfoTypeAliasDeclarationTest5)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 98;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    const size_t expectedSize = 1;
    std::vector<NodeInfo> expectedResult = {{"List", ark::es2panda::ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfoTypeAliasDeclarationTests, GetNodeInfoTypeAliasDeclarationTest6)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 169;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    const size_t expectedSize = 2;
    std::vector<NodeInfo> expectedResult = {{"Formatter", ark::es2panda::ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION},
                                            {"Formatter", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

}  // namespace