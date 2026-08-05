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

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ark::es2panda::gluegen {

std::filesystem::path ToLongPathIfNeeded(const std::filesystem::path &path)
{
#if defined(_WIN32)
    constexpr std::size_t WINDOWS_MAX_PATH_LENGTH = 260;

    std::error_code ec;
    auto absolutePath = std::filesystem::absolute(path, ec);
    if (ec) {
        absolutePath = path;
    }

    const auto &native = absolutePath.native();
    if (native.size() <= WINDOWS_MAX_PATH_LENGTH || native.rfind(LR"(\\?\)", 0) == 0) {
        return absolutePath;
    }
    if (native.rfind(LR"(\\)", 0) == 0) {
        // UNC path (\\server\share\...): the extended-length form is \\?\UNC\server\share\...
        static constexpr std::size_t kUncPrefixLen = 2;
        return std::filesystem::path(LR"(\\?\UNC\)" + native.substr(kUncPrefixLen));
    }
    return std::filesystem::path(LR"(\\?\)" + native);
#else
    return path;
#endif
}

std::optional<std::filesystem::file_time_type> GetLastModifiedTime(const std::filesystem::path &path)
{
    auto queryPath = ToLongPathIfNeeded(path);
    std::error_code ec;
    if (!std::filesystem::exists(queryPath, ec) || ec) {
        return std::nullopt;
    }

    auto time = std::filesystem::last_write_time(queryPath, ec);
    if (ec) {
        return std::nullopt;
    }
    return time;
}

std::string FileTimeToString(const std::filesystem::file_time_type &fileTime)
{
    using namespace std::chrono;

    // std::filesystem::file_time_type uses an unspecified clock, so it must be
    // rebased against system_clock before it can be formatted as calendar time.
    const auto systemTime = time_point_cast<system_clock::duration>(
        fileTime - std::filesystem::file_time_type::clock::now() + system_clock::now());

    const auto timeT = system_clock::to_time_t(systemTime);
    const auto ms = duration_cast<milliseconds>(systemTime.time_since_epoch()) % 1000;

    std::tm tmUtc {};
    bool ok = false;
#if defined(_WIN32)
    // gmtime_s returns errno_t: 0 on success, nonzero on failure (e.g. timeT out of representable range).
    ok = gmtime_s(&tmUtc, &timeT) == 0;
#else
    // gmtime_r writes into the caller-provided struct and returns a pointer to it, or nullptr on failure.
    ok = gmtime_r(&timeT, &tmUtc) != nullptr;
#endif
    if (!ok) {
        return {};  // unrepresentable time point -- signal failure with an empty string rather than garbage
    }
    constexpr int setwWidth = 3;
    std::ostringstream oss;
    oss << std::put_time(&tmUtc, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(setwWidth) << ms.count();
    oss << 'Z';
    return oss.str();
}

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
        task();
    }
}

}  // namespace ark::es2panda::gluegen
