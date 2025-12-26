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

#ifndef EXTRACT_SYMBOL_H
#define EXTRACT_SYMBOL_H

#include <string>
#include "ir/astNode.h"
#include "refactor_types.h"

namespace ark::es2panda::lsp {
constexpr RefactorActionView EXTRACT_CONSTANT_ACTION_GLOBAL {
    "extract_constant_scope_2", "Extract Constant in Global Scope", "refactor.extract.constant"};
constexpr RefactorActionView EXTRACT_FUNCTION_ACTION_GLOBAL {
    "extract_function_scope_2", "Extract Function in Global Scope", "refactor.extract.function"};
constexpr RefactorActionView EXTRACT_FUNCTION_ACTION_CLASS {"extract_function_scope_1", "Extract Function in ",
                                                            "refactor.extract.function"};
constexpr RefactorActionView EXTRACT_CONSTANT_ACTION_CLASS {"extract_constant_scope_1", "Extract Constant in ",
                                                            "refactor.extract.constant"};
constexpr RefactorActionView EXTRACT_CONSTANT_ACTION_ENCLOSE {
    "extract_constant_scope_0", "Extract Constant in Enclose Scope", "refactor.extract.constant"};
constexpr RefactorActionView EXTRACT_VARIABLE_ACTION_GLOBAL {
    "extract_variable_scope_2", "Extract Variable in Global Scope", "refactor.extract.variable"};
constexpr RefactorActionView EXTRACT_VARIABLE_ACTION_ENCLOSE {
    "extract_variable_scope_0", "Extract Variable in Enclose Scope", "refactor.extract.variable"};

struct RangeToExtract {
    TextRange range;
    std::string error;
};

struct FunctionExtraction {
    ir::AstNode *node = nullptr;
    TextRange targetRange = {};
    std::string description;
    std::vector<ir::ETSParameterExpression *> parameters;
    std::vector<std::string> freeVars;
};

const auto REFACTOR_NAME = "ExtractSymbolRefactor";
const auto REFACTOR_DESCRIPTION = "Extract Symbol";

class ExtractSymbolRefactor : public Refactor {
public:
    ExtractSymbolRefactor();
    std::vector<ApplicableRefactorInfo> GetAvailableActions(const RefactorContext &context) const override;
    std::unique_ptr<RefactorEditInfo> GetEditsForAction(const RefactorContext &context,
                                                        const std::string &actionName) const override;
};

}  // namespace ark::es2panda::lsp

#endif