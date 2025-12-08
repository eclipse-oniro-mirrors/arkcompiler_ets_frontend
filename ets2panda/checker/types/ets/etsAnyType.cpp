/**
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

#include "etsAnyType.h"
#include <cstddef>

#include "checker/ETSchecker.h"
#include "checker/ets/conversion.h"
#include "etsTypeParameter.h"

namespace ark::es2panda::checker {
void ETSAnyType::Identical(TypeRelation *relation, Type *other)
{
    relation->Result(other->IsETSAnyType());
}

void ETSAnyType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    if (!source->IsETSPrimitiveType()) {
        relation->Result(true);
        return;
    }

    relation->Result(true);
}

bool ETSAnyType::AssignmentSource(TypeRelation *relation, Type *target)
{
    Identical(relation, target);
    return relation->IsTrue();
}

void ETSAnyType::Compare([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *other)
{
    ES2PANDA_UNREACHABLE();
}

void ETSAnyType::Cast(TypeRelation *relation, Type *target)
{
    if (!relation->InCastingContext()) {
        Identical(relation, target);
        return;
    }

    if (!target->IsETSPrimitiveType()) {
        relation->Result(true);
        return;
    }

    auto *const boxedTarget = relation->GetChecker()->AsETSChecker()->MaybeBoxInRelation(target);
    ES2PANDA_ASSERT(boxedTarget != nullptr);
    conversion::Unboxing(relation, boxedTarget->AsETSObjectType());
    relation->Result(true);
}

void ETSAnyType::CastTarget(TypeRelation *relation, [[maybe_unused]] Type *source)
{
    AssignmentTarget(relation, source);
}

void ETSAnyType::IsSubtypeOf(TypeRelation *relation, Type *target)
{
    Identical(relation, target);
}

void ETSAnyType::IsSupertypeOf(TypeRelation *relation, Type *source)
{
    relation->Result(!source->IsETSPrimitiveType());
}

void ETSAnyType::ToString(std::stringstream &ss, [[maybe_unused]] bool precise) const
{
    ss << (IsRelaxed() ? compiler::Signatures::ANY : compiler::Signatures::ANY_TYPE_NAME);
}

void ETSAnyType::ToAssemblerType(std::stringstream &ss) const
{
    ss << compiler::Signatures::BUILTIN_OBJECT;
}

TypeFacts ETSAnyType::GetTypeFacts() const
{
    return TypeFacts::NONE;
}

void ETSAnyType::ToDebugInfoType(std::stringstream &ss) const
{
    ss << ETSObjectType::NameToDescriptor(compiler::Signatures::BUILTIN_OBJECT);
}

Type *ETSAnyType::Instantiate(ArenaAllocator *allocator, [[maybe_unused]] TypeRelation *relation,
                              [[maybe_unused]] GlobalTypesHolder *globalTypes)
{
    return allocator->New<ETSAnyType>(isRelaxed_);
}
}  // namespace ark::es2panda::checker
