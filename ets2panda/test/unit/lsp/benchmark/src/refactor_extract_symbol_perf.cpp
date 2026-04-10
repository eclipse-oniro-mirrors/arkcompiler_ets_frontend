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
#include "include/lspMemoryManager.h"
#include "include/lsp_benchmark_utils.h"
#include "lsp/include/refactors/extract_symbol.h"
#include "lsp/include/refactors/refactor_types.h"

#ifndef CASES_DIR
#define CASES_DIR "./cases"
#endif

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::RefactorContext;

struct ExtractSymbolTestCase {
    std::string fileName;
    size_t start;
    size_t end;
    std::string anchorText;
    size_t anchorOffset;
    size_t selectionLength;
    std::string refactorName;
    std::string actionName;
    std::string expectedFileName;
};

static RefactorContext *CreateExtractContext(Initializer *initializer, const std::string &filePath, size_t start,
                                             size_t end)
{
    auto ctx = initializer->CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);

    ark::es2panda::lsp::UserPreferences prefs = ark::es2panda::lsp::UserPreferences::GetDefaultUserPreferences();
    ark::es2panda::lsp::FormatCodeSettings settings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
    ark::es2panda::lsp::FormatContext fmt = ark::es2panda::lsp::GetFormatContext(settings);
    ::LanguageServiceHost host;
    auto *textChangesContext = new ::TextChangesContext {host, fmt, prefs};

    auto *refactorContext = new RefactorContext;
    refactorContext->context = ctx;
    refactorContext->textChangesContext = textChangesContext;
    refactorContext->span.pos = start;
    refactorContext->span.end = end;

    return refactorContext;
}

std::string ApplyEdits(const std::string &original, const std::vector<::TextChange> &edits)
{
    if (edits.empty()) {
        return original;
    }

    std::vector<const TextChange *> ordered;
    ordered.reserve(edits.size());
    for (const auto &change : edits) {
        ordered.push_back(&change);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const TextChange *lhs, const TextChange *rhs) { return lhs->span.start < rhs->span.start; });

    std::string result;
    result.reserve(original.size());
    size_t cursor = 0;
    for (const auto *change : ordered) {
        size_t start = std::min(change->span.start, original.size());
        if (start < cursor) {
            start = cursor;
        }
        size_t end = std::min(start + change->span.length, static_cast<size_t>(original.size()));
        if (cursor < start) {
            result.append(original, cursor, start - cursor);
        }
        result.append(change->newText);
        cursor = end;
    }

    if (cursor < original.size()) {
        result.append(original, cursor, original.size() - cursor);
    }
    return result;
}

bool ValidateExtractResult(const std::string &original, const std::string &expected,
                           const ark::es2panda::lsp::RefactorEditInfo *res, benchmark::State &state)
{
    if (res == nullptr) {
        state.SkipWithError("Extract result is null");
        return false;
    }
    if (res->GetFileTextChanges().empty()) {
        state.SkipWithError("Extract result has no file changes");
        return false;
    }
    const auto &fileEdit = res->GetFileTextChanges().front();
    const auto &textChanges = fileEdit.textChanges;

    std::string result = ApplyEdits(original, textChanges);
    if (result == original) {
        state.SkipWithError("Extract result does not change original content");
        return false;
    }

    if (!expected.empty() && result != expected && result.find("function") == std::string::npos) {
        state.SkipWithError("Extract result content mismatch");
        return false;
    }
    return true;
}

static const ExtractSymbolTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", 0, 0, "BENCH_CHAIN_SYMBOL_L1 + \"-a\"", 0, 28,
     std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME),
     std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name), ""},
    {"project/bench_l2.ets", 0, 0, "BENCH_CHAIN_SYMBOL_L2 + \"-a\"", 0, 28,
     std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME),
     std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name), ""},
    {"project/bench_l3.ets", 0, 0, "BENCH_CHAIN_SYMBOL_L3 + \"-0\"", 0, 28,
     std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME),
     std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name), ""},
    {"project/bench_l4.ets", 0, 0, "BENCH_CHAIN_SYMBOL_L4 + \"-0\"", 0, 28,
     std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME),
     std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name), ""},
    {"project/entry.ets", 0, 0, "targetAdminValue.featureTag + \":\" + shadow", 0, 43,
     std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME),
     std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name), ""},
    {"project/entry.ets", 0, 0,
     "lookupValue + \"|\" + otherAdmin.traceId + \"|\" + name + \"|\" + symbols.size().toString()", 0, 85,
     std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME),
     std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name), ""},
    {"project/shared_small_100.ets", 0, 0, "scenario.summary() + \"|\" + refs + \"|\" + quick", 0, 45,
     std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME),
     std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name), ""},
    {"project/shared_large_1000.ets", 0, 0,
     "AnchorUse0() + \"|\" + AnchorUse1() + \"|\" + AnchorUse2() + \"|\" + AnchorUse3()", 0, 75,
     std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME),
     std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name), ""},
};

static void BM_ExtractSymbolRefactor(benchmark::State &state)
{
    LspBenchmarkMemoryCountersScope memCountersScope(state);
    const auto &testCase = K_TEST_CASES[state.range(0)];
    std::string filePath = MakeCasePath(testCase.fileName);

    std::ifstream inFile(filePath);
    std::string original((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    std::string expected;
    if (!testCase.expectedFileName.empty()) {
        std::string expectedFilePath = MakeCasePath(testCase.expectedFileName);
        std::ifstream expectedFile(expectedFilePath);
        expected.assign(std::istreambuf_iterator<char>(expectedFile), std::istreambuf_iterator<char>());
        expectedFile.close();
    }

    size_t start = testCase.start;
    size_t end = testCase.end;
    if (!testCase.anchorText.empty()) {
        size_t anchorPos = original.find(testCase.anchorText);
        if (anchorPos == std::string::npos) {
            state.SkipWithError("Anchor text not found");
            return;
        }
        start = anchorPos + testCase.anchorOffset;
        end = start + testCase.selectionLength;
    }
    if (end <= start || end > original.size()) {
        state.SkipWithError("Invalid refactor selection range");
        return;
    }

    Initializer initializer;
    auto *refactorContext = CreateExtractContext(&initializer, filePath, start, end);
    if (refactorContext == nullptr) {
        state.SkipWithError("CreateExtractContext failed");
        return;
    }

    LSPAPI const *lspApi = GetImpl();
    auto res =
        lspApi->getEditsForRefactor(*refactorContext, testCase.refactorName.c_str(), testCase.actionName.c_str());
    if (!ValidateExtractResult(original, expected, res.get(), state)) {
        initializer.DestroyContext(refactorContext->context);
        return;
    }

    for (auto _ : state) {
        lspApi->getEditsForRefactor(*refactorContext, testCase.refactorName.c_str(), testCase.actionName.c_str());
    }

    initializer.DestroyContext(refactorContext->context);
}

BENCHMARK(BM_ExtractSymbolRefactor)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
