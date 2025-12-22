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

class LspGetInfosAnnotationDeclarationTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        sourceCode_ = R"(
            @interface Validate {
            }

            @interface Log {
                level: string;
            }

            @interface Component {
                name: string;
                version: number;
            }

            @interface Deprecated {
            }

            @Component({name: "Service", version: 1})
            class Service {
                @Validate
                @Log({level: 'info'})
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
    }

    static void GenerateContexts(Initializer &initializer)
    {
        contexts_ = initializer.CreateContext("GetInfosAnnotationDeclarationTest.ets", ES2PANDA_STATE_PARSED,
                                              sourceCode_.c_str());
    }

    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetInfosAnnotationDeclarationTests, GetSimpleAnnotationDeclarationInfo)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 24;
    const size_t expectedSize = 2;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"Validate", ark::es2panda::ir::AstNodeType::ANNOTATION_DECLARATION},
                                            {"Validate", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetInfosAnnotationDeclarationTests, GetAnnotationDeclarationWithPropertiesInfo)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 73;
    const size_t expectedSize = 2;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"Log", ark::es2panda::ir::AstNodeType::ANNOTATION_DECLARATION},
                                            {"Log", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetInfosAnnotationDeclarationTests, GetComplexAnnotationDeclarationInfo)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 148;
    const size_t expectedSize = 2;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"Component", ark::es2panda::ir::AstNodeType::ANNOTATION_DECLARATION},
                                            {"Component", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetInfosAnnotationDeclarationTests, GetAnotherSimpleAnnotationDeclarationInfo)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 261;
    const size_t expectedSize = 2;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    std::vector<NodeInfo> expectedResult = {{"Deprecated", ark::es2panda::ir::AstNodeType::ANNOTATION_DECLARATION},
                                            {"Deprecated", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetInfosAnnotationDeclarationTests, GetAnnotationDeclarationInfo_Error)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 660;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    ASSERT_TRUE(result.empty());
}
}  // namespace