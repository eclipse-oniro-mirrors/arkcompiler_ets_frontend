/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_UNION_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_UNION_TYPE_H

#include "checker/types/type.h"
#include "checker/types/ets/etsObjectType.h"

namespace ark::es2panda::checker {
class GlobalTypesHolder;

class ETSUnionType : public Type {
public:
    // constituentTypes must be normalized
    explicit ETSUnionType(ETSChecker *checker, ArenaVector<Type *> &&constituentTypes, Type *normalizedType = nullptr);

    [[nodiscard]] const ArenaVector<Type *> &ConstituentTypes() const noexcept
    {
        return constituentTypes_;
    }

    void Iterate(const TypeTraverser &func) const override;
    void ToString(std::stringstream &ss, bool precise) const override;
    void ToAssemblerType(std::stringstream &ss) const override;
    void ToDebugInfoType(std::stringstream &ss) const override;
    void Identical(TypeRelation *relation, Type *other) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) override;
    Type *Substitute(TypeRelation *relation, const Substitution *substitution) override;
    void Cast(TypeRelation *relation, Type *target) override;
    void CastTarget(TypeRelation *relation, Type *source) override;
    void IsSupertypeOf(TypeRelation *relation, Type *source) override;
    void IsSubtypeOf(TypeRelation *relation, Type *target) override;
    void CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag) override;
    static void LinearizeAndEraseIdentical(TypeRelation *relation, ArenaVector<Type *> &types,
                                           bool needSubtypeReduction);

    [[nodiscard]] Type *FindUnboxableType() const noexcept;

    [[nodiscard]] bool IsOverlapWith(TypeRelation *relation, Type const *type) const noexcept;

    static void NormalizeTypes(TypeRelation *relation, ArenaVector<Type *> &types);

    const util::StringView &GetAssemblerType() const
    {
        return assemblerTypeCache_;
    }

    template <class UnaryPredicate>
    [[nodiscard]] bool AllOfConstituentTypes(UnaryPredicate p) const noexcept
    {
        return std::all_of(constituentTypes_.cbegin(), constituentTypes_.cend(), p);
    }

    template <class UnaryPredicate>
    [[nodiscard]] bool AnyOfConstituentTypes(UnaryPredicate p) const noexcept
    {
        return std::any_of(constituentTypes_.cbegin(), constituentTypes_.cend(), p);
    }

    template <class UnaryPredicate>
    [[nodiscard]] Type *FindSpecificType(UnaryPredicate p) const noexcept
    {
        auto const it = std::find_if(constituentTypes_.cbegin(), constituentTypes_.cend(), p);
        return it != constituentTypes_.cend() ? *it : nullptr;
    }

    template <class UnaryPredicate>
    [[nodiscard]] bool HasSpecificType(UnaryPredicate p) const noexcept
    {
        return FindSpecificType(p) != nullptr;
    }

    [[nodiscard]] checker::Type *GetAssignableType(ETSChecker *checker, checker::Type *sourceType,
                                                   std::optional<double> value) const;
    [[nodiscard]] std::pair<checker::Type *, checker::Type *> GetComplimentaryType(ETSChecker *checker,
                                                                                   checker::Type *sourceType);
    [[nodiscard]] Type *NormalizedType() const
    {
        return normalizedType_;
    }

    [[nodiscard]] bool IsNormalizedUnion() const
    {
        return normalizedType_ == this;
    }

private:
    static bool EachTypeRelatedToSomeType(TypeRelation *relation, ETSUnionType *source, ETSUnionType *target);
    static bool TypeRelatedToSomeType(TypeRelation *relation, Type *source, ETSUnionType *target);

    template <typename RelFN>
    void RelationTarget(TypeRelation *relation, Type *source, RelFN const &relFn);
    [[nodiscard]] static bool ExtractType(ETSChecker *checker, checker::Type *source,
                                          std::vector<Type *> &unionTypes) noexcept;

    [[nodiscard]] checker::Type *GetAssignableBuiltinType(
        checker::ETSChecker *checker, checker::ETSObjectType *sourceType,
        std::map<std::uint32_t, checker::ETSObjectType *> &numericTypes) const;

    void InitCanonicalAsmTypeCache(ETSChecker *checker);
    void CanonicalizedAssemblerType(ETSChecker *checker);
    void InitAssemblerTypeCache(ETSChecker *checker);

    const ArenaVector<Type *> &GetAssemblerTypes() const
    {
        return assemblerConstituentTypes_;
    }

    ArenaVector<Type *> const constituentTypes_;
    ArenaVector<Type *> assemblerConstituentTypes_;
    util::StringView assemblerTypeCache_;
    Type *const normalizedType_;
};
}  // namespace ark::es2panda::checker

#endif /* ETS_TYPES_ETS_UNION_TYPE_H */
