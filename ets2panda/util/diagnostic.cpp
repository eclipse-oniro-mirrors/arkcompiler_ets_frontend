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

#include "diagnostic.h"
#include <memory>
#include <vector>
#include "generated/diagnostic.h"
#include "lexer/token/sourceLocation.h"
#include "parser/program/program.h"
#include "checker/types/type.h"
#include "checker/types/signature.h"

namespace ark::es2panda::util {

std::string Format(const DiagnosticMessageElement &elem)
{
    if (std::holds_alternative<std::string>(elem)) {
        return std::get<std::string>(elem);
    }
    if (std::holds_alternative<std::string_view>(elem)) {
        return std::string(std::get<std::string_view>(elem));
    }
    if (std::holds_alternative<const char *const>(elem)) {
        return std::get<const char *const>(elem);
    }
    if (std::holds_alternative<StringView>(elem)) {
        return std::string(std::get<StringView>(elem));
    }
    if (std::holds_alternative<size_t>(elem)) {
        return std::to_string(std::get<size_t>(elem));
    }
    if (std::holds_alternative<lexer::TokenType>(elem)) {
        return TokenToString(std::get<lexer::TokenType>(elem));
    }
    if (std::holds_alternative<AsSrc>(elem)) {
        std::stringstream ss;
        std::get<AsSrc>(elem).GetType()->ToStringAsSrc(ss);
        return ss.str();
    }
    if (std::holds_alternative<const checker::Type *const>(elem)) {
        std::stringstream ss;
        std::get<const checker::Type *const>(elem)->ToString(ss);
        return ss.str();
    }
    if (std::holds_alternative<const checker::Signature *const>(elem)) {
        std::stringstream ss;
        std::get<const checker::Signature *const>(elem)->ToString(ss, nullptr, true);
        return ss.str();
    }
    ES2PANDA_UNREACHABLE();
}
std::string Format(const DiagnosticMessageParams &list)
{
    std::stringstream ss;
    for (const auto &it : list) {
        ss << Format(it);
    }
    return ss.str();
}

std::vector<std::string> FormatParams(const DiagnosticMessageParams &list)
{
    std::vector<std::string> params;
    params.reserve(list.size());
    for (const auto &it : list) {
        std::stringstream ss;
        ss << Format(it);
        params.push_back(ss.str());
    }
    return params;
}

std::string Format(std::string_view formatString, const std::vector<std::string> &params)
{
    std::string result;
    size_t pos = 0;
    size_t paramIndex = 0;
    while (pos < formatString.size()) {
        auto nextPos = formatString.find("{}", pos);
        if (nextPos == std::string::npos) {
            break;
        }
        result.append(formatString.substr(pos, nextPos - pos));
        ASSERT_PRINT(paramIndex < params.size(),
                     "Too few params (" + std::to_string(params.size()) + ") given for " + std::string(formatString));
        result.append(params.at(paramIndex++));
        pos = nextPos + 2U;
    }
    ASSERT_PRINT(paramIndex == params.size(), "Placeholder and param count mismatch in " + std::string(formatString));
    result.append(formatString.substr(pos));
    return result;
}

bool DiagnosticBase::operator<(const DiagnosticBase &rhs) const
{
    if (File() != rhs.File()) {
        return File() < rhs.File();
    }
    if (Line() != rhs.Line()) {
        return Line() < rhs.Line();
    }
    if (Offset() != rhs.Offset()) {
        return Offset() < rhs.Offset();
    }
    if (Type() != rhs.Type()) {
        return Type() < rhs.Type();
    }
    return false;
}

bool DiagnosticBase::operator==(const DiagnosticBase &rhs) const
{
    if (File() != rhs.File()) {
        return false;
    }
    if (Line() != rhs.Line()) {
        return false;
    }
    if (Offset() != rhs.Offset()) {
        return false;
    }
    if (Type() != rhs.Type()) {
        return false;
    }
    return Message() == rhs.Message();
}

DiagnosticType Diagnostic::Type() const
{
    return diagnosticKind_->Type();
}

uint32_t Diagnostic::GetId() const
{
    return diagnosticKind_->Id();
}

std::string Diagnostic::Message() const
{
    return Format(diagnosticKind_->Message(), diagnosticParams_);
}

const char *DiagnosticTypeToString(DiagnosticType type)
{
    switch (type) {
        case DiagnosticType::FATAL:
            return "Fatal error";
        case DiagnosticType::SYNTAX:
            return "SyntaxError";
        case DiagnosticType::SEMANTIC:
            return "TypeError";
        case DiagnosticType::WARNING:
            return "Warning";
        case DiagnosticType::PLUGIN_WARNING:
            return "Plugin warning";
        case DiagnosticType::PLUGIN_ERROR:
            return "Plugin error";
        case DiagnosticType::DECLGEN_ETS2TS_ERROR:
            return "Declgen ets2ts error";
        case DiagnosticType::DECLGEN_ETS2TS_WARNING:
            return "Declgen ets2ts warning";
        case DiagnosticType::ISOLATED_DECLGEN:
            return "Isolated declgen error";
        case DiagnosticType::ARKTS_CONFIG_ERROR:
            return "ArkTS config error";
        case DiagnosticType::SUGGESTION:
            return "SUGGESTION";
        default:
            ES2PANDA_UNREACHABLE();
    }
}

DiagnosticBase::DiagnosticBase(const lexer::SourcePosition &pos)
{
    if (pos.Program() != nullptr) {
        lexer::SourceLocation loc = pos.ToLocation();
        file_ = pos.Program()->SourceFilePath().Utf8();
        line_ = loc.line;
        offset_ = loc.col;
    }
}

DiagnosticBase::DiagnosticBase(const lexer::SourceLocation &loc)
{
    if (loc.Program() != nullptr) {
        file_ = loc.Program()->SourceFilePath().Utf8();
        line_ = loc.line;
        offset_ = loc.col;
    }
}

ThrowableDiagnostic::ThrowableDiagnostic(DiagnosticType type, const DiagnosticMessageParams &params,
                                         const lexer::SourceLocation &loc)
    : DiagnosticBase(loc), type_(type), message_(Format(params))
{
}

ThrowableDiagnostic::ThrowableDiagnostic(DiagnosticType type, const DiagnosticMessageParams &params,
                                         const lexer::SourcePosition &pos)
    : DiagnosticBase(pos), type_(type), message_(Format(params))
{
}

ThrowableDiagnostic::ThrowableDiagnostic(DiagnosticType type, const DiagnosticMessageParams &params,
                                         std::string_view file, size_t line, size_t offset)
    : DiagnosticBase(file, line, offset), type_(type), message_(Format(params))
{
}

ThrowableDiagnostic::ThrowableDiagnostic(const DiagnosticType type, const diagnostic::DiagnosticKind &diagnosticKind,
                                         const util::DiagnosticMessageParams &diagnosticParams,
                                         const lexer::SourcePosition &pos)
    : DiagnosticBase(pos), type_(type), message_(Format(diagnosticKind.Message(), FormatParams(diagnosticParams)))
{
}

Suggestion::Suggestion(const diagnostic::DiagnosticKind *kind, std::vector<std::string> &params,
                       const char *substitutionCode, const char *title, const lexer::SourceRange *range)
    : kind_(kind),
      substitutionCode_(substitutionCode),
      title_(title),
      message_(Format(kind->Message(), params)),
      range_(range)
{
}

DiagnosticType Suggestion::Type() const
{
    return kind_->Type();
}

uint32_t Suggestion::GetId() const
{
    return kind_->Id();
}

Diagnostic::Diagnostic(const diagnostic::DiagnosticKind &diagnosticKind,
                       const util::DiagnosticMessageParams &diagnosticParams, const lexer::SourcePosition &pos,
                       std::initializer_list<class Suggestion *> suggestions)
    : DiagnosticBase(pos), diagnosticKind_(&diagnosticKind), diagnosticParams_(FormatParams(diagnosticParams))
{
    if (suggestions.size() != 0) {
        suggestions_ = std::make_unique<std::vector<class Suggestion *>>();
        for (auto suggestion : suggestions) {
            suggestions_->emplace_back(suggestion);
        }
    }
}

Diagnostic::Diagnostic(const diagnostic::DiagnosticKind &diagnosticKind,
                       const util::DiagnosticMessageParams &diagnosticParams)
    : Diagnostic(diagnosticKind, diagnosticParams, lexer::SourcePosition(), {})
{
}

Diagnostic::Diagnostic(const diagnostic::DiagnosticKind &diagnosticKind,
                       const util::DiagnosticMessageParams &diagnosticParams, const lexer::SourcePosition &pos,
                       class Suggestion *suggestion)
    : Diagnostic(diagnosticKind, diagnosticParams, pos, {suggestion})
{
}

Diagnostic::Diagnostic(const diagnostic::DiagnosticKind &diagnosticKind,
                       const util::DiagnosticMessageParams &diagnosticParams, const lexer::SourcePosition &pos)
    : Diagnostic(diagnosticKind, diagnosticParams, pos, {})
{
}

}  // namespace ark::es2panda::util
