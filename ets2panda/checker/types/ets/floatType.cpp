/**
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

#include "floatType.h"

#include "checker/ets/conversion.h"
#include "checker/ets/wideningConverter.h"

namespace ark::es2panda::checker {
void FloatType::Identical(TypeRelation *relation, Type *other)
{
    if (other->IsFloatType()) {
        relation->Result(true);
    }
}

void FloatType::AssignmentTarget(TypeRelation *relation, [[maybe_unused]] Type *source)
{
    WideningConverter(relation->GetChecker()->AsETSChecker(), relation, this, source);
}

bool FloatType::AssignmentSource([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *target)
{
    if (relation->InAssignmentContext()) {
        relation->GetChecker()->AsETSChecker()->CheckUnboxedTypeWidenable(relation, target, this);
        if (!relation->IsTrue()) {
            return false;
        }
    }

    if (target->IsETSObjectType()) {
        relation->GetChecker()->AsETSChecker()->CheckBoxedSourceTypeAssignable(relation, this, target);
    }

    return relation->IsTrue();
}

void FloatType::Cast(TypeRelation *const relation, Type *const target)
{
    if (target->HasTypeFlag(TypeFlag::FLOAT)) {
        conversion::Identity(relation, this, target);
        return;
    }

    if (target->HasTypeFlag(TypeFlag::ETS_OBJECT) &&
        target->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::BUILTIN_FLOAT)) {
        conversion::Boxing(relation, this);
        return;
    }

    conversion::Forbidden(relation);
}

Type *FloatType::Instantiate([[maybe_unused]] ArenaAllocator *allocator, [[maybe_unused]] TypeRelation *relation,
                             [[maybe_unused]] GlobalTypesHolder *globalTypes)
{
    return this;
}
}  // namespace ark::es2panda::checker
