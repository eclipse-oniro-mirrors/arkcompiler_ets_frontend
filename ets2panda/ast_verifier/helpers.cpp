/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "helpers.h"

#include "checker/types/typeFlag.h"
#include "checker/types/type.h"
#include "checker/types/ets/etsObjectType.h"
#include "checker/types/ets/etsUnionType.h"
#include "ir/statements/blockStatement.h"
#include "ir/ets/etsModule.h"
#include "parser/program/program.h"
#include "ir/expressions/memberExpression.h"
#include "ir/expressions/callExpression.h"

namespace ark::es2panda::compiler::ast_verifier {

bool IsImportLike(const ir::AstNode *ast)
{
    return (ast->IsETSImportDeclaration() || ast->IsETSReExportDeclaration() || ast->IsImportExpression() ||
            ast->IsImportSpecifier() || ast->IsImportDefaultSpecifier() || ast->IsImportNamespaceSpecifier());
}

bool IsExportLike(const ir::AstNode *ast)
{
    return (ast->IsExportDefaultDeclaration() || ast->IsExportSpecifier() || ast->IsExportAllDeclaration() ||
            ast->IsExportNamedDeclaration() || ast->IsETSReExportDeclaration());
}

bool IsBooleanType(const ir::AstNode *ast)
{
    if (ast == nullptr) {
        return false;
    }

    if (!ast->IsTyped()) {
        return false;
    }

    auto typedAst = static_cast<const ir::TypedAstNode *>(ast);
    if (typedAst->TsType() == nullptr) {
        return false;
    }

    auto type = typedAst->TsType();
    if (type->HasTypeFlag(checker::TypeFlag::ETS_OBJECT)) {
        return type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::BUILTIN_BOOLEAN);
    }

    return type->HasTypeFlag(checker::TypeFlag::ETS_BOOLEAN) || type->HasTypeFlag(checker::TypeFlag::BOOLEAN_LIKE);
}

bool IsValidTypeForBinaryOp(const ir::AstNode *ast, bool isBitwise)
{
    if (ast == nullptr) {
        return false;
    }

    if (!ast->IsTyped()) {
        return false;
    }

    auto typedAst = static_cast<const ir::TypedAstNode *>(ast);
    if (typedAst->TsType() == nullptr) {
        return false;
    }

    auto type = typedAst->TsType();
    if (IsBooleanType(ast)) {
        return isBitwise;
    }

    if (type->HasTypeFlag(checker::TypeFlag::ETS_OBJECT) &&
        type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::BUILTIN_BIGINT)) {
        return true;
    }

    if (type->HasTypeFlag(checker::TypeFlag::ETS_OBJECT)) {
        return type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::BUILTIN_TYPE) &&
               !type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::BUILTIN_BOOLEAN);
    }

    return type->HasTypeFlag(checker::TypeFlag::ETS_CONVERTIBLE_TO_NUMERIC) ||
           type->HasTypeFlag(checker::TypeFlag::NUMBER_LITERAL) || type->HasTypeFlag(checker::TypeFlag::BIGINT) ||
           type->HasTypeFlag(checker::TypeFlag::BIGINT_LITERAL);
}

bool IsStringType(const ir::AstNode *ast)
{
    if (ast == nullptr) {
        return false;
    }

    if (!ast->IsTyped()) {
        return false;
    }

    auto typedAst = static_cast<const ir::TypedAstNode *>(ast);
    if (typedAst->TsType() == nullptr) {
        return false;
    }

    auto type = typedAst->TsType();
    if (type->HasTypeFlag(checker::TypeFlag::ETS_OBJECT)) {
        return type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::STRING) ||
               type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::BUILTIN_STRING);
    }

    return type->HasTypeFlag(checker::TypeFlag::STRING_LIKE);
}

const checker::Type *GetClassDefinitionType(const ir::AstNode *ast)
{
    const ir::AstNode *tmpNode = ast;
    while (tmpNode->Parent() != nullptr && !tmpNode->IsClassDefinition()) {
        tmpNode = tmpNode->Parent();
    }
    if (!tmpNode->IsClassDefinition()) {
        return nullptr;
    }
    return tmpNode->AsClassDefinition()->TsType();
}

const checker::Type *GetTSInterfaceDeclarationType(const ir::AstNode *ast)
{
    const ir::AstNode *tmpNode = ast;
    while (tmpNode->Parent() != nullptr && !tmpNode->IsTSInterfaceDeclaration()) {
        tmpNode = tmpNode->Parent();
    }
    if (!tmpNode->IsTSInterfaceDeclaration()) {
        return nullptr;
    }
    return tmpNode->AsTSInterfaceDeclaration()->TsType();
}

bool ValidateMethodAccessForClass(const ir::AstNode *ast, const ir::AstNode *ownerSignDeclNode,
                                  checker::Signature *signature)
{
    // Check if the method is used where it is declared
    if (IsContainedIn<const ir::AstNode>(ast, ownerSignDeclNode)) {
        return true;
    }
    if (signature->HasSignatureFlag(checker::SignatureFlags::PRIVATE)) {
        return false;
    }
    if (signature->HasSignatureFlag(checker::SignatureFlags::PROTECTED)) {
        // Check if the method is inherited and is used in class in which it is inherited
        auto *classDefinitionType = GetClassDefinitionType(ast);
        if (classDefinitionType == nullptr || !classDefinitionType->IsETSObjectType()) {
            return false;
        }
        auto *classObjectType = classDefinitionType->AsETSObjectType();
        return classObjectType->IsDescendantOf(signature->Owner());
    }
    return true;
}

bool ValidateMethodAccessForTSInterface(const ir::AstNode *ast, const ir::AstNode *ownerSignDeclNode,
                                        checker::Signature *signature)
{
    // Check if the method is used where it is declared
    if (IsContainedIn<const ir::AstNode>(ast, ownerSignDeclNode)) {
        return true;
    }
    if (signature->HasSignatureFlag(checker::SignatureFlags::PRIVATE)) {
        return false;
    }
    if (signature->HasSignatureFlag(checker::SignatureFlags::PROTECTED)) {
        // Check if the method is inherited and is used in class in which it is inherited
        auto *tsInterfaceDeclarationType = GetTSInterfaceDeclarationType(ast);
        if (tsInterfaceDeclarationType == nullptr || !tsInterfaceDeclarationType->IsETSObjectType()) {
            return false;
        }
        auto *tsInterfaceObjectType = tsInterfaceDeclarationType->AsETSObjectType();
        return tsInterfaceObjectType->IsDescendantOf(signature->Owner());
    }
    return true;
}

bool ValidatePropertyAccessForClass(const ir::AstNode *ast, const ir::AstNode *propVarDeclNode,
                                    const ir::AstNode *propVarDeclNodeParent, const varbinder::LocalVariable *propVar)
{
    // Check if the variable is used where it is declared
    if (IsContainedIn<const ir::AstNode>(ast, propVarDeclNodeParent)) {
        return true;
    }
    if (propVarDeclNode->IsPrivate()) {
        return false;
    }
    if (propVarDeclNode->IsProtected()) {
        auto *classDefinitionType = GetClassDefinitionType(ast);
        if (classDefinitionType != nullptr && classDefinitionType->IsETSObjectType()) {
            auto *classObjectType = classDefinitionType->AsETSObjectType();
            return classObjectType->IsPropertyOfAscendant(propVar);
        }
        auto *interfaceDefType = GetTSInterfaceDeclarationType(ast);
        if (interfaceDefType != nullptr && interfaceDefType->IsETSObjectType()) {
            auto *interfaceObjectType = interfaceDefType->AsETSObjectType();
            return interfaceObjectType->IsPropertyOfAscendant(propVar);
        }
        return false;
    }
    return true;
}

bool ValidateVariableAccess(const varbinder::LocalVariable *propVar, const ir::MemberExpression *ast)
{
    const auto *propVarDecl = propVar->Declaration();
    if (propVarDecl == nullptr) {
        return false;
    }
    const auto *propVarDeclNode = propVarDecl->Node();
    if (propVarDeclNode == nullptr) {
        return false;
    }

    // NOTE: need to refactor: type of member expression object can be obtained via
    // me->ObjType() or me->Object()->TsType() and they may differ!!!!
    if (auto objType = const_cast<ir::MemberExpression *>(ast)->Object()->TsType();
        objType != nullptr && objType->IsETSUnionType()) {
        bool res = true;
        for (auto type : objType->AsETSUnionType()->ConstituentTypes()) {
            const_cast<ir::MemberExpression *>(ast)->SetObjectType(type->AsETSObjectType());
            // Just to skip enclosing if clause checking whether object tsType is ETSUnionType in subsequent recursive
            // call
            const_cast<ir::MemberExpression *>(ast)->Object()->SetTsType(type->AsETSObjectType());
        }
        const_cast<ir::MemberExpression *>(ast)->SetObjectType(ast->ObjType());
        const_cast<ir::MemberExpression *>(ast)->Object()->SetTsType(objType);
        return res;
    }

    auto *objType = ast->ObjType();
    if (objType == nullptr) {
        return false;
    }
    const auto *objTypeDeclNode = objType->GetDeclNode();
    if (objTypeDeclNode == nullptr) {
        return false;
    }
    if (objTypeDeclNode->Parent() != nullptr && objTypeDeclNode->Parent()->IsImportNamespaceSpecifier()) {
        return true;
    }
    const auto *propVarDeclNodeParent = propVarDeclNode->Parent();
    if (propVarDeclNodeParent == nullptr) {
        return false;
    }
    if ((propVarDeclNodeParent->IsClassDefinition() && objTypeDeclNode->IsClassDefinition()) ||
        (propVarDeclNodeParent->IsTSInterfaceDeclaration() && objTypeDeclNode->IsTSInterfaceDeclaration())) {
        return ValidatePropertyAccessForClass(ast, propVarDeclNode, propVarDeclNodeParent, propVar);
    }
    return false;
}

bool ValidateMethodAccess(const ir::MemberExpression *memberExpression, const ir::CallExpression *ast)
{
    // NOTE: need to refactor: type of member expression object can be obtained via
    // me->ObjType() or me->Object()->TsType() and they may differ!!!!
    if (memberExpression->Object()->TsType() != nullptr) {
        // When calling enum methods member expression
        // object has ETSEnumType instead of ETSObjectType.
        const auto *const type = memberExpression->Object()->TsType();
        if (type->IsETSEnumType()) {
            return true;
        }

        // When calling enum methods member expression
        // object has ETSUnionType instead of ETSObjectType.
        if (type->IsETSUnionType()) {
            return true;
        }
    }

    auto *memberObjType = memberExpression->ObjType();
    if (memberObjType == nullptr) {
        return false;
    }
    if (memberObjType->HasObjectFlag(checker::ETSObjectFlags::RESOLVED_SUPER) &&
        memberObjType->SuperType() != nullptr &&
        memberObjType->SuperType()->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::BUILTIN_TYPE |
                                                                     checker::ETSObjectFlags::GLOBAL)) {
        return true;
    }
    const auto *memberObjTypeDeclNode = memberObjType->GetDeclNode();
    if (memberObjTypeDeclNode == nullptr) {
        return false;
    }
    if (memberObjTypeDeclNode->Parent() != nullptr && memberObjTypeDeclNode->Parent()->IsImportNamespaceSpecifier()) {
        return true;
    }
    auto *signature = ast->Signature();
    if (signature == nullptr) {
        return false;
    }
    auto *ownerSign = signature->Owner();
    if (ownerSign == nullptr) {
        return false;
    }
    auto *ownerSignDeclNode = ownerSign->GetDeclNode();
    if (ownerSignDeclNode == nullptr) {
        return false;
    }
    if (!ownerSignDeclNode->IsClassDefinition() && !ownerSignDeclNode->IsTSInterfaceDeclaration()) {
        return false;
    }
    bool ret = false;
    if (memberObjTypeDeclNode->IsClassDefinition()) {
        ret = ValidateMethodAccessForClass(ast, ownerSignDeclNode, signature);
    } else if (memberObjTypeDeclNode->IsTSInterfaceDeclaration()) {
        ret = ValidateMethodAccessForTSInterface(ast, ownerSignDeclNode, signature);
    }
    return ret;
}

}  // namespace ark::es2panda::compiler::ast_verifier
