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

#include "lsp/include/register_code_fix/fix_remove_illegal_await.h"

#include <string>

#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "public/public.h"

namespace ark::es2panda::lsp {

using codefixes::FIX_REMOVE_ILLEGAL_AWAIT;

FixRemoveIllegalAwait::FixRemoveIllegalAwait()
{
    auto errorCodes = FIX_REMOVE_ILLEGAL_AWAIT.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_REMOVE_ILLEGAL_AWAIT.GetFixId().data()});
}

void FixRemoveIllegalAwait::MakeChangeForRemoveIllegalAwait(ChangeTracker &changeTracker, es2panda_Context *context,
                                                            size_t pos)
{
    auto *token = GetTouchingTokenRightMatch(context, pos, false);
    if (token == nullptr) {
        return;
    }

    // Find the await keyword range near the diagnostic position
    TextRange awaitTokenRange = {0, 0};
    changeTracker.RfindNearestKeyWordTextRange(context, token->Start().index, "await", awaitTokenRange);
    if (awaitTokenRange.end == 0) {
        return;
    }

    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);

    // Delete "await" and the following whitespace
    size_t deleteEnd = awaitTokenRange.end;
    const auto &source = std::string(ctx->parserProgram->SourceCode());
    while (deleteEnd < source.size() && source[deleteEnd] == ' ') {
        deleteEnd++;
    }

    changeTracker.DeleteRange(ctx->sourceFile, {awaitTokenRange.pos, deleteEnd});
}

std::vector<FileTextChanges> FixRemoveIllegalAwait::GetCodeActionsToRemoveIllegalAwait(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    return ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForRemoveIllegalAwait(tracker, context.context, context.span.start);
    });
}

std::vector<CodeFixAction> FixRemoveIllegalAwait::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToRemoveIllegalAwait(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = FIX_REMOVE_ILLEGAL_AWAIT.GetFixId().data();
        codeAction.fixId = FIX_REMOVE_ILLEGAL_AWAIT.GetFixId().data();
        codeAction.fixAllDescription = "Remove all illegal 'await' keywords";
        codeAction.description = "Remove illegal 'await' keyword";
        codeAction.changes = changes;
        returnedActions.push_back(std::move(codeAction));
    }

    return returnedActions;
}

CombinedCodeActions FixRemoveIllegalAwait::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeChangeForRemoveIllegalAwait(tracker, codeFixAllCtx.context, diag.GetStart());
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;
    return combinedCodeActions;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixRemoveIllegalAwait> g_fixRemoveIllegalAwait(FIX_REMOVE_ILLEGAL_AWAIT.GetFixId().data());

}  // namespace ark::es2panda::lsp
