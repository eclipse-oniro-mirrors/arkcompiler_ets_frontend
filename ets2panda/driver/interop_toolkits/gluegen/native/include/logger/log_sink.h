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

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_SINK_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_SINK_H

#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "log_formatter.h"
#include "log_level.h"
#include "log_record.h"

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace ark::es2panda::gluegen::log {

// A single output destination for formatted log records (console, a file, a network socket, ...).
// The Logger fans every emitted record out to each attached sink, so this is the multi-target
// output seam: attach a ConsoleSink and a FileSink to the same logger to get both at once.
//
// Threading contract: a sink's Emit()/Flush() are called only from the logger's single backend
// thread (async mode) or under the logger's dispatch lock (sync mode) -- i.e. never concurrently
// with themselves -- so sink implementations do NOT need internal locking around their I/O. The
// per-sink level threshold is the one field a caller may flip from another thread at runtime, so it
// is atomic. Sinks should be attached (and their formatter chosen) before the logger starts;
// swapping the formatter concurrently with logging is not supported.
class LogSink {
public:
    explicit LogSink(std::shared_ptr<LogFormatter> formatter = nullptr)
        : formatter_(formatter ? std::move(formatter) : std::make_shared<TextFormatter>())
    {
    }
    virtual ~LogSink() = default;

    LogSink(const LogSink &) = delete;
    LogSink &operator=(const LogSink &) = delete;
    LogSink(LogSink &&) = delete;
    LogSink &operator=(LogSink &&) = delete;

    // Called by the logger for every record that passed the logger-level threshold. Applies this
    // sink's own (possibly stricter) threshold, then formats and writes. Not virtual: the
    // level-gating policy is uniform across sinks; only the actual write (Write) and Flush vary.
    void Emit(const LogRecord &record)
    {
        // Atomic with relaxed order reason: per-sink level gate is advisory, replay-safe
        if (record.level < level_.load(std::memory_order_relaxed)) {
            return;
        }
        Write(record, formatter_->Format(record));
    }

    // Flushes any buffered bytes to the underlying medium. Invoked by the logger on its flush
    // interval, on Flush(), on shutdown, and during crash handling.
    virtual void Flush() = 0;

    // Per-sink minimum level: lets one sink be noisier than another (e.g. DEBUG to a file but only
    // WARN to the console) beneath the logger's global threshold. Safe to change at any time.
    void SetLevel(LogLevel level)
    {
        // Atomic with relaxed order reason: runtime level change is advisory, no ordering needed
        level_.store(level, std::memory_order_relaxed);
    }
    LogLevel GetLevel() const
    {
        // Atomic with relaxed order reason: advisory read, no ordering guarantee needed
        return level_.load(std::memory_order_relaxed);
    }

    // Replaces the layout/encoder. Intended to be called during setup, before the logger starts
    // dispatching to this sink (see the threading contract above).
    void SetFormatter(std::shared_ptr<LogFormatter> formatter)
    {
        formatter_ = std::move(formatter);
    }

protected:
    // Writes the already-formatted bytes for `record` (raw record supplied too, for sinks that want
    // to route by level, e.g. ConsoleSink's stdout/stderr split).
    virtual void Write(const LogRecord &record, const std::string &formatted) = 0;

private:
    std::atomic<LogLevel> level_ {LogLevel::DEBUG};
    std::shared_ptr<LogFormatter> formatter_;
};

// Writes to the process console: records at or above `stderrThreshold` go to stderr, everything
// else to stdout -- the conventional split so warnings/errors survive stdout redirection and stay
// visible on a terminal. Optionally colorizes the level token with ANSI escapes when the target is
// an interactive TTY (auto-detected; never emitted into a redirected file/pipe, so captured logs
// stay clean).
class ConsoleSink final : public LogSink {
public:
    // `stderrThreshold` = the lowest level routed to stderr (default WARN, so WARN/ERROR -> stderr,
    // DEBUG/INFO -> stdout). `colorMode` controls ANSI coloring: Auto colorizes only when the
    // corresponding stream is a TTY; Always/Never force it.
    enum class ColorMode { AUTO, ALWAYS, NEVER };

    explicit ConsoleSink(LogLevel stderrThreshold = LogLevel::WARN, ColorMode colorMode = ColorMode::AUTO,
                         std::shared_ptr<LogFormatter> formatter = nullptr)
        : LogSink(std::move(formatter)),
          stderrThreshold_(stderrThreshold),
          colorStdout_(ResolveColor(colorMode, false)),
          colorStderr_(ResolveColor(colorMode, true))
    {
    }

    void Flush() override
    {
        std::cout.flush();
        std::cerr.flush();
    }

protected:
    void Write(const LogRecord &record, const std::string &formatted) override
    {
        const bool toStderr = record.level >= stderrThreshold_;
        std::ostream &os = toStderr ? std::cerr : std::cout;
        const bool color = toStderr ? colorStderr_ : colorStdout_;
        if (color) {
            os << LevelColorCode(record.level) << formatted << ANSI_RESET;
        } else {
            os << formatted;
        }
    }

private:
    static constexpr const char *ANSI_RESET = "\033[0m";

    static const char *LevelColorCode(LogLevel level)
    {
        switch (level) {
            case LogLevel::DEBUG:
                return "\033[36m";  // cyan
            case LogLevel::INFO:
                return "\033[32m";  // green
            case LogLevel::WARN:
                return "\033[33m";  // yellow
            case LogLevel::ERROR:
                return "\033[31m";  // red
            case LogLevel::OFF:
                return "";
        }
        return "";
    }

    static bool IsTty(bool isStderr)
    {
#if defined(_WIN32)
        return _isatty(_fileno(isStderr ? stderr : stdout)) != 0;
#else
        return ::isatty(isStderr ? STDERR_FILENO : STDOUT_FILENO) != 0;
#endif
    }

    static bool ResolveColor(ColorMode mode, bool isStderr)
    {
        switch (mode) {
            case ColorMode::ALWAYS:
                return true;
            case ColorMode::NEVER:
                return false;
            case ColorMode::AUTO:
                return IsTty(isStderr);
        }
        return false;
    }

    LogLevel stderrThreshold_;
    bool colorStdout_;
    bool colorStderr_;
};

// Appends (or truncates, per `truncate`) formatted records to a file. Buffered via std::ofstream;
// the logger's periodic flush and crash-time flush drive the actual disk write, so records aren't
// stranded in the C++ stream buffer if the process dies. IsOpen() lets a caller detect an open
// failure at construction time rather than silently dropping every record.
class FileSink final : public LogSink {
public:
    explicit FileSink(const std::string &path, bool truncate = false, std::shared_ptr<LogFormatter> formatter = nullptr)
        : LogSink(std::move(formatter)), out_(path, std::ios::binary | (truncate ? std::ios::trunc : std::ios::app))
    {
    }

    bool IsOpen() const
    {
        return out_.is_open();
    }

    void Flush() override
    {
        out_.flush();
    }

protected:
    void Write(const LogRecord & /*record*/, const std::string &formatted) override
    {
        out_ << formatted;
    }

private:
    std::ofstream out_;
};

}  // namespace ark::es2panda::gluegen::log

#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_SINK_H
