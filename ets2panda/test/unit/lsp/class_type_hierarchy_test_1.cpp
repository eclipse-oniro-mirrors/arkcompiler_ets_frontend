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
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "lsp_api_test.h"
#include "lsp/include/class_hierarchy.h"
#include "lsp/include/internal_api.h"

namespace {

using ark::es2panda::lsp::Initializer;

class LspClassTypeHierarchyTests : public LSPAPITests {};

size_t DeclNamePos(const std::string &content, const std::string &keyword, const std::string &name)
{
    auto pos = content.find(keyword + " " + name);
    EXPECT_NE(pos, std::string::npos);
    return pos + keyword.size() + 1;
}

// Class extends class: querying the derived class reports the base class in superHierarchies,
// querying the base class reports the derived class in subHierarchies. One context, two queries.
TEST_F(LspClassTypeHierarchyTests, ClassExtendsClass)
{
    std::vector<std::string> fileNames = {"cth1_extends.ets"};
    std::vector<std::string> fileContents = {R"(class CthVehicle {
    speed: number = 0;
}
class CthCar extends CthVehicle {
    wheel: number = 4;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto vehiclePos = DeclNamePos(fileContents[0], "class", "CthVehicle");
    const auto carPos = DeclNamePos(fileContents[0], "class", "CthCar");

    // Query the derived class: super chain Car -> Vehicle, no sub classes.
    auto carRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, carPos);
    ASSERT_EQ(carRes.name, "CthCar");
    EXPECT_EQ(carRes.type, ::HierarchyType::CLASS);
    EXPECT_EQ(carRes.fileName, filePaths[0]);
    EXPECT_EQ(carRes.pos, carPos);
    ASSERT_EQ(carRes.superHierarchies.subOrSuper.size(), 1U);
    EXPECT_EQ(carRes.superHierarchies.subOrSuper[0].name, "CthVehicle");
    EXPECT_EQ(carRes.superHierarchies.subOrSuper[0].type, ::HierarchyType::CLASS);
    EXPECT_EQ(carRes.superHierarchies.subOrSuper[0].fileName, filePaths[0]);
    EXPECT_EQ(carRes.superHierarchies.subOrSuper[0].pos, vehiclePos);
    EXPECT_EQ(carRes.superHierarchies.subOrSuper[0].subOrSuper.size(), 0U);
    EXPECT_EQ(carRes.subHierarchies.subOrSuper.size(), 0U);

    // Query the base class: sub chain Vehicle -> Car, no super classes.
    auto vehicleRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, vehiclePos);
    initializer.DestroyContext(context);

    ASSERT_EQ(vehicleRes.name, "CthVehicle");
    EXPECT_EQ(vehicleRes.type, ::HierarchyType::CLASS);
    EXPECT_EQ(vehicleRes.fileName, filePaths[0]);
    EXPECT_EQ(vehicleRes.pos, vehiclePos);
    EXPECT_EQ(vehicleRes.superHierarchies.subOrSuper.size(), 0U);
    ASSERT_EQ(vehicleRes.subHierarchies.subOrSuper.size(), 1U);
    EXPECT_EQ(vehicleRes.subHierarchies.subOrSuper[0].name, "CthCar");
    EXPECT_EQ(vehicleRes.subHierarchies.subOrSuper[0].type, ::HierarchyType::CLASS);
    EXPECT_EQ(vehicleRes.subHierarchies.subOrSuper[0].fileName, filePaths[0]);
    EXPECT_EQ(vehicleRes.subHierarchies.subOrSuper[0].pos, carPos);
    EXPECT_EQ(vehicleRes.subHierarchies.subOrSuper[0].subOrSuper.size(), 0U);
}

// Interface extends interface: the derived interface reports both extended interfaces in order;
// the root interface reports the whole sub-interface chain.
TEST_F(LspClassTypeHierarchyTests, InterfaceExtendsInterface)
{
    std::vector<std::string> fileNames = {"cth1_iface.ets"};
    std::vector<std::string> fileContents = {R"(interface CthReadable {
    read(): string;
}
interface CthWritable {
    write(data: string): void;
}
interface CthReadWritable extends CthReadable, CthWritable {
    flush(): void;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto readablePos = DeclNamePos(fileContents[0], "interface", "CthReadable");
    const auto writablePos = DeclNamePos(fileContents[0], "interface", "CthWritable");
    const auto readWritablePos = DeclNamePos(fileContents[0], "interface", "CthReadWritable");

    // Query the derived interface: super interfaces appear in heritage order.
    auto derivedRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, readWritablePos);
    ASSERT_EQ(derivedRes.name, "CthReadWritable");
    EXPECT_EQ(derivedRes.type, ::HierarchyType::INTERFACE);
    EXPECT_EQ(derivedRes.fileName, filePaths[0]);
    EXPECT_EQ(derivedRes.pos, readWritablePos);
    ASSERT_EQ(derivedRes.superHierarchies.subOrSuper.size(), 2U);
    EXPECT_EQ(derivedRes.superHierarchies.subOrSuper[0].name, "CthReadable");
    EXPECT_EQ(derivedRes.superHierarchies.subOrSuper[0].type, ::HierarchyType::INTERFACE);
    EXPECT_EQ(derivedRes.superHierarchies.subOrSuper[0].fileName, filePaths[0]);
    EXPECT_EQ(derivedRes.superHierarchies.subOrSuper[0].pos, readablePos);
    EXPECT_EQ(derivedRes.superHierarchies.subOrSuper[1].name, "CthWritable");
    EXPECT_EQ(derivedRes.superHierarchies.subOrSuper[1].type, ::HierarchyType::INTERFACE);
    EXPECT_EQ(derivedRes.superHierarchies.subOrSuper[1].fileName, filePaths[0]);
    EXPECT_EQ(derivedRes.superHierarchies.subOrSuper[1].pos, writablePos);
    EXPECT_EQ(derivedRes.subHierarchies.subOrSuper.size(), 0U);

    // Query the root interface: sub interface CthReadWritable is found.
    auto rootRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, readablePos);
    initializer.DestroyContext(context);

    ASSERT_EQ(rootRes.name, "CthReadable");
    EXPECT_EQ(rootRes.type, ::HierarchyType::INTERFACE);
    EXPECT_EQ(rootRes.pos, readablePos);
    EXPECT_EQ(rootRes.superHierarchies.subOrSuper.size(), 0U);
    ASSERT_EQ(rootRes.subHierarchies.subOrSuper.size(), 1U);
    EXPECT_EQ(rootRes.subHierarchies.subOrSuper[0].name, "CthReadWritable");
    EXPECT_EQ(rootRes.subHierarchies.subOrSuper[0].type, ::HierarchyType::INTERFACE);
    EXPECT_EQ(rootRes.subHierarchies.subOrSuper[0].fileName, filePaths[0]);
    EXPECT_EQ(rootRes.subHierarchies.subOrSuper[0].pos, readWritablePos);
}

// Verifies the super hierarchy of CthPerson: the extended class comes first, then the
// implemented interfaces in heritage order.
void VerifyPersonSuperEntries(const TypeHierarchiesInfo &personRes, size_t entityPos, size_t namedPos, size_t agedPos)
{
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t agedSuperIndex = 2;
    ASSERT_EQ(personRes.superHierarchies.subOrSuper.size(), 3U);
    EXPECT_EQ(personRes.superHierarchies.subOrSuper[0].name, "CthEntity");
    EXPECT_EQ(personRes.superHierarchies.subOrSuper[0].type, ::HierarchyType::CLASS);
    EXPECT_EQ(personRes.superHierarchies.subOrSuper[0].pos, entityPos);
    EXPECT_EQ(personRes.superHierarchies.subOrSuper[1].name, "CthNamed");
    EXPECT_EQ(personRes.superHierarchies.subOrSuper[1].type, ::HierarchyType::INTERFACE);
    EXPECT_EQ(personRes.superHierarchies.subOrSuper[1].pos, namedPos);
    EXPECT_EQ(personRes.superHierarchies.subOrSuper[agedSuperIndex].name, "CthAged");
    EXPECT_EQ(personRes.superHierarchies.subOrSuper[agedSuperIndex].type, ::HierarchyType::INTERFACE);
    EXPECT_EQ(personRes.superHierarchies.subOrSuper[agedSuperIndex].pos, agedPos);
}

// Verifies that the implemented interface reports empty super and sub hierarchies.
void VerifyNamedInterfaceResult(const TypeHierarchiesInfo &namedRes, const std::string &fileName, size_t namedPos)
{
    ASSERT_EQ(namedRes.name, "CthNamed");
    EXPECT_EQ(namedRes.type, ::HierarchyType::INTERFACE);
    EXPECT_EQ(namedRes.fileName, fileName);
    EXPECT_EQ(namedRes.pos, namedPos);
    EXPECT_EQ(namedRes.superHierarchies.subOrSuper.size(), 0U);
    EXPECT_EQ(namedRes.subHierarchies.subOrSuper.size(), 0U);
}

// Class implements interface: both implemented interfaces appear after the extended class in
// superHierarchies; the interface reports the implementing class in subHierarchies.
TEST_F(LspClassTypeHierarchyTests, ClassImplementsInterface)
{
    std::vector<std::string> fileNames = {"cth1_implements.ets"};
    std::vector<std::string> fileContents = {R"(interface CthNamed {
    getName(): string;
}
interface CthAged {
    getAge(): number;
}
class CthEntity {
    id: number = 0;
}
class CthPerson extends CthEntity implements CthNamed, CthAged {
    getName(): string {
        return "p";
    }
    getAge(): number {
        return 1;
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto namedPos = DeclNamePos(fileContents[0], "interface", "CthNamed");
    const auto agedPos = DeclNamePos(fileContents[0], "interface", "CthAged");
    const auto entityPos = DeclNamePos(fileContents[0], "class", "CthEntity");
    const auto personPos = DeclNamePos(fileContents[0], "class", "CthPerson");

    // Query the class: super chain is extended class first, then implemented interfaces in order.
    auto personRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, personPos);
    ASSERT_EQ(personRes.name, "CthPerson");
    EXPECT_EQ(personRes.type, ::HierarchyType::CLASS);
    EXPECT_EQ(personRes.fileName, filePaths[0]);
    EXPECT_EQ(personRes.pos, personPos);
    VerifyPersonSuperEntries(personRes, entityPos, namedPos, agedPos);

    // Query the interface: the sub direction never reports implementing classes. GetTypeHierarchiesImpl
    // only walks extends chains when matching sub nodes, so an interface implemented by a class has an
    // empty sub hierarchy (a class only appears under an interface it transitively extends).
    auto namedRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, namedPos);
    initializer.DestroyContext(context);

    VerifyNamedInterfaceResult(namedRes, filePaths[0], namedPos);
}

// Abstract class: super hierarchy contains both the extended class and the implemented interface,
// sub hierarchy contains the concrete subclass.
TEST_F(LspClassTypeHierarchyTests, AbstractClassExtendsAndImplements)
{
    std::vector<std::string> fileNames = {"cth1_abstract.ets"};
    std::vector<std::string> fileContents = {R"(interface CthFlyable {
    fly(): void;
}
abstract class CthAnimal {
    name: string = "";
}
abstract class CthBird extends CthAnimal implements CthFlyable {
    fly(): void {}
}
class CthSparrow extends CthBird {
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto birdPos = DeclNamePos(fileContents[0], "class", "CthBird");
    auto res = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, birdPos);
    initializer.DestroyContext(context);

    ASSERT_EQ(res.name, "CthBird");
    ASSERT_EQ(res.type, ::HierarchyType::CLASS);
    ASSERT_EQ(res.fileName, filePaths[0]);
    ASSERT_EQ(res.pos, birdPos);

    // Super: extended class first, then implemented interface.
    ASSERT_EQ(res.superHierarchies.subOrSuper.size(), 2U);
    const auto &superClass = res.superHierarchies.subOrSuper[0];
    EXPECT_EQ(superClass.name, "CthAnimal");
    EXPECT_EQ(superClass.type, ::HierarchyType::CLASS);
    EXPECT_EQ(superClass.fileName, filePaths[0]);
    EXPECT_EQ(superClass.pos, DeclNamePos(fileContents[0], "class", "CthAnimal"));
    EXPECT_EQ(superClass.subOrSuper.size(), 0U);
    const auto &superIface = res.superHierarchies.subOrSuper[1];
    EXPECT_EQ(superIface.name, "CthFlyable");
    EXPECT_EQ(superIface.type, ::HierarchyType::INTERFACE);
    EXPECT_EQ(superIface.fileName, filePaths[0]);
    EXPECT_EQ(superIface.pos, DeclNamePos(fileContents[0], "interface", "CthFlyable"));
    EXPECT_EQ(superIface.subOrSuper.size(), 0U);

    // Sub: the concrete subclass of the abstract class.
    ASSERT_EQ(res.subHierarchies.subOrSuper.size(), 1U);
    EXPECT_EQ(res.subHierarchies.subOrSuper[0].name, "CthSparrow");
    EXPECT_EQ(res.subHierarchies.subOrSuper[0].type, ::HierarchyType::CLASS);
    EXPECT_EQ(res.subHierarchies.subOrSuper[0].fileName, filePaths[0]);
    EXPECT_EQ(res.subHierarchies.subOrSuper[0].pos, DeclNamePos(fileContents[0], "class", "CthSparrow"));
}

// Verifies the full super chain reported for the leaf class: Leaf -> MidBase -> GrandBase.
void VerifyLeafSuperChain(const TypeHierarchiesInfo &leafRes, const std::string &fileName, size_t midPos,
                          size_t grandPos)
{
    ASSERT_EQ(leafRes.superHierarchies.subOrSuper.size(), 1U);
    const auto &midSuper = leafRes.superHierarchies.subOrSuper[0];
    ASSERT_EQ(midSuper.name, "CthMidBase");
    EXPECT_EQ(midSuper.type, ::HierarchyType::CLASS);
    EXPECT_EQ(midSuper.fileName, fileName);
    EXPECT_EQ(midSuper.pos, midPos);
    ASSERT_EQ(midSuper.subOrSuper.size(), 1U);
    EXPECT_EQ(midSuper.subOrSuper[0].name, "CthGrandBase");
    EXPECT_EQ(midSuper.subOrSuper[0].type, ::HierarchyType::CLASS);
    EXPECT_EQ(midSuper.subOrSuper[0].fileName, fileName);
    EXPECT_EQ(midSuper.subOrSuper[0].pos, grandPos);
    EXPECT_EQ(midSuper.subOrSuper[0].subOrSuper.size(), 0U);
}

// Verifies the full sub chain reported for the root class: GrandBase -> MidBase -> Leaf.
void VerifyRootSubChain(const TypeHierarchiesInfo &rootRes, size_t midPos, size_t leafPos)
{
    ASSERT_EQ(rootRes.subHierarchies.subOrSuper.size(), 1U);
    const auto &midSub = rootRes.subHierarchies.subOrSuper[0];
    ASSERT_EQ(midSub.name, "CthMidBase");
    EXPECT_EQ(midSub.type, ::HierarchyType::CLASS);
    EXPECT_EQ(midSub.pos, midPos);
    ASSERT_EQ(midSub.subOrSuper.size(), 1U);
    EXPECT_EQ(midSub.subOrSuper[0].name, "CthLeaf");
    EXPECT_EQ(midSub.subOrSuper[0].type, ::HierarchyType::CLASS);
    EXPECT_EQ(midSub.subOrSuper[0].pos, leafPos);
    EXPECT_EQ(midSub.subOrSuper[0].subOrSuper.size(), 0U);
}

// Verifies the leaf query result: the leaf entry itself plus its full super chain.
void VerifyLeafQueryResult(const TypeHierarchiesInfo &leafRes, const std::string &fileName, size_t leafPos,
                           size_t midPos, size_t grandPos)
{
    ASSERT_EQ(leafRes.name, "CthLeaf");
    ASSERT_EQ(leafRes.pos, leafPos);
    VerifyLeafSuperChain(leafRes, fileName, midPos, grandPos);
    EXPECT_EQ(leafRes.subHierarchies.subOrSuper.size(), 0U);
}

// Verifies the root query result: the root entry itself plus its full sub chain.
void VerifyRootQueryResult(const TypeHierarchiesInfo &rootRes, size_t grandPos, size_t midPos, size_t leafPos)
{
    ASSERT_EQ(rootRes.name, "CthGrandBase");
    ASSERT_EQ(rootRes.pos, grandPos);
    EXPECT_EQ(rootRes.superHierarchies.subOrSuper.size(), 0U);
    VerifyRootSubChain(rootRes, midPos, leafPos);
}

// Multi-level inheritance: querying the leaf walks the whole super chain; querying the root walks
// the whole sub chain. Both queries reuse a single context.
TEST_F(LspClassTypeHierarchyTests, MultiLevelInheritanceChain)
{
    std::vector<std::string> fileNames = {"cth1_multilevel.ets"};
    std::vector<std::string> fileContents = {R"(class CthGrandBase {
    a: number = 1;
}
class CthMidBase extends CthGrandBase {
    b: number = 2;
}
class CthLeaf extends CthMidBase {
    c: number = 3;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto grandPos = DeclNamePos(fileContents[0], "class", "CthGrandBase");
    const auto midPos = DeclNamePos(fileContents[0], "class", "CthMidBase");
    const auto leafPos = DeclNamePos(fileContents[0], "class", "CthLeaf");

    // Query on the leaf class: full super chain Leaf -> MidBase -> GrandBase.
    auto leafRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, leafPos);
    VerifyLeafQueryResult(leafRes, filePaths[0], leafPos, midPos, grandPos);

    // Query on the root class: full sub chain GrandBase -> MidBase -> Leaf.
    auto rootRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, grandPos);
    initializer.DestroyContext(context);

    VerifyRootQueryResult(rootRes, grandPos, midPos, leafPos);
}

// Cross-file parent/child: the child context resolves the parent declared in another file
// (super direction), and the parent declaration node finds the child in the other file's AST
// (sub direction).
TEST_F(LspClassTypeHierarchyTests, CrossFileParentAndChild)
{
    std::vector<std::string> fileNames = {"cth1_cross_base.ets", "cth1_cross_child.ets"};
    std::vector<std::string> fileContents = {R"(export class CthCrossAnimal {
    leg: number = 4;
}
)",
                                             R"(import { CthCrossAnimal } from "./cth1_cross_base";
export class CthCrossDog extends CthCrossAnimal {
    name: string = "dog";
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    Initializer initializer;
    auto *childContext = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(childContext, nullptr);

    const auto animalPos = DeclNamePos(fileContents[0], "class", "CthCrossAnimal");
    const auto dogPos = DeclNamePos(fileContents[1], "class", "CthCrossDog");

    // Super direction: query the child class in its own context; the parent entry must point
    // to the base file.
    auto childRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(childContext, dogPos);
    ASSERT_EQ(childRes.name, "CthCrossDog");
    ASSERT_EQ(childRes.fileName, filePaths[1]);
    ASSERT_EQ(childRes.pos, dogPos);
    ASSERT_EQ(childRes.superHierarchies.subOrSuper.size(), 1U);
    EXPECT_EQ(childRes.superHierarchies.subOrSuper[0].name, "CthCrossAnimal");
    EXPECT_EQ(childRes.superHierarchies.subOrSuper[0].type, ::HierarchyType::CLASS);
    EXPECT_EQ(childRes.superHierarchies.subOrSuper[0].fileName, filePaths[0]);
    EXPECT_EQ(childRes.superHierarchies.subOrSuper[0].pos, animalPos);

    // Sub direction: take the parent declaration node from the base-file context and search
    // for children inside the child-file context.
    auto *baseContext = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(baseContext, nullptr);
    auto *node = ark::es2panda::lsp::GetTargetDeclarationNodeByPosition(baseContext, animalPos);
    ASSERT_NE(node, nullptr);
    auto baseRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(childContext, 0, node);
    initializer.DestroyContext(baseContext);
    initializer.DestroyContext(childContext);

    ASSERT_EQ(baseRes.name, "CthCrossAnimal");
    ASSERT_EQ(baseRes.fileName, filePaths[0]);
    ASSERT_EQ(baseRes.pos, animalPos);
    ASSERT_EQ(baseRes.subHierarchies.subOrSuper.size(), 1U);
    EXPECT_EQ(baseRes.subHierarchies.subOrSuper[0].name, "CthCrossDog");
    EXPECT_EQ(baseRes.subHierarchies.subOrSuper[0].type, ::HierarchyType::CLASS);
    EXPECT_EQ(baseRes.subHierarchies.subOrSuper[0].fileName, filePaths[1]);
    EXPECT_EQ(baseRes.subHierarchies.subOrSuper[0].pos, dogPos);
}

// Cursor on the class name, inside the constructor body and inside a method body must resolve
// to the same class declaration and produce identical hierarchies. One context, three positions.
TEST_F(LspClassTypeHierarchyTests, CursorOnNameConstructorAndBodyBehavesConsistently)
{
    std::vector<std::string> fileNames = {"cth1_cursor.ets"};
    std::vector<std::string> fileContents = {R"(export class CthWorker {
    id: number = 0;
    constructor() {
        this.id = 1;
    }
    run(): void {
        this.id = 2;
    }
}
export class CthSpecialWorker extends CthWorker {
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto namePos = DeclNamePos(fileContents[0], "class", "CthWorker");
    const auto ctorPos = fileContents[0].find("this.id = 1;");
    const auto bodyPos = fileContents[0].find("this.id = 2;");
    ASSERT_NE(ctorPos, std::string::npos);
    ASSERT_NE(bodyPos, std::string::npos);
    const auto subPos = DeclNamePos(fileContents[0], "class", "CthSpecialWorker");

    auto nameRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, namePos);
    auto ctorRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, ctorPos);
    auto bodyRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, bodyPos);
    initializer.DestroyContext(context);

    for (const auto *res : {&nameRes, &ctorRes, &bodyRes}) {
        EXPECT_EQ(res->name, "CthWorker");
        EXPECT_EQ(res->type, ::HierarchyType::CLASS);
        EXPECT_EQ(res->fileName, filePaths[0]);
        EXPECT_EQ(res->pos, namePos);
        EXPECT_EQ(res->superHierarchies.subOrSuper.size(), 0U);
        ASSERT_EQ(res->subHierarchies.subOrSuper.size(), 1U);
        EXPECT_EQ(res->subHierarchies.subOrSuper[0].name, "CthSpecialWorker");
        EXPECT_EQ(res->subHierarchies.subOrSuper[0].type, ::HierarchyType::CLASS);
        EXPECT_EQ(res->subHierarchies.subOrSuper[0].fileName, filePaths[0]);
        EXPECT_EQ(res->subHierarchies.subOrSuper[0].pos, subPos);
    }
}

// Same-name classes in different files must not mix: a subclass binding to file A's Twin must not
// be reported as a child of file B's Twin, even though the names are identical.
TEST_F(LspClassTypeHierarchyTests, SameNameClassesInDifferentFilesDoNotMix)
{
    std::vector<std::string> fileNames = {"cth1_twin_a.ets", "cth1_twin_b.ets", "cth1_twin_child.ets"};
    std::vector<std::string> fileContents = {R"(export class CthTwin {
    a: number = 1;
}
)",
                                             R"(export class CthTwin {
    b: string = "x";
}
)",
                                             R"(import { CthTwin } from "./cth1_twin_a";
export class CthTwinChild extends CthTwin {
    c: number = 3;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t childFileIndex = 2;
    Initializer initializer;
    auto *ctxA = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxA, nullptr);
    auto *ctxB = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxB, nullptr);
    auto *ctxC = initializer.CreateContext(filePaths[childFileIndex].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxC, nullptr);

    const auto twinAPos = DeclNamePos(fileContents[0], "class", "CthTwin");
    const auto twinBPos = DeclNamePos(fileContents[1], "class", "CthTwin");
    const auto childPos = DeclNamePos(fileContents[childFileIndex], "class", "CthTwinChild");

    auto *nodeA = ark::es2panda::lsp::GetTargetDeclarationNodeByPosition(ctxA, twinAPos);
    ASSERT_NE(nodeA, nullptr);
    auto *nodeB = ark::es2panda::lsp::GetTargetDeclarationNodeByPosition(ctxB, twinBPos);
    ASSERT_NE(nodeB, nullptr);

    // File A's Twin is the real parent of TwinChild.
    auto resA = ark::es2panda::lsp::GetTypeHierarchiesImpl(ctxC, 0, nodeA);
    ASSERT_EQ(resA.name, "CthTwin");
    ASSERT_EQ(resA.fileName, filePaths[0]);
    ASSERT_EQ(resA.pos, twinAPos);
    ASSERT_EQ(resA.subHierarchies.subOrSuper.size(), 1U);
    EXPECT_EQ(resA.subHierarchies.subOrSuper[0].name, "CthTwinChild");
    EXPECT_EQ(resA.subHierarchies.subOrSuper[0].fileName, filePaths[childFileIndex]);
    EXPECT_EQ(resA.subHierarchies.subOrSuper[0].pos, childPos);

    // File B's Twin has no children despite the identical name.
    auto resB = ark::es2panda::lsp::GetTypeHierarchiesImpl(ctxC, 0, nodeB);
    initializer.DestroyContext(ctxA);
    initializer.DestroyContext(ctxB);
    initializer.DestroyContext(ctxC);

    ASSERT_EQ(resB.name, "CthTwin");
    ASSERT_EQ(resB.fileName, filePaths[1]);
    ASSERT_EQ(resB.pos, twinBPos);
    EXPECT_EQ(resB.subHierarchies.subOrSuper.size(), 0U);
}

// Missing parent class and missing interface (both unresolvable) must not crash; the hierarchy
// degrades to empty super/sub lists. Also the cursor on the unresolved parent name still resolves
// to the child class declaration.
TEST_F(LspClassTypeHierarchyTests, MissingParentAndInterfaceDoNotCrash)
{
    std::vector<std::string> fileNames = {"cth1_missing.ets"};
    std::vector<std::string> fileContents = {R"(class CthLonely extends CthMissingBase implements CthMissingIface {
    value: number = 1;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto lonelyPos = DeclNamePos(fileContents[0], "class", "CthLonely");
    auto res = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, lonelyPos);

    const auto missingPos = fileContents[0].find("CthMissingBase");
    ASSERT_NE(missingPos, std::string::npos);
    auto missingRefRes = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, missingPos);
    initializer.DestroyContext(context);

    for (const auto *r : {&res, &missingRefRes}) {
        EXPECT_EQ(r->name, "CthLonely");
        EXPECT_EQ(r->type, ::HierarchyType::CLASS);
        EXPECT_EQ(r->fileName, filePaths[0]);
        EXPECT_EQ(r->pos, lonelyPos);
        EXPECT_EQ(r->superHierarchies.subOrSuper.size(), 0U);
        EXPECT_EQ(r->subHierarchies.subOrSuper.size(), 0U);
    }
}

// Generic base class chain: a concrete leaf extending GMid<number> resolves the generic
// declaration CthGMid and, transitively, CthGBase<T>.
TEST_F(LspClassTypeHierarchyTests, GenericBaseClassChain)
{
    std::vector<std::string> fileNames = {"cth1_generic.ets"};
    std::vector<std::string> fileContents = {R"(class CthGBase<T> {
    item: T = null!;
}
class CthGMid<T> extends CthGBase<T> {
    extra: number = 0;
}
class CthGLeaf extends CthGMid<number> {
    leaf: string = "x";
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto leafPos = DeclNamePos(fileContents[0], "class", "CthGLeaf");
    auto res = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, leafPos);
    initializer.DestroyContext(context);

    ASSERT_EQ(res.name, "CthGLeaf");
    ASSERT_EQ(res.type, ::HierarchyType::CLASS);
    ASSERT_EQ(res.fileName, filePaths[0]);
    ASSERT_EQ(res.pos, leafPos);
    ASSERT_EQ(res.superHierarchies.subOrSuper.size(), 1U);
    const auto &mid = res.superHierarchies.subOrSuper[0];
    ASSERT_EQ(mid.name, "CthGMid");
    EXPECT_EQ(mid.type, ::HierarchyType::CLASS);
    EXPECT_EQ(mid.fileName, filePaths[0]);
    EXPECT_EQ(mid.pos, DeclNamePos(fileContents[0], "class", "CthGMid"));
    ASSERT_EQ(mid.subOrSuper.size(), 1U);
    EXPECT_EQ(mid.subOrSuper[0].name, "CthGBase");
    EXPECT_EQ(mid.subOrSuper[0].type, ::HierarchyType::CLASS);
    EXPECT_EQ(mid.subOrSuper[0].fileName, filePaths[0]);
    EXPECT_EQ(mid.subOrSuper[0].pos, DeclNamePos(fileContents[0], "class", "CthGBase"));
    EXPECT_EQ(mid.subOrSuper[0].subOrSuper.size(), 0U);
}

// Wrapper-level test: GetClassHierarchies(contextList, fileName, pos) collects
// the class hierarchy items for the class at the given position. The position is
// a code-point offset, matching the Impl behavior.
TEST_F(LspClassTypeHierarchyTests, GetClassHierarchiesWrapper)
{
    std::vector<std::string> fileNames = {"cth1_hierarchies_wrapper.ets"};
    std::vector<std::string> fileContents = {R"(class CthWrapperBase {
    id: number = 0;
}
class CthWrapperDerived extends CthWrapperBase {
    extra: number = 1;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto derivedPos = DeclNamePos(fileContents[0], "class", "CthWrapperDerived");
    auto contextList = std::vector<es2panda_Context *> {context};
    LSPAPI const *lspApi = GetImpl();
    auto infos = lspApi->getClassHierarchiesImpl(&contextList, fileNames[0].c_str(), derivedPos);
    initializer.DestroyContext(context);

    // The wrapper reports the super class of the queried derived class.
    ASSERT_FALSE(infos.empty());
    EXPECT_EQ(infos[0].kind, ::ark::es2panda::lsp::ClassRelationKind::CLASS);
    EXPECT_NE(infos[0].description.find("CthWrapperBase"), std::string::npos);
}

// Wrapper-level test: GetClassHierarchies with a position that is not on a class
// or interface declaration returns no items.
TEST_F(LspClassTypeHierarchyTests, GetClassHierarchiesWrapperInvalidPosition)
{
    std::vector<std::string> fileNames = {"cth1_hierarchies_wrapper_invalid.ets"};
    std::vector<std::string> fileContents = {R"(let wrapperPlainVar: number = 1;
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto varPos = fileContents[0].find("wrapperPlainVar");
    ASSERT_NE(varPos, std::string::npos);
    auto contextList = std::vector<es2panda_Context *> {context};
    LSPAPI const *lspApi = GetImpl();
    auto infos = lspApi->getClassHierarchiesImpl(&contextList, fileNames[0].c_str(), varPos);
    initializer.DestroyContext(context);

    ASSERT_TRUE(infos.empty());
}

}  // namespace
