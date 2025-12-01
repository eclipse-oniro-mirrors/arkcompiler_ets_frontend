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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_FUNCTION_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_FUNCTION_TYPE_H

#include "checker/types/type.h"
#include "checker/types/signature.h"
#include "ir/base/scriptFunction.h"

namespace ark::es2panda::checker {

class ETSFunctionType : public Type {
public:
    // create an "arrow" type
    explicit ETSFunctionType(ETSChecker *checker, Signature *signature);

    // create a "method" or "function" ETSFunctionType
    explicit ETSFunctionType(ETSChecker *checker, util::StringView name, ArenaVector<Signature *> &&signatures);

    [[nodiscard]] ArenaVector<Signature *> &CallSignatures()
    {
        ES2PANDA_ASSERT(!IsETSArrowType());
        return callSignatures_;
    }

    [[nodiscard]] const ArenaVector<Signature *> &CallSignatures() const
    {
        ES2PANDA_ASSERT(!IsETSArrowType());
        return callSignatures_;
    }

    // #22952: used temporarily as arrow and method types share the ETSFunctionType representation
    [[nodiscard]] ArenaVector<Signature *> &CallSignaturesOfMethodOrArrow()
    {
        return callSignatures_;
    }

    [[nodiscard]] const ArenaVector<Signature *> &CallSignaturesOfMethodOrArrow() const
    {
        return callSignatures_;
    }

    [[nodiscard]] util::StringView Name() const
    {
        ES2PANDA_ASSERT(!IsETSArrowType());
        return nameOrAsmName_;
    }

    void AddCallSignature(Signature *signature);

    template <class UnaryPredicate>
    Signature *FindSpecificSignature(UnaryPredicate predicate) const noexcept
    {
        ES2PANDA_ASSERT(!IsETSArrowType());
        auto const it = std::find_if(callSignatures_.cbegin(), callSignatures_.cend(), predicate);
        return it != callSignatures_.cend() ? *it : nullptr;
    }

    [[nodiscard]] Signature *FindSignature(const ir::ScriptFunction *func) const noexcept
    {
        return FindSpecificSignature([func](auto const *const sig) -> bool { return sig->Function() == func; });
    }

    [[nodiscard]] Signature *FindGetter() const noexcept
    {
        return FindSpecificSignature([](auto const *const sig) -> bool { return sig->Function()->IsGetter(); });
    }

    [[nodiscard]] Signature *FindSetter() const noexcept
    {
        return FindSpecificSignature([](auto const *const sig) -> bool { return sig->Function()->IsSetter(); });
    }

    [[nodiscard]] ArenaVector<Signature *> CollectExtensionAccessorSigs(ArenaAllocator *allocator)
    {
        ES2PANDA_ASSERT(!IsETSArrowType());
        ES2PANDA_ASSERT(IsExtensionAccessorType());
        ArenaVector<Signature *> accessorSigs {allocator->Adapter()};
        for (auto s : callSignatures_) {
            if (s->IsExtensionAccessor()) {
                accessorSigs.push_back(s);
            }
        }
        return accessorSigs;
    }

    [[nodiscard]] Signature *FirstAbstractSignature() const noexcept
    {
        return FindSpecificSignature(
            [](auto const *const sig) -> bool { return sig->HasSignatureFlag(SignatureFlags::ABSTRACT); });
    }

    Signature *ArrowSignature() const
    {
        ES2PANDA_ASSERT(IsETSArrowType());
        ES2PANDA_ASSERT(callSignatures_.size() == 1);
        auto sig = callSignatures_[0];
        ES2PANDA_ASSERT(!sig->HasFunction());
        return sig;
    }

    ETSFunctionType *MethodToArrow(ETSChecker *checker);
    ETSObjectType *ArrowToFunctionalInterface(ETSChecker *checker);
    ETSObjectType *ArrowToFunctionalInterfaceDesiredArity(ETSChecker *checker, size_t arity);
    [[nodiscard]] bool IsExtensionFunctionType() const
    {
        return hasExtensionSignatures_;
    }

    [[nodiscard]] bool IsExtensionAccessorType() const
    {
        return hasExtensionAccessorSignatures_;
    }

    void ToAssemblerType(std::stringstream &ss) const override;
    void ToDebugInfoType(std::stringstream &ss) const override;

    void ToString(std::stringstream &ss, bool precise) const override;
    void Identical(TypeRelation *relation, Type *other) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    void IsSupertypeOf(TypeRelation *relation, Type *source) override;
    void IsSubtypeOf(TypeRelation *relation, Type *target) override;
    Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) override;
    ETSFunctionType *Substitute(TypeRelation *relation, const Substitution *substitution) override;
    void CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag) override;
    void Cast(TypeRelation *relation, Type *target) override;
    void CastTarget(TypeRelation *relation, Type *source) override;

    void SetHelperSignature(Signature *signature) noexcept
    {
        helperSignature_ = signature;
    }

    const Signature *GetHelperSignature() const
    {
        return helperSignature_;
    }

    Signature *GetHelperSignature()
    {
        return helperSignature_;
    }

    bool HasHelperSignature()
    {
        return helperSignature_ != nullptr;
    }

private:
    ArenaVector<Signature *> callSignatures_;
    util::StringView const nameOrAsmName_;
    EPtr<Signature> helperSignature_ {};
    bool hasExtensionSignatures_ {};
    bool hasExtensionAccessorSignatures_ {};
};
}  // namespace ark::es2panda::checker

#endif /* TYPESCRIPT_TYPES_FUNCTION_TYPE_H */
