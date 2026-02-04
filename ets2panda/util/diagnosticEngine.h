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

#ifndef ES2PANDA_UTIL_DIAGNOSTIC_ENGINE_H
#define ES2PANDA_UTIL_DIAGNOSTIC_ENGINE_H

#include <memory>
#include <utility>
#include "es2panda.h"
#include "util/es2pandaMacros.h"
#include "generated/diagnostic.h"
#include "util/diagnostic.h"
#include "lexer/token/sourceLocation.h"

namespace ark::es2panda::util {

class DiagnosticPrinter {
public:
    DiagnosticPrinter() = default;
    NO_COPY_SEMANTIC(DiagnosticPrinter);
    NO_MOVE_SEMANTIC(DiagnosticPrinter);
    virtual ~DiagnosticPrinter() = default;

    virtual void Print(const DiagnosticBase &diagnostic, std::string basePath) const = 0;
    virtual void Print(const DiagnosticBase &diagnostic, std::ostream &out, std::string basePath) const = 0;
};

class CLIDiagnosticPrinter : public DiagnosticPrinter {
public:
    CLIDiagnosticPrinter() = default;
    NO_COPY_SEMANTIC(CLIDiagnosticPrinter);
    NO_MOVE_SEMANTIC(CLIDiagnosticPrinter);
    ~CLIDiagnosticPrinter() override = default;

    void Print(const DiagnosticBase &diagnostic, std::string basePath) const override;
    void Print(const DiagnosticBase &diagnostic, std::ostream &out, std::string basePath) const override;
};

using DiagnosticStorage = std::vector<std::shared_ptr<DiagnosticBase>>;

using DiagnosticCheckpoint = std::array<size_t, DiagnosticType::COUNT>;

class DiagnosticEngine {
public:
    explicit DiagnosticEngine() : printer_(std::make_unique<CLIDiagnosticPrinter>())
    {
        g_diagnosticEngine = this;
    }
    NO_COPY_SEMANTIC(DiagnosticEngine);
    NO_MOVE_SEMANTIC(DiagnosticEngine);
    ~DiagnosticEngine()
    {
        g_diagnosticEngine = nullptr;
    }

    // NOTE(schernykh): should be removed
    const DiagnosticBase &GetAnyError() const;

    DiagnosticCheckpoint Save() const;

    void Rollback(const DiagnosticCheckpoint &checkpoint);

    void UndoRange(const DiagnosticCheckpoint &from, const DiagnosticCheckpoint &to);

    [[nodiscard]] bool IsAnyError() const noexcept;

    void InsertLog(std::shared_ptr<DiagnosticBase> &&logInfo)
    {
        diagnostics_[logInfo->Type()].emplace_back(std::move(logInfo));
    }

    template <typename... T>
    Suggestion *CreateSuggestion(T &&...args)
    {
        return CreateDiagnostic<Suggestion>(std::forward<T>(args)...);
    }

    template <typename... T>
    Diagnostic *LogDiagnostic(T &&...args)
    {
        return LogDiagnostic<Diagnostic>(std::forward<T>(args)...);
    }

    // Make sure locations are accessible before all Programs mentioned in Positions
    // get invalidated.
    void EnsureLocations();

    void ClearDiagnostics()
    {
        for (auto &it : diagnostics_) {
            it.clear();
        }
    }

    // NOTE(schernykh): should be removed
    void Log(ThrowableDiagnostic const &error)
    {
        auto errCopy = error;
        errCopy.EnsureLocation();
        printer_->Print(errCopy, basePath_);
    };

    template <typename... T>
    void LogSyntaxError(T &&...args)
    {
        LogThrowableDiagnostic(DiagnosticType::SYNTAX, std::forward<T>(args)...);
    }
    template <typename... T>
    void LogSemanticError(T &&...args)
    {
        LogThrowableDiagnostic(DiagnosticType::SEMANTIC, std::forward<T>(args)...);
    }
    template <typename... T>
    void LogFatalError(T &&...args)
    {
        LogThrowableDiagnostic(DiagnosticType::FATAL, std::forward<T>(args)...);
    }

    // NOTE(schernykh): should not be able from ETS
    template <typename... T>
    [[noreturn]] void ThrowSyntaxError(T &&...args)
    {
        ThrowDiagnostic(DiagnosticType::SYNTAX, std::forward<T>(args)...);
    }
    template <typename... T>
    [[noreturn]] void ThrowSemanticError(T &&...args)
    {
        ThrowDiagnostic(DiagnosticType::SEMANTIC, std::forward<T>(args)...);
    }
    template <typename... T>
    [[noreturn]] void ThrowFatalError(T &&...args)
    {
        ThrowDiagnostic(DiagnosticType::FATAL, std::forward<T>(args)...);
    }

    void FlushDiagnostic();
    std::string PrintAndFlushErrorDiagnostic();
    void SetWError(bool wError)
    {
        wError_ = wError;
    }

    void SetBasePath(std::string basePath)
    {
        basePath_ = basePath;  // NOLINT(performance-unnecessary-value-param)
    }

    void CleanDuplicateLog(DiagnosticType type);

    const DiagnosticStorage &GetDiagnosticStorage(DiagnosticType type);

    static void InitializeSignalHandlers();

private:
    template <typename DIAGNOSTIC, typename... T>
    DIAGNOSTIC *CreateDiagnostic(T &&...args)
    {
        auto diag = std::make_unique<DIAGNOSTIC>(std::forward<T>(args)...);
        auto type = diag->Type();
        diagnostics_[type].push_back(std::move(diag));
        return reinterpret_cast<DIAGNOSTIC *>(diagnostics_[type].back().get());
    }

    template <typename DIAGNOSTIC, typename... T>
    DIAGNOSTIC *LogDiagnostic(T &&...args)
    {
        return CreateDiagnostic<DIAGNOSTIC>(std::forward<T>(args)...);
    }

    template <typename... T>
    ThrowableDiagnostic *LogThrowableDiagnostic(T &&...args)
    {
        return LogDiagnostic<ThrowableDiagnostic>(std::forward<T>(args)...);
    }

    template <typename... T>
    [[noreturn]] void ThrowDiagnostic(T &&...args) const
    {
        Throw(ThrowableDiagnostic {std::forward<T>(args)...});
    }
    [[noreturn]] void Throw(ThrowableDiagnostic diag) const;

    bool IsError(DiagnosticType type) const;
    DiagnosticStorage GetAllDiagnostic();
    DiagnosticStorage GetErrorDiagnostic();
    void WriteLog(const DiagnosticBase &error);

private:
    std::array<DiagnosticStorage, static_cast<size_t>(DiagnosticType::COUNT)> diagnostics_;
    std::unique_ptr<const DiagnosticPrinter> printer_;
    bool wError_ {false};
    // NOLINTNEXTLINE(readability-redundant-string-init)
    std::string basePath_ {""};
};

}  // namespace ark::es2panda::util

#endif  // ES2PANDA_UTIL_DIAGNOSTIC_ENGINE_H
