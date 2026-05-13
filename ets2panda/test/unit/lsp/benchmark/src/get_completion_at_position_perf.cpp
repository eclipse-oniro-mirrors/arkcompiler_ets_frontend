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
#include <algorithm>

#ifndef CASES_DIR
#define CASES_DIR "./cases"
#endif

using ark::es2panda::lsp::CompletionEntry;
using ark::es2panda::lsp::CompletionInfo;
using ark::es2panda::lsp::Initializer;

struct CompletionTestCase {
    std::string fileName;
    size_t offset;
    std::string anchorText;
    size_t anchorOffset;
    size_t expectMinSize;
    std::vector<std::string> expectNames;
};

bool CheckCompletionResult(CompletionInfo &completionResult, const CompletionTestCase &testCase,
                           benchmark::State &state)
{
    auto &entries = completionResult.GetEntries();
    if (entries.size() < testCase.expectMinSize) {
        state.SkipWithError(("Completion result size is less than expected minimum, expected_min=" +
                             std::to_string(testCase.expectMinSize) + ", actual=" + std::to_string(entries.size()))
                                .c_str());
        return false;
    }
    for (const auto &expectName : testCase.expectNames) {
        const bool found = std::any_of(entries.begin(), entries.end(), [&expectName](const CompletionEntry &entry) {
            const auto &name = entry.GetName();
            return name == expectName;
        });
        if (!found) {
            state.SkipWithError(("Expected completion name not found: " + expectName).c_str());
            return false;
        }
    }
    return true;
}

static const CompletionTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", 0, "benchObjL1.", 10, 0, {}},
    {"project/bench_l2.ets", 0, "benchObjL2.", 10, 0, {}},
    {"project/bench_l3.ets", 0, "benchObjL3.", 10, 0, {}},
    {"project/bench_l4.ets", 0, "benchObjL4.", 10, 0, {}},
    {"project/entry.ets", 0, "targetAdminValue.", 17, 5, {}},
    {"project/shared_small_100.ets", 0, "scenario.", 9, 6, {}},
    {"project/shared_large_1000.ets", 0, "runner.", 7, 7, {}},
    {"project/services.ets", 0, "registry.", 9, 3, {}},
};

static void BM_GetCompletionAtPosition(benchmark::State &state)
{
    LspBenchmarkMemoryCountersScope memCountersScope(state);
    const auto &testCase = K_TEST_CASES[state.range(0)];
    std::string filePath = MakeCasePath(testCase.fileName);
    size_t position = testCase.offset;
    if (!testCase.anchorText.empty()) {
        const std::string fileContent = ReadCaseFile(filePath);
        if (fileContent.empty()) {
            state.SkipWithError("Read case file failed");
            return;
        }
        const size_t anchorPos = fileContent.find(testCase.anchorText);
        if (anchorPos == std::string::npos) {
            state.SkipWithError("Anchor text not found");
            return;
        }
        position = anchorPos + testCase.anchorOffset;
    }

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
    if (ctx == nullptr) {
        state.SkipWithError("CreateContext failed");
        return;
    }
    LSPAPI const *lspApi = GetImpl();

    auto checkResult = lspApi->getCompletionsAtPosition(ctx, position);
    if (!CheckCompletionResult(checkResult, testCase, state)) {
        initializer.DestroyContext(ctx);
        return;
    }

    for (auto _ : state) {
        lspApi->getCompletionsAtPosition(ctx, position);
    }

    initializer.DestroyContext(ctx);
}

BENCHMARK(BM_GetCompletionAtPosition)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
