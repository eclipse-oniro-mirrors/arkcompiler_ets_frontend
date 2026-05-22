/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "lsp/include/register_code_fix/remove_catch_type_annotation.h"

#include <string>

#include "generated/code_fix_register.h"
#include "ir/base/catchClause.h"
#include "ir/expressions/identifier.h"
#include "ir/typeNode.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"

namespace ark::es2panda::lsp {
using codefixes::REMOVE_CATCH_TYPE_ANNOTATION;

void RemoveCatchTypeAnnotation::MakeChange(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr) {
        return;
    }

    // Find the CatchClause ancestor
    auto *catchClause =
        static_cast<ir::CatchClause *>(FindAncestor(token, [](ir::AstNode *node) { return node->IsCatchClause(); }));
    if (catchClause == nullptr) {
        return;
    }

    auto *param = catchClause->Param();
    if (param == nullptr || !param->IsIdentifier()) {
        return;
    }

    auto *identifier = param->AsIdentifier();
    auto *typeAnnotation = identifier->TypeAnnotation();
    if (typeAnnotation == nullptr) {
        return;
    }

    // Delete range from end of identifier to end of type annotation
    // This covers ": Error" including the colon and any whitespace
    size_t deleteStart = identifier->End().index;
    size_t deleteEnd = typeAnnotation->End().index;
    if (deleteStart >= deleteEnd) {
        return;
    }

    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    changeTracker.DeleteRange(astContext->sourceFile, {deleteStart, deleteEnd});
}

std::vector<FileTextChanges> RemoveCatchTypeAnnotation::GetCodeActionsToRemoveCatchTypeAnnotation(
    const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    return ChangeTracker::With(
        textChangesContext, [&](ChangeTracker &tracker) { MakeChange(tracker, context.context, context.span.start); });
}

RemoveCatchTypeAnnotation::RemoveCatchTypeAnnotation()
{
    auto errorCodes = REMOVE_CATCH_TYPE_ANNOTATION.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({REMOVE_CATCH_TYPE_ANNOTATION.GetFixId().data()});
}

std::vector<CodeFixAction> RemoveCatchTypeAnnotation::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToRemoveCatchTypeAnnotation(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = REMOVE_CATCH_TYPE_ANNOTATION.GetFixId().data();
        codeAction.description = "Remove type annotation in catch clause";
        codeAction.changes = changes;
        codeAction.fixId = REMOVE_CATCH_TYPE_ANNOTATION.GetFixId().data();
        codeAction.fixAllDescription = "Remove all type annotations in catch clauses";
        returnedActions.push_back(codeAction);
    }

    return returnedActions;
}

CombinedCodeActions RemoveCatchTypeAnnotation::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(codeFixAllCtx, GetErrorCodes(),
                                             [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
                                                 MakeChange(tracker, codeFixAllCtx.context, diag.GetStart());
                                             });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;

    return combinedCodeActions;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<RemoveCatchTypeAnnotation> g_removeCatchTypeAnnotation(
    REMOVE_CATCH_TYPE_ANNOTATION.GetFixId().data());
}  // namespace ark::es2panda::lsp
