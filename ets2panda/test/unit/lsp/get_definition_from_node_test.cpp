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

namespace {
using ark::es2panda::lsp::Initializer;

class LspGetDefinitionFromNodeTest : public LSPAPITests {};

TEST_F(LspGetDefinitionFromNodeTest, GetDefinitionFromNode1)
{
    std::vector<std::string> files = {"GetDefinitionFromNode1.ets"};
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

    auto res = lspApi->getDefinitionDataFromNode(context, nodeInfoPtrs);
    initializer.DestroyContext(context);
    size_t const expectedStart = 16;
    size_t const expectedLength = 3;
    ASSERT_EQ(res.start, expectedStart);
    ASSERT_EQ(res.length, expectedLength);
}

}  // namespace