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

#include "diagnostic.h"

#include <mutex>
#include <utility>

namespace ark::es2panda::gluegen {

namespace {
const char *SeverityLabel(DiagnosticSeverity severity)
{
    switch (severity) {
        case DiagnosticSeverity::ERROR:
            return "error";
        case DiagnosticSeverity::WARNING:
            return "warning";
        case DiagnosticSeverity::NOTE:
            return "note";
    }
    UNREACHABLE();
}
}  // namespace

void StreamDiagnosticConsumer::HandleDiagnostic(DiagnosticSeverity severity, const Diagnostic &diagnostic)
{
    // "error[1001]: failed to resolve import" -- a Rust-style headline, since it puts the two
    // things a reader needs first (how bad, which one) ahead of the free-form description.
    stream_ << SeverityLabel(severity) << "[" << diagnostic.code << "]: " << diagnostic.description << '\n';
    if (!diagnostic.position.empty()) {
        stream_ << "  --> " << diagnostic.position << '\n';
    }
    if (!diagnostic.cause.empty()) {
        stream_ << "  cause: " << diagnostic.cause << '\n';
    }
    if (!diagnostic.solutions.empty()) {
        stream_ << "  solutions:\n";
        for (const auto &solution : diagnostic.solutions) {
            stream_ << "    - " << solution << '\n';
        }
    }
}

void MultiplexDiagnosticConsumer::AddConsumer(DiagnosticConsumer *consumer)
{
    consumers_.push_back(consumer);
}

void MultiplexDiagnosticConsumer::HandleDiagnostic(DiagnosticSeverity severity, const Diagnostic &diagnostic)
{
    for (auto *consumer : consumers_) {
        consumer->HandleDiagnostic(severity, diagnostic);
    }
}

void DiagnosticEngine::SetConsumer(DiagnosticConsumer *consumer)
{
    std::lock_guard<std::mutex> lock(mutex_);
    consumer_ = consumer;
}

void DiagnosticEngine::Report(DiagnosticSeverity severity, Diagnostic diagnostic)
{
    // The consumer is invoked outside of mutex_ (after copying out the pointer and the just-built
    // record) so a consumer that itself calls back into this engine -- e.g. to report a follow-up
    // note -- can't deadlock on a mutex this thread already holds.
    DiagnosticConsumer *consumer = nullptr;
    DiagnosticRecord record {severity, std::move(diagnostic)};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        switch (severity) {
            case DiagnosticSeverity::ERROR:
                ++errorCount_;
                break;
            case DiagnosticSeverity::WARNING:
                ++warningCount_;
                break;
            case DiagnosticSeverity::NOTE:
                break;
        }
        consumer = consumer_;
        records_.push_back(record);
    }
    if (consumer != nullptr) {
        consumer->HandleDiagnostic(record.severity, record.diagnostic);
    }
}

void DiagnosticEngine::Error(uint32_t code, std::string description, std::string cause, std::string position,
                             std::vector<std::string> solutions)
{
    Report(DiagnosticSeverity::ERROR,
           Diagnostic {code, std::move(description), std::move(cause), std::move(position), std::move(solutions)});
}

void DiagnosticEngine::Warning(uint32_t code, std::string description, std::string cause, std::string position,
                               std::vector<std::string> solutions)
{
    Report(DiagnosticSeverity::WARNING,
           Diagnostic {code, std::move(description), std::move(cause), std::move(position), std::move(solutions)});
}

void DiagnosticEngine::Note(uint32_t code, std::string description, std::string cause, std::string position,
                            std::vector<std::string> solutions)
{
    Report(DiagnosticSeverity::NOTE,
           Diagnostic {code, std::move(description), std::move(cause), std::move(position), std::move(solutions)});
}

void DiagnosticEngine::Error(DiagnosticCode code, std::string description, std::string cause, std::string position,
                             std::vector<std::string> solutions)
{
    Error(static_cast<uint32_t>(code), std::move(description), std::move(cause), std::move(position),
          std::move(solutions));
}

void DiagnosticEngine::Warning(DiagnosticCode code, std::string description, std::string cause, std::string position,
                               std::vector<std::string> solutions)
{
    Warning(static_cast<uint32_t>(code), std::move(description), std::move(cause), std::move(position),
            std::move(solutions));
}

void DiagnosticEngine::Note(DiagnosticCode code, std::string description, std::string cause, std::string position,
                            std::vector<std::string> solutions)
{
    Note(static_cast<uint32_t>(code), std::move(description), std::move(cause), std::move(position),
         std::move(solutions));
}

bool DiagnosticEngine::HasErrors() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return errorCount_ > 0;
}

std::size_t DiagnosticEngine::ErrorCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return errorCount_;
}

std::size_t DiagnosticEngine::WarningCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return warningCount_;
}

std::vector<DiagnosticRecord> DiagnosticEngine::Records() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return records_;
}

std::string DiagnosticEngine::Serialize() const
{
    nlohmann::json j = Records();
    constexpr int indent = 2;  // human-readable, not compact
    return j.dump(indent);
}

void DiagnosticEngine::Clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
    errorCount_ = 0;
    warningCount_ = 0;
}

}  // namespace ark::es2panda::gluegen
