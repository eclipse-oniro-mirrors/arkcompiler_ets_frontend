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
#include <cstddef>
#include "lsp/include/internal_api.h"
#include "include/lsp_benchmark_utils.h"
#include "include/lspMemoryManager.h"

#ifndef CASES_DIR
#define CASES_DIR "./cases"
#endif

using ark::es2panda::lsp::Initializer;

struct SyntacticDiagnosticsTestCase {
    std::string fileName;
    size_t minDiagnosticCount;
    size_t maxDiagnosticCount;
};

static const SyntacticDiagnosticsTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", 0, 0},          {"project/bench_l2.ets", 0, 0}, {"project/bench_l3.ets", 0, 0},
    {"project/bench_l4.ets", 0, 0},          {"project/entry.ets", 0, 0},    {"project/shared_small_100.ets", 0, 0},
    {"project/shared_large_1000.ets", 0, 0},
};

static void BM_GetSyntacticDiagnostics(benchmark::State &state)
{
    LspBenchmarkMemoryCountersScope memCountersScope(state);
    const auto &testCase = K_TEST_CASES[state.range(0)];
    std::string filePath = MakeCasePath(testCase.fileName);

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
    if (ctx == nullptr) {
        state.SkipWithError("CreateContext failed");
        return;
    }

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSyntacticDiagnostics(ctx);
    const size_t count = result.diagnostic.size();
    if (count < testCase.minDiagnosticCount || count > testCase.maxDiagnosticCount) {
        state.SkipWithError("Syntactic diagnostics count mismatch");
        initializer.DestroyContext(ctx);
        return;
    }

    for (auto _ : state) {
        lspApi->getSyntacticDiagnostics(ctx);
    }

    initializer.DestroyContext(ctx);
}

BENCHMARK(BM_GetSyntacticDiagnostics)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
