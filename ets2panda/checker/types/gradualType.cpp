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

#include "gradualType.h"

#include "checker/ETSchecker.h"
#include "checker/ets/conversion.h"

namespace ark::es2panda::checker {
void GradualType::Identical(TypeRelation *relation, Type *other)
{
    if (other->IsGradualType()) {
        baseType_->Identical(relation, other->AsGradualType()->GetBaseType());
    } else {
        baseType_->Identical(relation, other);
    }
}

void GradualType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    if (source->IsGradualType()) {
        baseType_->AssignmentTarget(relation, source->AsGradualType()->GetBaseType());
    } else {
        baseType_->AssignmentTarget(relation, source);
    }
}

bool GradualType::AssignmentSource(TypeRelation *relation, Type *target)
{
    if (target->IsGradualType()) {
        return baseType_->AssignmentSource(relation, target->AsGradualType()->GetBaseType());
    }
    return baseType_->AssignmentSource(relation, target);
}

void GradualType::Compare(TypeRelation *relation, Type *other)
{
    if (other->IsGradualType()) {
        baseType_->Compare(relation, other->AsGradualType()->GetBaseType());
    } else {
        baseType_->Compare(relation, other);
    }
}

void GradualType::Cast(TypeRelation *relation, Type *target)
{
    if (target->IsGradualType()) {
        baseType_->Cast(relation, target->AsGradualType()->GetBaseType());
    } else {
        baseType_->Cast(relation, target);
    }
}

void GradualType::CastTarget(TypeRelation *relation, Type *source)
{
    if (source->IsGradualType()) {
        baseType_->CastTarget(relation, source->AsGradualType()->GetBaseType());
    } else {
        baseType_->CastTarget(relation, source);
    }
}

void GradualType::IsSubtypeOf(TypeRelation *relation, Type *target)
{
    if (target->IsGradualType()) {
        baseType_->IsSubtypeOf(relation, target->AsGradualType()->GetBaseType());
    } else {
        baseType_->IsSubtypeOf(relation, target);
    }
}

void GradualType::IsSupertypeOf(TypeRelation *relation, Type *source)
{
    if (source->IsGradualType()) {
        relation->IsSupertypeOf(baseType_, source->AsGradualType()->GetBaseType());
    } else {
        baseType_->IsSupertypeOf(relation, source);
    }
}

void GradualType::ToString(std::stringstream &ss, [[maybe_unused]] bool precise) const
{
    baseType_->ToString(ss);
}

Type *GradualType::Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes)
{
    auto baseType = baseType_->Instantiate(allocator, relation, globalTypes);
    return relation->GetChecker()->AsETSChecker()->CreateGradualType(baseType);
}

Type *GradualType::Substitute(TypeRelation *relation, const Substitution *substitution)
{
    return baseType_->Substitute(relation, substitution);
}

void GradualType::ToAssemblerType(std::stringstream &ss) const
{
    baseType_->ToAssemblerType(ss);
}

void GradualType::ToDebugInfoType(std::stringstream &ss) const
{
    baseType_->ToDebugInfoType(ss);
}

void GradualType::ToAssemblerTypeWithRank(std::stringstream &ss) const
{
    baseType_->ToAssemblerTypeWithRank(ss);
}

void GradualType::CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag)
{
    // The type of array should be Invariant
    relation->CheckVarianceRecursively(baseType_, varianceFlag);
}
}  // namespace ark::es2panda::checker