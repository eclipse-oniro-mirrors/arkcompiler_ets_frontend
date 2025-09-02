/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

class ETSTupleType : public Type {
    using TupleSizeType = std::size_t;

public:
    explicit ETSTupleType(ETSChecker *checker, const ArenaVector<Type *> &typeList)
        : Type(checker::TypeFlag::ETS_TUPLE),
          typeList_(typeList),
          // NOLINTNEXTLINE(readability-implicit-bool-conversion)
          wrapperType_(checker->GlobalBuiltinTupleType(typeList_.size()) != nullptr
                           ? checker->GlobalBuiltinTupleType(typeList_.size())->AsETSObjectType()
                           : nullptr)
    {
        typeFlags_ |= TypeFlag::ETS_TUPLE;
    }

    [[nodiscard]] TupleSizeType GetTupleSize() const
    {
        return typeList_.size();
    }

    [[nodiscard]] ArenaVector<Type *> const &GetTupleTypesList() const
    {
        return typeList_;
    }

    [[nodiscard]] ETSObjectType *GetWrapperType() const
    {
        return wrapperType_;
    }

    [[nodiscard]] Type *GetTypeAtIndex(TupleSizeType index) const;

    void ToString(std::stringstream &ss, bool precise) const override;

    void Identical(TypeRelation *relation, Type *other) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    Type *Substitute(TypeRelation *relation, const Substitution *substitution) override;
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
