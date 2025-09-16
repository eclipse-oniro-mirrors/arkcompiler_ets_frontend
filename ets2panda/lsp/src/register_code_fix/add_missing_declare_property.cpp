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

#include "lsp/include/register_code_fix/add_missing_declare_property.h"
#include <iostream>
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"

namespace ark::es2panda::lsp {

const int G_ADD_MISSING_DECLARE_PROPERTY_CODE = 1001;  // change this to the error code you want to handle

void MakeChange(ChangeTracker changeTracker, es2panda_Context *context, size_t pos,
                std::vector<ark::es2panda::ir::AstNode *> &fixedNodes)
{
    const auto token = GetTouchingToken(context, pos, false);
    if (token == nullptr || !token->IsIdentifier()) {
        return;
    }
    const auto declaration = token->Parent();
    if (declaration->IsProperty()) {
        fixedNodes.push_back(declaration);
        changeTracker.InsertModifierBefore(context, token, declaration);
    }
}

std::vector<FileTextChanges> GetCodeActionsToAddMissingDeclareOnProperty(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    std::vector<ark::es2panda::ir::AstNode *> fixedNodes;

    auto fileTextChanges = ChangeTracker::With(
        textChangesContext, [&](ChangeTracker &tracker) { MakeChange(tracker, context.context, 3, fixedNodes); });
    return fileTextChanges;
}

AddMissingDeclareProperty::AddMissingDeclareProperty()
{
    const char *addMissingDeclarationPropertyId = "AddMissingDeclareProperty";
    SetErrorCodes({G_ADD_MISSING_DECLARE_PROPERTY_CODE});
    SetFixIds({addMissingDeclarationPropertyId});
}

std::vector<CodeFixAction> AddMissingDeclareProperty::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToAddMissingDeclareOnProperty(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = "Fix";
        codeAction.description = "Fix Description";
        codeAction.changes = changes;
        codeAction.fixId = "AddMissingDeclareProperty";
        codeAction.fixAllDescription = "Fix All Description";
        InstallPackageAction codeActionCommand;
        codeActionCommand.file = "addMissingDeclareProperty.ets";
        codeActionCommand.packageName = "dummy-package";
        codeAction.commands.push_back(codeActionCommand);
        returnedActions.push_back(codeAction);
    }
    return returnedActions;
}

CombinedCodeActions AddMissingDeclareProperty::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    std::vector<ark::es2panda::ir::AstNode *> fixedNodes;
    CodeFixProvider provider;

    const auto changes = provider.CodeFixAll(codeFixAll, GetErrorCodes(),
                                             [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
                                                 MakeChange(tracker, codeFixAll.context, diag.GetStart(), fixedNodes);
                                             });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;
    return combinedCodeActions;
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<AddMissingDeclareProperty> g_addMissingDeclareProperty("AddMissingDeclareProperty");
}  // namespace ark::es2panda::lsp
