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

#include "register_code_fix/forgotten_this_property_access.h"
#include "code_fix_provider.h"
#include "generated/code_fix_register.h"

namespace ark::es2panda::lsp {
using codefixes::FORGOTTEN_THIS_PROPERTY_ACCESS;

ForgottenThisPropertyAccess::ForgottenThisPropertyAccess()
{
    auto errorCodes = FORGOTTEN_THIS_PROPERTY_ACCESS.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FORGOTTEN_THIS_PROPERTY_ACCESS.GetFixId().data()});
}

Info GetInfoThisProp(es2panda_Context *context, size_t offset)
{
    ir::AstNode *node = ark::es2panda::lsp::GetTouchingToken(context, offset, false);
    std::string className;
    if (node == nullptr) {
        return {nullptr, ""};
    }
    if (node->IsIdentifier()) {
        className = node->AsIdentifier()->Name().Utf8();
    }
    Info info(node, className);
    return info;
}

void ForgottenThisPropertyAccess::DoChanges(ChangeTracker &tracker, es2panda_Context *context, size_t pos)
{
    auto info = GetInfoThisProp(context, pos);
    auto node = info.GetNode();
    if (node == nullptr) {
        return;
    }

    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    es2panda_AstNode *thisExpr = impl->CreateThisExpression(context);
    es2panda_AstNode *memberExpr =
        impl->CreateMemberExpression(context, thisExpr, reinterpret_cast<es2panda_AstNode *>(node),
                                     MEMBER_EXPRESSION_KIND_PROPERTY_ACCESS, false, false);
    impl->AstNodeSetParent(context, thisExpr, memberExpr);
    impl->AstNodeSetParent(context, reinterpret_cast<es2panda_AstNode *>(node), memberExpr);
    auto memNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(memberExpr);
    if (memNode != nullptr) {
        tracker.ReplaceNode(context, node, memNode, {});
    }
}

std::vector<FileTextChanges> ForgottenThisPropertyAccess::GetCodeActionsToFix(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    auto fileTextChanges = ChangeTracker::With(
        textChangesContext, [&](ChangeTracker &tracker) { DoChanges(tracker, context.context, context.span.start); });
    return fileTextChanges;
}

std::vector<CodeFixAction> ForgottenThisPropertyAccess::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToFix(context);
    if (!changes.empty()) {
        CodeFixAction action;
        action.fixName = FORGOTTEN_THIS_PROPERTY_ACCESS.GetFixId().data();
        action.description = "Add 'this.' to property access";
        action.fixId = FORGOTTEN_THIS_PROPERTY_ACCESS.GetFixId().data();
        action.fixAllDescription = "Add 'this.' to all property accesses in the file";
        action.changes.insert(action.changes.end(), changes.begin(), changes.end());
        returnedActions.push_back(action);
    }
    return returnedActions;
}

CombinedCodeActions ForgottenThisPropertyAccess::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(codeFixAll, GetErrorCodes(),
                                             [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
                                                 auto info = GetInfoThisProp(codeFixAll.context, diag.GetStart());
                                                 if (info.GetNode() != nullptr) {
                                                     DoChanges(tracker, codeFixAll.context, diag.GetStart());
                                                 }
                                             });

    CombinedCodeActions combined;
    combined.changes = changes.changes;
    combined.commands = changes.commands;
    return combined;
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<ForgottenThisPropertyAccess> g_forgottenThisPropertyAccess("ForgottenThisPropertyAccess");

}  // namespace ark::es2panda::lsp