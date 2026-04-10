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

struct ToLineColumnOffsetTestCase {
    std::string fileName;
    std::string anchorText;
    size_t anchorOffset;
};

static const ToLineColumnOffsetTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", "BENCH_CHAIN_SYMBOL_L1", 2},
    {"project/bench_l2.ets", "BENCH_CHAIN_SYMBOL_L2", 2},
    {"project/bench_l3.ets", "BENCH_CHAIN_SYMBOL_L3", 2},
    {"project/bench_l4.ets", "BENCH_CHAIN_SYMBOL_L4", 2},
    {"project/entry.ets", "targetAdminValue", 2},
    {"project/shared_small_100.ets", "SMALL_SHARED_ANCHOR", 2},
    {"project/shared_large_1000.ets", "LARGE_SHARED_ANCHOR", 2},
};

static void BM_ToLineColumnOffset(benchmark::State &state)
{
    LspBenchmarkMemoryCountersScope memCountersScope(state);
    const auto &testCase = K_TEST_CASES[state.range(0)];
    std::string filePath = MakeCasePath(testCase.fileName);
    const std::string source = ReadCaseFile(filePath);
    if (source.empty()) {
        state.SkipWithError("Read case file failed");
        return;
    }
    const size_t anchorPos = source.find(testCase.anchorText);
    if (anchorPos == std::string::npos) {
        state.SkipWithError("Anchor text not found");
        return;
    }
    const size_t queryOffset = anchorPos + testCase.anchorOffset;

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
    if (ctx == nullptr) {
        state.SkipWithError("CreateContext failed");
        return;
    }
    LSPAPI const *lspApi = GetImpl();

    auto checkResult = lspApi->toLineColumnOffset(ctx, queryOffset);
    if (checkResult.GetLine() > source.size() || checkResult.GetCharacter() > source.size()) {
        state.SkipWithError("Line or character out of source range");
        initializer.DestroyContext(ctx);
        return;
    }

    for (auto _ : state) {
        lspApi->toLineColumnOffset(ctx, queryOffset);
    }

    initializer.DestroyContext(ctx);
}

BENCHMARK(BM_ToLineColumnOffset)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
