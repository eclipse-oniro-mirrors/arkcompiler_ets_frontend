/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include <gtest/gtest.h>
#include <iostream>

namespace {
using ark::es2panda::lsp::Initializer;

// The static language keeps no dedicated node for most export statements: a plain
// `export { x }` becomes an empty statement that only preserves the statement range,
// and a default export collapses into the exported declaration itself. Node info
// chains with export elements must still resolve to the export statement location.
class LspGetNodeExportStaticTests : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        sourceCode_ = R"(
export class Foo {
    bar(): void { }
}
export { Foo }
function Fn(): void { }
export { Fn }
export default class Def { }
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
            initializer.CreateContext("GetNodeExportStaticTest.ets", ES2PANDA_STATE_PARSED, sourceCode_.c_str());
    }
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline es2panda_Context *contexts_ = nullptr;
    static inline Initializer *initializer_ = nullptr;
    static inline std::string sourceCode_;
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(LspGetNodeExportStaticTests, DebugDumpNamespaceDefaultExport)
{
    Initializer initializer;
    std::string source = R"(
declare namespace hilog {
    function info(domain: number, tag: string, format: string): void;
}
export default hilog;
)";
    auto *ctx = initializer.CreateContext("NamespaceDefaultExport.ets", ES2PANDA_STATE_PARSED, source.c_str());
    auto *pctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    std::cout << "NS-ROOT type=" << ark::es2panda::ir::ToString(pctx->parserProgram->Ast()->Type())
              << " defaultExported=" << pctx->parserProgram->Ast()->IsDefaultExported() << std::endl;
    pctx->parserProgram->Ast()->IterateRecursively([](ark::es2panda::ir::AstNode *node) {
        if (node->IsIdentifier()) {
            auto *ident = node->AsIdentifier();
            auto *owner = ark::es2panda::lsp::GetOwner(ident);
            std::cout << "NS-OWNER name=" << ident->Name().Mutf8() << " start=" << ident->Start().index
                      << " ownerType=" << (owner != nullptr ? ark::es2panda::ir::ToString(owner->Type()) : "null")
                      << " ownerStart=" << (owner != nullptr ? std::to_string(owner->Start().index) : "-")
                      << " ownerName="
                      << (owner != nullptr && owner->IsIdentifier() ? owner->AsIdentifier()->Name().Mutf8() : "-")
                      << std::endl;
        }
    });
    initializer.DestroyContext(ctx);
    SUCCEED();
}

TEST_F(LspGetNodeExportStaticTests, ExportSpecifierOnlyChainLocatesStatement)
{
    LSPAPI const *lspApi = GetImpl();
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {"Foo", ark::es2panda::ir::AstNodeType::EXPORT_SPECIFIER});
    std::vector<NodeInfo *> nodeInfoPtrs;
    for (auto &info : nodeInfos) {
        nodeInfoPtrs.push_back(&info);
    }

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    // The position is the exported name inside the `export { Foo }` statement, not the
    // class declaration.
    auto statementOffset = sourceCode_.find("export { Foo }") + std::string("export { ").size();
    ASSERT_EQ(res.start, statementOffset);
    ASSERT_EQ(res.length, std::string("Foo").size());
}

TEST_F(LspGetNodeExportStaticTests, ExportNamedDeclarationOnlyChainLocatesStatement)
{
    LSPAPI const *lspApi = GetImpl();
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {"Foo", ark::es2panda::ir::AstNodeType::EXPORT_NAMED_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    for (auto &info : nodeInfos) {
        nodeInfoPtrs.push_back(&info);
    }

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    auto statementOffset = sourceCode_.find("export { Foo }") + std::string("export { ").size();
    ASSERT_EQ(res.start, statementOffset);
    ASSERT_EQ(res.length, std::string("Foo").size());
}

TEST_F(LspGetNodeExportStaticTests, ExportSpecifierLocatesFunctionExportStatement)
{
    LSPAPI const *lspApi = GetImpl();
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {"Fn", ark::es2panda::ir::AstNodeType::EXPORT_SPECIFIER});
    std::vector<NodeInfo *> nodeInfoPtrs;
    for (auto &info : nodeInfos) {
        nodeInfoPtrs.push_back(&info);
    }

    // `function Fn(): void { }` followed by `export { Fn }`: the specifier of a
    // declare-then-export statement resolves to its position inside the statement.
    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    auto statementOffset = sourceCode_.find("export { Fn }") + std::string("export { ").size();
    ASSERT_EQ(res.start, statementOffset);
    ASSERT_EQ(res.length, std::string("Fn").size());
}

TEST_F(LspGetNodeExportStaticTests, ExportDefaultDeclarationLocatesDeclaration)
{
    LSPAPI const *lspApi = GetImpl();
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {"Def", ark::es2panda::ir::AstNodeType::EXPORT_DEFAULT_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    for (auto &info : nodeInfos) {
        nodeInfoPtrs.push_back(&info);
    }

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    // The position is the declaration identifier of the default export.
    auto declarationOffset = sourceCode_.find("class Def");
    ASSERT_EQ(res.start, declarationOffset + std::string("class ").size());
    ASSERT_EQ(res.length, std::string("Def").size());
}

TEST_F(LspGetNodeExportStaticTests, RenameLocatesPlainExportStatement)
{
    LSPAPI const *lspApi = GetImpl();
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {"Foo", ark::es2panda::ir::AstNodeType::EXPORT_SPECIFIER});
    std::vector<NodeInfo *> nodeInfoPtrs;
    for (auto &info : nodeInfos) {
        nodeInfoPtrs.push_back(&info);
    }

    auto res = lspApi->findRenameLocationsFromNode(contexts_, nodeInfoPtrs);
    auto statementOffset = sourceCode_.find("export { Foo }") + std::string("export { ").size();
    ASSERT_EQ(res.start, statementOffset);
    ASSERT_EQ(res.end - res.start, static_cast<std::string::size_type>(std::string("Foo").size()));
}

TEST_F(LspGetNodeExportStaticTests, ChainWithExportWrapperPrefersDeclarationElement)
{
    LSPAPI const *lspApi = GetImpl();
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {"Foo", ark::es2panda::ir::AstNodeType::EXPORT_SPECIFIER});
    nodeInfos.emplace_back(NodeInfo {"Foo", ark::es2panda::ir::AstNodeType::CLASS_DECLARATION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    for (auto &info : nodeInfos) {
        nodeInfoPtrs.push_back(&info);
    }

    // The deeper declaration element resolves within the outer root and takes precedence
    // over the plain export statement anchor.
    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    auto declarationOffset = sourceCode_.find("class Foo");
    ASSERT_EQ(res.start, declarationOffset + std::string("class ").size());
    ASSERT_EQ(res.length, std::string("Foo").size());
}

TEST_F(LspGetNodeExportStaticTests, MissingExportWrapperFails)
{
    LSPAPI const *lspApi = GetImpl();
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {"Missing", ark::es2panda::ir::AstNodeType::EXPORT_SPECIFIER});
    std::vector<NodeInfo *> nodeInfoPtrs;
    for (auto &info : nodeInfos) {
        nodeInfoPtrs.push_back(&info);
    }

    auto res = lspApi->getDefinitionDataFromNode(contexts_, nodeInfoPtrs);
    ASSERT_EQ(res.start, 0U);
    ASSERT_EQ(res.length, 0U);
}
}  // namespace
