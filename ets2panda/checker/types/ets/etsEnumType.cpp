/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "etsEnumType.h"

#include "checker/ETSchecker.h"
#include "checker/ets/conversion.h"
#include "checker/types/ets/etsUnionType.h"

namespace ark::es2panda::checker {

void ETSEnumType::SetEnumType(ir::TypeNode *typeNode, ETSChecker *checker) noexcept
{
    ES2PANDA_ASSERT(typeNode != nullptr);
    ES2PANDA_ASSERT(checker != nullptr);

    typeNode->Check(checker);
    enumType_ = typeNode->GetType(checker);
}

Type *ETSEnumType::GetBaseEnumElementType(ETSChecker *checker)
{
    return checker->MaybeUnboxType(SuperType()->TypeArguments()[0]);
}

bool ETSStringEnumType::AssignmentSource(TypeRelation *relation, Type *target)
{
    bool result = false;
    if (target->IsETSObjectType() && target->AsETSObjectType()->IsGlobalETSObjectType()) {
        result = true;
    } else if (target->IsETSStringType()) {
        result = true;
        if (relation->GetNode() != nullptr) {
            relation->GetNode()->AddAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
        }
    } else if (target->IsETSUnionType()) {
        auto &unionConstituentTypes = target->AsETSUnionType()->ConstituentTypes();
        for (auto *constituentType : unionConstituentTypes) {
            if (relation->IsIdenticalTo(this, constituentType)) {
                result = true;
                break;
            }
        }
    }
    relation->Result(result);
    return relation->IsTrue();
}

void ETSStringEnumType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    relation->IsIdenticalTo(this, source) ? relation->Result(true) : relation->Result(false);
}

void ETSStringEnumType::Cast(TypeRelation *const relation, Type *const target)
{
    if (relation->IsIdenticalTo(this, target)) {
        relation->Result(true);
        return;
    }
    if (target->IsETSStringType()) {
        relation->RaiseError(diagnostic::ENUM_DEPRECATED_CAST, {this, target}, relation->GetNode()->Start());
        relation->Result(true);
        return;
    }
    conversion::Forbidden(relation);
}

void ETSStringEnumType::CastTarget(TypeRelation *relation, Type *source)
{
    if (source->IsETSStringType()) {
        relation->RaiseError(diagnostic::ENUM_DEPRECATED_CAST, {source, this}, relation->GetNode()->Start());
        relation->Result(true);
        return;
    }
    conversion::Forbidden(relation);
}

[[nodiscard]] bool ETSStringEnumType::CheckBuiltInType(const ETSChecker *checker, ETSObjectFlags flag) const noexcept
{
    if (enumType_ != nullptr) {
        return enumType_->AsETSObjectType()->HasObjectFlag(flag);
    }

    return checker->GlobalBuiltinETSStringType();
}

bool ETSNumericEnumType::CheckAssignableNumericTypes(Type *let)
{
    auto letObj = let->AsETSObjectType();
    auto enumObj = EnumAnnotedType()->AsETSObjectType();
    ES2PANDA_ASSERT(enumObj->HasObjectFlag(ETSObjectFlags::BUILTIN_NUMERIC));
    if (letObj->HasObjectFlag(ETSObjectFlags::BUILTIN_DOUBLE)) {
        return true;
    } else if (letObj->HasObjectFlag(ETSObjectFlags::BUILTIN_FLOAT)) {
        if (!enumObj->HasObjectFlag(ETSObjectFlags::BUILTIN_DOUBLE)) {
            return true;
        }
    } else if (letObj->HasObjectFlag(ETSObjectFlags::BUILTIN_LONG)) {
        if (!enumObj->HasObjectFlag(ETSObjectFlags::BUILTIN_DOUBLE) &&
            !enumObj->HasObjectFlag(ETSObjectFlags::BUILTIN_FLOAT)) {
            return true;
        }
    } else if (letObj->HasObjectFlag(ETSObjectFlags::BUILTIN_INT)) {
        if (!enumObj->HasObjectFlag(ETSObjectFlags::BUILTIN_DOUBLE) &&
            !enumObj->HasObjectFlag(ETSObjectFlags::BUILTIN_FLOAT) &&
            !enumObj->HasObjectFlag(ETSObjectFlags::BUILTIN_LONG)) {
            return true;
        }
    } else if (letObj->HasObjectFlag(ETSObjectFlags::BUILTIN_SHORT)) {
        if (enumObj->HasObjectFlag(ETSObjectFlags::BUILTIN_BYTE) ||
            enumObj->HasObjectFlag(ETSObjectFlags::BUILTIN_SHORT)) {
            return true;
        }
    } else if (letObj->HasObjectFlag(ETSObjectFlags::BUILTIN_BYTE)) {
        if (enumObj->HasObjectFlag(ETSObjectFlags::BUILTIN_BYTE)) {
            return true;
        }
    }

    return false;
}

bool ETSNumericEnumType::AssignmentSource(TypeRelation *relation, Type *target)
{
    bool result = false;
    if (target->IsETSObjectType()) {
        if (target->AsETSObjectType()->IsGlobalETSObjectType() ||
            target->AsETSObjectType()->Name() == compiler::Signatures::NUMERIC) {
            result = true;
        } else if (EnumAnnotedType() != nullptr) {
            if (CheckAssignableNumericTypes(target)) {
                result = true;
                relation->GetNode()->AddAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
            }
        } else if (target->IsBuiltinNumeric()) {
            result = true;
            relation->GetNode()->AddAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
        }
    } else if (target->HasTypeFlag(TypeFlag::ETS_NUMERIC)) {
        result = true;
        relation->GetNode()->AddAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
    } else if (target->IsETSUnionType()) {
        auto &unionConstituentTypes = target->AsETSUnionType()->ConstituentTypes();
        for (auto *constituentType : unionConstituentTypes) {
            if (relation->IsIdenticalTo(this, constituentType)) {
                result = true;
                break;
            }
        }
    }
    relation->Result(result);
    return relation->IsTrue();
}

void ETSNumericEnumType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    relation->IsIdenticalTo(this, source) ? relation->Result(true) : relation->Result(false);
}

void ETSNumericEnumType::Cast(TypeRelation *const relation, Type *const target)
{
    if (relation->IsIdenticalTo(this, target)) {
        relation->Result(true);
        return;
    }
    if (target->HasTypeFlag(TypeFlag::ETS_NUMERIC) || target->IsBuiltinNumeric()) {
        relation->RaiseError(diagnostic::ENUM_DEPRECATED_CAST, {this, target}, relation->GetNode()->Start());
        relation->Result(true);
        return;
    }
    conversion::Forbidden(relation);
}

void ETSNumericEnumType::CastTarget(TypeRelation *relation, Type *source)
{
    if (source->IsIntType() || source->IsBuiltinNumeric()) {
        relation->RaiseError(diagnostic::ENUM_DEPRECATED_CAST, {source, this}, relation->GetNode()->Start());
        relation->Result(true);
        return;
    }
    conversion::Forbidden(relation);
}

[[nodiscard]] bool ETSNumericEnumType::CheckBuiltInType(const ETSChecker *checker, ETSObjectFlags flag) const noexcept
{
    if (enumType_ != nullptr) {
        return enumType_->AsETSObjectType()->HasObjectFlag(flag);
    }

    return checker->GlobalIntBuiltinType();
}

}  // namespace ark::es2panda::checker
