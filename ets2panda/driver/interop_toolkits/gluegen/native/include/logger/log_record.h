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

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_RECORD_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_RECORD_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "log_level.h"

namespace ark::es2panda::gluegen::log {

// One structured key/value attached to a log record. Both key and value are captured as strings at
// log time (the record is handed to a background thread, so it can't hold references into the
// caller's stack). This is the "structured logging" primitive -- instead of baking variable data
// into a prose string, callers attach typed context as fields (key="file", value="a.ets") that a
// JSON sink can emit as real JSON members and a text sink can render as key=value pairs, keeping
// the human message stable and machine-parseable at once (the model used by zap, logrus, slog).
struct LogField {
    std::string key;
    std::string value;
};

// A single fully-captured log event, self-contained so it can be moved onto the async queue and
// rendered later by the backend thread without touching any caller state. `file`/`function` are
// stored as raw pointers because they always come from __FILE__/__func__ -- string literals with
// static storage duration that outlive the whole program, so copying them would be pure waste.
struct LogRecord {
    LogLevel level = LogLevel::INFO;
    // Captured at submission time so ordering/timestamps reflect when the event happened, not when
    // the backend thread happened to format it.
    std::chrono::system_clock::time_point timestamp {};
    // Small, stable, per-thread id (see CurrentThreadId) -- friendlier in logs than a hashed
    // std::thread::id and cheaper than formatting the native handle.
    std::uint64_t threadId = 0;
    const char *file = nullptr;      // __FILE__  (static storage; not owned)
    const char *function = nullptr;  // __func__  (static storage; not owned)
    int line = 0;
    std::string message;
    std::vector<LogField> fields;

    LogRecord() = default;

    LogRecord(LogLevel lvl, const char *fileName, int lineNo, const char *functionName, std::string msg)
        : level(lvl), file(fileName), function(functionName), line(lineNo), message(std::move(msg))
    {
    }
};

// Returns a small, stable, monotonically-assigned id for the calling thread (1, 2, 3, ...), unique
// for the process lifetime. Preferred over hashing std::thread::id because the raw hash is a large,
// noisy, platform-dependent number; a compact counter reads far better in log output. Assigned
// lazily on first use per thread and cached in thread_local storage, so the atomic increment is
// paid exactly once per thread.
inline std::uint64_t CurrentThreadId()
{
    static std::atomic<std::uint64_t> counter {0};
    // Atomic with relaxed order reason: monotonic thread-id counter, no happens-before needed
    thread_local std::uint64_t id = counter.fetch_add(1, std::memory_order_relaxed) + 1;
    return id;
}

}  // namespace ark::es2panda::gluegen::log

#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_RECORD_H
