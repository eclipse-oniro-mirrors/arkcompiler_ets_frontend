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

using ::DocumentHighlights;
using ark::es2panda::lsp::Initializer;

struct DocumentHighlightsTestCase {
    std::string fileName;
    size_t offset;
    std::string anchorText;
    size_t anchorOffset;
    size_t expectMinSpanCount;
};

bool CheckDocumentHighlightsResult(const DocumentHighlights &docHighlight, size_t expectMinSpanCount,
                                   benchmark::State &state)
{
    if (docHighlight.highlightSpans_.size() < expectMinSpanCount) {
        state.SkipWithError("Highlight span count is less than expected");
        return false;
    }
    for (const auto &span : docHighlight.highlightSpans_) {
        if (span.textSpan_.length == 0) {
            state.SkipWithError("Highlight span length is zero");
            return false;
        }
    }
    return true;
}

static const DocumentHighlightsTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", 0, "BENCH_CHAIN_SYMBOL_L1", 2, 1},
    {"project/bench_l2.ets", 0, "BENCH_CHAIN_SYMBOL_L2", 2, 1},
    {"project/bench_l3.ets", 0, "BENCH_CHAIN_SYMBOL_L3", 2, 1},
    {"project/bench_l4.ets", 0, "BENCH_CHAIN_SYMBOL_L4", 2, 1},
    {"project/entry.ets", 0, "targetAdminValue", 2, 1},
    {"project/shared_small_100.ets", 0, "SMALL_SHARED_ANCHOR", 2, 1},
    {"project/shared_large_1000.ets", 0, "LARGE_SHARED_ANCHOR", 2, 1},
};

static void BM_GetDocumentHighlights(benchmark::State &state)
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
        const std::string fileContent = ReadCaseFile(filePath);
        if (fileContent.empty()) {
            state.SkipWithError("Read case file failed");
            initializer.DestroyContext(ctx);
            return;
        }
        const size_t anchorPos = fileContent.find(testCase.anchorText);
        if (anchorPos == std::string::npos) {
            state.SkipWithError("Anchor text not found");
            initializer.DestroyContext(ctx);
            return;
        }
        queryOffset = anchorPos + testCase.anchorOffset;
    }

    auto result = lspApi->getDocumentHighlights(ctx, queryOffset);
    if (result.documentHighlights_.empty()) {
        state.SkipWithError("No highlights found");
        initializer.DestroyContext(ctx);
        return;
    }
    if (!CheckDocumentHighlightsResult(result.documentHighlights_[0], testCase.expectMinSpanCount, state)) {
        initializer.DestroyContext(ctx);
        return;
    }

    for (auto _ : state) {
        lspApi->getDocumentHighlights(ctx, queryOffset);
    }

    initializer.DestroyContext(ctx);
}

BENCHMARK(BM_GetDocumentHighlights)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
