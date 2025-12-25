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

#include "etsReadonlyType.h"

#include "checker/ETSchecker.h"

namespace ark::es2panda::checker {

void ETSReadonlyType::ToString(std::stringstream &ss, bool precise) const
{
    ss << "Readonly<";
    GetUnderlying()->ToString(ss, precise);
    ss << ">";
}

void ETSReadonlyType::Identical(TypeRelation *relation, Type *other)
{
    relation->Result(false);
    if (other->IsETSReadonlyType()) {
        relation->IsIdenticalTo(GetUnderlying(), other->AsETSReadonlyType()->GetUnderlying());
    }
}

bool ETSReadonlyType::AssignmentSource([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *target)
{
    return relation->Result(false);
}

void ETSReadonlyType::AssignmentTarget([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *source)
{
    if (source->IsETSTypeParameter() && source->AsETSTypeParameter()->GetOriginal() == GetUnderlying()) {
        relation->Result(true);
        return;
    }
    relation->Result(false);
}

void ETSReadonlyType::Cast(TypeRelation *relation, [[maybe_unused]] Type *target)
{
    if (relation->IsSupertypeOf(target, this)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }
    if (target->IsETSObjectType() && target->HasTypeFlag(TypeFlag::READONLY)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
    }
    relation->Result(relation->InCastingContext());
}

void ETSReadonlyType::CastTarget(TypeRelation *relation, [[maybe_unused]] Type *source)
{
    this->AssignmentTarget(relation, source);
    if (relation->IsTrue()) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }

    relation->Result(relation->InCastingContext());
}

void ETSReadonlyType::IsSupertypeOf([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *source)
{
    relation->Result(false);
}

void ETSReadonlyType::IsSubtypeOf([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *target)
{
    if (relation->IsSupertypeOf(target, GetUnderlying())) {
        return;
    }

    relation->Result(false);
}

Type *ETSReadonlyType::Substitute([[maybe_unused]] TypeRelation *relation, const Substitution *substitution)
{
    auto *substituted = GetUnderlying()->Substitute(relation, substitution);
    if (substituted == GetUnderlying()) {
        return this;
    }
    return relation->GetChecker()->AsETSChecker()->GetReadonlyType(substituted);
}

void ETSReadonlyType::ToAssemblerType(std::stringstream &ss) const
{
    GetUnderlying()->ToAssemblerTypeWithRank(ss);
}

void ETSReadonlyType::ToDebugInfoType(std::stringstream &ss) const
{
    GetUnderlying()->ToDebugInfoType(ss);
}

Type *ETSReadonlyType::Instantiate([[maybe_unused]] ArenaAllocator *allocator, [[maybe_unused]] TypeRelation *relation,
                                   [[maybe_unused]] GlobalTypesHolder *globalTypes)
{
    return allocator->New<ETSReadonlyType>(GetUnderlying());
}

void ETSReadonlyType::CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag)
{
    relation->CheckVarianceRecursively(GetUnderlying(),
                                       relation->TransferVariant(varianceFlag, VarianceFlag::COVARIANT));
}

void ETSReadonlyType::Iterate(const TypeTraverser &func) const
{
    func(tparam_);
}

}  // namespace ark::es2panda::checker
