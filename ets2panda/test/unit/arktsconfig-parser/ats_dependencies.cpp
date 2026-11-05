/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "ir/expressions/literals/stringLiteral.h"
#include "public/public.h"
#include "util/arktsconfig.h"
#include "util/diagnosticEngine.h"

namespace {
namespace fs = std::filesystem;
using ark::es2panda::ArkTsConfig;

class AtsDependenciesTest : public testing::Test {
protected:
    void SetUp() override
    {
        root_ = fs::path(testing::TempDir()) /
                ("ats_dependencies_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(root_);
    }

    void TearDown() override
    {
        fs::remove_all(root_);
    }

    void Write(const std::string &path, const std::string &contents = "")
    {
        auto filePath = root_;
        filePath.append(path);
        fs::create_directories(filePath.parent_path());
        std::ofstream(filePath) << contents;
    }

    std::string Configure(const std::string &options, const std::string &projectRoot = "")
    {
        const auto root = projectRoot.empty() ? root_.generic_string() : projectRoot;
        Write("arktsconfig.json",
              R"({"compilerOptions": {"baseUrl": ".", "rootDir": ")" + root + R"(", )" + options + "}}");
        return (root_ / "arktsconfig.json").string();
    }

    std::string ConfigureEntry(const std::string &jsonEntry)
    {
        std::string options = R"("atsDependencies": {"har": {)";
        options += R"("searchPath": ["."], "moduleRoot": "har", "entryFile":)";
        options += jsonEntry;
        options += R"(}}, "paths": {"har/Index": ["har/Legacy.ets"]})";
        return Configure(options);
    }

    static constexpr const char *ats = R"("atsDependencies": {
        "har": {"searchPath": ["src/target", "src/main", "."], "moduleRoot": "har"}
    })";
    fs::path root_;
    ark::es2panda::util::DiagnosticEngine diagnostics_;
};

class AtsImportResolutionTest : public AtsDependenciesTest {
protected:
    void TearDown() override
    {
        es2panda_GetImpl(ES2PANDA_LIB_VERSION)->MemFinalize();
        AtsDependenciesTest::TearDown();
    }

    void CheckImport(const std::string &options, const std::string &importPath, const std::string &expectedFile)
    {
        const auto *impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
        const auto configOption = "--arktsconfig=" + Configure(options);
        const std::array args = {ES2PANDA_BIN_PATH, "--extension=ets", configOption.c_str()};
        std::unique_ptr<es2panda_Config, decltype(impl->DestroyConfig)> config {
            impl->CreateConfig(args.size(), args.data()), impl->DestroyConfig};
        ASSERT_NE(config, nullptr);

        const auto sourcePath = fs::path(root_).append("runtime/ets-processed/Main.ats").string();
        const auto source = "import { value } from '" + importPath + "';";
        std::unique_ptr<es2panda_Context, decltype(impl->DestroyContext)> context {
            impl->CreateContextFromString(config.get(), source.c_str(), sourcePath.c_str()), impl->DestroyContext};
        ASSERT_NE(context, nullptr);
        impl->ProceedToState(context.get(), ES2PANDA_STATE_PARSED);
        ASSERT_EQ(impl->ContextState(context.get()), ES2PANDA_STATE_PARSED);

        auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context.get());
        auto *literal =
            ctx->Allocator()->New<ark::es2panda::ir::StringLiteral>(ark::es2panda::util::StringView {importPath});
        auto resolved = ctx->parser->GetImportPathManager()->ResolvePathAPI(ctx->parserProgram, literal);
        const auto expectedPath = fs::path(root_).append(expectedFile).string();
        EXPECT_EQ(resolved.Utf8(), expectedPath);
        EXPECT_FALSE(ctx->diagnosticEngine->IsAnyError());
    }
};

TEST_F(AtsImportResolutionTest, LegacyDirectoryIndexAfterAtsMiss)
{
    Write("runtime/annotations/index.ts", "export function value(): int { return 1; }");
    CheckImport(R"("atsDependencies": {"@koalaui/runtime": {"moduleRoot": "runtime/ets-processed"}},
        "paths": {"@koalaui/runtime/annotations": ["runtime/annotations"]})",
                "@koalaui/runtime/annotations", "runtime/annotations/index.ts");
}

TEST_F(AtsImportResolutionTest, LegacyExtensionAfterAtsMiss)
{
    Write("runtime/legacy/helper.ets", "'use static'; export function value(): int { return 1; }");
    CheckImport(R"("atsDependencies": {"@koalaui/runtime": {"moduleRoot": "runtime/ets-processed"}},
        "paths": {"@koalaui/runtime/helper": ["runtime/legacy/helper"]})",
                "@koalaui/runtime/helper", "runtime/legacy/helper.ets");
}

TEST_F(AtsImportResolutionTest, AtsSourceStillPrecedesLegacyPaths)
{
    Write("runtime/ets-processed/helper.ats", "export function value(): int { return 1; }");
    Write("runtime/legacy/helper.ets", "'use static'; export function value(): int { return 2; }");
    CheckImport(R"("atsDependencies": {"@koalaui/runtime": {"moduleRoot": "runtime/ets-processed"}},
        "paths": {"@koalaui/runtime/helper": ["runtime/legacy/helper"]})",
                "@koalaui/runtime/helper", "runtime/ets-processed/helper.ats");
}

TEST_F(AtsDependenciesTest, SearchRootsAndModuleIdentityAreIndependent)
{
    Write("har/src/main/ets/Foo.ats");
    Write("har/src/target/ets/Foo.ats");
    ArkTsConfig config {Configure(ats), diagnostics_};
    ASSERT_TRUE(config.Parse());
    auto result = config.ResolvePath("har/ets/Foo", false);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, (root_ / "har/src/target/ets/Foo.ats").string());
    auto owner = config.FindAtsPackageByPath(*result);
    ASSERT_NE(owner, nullptr);
    EXPECT_EQ(owner->first, "har");
    EXPECT_EQ(fs::relative(*result, owner->second.moduleRoot).generic_string(), "src/target/ets/Foo.ats");
    EXPECT_EQ(config.ResolvePath("har/src/target/ets/Foo.ats", false), result);
}

TEST_F(AtsDependenciesTest, FallsBackToLegacyPathsWhenAtsSourceIsMissing)
{
    Write("legacy/Missing.ets");
    ArkTsConfig config {Configure(std::string(ats) + R"(, "paths": {"har": ["legacy"]})"), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("har/Missing", false), (root_ / "legacy/Missing").string());
}

TEST_F(AtsDependenciesTest, PackageMatchRequiresSegmentBoundary)
{
    Write("legacy/Foo.ets");
    ArkTsConfig config {Configure(std::string(ats) + R"(, "paths": {"harExtra": ["legacy"]})"), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.FindAtsDependency("harExtra/Foo"), nullptr);
    EXPECT_EQ(config.FindAtsPackageByPath((root_ / "harExtra/Foo.ats").string()), nullptr);
    EXPECT_TRUE(config.ResolvePath("harExtra/Foo", false).has_value());
}

TEST_F(AtsDependenciesTest, ScopedPackagesAndLongestOwnerRoot)
{
    Write("har/nested/Foo.ats");
    auto options = std::string(R"("atsDependencies": {
        "har": {"searchPath": ["."], "moduleRoot": "har"},
        "@scope/nested": {"searchPath": ["."], "moduleRoot": "har/nested"}
    })");
    ArkTsConfig config {Configure(options), diagnostics_};
    ASSERT_TRUE(config.Parse());
    auto result = config.ResolvePath("@scope/nested/Foo", false);
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(config.FindAtsPackageByPath(*result), nullptr);
    EXPECT_EQ(config.FindAtsPackageByPath(*result)->first, "@scope/nested");
}

TEST_F(AtsDependenciesTest, OmittedEntryIgnoresLegacyIndexMapping)
{
    Write("har/src/main/Index.ats");
    ArkTsConfig config {Configure(ats), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("har", false), (root_ / "har/src/main/Index.ats").string());
    Write("har/custom.ats");
    ArkTsConfig explicitConfig {Configure(std::string(ats) + R"(, "paths": {"har/Index": ["har/custom.ats"]})"),
                                diagnostics_};
    ASSERT_TRUE(explicitConfig.Parse());
    EXPECT_EQ(explicitConfig.ResolvePath("har", false), (root_ / "har/src/main/Index.ats").string());
    EXPECT_EQ(explicitConfig.ResolvePath("har/Index", false), (root_ / "har/src/main/Index.ats").string());
}

TEST_F(AtsDependenciesTest, AbsoluteEntryOverridesLegacyMapping)
{
    Write("har/Custom.ats");
    Write("har/Legacy.ets");
    Write("har/Index.ats");
    Write("har/nested/index.ats");
    auto entry = (root_ / "har/Custom.ats").generic_string();
    ArkTsConfig config {ConfigureEntry("\"" + entry + "\""), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("har", false), entry);
    EXPECT_EQ(config.ResolvePath("har/Index", false), (root_ / "har/Index.ats").string());
    EXPECT_EQ(config.ResolvePath("har/Index.ats", false), (root_ / "har/Index.ats").string());
    EXPECT_EQ(config.ResolvePath("har/nested", false), (root_ / "har/nested/index.ats").string());
    ASSERT_NE(config.FindAtsPackageByPath(entry), nullptr);
    EXPECT_EQ(config.FindAtsPackageByPath(entry)->second.moduleRoot, (root_ / "har").string());
}

TEST_F(AtsDependenciesTest, MissingIndexDoesNotUseEntryFile)
{
    Write("har/Custom.ats");
    Write("har/Legacy.ets");
    ArkTsConfig config {Configure(R"("atsDependencies": {
        "har": {"moduleRoot": "har", "entryFile": "Custom.ats"}
    })"),
                        diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("har", false), (root_ / "har/Custom.ats").string());
    EXPECT_FALSE(config.ResolvePath("har/Index", false).has_value());
}

TEST_F(AtsDependenciesTest, InvalidExplicitEntryFallsBackToLegacyPaths)
{
    Write("har/Legacy.ets");
    Write("har/Index.ats");
    Write("har/WithoutExtension.ats");
    Write("har/directory/index.ats");
    for (auto name : {"har/Missing.ats", "har/WithoutExtension", "har/directory"}) {
        auto entry = (root_ / name).generic_string();
        ArkTsConfig config {ConfigureEntry("\"" + entry + "\""), diagnostics_};
        ASSERT_TRUE(config.Parse());
        EXPECT_EQ(config.ResolvePath("har", false), (root_ / "har/Legacy.ets").string()) << name;
        EXPECT_EQ(config.ResolvePath("har/Index", false), (root_ / "har/Index.ats").string()) << name;
    }
}

TEST_F(AtsDependenciesTest, RelativeEntryUsesModuleRoot)
{
    Write("har/src/Custom.ats");
    for (auto entry : {R"("src/Custom.ats")", R"("./src/../src/Custom.ats")"}) {
        ArkTsConfig config {ConfigureEntry(entry), diagnostics_};
        ASSERT_TRUE(config.Parse());
        EXPECT_EQ(config.ResolvePath("har", false), (root_ / "har/src/Custom.ats").string());
    }
}

TEST_F(AtsDependenciesTest, OmittedSearchPathDefaultsToModuleRoot)
{
    Write("har/Foo.ats");
    Write("har/Index.ats");
    ArkTsConfig config {Configure(R"("atsDependencies": {"har": {"moduleRoot": "har"}})"), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.AtsDependencies().at("har").searchPath, std::vector<std::string> {(root_ / "har").string()});
    EXPECT_EQ(config.ResolvePath("har/Foo", false), (root_ / "har/Foo.ats").string());
    EXPECT_EQ(config.ResolvePath("har", false), (root_ / "har/Index.ats").string());
}

TEST_F(AtsDependenciesTest, ExplicitSearchPathDoesNotAppendModuleRoot)
{
    Write("har/RootOnly.ats");
    Write("har/src/Foo.ats");
    ArkTsConfig config {Configure(R"("atsDependencies": {"har": {
        "moduleRoot": "har", "searchPath": ["src"]}})"),
                        diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("har/Foo", false), (root_ / "har/src/Foo.ats").string());
    EXPECT_FALSE(config.ResolvePath("har/RootOnly", false).has_value());
}

TEST_F(AtsDependenciesTest, AbsolutePathsAreNotRebased)
{
    Write("har/src/Foo.ats");
    Write("har/Custom.ats");
    auto moduleRoot = (root_ / "har").generic_string();
    auto options = R"("atsDependencies": {"har": {"moduleRoot": ")" + moduleRoot + R"(", "searchPath": [")" +
                   moduleRoot + R"(/src", "."], "entryFile": ")" + moduleRoot + R"(/Custom.ats"}})";
    ArkTsConfig config {Configure(options, (root_ / "har/src").generic_string()), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.AtsDependencies().at("har").moduleRoot, moduleRoot);
    EXPECT_EQ(config.ResolvePath("har/Foo", false), (root_ / "har/src/Foo.ats").string());
    EXPECT_EQ(config.ResolvePath("har", false), (root_ / "har/Custom.ats").string());
}

TEST_F(AtsDependenciesTest, ModuleRootUsesProjectRootInsteadOfConfigDirectory)
{
    Write("project/har/Foo.ats");
    const auto projectRoot = (root_ / "project").generic_string();
    const auto relativeRoot = fs::relative(root_ / "project", fs::current_path()).generic_string();
    for (const auto &root : {projectRoot, relativeRoot}) {
        ArkTsConfig config {Configure(R"("atsDependencies": {"har": {"moduleRoot": "har"}})", root), diagnostics_};
        ASSERT_TRUE(config.Parse());
        EXPECT_EQ(config.ResolvePath("har/Foo", false), (root_ / "project/har/Foo.ats").string());
    }
}

TEST_F(AtsDependenciesTest, OmittedProjectRootKeepsWorkingDirectoryBase)
{
    Write("arktsconfig.json", R"({"compilerOptions": {"baseUrl": ".", "atsDependencies": {
        "har": {"moduleRoot": "har"}
    }}})");
    ArkTsConfig config {(root_ / "arktsconfig.json").string(), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.AtsDependencies().at("har").moduleRoot, (fs::current_path() / "har").generic_string());
}

TEST_F(AtsDependenciesTest, RejectsInvalidEntry)
{
    for (auto value : {R"("")", "null", "42", "[]"}) {
        ArkTsConfig config {ConfigureEntry(value), diagnostics_};
        EXPECT_FALSE(config.Parse()) << value;
    }
}

TEST_F(AtsDependenciesTest, InheritedEntryKeepsAbsolutePath)
{
    Write("har/Custom.ats");
    auto entry = (root_ / "har/Custom.ats").generic_string();
    ConfigureEntry(R"("Custom.ats")");
    Write("child/config.json", R"({"extends": "../arktsconfig.json", "compilerOptions": {"baseUrl": "."}})");
    ArkTsConfig config {(root_ / "child/config.json").string(), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("har", false), entry);
}

TEST_F(AtsDependenciesTest, DirectoryDoesNotHideFileInNextSearchRoot)
{
    fs::create_directories(root_ / "har/src/target/Foo");
    Write("har/src/main/Foo.ats");
    ArkTsConfig config {Configure(ats), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("har/Foo", false), (root_ / "har/src/main/Foo.ats").string());
}

TEST_F(AtsDependenciesTest, KeepsLegacyEtsPrecedenceAndExplicitAts)
{
    Write("har/Foo.ets");
    Write("har/Foo.ats");
    ArkTsConfig config {Configure(ats), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("har/Foo", false), (root_ / "har/Foo.ets").string());
    EXPECT_EQ(config.ResolvePath("har/Foo.ats", false), (root_ / "har/Foo.ats").string());
}

TEST_F(AtsDependenciesTest, ResolvesAtsDeclarationsAndDirectoryEntries)
{
    Write("har/Types.d.ats");
    Write("har/nested/index.d.ats");
    ArkTsConfig config {Configure(ats), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("har/Types", false), (root_ / "har/Types.d.ats").string());
    EXPECT_EQ(config.ResolvePath("har/Types.d.ats", false), config.ResolvePath("har/Types", false));
    EXPECT_EQ(config.ResolvePath("har/nested", false), (root_ / "har/nested/index.d.ats").string());
}

TEST_F(AtsDependenciesTest, DoesNotResolveDynamicSourcesOrEscapeRoot)
{
    Write("har/Dynamic.ts");
    Write("Outside.ats");
    ArkTsConfig config {Configure(ats), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_FALSE(config.ResolvePath("har/Dynamic", false).has_value());
    EXPECT_FALSE(config.ResolvePath("har/Dynamic.ts", false).has_value());
    EXPECT_FALSE(config.ResolvePath("har/../../../Outside", false).has_value());
}

TEST_F(AtsDependenciesTest, DependenciesPrecedeAtsInBothContexts)
{
    Write("har/Foo.ats");
    auto options = std::string(ats) + R"(, "dependencies": {
        "har/Foo": {"language": "js", "ohmUrl": "dynamic/foo"}
    })";
    ArkTsConfig config {Configure(options), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("har/Foo", true), "har/Foo");
    EXPECT_EQ(config.ResolvePath("har/Foo", false), "har/Foo");
}

TEST_F(AtsDependenciesTest, DynamicContextResolvesAtsWhenDependencyIsAbsent)
{
    Write("har/Foo.ats");
    Write("legacy/Foo.ets");
    auto options = std::string(ats) + R"(, "paths": {"har": ["legacy"]})";
    ArkTsConfig config {Configure(options), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("har/Foo", true), (root_ / "har/Foo.ats").string());
    Write("legacy/Missing.ets");
    EXPECT_EQ(config.ResolvePath("har/Missing", true), (root_ / "legacy/Missing").string());
}

TEST_F(AtsDependenciesTest, DynamicAliasPrecedesAtsPackage)
{
    Write("har/Foo.ats");
    auto options = std::string(ats) + R"(, "dependencies": {
        "dynamicFoo": {"language": "js", "ohmUrl": "dynamic/foo", "alias": ["har/Foo"]}
    })";
    ArkTsConfig config {Configure(options), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("har/Foo", true), "dynamicFoo");
    EXPECT_EQ(config.ResolvePath("har/Foo", false), (root_ / "har/Foo.ats").string());
}

TEST_F(AtsDependenciesTest, DependenciesPrecedeLegacyPathsInBothContexts)
{
    Write("legacy/Foo.ets");
    auto options = R"("paths": {"legacy": ["legacy"]}, "dependencies": {
        "legacy/Foo": {"language": "js", "ohmUrl": "dynamic/foo"},
        "dynamicOnly": {"language": "js", "ohmUrl": "dynamic/only"}
    })";
    ArkTsConfig config {Configure(options), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("legacy/Foo", false), "legacy/Foo");
    EXPECT_EQ(config.ResolvePath("legacy/Foo", true), "legacy/Foo");
    EXPECT_EQ(config.ResolvePath("dynamicOnly", false), "dynamicOnly");
}

TEST_F(AtsDependenciesTest, InheritedPathsKeepDeclaringProjectRoot)
{
    Write("base/lib/Foo.ats");
    Write("base/base.json", R"({"compilerOptions": {"rootDir": ")" + (root_ / "base").generic_string() +
                                R"(", "atsDependencies": {"lib": {"moduleRoot": "lib"}}}})");
    Write("child/config.json", R"({"extends": "../base/base.json", "compilerOptions": {"baseUrl": ".", "rootDir": ")" +
                                   root_.generic_string() + R"("}})");
    ArkTsConfig config {(root_ / "child/config.json").string(), diagnostics_};
    ASSERT_TRUE(config.Parse());
    EXPECT_EQ(config.ResolvePath("lib/Foo", false), (root_ / "base/lib/Foo.ats").string());
}

TEST_F(AtsDependenciesTest, RejectsInvalidRecords)
{
    const char *invalidOptions[] = {R"("atsDependencies": [])",
                                    R"("atsDependencies": null)",
                                    R"("atsDependencies": {"har": null})",
                                    R"("atsDependencies": {"har": {"searchPath": [], "moduleRoot": "har"}})",
                                    R"("atsDependencies": {"har": {"searchPath": [42], "moduleRoot": "har"}})",
                                    R"("atsDependencies": {"har": {"searchPath": [""], "moduleRoot": "har"}})",
                                    R"("atsDependencies": {"har": {"searchPath": null, "moduleRoot": "har"}})",
                                    R"("atsDependencies": {"har": {"searchPath": ".", "moduleRoot": "har"}})",
                                    R"("atsDependencies": {"har": {"moduleRoot": ""}})",
                                    R"("atsDependencies": {"har": {"moduleRoot": null}})",
                                    R"("atsDependencies": {"har": {"searchPath": ["."]}})"};
    for (const auto *options : invalidOptions) {
        ArkTsConfig config {Configure(options), diagnostics_};
        EXPECT_FALSE(config.Parse()) << options;
    }
}

TEST_F(AtsDependenciesTest, IncludeMatchesAtsSources)
{
    ArkTsConfig::Pattern pattern {"**/*", root_.string()};
    EXPECT_TRUE(pattern.Match((root_ / "src/Foo.ats").string()));
    EXPECT_TRUE(pattern.Match((root_ / "src/Legacy.ets").string()));
}
}  // namespace
