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
#include <map>
#include <set>
#include <vector>

#include "import_path_manager_test.h"

namespace ark::es2panda::util::test {
namespace {

using Edge = std::pair<std::string, std::string>;
using Graph = std::map<std::string, std::set<std::string>>;

// Compute the impact of an edit from the expected forward edges. The manager answers
// the same question using a separate reverse index that it maintains across edits.
std::set<std::string> ExpectedImporters(const Graph &edges, const std::string &changed)
{
    std::set<std::string> affected {changed};
    size_t previousSize;
    do {
        previousSize = affected.size();
        for (const auto &[file, imports] : edges) {
            if (std::any_of(imports.begin(), imports.end(),
                            [&affected](const auto &key) { return affected.count(key) != 0; })) {
                affected.insert(file);
            }
        }
    } while (affected.size() != previousSize);
    affected.erase(changed);
    return affected;
}

class ImportGraphEditsTest : public ImportPathManagerTest {
protected:
    void ExpectGraph(const Graph &expected, const std::set<std::string> &files)
    {
        Graph actual;
        for (const auto &[file, imports] : Manager()->GetFileDependencies()) {
            for (const auto &dependency : imports) {
                actual[std::string(file)].insert(std::string(dependency));
            }
        }
        // Empty adjacency lists and absent keys both mean the file imports nothing.
        Graph nonempty;
        std::copy_if(expected.begin(), expected.end(), std::inserter(nonempty, nonempty.end()),
                     [](const auto &entry) { return !entry.second.empty(); });
        EXPECT_EQ(actual, nonempty);
        for (const auto &file : files) {
            SCOPED_TRACE(file);
            const auto result = Manager()->CollectReverseDependencies(file);
            EXPECT_EQ((std::set<std::string>(result.begin(), result.end())), ExpectedImporters(expected, file));
        }
    }
};

struct GraphRevision {
    const char *name;
    std::vector<Edge> initial;
    std::string edited;
    std::vector<std::string> replacementImports;
    std::unordered_set<std::string> deleted;
    std::vector<Edge> restored;
};

class ImportGraphHistoryTest : public ImportGraphEditsTest, public testing::WithParamInterface<GraphRevision> {};

TEST_P(ImportGraphHistoryTest, ForwardAndReverseEdgesAgreeAfterEachWorkspaceRevision)
{
    ASSERT_NO_FATAL_FAILURE(Start());
    const auto &revision = GetParam();
    Graph expected;
    std::set<std::string> known {revision.edited};
    auto add = [this, &expected, &known](const Edge &edge) {
        const auto &[file, dependency] = edge;
        Manager()->AddFileDependencies(file, dependency);
        expected[file].insert(dependency);
        known.insert(file);
        known.insert(dependency);
    };
    for (const auto &edge : revision.initial) {
        add(edge);
    }
    {
        SCOPED_TRACE("open workspace");
        ExpectGraph(expected, known);
    }

    Manager()->RemoveFileDependencies(revision.edited);
    expected.erase(revision.edited);
    for (const auto &dependency : revision.replacementImports) {
        add({revision.edited, dependency});
    }
    {
        SCOPED_TRACE("replace imports in one source");
        ExpectGraph(expected, known);
    }

    Manager()->RemoveProgramsFromFileDependencies(revision.deleted);
    for (const auto &removed : revision.deleted) {
        known.insert(removed);
        expected.erase(removed);
        for (auto &[file, imports] : expected) {
            imports.erase(removed);
        }
    }
    {
        SCOPED_TRACE("delete files");
        ExpectGraph(expected, known);
    }
    for (const auto &edge : revision.restored) {
        add(edge);
    }
    {
        SCOPED_TRACE("reintroduce imports");
        ExpectGraph(expected, known);
    }
}

const GraphRevision REVISIONS[] = {
    {"SplitSharedLibrary",
     {{"ui", "shared"}, {"cli", "shared"}, {"shared", "legacy"}},
     "shared",
     {"parser", "model"},
     {"legacy"},
     {{"cli", "legacy"}, {"legacy", "model"}}},
    {"MergeLayers",
     {{"app", "api"}, {"api", "bridge"}, {"bridge", "engine"}, {"test", "bridge"}},
     "api",
     {"engine"},
     {"bridge"},
     {{"test", "api"}}},
    {"BreakAndRestoreCycle",
     {{"app", "a"}, {"a", "b"}, {"b", "c"}, {"c", "a"}},
     "b",
     {"leaf"},
     {"c"},
     {{"b", "c"}, {"c", "a"}}},
    {"DeleteAdjacentCycleNodes",
     {{"ui", "a"}, {"cli", "b"}, {"a", "b"}, {"b", "a"}, {"b", "shared"}, {"test", "shared"}},
     "ui",
     {"a", "shared"},
     {"a", "b"},
     {{"ui", "a"}, {"a", "shared"}}},
    {"RenameSharedLeaf",
     {{"app", "left"}, {"app", "right"}, {"left", "old"}, {"right", "old"}},
     "left",
     {"new"},
     {"old"},
     {{"right", "new"}}},
    {"SelfImportAndRepeatedEdges",
     {{"first", "first"}, {"second", "first"}, {"second", "first"}},
     "first",
     {"second", "second", "first"},
     {"second"},
     {{"second", "first"}, {"second", "first"}}},
    {"DisconnectedToolsAndApplication",
     {{"app", "api"}, {"api", "store"}, {"tools", "reader"}, {"reader", "schema"}},
     "reader",
     {"schema", "api"},
     {"api"},
     {{"app", "service"}, {"service", "store"}}},
    {"RemoveAndRecreateTheImporter",
     {{"test", "app"}, {"app", "alpha"}, {"app", "beta"}, {"beta", "shared"}},
     "app",
     {"beta", "gamma"},
     {"app"},
     {{"test", "app"}, {"app", "shared"}}},
    {"PreviouslyUnseenConsumer",
     {{"main", "core"}, {"core", "types"}},
     "generated",
     {"core", "types"},
     {"notBuilt"},
     {{"main", "generated"}}},
    {"DeleteAllFiles", {{"a", "b"}, {"b", "c"}, {"c", "a"}}, "c", {"a", "b"}, {"a", "b", "c"}, {{"fresh", "leaf"}}}};

INSTANTIATE_TEST_SUITE_P(WorkspaceChanges, ImportGraphHistoryTest, testing::ValuesIn(REVISIONS),
                         [](const auto &info) { return info.param.name; });

TEST_F(ImportGraphEditsTest, RemovingTheMiddleFilePreservesReachabilityForEveryThreeFileGraph)
{
    ASSERT_NO_FATAL_FAILURE(Start());
    const std::string middleFile = "middle.ats";
    const std::array<std::string, 3> files {"entry.ats", middleFile, "leaf.ats"};
    const std::set<std::string> known(files.begin(), files.end());
    const std::unordered_set<std::string> allFiles(files.begin(), files.end());
    std::vector<Edge> possibleEdges;
    for (const auto &file : files) {
        for (const auto &dependency : files) {
            possibleEdges.emplace_back(file, dependency);
        }
    }
    // Nine possible directed edges, including self imports. Each bit mask is one workspace.
    constexpr unsigned graphCount = 1U << (files.size() * files.size());
    for (unsigned mask = 0; mask < graphCount; ++mask) {
        SCOPED_TRACE(mask);
        Graph expected;
        unsigned bit = 1U;
        for (const auto &[file, dependency] : possibleEdges) {
            if ((mask & bit) != 0) {
                Manager()->AddFileDependencies(file, dependency);
                expected[file].insert(dependency);
            }
            bit <<= 1U;
        }
        ExpectGraph(expected, known);
        Manager()->RemoveProgramsFromFileDependencies({middleFile});
        expected.erase(middleFile);
        for (auto &[file, imports] : expected) {
            imports.erase(middleFile);
        }
        ExpectGraph(expected, known);
        Manager()->RemoveProgramsFromFileDependencies(allFiles);
        if (testing::Test::HasFailure()) {
            return;
        }
    }
}

}  // namespace
}  // namespace ark::es2panda::util::test
