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

#include "lsp/include/register_code_fix/fix_extends_interface_becomes_implements.h"

#include "compiler/lowering/util.h"
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"

namespace ark::es2panda::lsp {
using codefixes::EXTENDS_INTERFACE_BECOMES_IMPLEMENTS;

constexpr std::string_view KEYW_IMPLEMENTS_STR = "implements";

void FixExtendsInterfaceBecomesImplements::MakeChangeForExtendsInterfaceBecomesImplements(ChangeTracker &changeTracker,
                                                                                          es2panda_Context *context,
                                                                                          size_t pos)
{
    auto *token = GetTouchingToken(context, pos, false);

    size_t changeEnd = 0;
    size_t changeStart = 0;
    size_t spaceChar = 1;
    if (!token->IsClassDeclaration()) {
        return;
    }
    token->FindChild([&](ir::AstNode *n) {
        if (n->IsIdentifier()) {
            changeStart = n->End().index + spaceChar;
            return true;
        }
        return false;
    });

    token->FindChild([&](ir::AstNode *n) {
        if (n->IsETSTypeReference()) {
            changeEnd = n->Start().index - spaceChar;
            return true;
        }
        return false;
    });

    TextRange extendsRange = {changeStart, changeEnd};
    const std::string replaceText(KEYW_IMPLEMENTS_STR);
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    changeTracker.ReplaceRangeWithText(astContext->sourceFile, extendsRange, replaceText);
}

std::vector<FileTextChanges> FixExtendsInterfaceBecomesImplements::GetCodeActionsToExtendsInterfaceBecomesImplements(
    const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForExtendsInterfaceBecomesImplements(tracker, context.context, context.span.start);
    });

    return fileTextChanges;
}

FixExtendsInterfaceBecomesImplements::FixExtendsInterfaceBecomesImplements()
{
    auto errorCodes = EXTENDS_INTERFACE_BECOMES_IMPLEMENTS.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({EXTENDS_INTERFACE_BECOMES_IMPLEMENTS.GetFixId().data()});
}

std::vector<CodeFixAction> FixExtendsInterfaceBecomesImplements::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToExtendsInterfaceBecomesImplements(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = EXTENDS_INTERFACE_BECOMES_IMPLEMENTS.GetFixId().data();
        codeAction.description = "Change 'extends' to 'implements'";
        codeAction.changes = changes;
        codeAction.fixId = EXTENDS_INTERFACE_BECOMES_IMPLEMENTS.GetFixId().data();
        codeAction.fixAllDescription = "Change all 'extends' on interfaces to 'implements'";
        returnedActions.push_back(codeAction);
    }

    return returnedActions;
}

CombinedCodeActions FixExtendsInterfaceBecomesImplements::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeChangeForExtendsInterfaceBecomesImplements(tracker, codeFixAllCtx.context, diag.GetStart());
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;

    return combinedCodeActions;
}

// NOLINTNEXTLINE
AutoCodeFixRegister<FixExtendsInterfaceBecomesImplements> g_FixExtendsInterfaceBecomesImplements(
    EXTENDS_INTERFACE_BECOMES_IMPLEMENTS.GetFixId().data());

}  // namespace ark::es2panda::lsp