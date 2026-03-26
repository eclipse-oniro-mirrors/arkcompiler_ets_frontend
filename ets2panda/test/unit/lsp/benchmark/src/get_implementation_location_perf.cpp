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

struct ImplementationLocationTestCase {
    std::string fileName;
    size_t offset;
    std::string anchorText;
    size_t anchorOffset;
};

static const ImplementationLocationTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", 0, "BenchRunnerL1", 2},         {"project/bench_l2.ets", 0, "BenchRunnerL2", 2},
    {"project/bench_l3.ets", 0, "BenchRunnerL3", 2},         {"project/bench_l4.ets", 0, "BenchRunnerL4", 2},
    {"project/shared_large_1000.ets", 0, "NodeResolver", 2}, {"project/shared_large_1000.ets", 0, "BaseResolver", 2},
};

static void BM_GetImplementationLocationAtPosition(benchmark::State &state)
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

    size_t queryOffset = testCase.offset;
    if (!testCase.anchorText.empty()) {
        std::string content = ReadCaseFile(filePath);
        if (content.empty()) {
            state.SkipWithError("Read case file failed");
            initializer.DestroyContext(ctx);
            return;
        }
        size_t anchorPos = content.find(testCase.anchorText);
        if (anchorPos == std::string::npos) {
            state.SkipWithError("Anchor text not found");
            initializer.DestroyContext(ctx);
            return;
        }
        queryOffset = anchorPos + testCase.anchorOffset;
    }

    auto result = lspApi->getImplementationLocationAtPosition(ctx, queryOffset);
    if (result.empty()) {
        state.SkipWithError("Implementation location result is empty");
        initializer.DestroyContext(ctx);
        return;
    }
    for (const auto &location : result) {
        if (location.range_.end.line_ < location.range_.start.line_) {
            state.SkipWithError("Implementation location range is invalid");
            initializer.DestroyContext(ctx);
            return;
        }
    }

    for (auto _ : state) {
        lspApi->getImplementationLocationAtPosition(ctx, queryOffset);
    }

    initializer.DestroyContext(ctx);
}

BENCHMARK(BM_GetImplementationLocationAtPosition)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
