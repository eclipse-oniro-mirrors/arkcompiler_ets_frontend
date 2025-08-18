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

#include "typeRelation.h"

#include "checker/checker.h"
#include "checker/ETSchecker.h"
#include "checker/types/ts/indexInfo.h"
#include "checker/types/signature.h"

namespace ark::es2panda::checker {
ArenaAllocator *TypeRelation::Allocator()
{
    return checker_->ProgramAllocator();
}

RelationResult TypeRelation::CacheLookup(const Type *source, const Type *target, const RelationHolder &holder,
                                         RelationType type) const
{
    ES2PANDA_ASSERT(source != nullptr);
    ES2PANDA_ASSERT(target != nullptr);

    auto key = RelationHolder::MakeKey(source->Id(), target->Id());
    auto res = holder.Find(key);
    if (res == nullptr) {
        return RelationResult::CACHE_MISS;
    }

    if (res->type >= type && res->result == RelationResult::TRUE) {
        return RelationResult::TRUE;
    }

    if (res->type <= type && res->result == RelationResult::FALSE) {
        return RelationResult::FALSE;
    }

    return RelationResult::CACHE_MISS;
}

bool TypeRelation::IsIdenticalTo(Type *source, Type *target)
{
    if (source == nullptr || target == nullptr) {
        return Result(false);
    }

    if (source == target) {
        return Result(true);
    }

    result_ = CacheLookup(source, target, checker_->IdenticalResults(), RelationType::IDENTICAL);
    if (result_ == RelationResult::CACHE_MISS) {
        checker_->ResolveStructuredTypeMembers(source);
        checker_->ResolveStructuredTypeMembers(target);
        result_ = RelationResult::FALSE;
        target->Identical(this, source);
        auto key = RelationHolder::MakeKey(source->Id(), target->Id());
        checker_->IdenticalResults().Insert(key, {result_, RelationType::IDENTICAL});
    }

    return IsTrue();
}

bool TypeRelation::SignatureIsIdenticalTo(Signature *source, Signature *target)
{
    if (source == target) {
        return Result(true);
    }

    Result(false);
    if (target->IsSubtypeOf(this, source), IsTrue()) {
        if (source->IsSubtypeOf(this, target), IsTrue()) {
            return Result(true);
        }
    }
    return Result(false);
}

bool TypeRelation::SignatureIsSupertypeOf(Signature *super, Signature *sub)
{
    if (super == sub) {
        return Result(true);
    }

    Result(false);
    sub->IsSubtypeOf(this, super);
    return IsTrue();
}

bool TypeRelation::IsIdenticalTo(IndexInfo *source, IndexInfo *target)
{
    if (source == target) {
        Result(true);
        return true;
    }

    result_ = RelationResult::FALSE;
    target->Identical(this, source);

    return result_ == RelationResult::TRUE;
}

bool TypeRelation::IsAssignableTo(Type *source, Type *target)
{
    if (source == target) {
        return Result(true);
    }

    result_ = CacheLookup(source, target, checker_->AssignableResults(), RelationType::ASSIGNABLE);
    if (result_ == RelationResult::CACHE_MISS) {
        // NOTE: we support assigning T to Readonly<T>, but do not support assigning Readonly<T> to T
        // more details in spec
        ES2PANDA_ASSERT(source != nullptr);
        ES2PANDA_ASSERT(target != nullptr);
        if (source->HasTypeFlag(TypeFlag::READONLY) && !target->HasTypeFlag(TypeFlag::READONLY)) {
            result_ = RelationResult::FALSE;
        }

        auto key = RelationHolder::MakeKey(source->Id(), target->Id());
        if (result_ != RelationResult::FALSE && IsIdenticalTo(source, target)) {
            checker_->AssignableResults().Insert(key, {result_, RelationType::ASSIGNABLE});
            return true;
        }

        if (result_ == RelationResult::ERROR) {
            // Return early to prevent logging same errors again
            return false;
        }

        result_ = RelationResult::FALSE;
        if (!source->AssignmentSource(this, target)) {
            target->AssignmentTarget(this, source);
        }

        if (flags_ == TypeRelationFlag::NONE) {
            checker_->AssignableResults().Insert(key, {result_, RelationType::ASSIGNABLE});
        }
    }

    return result_ == RelationResult::TRUE;
}

bool TypeRelation::IsComparableTo(Type *source, Type *target)
{
    result_ = CacheLookup(source, target, checker_->ComparableResults(), RelationType::COMPARABLE);
    ES2PANDA_ASSERT(source != nullptr);
    ES2PANDA_ASSERT(target != nullptr);
    if (result_ == RelationResult::CACHE_MISS) {
        if (IsAssignableTo(source, target)) {
            return true;
        }

        result_ = RelationResult::FALSE;
        target->Compare(this, source);
        ES2PANDA_ASSERT(source != nullptr);
        auto key = RelationHolder::MakeKey(source->Id(), target->Id());
        checker_->ComparableResults().Insert(key, {result_, RelationType::COMPARABLE});
    }

    return result_ == RelationResult::TRUE;
}

bool TypeRelation::IsCastableTo(Type *const source, Type *const target)
{
    result_ = CacheLookup(source, target, checker_->UncheckedCastableResult(), RelationType::UNCHECKED_CASTABLE);
    if (result_ == RelationResult::CACHE_MISS) {
        result_ = RelationResult::FALSE;
        flags_ |= TypeRelationFlag::UNCHECKED_CAST;

        source->Cast(this, target);
        if (!IsTrue()) {
            target->CastTarget(this, source);
        }

        if (!IsTrue()) {
            return false;
        }

        if (UncheckedCast() && !node_->HasAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF)) {
            auto key = RelationHolder::MakeKey(source->Id(), target->Id());
            checker_->UncheckedCastableResult().Insert(key, {result_, RelationType::UNCHECKED_CASTABLE});
        }

        return true;
    }

    return result_ == RelationResult::TRUE;
}

bool TypeRelation::IsLegalBoxedPrimitiveConversion(Type *target, Type *source)
{
    ETSChecker *checker = this->GetChecker()->AsETSChecker();

    if (target == nullptr || source == nullptr) {
        return false;
    }

    if (target->IsETSUnionType() && source->IsETSObjectType()) {
        Type *sourceUnboxedType = checker->MaybeUnboxType(source);
        if (sourceUnboxedType == nullptr || !sourceUnboxedType->IsETSPrimitiveType()) {
            return false;
        }
        Type *boxedUnionTarget = target->AsETSUnionType()->FindUnboxableType();
        if (boxedUnionTarget == nullptr) {
            return false;
        }
        Type *targetUnboxedType = checker->MaybeUnboxType(boxedUnionTarget);
        if (targetUnboxedType == nullptr || !targetUnboxedType->IsETSPrimitiveType()) {
            return false;
        }
        bool res = this->Result(this->IsAssignableTo(sourceUnboxedType, target));
        return res;
    }

    if (!target->IsETSObjectType() || !source->IsETSObjectType()) {
        return false;
    }

    if (!target->AsETSObjectType()->IsBoxedPrimitive() && !source->AsETSObjectType()->IsBoxedPrimitive()) {
        return false;
    }

    Type *targetUnboxedType = checker->MaybeUnboxType(target);
    Type *sourceUnboxedType = checker->MaybeUnboxType(source);

    if (source->IsETSIntEnumType()) {
        targetUnboxedType = checker->GlobalIntType();
    }

    if (source->IsETSDoubleEnumType()) {
        targetUnboxedType = checker->GlobalDoubleType();
    }

    if (targetUnboxedType == nullptr || sourceUnboxedType == nullptr) {
        return false;
    }
    if (!targetUnboxedType->IsETSPrimitiveType() || !sourceUnboxedType->IsETSPrimitiveType()) {
        return false;
    }

    bool res = this->Result(this->IsAssignableTo(sourceUnboxedType, targetUnboxedType));
    return res;
}

bool TypeRelation::IsSupertypeOf(Type *super, Type *sub)
{
    if (LIKELY(super == sub)) {
        return Result(true);
    }
    if (sub == nullptr) {
        return false;
    }
    if (super->IsETSPrimitiveType() != sub->IsETSPrimitiveType()) {
        return false;
    }

    result_ = CacheLookup(super, sub, checker_->SupertypeResults(), RelationType::SUPERTYPE);
    if (result_ == RelationResult::CACHE_MISS) {
        if (!IsIdenticalTo(super, sub)) {
            result_ = RelationResult::FALSE;
            if (super->IsSupertypeOf(this, sub), !IsTrue()) {
                sub->IsSubtypeOf(this, super);
            }
        }

        auto key = RelationHolder::MakeKey(super->Id(), sub->Id());
        checker_->SupertypeResults().Insert(key, {result_, RelationType::SUPERTYPE});
    }

    return result_ == RelationResult::TRUE;
}

bool TypeRelation::CheckVarianceRecursively(Type *type, VarianceFlag varianceFlag)
{
    type->CheckVarianceRecursively(this, varianceFlag);
    return result_ == RelationResult::TRUE;
}

VarianceFlag TypeRelation::TransferVariant(VarianceFlag variance, VarianceFlag posVariance)
{
    if (posVariance == VarianceFlag::INVARIANT || variance == VarianceFlag::INVARIANT) {
        return VarianceFlag::INVARIANT;
    }

    if (posVariance == VarianceFlag::COVARIANT) {
        return variance;
    }

    return variance == VarianceFlag::CONTRAVARIANT ? VarianceFlag::COVARIANT : VarianceFlag::CONTRAVARIANT;
}

void TypeRelation::RaiseError(const diagnostic::DiagnosticKind &kind, const lexer::SourcePosition &loc) const
{
    RaiseError(kind, {}, loc);
}

void TypeRelation::RaiseError(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &list,
                              const lexer::SourcePosition &loc) const
{
    checker_->LogError(kind, list, loc);
}
}  // namespace ark::es2panda::checker
