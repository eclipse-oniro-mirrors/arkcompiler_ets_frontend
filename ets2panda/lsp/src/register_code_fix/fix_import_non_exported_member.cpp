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
#include "lsp/include/register_code_fix/fix_import_non_exported_member.h"
#include <string>

#include "compiler/lowering/util.h"
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"

namespace ark::es2panda::lsp {
using codefixes::FIX_IMPORT_NON_EXPORTED_MEMBER;

void FixImportNonExportedMember::MakeChangeForImportNonExportedMember(ChangeTracker &changeTracker,
                                                                      es2panda_Context *context, size_t pos)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr) {
        return;
    }

    auto *importDeclNode = token;
    util::StringView functionName;

    if (!FindImportDeclaration(importDeclNode)) {
        return;
    }

    if (!FindFunctionName(importDeclNode, functionName)) {
        return;
    }

    size_t exportPosition = 0;

    if (importDeclNode->IsETSImportDeclaration()) {
        auto funcDecl = importDeclNode->AsETSImportDeclaration();
        if (!funcDecl->Specifiers().empty()) {
            ProcessExportPosition(funcDecl, functionName, exportPosition, changeTracker);
        }
    }
}

bool FixImportNonExportedMember::FindImportDeclaration(ir::AstNode *&importDeclNode)
{
    while (importDeclNode != nullptr && !importDeclNode->IsETSImportDeclaration()) {
        importDeclNode = importDeclNode->Parent();
    }
    return (importDeclNode != nullptr);
}

bool FixImportNonExportedMember::FindFunctionName(ir::AstNode *importDeclNode, util::StringView &functionName)
{
    importDeclNode->FindChild([&](ir::AstNode *n) {
        if (n->IsIdentifier() && n->Parent()->IsImportSpecifier()) {
            functionName = n->AsIdentifier()->Name();
            return true;
        }
        return false;
    });
    return !functionName.Empty();
}

void FixImportNonExportedMember::ProcessExportPosition(ir::AstNode *funcDecl, const util::StringView &functionName,
                                                       size_t &exportPosition, ChangeTracker &changeTracker)
{
    Initializer initializer = Initializer();
    const auto path = funcDecl->AsETSImportDeclaration()->ResolvedSource();
    auto targetContext = initializer.CreateContext(std::string(path).c_str(), ES2PANDA_STATE_CHECKED);
    auto cctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(targetContext);
    auto nodeOfAllContext = cctx->parserProgram->Ast();
    if (targetContext == nullptr || nodeOfAllContext == nullptr) {
        return;
    }

    nodeOfAllContext->FindChild([&](ir::AstNode *n) {
        if (n->IsIdentifier() && n->AsIdentifier()->Name() == functionName) {
            exportPosition = n->Parent()->Start().index;
            return true;
        }
        return false;
    });

    const std::string replaceText("export");
    changeTracker.ReplaceRangeWithText(cctx->sourceFile, {exportPosition, exportPosition}, replaceText);
}

std::vector<FileTextChanges> FixImportNonExportedMember::GetCodeActionsToImportNonExportedMember(
    const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForImportNonExportedMember(tracker, context.context, context.span.start);
    });

    return fileTextChanges;
}

FixImportNonExportedMember::FixImportNonExportedMember()
{
    auto errorCodes = FIX_IMPORT_NON_EXPORTED_MEMBER.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_IMPORT_NON_EXPORTED_MEMBER.GetFixId().data()});
}

std::vector<CodeFixAction> FixImportNonExportedMember::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToImportNonExportedMember(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = FIX_IMPORT_NON_EXPORTED_MEMBER.GetFixId().data();
        codeAction.description = "Fix Import Non Exported Member";
        codeAction.changes = changes;
        codeAction.fixId = FIX_IMPORT_NON_EXPORTED_MEMBER.GetFixId().data();
        returnedActions.push_back(codeAction);
    }

    return returnedActions;
}

CombinedCodeActions FixImportNonExportedMember::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeChangeForImportNonExportedMember(tracker, codeFixAllCtx.context, diag.GetStart());
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;

    return combinedCodeActions;
}

// NOLINTNEXTLINE
AutoCodeFixRegister<FixImportNonExportedMember> g_fixImportNonExportedMember(
    FIX_IMPORT_NON_EXPORTED_MEMBER.GetFixId().data());
}  // namespace ark::es2panda::lsp
