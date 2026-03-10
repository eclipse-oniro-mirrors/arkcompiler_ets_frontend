/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "etsWildcardType.h"

#include "checker/ETSchecker.h"

namespace ark::es2panda::checker {
void ETSWildcardType::ToString(std::stringstream &ss, [[maybe_unused]] bool precise) const
{
    ss << "*";
}

void ETSWildcardType::Identical(TypeRelation *relation, [[maybe_unused]] Type *other)
{
    relation->Result(false);
}

bool ETSWildcardType::AssignmentSource(TypeRelation *relation, Type *target)
{
    relation->Result(false);
    return relation->IsSupertypeOf(target, GetUnderlying()->GetConstraintType());
}

void ETSWildcardType::AssignmentTarget(TypeRelation *relation, [[maybe_unused]] Type *source)
{
    relation->Result(false);
}

void ETSWildcardType::Cast(TypeRelation *relation, Type *target)
{
    relation->Result(false);
    relation->IsCastableTo(GetUnderlying()->GetConstraintType(), target);
}

Type *ETSWildcardType::Instantiate([[maybe_unused]] ArenaAllocator *allocator, [[maybe_unused]] TypeRelation *relation,
                                   [[maybe_unused]] GlobalTypesHolder *globalTypes)
{
    return this;
}
}  // namespace ark::es2panda::checker
