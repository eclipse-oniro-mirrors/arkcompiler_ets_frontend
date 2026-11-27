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

#include <array>
#include <set>

#include "import_path_manager_test.h"

namespace ark::es2panda::util::test {
namespace {

struct ImportEdit {
    const char *name;
    std::vector<std::vector<std::string>> revisions;
    std::vector<std::set<std::string>> expected;
};

std::string MainWithImports(const std::vector<std::string> &specifiers)
{
    std::string source;
    for (size_t i = 0; i < specifiers.size(); ++i) {
        source += "import { value as imported" + std::to_string(i) + " } from '" + specifiers[i] + "';\n";
    }
    return source + "export function main(): void {}";
}

class ImportEditingSessionTest : public ImportPathManagerTest, public testing::WithParamInterface<ImportEdit> {
protected:
    void ExpectDirectImports(const std::set<std::string> &expected)
    {
        const auto main = Path("Main.ats").string();
        const auto &graph = Manager()->GetFileDependencies();
        const auto owner = graph.find(ArenaString(main));
        std::set<std::string> actual;
        if (owner != graph.end()) {
            for (const auto &dependency : owner->second) {
                const auto relative = fs::path(std::string(dependency)).lexically_relative(root_);
                if (!relative.empty() && *relative.begin() != "..") {
                    actual.insert(relative.generic_string());
                }
            }
        }
        for (const auto *file : {"src/First.ats", "src/Second.ets", "types/Third.d.ats"}) {
            const auto key = Path(file).string();
            const auto importers = Manager()->CollectReverseDependencies(key);
            EXPECT_EQ(importers.count(main), expected.count(file)) << file;
        }
        EXPECT_EQ(actual, expected);
    }
};

TEST_P(ImportEditingSessionTest, ReparseUpdatesBothDirectionsAndKeepsTheEntryProgramAcrossEdits)
{
    const auto &edit = GetParam();
    ASSERT_EQ(edit.revisions.size(), edit.expected.size());
    ASSERT_FALSE(edit.revisions.empty());
    const auto main = Path("Main.ats").string();
    Write("src/First.ats");
    Write("src/Second.ets");
    Write("types/Third.d.ats", "export declare const value: int;");
    const auto opening = MainWithImports(edit.revisions.front());
    Write("Main.ats", opening);
    ASSERT_NO_FATAL_FAILURE(Configure(R"("paths":{"first":["src/First.ats"],"second":["src/Second.ets"],
                                             "third":["types/Third.d.ats"]})"));
    handle_ = impl_->CreateCacheContextFromString(config_, opening.c_str(), main.c_str(), nullptr, false, true);
    ASSERT_NE(handle_, nullptr);
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    auto *entry = Main();
    ASSERT_NE(entry, nullptr);
    ExpectDirectImports(edit.expected.front());

    for (size_t i = 1; i < edit.revisions.size(); ++i) {
        SCOPED_TRACE(i);
        auto source = MainWithImports(edit.revisions[i]);
        ASSERT_EQ(impl_->IncrementalPrepareProgram(handle_, main.c_str(), source.c_str(), true), 0);
        ASSERT_EQ(Manager()->GetParseQueue().size(), 1U);
        EXPECT_EQ(Manager()->GetParseQueue().front().program, entry);
        impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
        ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
        EXPECT_EQ(Main(), entry);
        EXPECT_EQ(Main()->SourceCode(), source);
        EXPECT_EQ(Manager()->SearchResolvedExact(entry->GetImportInfo()), entry);
        ExpectDirectImports(edit.expected[i]);
    }
}

const ImportEdit EDITS[] = {
    {"RetargetThenUndo",
     {{"first"}, {"second"}, {"first"}},
     {{"src/First.ats"}, {"src/Second.ets"}, {"src/First.ats"}}},
    {"RemoveAllThenAddDeclaration",
     {{"first", "second"}, {}, {"third"}},
     {{"src/First.ats", "src/Second.ets"}, {}, {"types/Third.d.ats"}}},
    {"AliasBecomesRelativeThenExplicit",
     {{"first"}, {"./src/First"}, {"./src/First.ats"}},
     {{"src/First.ats"}, {"src/First.ats"}, {"src/First.ats"}}},
    {"RemoveOneOfTwoSpellings",
     {{"first", "./src/First.ats"}, {"./src/First"}, {"second"}},
     {{"src/First.ats"}, {"src/First.ats"}, {"src/Second.ets"}}},
    {"ReorderAndDropOneEdge",
     {{"second", "third", "first"}, {"first", "third", "second"}, {"third", "second"}},
     {{"src/First.ats", "src/Second.ets", "types/Third.d.ats"},
      {"src/First.ats", "src/Second.ets", "types/Third.d.ats"},
      {"src/Second.ets", "types/Third.d.ats"}}},
    {"StartEmptyAndRestoreEmpty",
     {{}, {"third"}, {"first", "third"}, {}},
     {{}, {"types/Third.d.ats"}, {"src/First.ats", "types/Third.d.ats"}, {}}},
};

INSTANTIATE_TEST_SUITE_P(EditorRevisions, ImportEditingSessionTest, testing::ValuesIn(EDITS),
                         [](const auto &info) { return info.param.name; });

class ImportBuildSessionTest : public ImportPathManagerTest {
protected:
    void ExpectSelectedProgram(std::string_view request, std::string_view key, std::string_view textFile,
                               std::string_view contents, ModuleKind kind)
    {
        auto *program = Import(request);
        ASSERT_NE(program, nullptr);
        const auto &info = program->GetImportInfo();
        EXPECT_EQ(info.Kind(), kind);
        EXPECT_EQ(info.ResolvedSource(), Path(key).string());
        EXPECT_EQ(info.TextSource(), Path(textFile).string());
        EXPECT_EQ(program->SourceCode(), contents);
        EXPECT_EQ(Manager()->SearchResolvedExact(info), program);
        EXPECT_EQ(Import(request), program);
    }
};

TEST_F(ImportBuildSessionTest, GeneratedDeclarationAppearsAndDisappearsBetweenBuilds)
{
    const auto options = CacheDirectory() + R"(,"paths":{"app.Entry":["src/Entry.ats"]})";
    const std::string implementation = "export const initial: int = 1;";
    const std::string declaration = "export declare const generated: int;";
    const std::string revised = "export const revised: int = 2;";
    Write("src/Entry.ats", implementation);
    ASSERT_NO_FATAL_FAILURE(Start(options));
    ExpectSelectedProgram("app.Entry", "src/Entry.ats", "src/Entry.ats", implementation, ModuleKind::MODULE);
    Write("cache/app/Entry.etscache", declaration);
    Manager()->ClearResolutionCaches();
    ExpectSelectedProgram("app.Entry", "src/Entry.ats", "src/Entry.ats", implementation, ModuleKind::MODULE);
    Close();

    ASSERT_NO_FATAL_FAILURE(Start(options));
    ExpectSelectedProgram("app.Entry", "src/Entry.ats", "cache/app/Entry.etscache", declaration,
                          ModuleKind::ETSCACHE_DECL);
    ASSERT_TRUE(fs::remove(Path("cache/app/Entry.etscache")));
    Write("src/Entry.ats", revised);
    Manager()->ClearResolutionCaches();
    ExpectSelectedProgram("app.Entry", "src/Entry.ats", "cache/app/Entry.etscache", declaration,
                          ModuleKind::ETSCACHE_DECL);
    Close();

    ASSERT_NO_FATAL_FAILURE(Start(options));
    ExpectSelectedProgram("app.Entry", "src/Entry.ats", "src/Entry.ats", revised, ModuleKind::MODULE);
}

TEST_F(ImportBuildSessionTest, ProfilesCanMapOneLogicalModuleToDifferentDeclarationCaches)
{
    const std::string common = R"(,"paths":{"ui.View":["widgets/View.ats"]})";
    const std::string source = "export class View {}";
    const std::string debug = "export declare class DebugView {}";
    const std::string release = "export declare class ReleaseView {}";
    Write("widgets/View.ats", source);
    Write("debug/ui/View.etscache", debug);
    Write("release/ui/View.etscache", release);
    for (const auto &cache : {"debug", "release", "empty", "debug"}) {
        SCOPED_TRACE(cache);
        const auto name = std::string(cache);
        ASSERT_NO_FATAL_FAILURE(Start(R"("cacheDir":)" + JsonPath(Path(name)) + common));
        if (name == "empty") {
            ExpectSelectedProgram("ui.View", "widgets/View.ats", "widgets/View.ats", source, ModuleKind::MODULE);
        } else {
            ExpectSelectedProgram("ui.View", "widgets/View.ats", name + "/ui/View.etscache",
                                  name == "debug" ? debug : release, ModuleKind::ETSCACHE_DECL);
        }
        auto *program = Import("ui.View");
        ASSERT_NE(program, nullptr);
        EXPECT_EQ(fs::path(Manager()->FormAbcFilePath(program->GetImportInfo())), Path(name + "/ui.View.abc"));
        Close();
    }
}

TEST_F(ImportBuildSessionTest, MockedAndProductionBuildsRecordTheirOwnPhysicalDependencies)
{
    Write("src/Clock.ats", "export const clock: int = 1;");
    Write("doubles/Clock.ats", "export const fake: int = 2;");
    const std::string paths = R"("paths":{"clock":["src/Clock.ats"]})";
    for (bool mock : {false, true, false}) {
        SCOPED_TRACE(mock);
        const auto config = mock ? paths + R"(,"mock":{"clock":{"source":"doubles/Clock.ats"}})" : paths;
        ASSERT_NO_FATAL_FAILURE(Start(config));
        const auto actual = Path(mock ? "doubles/Clock.ats" : "src/Clock.ats").string();
        const auto other = Path(mock ? "src/Clock.ats" : "doubles/Clock.ats").string();
        auto *clock = Import("clock");
        ASSERT_NE(clock, nullptr);
        EXPECT_EQ(clock->GetImportInfo().ResolvedSource(), actual);
        const auto &edges = Manager()->GetFileDependencies().at(ArenaString(Path("Main.ats").string()));
        EXPECT_EQ(edges.size(), 1U);
        EXPECT_EQ(edges.count(ArenaString(actual)), 1U);
        EXPECT_TRUE(Manager()->CollectReverseDependencies(other).empty());
        EXPECT_EQ(Manager()->GetOutputMatching().count(ArenaString(other)), 0U);
        Close();
    }
}

TEST_F(ImportBuildSessionTest, TwoDirectoriesCanResolveTheSameRelativeTextToDifferentPrograms)
{
    for (const auto *dir : {"feature", "support"}) {
        Write(std::string(dir) + "/Entry.ats");
        Write(std::string(dir) + "/Model.ats");
    }
    ASSERT_NO_FATAL_FAILURE(Start(R"("package":"app")"));
    auto *feature = Import("./feature/Entry");
    auto *support = Import("./support/Entry");
    ASSERT_NE(feature, nullptr);
    ASSERT_NE(support, nullptr);
    auto *first = Import("./Model", feature);
    auto *second = Import("./Model", support);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first, second);
    EXPECT_EQ(first->ModuleName(), "app.feature.Model");
    EXPECT_EQ(second->ModuleName(), "app.support.Model");
    EXPECT_EQ(Import("./Model.ats", support), second);
    EXPECT_EQ(Import("./Model.ats", feature), first);
    const auto main = Path("Main.ats").string();
    EXPECT_EQ(Manager()->CollectReverseDependencies(Path("support/Model.ats").string()),
              (std::unordered_set<std::string> {main, Path("support/Entry.ats").string()}));
    EXPECT_EQ(Manager()->CollectReverseDependencies(Path("feature/Model.ats").string()),
              (std::unordered_set<std::string> {main, Path("feature/Entry.ats").string()}));
}

TEST_F(ImportBuildSessionTest, RelativeImportsThroughASymlinkReuseTheCanonicalProgramAndOneDependency)
{
    Write("real/Api.ats");
    std::error_code error;
    fs::create_directory_symlink(Path("real"), Path("alias"), error);
    if (error) {
        GTEST_SKIP() << "Directory symlinks unavailable: " << error.message();
    }
    ASSERT_NO_FATAL_FAILURE(Start());
    auto *viaLink = Import("./alias/Api");
    ASSERT_NE(viaLink, nullptr);
    auto *viaSource = Import("./real/Api.ats");
    EXPECT_EQ(viaSource, viaLink);
    EXPECT_EQ(viaLink->GetImportInfo().ResolvedSource(), Path("real/Api.ats").string());
    ASSERT_EQ(Manager()->GetParseQueue().size(), 2U);
    EXPECT_EQ(Manager()->GetParseQueue().back().program, viaLink);
    const auto &edges = Manager()->GetFileDependencies().at(ArenaString(Path("Main.ats").string()));
    ASSERT_EQ(edges.size(), 1U);
    EXPECT_EQ(edges.count(ArenaString(Path("real/Api.ats").string())), 1U);
}

constexpr std::array REGISTRATION_FILES {"src/Item.ats", "types/Item.d.ats", "legacy/Item.d.ets"};

struct RemovalOrder {
    const char *name;
    std::array<size_t, REGISTRATION_FILES.size()> order;
};

class ModuleRegistrationHistoryTest : public ImportPathManagerTest, public testing::WithParamInterface<RemovalOrder> {
protected:
    using Programs = std::array<parser::Program *, REGISTRATION_FILES.size()>;
    using Presence = std::array<bool, REGISTRATION_FILES.size()>;

    void ExpectLookups(const Programs &programs, const Presence &present)
    {
        for (size_t i = 0; i < programs.size(); ++i) {
            SCOPED_TRACE(i);
            ImportInfo info {*Manager(), Path(REGISTRATION_FILES[i]).string()};
            auto *exact = present[i] ? programs[i] : nullptr;
            auto *effective = present[i] && present[0] ? programs[0] : exact;
            EXPECT_EQ(Manager()->SearchResolvedExact(info), exact);
            EXPECT_EQ(Manager()->SearchResolved(info), effective);
            if (programs[i] != nullptr) {
                EXPECT_EQ(Manager()->IsReplacedExactSource(programs[i]), exact != nullptr && exact != effective);
            }
        }
    }
};

TEST_P(ModuleRegistrationHistoryTest, EveryLoadOrderPreservesExactAndEffectiveProgramsDuringEviction)
{
    for (const auto *file : REGISTRATION_FILES) {
        Write(file);
    }
    std::array<size_t, REGISTRATION_FILES.size()> loadOrder {0, 1, 2};
    do {
        SCOPED_TRACE(::testing::PrintToString(loadOrder));
        ASSERT_NO_FATAL_FAILURE(
            Start(R"("paths":{"lib.Item":["src/Item.ats","types/Item.d.ats","legacy/Item.d.ets"]})"));
        Programs programs {};
        Presence present {};
        ExpectLookups(programs, present);
        for (const auto index : loadOrder) {
            auto *program = Import("./" + std::string(REGISTRATION_FILES[index]));
            ASSERT_NE(program, nullptr);
            ASSERT_EQ(program->ModuleName(), "lib.Item");
            ASSERT_EQ(program->IsDeclarationModule(), index != 0U);
            programs[index] = program;
            present[index] = true;
            ExpectLookups(programs, present);
        }

        for (const auto index : GetParam().order) {
            Manager()->RemoveProgramFromResolvedSources(ArenaString(programs[index]->GetImportInfo().Key()));
            present[index] = false;
            ExpectLookups(programs, present);
        }
        Close();
    } while (std::next_permutation(loadOrder.begin(), loadOrder.end()));
}

const RemovalOrder REMOVAL_ORDERS[] = {
    {"ImplementationThenAtsThenEts", {0, 1, 2}}, {"ImplementationThenEtsThenAts", {0, 2, 1}},
    {"AtsThenImplementationThenEts", {1, 0, 2}}, {"AtsThenEtsThenImplementation", {1, 2, 0}},
    {"EtsThenImplementationThenAts", {2, 0, 1}}, {"EtsThenAtsThenImplementation", {2, 1, 0}}};

INSTANTIATE_TEST_SUITE_P(DeclarationEviction, ModuleRegistrationHistoryTest, testing::ValuesIn(REMOVAL_ORDERS),
                         [](const auto &info) { return info.param.name; });

}  // namespace
}  // namespace ark::es2panda::util::test
