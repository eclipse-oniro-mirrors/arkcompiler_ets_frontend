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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_SIGNATURE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_SIGNATURE_H

#include "ir/ets/etsParameterExpression.h"
#include "type.h"

#include "varbinder/declaration.h"
#include "varbinder/variable.h"

namespace ark::es2panda::public_lib {
struct Context;
}  // namespace ark::es2panda::public_lib

namespace ark::es2panda::checker {

class SignatureInfo final {
public:
    explicit SignatureInfo(ArenaAllocator *allocator) : typeParams {allocator->Adapter()}, params {allocator->Adapter()}
    {
    }

    SignatureInfo(const SignatureInfo *other, ArenaAllocator *allocator)
        : typeParams(allocator->Adapter()), params(allocator->Adapter())
    {
        for (auto *it : other->typeParams) {
            typeParams.push_back(it);
        }
        for (auto *it : other->params) {
            params.push_back(it->Copy(allocator, it->Declaration()));
            params.back()->SetTsType(it->TsType());
        }

        minArgCount = other->minArgCount;

        if (other->restVar != nullptr) {
            restVar = other->restVar->Copy(allocator, other->restVar->Declaration());
            ES2PANDA_ASSERT(restVar != nullptr);
            restVar->SetTsType(other->restVar->TsType());
        }
    }

    SignatureInfo() = delete;
    ~SignatureInfo() = default;
    NO_COPY_SEMANTIC(SignatureInfo);
    NO_MOVE_SEMANTIC(SignatureInfo);

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    ArenaVector<Type *> typeParams;
    uint32_t minArgCount {};
    EPtr<varbinder::LocalVariable> restVar {};
    ArenaVector<varbinder::LocalVariable *> params;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

using ENUMBITOPS_OPERATORS;

enum class SignatureFlags : uint32_t {
    NO_OPTS = 0U,
    ABSTRACT = 1U << 0U,
    CALL = 1U << 1U,
    CONSTRUCT = 1U << 2U,
    PUBLIC = 1U << 3U,
    PROTECTED = 1U << 4U,
    PRIVATE = 1U << 5U,
    STATIC = 1U << 6U,
    FINAL = 1U << 7U,
    CONSTRUCTOR = 1U << 8U,
    PROXY = 1U << 9U,
    INTERNAL = 1U << 10U,
    NEED_RETURN_TYPE = 1U << 11U,
    INFERRED_RETURN_TYPE = 1U << 12U,
    THIS_RETURN_TYPE = 1U << 13U,
    GETTER = 1U << 14U,
    SETTER = 1U << 15U,
    THROWS = 1U << 16U,
    RETHROWS = 1U << 17U,
    EXTENSION_FUNCTION = 1U << 18U,
    DUPLICATE_ASM = 1U << 19U,
    BRIDGE = 1U << 20U,
    DEFAULT = 1U << 21U,

    INTERNAL_PROTECTED = INTERNAL | PROTECTED,
    GETTER_OR_SETTER = GETTER | SETTER,
    THROWING = THROWS | RETHROWS,
};

}  // namespace ark::es2panda::checker

template <>
struct enumbitops::IsAllowedType<ark::es2panda::checker::SignatureFlags> : std::true_type {
};

namespace ark::es2panda::checker {

class Signature final {
public:
    Signature(SignatureInfo *signatureInfo, Type *returnType, ir::ScriptFunction *func = nullptr)
        : signatureInfo_(signatureInfo), returnType_(returnType), func_(func)
    {
    }

    Signature() = delete;
    ~Signature() = default;
    NO_COPY_SEMANTIC(Signature);
    NO_MOVE_SEMANTIC(Signature);

    [[nodiscard]] const SignatureInfo *GetSignatureInfo() const noexcept
    {
        return signatureInfo_;
    }

    [[nodiscard]] SignatureInfo *GetSignatureInfo() noexcept
    {
        return signatureInfo_;
    }

    [[nodiscard]] const ArenaVector<Type *> &TypeParams() const noexcept
    {
        return signatureInfo_->typeParams;
    }

    [[nodiscard]] ArenaVector<Type *> &TypeParams() noexcept
    {
        return signatureInfo_->typeParams;
    }

    [[nodiscard]] const ArenaVector<varbinder::LocalVariable *> &Params() const noexcept
    {
        return signatureInfo_->params;
    }

    [[nodiscard]] ArenaVector<varbinder::LocalVariable *> &Params() noexcept
    {
        return signatureInfo_->params;
    }

    [[nodiscard]] const Type *ReturnType() const noexcept
    {
        return returnType_;
    }

    [[nodiscard]] Type *ReturnType() noexcept
    {
        return returnType_;
    }

    [[nodiscard]] uint32_t MinArgCount() const noexcept
    {
        return signatureInfo_->minArgCount;
    }

    [[nodiscard]] size_t ArgCount() const
    {
        return signatureInfo_->params.size();
    }

    void SetReturnType(Type *type) noexcept
    {
        returnType_ = type;
    }

    void SetOwner(ETSObjectType *owner) noexcept
    {
        ownerObj_ = owner;
    }

    void SetOwnerVar(varbinder::Variable *owner) noexcept
    {
        ownerVar_ = owner;
    }

    [[nodiscard]] bool HasFunction() const noexcept
    {
        return func_ != nullptr;
    }

    [[nodiscard]] ir::ScriptFunction *Function() noexcept
    {
        ES2PANDA_ASSERT(HasFunction());
        return func_;
    }

    [[nodiscard]] const ir::ScriptFunction *Function() const noexcept
    {
        ES2PANDA_ASSERT(HasFunction());
        return func_;
    }

    void SetInternalName(util::StringView internalName)
    {
        internalName_ = internalName;
    }

    [[nodiscard]] ETSObjectType *Owner()
    {
        return ownerObj_;
    }

    [[nodiscard]] const ETSObjectType *Owner() const noexcept
    {
        return ownerObj_;
    }

    [[nodiscard]] varbinder::Variable *OwnerVar() noexcept
    {
        return ownerVar_;
    }

    [[nodiscard]] const varbinder::LocalVariable *RestVar() const noexcept
    {
        return signatureInfo_->restVar;
    }

    [[nodiscard]] varbinder::LocalVariable *RestVar() noexcept
    {
        return signatureInfo_->restVar;
    }

    [[nodiscard]] uint8_t ProtectionFlag() const noexcept
    {
        if ((flags_ & SignatureFlags::PRIVATE) != 0) {
            return 2U;
        }

        if ((flags_ & SignatureFlags::PROTECTED) != 0) {
            return 1U;
        }

        return 0;
    }

    [[nodiscard]] bool HasProtectionFlagInternal() const noexcept
    {
        return (flags_ & SignatureFlags::INTERNAL) != 0;
    }

    [[nodiscard]] SignatureFlags Flags() const noexcept
    {
        return flags_;
    }

    void AddSignatureFlag(SignatureFlags const flag) noexcept
    {
        flags_ |= flag;
    }

    void RemoveSignatureFlag(SignatureFlags const flag) noexcept
    {
        flags_ &= ~flag;
    }

    [[nodiscard]] bool HasSignatureFlag(SignatureFlags const flag) const noexcept
    {
        return (flags_ & flag) != 0U;
    }

    [[nodiscard]] bool HasRestParameter() const noexcept
    {
        return signatureInfo_->restVar != nullptr;
    }

    void SetRestVar(varbinder::LocalVariable *restVar)
    {
        signatureInfo_->restVar = restVar;
    }

    [[nodiscard]] bool IsFinal() const noexcept
    {
        return HasSignatureFlag(SignatureFlags::FINAL);
    }

    [[nodiscard]] bool IsExtensionFunction() const noexcept
    {
        return HasSignatureFlag(SignatureFlags::EXTENSION_FUNCTION);
    }

    [[nodiscard]] bool IsExtensionAccessor() const noexcept
    {
        return IsExtensionFunction() && HasSignatureFlag(SignatureFlags::GETTER_OR_SETTER);
    }

    void ToAssemblerType(std::stringstream &ss) const;

    [[nodiscard]] util::StringView InternalName() const;

    [[nodiscard]] Signature *Copy(ArenaAllocator *allocator, TypeRelation *relation,
                                  GlobalTypesHolder *globalTypes) const;
    [[nodiscard]] Signature *Substitute(TypeRelation *relation, const Substitution *substitution);

    void ToString(std::stringstream &ss, const varbinder::Variable *variable, bool printAsMethod = false,
                  bool precise = false) const;
    std::string ToString() const;
    void IsSubtypeOf(TypeRelation *relation, Signature *super);
    void AssignmentTarget(TypeRelation *relation, Signature *source);
    [[nodiscard]] Signature *ToArrowSignature(ETSChecker *checker);
    friend class ETSExtensionFuncHelperType;

private:
    Signature *CreateSignatureForSubstitute(ArenaAllocator *allocator, SignatureInfo *sigInfo, Type *returnType);

    EPtr<checker::SignatureInfo> const signatureInfo_;
    EPtr<Type> returnType_;
    EPtr<ir::ScriptFunction> const func_ {};
    SignatureFlags flags_ {SignatureFlags::NO_OPTS};
    util::StringView internalName_ {};
    EPtr<ETSObjectType> ownerObj_ {};
    EPtr<varbinder::Variable> ownerVar_ {};
};
}  // namespace ark::es2panda::checker

#endif /* TYPESCRIPT_TYPES_SIGNATURE_H */
