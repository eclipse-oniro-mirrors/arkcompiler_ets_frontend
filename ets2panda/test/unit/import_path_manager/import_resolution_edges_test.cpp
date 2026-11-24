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
namespace {

class ImportResolutionEdgesTest : public ImportPathManagerTest {};

TEST_F(ImportResolutionEdgesTest, SingleComponentDynamicEntryResolvesARelativeChildByLogicalKey)
{
    ASSERT_NO_FATAL_FAILURE(Start(R"("dependencies":{
        "entry":{"language":"js", "ohmUrl":"entry"},
        "entry/Child":{"language":"js", "ohmUrl":"child"}})"));
    auto *entry = Import("entry");
    ASSERT_NE(entry, nullptr);
    auto *child = Import("./Child", entry);
    ASSERT_NE(child, nullptr);
    EXPECT_TRUE(child->Is<ModuleKind::DECLLESS_DYNAMIC>());
    EXPECT_EQ(child->GetImportInfo().ResolvedSource(), "entry/Child");
    EXPECT_EQ(child->GetImportInfo().OhmUrl(), "child");
    EXPECT_TRUE(child->SourceCode().empty());
    EXPECT_EQ(Import("entry/Child"), child);
    EXPECT_EQ(Manager()->GetParseQueue().size(), 1U);
}

TEST_F(ImportResolutionEdgesTest, DynamicDeclarationWithNoOriginalSiblingUsesTheLogicalParent)
{
    Write("generated/Entry.d.ats");
    Write("original/Entry.ts");
    Write("other/Child.d.ats", "export declare const fromDependency: int;");
    ASSERT_NO_FATAL_FAILURE(Start(R"("dependencies":{
        "lib/entry":{"language":"js", "ohmUrl":"entry", "path":"generated/Entry.d.ats",
            "sourceFilePath":"original/Entry.ts"},
        "lib/Child":{"language":"js", "ohmUrl":"child", "path":"other/Child.d.ats"}})"));
    auto *entry = Import("lib/entry");
    ASSERT_NE(entry, nullptr);
    auto *child = Import("./Child", entry);
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(child->GetImportInfo().ResolvedSource(), "lib/Child");
    EXPECT_EQ(fs::path(child->GetImportInfo().TextSource()), Path("other/Child.d.ats"));
    EXPECT_EQ(child->SourceCode(), "export declare const fromDependency: int;");
    EXPECT_EQ(Manager()->GetParseQueue().back().program, child);
}

TEST_F(ImportResolutionEdgesTest, AnalyzerQueuesStaticImportsAndKeepsSourceLessJsDependenciesInTheGraph)
{
    DepAnalyzer analyzer;
    Write("Local.ats", "export const local: int = 1;");
    ASSERT_NO_FATAL_FAILURE(Start(R"("dependencies":{
        "entry":{"language":"js", "ohmUrl":"entry"},
        "entry/Leaf":{"language":"js", "ohmUrl":"leaf"}})"));
    Context()->depAnalyzer = &analyzer;
    auto *local = Import("./Local");
    auto *entry = Import("entry");
    auto *leaf = entry == nullptr ? nullptr : Import("./Leaf", entry);
    Context()->depAnalyzer = nullptr;
    ASSERT_NE(local, nullptr);
    ASSERT_NE(entry, nullptr);
    ASSERT_NE(leaf, nullptr);
    EXPECT_TRUE(local->Is<ModuleKind::MODULE>());
    EXPECT_EQ(local->SourceCode(), "export const local: int = 1;");
    EXPECT_TRUE(entry->SourceCode().empty());
    EXPECT_TRUE(leaf->SourceCode().empty());
    const auto &queue = Manager()->GetParseQueue();
    ASSERT_EQ(queue.size(), 2U);
    EXPECT_EQ(queue.back().program, local);
    const auto &deps = Manager()->GetFileDependencies();
    const auto found = deps.find(ArenaString("entry"));
    ASSERT_NE(found, deps.end());
    EXPECT_EQ(found->second.count(ArenaString("entry/Leaf")), 1U);
}

TEST_F(ImportResolutionEdgesTest, CacheRelativeMissingDeclarationReportsItsOwnErrorAndCanBeGeneratedLater)
{
    Write("Entry.ats");
    Write("cache/Entry.etscache");
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + R"(, "dependencies":{
        "JsApi":{"language":"js", "ohmUrl":"api", "path":"declarations/Api.d.ats"}})"));
    auto *entry = Import("./Entry");
    ASSERT_NE(entry, nullptr);
    ASSERT_TRUE(entry->Is<ModuleKind::ETSCACHE_DECL>());
    const auto queueSize = Manager()->GetParseQueue().size();
    EXPECT_EQ(Import("./JsApi", entry), nullptr);
    const auto &errors = Context()->diagnosticEngine->GetDiagnosticStorage(DiagnosticType::FATAL);
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front()->GetId(), diagnostic::INTEROP_DYNAMIC_FILE_NOT_FOUND.Id());
    EXPECT_NE(errors.front()->Message().find("JsApi"), std::string::npos);
    EXPECT_EQ(Manager()->GetParseQueue().size(), queueSize);
    EXPECT_TRUE(Manager()->CollectReverseDependencies("JsApi").empty());
    Context()->diagnosticEngine->ClearDiagnostics();

    Write("declarations/Api.d.ats", "export declare const generated: int;");
    Manager()->ClearResolutionCaches();
    auto *api = Import("./JsApi", entry);
    ASSERT_NE(api, nullptr);
    EXPECT_EQ(api->GetImportInfo().ResolvedSource(), "JsApi");
    EXPECT_EQ(api->SourceCode(), "export declare const generated: int;");
    EXPECT_EQ(Manager()->GetParseQueue().size(), queueSize + 1);
}

TEST_F(ImportResolutionEdgesTest, RemovedCacheRootCannotBecomeAnEmptyRelativeModuleKey)
{
    Write("Entry.ats");
    Write("cache/Entry.etscache");
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory()));
    auto *entry = Import("./Entry");
    ASSERT_NE(entry, nullptr);
    ASSERT_TRUE(entry->Is<ModuleKind::ETSCACHE_DECL>());
    fs::rename(Path("cache"), Path("previous-cache"));
    Manager()->ClearResolutionCaches();
    EXPECT_EQ(Import("./.", entry), nullptr);
    ExpectDiagnostic(diagnostic::UNSUPPORTED_PATH);
    Context()->diagnosticEngine->ClearDiagnostics();

    Write("cache/Restored.ats");
    Manager()->ClearResolutionCaches();
    auto *restored = Import("./Restored", entry);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->GetImportInfo().ResolvedSource(), Path("cache/Restored.ats").string());
}

TEST_F(ImportResolutionEdgesTest, StandardLibraryModuleCanImportItselfThroughAnotherSpecifier)
{
    Write("Library.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("paths":{"std.Local":["Library.ats"]})"));
    auto *library = Import("std.Local");
    ASSERT_NE(library, nullptr);
    ASSERT_TRUE(library->IsStdLib());
    const auto queueSize = Manager()->GetParseQueue().size();
    EXPECT_EQ(Import("./Library.ats", library), library);
    EXPECT_EQ(Manager()->GetParseQueue().size(), queueSize);
    EXPECT_FALSE(Context()->diagnosticEngine->IsAnyError());
}

TEST_F(ImportResolutionEdgesTest, MappedSourceOutsideBaseUrlKeepsItsSdkNameAlongsideLocalPackageSources)
{
    Write("app/Main.ats");
    Write("app/src/View.ats");
    Write("vendor/sdk/View.ats");
    ASSERT_NO_FATAL_FAILURE(Configure(R"("package":"product","paths":{"sdk":["../vendor/sdk"]})", {}, "app"));
    ASSERT_NO_FATAL_FAILURE(OpenFile("app/Main.ats"));
    Manager()->SetupGlobalProgram();
    auto *local = Import("./src/View");
    auto *vendor = Import("sdk/View");
    ASSERT_NE(local, nullptr);
    ASSERT_NE(vendor, nullptr);
    EXPECT_EQ(local->ModuleName(), "product.src.View");
    EXPECT_EQ(vendor->ModuleName(), "sdk.View");
    EXPECT_EQ(vendor->GetImportInfo().ResolvedSource(), Path("vendor/sdk/View.ats").string());
    EXPECT_EQ(Import("../vendor/sdk/View"), vendor);
    EXPECT_EQ(Manager()->SearchResolved(local->GetImportInfo()), local);
}

TEST_F(ImportResolutionEdgesTest, NewEditorBufferUsesConfiguredImportsBeforeItsEntryFileExists)
{
    Write("buffers/Sibling.ats", "export const sibling: int = 1;");
    ASSERT_NO_FATAL_FAILURE(Configure(R"("paths":{"editor":["buffers"]})"));
    const auto virtualPath = Path("buffers/New.ats");
    const auto name = virtualPath.string();
    const auto source = R"(import { sibling } from "editor/Sibling";)";
    ASSERT_FALSE(fs::exists(virtualPath));
    handle_ = impl_->CreateContextFromString(config_, source, name.c_str());
    ASSERT_NE(handle_, nullptr);
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    ASSERT_NE(Main(), nullptr);
    EXPECT_EQ(Main()->GetImportInfo().ResolvedSource(), name);
    EXPECT_EQ(Main()->SourceCode(), source);
    ImportInfo sibling {*Manager(), Path("buffers/Sibling.ats").string()};
    auto *resolved = Manager()->SearchResolvedExact(sibling);
    ASSERT_NE(resolved, nullptr);
    EXPECT_EQ(resolved->SourceCode(), "export const sibling: int = 1;");
    EXPECT_EQ(Manager()->SearchResolvedExact(Main()->GetImportInfo()), Main());
    const auto &queue = Manager()->GetParseQueue();
    EXPECT_TRUE(std::any_of(queue.begin(), queue.end(),
                            [resolved](const auto &item) { return item.program == resolved && item.isParsed; }));
    EXPECT_FALSE(fs::exists(virtualPath));
}

TEST_F(ImportResolutionEdgesTest, EditedBufferUsesItsInMemoryContentsAndItsSavedFilesDirectory)
{
    Write("buffers/Existing.ats", "import { obsolete } from './Removed';");
    Write("buffers/Sibling.ats", "export const sibling: int = 7;");
    ASSERT_NO_FATAL_FAILURE(Configure(""));
    const auto filename = Path("buffers/Existing.ats").string();
    const auto source = R"(import { sibling } from "./Sibling";)";
    handle_ = impl_->CreateContextFromString(config_, source, filename.c_str());
    ASSERT_NE(handle_, nullptr);
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    ASSERT_FALSE(Context()->diagnosticEngine->IsAnyError());
    EXPECT_EQ(Main()->SourceCode(), source);
    const auto &deps = Manager()->GetFileDependencies();
    const auto found = deps.find(ArenaString(filename));
    ASSERT_NE(found, deps.end());
    EXPECT_EQ(found->second.count(ArenaString(Path("buffers/Sibling.ats").string())), 1U);
    ImportInfo key {*Manager(), Path("buffers/Sibling.ats").string()};
    ASSERT_NE(Manager()->SearchResolvedExact(key), nullptr);
    EXPECT_EQ(Manager()->SearchResolvedExact(key)->SourceCode(), "export const sibling: int = 7;");
}

struct AliasIdentity {
    const char *name;
    const char *alias;
    const char *target;
    const char *source;
    const char *expected;
};

class ImportAliasIdentityTest : public ImportPathManagerTest, public testing::WithParamInterface<AliasIdentity> {};

TEST_P(ImportAliasIdentityTest, RuntimeNameMatchesAcrossPhysicalAndIncrementalLookup)
{
    const auto &sample = GetParam();
    Write(sample.source);
    const auto alias = fs::path(sample.alias).make_preferred();
    const auto fields = std::string(R"("paths":{)") + JsonPath(alias) + ":[" + JsonPath(Path(sample.target)) + "]}";
    ASSERT_NO_FATAL_FAILURE(Start(fields));
    auto *program = Import("./" + std::string(sample.source));
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->ModuleName(), sample.expected);
    EXPECT_EQ(program->GetImportInfo().ResolvedSource(), Path(sample.source).string());
    EXPECT_EQ(Manager()->FindOrIntroduceProgramForIncremental(Path(sample.source).string()), program);
}

const AliasIdentity ALIASES[] = {
    {"LeadingSlash", "/sdk", "src", "src/View.ats", "sdk.View"},
    {"TrailingSlash", "sdk/", "src", "src/View.ats", "sdk.View"},
    {"ExactScopeWithoutSubpath", "@sdk", "src/View.ats", "src/View.ats", "@sdk"},
    {"ShortSubpath", "api", "src", "src/x", "api.x"},
    {"ScopedDeclarationAlias", "@sdk/public", "src/Api.d.ets", "src/Api.d.ets", "@sdk/public"},
    {"DirectoryWithTrailingSeparator", "api", "src/", "src/Model.ats", "api.Model"}};

INSTANTIATE_TEST_SUITE_P(AliasBoundaries, ImportAliasIdentityTest, testing::ValuesIn(ALIASES),
                         [](const auto &info) { return info.param.name; });

}  // namespace
}  // namespace ark::es2panda::util::test
