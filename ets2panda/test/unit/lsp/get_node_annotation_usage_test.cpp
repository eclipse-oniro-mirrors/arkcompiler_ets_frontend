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

class LspGetAnnotationUsageTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        sourceCode_ = R"(
            @interface Injectable {
            }
            
            @interface Component {
                name: string;
            }
            
            @interface Log {
                level: string;
            }
            
            @interface Deprecated {
            }

            @Injectable
            class DatabaseService {
                connect() {
                    return "Connected to DB";
                }
            }
            
            @Component({name: "test"})
            class TestComponent {
                @Log({level: "info"})
                doSomething() {
                    // ...
                }
                
                @Deprecated
                oldMethod() {
                    // ...
                }
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
        contexts_ = initializer.CreateContext("GetAnnotationUsageTest.ets", ES2PANDA_STATE_PARSED, sourceCode_.c_str());
    }

    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetAnnotationUsageTests, GetClassAnnotationUsage)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string annotationName = "Injectable";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {annotationName, ark::es2panda::ir::AstNodeType::ANNOTATION_USAGE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(annotationName), std::string::npos);
}

TEST_F(LspGetAnnotationUsageTests, GetClassAnnotationUsageWithProperties)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string annotationName = "Component";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {annotationName, ark::es2panda::ir::AstNodeType::ANNOTATION_USAGE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(annotationName), std::string::npos);
}

TEST_F(LspGetAnnotationUsageTests, GetMethodAnnotationUsage)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string annotationName = "Log";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {annotationName, ark::es2panda::ir::AstNodeType::ANNOTATION_USAGE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(annotationName), std::string::npos);
}

TEST_F(LspGetAnnotationUsageTests, GetAnotherMethodAnnotationUsage)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string annotationName = "Deprecated";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {annotationName, ark::es2panda::ir::AstNodeType::ANNOTATION_USAGE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(annotationName), std::string::npos);
}

TEST_F(LspGetAnnotationUsageTests, GetNonExistentAnnotationUsage)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string annotationName = "NonExistent";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {annotationName, ark::es2panda::ir::AstNodeType::ANNOTATION_USAGE});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    ASSERT_EQ(res.start, static_cast<size_t>(0));
    ASSERT_EQ(res.length, static_cast<size_t>(0));
}
}  // namespace