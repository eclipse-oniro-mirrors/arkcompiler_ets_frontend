/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "checker/ets/unusedVarsAnalyzer.h"

#include "checker/types/ets/etsObjectType.h"
#include "generated/diagnostic.h"
#include "generated/tokenType.h"
#include "ir/base/catchClause.h"
#include "ir/base/classDefinition.h"
#include "ir/base/classElement.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/spreadElement.h"
#include "ir/ets/etsDestructuring.h"
#include "ir/ets/etsImportDeclaration.h"
#include "ir/ets/etsParameterExpression.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/memberExpression.h"
#include "ir/module/importDefaultSpecifier.h"
#include "ir/module/importNamespaceSpecifier.h"
#include "ir/module/importSpecifier.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/functionDeclaration.h"
#include "ir/statements/variableDeclarator.h"
#include "ir/ts/tsEnumDeclaration.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "parser/program/program.h"
#include "varbinder/declaration.h"
#include "varbinder/scope.h"
#include "varbinder/variable.h"
#include "varbinder/variableFlags.h"

#include <algorithm>
#include <string_view>

namespace ark::es2panda::checker {

static bool ShouldSkipVariable(const varbinder::Variable *const variable)
{
    if (variable == nullptr) {
        return false;
    }

    if (variable->HasFlag(varbinder::VariableFlags::SYNTHETIC) ||
        variable->HasFlag(varbinder::VariableFlags::LOCAL_EXPORT)) {
        return true;
    }

    const auto *const decl = variable->Declaration();
    const auto *const declNode = decl != nullptr ? decl->Node() : nullptr;
    return declNode != nullptr && (declNode->IsExported() || declNode->IsDefaultExported() ||
                                   declNode->HasExportAlias() || declNode->IsDeclare());
}

static bool SourcePositionMatchesName(const lexer::SourcePosition &position, const util::StringView name)
{
    if (position.Program() == nullptr || name.Empty()) {
        return false;
    }

    const auto source = position.Program()->SourceCode();
    const auto nameText = std::string_view {name.Utf8()};
    return position.index + nameText.size() <= source.size() &&
           source.substr(position.index, nameText.size()) == nameText;
}

static const ir::ETSImportDeclaration *GetOwnerImportDeclaration(const ir::AstNode *node)
{
    while (node != nullptr && !node->IsETSImportDeclaration()) {
        node = node->Parent();
    }

    return node != nullptr ? node->AsETSImportDeclaration() : nullptr;
}

static bool IsCallCallee(const ir::Identifier *const ident)
{
    if (ident == nullptr || ident->Parent() == nullptr || !ident->Parent()->IsCallExpression()) {
        return false;
    }

    return ident->Parent()->AsCallExpression()->Callee() == ident;
}

static bool HasUserVisibleVariableDeclaration(const varbinder::Variable *const variable, const util::StringView name)
{
    if (variable == nullptr || variable->Declaration() == nullptr || name.Empty()) {
        return false;
    }

    const auto *const declNode = variable->Declaration()->Node();
    if (declNode == nullptr) {
        return false;
    }

    if (declNode->IsIdentifier()) {
        return SourcePositionMatchesName(declNode->AsIdentifier()->Start(), name);
    }

    if (declNode->IsVariableDeclarator()) {
        const auto *const id = declNode->AsVariableDeclarator()->Id();
        return id != nullptr && id->IsIdentifier() && SourcePositionMatchesName(id->AsIdentifier()->Start(), name);
    }

    if (declNode->IsImportSpecifier()) {
        const auto *const local = declNode->AsImportSpecifier()->Local();
        return local != nullptr && SourcePositionMatchesName(local->Start(), name);
    }

    if (declNode->IsImportDefaultSpecifier()) {
        const auto *const local = declNode->AsImportDefaultSpecifier()->Local();
        return local != nullptr && SourcePositionMatchesName(local->Start(), name);
    }

    if (declNode->IsImportNamespaceSpecifier()) {
        const auto *const local = declNode->AsImportNamespaceSpecifier()->Local();
        return local != nullptr && SourcePositionMatchesName(local->Start(), name);
    }

    return SourcePositionMatchesName(declNode->Start(), name);
}

UnusedVarsAnalyzer::UnusedVarsAnalyzer(util::DiagnosticEngine &diagnosticEngine) : diagnosticEngine_(diagnosticEngine)
{
}

bool UnusedVarsAnalyzer::HasUserVisibleDeclarationSource(const DeclarationInfo &info) const
{
    return SourcePositionMatchesName(info.position, info.name);
}

bool UnusedVarsAnalyzer::IsArkUIComponentImport(const DeclarationInfo &info) const
{
    if (info.kind != DeclarationKind::IMPORT || !HasUserVisibleDeclarationSource(info)) {
        return false;
    }

    const auto *const importDecl = GetOwnerImportDeclaration(info.declNode);
    if (importDecl == nullptr || importDecl->Source() == nullptr) {
        return false;
    }

    const auto source = std::string_view {importDecl->Source()->Str().Utf8()};
    return source == "@ohos.arkui.component" || source == "@kit.ArkUI";
}

bool UnusedVarsAnalyzer::IsArkUIDslCallReference(const DeclarationInfo &info, const ir::Identifier *const ident) const
{
    return IsArkUIComponentImport(info) && IsCallCallee(ident) &&
           SourcePositionMatchesName(ident->Start(), ident->Name());
}

void UnusedVarsAnalyzer::Analyze(const ir::AstNode *const node)
{
    if (node == nullptr) {
        return;
    }

    CollectDeclarations(node);
    MarkReferences(node);
    EmitWarnings();
}

bool UnusedVarsAnalyzer::ShouldSkipDeclaration(const ir::AstNode *const node) const
{
    return node == nullptr || node->IsExported() || node->IsDefaultExported() || node->HasExportAlias() ||
           node->IsDeclare();
}

void UnusedVarsAnalyzer::RegisterDeclaration(const varbinder::Variable *const variable, const util::StringView name,
                                             const lexer::SourcePosition &position, const DeclarationKind kind,
                                             const ir::AstNode *const declNode)
{
    if (variable == nullptr || ShouldSkipVariable(variable)) {
        return;
    }

    declarations_.try_emplace(variable, DeclarationInfo {variable, name, position, kind, declNode});
}

void UnusedVarsAnalyzer::RegisterNamedDeclaration(const varbinder::Variable *const variable,
                                                  const util::StringView name, const lexer::SourcePosition &position,
                                                  const DeclarationKind kind, const ir::AstNode *const declNode)
{
    if (name.Empty() || ShouldSkipDeclaration(declNode)) {
        return;
    }

    const auto hasSameDeclaration = std::any_of(
        namedDeclarations_.begin(), namedDeclarations_.end(), [variable, name, kind, declNode](const auto &info) {
            return info.variable == variable && info.name == name && info.kind == kind && info.declNode == declNode;
        });
    if (hasSameDeclaration) {
        return;
    }

    namedDeclarations_.push_back(DeclarationInfo {variable, name, position, kind, declNode});
}

static const ir::AstNode *GetOwnerClassDefinition(const ir::AstNode *node)
{
    while (node != nullptr && !node->IsClassDefinition()) {
        node = node->Parent();
    }

    return node;
}

static std::pair<const ir::AstNode *, std::string> MakeScopedMemberKey(const ir::AstNode *const node,
                                                                       const util::StringView name)
{
    return {GetOwnerClassDefinition(node), std::string {name.Utf8()}};
}

static const ir::AstNode *GetNamespaceOwnerClassDefinition(const ir::MemberExpression *const memberExpr)
{
    if (memberExpr == nullptr || memberExpr->Object() == nullptr || !memberExpr->Object()->IsIdentifier()) {
        return nullptr;
    }

    const auto *const variable = memberExpr->Object()->AsIdentifier()->Variable();
    if (variable == nullptr || variable->Declaration() == nullptr || variable->Declaration()->Node() == nullptr ||
        !variable->Declaration()->Node()->IsClassDefinition()) {
        return nullptr;
    }

    const auto *const node = variable->Declaration()->Node();
    return node->AsClassDefinition()->IsNamespaceTransformed() ? node : nullptr;
}

static bool IsUserClassDefinition(const ir::AstNode *const node)
{
    if (node == nullptr || !node->IsClassDefinition()) {
        return false;
    }

    const auto *const classDefinition = node->AsClassDefinition();
    return !classDefinition->IsNamespaceTransformed() && !classDefinition->IsGlobal() &&
           !classDefinition->IsFromExternal() && !classDefinition->IsEnumTransformed();
}

static const ir::AstNode *GetClassOwnerClassDefinition(const ir::MemberExpression *const memberExpr)
{
    if (memberExpr == nullptr || memberExpr->Object() == nullptr || !memberExpr->Object()->IsIdentifier()) {
        return nullptr;
    }

    const auto *const variable = memberExpr->Object()->AsIdentifier()->Variable();
    if (variable == nullptr || variable->Declaration() == nullptr || variable->Declaration()->Node() == nullptr ||
        !IsUserClassDefinition(variable->Declaration()->Node())) {
        return nullptr;
    }

    return variable->Declaration()->Node();
}

static const ir::AstNode *GetInstanceOwnerClassDefinition(const ir::MemberExpression *const memberExpr)
{
    if (memberExpr == nullptr || memberExpr->ObjType() == nullptr) {
        return nullptr;
    }

    const auto *const objectType = memberExpr->ObjType()->GetOriginalBaseType();
    if (objectType == nullptr || !objectType->HasObjectFlag(ETSObjectFlags::CLASS)) {
        return nullptr;
    }

    const auto *const declNode = objectType->GetDeclNode();
    return IsUserClassDefinition(declNode) ? declNode : nullptr;
}

static bool IsLexicalScopedMemberAccess(const ir::MemberExpression *const memberExpr)
{
    return memberExpr != nullptr && memberExpr->Object() != nullptr &&
           (memberExpr->Object()->IsThisExpression() || memberExpr->Object()->IsSuperExpression());
}

static std::pair<const ir::AstNode *, std::string> MakeScopedMemberReferenceKey(
    const ir::MemberExpression *const memberExpr, const util::StringView name)
{
    const auto *const namespaceOwner = GetNamespaceOwnerClassDefinition(memberExpr);
    if (namespaceOwner != nullptr) {
        return {namespaceOwner, std::string {name.Utf8()}};
    }

    const auto *const classOwner = GetClassOwnerClassDefinition(memberExpr);
    if (classOwner != nullptr) {
        return {classOwner, std::string {name.Utf8()}};
    }

    const auto *const instanceOwner = GetInstanceOwnerClassDefinition(memberExpr);
    if (instanceOwner != nullptr) {
        return {instanceOwner, std::string {name.Utf8()}};
    }

    if (!IsLexicalScopedMemberAccess(memberExpr)) {
        return {nullptr, std::string {name.Utf8()}};
    }

    return {GetOwnerClassDefinition(memberExpr), std::string {name.Utf8()}};
}

void UnusedVarsAnalyzer::RegisterScopedMemberDeclaration(const ir::Identifier *const ident,
                                                         const ir::AstNode *const declNode, const DeclarationKind kind)
{
    if (ident == nullptr || ident->Name().Empty() || ShouldSkipDeclaration(declNode)) {
        return;
    }
    if (ShouldSkipVariable(ident->Variable())) {
        return;
    }

    const auto key = MakeScopedMemberKey(declNode, ident->Name());
    if (key.first == nullptr) {
        return;
    }

    scopedMemberDeclarations_.try_emplace(
        key, DeclarationInfo {ident->Variable(), ident->Name(), ident->Start(), kind, declNode});
}

void UnusedVarsAnalyzer::TryCollectIdentifier(const ir::Identifier *const ident, const DeclarationKind kind,
                                              const ir::AstNode *const declNode)
{
    if (ident == nullptr || ShouldSkipDeclaration(declNode)) {
        return;
    }

    declarationNameNodes_.insert(ident);
    if (ident->Variable() != nullptr) {
        RegisterDeclaration(ident->Variable(), ident->Name(), ident->Start(), kind, declNode);
    }

    if (kind == DeclarationKind::IMPORT) {
        RegisterNamedDeclaration(ident->Variable(), ident->Name(), ident->Start(), kind, declNode);
    }

    if (kind == DeclarationKind::PRIVATE_MEMBER) {
        RegisterScopedMemberDeclaration(ident, declNode, kind);
        return;
    }

    if (ident->Variable() == nullptr) {
        RegisterNamedDeclaration(ident->Variable(), ident->Name(), ident->Start(), kind, declNode);
    }
}

static const ir::Identifier *GetClassIdentifier(const ir::ClassDeclaration *const classDecl)
{
    if (classDecl == nullptr || classDecl->Definition() == nullptr || classDecl->Definition()->Ident() == nullptr ||
        classDecl->Definition()->IsGlobal() || classDecl->Definition()->IsFromExternal()) {
        return nullptr;
    }

    return classDecl->Definition()->Ident();
}

static bool IsCollectableVariableDeclaration(const varbinder::Variable *const variable)
{
    if (variable == nullptr) {
        return false;
    }

    const auto *const decl = variable->Declaration();
    return decl != nullptr && (decl->IsVarDecl() || decl->IsLetDecl() || decl->IsConstDecl());
}

static const ir::Identifier *GetFunctionIdentifier(const ir::FunctionDeclaration *const functionDecl)
{
    if (functionDecl == nullptr || functionDecl->IsAnonymous() || functionDecl->Function() == nullptr) {
        return nullptr;
    }

    return functionDecl->Function()->Id();
}

static bool IsCollectablePrivateMember(const ir::AstNode *const node)
{
    if (node == nullptr || !(node->IsClassProperty() || node->IsMethodDefinition() || node->IsOverloadDeclaration())) {
        return false;
    }

    const auto *const element = node->AsClassElement();
    return (element->IsPrivateElement() || element->IsPrivate()) && element->Id() != nullptr;
}

static bool IsCollectableNamespaceMember(const ir::AstNode *const node)
{
    if (node == nullptr || !(node->IsClassProperty() || node->IsMethodDefinition() || node->IsOverloadDeclaration()) ||
        node->Parent() == nullptr || !node->Parent()->IsClassDefinition() ||
        !node->Parent()->AsClassDefinition()->IsNamespaceTransformed()) {
        return false;
    }

    return node->AsClassElement()->Id() != nullptr;
}

static bool IsCollectableGlobalVariableMember(const ir::AstNode *const node)
{
    if (node == nullptr || !node->IsClassProperty() || node->Parent() == nullptr ||
        !node->Parent()->IsClassDefinition() || !node->Parent()->AsClassDefinition()->IsGlobal()) {
        return false;
    }

    return node->AsClassElement()->Id() != nullptr;
}

static const ir::TSEnumDeclaration *GetOriginalEnumDeclaration(const ir::ClassDeclaration *const classDecl)
{
    if (classDecl == nullptr || classDecl->Definition() == nullptr || !classDecl->Definition()->IsEnumTransformed()) {
        return nullptr;
    }

    return classDecl->Definition()->OrigEnumDecl();
}

static bool IsGlobalMethodDefinition(const ir::AstNode *const node)
{
    return node != nullptr && node->IsMethodDefinition() && node->Parent() != nullptr &&
           node->Parent()->IsClassDefinition() && node->Parent()->AsClassDefinition()->IsGlobal();
}

void UnusedVarsAnalyzer::CollectBindingIdentifiers(const ir::AstNode *const node, const DeclarationKind kind,
                                                   const ir::AstNode *const declNode)
{
    if (node == nullptr) {
        return;
    }

    if (node->IsIdentifier()) {
        const auto *const ident = node->AsIdentifier();
        declarationNameNodes_.insert(ident);
        if (IsCollectableVariableDeclaration(ident->Variable()) && !ShouldSkipDeclaration(declNode)) {
            RegisterDeclaration(ident->Variable(), ident->Name(), ident->Start(), kind, declNode);
        }
        return;
    }

    if (node->IsRestElement()) {
        CollectBindingIdentifiers(node->AsRestElement()->Argument(), kind, declNode);
        return;
    }

    node->Iterate(
        [this, kind, declNode](const ir::AstNode *childNode) { CollectBindingIdentifiers(childNode, kind, declNode); });
}

static bool IsEntryPointMethodDefinition(const ir::AstNode *const node)
{
    return node != nullptr && node->IsMethodDefinition() && node->AsMethodDefinition()->Function() != nullptr &&
           node->AsMethodDefinition()->Function()->IsEntryPoint();
}

static void AddMethodFunctionIdentifier(const ir::AstNode *const node,
                                        std::unordered_set<const ir::AstNode *> &declarationNameNodes)
{
    if (node == nullptr || !node->IsMethodDefinition() || node->AsMethodDefinition()->Function() == nullptr ||
        node->AsMethodDefinition()->Function()->Id() == nullptr) {
        return;
    }

    declarationNameNodes.insert(node->AsMethodDefinition()->Function()->Id());
}

static bool IsPureAssignmentLeftHandSide(const ir::AstNode *const node)
{
    if (node == nullptr || node->Parent() == nullptr) {
        return false;
    }

    const auto *const parent = node->Parent();
    if (!parent->IsAssignmentExpression()) {
        return false;
    }

    const auto *const assignment = parent->AsAssignmentExpression();
    return assignment->Left() == node && assignment->OperatorType() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION;
}

static bool IsDestructuringAssignmentTarget(const ir::AstNode *const node)
{
    if (node == nullptr) {
        return false;
    }

    const auto *current = node;
    while (current->Parent() != nullptr && current->Parent()->IsETSDestructuring()) {
        current = current->Parent();
    }

    if (current == node || current->Parent() == nullptr || !current->Parent()->IsAssignmentExpression()) {
        return false;
    }

    const auto *const assignment = current->Parent()->AsAssignmentExpression();
    return assignment->Left() == current && assignment->OperatorType() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION;
}

static bool IsAssignedMemberProperty(const ir::AstNode *const node)
{
    if (node == nullptr || node->Parent() == nullptr || !node->Parent()->IsMemberExpression()) {
        return false;
    }

    const auto *const memberExpr = node->Parent()->AsMemberExpression();
    return memberExpr->Property() == node && IsPureAssignmentLeftHandSide(memberExpr);
}

static bool IsSetterDeclaration(const ir::AstNode *const declNode)
{
    return declNode != nullptr && declNode->IsMethodDefinition() && declNode->AsMethodDefinition()->IsSetter();
}

void UnusedVarsAnalyzer::CollectImportDeclaration(const ir::AstNode *const node)
{
    if (node == nullptr || !node->IsETSImportDeclaration()) {
        return;
    }

    for (const auto *specifier : node->AsETSImportDeclaration()->Specifiers()) {
        if (specifier->IsImportSpecifier()) {
            const auto *const importSpecifier = specifier->AsImportSpecifier();
            declarationNameNodes_.insert(importSpecifier->Imported());
            TryCollectIdentifier(importSpecifier->Local(), DeclarationKind::IMPORT, specifier);
        } else if (specifier->IsImportDefaultSpecifier()) {
            TryCollectIdentifier(specifier->AsImportDefaultSpecifier()->Local(), DeclarationKind::IMPORT, specifier);
        } else if (specifier->IsImportNamespaceSpecifier()) {
            TryCollectIdentifier(specifier->AsImportNamespaceSpecifier()->Local(), DeclarationKind::IMPORT, specifier);
        }
    }
}

void UnusedVarsAnalyzer::CollectClassOrTypeDeclaration(const ir::AstNode *const node, bool &skipChildren)
{
    if (node == nullptr) {
        return;
    }

    if (node->IsClassDeclaration()) {
        const auto *const enumDecl = GetOriginalEnumDeclaration(node->AsClassDeclaration());
        if (enumDecl != nullptr) {
            declarationNameNodes_.insert(GetClassIdentifier(node->AsClassDeclaration()));
            TryCollectIdentifier(enumDecl->Key(), DeclarationKind::ENUM, enumDecl);
            skipChildren = true;
        } else {
            TryCollectIdentifier(GetClassIdentifier(node->AsClassDeclaration()), DeclarationKind::CLASS, node);
        }
        return;
    }

    if (node->IsTSEnumDeclaration()) {
        TryCollectIdentifier(node->AsTSEnumDeclaration()->Key(), DeclarationKind::ENUM, node);
        return;
    }

    if (node->IsTSInterfaceDeclaration()) {
        const auto *const interfaceDecl = node->AsTSInterfaceDeclaration();
        if (!interfaceDecl->IsFromExternal()) {
            TryCollectIdentifier(interfaceDecl->Id(), DeclarationKind::INTERFACE, node);
        }
        return;
    }

    if (node->IsTSTypeAliasDeclaration()) {
        TryCollectIdentifier(node->AsTSTypeAliasDeclaration()->Id(), DeclarationKind::TYPE_ALIAS, node);
    }
}

void UnusedVarsAnalyzer::CollectFunctionOrMemberDeclaration(const ir::AstNode *const node)
{
    if (node == nullptr) {
        return;
    }

    if (node->IsFunctionDeclaration()) {
        const auto *const ident = GetFunctionIdentifier(node->AsFunctionDeclaration());
        if (ident != nullptr) {
            TryCollectIdentifier(ident, DeclarationKind::FUNCTION, node);
        }
        return;
    }

    if (IsGlobalMethodDefinition(node)) {
        AddMethodFunctionIdentifier(node, declarationNameNodes_);
        if (!IsEntryPointMethodDefinition(node)) {
            const auto *const ident = node->AsClassElement()->Id();
            TryCollectIdentifier(ident, DeclarationKind::FUNCTION, node);
            RegisterScopedMemberDeclaration(ident, node, DeclarationKind::FUNCTION);
        }
        return;
    }

    if (IsCollectablePrivateMember(node)) {
        TryCollectIdentifier(node->AsClassElement()->Id(), DeclarationKind::PRIVATE_MEMBER, node);
        AddMethodFunctionIdentifier(node, declarationNameNodes_);
    }
}

void UnusedVarsAnalyzer::CollectLocalDeclaration(const ir::AstNode *const node)
{
    if (node == nullptr) {
        return;
    }

    if (node->IsVariableDeclarator()) {
        const auto *const id = node->AsVariableDeclarator()->Id();
        if (id != nullptr && id->IsIdentifier()) {
            const auto *const ident = id->AsIdentifier();
            declarationNameNodes_.insert(ident);
            if (IsCollectableVariableDeclaration(ident->Variable()) && !ShouldSkipDeclaration(node)) {
                RegisterDeclaration(ident->Variable(), ident->Name(), ident->Start(), DeclarationKind::VARIABLE, node);
            }
        } else if (id != nullptr && id->IsETSDestructuring()) {
            CollectBindingIdentifiers(id, DeclarationKind::VARIABLE, node);
        }
        return;
    }

    if (node->IsCatchClause()) {
        const auto *const param = node->AsCatchClause()->Param();
        if (param != nullptr && param->IsIdentifier()) {
            TryCollectIdentifier(param->AsIdentifier(), DeclarationKind::PARAMETER, node);
        }
        return;
    }

    if (!node->IsScriptFunction()) {
        return;
    }

    const auto *const function = node->AsScriptFunction();
    if (!function->HasBody()) {
        return;
    }

    for (const auto *param : function->Params()) {
        if (param->IsETSParameterExpression()) {
            TryCollectIdentifier(param->AsETSParameterExpression()->Ident(), DeclarationKind::PARAMETER, param);
        }
    }
}

void UnusedVarsAnalyzer::CollectCurrentNodeDeclaration(const ir::AstNode *const node, bool &skipChildren)
{
    if (node == nullptr) {
        return;
    }

    if (node->IsETSImportDeclaration()) {
        CollectImportDeclaration(node);
        return;
    }

    if (node->IsClassDeclaration() || node->IsTSEnumDeclaration() || node->IsTSInterfaceDeclaration() ||
        node->IsTSTypeAliasDeclaration()) {
        CollectClassOrTypeDeclaration(node, skipChildren);
        return;
    }

    if (node->IsFunctionDeclaration() || IsGlobalMethodDefinition(node) || IsCollectablePrivateMember(node)) {
        CollectFunctionOrMemberDeclaration(node);
        return;
    }

    if (IsCollectableGlobalVariableMember(node)) {
        const auto *const ident = node->AsClassElement()->Id();
        declarationNameNodes_.insert(ident);
        if (ident->Variable() != nullptr) {
            RegisterDeclaration(ident->Variable(), ident->Name(), ident->Start(), DeclarationKind::GLOBAL_VARIABLE,
                                node);
        }
        RegisterScopedMemberDeclaration(ident, node, DeclarationKind::GLOBAL_VARIABLE);
        return;
    }

    if (IsCollectableNamespaceMember(node)) {
        const auto *const ident = node->AsClassElement()->Id();
        declarationNameNodes_.insert(ident);
        if (ident->Variable() != nullptr) {
            RegisterDeclaration(ident->Variable(), ident->Name(), ident->Start(), DeclarationKind::NAMESPACE_MEMBER,
                                node);
        }
        RegisterScopedMemberDeclaration(ident, node, DeclarationKind::NAMESPACE_MEMBER);
        AddMethodFunctionIdentifier(node, declarationNameNodes_);
        return;
    }

    if (node->IsVariableDeclarator() || node->IsCatchClause() || node->IsScriptFunction()) {
        CollectLocalDeclaration(node);
    }
}

void UnusedVarsAnalyzer::CollectDeclarations(const ir::AstNode *const node)
{
    if (node == nullptr) {
        return;
    }

    bool skipChildren = false;
    CollectCurrentNodeDeclaration(node, skipChildren);

    if (skipChildren) {
        return;
    }

    node->Iterate([this](const ir::AstNode *childNode) { CollectDeclarations(childNode); });
}

bool UnusedVarsAnalyzer::IsReadReference(const ir::AstNode *const node) const
{
    return !IsPureAssignmentLeftHandSide(node) && !IsDestructuringAssignmentTarget(node) &&
           !IsAssignedMemberProperty(node);
}

void UnusedVarsAnalyzer::TryMarkReference(const ir::Identifier *const ident)
{
    if (ident == nullptr || declarationNameNodes_.find(ident) != declarationNameNodes_.end() ||
        !IsReadReference(ident)) {
        return;
    }

    const auto *const variable = ident->Variable();
    if (variable != nullptr && declarations_.find(variable) != declarations_.end()) {
        references_.insert(variable);
        return;
    }

    for (const auto &[declarationVariable, info] : declarations_) {
        if (info.kind == DeclarationKind::ENUM && info.name == ident->Name()) {
            references_.insert(declarationVariable);
            return;
        }
    }

    const auto scopedMember = scopedMemberDeclarations_.find(MakeScopedMemberKey(ident, ident->Name()));
    if (scopedMember != scopedMemberDeclarations_.end()) {
        scopedMemberReferences_.insert(scopedMember->first);
        if (scopedMember->second.variable != nullptr) {
            references_.insert(scopedMember->second.variable);
        }
        return;
    }

    for (const auto &info : namedDeclarations_) {
        if (info.name != ident->Name()) {
            continue;
        }

        // UI plugins may rebind ArkUI DSL calls; keep this fallback guarded against user-visible shadowing.
        if (variable != nullptr && info.variable != variable &&
            (!IsArkUIDslCallReference(info, ident) || HasUserVisibleVariableDeclaration(variable, ident->Name()))) {
            continue;
        }

        namedReferences_.insert(
            NamedDeclarationKey {info.variable, info.declNode, info.kind, std::string {info.name.Utf8()}});
    }
}

void UnusedVarsAnalyzer::TryMarkPrivateMemberReference(const ir::MemberExpression *const memberExpr)
{
    if (memberExpr == nullptr || memberExpr->IsComputed()) {
        return;
    }

    const auto *const property = memberExpr->Property();
    if (property == nullptr || !property->IsIdentifier()) {
        return;
    }

    const auto *const ident = property->AsIdentifier();
    const auto key = MakeScopedMemberReferenceKey(memberExpr, ident->Name());
    const auto declaration = scopedMemberDeclarations_.find(key);
    if (declaration != scopedMemberDeclarations_.end()) {
        if (!IsReadReference(ident) && !IsSetterDeclaration(declaration->second.declNode)) {
            return;
        }
        scopedMemberReferences_.insert(key);
        if (declaration->second.variable != nullptr) {
            references_.insert(declaration->second.variable);
        }
        return;
    }

    // Non-computed member properties are names on the receiver, not standalone lexical references.
}

void UnusedVarsAnalyzer::MarkReferences(const ir::AstNode *const node)
{
    if (node == nullptr) {
        return;
    }

    if (node->IsClassDeclaration() && GetOriginalEnumDeclaration(node->AsClassDeclaration()) != nullptr) {
        return;
    }

    if (node->IsMemberExpression()) {
        const auto *const memberExpr = node->AsMemberExpression();
        TryMarkPrivateMemberReference(memberExpr);
        MarkReferences(memberExpr->Object());
        if (memberExpr->IsComputed()) {
            MarkReferences(memberExpr->Property());
        }
        return;
    }

    if (node->IsIdentifier()) {
        TryMarkReference(node->AsIdentifier());
    }

    node->Iterate([this](const ir::AstNode *childNode) { MarkReferences(childNode); });
}

void UnusedVarsAnalyzer::EmitWarnings() const
{
    for (const auto &[variable, info] : declarations_) {
        const auto scopedMemberReference = MakeScopedMemberKey(info.declNode, info.name);
        const auto isReferencedByVariable = references_.find(variable) != references_.end();
        const auto isScopedMember =
            info.kind == DeclarationKind::FUNCTION || info.kind == DeclarationKind::GLOBAL_VARIABLE ||
            info.kind == DeclarationKind::PRIVATE_MEMBER || info.kind == DeclarationKind::NAMESPACE_MEMBER;
        const auto isReferencedByScopedMember =
            isScopedMember && scopedMemberReferences_.find(scopedMemberReference) != scopedMemberReferences_.end();
        if (isReferencedByVariable || isReferencedByScopedMember || !HasUserVisibleDeclarationSource(info)) {
            continue;
        }

        diagnosticEngine_.LogDiagnostic(diagnostic::UNUSED_SYMBOL, util::DiagnosticMessageParams {info.name},
                                        info.position);
    }

    for (const auto &[key, info] : scopedMemberDeclarations_) {
        if (scopedMemberReferences_.find(key) != scopedMemberReferences_.end() ||
            !HasUserVisibleDeclarationSource(info) ||
            (info.variable != nullptr && declarations_.find(info.variable) != declarations_.end())) {
            continue;
        }

        diagnosticEngine_.LogDiagnostic(diagnostic::UNUSED_SYMBOL, util::DiagnosticMessageParams {info.name},
                                        info.position);
    }

    for (const auto &info : namedDeclarations_) {
        const auto key = NamedDeclarationKey {info.variable, info.declNode, info.kind, std::string {info.name.Utf8()}};
        if (namedReferences_.find(key) != namedReferences_.end() || !HasUserVisibleDeclarationSource(info)) {
            continue;
        }

        if (info.variable != nullptr && declarations_.find(info.variable) != declarations_.end()) {
            continue;
        }

        diagnosticEngine_.LogDiagnostic(diagnostic::UNUSED_SYMBOL, util::DiagnosticMessageParams {info.name},
                                        info.position);
    }
}

}  // namespace ark::es2panda::checker
