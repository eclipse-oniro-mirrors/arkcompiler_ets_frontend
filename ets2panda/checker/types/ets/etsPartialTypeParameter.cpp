/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "etsPartialTypeParameter.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::checker {

ETSPartialTypeParameter::ETSPartialTypeParameter(ETSTypeParameter *const typeParam, ETSChecker *const checker)
    : Type(TypeFlag::ETS_PARTIAL_TYPE_PARAMETER), checker_(checker), typeParameter_(typeParam)
{
    ES2PANDA_ASSERT(typeParameter_ != nullptr && checker_ != nullptr);
}

void ETSPartialTypeParameter::ToString(std::stringstream &ss, bool precise) const
{
    ss << "Partial<";
    GetUnderlying()->ToString(ss, precise);
    ss << ">";
}

void ETSPartialTypeParameter::Identical(TypeRelation *relation, Type *other)
{
    if (other->IsETSPartialTypeParameter()) {
        relation->IsIdenticalTo(GetUnderlying(), other->AsETSPartialTypeParameter()->GetUnderlying());
    }
}

bool ETSPartialTypeParameter::AssignmentSource(TypeRelation *relation, Type *target)
{
    return relation->IsSupertypeOf(target, this);
}

void ETSPartialTypeParameter::AssignmentTarget(TypeRelation *relation, Type *source)
{
    relation->IsSupertypeOf(this, source);
}

void ETSPartialTypeParameter::Cast(TypeRelation *relation, Type *target)
{
    if (relation->IsSupertypeOf(target, this)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }
    relation->Result(relation->InCastingContext());
}

void ETSPartialTypeParameter::CastTarget(TypeRelation *relation, Type *source)
{
    if (relation->IsSupertypeOf(this, source)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }
    relation->Result(relation->InCastingContext());
}

void ETSPartialTypeParameter::IsSupertypeOf(TypeRelation *relation, [[maybe_unused]] Type *source)
{
    relation->Result(false);
}

void ETSPartialTypeParameter::IsSubtypeOf(TypeRelation *relation, Type *target)
{
    relation->Result(false);
    if (target->IsETSPartialTypeParameter()) {
        relation->IsSupertypeOf(target->AsETSPartialTypeParameter()->GetUnderlying(), GetUnderlying());
    } else if (target->IsETSObjectType() && target->AsETSObjectType()->IsGlobalETSObjectType()) {
        relation->IsSupertypeOf(target, GetUnderlying());
    }
}

ETSPartialTypeParameter *ETSPartialTypeParameter::Instantiate(ArenaAllocator *allocator, TypeRelation *relation,
                                                              GlobalTypesHolder *globalTypes)
{
    auto *underlying = GetUnderlying();
    ES2PANDA_ASSERT(underlying != nullptr);
    auto *instantiated = underlying->Instantiate(allocator, relation, globalTypes);
    ES2PANDA_ASSERT(instantiated != nullptr);
    auto *typeParam = instantiated->AsETSTypeParameter();
    ES2PANDA_ASSERT(typeParam != nullptr);
    return allocator->New<ETSPartialTypeParameter>(typeParam, checker_);
}

Type *ETSPartialTypeParameter::Substitute(TypeRelation *relation, const Substitution *substitution)
{
    auto *substituted = GetUnderlying()->Substitute(relation, substitution);
    if (substituted == GetUnderlying()) {
        return this;
    }
    return checker_->CreatePartialType(substituted);
}

void ETSPartialTypeParameter::ToAssemblerType(std::stringstream &ss) const
{
    checker_->CreatePartialType(GetUnderlying()->GetConstraintType())->ToAssemblerTypeWithRank(ss);
}

void ETSPartialTypeParameter::ToDebugInfoType(std::stringstream &ss) const
{
    checker_->CreatePartialType(GetUnderlying()->GetConstraintType())->ToDebugInfoType(ss);
}

void ETSPartialTypeParameter::CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag)
{
    relation->CheckVarianceRecursively(GetUnderlying(),
                                       relation->TransferVariant(varianceFlag, VarianceFlag::COVARIANT));
}

}  // namespace ark::es2panda::checker
