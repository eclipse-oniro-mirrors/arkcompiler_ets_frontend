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

#include <algorithm>

#include "arktsconfig_test.h"

namespace ark::es2panda::util::test {
namespace {

struct GlobCase {
    const char *name;
    const char *pattern;
    const char *searchRoot;
    const char *source;
    bool wildcard;
    bool matches;
};

class ArkTsConfigGlobTest : public ArkTsConfigTest, public testing::WithParamInterface<GlobCase> {};

TEST_P(ArkTsConfigGlobTest, SearchesFromTheFixedPrefixAndSelectsExpectedSource)
{
    const auto &sample = GetParam();
    ArkTsConfig::Pattern glob(sample.pattern, root_.string());
    EXPECT_EQ(glob.IsPattern(), sample.wildcard);
    EXPECT_EQ(fs::path(glob.GetSearchRoot()), Path(sample.searchRoot).lexically_normal());
    EXPECT_EQ(glob.Match(Path(sample.source).string()), sample.matches);
}

const GlobCase GLOB_CASES[] = {
    {"LiteralFile", "src/Main.ats", "src/Main.ats", "src/Main.ats", false, true},
    {"OtherLiteralFile", "src/Main.ats", "src/Main.ats", "src/Other.ats", false, false},
    {"DirectoryAddsEts", "src", "src", "src/deep/Main.ets", false, true},
    {"DirectoryAddsTs", "src", "src", "src/Main.ts", false, true},
    {"DirectoryAddsSts", "src", "src", "src/Main.sts", false, true},
    {"DirectoryAddsAtsDeclaration", "src", "src", "src/Main.d.ats", false, true},
    {"DirectorySkipsJavascript", "src", "src", "src/Main.js", false, false},
    {"DirectorySkipsBytecode", "src", "src", "src/Main.abc", false, false},
    {"StarAcceptsEmptyStem", "src/Unit*.ats", "src", "src/Unit.ats", true, true},
    {"StarAcceptsSuffix", "src/Unit*.ats", "src", "src/UnitTest.ats", true, true},
    {"StarStopsAtDirectory", "src/*.ats", "src", "src/nested/Unit.ats", true, false},
    {"ExplicitExtensionFiltersOtherSource", "src/*.ets", "src", "src/Unit.ats", true, false},
    {"QuestionMatchesOne", "src/Unit?.ats", "src", "src/Unit1.ats", true, true},
    {"QuestionRejectsEmpty", "src/Unit?.ats", "src", "src/Unit.ats", true, false},
    {"QuestionRejectsTwo", "src/Unit?.ats", "src", "src/Unit12.ats", true, false},
    {"GlobstarMatchesDirectChild", "src/**/*.ats", "src", "src/Unit.ats", true, true},
    {"GlobstarMatchesGrandchild", "src/**/*.ats", "src", "src/a/b/Unit.ats", true, true},
    {"GlobstarRejectsAnotherTree", "src/**/*.ats", "src", "lib/Unit.ats", true, false},
    {"WildcardDirectory", "src/*/main.ts", "src", "src/app/main.ts", true, true},
    {"MixedWildcards", "src/v?/pkg/*.ats", "src", "src/v2/pkg/Unit.ats", true, true},
};

INSTANTIATE_TEST_SUITE_P(ProjectSelection, ArkTsConfigGlobTest, testing::ValuesIn(GLOB_CASES),
                         [](const auto &info) { return info.param.name; });

using SourcePair = std::pair<std::string, std::string>;
class ArkTsProjectSourcesTest : public ArkTsConfigTest {
protected:
    void SetUp() override
    {
        ASSERT_NO_FATAL_FAILURE(ModuleProjectTest::SetUp());
        Write("src/Main.ats");
        Write("src/sub/Dep.d.ets");
        Write("src/unused/Hidden.ts");
        Write("src/notes.txt", "");
        Write("out/previous/Generated.ats");
        Write("extra/Explicit.sts");
    }

    SourcePair Expected(std::string_view source, std::string_view output) const
    {
        return {Path(source).string(), Path(output).string()};
    }

    void ExpectSources(std::vector<SourcePair> expected)
    {
        auto sources = FindProjectSources(*config_);
        std::sort(sources.begin(), sources.end());
        std::sort(expected.begin(), expected.end());
        EXPECT_EQ(sources, expected);
        EXPECT_FALSE(diagnostics_.IsAnyError());
        for (const auto &pair : expected) {
            EXPECT_TRUE(fs::is_directory(fs::path(pair.second).parent_path()));
            EXPECT_FALSE(fs::exists(pair.second));
        }
    }
};

TEST_F(ArkTsProjectSourcesTest, DefaultSearchPrunesOutDirAndKeepsRelativeDirectoryStructure)
{
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("outDir":"out")")));
    ExpectSources({Expected("src/Main.ats", "out/src/Main.abc"), Expected("src/sub/Dep.d.ets", "out/src/sub/Dep.d.abc"),
                   Expected("src/unused/Hidden.ts", "out/src/unused/Hidden.abc"),
                   Expected("extra/Explicit.sts", "out/extra/Explicit.abc")});
}

TEST_F(ArkTsProjectSourcesTest, IncludeDirectoryHonorsExcludesWhileExplicitFilesAreAlwaysKept)
{
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("outDir":"out")", R"("files":["extra/Explicit.sts"],
        "include":["src"], "exclude":["src/unused", "extra"])")));
    ExpectSources({Expected("src/Main.ats", "out/src/Main.abc"), Expected("src/sub/Dep.d.ets", "out/src/sub/Dep.d.abc"),
                   Expected("extra/Explicit.sts", "out/extra/Explicit.abc")});
}

TEST_F(ArkTsProjectSourcesTest, LiteralIncludeCanSelectOneFileAndExcludesApplyToLiteralFiles)
{
    ASSERT_NO_FATAL_FAILURE(
        Parse(Project(R"("outDir":"out")", R"("include":["absent","src/Main.ats","src/sub/Dep.d.ets"],
        "exclude":["src/sub/Dep.d.ets"])")));
    ExpectSources({Expected("src/Main.ats", "out/src/Main.abc")});
}

TEST_F(ArkTsProjectSourcesTest, FilesWithoutIncludeDisableTheImplicitRecursiveScan)
{
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("outDir":"out")", R"("files":["src/sub/Dep.d.ets"])")));
    ExpectSources({Expected("src/sub/Dep.d.ets", "out/src/sub/Dep.d.abc")});
}

TEST_F(ArkTsProjectSourcesTest, MissingExplicitFileRejectsTheWholeCompilationList)
{
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("outDir":"out")", R"("files":["src/Main.ats","src/Absent.ts"])")));
    EXPECT_TRUE(FindProjectSources(*config_).empty());
    const auto &errors = diagnostics_.GetDiagnosticStorage(diagnostic::NO_FILE.Type());
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front()->GetId(), diagnostic::NO_FILE.Id());
    EXPECT_NE(errors.front()->Message().find("Absent.ts"), std::string::npos);
}

TEST_F(ArkTsProjectSourcesTest, SourceOutsideRootRejectsTheDestinationAndTheCompilationList)
{
    const auto options = R"({"compilerOptions":{"rootDir":)" + JsonPath(Path("src")) +
                         R"(,"outDir":"out"},"files":["src/Main.ats","extra/Explicit.sts"]})";
    ASSERT_NO_FATAL_FAILURE(Parse(options));
    EXPECT_TRUE(FindProjectSources(*config_).empty());
    const auto &errors = diagnostics_.GetDiagnosticStorage(diagnostic::NOT_ROOT_DIR.Type());
    ASSERT_EQ(errors.size(), 2U);
    EXPECT_EQ(errors[0]->GetId(), diagnostic::NOT_ROOT_DIR.Id());
    EXPECT_NE(errors[0]->Message().find("Explicit.sts"), std::string::npos);
    EXPECT_EQ(errors[1]->GetId(), diagnostic::INVALID_DESTINATION_FILE.Id());
}

TEST_F(ArkTsConfigTest, CompilerOptionsKeepIndependentBaseOutputAndGeneratedDeclarationLocations)
{
    const auto cache = Path("work/cache/nested");
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("baseUrl":"src", "outDir":"out", "declgenV2OutPath":"types/v2",
        "package":"@org/app", "useEmptyPackage":true, "cacheDir":)" +
                                          JsonPath(cache))));
    EXPECT_EQ(fs::path(config_->ConfigPath()), Path("arktsconfig.json"));
    EXPECT_EQ(fs::path(config_->BaseUrl()), Path("src"));
    EXPECT_EQ(fs::path(config_->RootDir()), root_);
    EXPECT_EQ(fs::path(config_->OutDir()), Path("out"));
    EXPECT_EQ(fs::path(config_->DeclgenV2OutPath()), Path("types/v2"));
    EXPECT_EQ(fs::path(config_->CacheDir()), cache);
    EXPECT_TRUE(fs::is_directory(cache));
    EXPECT_EQ(config_->Package(), "@org/app");
    EXPECT_TRUE(config_->UseUrl());
}

TEST_F(ArkTsConfigTest, AncestorNodeModulesSuppliesBaseConfigAndRetainsItsDeclaringDirectory)
{
    Write("node_modules/settings/base.json", R"({"compilerOptions":{"package":"shared", "baseUrl":"src",
        "paths":{"common":["Common.ats"]}}, "files":["Entry.ats"], "include":["lib/**/*.ats"],
            "exclude":["generated"]})");
    ASSERT_NO_FATAL_FAILURE(Parse(R"({"extends":"settings/base.json"})", "projects/app/config.json"));
    const auto base = Path("node_modules/settings");
    EXPECT_EQ(config_->Package(), "shared");
    EXPECT_EQ(fs::path(config_->BaseUrl()), base / "src");
    EXPECT_EQ(config_->Paths().at("common"), std::vector<std::string> {(base / "src/Common.ats").string()});
    EXPECT_EQ(config_->Files(), std::vector<std::string> {(base / "Entry.ats").string()});
    ASSERT_EQ(config_->Include().size(), 1U);
    EXPECT_TRUE(config_->Include().front().Match((base / "lib/Part.ats").string()));
    ASSERT_EQ(config_->Exclude().size(), 1U);
    EXPECT_TRUE(config_->Exclude().front().Match((base / "generated/Output.ats").string()));
}

TEST_F(ArkTsConfigTest, LocalExtendsFilePrecedesNodeModulesAndNearestNodeModulesWins)
{
    Write("node_modules/settings/base.json", R"({"compilerOptions":{"package":"outer"}})");
    Write("app/node_modules/settings/base.json", R"({"compilerOptions":{"package":"near"}})");
    Write("app/settings/base.json", R"({"compilerOptions":{"package":"local"}})");
    ASSERT_NO_FATAL_FAILURE(Parse(R"({"extends":"settings/base.json"})", "app/config.json"));
    EXPECT_EQ(config_->Package(), "local");
    ASSERT_TRUE(fs::remove(Path("app/settings/base.json")));
    ASSERT_NO_FATAL_FAILURE(Parse(R"({"extends":"settings/base.json"})", "app/config.json"));
    EXPECT_EQ(config_->Package(), "near");
}

TEST_F(ArkTsConfigTest, ChildOverridesCollectionsAndAppendsSubstitutionsToInheritedPaths)
{
    Write("base/config.json", R"({"compilerOptions":{"paths":{"pkg":["baseSrc"], "shared":["lib"]},
        "internalPaths":["base.internal"]}, "files":["old.ats"], "include":["old"], "exclude":["oldOut"]})");
    ASSERT_NO_FATAL_FAILURE(Parse(R"({"extends":"../base/config.json", "compilerOptions":{"paths":{"pkg":["childSrc"]},
        "internalPaths":["child.internal"]}, "files":["new.ats"], "include":["new"], "exclude":["newOut"]})",
                                  "child/config.json"));
    EXPECT_EQ(config_->Paths().at("pkg"),
              (std::vector<std::string> {Path("base/baseSrc").string(), Path("child/childSrc").string()}));
    EXPECT_EQ(config_->Paths().at("shared"), std::vector<std::string> {Path("base/lib").string()});
    EXPECT_FALSE(config_->IsInternalPath("base.internal"));
    EXPECT_TRUE(config_->IsInternalPath("child.internal"));
    EXPECT_EQ(config_->Files(), std::vector<std::string> {Path("child/new.ats").string()});
    ASSERT_EQ(config_->Include().size(), 1U);
    EXPECT_EQ(fs::path(config_->Include().front().GetSearchRoot()), Path("child/new"));
    ASSERT_EQ(config_->Exclude().size(), 1U);
    EXPECT_EQ(fs::path(config_->Exclude().front().GetSearchRoot()), Path("child/newOut"));
}

TEST_F(ArkTsConfigTest, StdlibOptionOverridesThreePrefixesAndPreservesApplicationAliases)
{
    for (const auto *folder : {"sdk/std", "sdk/escompat", "sdk/arkruntime"}) {
        ASSERT_TRUE(fs::create_directories(Path(folder)));
    }
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("paths":{"std":["old"],"arkruntime":["override"],"app":["src"]})")));
    config_->FixupWithStdlibOption(Path("sdk").string());
    for (const auto *prefix : {"std", "escompat", "arkruntime"}) {
        EXPECT_EQ(config_->Paths().at(prefix), std::vector<std::string> {(Path("sdk") / prefix).string()});
    }
    EXPECT_EQ(config_->Paths().at("app"), std::vector<std::string> {Path("src").string()});
}

TEST_F(ArkTsConfigTest, SingleStdLocationDerivesSiblingRuntimeAndKeepsAnExplicitOverride)
{
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("paths":{"std":["libs/std"]})")));
    config_->FixupWithoutStdlibOption();
    EXPECT_EQ(config_->Paths().at("arkruntime"), std::vector<std::string> {Path("libs/arkruntime").string()});
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("paths":{"std":["libs/std"],"arkruntime":["custom"]})")));
    config_->FixupWithoutStdlibOption();
    EXPECT_EQ(config_->Paths().at("arkruntime"), std::vector<std::string> {Path("custom").string()});
}

TEST_F(ArkTsConfigTest, MultipleOrMissingStdLocationsDoNotSynthesizeRuntimeAlias)
{
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("paths":{"std":["one/std","two/std"]})")));
    config_->FixupWithoutStdlibOption();
    EXPECT_EQ(config_->Paths().count(std::string_view {"arkruntime"}), 0U);
    ASSERT_NO_FATAL_FAILURE(Parse(Project("")));
    config_->FixupWithoutStdlibOption();
    EXPECT_TRUE(config_->Paths().empty());
}

TEST_F(ArkTsConfigTest, MocksRetainModuleKeysAndNormalizeSourcesAgainstBaseUrl)
{
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("baseUrl":"app", "mock":{"library":{"source":"./test/../Fake.ats"},
        "other":{"source":"Fake.ats"}, "time":{"source":"test/Clock.ats"}})")));
    const auto fake = Path("app/Fake.ats").string();
    const auto clock = Path("app/test/Clock.ats").string();
    ASSERT_EQ(config_->MockMap().size(), 3U);
    EXPECT_EQ(config_->MockMap().at("library"), fake);
    EXPECT_EQ(config_->MockMap().at("other"), fake);
    EXPECT_EQ(config_->MockMap().at("time"), clock);
    EXPECT_EQ(config_->MockSources(), (std::set<std::string, std::less<>> {fake, clock}));
}

TEST_F(ArkTsConfigTest, RelativeJoinPreservesLeadingParentsAndNormalizesInnerSegments)
{
    EXPECT_EQ(fs::path(JoinPaths("..", "../project/./src/../Main.ats")), fs::path("../..") / "project" / "Main.ats");
    EXPECT_EQ(fs::path(JoinPaths(root_.string(), Path("owned/Main.ats").string())), Path("owned/Main.ats"));
    EXPECT_EQ(fs::path(ParentPath(Path("nested/Main.ats").string())), Path("nested"));
}

TEST_F(ArkTsConfigTest, ReplacingInheritedPackageEntryAlsoReplacesItsSearchRoots)
{
    Write("base.json", Project(R"("atsDependencies":{
        "app":{"moduleRoot":"first", "searchPath":["old"], "entryFile":"wrong.ats"},
        "common":{"moduleRoot":"common", "entryFile":"Shared.ats"}})"));
    Write("common/Shared.ats");
    Write("second/new/Entry.ats");
    Write("second/old/Absent.ats");
    const auto options = Project(R"("atsDependencies":{"app":{"moduleRoot":"second","searchPath":["new"]}})",
                                 R"("extends":"base.json")");
    ASSERT_NO_FATAL_FAILURE(Parse(options));
    const auto &entry = config_->AtsDependencies().at("app");
    EXPECT_EQ(entry.moduleRoot, Path("second").string());
    EXPECT_EQ(entry.searchPath, std::vector<std::string> {Path("second/new").string()});
    EXPECT_TRUE(entry.entryFile.empty());
    EXPECT_EQ(config_->ResolvePath("app/Entry", false, nullptr), Path("second/new/Entry.ats").string());
    EXPECT_FALSE(config_->ResolvePath("app/Absent", false, nullptr).has_value());
    EXPECT_EQ(config_->ResolvePath("common", false, nullptr), Path("common/Shared.ats").string());
}

TEST_F(ArkTsConfigTest, LateCreatedGeneratedSourceResolvesAfterNormalizingASymlinkedSubstitution)
{
    ASSERT_TRUE(fs::create_directory(Path("real")));
    std::error_code error;
    fs::create_directory_symlink(Path("real"), Path("linked"), error);
    if (error) {
        GTEST_SKIP() << "Directory symlinks unavailable: " << error.message();
    }
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("paths":{"generated":["linked/New.ats"]})")));
    EXPECT_FALSE(config_->ResolvePath("generated", false, nullptr).has_value());
    Write("real/New.ats");
    config_->ResolveAllDependenciesInArkTsConfig();
    EXPECT_EQ(config_->Paths().at("generated"), std::vector<std::string> {Path("real/New.ats").string()});
    EXPECT_EQ(config_->ResolvePath("generated", false, nullptr), Path("real/New.ats").string());
}

}  // namespace
}  // namespace ark::es2panda::util::test
