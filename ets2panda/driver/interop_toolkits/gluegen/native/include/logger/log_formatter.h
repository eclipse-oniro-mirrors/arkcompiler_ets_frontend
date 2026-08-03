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

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_FORMATTER_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_FORMATTER_H

#include <array>
#include <chrono>
#include <ctime>
#include <optional>
#include <string>
#include <cassert>
#include <iomanip>
#include <sstream>

#include "libarkbase/macros.h"
#include "log_level.h"
#include "log_record.h"

namespace ark::es2panda::gluegen::log {

namespace detail {

// Thread-safe UTC breakdown of a time_t. std::gmtime returns a pointer into a shared static buffer
// and so is not thread-safe; the reentrant gmtime_r (POSIX) / gmtime_s (Windows CRT) variants write
// into a caller-provided struct instead. Formatting only ever runs on the backend thread today, but
// using the reentrant call keeps the formatter safe to reuse from anywhere. Returns nullopt when the
// time_t cannot be broken down (e.g. a value outside the representable calendar range).
inline std::optional<std::tm> GmTime(std::time_t t)
{
    std::tm out {};
#if defined(_WIN32)
    // gmtime_s returns errno_t: 0 on success, nonzero on failure.
    if (gmtime_s(&out, &t) != 0) {
        return std::nullopt;
    }
#else
    // gmtime_r writes into the caller-provided struct and returns a pointer to it, or nullptr on failure.
    if (gmtime_r(&t, &out) == nullptr) {
        return std::nullopt;
    }
#endif
    return out;
}

// Renders a system_clock time point as ISO-8601 UTC with millisecond precision, e.g.
// "2026-07-27T12:34:56.789Z" -- a sortable, unambiguous, timezone-explicit timestamp (the format
// structured-logging backends and log aggregators expect).
// Milliseconds are 0-999, so the fractional-seconds field is zero-padded to this width.
inline std::string FormatTimestampIso8601(std::chrono::system_clock::time_point tp)
{
    static constexpr int kMillisFieldWidth = 3;
    const auto secs = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch());
    const std::tm tm = GmTime(static_cast<std::time_t>(secs.count())).value_or(std::tm {});
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S") << '.' << std::setfill('0') << std::setw(kMillisFieldWidth)
        << std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch() - secs).count() << 'Z';
    return oss.str();
}
}  // namespace detail

// Turns a LogRecord into the exact bytes a sink writes. Kept as a separate strategy object (rather
// than hard-coded in each sink) so the same sink type can emit human-readable text on the console
// and machine-readable JSON to a file just by swapping formatters -- the layout/encoder split every
// mature logger draws (spdlog's formatter, log4j's Layout, zap's Encoder).
//
// A formatter is invoked only on the backend/consumer thread (or synchronously by the caller in
// sync mode), one record at a time, so implementations need not be internally synchronized. The
// returned string should include a trailing newline if line-delimited output is desired.
class LogFormatter {
public:
    LogFormatter() = default;
    virtual ~LogFormatter() = default;
    LogFormatter(const LogFormatter &) = delete;
    LogFormatter &operator=(const LogFormatter &) = delete;
    LogFormatter(LogFormatter &&) = delete;
    LogFormatter &operator=(LogFormatter &&) = delete;

    virtual std::string Format(const LogRecord &record) const = 0;
};

// Human-readable, single-line layout, e.g.:
//   2026-07-27T12:34:56.789Z INFO  [t2] gluec parsed source file=a.ets symbols=12 (gluec.cpp:820 Run)
// Structured fields are appended as space-separated key=value pairs after the message; source
// location (file:line function) is appended in parentheses when present. Chosen so a developer can
// eyeball logs directly while the key=value tail still greps cleanly.
class TextFormatter final : public LogFormatter {
public:
    // `includeSourceLocation` appends the "(file:line function)" suffix; disable it for tidier
    // production console output where the code site is noise.
    explicit TextFormatter(bool includeSourceLocation = true) : includeSourceLocation_(includeSourceLocation) {}

    std::string Format(const LogRecord &record) const override
    {
        std::string out;
        constexpr size_t redundantReserve = 64;
        out.reserve(redundantReserve + record.message.size());
        out += detail::FormatTimestampIso8601(record.timestamp);
        out += ' ';
        out += ToPaddedString(record.level);
        out += " [t";
        out += std::to_string(record.threadId);
        out += "] ";
        out += record.message;
        for (const auto &field : record.fields) {
            out += ' ';
            out += field.key;
            out += '=';
            out += field.value;
        }
        if (includeSourceLocation_ && record.file != nullptr) {
            out += " (";
            out += record.file;
            out += ':';
            out += std::to_string(record.line);
            if (record.function != nullptr) {
                out += ' ';
                out += record.function;
            }
            out += ')';
        }
        out += '\n';
        return out;
    }

private:
    bool includeSourceLocation_;
};

// One JSON object per line ("JSON Lines" / ndjson), e.g.:
//   {"time":"2026-07-27T12:34:56.789Z","level":"INFO","thread":2,"message":"...",
//    "file":"gluec.cpp","line":820,"function":"Run","fields":{"file":"a.ets","symbols":"12"}}
// This is the format log shippers (Fluent Bit, Vector, Loki, ...) ingest directly. Field values are
// emitted as JSON strings because the structured API captures them as strings; numeric typing is
// intentionally not inferred to avoid mis-typing values that only look numeric.
class JsonFormatter final : public LogFormatter {
public:
    std::string Format(const LogRecord &record) const override
    {
        std::string out;
        constexpr size_t redundantReserve = 96;  // worst-case: every char is escaped, plus a trailing null
        out.reserve(redundantReserve + record.message.size());
        out += "{\"time\":\"";
        out += detail::FormatTimestampIso8601(record.timestamp);
        out += "\",\"level\":\"";
        out += ToString(record.level);
        out += "\",\"thread\":";
        out += std::to_string(record.threadId);
        out += ",\"message\":\"";
        out += EscapeJson(record.message);
        out += '"';
        if (record.file != nullptr) {
            out += ",\"file\":\"";
            out += EscapeJson(record.file);
            out += "\",\"line\":";
            out += std::to_string(record.line);
            if (record.function != nullptr) {
                out += ",\"function\":\"";
                out += EscapeJson(record.function);
                out += '"';
            }
        }
        if (!record.fields.empty()) {
            out += ",\"fields\":{";
            bool first = true;
            for (const auto &field : record.fields) {
                if (!first) {
                    out += ',';
                }
                first = false;
                out += '"';
                out += EscapeJson(field.key);
                out += "\":\"";
                out += EscapeJson(field.value);
                out += '"';
            }
            out += '}';
        }
        out += "}\n";
        return out;
    }

private:
    // Minimal RFC-8259 JSON string escaping: escapes the seven mandatory characters
    // (" \ and the control chars \b \f \n \r \t) and emits any remaining control byte as a \u00XX
    // escape. Non-control bytes (including UTF-8 multibyte sequences) are passed through unchanged.
    // NOLINTNEXTLINE
    static std::string EscapeJson(const std::string &s)
    {
        std::string out;
        constexpr size_t redundantReserve = 2;
        out.reserve(s.size() + redundantReserve);
        for (const char ch : s) {
            switch (ch) {
                case '"':
                    out += "\\\"";
                    break;
                case '\\':
                    out += "\\\\";
                    break;
                case '\b':
                    out += "\\b";
                    break;
                case '\f':
                    out += "\\f";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                default: {
                    static constexpr unsigned char kAsciiSpace = 0x20;
                    static constexpr char kHex[] = "0123456789abcdef";  // lowercase, matching std::hex output
                    static constexpr unsigned kNibbleMask = 0xF;
                    static constexpr size_t kOff = 4;
                    const auto uch = static_cast<unsigned char>(ch);
                    if (uch < kAsciiSpace) {
                        // uch is a control byte (< kAsciiSpace): the \uXXXX escape is \u00 + its two hex digits.
                        out += "\\u00";
                        out += kHex[(uch >> kOff) & kNibbleMask];
                        out += kHex[uch & kNibbleMask];
                    } else {
                        out += ch;
                    }
                    break;
                }
            }
        }
        return out;
    }
};
}  // namespace ark::es2panda::gluegen::log

#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_FORMATTER_H
