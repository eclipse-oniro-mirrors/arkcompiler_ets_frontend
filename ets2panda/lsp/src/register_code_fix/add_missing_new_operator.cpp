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

#include "lsp/include/internal_api.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/register_code_fix/add_missing_new_operator.h"

namespace ark::es2panda::lsp {
const int G_ADD_MISSING_NEW_OPERATOR_CODE = 1022;
FixAddMissingNewOperator::FixAddMissingNewOperator()
{
    const char *fixId = "FixAddMissingNewOperator";
    SetErrorCodes({G_ADD_MISSING_NEW_OPERATOR_CODE});
    SetFixIds({fixId});
}

bool FixAddMissingNewOperator::IsValidTarget(const ir::AstNode *node)
{
    return node != nullptr && node->IsCallExpression();
}

void FixAddMissingNewOperator::MakeChange(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos,
                                          std::vector<ir::AstNode *> &fixedNodes)
{
    const auto *impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr) {
        return;
    }
    const ir::AstNode *callExpr = token;
    while (callExpr != nullptr && !callExpr->IsCallExpression()) {
        callExpr = callExpr->Parent();
    }
    if (!IsValidTarget(callExpr)) {
        return;
    }
    auto *call = const_cast<ir::CallExpression *>(callExpr->AsCallExpression());
    auto *callee = const_cast<ir::Expression *>(call->Callee());
    if (callee == nullptr) {
        return;
    }
    auto *calleeNode = reinterpret_cast<es2panda_AstNode *>(callee);
    if (calleeNode == nullptr) {
        return;
    }
    es2panda_AstNode *part = impl->CreateETSTypeReferencePart1(context, calleeNode);
    impl->AstNodeSetParent(context, calleeNode, part);
    es2panda_AstNode *typeRef = impl->CreateETSTypeReference(context, part);
    impl->AstNodeSetParent(context, part, typeRef);
    es2panda_AstNode *newExpr = impl->CreateETSNewClassInstanceExpression(context, typeRef, nullptr, 0);
    impl->AstNodeSetParent(context, typeRef, newExpr);
    auto *newExprNode = reinterpret_cast<ir::AstNode *>(newExpr);
    if (newExprNode == nullptr) {
        return;
    }
    newExprNode->SetParent(call->Parent());
    changeTracker.ReplaceNode(context, call, newExprNode, {});
    fixedNodes.push_back(newExprNode);
}

std::vector<FileTextChanges> FixAddMissingNewOperator::GetCodeActionsToFix(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    std::vector<ir::AstNode *> fixedNodes;
    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChange(tracker, context.context, context.span.start, fixedNodes);
    });
    return fileTextChanges;
}

std::vector<CodeFixAction> FixAddMissingNewOperator::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToFix(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = "addMissingNewOperator";
        codeAction.description = "Add missing 'new' operator to constructor call";
        codeAction.changes = changes;
        codeAction.fixId = "AddMissingNewOperator";
        returnedActions.push_back(codeAction);
    }
    return returnedActions;
}

CombinedCodeActions FixAddMissingNewOperator::GetAllCodeActions([[maybe_unused]] const CodeFixAllContext &codeFixAll)
{
    return {};
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixAddMissingNewOperator> g_addMissingNew("AddMissingNewOperator");
}  // namespace ark::es2panda::lsp