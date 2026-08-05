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

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_H

#include <sstream>
#include <string>
#include <utility>

#include "log_level.h"
#include "log_record.h"
#include "logger.h"

namespace ark::es2panda::gluegen::log {

// RAII builder that a logging macro instantiates for one call site. It accumulates the message via
// operator<< and structured attributes via Field(), then submits the finished record to its logger
// exactly once, in its destructor -- so `GLUEGEN_LOG_INFO(log) << "a=" << a << " b=" << b;` builds
// one record and emits it at the end of the full-expression. This "temporary object stream" idiom
// is the same one glog/Abseil LOG() use; it makes the streaming syntax natural while guaranteeing a
// single Submit per statement.
//
// A builder is created only after the logger's cheap level gate has already passed (see the macros
// below), so building the message string is never paid for a suppressed level.
class LogEntry {
public:
    LogEntry(Logger &logger, LogLevel level, const char *file, int line, const char *function)
        : logger_(&logger), record_(level, file, line, function, std::string {})
    {
    }

    // Moveable so a macro can hand the temporary along an operator<< chain; the moved-from entry is
    // neutralized (logger_ = nullptr) so only the final owner submits.
    LogEntry(LogEntry &&other) noexcept : logger_(other.logger_), record_(std::move(other.record_))
    {
        other.logger_ = nullptr;
    }
    LogEntry &operator=(LogEntry &&) = delete;
    LogEntry(const LogEntry &) = delete;
    LogEntry &operator=(const LogEntry &) = delete;

    ~LogEntry()
    {
        if (logger_ != nullptr) {
            record_.message = stream_.str();
            logger_->Submit(std::move(record_));
        }
    }

    // Append anything ostream-formattable to the human message.
    template <typename T>
    LogEntry &operator<<(const T &value)
    {
        stream_ << value;
        return *this;
    }

    // Attach a structured key/value pair. Chainable and mixable with operator<<. The value may be
    // any ostream-formattable type; it is stringified immediately so the record stays self-owned.
    template <typename T>
    LogEntry &Field(std::string key, const T &value)
    {
        std::ostringstream oss;
        oss << value;
        record_.fields.push_back(LogField {std::move(key), oss.str()});
        return *this;
    }

    // String-value fast path avoiding an ostringstream round-trip.
    LogEntry &Field(std::string key, std::string value)
    {
        record_.fields.push_back(LogField {std::move(key), std::move(value)});
        return *this;
    }

private:
    Logger *logger_;
    LogRecord record_;
    std::ostringstream stream_;
};

}  // namespace ark::es2panda::gluegen::log

// Source location captured per log statement: file/line/function in debug builds (NDEBUG unset,
// the standard convention also used by assert()), for pinpointing exactly which call site produced
// a record. Omitted entirely in release builds (NDEBUG defined) -- LogEntry gets nullptr/0/nullptr,
// which both TextFormatter and JsonFormatter already treat as "no location to render" (see their
// `record.file != nullptr` checks), so release log output stays free of code-location noise without
// any change to LogRecord/LogEntry/the formatters themselves.
#if defined(NDEBUG)
#define GLUEGEN_LOG_SOURCE_FILE nullptr
#define GLUEGEN_LOG_SOURCE_LINE 0
#define GLUEGEN_LOG_SOURCE_FUNC nullptr
#else
#define GLUEGEN_LOG_SOURCE_FILE __FILE__
#define GLUEGEN_LOG_SOURCE_LINE __LINE__
#define GLUEGEN_LOG_SOURCE_FUNC __func__
#endif

// Core macro: evaluate the body only if the logger admits `level`, otherwise skip it entirely. The
// `if (cond) ; else <expr>` shape (rather than `if (!cond) <expr>`) is the standard glog pattern --
// it keeps the macro a single statement, composes correctly after a bare `if`, and lets the
// trailing `<< ...` in the call site bind to the LogEntry temporary. Because ShouldLog is a single
// relaxed atomic load, a suppressed log statement costs almost nothing and never builds a record.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLUEGEN_LOG(logger, level)                                                                                   \
    if (!(logger).ShouldLog(level)) {                                                                                \
    } else                                                                                                           \
        ::ark::es2panda::gluegen::log::LogEntry((logger), (level), GLUEGEN_LOG_SOURCE_FILE, GLUEGEN_LOG_SOURCE_LINE, \
                                                GLUEGEN_LOG_SOURCE_FUNC)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLUEGEN_LOG_DEBUG(logger) GLUEGEN_LOG((logger), ::ark::es2panda::gluegen::log::LogLevel::DEBUG)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLUEGEN_LOG_INFO(logger) GLUEGEN_LOG((logger), ::ark::es2panda::gluegen::log::LogLevel::INFO)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLUEGEN_LOG_WARN(logger) GLUEGEN_LOG((logger), ::ark::es2panda::gluegen::log::LogLevel::WARN)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLUEGEN_LOG_ERROR(logger) GLUEGEN_LOG((logger), ::ark::es2panda::gluegen::log::LogLevel::ERROR)

#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_H
