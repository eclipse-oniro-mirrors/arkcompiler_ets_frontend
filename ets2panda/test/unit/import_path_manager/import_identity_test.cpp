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
#include "compiler/lowering/phase.h"
#include "parser/ASparser.h"
#include "parser/JSparser.h"
#include "parser/TSparser.h"
#include "util/eheap.h"

namespace ark::es2panda::util::test {
namespace {

struct ModuleIdentity {
    const char *name;
    const char *options;
    const char *file;
    const char *specifier;
    const char *module;
    ModuleKind kind;
};

class ImportModuleIdentityTest : public ImportPathManagerTest, public testing::WithParamInterface<ModuleIdentity> {};

TEST_P(ImportModuleIdentityTest, ConfigAndPhysicalSourceAgreeOnProgramIdentityAndOutput)
{
    const auto &sample = GetParam();
    Write(sample.file);
    ASSERT_NO_FATAL_FAILURE(Start(sample.options));
    EXPECT_EQ(Resolve(sample.specifier), Path(sample.file));
    auto *program = Import(sample.specifier);
    ASSERT_NE(program, nullptr);
    const auto &info = program->GetImportInfo();
    EXPECT_EQ(program->ModuleName(), sample.module);
    EXPECT_EQ(info.Kind(), sample.kind);
    EXPECT_EQ(info.ResolvedSource(), Path(sample.file).string());
    EXPECT_EQ(info.Key(), info.ResolvedSource());
    EXPECT_FALSE(info.HasSpecifiedDeclPath());
    EXPECT_FALSE(info.ReferencesABC());
    EXPECT_TRUE(info.OhmUrl().empty());
    EXPECT_EQ(Manager()->SearchResolvedExact(info), program);
    EXPECT_EQ(Import("./" + std::string(sample.file)), program);
    const auto &outputs = Manager()->GetOutputMatching();
    const fs::path expected = fs::absolute("build") / (std::string(sample.module) + ".abc");
    EXPECT_EQ(std::string_view(outputs.at(ArenaString(info.Key()))), expected.string());
}

const ModuleIdentity MODULE_IDENTITIES[] = {
    {"EmptyPackageKeepsDirectory", R"("useEmptyPackage":true)", "app/ui/View.ats", "./app/ui/View", "app.ui.View",
     ModuleKind::MODULE},
    {"DefaultUnmappedSource", "", "app/ui/View.ats", "./app/ui/View", "View", ModuleKind::MODULE},
    {"PackageAndLegacyFile", R"("package":"app")", "src/Old.ets", "./src/Old", "app.src.Old", ModuleKind::MODULE},
    {"ParentPackageEndsInDot", R"("package":"app.","paths":{"alias":["src"]})", "src/Service.ats", "alias/Service",
     "app.src.Service", ModuleKind::MODULE},
    {"KnownCompoundEtsExtension", R"("package":"app")", "types/Model.d.ets", "./types/Model", "app.types.Model",
     ModuleKind::SOURCE_DECL},
    {"KnownCompoundAtsExtension", R"("useEmptyPackage":true)", "types/Model.d.ats", "./types/Model", "types.Model",
     ModuleKind::SOURCE_DECL},
    {"ExtensionlessFile", R"("useEmptyPackage":true)", "src/Plain", "./src/Plain", "src.Plain", ModuleKind::MODULE},
    {"PathAliasRoot", R"("paths":{"api":["src/util"]})", "src/util/Convert.ets", "api/Convert", "api.Convert",
     ModuleKind::MODULE},
    {"ExactPathAlias", R"("paths":{"api.Entry":["src/Entry.ats"]})", "src/Entry.ats", "api.Entry", "api.Entry",
     ModuleKind::MODULE},
    {"ScopedAliasKeepsScopeSeparator", R"("paths":{"@org/api":["vendor/lib"]})", "vendor/lib/Type.ets", "@org/api/Type",
     "@org/api.Type", ModuleKind::MODULE},
    {"ScopedAtsEntry", R"("atsDependencies":{"@org/pkg":{"moduleRoot":"vendor","entryFile":"src/Entry.ats"}})",
     "vendor/src/Entry.ats", "@org/pkg", "@org/pkg.src.Entry", ModuleKind::MODULE},
    {"AtsRootPrecedesPackageAndPaths",
     R"("package":"app", "atsDependencies":{"lib":{"moduleRoot":"src/lib", "searchPath":["api"]}},
         "paths":{"legacy":["src"]})",
     "src/lib/api/Item.ats", "legacy/lib/api/Item", "lib.api.Item", ModuleKind::MODULE},
    {"SelectedDirectoryIndex", R"("atsDependencies":{"lib":{"moduleRoot":"vendor"}})", "vendor/a/index.d.ats", "lib/a",
     "lib.a.index", ModuleKind::SOURCE_DECL},
    {"DotInsideFilename", R"("useEmptyPackage":true)", "src/Foo.generated.ats", "./src/Foo.generated.ats",
     "src.Foo.generated", ModuleKind::MODULE},
};

INSTANTIATE_TEST_SUITE_P(ModuleNames, ImportModuleIdentityTest, testing::ValuesIn(MODULE_IDENTITIES),
                         [](const auto &info) { return info.param.name; });

TEST_F(ImportPathManagerTest, DistinctIndexFilesMayShareRuntimeNameAndKeepSeparateExactKeys)
{
    Write("one/index.ats");
    Write("two/index.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("paths":{"lib":["one","two"]})"));
    auto *one = Import("./one/index.ats");
    auto *two = Import("./two/index.ats");
    ASSERT_NE(one, nullptr);
    ASSERT_NE(two, nullptr);
    EXPECT_NE(one, two);
    EXPECT_EQ(one->ModuleName(), "lib.index");
    EXPECT_EQ(two->ModuleName(), one->ModuleName());
    EXPECT_EQ(Manager()->SearchResolvedExact(one->GetImportInfo()), one);
    EXPECT_EQ(Manager()->SearchResolvedExact(two->GetImportInfo()), two);
    EXPECT_FALSE(Context()->diagnosticEngine->IsAnyError());
}

TEST_F(ImportPathManagerTest, CacheDirectoryDeterminesIdentityBeforeAtsPackageOrPathAlias)
{
    Write("cache/lib/Feature.ats");
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory() + R"(, "package":"app", "paths":{"alias":["cache/lib"]},
        "atsDependencies":{"other":{"moduleRoot":"cache", "searchPath":["lib"]}})"));
    auto *program = Import("other/Feature");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->ModuleName(), "lib.Feature");
    EXPECT_EQ(Import("alias/Feature"), program);
    EXPECT_EQ(Import("./cache/lib/Feature"), program);
    EXPECT_EQ(fs::path(Manager()->FormAbcFilePath(program->GetImportInfo())), Path("cache/lib.Feature.abc"));
}

TEST_F(ImportPathManagerTest, MainProgramCanBelongToAnImportedDirectoryWithoutBecomingItsFraction)
{
    Write("Helper.ets");
    ASSERT_NO_FATAL_FAILURE(Start());
    const auto mainName = std::string(Main()->ModuleName());
    auto *dir = Import("./");
    ASSERT_NE(dir, nullptr);
    ASSERT_TRUE(dir->Is<ModuleKind::PACKAGE>());
    const auto &fractions = dir->As<ModuleKind::PACKAGE>()->GetUnmergedPackagePrograms();
    ASSERT_EQ(fractions.size(), 1U);
    EXPECT_EQ(fractions.front()->GetImportInfo().ResolvedSource(), Path("Helper.ets").string());
    EXPECT_EQ(Main()->ModuleName(), mainName);
    EXPECT_EQ(Manager()->SearchResolvedExact(Main()->GetImportInfo()), Main());
}

TEST_F(ImportPathManagerTest, NamesWithColonReportTheLogicalModuleEvenWhenTheFileNameIsValid)
{
    Write("Entry.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("paths":{"invalid:name":["Entry.ats"]})"));
    Import("invalid:name");
    const auto &errors = Context()->diagnosticEngine->GetDiagnosticStorage(diagnostic::UNSUPPORTED_FILE_NAME.Type());
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front()->GetId(), diagnostic::UNSUPPORTED_FILE_NAME.Id());
    EXPECT_NE(errors.front()->Message().find("invalid:name"), std::string::npos);
    Context()->diagnosticEngine->ClearDiagnostics();
}

TEST_F(ImportPathManagerTest, UnnamedEntryKeepsItsPhysicalSourceAndOutputMapping)
{
    ASSERT_NO_FATAL_FAILURE(Start("", {"--ets-unnamed=true"}));
    EXPECT_TRUE(Main()->ModuleName().empty());
    const auto &info = Main()->GetImportInfo();
    EXPECT_EQ(info.ResolvedSource(), Path("Main.ats").string());
    EXPECT_EQ(Manager()->SearchResolvedExact(info), Main());
    EXPECT_EQ(std::string_view(Manager()->GetOutputMatching().at(ArenaString(info.Key()))),
              (fs::absolute("build") / "Main.abc").string());
}

class ImportCompilerEntryTest : public ModuleProjectTest {
protected:
    void SetUp() override
    {
        ScopedAllocatorsManager::Initialize();
        ASSERT_NO_FATAL_FAILURE(ModuleProjectTest::SetUp());
    }

    void TearDown() override
    {
        parser::ImportCache<parser::CacheType::SOURCES>::ClearAll();
        parser::ImportCache<parser::CacheType::METADATA>::ClearAll();
        ScopedAllocatorsManager::Finalize();
        ModuleProjectTest::TearDown();
    }

    template <class Parser>
    void ParseEntry(std::string_view extension, std::string_view source, bool unnamedBuffer = false)
    {
        const auto name = "src/Entry." + std::string(extension);
        Write(name, source);
        const auto inputName = Path(name).string();
        const auto filename = unnamedBuffer ? Path("buffers/Untitled." + std::string(extension)).string() : inputName;
        const auto output = Path("products/compiled.abc");
        const auto outputArg = "--output=" + output.string();
        const std::array args = {ES2PANDA_BIN_PATH, "--thread=0", outputArg.c_str(),
                                 unnamedBuffer ? "--ets-unnamed=true" : "--ets-unnamed=false", inputName.c_str()};
        DiagnosticEngine diagnostics;
        Options options(ES2PANDA_BIN_PATH, diagnostics);
        ASSERT_TRUE(options.Parse({args.data(), args.size()}));

        EHeap::Scope heap;
        auto arena = EHeap::CreateAllocator();
        parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
        SourceFile input(filename, source, options.IsModule());
        public_lib::ConfigImpl config {};
        config.options = &options;
        public_lib::Context context;
        context.config = &config;
        context.allocator = &arena;
        context.input = source;
        context.sourceFile = &input;
        context.diagnosticEngine = &diagnostics;
        compiler::PhaseManager phases(&context, options.GetExtension(), &arena);
        context.phaseManager = &phases;
        Parser parser(&context);
        context.parser = &parser;
        parser.ParseGlobal();

        ASSERT_FALSE(diagnostics.IsAnyError());
        auto *program = context.parserProgram;
        ASSERT_NE(program, nullptr);
        ASSERT_NE(program->Ast(), nullptr);
        EXPECT_EQ(program->SourceCode(), source);
        EXPECT_EQ(program->Extension(), options.GetExtension());
        EXPECT_EQ(program->ModuleName(), unnamedBuffer ? "" : filename);
        EXPECT_EQ(program->GetImportInfo().ModuleName(), filename);
        const auto &info = program->GetImportInfo();
        EXPECT_EQ(info.Lang(), ToLanguage(options.GetExtension()).GetId());
        auto *manager = parser.GetImportPathManager();
        ASSERT_EQ(manager->GetParseQueue().size(), 1U);
        EXPECT_EQ(manager->GetParseQueue().front().program, program);
        EXPECT_EQ(fs::path(manager->FormAbcFilePath(info)), output);
    }
};

TEST_F(ImportCompilerEntryTest, TypeScriptGlobalProgramUsesItsSourcePathAndBinder)
{
    ParseEntry<parser::TSParser>("ts", "let value: number = 1;");
}

TEST_F(ImportCompilerEntryTest, AssemblyScriptGlobalProgramUsesItsSourcePathAndBinder)
{
    ParseEntry<parser::ASParser>("as", "let value = 2;");
}

TEST_F(ImportCompilerEntryTest, TypeScriptCanParseAnUnnamedInMemoryBuffer)
{
    ParseEntry<parser::TSParser>("ts", "let value: number = 2;", true);
}

TEST_F(ImportCompilerEntryTest, AssemblyScriptCanParseAnUnnamedInMemoryBuffer)
{
    ParseEntry<parser::ASParser>("as", "let value = 3;", true);
}

#ifdef PANDA_WITH_ECMASCRIPT
TEST_F(ImportCompilerEntryTest, JavaScriptGlobalProgramUsesItsSourcePathAndBinder)
{
    ParseEntry<parser::JSParser>("js", "let value = 3;");
}
#endif

}  // namespace
}  // namespace ark::es2panda::util::test
