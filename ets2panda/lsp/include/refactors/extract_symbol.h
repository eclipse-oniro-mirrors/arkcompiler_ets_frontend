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

constexpr RefactorActionView EXTRACT_CONSTANT_ACTION {"Extract Constant", "Extract Constant",
                                                      "refactor.extract.constant"};
constexpr RefactorActionView EXTRACT_FUNCTION_ACTION {"Extract Function", "Extract Function",
                                                      "refactor.extract.function"};
constexpr RefactorActionView EXTRACT_VARIABLE_ACTION {"Extract Variable", "Extract Variable",
                                                      "refactor.extract.variable"};

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

const auto REFACTOR_NAME = "Extract Symbol";

class ExtractSymbolRefactor : public Refactor {
public:
    ExtractSymbolRefactor();
    ApplicableRefactorInfo GetAvailableActions(const RefactorContext &context) const override;
    std::unique_ptr<RefactorEditInfo> GetEditsForAction(const RefactorContext &context,
                                                        const std::string &actionName) const override;
};
std::vector<FunctionExtraction> GetPossibleFunctionExtractions(const RefactorContext &context);
std::string GenerateInlineEdits(const RefactorContext &context, ir::AstNode *extractedText);
void CollectFunctionParameters(FunctionExtraction &funExt);
std::string BuildFunctionText(const FunctionExtraction &candidate, const RefactorContext &context);
ir::AstNode *FindRefactor(const RefactorContext &context);
std::string ReplaceWithFunctionCall(const FunctionExtraction &candidate, const std::string &functionText);

}  // namespace ark::es2panda::lsp

#endif  // CONVERT_EXPORT_H
