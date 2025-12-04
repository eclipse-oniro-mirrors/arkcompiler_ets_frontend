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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_TYPE_RELATION_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_TYPE_RELATION_H

#include <mutex>
#include "lexer/token/sourceLocation.h"
#include "generated/tokenType.h"
#include "util/ustring.h"
#include "util/enumbitops.h"
#include "util/diagnosticEngine.h"

namespace ark::es2panda::ir {
class Expression;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::checker {
class Signature;
class IndexInfo;
class Type;
class Checker;

using ENUMBITOPS_OPERATORS;

enum class TypeRelationFlag : uint32_t {
    NONE = 0U,
    BOXING = 1U << 1U,
    UNBOXING = 1U << 2U,
    CAPTURE = 1U << 3U,
    STRING = 1U << 4U,
    VALUE_SET = 1U << 5U,
    UNCHECKED = 1U << 6U,
    NO_THROW = 1U << 7U,
    SELF_REFERENCE = 1U << 8U,
    NO_RETURN_TYPE_CHECK = 1U << 9U,
    DIRECT_RETURN = 1U << 10U,
    NO_BOXING = 1U << 11U,
    NO_UNBOXING = 1U << 12U,
    ONLY_CHECK_BOXING_UNBOXING = 1U << 13U,
    IN_ASSIGNMENT_CONTEXT = 1U << 14U,
    IN_CASTING_CONTEXT = 1U << 15U,
    UNCHECKED_CAST = 1U << 16U,
    IGNORE_TYPE_PARAMETERS = 1U << 17U,
    CHECK_PROXY = 1U << 18U,
    NO_CHECK_TRAILING_LAMBDA = 1U << 19U,
    NO_THROW_GENERIC_TYPEALIAS = 1U << 20U,
    OVERRIDING_CONTEXT = 1U << 21U,
    IGNORE_REST_PARAM = 1U << 22U,
    STRING_TO_CHAR = 1U << 23U,
    OVERLOADING_CONTEXT = 1U << 24U,
    NO_SUBSTITUTION_NEEDED = 1U << 25U,

    ASSIGNMENT_CONTEXT = BOXING | UNBOXING,
    BRIDGE_CHECK = OVERRIDING_CONTEXT | IGNORE_TYPE_PARAMETERS,
    CASTING_CONTEXT = BOXING | UNBOXING | UNCHECKED_CAST,
};

enum class RelationResult : uint8_t { TRUE, FALSE, UNKNOWN, MAYBE, CACHE_MISS, ERROR };

enum class RelationType : uint8_t { COMPARABLE, ASSIGNABLE, IDENTICAL, UNCHECKED_CASTABLE, SUPERTYPE };

enum class VarianceFlag { COVARIANT, CONTRAVARIANT, INVARIANT };

}  // namespace ark::es2panda::checker

template <>
struct enumbitops::IsAllowedType<ark::es2panda::checker::TypeRelationFlag> : std::true_type {
};

namespace ark::es2panda::checker {

class RelationHolder {
public:
    using RelationKey = uint64_t;

    class RelationEntry {
    public:
        RelationResult result;
        RelationType type;
    };

    explicit RelationHolder(ArenaAllocator *allocator) : cached_(allocator->Adapter()) {}

    static RelationKey MakeKey(uint32_t sourceId, uint32_t targetId)
    {
        constexpr size_t U32_NR_BITS = 32;  // CC-OFF(G.NAM.03-CPP) project code style
        return (static_cast<uint64_t>(sourceId) << U32_NR_BITS) | targetId;
    }

    const RelationEntry *Find(RelationKey key) const
    {
        auto it = cached_.find(key);
        if (it == cached_.cend()) {
            return nullptr;
        }
        return &it->second;
    }

    void Insert(RelationKey key, RelationEntry entry)
    {
        cached_.insert({key, entry});
    }

    void Clear()
    {
        cached_.clear();
    }

private:
    ArenaUnorderedMap<RelationKey, RelationEntry> cached_;
};

class TypeRelation {
public:
    explicit TypeRelation(Checker *checker)
        : checker_(checker), result_(RelationResult::FALSE), instantiationRecursionMap_(Allocator()->Adapter())
    {
    }

    bool IsTrue() const
    {
        return result_ == RelationResult::TRUE;
    }

    bool IsError() const
    {
        return result_ == RelationResult::ERROR;
    }

    bool ApplyBoxing() const
    {
        return (flags_ & TypeRelationFlag::BOXING) != 0;
    }

    bool ApplyUnboxing() const
    {
        return (flags_ & TypeRelationFlag::UNBOXING) != 0;
    }

    bool NoReturnTypeCheck() const
    {
        return (flags_ & TypeRelationFlag::NO_RETURN_TYPE_CHECK) != 0;
    }

    bool DirectReturn() const
    {
        return (flags_ & TypeRelationFlag::DIRECT_RETURN) != 0;
    }

    bool InAssignmentContext() const
    {
        return (((flags_ & TypeRelationFlag::IN_ASSIGNMENT_CONTEXT) !=
                 static_cast<std::underlying_type_t<TypeRelationFlag>>(0U)) ||
                ((flags_ & TypeRelationFlag::ASSIGNMENT_CONTEXT) ==
                 static_cast<std::underlying_type_t<TypeRelationFlag>>(TypeRelationFlag::ASSIGNMENT_CONTEXT)));
    }

    bool OnlyCheckBoxingUnboxing() const
    {
        return (flags_ & TypeRelationFlag::ONLY_CHECK_BOXING_UNBOXING) != 0;
    }

    bool IgnoreTypeParameters() const
    {
        return (flags_ & TypeRelationFlag::IGNORE_TYPE_PARAMETERS) != 0;
    }

    [[nodiscard]] bool InCastingContext() const noexcept
    {
        return (flags_ & TypeRelationFlag::IN_CASTING_CONTEXT) != 0;
    }

    [[nodiscard]] bool UncheckedCast() const noexcept
    {
        return (flags_ & TypeRelationFlag::UNCHECKED_CAST) != 0;
    }

    [[nodiscard]] bool NoThrow() const noexcept
    {
        return (flags_ & TypeRelationFlag::NO_THROW) != 0;
    }

    [[nodiscard]] bool NoThrowGenericTypeAlias() const noexcept
    {
        return (flags_ & TypeRelationFlag::NO_THROW_GENERIC_TYPEALIAS) != 0;
    }

    [[nodiscard]] bool IsOverridingCheck() const noexcept
    {
        return (flags_ & TypeRelationFlag::OVERRIDING_CONTEXT) != 0;
    }

    [[nodiscard]] bool IsBridgeCheck() const noexcept
    {
        return (flags_ & TypeRelationFlag::BRIDGE_CHECK) == helpers::ToUnderlying(TypeRelationFlag::BRIDGE_CHECK);
    }

    [[nodiscard]] TypeRelationFlag GetTypeRelationFlags() const noexcept
    {
        return flags_;
    }

    const Checker *GetChecker() const
    {
        return checker_;
    }

    void SetChecker(Checker *checker)
    {
        checker_ = checker;
    }

    ir::Expression *GetNode() const
    {
        return node_;
    }

    Checker *GetChecker()
    {
        return checker_;
    }

    void IncreaseTypeRecursionCount(Type *const type)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (const auto foundType = instantiationRecursionMap_.find(type);
            foundType != instantiationRecursionMap_.end()) {
            foundType->second += 1;
            return;
        }

        instantiationRecursionMap_.insert({type, 1});
    }

    bool IsAtTypeDepthLimit(Type *const type)
    {
        // This limitation makes sure that no type can be instantiated in infinite recursion. When declaring generic
        // classes with recursive types, so the generic class itself, we need to allow 2 depth of recursion, to make it
        // possible to reference the correct types of it's members and methods. 2 is possibly enough, because if we
        // chain expressions, every one of them will be rechecked separately, thus allowing another 2 recursion.
        constexpr auto MAX_RECURSIVE_TYPE_INST = 2;
        std::lock_guard<std::mutex> lock(mtx_);
        const auto foundType = instantiationRecursionMap_.find(type);
        return foundType == instantiationRecursionMap_.end() ? true : (foundType->second < MAX_RECURSIVE_TYPE_INST);
    }

    void DecreaseTypeRecursionCount(Type *const type)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        const auto foundType = instantiationRecursionMap_.find(type);
        if (foundType == instantiationRecursionMap_.end()) {
            return;
        }

        if (foundType->second > 1) {
            foundType->second -= 1;
            return;
        }

        instantiationRecursionMap_.erase(type);
    }

    [[nodiscard]] bool CheckTypeParameterConstraints(ArenaVector<Type *> const &typeParameters1,
                                                     ArenaVector<Type *> const &typeParameters2);

    //  NOTE: special overloading to be used mainly in ETSCompiler where types and nodes are 'const'.
    //  Unfortunately now we cannot have only a single method with 'const Types *' because it affects
    //  a lot of non-const references... :(((
    bool IsIdenticalTo(Type const *source, Type const *target)
    {
        return IsIdenticalTo(const_cast<Type *>(source), const_cast<Type *>(target));
    }
    bool IsIdenticalTo(Type *source, Type *target);
    bool SignatureIsIdenticalTo(Signature *source, Signature *target);
    bool IsAssignableTo(Type *source, Type *target);
    bool IsComparableTo(Type *source, Type *target);
    bool IsCastableTo(Type *const source, Type *const target);
    bool IsSupertypeOf(Type const *super, Type const *sub)
    {
        return IsSupertypeOf(const_cast<Type *>(super), const_cast<Type *>(sub));
    }
    bool IsLegalBoxedPrimitiveConversion(Type *target, Type *source);
    bool IsSupertypeOf(Type *super, Type *sub);
    bool IsIdenticalTo(IndexInfo *source, IndexInfo *target);
    bool SignatureIsSupertypeOf(Signature *super, Signature *sub);
    bool CheckVarianceRecursively(Type *type, VarianceFlag varianceFlag);
    VarianceFlag TransferVariant(VarianceFlag variance, VarianceFlag posVariance);

    void RaiseError(const diagnostic::DiagnosticKind &kind, const lexer::SourcePosition &loc) const;
    void RaiseError(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &list,
                    const lexer::SourcePosition &loc) const;

    bool Result(bool res)
    {
        result_ = res ? RelationResult::TRUE : RelationResult::FALSE;
        return res;
    }

    void Result(RelationResult res)
    {
        result_ = res;
    }

    void SetNode(ir::Expression *node)
    {
        node_ = node;
    }

    void SetFlags(TypeRelationFlag flags)
    {
        flags_ = flags;
    }

    void RemoveFlags(TypeRelationFlag flags)
    {
        flags_ &= ~flags;
    }

    ArenaAllocator *Allocator();

    friend class SavedTypeRelationFlagsContext;

private:
    RelationResult CacheLookup(const Type *source, const Type *target, const RelationHolder &holder,
                               RelationType type) const;

    std::mutex mtx_;
    Checker *checker_;
    RelationResult result_ {};
    TypeRelationFlag flags_ {};
    ir::Expression *node_ {};
    ArenaMap<checker::Type *, int8_t> instantiationRecursionMap_;
};
class SavedTypeRelationFlagsContext {
public:
    explicit SavedTypeRelationFlagsContext(TypeRelation *relation, TypeRelationFlag newFlag)
        : relation_(relation), prev_(relation->flags_)
    {
        relation_->flags_ = newFlag;
    }

    NO_COPY_SEMANTIC(SavedTypeRelationFlagsContext);
    DEFAULT_MOVE_SEMANTIC(SavedTypeRelationFlagsContext);

    ~SavedTypeRelationFlagsContext()
    {
        relation_->flags_ = prev_;
    }

private:
    TypeRelation *relation_;
    TypeRelationFlag prev_;
};
}  // namespace ark::es2panda::checker

#endif /* TYPESCRIPT_TYPES_TYPE_RELATION_H */
