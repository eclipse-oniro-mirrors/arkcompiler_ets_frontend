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

#include "generated/code_fix_register.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/register_code_fix/fix_property_assignment.h"

namespace ark::es2panda::lsp {
using codefixes::FIX_PROPERTY_ASSIGNMENT;
FixPropertyAssignment::FixPropertyAssignment()
{
    auto errorCodes = FIX_PROPERTY_ASSIGNMENT.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_PROPERTY_ASSIGNMENT.GetFixId().data()});
}

ir::Property *FixPropertyAssignment::GetInvalidEqualsProperty(const ir::AstNode *node)
{
    if (node == nullptr || !node->IsProperty()) {
        return nullptr;
    }

    const auto *property = node->AsProperty();
    if (property->Value() != nullptr && property->Key()->IsIdentifier() && property->Value()->IsAssignmentPattern()) {
        return const_cast<ir::Property *>(property);
    }

    return nullptr;
}

void FixPropertyAssignment::MakeChange(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos,
                                       std::vector<ir::AstNode *> &fixedNodes)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr) {
        return;
    }

    const ir::AstNode *node = token;
    while (node != nullptr && !node->IsProperty()) {
        node = node->Parent();
    }

    ir::Property *invalidProp = GetInvalidEqualsProperty(node);
    if (invalidProp == nullptr) {
        return;
    }

    const auto *impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *keyNode = reinterpret_cast<es2panda_AstNode *>(invalidProp->Key());
    auto *valueNode = reinterpret_cast<es2panda_AstNode *>(invalidProp->Value());
    if (keyNode == nullptr || valueNode == nullptr) {
        return;
    }

    auto *assign = invalidProp->Value()->AsAssignmentPattern();
    auto *rhs = reinterpret_cast<es2panda_AstNode *>(assign->Right());
    es2panda_AstNode *replacement = impl->CreateProperty(context, keyNode, rhs);
    if (replacement == nullptr) {
        return;
    }

    impl->AstNodeSetParent(context, keyNode, replacement);
    impl->AstNodeSetParent(context, rhs, replacement);
    auto *replacementNode = reinterpret_cast<ir::AstNode *>(replacement);
    if (replacementNode == nullptr) {
        return;
    }

    replacementNode->SetParent(invalidProp->Parent());
    changeTracker.ReplaceNode(context, invalidProp, replacementNode, {});
    fixedNodes.push_back(replacementNode);
}

std::vector<FileTextChanges> FixPropertyAssignment::GetCodeActionsToFix(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    std::vector<ir::AstNode *> fixedNodes;
    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChange(tracker, context.context, context.span.start, fixedNodes);
    });

    return fileTextChanges;
}

std::vector<CodeFixAction> FixPropertyAssignment::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToFix(context);
    if (!changes.empty()) {
        CodeFixAction action;
        action.fixName = FIX_PROPERTY_ASSIGNMENT.GetFixId().data();
        action.description = "Change '=' to ':' in object property";
        action.fixId = FIX_PROPERTY_ASSIGNMENT.GetFixId().data();
        action.changes = changes;
        returnedActions.push_back(action);
    }

    return returnedActions;
}

CombinedCodeActions FixPropertyAssignment::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    CodeFixProvider provider;
    std::vector<ir::AstNode *> fixedNodes;

    const auto changes = provider.CodeFixAll(
        codeFixAll, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            Initializer initializer;
            std::string fileName = std::string(diag.GetFile().filePath);
            std::string fileContent = std::string(diag.GetFile().source);
            es2panda_Context *ctx =
                initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
            MakeChange(tracker, ctx, diag.GetStart(), fixedNodes);
            initializer.DestroyContext(ctx);
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;
    return combinedCodeActions;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixPropertyAssignment> g_fixPropertyAssignment(FIX_PROPERTY_ASSIGNMENT.GetFixId().data());
}  // namespace ark::es2panda::lsp