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

#include <future>
#include <iterator>

#include "import_archive_test.h"
#include "libarkfile/file.h"

namespace ark::es2panda::util::test {

class ImportPathManagerBytecodeTest : public ImportArchiveTest {
protected:
    void BuildLibrary(bool emitMetadata = true)
    {
        const std::string declaration =
            "'use static'\nexport declare class PublicClass { static value(argument?: string): int; }";
        panda_file::MetadataByPackages metadata;
        if (emitMetadata) {
            metadata["har.Library"].emplace("PublicClass", EmptyDeclarations());
        }
        ASSERT_NO_FATAL_FAILURE(
            BuildArchive({{"Lhar/Library/ETSGLOBAL;", declaration}}, std::move(metadata), "Library.abc"));
    }

    static constexpr auto libraryConfig = R"("dependencies":{"har":{"path":"Library.abc","mainFile":"Library"}},
                                               "internalPaths":["internal.sdk"])";
};

TEST_F(ImportPathManagerBytecodeTest, MetadataDependencyNeedsNoSourceFileOrParseQueueItem)
{
    ASSERT_NO_FATAL_FAILURE(BuildLibrary());
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + "," + libraryConfig, {"--read-metadata=true"}));
    auto *program = Import("har");
    ASSERT_NE(program, nullptr);
    EXPECT_TRUE(program->Is<ModuleKind::METADATA_DECL>());
    EXPECT_TRUE(program->GetImportInfo().ReferencesABC());
    EXPECT_EQ(fs::path(program->GetImportInfo().AbcPath()), Path("Library.abc"));
    EXPECT_FALSE(Manager()->ShouldRejectInternalImport(program, "internal.sdk"));
    EXPECT_EQ(Import("har"), program);
    ASSERT_EQ(Manager()->GetParseQueue().size(), 1U);
    EXPECT_EQ(Manager()->GetParseQueue().front().program, Main());
    // A second module key in the same ABC misses metadata and triggers text extraction.
    ASSERT_FALSE(fs::exists(Path("cache/har/Library.etscache")));
    EXPECT_EQ(Import("har/Absent"), nullptr);
    ExpectDiagnostic(diagnostic::OPEN_FAILED);
    Context()->diagnosticEngine->ClearDiagnostics();
    EXPECT_TRUE(fs::is_regular_file(Path("cache/har/Library.etscache")));
}

TEST_F(ImportPathManagerBytecodeTest, TextDeclarationIsExtractedFromAbcWhenMetadataReadingIsOff)
{
    ASSERT_NO_FATAL_FAILURE(BuildLibrary());
    ASSERT_FALSE(fs::exists(Path("cache/har/Library.etscache")));
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + "," + libraryConfig, {"--read-metadata=false"}));
    auto *program = Import("har");
    ASSERT_NE(program, nullptr);
    EXPECT_TRUE(program->Is<ModuleKind::ETSCACHE_DECL>());
    EXPECT_EQ(fs::path(program->GetImportInfo().TextSource()), Path("cache/har/Library.etscache"));
    EXPECT_TRUE(fs::is_regular_file(Path("cache/har/Library.etscache")));
    EXPECT_EQ(program->SourceCode().find("'use static'\n"), 0U);
    EXPECT_NE(program->SourceCode().find("argument?:"), std::string_view::npos) << program->SourceCode();
    EXPECT_NE(program->SourceCode().find("PublicClass"), std::string_view::npos);
    EXPECT_EQ(Manager()->GetParseQueue().back().program, program);
    EXPECT_EQ(Import("har"), program);
}

TEST_F(ImportPathManagerBytecodeTest, LibraryWithoutMetadataFallsBackToEmbeddedDeclaration)
{
    ASSERT_NO_FATAL_FAILURE(BuildLibrary(false));
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + "," + libraryConfig, {"--read-metadata=true"}));
    auto *program = Import("har");
    ASSERT_NE(program, nullptr);
    EXPECT_TRUE(program->Is<ModuleKind::ETSCACHE_DECL>());
    EXPECT_EQ(fs::path(program->GetImportInfo().TextSource()), Path("cache/har/Library.etscache"));
    EXPECT_NE(program->SourceCode().find("PublicClass"), std::string_view::npos);
}

TEST_F(ImportPathManagerBytecodeTest, ConcurrentExtractionReplacesStaleTextAndPreservesANewerDeclaration)
{
    ASSERT_NO_FATAL_FAILURE(BuildLibrary());
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory()));
    const auto abc = Path("Library.abc");
    auto firstReader = panda_file::OpenPandaFile(abc.string());
    auto secondReader = panda_file::OpenPandaFile(abc.string());
    ASSERT_NE(firstReader, nullptr);
    ASSERT_NE(secondReader, nullptr);
    const auto cacheDir = Path("cache").string();
    const auto decl = Path("cache/har/Library.etscache");
    auto readDecl = [&decl]() {
        std::ifstream input(decl, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input), {});
    };
    ImportPathManager::ExtractEtscacheToFile(*firstReader, abc.string(), cacheDir);
    const auto original = readDecl();
    ASSERT_NE(original.find("PublicClass"), std::string::npos);

    Write("cache/har/Library.etscache", original + "stale trailing bytes");
    const auto abcTime = fs::last_write_time(abc);
    fs::last_write_time(decl, abcTime - std::chrono::hours(1));
    std::promise<void> start;
    const auto ready = start.get_future().share();
    auto extract = [&ready, &abc, &cacheDir](const panda_file::File &file) {
        ready.wait();
        ImportPathManager::ExtractEtscacheToFile(file, abc.string(), cacheDir);
    };
    auto writer = std::async(std::launch::async, extract, std::cref(*firstReader));
    start.set_value();
    extract(*secondReader);
    writer.get();
    EXPECT_EQ(readDecl(), original);

    Write("cache/har/Library.etscache", "newer declaration");
    fs::last_write_time(decl, abcTime + std::chrono::hours(1));
    ImportPathManager::ExtractEtscacheToFile(*firstReader, abc.string(), cacheDir);
    EXPECT_EQ(readDecl(), "newer declaration");
}

TEST_F(ImportPathManagerBytecodeTest, TwoModulesShareAnArchiveAndRetainSeparateMetadataAndSourceIdentity)
{
    const auto abc = Path("Bundle.abc").string();
    ASSERT_NO_FATAL_FAILURE(
        BuildArchive({{"Lhar/Entry/ETSGLOBAL;", "export declare class Entry {}"},
                      {"Lhar/Part/ETSGLOBAL;", "export declare class Part {}"}},
                     {{"har.Entry", {{"Entry", EmptyDeclarations()}}}, {"har.Part", {{"Part", EmptyDeclarations()}}}},
                     "Bundle.abc"));

    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + R"(, "dependencies":{
        "har":{"path":"Bundle.abc", "mainFile":"Entry", "sourceFilePath":"src/original.ats"}})",
                                  {"--read-metadata=true"}));
    auto *mainModule = Import("har");
    auto *submodule = Import("har/Part");
    ASSERT_NE(mainModule, nullptr);
    ASSERT_NE(submodule, nullptr);
    ASSERT_NE(mainModule, submodule);
    for (auto *program : {mainModule, submodule}) {
        EXPECT_TRUE(program->Is<ModuleKind::METADATA_DECL>());
        const auto &info = program->GetImportInfo();
        EXPECT_EQ(info.AbcPath(), abc);
        EXPECT_EQ(info.OhmUrl(), abc);
        EXPECT_EQ(info.TextSource(), Path("src/original.ats").string());
        EXPECT_EQ(Manager()->SearchResolvedExact(info), program);
    }
    EXPECT_EQ(mainModule->ModuleName(), "har.Entry");
    EXPECT_EQ(submodule->ModuleName(), "har.Part");
    EXPECT_EQ(Import("har/Part"), submodule);
    EXPECT_EQ(Manager()->GetParseQueue().size(), 1U);
    EXPECT_FALSE(fs::exists(Path("cache/har/Part.etscache")));
}

TEST_F(ImportPathManagerBytecodeTest, VanishedArchiveReportsItsPathAndRetainsAnExistingTextDeclaration)
{
    ASSERT_NO_FATAL_FAILURE(BuildLibrary());
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + "," + libraryConfig, {"--read-metadata=true"}));
    EXPECT_EQ(Resolve("har"), fs::path("har.Library"));
    ASSERT_TRUE(fs::remove(Path("Library.abc")));
    Write("cache/har/Library.etscache", "export declare const retained: int;");
    auto *program = Import("har");
    ASSERT_NE(program, nullptr);
    EXPECT_TRUE(program->Is<ModuleKind::ETSCACHE_DECL>());
    EXPECT_EQ(program->SourceCode(), "export declare const retained: int;");
    EXPECT_EQ(program->GetImportInfo().OhmUrl(), Path("Library.abc").string());
    const auto &errors = Context()->diagnosticEngine->GetDiagnosticStorage(diagnostic::OPEN_FAILED.Type());
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors[0]->GetId(), diagnostic::OPEN_FAILED.Id());
    EXPECT_NE(errors[0]->Message().find("Library.abc"), std::string::npos);
    Context()->diagnosticEngine->ClearDiagnostics();
}

TEST_F(ImportPathManagerBytecodeTest, SecondTextLookupUsesTheDeclarationAlreadyExtractedFromTheArchive)
{
    ASSERT_NO_FATAL_FAILURE(BuildLibrary(false));
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + "," + libraryConfig, {"--read-metadata=false"}));
    auto *entry = Import("har");
    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->Is<ModuleKind::ETSCACHE_DECL>());
    Write("cache/har/Extra.etscache", "export declare const extra: int;");
    auto *extra = Import("har/Extra");
    ASSERT_NE(extra, nullptr);
    EXPECT_TRUE(extra->Is<ModuleKind::ETSCACHE_DECL>());
    EXPECT_EQ(extra->SourceCode(), "export declare const extra: int;");
    EXPECT_EQ(extra->ModuleName(), "har.Extra");
    EXPECT_EQ(extra->GetImportInfo().AbcPath(), entry->GetImportInfo().AbcPath());
    EXPECT_EQ(Import("har/Extra"), extra);
    EXPECT_EQ(Manager()->GetParseQueue().size(), 3U);
}

TEST_F(ImportPathManagerBytecodeTest, ArchiveWithoutMainFileUsesMetadataForThePackageItself)
{
    ASSERT_NO_FATAL_FAILURE(BuildLibrary());
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + R"(,"dependencies":{"har.Library":{"path":"Library.abc"}})",
                                  {"--read-metadata=true"}));
    auto *program = Import("har.Library");
    ASSERT_NE(program, nullptr);
    EXPECT_TRUE(program->Is<ModuleKind::METADATA_DECL>());
    EXPECT_EQ(program->GetImportInfo().AbcPath(), Path("Library.abc").string());
    EXPECT_EQ(Manager()->SearchResolvedExact(program->GetImportInfo()), program);
    EXPECT_EQ(Manager()->GetParseQueue().size(), 1U);
    EXPECT_FALSE(fs::exists(Path("cache/har/Library.etscache")));
}

}  // namespace ark::es2panda::util::test
