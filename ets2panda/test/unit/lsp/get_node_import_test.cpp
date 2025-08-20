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

class LspGetNodeImportTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        sourceCode_ = R"(import { PI } from "std/math";
import * as All from "std/math";
import CommonEventManager from '@ohos.commonEventManager';
export function add(a: number, b: number): number {
  return a + b;
})";
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
        contexts_ = initializer.CreateContext("GetNodeImportTest.ets", ES2PANDA_STATE_CHECKED, sourceCode_.c_str());
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetNodeImportTests, GetImportSpecifier)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "PI";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::IMPORT_SPECIFIER});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeImportTests, GetImportDefaultSpecifier)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "CommonEventManager";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::IMPORT_DEFAULT_SPECIFIER});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

TEST_F(LspGetNodeImportTests, GetImportNamespaceSpecifier)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string moduleName = "All";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {moduleName, ark::es2panda::ir::AstNodeType::IMPORT_NAMESPACE_SPECIFIER});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(moduleName), std::string::npos);
}

}  // namespace