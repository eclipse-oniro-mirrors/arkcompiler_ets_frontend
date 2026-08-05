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

#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include "gluec.h"
#include "gluel.h"
#include "symbol.h"
#include "utils.h"

using ark::es2panda::gluegen::Context;
using ark::es2panda::gluegen::ExternalLinkKind;
using ark::es2panda::gluegen::ExternalLinks;
using ark::es2panda::gluegen::GlueConfig;
using ark::es2panda::gluegen::Gluel;
using ark::es2panda::gluegen::IntermediateCache;
using ark::es2panda::gluegen::NamedBinding;
using ark::es2panda::gluegen::NormalizePath;
using ark::es2panda::gluegen::SymbolKind;
using ark::es2panda::gluegen::SymbolNode;
using ark::es2panda::gluegen::ToForwardSlashPath;

namespace {

// Fresh Context per TEST -- each TEST_F/TEST body below calls this exactly once, at its start, so
// MakeSymbol/Gluel constructions within a single test share one SymbolNodeManager while staying
// isolated from every other test.
SymbolNode *MakeSymbol(Context &context, const std::string &name, SymbolKind kind = SymbolKind::CLASS)
{
    return context.symbolNodeManager.CreateSymbolNode(name, kind);
}

// Applies the same normalization Gluel::Link()/IntermediateCache::sourceFile rely on, so a
// literal path string used here for a cache's sourceFile, a static ExternalLinks::sourceFile, a
// Link() target, and a result-lookup key all agree regardless of how NormalizePath happens to
// canonicalize it on this platform.
std::string Norm(const std::string &path)
{
    return NormalizePath(path);
}

// Gluel::Link() emits GlueConfig::files keyed in forward-slash form (see gluel.h's comment on
// `GlueConfig::files`), regardless of how NormalizePath canonicalizes the separator on this
// platform -- so result lookups in these tests must go through this instead of plain Norm().
std::string NormForward(const std::string &path)
{
    return ToForwardSlashPath(NormalizePath(path));
}

// `sourceFile` is normalized the same way Gluel::Link() normalizes its `targets` before looking
// them up, so a test can pass the same literal path string to both MakeCache() and Link() and
// have them match regardless of how NormalizePath happens to canonicalize it on this platform.
std::shared_ptr<IntermediateCache> MakeCache(const std::string &sourceFile)
{
    auto cache = std::make_shared<IntermediateCache>();
    cache->sourceFile = NormalizePath(sourceFile);
    cache->sourceMTime = "2026-01-01T00:00:00.000Z";
    cache->globalClassDescriptor = "Lentry/x/ETSGLOBAL;";
    return cache;
}

TEST(GluegenGluelTest, LinkReturnsErrorWhenTargetNotInCaches)
{
    Context context;
    std::vector<std::shared_ptr<IntermediateCache>> caches;
    Gluel gluel(caches, context);
    auto result = gluel.Link({"/does/not/exist.ets"});
    ASSERT_FALSE(result);
    EXPECT_NE(result.Error().find("has no intermediate cache"), std::string::npos);

    // The failure should also have been reported through DiagnosticEngine (not just returned as a
    // plain ark::Unexpected string) -- callers that install a DiagnosticConsumer must see it too.
    const auto records = context.diagnosticEngine.Records();
    ASSERT_EQ(records.size(), 1U);
    EXPECT_EQ(records[0].severity, ark::es2panda::gluegen::DiagnosticSeverity::ERROR);
    EXPECT_EQ(records[0].diagnostic.code,
              static_cast<uint32_t>(ark::es2panda::gluegen::DiagnosticCode::LINK_TARGET_NOT_FOUND));
    EXPECT_NE(records[0].diagnostic.description.find("has no intermediate cache"), std::string::npos);
}

TEST(GluegenGluelTest, LinkReturnsOwnRootSymbolsWithNoExternals)
{
    Context context;
    auto cache = MakeCache("/a.ets");
    cache->root["Foo"] = MakeSymbol(context, "Foo", SymbolKind::CLASS);

    Gluel gluel({cache}, context);
    auto result = gluel.Link({Norm("/a.ets")});
    ASSERT_TRUE(result);
    ASSERT_EQ(result.Value()->files.count(NormForward("/a.ets")), 1U);
    const auto &root = result.Value()->files.at(NormForward("/a.ets")).root;
    ASSERT_EQ(root.count("Foo"), 1U);
    EXPECT_EQ(root.at("Foo")->kind, SymbolKind::CLASS);
    EXPECT_EQ(result.Value()->status, "success");
    EXPECT_EQ(result.Value()->files.at(NormForward("/a.ets")).globalClassDescriptor, "Lentry/x/ETSGLOBAL;");
}

TEST(GluegenGluelTest, StaticBareStarReExportSplicesOtherExportsExcludingDefault)
{
    Context context;
    auto other = MakeCache("/other.ets");
    other->root["A"] = MakeSymbol(context, "A");
    other->root["B"] = MakeSymbol(context, "B");
    other->root["default"] = MakeSymbol(context, "default");

    auto main = MakeCache("/main.ets");
    main->root["Local"] = MakeSymbol(context, "Local");
    ExternalLinks link;
    link.sourceFile = Norm("/other.ets");
    link.kind = ExternalLinkKind::NAMESPACE;
    link.isDynamic = false;
    main->externals.push_back(link);

    Gluel gluel({main, other}, context);
    auto result = gluel.Link({Norm("/main.ets")});
    ASSERT_TRUE(result);
    const auto &root = result.Value()->files.at(NormForward("/main.ets")).root;
    EXPECT_EQ(root.count("Local"), 1U);
    EXPECT_EQ(root.count("A"), 1U);
    EXPECT_EQ(root.count("B"), 1U);
    // `default` is never spliced in by a bare `export *`.
    EXPECT_EQ(root.count("default"), 0U);
}

TEST(GluegenGluelTest, LocallyDeclaredSymbolTakesPrecedenceOverStarReExportOnNameClash)
{
    Context context;
    auto other = MakeCache("/other.ets");
    auto *otherA = MakeSymbol(context, "A", SymbolKind::FUNCTION);
    other->root["A"] = otherA;

    auto main = MakeCache("/main.ets");
    auto *mainA = MakeSymbol(context, "A", SymbolKind::CLASS);
    main->root["A"] = mainA;
    ExternalLinks link;
    link.sourceFile = Norm("/other.ets");
    link.kind = ExternalLinkKind::NAMESPACE;
    link.isDynamic = false;
    main->externals.push_back(link);

    Gluel gluel({main, other}, context);
    auto result = gluel.Link({Norm("/main.ets")});
    ASSERT_TRUE(result);
    const auto &root = result.Value()->files.at(NormForward("/main.ets")).root;
    ASSERT_EQ(root.count("A"), 1U);
    EXPECT_EQ(root.at("A"), mainA);
}

TEST(GluegenGluelTest, StaticNamespaceAliasReExportCreatesNamespaceNode)
{
    Context context;
    auto other = MakeCache("/other.ets");
    other->root["A"] = MakeSymbol(context, "A");
    other->root["default"] = MakeSymbol(context, "default");

    auto main = MakeCache("/main.ets");
    ExternalLinks link;
    link.sourceFile = Norm("/other.ets");
    link.kind = ExternalLinkKind::NAMESPACE;
    link.isDynamic = false;
    link.exportedName = "ns";
    main->externals.push_back(link);

    Gluel gluel({main, other}, context);
    auto result = gluel.Link({Norm("/main.ets")});
    ASSERT_TRUE(result);
    const auto &root = result.Value()->files.at(NormForward("/main.ets")).root;
    ASSERT_EQ(root.count("ns"), 1U);
    auto *nsNode = root.at("ns");
    EXPECT_EQ(nsNode->kind, SymbolKind::NAMESPACE);
    EXPECT_EQ(nsNode->children.count("A"), 1U);
    EXPECT_EQ(nsNode->children.count("default"), 0U);
}

TEST(GluegenGluelTest, StaticNamedReExportRenamesAndSkipsMissingBindings)
{
    Context context;
    auto other = MakeCache("/other.ets");
    auto *otherA = MakeSymbol(context, "A", SymbolKind::FUNCTION);
    other->root["A"] = otherA;

    auto main = MakeCache("/main.ets");
    ExternalLinks link;
    link.sourceFile = Norm("/other.ets");
    link.kind = ExternalLinkKind::NAMED;
    link.isDynamic = false;
    link.bindings.push_back(NamedBinding {"A", "RenamedA"});
    link.bindings.push_back(NamedBinding {"Missing", "Ignored"});
    main->externals.push_back(link);

    Gluel gluel({main, other}, context);
    auto result = gluel.Link({Norm("/main.ets")});
    ASSERT_TRUE(result);
    const auto &root = result.Value()->files.at(NormForward("/main.ets")).root;
    ASSERT_EQ(root.count("RenamedA"), 1U);
    EXPECT_EQ(root.at("RenamedA")->name, "RenamedA");
    EXPECT_EQ(root.at("RenamedA")->kind, SymbolKind::FUNCTION);
    EXPECT_NE(root.at("RenamedA"), otherA);  // renamed re-export is a clone, not the original node
    EXPECT_EQ(root.count("Ignored"), 0U);
}

TEST(GluegenGluelTest, DynamicNamespaceReExportRecordsDynamicReExportNode)
{
    Context context;
    auto main = MakeCache("/main.ets");
    ExternalLinks link;
    link.sourceFile = "some-dynamic-module";
    link.kind = ExternalLinkKind::NAMESPACE;
    link.isDynamic = true;
    link.exportedName = "ns";
    main->externals.push_back(link);

    Gluel gluel({main}, context);
    auto result = gluel.Link({Norm("/main.ets")});
    ASSERT_TRUE(result);
    const auto &root = result.Value()->files.at(NormForward("/main.ets")).root;
    ASSERT_EQ(root.count("ns"), 1U);
    auto *node = root.at("ns");
    EXPECT_EQ(node->kind, SymbolKind::DYNAMIC_RE_EXPORT);
    ASSERT_TRUE(node->localName.has_value());
    EXPECT_EQ(*node->localName, "*");
    ASSERT_TRUE(node->source.has_value());
    EXPECT_EQ(*node->source, "some-dynamic-module");
}

TEST(GluegenGluelTest, DynamicBareStarReExportIsKeyedBySourceFile)
{
    Context context;
    auto main = MakeCache("/main.ets");
    ExternalLinks link;
    link.sourceFile = "some-dynamic-module";
    link.kind = ExternalLinkKind::NAMESPACE;
    link.isDynamic = true;
    main->externals.push_back(link);

    Gluel gluel({main}, context);
    auto result = gluel.Link({Norm("/main.ets")});
    ASSERT_TRUE(result);
    const auto &root = result.Value()->files.at(NormForward("/main.ets")).root;
    ASSERT_EQ(root.count("some-dynamic-module"), 1U);
    EXPECT_EQ(root.at("some-dynamic-module")->kind, SymbolKind::DYNAMIC_RE_EXPORT);
}

TEST(GluegenGluelTest, DynamicNamedReExportRecordsOriginalNameAndSource)
{
    Context context;
    auto main = MakeCache("/main.ets");
    ExternalLinks link;
    link.sourceFile = "some-dynamic-module";
    link.kind = ExternalLinkKind::NAMED;
    link.isDynamic = true;
    link.bindings.push_back(NamedBinding {"default", "D"});
    main->externals.push_back(link);

    Gluel gluel({main}, context);
    auto result = gluel.Link({Norm("/main.ets")});
    ASSERT_TRUE(result);
    const auto &root = result.Value()->files.at(NormForward("/main.ets")).root;
    ASSERT_EQ(root.count("D"), 1U);
    auto *node = root.at("D");
    EXPECT_EQ(node->kind, SymbolKind::DYNAMIC_RE_EXPORT);
    ASSERT_TRUE(node->localName.has_value());
    EXPECT_EQ(*node->localName, "default");
    ASSERT_TRUE(node->source.has_value());
    EXPECT_EQ(*node->source, "some-dynamic-module");
}

TEST(GluegenGluelTest, ReExportCycleIsResolvedWithoutInfiniteRecursion)
{
    Context context;
    auto cacheA = MakeCache("/a.ets");
    cacheA->root["OnlyA"] = MakeSymbol(context, "OnlyA");
    ExternalLinks aToB;
    aToB.sourceFile = Norm("/b.ets");
    aToB.kind = ExternalLinkKind::NAMESPACE;
    aToB.isDynamic = false;
    cacheA->externals.push_back(aToB);

    auto cacheB = MakeCache("/b.ets");
    cacheB->root["OnlyB"] = MakeSymbol(context, "OnlyB");
    ExternalLinks bToA;
    bToA.sourceFile = Norm("/a.ets");
    bToA.kind = ExternalLinkKind::NAMESPACE;
    bToA.isDynamic = false;
    cacheB->externals.push_back(bToA);

    Gluel gluel({cacheA, cacheB}, context);
    auto result = gluel.Link({Norm("/a.ets")});
    ASSERT_TRUE(result);
    const auto &root = result.Value()->files.at(NormForward("/a.ets")).root;
    EXPECT_EQ(root.count("OnlyA"), 1U);
    EXPECT_EQ(root.count("OnlyB"), 1U);
}

TEST(GluegenGluelTest, GlueConfigSerializeDeserializeRoundTrips)
{
    Context context;
    GlueConfig config;
    config.status = "success";
    ark::es2panda::gluegen::GlueFileConfig fileConfig;
    fileConfig.root["Foo"] = MakeSymbol(context, "Foo", SymbolKind::CLASS);
    fileConfig.globalClassDescriptor = "Lentry/a/ETSGLOBAL;";
    config.files["/a.ets"] = fileConfig;

    auto jsonStr = GlueConfig::serialize(config);
    auto roundTripped = GlueConfig::deserialize(jsonStr, context.symbolNodeManager);

    EXPECT_EQ(roundTripped.status, "success");
    ASSERT_EQ(roundTripped.files.count("/a.ets"), 1U);
    ASSERT_EQ(roundTripped.files.at("/a.ets").root.count("Foo"), 1U);
    EXPECT_EQ(roundTripped.files.at("/a.ets").root.at("Foo")->kind, SymbolKind::CLASS);
    EXPECT_EQ(roundTripped.files.at("/a.ets").globalClassDescriptor, "Lentry/a/ETSGLOBAL;");
}

}  // namespace
