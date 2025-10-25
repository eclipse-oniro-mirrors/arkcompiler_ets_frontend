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

#ifndef REFACTOR_TYPES_H
#define REFACTOR_TYPES_H

#include "../formatting/formatting_settings.h"
#include "../formatting/formatting.h"
#include "public/es2panda_lib.h"
#include "../cancellation_token.h"
#include "../user_preferences.h"
#include "../types.h"
#include "es2panda.h"
#include "lsp/include/services/text_change/text_change_context.h"
#include <string>
#include <string_view>
#include <vector>

namespace ark::es2panda::lsp {

struct RefactorEditInfo {
private:
    std::vector<FileTextChanges> fileTextChanges_;

public:
    explicit RefactorEditInfo(std::vector<FileTextChanges> fileTextChanges = {})
        : fileTextChanges_(std::move(fileTextChanges))
    {
    }
    const std::vector<FileTextChanges> &GetFileTextChanges() const
    {
        return fileTextChanges_;
    }
    void SetFileTextChanges(const std::vector<FileTextChanges> &fileTextChanges)
    {
        fileTextChanges_ = fileTextChanges;
    }
    void AddFileTextChange(const FileTextChanges &change)
    {
        fileTextChanges_.emplace_back(change);
    }
};

struct TextRange {
    size_t pos;
    size_t end;
};

struct RefactorContext {
    TextChangesContext *textChangesContext = nullptr;
    CancellationToken *cancellationToken = nullptr;
    TextRange span = {0, 0};
    es2panda_Context *context = nullptr;
    std::string kind;
};

struct RefactorActionView {
    std::string_view name;
    std::string_view description;
    std::string_view kind;
};

struct RefactorAction {
    std::string name;
    std::string description;
    std::string kind;
};

struct ApplicableRefactorInfo {
    std::string name;
    std::string description;
    RefactorAction action;
};

namespace refactor_name {
constexpr std::string_view CONVERT_FUNCTION_REFACTOR_NAME = "Convert arrow function or function expression";
constexpr std::string_view CONVERT_EXPORT_REFACTOR_NAME = "ConvertExportRefactor";
constexpr std::string_view CONVERT_IMPORT_REFACTOR_NAME = "Convert import";
constexpr std::string_view CONVERT_TEMPLATE_REFACTOR_NAME = "Convert to template string";
constexpr std::string_view CONVERT_CHAIN_REFACTOR_NAME = "ConvertToOptionalChainExpressionRefactor";
constexpr std::string_view GENERATE_OVERRIDE_METHODS_NAME = "Generate override methods";
constexpr std::string_view CONVERT_FUNCTION_TO_CLASS_NAME = "ConvertFunctionToClassRefactor";
constexpr std::string_view EXTRACT_TYPE_NAME = "ExtractTypeRefactor";
constexpr std::string_view INFER_FUNCTION_RETURN_TYPE = "Infer function return type";

constexpr std::string_view EXTRACT_CONSTANT_ACTION_NAME = "ExtractSymbolRefactor";
constexpr std::string_view EXTRACT_FUNCTION_ACTION_NAME = "ExtractSymbolRefactor";
constexpr std::string_view EXTRACT_VARIABLE_ACTION_NAME = "ExtractSymbolRefactor";

constexpr std::string_view CONVERT_OVERLOAD_LIST_REFACTOR_NAME = "Convert overload list to single signature";
constexpr std::string_view CONVERT_PARAMS_TO_OBJECT = "Convert parameters to object and introduce interface";
constexpr std::string_view GENERATE_CONSTRUCTOR_REFACTOR_NAME = "Generate Constructor";

}  // namespace refactor_name

namespace refactor_description {
constexpr std::string_view CONVERT_FUNCTION_REFACTOR_DESC = "Convert arrow function or function expression";
constexpr std::string_view CONVERT_TEMPLATE_REFACTOR_DESC = "Convert to template string";
constexpr std::string_view CONVERT_CHAIN_REFACTOR_DESC = "Convert to optional chain expression";
constexpr std::string_view GENERATE_OVERRIDE_METHODS_DESC = "Generate override methods";
constexpr std::string_view CONVERT_IMPORT_REFACTOR_DESC = "Convert to named import";
constexpr std::string_view CONVERT_FUNCTION_TO_CLASS_DESC =
    "Convert a standalone function, arrow function, or function expression into a class declaration";
constexpr std::string_view EXTRACT_TYPE_DESC = "Extract selected type";
constexpr std::string_view INFER_FUNCTION_RETURN_TYPE_DESC = "Infer function return type";

constexpr std::string_view EXTRACT_CONSTANT_ACTION_DESC = "Extract Constant";
constexpr std::string_view EXTRACT_FUNCTION_ACTION_DESC = "Extract Function";
constexpr std::string_view EXTRACT_VARIABLE_ACTION_DESC = "Extract Variable";

constexpr std::string_view CONVERT_OVERLOAD_LIST_REFACTOR_DESC =
    "Convert multiple function overloads to a single signature with union types";
constexpr std::string_view CONVERT_PARAMS_TO_OBJECT_DESC =
    "Convert multiple function parameters to a single object parameter with an interface";
constexpr std::string_view GENERATE_CONSTRUCTOR_REFACTOR_DESC = "Generate Constructor";
}  // namespace refactor_description

class Refactor {
private:
    std::vector<std::string> kinds_;

public:
    bool IsKind(const std::string &kind) const;
    void AddKind(const std::string &kind);
    virtual std::vector<ApplicableRefactorInfo> GetAvailableActions(const RefactorContext &context) const = 0;

    virtual std::unique_ptr<RefactorEditInfo> GetEditsForAction(const RefactorContext &context,
                                                                const std::string &actionName) const = 0;
    virtual ~Refactor() = default;
    Refactor() = default;
    Refactor &operator=(const Refactor &other);
    Refactor &operator=(Refactor &&other);
    Refactor(const Refactor &other);
    Refactor(Refactor &&other);
};

}  // namespace ark::es2panda::lsp

#endif  // REFACTOR_TYPES_H