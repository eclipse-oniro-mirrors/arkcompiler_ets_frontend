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
#include <array>

#include "arktsconfig_test.h"
#include "util/fsQueryCache.h"

namespace ark::es2panda::util::test {
namespace {

class ArkTsWorkspaceTest : public ArkTsConfigTest {
protected:
    std::string Profile(std::string_view searchDir, std::string_view entry) const
    {
        return Project(R"("atsDependencies":{"@app/ui":{"moduleRoot":"packages/ui","searchPath":[)" +
                           JsonPath(fs::path(searchDir)) + R"(,"common"],"entryFile":)" + JsonPath(fs::path(entry)) +
                           R"(}})",
                       R"("extends":"base.json")");
    }

    void ExpectResolution(const ArkTsConfig &config, std::string_view request, std::string_view file,
                          FsQueryCache &cache)
    {
        for (bool dynamic : {false, true}) {
            SCOPED_TRACE(dynamic ? "dynamic importer" : "static importer");
            const auto resolved = config.ResolvePath(request, dynamic, &cache);
            ASSERT_TRUE(resolved.has_value()) << request;
            EXPECT_EQ(*resolved, Path(file).string()) << request;
        }
    }

    using CompilationList = std::vector<std::pair<std::string, std::string>>;

    void ExpectCompilationList(const ArkTsConfig &config, CompilationList expected)
    {
        auto actual = FindProjectSources(config);
        for (auto &[input, output] : expected) {
            input = Path(input).string();
            output = Path(output).string();
        }
        std::sort(actual.begin(), actual.end());
        std::sort(expected.begin(), expected.end());
        EXPECT_EQ(actual, expected);
        EXPECT_FALSE(diagnostics_.IsAnyError());
    }
};

TEST_F(ArkTsWorkspaceTest, DesktopAndPhoneShareTheBaseButSelectDifferentEntriesAndOverlays)
{
    Write("profiles/base.json", Project(R"("atsDependencies":{"@app/model":{"moduleRoot":"packages/model"}},
                                          "paths":{"fallback":["../generated/Fallback.ats"]})"));
    Write("profiles/phone.json", Profile("phone", "phone/Index.ats"));
    Write("profiles/desktop.json", Profile("desktop", "desktop/Index.ets"));
    for (const auto *file : {"packages/ui/phone/Index.ats", "packages/ui/desktop/Index.ets",
                             "packages/ui/phone/Layout.ats", "packages/ui/desktop/Layout.ats",
                             "packages/ui/common/Colors.d.ats", "packages/model/Data.ats", "generated/Fallback.ats"}) {
        Write(file);
    }
    ArkTsConfig phone(Path("profiles/phone.json").string(), diagnostics_);
    ArkTsConfig desktop(Path("profiles/desktop.json").string(), diagnostics_);
    ASSERT_TRUE(phone.Parse());
    ASSERT_TRUE(desktop.Parse());
    FsQueryCache cache;
    ExpectResolution(phone, "@app/ui", "packages/ui/phone/Index.ats", cache);
    ExpectResolution(desktop, "@app/ui", "packages/ui/desktop/Index.ets", cache);
    ExpectResolution(phone, "@app/ui/Layout", "packages/ui/phone/Layout.ats", cache);
    ExpectResolution(desktop, "@app/ui/Layout", "packages/ui/desktop/Layout.ats", cache);
    for (const auto *config : {&desktop, &phone}) {
        ExpectResolution(*config, "@app/ui/Colors", "packages/ui/common/Colors.d.ats", cache);
        ExpectResolution(*config, "@app/model/Data", "packages/model/Data.ats", cache);
        ExpectResolution(*config, "fallback", "generated/Fallback.ats", cache);
    }
}

TEST_F(ArkTsWorkspaceTest, ReloadingAnEditedManifestKeepsTheOldConfigurationUsable)
{
    Write("packages/v1/Entry.ets");
    Write("packages/v2/Entry.ats");
    const auto manifest = Path("app.json").string();
    Write("app.json", Project(R"("atsDependencies":{"api":{"moduleRoot":"packages/v1","entryFile":"Entry.ets"}})"));
    ArkTsConfig before(manifest, diagnostics_);
    ASSERT_TRUE(before.Parse());
    FsQueryCache cache;
    ExpectResolution(before, "api", "packages/v1/Entry.ets", cache);
    Write("app.json", Project(R"("atsDependencies":{"api":{"moduleRoot":"packages/v2","entryFile":"Entry.ats"}})"));
    ArkTsConfig after(manifest, diagnostics_);
    ASSERT_TRUE(after.Parse());
    ExpectResolution(after, "api", "packages/v2/Entry.ats", cache);
    ExpectResolution(before, "api", "packages/v1/Entry.ets", cache);
    EXPECT_EQ(before.FindAtsPackageByPath(Path("packages/v2/Entry.ats").string()), nullptr);
    EXPECT_EQ(after.FindAtsPackageByPath(Path("packages/v1/Entry.ets").string()), nullptr);
    EXPECT_FALSE(diagnostics_.IsAnyError());
}

TEST_F(ArkTsWorkspaceTest, FailedManifestReloadLeavesTheLastParsedPackageUsableDuringRepair)
{
    Write("packages/core/Ready.ats");
    const auto good = Project(R"("atsDependencies":{"core":{"moduleRoot":"packages/core"}})");
    Write("project.json", good);
    const auto manifest = Path("project.json").string();
    ArkTsConfig current(manifest, diagnostics_);
    ASSERT_TRUE(current.Parse());
    Write("project.json", Project(R"("atsDependencies":{"core":{"moduleRoot":"packages/core","searchPath":[]}})"));
    ArkTsConfig invalid(manifest, diagnostics_);
    EXPECT_FALSE(invalid.Parse());
    const auto &errors = diagnostics_.GetDiagnosticStorage(diagnostic::INVALID_VALUE.Type());
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front()->GetId(), diagnostic::INVALID_VALUE.Id());
    EXPECT_NE(errors.front()->Message().find("searchPath"), std::string::npos);
    diagnostics_.ClearDiagnostics();
    FsQueryCache cache;
    ExpectResolution(current, "core/Ready", "packages/core/Ready.ats", cache);
    Write("project.json", good);
    ArkTsConfig repaired(manifest, diagnostics_);
    ASSERT_TRUE(repaired.Parse());
    ExpectResolution(repaired, "core/Ready", "packages/core/Ready.ats", cache);
}

TEST_F(ArkTsWorkspaceTest, AddingAnEntryMovesResolutionFromLegacyMappingToAtsAndRemovingItMovesBack)
{
    Write("fallback/Initial.ets");
    Write("pkg/src/Util.ats");
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("paths":{"pkg":["fallback/Initial.ets"]},
        "atsDependencies":{"pkg":{"moduleRoot":"pkg", "entryFile":"generated/Entry.ats", "searchPath":["src"]}})")));
    FsQueryCache cache;
    ExpectResolution(*config_, "pkg", "fallback/Initial.ets", cache);
    ExpectResolution(*config_, "pkg/Util", "pkg/src/Util.ats", cache);
    Write("pkg/generated/Entry.ats");
    cache.Clear();
    ExpectResolution(*config_, "pkg", "pkg/generated/Entry.ats", cache);
    ExpectResolution(*config_, "pkg/Util", "pkg/src/Util.ats", cache);
    ASSERT_TRUE(fs::remove(Path("pkg/generated/Entry.ats")));
    cache.Clear();
    ExpectResolution(*config_, "pkg", "fallback/Initial.ets", cache);
    ExpectResolution(*config_, "pkg/Util", "pkg/src/Util.ats", cache);
}

TEST_F(ArkTsWorkspaceTest, RediscoveryReflectsSourceTreeEditsAndSkipsPreviousOutputs)
{
    Write("src/Main.ats");
    Write("src/generated/Old.d.ats");
    Write("dist/previous.ats");
    ASSERT_NO_FATAL_FAILURE(
        Parse(Project(R"("outDir":"dist")", R"("include":["src","dist"],"exclude":["src/generated"])")));
    ExpectCompilationList(*config_, {{"src/Main.ats", "dist/src/Main.abc"}});
    Write("src/features/First.ets");
    Write("dist/src/First.ets");
    ExpectCompilationList(
        *config_, {{"src/Main.ats", "dist/src/Main.abc"}, {"src/features/First.ets", "dist/src/features/First.abc"}});
    fs::rename(Path("src/features/First.ets"), Path("src/features/Renamed.d.ets"));
    ASSERT_TRUE(fs::remove(Path("src/Main.ats")));
    Write("src/generated/Another.ats");
    ExpectCompilationList(*config_, {{"src/features/Renamed.d.ets", "dist/src/features/Renamed.d.abc"}});
}

TEST_F(ArkTsWorkspaceTest, TwoOutputDirectoriesCompileTheSameExplicitInputRelativeToDifferentRoots)
{
    Write("application/src/widgets/View.ats");
    const auto source = JsonPath(Path("application/src/widgets/View.ats"));
    auto configFor = [&source](const fs::path &root, const fs::path &output) {
        return R"({"compilerOptions":{"rootDir":)" + JsonPath(root) + R"(,"outDir":)" + JsonPath(output) +
               R"(},"files":[)" + source + "]}";
    };
    Write("application/project.json", configFor(Path("application"), Path("dist/application")));
    Write("widgets/project.json", configFor(Path("application/src/widgets"), Path("dist/widgets")));
    ArkTsConfig application(Path("application/project.json").string(), diagnostics_);
    ArkTsConfig widgets(Path("widgets/project.json").string(), diagnostics_);
    ASSERT_TRUE(application.Parse());
    ASSERT_TRUE(widgets.Parse());
    ExpectCompilationList(application, {{"application/src/widgets/View.ats", "dist/application/src/widgets/View.abc"}});
    ExpectCompilationList(widgets, {{"application/src/widgets/View.ats", "dist/widgets/View.abc"}});
    EXPECT_TRUE(fs::is_directory(Path("dist/application/src/widgets")));
    EXPECT_TRUE(fs::is_directory(Path("dist/widgets")));
}

TEST_F(ArkTsWorkspaceTest, SwitchingDependenciesRebuildsLogicalNamesAndAliasesForTheSamePhysicalSource)
{
    Write("original/Utils.js");
    Write("declarations/Utils.d.ats");
    auto options = [](const std::string &name, const std::string &alias) {
        return R"("dependencies":{")" + name + R"(":{"language":"js", "ohmUrl":"original/utils",
              "sourceFilePath":"original/Utils.js", "path":"declarations/Utils.d.ats", "alias":[")" +
               alias + R"("]}})";
    };
    Write("test.json", Project(options("test.utils", "testAlias")));
    Write("release.json", Project(options("sdk.utils", "releaseAlias")));
    ArkTsConfig test(Path("test.json").string(), diagnostics_);
    ArkTsConfig release(Path("release.json").string(), diagnostics_);
    ASSERT_TRUE(test.Parse());
    ASSERT_TRUE(release.Parse());
    test.GenerateSourcePathMap();
    release.GenerateSourcePathMap();
    auto original = Path("original/Utils.js").string();
    ASSERT_EQ(test.SourcePathMap().size(), 1U);
    ASSERT_EQ(release.SourcePathMap().size(), 1U);
    EXPECT_EQ(test.SourcePathMap().at(original), "test.utils");
    EXPECT_EQ(release.SourcePathMap().at(original), "sdk.utils");
    EXPECT_EQ(test.ResolvePath("testAlias", true, nullptr), "test.utils");
    EXPECT_EQ(release.ResolvePath("releaseAlias", true, nullptr), "sdk.utils");
    EXPECT_FALSE(test.ResolvePath("releaseAlias", true, nullptr).has_value());
    EXPECT_FALSE(release.ResolvePath("testAlias", true, nullptr).has_value());
}

TEST_F(ArkTsWorkspaceTest, InheritedSourceSelectionKeepsItsDirectoryWhenTwoChildrenChangeOutputRoots)
{
    Write("shared/src/Module.ats");
    Write("shared/src/private/Hidden.ats");
    Write("desktop/src/Unrelated.ats");
    Write("mobile/src/Unrelated.ats");
    Write("shared/base.json", Project("", R"("include":["src"],"exclude":["src/private"])"));
    Write("desktop/app.json", Project(R"("outDir":"artifacts")", R"("extends":"../shared/base.json")"));
    Write("mobile/app.json", Project(R"("outDir":"generated")", R"("extends":"../shared/base.json")"));
    ArkTsConfig desktop(Path("desktop/app.json").string(), diagnostics_);
    ArkTsConfig mobile(Path("mobile/app.json").string(), diagnostics_);
    ASSERT_TRUE(desktop.Parse());
    ASSERT_TRUE(mobile.Parse());
    ExpectCompilationList(desktop, {{"shared/src/Module.ats", "desktop/artifacts/shared/src/Module.abc"}});
    ExpectCompilationList(mobile, {{"shared/src/Module.ats", "mobile/generated/shared/src/Module.abc"}});
}

TEST_F(ArkTsWorkspaceTest, ExplicitSourceOrderIsPreservedWhenTwoDirectoriesHaveTheSameFileName)
{
    Write("alpha/Main.ats");
    Write("beta/Main.ats");
    Write("gamma/Main.ats");
    ASSERT_NO_FATAL_FAILURE(
        Parse(Project(R"("outDir":"out")", R"("files":["beta/Main.ats","gamma/Main.ats","alpha/Main.ats"])")));
    const auto sources = FindProjectSources(*config_);
    ASSERT_EQ(sources.size(), 3U);
    const std::array<const char *, 3> expected {"beta", "gamma", "alpha"};
    for (size_t i = 0; i < expected.size(); ++i) {
        SCOPED_TRACE(i);
        EXPECT_EQ(sources[i].first, (Path(expected[i]) / "Main.ats").string());
        EXPECT_EQ(sources[i].second, (Path("out") / expected[i] / "Main.abc").string());
    }
}

TEST_F(ArkTsWorkspaceTest, ExactDependencyAliasAndLegacyMappingKeepTheirPrecedenceAcrossOverlayUpdates)
{
    Write("legacy/Button.ats");
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"(
        "atsDependencies":{"ui":{"moduleRoot":"overlay"}},
        "paths":{"ui/Button":["legacy/Button.ats"]},
        "dependencies":{"dynamicButton":{"language":"js", "ohmUrl":"ui/button", "alias":["ui/Button"]}}
    )")));
    FsQueryCache cache;
    auto expectButton = [this, &cache](const fs::path &expectedStatic) {
        EXPECT_EQ(config_->ResolvePath("ui/Button", false, &cache), expectedStatic.string());
        EXPECT_EQ(config_->ResolvePath("ui/Button", true, &cache), "dynamicButton");
    };
    expectButton(Path("legacy/Button.ats"));
    Write("overlay/Button.d.ats");
    cache.Clear();
    expectButton(Path("overlay/Button.d.ats"));
    Write("overlay/Button.ats");
    cache.Clear();
    expectButton(Path("overlay/Button.ats"));
    ASSERT_TRUE(fs::remove(Path("overlay/Button.ats")));
    ASSERT_TRUE(fs::remove(Path("overlay/Button.d.ats")));
    cache.Clear();
    expectButton(Path("legacy/Button.ats"));
}

TEST_F(ArkTsWorkspaceTest, SiblingPackagesWithSimilarNamesHaveDistinctSourceOwnersAcrossSharedSearchRoots)
{
    Write("packages/ui/src/Button.ats");
    Write("packages/ui-kit/src/Button.ats");
    Write("packages/ui-kit/src/widgets/Label.ats");
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("atsDependencies":{
        "@lib/ui":{"moduleRoot":"packages/ui", "searchPath":["src"]},
        "@lib/ui-kit":{"moduleRoot":"packages/ui-kit", "searchPath":["src"]},
        "@lib/widgets":{"moduleRoot":"packages/ui-kit/src/widgets"}
    })")));
    FsQueryCache cache;
    ExpectResolution(*config_, "@lib/ui/Button", "packages/ui/src/Button.ats", cache);
    ExpectResolution(*config_, "@lib/ui-kit/Button", "packages/ui-kit/src/Button.ats", cache);
    ExpectResolution(*config_, "@lib/ui-kit/widgets/Label", "packages/ui-kit/src/widgets/Label.ats", cache);
    ExpectResolution(*config_, "@lib/widgets/Label", "packages/ui-kit/src/widgets/Label.ats", cache);
    const auto *ui = config_->FindAtsPackageByPath(Path("packages/ui/src/Button.ats").string());
    const auto *kit = config_->FindAtsPackageByPath(Path("packages/ui-kit/src/Button.ats").string());
    const auto *widgets = config_->FindAtsPackageByPath(Path("packages/ui-kit/src/widgets/Label.ats").string());
    ASSERT_NE(ui, nullptr);
    ASSERT_NE(kit, nullptr);
    ASSERT_NE(widgets, nullptr);
    EXPECT_EQ(ui->first, "@lib/ui");
    EXPECT_EQ(kit->first, "@lib/ui-kit");
    EXPECT_EQ(widgets->first, "@lib/widgets");
    EXPECT_EQ(config_->FindAtsPackageByPath(Path("packages/ui-kitchen/Button.ats").string()), nullptr);
}

}  // namespace
}  // namespace ark::es2panda::util::test
