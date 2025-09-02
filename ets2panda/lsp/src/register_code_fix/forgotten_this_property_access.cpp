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
#include <iostream>
#include "code_fix_provider.h"
#include "generated/code_fix_register.h"

namespace ark::es2panda::lsp {
using codefixes::FORGOTTEN_THIS_PROPERTY_ACCESS;

ForgottenThisPropertyAccess::ForgottenThisPropertyAccess()
{
    auto errorCodes = FORGOTTEN_THIS_PROPERTY_ACCESS.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});  // change this to the error code you want to handle
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

void DoChanges(es2panda_Context *context, ChangeTracker tracker)
{
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);

    const auto &diagnostics =
        ctx->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);

    for (const auto &diagnostic : diagnostics) {
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto offset = index.GetOffset(
            ark::es2panda::lexer::SourceLocation(diagnostic->Line(), diagnostic->Offset(), ctx->parserProgram));
        auto node = ark::es2panda::lsp::GetTouchingToken(context, offset, false);
        es2panda_AstNode *thisExpr = impl->CreateThisExpression(context);
        es2panda_AstNode *memberExpr =
            impl->CreateMemberExpression(context, thisExpr, reinterpret_cast<es2panda_AstNode *>(node),
                                         MEMBER_EXPRESSION_KIND_PROPERTY_ACCESS, false, false);
        impl->AstNodeSetParent(context, thisExpr, memberExpr);
        impl->AstNodeSetParent(context, reinterpret_cast<es2panda_AstNode *>(node), memberExpr);
        auto memNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(memberExpr);
        if (memNode == nullptr) {
            continue;
        }
        tracker.ReplaceNode(context, node, memNode, {});
    }
}

std::vector<CodeFixAction> ForgottenThisPropertyAccess::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;

    const auto info = GetInfoThisProp(context.context, context.span.start);
    if (info.GetNode() == nullptr) {
        return {};
    }
    TextChangesContext textChangesContext {context.host, context.formatContext, context.preferences};
    const auto changes =
        ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) { DoChanges(context.context, tracker); });
    std::vector<CodeFixAction> actions;
    CodeFixAction action;
    action.fixName = FORGOTTEN_THIS_PROPERTY_ACCESS.GetFixId().data();
    action.description = "Add 'this.' to property access";
    action.fixId = FORGOTTEN_THIS_PROPERTY_ACCESS.GetFixId().data();
    action.changes.insert(action.changes.end(), changes.begin(), changes.end());
    action.fixAllDescription = "Add 'this.' to all property accesses in the file";
    returnedActions.push_back(action);
    return returnedActions;
}

CombinedCodeActions ForgottenThisPropertyAccess::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(codeFixAll, GetErrorCodes(),
                                             [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
                                                 auto info = GetInfoThisProp(codeFixAll.context, diag.GetStart());
                                                 if (info.GetNode() != nullptr) {
                                                     DoChanges(codeFixAll.context, tracker);
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