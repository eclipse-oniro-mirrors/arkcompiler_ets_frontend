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

#include "etsAwaitedType.h"

#include "checker/ETSchecker.h"

namespace ark::es2panda::checker {
void ETSAwaitedType::ToString(std::stringstream &ss, bool precise) const
{
    ss << "Awaited<";
    GetUnderlying()->ToString(ss, precise);
    ss << ">";
}

void ETSAwaitedType::Identical(TypeRelation *relation, Type *other)
{
    if (other->IsETSAwaitedType()) {
        relation->IsIdenticalTo(GetUnderlying(), other->AsETSAwaitedType()->GetUnderlying());
    }
}

bool ETSAwaitedType::AssignmentSource(TypeRelation *relation, Type *target)
{
    return relation->IsSupertypeOf(target, this);
}

void ETSAwaitedType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    relation->IsSupertypeOf(this, source);
}

void ETSAwaitedType::Cast(TypeRelation *relation, Type *target)
{
    if (relation->IsSupertypeOf(target, this)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }
    relation->Result(relation->InCastingContext());
}

void ETSAwaitedType::CastTarget(TypeRelation *relation, Type *source)
{
    if (relation->IsSupertypeOf(this, source)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }
    relation->Result(relation->InCastingContext());
}

void ETSAwaitedType::IsSupertypeOf(TypeRelation *relation, [[maybe_unused]] Type *source)
{
    relation->Result(false);
}

void ETSAwaitedType::IsSubtypeOf(TypeRelation *relation, Type *target)
{
    relation->Result(false);
    if (target->IsETSAwaitedType()) {
        relation->IsSupertypeOf(target->AsETSAwaitedType()->GetUnderlying(), GetUnderlying());
    }
}
ETSAwaitedType *ETSAwaitedType::Instantiate([[maybe_unused]] ArenaAllocator *allocator,
                                            [[maybe_unused]] TypeRelation *relation,
                                            [[maybe_unused]] GlobalTypesHolder *globalTypes)
{
    return allocator->New<ETSAwaitedType>(
        GetUnderlying()->Instantiate(allocator, relation, globalTypes)->AsETSTypeParameter());
}

Type *ETSAwaitedType::Substitute([[maybe_unused]] TypeRelation *relation, const Substitution *substitution)
{
    auto *substituted = GetUnderlying()->Substitute(relation, substitution);
    auto *checker = relation->GetChecker()->AsETSChecker();
    if (substituted == GetUnderlying()) {
        return this;
    }

    return checker->HandleAwaitedUtilityType(substituted);
}

void ETSAwaitedType::ToAssemblerType(std::stringstream &ss) const
{
    GetUnderlying()->ToAssemblerTypeWithRank(ss);
}

void ETSAwaitedType::ToDebugInfoType(std::stringstream &ss) const
{
    GetUnderlying()->ToDebugInfoType(ss);
}

void ETSAwaitedType::CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag)
{
    relation->CheckVarianceRecursively(GetUnderlying(),
                                       relation->TransferVariant(varianceFlag, VarianceFlag::COVARIANT));
}

void ETSAwaitedType::Iterate(const TypeTraverser &func) const
{
    func(tparam_);
}

}  // namespace ark::es2panda::checker
