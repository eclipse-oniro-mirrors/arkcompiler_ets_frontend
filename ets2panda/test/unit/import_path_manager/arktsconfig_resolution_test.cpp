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

#include "arktsconfig_test.h"
#include "util/fsQueryCache.h"

namespace ark::es2panda::util::test {
namespace {

struct ResolutionCase {
    const char *name;
    const char *specifier;
    const char *staticTarget;
    const char *dynamicTarget;
    bool staticIsFile;
    bool dynamicIsFile;
};

class ArkTsConfigResolutionTest : public ArkTsConfigTest, public testing::WithParamInterface<ResolutionCase> {
protected:
    void SetUp() override
    {
        ASSERT_NO_FATAL_FAILURE(ModuleProjectTest::SetUp());
        Write("ats/Entry.ats");
        Write("ats/Hit.ats");
        Write("legacy/Hit.ats");
        Write("legacy/Fallback.ats");
        Write("legacy/Exact.ats");
        Write("legacy/Alternate.ats");
        Write("legacy/Special.ats");
        ASSERT_NO_FATAL_FAILURE(Parse(Project(R"(
            "dependencies":{
                "foreign":{"language":"js", "ohmUrl":"app/foreign",
                    "alias":["pkg/Hit", "legacyKey", "aliasOnly", "exact"]},
                "exact":{"path":"exact.d.ats"},
                "archive":{"path":"library.abc", "mainFile":"Entry"}
            },
            "atsDependencies":{"pkg":{"moduleRoot":"ats", "entryFile":"Entry.ats"}},
            "paths":{
                "pkg/*":["legacy/*"], "legacyKey":["legacy/Exact.ats"], "exact":["legacy/Exact.ats"],
                "tryMany":["missing", "legacy/Alternate.ats", "legacy/Exact.ats"],
                "indexAlias/Index":["legacy/Special.ats"], "indexAlias":["legacy/Exact.ats"],
                "fallbackIndex/Index":["missing.ats"], "fallbackIndex":["legacy/Exact.ats"]
            })")));
    }

    void ExpectTarget(const char *expected, bool isFile, const std::optional<std::string> &actual)
    {
        if (expected == nullptr) {
            EXPECT_FALSE(actual.has_value());
            return;
        }
        ASSERT_TRUE(actual.has_value());
        EXPECT_EQ(*actual, isFile ? Path(expected).string() : expected);
    }
};

TEST_P(ArkTsConfigResolutionTest, StaticAndDynamicLookupsChooseTheExpectedCandidateWithAndWithoutCache)
{
    const auto &sample = GetParam();
    FsQueryCache cache;
    for (auto *queries : {static_cast<FsQueryCache *>(nullptr), &cache}) {
        SCOPED_TRACE(queries == nullptr ? "uncached" : "cached");
        ExpectTarget(sample.dynamicTarget, sample.dynamicIsFile, config_->ResolvePath(sample.specifier, true, queries));
        ExpectTarget(sample.staticTarget, sample.staticIsFile, config_->ResolvePath(sample.specifier, false, queries));
    }
}

const ResolutionCase RESOLUTIONS[] = {
    {"ConfiguredEntry", "pkg", "ats/Entry.ats", "ats/Entry.ats", true, true},
    {"SourceCompetesWithAlias", "pkg/Hit", "ats/Hit.ats", "foreign", true, false},
    {"PathsCompeteWithAlias", "legacyKey", "legacy/Exact.ats", "foreign", true, false},
    {"MissingAtsUsesLegacyPrototype", "pkg/Fallback", "legacy/Fallback", "legacy/Fallback", true, true},
    {"DependencyPrecedesPathsAndAlias", "exact", "exact", "exact", false, false},
    {"DeclarationlessDependency", "foreign", "foreign", "foreign", false, false},
    {"DeclarationlessSubpathIsUnknown", "foreign/sub", nullptr, nullptr, false, false},
    {"AliasWithoutCompetingFile", "aliasOnly", "foreign", "foreign", false, false},
    {"AliasesAreExact", "aliasOnly/sub", nullptr, nullptr, false, false},
    {"FirstExistingSubstitution", "tryMany", "legacy/Alternate.ats", "legacy/Alternate.ats", true, true},
    {"IndexAliasHasPriority", "indexAlias", "legacy/Special.ats", "legacy/Special.ats", true, true},
    {"MissingIndexAliasFallsBack", "fallbackIndex", "legacy/Exact.ats", "legacy/Exact.ats", true, true},
    {"BinaryUsesMainFile", "archive", "archive.Entry", "archive.Entry", false, false},
    {"BinarySubmoduleRetainsKey", "archive/Util", "archive/Util", "archive/Util", false, false},
    {"UnmappedName", "unknown", nullptr, nullptr, false, false},
};

INSTANTIATE_TEST_SUITE_P(ModuleLookup, ArkTsConfigResolutionTest, testing::ValuesIn(RESOLUTIONS),
                         [](const auto &info) { return info.param.name; });

TEST_F(ArkTsConfigTest, SourcePathMapIndexesOnlyPhysicalSourcesAndGenerationIsIdempotent)
{
    Write("input/Real.js");
    const auto original = Path("input/Real.js").string();
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("baseUrl":"input", "dependencies":{
        "physical":{"language":"js", "sourceFilePath":"./Real.js", "ohmUrl":"@org/pkg/Real", "mainFile":"Entry",
                    "path":"Real.d.ats", "alias":["renamed", "", 12, "second"]},
        "declaration":{"path":"Other.d.ats"}, "external":{"language":"js", "ohmUrl":"external"}
    })")));
    const auto &data = config_->Dependencies().at("physical");
    EXPECT_EQ(data.GetLanguage().GetId(), Language::Id::JS);
    EXPECT_EQ(data.OhmUrl(), "@org/pkg/Real");
    EXPECT_EQ(data.SourceFilePath(), original);
    EXPECT_EQ(fs::path(data.Path()), Path("input/Real.d.ats"));
    EXPECT_EQ(data.MainFile(), "Entry");
    EXPECT_EQ(data.Alias(), (std::vector<std::string> {"renamed", "second"}));
    EXPECT_EQ(config_->Dependencies().at("declaration").GetLanguage().GetId(), Language::Id::ETS);
    EXPECT_TRUE(config_->Dependencies().at("external").Path().empty());
    config_->GenerateSourcePathMap();
    ASSERT_EQ(config_->SourcePathMap().size(), 1U);
    EXPECT_EQ(config_->SourcePathMap().at(original), "physical");
    config_->GenerateSourcePathMap();
    ASSERT_EQ(config_->SourcePathMap().size(), 1U);
    EXPECT_EQ(config_->SourcePathMap().at(original), "physical");
}

TEST_F(ArkTsConfigTest, InternalNamespaceRequiresDotOrSlashBoundary)
{
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("internalPaths":["","@org/hidden","sdk.internal","sdk.internal"])")));
    EXPECT_EQ(config_->InternalPaths(), (std::set<std::string, std::less<>> {"@org/hidden", "sdk.internal"}));
    constexpr const char *members[] = {
        "@org/hidden", "@org/hidden/item", "sdk.internal", "sdk.internal.api", "sdk.internal/part",
    };
    for (const auto *member : members) {
        SCOPED_TRACE(member);
        EXPECT_TRUE(config_->IsInternalPath(member));
    }
    for (const auto *visible : {"", "@org", "@org/hidden2", "sdk", "sdk.internalized", "public/sdk.internal"}) {
        SCOPED_TRACE(visible);
        EXPECT_FALSE(config_->IsInternalPath(visible));
    }
}

TEST_F(ArkTsConfigTest, NestedArchivesOwnTheirSubmodulesAndDeclarationsRequireAnExactKey)
{
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("dependencies":{"lib":{"path":"all.abc", "mainFile":"All"},
        "lib/special":{"path":"one.abc", "mainFile":"One"}, "decl":{"path":"decl.d.ats"}})")));
    const auto nested = config_->FindInDependencies(std::string_view {"lib/special/Part"});
    ASSERT_TRUE(nested.has_value());
    EXPECT_EQ(nested->first, "lib/special");
    EXPECT_EQ(fs::path(nested->second.Path()), Path("one.abc"));
    const auto parent = config_->FindInDependencies(std::string_view {"lib/Ordinary"});
    ASSERT_TRUE(parent.has_value());
    EXPECT_EQ(parent->first, "lib");
    EXPECT_TRUE(config_->FindInDependencies(std::string_view {"decl"}).has_value());
    EXPECT_FALSE(config_->FindInDependencies(std::string_view {"decl/Part"}).has_value());
}

TEST_F(ArkTsConfigTest, NormalizingAllSubstitutionsHandlesExistingAndFutureFiles)
{
    Write("existing/Entry.ats");
    const auto raw = root_ / "existing" / ".." / "existing" / "Entry.ats";
    const auto future = root_ / "existing" / ".." / "generated" / "Later.ats";
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("paths":{"pkg":[)" + JsonPath(raw) + "," + JsonPath(future) + "]}")));
    config_->ResolveAllDependenciesInArkTsConfig();
    const std::vector<std::string> expected {Path("existing/Entry.ats").string(), Path("generated/Later.ats").string()};
    EXPECT_EQ(config_->Paths().at("pkg"), expected);
    config_->ResolveAllDependenciesInArkTsConfig();
    EXPECT_EQ(config_->Paths().at("pkg"), expected);
}

TEST_F(ArkTsConfigTest, EarlierDirectoryIsRememberedUntilALaterSearchRootProvidesAFile)
{
    ASSERT_TRUE(fs::create_directories(Path("pkg/first/Part")));
    ASSERT_TRUE(fs::create_directories(Path("pkg/second/Part")));
    ASSERT_NO_FATAL_FAILURE(
        Parse(Project(R"("atsDependencies":{"pkg":{"moduleRoot":"pkg","searchPath":["first","second"]}})")));
    FsQueryCache cache;
    EXPECT_EQ(config_->ResolvePath("pkg/Part", false, &cache), Path("pkg/first/Part").string());
    Write("pkg/second/Part/index.d.ats");
    EXPECT_EQ(config_->ResolvePath("pkg/Part", false, nullptr), Path("pkg/second/Part/index.d.ats").string());
    cache.Clear();
    EXPECT_EQ(config_->ResolvePath("pkg/Part", false, &cache), Path("pkg/second/Part/index.d.ats").string());
    Write("pkg/first/Part.ets");
    cache.Clear();
    EXPECT_EQ(config_->ResolvePath("pkg/Part", false, &cache), Path("pkg/first/Part.ets").string());
}

TEST_F(ArkTsConfigTest, ExplicitMissingEntryDoesNotImplicitlySelectAnAdjacentIndex)
{
    Write("pkg/index.ets");
    ASSERT_NO_FATAL_FAILURE(
        Parse(Project(R"("atsDependencies":{"lib":{"moduleRoot":"pkg","entryFile":"Entry.ats"}})")));
    EXPECT_FALSE(config_->ResolvePath("lib", false, nullptr).has_value());
    EXPECT_EQ(config_->ResolvePath("lib/index", false, nullptr), Path("pkg/index.ets").string());
    Write("pkg/Entry.ats");
    EXPECT_EQ(config_->ResolvePath("lib", false, nullptr), Path("pkg/Entry.ats").string());
}

TEST_F(ArkTsConfigTest, NestedPackageRootAndPublicNameAreSelectedIndependently)
{
    Write("vendor/child/private/Impl.ats");
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("atsDependencies":{
        "@org/outer":{"moduleRoot":"vendor"}, "short":{"moduleRoot":"vendor/child", "searchPath":["private"]}})")));
    const auto expected = Path("vendor/child/private/Impl.ats").string();
    EXPECT_EQ(config_->ResolvePath("short/Impl", false, nullptr), expected);
    EXPECT_EQ(config_->ResolvePath("@org/outer/child/private/Impl", false, nullptr), expected);
    auto owner = config_->FindAtsPackageByPath(expected);
    ASSERT_NE(owner, nullptr);
    EXPECT_EQ(owner->first, "short");
    EXPECT_EQ(config_->FindAtsPackageByPath(Path("vendor2/Other.ats").string()), nullptr);
}

struct SourceCandidate {
    const char *name;
    const char *candidate;
    const char *competitor;
};

class AtsSourceCandidateTest : public ArkTsConfigTest, public testing::WithParamInterface<SourceCandidate> {};

TEST_P(AtsSourceCandidateTest, HigherPrioritySourceWinsUntilItIsRemoved)
{
    const auto &test = GetParam();
    Write(test.candidate);
    Write(test.competitor);
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("atsDependencies":{"pkg":{"moduleRoot":"pkg"}})")));
    FsQueryCache cache;
    // A case-insensitive filesystem gives Index.ets the same priority as index.ets.
    if (std::string_view(test.competitor) == "pkg/Unit/Index.ets" && fs::exists(Path("pkg/Unit/index.ets"))) {
        GTEST_SKIP() << "Comparing index and Index requires a case-sensitive directory";
    }
    auto expectFile = [this, &cache](const char *expected) {
        auto selected = config_->ResolvePath("pkg/Unit", false, &cache);
        ASSERT_TRUE(selected.has_value());
        EXPECT_TRUE(fs::equivalent(*selected, Path(expected))) << *selected;
    };
    ASSERT_NO_FATAL_FAILURE(expectFile(test.candidate));
    ASSERT_TRUE(fs::remove(Path(test.candidate)));
    cache.Clear();
    ASSERT_NO_FATAL_FAILURE(expectFile(test.competitor));
}

const SourceCandidate ATS_CANDIDATES[] = {
    {"EtsBeforeEtsDeclaration", "pkg/Unit.ets", "pkg/Unit.d.ets"},
    {"EtsDeclarationBeforeAts", "pkg/Unit.d.ets", "pkg/Unit.ats"},
    {"AtsBeforeAtsDeclaration", "pkg/Unit.ats", "pkg/Unit.d.ats"},
    {"AtsDeclarationBeforeDirectoryIndex", "pkg/Unit.d.ats", "pkg/Unit/index.ets"},
    {"IndexEtsBeforeEtsDeclaration", "pkg/Unit/index.ets", "pkg/Unit/index.d.ets"},
    {"IndexEtsDeclarationBeforeAts", "pkg/Unit/index.d.ets", "pkg/Unit/index.ats"},
    {"IndexAtsBeforeAtsDeclaration", "pkg/Unit/index.ats", "pkg/Unit/index.d.ats"},
    {"LowercaseAtsDeclarationBeforeCapitalizedEts", "pkg/Unit/index.d.ats", "pkg/Unit/Index.ets"},
    {"CapitalizedEtsBeforeEtsDeclaration", "pkg/Unit/Index.ets", "pkg/Unit/Index.d.ets"},
    {"CapitalizedEtsDeclarationBeforeAts", "pkg/Unit/Index.d.ets", "pkg/Unit/Index.ats"},
    {"CapitalizedAtsBeforeAtsDeclaration", "pkg/Unit/Index.ats", "pkg/Unit/Index.d.ats"},
};

INSTANTIATE_TEST_SUITE_P(StaticSourceOrder, AtsSourceCandidateTest, testing::ValuesIn(ATS_CANDIDATES),
                         [](const auto &info) { return info.param.name; });

TEST_F(ArkTsConfigTest, StandardLibraryBinaryKeepsItsPackageSpecifierInBothContexts)
{
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("dependencies":{"std/core":{"path":"stdlib.abc", "mainFile":"Ignored"},
        "app":{"path":"application.abc", "mainFile":"Bootstrap"}})")));
    for (bool dynamic : {false, true}) {
        EXPECT_EQ(config_->ResolvePath("std/core", dynamic, nullptr), "std/core");
        EXPECT_EQ(config_->ResolvePath("app", dynamic, nullptr), "app.Bootstrap");
    }
}

TEST_F(ArkTsConfigTest, ChoosingExplicitAtsFileBypassesLegacyEtsWithTheSameStem)
{
    Write("pkg/Impl.ets");
    Write("pkg/Impl.ats");
    Write("pkg/Impl.d.ats");
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("atsDependencies":{"lib":{"moduleRoot":"pkg"}})")));
    FsQueryCache cache;
    EXPECT_EQ(config_->ResolvePath("lib/Impl", false, &cache), Path("pkg/Impl.ets").string());
    EXPECT_EQ(config_->ResolvePath("lib/Impl.ats", false, &cache), Path("pkg/Impl.ats").string());
    EXPECT_EQ(config_->ResolvePath("lib/Impl.d.ats", false, &cache), Path("pkg/Impl.d.ats").string());
}

}  // namespace
}  // namespace ark::es2panda::util::test
