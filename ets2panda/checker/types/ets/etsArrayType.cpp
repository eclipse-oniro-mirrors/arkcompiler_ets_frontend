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

#include "etsArrayType.h"
#include <cstddef>

#include "varbinder/variable.h"
#include "checker/ETSchecker.h"
#include "checker/ets/conversion.h"
#include "checker/types/typeRelation.h"
#include "checker/types/ets/etsTupleType.h"

namespace ark::es2panda::checker {
void ETSArrayType::ToString(std::stringstream &ss, bool precise) const
{
    if (HasTypeFlag(TypeFlag::READONLY)) {
        ss << "readonly ";
    }
    ss << "FixedArray<";
    bool needParens = (element_->IsETSUnionType() || element_->IsETSFunctionType());
    if (needParens) {
        ss << "(";
    }
    element_->ToString(ss, precise);
    if (needParens) {
        ss << ")";
    }
    ss << ">";
}

void ETSArrayType::ToAssemblerType(std::stringstream &ss) const
{
    element_->ToAssemblerType(ss);
}

void ETSArrayType::ToAssemblerTypeWithRank(std::stringstream &ss) const
{
    element_->ToAssemblerType(ss);

    for (uint32_t i = Rank(); i > 0; --i) {
        ss << "[]";
    }
}

void ETSArrayType::ToDebugInfoType(std::stringstream &ss) const
{
    ss << "[";
    element_->ToDebugInfoType(ss);
}

uint32_t ETSArrayType::Rank() const
{
    uint32_t rank = 1;
    const auto *iter = element_;
    while (iter->IsETSArrayType() && iter->AsETSArrayType()->ElementType() != iter) {
        iter = iter->AsETSArrayType()->ElementType();
        rank++;
    }

    return rank;
}

void ETSArrayType::Identical(TypeRelation *relation, Type *other)
{
    if (other->IsETSArrayType()) {
        // will be removed, if wildcard type is assigned to array type, not element type
        if (element_->IsWildcardType() || other->AsETSArrayType()->ElementType()->IsWildcardType()) {
            relation->Result(true);
            return;
        }
        relation->IsIdenticalTo(element_, other->AsETSArrayType()->ElementType());
    }
}

void ETSArrayType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    if (source->HasTypeFlag(TypeFlag::READONLY)) {
        relation->Result(false);
        return;
    }
    if (source->IsETSArrayType()) {
        if (ElementType()->IsETSPrimitiveOrEnumType() ||
            source->AsETSArrayType()->ElementType()->IsETSPrimitiveOrEnumType()) {
            return;
        }
        relation->IsSupertypeOf(element_, source->AsETSArrayType()->ElementType());
    }
}

void ETSArrayType::Cast(TypeRelation *const relation, Type *const target)
{
    if (relation->IsSupertypeOf(this, target)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }

    if (target->HasTypeFlag(TypeFlag::ETS_ARRAY)) {
        conversion::NarrowingReference(relation, this, target->AsETSArrayType());
        if (relation->IsTrue()) {
            return;
        }

        if (ElementType()->IsETSTypeParameter()) {
            // unchecked cast!
            relation->Result(true);
            return;
        }
        relation->Result(relation->InCastingContext());
        return;
    }

    relation->Result(relation->InCastingContext());
}

void ETSArrayType::IsSupertypeOf(TypeRelation *const relation, Type *source)
{
    if (source->IsETSArrayType()) {
        relation->IsSupertypeOf(this->AsETSArrayType()->ElementType(), source->AsETSArrayType()->ElementType());
    }
}

void ETSArrayType::IsSubtypeOf(TypeRelation *const relation, Type *target)
{
    if (target->IsETSObjectType() && target->AsETSObjectType()->IsGlobalETSObjectType()) {
        relation->Result(true);
        return;
    }
}

Type *ETSArrayType::Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes)
{
    auto *elementType = element_->Instantiate(allocator, relation, globalTypes);

    // Some TypeFlag such as READONLY may pollute the ETSArrayType in the cache
    ETSArrayType *arrayType =
        relation->GetChecker()->AsETSChecker()->CreateETSArrayType(elementType, HasTypeFlag(TypeFlag::READONLY));
    arrayType->typeFlags_ = typeFlags_;
    return arrayType;
}

Type *ETSArrayType::Substitute(TypeRelation *relation, const Substitution *substitution)
{
    if (substitution == nullptr || substitution->empty()) {
        return this;
    }

    auto *resultElt = element_->Substitute(relation, substitution);

    if (resultElt == element_) {
        return this;
    }

    // Some TypeFlag such as READONLY may pollute the ETSArrayType in the cache
    ETSArrayType *result =
        relation->GetChecker()->AsETSChecker()->CreateETSArrayType(resultElt, HasTypeFlag(TypeFlag::READONLY));
    result->typeFlags_ = typeFlags_;
    return result;
}

void ETSArrayType::CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag)
{
    // The type of array should be Invariant
    relation->CheckVarianceRecursively(element_, relation->TransferVariant(varianceFlag, VarianceFlag::INVARIANT));
}

}  // namespace ark::es2panda::checker
