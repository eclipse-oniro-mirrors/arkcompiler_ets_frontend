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

#include "lsp/include/register_code_fix/fix_class_super_must_precede_this_access.h"
#include <string>
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"

namespace ark::es2panda::lsp {
using codefixes::CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS;

ir::AstNode *FindNearestFunction(ir::AstNode *node)
{
    while (node != nullptr) {
        if (node->IsConstructor()) {
            return node;
        }
        node = node->Parent();
    }
    return nullptr;
}

void FixClassSuperMustPrecedeThisAccess::MakeChangeForClassSuperMustPrecedeThisAccess(ChangeTracker &changeTracker,
                                                                                      es2panda_Context *context,
                                                                                      size_t pos)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr || (!token->IsThisExpression() && !token->IsSuperExpression())) {
        return;
    }

    ir::AstNode *ctr = FindNearestFunction(token);
    if (ctr == nullptr) {
        return;
    }

    ir::AstNode *superCall = nullptr;
    ctr->FindChild([&](ir::AstNode *n) {
        if (n->IsSuperExpression()) {
            superCall = n;
            return true;
        }
        return false;
    });

    ir::AstNode *blockStatement = nullptr;
    ctr->FindChild([&](ir::AstNode *n) {
        if (n->IsBlockStatement()) {
            blockStatement = n;
            return true;
        }
        return false;
    });

    if (superCall == nullptr) {
        return;
    }

    ir::AstNode *callExpr = superCall->Parent();
    if (callExpr == nullptr || !callExpr->IsCallExpression()) {
        return;
    }

    ir::AstNode *statement = callExpr->Parent();
    if (statement == nullptr || !statement->IsExpressionStatement()) {
        return;
    }

    if (!token->IsSuperExpression() && token->Start().index > superCall->Start().index) {
        return;
    }

    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);

    changeTracker.DeleteRange(ctx->sourceFile, {statement->Start().index, statement->End().index});

    auto *exprStmt = ctx->allocator->New<ir::ExpressionStatement>(callExpr->AsExpression());

    std::string text = "\n" + exprStmt->DumpEtsSrc() + ";";
    changeTracker.InsertText(ctx->sourceFile, blockStatement->Start().index + 1, text);
}

std::vector<FileTextChanges> FixClassSuperMustPrecedeThisAccess::GetCodeActionsForClassSuperMustPrecedeThisAccess(
    const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForClassSuperMustPrecedeThisAccess(tracker, context.context, context.span.start);
    });

    return fileTextChanges;
}

FixClassSuperMustPrecedeThisAccess::FixClassSuperMustPrecedeThisAccess()
{
    auto errorCodes = CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS.GetFixId().data()});
}

std::vector<CodeFixAction> FixClassSuperMustPrecedeThisAccess::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsForClassSuperMustPrecedeThisAccess(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS.GetFixId().data();
        codeAction.description = "Fix 'super' access before 'this'";
        codeAction.changes = changes;
        codeAction.fixId = CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS.GetFixId().data();
        codeAction.fixAllDescription = "Fix all 'super' access before 'this'";
        returnedActions.push_back(codeAction);
    }

    return returnedActions;
}

CombinedCodeActions FixClassSuperMustPrecedeThisAccess::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeChangeForClassSuperMustPrecedeThisAccess(tracker, codeFixAllCtx.context, diag.GetStart());
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;

    return combinedCodeActions;
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixClassSuperMustPrecedeThisAccess> g_fixClassSuperMustPrecedeThisAccess(
    CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS.GetFixId().data());
}  // namespace ark::es2panda::lsp
