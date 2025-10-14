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

#ifndef CONVERT_FUNCTION_H
#define CONVERT_FUNCTION_H

#include "ir/astNode.h"
#include "ir/expression.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/statement.h"
#include "ir/statements/functionDeclaration.h"
#include "ir/statements/variableDeclaration.h"
#include "ir/statements/variableDeclarator.h"
#include "refactor_types.h"

namespace ark::es2panda::lsp {

constexpr RefactorActionView TO_ANONYMOUS_FUNCTION_ACTION {
    "Convert to anonymous function", "Convert to anonymous function", "refactor.rewrite.function.anonymous"};
constexpr RefactorActionView TO_NAMED_FUNCTION_ACTION {"Convert to named function", "Convert to named function",
                                                       "refactor.rewrite.function.named"};
constexpr RefactorActionView TO_ARROW_FUNCTION_ACTION {"Convert to arrow function", "Convert to arrow function",
                                                       "refactor.rewrite.function.arrow"};

struct FunctionInfo {
    bool selectedVariableDeclaration;
    ir::AstNode *func;
};

struct VariableInfo {
    const ir::VariableDeclaration *variableDeclaration;
    // const ir::VariableDeclarationList variableDeclarationList;
    const ir::Statement *statement;
    const ir::Identifier *name;
};

class ConvertFunctionRefactor : public Refactor {
public:
    ConvertFunctionRefactor();
    std::vector<ApplicableRefactorInfo> GetAvailableActions(const RefactorContext &context) const override;
    std::unique_ptr<RefactorEditInfo> GetEditsForAction(const RefactorContext &context,
                                                        const std::string &actionName) const override;
};

ir::Expression *TryGetFunctionFromVariableDeclaration(ir::AstNode *parent);
FunctionInfo GetFunctionInfo(es2panda_Context *context, const size_t startPosition);
std::optional<VariableInfo> GetVariableInfo(ir::AstNode *func);
std::vector<FileTextChanges> GetEditInfoForConvertToAnonymousFunction(RefactorContext context, ir::Expression *func);
std::vector<FileTextChanges> GetEditInfoForConvertToNamedFunction(RefactorContext context,
                                                                  ir::ArrowFunctionExpression *const arrow,
                                                                  VariableInfo info);
std::vector<FileTextChanges> GetEditInfoForConvertToArrowFunction(RefactorContext context,
                                                                  ir::FunctionDeclaration *func);
RefactorEditInfo GetRefactorEditsToConvertFunctionExpressions(RefactorContext &context, std::string_view actionName);
}  // namespace ark::es2panda::lsp
#endif  // CONVERT_FUNCTION_H