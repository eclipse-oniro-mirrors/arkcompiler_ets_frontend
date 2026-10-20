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

#include "utils.h"
#include "public/public.h"

#include <cstdint>

#if defined(PANDA_TARGET_WINDOWS)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#endif

namespace ark::es2panda::gluegen {

#if !defined(GLUEGEN_FORCE_EXPERIMENTAL_FILESYSTEM) && __has_include(<filesystem>)
fs::path AbsolutePath(const fs::path &path)
{
    return fs::absolute(path);
}

fs::path WeaklyCanonical(const fs::path &path)
{
    return fs::weakly_canonical(path);
}

fs::path RelativePath(const fs::path &path, const fs::path &base)
{
    return fs::relative(path, base);
}
#else
fs::path AbsolutePath(const fs::path &path)
{
    return fs::absolute(path);
}

fs::path WeaklyCanonical(const fs::path &path)
{
    fs::path existingPath = AbsolutePath(path);
    std::vector<fs::path> missingComponents;
    while (!fs::exists(existingPath)) {
        const auto parentPath = existingPath.parent_path();
        if (parentPath == existingPath) {
            return existingPath;
        }
        missingComponents.push_back(existingPath.filename());
        existingPath = parentPath;
    }

    auto canonicalPath = fs::canonical(existingPath);
    for (auto it = missingComponents.rbegin(); it != missingComponents.rend(); ++it) {
        if (*it == ".") {
            continue;
        }
        if (*it == "..") {
            canonicalPath = canonicalPath.parent_path();
            continue;
        }
        canonicalPath /= *it;
    }
    return canonicalPath;
}

fs::path RelativePath(const fs::path &path, const fs::path &base)
{
    const auto absolutePath = WeaklyCanonical(AbsolutePath(path));
    const auto absoluteBase = WeaklyCanonical(AbsolutePath(base));
    // Mirror std::filesystem::path::lexically_relative: no relative form exists across a
    // differing root-name (e.g. Windows drives) or differing absoluteness -- return empty.
    if (absolutePath.root_name() != absoluteBase.root_name() ||
        absolutePath.is_absolute() != absoluteBase.is_absolute() ||
        (!absolutePath.has_root_directory() && absoluteBase.has_root_directory())) {
        return fs::path();
    }

    auto pathIt = absolutePath.begin();
    auto baseIt = absoluteBase.begin();
    while (pathIt != absolutePath.end() && baseIt != absoluteBase.end() && *pathIt == *baseIt) {
        ++pathIt;
        ++baseIt;
    }

    fs::path result;
    for (; baseIt != absoluteBase.end(); ++baseIt) {
        // Skip empty (trailing-separator) and "." elements, matching lexically_relative.
        if (baseIt->empty() || *baseIt == ".") {
            continue;
        }
        result /= "..";
    }
    for (; pathIt != absolutePath.end(); ++pathIt) {
        result /= *pathIt;
    }
    return result.empty() ? fs::path(".") : result;
}
#endif

fs::path ToLongPathIfNeeded(const fs::path &path)
{
#if defined(PANDA_TARGET_WINDOWS)
    return fs::path(ark::os::file::File::GetExtendedFilePath(path.string()));
#else
    return path;
#endif
}

#if defined(PANDA_TARGET_WINDOWS)
std::optional<std::string> GetLastModifiedTimeStamp(const fs::path &path)
{
    // FILETIME is unsigned 100-ns ticks since 1601-01-01 UTC; its high DWORD holds bits 32..63
    // of the 64-bit tick count.
    constexpr uint64_t kFiletimeHighDwordShift = 32U;
    constexpr uint64_t kNanosecondsPerFiletimeTick = 100ULL;
    constexpr int64_t kFiletimeEpochToUnixEpoch100Ns = 116444736000000000LL;

    // GetFileAttributesExW reads metadata only and does not open a file handle, so it cannot
    // conflict with any concurrent reader/writer of the file itself. Note: unlike `stat`, it
    // does not follow symlinks -- callers always pass NormalizePath() output, whose
    // weakly-canonical step has already resolved any links.
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExW(ToLongPathIfNeeded(path).wstring().c_str(), GetFileExInfoStandard, &data) == 0) {
        return std::nullopt;
    }
    const uint64_t ticks = (static_cast<uint64_t>(data.ftLastWriteTime.dwHighDateTime) << kFiletimeHighDwordShift) |
                           data.ftLastWriteTime.dwLowDateTime;
    const int64_t unix100Ns = static_cast<int64_t>(ticks) - kFiletimeEpochToUnixEpoch100Ns;
    // Unsigned arithmetic keeps even absurd (pre-1970 / far-future) values well-defined;
    // every run computes the identical wrapped value, which is all a cache key needs.
    const int64_t ns = static_cast<int64_t>(static_cast<uint64_t>(unix100Ns) * kNanosecondsPerFiletimeTick);
    return std::to_string(ns);
}
#else
std::optional<std::string> GetLastModifiedTimeStamp(const fs::path &path)
{
    constexpr uint64_t kNanosecondsPerSecond = 1000000000ULL;
    struct stat st;
    if (::stat(path.c_str(), &st) != 0) {
        return std::nullopt;
    }
    const int64_t sec = static_cast<int64_t>(st.st_mtime);
#if defined(__APPLE__)
    const int64_t nsec = static_cast<int64_t>(st.st_mtimespec.tv_nsec);
#else
    const int64_t nsec = static_cast<int64_t>(st.st_mtim.tv_nsec);
#endif
    // See the Windows branch for why the arithmetic is done unsigned.
    const int64_t ns =
        static_cast<int64_t>(static_cast<uint64_t>(sec) * kNanosecondsPerSecond + static_cast<uint64_t>(nsec));
    return std::to_string(ns);
}
#endif

ThreadPool::ThreadPool(std::size_t threadCount)
{
    if (threadCount == 0) {
        const auto hw = std::thread::hardware_concurrency();
        static constexpr std::size_t kMinThreads = 2;
        static constexpr std::size_t kMaxThreads = 8;
        static constexpr std::size_t kHwDivisor = 2;
        threadCount = hw > 0 ? std::max(kMinThreads, std::min<std::size_t>(hw / kHwDivisor, kMaxThreads)) : kMinThreads;
    }
    workers_.reserve(threadCount);
    for (std::size_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back([this]() { WorkerLoop(); });
    }
}

ThreadPool::~ThreadPool()
{
    Join();
}

void ThreadPool::Post(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

void ThreadPool::Join()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) {
            return;
        }
        stopping_ = true;
    }
    cv_.notify_all();
    for (auto &worker : workers_) {
        worker.join();
    }
}

void ThreadPool::WorkerLoop()
{
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return stopping_ || !tasks_.empty(); });
            if (tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        // A task must never let an exception escape: it would propagate out of this worker's
        // std::thread function, which calls std::terminate and takes the whole process down --
        // and with several workers failing at once that surfaces as "terminate called
        // recursively", hiding whatever the original error was. Every task posted to these pools
        // reports its own failures through its owner's DiagnosticEngine, so reaching this handler
        // means something unforeseen went wrong; swallowing it here keeps the remaining tasks
        // running and lets the owner finish and report normally.
        try {
            task();
        } catch (...) {  // NOLINT(bugprone-empty-catch)
        }
    }
}

}  // namespace ark::es2panda::gluegen
