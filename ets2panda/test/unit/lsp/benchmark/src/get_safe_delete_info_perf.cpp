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

struct SafeDeleteInfoTestCase {
    const char *relativeFileName;
    const char *token;
    size_t tokenOccurrence;
    size_t offsetInToken;
};

static const SafeDeleteInfoTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", "BENCH_CHAIN_SYMBOL_L1", 0, 2},
    {"project/bench_l2.ets", "BENCH_CHAIN_SYMBOL_L2", 0, 2},
    {"project/bench_l3.ets", "BENCH_CHAIN_SYMBOL_L3", 0, 2},
    {"project/bench_l4.ets", "BENCH_CHAIN_SYMBOL_L4", 0, 2},
    {"project/entry.ets", "targetAdminValue", 0, 2},
    {"project/shared_small_100.ets", "SMALL_SHARED_ANCHOR", 0, 2},
    {"project/shared_large_1000.ets", "LARGE_SHARED_ANCHOR", 0, 2},
};

static size_t FindNthOccurrence(const std::string &source, const std::string &token, size_t occurrence)
{
    size_t start = 0;
    size_t pos = std::string::npos;
    for (size_t i = 0; i <= occurrence; i++) {
        pos = source.find(token, start);
        if (pos == std::string::npos) {
            return pos;
        }
        start = pos + token.size();
    }
    return pos;
}

static void BM_GetSafeDeleteInfo(benchmark::State &state)
{
    LspBenchmarkMemoryCountersScope memCountersScope(state);
    const auto &testCase = K_TEST_CASES[state.range(0)];
    const std::string filePath = MakeCasePath(testCase.relativeFileName);
    const std::string source = ReadCaseFile(filePath);
    if (source.empty()) {
        state.SkipWithError("Read case file failed");
        return;
    }

    const size_t tokenPos = FindNthOccurrence(source, testCase.token, testCase.tokenOccurrence);
    if (tokenPos == std::string::npos) {
        state.SkipWithError("Cannot find token in test source");
        return;
    }

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    if (ctx == nullptr) {
        state.SkipWithError("CreateContext failed");
        return;
    }

    LSPAPI const *lspApi = GetImpl();
    const size_t offset = tokenPos + testCase.offsetInToken;
    (void)lspApi->getSafeDeleteInfo(ctx, offset);

    for (auto _ : state) {
        lspApi->getSafeDeleteInfo(ctx, offset);
    }

    initializer.DestroyContext(ctx);
}

BENCHMARK(BM_GetSafeDeleteInfo)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
