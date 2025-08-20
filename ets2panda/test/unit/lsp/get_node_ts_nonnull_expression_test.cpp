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

class LspGetNodeTsNonNullExpressionTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        sourceCode_ = R"(
let x: string | null = null;
x!;
function processString(str: string | null): number {
    return str!.length;
}

function logMessage(msg: string | undefined): void {
    let result = msg!.toLowerCase();
}
interface User {
    profile: "name";
}
function getUserCity(user: User): string {
    return user.profile!;
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
            initializer.CreateContext("GetNodeNonNullExpressionTest.ts", ES2PANDA_STATE_PARSED, sourceCode_.c_str());
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetNodeTsNonNullExpressionTests, GetNonNullExpressionTest)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "x";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::TS_NON_NULL_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeTsNonNullExpressionTests, GetNonNullExpressionTest1)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "str";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::TS_NON_NULL_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeTsNonNullExpressionTests, GetNonNullExpressionTest2)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "msg";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::TS_NON_NULL_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeTsNonNullExpressionTests, GetNonNullExpressionTest3)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "profile";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::TS_NON_NULL_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeTsNonNullExpressionTests, GetNonNullExpressionTest4)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "user";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::TS_NON_NULL_EXPRESSION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

}  // namespace