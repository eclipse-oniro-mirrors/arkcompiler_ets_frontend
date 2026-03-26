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

struct QuickInfoTestCase {
    std::string fileName;
    int offset;
    std::string anchorText;
    int anchorOffset;
    std::string expectKind;
    int expectSpanStart;
    int expectSpanLength;
};

static const QuickInfoTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", 0, "BenchRunnerL1Impl", 2, "class", 0, 0},
    {"project/bench_l2.ets", 0, "BenchRunnerL2Impl", 2, "class", 0, 0},
    {"project/bench_l3.ets", 0, "BenchRunnerL3Impl", 2, "class", 0, 0},
    {"project/bench_l4.ets", 0, "BenchRunnerL4Impl", 2, "class", 0, 0},
    {"project/domain.ets", 0, "FeatureAdmin", 2, "class", 0, 12},
    {"project/entry.ets", 0, "FeatureAdmin", 2, "class", 0, 12},
    {"project/shared_large_1000.ets", 0, "KeyResolver", 2, "class", 0, 11},
};

static void BM_GetQuickInfoAtPosition(benchmark::State &state)
{
    LspBenchmarkMemoryCountersScope memCountersScope(state);
    const auto &testCase = K_TEST_CASES[state.range(0)];
    std::string filePath = MakeCasePath(testCase.fileName);
    int position = testCase.offset;
    int expectSpanStart = testCase.expectSpanStart;
    if (!testCase.anchorText.empty()) {
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
        position = static_cast<int>(anchorPos) + testCase.anchorOffset;
        expectSpanStart = static_cast<int>(anchorPos);
    }

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
    if (ctx == nullptr) {
        state.SkipWithError("CreateContext failed");
        return;
    }
    LSPAPI const *lspApi = GetImpl();

    auto result = lspApi->getQuickInfoAtPosition(filePath.c_str(), ctx, position);
    if (result.GetKind() != testCase.expectKind) {
        state.SkipWithError("QuickInfo kind mismatch");
        initializer.DestroyContext(ctx);
        return;
    }
    auto span = result.GetTextSpan();
    if (testCase.expectSpanLength > 0 && (static_cast<int>(span.start) != expectSpanStart ||
                                          static_cast<int>(span.length) != testCase.expectSpanLength)) {
        state.SkipWithError("QuickInfo textSpan mismatch");
        initializer.DestroyContext(ctx);
        return;
    }

    for (auto _ : state) {
        lspApi->getQuickInfoAtPosition(filePath.c_str(), ctx, position);
    }

    initializer.DestroyContext(ctx);
}

BENCHMARK(BM_GetQuickInfoAtPosition)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
