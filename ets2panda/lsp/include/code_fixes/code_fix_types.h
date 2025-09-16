/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CODE_FIX_TYPES_H
#define CODE_FIX_TYPES_H

#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../user_preferences.h"
#include "../cancellation_token.h"
#include "../types.h"
#include "../api.h"
#include "es2panda.h"
#include "generated/code_fix_register.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "../get_class_property_info.h"
#include "lsp/include/services/text_change/text_change_context.h"

namespace ark::es2panda::lsp {

enum DiagnosticCategory { WARNING, ERROR, SUGGESTION, MESSAGE };

struct DiagnosticMessage {
    std::string key;
    DiagnosticCategory category;
    size_t code;
    std::string message;
    std::string reportsUnnecessary = {};
    std::string reportsDeprecated = {};
    bool elidedInCompatabilityPyramid;
};
struct DiagnosticAndArguments {
    DiagnosticMessage message;
    std::vector<std::string> arguments;
};

struct CodeAction {
    std::string description;
    std::vector<FileTextChanges> changes;
    std::vector<CodeActionCommand> commands;
};
struct CombinedCodeActions {
    std::vector<FileTextChanges> changes;
    std::vector<CodeActionCommand> commands;
};
struct CodeFixAction : CodeAction {
    std::string fixName;
    std::string fixId = {};
    std::string fixAllDescription = {};
};

struct CodeFixContextBase : TextChangesContext {
    es2panda_Context *context;
    CancellationToken cancellationToken;
};

struct CodeFixAllContext : CodeFixContextBase {
    std::string fixId = {};
};

class DiagnosticWithLocation : Diagnostic {
public:
    DiagnosticWithLocation(Diagnostic &&base, const SourceFile &file, const size_t start = 0, const size_t length = 0)
        : Diagnostic(std::move(base)), file_(file), start_(start), length_(length)
    {
    }

    using Diagnostic::Diagnostic;

    SourceFile GetFile() const
    {
        return file_;
    }

    size_t GetStart() const
    {
        return start_;
    }

    size_t Getlength() const
    {
        return length_;
    }

private:
    SourceFile file_;
    size_t start_ = 0;
    size_t length_ = 0;
};

struct CodeFixContext : CodeFixContextBase {
    int errorCode = 0;
    TextSpan span = {0, 0};
};

class CodeFixRegistration {
private:
    std::vector<int> errorCodes_;
    std::vector<std::string> fixIds_;

public:
    std::vector<int> GetErrorCodes() const
    {
        return errorCodes_;
    }
    std::vector<std::string> GetFixIds() const
    {
        return fixIds_;
    }
    void SetErrorCodes(const std::vector<int> &codes)
    {
        errorCodes_ = codes;
    }

    void SetFixIds(const std::vector<std::string> &ids)
    {
        fixIds_ = ids;
    }
    virtual std::vector<CodeFixAction> GetCodeActions(const CodeFixContext &) = 0;
    virtual CombinedCodeActions GetAllCodeActions(const CodeFixAllContext &) = 0;
    CodeFixRegistration() = default;
    CodeFixRegistration(const CodeFixRegistration &) = delete;
    CodeFixRegistration &operator=(const CodeFixRegistration &) = delete;
    CodeFixRegistration(CodeFixRegistration &&) = delete;
    CodeFixRegistration &operator=(CodeFixRegistration &&) = delete;
    virtual ~CodeFixRegistration() = default;
};

}  // namespace ark::es2panda::lsp

#endif
