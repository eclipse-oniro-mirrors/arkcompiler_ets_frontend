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

class LspGetTsClassImplementsTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        sourceCode_ = R"(
interface Printable { print(): void; getTitle(): string; }

class Document implements Printable {
    constructor() {}
    print(): void {}
    getTitle(): string { return ""; }
}
interface Drawable { draw(): void; }

interface Serializable { serialize(): string; }

class Circle implements Drawable, Serializable {
    draw(): void {}
    serialize(): string { return "{}"; }
}

namespace Graphics {
    interface Shape { area(): number; }
    class Rectangle implements Shape { area(): number { return 0; } }
}

class Base {}
interface Loggable { log(): void; }
class Derived extends Base implements Loggable { log(): void {} }

interface Repository<T> { save(item: T): void; }
class UserRepository implements Repository<User> { save(item: User): void {} }
class User {
    id: number;
    name: string;
    constructor(id: number, name: string) { this.id = id; this.name = name; }
}

interface RealInterface { method(): void; }
type Alias = RealInterface;
class UsingAlias implements Alias { method(): void {} }

interface MyInterface { method(): void; }
class MyClass implements MyInterface { method(): void {} }
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
            initializer.CreateContext("GetNodeTsClassImplements.ts", ES2PANDA_STATE_PARSED, sourceCode_.c_str());
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_ = "";
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetTsClassImplementsTests, GetTsClassImplements)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string memberName = "Printable";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {memberName, ark::es2panda::ir::AstNodeType::TS_CLASS_IMPLEMENTS});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(memberName), std::string::npos);
}

TEST_F(LspGetTsClassImplementsTests, MultipleInterfaces)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string memberName = "Drawable";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {memberName, ark::es2panda::ir::AstNodeType::TS_CLASS_IMPLEMENTS});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(memberName), std::string::npos);
}

TEST_F(LspGetTsClassImplementsTests, MultipleInterfaces2)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string memberName = "Serializable";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {memberName, ark::es2panda::ir::AstNodeType::TS_CLASS_IMPLEMENTS});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(memberName), std::string::npos);
}

TEST_F(LspGetTsClassImplementsTests, InterfaceNotFound)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string memberName = "NonExistentInterface";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {memberName, ark::es2panda::ir::AstNodeType::TS_CLASS_IMPLEMENTS});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);
    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_EQ(extractedText.find(memberName), std::string::npos);
}
TEST_F(LspGetTsClassImplementsTests, InNamespace)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string memberName = "Shape";

    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {memberName, ark::es2panda::ir::AstNodeType::TS_CLASS_IMPLEMENTS});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);
    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(memberName), std::string::npos);
}

TEST_F(LspGetTsClassImplementsTests, ExtendsAndImplements)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string memberName = "Loggable";

    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {memberName, ark::es2panda::ir::AstNodeType::TS_CLASS_IMPLEMENTS});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);
    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(memberName), std::string::npos);
}

TEST_F(LspGetTsClassImplementsTests, GenericInterface)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string memberName = "Repository";

    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {memberName, ark::es2panda::ir::AstNodeType::TS_CLASS_IMPLEMENTS});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);
    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(memberName), std::string::npos);
}

TEST_F(LspGetTsClassImplementsTests, CaseSensitivity)
{
    LSPAPI const *lspApi = GetImpl();
    const std::string memberName = "MyInterface";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {memberName, ark::es2panda::ir::AstNodeType::TS_CLASS_IMPLEMENTS});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);
    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    std::string extractedText(sourceCode_.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(memberName), std::string::npos);
}
}  // namespace