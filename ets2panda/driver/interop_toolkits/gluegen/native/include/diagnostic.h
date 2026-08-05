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

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_DIAGNOSTIC_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_DIAGNOSTIC_H

#include <cstdint>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"
#include "libarkbase/macros.h"

#define DIAGNOSTIC_CODE_BASE 11420000
namespace ark::es2panda::gluegen {
namespace {
inline constexpr uint32_t MakeDiagnosticCode(uint32_t offset)
{
    return DIAGNOSTIC_CODE_BASE + offset;
}
}  // namespace

enum class DiagnosticCode : uint32_t {
    INVALID_CONFIG = MakeDiagnosticCode(1),              // bad/missing CLI options, arktsconfig, or other run config
    SOURCE_PARSE_FAILED = MakeDiagnosticCode(2),         // es2panda failed to parse/bind a source file
    OUTPUT_WRITE_FAILED = MakeDiagnosticCode(3),         // failed to write a generated output/status/report file
    IO_ERROR = MakeDiagnosticCode(4),                    // a filesystem operation (stat, read, ...) failed
    LINK_TARGET_NOT_FOUND = MakeDiagnosticCode(5),       // a requested link target has no intermediate cache
    MISSING_EXPORT_BINDING = MakeDiagnosticCode(6),      // a named re-export binding could not be resolved
    UNRESOLVED_REEXPORT_SOURCE = MakeDiagnosticCode(7),  // a re-export's source module has no intermediate cache
    CACHE_MANIFEST_CORRUPT = MakeDiagnosticCode(8),      // <cacheDir>/manifest.json was missing/unparsable
    CACHE_IO_ERROR = MakeDiagnosticCode(9),              // failed to read/write an intermediate cache file
};
struct Diagnostic {
    uint32_t code;
    std::string description;
    std::string cause;
    std::string position;
    std::vector<std::string> solutions;
};

inline void to_json(nlohmann::json &j, const Diagnostic &d)
{
    j = nlohmann::json {
        {"code", std::to_string(d.code)}, {"description", d.description}, {"cause", d.cause},
        {"position", d.position},         {"solutions", d.solutions},
    };
}

inline void from_json(const nlohmann::json &j, Diagnostic &d)
{
    const auto codeStr = j.at("code").get<std::string>();
    d.code = static_cast<uint32_t>(std::stoul(codeStr));
    j.at("description").get_to(d.description);
    j.at("cause").get_to(d.cause);
    j.at("position").get_to(d.position);
    j.at("solutions").get_to(d.solutions);
}

enum class DiagnosticSeverity : uint8_t {
    NOTE,
    WARNING,
    ERROR,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DiagnosticSeverity, {
                                                     {DiagnosticSeverity::NOTE, "note"},
                                                     {DiagnosticSeverity::WARNING, "warning"},
                                                     {DiagnosticSeverity::ERROR, "error"},
                                                 })

struct DiagnosticRecord {
    DiagnosticSeverity severity;
    Diagnostic diagnostic;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DiagnosticRecord, severity, diagnostic)

class DiagnosticConsumer {
public:
    DiagnosticConsumer() = default;
    virtual ~DiagnosticConsumer() = default;

    NO_COPY_SEMANTIC(DiagnosticConsumer);
    NO_MOVE_SEMANTIC(DiagnosticConsumer);

    virtual void HandleDiagnostic(DiagnosticSeverity severity, const Diagnostic &diagnostic) = 0;
};

//
class StreamDiagnosticConsumer final : public DiagnosticConsumer {
public:
    explicit StreamDiagnosticConsumer(std::ostream &stream) : stream_(stream) {}

    void HandleDiagnostic(DiagnosticSeverity severity, const Diagnostic &diagnostic) override;

private:
    std::ostream &stream_;
};

class MultiplexDiagnosticConsumer final : public DiagnosticConsumer {
public:
    MultiplexDiagnosticConsumer() = default;

    // Appends `consumer` to the end of the forwarding list. `consumer` must not be nullptr and must
    // outlive this MultiplexDiagnosticConsumer.
    void AddConsumer(DiagnosticConsumer *consumer);

    void HandleDiagnostic(DiagnosticSeverity severity, const Diagnostic &diagnostic) override;

private:
    std::vector<DiagnosticConsumer *> consumers_;
};

class DiagnosticEngine {
public:
    DiagnosticEngine() = default;
    explicit DiagnosticEngine(DiagnosticConsumer *consumer) : consumer_(consumer) {}

    NO_COPY_SEMANTIC(DiagnosticEngine);
    NO_MOVE_SEMANTIC(DiagnosticEngine);

    ~DiagnosticEngine() = default;

    // Installs (or, with nullptr, removes) the consumer notified of diagnostics reported from now
    // on. Does not affect diagnostics already reported/recorded before the call.
    void SetConsumer(DiagnosticConsumer *consumer);

    // Records `diagnostic` at `severity`, updates the running error/warning tally, and -- if one
    // is installed -- forwards it to the current DiagnosticConsumer.
    void Report(DiagnosticSeverity severity, Diagnostic diagnostic);

    // Convenience wrappers around Report() for each severity, so callers don't need to hand-build
    // a Diagnostic just to fill in its fields inline.
    void Error(uint32_t code, std::string description, std::string cause = "", std::string position = "",
               std::vector<std::string> solutions = {});
    void Warning(uint32_t code, std::string description, std::string cause = "", std::string position = "",
                 std::vector<std::string> solutions = {});
    void Note(uint32_t code, std::string description, std::string cause = "", std::string position = "",
              std::vector<std::string> solutions = {});

    // Same as above, taking a DiagnosticCode instead of a raw uint32_t so call sites don't need an
    // explicit static_cast.
    void Error(DiagnosticCode code, std::string description, std::string cause = "", std::string position = "",
               std::vector<std::string> solutions = {});
    void Warning(DiagnosticCode code, std::string description, std::string cause = "", std::string position = "",
                 std::vector<std::string> solutions = {});
    void Note(DiagnosticCode code, std::string description, std::string cause = "", std::string position = "",
              std::vector<std::string> solutions = {});

    // Whether any DiagnosticSeverity::ERROR has been reported so far -- the usual "should the
    // overall operation be considered failed?" check (equivalent to Clang's hasErrorOccurred()).
    bool HasErrors() const;
    std::size_t ErrorCount() const;
    std::size_t WarningCount() const;

    // Snapshot of every diagnostic reported so far, in report order.
    std::vector<DiagnosticRecord> Records() const;

    // Serializes every diagnostic reported so far into a JSON array of DiagnosticRecord, in report
    // order -- the payload gluegen's design doc's structured diagnostics file (e.g.
    // `foo.gluegen.diagnostics.json`) is built from.
    std::string Serialize() const;

    // Discards every diagnostic recorded so far and resets the error/warning tally. The installed
    // consumer (if any) is left untouched.
    void Clear();

private:
    mutable std::mutex mutex_;
    DiagnosticConsumer *consumer_ = nullptr;
    std::vector<DiagnosticRecord> records_;
    std::size_t errorCount_ = 0;
    std::size_t warningCount_ = 0;
};

}  // namespace ark::es2panda::gluegen

#undef DIAGNOSTIC_CODE_BASE
#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_DIAGNOSTIC_H