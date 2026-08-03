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

#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <gtest/gtest.h>
#include "nlohmann/json.hpp"
#include "gluec.h"
#include "symbol.h"
#include "utils.h"

using ark::es2panda::gluegen::CacheManifestEntry;
using ark::es2panda::gluegen::Context;
using ark::es2panda::gluegen::ExternalLinkKind;
using ark::es2panda::gluegen::ExternalLinks;
using ark::es2panda::gluegen::IntermediateCache;
using ark::es2panda::gluegen::IntermediateCacheReader;
using ark::es2panda::gluegen::IntermediateCacheWriter;
using ark::es2panda::gluegen::NamedBinding;
using ark::es2panda::gluegen::NormalizePath;
using ark::es2panda::gluegen::SymbolKind;
using ark::es2panda::gluegen::SymbolNode;
using ark::es2panda::gluegen::SymbolNodeFromJson;
using ark::es2panda::gluegen::SymbolNodeManager;
using ark::es2panda::gluegen::SymbolNodeToJson;

namespace {

// RAII helper creating a fresh, empty temp directory for a single test and removing it (and
// everything under it) again on destruction, mirroring utils_test.cpp's manual-cleanup-via-
// std::error_code pattern for temp-file-based tests.
class ScopedTempDir {
public:
    explicit ScopedTempDir(const std::string &prefix)
    {
        auto unique = std::filesystem::temp_directory_path() /
                      (prefix + "_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        path_ = unique;
        std::filesystem::create_directories(path_);
    }

    ~ScopedTempDir()
    {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    ScopedTempDir(const ScopedTempDir &) = delete;
    ScopedTempDir &operator=(const ScopedTempDir &) = delete;

    const std::filesystem::path &Path() const
    {
        return path_;
    }

private:
    std::filesystem::path path_;
};

SymbolNode *MakeSymbol(SymbolNodeManager &manager, const std::string &name, SymbolKind kind)
{
    return manager.CreateSymbolNode(name, kind);
}

TEST(GluegenGluecTest, IntermediateCacheSerializeDeserializeRoundTrips)
{
    SymbolNodeManager manager;
    IntermediateCache cache;
    cache.sourceFile = "/some/path/a.ets";
    cache.sourceMTime = "2026-01-01T00:00:00.000Z";
    cache.globalClassDescriptor = "Lentry/a/ETSGLOBAL;";

    auto *fooClass = MakeSymbol(manager, "Foo", SymbolKind::CLASS);
    auto *nsNode = MakeSymbol(manager, "ns", SymbolKind::NAMESPACE);
    auto *nsChild = MakeSymbol(manager, "member", SymbolKind::PROPERTY);
    nsNode->children["member"] = nsChild;
    cache.root["Foo"] = fooClass;
    cache.root["ns"] = nsNode;

    ExternalLinks link;
    link.sourceFile = "/some/path/b.ets";
    link.kind = ExternalLinkKind::NAMED;
    link.isDynamic = false;
    link.bindings.push_back(NamedBinding {"A", "B"});
    cache.externals.push_back(link);

    auto jsonStr = IntermediateCache::serialize(cache);
    auto roundTripped = IntermediateCache::deserialize(jsonStr, manager);

    EXPECT_EQ(roundTripped.sourceFile, cache.sourceFile);
    EXPECT_EQ(roundTripped.sourceMTime, cache.sourceMTime);
    EXPECT_EQ(roundTripped.globalClassDescriptor, cache.globalClassDescriptor);
    ASSERT_EQ(roundTripped.root.count("Foo"), 1U);
    EXPECT_EQ(roundTripped.root.at("Foo")->name, "Foo");
    EXPECT_EQ(roundTripped.root.at("Foo")->kind, SymbolKind::CLASS);
    ASSERT_EQ(roundTripped.root.count("ns"), 1U);
    ASSERT_EQ(roundTripped.root.at("ns")->children.count("member"), 1U);
    EXPECT_EQ(roundTripped.root.at("ns")->children.at("member")->name, "member");

    ASSERT_EQ(roundTripped.externals.size(), 1U);
    EXPECT_EQ(roundTripped.externals[0].sourceFile, "/some/path/b.ets");
    EXPECT_EQ(roundTripped.externals[0].kind, ExternalLinkKind::NAMED);
    ASSERT_EQ(roundTripped.externals[0].bindings.size(), 1U);
    EXPECT_EQ(roundTripped.externals[0].bindings[0].importedName, "A");
    EXPECT_EQ(roundTripped.externals[0].bindings[0].exportedName, "B");
}

TEST(GluegenGluecTest, DeserializeToleratesMissingGlobalClassDescriptor)
{
    SymbolNodeManager manager;
    nlohmann::json j;
    j["sourceFile"] = "/some/path/a.ets";
    j["sourceMTime"] = "2026-01-01T00:00:00.000Z";
    j["root"] = nlohmann::json::object();
    j["externals"] = nlohmann::json::array();

    auto cache = IntermediateCache::deserialize(j.dump(), manager);
    EXPECT_EQ(cache.globalClassDescriptor, std::string());
}

TEST(GluegenGluecTest, SymbolNodeToJsonFromJsonRoundTripsNestedChildren)
{
    SymbolNodeManager manager;
    auto *parent = MakeSymbol(manager, "Outer", SymbolKind::NAMESPACE);
    auto *child = MakeSymbol(manager, "Inner", SymbolKind::CLASS);
    parent->children["Inner"] = child;

    auto j = SymbolNodeToJson(*parent);
    auto *roundTripped = SymbolNodeFromJson(j, manager);

    EXPECT_EQ(roundTripped->name, "Outer");
    EXPECT_EQ(roundTripped->kind, SymbolKind::NAMESPACE);
    ASSERT_EQ(roundTripped->children.count("Inner"), 1U);
    EXPECT_EQ(roundTripped->children.at("Inner")->name, "Inner");
    EXPECT_EQ(roundTripped->children.at("Inner")->kind, SymbolKind::CLASS);
}

TEST(GluegenGluecTest, SymbolNodeFromJsonToleratesNullChildren)
{
    SymbolNodeManager manager;
    nlohmann::json j;
    j["name"] = "Leaf";
    j["kind"] = SymbolKind::PROPERTY;
    j["runtimeName"] = nullptr;
    j["localName"] = nullptr;
    j["initModuleParam"] = nullptr;
    j["source"] = nullptr;
    j["children"] = nullptr;

    auto *node = SymbolNodeFromJson(j, manager);
    EXPECT_EQ(node->name, "Leaf");
    EXPECT_TRUE(node->children.empty());
}

TEST(GluegenGluecTest, NamedBindingAndExternalLinksRoundTripThroughJson)
{
    ExternalLinks link;
    link.sourceFile = "./dyn";
    link.kind = ExternalLinkKind::NAMESPACE;
    link.isDynamic = true;
    link.exportedName = "ns";

    nlohmann::json j = link;
    ExternalLinks roundTripped = j.get<ExternalLinks>();

    EXPECT_EQ(roundTripped.sourceFile, "./dyn");
    EXPECT_EQ(roundTripped.kind, ExternalLinkKind::NAMESPACE);
    EXPECT_TRUE(roundTripped.isDynamic);
    ASSERT_TRUE(roundTripped.exportedName.has_value());
    EXPECT_EQ(*roundTripped.exportedName, "ns");
}

TEST(GluegenGluecTest, CacheManifestEntryRoundTripsThroughJson)
{
    CacheManifestEntry entry {"abc123_file.ets.json", "2026-01-01T00:00:00.000Z"};
    nlohmann::json j = entry;
    CacheManifestEntry roundTripped = j.get<CacheManifestEntry>();

    EXPECT_EQ(roundTripped.cacheFile, entry.cacheFile);
    EXPECT_EQ(roundTripped.sourceMTime, entry.sourceMTime);
}

TEST(GluegenGluecTest, WriterThenReaderRoundTripsCacheThroughDisk)
{
    Context context;
    auto &manager = context.symbolNodeManager;
    ScopedTempDir cacheDir("gluec_writer_reader");
    ScopedTempDir sourceDir("gluec_writer_reader_src");

    const auto sourceFilePath = sourceDir.Path() / "a.ets";
    {
        std::ofstream out(sourceFilePath);
        out << "export class A {}\n";
    }
    const auto normalizedSourceFile = NormalizePath(sourceFilePath);

    auto cache = std::make_shared<IntermediateCache>();
    cache->sourceFile = normalizedSourceFile;
    cache->sourceMTime = "2026-01-01T00:00:00.000Z";
    cache->globalClassDescriptor = "Lentry/a/ETSGLOBAL;";
    cache->root["A"] = MakeSymbol(manager, "A", SymbolKind::CLASS);

    {
        IntermediateCacheWriter writer(cacheDir.Path().string(), context);
        writer.Enqueue(cache);
        auto errors = writer.Wait();
        EXPECT_TRUE(errors.empty());
    }

    EXPECT_TRUE(std::filesystem::exists(cacheDir.Path() / "manifest.json"));
    EXPECT_TRUE(std::filesystem::exists(cacheDir.Path() / "intermediates"));

    IntermediateCacheReader reader(cacheDir.Path().string(), context);
    auto cachedMTime = reader.CachedMTime(normalizedSourceFile);
    ASSERT_TRUE(cachedMTime.has_value());
    EXPECT_EQ(*cachedMTime, "2026-01-01T00:00:00.000Z");

    EXPECT_FALSE(reader.CachedMTime(NormalizePath(sourceDir.Path() / "does-not-exist.ets")).has_value());

    std::shared_ptr<IntermediateCache> loaded;
    reader.Enqueue(normalizedSourceFile,
                   [&loaded](std::shared_ptr<IntermediateCache> result) { loaded = std::move(result); });
    reader.Wait();

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->sourceFile, normalizedSourceFile);
    EXPECT_EQ(loaded->globalClassDescriptor, "Lentry/a/ETSGLOBAL;");
    ASSERT_EQ(loaded->root.count("A"), 1U);
    EXPECT_EQ(loaded->root.at("A")->kind, SymbolKind::CLASS);
}

TEST(GluegenGluecTest, ReaderReturnsNulloptAndNullptrForUnknownSourceFile)
{
    Context context;
    ScopedTempDir cacheDir("gluec_reader_miss");

    IntermediateCacheReader reader(cacheDir.Path().string(), context);
    EXPECT_FALSE(reader.CachedMTime("/never/written.ets").has_value());

    std::atomic<bool> called {false};
    std::shared_ptr<IntermediateCache> loaded;
    reader.Enqueue("/never/written.ets", [&loaded, &called](std::shared_ptr<IntermediateCache> result) {
        loaded = std::move(result);
        called = true;
    });
    reader.Wait();

    EXPECT_TRUE(called);
    EXPECT_EQ(loaded, nullptr);
}

}  // namespace
