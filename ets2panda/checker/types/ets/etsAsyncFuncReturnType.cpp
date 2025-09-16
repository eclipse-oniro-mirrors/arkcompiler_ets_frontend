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

#include "checker/ETSchecker.h"
#include "checker/types/globalTypesHolder.h"
#include "checker/types/ets/etsAsyncFuncReturnType.h"

namespace ark::es2panda::checker {
void ETSAsyncFuncReturnType::ToString(std::stringstream &ss, bool precise) const
{
    promiseType_->ToString(ss, precise);
    ss << " | ";
    GetPromiseTypeArg()->ToString(ss, precise);
}

void ETSAsyncFuncReturnType::Identical(TypeRelation *relation, Type *other)
{
    if (other->IsETSAsyncFuncReturnType()) {
        auto *otherRetType = other->AsETSAsyncFuncReturnType();
        if (relation->IsIdenticalTo(promiseType_, otherRetType->promiseType_) &&
            relation->IsIdenticalTo(GetPromiseTypeArg(), otherRetType->GetPromiseTypeArg())) {
            relation->Result(true);
            return;
        }
    }

    relation->Result(false);
}

void ETSAsyncFuncReturnType::IsSupertypeOf(TypeRelation *relation, Type *source)
{
    GetPromiseTypeArg()->IsSupertypeOf(relation, source);
    if (relation->IsTrue()) {
        return;
    }
    promiseType_->IsSupertypeOf(relation, source);
}

bool ETSAsyncFuncReturnType::AssignmentSource([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *target)
{
    return false;
}

void ETSAsyncFuncReturnType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    relation->IsAssignableTo(source, promiseType_) || relation->IsAssignableTo(source, GetPromiseTypeArg());
}

void ETSAsyncFuncReturnType::CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag)
{
    relation->CheckVarianceRecursively(PromiseType(), relation->TransferVariant(varianceFlag, VarianceFlag::COVARIANT));
}

}  // namespace ark::es2panda::checker
