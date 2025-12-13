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

class LspGetNodeStringLiteralTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        sourceCode_ = R"(
namespace A {
   let a: 'string';
   type Status = "active" | "inactive" | "pending";
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
        contexts_ =
            initializer.CreateContext("GetNodeStringLiteralTest.ets", ES2PANDA_STATE_PARSED, sourceCode_.c_str());
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetNodeStringLiteralTests, GetStringLiteralTypeTest)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "a";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::ETS_STRING_LITERAL_TYPE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeStringLiteralTests, GetStringLiteralTypeTest1)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Status";
    const std::string value = "active";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::ETS_STRING_LITERAL_TYPE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(value), std::string::npos);
}

TEST_F(LspGetNodeStringLiteralTests, GetStringLiteralTypeTest2)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Status";
    const std::string value = "inactive";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::ETS_STRING_LITERAL_TYPE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(value), std::string::npos);
}

TEST_F(LspGetNodeStringLiteralTests, GetStringLiteralTypeTest3)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Status";
    const std::string value = "pending";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::ETS_STRING_LITERAL_TYPE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(value), std::string::npos);
}

TEST_F(LspGetNodeStringLiteralTests, GetStringLiteralTypeTest4)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(
namespace A {
   type Ark = "active1";
}
)";
    es2panda_Context *contexts =
        initializer.CreateContext("GetNodeStringLiteralTest4.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Ark";
    const std::string value = "active1";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::ETS_STRING_LITERAL_TYPE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeStringLiteralTests, GetStringLiteralTypeTest5)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(
namespace A {
   function expectOne(arg: "1"): void {
      let b: '2';
   }
}
)";
    es2panda_Context *contexts =
        initializer.CreateContext("GetNodeStringLiteralTest5.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "arg";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::ETS_STRING_LITERAL_TYPE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    initializer.DestroyContext(contexts);
}

}  // namespace