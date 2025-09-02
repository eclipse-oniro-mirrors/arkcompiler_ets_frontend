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

#include "checker/ETSchecker.h"
#include "checker/types/globalTypesHolder.h"
#include "checker/types/typeError.h"
#include "compiler/lowering/phase.h"

namespace ark::es2panda::checker {

ETSFunctionType::ETSFunctionType([[maybe_unused]] ETSChecker *checker, util::StringView name,
                                 ArenaVector<Signature *> &&signatures)
    : Type(TypeFlag::FUNCTION | TypeFlag::ETS_METHOD),
      callSignatures_(std::move(signatures)),
      extensionFunctionSigs_(ArenaVector<Signature *>(checker->ProgramAllocator()->Adapter())),
      extensionAccessorSigs_(ArenaVector<Signature *>(checker->ProgramAllocator()->Adapter())),
      name_(name)
{
    auto flag = TypeFlag::NONE;
    for (auto *sig : callSignatures_) {
        flag |= sig->HasSignatureFlag(SignatureFlags::GETTER) ? TypeFlag::GETTER : TypeFlag::NONE;
        flag |= sig->HasSignatureFlag(SignatureFlags::SETTER) ? TypeFlag::SETTER : TypeFlag::NONE;
        if (sig->IsExtensionAccessor()) {
            extensionAccessorSigs_.emplace_back(sig);
        } else if (sig->IsExtensionFunction()) {
            extensionFunctionSigs_.emplace_back(sig);
        }
    }
    AddTypeFlag(flag);
}

ETSFunctionType::ETSFunctionType(ETSChecker *checker, Signature *signature)
    : Type(TypeFlag::FUNCTION),
      callSignatures_({{signature->ToArrowSignature(checker)}, checker->ProgramAllocator()->Adapter()}),
      extensionFunctionSigs_(ArenaVector<Signature *>(checker->ProgramAllocator()->Adapter())),
      extensionAccessorSigs_(ArenaVector<Signature *>(checker->ProgramAllocator()->Adapter())),
      name_(""),
      assemblerName_(checker->GlobalBuiltinFunctionType(signature->MinArgCount(), signature->HasRestParameter()) !=
                             nullptr
                         ? checker->GlobalBuiltinFunctionType(signature->MinArgCount(), signature->HasRestParameter())
                               ->AsETSObjectType()
                               ->AssemblerName()
                         : "")
{
}

// #22951: proper this type implementation
static void HackThisParameterInExtensionFunctionInvoke(ETSObjectType *interface, std::string &invokeName)
{
    auto *property = interface->AsETSObjectType()->GetOwnProperty<checker::PropertyType::INSTANCE_METHOD>(
        util::StringView(invokeName));
    ES2PANDA_ASSERT(property != nullptr);
    auto *tsType = property->TsType();
    ES2PANDA_ASSERT(tsType != nullptr);
    auto &callSigsOfInvoke0 = tsType->AsETSFunctionType()->CallSignatures();
    for (auto sig : callSigsOfInvoke0) {
        sig->AddSignatureFlag(SignatureFlags::THIS_RETURN_TYPE);
    }
}

static ETSObjectType *FunctionTypeToFunctionalInterfaceType(ETSChecker *checker, Signature *signature, size_t arity)
{
    // #22951: remove flags that affect ETSObjectType::Substitute
    bool isExtensionHack = signature->HasSignatureFlag(SignatureFlags::EXTENSION_FUNCTION);

    if (signature->RestVar() != nullptr) {
        auto sigParamsSize = signature->Params().size();
        auto nPosParams = arity < sigParamsSize ? arity : sigParamsSize;
        auto *functionN = checker->GlobalBuiltinFunctionType(nPosParams, true);

        if (nPosParams >= checker->GetGlobalTypesHolder()->VariadicFunctionTypeThreshold()) {
            return functionN;
        }

        auto substitution = Substitution {};
        ES2PANDA_ASSERT(functionN != nullptr && nPosParams <= functionN->TypeArguments().size());
        for (size_t i = 0; i < nPosParams; i++) {
            substitution.emplace(functionN->TypeArguments()[i]->AsETSTypeParameter(),
                                 checker->MaybeBoxType(signature->Params()[i]->TsType()));
        }
        auto *elementType = !signature->RestVar()->TsType()->IsETSTupleType()
                                ? checker->GetElementTypeOfArray(signature->RestVar()->TsType())
                                : checker->GlobalETSAnyType();
        substitution.emplace(functionN->TypeArguments()[nPosParams]->AsETSTypeParameter(),
                             checker->MaybeBoxType(elementType));
        substitution.emplace(functionN->TypeArguments()[nPosParams + 1]->AsETSTypeParameter(),
                             checker->MaybeBoxType(signature->ReturnType()));
        auto result = functionN->Substitute(checker->Relation(), &substitution, true, isExtensionHack);
        result->AddObjectFlag(checker::ETSObjectFlags::FUNCTIONAL);
        return result;
    }

    ES2PANDA_ASSERT(arity >= signature->MinArgCount() && arity <= signature->ArgCount());

    auto *funcIface = checker->GlobalBuiltinFunctionType(arity, false);
    if (arity >= checker->GetGlobalTypesHolder()->VariadicFunctionTypeThreshold()) {
        return funcIface;
    }

    auto substitution = Substitution {};
    for (size_t i = 0; i < arity; i++) {
        substitution.emplace(funcIface->TypeArguments()[i]->AsETSTypeParameter(),
                             checker->MaybeBoxType(signature->Params()[i]->TsType()));
    }
    substitution.emplace(funcIface->TypeArguments()[arity]->AsETSTypeParameter(),
                         checker->MaybeBoxType(signature->ReturnType()));
    auto result = funcIface->Substitute(checker->Relation(), &substitution, true, isExtensionHack);

    if (signature->HasSignatureFlag(SignatureFlags::THIS_RETURN_TYPE)) {
        auto invokeName = checker->FunctionalInterfaceInvokeName(arity, false);
        HackThisParameterInExtensionFunctionInvoke(result, invokeName);
    }

    result->AddObjectFlag(checker::ETSObjectFlags::FUNCTIONAL);
    return result;
}

ETSObjectType *ETSFunctionType::ArrowToFunctionalInterface(ETSChecker *checker)
{
    auto &cached = compiler::GetPhaseManager()->Context()->GetChecker()->AsETSChecker()->GetArrowToFuncInterfaces();

    auto found = cached.find(this);
    if (LIKELY(found != cached.end())) {
        return found->second;
    }
    return cached
        .emplace(this,
                 FunctionTypeToFunctionalInterfaceType(checker, ArrowSignature(), ArrowSignature()->MinArgCount()))
        .first->second;
}

ETSObjectType *ETSFunctionType::ArrowToFunctionalInterfaceDesiredArity(ETSChecker *checker, size_t arity)
{
    if (LIKELY(arity == ArrowSignature()->MinArgCount())) {
        return ArrowToFunctionalInterface(checker);
    }
    return FunctionTypeToFunctionalInterfaceType(checker, ArrowSignature(), arity);
}

ETSFunctionType *ETSFunctionType::MethodToArrow(ETSChecker *checker)
{
    auto &cached = compiler::GetPhaseManager()->Context()->GetChecker()->AsETSChecker()->GetInvokeToArrowSignatures();

    auto found = cached.find(this);
    if (LIKELY(found != cached.end())) {
        return found->second;
    }

    ERROR_SANITY_CHECK(checker, !IsETSArrowType() && CallSignatures().size() == 1, return nullptr);
    return cached.emplace(this, checker->CreateETSArrowType(CallSignatures()[0])).first->second;
}

void ETSFunctionType::AddCallSignature(Signature *signature)
{
    ES2PANDA_ASSERT(!IsETSArrowType());

    if (signature->Function()->IsGetter()) {
        AddTypeFlag(TypeFlag::GETTER);
    } else if (signature->Function()->IsSetter()) {
        AddTypeFlag(TypeFlag::SETTER);
    }
    callSignatures_.push_back(signature);
}

void ETSFunctionType::ToString(std::stringstream &ss, bool precise) const
{
    callSignatures_[0]->ToString(ss, nullptr, false, precise);
}

// Method types do not participate in most type relations
static inline void AssertNoMethodsInFunctionRelation([[maybe_unused]] Type *left, [[maybe_unused]] Type *right)
{
    ES2PANDA_ASSERT(!left->IsETSMethodType());
    ES2PANDA_ASSERT(!right->IsETSMethodType());
}

static Signature *EnhanceSignatureSubstitution(TypeRelation *relation, Signature *super, Signature *sub)
{
    auto checker = relation->GetChecker()->AsETSChecker();
    auto substitution = Substitution {};

    auto const enhance = [checker, sub, &substitution](Type *param, Type *arg) {
        return checker->EnhanceSubstitutionForType(sub->GetSignatureInfo()->typeParams, param, arg, &substitution);
    };
    for (size_t ix = 0; ix < sub->ArgCount(); ix++) {
        if (!enhance(sub->GetSignatureInfo()->params[ix]->TsType(), super->GetSignatureInfo()->params[ix]->TsType())) {
            return nullptr;
        }
    }

    if (!enhance(sub->ReturnType(), super->ReturnType())) {
        return nullptr;
    }

    if (super->RestVar() != nullptr) {
        if (!enhance(sub->RestVar()->TsType(), super->RestVar()->TsType())) {
            return nullptr;
        }
    }

    return sub->Substitute(relation, &substitution);
}

static bool SignatureIsSupertypeOf(TypeRelation *relation, Signature *super, Signature *sub)
{
    if (super->MinArgCount() < sub->MinArgCount()) {
        return false;
    }
    if ((super->RestVar() != nullptr && sub->RestVar() == nullptr) ||
        (super->RestVar() == nullptr && sub->RestVar() != nullptr)) {
        return false;
    }
    if (!sub->GetSignatureInfo()->typeParams.empty()) {
        sub = EnhanceSignatureSubstitution(relation, super, sub);
        if (sub == nullptr) {
            return false;
        }
    }
    for (size_t idx = 0; idx != sub->MinArgCount(); idx++) {
        if (!relation->IsSupertypeOf(sub->Params()[idx]->TsType(), super->Params()[idx]->TsType())) {
            return false;
        }
    }
    if (super->RestVar() != nullptr && !relation->IsSupertypeOf(sub->RestVar()->TsType(), super->RestVar()->TsType())) {
        return false;
    }
    if (!relation->IsSupertypeOf(super->ReturnType(), sub->ReturnType())) {
        return false;
    }
    return true;
}

static ETSFunctionType *CoerceToArrowType(TypeRelation *relation, Type *type)
{
    if (type->IsETSFunctionType()) {
        return type->AsETSFunctionType();
    }
    if (type->IsETSObjectType() && type->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::FUNCTIONAL)) {
        auto checker = relation->GetChecker()->AsETSChecker();
        return type->AsETSObjectType()->GetFunctionalInterfaceInvokeType()->MethodToArrow(checker);
    }
    return nullptr;
}

void ETSFunctionType::Identical(TypeRelation *relation, Type *other)
{
    AssertNoMethodsInFunctionRelation(this, other);

    ETSFunctionType *const otherFnType = CoerceToArrowType(relation, other);
    if (otherFnType == nullptr) {
        relation->Result(false);
        return;
    }

    relation->Result(SignatureIsSupertypeOf(relation, this->ArrowSignature(), otherFnType->ArrowSignature()) &&
                     SignatureIsSupertypeOf(relation, otherFnType->ArrowSignature(), this->ArrowSignature()));
}

bool ETSFunctionType::AssignmentSource(TypeRelation *relation, Type *target)
{
    AssertNoMethodsInFunctionRelation(this, target);

    return relation->IsSupertypeOf(target, this);
}

void ETSFunctionType::IsSupertypeOf(TypeRelation *relation, Type *source)
{
    AssertNoMethodsInFunctionRelation(this, source);

    ETSFunctionType *const sourceFnType = CoerceToArrowType(relation, source);
    if (sourceFnType == nullptr) {
        relation->Result(false);
        return;
    }

    relation->Result(SignatureIsSupertypeOf(relation, this->ArrowSignature(), sourceFnType->ArrowSignature()));
}

void ETSFunctionType::AssignmentTarget(TypeRelation *relation, Type *source)
{
    AssertNoMethodsInFunctionRelation(this, source);

    relation->IsSupertypeOf(this, source);
}

Type *ETSFunctionType::Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes)
{
    auto checker = relation->GetChecker()->AsETSChecker();
    if (IsETSArrowType()) {
        return allocator->New<ETSFunctionType>(checker, ArrowSignature());
    }

    auto signatures = ArenaVector<Signature *>(allocator->Adapter());
    for (auto *const signature : callSignatures_) {
        signatures.emplace_back(signature->Copy(allocator, relation, globalTypes));
    }
    return allocator->New<ETSFunctionType>(checker, name_, std::move(signatures));
}

ETSFunctionType *ETSFunctionType::Substitute(TypeRelation *relation, const Substitution *substitution)
{
    if (substitution != nullptr && !substitution->empty()) {
        auto *const checker = relation->GetChecker()->AsETSChecker();
        auto *const allocator = checker->ProgramAllocator();

        auto signatures = ArenaVector<Signature *>(allocator->Adapter());
        bool anyChange = false;

        for (auto *const signature : callSignatures_) {
            auto *newSignature = signature->Substitute(relation, substitution);
            anyChange |= newSignature != signature;
            signatures.emplace_back(newSignature);
        }

        if (anyChange) {
            if (IsETSArrowType()) {
                return allocator->New<ETSFunctionType>(checker, signatures[0]);
            }
            return allocator->New<ETSFunctionType>(checker, name_, std::move(signatures));
        }
    }

    return this;
}

void ETSFunctionType::Cast(TypeRelation *relation, Type *target)
{
    AssertNoMethodsInFunctionRelation(this, target);

    if (target->IsETSObjectType() && target->AsETSObjectType()->IsGlobalETSObjectType()) {
        relation->Result(true);
        return;
    }
}

void ETSFunctionType::CastTarget(TypeRelation *relation, Type *source)
{
    AssertNoMethodsInFunctionRelation(this, source);

    if (relation->IsSupertypeOf(this, source)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }
    if (relation->InCastingContext() && relation->IsSupertypeOf(source, this)) {
        relation->RemoveFlags(TypeRelationFlag::UNCHECKED_CAST);
        return;
    }
    relation->Result(false);
}

void ETSFunctionType::IsSubtypeOf(TypeRelation *relation, Type *target)
{
    AssertNoMethodsInFunctionRelation(this, target);

    ETSChecker *checker = relation->GetChecker()->AsETSChecker();

    // fastpath
    if (relation->IsSupertypeOf(target, checker->GlobalBuiltinFunctionType())) {
        relation->Result(true);
        return;
    }

    // fastpath, as IsSupertypeOf has already failed
    if (target->IsETSFunctionType()) {
        return;
    }

    relation->IsSupertypeOf(target, ArrowToFunctionalInterface(checker));
}

void ETSFunctionType::ToAssemblerType([[maybe_unused]] std::stringstream &ss) const
{
    ss << assemblerName_;
}

// #22952: some function types are still in AST
static std::string FunctionAssemblyTypeFromArity(uint32_t arity)
{
    return "std.core.Function" + std::to_string(arity);
}

void ETSFunctionType::ToDebugInfoType([[maybe_unused]] std::stringstream &ss) const
{
    ES2PANDA_ASSERT(IsETSArrowType());
    ss << FunctionAssemblyTypeFromArity(ArrowSignature()->MinArgCount());
}

void ETSFunctionType::CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag)
{
    // For function, param is `in` and returntype is `out`，(in)=>out
    for (auto *sig : callSignatures_) {
        if (sig->HasFunction() && sig->Function()->ReturnTypeAnnotation() != nullptr) {
            relation->SetNode(sig->Function()->ReturnTypeAnnotation());
        }
        relation->CheckVarianceRecursively(sig->ReturnType(),
                                           relation->TransferVariant(varianceFlag, VarianceFlag::COVARIANT));

        if (sig->HasRestParameter()) {
            if (sig->HasFunction()) {
                relation->SetNode(sig->Function()->Params().at(sig->Params().size()));
            }
            relation->CheckVarianceRecursively(sig->RestVar()->TsType(),
                                               relation->TransferVariant(varianceFlag, VarianceFlag::COVARIANT));
        }
        for (auto *typeParam : sig->Params()) {
            relation->SetNode(
                typeParam->Declaration()->AsParameterDecl()->Node()->AsETSParameterExpression()->TypeAnnotation());
            relation->CheckVarianceRecursively(typeParam->TsType(),
                                               relation->TransferVariant(varianceFlag, VarianceFlag::CONTRAVARIANT));
        }
    }
}

}  // namespace ark::es2panda::checker
