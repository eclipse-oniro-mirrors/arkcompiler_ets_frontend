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

#ifndef INFER_FUNCTION_H
#define INFER_FUNCTION_H

#include "ir/astNode.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "refactor_types.h"

namespace ark::es2panda::lsp {

constexpr RefactorActionView TO_INFER_FUNCTION_RETURN_TYPE {"Infer function return type", "Infer function return type",
                                                            "refactor.rewrite.function.returnType"};

struct FunctionInfo {
    ir::AstNode *declaration;
    ir::AstNode *returnTypeNode;
};
class InferFunctionRefactor : public Refactor {
public:
    InferFunctionRefactor();
    ApplicableRefactorInfo GetAvailableActions(const RefactorContext &context) const override;
    std::unique_ptr<RefactorEditInfo> GetEditsForAction(const RefactorContext &context,
                                                        const std::string &actionName) const override;
};
FunctionInfo GetInfoInferRet(const RefactorContext &context);
void DoChanges(ChangeTracker &changes, es2panda_Context *context, ir::AstNode *declaration, ir::AstNode *typeNode);

}  // namespace ark::es2panda::lsp

#endif  // INFER_FUNCTION_H
