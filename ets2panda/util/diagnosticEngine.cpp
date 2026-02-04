/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "diagnosticEngine.h"
#include <memory>
#include "util/diagnostic.h"
#include "util/helpers.h"
#include "util/options.h"

#include <csignal>

namespace ark::es2panda::util {

void CLIDiagnosticPrinter::Print(const DiagnosticBase &diagnostic, std::ostream &out, std::string basePath) const
{
    // Message collected and printed once to avoid spliting printed message in case multithreading execution
    std::ostringstream stream;
    // NOLINTNEXTLINE(readability-redundant-string-init)
    std::string fileName = "";
    if (!basePath.empty()) {
        fileName = util::Helpers::CalcRelativePath(diagnostic.File(), basePath);
    }
    // if basePath is empty, it means basepath in cmdline is not setted, then output base name of file
    // if fileName is empty, it means basePath in cmdline option is wrong, maybe basepath is not a real path in system
    if (fileName.empty() || basePath.empty()) {
        fileName = util::BaseName(diagnostic.File());
    }
    if (!diagnostic.File().empty()) {
        stream << "[" << fileName << ":" << diagnostic.Line() << ":" << diagnostic.Offset() << "] ";
    }
    stream << DiagnosticTypeToString(diagnostic.Type()) << " " << diagnostic.ToStringUniqueNumber() << ": "
           << diagnostic.Message() << std::endl;
    out << stream.str();
}

void CLIDiagnosticPrinter::Print(const DiagnosticBase &diagnostic, std::string basePath) const
{
    // NOTE(pronai) test harness is not prepared for cerr
    Print(diagnostic, std::cout, basePath);
}

void DiagnosticEngine::CleanDuplicateLog(DiagnosticType type)
{
    DiagnosticStorage &log = diagnostics_[type];
    std::sort(log.begin(), log.end(), [](const auto &lhs, const auto &rhs) { return *lhs < *rhs; });
    auto last =
        std::unique(log.begin(), log.end(), [&](const auto &rhs, const auto &lhs) -> bool { return *rhs == *lhs; });
    log.resize(std::distance(log.begin(), last));
}

const DiagnosticStorage &DiagnosticEngine::GetDiagnosticStorage(DiagnosticType type)
{
    return diagnostics_[type];
}

[[nodiscard]] DiagnosticCheckpoint DiagnosticEngine::Save() const
{
    DiagnosticCheckpoint cp;
    for (size_t i = 0; i < diagnostics_.size(); i++) {
        cp[i] = diagnostics_[i].size();
    }
    return cp;
}

void DiagnosticEngine::Rollback(const DiagnosticCheckpoint &checkpoint)
{
    for (size_t i = 0; i < diagnostics_.size(); i++) {
        diagnostics_[i].resize(checkpoint[i]);
    }
}

void DiagnosticEngine::UndoRange(const DiagnosticCheckpoint &from, const DiagnosticCheckpoint &to)
{
    for (size_t i = 0; i < diagnostics_.size(); i++) {
        diagnostics_[i].erase(diagnostics_[i].begin() + from[i], diagnostics_[i].begin() + to[i]);
    }
}

[[noreturn]] void DiagnosticEngine::Throw(ThrowableDiagnostic diag) const
{
    throw diag;
}

void DiagnosticEngine::EnsureLocations()
{
    for (auto &kindVec : diagnostics_) {
        for (auto &diag : kindVec) {
            diag->EnsureLocation();
        }
    }
}

DiagnosticStorage DiagnosticEngine::GetAllDiagnostic()
{
    size_t totalSize = 0;
    for (const auto &vec : diagnostics_) {
        totalSize += vec.size();
    }

    DiagnosticStorage merged;
    merged.reserve(totalSize);
    for (auto &vec : diagnostics_) {
        for (auto &diag : vec) {
            merged.emplace_back(diag);
        }
    }
    return merged;
}

DiagnosticStorage DiagnosticEngine::GetErrorDiagnostic()
{
    size_t errorCount = 0;
    for (const auto &vec : diagnostics_) {
        if (!vec.empty() && IsError(vec.front()->Type())) {
            errorCount += vec.size();
        }
    }

    DiagnosticStorage merged;
    merged.reserve(errorCount);
    for (const auto &vec : diagnostics_) {
        if (!vec.empty() && IsError(vec.front()->Type())) {
            merged.insert(merged.end(), vec.begin(), vec.end());
        }
    }
    return merged;
}

std::string DiagnosticEngine::PrintAndFlushErrorDiagnostic()
{
    EnsureLocations();
    auto log = GetErrorDiagnostic();
    std::sort(log.begin(), log.end(), [](const auto &lhs, const auto &rhs) { return *lhs < *rhs; });
    auto last = std::unique(log.begin(), log.end(), [](const auto &lhs, const auto &rhs) { return *lhs == *rhs; });
    std::ostringstream oss;
    for (auto it = log.begin(); it != last; ++it) {
        printer_->Print(**it, oss, basePath_);
    }
    return oss.str();
}

void DiagnosticEngine::FlushDiagnostic()
{
    EnsureLocations();
    auto log = GetAllDiagnostic();
    std::sort(log.begin(), log.end(), [](const auto &lhs, const auto &rhs) { return *lhs < *rhs; });
    auto last =
        std::unique(log.begin(), log.end(), [&](const auto &rhs, const auto &lhs) -> bool { return *rhs == *lhs; });
    for (auto it = log.begin(); it != last; it++) {
        printer_->Print(**it, basePath_);
    }
    for (auto &vec : diagnostics_) {
        vec.clear();
    }
}
#ifndef FUZZING_EXIT_ON_FAILED_ASSERT
static void SigSegvHandler([[maybe_unused]] int sig)
{
    CompilerBugAction(lexer::SourcePosition {});
    ark::PrintStack(ark::GetStacktrace(), std::cerr);
    std::abort();  // CC-OFF(G.STD.16-CPP) fatal error
}
#endif

void DiagnosticEngine::InitializeSignalHandlers()
{
#ifndef FUZZING_EXIT_ON_FAILED_ASSERT
    std::signal(SIGSEGV, SigSegvHandler);
#endif
}

bool DiagnosticEngine::IsAnyError() const noexcept
{
    for (size_t i = DiagnosticType::BEGIN; i < DiagnosticType::COUNT; ++i) {
        if (IsError(static_cast<DiagnosticType>(i)) && !diagnostics_[i].empty()) {
            return true;
        }
    }
    return false;
}

const DiagnosticBase &DiagnosticEngine::GetAnyError() const
{
    ES2PANDA_ASSERT(IsAnyError());
    for (size_t i = DiagnosticType::BEGIN; i < DiagnosticType::COUNT; ++i) {
        if (IsError(static_cast<DiagnosticType>(i)) && !diagnostics_[i].empty()) {
            return *diagnostics_[i].front();
        }
    }
    ES2PANDA_UNREACHABLE();
}

bool DiagnosticEngine::IsError(DiagnosticType type) const
{
    switch (type) {
        case DiagnosticType::FATAL:
        case DiagnosticType::SYNTAX:
        case DiagnosticType::SEMANTIC:
        case DiagnosticType::PLUGIN_ERROR:
        case DiagnosticType::DECLGEN_ETS2TS_ERROR:
        case DiagnosticType::ISOLATED_DECLGEN:
        case DiagnosticType::ARKTS_CONFIG_ERROR:
            return true;
        case DiagnosticType::WARNING:
        case DiagnosticType::DECLGEN_ETS2TS_WARNING:
        case DiagnosticType::PLUGIN_WARNING:
            return wError_;
        case DiagnosticType::SUGGESTION:
            return false;
        default:
            ES2PANDA_UNREACHABLE();
    }
}

}  // namespace ark::es2panda::util
