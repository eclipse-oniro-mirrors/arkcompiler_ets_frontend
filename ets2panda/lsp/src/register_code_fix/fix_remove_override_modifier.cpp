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

#include "lsp/include/register_code_fix/fix_remove_override_modifier.h"

#include <iostream>
#include <string>

#include "compiler/lowering/util.h"
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"

namespace ark::es2panda::lsp {
using codefixes::REMOVE_OVERRIDE_MODIFIER;

ir::AstNode *FindNearestClass(ir::AstNode *node)
{
    while (node != nullptr) {
        if (node->IsClassDefinition()) {
            return node;
        }
        node = node->Parent();
    }
    return nullptr;
}

void FixRemoveOverrideModifier::MakeChangeForRemoveOverrideModifier(ChangeTracker &changeTracker,
                                                                    es2panda_Context *context, size_t pos)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr) {
        return;
    }

    // Remove the override modifier if found
    TextRange overrideTokenRange = {0, 0};
    changeTracker.RfindNearestKeyWordTextRange(context, token->Start().index, "override", overrideTokenRange);
    if (overrideTokenRange.end == 0) {
        return;
    }

    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    changeTracker.DeleteRange(astContext->sourceFile, overrideTokenRange);
}

std::vector<FileTextChanges> FixRemoveOverrideModifier::GetCodeActionsToRemoveOverrideModifier(
    const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForRemoveOverrideModifier(tracker, context.context, context.span.start);
    });

    return fileTextChanges;
}

FixRemoveOverrideModifier::FixRemoveOverrideModifier()
{
    auto errorCodes = REMOVE_OVERRIDE_MODIFIER.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({REMOVE_OVERRIDE_MODIFIER.GetFixId().data()});
}

std::vector<CodeFixAction> FixRemoveOverrideModifier::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToRemoveOverrideModifier(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = REMOVE_OVERRIDE_MODIFIER.GetFixId().data();
        codeAction.description = "Remove override modifier";
        codeAction.changes = changes;
        codeAction.fixId = REMOVE_OVERRIDE_MODIFIER.GetFixId().data();
        codeAction.fixAllDescription = "Remove all unnecessary override modifiers";
        returnedActions.push_back(codeAction);
    }

    return returnedActions;
}

CombinedCodeActions FixRemoveOverrideModifier::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeChangeForRemoveOverrideModifier(tracker, codeFixAllCtx.context, diag.GetStart());
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;

    return combinedCodeActions;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixRemoveOverrideModifier> g_fixRemoveOverrideModifier(REMOVE_OVERRIDE_MODIFIER.GetFixId().data());
}  // namespace ark::es2panda::lsp