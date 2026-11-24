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
#include "driver/dependency_analyzer/dep_analyzer.h"

namespace ark::es2panda::util::test {

TEST_F(ImportPathManagerTest, DynamicRelativeImportUsesOriginalSourceAndMapsBackToLogicalKey)
{
    Write("generated/Entry.d.ats", "export declare function entry(): int;");
    Write("generated/Helper.ats");
    Write("original/Helper.ts", "export const fromSource = 1;");
    Write("other/Helper.d.ats", "export declare const fromSource: int;");
    ASSERT_NO_FATAL_FAILURE(Start(R"("dependencies":{
        "unrelated-entry-key":{"language":"js", "ohmUrl":"entry", "path":"generated/Entry.d.ats",
                               "sourceFilePath":"original/Entry.ts"},
        "helper-key":{"language":"js", "ohmUrl":"helper", "path":"other/Helper.d.ats",
            "sourceFilePath":"original/Helper.ts"}
    })"));
    auto *entry = Import("unrelated-entry-key");
    ASSERT_NE(entry, nullptr);
    ASSERT_TRUE(entry->Is<ModuleKind::SOURCE_DECL>());
    auto *helper = Import("./Helper", entry);
    ASSERT_NE(helper, nullptr);
    EXPECT_EQ(helper->GetImportInfo().ResolvedSource(), "helper-key");
    EXPECT_EQ(fs::path(helper->GetImportInfo().TextSource()), Path("other/Helper.d.ats"));
    EXPECT_EQ(helper->GetImportInfo().OhmUrl(), "helper");
}

TEST_F(ImportPathManagerTest, DynamicMissFallsBackToLogicalKeyIncludingIndexlessSourceDirectory)
{
    Write("generated/Entry.d.ats");
    fs::create_directories(Path("original/Helper"));
    ASSERT_NO_FATAL_FAILURE(Start(R"("dependencies":{
        "pkg/entry":{"language":"js", "ohmUrl":"entry", "path":"generated/Entry.d.ats",
            "sourceFilePath":"original/Entry.ts"},
        "pkg/Helper":{"language":"js", "ohmUrl":"helper"}
    })"));
    auto *entry = Import("pkg/entry");
    ASSERT_NE(entry, nullptr);
    auto *helper = Import("./Helper", entry);
    ASSERT_NE(helper, nullptr);
    EXPECT_TRUE(helper->Is<ModuleKind::DECLLESS_DYNAMIC>());
    EXPECT_EQ(helper->GetImportInfo().ResolvedSource(), "pkg/Helper");
    EXPECT_TRUE(helper->SourceCode().empty());
    EXPECT_TRUE(std::none_of(Manager()->GetParseQueue().begin(), Manager()->GetParseQueue().end(),
                             [helper](const auto &item) { return item.program == helper; }));
}

TEST_F(ImportPathManagerTest, DynamicLogicalRelativeKeysCanMapBackToStaticSourceFiles)
{
    Write("Entry.d.ats");
    Write("src/Preferred.ats");
    Write("src/Mapped.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("dependencies":{
        "pkg/entry":{"language":"js", "ohmUrl":"entry", "path":"Entry.d.ats"},
        "pkg/Both":{"language":"js", "ohmUrl":"both"}},
        "paths":{"pkg/Both":["src/Preferred.ats"], "pkg/OnlyStatic":["src/Mapped.ats"]})"));
    auto *entry = Import("pkg/entry");
    ASSERT_NE(entry, nullptr);
    auto *preferred = Import("./Both", entry);
    auto *mapped = Import("./OnlyStatic", entry);
    ASSERT_NE(preferred, nullptr);
    ASSERT_NE(mapped, nullptr);
    EXPECT_TRUE(preferred->Is<ModuleKind::MODULE>());
    EXPECT_TRUE(mapped->Is<ModuleKind::MODULE>());
    EXPECT_EQ(preferred->GetImportInfo().ResolvedSource(), Path("src/Preferred.ats").string());
    EXPECT_EQ(mapped->GetImportInfo().ResolvedSource(), Path("src/Mapped.ats").string());
}

TEST_F(ImportPathManagerTest, StaticAndDynamicImportersDoNotShareAliasResolution)
{
    Write("src/Shared.ats");
    Write("Entry.d.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("atsDependencies":{"pkg":{"moduleRoot":"src"}}, "dependencies":{
        "entry":{"language":"js", "ohmUrl":"entry", "path":"Entry.d.ats"},
        "dynamic-target":{"language":"js", "ohmUrl":"dynamic", "alias":["pkg/Shared"]}})"));
    auto *entry = Import("entry");
    ASSERT_NE(entry, nullptr);
    auto *source = Import("pkg/Shared");
    ASSERT_NE(source, nullptr);
    EXPECT_EQ(source->GetImportInfo().ResolvedSource(), Path("src/Shared.ats").string());
    auto *dynamic = Import("pkg/Shared", entry);
    ASSERT_NE(dynamic, nullptr);
    EXPECT_EQ(dynamic->GetImportInfo().ResolvedSource(), "dynamic-target");
    EXPECT_EQ(Import("pkg/Shared"), source);
}

TEST_F(ImportPathManagerTest, MissingDynamicDeclarationProducesOnlyInteropDiagnostic)
{
    ASSERT_NO_FATAL_FAILURE(
        Start(R"("dependencies":{"missing":{"language":"js","ohmUrl":"m","path":"absent.d.ats"}})"));
    EXPECT_EQ(Import("missing"), nullptr);
    ASSERT_EQ(Context()->diagnosticEngine->GetDiagnosticStorage(DiagnosticType::FATAL).size(), 1U);
    ExpectDiagnostic(diagnostic::INTEROP_DYNAMIC_FILE_NOT_FOUND);
    Context()->diagnosticEngine->ClearDiagnostics();
}

TEST_F(ImportPathManagerTest, RelativeDynamicSourceWithMissingMappedDeclarationStopsAtTheMissingFile)
{
    Write("Entry.d.ats");
    Write("original/Helper.ts");
    ASSERT_NO_FATAL_FAILURE(Start(R"("dependencies":{
        "entry":{"language":"js", "ohmUrl":"entry", "path":"Entry.d.ats", "sourceFilePath":"original/Entry.ts"},
        "helper":{"language":"js", "ohmUrl":"helper", "path":"missing/Helper.d.ats",
            "sourceFilePath":"original/Helper.ts"}})"));
    auto *entry = Import("entry");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(Import("./Helper", entry), nullptr);
    ASSERT_EQ(Context()->diagnosticEngine->GetDiagnosticStorage(DiagnosticType::FATAL).size(), 1U);
    ExpectDiagnostic(diagnostic::INTEROP_DYNAMIC_FILE_NOT_FOUND);
    Context()->diagnosticEngine->ClearDiagnostics();
}

TEST_F(ImportPathManagerTest, DependencyAnalyzerQueuesOriginalDynamicSourceAndUsesLogicalGraphKeys)
{
    DepAnalyzer analyzer;
    Write("original/Entry.ts", "import { value } from './Leaf';");
    Write("original/Leaf.ts");
    ASSERT_NO_FATAL_FAILURE(Start(R"("dependencies":{
        "entry":{"language":"js", "ohmUrl":"entry", "path":"unbuilt.d.ats", "sourceFilePath":"original/Entry.ts"},
        "leaf":{"language":"js", "ohmUrl":"leaf", "sourceFilePath":"original/Leaf.ts"},
        "unavailable":{"language":"js", "ohmUrl":"unavailable", "sourceFilePath":"missing.ts"}})"));
    Context()->depAnalyzer = &analyzer;
    auto *entry = Import("entry");
    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->Is<ModuleKind::DECLLESS_DYNAMIC>());
    EXPECT_EQ(entry->SourceCode(), "import { value } from './Leaf';");
    EXPECT_EQ(Manager()->GetParseQueue().back().program, entry);
    auto *leaf = Import("./Leaf", entry);
    ASSERT_NE(leaf, nullptr);
    EXPECT_EQ(leaf->GetImportInfo().ResolvedSource(), "leaf");
    EXPECT_EQ(Manager()->GetFileDependencies().at(ArenaString("entry")).count(ArenaString("leaf")), 1U);
    const auto queueSize = Manager()->GetParseQueue().size();
    auto *unavailable = Import("unavailable");
    Context()->depAnalyzer = nullptr;
    ASSERT_NE(unavailable, nullptr);
    EXPECT_TRUE(unavailable->Is<ModuleKind::DECLLESS_DYNAMIC>());
    EXPECT_TRUE(unavailable->SourceCode().empty());
    EXPECT_EQ(Manager()->GetParseQueue().size(), queueSize);
}

TEST_F(ImportPathManagerTest, CachedDeclarationPrefersPhysicalSiblingThenConfigMapping)
{
    Write("Entry.ats");
    Write("cache/Entry.etscache", "export declare const cached: int;");
    Write("cache/Sibling.ats");
    Write("alternate/Sibling.ats");
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + R"(,"paths":{"Sibling":["alternate/Sibling.ats"]})"));
    auto *entry = Import("./Entry");
    ASSERT_NE(entry, nullptr);
    ASSERT_TRUE(entry->Is<ModuleKind::ETSCACHE_DECL>());
    EXPECT_EQ(entry->SourceCode(), "export declare const cached: int;");
    auto *physical = Import("./Sibling", entry);
    ASSERT_NE(physical, nullptr);
    EXPECT_EQ(physical->GetImportInfo().ResolvedSource(), Path("cache/Sibling.ats").string());
    ASSERT_TRUE(fs::remove(Path("cache/Sibling.ats")));
    Manager()->RemoveProgramFromResolvedSources(ArenaString(physical->GetImportInfo().Key()));
    Manager()->ClearResolutionCaches();
    auto *mapped = Import("./Sibling", entry);
    ASSERT_NE(mapped, nullptr);
    EXPECT_EQ(mapped->GetImportInfo().ResolvedSource(), Path("alternate/Sibling.ats").string());
}

TEST_F(ImportPathManagerTest, CacheRelativeMissAndAdjacentDirectoryAreRejected)
{
    Write("Entry.ats");
    Write("cache/Entry.etscache");
    Write("alternate/Secret.ats");
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + R"(,"paths":{"Other/Secret":["alternate/Secret.ats"]})"));
    auto *entry = Import("./Entry");
    ASSERT_NE(entry, nullptr);
    ASSERT_TRUE(entry->Is<ModuleKind::ETSCACHE_DECL>());
    for (auto path : {"./Absent", "../cacheOther/Secret", "../outside/Secret"}) {
        SCOPED_TRACE(path);
        EXPECT_EQ(Import(path, entry), nullptr);
        ASSERT_EQ(Context()->diagnosticEngine->GetDiagnosticStorage(DiagnosticType::FATAL).size(), 1U);
        ExpectDiagnostic(diagnostic::UNSUPPORTED_PATH);
        Context()->diagnosticEngine->ClearDiagnostics();
    }
}

TEST_F(ImportPathManagerTest, InternalModulesAreRestrictedOnlyForUserSources)
{
    Write("Public.ats");
    Write("cache/Public.etscache");
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + R"(,"internalPaths":["sdk.hidden"])"));
    auto *cached = Import("./Public");
    ASSERT_NE(cached, nullptr);
    ASSERT_TRUE(cached->Is<ModuleKind::ETSCACHE_DECL>());
    for (auto path : {"sdk.hidden", "sdk.hidden/Class", "sdk.hidden.sub"}) {
        EXPECT_TRUE(Manager()->ShouldRejectInternalImport(Main(), StringView(path)));
        EXPECT_FALSE(Manager()->ShouldRejectInternalImport(cached, StringView(path)));
    }
    EXPECT_FALSE(Manager()->ShouldRejectInternalImport(Main(), "sdk.hiddenly"));
}

TEST_F(ImportPathManagerTest, ParserReportsInternalImportBeforeRecordingAnEdgeAndStillResolvesPublicSibling)
{
    Write("Main.ats", R"(import { hidden } from "sdk/internal";
        import { visible } from "sdk/internalTools";)");
    Write("sdk/internal.ats", "export const hidden: int = 1;");
    Write("sdk/internalTools.ats", "export const visible: int = 2;");
    ASSERT_NO_FATAL_FAILURE(Configure(R"("paths":{"sdk":["sdk"]},"internalPaths":["sdk/internal"])"));
    ASSERT_NO_FATAL_FAILURE(OpenFile("Main.ats"));
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    const auto &errors = Context()->diagnosticEngine->GetDiagnosticStorage(diagnostic::IMPORT_CANT_FIND_PREFIX.Type());
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front()->GetId(), diagnostic::IMPORT_CANT_FIND_PREFIX.Id());
    EXPECT_NE(errors.front()->Message().find("sdk/internal"), std::string::npos);
    Context()->diagnosticEngine->ClearDiagnostics();
    const auto &deps = Manager()->GetFileDependencies().at(ArenaString(Path("Main.ats").string()));
    EXPECT_EQ(deps.count(ArenaString(Path("sdk/internal.ats").string())), 0U);
    EXPECT_EQ(deps.count(ArenaString(Path("sdk/internalTools.ats").string())), 1U);
}

TEST_F(ImportPathManagerTest, ParserFollowsSdkInternalImportsInsideACachedDeclaration)
{
    Write("Main.ats", R"(import { Wrapper } from "./Wrapper";)");
    Write("Wrapper.ats", "export class Wrapper {}");
    Write("cache/Wrapper.etscache", R"(import { Internal } from "sdk/internal";
        export declare class Wrapper { value: Internal; })");
    Write("sdk/internal.ats", "export class Internal {}");
    ASSERT_NO_FATAL_FAILURE(
        Configure(CacheDirectory() + R"(,"paths":{"sdk":["sdk"]},"internalPaths":["sdk/internal"])"));
    ASSERT_NO_FATAL_FAILURE(OpenFile("Main.ats"));
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    EXPECT_FALSE(Context()->diagnosticEngine->IsAnyError());
    ImportInfo wrapperInfo {*Manager(), Path("Wrapper.ats").string()};
    auto *wrapper = Manager()->SearchResolvedExact(wrapperInfo);
    ASSERT_NE(wrapper, nullptr);
    EXPECT_TRUE(wrapper->Is<ModuleKind::ETSCACHE_DECL>());
    const auto &graph = Manager()->GetFileDependencies();
    const auto &deps = graph.at(ArenaString(wrapper->AbsoluteName().Utf8()));
    EXPECT_EQ(deps.count(ArenaString(Path("sdk/internal.ats").string())), 1U);
}

}  // namespace ark::es2panda::util::test
