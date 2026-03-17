/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_TUPLE_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_TUPLE_TYPE_H

#include "checker/ETSchecker.h"
#include "checker/types/type.h"

namespace ark::es2panda::checker {

class ETSTupleType final : public Type {
    using TupleSizeType = std::size_t;

public:
    ETSTupleType() = delete;
    ~ETSTupleType() override = default;

    NO_COPY_SEMANTIC(ETSTupleType);
    NO_MOVE_SEMANTIC(ETSTupleType);

    explicit ETSTupleType(ETSChecker *checker, ArenaVector<Type *> &&typeList);

    [[nodiscard]] TupleSizeType GetTupleSize() const noexcept
    {
        return typeList_.size();
    }

    [[nodiscard]] ArenaVector<Type *> const &GetTupleTypesList() const noexcept
    {
        return typeList_;
    }

    [[nodiscard]] ETSObjectType *GetWrapperType() const noexcept
    {
        return wrapperType_;
    }

    [[nodiscard]] Type *GetTypeAtIndex(TupleSizeType index) const;

    void Iterate(const TypeTraverser &func) const override;
    void ToString(std::stringstream &ss, bool precise) const override;

    void Identical(TypeRelation *relation, Type *other) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    Type *Substitute(TypeRelation *relation, const Substitution *substitution) override;

    void IsSupertypeOf(TypeRelation *relation, Type *source) override;
    void IsSubtypeOf(TypeRelation *relation, Type *target) override;
    void Cast(TypeRelation *relation, Type *target) override;
    Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) override;
    void CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag) override;

    void ToAssemblerType(std::stringstream &ss) const override;
    void ToDebugInfoType(std::stringstream &ss) const override;

private:
    bool CheckElementsIdentical(TypeRelation *relation, const ETSTupleType *other) const;

    const ArenaVector<Type *> typeList_;
    ETSObjectType *wrapperType_;
};

}  // namespace ark::es2panda::checker

#endif /* ES2PANDA_COMPILER_CHECKER_TYPES_ETS_TUPLE_TYPE_H */
