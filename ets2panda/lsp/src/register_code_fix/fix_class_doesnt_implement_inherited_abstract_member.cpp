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

#include "code_fix_provider.h"
#include "code_fixes/code_fix_types.h"
#include "compiler/lowering/util.h"
#include "generated/code_fix_register.h"
#include "internal_api.h"
#include "ir/astNode.h"
#include "ir/base/classDefinition.h"
#include "public/es2panda_lib.h"
#include "register_code_fix/fix_class_doesnt_implement_inherited_abstract_member.h"
#include "public/public.h"
#include "types.h"
#include <cstddef>
#include <vector>

namespace ark::es2panda::lsp {
using codefixes::FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS;

FixClassNotImplementingInheritedMembers::FixClassNotImplementingInheritedMembers()
{
    auto errorCodes = FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetFixId().data()});
}

ir::AstNode *FixClassNotImplementingInheritedMembers::GetSuperClassDefinition(ir::AstNode *node)
{
    if (node == nullptr || !node->IsClassDefinition()) {
        return nullptr;
    }
    auto superClass = node->AsClassDefinition()->Super();
    if (superClass->IsETSTypeReference()) {
        auto part = superClass->AsETSTypeReference()->Part();
        if (part->IsETSTypeReferencePart()) {
            auto name = part->Name();
            return compiler::DeclarationFromIdentifier(name->AsIdentifier());
        }
    }
    return nullptr;
}

std::string FixClassNotImplementingInheritedMembers::MakeMethodSignature(ir::AstNode *node)
{
    if (node == nullptr || !node->IsMethodDefinition()) {
        return "";
    }
    auto methodName = node->AsMethodDefinition()->Key()->AsIdentifier()->Name();
    return std::string(methodName) + node->AsMethodDefinition()->Function()->Signature()->ToString();
}

std::string FixClassNotImplementingInheritedMembers::MakeNewText(ir::AstNode *node)
{
    if (node == nullptr || !node->IsMethodDefinition()) {
        return "";
    }
    const std::string suffix = " {}";
    const std::string prefix = "  ";
    auto methodName = node->AsMethodDefinition()->Key()->AsIdentifier()->Name();
    auto newText = std::string(methodName) + node->AsMethodDefinition()->Function()->DumpEtsSrc();
    newText.insert(0, prefix);
    newText.insert(newText.size() - 1, suffix);
    return newText;
}

void FixClassNotImplementingInheritedMembers::MakeTextChangeForNotImplementedMembers(ChangeTracker &changeTracker,
                                                                                     es2panda_Context *context,
                                                                                     size_t pos)
{
    TextChange res {{0, 0}, ""};
    if (context == nullptr) {
        return;
    }

    auto targetNode = GetTouchingToken(context, pos, false);
    if (targetNode == nullptr) {
        return;
    }

    while (targetNode->Parent() != nullptr) {
        if (targetNode->IsClassDefinition()) {
            targetNode = targetNode->AsClassDefinition()->Ident();
            break;
        }
        targetNode = targetNode->Parent();
    }
    if (!targetNode->IsIdentifier()) {
        return;
    }
    auto targetDecl = compiler::DeclarationFromIdentifier(targetNode->AsIdentifier());
    if (targetDecl == nullptr || !targetDecl->IsClassDefinition()) {
        return;
    }

    auto targetClassBodys = targetDecl->AsClassDefinition()->Body();
    res.span.start = targetDecl->AsClassDefinition()->Start().index + 1;
    std::unordered_set<std::string> methodSignature;
    for (auto it : targetClassBodys) {
        methodSignature.insert(MakeMethodSignature(it));
    }

    auto superClassDecl = GetSuperClassDefinition(targetDecl);
    if (superClassDecl == nullptr || !superClassDecl->IsClassDefinition()) {
        return;
    }
    auto superClassBodys = superClassDecl->AsClassDefinition()->Body();
    for (const auto &method : superClassBodys) {
        if (!method->IsAbstract()) {
            continue;
        }
        auto signature = MakeMethodSignature(method);
        if (methodSignature.count(signature) == 0) {
            res.newText += MakeNewText(method);
        }
    }
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    changeTracker.ReplaceRangeWithText(ctx->sourceFile, {res.span.start, res.span.start + res.span.length},
                                       res.newText);
}

std::vector<FileTextChanges> FixClassNotImplementingInheritedMembers::GetCodeActionsForAbstractMissingMembers(
    const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};

    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeTextChangeForNotImplementedMembers(tracker, context.context, context.span.start);
    });

    return fileTextChanges;
}

std::vector<CodeFixAction> FixClassNotImplementingInheritedMembers::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> actions;
    auto changes = GetCodeActionsForAbstractMissingMembers(context);
    if (!changes.empty()) {
        CodeFixAction fix;
        fix.fixName = FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetFixId().data();
        fix.description = "Add missing inherited abstract members";
        fix.fixAllDescription = "Add missing all inherited abstract members";
        fix.changes = changes;
        fix.fixId = FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetFixId().data();
        actions.push_back(std::move(fix));
    }

    return actions;
}

CombinedCodeActions FixClassNotImplementingInheritedMembers::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    CodeFixProvider provider;
    auto changes = provider.CodeFixAll(
        codeFixAll, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeTextChangeForNotImplementedMembers(tracker, codeFixAll.context, diag.GetStart());
        });

    CombinedCodeActions combined;
    combined.changes = std::move(changes.changes);
    combined.commands = std::move(changes.commands);
    return combined;
}

// NOLINTNEXTLINE
AutoCodeFixRegister<FixClassNotImplementingInheritedMembers> g_fixClassNotImplementingInheritedMembers(
    FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetFixId().data());

}  // namespace ark::es2panda::lsp