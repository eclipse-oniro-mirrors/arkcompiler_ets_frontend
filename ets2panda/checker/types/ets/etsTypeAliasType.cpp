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

#include "etsTypeAliasType.h"

#include "checker/ETSchecker.h"

namespace ark::es2panda::checker {

ETSTypeAliasType::ETSTypeAliasType(ETSChecker *checker, util::StringView name, const ir::AstNode *declNode,
                                   bool isRecursive)
    : Type(TypeFlag::ETS_TYPE_ALIAS),
      name_(name),
      declNode_(declNode),
      isRecursive_(isRecursive),
      instantiationMap_(checker->Allocator()->Adapter()),
      typeArguments_(checker->Allocator()->Adapter())
{
}

void ETSTypeAliasType::ToString(std::stringstream &ss, bool precise) const
{
    if (!isRecursive_) {
        targetType_->ToString(ss, precise);
        return;
    }

    if (precise) {
        ToAssemblerType(ss);
    } else {
        ss << name_;
    }

    if (!typeArguments_.empty()) {
        ss << compiler::Signatures::GENERIC_BEGIN;
        for (auto arg = typeArguments_.cbegin(); arg != typeArguments_.cend(); ++arg) {
            (*arg)->ToString(ss, precise);

            if (next(arg) != typeArguments_.cend()) {
                ss << lexer::TokenToString(lexer::TokenType::PUNCTUATOR_COMMA);
            }
        }
        ss << compiler::Signatures::GENERIC_END;
    }
}

void ETSTypeAliasType::ToAssemblerType(std::stringstream &ss) const
{
    if (targetType_ == nullptr || recursionCount_) {
        ss << compiler::Signatures::BUILTIN_OBJECT;
        return;
    }

    recursionCount_ = true;
    targetType_->ToAssemblerType(ss);
    recursionCount_ = false;
}

void ETSTypeAliasType::ToAssemblerTypeWithRank(std::stringstream &ss) const
{
    if (targetType_ == nullptr || recursionCount_) {
        ss << compiler::Signatures::BUILTIN_OBJECT;
        return;
    }

    recursionCount_ = true;
    targetType_->ToAssemblerTypeWithRank(ss);
    recursionCount_ = false;
}

void ETSTypeAliasType::ToDebugInfoType(std::stringstream &ss) const
{
    if (isRecursive_) {
        ss << name_;
        return;
    }

    targetType_->ToDebugInfoType(ss);
}

bool ETSTypeAliasType::IsArgumentsIdentical(TypeRelation *relation, Type *other)
{
    auto const otherTypeArguments = other->AsETSTypeAliasType()->typeArguments_;

    auto const argsNumber = typeArguments_.size();
    if (argsNumber != otherTypeArguments.size()) {
        return false;
    }

    for (size_t idx = 0U; idx < argsNumber; ++idx) {
        if (typeArguments_[idx]->IsWildcardType() || otherTypeArguments[idx]->IsWildcardType()) {
            continue;
        }
        if (!relation->IsIdenticalTo(typeArguments_[idx], otherTypeArguments[idx])) {
            return false;
        }
    }

    return true;
}

void ETSTypeAliasType::Identical(TypeRelation *relation, Type *other)
{
    if (other->IsETSTypeAliasType() && other->AsETSTypeAliasType()->IsRecursive() && isRecursive_) {
        if (other->AsETSTypeAliasType()->GetDeclNode() == this->declNode_) {
            relation->Result(IsArgumentsIdentical(relation, other));
            return;
        }
    }

    if (targetType_ != nullptr) {
        relation->IsIdenticalTo(other, targetType_);
    }
}

void ETSTypeAliasType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    if (source->IsETSTypeAliasType()) {
        relation->IsIdenticalTo(this, source);
    }

    if (!relation->IsTrue() && relation->IsAtTypeDepthLimit(GetBaseType()) && targetType_ != nullptr) {
        relation->IncreaseTypeRecursionCount(GetBaseType());
        relation->IsAssignableTo(source, targetType_);
        relation->DecreaseTypeRecursionCount(GetBaseType());
    }
}

bool ETSTypeAliasType::AssignmentSource(TypeRelation *relation, Type *target)
{
    if (target->IsETSTypeAliasType()) {
        relation->IsIdenticalTo(target, this);
    }

    if (!relation->IsTrue() && relation->IsAtTypeDepthLimit(GetBaseType()) && targetType_ != nullptr) {
        relation->IncreaseTypeRecursionCount(GetBaseType());
        relation->IsAssignableTo(targetType_, target);
        relation->DecreaseTypeRecursionCount(GetBaseType());
    }

    return relation->IsTrue();
}

void ETSTypeAliasType::Cast(TypeRelation *const relation, Type *const target)
{
    if (target->IsETSTypeAliasType()) {
        relation->IsIdenticalTo(this, target);
    }

    if (!relation->IsTrue() && relation->IsAtTypeDepthLimit(GetBaseType())) {
        relation->IncreaseTypeRecursionCount(GetBaseType());
        targetType_->Cast(relation, target);
        relation->DecreaseTypeRecursionCount(GetBaseType());
    }
}

void ETSTypeAliasType::CastTarget(TypeRelation *relation, Type *source)
{
    if (source->IsETSTypeAliasType()) {
        relation->IsIdenticalTo(this, source);
    }

    if (!relation->IsTrue() && relation->IsAtTypeDepthLimit(GetBaseType())) {
        relation->IncreaseTypeRecursionCount(GetBaseType());
        targetType_->CastTarget(relation, source);
        relation->DecreaseTypeRecursionCount(GetBaseType());
    }
}

void ETSTypeAliasType::IsSupertypeOf(TypeRelation *relation, Type *source)
{
    if (source->IsETSTypeAliasType()) {
        relation->IsIdenticalTo(this, source);
    }

    if (!relation->IsTrue() && relation->IsAtTypeDepthLimit(GetBaseType()) && targetType_ != nullptr) {
        relation->IncreaseTypeRecursionCount(GetBaseType());
        relation->IsSupertypeOf(targetType_, source);
        relation->DecreaseTypeRecursionCount(GetBaseType());
    }
}

void ETSTypeAliasType::IsSubtypeOf(TypeRelation *relation, Type *target)
{
    if (target->IsETSTypeAliasType()) {
        relation->IsIdenticalTo(this, target);
    }

    if (!relation->IsTrue() && relation->IsAtTypeDepthLimit(GetBaseType()) && targetType_ != nullptr) {
        relation->IncreaseTypeRecursionCount(GetBaseType());
        relation->IsSupertypeOf(target, targetType_);
        relation->DecreaseTypeRecursionCount(GetBaseType());
    }
}

uint32_t ETSTypeAliasType::Rank() const
{
    if (isRecursive_) {
        return 0;
    }

    return targetType_->Rank();
}

Type *ETSTypeAliasType::Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes)
{
    return targetType_->Instantiate(allocator, relation, globalTypes);
}

ETSTypeAliasType *ETSTypeAliasType::GetInstantiatedType(util::StringView hash)
{
    auto &instantiationMap = base_ == nullptr ? instantiationMap_ : base_->instantiationMap_;

    auto found = instantiationMap.find(hash);
    if (found != instantiationMap.end()) {
        return found->second;
    }

    return nullptr;
}

void ETSTypeAliasType::EmplaceInstantiatedType(util::StringView hash, ETSTypeAliasType *emplaceType)
{
    auto &instantiationMap = base_ == nullptr ? instantiationMap_ : base_->instantiationMap_;

    instantiationMap.try_emplace(hash, emplaceType);
}

bool ETSTypeAliasType::SubstituteTypeArgs(TypeRelation *const relation, ArenaVector<Type *> &newTypeArgs,
                                          const Substitution *const substitution)
{
    bool anyChange = false;
    newTypeArgs.reserve(typeArguments_.size());

    for (auto *const arg : typeArguments_) {
        auto *const newArg = arg->Substitute(relation, substitution);
        ES2PANDA_ASSERT(newArg->IsETSReferenceType());
        newTypeArgs.emplace_back(newArg);
        anyChange = anyChange || (newArg != arg);
    }

    return anyChange;
}

void ETSTypeAliasType::SetTypeArguments(ArenaVector<Type *> typeArguments)
{
    typeArguments_ = std::move(typeArguments);
}

Type *ETSTypeAliasType::Substitute(TypeRelation *relation, const Substitution *substitution)
{
    if (substitution == nullptr || substitution->empty()) {
        return this;
    }

    auto *const checker = relation->GetChecker()->AsETSChecker();

    ArenaVector<Type *> newTypeArgs {checker->Allocator()->Adapter()};

    if (!SubstituteTypeArgs(relation, newTypeArgs, substitution)) {
        return this;
    }

    const util::StringView hash = checker->GetHashFromTypeArguments(newTypeArgs);

    ETSTypeAliasType *copiedType = GetInstantiatedType(hash);
    if (copiedType != nullptr) {
        return copiedType;
    }

    auto arenaSubst = checker->NewArenaSubstitution();
    std::copy(substitution->begin(), substitution->end(), std::inserter(*arenaSubst, arenaSubst->end()));

    copiedType = checker->CreateETSTypeAliasType(name_, declNode_, isRecursive_);
    copiedType->base_ = base_ == nullptr ? this : base_;
    copiedType->parent_ = this;
    copiedType->substitution_ = arenaSubst;
    copiedType->typeArguments_ = newTypeArgs;

    EmplaceInstantiatedType(hash, copiedType);

    if (targetType_ != nullptr) {
        copiedType->SetTargetType(targetType_->Substitute(relation, substitution));
    }

    return copiedType;
}

void ETSTypeAliasType::Iterate(const TypeTraverser &func) const
{
    func(targetType_);

    for (auto const *const type : typeArguments_) {
        func(type);
    }
}

}  // namespace ark::es2panda::checker
