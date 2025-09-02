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
#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_GRADUAL_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_GRADUAL_TYPE_H

#include "checker/types/type.h"
#include "ir/astNode.h"

namespace ark::es2panda::checker {
class GradualType : public Type {
public:
    explicit GradualType(checker::Type *baseType)
        : Type(TypeFlag::GRADUAL_TYPE), baseType_(baseType), lang_(es2panda::Language(Language::Id::ETS))
    {
    }

    explicit GradualType(checker::Type *baseType, Language lang)
        : Type(TypeFlag::GRADUAL_TYPE), baseType_(baseType), lang_(lang)
    {
    }

    void Identical(TypeRelation *relation, Type *other) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    void Compare(TypeRelation *relation, Type *other) override;
    void Cast(TypeRelation *relation, Type *target) override;
    void CastTarget(TypeRelation *relation, Type *source) override;
    void IsSubtypeOf(TypeRelation *relation, Type *target) override;
    void IsSupertypeOf(TypeRelation *relation, Type *source) override;
    void ToString(std::stringstream &ss, bool precise) const override;
    void ToAssemblerType(std::stringstream &ss) const override;
    void ToDebugInfoType(std::stringstream &ss) const override;
    void ToAssemblerTypeWithRank(std::stringstream &ss) const override;
    Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) override;
    Type *Substitute(TypeRelation *relation, const Substitution *substitution) override;
    void CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag) override;

    const Type *GetBaseType() const
    {
        auto baseType = baseType_;
        while (baseType->IsGradualType()) {
            baseType = baseType->AsGradualType()->BaseType();
        }
        return baseType;
    }

    Type *GetBaseType()
    {
        auto baseType = baseType_;
        while (baseType->IsGradualType()) {
            baseType = baseType->AsGradualType()->BaseType();
        }
        return baseType;
    }

    Type *BaseType()
    {
        return baseType_;
    }

    Type *BaseType() const
    {
        return baseType_;
    }

    es2panda::Language Language() const
    {
        return lang_;
    }

private:
    Type *baseType_;
    es2panda::Language lang_;
};
}  // namespace ark::es2panda::checker

#endif