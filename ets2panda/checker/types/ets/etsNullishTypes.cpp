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

#include "etsNullishTypes.h"
#include "etsTypeParameter.h"
#include "ir/expressions/identifier.h"
#include "ir/ts/tsTypeParameter.h"
#include "checker/ETSchecker.h"
#include "checker/ets/conversion.h"

namespace ark::es2panda::checker {

void ETSNullType::Identical(TypeRelation *relation, Type *other)
{
    relation->Result(other->IsETSNullType());
}

void ETSNullType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    Identical(relation, source);
}

bool ETSNullType::AssignmentSource(TypeRelation *relation, Type *target)
{
    return relation->IsSupertypeOf(target, this);
}

void ETSNullType::Compare([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *other)
{
    ES2PANDA_UNREACHABLE();
}

void ETSNullType::Cast(TypeRelation *relation, Type *target)
{
    Identical(relation, target);
}

void ETSNullType::CastTarget(TypeRelation *relation, Type *source)
{
    relation->IsSupertypeOf(source, this);
}

void ETSNullType::ToString(std::stringstream &ss, [[maybe_unused]] bool precise) const
{
    ss << "null";
}

void ETSNullType::ToAssemblerType(std::stringstream &ss) const
{
    ss << compiler::Signatures::NULL_ASSEMBLY_TYPE;
}

void ETSNullType::ToDebugInfoType(std::stringstream &ss) const
{
    ss << ETSObjectType::NameToDescriptor(compiler::Signatures::NULL_ASSEMBLY_TYPE);
}

Type *ETSNullType::Instantiate([[maybe_unused]] ArenaAllocator *allocator, [[maybe_unused]] TypeRelation *relation,
                               [[maybe_unused]] GlobalTypesHolder *globalTypes)
{
    return allocator->New<ETSNullType>();
}

void ETSUndefinedType::Identical(TypeRelation *relation, Type *other)
{
    relation->Result(other->IsETSUndefinedType());
}

void ETSUndefinedType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    Identical(relation, source);
}

bool ETSUndefinedType::AssignmentSource(TypeRelation *relation, Type *target)
{
    return relation->IsSupertypeOf(target, this);
}

void ETSUndefinedType::Compare([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *other)
{
    ES2PANDA_UNREACHABLE();
}

void ETSUndefinedType::Cast(TypeRelation *relation, Type *target)
{
    Identical(relation, target);
}

void ETSUndefinedType::CastTarget(TypeRelation *relation, Type *source)
{
    relation->IsSupertypeOf(source, this);
}

void ETSUndefinedType::ToString(std::stringstream &ss, [[maybe_unused]] bool precise) const
{
    ss << "undefined";
}

void ETSUndefinedType::ToAssemblerType(std::stringstream &ss) const
{
    ss << compiler::Signatures::BUILTIN_OBJECT;
}

void ETSUndefinedType::ToDebugInfoType(std::stringstream &ss) const
{
    ss << ETSObjectType::NameToDescriptor(compiler::Signatures::BUILTIN_OBJECT);
}

Type *ETSUndefinedType::Instantiate([[maybe_unused]] ArenaAllocator *allocator, [[maybe_unused]] TypeRelation *relation,
                                    [[maybe_unused]] GlobalTypesHolder *globalTypes)
{
    return allocator->New<ETSUndefinedType>();
}

}  // namespace ark::es2panda::checker
