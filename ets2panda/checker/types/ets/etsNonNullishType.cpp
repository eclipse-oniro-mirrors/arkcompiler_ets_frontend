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

#include "etsNonNullishType.h"

#include "checker/ETSchecker.h"

namespace ark::es2panda::checker {

void ETSNonNullishType::ToString(std::stringstream &ss, bool precise) const
{
    ss << "NonNullable<";
    GetUnderlying()->ToString(ss, precise);
    ss << ">";
}

void ETSNonNullishType::Identical(TypeRelation *relation, Type *other)
{
    if (other->IsETSNonNullishType()) {
        relation->IsIdenticalTo(GetUnderlying(), other->AsETSNonNullishType()->GetUnderlying());
    }
}

bool ETSNonNullishType::AssignmentSource([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *target)
{
    return relation->IsSupertypeOf(target, this);
}

void ETSNonNullishType::AssignmentTarget([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *source)
{
    relation->IsSupertypeOf(this, source);
}

void ETSNonNullishType::Cast(TypeRelation *relation, Type *target)
{
    if (relation->IsSupertypeOf(target, this)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }
    if (relation->IsIdenticalTo(GetUnderlying(), target)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }

    if (target->IsETSReferenceType()) {
        relation->Result(relation->InCastingContext());
    }
}

void ETSNonNullishType::CastTarget(TypeRelation *relation, Type *source)
{
    if (relation->IsSupertypeOf(this, source)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }
    if (relation->IsIdenticalTo(source, GetUnderlying())) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
    }
    relation->Result(relation->InCastingContext());
}

void ETSNonNullishType::IsSupertypeOf([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *source)
{
    relation->Result(false);
}

void ETSNonNullishType::IsSubtypeOf([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *target)
{
    if (relation->IsSupertypeOf(target, GetUnderlying())) {
        return;
    }

    relation->Result(false);
}

Type *ETSNonNullishType::Substitute([[maybe_unused]] TypeRelation *relation, const Substitution *substitution)
{
    auto *substituted = GetUnderlying()->Substitute(relation, substitution);
    if (substituted == GetUnderlying()) {
        return this;
    }
    return relation->GetChecker()->AsETSChecker()->GetNonNullishType(substituted);
}

void ETSNonNullishType::ToAssemblerType(std::stringstream &ss) const
{
    GetUnderlying()->ToAssemblerTypeWithRank(ss);
}

void ETSNonNullishType::ToDebugInfoType(std::stringstream &ss) const
{
    GetUnderlying()->ToDebugInfoType(ss);
}

Type *ETSNonNullishType::Instantiate([[maybe_unused]] ArenaAllocator *allocator,
                                     [[maybe_unused]] TypeRelation *relation,
                                     [[maybe_unused]] GlobalTypesHolder *globalTypes)
{
    return allocator->New<ETSNonNullishType>(GetUnderlying());
}

void ETSNonNullishType::CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag)
{
    relation->CheckVarianceRecursively(GetUnderlying(),
                                       relation->TransferVariant(varianceFlag, VarianceFlag::COVARIANT));
}

void ETSNonNullishType::Iterate(const TypeTraverser &func) const
{
    func(tparam_);
}

}  // namespace ark::es2panda::checker
