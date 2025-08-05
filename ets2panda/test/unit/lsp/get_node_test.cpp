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

class LspGetNodeTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        sourceCode_ = R"(class Foo {
    Foo = 1;
    bar() {}
    enum Color {
        Red,
        Green,
        Blue
    };

    static staticProperty: number = 42;
    optionalProperty?: boolean;
    readonly readOnlyProperty: string = "read-only";
    protected protectedProperty: string = "protected";
}

const obj = {
    prop: "value",
    methodProp: function() {
        return "method result";
    },
    arrowProp: () => {
        console.log("arrow function property");
    },
    arrayProp: [1, 2, 3, 4, 5],
};

const myClassInstance = new Foo();
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
        contexts_ = initializer.CreateContext("GetNodeTest.ets", ES2PANDA_STATE_PARSED, sourceCode_.c_str());
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_ = "";
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetNodeTests, GetProgramAst1)
{
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(contexts_);
    auto expectedAst = ctx->parserProgram->Ast();
    LSPAPI const *lspApi = GetImpl();
    auto ast = lspApi->getProgramAst(contexts_);
    ASSERT_EQ(reinterpret_cast<ark::es2panda::ir::AstNode *>(ast), expectedAst);
}

TEST_F(LspGetNodeTests, GetClassDefinition1)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Foo";

    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::CLASS_DEFINITION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetIdentifier1)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Foo";

    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::IDENTIFIER});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetClassProperty1)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "Foo";

    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::CLASS_PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetClassProperty2)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "staticProperty";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::CLASS_PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetClassProperty3)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "optionalProperty";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::CLASS_PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetClassProperty4)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "readOnlyProperty";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::CLASS_PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetClassProperty5)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "protectedProperty";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::CLASS_PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetClassPropertyNotFound)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "nonExistentProperty";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::CLASS_PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_EQ(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetProperty1)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "prop";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetProperty2)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "methodProp";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetProperty3)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "arrowProp";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetProperty4)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "arrayProp";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::PROPERTY});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetMethodDefinition1)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "bar";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::METHOD_DEFINITION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetMethodDefinition_NotFound)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "nonExistentMethod";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::METHOD_DEFINITION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_EQ(extractedText.find(nodeName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetTsEnumDeclaration)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string enumName = "Color";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {enumName, ark::es2panda::ir::AstNodeType::TS_ENUM_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(enumName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetTsEnumDeclaration_NotFound)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string enumName = "nonExistentEnum";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {enumName, ark::es2panda::ir::AstNodeType::TS_ENUM_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_EQ(extractedText.find(enumName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetTsEnumMember)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string memberName = "Red";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {memberName, ark::es2panda::ir::AstNodeType::TS_ENUM_MEMBER});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(memberName), std::string::npos);
}

TEST_F(LspGetNodeTests, GetTsEnumMember_NotFound)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string memberName = "nonExistentEnumMember";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {memberName, ark::es2panda::ir::AstNodeType::TS_ENUM_MEMBER});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_EQ(extractedText.find(memberName), std::string::npos);
}
}  // namespace