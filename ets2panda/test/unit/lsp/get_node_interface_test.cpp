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

class LspGetNodeInterfaceTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        sourceCode_ = R"(export namespace a {
    export interface User {
        bar(): void;
    }
    export namespace b {
        interface Client extends a.User {}
    }
}
interface Worker extends a.User {})";
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
        contexts_ = initializer.CreateContext("GetNodeInterfaceTest.ets", ES2PANDA_STATE_PARSED, sourceCode_.c_str());
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetNodeInterfaceTests, GetTsInterfaceDeclarationNonExistentTest)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string interfaceName = "NonExistentInterface";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {interfaceName, ark::es2panda::ir::AstNodeType::TS_INTERFACE_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_EQ(extractedText.find(interfaceName), std::string::npos);
}

TEST_F(LspGetNodeInterfaceTests, GetTsInterfaceDeclarationTest)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string interfaceName = "User";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {interfaceName, ark::es2panda::ir::AstNodeType::TS_INTERFACE_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(interfaceName), std::string::npos);
}

TEST_F(LspGetNodeInterfaceTests, GetTsInterfaceDeclarationExtendsTest)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string interfaceName = "Client";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {interfaceName, ark::es2panda::ir::AstNodeType::TS_INTERFACE_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(interfaceName), std::string::npos);
}

TEST_F(LspGetNodeInterfaceTests, GetTsInterfaceDeclarationExtendsTest1)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string interfaceName = "Worker";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {interfaceName, ark::es2panda::ir::AstNodeType::TS_INTERFACE_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(interfaceName), std::string::npos);
}
}  // namespace