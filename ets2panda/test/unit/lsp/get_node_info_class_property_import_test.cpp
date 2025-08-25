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
#include <cstddef>

namespace {
using ark::es2panda::lsp::Initializer;

class LspGetNodeInfoClassPropertyImportTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
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
        contexts_ =
            initializer.CreateContext("GetNodeInfoClassPropertyImport.ets", ES2PANDA_STATE_PARSED, R"('use static'
const obj = {
    prop: 'value'
};
interface Printable {}
class Document1 implements Printable {
}
)");
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetNodeInfoClassPropertyImportTests, GetNodeInfoClassPropertyImportTest1)
{
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getNodeInfosByDefinitionData(nullptr, 0);
    ASSERT_TRUE(result.empty());
}

TEST_F(LspGetNodeInfoClassPropertyImportTests, GetNodeInfoClassPropertyImportTest2)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t errorOffset = 460;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, errorOffset);
    ASSERT_TRUE(result.empty());
}

TEST_F(LspGetNodeInfoClassPropertyImportTests, GetNodeInfoClassPropertyImportTest3)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 33;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    const size_t expectedSize = 4;
    std::vector<NodeInfo> expectedResult = {{"obj", ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATION},
                                            {"obj", ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATOR},
                                            {"prop", ark::es2panda::ir::AstNodeType::PROPERTY},
                                            {"prop", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfoClassPropertyImportTests, GetNodeInfoClassPropertyImportTest4)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 103;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts_, offset);
    const size_t expectedSize = 4;
    std::vector<NodeInfo> expectedResult = {{"Document1", ark::es2panda::ir::AstNodeType::CLASS_DECLARATION},
                                            {"Document1", ark::es2panda::ir::AstNodeType::CLASS_DEFINITION},
                                            {"Printable", ark::es2panda::ir::AstNodeType::TS_CLASS_IMPLEMENTS},
                                            {"Printable", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }
}

TEST_F(LspGetNodeInfoClassPropertyImportTests, GetNodeInfoClassPropertyImportTest5)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(class Foo {
    Foo = 1;
})";
    es2panda_Context *contexts =
        initializer.CreateContext("GetNodeInfoClassPropertyImport5.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 16;
    const size_t expectedSize = 4;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts, offset);
    std::vector<NodeInfo> expectedResult = {{"Foo", ark::es2panda::ir::AstNodeType::CLASS_DECLARATION},
                                            {"Foo", ark::es2panda::ir::AstNodeType::CLASS_DEFINITION},
                                            {"Foo", ark::es2panda::ir::AstNodeType::CLASS_PROPERTY},
                                            {"Foo", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }

    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeInfoClassPropertyImportTests, GetNodeInfoClassPropertyImportTest6)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(export { PI } from "std/math";)";
    es2panda_Context *contexts =
        initializer.CreateContext("GetNodeInfoClassPropertyImport6.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 9;
    const size_t expectedSize = 2;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts, offset);
    std::vector<NodeInfo> expectedResult = {{"PI", ark::es2panda::ir::AstNodeType::IMPORT_SPECIFIER},
                                            {"PI", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }

    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeInfoClassPropertyImportTests, GetNodeInfoClassPropertyImportTest7)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(import PI from "std/math";)";
    es2panda_Context *contexts =
        initializer.CreateContext("GetNodeInfoClassPropertyImport7.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 7;
    const size_t expectedSize = 2;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts, offset);
    std::vector<NodeInfo> expectedResult = {{"PI", ark::es2panda::ir::AstNodeType::IMPORT_DEFAULT_SPECIFIER},
                                            {"PI", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }

    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeInfoClassPropertyImportTests, GetNodeInfoClassPropertyImportTest8)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(import * as All from "std/math";)";
    es2panda_Context *contexts =
        initializer.CreateContext("GetNodeInfoClassPropertyImport8.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 12;
    const size_t expectedSize = 2;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts, offset);
    std::vector<NodeInfo> expectedResult = {{"All", ark::es2panda::ir::AstNodeType::IMPORT_NAMESPACE_SPECIFIER},
                                            {"All", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }

    initializer.DestroyContext(contexts);
}

}  // namespace