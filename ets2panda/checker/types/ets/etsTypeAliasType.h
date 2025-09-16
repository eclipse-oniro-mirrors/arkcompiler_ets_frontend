/**
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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_ETS_TYPE_ALIAS_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_ETS_TYPE_ALIAS_TYPE_H

#include "checker/types/ets/etsObjectType.h"
#include "checker/types/typeMapping.h"

namespace ark::es2panda::checker {
class ETSTypeAliasType : public Type {
public:
    using InstantiationMap = ArenaUnorderedMap<util::StringView, ETSTypeAliasType *>;

    explicit ETSTypeAliasType(ETSChecker *checker, util::StringView name, const ir::AstNode *declNode,
                              bool isRecursive = false);

    util::StringView TypeName()
    {
        return name_;
    }

    Type *GetTargetType()
    {
        return targetType_;
    }

    const ir::AstNode *GetDeclNode() const
    {
        return declNode_;
    }

    const Type *GetTargetType() const
    {
        ES2PANDA_ASSERT(targetType_ != nullptr);
        return targetType_;
    }

    void SetTargetType(Type *targetType)
    {
        targetType_ = targetType;
    }

    void SetRecursive(bool value = true)
    {
        isRecursive_ = value;
    }

    bool IsRecursive() const
    {
        return isRecursive_;
    }

    void ToString(std::stringstream &ss, bool precise) const override;

    void ToAssemblerType(std::stringstream &ss) const override;
    void ToAssemblerTypeWithRank(std::stringstream &ss) const override;
    void ToDebugInfoType(std::stringstream &ss) const override;

    void Identical(TypeRelation *relation, Type *other) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    void Cast(TypeRelation *relation, Type *target) override;
    void CastTarget(TypeRelation *relation, Type *source) override;
    void IsSupertypeOf(TypeRelation *relation, Type *source) override;
    void IsSubtypeOf(TypeRelation *relation, Type *target) override;
    Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) override;

    uint32_t Rank() const override;

    Type *Substitute(TypeRelation *relation, const Substitution *substitution) override;

    void SetTypeArguments(ArenaVector<Type *> typeArguments);

private:
    ETSTypeAliasType *GetInstantiatedType(util::StringView hash);
    void EmplaceInstantiatedType(util::StringView hash, ETSTypeAliasType *emplaceType);
    bool SubstituteTypeArgs(TypeRelation *const relation, ArenaVector<Type *> &newTypeArgs,
                            const Substitution *const substitution);

    bool IsArgumentsIdentical(TypeRelation *relation, Type *other);

    ETSTypeAliasType *GetBaseType()
    {
        return base_ == nullptr ? this : base_;
    }

    util::StringView name_;
    const ir::AstNode *declNode_;
    bool isRecursive_;
    ETSTypeAliasType *base_ = nullptr;
    ETSTypeAliasType *parent_ = nullptr;
    Type *targetType_ = nullptr;
    InstantiationMap instantiationMap_;
    ArenaVector<Type *> typeArguments_;
    const ArenaSubstitution *substitution_ = nullptr;
    mutable bool recursionCount_ = false;
};
}  // namespace ark::es2panda::checker

#endif /* ES2PANDA_COMPILER_CHECKER_TYPES_ETS_ETS_RECURSIVE_TYPE_H */
