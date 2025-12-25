/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "checker/ETSchecker.h"
#include "checker/ets/conversion.h"

namespace ark::es2panda::checker {
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
    if (GetTupleSize() != other->GetTupleSize()) {
        return false;
    }

    for (TupleSizeType idx = 0; idx < GetTupleSize(); ++idx) {
        if (!relation->IsIdenticalTo(GetTypeAtIndex(idx), other->GetTypeAtIndex(idx))) {
            return false;
        }
    }
    return true;
}

void ETSTupleType::Identical([[maybe_unused]] TypeRelation *const relation, Type *const other)
{
    if (!other->IsETSTupleType()) {
        return;
    }

    const auto *const otherTuple = other->AsETSTupleType();

    if (HasTypeFlag(TypeFlag::READONLY) != other->HasTypeFlag(TypeFlag::READONLY)) {
        relation->Result(false);
        return;
    }

    if (!CheckElementsIdentical(relation, otherTuple)) {
        relation->Result(false);
        return;
    }

    relation->Result(true);
}

bool ETSTupleType::AssignmentSource(TypeRelation *const relation, Type *const target)
{
    if (!target->IsETSTupleType()) {
        return false;
    }

    return relation->IsTrue();
}

void ETSTupleType::AssignmentTarget(TypeRelation *const relation, Type *const source)
{
    if (!source->HasTypeFlag(TypeFlag::READONLY) && source->IsETSTupleType()) {
        source->AsETSTupleType()->IsSubtypeOf(relation, this);
    }
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
    if (target->IsETSTupleType()) {
        if (!HasTypeFlag(TypeFlag::READONLY) && CheckElementsIdentical(relation, target->AsETSTupleType())) {
            relation->Result(true);
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
