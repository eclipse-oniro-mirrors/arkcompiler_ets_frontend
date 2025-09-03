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

class LspGetNodeInfoSpreadElementTests : public LSPAPITests {
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
        contexts_ = initializer.CreateContext("GetNodeInfoSpreadElement.ets", ES2PANDA_STATE_PARSED, R"('use static'
const numbers = [1, 2, 3];
const moreNumbers = [0, ...numbers, 4, 5];
const part1 = [1, 2];
const part2 = [3, 4];
const combined = [...part1, ...part2, 5];
const original = [10, 20, 30];
const copy = [...original];
)");
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetNodeInfoSpreadElementTests, GetNodeInfoSpreadElementTest1)
{
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getNodeInfosByDefinitionData(nullptr, 0);
    ASSERT_TRUE(result.empty());
}

TEST_F(LspGetNodeInfoSpreadElementTests, GetNodeInfoSpreadElementTest2)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t errorOffset = 460;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, errorOffset);
    ASSERT_TRUE(result.empty());
}

TEST_F(LspGetNodeInfoSpreadElementTests, GetNodeInfoSpreadElementTest3)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 69;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    const size_t expectedSize = 4;
    std::vector<NodeInfo> expectedResult = {{"moreNumbers", ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATION},
                                            {"moreNumbers", ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATOR},
                                            {"numbers", ark::es2panda::ir::AstNodeType::SPREAD_ELEMENT},
                                            {"numbers", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfoSpreadElementTests, GetNodeInfoSpreadElementTest4)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 152;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    const size_t expectedSize = 4;
    std::vector<NodeInfo> expectedResult = {{"combined", ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATION},
                                            {"combined", ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATOR},
                                            {"part1", ark::es2panda::ir::AstNodeType::SPREAD_ELEMENT},
                                            {"part1", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfoSpreadElementTests, GetNodeInfoSpreadElementTest5)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 162;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    const size_t expectedSize = 4;
    std::vector<NodeInfo> expectedResult = {{"combined", ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATION},
                                            {"combined", ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATOR},
                                            {"part2", ark::es2panda::ir::AstNodeType::SPREAD_ELEMENT},
                                            {"part2", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfoSpreadElementTests, GetNodeInfoSpreadElementTest6)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 223;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    const size_t expectedSize = 4;
    std::vector<NodeInfo> expectedResult = {{"copy", ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATION},
                                            {"copy", ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATOR},
                                            {"original", ark::es2panda::ir::AstNodeType::SPREAD_ELEMENT},
                                            {"original", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

}  // namespace