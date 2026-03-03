/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "etsTupleType.h"

#include "checker/ets/conversion.h"

namespace ark::es2panda::checker {

ETSTupleType::ETSTupleType(ETSChecker *checker, ArenaVector<Type *> &&typeList)
    : Type(checker::TypeFlag::ETS_TUPLE), typeList_(std::move(typeList))
{
    typeFlags_ |= TypeFlag::ETS_TUPLE;

    auto typeSize = typeList_.size();
    auto *type = checker->GlobalBuiltinTupleType(typeSize);

    auto const &typeArguments = type->TypeArguments();
    typeSize = typeArguments.size();  // We can have more actual parameters than type arguments!

    Substitution substitution {};
    for (std::size_t i = 0U; i < typeSize; ++i) {
        ES2PANDA_ASSERT(typeArguments[i]->IsETSTypeParameter());
        substitution.emplace(typeArguments[i]->AsETSTypeParameter(), typeList_[i]);
    }

    wrapperType_ = type->Substitute(checker->Relation(), &substitution);
}

void ETSTupleType::ToString(std::stringstream &ss, bool precise) const
{
    if (HasTypeFlag(TypeFlag::READONLY)) {
        ss << "readonly ";
    }

    ss << "[";

    for (auto it = typeList_.begin(); it != typeList_.end(); it++) {
        (*it)->ToString(ss, precise);

        if (std::next(it) != typeList_.end()) {
            ss << ", ";
        }
    }

    ss << "]";
}

void ETSTupleType::ToAssemblerType(std::stringstream &ss) const
{
    wrapperType_->ToAssemblerType(ss);
}

void ETSTupleType::ToDebugInfoType(std::stringstream &ss) const
{
    if (HasTypeFlag(TypeFlag::READONLY)) {
        ss << "readonly ";
    }

    ss << "[";

    for (auto it = typeList_.begin(); it != typeList_.end(); it++) {
        (*it)->ToDebugInfoType(ss);

        if (std::next(it) != typeList_.end()) {
            ss << ", ";
        }
    }

    ss << "]";
}

Type *ETSTupleType::GetTypeAtIndex(const TupleSizeType index) const
{
    if (index >= GetTupleSize()) {  // happens when dealing with type errors
        return nullptr;
    }
    return GetTupleTypesList().at(index);
}

bool ETSTupleType::CheckElementsIdentical(TypeRelation *relation, const ETSTupleType *other) const
{
    ES2PANDA_ASSERT(GetTupleSize() <= other->GetTupleSize());
    for (TupleSizeType idx = 0U; idx < GetTupleSize(); ++idx) {
        if (!relation->IsIdenticalTo(GetTypeAtIndex(idx), other->GetTypeAtIndex(idx))) {
            return false;
        }
    }
    return true;
}

void ETSTupleType::Identical(TypeRelation *const relation, Type *const other)
{
    relation->Result(false);

    if (other->IsETSTupleType() && HasTypeFlag(TypeFlag::READONLY) == other->HasTypeFlag(TypeFlag::READONLY)) {
        auto *tupleType = other->AsETSTupleType();
        if (GetTupleSize() == tupleType->GetTupleSize() && CheckElementsIdentical(relation, tupleType)) {
            relation->Result(true);
        }
    }
}

bool ETSTupleType::AssignmentSource(TypeRelation *const relation, Type *const target)
{
    IsSubtypeOf(relation, target);
    return relation->IsTrue();
}

void ETSTupleType::AssignmentTarget(TypeRelation *const relation, Type *const source)
{
    IsSupertypeOf(relation, source);
}

Type *ETSTupleType::Substitute(TypeRelation *relation, const Substitution *substitution)
{
    auto *const checker = relation->GetChecker()->AsETSChecker();
    std::vector<Type *> newTypeList;

    for (auto *const tupleTypeListElement : GetTupleTypesList()) {
        newTypeList.emplace_back(tupleTypeListElement->Substitute(relation, substitution));
    }

    return checker->CreateETSTupleType(std::move(newTypeList), HasTypeFlag(TypeFlag::READONLY));
}

void ETSTupleType::IsSubtypeOf(TypeRelation *const relation, Type *target)
{
    if (target->IsETSObjectType() && target->AsETSObjectType()->IsGlobalETSObjectType()) {
        relation->Result(true);
        return;
    }

    relation->Result(false);

    if (!HasTypeFlag(TypeFlag::READONLY) || target->HasTypeFlag(TypeFlag::READONLY)) {
        if (target->IsETSObjectType()) {
            relation->IsSupertypeOf(target, GetWrapperType());
        } else if (target->IsETSTupleType()) {
            auto *tupleType = target->AsETSTupleType();
            if (GetTupleSize() >= tupleType->GetTupleSize() && tupleType->CheckElementsIdentical(relation, this)) {
                relation->Result(true);
            }
        }
    }
}

void ETSTupleType::IsSupertypeOf(TypeRelation *relation, Type *source)
{
    relation->Result(false);

    if (!source->HasTypeFlag(TypeFlag::READONLY) || HasTypeFlag(TypeFlag::READONLY)) {
        if (source->IsETSTupleType()) {
            auto *tupleType = source->AsETSTupleType();
            if (GetTupleSize() <= tupleType->GetTupleSize() && CheckElementsIdentical(relation, tupleType)) {
                relation->Result(true);
            }
        } else if (source->IsETSObjectType()) {
            relation->IsSupertypeOf(GetWrapperType(), source);
        }
    }
}

void ETSTupleType::Cast(TypeRelation *const relation, Type *const target)
{
    if (!(target->IsETSTupleType() || target->IsETSArrayType())) {
        conversion::Forbidden(relation);
        return;
    }

    if (target->IsETSArrayType() && (!target->IsETSTupleType())) {
        auto *const arrayTarget = target->AsETSArrayType();

        if (!arrayTarget->ElementType()->IsETSObjectType()) {
            conversion::Forbidden(relation);
            return;
        }

        const bool elementsAreSupertypes =
            std::all_of(GetTupleTypesList().begin(), GetTupleTypesList().end(),
                        [&relation, &arrayTarget](auto *const tupleTypeAtIdx) {
                            return relation->IsSupertypeOf(arrayTarget->ElementType(), tupleTypeAtIdx);
                        });

        relation->Result(elementsAreSupertypes);
        return;
    }

    const auto *const tupleTarget = target->AsETSTupleType();

    if (tupleTarget->GetTupleSize() != GetTupleSize()) {
        return;
    }

    for (TupleSizeType idx = 0; idx < GetTupleSize(); ++idx) {
        if (!relation->IsSupertypeOf(GetTypeAtIndex(idx), tupleTarget->GetTypeAtIndex(idx))) {
            return;
        }
    }

    relation->Result(true);
}

Type *ETSTupleType::Instantiate([[maybe_unused]] ArenaAllocator *allocator, [[maybe_unused]] TypeRelation *relation,
                                [[maybe_unused]] GlobalTypesHolder *globalTypes)
{
    auto *const checker = relation->GetChecker()->AsETSChecker();
    ArenaVector<Type *> copiedElements(checker->Allocator()->Adapter());
    copiedElements.reserve(GetTupleTypesList().size());
    for (auto t : GetTupleTypesList()) {
        copiedElements.push_back(t->Instantiate(allocator, relation, globalTypes));
    }
    auto *const tupleType = allocator->New<ETSTupleType>(checker, std::move(copiedElements));
    ES2PANDA_ASSERT(tupleType != nullptr);
    tupleType->typeFlags_ = typeFlags_;
    return tupleType;
}

void ETSTupleType::CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag)
{
    for (auto const &ctype : typeList_) {
        relation->CheckVarianceRecursively(ctype, relation->TransferVariant(varianceFlag, VarianceFlag::INVARIANT));
    }
}

void ETSTupleType::Iterate(const TypeTraverser &func) const
{
    for (auto const *const type : typeList_) {
        func(type);
    }
}

}  // namespace ark::es2panda::checker
