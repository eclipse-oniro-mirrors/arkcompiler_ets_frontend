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

#include "include/lspMemoryManager.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#if !defined(_WIN32)
#include <sys/resource.h>
#endif

namespace {
size_t ParseStatusValueKb(std::string_view key)
{
#ifdef __linux__
    std::ifstream statusFile("/proc/self/status");
    if (!statusFile.is_open()) {
        return 0;
    }

    std::string line;
    while (std::getline(statusFile, line)) {
        if (line.rfind(std::string(key), 0) != 0) {
            continue;
        }

        std::istringstream iss(line);
        std::string name;
        size_t valueKb = 0;
        std::string unit;
        iss >> name >> valueKb >> unit;
        return valueKb * KILOBYTE;
    }
#endif
    return 0;
}

size_t GetPeakRSS()
{
#if defined(_WIN32)
    return 0;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
#ifdef __linux__
        return static_cast<size_t>(usage.ru_maxrss) * KILOBYTE;
#else
        return static_cast<size_t>(usage.ru_maxrss);
#endif
    }
    return 0;
#endif
}

size_t GetCurrentRSS()
{
    size_t rss = ParseStatusValueKb("VmRSS:");
    return rss > 0 ? rss : GetPeakRSS();
}
}  // namespace

#ifdef CASES_DIR
const std::string &ResolveCasesDir()
{
    static const std::string kCasesDir = []() {
        std::string configured = K_CONFIGURED_CASES_DIR;
        if (configured.rfind("//", 0) != 0) {
            return configured;
        }

#if defined(_WIN32)
        return configured;
#else
        std::error_code ec;
        auto exePath = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (ec) {
            return configured;
        }

        const std::string exePathStr = exePath.string();
        constexpr const char *OUT_MARKER = "/out/";
        const auto outPos = exePathStr.find(OUT_MARKER);
        if (outPos == std::string::npos) {
            return configured;
        }

        std::string resolved = exePathStr.substr(0, outPos) + configured.substr(1);
        return std::filesystem::exists(resolved) ? resolved : configured;
#endif
    }();
    return kCasesDir;
}
#endif

void ConfigureBuildFolderForBenchmarks()
{
    static const bool kConfigured = []() {
        const char *existing = std::getenv("BUILD_FOLDER");
        if (existing != nullptr && existing[0] != '\0') {
            return true;
        }

#if defined(_WIN32)
        // Keep host mingw build compile-safe; benchmark runtime path fix is only needed for Linux host runs.
        return _putenv_s("BUILD_FOLDER", "") == 0;
#else
        std::error_code ec;
        auto exePath = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (ec) {
            return false;
        }

        const auto exeDir = exePath.parent_path();
        if (exeDir.empty()) {
            return false;
        }

        // Initializer treats BUILD_FOLDER as argv[0], then resolves arktsconfig via dirname(argv[0]).
        // Use a stable fake executable path in the same directory as current benchmark binary.
        const std::string fakeExecPath = (exeDir / "es2panda").string();
        return setenv("BUILD_FOLDER", fakeExecPath.c_str(), 1) == 0;
#endif
    }();
    (void)kConfigured;
}

LspBenchmarkMemoryCountersScope::LspBenchmarkMemoryCountersScope(benchmark::State &state)
    : state_(state), start_current_rss_(GetCurrentRSS()), start_peak_rss_(GetPeakRSS())
{
}

LspBenchmarkMemoryCountersScope::~LspBenchmarkMemoryCountersScope()
{
    const size_t endCurrentRss = GetCurrentRSS();
    const size_t endPeakRss = GetPeakRSS();
    const size_t currentDelta = endCurrentRss > start_current_rss_ ? endCurrentRss - start_current_rss_ : 0;
    const size_t peakDelta = endPeakRss > start_peak_rss_ ? endPeakRss - start_peak_rss_ : 0;

    state_.counters["mem_delta_rss_kb"] = benchmark::Counter(static_cast<double>(currentDelta) / KILOBYTE);
    state_.counters["mem_delta_hwm_kb"] = benchmark::Counter(static_cast<double>(peakDelta) / KILOBYTE);
}

LspMemoryManager::LspMemoryManager() : start_current_rss_(0), end_current_rss_(0), start_peak_rss_(0), end_peak_rss_(0)
{
    ConfigureBuildFolderForBenchmarks();
}

void LspMemoryManager::Start()
{
    start_current_rss_ = ::GetCurrentRSS();
    start_peak_rss_ = ::GetPeakRSS();
}

void LspMemoryManager::Stop(Result &result)
{
    end_current_rss_ = ::GetCurrentRSS();
    end_peak_rss_ = ::GetPeakRSS();
    const size_t currentDelta = end_current_rss_ > start_current_rss_ ? end_current_rss_ - start_current_rss_ : 0;
    const size_t peakDelta = end_peak_rss_ > start_peak_rss_ ? end_peak_rss_ - start_peak_rss_ : 0;

    result.max_bytes_used = std::max(currentDelta, peakDelta) / KILOBYTE;
    result.num_allocs = 0;
}
