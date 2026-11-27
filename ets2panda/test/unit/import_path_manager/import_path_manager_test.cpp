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

#include "import_path_manager_test.h"

namespace ark::es2panda::util::test {

TEST_F(ImportPathManagerTest, ExplicitAndOmittedExtensionsSelectTheExpectedFile)
{
    const std::vector<std::string> extensions {".ets", ".d.ets", ".ats", ".d.ats", ".etscache",
                                               ".sts", ".d.sts", ".ts",  ".d.ts",  ".js"};
    ASSERT_NO_FATAL_FAILURE(Start());
    Write("Target");
    for (const auto &extension : extensions) {
        Write("Target" + extension);
    }
    EXPECT_EQ(Resolve("./Target"), Path("Target"));
    ASSERT_TRUE(fs::remove(Path("Target")));
    for (const auto &extension : extensions) {
        SCOPED_TRACE(extension);
        auto candidate = Path("Target" + extension);
        Manager()->ClearResolutionCaches();
        EXPECT_EQ(Resolve("./Target"), candidate);
        EXPECT_EQ(Resolve("./Target" + extension), candidate);
        ASSERT_TRUE(fs::remove(candidate));
    }
}

TEST_F(ImportPathManagerTest, IndexOrderAndDirectoryFallback)
{
    ASSERT_NO_FATAL_FAILURE(Start());
    const std::vector<std::string> entries {"index.ets", "index.sts",   "index.ts",  "index.d.ets",
                                            "index.ats", "index.d.ats", "Index.ets", "Index.sts",
                                            "Index.ts",  "Index.d.ets", "Index.ats", "Index.d.ats"};
    // Case-insensitive hosts cannot represent index and Index simultaneously.
    for (const auto &entry : entries) {
        Write("dir/" + entry);
    }
    Write("dir.ats");
    EXPECT_EQ(Resolve("./dir"), Path("dir.ats"));
    ASSERT_TRUE(fs::remove(Path("dir.ats")));
    for (const auto &entry : entries) {
        const auto candidate = Path("dir/" + entry);
        if (!fs::exists(candidate)) {
            continue;
        }
        SCOPED_TRACE(entry);
        Manager()->ClearResolutionCaches();
        EXPECT_EQ(Resolve("./dir"), candidate);
        ASSERT_TRUE(fs::remove(candidate));
    }
    Manager()->ClearResolutionCaches();
    EXPECT_EQ(Resolve("./dir"), Path("dir"));
}

TEST_F(ImportPathManagerTest, RelativePathsNormalizeSeparatorsAndDotSegments)
{
    Write("sub/Entry.ats");
    Write("Target.ats");
    ASSERT_NO_FATAL_FAILURE(Start());
    auto *entry = Import("./sub/Entry");
    ASSERT_NE(entry, nullptr);
    auto *target = Import(".././Target", entry);
    ASSERT_NE(target, nullptr);
    EXPECT_EQ(target->GetImportInfo().ResolvedSource(), Path("Target.ats").string());
    EXPECT_EQ(Import(R"(..\Target)", entry), target);
}

TEST_F(ImportPathManagerTest, RootedSpecifierUsesBaseUrlAndPackageAliasUsesSearchRoot)
{
    Write("lib/Target.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("paths":{"@lib":["lib"]})"));
    const std::string rooted = std::string(1, Path::GetPathDelimiter()) + "lib/Target";
    EXPECT_EQ(Resolve(rooted), Path("lib/Target.ats"));
    EXPECT_EQ(Resolve("@lib/Target"), Path("lib/Target.ats"));
}

TEST_F(ImportPathManagerTest, AtsPackagePrecedesLegacyAliasAndFallsBackForMissingSubpath)
{
    Write("har/src/Hit.ats");
    Write("legacy/Hit.ets");
    Write("legacy/Miss/index.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("atsDependencies":{"har":{"moduleRoot":"har","searchPath":["src"]}},
        "paths":{"har":["legacy"]})"));
    auto *hit = Import("har/Hit");
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->GetImportInfo().ResolvedSource(), Path("har/src/Hit.ats").string());
    EXPECT_EQ(hit->ModuleName(), "har.src.Hit");
    EXPECT_EQ(Resolve("har/Miss"), Path("legacy/Miss/index.ats"));
}

TEST_F(ImportPathManagerTest, PackageRebasesBothAliasedAndUnmappedModuleNames)
{
    Write("src/Entry.ats");
    Write("other/Helper.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("package":"app","paths":{"alias":["src"]})"));
    auto *mapped = Import("alias/Entry");
    auto *unmapped = Import("./other/Helper");
    ASSERT_NE(mapped, nullptr);
    ASSERT_NE(unmapped, nullptr);
    EXPECT_EQ(mapped->ModuleName(), "app.src.Entry");
    EXPECT_EQ(unmapped->ModuleName(), "app.other.Helper");
}

TEST_F(ImportPathManagerTest, ExplicitEtsPathTakesPrecedenceOverConfiguredPackageAndAlias)
{
    Write("src/Entry.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("package":"app","paths":{"alias":["src"]})", {"--ets-path=" + root_.string()}));
    auto *entry = Import("alias/Entry");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->ModuleName(), "src.Entry");
}

TEST_F(ImportPathManagerTest, StandardLibraryBuildUsesItsSyntheticMain)
{
    ASSERT_NO_FATAL_FAILURE(Start("", {"--gen-stdlib=true"}));
    EXPECT_EQ(Main()->GetImportInfo().ResolvedSource(), "etsstdlib.ats");
    EXPECT_TRUE(Main()->Is<ModuleKind::MODULE>());
    EXPECT_EQ(Manager()->SearchResolvedExact(Main()->GetImportInfo()), Main());
    ASSERT_EQ(Manager()->GetParseQueue().size(), 1U);
    EXPECT_EQ(Manager()->GetParseQueue().front().program, Main());
}

TEST_F(ImportPathManagerTest, RawAndResolvedMockKeysAndImportsFromTheMock)
{
    Write("Real.ats");
    Write("Double.ats");
    const auto real = JsonPath(Path("Real.ats"));
    ASSERT_NO_FATAL_FAILURE(
        Start(R"("mock":{"virtual":{"source":"Double.ats"},)" + real + R"(:{"source":"Double.ats"}})"));
    auto *mock = Import("virtual");
    ASSERT_NE(mock, nullptr);
    EXPECT_EQ(mock->GetImportInfo().ResolvedSource(), Path("Double.ats").string());
    EXPECT_EQ(Import("./Real"), mock);
    auto *original = Import("./Real", mock);
    ASSERT_NE(original, nullptr);
    EXPECT_EQ(original->GetImportInfo().ResolvedSource(), Path("Real.ats").string());
}

TEST_F(ImportPathManagerTest, InvalidSpecifiersReportTheirSpecificDiagnostics)
{
    ASSERT_NO_FATAL_FAILURE(Start());
    EXPECT_EQ(Import(""), nullptr);
    ExpectDiagnostic(diagnostic::EMPTY_IMPORT_PATH);
    Context()->diagnosticEngine->ClearDiagnostics();
    EXPECT_EQ(Import("unknown-package"), nullptr);
    ExpectDiagnostic(diagnostic::IMPORT_CANT_FIND_PREFIX);
    Context()->diagnosticEngine->ClearDiagnostics();
    EXPECT_EQ(Import("./missing"), nullptr);
    ExpectDiagnostic(diagnostic::UNSUPPORTED_PATH);
    Context()->diagnosticEngine->ClearDiagnostics();
}

TEST_F(ImportPathManagerTest, ImportOfMainReturnsExistingProgramAndReportsSelfImport)
{
    ASSERT_NO_FATAL_FAILURE(Start());
    EXPECT_EQ(Import("./Main.ats"), Main());
    ExpectDiagnostic(diagnostic::IMPORT_ITSELF);
    Context()->diagnosticEngine->ClearDiagnostics();
}

TEST_F(ImportPathManagerTest, ClearResolutionCachesSeesCreatedFilesAndChangedPriority)
{
    ASSERT_NO_FATAL_FAILURE(Start(R"("atsDependencies":{"pkg":{"moduleRoot":"src"}})"));
    EXPECT_EQ(Import("pkg/New"), nullptr);
    ExpectDiagnostic(diagnostic::IMPORT_CANT_FIND_PREFIX);
    Context()->diagnosticEngine->ClearDiagnostics();

    Write("src/New.d.ats");
    Manager()->ClearResolutionCaches();
    EXPECT_EQ(Resolve("pkg/New"), Path("src/New.d.ats"));
    Write("src/New.ats");
    Manager()->ClearResolutionCaches();
    EXPECT_EQ(Resolve("pkg/New"), Path("src/New.ats"));
    ASSERT_TRUE(fs::remove(Path("src/New.ats")));
    Manager()->ClearResolutionCaches();
    EXPECT_EQ(Resolve("pkg/New"), Path("src/New.d.ats"));
}

TEST_F(ImportPathManagerTest, ParserCollectsTransitiveDependenciesAndMarksSourcesParsed)
{
    Write("Main.ats", "import { middle } from './Middle'; export function main(): int { return middle(); }");
    Write("Middle.ats", "import { value } from './Leaf'; export function middle(): int { return value; }");
    Write("Leaf.ats");
    ASSERT_NO_FATAL_FAILURE(Configure(""));
    ASSERT_NO_FATAL_FAILURE(OpenFile("Main.ats"));
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    const auto &deps = Manager()->GetFileDependencies();
    const ArenaString middle(Path("Middle.ats").string());
    const ArenaString leaf(Path("Leaf.ats").string());
    EXPECT_EQ(deps.at(ArenaString(Path("Main.ats").string())).count(middle), 1U);
    EXPECT_EQ(deps.at(middle).count(leaf), 1U);
    const auto &queue = Manager()->GetParseQueue();
    EXPECT_TRUE(std::all_of(queue.begin(), queue.end(), [](const auto &item) { return item.isParsed; }));
}

}  // namespace ark::es2panda::util::test
