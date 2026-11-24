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

#include <fstream>
#include <iterator>

#include "import_archive_test.h"
#include "driver/dependency_analyzer/dep_analyzer.h"

namespace ark::es2panda::util::test {
namespace {

class ImportArchiveReaderTest : public ImportArchiveTest {
protected:
    void ExtractArchive()
    {
        auto archive = panda_file::OpenPandaFile(Path("bundle.abc").string());
        ASSERT_NE(archive, nullptr);
        ImportPathManager::ExtractEtscacheToFile(*archive, Path("bundle.abc").string(), Path("cache").string());
    }

    void ExpectContents(std::string_view file, std::string_view expected)
    {
        std::ifstream stream(Path(file), std::ios::binary);
        ASSERT_TRUE(stream) << file;
        std::string text {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
        EXPECT_EQ(text, expected);
    }

    static constexpr auto dependency = R"("dependencies":{"bundle":{"path":"bundle.abc","mainFile":"Entry"}})";
};

TEST_F(ImportArchiveReaderTest, OnlyGlobalRecordsExportDeclarationsAndEmptyOnesDoNotOverwriteText)
{
    ASSERT_NO_FATAL_FAILURE(BuildArchive({{"Lbundle/First/ETSGLOBAL;", "export declare class First {}"},
                                          {"Lbundle/Empty/ETSGLOBAL;", ""},
                                          {"Lbundle/Missing/ETSGLOBAL;", std::nullopt},
                                          {"Lbundle/Ordinary;", "ignored"},
                                          {"Lbundle/Second/ETSGLOBAL;", "export declare class Second {}"}}));
    Write("cache/bundle/Empty.etscache", "last known interface");
    fs::last_write_time(Path("cache/bundle/Empty.etscache"),
                        fs::last_write_time(Path("bundle.abc")) - std::chrono::hours(1));
    ASSERT_NO_FATAL_FAILURE(ExtractArchive());
    ExpectContents("cache/bundle/First.etscache", "export declare class First {}");
    ExpectContents("cache/bundle/Second.etscache", "export declare class Second {}");
    ExpectContents("cache/bundle/Empty.etscache", "last known interface");
    ExpectContents("cache/bundle/Missing.etscache", "");
    EXPECT_FALSE(fs::exists(Path("cache/bundle/Ordinary.etscache")));
}

TEST_F(ImportArchiveReaderTest, ExtractedAnnotationPreservesOptionalMarkersAndEscapedControlCharacters)
{
    const std::string chars = "'use static'\n// columns\tend\r\nexport declare function read(arg?: string): void;";
    ASSERT_NO_FATAL_FAILURE(
        BuildArchive({{"Lbundle/Api/ETSGLOBAL;", chars}, {"Lbundle/Path/ETSGLOBAL;", "// suffix\\"}}));
    ASSERT_NO_FATAL_FAILURE(ExtractArchive());
    ExpectContents("cache/bundle/Api.etscache", R"('use static'
// columns\tend\r
export declare function read(arg?: string): void;)");
    ExpectContents("cache/bundle/Path.etscache", R"(// suffix\\)");
}

TEST_F(ImportArchiveReaderTest, AnUnwritableCacheEntryDoesNotPreventOtherModulesBeingExtracted)
{
    ASSERT_NO_FATAL_FAILURE(BuildArchive({{"Lbundle/Blocked/ETSGLOBAL;", "export class Fixed {}"},
                                          {"Lbundle/Other/ETSGLOBAL;", "export class Other {}"}}));
    const auto blocked = Path("cache/bundle/Blocked.etscache");
    ASSERT_TRUE(fs::create_directories(blocked));
    fs::last_write_time(blocked, fs::last_write_time(Path("bundle.abc")) - std::chrono::hours(1));
    ASSERT_NO_FATAL_FAILURE(ExtractArchive());
    EXPECT_TRUE(fs::is_directory(blocked));
    ExpectContents("cache/bundle/Other.etscache", "export class Other {}");
    ASSERT_TRUE(fs::remove(blocked));
    ASSERT_NO_FATAL_FAILURE(ExtractArchive());
    ExpectContents("cache/bundle/Blocked.etscache", "export class Fixed {}");
}

TEST_F(ImportArchiveReaderTest, MetadataSelfImportReusesTheModuleWithoutEnqueuingOrDiagnosingIt)
{
    ASSERT_NO_FATAL_FAILURE(BuildArchive({{"Lbundle/Entry/ETSGLOBAL;", "export class Entry {}"}},
                                         {{"bundle.Entry", {{"bundle.Entry", EmptyDeclarations()}}}}));
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + "," + dependency, {"--read-metadata=true"}));
    auto *entry = Import("bundle");
    ASSERT_NE(entry, nullptr);
    ASSERT_TRUE(entry->Is<ModuleKind::METADATA_DECL>());
    auto previousSize = Manager()->GetParseQueue().size();
    EXPECT_EQ(Import("bundle", entry), entry);
    EXPECT_EQ(Manager()->GetParseQueue().size(), previousSize);
    EXPECT_FALSE(Context()->diagnosticEngine->IsAnyError());
    EXPECT_FALSE(fs::exists(Path("cache/bundle/Entry.etscache")));
}

TEST_F(ImportArchiveReaderTest, VerificationSkipsAnEmptyMetadataRecordAlongsideACompleteInterface)
{
    panda_file::MetadataByModules modules {{"a.omitted", {}}, {"b.present", EmptyDeclarations()}};
    EXPECT_TRUE(VerifyMetadataModules(modules));
    ASSERT_NO_FATAL_FAILURE(
        BuildArchive({{"Lbundle/Entry/ETSGLOBAL;", "export class Entry {}"}}, {{"bundle.Entry", std::move(modules)}}));
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + "," + dependency, {"--read-metadata=true"}));
    auto *entry = Import("bundle");
    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->Is<ModuleKind::METADATA_DECL>());
    EXPECT_EQ(entry->GetImportInfo().AbcPath(), Path("bundle.abc").string());
    EXPECT_FALSE(fs::exists(Path("cache/bundle/Entry.etscache")));
}

TEST_F(ImportArchiveReaderTest, PackageWithUnreadableMetadataFallsBackToTextAndItsSiblingStillUsesMetadata)
{
    const std::string fallback = "export declare class FromText {}";
    // An ordinary incomplete metadata section should select the embedded interface.
    panda_file::MetadataByPackages metadata {{"bundle.Entry", {{"first", EmptyDeclarations()}, {"unfinished", {0}}}},
                                             {"bundle.Other", {{"other", EmptyDeclarations()}}}};
    ASSERT_NO_FATAL_FAILURE(BuildArchive(
        {{"Lbundle/Entry/ETSGLOBAL;", fallback}, {"Lbundle/Other/ETSGLOBAL;", "export declare class Other {}"}},
        std::move(metadata)));
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + "," + dependency, {"--read-metadata=true"}));
    auto *entry = Import("bundle");
    ASSERT_NE(entry, nullptr);
    ASSERT_TRUE(entry->Is<ModuleKind::ETSCACHE_DECL>());
    EXPECT_EQ(entry->SourceCode(), fallback);
    auto *other = Import("bundle/Other");
    ASSERT_NE(other, nullptr);
    EXPECT_TRUE(other->Is<ModuleKind::METADATA_DECL>());
    EXPECT_EQ(Manager()->GetParseQueue().size(), 2U);
    EXPECT_EQ(Manager()->GetParseQueue().back().program, entry);
}

TEST_F(ImportArchiveReaderTest, TextOnlyArchiveIsReusedForSecondImportWithMetadataRequested)
{
    const std::string first = "export declare class First {}";
    const std::string second = "export declare class Second {}";
    ASSERT_NO_FATAL_FAILURE(BuildArchive({{"Lbundle/Entry/ETSGLOBAL;", first}, {"Lbundle/Second/ETSGLOBAL;", second}}));
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + "," + dependency, {"--read-metadata=true"}));
    auto *entry = Import("bundle");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->SourceCode(), first);
    ExpectContents("cache/bundle/Second.etscache", second);
    auto *next = Import("bundle/Second");
    ASSERT_NE(next, nullptr);
    EXPECT_TRUE(next->Is<ModuleKind::ETSCACHE_DECL>());
    EXPECT_EQ(next->SourceCode(), second);
    EXPECT_EQ(Import("bundle/Second"), next);
}

TEST_F(ImportArchiveReaderTest, ProcessedArchiveRemovedBeforeTextFallbackUsesLastGeneratedDeclaration)
{
    const std::string extra = "export declare class Retained {}";
    ASSERT_NO_FATAL_FAILURE(BuildArchive({{"Lbundle/Entry/ETSGLOBAL;", std::nullopt}},
                                         {{"bundle.Entry", {{"entry", EmptyDeclarations()}}}}));
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + "," + dependency, {"--read-metadata=true"}));
    auto *entry = Import("bundle");
    ASSERT_NE(entry, nullptr);
    ASSERT_TRUE(entry->Is<ModuleKind::METADATA_DECL>());
    Write("cache/bundle/Extra.etscache", extra);
    ASSERT_TRUE(fs::remove(Path("bundle.abc")));
    auto *retained = Import("bundle/Extra");
    ASSERT_NE(retained, nullptr);
    EXPECT_TRUE(retained->Is<ModuleKind::ETSCACHE_DECL>());
    EXPECT_EQ(retained->SourceCode(), extra);
    const auto &errors = Context()->diagnosticEngine->GetDiagnosticStorage(diagnostic::OPEN_FAILED.Type());
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front()->GetId(), diagnostic::OPEN_FAILED.Id());
    EXPECT_NE(errors.front()->Message().find("bundle.abc"), std::string::npos);
    Context()->diagnosticEngine->ClearDiagnostics();
}

TEST_F(ImportArchiveReaderTest, DependencyWithoutMainFileLooksUpMetadataUnderThePackageName)
{
    ASSERT_NO_FATAL_FAILURE(BuildArchive({{"Lbundle/ETSGLOBAL;", "export declare class Api {}"}},
                                         {{"bundle", {{"api", EmptyDeclarations()}}}}));
    ASSERT_NO_FATAL_FAILURE(
        Start(CacheDirectory() + R"(,"dependencies":{"bundle":{"path":"bundle.abc"}})", {"--read-metadata=true"}));
    auto *program = Import("bundle");
    ASSERT_NE(program, nullptr);
    EXPECT_TRUE(program->Is<ModuleKind::METADATA_DECL>());
    EXPECT_EQ(program->GetImportInfo().ResolvedSource(), "bundle.");
    EXPECT_EQ(program->GetImportInfo().AbcPath(), Path("bundle.abc").string());
    EXPECT_EQ(Manager()->GetParseQueue().size(), 1U);
    EXPECT_FALSE(fs::exists(Path("cache/bundle.etscache")));
}

TEST_F(ImportArchiveReaderTest, StandardLibraryGenerationReadsTextFromAMetadataEnabledArchive)
{
    const std::string declaration = "export declare class LibraryType {}";
    ASSERT_NO_FATAL_FAILURE(
        BuildArchive({{"Lbundle/Entry/ETSGLOBAL;", declaration}}, {{"bundle.Entry", {{"api", EmptyDeclarations()}}}}));
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + "," + dependency, {"--gen-stdlib=true", "--read-metadata=true"}));
    ASSERT_EQ(Main()->GetImportInfo().ResolvedSource(), "etsstdlib.ats");
    auto *program = Import("bundle");
    ASSERT_NE(program, nullptr);
    EXPECT_TRUE(program->Is<ModuleKind::ETSCACHE_DECL>());
    EXPECT_EQ(program->SourceCode(), declaration);
    ExpectContents("cache/bundle/Entry.etscache", declaration);
    EXPECT_EQ(Manager()->GetParseQueue().back().program, program);
}

TEST_F(ImportArchiveReaderTest, DynamicBytecodeWithoutADeclarationCanReferenceAnInternalSdkModule)
{
    ASSERT_NO_FATAL_FAILURE(BuildArchive({{"Lbundle/Implementation;", std::nullopt}}));
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + R"(, "internalPaths":["sdk.hidden"],
        "dependencies":{"bundle":{"language":"js", "ohmUrl":"lib", "path":"bundle.abc", "mainFile":"Entry"}})"));
    auto *program = Import("bundle");
    ASSERT_NE(program, nullptr);
    EXPECT_TRUE(program->Is<ModuleKind::DECLLESS_DYNAMIC>());
    EXPECT_TRUE(program->GetImportInfo().ReferencesABC());
    EXPECT_TRUE(program->SourceCode().empty());
    EXPECT_FALSE(Manager()->ShouldRejectInternalImport(program, "sdk.hidden/Private"));
    EXPECT_TRUE(Manager()->ShouldRejectInternalImport(Main(), "sdk.hidden/Private"));
    EXPECT_EQ(Manager()->GetParseQueue().size(), 1U);
}

TEST_F(ImportArchiveReaderTest, AnalyzerResolvesRelativeImportsOfAnArchiveSubmoduleByItsLogicalParent)
{
    ASSERT_NO_FATAL_FAILURE(BuildArchive({{"Lbundle/Implementation;", std::nullopt}}));
    Write("Main.ats", R"(import { run } from "bundle/feature";)");
    Write("src/Entry.ts", R"(import { peer } from "./Peer";)");
    Write("src/Peer.ts");
    Write("external/Peer.ts");
    ASSERT_NO_FATAL_FAILURE(Configure(CacheDirectory() + R"(, "dependencies":{
        "bundle":{"language":"js", "ohmUrl":"archive", "path":"bundle.abc", "sourceFilePath":"src/Entry.ts"},
        "bundle/Peer":{"language":"js", "ohmUrl":"peer", "sourceFilePath":"external/Peer.ts"},
        "entry-peer":{"language":"js", "ohmUrl":"other", "sourceFilePath":"src/Peer.ts"}})"));
    ASSERT_NO_FATAL_FAILURE(OpenFile("Main.ats"));
    DepAnalyzer analyzer;
    Context()->depAnalyzer = &analyzer;
    Context()->parser->AddParserStatus(parser::ParserStatus::DEPENDENCY_ANALYZER_MODE);
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    Context()->depAnalyzer = nullptr;
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    ASSERT_FALSE(Context()->diagnosticEngine->IsAnyError());
    const auto &graph = Manager()->GetFileDependencies();
    const auto submodule = graph.find(ArenaString("bundle/feature"));
    ASSERT_NE(submodule, graph.end());
    EXPECT_EQ(submodule->second.size(), 1U);
    EXPECT_EQ(submodule->second.count(ArenaString("bundle/Peer")), 1U);
    EXPECT_TRUE(Manager()->CollectReverseDependencies("entry-peer").empty());
    EXPECT_EQ(Manager()->CollectReverseDependencies("bundle/Peer").count(Path("Main.ats").string()), 1U);
}

TEST_F(ImportArchiveReaderTest, QuotedCommentBeforeAnExportStaysOnItsOwnSourceLine)
{
    const std::string declaration = "export declare function launch(): void;";
    ASSERT_NO_FATAL_FAILURE(BuildArchive({{"Lbundle/Api/ETSGLOBAL;", "// it's public\n" + declaration}}));
    ASSERT_NO_FATAL_FAILURE(ExtractArchive());
    std::ifstream stream(Path("cache/bundle/Api.etscache"));
    ASSERT_TRUE(stream);
    std::string comment;
    std::string exportLine;
    ASSERT_TRUE(std::getline(stream, comment));
    ASSERT_TRUE(std::getline(stream, exportLine));
    EXPECT_EQ(comment.find("// it"), 0U);
    EXPECT_NE(comment.find("s public"), std::string::npos);
    EXPECT_EQ(exportLine, declaration);
}

}  // namespace
}  // namespace ark::es2panda::util::test
