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

class LspGetInfoAnnotationUsageTests : public LSPAPITests {
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
        contexts_ =
            initializer.CreateContext("GetInfoAnnotationUsageTest.ets", ES2PANDA_STATE_PARSED, sourceCode_.c_str());
    }

    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetInfoAnnotationUsageTests, GetInfoClassAnnotationUsage)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 307;
    const size_t expectedSize = 4;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"DatabaseService", ark::es2panda::ir::AstNodeType::CLASS_DECLARATION},
                                            {"DatabaseService", ark::es2panda::ir::AstNodeType::CLASS_DEFINITION},
                                            {"Injectable", ark::es2panda::ir::AstNodeType::ANNOTATION_USAGE},
                                            {"Injectable", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetInfoAnnotationUsageTests, GetInfoClassAnnotationUsageWithProperties)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 486;
    const size_t expectedSize = 4;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"TestComponent", ark::es2panda::ir::AstNodeType::CLASS_DECLARATION},
                                            {"TestComponent", ark::es2panda::ir::AstNodeType::CLASS_DEFINITION},
                                            {"Component", ark::es2panda::ir::AstNodeType::ANNOTATION_USAGE},
                                            {"Component", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetInfoAnnotationUsageTests, GetInfoMethodAnnotationUsage)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 563;
    const size_t expectedSize = 6;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"TestComponent", ark::es2panda::ir::AstNodeType::CLASS_DECLARATION},
                                            {"TestComponent", ark::es2panda::ir::AstNodeType::CLASS_DEFINITION},
                                            {"doSomething", ark::es2panda::ir::AstNodeType::METHOD_DEFINITION},
                                            {"doSomething", ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION},
                                            {"Log", ark::es2panda::ir::AstNodeType::ANNOTATION_USAGE},
                                            {"Log", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetInfoAnnotationUsageTests, GetInfoAnotherMethodAnnotationUsage)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 695;
    const size_t expectedSize = 6;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"TestComponent", ark::es2panda::ir::AstNodeType::CLASS_DECLARATION},
                                            {"TestComponent", ark::es2panda::ir::AstNodeType::CLASS_DEFINITION},
                                            {"oldMethod", ark::es2panda::ir::AstNodeType::METHOD_DEFINITION},
                                            {"oldMethod", ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION},
                                            {"Deprecated", ark::es2panda::ir::AstNodeType::ANNOTATION_USAGE},
                                            {"Deprecated", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetInfoAnnotationUsageTests, GetNonInfoAnnotationUsage)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t errorOffset = 850;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, errorOffset);
    ASSERT_TRUE(result.empty());
}
}  // namespace