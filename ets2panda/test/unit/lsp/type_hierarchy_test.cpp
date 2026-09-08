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

class LspTypeHierarchyTests : public LSPAPITests {};

// Test: class extends class - superHierarchies should contain the base class
TEST_F(LspTypeHierarchyTests, ClassExtendsClassReturnsBaseInSuperHierarchies)
{
    std::vector<std::string> fileNames = {"th_base.ets", "th_derived.ets"};
    std::vector<std::string> fileContents = {
        R"(export class Base {
    value: number = 1;
})",
        R"(import { Base } from './th_base';
export class Derived extends Base {
    extra: string = "hello";
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    ark::es2panda::lsp::Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position on "Derived" class name
    const auto pos = fileContents[1].find("Derived");
    ASSERT_NE(pos, std::string::npos);
    auto res = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, pos);

    initializer.DestroyContext(context);

    // superHierarchies should contain Base (the parent class)
    ASSERT_EQ(res.superHierarchies.subOrSuper.size(), 1U);
    EXPECT_EQ(res.superHierarchies.subOrSuper[0].name, "Base");
    EXPECT_EQ(res.superHierarchies.subOrSuper[0].type, ::HierarchyType::CLASS);
}

// Test: interface extends interface - superHierarchies should contain the base interface
TEST_F(LspTypeHierarchyTests, InterfaceExtendsInterfaceReturnsBaseInSuperHierarchies)
{
    std::vector<std::string> fileNames = {"th_iface_base.ets", "th_iface_derived.ets"};
    std::vector<std::string> fileContents = {
        R"(export interface IBase {
    method(): void;
})",
        R"(import { IBase } from './th_iface_base';
export interface IDerived extends IBase {
    extraMethod(): void;
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    ark::es2panda::lsp::Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto pos = fileContents[1].find("IDerived");
    ASSERT_NE(pos, std::string::npos);
    auto res = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, pos);

    initializer.DestroyContext(context);

    // superHierarchies should contain IBase (the parent interface)
    ASSERT_EQ(res.superHierarchies.subOrSuper.size(), 1U);
    EXPECT_EQ(res.superHierarchies.subOrSuper[0].name, "IBase");
    EXPECT_EQ(res.superHierarchies.subOrSuper[0].type, ::HierarchyType::INTERFACE);
}

// Test: generic base class - superHierarchies should contain the generic base class
TEST_F(LspTypeHierarchyTests, GenericBaseClassReturnsBaseInSuperHierarchies)
{
    std::vector<std::string> fileNames = {"th_generic_base.ets", "th_generic_derived.ets"};
    std::vector<std::string> fileContents = {
        R"(export class Container<T> {
    item: T = null!;
})",
        R"(import { Container } from './th_generic_base';
export class NumberContainer extends Container<number> {
    extra: number = 0;
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    ark::es2panda::lsp::Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto pos = fileContents[1].find("NumberContainer");
    ASSERT_NE(pos, std::string::npos);
    auto res = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, pos);

    initializer.DestroyContext(context);

    // superHierarchies should contain Container (the generic base class)
    ASSERT_EQ(res.superHierarchies.subOrSuper.size(), 1U);
    EXPECT_EQ(res.superHierarchies.subOrSuper[0].name, "Container");
    EXPECT_EQ(res.superHierarchies.subOrSuper[0].type, ::HierarchyType::CLASS);
}

// Test: same-name classes in different files should not mix
TEST_F(LspTypeHierarchyTests, SameNameClassesInDifferentFilesDoNotMix)
{
    std::vector<std::string> fileNames = {"th_same_a.ets", "th_same_b.ets"};
    std::vector<std::string> fileContents = {
        R"(export class SameName {
    fieldA: number = 1;
})",
        R"(export class SameName {
    fieldB: string = "hello";
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    ark::es2panda::lsp::Initializer initializer;
    auto *ctxA = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxA, nullptr);
    auto *ctxB = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxB, nullptr);

    const auto posA = fileContents[0].find("SameName");
    ASSERT_NE(posA, std::string::npos);
    auto resA = ark::es2panda::lsp::GetTypeHierarchiesImpl(ctxA, posA);

    const auto posB = fileContents[1].find("SameName");
    ASSERT_NE(posB, std::string::npos);
    auto resB = ark::es2panda::lsp::GetTypeHierarchiesImpl(ctxB, posB);

    initializer.DestroyContext(ctxA);
    initializer.DestroyContext(ctxB);

    // Both classes have the same name but live in different files; the hierarchy
    // result for each context should reference its own file.
    EXPECT_EQ(resA.fileName, filePaths[0]);
    EXPECT_EQ(resB.fileName, filePaths[1]);
    // Neither should pick up the other file's class as a parent
    ASSERT_EQ(resA.superHierarchies.subOrSuper.size(), 0U);
    ASSERT_EQ(resB.superHierarchies.subOrSuper.size(), 0U);
}

// Test: missing parent class (referenced but not imported) should not crash
TEST_F(LspTypeHierarchyTests, MissingParentClassDoesNotCrash)
{
    std::vector<std::string> fileNames = {"th_missing_parent.ets"};
    std::vector<std::string> fileContents = {
        R"(class Orphan extends NonExistentBase {
    value: number = 1;
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    ark::es2panda::lsp::Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto pos = fileContents[0].find("Orphan");
    ASSERT_NE(pos, std::string::npos);
    // Should not crash even though NonExistentBase cannot be resolved
    auto res = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, pos);

    initializer.DestroyContext(context);

    // The result should be usable; superHierarchies may be empty since the parent is missing
    EXPECT_EQ(res.name, "Orphan");
    EXPECT_EQ(res.type, ::HierarchyType::CLASS);
}

// Test: class implements interface - superHierarchies should contain the interface
TEST_F(LspTypeHierarchyTests, ClassImplementsInterfaceReturnsInterfaceInSuperHierarchies)
{
    std::vector<std::string> fileNames = {"th_impl_iface.ets", "th_impl_class.ets"};
    std::vector<std::string> fileContents = {
        R"(export interface IRunnable {
    run(): void;
})",
        R"(import { IRunnable } from './th_impl_iface';
export class Runner implements IRunnable {
    run(): void {}
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    ark::es2panda::lsp::Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto pos = fileContents[1].find("Runner");
    ASSERT_NE(pos, std::string::npos);
    auto res = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, pos);

    initializer.DestroyContext(context);

    // superHierarchies should contain IRunnable (the implemented interface)
    ASSERT_EQ(res.superHierarchies.subOrSuper.size(), 1U);
    EXPECT_EQ(res.superHierarchies.subOrSuper[0].name, "IRunnable");
    EXPECT_EQ(res.superHierarchies.subOrSuper[0].type, ::HierarchyType::INTERFACE);
}

// Test: querying at a position with no class/interface returns empty hierarchies
TEST_F(LspTypeHierarchyTests, QueryAtNonTypePositionReturnsEmptyHierarchies)
{
    std::vector<std::string> fileNames = {"th_no_type.ets"};
    std::vector<std::string> fileContents = {
        R"(let standalone: number = 42;
console.log(standalone);)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileContents.size());

    ark::es2panda::lsp::Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto pos = fileContents[0].find("standalone");
    ASSERT_NE(pos, std::string::npos);
    // Should not crash when querying at a non-class/interface position
    auto res = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, pos);

    initializer.DestroyContext(context);

    // A variable has no inheritance relationships; super/sub hierarchies should be empty
    EXPECT_EQ(res.superHierarchies.subOrSuper.size(), 0U);
    EXPECT_EQ(res.subHierarchies.subOrSuper.size(), 0U);
}

}  // namespace
