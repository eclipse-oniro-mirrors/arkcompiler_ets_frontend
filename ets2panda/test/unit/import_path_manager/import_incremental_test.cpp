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
namespace {

TEST_F(ImportPathManagerTest, EditingAParsedFileReplacesItsImportsAndRetainsIncomingEdges)
{
    const auto main = Path("Main.ats").string();
    const auto first = Path("First.ats").string();
    const auto second = Path("Second.ats").string();
    const std::string initial = "import { value } from './First'; export function main(): int { return value; }";
    const std::string edited = "import { value } from './Second'; export function main(): int { return value; }";
    Write("Main.ats", initial);
    Write("First.ats");
    Write("Second.ats");
    ASSERT_NO_FATAL_FAILURE(Configure(""));
    handle_ = impl_->CreateCacheContextFromString(config_, initial.c_str(), main.c_str(), nullptr, false, true);
    ASSERT_NE(handle_, nullptr);
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    auto *original = Main();
    Manager()->AddFileDependencies("consumer", main);
    auto *tree = original->Ast();
    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(impl_->IncrementalPrepareProgram(handle_, main.c_str(), initial.c_str(), false), 1);
    EXPECT_EQ(Main()->Ast(), tree);
    EXPECT_EQ(Manager()->CollectReverseDependencies(first), (std::unordered_set<std::string> {main, "consumer"}));

    ASSERT_EQ(impl_->IncrementalPrepareProgram(handle_, main.c_str(), edited.c_str(), true), 0);
    EXPECT_EQ(Main(), original);
    EXPECT_EQ(Main()->SourceCode(), edited);
    ASSERT_EQ(Manager()->GetParseQueue().size(), 1U);
    EXPECT_EQ(Manager()->GetParseQueue().front().program, original);
    EXPECT_FALSE(Manager()->GetParseQueue().front().isParsed);
    EXPECT_TRUE(Manager()->CollectReverseDependencies(first).empty());
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    EXPECT_EQ(Manager()->CollectReverseDependencies(second), (std::unordered_set<std::string> {main, "consumer"}));
    EXPECT_EQ(Manager()->GetFileDependencies().at(ArenaString(main)).count(ArenaString(first)), 0U);
}

TEST_F(ImportPathManagerTest, RemovingUnknownAndRepeatedKeysLeavesUnrelatedProgramAndGraphIntact)
{
    Write("Survives.ats");
    ASSERT_NO_FATAL_FAILURE(Start());
    auto *survives = Import("./Survives");
    ASSERT_NE(survives, nullptr);
    auto key = std::string(survives->GetImportInfo().Key());
    auto main = std::string(Main()->GetImportInfo().Key());
    Manager()->RemoveFileDependencies("");
    Manager()->RemoveFileDependencies("unknown");
    Manager()->RemoveProgramsFromFileDependencies({});
    Manager()->RemoveProgramsFromFileDependencies({"", "unregistered"});
    Manager()->RemoveProgramFromResolvedSources(ArenaString("missing"));
    EXPECT_EQ(Manager()->SearchResolvedExact(survives->GetImportInfo()), survives);
    EXPECT_EQ(Manager()->CollectReverseDependencies(key), (std::unordered_set<std::string> {main}));
    Manager()->RemoveFileDependencies(main);
    Manager()->RemoveFileDependencies(main);
    EXPECT_TRUE(Manager()->CollectReverseDependencies(key).empty());
    EXPECT_EQ(Manager()->SearchResolved(survives->GetImportInfo()), survives);
}

TEST_F(ImportPathManagerTest, RemovingOneOfSeveralDeclarationsOnlyRestoresSurvivingDeclarations)
{
    Write("first/Thing.d.ats");
    Write("second/Thing.d.ets");
    Write("src/Thing.ats");
    ASSERT_NO_FATAL_FAILURE(
        Start(R"("paths":{"lib.Thing":["src/Thing.ats","first/Thing.d.ats","second/Thing.d.ets"]})"));
    auto *first = Import("./first/Thing.d.ats");
    auto *second = Import("./second/Thing.d.ets");
    auto *implementation = Import("./src/Thing.ats");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    ASSERT_NE(implementation, nullptr);
    EXPECT_FALSE(Manager()->IsReplacedExactSource(nullptr));
    EXPECT_FALSE(Manager()->IsReplacedExactSource(implementation));
    EXPECT_TRUE(Manager()->IsReplacedExactSource(first));
    EXPECT_TRUE(Manager()->IsReplacedExactSource(second));
    Manager()->RemoveProgramFromResolvedSources(ArenaString(first->GetImportInfo().Key()));
    EXPECT_EQ(Manager()->SearchResolved(first->GetImportInfo()), nullptr);
    EXPECT_FALSE(Manager()->IsReplacedExactSource(first));
    EXPECT_EQ(Manager()->SearchResolved(second->GetImportInfo()), implementation);
    Manager()->RemoveProgramFromResolvedSources(ArenaString(implementation->GetImportInfo().Key()));
    EXPECT_EQ(Manager()->SearchResolved(second->GetImportInfo()), second);
    EXPECT_EQ(Manager()->SearchResolvedExact(second->GetImportInfo()), second);
    EXPECT_EQ(Manager()->SearchResolved(first->GetImportInfo()), nullptr);
}

TEST_F(ImportPathManagerTest, InvalidIncrementalOutputReportsTheDirectoryRequirement)
{
    const auto output = Path("target");
    ASSERT_TRUE(fs::create_directory(output));
    ASSERT_NO_FATAL_FAILURE(
        Configure("", {"--simultaneous=true", "--incremental=true", "--output=" + output.string()}));
    const auto main = Path("Main.ats").string();
    const char *files[] = {main.c_str()};
    handle_ = impl_->CreateContextSimultaneousMode(config_, 1, files);
    ASSERT_NE(handle_, nullptr);
    Manager()->InitParseQueueForSimult();
    ASSERT_TRUE(fs::remove(output));
    Write("target", "");
    ASSERT_EQ(Manager()->GetParseQueue().size(), 1U);
    const auto &info = Manager()->GetParseQueue().front().program->GetImportInfo();
    EXPECT_TRUE(Manager()->FormAbcFilePath(info).empty());
    ExpectDiagnostic(diagnostic::SIMULTANEOUS_INCREMENTAL_OUTPUT);
    Context()->diagnosticEngine->ClearDiagnostics();
    ASSERT_TRUE(fs::remove(output));
    ASSERT_TRUE(fs::create_directory(output));
    EXPECT_EQ(fs::path(Manager()->FormAbcFilePath(info)), output / "Main.abc");
}

TEST_F(ImportPathManagerTest, DependencyEditsAreIsolatedAcrossStronglyConnectedComponents)
{
    ASSERT_NO_FATAL_FAILURE(Start());
    auto *manager = Manager();
    manager->AddFileDependencies("app", "api");
    manager->AddFileDependencies("api", "engine");
    manager->AddFileDependencies("engine", "api");
    manager->AddFileDependencies("engine", "store");
    manager->AddFileDependencies("store", "backend");
    manager->AddFileDependencies("backend", "store");
    manager->AddFileDependencies("tools", "store");
    manager->AddFileDependencies("tools", "tools");
    EXPECT_EQ(manager->CollectReverseDependencies("backend"),
              (std::unordered_set<std::string> {"app", "api", "engine", "store", "tools"}));
    manager->RemoveFileDependencies("engine");
    EXPECT_EQ(manager->CollectReverseDependencies("backend"), (std::unordered_set<std::string> {"store", "tools"}));
    manager->AddFileDependencies("engine", "backend");
    manager->RemoveProgramsFromFileDependencies({"tools", "store"});
    EXPECT_EQ(manager->CollectReverseDependencies("backend"),
              (std::unordered_set<std::string> {"app", "api", "engine"}));
    EXPECT_TRUE(manager->CollectReverseDependencies("store").empty());
    EXPECT_TRUE(manager->CollectReverseDependencies("tools").empty());
    EXPECT_TRUE(manager->GetFileDependencies().at(ArenaString("backend")).empty());
}

TEST_F(ImportPathManagerTest, ParsedMultiPackageProjectSharesOneLeafAcrossAllImportingFeatures)
{
    constexpr size_t featureCount = 32;
    std::string mainText;
    std::unordered_set<std::string> consumers {Path("Main.ats").string()};
    for (size_t index = 0; index < featureCount; ++index) {
        const auto feature = "Feature" + std::to_string(index);
        mainText += "import { " + feature + " } from '@app/features/" + feature + "';\n";
        const auto file = "modules/ui/src/" + feature + ".ats";
        Write(file, "import { value } from '@app/core/Leaf'; export function " + feature + "(): int { return value; }");
        consumers.emplace(Path(file).string());
    }
    Write("Main.ats", mainText);
    Write("modules/core/src/Leaf.ats");
    ASSERT_NO_FATAL_FAILURE(Configure(R"("atsDependencies":{
        "@app/features":{"moduleRoot":"modules/ui", "searchPath":["src"]},
        "@app/core":{"moduleRoot":"modules/core", "searchPath":["src"]}})"));
    ASSERT_NO_FATAL_FAILURE(OpenFile("Main.ats"));
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    const auto main = Path("Main.ats").string();
    const auto leaf = Path("modules/core/src/Leaf.ats").string();
    EXPECT_EQ(Manager()->CollectReverseDependencies(leaf), consumers);
    const auto &graph = Manager()->GetFileDependencies();
    const auto &mainImports = graph.at(ArenaString(main));
    ImportInfo leafInfo {*Manager(), leaf};
    auto *leafProgram = Manager()->SearchResolvedExact(leafInfo);
    ASSERT_NE(leafProgram, nullptr);
    EXPECT_EQ(leafProgram->ModuleName(), "@app/core.src.Leaf");
    for (const auto &consumer : consumers) {
        if (consumer == main) {
            continue;
        }
        SCOPED_TRACE(consumer);
        EXPECT_EQ(mainImports.count(ArenaString(consumer)), 1U);
        EXPECT_EQ(graph.at(ArenaString(consumer)).count(ArenaString(leaf)), 1U);
    }
    auto &queue = Manager()->GetParseQueue();
    EXPECT_EQ(std::count_if(queue.begin(), queue.end(),
                            [leafProgram](const auto &item) { return item.program == leafProgram && item.isParsed; }),
              1);
}

TEST_F(ImportPathManagerTest, UnsuccessfulResolutionDoesNotRegisterAPhantomProgramOrOutput)
{
    ASSERT_NO_FATAL_FAILURE(Start(R"("paths":{"feature":["missing/Feature.ats"]})"));
    const auto queued = Manager()->GetParseQueue().size();
    const auto outputs = Manager()->GetOutputMatching().size();
    EXPECT_EQ(Import("feature"), nullptr);
    ExpectDiagnostic(diagnostic::IMPORT_CANT_FIND_PREFIX);
    Context()->diagnosticEngine->ClearDiagnostics();
    EXPECT_EQ(Manager()->GetParseQueue().size(), queued);
    EXPECT_EQ(Manager()->GetOutputMatching().size(), outputs);
    EXPECT_TRUE(Manager()->GetFileDependencies().empty());
    Write("missing/Feature.ats");
    Manager()->ClearResolutionCaches();
    auto *program = Import("feature");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(Manager()->GetParseQueue().size(), queued + 1);
    EXPECT_EQ(Manager()->GetOutputMatching().size(), outputs + 1);
    EXPECT_EQ(program->ModuleName(), "feature");
}

}  // namespace
}  // namespace ark::es2panda::util::test
