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

#include <cstddef>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/class_hierarchy.h"
#include "lsp/include/class_hierarchies.h"
#include "lsp/include/internal_api.h"

namespace {

using ark::es2panda::lsp::Initializer;

// Edge-focused suite for the class/type hierarchy public entries:
// null/invalid inputs, generic implements, multi-layer chains.
class LspClassHierarchyEdgeTests : public LSPAPITests {};

// A null context must produce a fully empty TypeHierarchiesInfo instead of
// touching any program state.
TEST_F(LspClassHierarchyEdgeTests, NullContextReturnsFullyEmptyHierarchyInfo)
{
    auto info = ark::es2panda::lsp::GetTypeHierarchiesImpl(nullptr, 0);
    EXPECT_TRUE(info.fileName.empty());
    EXPECT_TRUE(info.name.empty());
    EXPECT_EQ(info.type, ::HierarchyType::OTHERS);
    EXPECT_EQ(info.pos, 0U);
    EXPECT_TRUE(info.superHierarchies.subOrSuper.empty());
    EXPECT_TRUE(info.subHierarchies.subOrSuper.empty());
}

// An explicit declaration argument that is neither a class nor an interface is
// rejected before any hierarchy traversal happens.
TEST_F(LspClassHierarchyEdgeTests, ExplicitNonTypeDeclarationYieldsEmptyResult)
{
    std::vector<std::string> files = {"ch5_plain.ets"};
    std::vector<std::string> texts = {R"(let plainValue: number = 1;
console.log(plainValue);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // The program root node is an ETS module - not a class/interface declaration.
    auto *astRoot = GetAstFromContext<ark::es2panda::ir::AstNode>(context);
    ASSERT_NE(astRoot, nullptr);
    auto info = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, 0, astRoot);
    EXPECT_TRUE(info.fileName.empty());
    EXPECT_TRUE(info.name.empty());
    EXPECT_EQ(info.type, ::HierarchyType::OTHERS);
    EXPECT_EQ(info.pos, 0U);
    EXPECT_TRUE(info.superHierarchies.subOrSuper.empty());
    EXPECT_TRUE(info.subHierarchies.subOrSuper.empty());

    initializer.DestroyContext(context);
}

// Querying at a position inside a plain top-level function body never finds a
// user-written class/interface; the enclosing declaration is ETS' synthetic
// ETSGLOBAL class that owns all top-level statements.
TEST_F(LspClassHierarchyEdgeTests, FunctionBodyPositionResolvesToSyntheticEtsGlobalOwner)
{
    std::vector<std::string> files = {"ch5_function_scope.ets"};
    std::vector<std::string> texts = {R"(function standaloneCompute(): number {
    let inner = 41;
    return inner + 1;
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto pos = texts[0].find("return inner") + std::string("return ").size();
    auto info = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, pos);
    // Characterization: the nearest class-like owner is the compiler-generated
    // ETSGLOBAL declaration, not any user class or interface.
    EXPECT_EQ(info.fileName, filePaths[0]);
    EXPECT_EQ(info.name, "ETSGLOBAL");
    EXPECT_EQ(info.type, ::HierarchyType::CLASS);
    EXPECT_TRUE(info.superHierarchies.subOrSuper.empty());
    EXPECT_TRUE(info.subHierarchies.subOrSuper.empty());

    initializer.DestroyContext(context);
}

// Generic heritage clauses: implementing Producer<T> resolves to the generic
// interface declaration, and both concrete and generic implementors show up in
// its sub-hierarchy.
TEST_F(LspClassHierarchyEdgeTests, GenericImplementsResolvesInterfaceAndImplementors)
{
    std::vector<std::string> files = {"ch5_generic.ets"};
    std::vector<std::string> texts = {R"(interface Producer<T> {
    produce(): T;
}

class StringProducer implements Producer<string> {
    produce(): string {
        return "data";
    }
}

class Box<T> implements Producer<T> {
    innerValue: T | undefined
    produce(): T {
        return this.innerValue as T;
    }
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Query inside the Producer interface body (offset 0 would miss the token).
    const auto producerPos = texts[0].find("produce(): T;");
    auto info = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, producerPos);
    ASSERT_EQ(info.type, ::HierarchyType::INTERFACE);
    EXPECT_EQ(info.name, "Producer");
    // Characterization: GetTypeHierarchiesImpl discovers sub-types through
    // extends chains only - implementors of an interface are not listed here
    // (that is GetClassHierarchiesImpl's job), so the sub side stays empty.
    EXPECT_TRUE(info.superHierarchies.subOrSuper.empty());
    EXPECT_TRUE(info.subHierarchies.subOrSuper.empty());

    // The generic subclass still reports Producer as its single super interface.
    const auto boxPos = texts[0].find("class Box");
    auto boxInfo = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, boxPos);
    ASSERT_EQ(boxInfo.type, ::HierarchyType::CLASS);
    EXPECT_EQ(boxInfo.name, "Box");
    ASSERT_EQ(boxInfo.superHierarchies.subOrSuper.size(), 1U);
    EXPECT_EQ(boxInfo.superHierarchies.subOrSuper[0].name, "Producer");
    EXPECT_EQ(boxInfo.superHierarchies.subOrSuper[0].type, ::HierarchyType::INTERFACE);
    EXPECT_TRUE(boxInfo.subHierarchies.subOrSuper.empty());

    initializer.DestroyContext(context);
}

// A pure extends chain nests every ancestor level below the previous one.
TEST_F(LspClassHierarchyEdgeTests, MultiLayerExtendsChainNestsEachAncestor)
{
    std::vector<std::string> files = {"ch5_chain.ets"};
    std::vector<std::string> texts = {R"(class LevelZero {}
class LevelOne extends LevelZero {}
class LevelTwo extends LevelOne {}
class LevelThree extends LevelTwo {}
let instance = new LevelThree();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto pos = texts[0].find("class LevelThree");
    auto info = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, pos);
    ASSERT_EQ(info.type, ::HierarchyType::CLASS);
    EXPECT_EQ(info.name, "LevelThree");

    // LevelTwo -> LevelOne -> LevelZero, one nested entry per level.
    const auto &top = info.superHierarchies.subOrSuper;
    ASSERT_EQ(top.size(), 1U);
    EXPECT_EQ(top[0].name, "LevelTwo");
    EXPECT_EQ(top[0].type, ::HierarchyType::CLASS);
    ASSERT_EQ(top[0].subOrSuper.size(), 1U);
    EXPECT_EQ(top[0].subOrSuper[0].name, "LevelOne");
    ASSERT_EQ(top[0].subOrSuper[0].subOrSuper.size(), 1U);
    EXPECT_EQ(top[0].subOrSuper[0].subOrSuper[0].name, "LevelZero");
    EXPECT_TRUE(top[0].subOrSuper[0].subOrSuper[0].subOrSuper.empty());

    // No subclasses exist for the chain bottom queried from the top.
    EXPECT_TRUE(info.subHierarchies.subOrSuper.empty());

    initializer.DestroyContext(context);
}

// Multi-layer interface trees: sub-interfaces are reported per level together
// with the class implementing the deepest interface of the chain.
TEST_F(LspClassHierarchyEdgeTests, MultiLayerInterfaceTreeListsSubInterfacesAndImplementingClass)
{
    std::vector<std::string> files = {"ch5_iface_tree.ets"};
    std::vector<std::string> texts = {R"(interface Base0 {
    id(): number;
}
interface Mid1 extends Base0 {}
interface Leaf2 extends Mid1 {}
class ImplLeaf implements Leaf2 {
    id(): number {
        return 7;
    }
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    std::vector<es2panda_Context *> contexts {context};

    const auto basePos = texts[0].find("id(): number;");
    auto infos = ark::es2panda::lsp::GetClassHierarchiesImpl(&contexts, filePaths[0], basePos);
    // Only the extends chain shows up: Mid1 and Leaf2. Characterization:
    // GetInterfaceImplementingClasses matches classes that directly implement
    // the queried interface; ImplLeaf implements Leaf2, not Base0.
    ASSERT_EQ(infos.size(), 2U);

    EXPECT_EQ(infos[0].description, "Mid1");
    EXPECT_EQ(infos[0].kind, ark::es2panda::lsp::ClassRelationKind::INTERFACE);
    EXPECT_EQ(infos[1].description, "Leaf2");
    EXPECT_EQ(infos[1].kind, ark::es2panda::lsp::ClassRelationKind::INTERFACE);

    // Neither sub-interface declares id(), so Base0's id() lands in the
    // overriding bucket of each entry with the exact source span of the
    // Base0 member; the overridden bucket stays empty.
    const auto baseMemberPos = texts[0].find("id(): number;");
    for (const auto &info : infos) {
        EXPECT_TRUE(info.overridden.empty());
        ASSERT_EQ(info.overriding.size(), 1U);
        EXPECT_EQ(info.overriding[0].fileName, filePaths[0]);
        EXPECT_EQ(info.overriding[0].pos, baseMemberPos);
        EXPECT_EQ(info.overriding[0].kind, ark::es2panda::lsp::ClassRelationKind::METHOD);
        EXPECT_TRUE(info.implemented.empty());
        EXPECT_TRUE(info.implementing.empty());
    }

    // Querying the leaf interface lists its super interfaces (Mid1, then the
    // transitive Base0) and the direct implementing class ImplLeaf.
    const auto leafPos = texts[0].find("interface Leaf2");
    auto leafInfos = ark::es2panda::lsp::GetClassHierarchiesImpl(&contexts, filePaths[0], leafPos);
    ASSERT_EQ(leafInfos.size(), 3U);
    // Entry 2 is the direct implementing class listed after both super interfaces.
    constexpr size_t implementingClassIndex = 2;
    EXPECT_EQ(leafInfos[0].description, "Mid1");
    EXPECT_EQ(leafInfos[0].kind, ark::es2panda::lsp::ClassRelationKind::INTERFACE);
    EXPECT_EQ(leafInfos[1].description, "Base0");
    EXPECT_EQ(leafInfos[1].kind, ark::es2panda::lsp::ClassRelationKind::INTERFACE);
    EXPECT_EQ(leafInfos[implementingClassIndex].description, "ImplLeaf");
    EXPECT_EQ(leafInfos[implementingClassIndex].kind, ark::es2panda::lsp::ClassRelationKind::CLASS);

    initializer.DestroyContext(context);
}
}  // namespace
