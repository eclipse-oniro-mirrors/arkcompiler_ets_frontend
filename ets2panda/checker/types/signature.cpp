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

#include "signature.h"

#include "checker/ETSchecker.h"
#include "ets/etsObjectType.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::checker {

util::StringView Signature::InternalName() const
{
    return internalName_.Empty() && func_ != nullptr ? func_->Scope()->InternalName() : internalName_;
}

Signature *Signature::Substitute(TypeRelation *relation, const Substitution *substitution)
{
    if (substitution == nullptr || substitution->empty()) {
        return this;
    }
    auto *checker = relation->GetChecker()->AsETSChecker();
    auto *allocator = checker->ProgramAllocator();
    bool anyChange = false;
    SignatureInfo *newSigInfo = allocator->New<SignatureInfo>(allocator);
    ES2PANDA_ASSERT(newSigInfo != nullptr);
    if (!signatureInfo_->typeParams.empty()) {
        for (auto *tparam : signatureInfo_->typeParams) {
            auto *newTparam = tparam->Substitute(relation, substitution);
            newSigInfo->typeParams.push_back(newTparam);
            anyChange |= (newTparam != tparam);
        }
    }
    newSigInfo->minArgCount = signatureInfo_->minArgCount;

    for (auto *param : signatureInfo_->params) {
        auto *newParam = param;
        auto *newParamType = param->TsType()->Substitute(relation, substitution);
        if (newParamType != param->TsType()) {
            anyChange = true;
            newParam = param->Copy(allocator, param->Declaration());
            if (newParamType->IsETSVoidType()) {
                // since `void` is not allowed to be used as param type
                newParamType = checker->GlobalETSUndefinedType();
            }
            newParam->SetTsType(newParamType);
        }
        newSigInfo->params.push_back(newParam);
    }

    if (signatureInfo_->restVar != nullptr) {
        auto *newRestVar = signatureInfo_->restVar;
        auto *newRestType = signatureInfo_->restVar->TsType()->Substitute(relation, substitution);
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

    auto *newReturnType = returnType_->Substitute(relation, substitution);
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

Signature *Signature::Copy(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes)
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
static bool MethodSignaturesAreCompatible(TypeRelation *relation, bool checkIdentical, Signature *const super,
                                          Signature *const sub)
{
    if (sub->ArgCount() != super->ArgCount()) {
        return false;
    }

    if (sub->HasRestParameter() != super->HasRestParameter()) {
        return false;
    }

    auto const areCompatible = [relation, checkIdentical](Type *superT, Type *subT) {
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

void Signature::IsSubtypeOf(TypeRelation *relation, Signature *super)
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
    for (auto param : sigInfo->params) {
        param->SetTsType(checker->MaybeBoxType(param->TsType()));
    }
    auto *retType = checker->MaybeBoxType(returnType_);

    auto *resultSig = allocator->New<Signature>(sigInfo, retType);
    ES2PANDA_ASSERT(resultSig != nullptr);
    resultSig->flags_ = flags_;
    resultSig->SetOwner(Owner());
    resultSig->SetOwnerVar(OwnerVar());
    ES2PANDA_ASSERT(!resultSig->HasFunction());
    return resultSig;
}

}  // namespace ark::es2panda::checker
