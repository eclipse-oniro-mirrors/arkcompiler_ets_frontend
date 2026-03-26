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

using ark::es2panda::lsp::FormatCodeSettings;
using ark::es2panda::lsp::Initializer;

std::string ApplyChanges(const std::string &original, const std::vector<TextChange> &changes)
{
    std::string result = original;
    auto sorted = changes;
    std::sort(sorted.begin(), sorted.end(),
              [](const TextChange &a, const TextChange &b) { return a.span.start > b.span.start; });
    for (const auto &change : sorted) {
        result.replace(change.span.start, change.span.length, change.newText);
    }
    return result;
}

struct FormattingTestCase {
    std::string fileName;
    size_t settingsIndex;
};

static const std::vector<ark::es2panda::lsp::FormatCodeSettings> K_SETTINGS_LIST = {
    [] {
        ark::es2panda::lsp::FormatCodeSettings settings;
        return settings;
    }(),
    [] {
        ark::es2panda::lsp::FormatCodeSettings settings;
        settings.SetPlaceOpenBraceOnNewLineForControlBlocks(false);
        return settings;
    }(),
    [] {
        ark::es2panda::lsp::FormatCodeSettings settings;
        settings.SetPlaceOpenBraceOnNewLineForFunctions(false);
        return settings;
    }(),
    [] {
        ark::es2panda::lsp::FormatCodeSettings settings;
        settings.SetInsertSpaceAfterConstructor(false);
        return settings;
    }(),
    [] {
        ark::es2panda::lsp::FormatCodeSettings settings;
        settings.SetInsertSpaceAfterCommaDelimiter(true);
        return settings;
    }(),
    [] {
        ark::es2panda::lsp::FormatCodeSettings settings;
        settings.SetInsertSpaceBeforeFunctionParenthesis(true);
        return settings;
    }(),
    [] {
        ark::es2panda::lsp::FormatCodeSettings settings;
        settings.SetInsertSpaceAfterKeywordsInControlFlowStatements(true);
        return settings;
    }(),
    [] {
        ark::es2panda::lsp::FormatCodeSettings settings;
        settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets(true);
        return settings;
    }(),
};

static const FormattingTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", 0}, {"project/bench_l2.ets", 0},         {"project/bench_l3.ets", 0},
    {"project/bench_l4.ets", 0}, {"project/formatting_cases.ets", 0},
};

bool CheckFormattingResult(const FormattingTestCase &testCase, const std::vector<TextChange> &changes,
                           benchmark::State &state)
{
    std::string original = ReadCaseFile(MakeCasePath(testCase.fileName));
    if (original.empty()) {
        state.SkipWithError("Original file not found");
        return false;
    }
    std::string result = ApplyChanges(original, changes);
    if (result.empty()) {
        state.SkipWithError("Formatting result is empty");
        return false;
    }
    return true;
}

static void BM_GetFormattingEdits(benchmark::State &state)
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
    auto settings = K_SETTINGS_LIST.at(testCase.settingsIndex);

    std::vector<TextChange> result = lspApi->getFormattingEditsForDocument(ctx, settings);

    if (!CheckFormattingResult(testCase, result, state)) {
        initializer.DestroyContext(ctx);
        return;
    }

    for (auto _ : state) {
        lspApi->getFormattingEditsForDocument(ctx, settings);
    }

    initializer.DestroyContext(ctx);
}

BENCHMARK(BM_GetFormattingEdits)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
