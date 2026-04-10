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
#include "lsp/include/internal_api.h"
#include "include/lsp_benchmark_utils.h"
#include "include/lspMemoryManager.h"

#ifndef CASES_DIR
#define CASES_DIR "./cases"
#endif

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::RenameLocation;

struct FindRenameLocationsTestCase {
    std::string fileName;
    std::string anchorText;
    size_t anchorOffset;
    size_t expectedMinCount;
};

template <class T>
bool CheckRenameLocationsResult(const T &result, const FindRenameLocationsTestCase &testCase)
{
    if (result.size() < testCase.expectedMinCount) {
        return false;
    }

    for (const auto &loc : result) {
        if (loc.fileName.empty() || loc.end <= loc.start) {
            return false;
        }
    }

    return true;
}

static const FindRenameLocationsTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", "BenchRunnerL1Impl", 2, 2}, {"project/bench_l2.ets", "BenchRunnerL2Impl", 2, 2},
    {"project/bench_l3.ets", "BenchRunnerL3Impl", 2, 2}, {"project/bench_l4.ets", "BenchRunnerL4Impl", 2, 2},
    {"project/entry.ets", "BuildScenarioData", 2, 2},    {"project/domain.ets", "FindByName", 2, 1},
    {"project/entry.ets", "targetAdminValue", 2, 7},     {"project/domain.ets", "FeatureAdmin", 2, 4},
};

static void BM_FindRenameLocations(benchmark::State &state)
{
    LspBenchmarkMemoryCountersScope memCountersScope(state);
    const auto &testCase = K_TEST_CASES[state.range(0)];
    const std::string filePath = MakeCasePath(testCase.fileName);
    const std::string fileContent = ReadCaseFile(filePath);
    if (fileContent.empty()) {
        state.SkipWithError("Read case file failed");
        return;
    }
    size_t anchorPos = fileContent.find(testCase.anchorText);
    if (anchorPos == std::string::npos) {
        state.SkipWithError("Anchor text not found");
        return;
    }
    const size_t position = anchorPos + testCase.anchorOffset;

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
    if (ctx == nullptr) {
        state.SkipWithError("CreateContext failed");
        return;
    }
    LSPAPI const *lspApi = GetImpl();
    auto checkResult = lspApi->findRenameLocationsInCurrentFile(ctx, position);
    if (!CheckRenameLocationsResult(checkResult, testCase)) {
        state.SkipWithError("Result does not match expected value");
        initializer.DestroyContext(ctx);
        return;
    }

    for (auto _ : state) {
        lspApi->findRenameLocationsInCurrentFile(ctx, position);
    }
    initializer.DestroyContext(ctx);
}
BENCHMARK(BM_FindRenameLocations)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
