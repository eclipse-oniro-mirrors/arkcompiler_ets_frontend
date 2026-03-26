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

static std::string GetBaseName(const std::string &path)
{
    const size_t pos = path.find_last_of("/\\");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

struct DefinitionTestCase {
    std::string fileName;
    size_t offset;
    std::string anchorText;
    size_t anchorOffset;
    std::string expectFileName;
    std::string expectStartAnchorText;
    size_t expectStartAnchorOffset;
    size_t expectStart;
    size_t expectLength;
};

static bool MatchOptionalString(const std::string &expected, const std::string &actual)
{
    return expected.empty() || expected == actual;
}

static bool MatchOptionalSize(size_t expected, size_t actual)
{
    return expected == 0 || expected == actual;
}

bool CheckDefinitionResult(const DefinitionInfo *result, const DefinitionTestCase &testCase, size_t expectedStart)
{
    if (result == nullptr) {
        return testCase.expectFileName.empty();
    }

    const std::string actualFile = GetBaseName(result->fileName);
    const std::string expectedFile = GetBaseName(testCase.expectFileName);
    if (!MatchOptionalString(expectedFile, actualFile)) {
        return false;
    }

    if (result->length == 0) {
        return false;
    }
    if (!MatchOptionalSize(testCase.expectLength, result->length)) {
        return false;
    }
    if (!testCase.expectStartAnchorText.empty() && result->start != expectedStart) {
        return false;
    }
    return true;
}

static const DefinitionTestCase K_TEST_CASES[] = {
    {"project/bench_l1.ets", 0, "BenchRunnerL1Impl", 2, "project/bench_l1.ets", "export class BenchRunnerL1Impl", 13, 0,
     0},
    {"project/bench_l2.ets", 0, "L1MakeToken()", 2, "project/bench_l1.ets", "export function L1MakeToken", 16, 0, 0},
    {"project/bench_l3.ets", 0, "L2MakeToken()", 2, "project/bench_l2.ets", "export function L2MakeToken", 16, 0, 0},
    {"project/bench_l4.ets", 0, "L3MakeToken()", 2, "project/bench_l3.ets", "export function L3MakeToken", 16, 0, 0},
    {"project/entry.ets", 0, "createAdmin(\"Mika\")", 2, "project/services.ets", "export function createAdmin", 16, 0,
     11},
    {"project/shared_small_100.ets", 0, "BuildStableKey(", 2, "project/helpers.ets", "export function BuildStableKey",
     16, 0, 14},
    {"project/shared_large_1000.ets", 0, "CreateSymbolRegistry(", 2, "project/registry.ets",
     "export function CreateSymbolRegistry", 16, 0, 20},
};

static bool ResolveQueryPosition(const DefinitionTestCase &testCase, const std::string &filePath, size_t &position,
                                 benchmark::State &state)
{
    position = testCase.offset;
    if (testCase.anchorText.empty()) {
        return true;
    }

    const std::string fileContent = ReadCaseFile(filePath);
    if (fileContent.empty()) {
        state.SkipWithError("Read case file failed");
        return false;
    }
    const size_t anchorPos = fileContent.find(testCase.anchorText);
    if (anchorPos == std::string::npos) {
        state.SkipWithError("Anchor text not found");
        return false;
    }
    position = anchorPos + testCase.anchorOffset;
    return true;
}

static bool ResolveExpectedStart(const DefinitionTestCase &testCase, size_t &expectedStart, benchmark::State &state)
{
    expectedStart = testCase.expectStart;
    if (testCase.expectStartAnchorText.empty()) {
        return true;
    }

    const std::string expectedFilePath = MakeCasePath(testCase.expectFileName);
    const std::string expectedFileContent = ReadCaseFile(expectedFilePath);
    if (expectedFileContent.empty()) {
        state.SkipWithError("Read expected case file failed");
        return false;
    }
    const size_t expectAnchorPos = expectedFileContent.find(testCase.expectStartAnchorText);
    if (expectAnchorPos == std::string::npos) {
        state.SkipWithError("Expected anchor text not found");
        return false;
    }
    expectedStart = expectAnchorPos + testCase.expectStartAnchorOffset;
    return true;
}

static void BM_GetDefinitionAtPosition(benchmark::State &state)
{
    LspBenchmarkMemoryCountersScope memCountersScope(state);
    const auto &testCase = K_TEST_CASES[state.range(0)];
    std::string filePath = MakeCasePath(testCase.fileName);
    size_t position = 0;
    if (!ResolveQueryPosition(testCase, filePath, position, state)) {
        return;
    }

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
    if (ctx == nullptr) {
        state.SkipWithError("CreateContext failed");
        return;
    }
    LSPAPI const *lspApi = GetImpl();

    size_t expectedStart = 0;
    if (!ResolveExpectedStart(testCase, expectedStart, state)) {
        initializer.DestroyContext(ctx);
        return;
    }

    auto checkResult = lspApi->getDefinitionAtPosition(ctx, position);
    if (!CheckDefinitionResult(&checkResult, testCase, expectedStart)) {
        state.SkipWithError("Result does not match expected value");
        initializer.DestroyContext(ctx);
        return;
    }
    for (auto _ : state) {
        lspApi->getDefinitionAtPosition(ctx, position);
    }
    initializer.DestroyContext(ctx);
}

BENCHMARK(BM_GetDefinitionAtPosition)->DenseRange(0, LastCaseIndex(K_TEST_CASES));

int main(int argc, char **argv)
{
    LspMemoryManager memMgr;
    benchmark::RegisterMemoryManager(&memMgr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
