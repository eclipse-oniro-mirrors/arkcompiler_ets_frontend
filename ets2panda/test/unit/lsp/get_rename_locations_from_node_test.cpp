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

#include "lsp/include/api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include <gtest/gtest.h>

using ark::es2panda::lsp::Initializer;

class LspGetRenameLocationsFromNodeTest : public LSPAPITests {};

TEST_F(LspGetRenameLocationsFromNodeTest, GetRenameLocationsFromNode_Test)
{
    std::vector<std::string> files = {"GetRenameLocationsFromNode1.ets"};
    std::vector<std::string> texts = {R"(class Foo {
    Foo = 1;
})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Foo";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::CLASS_PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->findRenameLocationsFromNode(context, nodeInfoPtrs);
    initializer.DestroyContext(context);
    size_t const expectedStart = 16;
    size_t const expectedEnd = 19;
    size_t const expectedLength = 3;
    ASSERT_EQ(res.start, expectedStart);
    ASSERT_EQ(res.line, expectedLength);
    ASSERT_EQ(res.end, expectedEnd);
}

TEST_F(LspGetRenameLocationsFromNodeTest, GetRenameLocationsFromNode_Test1)
{
    std::vector<std::string> files = {"GetRenameLocationsFromNode2.ets"};
    std::vector<std::string> texts = {R"(class Foo {
    Foo () {};
})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Foo";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::METHOD_DEFINITION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->findRenameLocationsFromNode(context, nodeInfoPtrs);
    initializer.DestroyContext(context);
    size_t const expectedStart = 16;
    size_t const expectedEnd = 19;
    size_t const expectedLength = 3;
    ASSERT_EQ(res.start, expectedStart);
    ASSERT_EQ(res.line, expectedLength);
    ASSERT_EQ(res.end, expectedEnd);
}

TEST_F(LspGetRenameLocationsFromNodeTest, GetRenameLocationsFromNode_Test2)
{
    std::vector<std::string> files = {"GetRenameLocationsFromNode2.ets"};
    std::vector<std::string> texts = {R"(interface Foo {
    Foo: number;
})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Foo";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::TS_INTERFACE_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->findRenameLocationsFromNode(context, nodeInfoPtrs);
    initializer.DestroyContext(context);
    size_t const expectedStart = 10;
    size_t const expectedEnd = 13;
    size_t const expectedLength = 3;
    ASSERT_EQ(res.start, expectedStart);
    ASSERT_EQ(res.line, expectedLength);
    ASSERT_EQ(res.end, expectedEnd);
}