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

class LspGetClassDeclarationTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        sourceCode_ = R"(
            class Base {
                prop1: number = 1;
            }
            
            class Derived extends Base {
                prop2: string = "test";
            }
            
            class StaticClass {
                static staticMethod() {}
            }
            
            class PrivateClass {
                private privProp: boolean = true;
            }
            
            class EmptyClass {}
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
            initializer.CreateContext("GetClassDeclarationTest.ets", ES2PANDA_STATE_PARSED, sourceCode_.c_str());
    }

    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetClassDeclarationTests, GetBaseClassDeclaration)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string className = "Base";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {className, ark::es2panda::ir::AstNodeType::CLASS_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(className), std::string::npos);
}

TEST_F(LspGetClassDeclarationTests, GetDerivedClassDeclaration)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string className = "Derived";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {className, ark::es2panda::ir::AstNodeType::CLASS_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(className), std::string::npos);
}

TEST_F(LspGetClassDeclarationTests, GetStaticClassDeclaration)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string className = "StaticClass";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {className, ark::es2panda::ir::AstNodeType::CLASS_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(className), std::string::npos);
}

TEST_F(LspGetClassDeclarationTests, GetPrivateClassDeclaration)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string className = "PrivateClass";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {className, ark::es2panda::ir::AstNodeType::CLASS_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(className), std::string::npos);
}

TEST_F(LspGetClassDeclarationTests, GetEmptyClassDeclaration)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string className = "EmptyClass";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {className, ark::es2panda::ir::AstNodeType::CLASS_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(className), std::string::npos);
}

TEST_F(LspGetClassDeclarationTests, GetNonExistentClass)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string className = "NonExistentClass";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {className, ark::es2panda::ir::AstNodeType::CLASS_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    ASSERT_EQ(res.start, static_cast<size_t>(0));
    ASSERT_EQ(res.length, static_cast<size_t>(0));
}
}  // namespace