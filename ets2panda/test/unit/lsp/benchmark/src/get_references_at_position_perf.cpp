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

#include <benchmark/benchmark.h>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include "lsp/include/internal_api.h"
#include "include/lsp_benchmark_utils.h"
#include "include/lspMemoryManager.h"

#ifndef CASES_DIR
#define CASES_DIR "./cases"
#endif

struct ReferenceInfoExpect {
    std::string fileName;
    int start;
    int length;
};

struct ReferenceTestCase {
    std::vector<std::string> fileNames;
    std::vector<std::string> fileContents;
    std::string mainFile;
    std::string anchorText;
    size_t anchorOffset;
    size_t expectedMinCount;
};

using ReferenceList = std::vector<ReferenceInfoExpect>;

static const ReferenceTestCase K_TEST_CASES[] = {
    {{"project/bench_l1.ets"}, {}, "project/bench_l1.ets", "BenchRunnerL1Impl", 2, 2},
    {{"project/bench_l1.ets", "project/bench_l2.ets"}, {}, "project/bench_l2.ets", "BenchRunnerL2Impl", 2, 2},
    {{"project/bench_l1.ets", "project/bench_l2.ets", "project/bench_l3.ets"},
     {},
     "project/bench_l3.ets",
     "BenchRunnerL3Impl",
     2,
     2},
    {{"project/bench_l1.ets", "project/bench_l2.ets", "project/bench_l3.ets", "project/bench_l4.ets"},
     {},
     "project/bench_l4.ets",
     "BenchRunnerL4Impl",
     2,
     2},
    {{"project/domain.ets"}, {}, "project/domain.ets", "FeatureAdmin", 2, 3}};

static ReferenceList NormalizeReferences(ReferenceList refs)
{
    const auto lessByLocation = [](const ReferenceInfoExpect &lhs, const ReferenceInfoExpect &rhs) {
        if (lhs.fileName != rhs.fileName) {
            return lhs.fileName < rhs.fileName;
        }
        if (lhs.start != rhs.start) {
            return lhs.start < rhs.start;
        }
        return lhs.length < rhs.length;
    };
    const auto sameLocation = [](const ReferenceInfoExpect &lhs, const ReferenceInfoExpect &rhs) {
        return lhs.fileName == rhs.fileName && lhs.start == rhs.start && lhs.length == rhs.length;
    };

    std::sort(refs.begin(), refs.end(), lessByLocation);
    refs.erase(std::unique(refs.begin(), refs.end(), sameLocation), refs.end());
    return refs;
}

bool CheckReferencesResult(const ReferenceTestCase &testCase, const std::vector<ReferenceInfoExpect> &result,
                           const std::map<std::string, std::string> &fileContentMap, benchmark::State &state)
{
    (void)fileContentMap;
    const auto normalizedResult = NormalizeReferences(result);
    if (normalizedResult.size() < testCase.expectedMinCount) {
        state.SkipWithError("References result count is less than expected minimum");
        return false;
    }
    return true;
}

std::map<std::string, std::string> LoadAllCaseFiles()
{
    std::map<std::string, std::string> fileContentMap;
    for (const auto &testCase : K_TEST_CASES) {
        for (const auto &fileName : testCase.fileNames) {
            std::string absPath = MakeCasePath(fileName);
            if (fileContentMap.find(absPath) == fileContentMap.end()) {
                fileContentMap[absPath] = ReadCaseFile(absPath);
            }
        }
        std::string mainFilePath = MakeCasePath(testCase.mainFile);
        if (fileContentMap.find(mainFilePath) == fileContentMap.end()) {
            fileContentMap[mainFilePath] = ReadCaseFile(mainFilePath);
        }
    }
    return fileContentMap;
}

static void BM_GetReferencesAtPosition(benchmark::State &state)
{
    LspBenchmarkMemoryCountersScope memCountersScope(state);
    static const std::map<std::string, std::string> fileContentMap = LoadAllCaseFiles();
    const auto &testCase = K_TEST_CASES[state.range(0)];

    std::vector<ark::es2panda::SourceFile> sourceFiles;
    for (const auto &fileName : testCase.fileNames) {
        std::string absPath = MakeCasePath(fileName);
        auto it = fileContentMap.find(absPath);
        std::string content = (it != fileContentMap.end()) ? it->second : "";
        sourceFiles.emplace_back(absPath, content);
    }

    std::string mainFilePath = MakeCasePath(testCase.mainFile);
    auto it = fileContentMap.find(mainFilePath);
    std::string mainFileContent = (it != fileContentMap.end()) ? it->second : "";
    size_t anchorPos = mainFileContent.find(testCase.anchorText);
    if (anchorPos == std::string::npos) {
        state.SkipWithError("Anchor text not found");
        return;
    }
    size_t offset = anchorPos + testCase.anchorOffset;
    ark::es2panda::SourceFile mainSrcFile(mainFilePath, mainFileContent);

    ark::es2panda::lsp::CancellationToken token;
    LSPAPI const *lspApi = GetImpl();
    auto referencedNodes = lspApi->findReferences(&token, sourceFiles, mainSrcFile, offset);

    std::vector<ReferenceInfoExpect> result;
    for (const auto &node : referencedNodes) {
        if (node.filePath.empty() || node.end <= node.start) {
            continue;
        }
        std::string baseName = node.filePath.substr(node.filePath.find_last_of("/\\") + 1);
        if (baseName.empty()) {
            continue;
        }
        result.push_back({baseName, static_cast<int>(node.start), static_cast<int>(node.end - node.start)});
    }

    if (!CheckReferencesResult(testCase, result, fileContentMap, state)) {
        return;
    }

    for (auto _ : state) {
        lspApi->findReferences(&token, sourceFiles, mainSrcFile, offset);
    }
}

BENCHMARK(BM_GetReferencesAtPosition)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
