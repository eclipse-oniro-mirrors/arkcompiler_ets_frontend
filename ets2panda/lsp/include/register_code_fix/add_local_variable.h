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

#ifndef ADD_LOCAL_VARIABLE_H
#define ADD_LOCAL_VARIABLE_H

#include <cstddef>
#include <string>

#include "lsp/include/code_fixes/code_fix_types.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp/include/types.h"

namespace ark::es2panda::lsp {

class AddLocalVariable : public CodeFixRegistration {
public:
    AddLocalVariable();

    std::vector<CodeFixAction> GetCodeActions(const CodeFixContext &context) override;
    CombinedCodeActions GetAllCodeActions(const CodeFixAllContext &codeFixAll) override;

private:
    void MakeChangeForAddLocalVariable(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos);
    std::vector<FileTextChanges> GetCodeActionsToAddLocalVariable(const CodeFixContext &context);

    std::string DetermineVariableType(ir::AstNode *unresolvedNode);
    std::string GetTypeFromDirectAssignment(ir::AstNode *unresolvedNode, ir::AstNode *parent);
    std::string GetTypeFromMemberAssignment(ir::AstNode *unresolvedNode, ir::AstNode *parent);
    std::string InferTypeFromExpression(ir::AstNode *expression);
    std::string InferTypeFromLiteral(ir::AstNode *expression);
    std::string InferTypeFromComplexExpression(ir::AstNode *expression);
    std::string InferTypeFromBinaryExpression(ir::AstNode *expression);
    std::string InferTypeFromOtherExpressions(ir::AstNode *expression);
    std::string GenerateVariableDeclaration(const std::string &variableName, const std::string &variableType);
    ir::AstNode *FindInsertionPoint(ir::AstNode *unresolvedNode, bool isThisProperty);
    ir::AstNode *FindClassInsertionPoint(ir::AstNode *current);
    ir::AstNode *FindFunctionInsertionPoint(ir::AstNode *current);
    ir::AstNode *GetFunctionBody(ir::AstNode *node);
    bool IsThisPropertyAccess(es2panda_Context *context, size_t pos);
};

}  // namespace ark::es2panda::lsp

#endif