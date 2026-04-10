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
#include "include/lsp_benchmark_utils.h"
#include "lsp/include/internal_api.h"
#include "include/lspMemoryManager.h"
#include <fstream>
#include <sstream>

#ifndef CASES_DIR
#define CASES_DIR "./cases"
#endif

using ark::es2panda::lsp::Initializer;

struct OffsetTestCase {
    std::string fileName;
    int line;
    int col;
};

static const OffsetTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", 0, 0},           {"project/bench_l2.ets", 2, 4}, {"project/bench_l3.ets", 4, 4},
    {"project/bench_l4.ets", 6, 4},           {"project/entry.ets", 0, 0},    {"project/shared_small_100.ets", 10, 4},
    {"project/shared_large_1000.ets", 20, 4},
};

static void BM_GetOffsetByColAndLine(benchmark::State &state)
{
    LspBenchmarkMemoryCountersScope memCountersScope(state);
    const auto &testCase = K_TEST_CASES[state.range(0)];
    std::string filePath = MakeCasePath(testCase.fileName);

    std::ifstream in(filePath);
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string source = ss.str();

    LSPAPI const *lspApi = GetImpl();

    int offset = lspApi->getOffsetByColAndLine(source, testCase.line, testCase.col);
    if (offset < 0 || static_cast<size_t>(offset) > source.size()) {
        state.SkipWithError("getOffsetByColAndLine result out of source range");
        return;
    }

    for (auto _ : state) {
        lspApi->getOffsetByColAndLine(source, testCase.line, testCase.col);
    }
}

BENCHMARK(BM_GetOffsetByColAndLine)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
