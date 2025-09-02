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

TextChange FixClassNotImplementingInheritedMembers::MakeTextChange(es2panda_Context *context, size_t offset)
{
    TextChange res {{0, 0}, ""};
    if (context == nullptr) {
        return res;
    }

    auto targetNode = GetTouchingToken(context, offset, false);
    if (targetNode == nullptr) {
        return res;
    }

    while (targetNode->Parent() != nullptr) {
        if (targetNode->IsClassDefinition()) {
            targetNode = targetNode->AsClassDefinition()->Ident();
            break;
        }
        targetNode = targetNode->Parent();
    }
    if (!targetNode->IsIdentifier()) {
        return res;
    }
    auto targetDecl = compiler::DeclarationFromIdentifier(targetNode->AsIdentifier());
    if (targetDecl == nullptr || !targetDecl->IsClassDefinition()) {
        return res;
    }

    auto targetClassBodys = targetDecl->AsClassDefinition()->Body();
    res.span.start = targetDecl->AsClassDefinition()->Start().index + 1;
    std::unordered_set<std::string> methodSignature;
    for (auto it : targetClassBodys) {
        methodSignature.insert(MakeMethodSignature(it));
    }

    auto superClassDecl = GetSuperClassDefinition(targetDecl);
    if (superClassDecl == nullptr || !superClassDecl->IsClassDefinition()) {
        return res;
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
    return res;
}

const int G_FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS = 1006;

FixClassNotImplementingInheritedMembers::FixClassNotImplementingInheritedMembers()
{
    const char *fixClassNotImplementingInheritedMembersId = "FixClassNotImplementingInheritedMembers";
    SetErrorCodes({G_FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS});
    SetFixIds({fixClassNotImplementingInheritedMembersId});
}

std::vector<CodeFixAction> FixClassNotImplementingInheritedMembers::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    if (context.span.length == 0) {
        return returnedActions;
    }
    std::vector<TextChange> textChanges;
    auto ctx = reinterpret_cast<public_lib::Context *>(context.context);
    const auto &diagnostics = ctx->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::SEMANTIC);
    for (const auto &diagnostic : diagnostics) {
        auto index = lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto offset =
            index.GetOffset(lexer::SourceLocation(diagnostic->Line(), diagnostic->Offset(), ctx->parserProgram));
        size_t end = context.span.start + context.span.length;
        if (offset < context.span.start || offset >= end) {
            break;
        }
        textChanges.push_back(MakeTextChange(context.context, offset));
    }
    CodeFixAction codeAction;
    codeAction.changes.emplace_back(std::string(ctx->parserProgram->SourceFilePath()), textChanges);
    codeAction.fixId = "FixClassNotImplementingInheritedMembers";
    returnedActions.push_back(codeAction);
    return returnedActions;
}

CombinedCodeActions FixClassNotImplementingInheritedMembers::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    CombinedCodeActions combinedCodeActions;
    std::vector<TextChange> textChanges;
    auto ctx = reinterpret_cast<public_lib::Context *>(codeFixAll.context);
    const auto &diagnostics = ctx->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::SEMANTIC);
    for (const auto &diagnostic : diagnostics) {
        auto index = lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto offset =
            index.GetOffset(lexer::SourceLocation(diagnostic->Line(), diagnostic->Offset(), ctx->parserProgram));
        textChanges.push_back(MakeTextChange(codeFixAll.context, offset));
    }
    combinedCodeActions.changes.emplace_back(std::string(ctx->parserProgram->SourceFilePath()), textChanges);
    return combinedCodeActions;
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixClassNotImplementingInheritedMembers> g_fixClassNotImplementingInheritedMembers(
    "FixClassNotImplementingInheritedMembers");

}  // namespace ark::es2panda::lsp