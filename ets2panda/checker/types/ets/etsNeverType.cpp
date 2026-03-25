/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "etsNeverType.h"

#include "checker/ETSchecker.h"
#include "checker/ets/conversion.h"
#include "etsTypeParameter.h"

namespace ark::es2panda::checker {
void ETSNeverType::Identical(TypeRelation *relation, Type *other)
{
    relation->Result(other->IsETSNeverType());
}

void ETSNeverType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    Identical(relation, source);
}

bool ETSNeverType::AssignmentSource(TypeRelation *relation, [[maybe_unused]] Type *target)
{
    relation->Result(true);
    return true;
}

void ETSNeverType::Compare([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *other)
{
    ES2PANDA_UNREACHABLE();
}

void ETSNeverType::Cast(TypeRelation *relation, Type *target)
{
    Identical(relation, target);
}

void ETSNeverType::CastTarget(TypeRelation *relation, Type *source)
{
    relation->IsSupertypeOf(source, this);
}

void ETSNeverType::IsSubtypeOf(TypeRelation *relation, [[maybe_unused]] Type *target)
{
    relation->Result(true);
}

void ETSNeverType::IsSupertypeOf(TypeRelation *relation, Type *source)
{
    Identical(relation, source);
}

void ETSNeverType::ToString(std::stringstream &ss, [[maybe_unused]] bool precise) const
{
    ss << "never";
}

void ETSNeverType::ToAssemblerType(std::stringstream &ss) const
{
    ss << compiler::Signatures::NEVER_ASSEMBLY_TYPE;
}

TypeFacts ETSNeverType::GetTypeFacts() const
{
    return TypeFacts::NONE;
}

void ETSNeverType::ToDebugInfoType(std::stringstream &ss) const
{
    ss << ETSObjectType::NameToDescriptor(compiler::Signatures::TYPE_DESCRIPTOR_NEVER);
}

Type *ETSNeverType::Instantiate([[maybe_unused]] ArenaAllocator *allocator, [[maybe_unused]] TypeRelation *relation,
                                [[maybe_unused]] GlobalTypesHolder *globalTypes)
{
    return allocator->New<ETSNeverType>();
}
}  // namespace ark::es2panda::checker