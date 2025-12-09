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

#include "lsp/include/register_code_fix/fix_class_incorrectly_implements_interface.h"

#include "compiler/lowering/util.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "ir/astNode.h"
#include "ir/base/classDefinition.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsInterfaceBody.h"
#include "ir/ts/tsClassImplements.h"
#include "ir/base/methodDefinition.h"

namespace ark::es2panda::lsp {
using codefixes::FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER;
using codefixes::FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER;

std::string FixClassIncorrectlyImplementsInterface::MakeNewTextForMember(ir::AstNode *node)
{
    if (node == nullptr || !node->IsMethodDefinition()) {
        return "";
    }

    auto *methodDef = node->AsMethodDefinition();
    auto methodName = methodDef->Key()->AsIdentifier()->Name();
    std::string newText;
    std::string prefix = "\n\n  ";
    std::string suffix;

    if (methodDef->IsGetter()) {
        newText = "get " + std::string(methodName) + methodDef->Function()->DumpEtsSrc();
        suffix = " {\n    return null;\n  }";
    } else if (methodDef->IsSetter()) {
        newText = "set " + std::string(methodName) + methodDef->Function()->DumpEtsSrc();
        suffix = " {\n  }";
    } else {
        newText = std::string(methodName) + methodDef->Function()->DumpEtsSrc();
        suffix = " {}";
    }

    newText.insert(0, prefix);
    newText.insert(newText.size() - 1, suffix);
    return newText;
}

void FixClassIncorrectlyImplementsInterface::MakeChangeForMissingInterfaceMembers(ChangeTracker &changeTracker,
                                                                                  es2panda_Context *context, size_t pos)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr) {
        return;
    }

    auto *classNode = token;
    while (classNode != nullptr && !classNode->IsClassDefinition()) {
        classNode = classNode->Parent();
    }

    if (classNode == nullptr || !classNode->IsClassDefinition()) {
        return;
    }

    auto *classDef = classNode->AsClassDefinition();
    std::vector<ir::AstNode *> missingMembers = FindMissingInterfaceMembers(classDef);
    if (missingMembers.empty()) {
        return;
    }

    auto classBody = classDef->Body();
    size_t insertPos = classDef->End().index;
    if (!classBody.empty()) {
        auto lastElement = classBody.back();
        insertPos = lastElement->End().index;
    }

    std::string newText;
    for (auto *missingMember : missingMembers) {
        newText += MakeNewTextForMember(missingMember);
    }

    if (!newText.empty()) {
        TextRange insertRange = {insertPos, insertPos};
        auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        changeTracker.ReplaceRangeWithText(astContext->sourceFile, insertRange, newText);
    }
}

std::vector<FileTextChanges> FixClassIncorrectlyImplementsInterface::GetCodeActionsToImplementMissingMembers(
    const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};

    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForMissingInterfaceMembers(tracker, context.context, context.span.start);
    });

    return fileTextChanges;
}

FixClassIncorrectlyImplementsInterface::FixClassIncorrectlyImplementsInterface()
{
    auto getterErrorCodes = FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER.GetSupportedCodeNumbers();
    auto setterErrorCodes = FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER.GetSupportedCodeNumbers();

    std::vector<int> allErrorCodes;
    allErrorCodes.insert(allErrorCodes.end(), getterErrorCodes.begin(), getterErrorCodes.end());
    allErrorCodes.insert(allErrorCodes.end(), setterErrorCodes.begin(), setterErrorCodes.end());

    SetErrorCodes(allErrorCodes);
    SetFixIds({FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER.GetFixId().data(),
               FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER.GetFixId().data()});
}

void FixClassIncorrectlyImplementsInterface::GroupMissingMembers(const std::vector<ir::AstNode *> &missingMembers,
                                                                 std::vector<ir::AstNode *> &missingGetters,
                                                                 std::vector<ir::AstNode *> &missingSetters)
{
    for (auto *member : missingMembers) {
        if (!member->IsMethodDefinition()) {
            continue;
        }

        auto *methodDef = member->AsMethodDefinition();
        if (methodDef->IsGetter()) {
            missingGetters.push_back(member);
        } else if (methodDef->IsSetter()) {
            missingSetters.push_back(member);
        }
    }
}

void FixClassIncorrectlyImplementsInterface::CreateCodeActionForType(const std::vector<ir::AstNode *> &members,
                                                                     const CodeFixContext &context, bool isGetter,
                                                                     std::vector<CodeFixAction> &returnedActions)
{
    if (members.empty()) {
        return;
    }

    auto changes = GetCodeActionsToImplementMissingMembers(context);
    if (changes.empty()) {
        return;
    }

    CodeFixAction codeAction;
    if (isGetter) {
        codeAction.fixName = FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER.GetFixId().data();
        codeAction.description = "Add missing interface getter implementations";
        codeAction.fixAllDescription = "Implement all missing interface getters";
        codeAction.fixId = FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER.GetFixId().data();
    } else {
        codeAction.fixName = FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER.GetFixId().data();
        codeAction.description = "Add missing interface setter implementations";
        codeAction.fixAllDescription = "Implement all missing interface setters";
        codeAction.fixId = FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER.GetFixId().data();
    }

    codeAction.changes = changes;
    returnedActions.push_back(codeAction);
}

std::vector<CodeFixAction> FixClassIncorrectlyImplementsInterface::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;

    auto *token = GetTouchingToken(context.context, context.span.start, false);
    if (token == nullptr) {
        return returnedActions;
    }

    auto *classNode = token;
    while (classNode != nullptr && !classNode->IsClassDefinition()) {
        classNode = classNode->Parent();
    }

    if (classNode == nullptr || !classNode->IsClassDefinition()) {
        return returnedActions;
    }

    auto *classDef = classNode->AsClassDefinition();
    std::vector<ir::AstNode *> missingMembers = FindMissingInterfaceMembers(classDef);
    if (missingMembers.empty()) {
        return returnedActions;
    }

    std::vector<ir::AstNode *> missingGetters;
    std::vector<ir::AstNode *> missingSetters;
    GroupMissingMembers(missingMembers, missingGetters, missingSetters);

    CreateCodeActionForType(missingGetters, context, true, returnedActions);
    CreateCodeActionForType(missingSetters, context, false, returnedActions);

    return returnedActions;
}

CombinedCodeActions FixClassIncorrectlyImplementsInterface::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeChangeForMissingInterfaceMembers(tracker, codeFixAllCtx.context, diag.GetStart());
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;

    return combinedCodeActions;
}

ir::AstNode *FixClassIncorrectlyImplementsInterface::FindInterfaceDefinition(ir::TSClassImplements *implement)
{
    if (!implement->IsTSClassImplements()) {
        return nullptr;
    }

    auto *tsImplements = implement->AsTSClassImplements();
    auto *expr = tsImplements->Expr();
    if (!expr->IsETSTypeReference()) {
        return nullptr;
    }

    auto *part = expr->AsETSTypeReference()->Part();
    if (!part->IsETSTypeReferencePart()) {
        return nullptr;
    }

    auto *name = part->Name();
    if (!name->IsIdentifier()) {
        return nullptr;
    }

    return compiler::DeclarationFromIdentifier(name->AsIdentifier());
}

std::vector<ir::AstNode *> FixClassIncorrectlyImplementsInterface::GetInterfaceDefinitions(
    ir::ClassDefinition *classDef)
{
    std::vector<ir::AstNode *> interfaces;
    if (classDef == nullptr) {
        return interfaces;
    }

    auto implements = classDef->Implements();
    for (auto *implement : implements) {
        auto *interfaceDef = FindInterfaceDefinition(implement);
        if (interfaceDef != nullptr) {
            interfaces.push_back(interfaceDef);
        }
    }
    return interfaces;
}

std::string FixClassIncorrectlyImplementsInterface::GenerateMemberSignature(ir::MethodDefinition *methodDef,
                                                                            const std::string &memberName)
{
    if (methodDef->IsGetter()) {
        return "get " + memberName;
    }
    if (methodDef->IsSetter()) {
        return "set " + memberName;
    }
    return "";
}

bool FixClassIncorrectlyImplementsInterface::IsMemberImplemented(ir::ClassDefinition *classDef,
                                                                 const std::string &memberSignature)
{
    if (classDef == nullptr) {
        return false;
    }

    auto classBody = classDef->Body();
    for (auto *member : classBody) {
        if (!member->IsMethodDefinition()) {
            continue;
        }

        auto *methodDef = member->AsMethodDefinition();
        auto *methodId = methodDef->Key()->AsIdentifier();
        std::string currentMemberName = std::string(methodId->Name());
        std::string currentSignature = GenerateMemberSignature(methodDef, currentMemberName);
        if (currentSignature.empty()) {
            continue;
        }

        if (currentSignature == memberSignature) {
            return true;
        }
    }
    return false;
}

void FixClassIncorrectlyImplementsInterface::ProcessInterfaceMembers(ir::TSInterfaceDeclaration *interface,
                                                                     ir::ClassDefinition *classDef,
                                                                     std::vector<ir::AstNode *> &missingMembers)
{
    auto *interfaceBodyNode = interface->Body();
    if (interfaceBodyNode == nullptr) {
        return;
    }

    auto interfaceBody = interfaceBodyNode->Body();
    for (auto *member : interfaceBody) {
        if (!member->IsMethodDefinition()) {
            continue;
        }

        auto *methodDef = member->AsMethodDefinition();
        auto *methodId = methodDef->Key()->AsIdentifier();
        std::string memberName = std::string(methodId->Name());
        std::string memberSignature = GenerateMemberSignature(methodDef, memberName);
        if (memberSignature.empty()) {
            continue;
        }

        if (!IsMemberImplemented(classDef, memberSignature)) {
            missingMembers.push_back(member);
        }
    }
}

std::vector<ir::AstNode *> FixClassIncorrectlyImplementsInterface::FindMissingInterfaceMembers(
    ir::ClassDefinition *classDef)
{
    std::vector<ir::AstNode *> missingMembers;
    if (classDef == nullptr) {
        return missingMembers;
    }

    auto interfaces = GetInterfaceDefinitions(classDef);
    for (auto *interfaceDef : interfaces) {
        if (interfaceDef->IsTSInterfaceDeclaration()) {
            auto *interface = interfaceDef->AsTSInterfaceDeclaration();
            ProcessInterfaceMembers(interface, classDef, missingMembers);
        }
    }

    return missingMembers;
}

// NOLINTNEXTLINE
AutoCodeFixRegister<FixClassIncorrectlyImplementsInterface> g_FixClassIncorrectlyImplementsInterfaceForGetter(
    FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER.GetFixId().data());

// NOLINTNEXTLINE
AutoCodeFixRegister<FixClassIncorrectlyImplementsInterface> g_FixClassIncorrectlyImplementsInterfaceForSetter(
    FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER.GetFixId().data());

}  // namespace ark::es2panda::lsp
