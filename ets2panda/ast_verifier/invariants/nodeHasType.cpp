/*
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

#include "ast_verifier/helpers.h"
#include "nodeHasType.h"
#include "ast_verifier/invariantBase.h"
#include "checker/types/ets/etsObjectTypeConstants.h"
#include "ir/astNode.h"
#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/spreadElement.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/ets/etsParameterExpression.h"
#include "ir/statements/annotationDeclaration.h"
#include "ir/ts/tsEnumDeclaration.h"
#include "ir/ts/tsInterfaceBody.h"
#include "ir/ts/tsInterfaceDeclaration.h"

namespace ark::es2panda::compiler::ast_verifier {

// NOTE(dkofanov): These exceptions are inadequate and are not actual exceptions.
class NodeHasType::ExceptionsMatcher {
public:
    explicit ExceptionsMatcher(const ir::AstNode *ast) : nulltypeNode_(ast) {}

    auto ShouldSkipNode() const
    {
        return nulltypeNode_->IsIdentifier() || MatchFunctionExpression() || nulltypeNode_->IsTSClassImplements() ||
               nulltypeNode_->IsSpreadElement() || nulltypeNode_->IsTSThisType() || nulltypeNode_->IsETSNullType() ||
               nulltypeNode_->IsStringLiteral() || AnyChildStringLiteral() || nulltypeNode_->IsOverloadDeclaration() ||
               nulltypeNode_->IsProperty();
    }

    auto ShouldSkipSubtree() const
    {
        return nulltypeNode_->IsAnnotationUsage() || nulltypeNode_->IsVariableDeclarator() || MatchTypeParameter() ||
               nulltypeNode_->IsTSEnumDeclaration() || nulltypeNode_->IsTSInterfaceDeclaration() ||
               nulltypeNode_->IsTSQualifiedName() || nulltypeNode_->IsETSParameterExpression() ||
               nulltypeNode_->IsETSTypeReference() || MatchImportExport() || nulltypeNode_->IsTryStatement() ||
               nulltypeNode_->IsAssignmentExpression();
    }

private:
    bool MatchFunctionExpression() const
    {
        if (!nulltypeNode_->IsFunctionExpression()) {
            return false;
        }
        auto fe = nulltypeNode_->AsFunctionExpression();
        return (fe->Parent() != nullptr) && (fe->Parent()->IsMethodDefinition() || fe->Parent()->IsClassStaticBlock());
    }

    bool MatchTypeParameter() const
    {
        return nulltypeNode_->IsTSTypeParameterInstantiation() || nulltypeNode_->IsTSTypeParameterDeclaration();
    }

    bool MatchImportExport() const
    {
        return nulltypeNode_->IsETSReExportDeclaration() || nulltypeNode_->IsETSImportDeclaration();
    }

    bool AnyChildStringLiteral() const
    {
        return nulltypeNode_->IsAnyChild([](auto *child) { return child->IsStringLiteral(); });
    }

private:
    const ir::AstNode *nulltypeNode_ {};
};

CheckResult NodeHasType::operator()(const ir::AstNode *ast)
{
    type_ = nullptr;

    if (ExceptionsMatcher {ast}.ShouldSkipSubtree()) {
        return {CheckDecision::CORRECT, CheckAction::SKIP_SUBTREE};
    }

    if (!ast->IsTyped()) {
        return {CheckDecision::CORRECT, CheckAction::CONTINUE};
    }

    type_ = ast->AsTyped()->TsType();
    if (type_ != nullptr || ExceptionsMatcher {ast}.ShouldSkipNode()) {
        return {CheckDecision::CORRECT, CheckAction::CONTINUE};
    }

    AddCheckMessage("NULL_TS_TYPE", *ast);
    return {CheckDecision::INCORRECT, CheckAction::CONTINUE};
}

CheckResult EnumHasCorrectType::operator()(const ir::AstNode *ast)
{
    const auto *type = Get<NodeHasType>().type_;
    if (type == nullptr || !type->IsETSEnumType()) {
        return {CheckDecision::CORRECT, CheckAction::CONTINUE};
    }

    auto enumObject = type->AsETSEnumType();

    const checker::Type *enumType =
        (enumObject->EnumAnnotedType() != nullptr) ? enumObject->EnumAnnotedType() : enumObject->Underlying();

    if (enumType->IsETSObjectType()) {
        const auto &asObject = enumType->AsETSObjectType();
        if (asObject->HasObjectFlag(checker::ETSObjectFlags::BUILTIN_NUMERIC) ||
            asObject->HasObjectFlag(checker::ETSObjectFlags::BUILTIN_STRING)) {
            return {CheckDecision::CORRECT, CheckAction::CONTINUE};
        }
    }

    if (enumType->IsIntType() || enumType->IsLongType() || enumType->IsDoubleType() || enumType->IsFloatType() ||
        enumType->IsShortType() || enumType->IsByteType()) {
        return {CheckDecision::CORRECT, CheckAction::CONTINUE};
    }

    AddCheckMessage("ENUM_HAS_WRONG_TYPE", *ast);
    return {CheckDecision::INCORRECT, CheckAction::CONTINUE};
}

}  // namespace ark::es2panda::compiler::ast_verifier
