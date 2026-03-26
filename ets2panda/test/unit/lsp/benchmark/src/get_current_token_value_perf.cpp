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

struct CurrentTokenValueTestCase {
    const char *relativeFileName;
    size_t offset;
    const char *anchorText;
    size_t anchorOffset;
    const char *expectedToken;
};

static const CurrentTokenValueTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", 0, "BENCH_CHAIN_SYMBOL_L1", 2, "BE"},
    {"project/bench_l2.ets", 0, "BENCH_CHAIN_SYMBOL_L2", 2, "BE"},
    {"project/bench_l3.ets", 0, "BENCH_CHAIN_SYMBOL_L3", 2, "BE"},
    {"project/bench_l4.ets", 0, "BENCH_CHAIN_SYMBOL_L4", 2, "BE"},
    {"project/entry.ets", 0, "targetAdminValue", 2, "ta"},
    {"project/shared_small_100.ets", 0, "SMALL_SHARED_ANCHOR", 2, "SM"},
    {"project/shared_large_1000.ets", 0, "LARGE_SHARED_ANCHOR", 2, "LA"},
    {"project/domain.ets", 0, "FeatureAdmin", 2, "Fe"},
};

static void BM_GetCurrentTokenValue(benchmark::State &state)
{
    LspBenchmarkMemoryCountersScope memCountersScope(state);
    const auto &testCase = K_TEST_CASES[state.range(0)];
    const std::string filePath = MakeCasePath(testCase.relativeFileName);
    const std::string source = ReadCaseFile(filePath);
    if (source.empty()) {
        state.SkipWithError("Read case file failed");
        return;
    }
    size_t position = testCase.offset;
    if (testCase.anchorText[0] != '\0') {
        const size_t anchorPos = source.find(testCase.anchorText);
        if (anchorPos == std::string::npos) {
            state.SkipWithError("Anchor text not found");
            return;
        }
        position = anchorPos + testCase.anchorOffset;
    }

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    if (ctx == nullptr) {
        state.SkipWithError("CreateContext failed");
        return;
    }

    LSPAPI const *lspApi = GetImpl();
    const std::string checkResult = lspApi->getCurrentTokenValue(ctx, position);
    if (checkResult != testCase.expectedToken) {
        state.SkipWithError(("Current token value mismatch, expected=" + std::string(testCase.expectedToken) +
                             ", actual=" + checkResult)
                                .c_str());
        initializer.DestroyContext(ctx);
        return;
    }

    for (auto _ : state) {
        lspApi->getCurrentTokenValue(ctx, position);
    }

    initializer.DestroyContext(ctx);
}

BENCHMARK(BM_GetCurrentTokenValue)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
