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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_TYPE_H

#include <mutex>
#include "generated/signatures.h"
#include "checker/types/typeMapping.h"
#include "checker/types/typeRelation.h"
#include "checker/types/typeFacts.h"

namespace ark::es2panda::varbinder {
class Variable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::checker {
class ObjectDescriptor;
class GlobalTypesHolder;
class ETSAsyncFuncReturnType;
class ETSChecker;
class ETSTypeParameter;
class ETSEnumType;
class GradualType;

// CC-OFFNXT(G.PRE.02) name part
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_TYPENAMES(typeFlag, typeName) class typeName;  // CC-OFF(G.PRE.09) code gen
TYPE_MAPPING(DECLARE_TYPENAMES)
#undef DECLARE_TYPENAMES
class ETSStringType;
class ETSBigIntType;
class ETSResizableArrayType;

using Substitution = std::map<ETSTypeParameter *, Type *>;
using ArenaSubstitution = ArenaMap<ETSTypeParameter *, Type *>;

class Type {
public:
    explicit Type(TypeFlag flag) : typeFlags_(flag)
    {
        std::lock_guard<std::mutex> lock(idLock_);
        static uint32_t typeId = 0;
        ES2PANDA_ASSERT(typeId < std::numeric_limits<uint32_t>::max());
        id_ = ++typeId;
    }

    NO_COPY_SEMANTIC(Type);
    NO_MOVE_SEMANTIC(Type);

    virtual ~Type() = default;
    static std::mutex idLock_;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPE_IS_CHECKS(typeFlag, typeName)                                                  \
    bool Is##typeName() const noexcept                                                      \
    {                                                                                       \
        /* CC-OFFNXT(G.PRE.05) The macro is used to generate a function. Return is needed*/ \
        return HasTypeFlag(typeFlag);                                                       \
    }
    TYPE_MAPPING(TYPE_IS_CHECKS)
#undef DECLARE_IS_CHECKS

/* CC-OFFNXT(G.PRE.06,G.PRE.02) solid logic, name part */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPE_AS_CASTS(typeFlag, typeName)                                                   \
    /* CC-OFFNXT(G.PRE.02) name part*/                                                      \
    typeName *As##typeName()                                                                \
    {                                                                                       \
        if (IsTypeError()) {                                                                \
            LOG(INFO, ES2PANDA) << "Erroneous cast to '" << #typeName << "' type.";         \
            throw std::exception();                                                         \
        }                                                                                   \
        ES2PANDA_ASSERT(Is##typeName());                                                    \
        /* CC-OFFNXT(G.PRE.05) The macro is used to generate a function. Return is needed*/ \
        return reinterpret_cast<typeName *>(this); /* CC-OFF(G.PRE.02) name part*/          \
    }                                                                                       \
    const typeName *As##typeName() const                                                    \
    {                                                                                       \
        if (IsTypeError()) {                                                                \
            LOG(INFO, ES2PANDA) << "Erroneous cast to '" << #typeName << "' type.";         \
            throw std::exception();                                                         \
        }                                                                                   \
        ES2PANDA_ASSERT(Is##typeName());                                                    \
        /* CC-OFFNXT(G.PRE.05) The macro is used to generate a function. Return is needed*/ \
        return reinterpret_cast<const typeName *>(this);                                    \
    }
    TYPE_MAPPING(TYPE_AS_CASTS)
#undef TYPE_AS_CASTS

    bool IsETSResizableArrayType() const;
    bool IsETSReadonlyArrayType() const;
    bool IsETSStringType() const;
    bool IsETSCharType() const;
    bool IsETSBigIntType() const;
    bool IsETSArrowType() const;
    bool IsETSMethodType() const;
    bool IsETSRelaxedAnyType() const;
    bool IsETSPrimitiveType() const;
    bool IsETSReferenceType() const;
    bool IsETSAsyncFuncReturnType() const;
    bool IsETSUnboxableObject() const;
    bool IsETSPrimitiveOrEnumType() const;

    bool PossiblyETSNull() const;
    bool PossiblyETSUndefined() const;
    bool PossiblyETSNullish() const;
    bool DefinitelyETSNullish() const;
    bool DefinitelyNotETSNullish() const;

    bool PossiblyETSString() const;
    bool PossiblyETSValueTyped() const;
    bool PossiblyETSValueTypedExceptNullish() const;

    bool PossiblyInForeignDomain() const;

    ETSStringType *AsETSStringType()
    {
        ES2PANDA_ASSERT(IsETSObjectType());
        return reinterpret_cast<ETSStringType *>(this);
    }

    const ETSStringType *AsETSStringType() const
    {
        ES2PANDA_ASSERT(IsETSObjectType());
        return reinterpret_cast<const ETSStringType *>(this);
    }

    const ETSBigIntType *AsETSBigIntType() const
    {
        ES2PANDA_ASSERT(IsETSObjectType());
        return reinterpret_cast<const ETSBigIntType *>(this);
    }

    ETSResizableArrayType *AsETSResizableArrayType()
    {
        ES2PANDA_ASSERT(IsETSResizableArrayType());
        return reinterpret_cast<ETSResizableArrayType *>(this);
    }

    const ETSResizableArrayType *AsETSResizableArrayType() const
    {
        ES2PANDA_ASSERT(IsETSResizableArrayType());
        return reinterpret_cast<const ETSResizableArrayType *>(this);
    }

    [[nodiscard]] bool IsBuiltinNumeric() const noexcept;

    ETSAsyncFuncReturnType *AsETSAsyncFuncReturnType()
    {
        ES2PANDA_ASSERT(IsETSAsyncFuncReturnType());
        return reinterpret_cast<ETSAsyncFuncReturnType *>(this);
    }

    const ETSAsyncFuncReturnType *AsETSAsyncFuncReturnType() const
    {
        ES2PANDA_ASSERT(IsETSAsyncFuncReturnType());
        return reinterpret_cast<const ETSAsyncFuncReturnType *>(this);
    }

    bool IsConstantType() const
    {
        return HasTypeFlag(checker::TypeFlag::CONSTANT);
    }

    bool IsAnyETSArrayOrTupleType() const
    {
        return IsETSArrayType() || IsETSResizableArrayType() || IsETSReadonlyArrayType() || IsETSTupleType();
    }

    Type *MaybeBaseTypeOfGradualType();

    TypeFlag TypeFlags() const
    {
        return typeFlags_;
    }

    bool HasTypeFlag(TypeFlag typeFlag) const
    {
        return (typeFlags_ & typeFlag) != 0;
    }

    void AddTypeFlag(TypeFlag typeFlag)
    {
        typeFlags_ |= typeFlag;
    }

    void RemoveTypeFlag(TypeFlag typeFlag)
    {
        typeFlags_ &= ~typeFlag;
    }

    uint32_t Id() const
    {
        return id_;
    }

    void SetVariable(varbinder::Variable *variable)
    {
        variable_ = variable;
    }

    varbinder::Variable *Variable()
    {
        return variable_;
    }

    const varbinder::Variable *Variable() const
    {
        return variable_;
    }

    util::StringView ToAssemblerTypeView(ArenaAllocator *allocator) const
    {
        std::stringstream ss;
        ToAssemblerType(ss);
        return util::UString(ss.str(), allocator).View();
    }

    std::stringstream ToAssemblerName() const
    {
        std::stringstream ss;
        ToAssemblerType(ss);
        return ss;
    }

    bool IsLambdaObject() const;
    virtual void ToString(std::stringstream &ss, bool precise) const = 0;
    void ToString(std::stringstream &ss) const;
    [[nodiscard]] std::string ToString() const;
    [[nodiscard]] std::string ToStringPrecise() const;

    virtual void ToStringAsSrc(std::stringstream &ss) const;
    std::string ToStringAsSrc() const;

    virtual TypeFacts GetTypeFacts() const;
    virtual void ToAssemblerType([[maybe_unused]] std::stringstream &ss) const {};
    virtual void ToDebugInfoType([[maybe_unused]] std::stringstream &ss) const {};
    virtual void ToAssemblerTypeWithRank([[maybe_unused]] std::stringstream &ss) const
    {
        ToAssemblerType(ss);
    }

    std::string ToAssemblerType() const
    {
        std::stringstream ss;
        ToAssemblerType(ss);
        return ss.str();
    }

    std::string ToAssemblerTypeWithRank() const
    {
        std::stringstream ss;
        ToAssemblerTypeWithRank(ss);
        return ss.str();
    }

    virtual uint32_t Rank() const
    {
        return 0;
    }

    virtual void Identical(TypeRelation *relation, Type *other);
    virtual void AssignmentTarget(TypeRelation *relation, Type *source) = 0;
    virtual bool AssignmentSource(TypeRelation *relation, Type *target);
    virtual void Compare(TypeRelation *relation, Type *other);
    virtual void Cast(TypeRelation *relation, Type *target);
    virtual void CastTarget(TypeRelation *relation, Type *source);
    virtual void IsSupertypeOf(TypeRelation *relation, Type *source);
    virtual void IsSubtypeOf(TypeRelation *relation, Type *target);
    virtual Type *AsSuper(Checker *checker, varbinder::Variable *sourceVar);
    virtual void CheckVarianceRecursively([[maybe_unused]] TypeRelation *relation,
                                          [[maybe_unused]] VarianceFlag varianceFlag)
    {
    }

    virtual Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes);
    [[nodiscard]] virtual Type *Clone(Checker *checker);
    virtual Type *Substitute(TypeRelation *relation, const Substitution *substitution);

protected:
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    TypeFlag typeFlags_;
    varbinder::Variable *variable_ {};  // Variable associated with the type if any
    uint32_t id_;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

// NOLINTBEGIN(readability-redundant-declaration)
// To avoid including type.h from variable.h, astNode.h
bool IsTypeError(Type const *tp);
// NOLINTEND(readability-redundant-declaration)

}  // namespace ark::es2panda::checker

#endif /* TYPESCRIPT_TYPES_TYPE_H */
