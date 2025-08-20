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

class LspGetNodeTSTypeReferenceTests : public LSPAPITests {};

TEST_F(LspGetNodeTSTypeReferenceTests, GetTSTypeReference_TEST1)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
interface Animal {
  name: string;
}
let animal: Animal = { name: "Generic Animal" };
)";
    es2panda_Context *contexts =
        initializer.CreateContext("TSTypeReferenceTest1.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Animal";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::TS_TYPE_REFERENCE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeTSTypeReferenceTests, GetTSTypeReference_TEST2)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
let name: string = "test";
)";
    es2panda_Context *contexts =
        initializer.CreateContext("TSTypeReferenceTest2.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "string";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::TS_TYPE_REFERENCE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeTSTypeReferenceTests, GetTSTypeReference_TEST3)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
interface Animal {
  name: string;
}
interface Dog extends Animal {
  breed: string;
}
let dog: Dog = { name: "Buddy", breed: "Golden Retriever" };
)";
    es2panda_Context *contexts =
        initializer.CreateContext("TSTypeReferenceTest3.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Dog";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::TS_TYPE_REFERENCE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeTSTypeReferenceTests, GetTSTypeReference_TEST4)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
interface Animal {
  name: string;
}
let animals: Animal[] = [{ name: "Cat" }, { name: "Dog" }];
)";
    es2panda_Context *contexts =
        initializer.CreateContext("TSTypeReferenceTest4.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Animal";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::TS_TYPE_REFERENCE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeTSTypeReferenceTests, GetTSTypeReference_TEST5)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
interface Animal {
  name: string;
}
function feedAnimal(animal: Animal): void {
  console.log("Feeding " + animal.name);
}
)";
    es2panda_Context *contexts =
        initializer.CreateContext("TSTypeReferenceTest5.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Animal";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::TS_TYPE_REFERENCE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeTSTypeReferenceTests, GetTSTypeReference_TEST6)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
interface Animal<T> {
  name: T;
}
let animal: Animal<string> = { name: "Generic Animal" };
)";
    es2panda_Context *contexts =
        initializer.CreateContext("TSTypeReferenceTest6.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Animal";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::TS_TYPE_REFERENCE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeTSTypeReferenceTests, GetTSTypeReference_TEST7)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
interface Animal {
  name: string;
}
type Pet = Animal;
let pet: Pet = { name: "My Pet" };
)";
    es2panda_Context *contexts =
        initializer.CreateContext("TSTypeReferenceTest7.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Pet";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::TS_TYPE_REFERENCE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeTSTypeReferenceTests, GetTSTypeReference_TEST8)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
interface Animal {
  name: string;
}
type Pet = Animal;
let pet: Pet = { name: "My Pet" };
)";
    es2panda_Context *contexts =
        initializer.CreateContext("TSTypeReferenceTest8.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "NotExistingType";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::TS_TYPE_REFERENCE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_EQ(extractedText.find(nodeName), std::string::npos);
    initializer.DestroyContext(contexts);
}
}  // namespace