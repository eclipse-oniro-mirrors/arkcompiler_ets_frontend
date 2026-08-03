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
#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_UTILS_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_UTILS_H

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <optional>
#include <string>
#include <functional>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <condition_variable>
#include <queue>
#include <thread>
#include "libarkbase/macros.h"

namespace ark::es2panda::gluegen {

/**
 * @brief normalize a path-like value into a `string` absolute, canonicalized
 * (symlinks resolved where possible) path with directory separators
 * normalized to the current platform's native separator (`\` on Windows,
 * `/` everywhere else) -- i.e. the separator follows the host system, it is
 * NOT unconditionally forward-slash. Use `ToForwardSlashPath` instead when a
 * portable, platform-independent forward-slash form is required (e.g. a
 * value that is persisted and may be read back on a different platform).
 *
 * Accepts `std::filesystem::path`, `std::string`, `std::string_view`,
 * `const char *` and similar standard string-like inputs. Implemented as a
 * single function template (rather than overloads on `std::filesystem::path`
 * and `std::string_view`) to avoid ambiguous overload resolution when called
 * with a string literal, since both types have implicit converting
 * constructors from `const char *`.
 *
 * Uses `std::filesystem::weakly_canonical` (rather than `canonical`) so this
 * doesn't throw/fail for paths that don't fully exist yet (e.g. an output
 * file not written yet): the longest existing prefix is resolved to its real
 * path (symlinks included), and any non-existent remainder is appended as-is
 * after lexical normalization. Falls back to a plain `absolute()` path (or,
 * failing that, the input path as-is) if canonicalization can't be performed
 * (e.g. due to a filesystem error), rather than throwing.
 */
template <typename PathLike>
std::string NormalizePath(const PathLike &pathLike)
{
    std::filesystem::path path;
    if constexpr (std::is_same_v<std::decay_t<PathLike>, std::filesystem::path>) {
        path = pathLike;
    } else {
        path = std::filesystem::path(std::string_view(pathLike));
    }

    std::error_code ec;
    auto absolutePath = std::filesystem::absolute(path, ec);
    if (ec) {
        absolutePath = path;
    }

    std::error_code canonicalEc;
    auto canonicalPath = std::filesystem::weakly_canonical(absolutePath, canonicalEc);
    if (!canonicalEc) {
        absolutePath = canonicalPath;
    }

    std::string result = absolutePath.string();
#ifdef PANDA_TARGET_WINDOWS
    std::replace(result.begin(), result.end(), '/', '\\');
#else
    std::replace(result.begin(), result.end(), '\\', '/');
#endif
    return result;
}

/**
 * @brief uniformly convert every directory separator in a path-like value to a forward slash
 * (`/`), regardless of the host platform, unlike `NormalizePath` whose separator follows the
 * current platform's convention.
 *
 * This is a plain string-level substitution -- it performs no filesystem access, does not make
 * the path absolute, and does not canonicalize it. Intended for values that must be portable
 * across platforms (e.g. a key persisted to a file that may be produced on one platform and
 * consumed on another), typically combined with `NormalizePath` by calling this on its result
 * when a forward-slash form is also needed.
 *
 * Accepts the same path-like inputs as `NormalizePath` (`std::filesystem::path`, `std::string`,
 * `std::string_view`, `const char *`, and similar).
 */
template <typename PathLike>
std::string ToForwardSlashPath(const PathLike &pathLike)
{
    std::string result;
    if constexpr (std::is_same_v<std::decay_t<PathLike>, std::filesystem::path>) {
        result = pathLike.string();
    } else {
        result = std::string(std::string_view(pathLike));
    }
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

/**
 * @brief On Windows, rewrites `path` into extended-length ("\\?\...") form when its absolute
 * native representation is longer than the legacy 260-character MAX_PATH limit, so subsequent
 * Win32 file-API calls (which every standard file operation -- fstream, std::filesystem -- goes
 * through on Windows) don't fail to open/create/query it. The \\?\ prefix disables both the
 * length check and further path normalization, so it is only added to a path already made
 * absolute here (a UNC path, `\\server\share\...`, instead needs the `\\?\UNC\` form). No-op
 * (returns `path` unchanged) on every other platform, and if `path` is already prefixed.
 * See https://learn.microsoft.com/windows/win32/fileio/naming-a-file#maximum-path-length-limitation
 */
std::filesystem::path ToLongPathIfNeeded(const std::filesystem::path &path);

/**
 * @brief get the last modified time of a file.
 * @return the file's last write time, or `std::nullopt` if the file does not
 * exist or its modification time could not be queried.
 */
std::optional<std::filesystem::file_time_type> GetLastModifiedTime(const std::filesystem::path &path);

/**
 * @brief convert a `std::filesystem::file_time_type` (e.g. from
 * `GetLastModifiedTime`) into an ISO-8601 UTC string with millisecond
 * precision, e.g. "2026-07-20T15:30:45.123Z".
 */
std::string FileTimeToString(const std::filesystem::file_time_type &fileTime);

// Non-owning view letting an entire container be streamed straight into a single log statement
// (e.g. `GLUEGEN_LOG_INFO(logger) << "files: " << Joined(files);`), so printing N items costs one
// LogEntry/one queue submission instead of N -- and without first materializing a separate
// std::string via a manual accumulate-then-log loop. `items` is only ever read once, in operator<<
// below, at the point the log statement itself streams it (nothing is copied or deferred), so
// binding to the caller's container by reference is safe as long as the JoinedView doesn't outlive
// the full expression it was created in -- exactly how it's used here.
template <typename Container>
struct JoinedView {
    const Container &items;
    const char *separator;
};

template <typename Container>
JoinedView<Container> Joined(const Container &items, const char *separator = ", ")
{
    return JoinedView<Container> {items, separator};
}

template <typename Container>
std::ostream &operator<<(std::ostream &os, const JoinedView<Container> &view)
{
    bool first = true;
    for (const auto &item : view.items) {
        if (!first) {
            os << view.separator;
        }
        first = false;
        os << item;
    }
    return os;
}

// A minimal fixed-size thread pool: `threadCount` worker threads drain a shared FIFO task queue
// until `Join()` is called *and* the queue is empty. Replaces asio::thread_pool (previously used
// here) with a small std-library-only implementation, so IntermediateCacheWriter/Reader's
// background disk I/O no longer pulls in a third-party dependency for what is, in essence, a
// handful of lines of producer/consumer bookkeeping.
class ThreadPool {
public:
    // `threadCount == 0` (the default) means "auto-detect": at least 2 threads, at most
    // min(hardware_concurrency / 2, 8), with a floor of 2 -- a reasonable range for the
    // disk-I/O-bound background work these pools perform. Callers that need a specific size
    // (e.g. tests) can still pass an explicit non-zero value.
    explicit ThreadPool(std::size_t threadCount = 0);
    ~ThreadPool();

    NO_COPY_SEMANTIC(ThreadPool);
    NO_MOVE_SEMANTIC(ThreadPool);

    // Schedules `task` to run on the next available worker thread. Safe to call from any thread.
    void Post(std::function<void()> task);

    // Blocks until every task posted so far has finished running, then stops and joins every
    // worker thread. Idempotent (a second call is a no-op), matching asio::thread_pool::join().
    // Not safe to `Post` after `Join` has returned -- same restriction as asio::thread_pool.
    void Join();

private:
    void WorkerLoop();

    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
    bool stopping_ = false;
};

}  // namespace ark::es2panda::gluegen

#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_UTILS_H
