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

#include "etsReturnTypeUtilityType.h"

#include "checker/ETSchecker.h"

namespace ark::es2panda::checker {
void ETSReturnTypeUtilityType::ToString(std::stringstream &ss, bool precise) const
{
    ss << "ReturnType<";
    GetUnderlying()->ToString(ss, precise);
    ss << ">";
}

void ETSReturnTypeUtilityType::Identical(TypeRelation *relation, Type *other)
{
    if (other->IsETSReturnTypeUtilityType()) {
        relation->IsIdenticalTo(GetUnderlying(), other->AsETSReturnTypeUtilityType()->GetUnderlying());
    }
}

bool ETSReturnTypeUtilityType::AssignmentSource(TypeRelation *relation, Type *target)
{
    return relation->IsSupertypeOf(target, this);
}

void ETSReturnTypeUtilityType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    relation->IsSupertypeOf(this, source);
}

void ETSReturnTypeUtilityType::Cast(TypeRelation *relation, Type *target)
{
    if (relation->IsSupertypeOf(target, this)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }
    relation->Result(relation->InCastingContext());
}

void ETSReturnTypeUtilityType::CastTarget(TypeRelation *relation, Type *source)
{
    if (relation->IsSupertypeOf(this, source)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }
    relation->Result(relation->InCastingContext());
}

void ETSReturnTypeUtilityType::IsSupertypeOf(TypeRelation *relation, [[maybe_unused]] Type *source)
{
    relation->Result(false);
}

void ETSReturnTypeUtilityType::IsSubtypeOf(TypeRelation *relation, Type *target)
{
    relation->Result(false);
    if (target->IsETSReturnTypeUtilityType()) {
        relation->IsSupertypeOf(target->AsETSReturnTypeUtilityType()->GetUnderlying(), GetUnderlying());
    }
}
ETSReturnTypeUtilityType *ETSReturnTypeUtilityType::Instantiate([[maybe_unused]] ArenaAllocator *allocator,
                                                                [[maybe_unused]] TypeRelation *relation,
                                                                [[maybe_unused]] GlobalTypesHolder *globalTypes)
{
    return allocator->New<ETSReturnTypeUtilityType>(
        GetUnderlying()->Instantiate(allocator, relation, globalTypes)->AsETSTypeParameter());
}

Type *ETSReturnTypeUtilityType::Substitute([[maybe_unused]] TypeRelation *relation, const Substitution *substitution)
{
    auto *substituted = GetUnderlying()->Substitute(relation, substitution);
    auto *checker = relation->GetChecker()->AsETSChecker();
    if (substituted == GetUnderlying()) {
        return this;
    }

    return checker->HandleReturnTypeUtilityType(substituted);
}

void ETSReturnTypeUtilityType::ToAssemblerType(std::stringstream &ss) const
{
    // We should only proceed here, if the underlying type is created from a valid type parameter. In that case we don't
    // know the return type, so emit Object, to be able to handle all types.
    ES2PANDA_ASSERT(GetUnderlying() != nullptr);
    ss << compiler::Signatures::BUILTIN_OBJECT;
}

void ETSReturnTypeUtilityType::CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag)
{
    relation->CheckVarianceRecursively(GetUnderlying(),
                                       relation->TransferVariant(varianceFlag, VarianceFlag::COVARIANT));
}

void ETSReturnTypeUtilityType::Iterate(const TypeTraverser &func) const
{
    func(tparam_);
}

}  // namespace ark::es2panda::checker
