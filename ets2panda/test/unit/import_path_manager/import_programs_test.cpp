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

#include <iterator>

#include "import_path_manager_test.h"

namespace ark::es2panda::util::test {

TEST_F(ImportPathManagerTest, RepeatedImportsRegisterOneProgramOneQueueItemAndOneDependency)
{
    Write("Leaf.ats");
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory()));
    auto *leaf = Import("./Leaf");
    ASSERT_NE(leaf, nullptr);
    EXPECT_EQ(Import("./Leaf.ats"), leaf);
    const auto &external = Main()->GetExternalPrograms()->Get<ModuleKind::MODULE>();
    EXPECT_EQ(std::count(external.begin(), external.end(), leaf), 1);
    ASSERT_EQ(Manager()->GetParseQueue().size(), 2U);
    EXPECT_EQ(Manager()->GetParseQueue().front().program, Main());
    EXPECT_EQ(Manager()->GetParseQueue().back().program, leaf);
    EXPECT_FALSE(Manager()->GetParseQueue().back().isParsed);
    const auto &deps = Manager()->GetFileDependencies().at(ArenaString(Main()->AbsoluteName().Utf8()));
    ASSERT_EQ(deps.size(), 1U);
    EXPECT_EQ(*deps.begin(), leaf->GetImportInfo().Key());
    EXPECT_EQ(std::string_view(Manager()->GetOutputMatching().at(ArenaString(leaf->GetImportInfo().Key()))),
              Path("cache/Leaf.abc").string());
}

TEST_F(ImportPathManagerTest, DeclaredProgramRetainsExactIdentityWhenImplementationIsAddedAndRemoved)
{
    Write("types/Item.d.ats");
    Write("src/Item.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("paths":{"item":["src/Item.ats","types/Item.d.ats"]})"));
    auto *decl = Import("./types/Item.d.ats");
    ASSERT_NE(decl, nullptr);
    ASSERT_TRUE(decl->Is<ModuleKind::SOURCE_DECL>());
    auto *implementation = Import("./src/Item.ats");
    ASSERT_NE(implementation, nullptr);
    ASSERT_EQ(decl->ModuleName(), implementation->ModuleName());
    EXPECT_EQ(Manager()->SearchResolvedExact(decl->GetImportInfo()), decl);
    EXPECT_EQ(Manager()->SearchResolved(decl->GetImportInfo()), implementation);
    EXPECT_TRUE(Manager()->IsReplacedExactSource(decl));
    EXPECT_EQ(Import("./types/Item.d.ats"), implementation);

    Manager()->RemoveProgramFromResolvedSources(ArenaString(implementation->GetImportInfo().Key()));
    EXPECT_EQ(Manager()->SearchResolved(implementation->GetImportInfo()), nullptr);
    EXPECT_EQ(Manager()->SearchResolved(decl->GetImportInfo()), decl);
    EXPECT_FALSE(Manager()->IsReplacedExactSource(decl));
}

TEST_F(ImportPathManagerTest, ImplementationFirstAlsoPreservesExactDeclaration)
{
    Write("src/Object.ats");
    Write("types/Object.d.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("paths":{"object":["src/Object.ats","types/Object.d.ats"]})"));
    auto *implementation = Import("./src/Object.ats");
    auto *decl = Import("./types/Object.d.ats");
    ASSERT_NE(implementation, nullptr);
    ASSERT_NE(decl, nullptr);
    ASSERT_NE(implementation, decl);
    EXPECT_EQ(Manager()->SearchResolvedExact(decl->GetImportInfo()), decl);
    EXPECT_EQ(Manager()->SearchResolved(decl->GetImportInfo()), implementation);
    Manager()->RemoveProgramFromResolvedSources(ArenaString(decl->GetImportInfo().Key()));
    EXPECT_EQ(Manager()->SearchResolvedExact(decl->GetImportInfo()), nullptr);
    EXPECT_EQ(Manager()->SearchResolved(implementation->GetImportInfo()), implementation);
}

TEST_F(ImportPathManagerTest, DistinctImplementationsWithSameRuntimeNameAreDiagnosed)
{
    Write("one/Item.ats");
    Write("two/Item.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("paths":{"item":["one/Item.ats","two/Item.ats"]})"));
    ASSERT_NE(Import("./one/Item"), nullptr);
    ASSERT_NE(Import("./two/Item"), nullptr);
    ExpectDiagnostic(diagnostic::FILE_RUNTIME_NAME_CLASH);
    Context()->diagnosticEngine->ClearDiagnostics();
}

TEST_F(ImportPathManagerTest, ImportingDirectoryGroupsOnlyImmediatePackageFractions)
{
    Write("pkg/One.ats");
    Write("pkg/Two.ets");
    Write("pkg/ignored.ts");
    Write("pkg/nested/Three.ats");
    ASSERT_NO_FATAL_FAILURE(Start());
    auto *existing = Import("./pkg/One");
    ASSERT_NE(existing, nullptr);
    auto *dir = Import("./pkg");
    ASSERT_NE(dir, nullptr);
    ASSERT_TRUE(dir->Is<ModuleKind::PACKAGE>());
    const auto &fractions = dir->As<ModuleKind::PACKAGE>()->GetUnmergedPackagePrograms();
    std::unordered_set<std::string> paths;
    for (const auto *fraction : fractions) {
        paths.emplace(fraction->GetImportInfo().ResolvedSource());
        EXPECT_EQ(fraction->ModuleName(), dir->ModuleName());
    }
    EXPECT_EQ(paths, (std::unordered_set<std::string> {Path("pkg/One.ats").string(), Path("pkg/Two.ets").string()}));
    EXPECT_EQ(std::count(fractions.begin(), fractions.end(), existing), 1);
    const auto &external = Main()->GetExternalPrograms()->Get<ModuleKind::MODULE>();
    EXPECT_EQ(std::count(external.begin(), external.end(), existing), 0);
    EXPECT_EQ(Import("./pkg"), dir);
    ImportInfo byName {*Manager(), dir->ModuleName()};
    ASSERT_EQ(Manager()->SearchResolved(byName), dir);
    Manager()->RemoveProgramFromResolvedSources(ArenaString(dir->GetImportInfo().Key()));
    EXPECT_EQ(Manager()->SearchResolved(byName), nullptr);
    EXPECT_EQ(Manager()->SearchResolvedExact(byName), nullptr);
    EXPECT_EQ(Manager()->SearchResolved(dir->GetImportInfo()), nullptr);
}

TEST_F(ImportPathManagerTest, DeprecatedPackageDeclarationStillRegistersTheExactFraction)
{
    Write("Main.ats", "import { value } from './Part';");
    Write("Part.ats", "package Example; export const value: int = 42;");
    ASSERT_NO_FATAL_FAILURE(Configure(""));
    ASSERT_NO_FATAL_FAILURE(OpenFile("Main.ats"));
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    const auto &errors = Context()->diagnosticEngine->GetDiagnosticStorage(DiagnosticType::SYNTAX);
    ASSERT_EQ(errors.size(), 1U);
    ExpectDiagnostic(diagnostic::PACKAGE_DEPRECATED);
    Context()->diagnosticEngine->ClearDiagnostics();
    ImportInfo key {*Manager(), Path("Part.ats").string()};
    auto *fraction = Manager()->SearchResolvedExact(key);
    auto *package = Manager()->SearchResolved(key);
    ASSERT_NE(fraction, nullptr);
    ASSERT_NE(package, nullptr);
    ASSERT_TRUE(package->Is<ModuleKind::PACKAGE>());
    EXPECT_EQ(package->ModuleName(), "Example");
    const auto &fractions = package->As<ModuleKind::PACKAGE>()->GetUnmergedPackagePrograms();
    ASSERT_EQ(fractions.size(), 1U);
    EXPECT_EQ(fractions.front(), fraction);
}

TEST_F(ImportPathManagerTest, ExplicitOutputOverridesCacheDirectory)
{
    Write("Leaf.ats");
    const auto output = Path("combined.abc");
    ASSERT_NO_FATAL_FAILURE(Start(CacheDirectory(), {"--output=" + output.string(), Path("Main.ats").string()}));
    auto *leaf = Import("./Leaf");
    ASSERT_NE(leaf, nullptr);
    EXPECT_EQ(fs::path(Manager()->FormAbcFilePath(leaf->GetImportInfo())), output);
    EXPECT_EQ(fs::path(Manager()->FormAbcFilePath(Main()->GetImportInfo())), output);
}

TEST_F(ImportPathManagerTest, IncrementalLookupAttachesExternalOnlyWhenImported)
{
    Write("Incremental.ats");
    ASSERT_NO_FATAL_FAILURE(Start());
    auto *program = Manager()->FindOrIntroduceProgramForIncremental(Path("Incremental.ats").string());
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(Manager()->FindOrIntroduceProgramForIncremental(Path("Incremental.ats").string()), program);
    const auto &external = Main()->GetExternalPrograms()->Get<ModuleKind::MODULE>();
    EXPECT_TRUE(external.empty());
    EXPECT_EQ(Import("./Incremental"), program);
    ASSERT_EQ(external.size(), 1U);
    EXPECT_EQ(external.front(), program);
}

TEST_F(ImportPathManagerTest, ReregisteringAnEvictedProgramCanReuseSourceTextFromMemory)
{
    Write("Cached.ats", "export const cachedValue: int = 7;");
    ASSERT_NO_FATAL_FAILURE(Start());
    auto *first = Import("./Cached");
    ASSERT_NE(first, nullptr);
    const auto key = Path("Cached.ats").string();
    Manager()->RemoveProgramFromResolvedSources(ArenaString(key));
    ASSERT_TRUE(fs::remove(Path("Cached.ats")));
    auto *restored = Manager()->FindOrIntroduceProgramForIncremental(key);
    ASSERT_NE(restored, nullptr);
    EXPECT_NE(restored, first);
    EXPECT_EQ(restored->SourceCode(), "export const cachedValue: int = 7;");
    EXPECT_EQ(Manager()->SearchResolvedExact(restored->GetImportInfo()), restored);
}

TEST_F(ImportPathManagerTest, PreviouslyParsedExternalProgramIsReusedWithoutReparsing)
{
    Write("Main.ats", "import { value } from './Leaf';");
    Write("Leaf.ats");
    ASSERT_NO_FATAL_FAILURE(Configure(""));
    ASSERT_NO_FATAL_FAILURE(OpenFile("Main.ats"));
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    auto *leaf = Import("./Leaf");
    ASSERT_NE(leaf, nullptr);
    ASSERT_NE(leaf->Ast(), nullptr);
    auto key = Path("Leaf.ats").string();
    Manager()->RemoveProgramFromResolvedSources(ArenaString(key));
    public_lib::GlobalContext global;
    global.cachedExternalPrograms.emplace(key, leaf);
    Context()->globalContext = &global;
    auto *reused = Import("./Leaf");
    Context()->globalContext = nullptr;
    EXPECT_EQ(reused, leaf);
    Manager()->RemoveProgramFromResolvedSources(ArenaString(key));
    global.cachedExternalPrograms.clear();
    parser::Program::ExternalPrograms stdlibCache;
    stdlibCache.Add(leaf);
    global.stdLibAstCache = &stdlibCache;
    Context()->globalContext = &global;
    auto *fromStdlibCache = Import("./Leaf");
    Context()->globalContext = nullptr;
    EXPECT_EQ(fromStdlibCache, leaf);
    EXPECT_TRUE(Manager()->GetParseQueue().back().isParsed);
}

TEST_F(ImportPathManagerTest, LspDependencyGraphUsesHostDriveLetterRules)
{
    ASSERT_NO_FATAL_FAILURE(Start());
    Context()->isLspUsage = true;
    Manager()->AddFileDependencies("d:/Parent.ats", "c:/Leaf.ats");
#ifdef PANDA_TARGET_WINDOWS
    EXPECT_EQ(Manager()->CollectReverseDependencies("C:/Leaf.ats"),
              (std::unordered_set<std::string> {"D:/Parent.ats"}));
    EXPECT_EQ(Manager()->CollectReverseDependencies("c:/Leaf.ats"),
              (std::unordered_set<std::string> {"D:/Parent.ats"}));
#else
    EXPECT_TRUE(Manager()->CollectReverseDependencies("C:/Leaf.ats").empty());
    EXPECT_EQ(Manager()->CollectReverseDependencies("c:/Leaf.ats"),
              (std::unordered_set<std::string> {"d:/Parent.ats"}));
#endif
}

TEST_F(ImportPathManagerTest, ReverseDependencyClosureHandlesDiamondCycleAndDuplicateEdges)
{
    ASSERT_NO_FATAL_FAILURE(Start());
    auto *manager = Manager();
    manager->AddFileDependencies("A", "B");
    manager->AddFileDependencies("A", "C");
    manager->AddFileDependencies("B", "D");
    manager->AddFileDependencies("B", "D");
    manager->AddFileDependencies("C", "D");
    manager->AddFileDependencies("D", "A");
    manager->AddFileDependencies("X", "Y");
    EXPECT_EQ(manager->CollectReverseDependencies("D"), (std::unordered_set<std::string> {"A", "B", "C"}));
    EXPECT_EQ(manager->GetFileDependencies().at(ArenaString("B")).size(), 1U);
    manager->RemoveProgramsFromFileDependencies({"B", "C"});
    EXPECT_TRUE(manager->CollectReverseDependencies("D").empty());
    EXPECT_TRUE(manager->GetFileDependencies().at(ArenaString("A")).empty());
    EXPECT_EQ(manager->GetFileDependencies().count(ArenaString("B")), 0U);
    EXPECT_EQ(manager->CollectReverseDependencies("A"), (std::unordered_set<std::string> {"D"}));
    EXPECT_EQ(manager->CollectReverseDependencies("Y"), (std::unordered_set<std::string> {"X"}));
}

TEST_F(ImportPathManagerTest, PreparingProgramForReparseDropsOutgoingEdgesAndRefreshesFileProbes)
{
    Write("Dep.d.ats");
    ASSERT_NO_FATAL_FAILURE(Start());
    auto *dep = Import("./Dep");
    ASSERT_NE(dep, nullptr);
    Manager()->AddFileDependencies("upstream", Main()->AbsoluteName().Utf8());
    Write("Dep.ats");
    Manager()->PrepareParseQueueForProgram(Main());
    ASSERT_EQ(Manager()->GetParseQueue().size(), 1U);
    EXPECT_EQ(Manager()->GetParseQueue().front().program, Main());
    EXPECT_FALSE(Manager()->GetParseQueue().front().isParsed);
    EXPECT_TRUE(Manager()->CollectReverseDependencies(dep->GetImportInfo().Key()).empty());
    EXPECT_EQ(Manager()->CollectReverseDependencies(Main()->AbsoluteName().Utf8()),
              (std::unordered_set<std::string> {"upstream"}));
    EXPECT_EQ(Resolve("./Dep"), Path("Dep.ats"));
}

TEST_F(ImportPathManagerTest, SimultaneousInputsHaveIndividualQueueEntriesAndSyntheticMain)
{
    Write("Other.ats");
    const auto output = Path("out");
    ASSERT_TRUE(fs::create_directory(output));
    ASSERT_NO_FATAL_FAILURE(
        Configure("", {"--simultaneous=true", "--incremental=true", "--output=" + output.string()}));
    const auto main = Path("Main.ats").string();
    const auto other = Path("Other.ats").string();
    const char *files[] = {main.c_str(), other.c_str()};
    handle_ = impl_->CreateContextSimultaneousMode(config_, static_cast<int>(std::size(files)), files);
    ASSERT_NE(handle_, nullptr);
    Manager()->InitParseQueueForSimult();
    ASSERT_NE(Main(), nullptr);
    EXPECT_TRUE(Main()->Is<ModuleKind::SIMULT_MAIN>());
    std::unordered_set<std::string> inputs;
    for (const auto &item : Manager()->GetParseQueue()) {
        ASSERT_NE(item.program, nullptr);
        EXPECT_TRUE(item.program->IsBuiltSimultaneously());
        EXPECT_TRUE(item.program->GetImportInfo().PointsToPackage());
        EXPECT_FALSE(item.isParsed);
        inputs.emplace(item.program->GetImportInfo().ResolvedSource());
        const auto expected = output / (std::string(item.program->ModuleName()) + ".abc");
        EXPECT_EQ(fs::path(Manager()->FormAbcFilePath(item.program->GetImportInfo())), expected);
    }
    EXPECT_EQ(inputs, (std::unordered_set<std::string> {main, other}));
    EXPECT_EQ(Main()->GetExternalPrograms()->Get<ModuleKind::MODULE>().size(), 2U);
}

TEST_F(ImportPathManagerTest, DebuggerAndDefaultImportProgramsCanBeLocatedAgain)
{
    ASSERT_NO_FATAL_FAILURE(Start());
    auto debugPath = Path("Debug.ats").string();
    auto *debug = Manager()->SetupProgramForDebugInfoPlugin(debugPath, "Debug");
    ASSERT_NE(debug, nullptr);
    ASSERT_NE(debug->Ast(), nullptr);
    EXPECT_EQ(Manager()->SearchResolved(debug->GetImportInfo()), debug);
    auto *defaults = Manager()->IntroduceStdlibImportProgram("import * as ns from 'std.core';");
    ASSERT_NE(defaults, nullptr);
    EXPECT_EQ(Manager()->IntroduceStdlibImportProgram(""), defaults);
}

}  // namespace ark::es2panda::util::test
