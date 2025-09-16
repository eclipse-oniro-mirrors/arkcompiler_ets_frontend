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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_ARRAY_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_ARRAY_TYPE_H

#include "checker/types/type.h"

namespace ark::es2panda::checker {
class ETSArrayType : public Type {
public:
    explicit ETSArrayType(Type *elementType) : Type(TypeFlag::ETS_ARRAY), element_(elementType) {}

    Type *ElementType()
    {
        return element_;
    }

    const Type *ElementType() const
    {
        return element_;
    }

    void SetElementType(Type *element)
    {
        element_ = element;
    }

    void ToString(std::stringstream &ss, bool precise) const override;

    void ToAssemblerType(std::stringstream &ss) const override;
    void ToAssemblerTypeWithRank(std::stringstream &ss) const override;
    void ToDebugInfoType(std::stringstream &ss) const override;

    uint32_t Rank() const override;
    void Identical(TypeRelation *relation, Type *other) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    void Cast(TypeRelation *relation, Type *target) override;
    void IsSupertypeOf(TypeRelation *relation, Type *source) override;
    void IsSubtypeOf(TypeRelation *relation, Type *target) override;
    void CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag) override;
    Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) override;
    Type *Substitute(TypeRelation *relation, const Substitution *substitution) override;

private:
    Type *element_;
};
}  // namespace ark::es2panda::checker

#endif /* TYPESCRIPT_TYPES_ARRAY_TYPE_H */
