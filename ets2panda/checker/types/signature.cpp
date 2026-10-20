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

#include "signature.h"

#include "checker/ETSchecker.h"
#include "ets/etsObjectType.h"
#include "ets/etsTypeParameter.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::checker {

util::StringView Signature::InternalName() const
{
    return internalName_.Empty() && func_ != nullptr ? func_->Scope()->InternalName() : internalName_;
}

struct ParamPlan {
    Type *result {};                // wholesale-replaced or polymorphically substituted entry
    ETSTypeParameter *original {};  // type parameter that stays generic in the copy
    ETSTypeParameter *clone {};     // clone of 'original', if it was re-parameterized
};

static void FinalizeClonedParameters(TypeRelation *relation, const ArenaVector<ParamPlan> *plans,
                                     const Substitution *combinedSubstitution, bool substituteDefault)
{
    for (auto &plan : *plans) {
        if (plan.clone == nullptr) {
            continue;
        }
        auto *const constraint = plan.original->GetConstraintType();
        if (constraint != nullptr) {
            plan.clone->SetConstraintType(constraint->Substitute(relation, combinedSubstitution));
        }
        auto *const defaultType = substituteDefault ? plan.original->GetDefaultType() : nullptr;
        if (defaultType != nullptr) {
            plan.clone->SetDefaultType(defaultType->Substitute(relation, combinedSubstitution));
        }
    }
}

static bool CloneDependentTypeParameters(TypeRelation *relation, ArenaVector<ParamPlan> *plans,
                                         const Substitution *substitution, Substitution *combinedSubstitution,
                                         bool substituteDefault)
{
    auto *const checker = relation->GetChecker()->AsETSChecker();
    auto propertiesChanged = [relation, substituteDefault](ETSTypeParameter *tpar, const Substitution *subst) {
        auto *const constraint = tpar->GetConstraintType();
        if (constraint != nullptr && constraint->Substitute(relation, subst) != constraint) {
            return true;
        }
        if (!substituteDefault) {
            return false;
        }
        auto *const defaultType = tpar->GetDefaultType();
        return defaultType != nullptr && defaultType->IsETSTypeParameter() &&
               defaultType->Substitute(relation, subst) != defaultType;
    };

    bool clonedAny = false;
    bool newClone = true;
    while (newClone) {
        newClone = false;
        for (auto &plan : *plans) {
            if (plan.original == nullptr || plan.clone != nullptr) {
                continue;
            }
            auto *const defaultType = plan.original->GetDefaultType();
            auto *const constraint = defaultType != nullptr ? plan.original->GetConstraintType() : nullptr;
            if (defaultType != nullptr &&
                (!substituteDefault ||
                 (constraint != nullptr && constraint->Substitute(relation, substitution) != constraint))) {
                continue;
            }
            const auto *const subst = clonedAny ? combinedSubstitution : substitution;
            if (!propertiesChanged(plan.original, subst)) {
                continue;
            }
            if (!clonedAny) {
                *combinedSubstitution = *substitution;
            }
            plan.clone = plan.original->Clone(checker)->AsETSTypeParameter();
            checker->EmplaceSubstituted(combinedSubstitution, plan.original->GetOriginal(), plan.clone);
            clonedAny = true;
            newClone = true;
        }
    }

    FinalizeClonedParameters(relation, plans, combinedSubstitution, substituteDefault);
    return clonedAny;
}

static std::pair<SignatureInfo *, bool> SubstituteTypeParameters(Signature *signature, TypeRelation *relation,
                                                                 const Substitution *substitution,
                                                                 Substitution *combinedSubstitution)
{
    auto *const checker = relation->GetChecker()->AsETSChecker();
    auto *const signatureInfo = signature->GetSignatureInfo();
    auto *const newSigInfo = checker->ProgramAllocator()->New<SignatureInfo>(checker->ProgramAllocator());
    ES2PANDA_ASSERT(newSigInfo != nullptr);
    bool anyChange = false;

    auto plans = ArenaVector<ParamPlan> {checker->ProgramAllocator()->Adapter()};

    for (auto *typeParam : signatureInfo->typeParams) {
        // A previously substituted signature may hold non-type-parameter entries here;
        // substitute them polymorphically, as before.
        if (!typeParam->IsETSTypeParameter()) {
            auto *substitutedEntry = typeParam->Substitute(relation, substitution);
            anyChange |= (substitutedEntry != typeParam);
            plans.push_back(ParamPlan {substitutedEntry, nullptr, nullptr});
            continue;
        }

        auto *const tpar = typeParam->AsETSTypeParameter();
        auto *const substitutedParam = tpar->Substitute(relation, substitution);

        if (substitutedParam != typeParam) {
            plans.push_back(ParamPlan {substitutedParam, nullptr, nullptr});
            anyChange = true;
            continue;
        }

        plans.push_back(ParamPlan {nullptr, tpar, nullptr});
    }

    const bool substituteDefault = signatureInfo->typeParams.size() == 1U && signatureInfo->restVar == nullptr &&
                                   signatureInfo->params.empty() &&
                                   signature->ReturnType() == signatureInfo->typeParams.front();

    anyChange |= CloneDependentTypeParameters(relation, &plans, substitution, combinedSubstitution, substituteDefault);

    for (auto &plan : plans) {
        newSigInfo->typeParams.push_back(
            plan.clone != nullptr ? plan.clone : (plan.original != nullptr ? plan.original : plan.result));
    }

    return {newSigInfo, anyChange};
}

Signature *Signature::Substitute(TypeRelation *relation, const Substitution *substitution)
{
    if (substitution == nullptr || substitution->empty()) {
        return this;
    }
    auto *checker = relation->GetChecker()->AsETSChecker();
    auto *allocator = checker->ProgramAllocator();
    Substitution combinedSubstitution {};
    auto [newSigInfo, anyChange] = SubstituteTypeParameters(this, relation, substitution, &combinedSubstitution);
    auto *effectiveSubstitution = combinedSubstitution.empty() ? substitution : &combinedSubstitution;
    newSigInfo->minArgCount = signatureInfo_->minArgCount;

    for (auto *param : signatureInfo_->params) {
        auto *newParam = param;
        auto *newParamType = param->TsType()->Substitute(relation, effectiveSubstitution);
        if (newParamType != param->TsType()) {
            anyChange = true;
            newParam = param->Copy(allocator, param->Declaration());
            newParam->SetTsType(newParamType);
        }
        newSigInfo->params.push_back(newParam);
    }

    if (signatureInfo_->restVar != nullptr) {
        auto *newRestVar = &*signatureInfo_->restVar;
        auto *newRestType = signatureInfo_->restVar->TsType()->Substitute(relation, effectiveSubstitution);
        if (newRestType != signatureInfo_->restVar->TsType()) {
            anyChange = true;
            newRestVar = signatureInfo_->restVar->Copy(allocator, signatureInfo_->restVar->Declaration());
            newRestVar->SetTsType(newRestType);
        }
        newSigInfo->restVar = newRestVar;
    }

    if (!anyChange) {
        newSigInfo = signatureInfo_;
    }

    auto *newReturnType = returnType_->Substitute(relation, effectiveSubstitution);
    if (newReturnType == returnType_ && !anyChange) {
        return this;
    }

    return CreateSignatureForSubstitute(allocator, newSigInfo, newReturnType);
}

Signature *Signature::CreateSignatureForSubstitute(ArenaAllocator *allocator, SignatureInfo *sigInfo, Type *returnType)
{
    auto *result = allocator->New<Signature>(sigInfo, returnType, func_);
    ES2PANDA_ASSERT(result != nullptr);
    result->flags_ = flags_;
    result->internalName_ = internalName_;
    result->ownerObj_ = ownerObj_;
    result->ownerVar_ = ownerVar_;
    return result;
}

void Signature::ToAssemblerType(std::stringstream &ss) const
{
    ss << compiler::Signatures::MANGLE_BEGIN;

    for (const auto *param : signatureInfo_->params) {
        param->TsType()->ToAssemblerTypeWithRank(ss);
        ss << compiler::Signatures::MANGLE_SEPARATOR;
    }

    if (signatureInfo_->restVar != nullptr) {
        signatureInfo_->restVar->TsType()->ToAssemblerTypeWithRank(ss);
        ss << compiler::Signatures::MANGLE_SEPARATOR;
    }

    returnType_->ToAssemblerTypeWithRank(ss);
    ss << compiler::Signatures::MANGLE_SEPARATOR;
}

Signature *Signature::Copy(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) const
{
    SignatureInfo *copiedInfo = allocator->New<SignatureInfo>(signatureInfo_, allocator);
    ES2PANDA_ASSERT(copiedInfo != nullptr);
    for (size_t idx = 0U; idx < signatureInfo_->params.size(); ++idx) {
        auto *const paramType = signatureInfo_->params[idx]->TsType();
        if (paramType->HasTypeFlag(TypeFlag::GENERIC) && paramType->IsETSObjectType()) {
            copiedInfo->params[idx]->SetTsType(paramType->Instantiate(allocator, relation, globalTypes));
            auto originalTypeArgs = paramType->AsETSObjectType()->GetOriginalBaseType()->TypeArguments();
            copiedInfo->params[idx]->TsType()->AsETSObjectType()->SetTypeArguments(std::move(originalTypeArgs));
        } else {
            copiedInfo->params[idx]->SetTsType(
                ETSChecker::TryToInstantiate(paramType, allocator, relation, globalTypes));
        }
    }

    auto *const copiedSignature = allocator->New<Signature>(copiedInfo, returnType_, func_);
    ES2PANDA_ASSERT(copiedSignature != nullptr);
    copiedSignature->flags_ = flags_;
    copiedSignature->internalName_ = internalName_;
    copiedSignature->ownerObj_ = ownerObj_;
    copiedSignature->ownerVar_ = ownerVar_;

    return copiedSignature;
}

void Signature::ToString(std::stringstream &ss, const varbinder::Variable *variable, bool printAsMethod,
                         bool precise) const
{
    if (!signatureInfo_->typeParams.empty()) {
        ss << "<";
        for (auto it = signatureInfo_->typeParams.begin(); it != signatureInfo_->typeParams.end(); ++it) {
            (*it)->ToString(ss, precise);
            if (std::next(it) != signatureInfo_->typeParams.end()) {
                ss << ", ";
            }
        }
        ss << ">";
    }

    ss << "(";
    size_t pidx = 1;

    for (auto it = signatureInfo_->params.begin(); it != signatureInfo_->params.end(); it++) {
        ss << (!this->HasFunction()
                   ? "p" + std::to_string(pidx++)
                   : (!(*it)->Name().StartsWith(compiler::GENSYM_CORE) ? (*it)->Name().Utf8() : compiler::DUMMY_ID));

        if ((*it)->HasFlag(varbinder::VariableFlags::OPTIONAL)) {
            ss << "?";
        }

        ss << ": ";

        (*it)->TsType()->ToString(ss, precise);

        if (std::next(it) != signatureInfo_->params.end()) {
            ss << ", ";
        }
    }

    if (signatureInfo_->restVar != nullptr) {
        if (!signatureInfo_->params.empty()) {
            ss << ", ";
        }

        ss << "...";
        ss << (!this->HasFunction() ? "p" + std::to_string(pidx++)
               : !signatureInfo_->restVar->Name().StartsWith(compiler::GENSYM_CORE)
                   ? signatureInfo_->restVar->Name().Utf8()
                   : compiler::DUMMY_ID);
        ss << ": ";
        signatureInfo_->restVar->TsType()->ToString(ss, precise);
    }

    ss << ")";

    if (printAsMethod || (variable != nullptr && variable->HasFlag(varbinder::VariableFlags::METHOD))) {
        ss << ": ";
    } else {
        ss << " => ";
    }

    returnType_->ToString(ss, precise);
}

std::string Signature::ToString() const
{
    std::stringstream ss;
    ToString(ss, nullptr);
    return ss.str();
}

// this function intentionally ignores relation->Result
static bool MethodSignaturesAreCompatible(TypeRelation *relation, bool checkIdentical, Signature const *super,
                                          Signature const *sub)
{
    if (sub->ArgCount() != super->ArgCount()) {
        return false;
    }

    if (sub->HasRestParameter() != super->HasRestParameter()) {
        return false;
    }

    auto const areCompatible = [relation, checkIdentical](Type const *superT, Type const *subT) -> bool {
        return checkIdentical ? relation->IsIdenticalTo(superT, subT) : relation->IsSupertypeOf(superT, subT);
    };
    if (!relation->NoReturnTypeCheck() && !areCompatible(super->ReturnType(), sub->ReturnType())) {
        return false;
    }

    for (size_t idx = 0; idx < sub->ArgCount(); ++idx) {
        if (!areCompatible(sub->Params()[idx]->TsType(), super->Params()[idx]->TsType())) {
            return false;
        }
    }

    return !sub->HasRestParameter() || relation->IsIdenticalTo(sub->RestVar()->TsType(), super->RestVar()->TsType());
}

void Signature::IsSubtypeOf(TypeRelation *relation, Signature const *super) const
{
    auto sub = this;
    if (sub->HasSignatureFlag(SignatureFlags::GETTER_OR_SETTER) !=
        super->HasSignatureFlag(SignatureFlags::GETTER_OR_SETTER)) {
        relation->Result(false);
        return;
    }

    if (!relation->GetChecker()->IsETSChecker() && sub->MinArgCount() != super->MinArgCount() &&
        sub->RestVar() == nullptr && super->RestVar() == nullptr) {
        relation->Result(false);
        return;
    }

    relation->Result(MethodSignaturesAreCompatible(relation, !relation->IsOverridingCheck(), super, sub));
}

static bool IsCoveredByDetailedCheck(TypeRelation *relation, Signature *super, Signature *sub)
{
    auto *checker = relation->GetChecker()->AsETSChecker();
    SavedTypeRelationFlagsContext trCtx(relation, TypeRelationFlag::IN_ASSIGNMENT_CONTEXT);

    auto const checkCompatibility = [relation](Type *subT, Type *superT) -> bool {
        if (subT->IsTypeError() || superT->IsTypeError()) {
            return false;
        }

        if (subT == superT) {
            return true;
        }

        if (relation->IsAssignableTo(superT, subT)) {
            return true;
        }

        return (relation->IsLegalBoxedPrimitiveConversion(subT, superT));
    };

    if (!relation->NoReturnTypeCheck()) {
        if (!checkCompatibility(super->ReturnType(), sub->ReturnType())) {
            return false;
        }
    }

    size_t const argsToProcess =
        std::max(sub->ArgCount(), super->ArgCount()) + (sub->HasRestParameter() || super->HasRestParameter());

    for (size_t idx = 0; idx < argsToProcess; ++idx) {
        Type *subParamType = nullptr;
        Type *superParamType = nullptr;

        if (idx < sub->ArgCount()) {
            subParamType = sub->Params()[idx]->TsType();
        } else if (sub->HasRestParameter() && util::Helpers::IsArrayType(sub->RestVar()->TsType())) {
            subParamType = checker->GetElementTypeOfArray(sub->RestVar()->TsType());
        } else {
            return false;
        }

        if (idx < super->ArgCount()) {
            superParamType = super->Params()[idx]->TsType();
        } else if (super->HasRestParameter() && util::Helpers::IsArrayType(super->RestVar()->TsType())) {
            superParamType = checker->GetElementTypeOfArray(super->RestVar()->TsType());
        } else {
            break;
        }

        if (!checkCompatibility(subParamType, superParamType)) {
            return false;
        }
    }

    return true;
}

void Signature::IsCoveredBy(TypeRelation *relation, Signature *super)
{
    auto *sub = this;

    if (sub->HasSignatureFlag(SignatureFlags::GETTER_OR_SETTER) !=
        super->HasSignatureFlag(SignatureFlags::GETTER_OR_SETTER)) {
        relation->Result(false);
        return;
    }

    if (sub->GetSignatureInfo()->typeParams.size() != super->GetSignatureInfo()->typeParams.size()) {
        relation->Result(false);
        return;
    }

    if (sub->MinArgCount() > super->MinArgCount()) {
        relation->Result(false);
        return;
    }

    if (super->HasRestParameter() && !sub->HasRestParameter()) {
        relation->Result(false);
        return;
    }

    relation->Result(IsCoveredByDetailedCheck(relation, super, sub));
}

// This function is not used by ets extension
void Signature::AssignmentTarget([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Signature *source)
{
    if (signatureInfo_->restVar == nullptr && source->MinArgCount() > signatureInfo_->params.size()) {
        relation->Result(false);
        return;
    }

    for (size_t i = 0; i < source->Params().size(); i++) {
        if (signatureInfo_->restVar == nullptr && i >= Params().size()) {
            break;
        }

        if (signatureInfo_->restVar != nullptr) {
            relation->IsAssignableTo(source->Params()[i]->TsType(), signatureInfo_->restVar->TsType());

            if (!relation->IsTrue()) {
                return;
            }

            continue;
        }

        relation->IsAssignableTo(source->Params()[i]->TsType(), Params()[i]->TsType());

        if (!relation->IsTrue()) {
            return;
        }
    }

    relation->IsAssignableTo(source->ReturnType(), returnType_);

    if (relation->IsTrue() && signatureInfo_->restVar != nullptr && source->RestVar() != nullptr) {
        relation->IsAssignableTo(source->RestVar()->TsType(), signatureInfo_->restVar->TsType());
    }
}

Signature *Signature::ToArrowSignature(ETSChecker *checker)
{
    auto *allocator = checker->ProgramAllocator();
    auto *sigInfo = allocator->New<SignatureInfo>(signatureInfo_, allocator);
    ES2PANDA_ASSERT(sigInfo != nullptr);
    Type *retType = returnType_;

    auto *resultSig = allocator->New<Signature>(sigInfo, retType);
    ES2PANDA_ASSERT(resultSig != nullptr);
    resultSig->flags_ = flags_;
    // Whatever the original function. now we already know the return type.
    resultSig->flags_ &= ~SignatureFlags::NEED_RETURN_TYPE;
    resultSig->SetOwner(Owner());
    resultSig->SetOwnerVar(OwnerVar());
    ES2PANDA_ASSERT(!resultSig->HasFunction());
    return resultSig;
}

}  // namespace ark::es2panda::checker
