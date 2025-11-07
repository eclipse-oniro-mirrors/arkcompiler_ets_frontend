/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_RETURN_TYPE_UTILITY_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_RETURN_TYPE_UTILITY_TYPE_H

#include "checker/types/type.h"

namespace ark::es2panda::checker {
class ETSReturnTypeUtilityType : public Type {
public:
    ETSReturnTypeUtilityType() = delete;
    ~ETSReturnTypeUtilityType() override = default;

    NO_COPY_SEMANTIC(ETSReturnTypeUtilityType);
    NO_MOVE_SEMANTIC(ETSReturnTypeUtilityType);

    explicit ETSReturnTypeUtilityType(ETSTypeParameter *tparam)
        : Type(TypeFlag::ETS_RETURN_TYPE_UTILITY), tparam_(tparam)
    {
    }

    ETSTypeParameter *GetUnderlying() const
    {
        return tparam_;
    }

    Type *Substitute(TypeRelation *relation, const Substitution *substitution) override;

    ETSReturnTypeUtilityType *Instantiate(ArenaAllocator *allocator, TypeRelation *relation,
                                          GlobalTypesHolder *globalTypes) override;

    void Identical(TypeRelation *relation, Type *other) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    void Cast(TypeRelation *relation, Type *target) override;
    void CastTarget(TypeRelation *relation, Type *source) override;
    void IsSupertypeOf(TypeRelation *relation, Type *source) override;
    void IsSubtypeOf(TypeRelation *relation, Type *target) override;

    void CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag) override;

    void Iterate(const TypeTraverser &func) const override;
    void ToString(std::stringstream &ss, bool precise) const override;
    void ToAssemblerType(std::stringstream &ss) const override;

private:
    ETSTypeParameter *const tparam_;
};

}  // namespace ark::es2panda::checker

#endif
