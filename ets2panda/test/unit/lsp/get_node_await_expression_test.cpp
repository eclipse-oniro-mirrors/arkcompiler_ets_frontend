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
#include "ir/ets/etsReExportDeclaration.h"
#include <gtest/gtest.h>

namespace {
using ark::es2panda::lsp::Initializer;

class LspGetAwaitExpressionTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        sourceCode_ = R"(
async function foo1(p: Promise<Promise<string>>): Promise<void> {
    let result: string = await p;
}

async function foo2(): Promise<void> {
    let x: Promise<number> = Promise.resolve(10.0);
    let result2: number = await x;
}
        )";
        GenerateContexts(*initializer_);
    }

    static void TearDownTestSuite()
    {
        initializer_->DestroyContext(contexts_);
        delete initializer_;
        initializer_ = nullptr;
        sourceCode_ = "";
    }

    static void GenerateContexts(Initializer &initializer)
    {
        contexts_ = initializer.CreateContext("GetAwaitExpressionTest.ets", ES2PANDA_STATE_PARSED, sourceCode_.c_str());
    }

    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetAwaitExpressionTests, GetAnyAwaitExpression)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "p";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::AWAIT_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find("p"), std::string::npos);
}

TEST_F(LspGetAwaitExpressionTests, GetAwaitExpressionByAnotherParameterName)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string paramName = "x";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {paramName, ark::es2panda::ir::AstNodeType::AWAIT_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(paramName), std::string::npos);
}

TEST_F(LspGetAwaitExpressionTests, GetNonExistentAwaitExpression)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string paramName = "nonExistent";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {paramName, ark::es2panda::ir::AstNodeType::AWAIT_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    ASSERT_EQ(res.start, static_cast<size_t>(0));
    ASSERT_EQ(res.length, static_cast<size_t>(0));
}

}  // namespace