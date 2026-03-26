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

#ifndef LSP_MEMORY_MANAGER_H
#define LSP_MEMORY_MANAGER_H

#include <benchmark/benchmark.h>
#include <cstddef>
#include <string>

constexpr size_t KILOBYTE = 1024;

#ifdef CASES_DIR
constexpr const char *K_CONFIGURED_CASES_DIR = CASES_DIR;
const std::string &ResolveCasesDir();

#undef CASES_DIR
#define CASES_DIR (ResolveCasesDir().c_str())
#endif

void ConfigureBuildFolderForBenchmarks();

class LspBenchmarkMemoryCountersScope {
public:
    explicit LspBenchmarkMemoryCountersScope(benchmark::State &state);
    ~LspBenchmarkMemoryCountersScope();

private:
    benchmark::State &state_;
    size_t start_current_rss_;
    size_t start_peak_rss_;
};

class LspMemoryManager : public benchmark::MemoryManager {
public:
    LspMemoryManager();

    void Start() override;

    void Stop(Result &result) override;

private:
    size_t start_current_rss_;
    size_t end_current_rss_;
    size_t start_peak_rss_;
    size_t end_peak_rss_;
};

#endif  // LSP_MEMORY_MANAGER_H
