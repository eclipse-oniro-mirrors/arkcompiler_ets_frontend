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
#include <string>
#include "lsp/include/internal_api.h"
#include "include/lsp_benchmark_utils.h"
#include "include/lspMemoryManager.h"

#ifndef CASES_DIR
#define CASES_DIR "./cases"
#endif

using ark::es2panda::lsp::Initializer;

struct SemanticDiagnosticsTestCase {
    const char *relativeFileName;
    size_t expectedMinDiagnosticCount;
    size_t expectedMaxDiagnosticCount;
};

static const SemanticDiagnosticsTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", 0, 16},          {"project/bench_l2.ets", 0, 24}, {"project/bench_l3.ets", 0, 32},
    {"project/bench_l4.ets", 0, 48},          {"project/entry.ets", 0, 16},    {"project/shared_small_100.ets", 0, 16},
    {"project/shared_large_1000.ets", 0, 32},
};

static void BM_GetSemanticDiagnostics(benchmark::State &state)
{
    LspBenchmarkMemoryCountersScope memCountersScope(state);
    const auto &testCase = K_TEST_CASES[state.range(0)];
    const std::string filePath = MakeCasePath(testCase.relativeFileName);
    const std::string source = ReadCaseFile(filePath);
    if (source.empty()) {
        state.SkipWithError("Read case file failed");
        return;
    }

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    if (ctx == nullptr) {
        state.SkipWithError("CreateContext failed");
        return;
    }

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    const size_t diagCount = result.diagnostic.size();
    if (diagCount < testCase.expectedMinDiagnosticCount || diagCount > testCase.expectedMaxDiagnosticCount) {
        state.SkipWithError("Semantic diagnostics count out of expected range");
        initializer.DestroyContext(ctx);
        return;
    }

    for (auto _ : state) {
        lspApi->getSemanticDiagnostics(ctx);
    }

    initializer.DestroyContext(ctx);
}

BENCHMARK(BM_GetSemanticDiagnostics)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
