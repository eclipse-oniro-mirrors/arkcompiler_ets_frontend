/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_UTIL_DIAGNOSTIC_H
#define ES2PANDA_UTIL_DIAGNOSTIC_H

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>
#include "util/es2pandaMacros.h"
#include "util/ustring.h"
#include "generated/tokenType.h"

namespace ark::es2panda::diagnostic {
class DiagnosticKind;
}  // namespace ark::es2panda::diagnostic

namespace ark::es2panda::checker {
class Type;
class Signature;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::lexer {
class SourcePosition;
class SourceLocation;
}  // namespace ark::es2panda::lexer

namespace ark::es2panda::util {

enum DiagnosticType {
    BEGIN = 0,
    FATAL = BEGIN,
    SYNTAX,
    SEMANTIC,
    WARNING,
    PLUGIN_ERROR,
    PLUGIN_WARNING,
    DECLGEN_ETS2TS_ERROR,
    DECLGEN_ETS2TS_WARNING,
    ISOLATED_DECLGEN,
    ARKTS_CONFIG_ERROR,
    SUGGESTION,
    COUNT,
    INVALID = COUNT
};

const char *DiagnosticTypeToString(DiagnosticType type);

class DiagnosticBase {
public:
    explicit DiagnosticBase(std::string_view file = "", size_t line = 0, size_t offset = 0)
        : file_(file), line_(line), offset_(offset)
    {
    }

    explicit DiagnosticBase(const lexer::SourcePosition &pos);
    explicit DiagnosticBase(const lexer::SourceLocation &loc);

    DEFAULT_COPY_SEMANTIC(DiagnosticBase);
    DEFAULT_MOVE_SEMANTIC(DiagnosticBase);
    virtual ~DiagnosticBase() = default;

    virtual DiagnosticType Type() const = 0;
    virtual uint32_t GetId() const = 0;
    virtual std::string Message() const = 0;

    bool operator<(const DiagnosticBase &rhs) const;
    bool operator==(const DiagnosticBase &rhs) const;

    const std::string &File() const
    {
        return file_;
    }

    std::pair<size_t, size_t> GetLoc() const
    {
        return {line_, offset_};
    }

    size_t Line() const
    {
        return line_;
    }

    size_t Offset() const
    {
        return offset_;
    }

private:
    std::string file_;
    size_t line_ {};
    size_t offset_ {};
};

class AsSrc {
public:
    explicit AsSrc(const checker::Type *type) : type_(const_cast<checker::Type *>(type)) {}

    const checker::Type *GetType() const
    {
        return type_;
    }

private:
    checker::Type *type_;
};

using DiagnosticMessageElement =
    std::variant<std::string, std::string_view, const char *const, StringView, size_t, lexer::TokenType, AsSrc,
                 const checker::Type *const, const checker::Signature *const>;
using DiagnosticMessageParams = std::vector<DiagnosticMessageElement>;

struct DiagnosticWithParams {
    const diagnostic::DiagnosticKind &kind;     // NOLINT(readability-identifier-naming)
    const DiagnosticMessageParams params = {};  // NOLINT(readability-identifier-naming)
};

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ThrowableDiagnostic : public DiagnosticBase, public std::exception {
public:
    ThrowableDiagnostic() = default;
    ThrowableDiagnostic(DiagnosticType type, std::string_view message, std::string_view file = "", size_t line = 0,
                        size_t offset = 0)
        : DiagnosticBase(file, line, offset), type_(type), message_(message)
    {
    }
    ThrowableDiagnostic(DiagnosticType type, std::string_view message, const lexer::SourcePosition &pos)
        : DiagnosticBase(pos), type_(type), message_(message)
    {
    }
    ThrowableDiagnostic(DiagnosticType type, std::string_view message, const lexer::SourceLocation &loc)
        : DiagnosticBase(loc), type_(type), message_(message)
    {
    }
    ThrowableDiagnostic(DiagnosticType type, const DiagnosticMessageParams &params, const lexer::SourcePosition &pos);
    ThrowableDiagnostic(DiagnosticType type, const DiagnosticMessageParams &params, const lexer::SourceLocation &loc);
    ThrowableDiagnostic(DiagnosticType type, const DiagnosticMessageParams &params, std::string_view file = "",
                        size_t line = 0, size_t offset = 0);
    ThrowableDiagnostic(DiagnosticType type, const diagnostic::DiagnosticKind &diagnosticKind,
                        const util::DiagnosticMessageParams &diagnosticParams, const lexer::SourcePosition &pos);

    DEFAULT_COPY_SEMANTIC(ThrowableDiagnostic);
    DEFAULT_MOVE_SEMANTIC(ThrowableDiagnostic);
    ~ThrowableDiagnostic() override = default;

    DiagnosticType Type() const override
    {
        return type_;
    }

    std::string Message() const override
    {
        return message_;
    }

    uint32_t GetId() const override
    {
        return 0;
    }

private:
    DiagnosticType type_ {DiagnosticType::INVALID};
    std::string message_ {};
};

class Suggestion : public DiagnosticBase {
public:
    explicit Suggestion(const diagnostic::DiagnosticKind *kind, std::vector<std::string> &params,
                        const char *substitutionCode, const char *title, const lexer::SourceRange *range);

    const lexer::SourceRange *SourceRange() const
    {
        return range_;
    }

    std::string SubstitutionCode() const
    {
        return substitutionCode_;
    }

    std::string Title() const
    {
        return title_;
    }

    std::string Message() const override
    {
        return message_;
    }

    DiagnosticType Type() const override;

    uint32_t GetId() const override;

private:
    const diagnostic::DiagnosticKind *kind_;
    const std::string substitutionCode_;
    const std::string title_;
    const std::string message_;
    const lexer::SourceRange *range_;
};

class Diagnostic : public DiagnosticBase {
public:
    explicit Diagnostic(const diagnostic::DiagnosticKind &diagnosticKind,
                        const util::DiagnosticMessageParams &diagnosticParams);
    explicit Diagnostic(const diagnostic::DiagnosticKind &diagnosticKind,
                        const util::DiagnosticMessageParams &diagnosticParams, const lexer::SourcePosition &pos);
    explicit Diagnostic(const diagnostic::DiagnosticKind &diagnosticKind,
                        const util::DiagnosticMessageParams &diagnosticParams, const lexer::SourcePosition &pos,
                        Suggestion *suggestion);
    explicit Diagnostic(const diagnostic::DiagnosticKind &diagnosticKind,
                        const util::DiagnosticMessageParams &diagnosticParams, const lexer::SourcePosition &pos,
                        std::initializer_list<Suggestion *> suggestions);

    NO_COPY_SEMANTIC(Diagnostic);
    DEFAULT_MOVE_SEMANTIC(Diagnostic);
    ~Diagnostic() override = default;

    DiagnosticType Type() const override;
    uint32_t GetId() const override;
    std::string Message() const override;

    bool HasSuggestions() const
    {
        return suggestions_ != nullptr;
    }

    const std::vector<class Suggestion *> &Suggestion() const
    {
        return *suggestions_;
    }

private:
    const diagnostic::DiagnosticKind *diagnosticKind_;
    std::vector<std::string> diagnosticParams_ {};
    std::unique_ptr<std::vector<class Suggestion *>> suggestions_ {};
};
}  // namespace ark::es2panda::util

#endif  // ES2PANDA_UTIL_DIAGNOSTICE_H
