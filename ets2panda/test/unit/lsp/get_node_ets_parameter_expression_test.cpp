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

class LspGetNodeEtsParameterExpressionTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        sourceCode_ = R"(
function greetTest(name: string, age: number): void {
}
const addTest = (a: number, b: number): number => a + b;

class Person {
    introduce(firstName: string, lastName: string): string {
        return `I am`;
    }
}

interface MathOperation {
    calculate(x: number, y: number): number;
}
type BinaryFunction = (left: any, right: any) => boolean;
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
        contexts_ = initializer.CreateContext("GetNodeEtsParameterExpressionTest.ets", ES2PANDA_STATE_PARSED,
                                              sourceCode_.c_str());
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetNodeEtsParameterExpressionTests, GetEtsParameterExpressionTest)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "name";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::ETS_PARAMETER_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeEtsParameterExpressionTests, GetEtsParameterExpressionTest1)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "age";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::ETS_PARAMETER_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeEtsParameterExpressionTests, GetEtsParameterExpressionTest2)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "a";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::ETS_PARAMETER_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeEtsParameterExpressionTests, GetEtsParameterExpressionTest3)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "b";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::ETS_PARAMETER_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeEtsParameterExpressionTests, GetEtsParameterExpressionTest4)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "firstName";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::ETS_PARAMETER_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeEtsParameterExpressionTests, GetEtsParameterExpressionTest5)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "lastName";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::ETS_PARAMETER_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeEtsParameterExpressionTests, GetEtsParameterExpressionTest6)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "x";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::ETS_PARAMETER_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeEtsParameterExpressionTests, GetEtsParameterExpressionTest7)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "y";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::ETS_PARAMETER_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeEtsParameterExpressionTests, GetEtsParameterExpressionTest8)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "left";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::ETS_PARAMETER_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeEtsParameterExpressionTests, GetEtsParameterExpressionTest9)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "right";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::ETS_PARAMETER_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

}  // namespace