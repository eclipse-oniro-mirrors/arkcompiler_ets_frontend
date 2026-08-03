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

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_LEVEL_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_LEVEL_H

#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ark::es2panda::gluegen::log {

// Severity levels, strictly ordered DEBUG < INFO < WARN < ERROR so that a single relational compare
// against the active threshold decides whether a record is emitted (the standard leveled-logging
// model shared by log4j/spdlog/glog/zap). OFF sits above ERROR as a sentinel meaning "emit nothing"
// -- used only as a threshold, never as a record's own level. Backed by uint8_t so it is trivially
// storable in an atomic threshold and cheap to pass around.
enum class LogLevel : std::uint8_t {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    OFF = 4,
};
static_assert(static_cast<std::uint8_t>(LogLevel::DEBUG) == 0, "DEBUG must be 0");
static_assert(static_cast<std::uint8_t>(LogLevel::INFO) == 1, "INFO must be 1");
static_assert(static_cast<std::uint8_t>(LogLevel::WARN) == 2, "WARN must be 2");
static_assert(static_cast<std::uint8_t>(LogLevel::ERROR) == 3, "ERROR must be 3");
static_assert(static_cast<std::uint8_t>(LogLevel::OFF) == 4, "OFF must be 4");

// Uppercase name of a level, e.g. "INFO". OFF renders as "OFF". Returned as a static string literal
// (never allocates) so it is safe to hold onto and cheap to use in hot formatting paths.
inline const char *ToString(LogLevel level)
{
    static constexpr const char *kNames[] = {"DEBUG", "INFO", "WARN", "ERROR", "OFF"};
    static constexpr std::uint8_t kMaxLevelIndex = static_cast<std::uint8_t>(LogLevel::OFF);
    auto i = static_cast<std::uint8_t>(level);
    return i <= kMaxLevelIndex ? kNames[i] : "UNKNOWN";
}

// Same as ToString but right-padded to a fixed 5-character width ("DEBUG", "INFO ", "WARN ",
// "ERROR") so level columns line up in human-readable console/file output without the formatter
// having to pad at runtime.
inline const char *ToPaddedString(LogLevel level)
{
    static constexpr const char *kPadded[] = {"DEBUG", "INFO ", "WARN ", "ERROR", "OFF  "};
    static constexpr std::uint8_t kMaxLevelIndex = static_cast<std::uint8_t>(LogLevel::OFF);
    auto i = static_cast<std::uint8_t>(level);
    return i <= kMaxLevelIndex ? kPadded[i] : "?????";
}

// Parses a level name case-insensitively (e.g. "debug", "INFO", "Warn", "error", "off"), returning
// std::nullopt for anything unrecognized. Useful for driving the threshold from an environment
// variable or CLI flag. Also accepts the common alias "warning" for WARN.
template <class StringLike>
std::optional<LogLevel> LogLevelFromString(const StringLike &str)
{
    static_assert(std::is_convertible_v<StringLike, std::string_view>,
                  "LogLevelFromString() accepts string-like types");
    std::string_view text(str);
    std::string lowered;
    lowered.reserve(text.size());
    for (char c : text) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    if (lowered == "debug") {
        return LogLevel::DEBUG;
    }
    if (lowered == "info") {
        return LogLevel::INFO;
    }
    if (lowered == "warn" || lowered == "warning") {
        return LogLevel::WARN;
    }
    if (lowered == "error") {
        return LogLevel::ERROR;
    }
    if (lowered == "off" || lowered == "none") {
        return LogLevel::OFF;
    }
    return std::nullopt;
}

}  // namespace ark::es2panda::gluegen::log

#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_LOGGER_LOG_LEVEL_H
