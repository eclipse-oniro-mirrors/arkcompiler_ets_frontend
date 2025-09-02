/*
 * Copyright (c) 2021 - 2025 Huawei Device Co., Ltd.
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

#include "etsVoidType.h"

namespace ark::es2panda::checker {
void ETSVoidType::Identical(TypeRelation *relation, Type *other)
{
    if (other->IsETSVoidType()) {
        relation->Result(true);
    }
}

void ETSVoidType::IsSupertypeOf(TypeRelation *const relation, Type *source)
{
    relation->Result(source->IsETSUndefinedType());
}

bool ETSVoidType::AssignmentSource(TypeRelation *relation, Type *target)
{
    // NOTE(vpukhov): #19701 void refactoring
    if (!target->IsETSUndefinedType()) {
        Identical(relation, target);
    } else {
        relation->Result(true);
    }

    return relation->IsTrue();
}

void ETSVoidType::AssignmentTarget([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *source)
{
    // NOTE(vpukhov): #19701 void refactoring
    if (!source->IsETSUndefinedType()) {
        Identical(relation, source);
    } else {
        relation->Result(true);
    }
}

Type *ETSVoidType::Instantiate([[maybe_unused]] ArenaAllocator *allocator, [[maybe_unused]] TypeRelation *relation,
                               [[maybe_unused]] GlobalTypesHolder *globalTypes)
{
    return this;
}
}  // namespace ark::es2panda::checker
