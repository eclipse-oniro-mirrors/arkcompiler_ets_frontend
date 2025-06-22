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

#include <numeric>
#include "etsObjectType.h"
#include "etsUnionType.h"
#include "checker/ets/conversion.h"
#include "checker/types/ets/etsTupleType.h"
#include "checker/types/globalTypesHolder.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::checker {
void ETSUnionType::ToString(std::stringstream &ss, bool precise) const
{
    for (auto it = constituentTypes_.begin(); it != constituentTypes_.end(); it++) {
        (*it)->ToString(ss, precise);
        if (std::next(it) != constituentTypes_.end()) {
            ss << "|";
        }
    }
}

void ETSUnionType::ToAssemblerType(std::stringstream &ss) const
{
    assemblerLub_->ToAssemblerTypeWithRank(ss);
}

void ETSUnionType::ToDebugInfoType(std::stringstream &ss) const
{
    assemblerLub_->ToDebugInfoType(ss);
}

ETSUnionType::ETSUnionType(ETSChecker *checker, ArenaVector<Type *> &&constituentTypes)
    : Type(TypeFlag::ETS_UNION), constituentTypes_(std::move(constituentTypes))
{
    ES2PANDA_ASSERT(constituentTypes_.size() > 1);
    assemblerLub_ = ComputeAssemblerLUB(checker, this);
}

bool ETSUnionType::EachTypeRelatedToSomeType(TypeRelation *relation, ETSUnionType *source, ETSUnionType *target)
{
    return std::all_of(source->constituentTypes_.begin(), source->constituentTypes_.end(),
                       [relation, target](auto *s) { return TypeRelatedToSomeType(relation, s, target); });
}

bool ETSUnionType::TypeRelatedToSomeType(TypeRelation *relation, Type *source, ETSUnionType *target)
{
    return std::any_of(target->constituentTypes_.begin(), target->constituentTypes_.end(),
                       [relation, source](auto *t) { return relation->IsIdenticalTo(source, t); });
}

// This function computes effective runtime representation of union type
Type *ETSUnionType::ComputeAssemblerLUB(ETSChecker *checker, ETSUnionType *un)
{
    auto *const apparent = checker->GetApparentType(un);
    if (!apparent->IsETSUnionType()) {
        return apparent;
    }
    if (apparent != un) {
        return apparent->AsETSUnionType()->assemblerLub_;
    }
    un = apparent->AsETSUnionType();

    Type *lub = nullptr;
    for (auto *t : un->ConstituentTypes()) {
        if (t->IsTypeError()) {
            return checker->GlobalTypeError();
        }
        // NOTE(vpukhov): #19701 void refactoring
        ES2PANDA_ASSERT(t->IsETSReferenceType() || t->IsETSVoidType());
        t = t->IsETSVoidType() ? checker->GlobalETSUndefinedType() : t;

        if (lub == nullptr || lub->IsETSUndefinedType()) {
            lub = t;
            continue;
        }
        if (lub == t || t->IsETSUndefinedType()) {
            continue;
        }
        if (t->IsETSNullType()) {
            return checker->GetGlobalTypesHolder()->GlobalETSObjectType();
        }
        if (t->IsETSObjectType() && lub->IsETSObjectType()) {
            lub = checker->GetClosestCommonAncestor(lub->AsETSObjectType(), t->AsETSObjectType());
        } else if (t->IsETSArrayType() && lub->IsETSArrayType()) {
            // NOTE: can compute "common(lub, t)[]"
            return checker->GetGlobalTypesHolder()->GlobalETSObjectType();
        } else {
            return checker->GetGlobalTypesHolder()->GlobalETSObjectType();
        }
    }
    return checker->GetNonConstantType(lub);
}

void ETSUnionType::Identical(TypeRelation *relation, Type *other)
{
    if (other->IsETSUnionType()) {
        if (EachTypeRelatedToSomeType(relation, this, other->AsETSUnionType()) &&
            EachTypeRelatedToSomeType(relation, other->AsETSUnionType(), this)) {
            relation->Result(true);
            return;
        }
    }

    relation->Result(false);
}

static void AmbiguousUnionOperation(TypeRelation *relation)
{
    auto checker = relation->GetChecker()->AsETSChecker();
    if (!relation->NoThrow()) {
        checker->LogError(diagnostic::AMBIGUOUS_UNION_TYPE_OP, {}, relation->GetNode()->Start());
    }
    conversion::Forbidden(relation);
}

template <typename RelFN>
void ETSUnionType::RelationTarget(TypeRelation *relation, Type *source, RelFN const &relFn)
{
    auto *const checker = relation->GetChecker()->AsETSChecker();
    auto *const refsource = checker->MaybeBoxType(source);

    relation->Result(false);

    if (refsource != source && !relation->ApplyBoxing()) {
        return;
    }

    if (AnyOfConstituentTypes([relation, refsource, relFn](auto *t) { return relFn(relation, refsource, t); })) {
        relation->Result(true);
        return;
    }

    if (refsource == source) {
        relation->IsSupertypeOf(this, refsource);
        return;
    }

    bool related = false;
    for (auto *ct : ConstituentTypes()) {
        if (relFn(relation, source, checker->MaybeUnboxType(ct))) {
            if (related) {
                AmbiguousUnionOperation(relation);
                return;
            }
            related = true;
        }
    }

    relation->Result(related);
}

bool ETSUnionType::AssignmentSource(TypeRelation *relation, Type *target)
{
    ES2PANDA_ASSERT(!target->IsETSPrimitiveType());
    return relation->Result(
        AllOfConstituentTypes([relation, target](auto *t) { return relation->IsAssignableTo(t, target); }));
}

void ETSUnionType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    auto const relFn = [](TypeRelation *rel, Type *src, Type *tgt) { return rel->IsAssignableTo(src, tgt); };
    RelationTarget(relation, source, relFn);
}

void ETSUnionType::Cast(TypeRelation *relation, Type *target)
{
    ES2PANDA_ASSERT(!target->IsETSPrimitiveType());

    if (relation->InCastingContext()) {
        relation->Result(
            AnyOfConstituentTypes([relation, target](auto *t) { return relation->IsCastableTo(t, target); }));
        return;
    }

    relation->Result(AllOfConstituentTypes([relation, target](auto *t) { return relation->IsCastableTo(t, target); }));
}

void ETSUnionType::CastTarget(TypeRelation *relation, Type *source)
{
    auto const relFn = [](TypeRelation *rel, Type *src, Type *tgt) -> bool { return rel->IsCastableTo(src, tgt); };
    RelationTarget(relation, source, relFn);
}

static std::optional<Type *> TryMergeTypes(TypeRelation *relation, Type *const t1, Type *const t2)
{
    auto *const checker = relation->GetChecker()->AsETSChecker();
    auto *const never = checker->GetGlobalTypesHolder()->GlobalETSNeverType();

    if (relation->IsSupertypeOf(t1, t2) || t2 == never) {
        return t1;
    }
    if (relation->IsSupertypeOf(t2, t1) || t1 == never) {
        return t2;
    }
    return std::nullopt;
}

void ETSUnionType::LinearizeAndEraseIdentical(TypeRelation *relation, ArenaVector<Type *> &types)
{
    // Linearize
    std::size_t const initialSz = types.size();
    for (std::size_t i = 0U; i < initialSz; ++i) {
        auto ct = types[i];
        if (ct->IsETSUnionType()) {
            auto const &otherTypes = ct->AsETSUnionType()->ConstituentTypes();
            types.insert(types.end(), otherTypes.begin(), otherTypes.end());
            types[i] = nullptr;
        } else if (ct->IsNeverType()) {
            types[i] = nullptr;
        }
    }

    // Remove nullptrs
    types.erase(std::remove_if(types.begin(), types.end(), [](Type *ct) { return ct == nullptr; }), types.end());

    // Reduce subtypes
    for (auto cmpIt = types.begin(); cmpIt != types.end(); ++cmpIt) {
        auto it = std::next(cmpIt);
        while (it != types.end()) {
            if (auto merged = TryMergeTypes(relation, *cmpIt, *it); !merged) {
                ++it;
            } else if (*merged == *cmpIt) {
                it = types.erase(it);
            } else {
                cmpIt = types.erase(cmpIt);
                it = cmpIt != types.end() ? std::next(cmpIt) : cmpIt;
            }
        }
    }
}

void ETSUnionType::NormalizeTypes(TypeRelation *relation, ArenaVector<Type *> &types)
{
    if (types.size() == 1U) {
        return;
    }

    LinearizeAndEraseIdentical(relation, types);
}

Type *ETSUnionType::Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes)
{
    auto *const checker = relation->GetChecker()->AsETSChecker();
    ArenaVector<Type *> copiedConstituents(allocator->Adapter());
    for (auto *it : constituentTypes_) {
        copiedConstituents.emplace_back(it->Instantiate(allocator, relation, globalTypes));
    }
    return checker->CreateETSUnionType(std::move(copiedConstituents));
}

Type *ETSUnionType::Substitute(TypeRelation *relation, const Substitution *substitution)
{
    auto *const checker = relation->GetChecker()->AsETSChecker();
    ArenaVector<Type *> substitutedConstituents(checker->Allocator()->Adapter());
    for (auto *ctype : constituentTypes_) {
        substitutedConstituents.emplace_back(ctype->Substitute(relation, substitution));
    }
    return checker->CreateETSUnionType(std::move(substitutedConstituents));
}

void ETSUnionType::IsSupertypeOf(TypeRelation *relation, Type *source)
{
    for (auto const *ctype : ConstituentTypes()) {
        if (relation->IsSupertypeOf(ctype, source)) {
            return;
        }
    }
}

void ETSUnionType::IsSubtypeOf(TypeRelation *relation, Type *target)
{
    for (auto const *ctype : ConstituentTypes()) {
        if (!relation->IsSupertypeOf(target, ctype)) {
            return;
        }
    }
}

void ETSUnionType::CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag)
{
    for (auto *ctype : ConstituentTypes()) {
        relation->CheckVarianceRecursively(ctype, relation->TransferVariant(varianceFlag, VarianceFlag::COVARIANT));
    }
}

//  ATTENTION! When calling this method we assume that 'AssignmentTarget(...)' check was passes successfully,
//  thus the required assignable type (or corresponding supertype) always exists.
checker::Type *ETSUnionType::GetAssignableType(checker::ETSChecker *checker, checker::Type *sourceType,
                                               [[maybe_unused]] std::optional<double> value) const noexcept
{
    for (auto *ctype : ConstituentTypes()) {
        if (checker->Relation()->IsSupertypeOf(ctype, sourceType)) {
            return ctype;
        }
    }

    if (!sourceType->IsBuiltinNumeric()) {
        return nullptr;
    }

    // NOTE (DZ): we still keep 'numericTypes` collection for possible processing cases like 'let x: short|double = 1`
    //            Waiting for complete clearness in spec - now return the highest type in such a case or type itself.
    //            Maybe 'value' will be used for this purpose
    std::map<std::uint32_t, checker::ETSObjectType *> numericTypes {};
    auto *objectType = sourceType->AsETSObjectType();
    if (auto *assignableType = GetAssignableBuiltinType(checker, objectType, numericTypes); assignableType != nullptr) {
        return assignableType;
    }

    if (!numericTypes.empty()) {
        return (*std::prev(numericTypes.end())).second;
    }
    return nullptr;
}

checker::Type *ETSUnionType::GetAssignableBuiltinType(
    checker::ETSChecker *checker, checker::ETSObjectType *sourceType,
    std::map<std::uint32_t, checker::ETSObjectType *> &numericTypes) const noexcept
{
    for (auto *constituentType : constituentTypes_) {
        if (!constituentType->IsETSObjectType() && !constituentType->IsETSTupleType()) {
            continue;
        }

        ETSObjectType *objectType = constituentType->AsETSObjectType();
        if (!objectType->IsBuiltinNumeric()) {
            continue;
        }

        if (checker->Relation()->IsIdenticalTo(objectType, sourceType)) {
            return sourceType;
        }

        numericTypes.emplace(ETSObjectType::GetPrecedence(checker, objectType), objectType);
    }

    return nullptr;
}

bool ETSUnionType::ExtractType(checker::ETSChecker *checker, checker::Type *source,
                               ArenaVector<Type *> &unionTypes) noexcept
{
    std::map<std::uint32_t, ArenaVector<checker::Type *>::const_iterator> numericTypes {};
    source = checker->GetNonConstantType(source);

    bool rc = false;
    auto it = unionTypes.cbegin();
    while (it != unionTypes.cend()) {
        auto *constituentType = (*it)->MaybeBaseTypeOfGradualType();
        //  Because 'instanceof' expression does not check for type parameters, then for generic types we should
        //  consider that expressions like 'SomeType<U...>' and 'SomeType<T...>' are identical for smart casting.
        //  We also have to pass through all the union to process cases like 'C<T>|A|B|C<U>|undefined`
        if (constituentType->IsETSTypeParameter()) {
            constituentType = constituentType->AsETSTypeParameter()->GetConstraintType();
        } else if (constituentType->HasTypeFlag(checker::TypeFlag::GENERIC)) {
            constituentType = constituentType->Clone(checker);
            constituentType->RemoveTypeFlag(checker::TypeFlag::GENERIC);
        }

        if (checker->Relation()->IsIdenticalTo(constituentType, source)) {
            rc = true;
            if (!(*it)->IsETSTypeParameter()) {
                it = unionTypes.erase(it);
            }
            continue;
        }

        if (checker->Relation()->IsSupertypeOf(constituentType, source)) {
            rc = true;
        } else if (!rc && constituentType->IsBuiltinNumeric()) {
            if (auto const id = ETSObjectType::GetPrecedence(checker, constituentType->AsETSObjectType()); id > 0U) {
                numericTypes.emplace(id, it);
            }
        }

        ++it;
    }

    if (rc) {
        return true;
    }

    if (source->IsBuiltinNumeric() && !numericTypes.empty()) {
        unionTypes.erase((*std::prev(numericTypes.end())).second);
        return true;
    }

    return false;
}

std::pair<checker::Type *, checker::Type *> ETSUnionType::GetComplimentaryType(ETSChecker *const checker,
                                                                               checker::Type *sourceType)
{
    ArenaVector<Type *> unionTypes(checker->Allocator()->Adapter());
    for (auto *ct : constituentTypes_) {
        unionTypes.emplace_back(ct->Clone(checker));
    }

    auto const extractType = [checker, &unionTypes](Type *&type) -> bool {
        ES2PANDA_ASSERT(!type->IsETSPrimitiveType());
        if (type->IsETSEnumType()) {
            return true;
        }
        if (type->HasTypeFlag(checker::TypeFlag::GENERIC)) {
            //  Because 'instanceof' expression does not check for type parameters, then for generic types we should
            //  consider that expressions like 'SomeType<U>' and 'SomeType<T>' are identical for smart casting.
            type = type->Clone(checker);
            type->RemoveTypeFlag(checker::TypeFlag::GENERIC);
        }
        return ExtractType(checker, type, unionTypes);
    };

    bool ok = true;

    if (sourceType->IsETSUnionType()) {
        for (auto *constituentType : sourceType->AsETSUnionType()->ConstituentTypes()) {
            if (ok = extractType(constituentType); !ok) {
                break;
            }
        }
    } else {
        ok = extractType(sourceType);
    }

    if (!ok) {
        return std::make_pair(checker->GetGlobalTypesHolder()->GlobalETSNeverType(), this);
    }

    checker::Type *complimentaryType;
    if (auto const size = unionTypes.size(); size == 0U) {
        complimentaryType = checker->GetGlobalTypesHolder()->GlobalETSNeverType();
    } else if (size == 1U) {
        complimentaryType = unionTypes.front();
    } else {
        complimentaryType = checker->CreateETSUnionType(std::move(unionTypes));
    }

    return std::make_pair(sourceType, complimentaryType);
}

Type *ETSUnionType::FindUnboxableType() const noexcept
{
    return FindSpecificType([](Type *t) { return t->IsETSUnboxableObject(); });
}

bool ETSUnionType::IsOverlapWith(TypeRelation *relation, Type const *type) const noexcept
{
    // NOTE(aakmaev): replace this func with intersection type when it will be implemented
    for (auto *ct : constituentTypes_) {
        if (type->IsETSUnionType() && type->AsETSUnionType()->IsOverlapWith(relation, ct)) {
            return true;
        }
        if (type->IsETSObjectType() && ct->IsETSObjectType()) {
            if (type->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::BUILTIN_NUMERIC) &&
                ct->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::BUILTIN_NUMERIC)) {
                return true;
            }
        }
        if (relation->IsSupertypeOf(ct, type) || relation->IsSupertypeOf(type, ct)) {
            return true;
        }
    }
    return false;
}

ArenaVector<Type *> ETSUnionType::GetNonConstantTypes(ETSChecker *checker) const noexcept
{
    ArenaVector<Type *> nonConstTypes(checker->Allocator()->Adapter());
    for (auto *ct : constituentTypes_) {
        nonConstTypes.emplace_back(checker->GetNonConstantType(ct));
    }
    return nonConstTypes;
}

}  // namespace ark::es2panda::checker
