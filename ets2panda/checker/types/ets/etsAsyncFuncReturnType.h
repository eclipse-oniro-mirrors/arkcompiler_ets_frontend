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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_ASYNC_FUNC_RETURN_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_ASYNC_FUNC_RETURN_TYPE_H

#include "checker/types/ets/etsObjectType.h"

namespace ark::es2panda::checker {
class GlobalTypesHolder;

class ETSAsyncFuncReturnType : public ETSObjectType {
public:
    ETSAsyncFuncReturnType(ArenaAllocator *allocator, TypeRelation *relation, ETSObjectType *promiseType)
        : ETSObjectType(allocator, "", compiler::Signatures::BUILTIN_OBJECT,
                        std::make_tuple(nullptr, ETSObjectFlags::ASYNC_FUNC_RETURN_TYPE, relation)),
          promiseType_(promiseType)
    {
        ES2PANDA_ASSERT(promiseType->TypeArguments().size() == 1);
    }

    void Iterate(const TypeTraverser &func) const override;
    void ToString(std::stringstream &ss, bool precise) const override;
    void Identical(TypeRelation *relation, Type *other) override;
    void IsSupertypeOf(TypeRelation *relation, Type *source) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    void CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag) override;

    const Type *GetPromiseTypeArg() const
    {
        return promiseType_->TypeArguments()[0];
    }

    Type *GetPromiseTypeArg()
    {
        return promiseType_->TypeArguments()[0];
    }

    [[nodiscard]] const Type *PromiseType() const noexcept
    {
        return promiseType_;
    }

    [[nodiscard]] Type *PromiseType() noexcept
    {
        return promiseType_;
    }

private:
    ETSObjectType *promiseType_;
};
}  // namespace ark::es2panda::checker

#endif /* ES2PANDA_COMPILER_CHECKER_TYPES_ETS_ASYNC_FUNC_RETURN_TYPE_H \
# */
