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

#include <gtest/gtest.h>
#include "symbol.h"

using ark::es2panda::gluegen::SymbolKind;
using ark::es2panda::gluegen::SymbolNodeManager;

namespace {

TEST(GluegenSymbolTest, CreateSymbolNodeSetsRequiredFields)
{
    SymbolNodeManager manager;
    auto *node = manager.CreateSymbolNode("Foo", SymbolKind::CLASS);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->name, "Foo");
    EXPECT_EQ(node->kind, SymbolKind::CLASS);
    EXPECT_FALSE(node->runtimeName.has_value());
    EXPECT_FALSE(node->localName.has_value());
    EXPECT_FALSE(node->initModuleParam.has_value());
    EXPECT_FALSE(node->source.has_value());
    EXPECT_TRUE(node->children.empty());
}

TEST(GluegenSymbolTest, CreateSymbolNodeSetsAllOptionalFields)
{
    SymbolNodeManager manager;
    auto *node = manager.CreateSymbolNode(
        "alias", SymbolKind::DYNAMIC_RE_EXPORT, std::optional<std::string>("Lentry/ETSGLOBAL;"),
        std::optional<std::string>("original"), std::optional<std::string>("moduleParam"),
        std::optional<std::string>("./other.ets"));
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->name, "alias");
    EXPECT_EQ(node->kind, SymbolKind::DYNAMIC_RE_EXPORT);
    ASSERT_TRUE(node->runtimeName.has_value());
    EXPECT_EQ(*node->runtimeName, "Lentry/ETSGLOBAL;");
    ASSERT_TRUE(node->localName.has_value());
    EXPECT_EQ(*node->localName, "original");
    ASSERT_TRUE(node->initModuleParam.has_value());
    EXPECT_EQ(*node->initModuleParam, "moduleParam");
    ASSERT_TRUE(node->source.has_value());
    EXPECT_EQ(*node->source, "./other.ets");
}

TEST(GluegenSymbolTest, CreateSymbolNodeReturnsDistinctInstances)
{
    SymbolNodeManager manager;
    auto *first = manager.CreateSymbolNode("A", SymbolKind::CLASS);
    auto *second = manager.CreateSymbolNode("A", SymbolKind::CLASS);
    EXPECT_NE(first, second);
}

TEST(GluegenSymbolTest, RegisterFileRootSymbolNodeDoesNotCrash)
{
    SymbolNodeManager manager;
    auto *root = manager.CreateSymbolNode("root", SymbolKind::NAMESPACE);
    EXPECT_NO_THROW(manager.RegisterFileRootSymbolNode("/tmp/some/file.ets", root));
}

TEST(GluegenSymbolTest, ChildrenCanBePopulatedAndLookedUp)
{
    SymbolNodeManager manager;
    auto *parent = manager.CreateSymbolNode("ns", SymbolKind::NAMESPACE);
    auto *child = manager.CreateSymbolNode("member", SymbolKind::PROPERTY);
    parent->children["member"] = child;

    ASSERT_EQ(parent->children.size(), 1U);
    EXPECT_EQ(parent->children.at("member"), child);
}

TEST(GluegenSymbolTest, SeparateManagersOwnIndependentSymbolNodes)
{
    SymbolNodeManager first;
    SymbolNodeManager second;
    auto *root = first.CreateSymbolNode("root", SymbolKind::NAMESPACE);
    first.RegisterFileRootSymbolNode("/tmp/some/file.ets", root);

    // `second` is an entirely separate instance -- it never observes `first`'s registrations, and
    // creating a same-named node on it yields a distinct SymbolNode.
    auto *other = second.CreateSymbolNode("root", SymbolKind::NAMESPACE);
    EXPECT_NE(root, other);
}

TEST(GluegenSymbolTest, SymbolKindSerializesToExpectedStrings)
{
    nlohmann::json functionJson = SymbolKind::FUNCTION;
    nlohmann::json classJson = SymbolKind::CLASS;
    nlohmann::json propertyJson = SymbolKind::PROPERTY;
    nlohmann::json namespaceJson = SymbolKind::NAMESPACE;
    nlohmann::json dynamicReExportJson = SymbolKind::DYNAMIC_RE_EXPORT;
    nlohmann::json initModuleJson = SymbolKind::INIT_MODULE;

    EXPECT_EQ(functionJson, "function");
    EXPECT_EQ(classJson, "class");
    EXPECT_EQ(propertyJson, "property");
    EXPECT_EQ(namespaceJson, "namespace");
    EXPECT_EQ(dynamicReExportJson, "dynamic-re-export");
    EXPECT_EQ(initModuleJson, "init-module");
}

TEST(GluegenSymbolTest, SymbolKindRoundTripsThroughJson)
{
    for (auto kind : {SymbolKind::FUNCTION, SymbolKind::CLASS, SymbolKind::PROPERTY, SymbolKind::NAMESPACE,
                      SymbolKind::DYNAMIC_RE_EXPORT, SymbolKind::INIT_MODULE}) {
        nlohmann::json j = kind;
        EXPECT_EQ(j.get<SymbolKind>(), kind);
    }
}

}  // namespace
