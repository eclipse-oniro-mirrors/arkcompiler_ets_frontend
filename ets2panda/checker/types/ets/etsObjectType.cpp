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

#include "etsObjectType.h"

#include "checker/ETSchecker.h"
#include "checker/ets/conversion.h"
#include "checker/types/globalTypesHolder.h"
#include "checker/types/ets/etsAsyncFuncReturnType.h"
#include "checker/types/ets/etsEnumType.h"
#include "compiler/lowering/phase.h"
#include "ir/statements/annotationDeclaration.h"

namespace ark::es2panda::checker {

void ETSObjectType::Iterate(const PropertyTraverser &cb) const
{
    ForEachAllOwnProperties(cb);
    ForEachAllNonOwnProperties(cb);
}

void ETSObjectType::AddInterface(ETSObjectType *interfaceType)
{
    if (std::find(interfaces_.begin(), interfaces_.end(), interfaceType) == interfaces_.end()) {
        interfaces_.push_back(interfaceType);
        CacheSupertypeTransitive(interfaceType);
    }
}

void ETSObjectType::SetSuperType(ETSObjectType *super)
{
    superType_ = super;
    if (super == nullptr) {
        return;
    }
    CacheSupertypeTransitive(super);
}

void ETSObjectType::CacheSupertypeTransitive(ETSObjectType *type)
{
    auto const insertType = [this](ETSObjectType *t) {
        return transitiveSupertypes_.insert(t->GetOriginalBaseType()).second;
    };
    if (insertType(type)) {
        for (auto &t : type->transitiveSupertypes_) {
            insertType(t);
        }
    }
}

varbinder::LocalVariable *ETSObjectType::SearchFieldsDecls(util::StringView name, PropertySearchFlags flags) const
{
    varbinder::LocalVariable *res {};
    if ((flags & PropertySearchFlags::SEARCH_INSTANCE_FIELD) != 0) {
        res = GetOwnProperty<PropertyType::INSTANCE_FIELD>(name);
    }

    if (res == nullptr && ((flags & PropertySearchFlags::SEARCH_STATIC_FIELD) != 0)) {
        res = GetOwnProperty<PropertyType::STATIC_FIELD>(name);
    }

    if (res == nullptr && ((flags & PropertySearchFlags::SEARCH_INSTANCE_DECL) != 0)) {
        res = GetOwnProperty<PropertyType::INSTANCE_DECL>(name);
    }

    if (res == nullptr && ((flags & PropertySearchFlags::SEARCH_STATIC_DECL) != 0)) {
        res = GetOwnProperty<PropertyType::STATIC_DECL>(name);
    }
    return res;
}

varbinder::LocalVariable *ETSObjectType::GetProperty(util::StringView name, PropertySearchFlags flags) const
{
    // CC-OFFNXT(G.FMT.14-CPP) project code style
    auto const searchOwnMethod = [this, flags, name]() -> varbinder::LocalVariable * {
        if ((flags & PropertySearchFlags::SEARCH_INSTANCE_METHOD) != 0) {
            if (auto res = GetOwnProperty<PropertyType::INSTANCE_METHOD>(name); res != nullptr) {
                return res;
            }
        }
        if ((flags & PropertySearchFlags::SEARCH_STATIC_METHOD) != 0) {
            if (auto res = GetOwnProperty<PropertyType::STATIC_METHOD>(name); res != nullptr) {
                return res;
            }
        }
        return nullptr;
    };

    if (auto res = SearchFieldsDecls(name, flags); res != nullptr) {
        return res;
    }

    if ((flags & PropertySearchFlags::SEARCH_METHOD) != 0) {
        if ((flags & PropertySearchFlags::DISALLOW_SYNTHETIC_METHOD_CREATION) != 0) {
            if (auto res = searchOwnMethod(); res != nullptr) {
                return res;
            }
        } else {
            if (auto res = CreateSyntheticVarFromEverySignature(name, flags)) {
                return res;
            }
        }
    }

    if (((flags & PropertySearchFlags::SEARCH_INSTANCE) != 0 || (flags & PropertySearchFlags::SEARCH_STATIC) == 0) &&
        (flags & PropertySearchFlags::SEARCH_IN_INTERFACES) != 0) {
        for (auto *interface : interfaces_) {
            if (auto res = interface->GetProperty(name, flags); res != nullptr) {
                return res;
            }
        }
    }

    if ((flags & PropertySearchFlags::SEARCH_IN_BASE) != 0 && superType_ != nullptr) {
        return superType_->GetProperty(name, flags);
    }

    return nullptr;
}

bool ETSObjectType::IsPropertyInherited(const varbinder::Variable *var)
{
    if (var->HasFlag(varbinder::VariableFlags::PRIVATE)) {
        return GetProperty(var->Name(), PropertySearchFlags::SEARCH_FIELD | PropertySearchFlags::SEARCH_DECL) == var;
    }

    if (var->HasFlag(varbinder::VariableFlags::PROTECTED)) {
        return (GetProperty(var->Name(), PropertySearchFlags::SEARCH_FIELD | PropertySearchFlags::SEARCH_DECL) ==
                var) ||
               this->IsPropertyOfAscendant(var);
    }

    return true;
}

bool ETSObjectType::IsPropertyOfAscendant(const varbinder::Variable *var) const
{
    if (this->SuperType() == nullptr) {
        return false;
    }

    if (this->SuperType()->GetProperty(var->Name(),
                                       PropertySearchFlags::SEARCH_FIELD | PropertySearchFlags::SEARCH_DECL) == var) {
        return true;
    }

    return this->SuperType()->IsPropertyOfAscendant(var);
}

bool ETSObjectType::IsSignatureInherited(Signature *signature)
{
    if (signature->HasSignatureFlag(SignatureFlags::PRIVATE)) {
        return signature->Owner() == this;
    }

    if (signature->HasSignatureFlag(SignatureFlags::PROTECTED)) {
        return signature->Owner() == this || this->IsDescendantOf(signature->Owner());
    }

    return true;
}

bool ETSObjectType::IsDescendantOf(const ETSObjectType *ascendant) const
{
    if (this->SuperType() == nullptr) {
        return false;
    }

    if (this->SuperType() == ascendant) {
        return true;
    }

    return this->SuperType()->IsDescendantOf(ascendant);
}

static bool HasAccessor(const PropertySearchFlags &flags, const ETSFunctionType *funcType)
{
    if ((flags & (PropertySearchFlags::IS_GETTER | PropertySearchFlags::IS_SETTER)) != 0) {
        return true;
    }
    return funcType->HasTypeFlag(TypeFlag::GETTER) || funcType->HasTypeFlag(TypeFlag::SETTER);
}

static void UpdateDeclarationForGetterSetter(varbinder::LocalVariable *res, const ETSFunctionType *funcType,
                                             const PropertySearchFlags &flags)
{
    if (!HasAccessor(flags, funcType) || res->Declaration() != nullptr) {
        return;
    }

    auto frontGetter = std::find_if(funcType->CallSignatures().begin(), funcType->CallSignatures().end(),
                                    [](Signature *sig) { return sig->Function()->IsGetter(); });
    auto var = frontGetter == funcType->CallSignatures().end() ? funcType->CallSignatures().front()->OwnerVar()
                                                               : (*frontGetter)->OwnerVar();
    auto decl = var->Declaration();
    if (decl == nullptr || decl->Node() == nullptr) {
        return;
    }
    res->Reset(decl, var->Flags());
}

static PropertySearchFlags UpdateOverloadDeclarationSearchFlags(const PropertySearchFlags &flags)
{
    if ((flags & PropertySearchFlags::IGNORE_OVERLOAD) != 0) {
        return flags;
    }
    PropertySearchFlags syntheticFlags = flags;
    if ((flags & PropertySearchFlags::SEARCH_INSTANCE_METHOD) != 0) {
        syntheticFlags &= ~PropertySearchFlags::SEARCH_INSTANCE_METHOD;
        syntheticFlags |= PropertySearchFlags::SEARCH_INSTANCE_DECL;
    }
    if ((flags & PropertySearchFlags::SEARCH_STATIC_METHOD) != 0) {
        syntheticFlags &= ~PropertySearchFlags::SEARCH_STATIC_METHOD;
        syntheticFlags |= PropertySearchFlags::SEARCH_STATIC_DECL;
    }
    return syntheticFlags;
}

static PropertySearchFlags UpdateMethodSearchFlags(const PropertySearchFlags &flags)
{
    if ((flags & PropertySearchFlags::IGNORE_OVERLOAD) != 0) {
        return flags;
    }
    PropertySearchFlags syntheticFlags = flags;
    if ((flags & PropertySearchFlags::SEARCH_INSTANCE_DECL) != 0) {
        syntheticFlags &= ~PropertySearchFlags::SEARCH_INSTANCE_DECL;
        syntheticFlags |= PropertySearchFlags::SEARCH_INSTANCE_METHOD;
    }
    if ((flags & PropertySearchFlags::SEARCH_STATIC_DECL) != 0) {
        syntheticFlags &= ~PropertySearchFlags::SEARCH_STATIC_DECL;
        syntheticFlags |= PropertySearchFlags::SEARCH_STATIC_METHOD;
    }
    return syntheticFlags;
}

varbinder::LocalVariable *ETSObjectType::CreateSyntheticVarFromEverySignature(const util::StringView &name,
                                                                              PropertySearchFlags flags) const
{
    std::vector<Signature *> signatures;
    // Since both "first match" and "best match" exist at present, overloadDeclarationCall is temporarily used. After
    // "best match" removed, this marking needs to be removed.
    auto *overloadDeclaration = SearchFieldsDecls(name, UpdateOverloadDeclarationSearchFlags(flags));
    bool overloadDeclarationCall = overloadDeclaration != nullptr;
    PropertySearchFlags syntheticFlags = overloadDeclarationCall ? UpdateOverloadDeclarationSearchFlags(flags) : flags;

    varbinder::LocalVariable *functionalInterface = CollectSignaturesForSyntheticType(signatures, name, syntheticFlags);
    // #22952: the called function *always* returns nullptr
    ES2PANDA_ASSERT(functionalInterface == nullptr);
    (void)functionalInterface;

    if (signatures.empty()) {
        return nullptr;
    }

    varbinder::VariableFlags varianceFlag =
        overloadDeclarationCall ? varbinder::VariableFlags::SYNTHETIC | varbinder::VariableFlags::METHOD |
                                      varbinder::VariableFlags::OVERLOAD
                                : varbinder::VariableFlags::SYNTHETIC | varbinder::VariableFlags::METHOD;
    varbinder::LocalVariable *res = allocator_->New<varbinder::LocalVariable>(varianceFlag);

    ETSFunctionType *funcType = CreateMethodTypeForProp(name);
    ES2PANDA_ASSERT(funcType != nullptr);
    for (auto &s : signatures) {
        funcType->AddCallSignature(s);
    }
    ES2PANDA_ASSERT(res != nullptr);
    res->SetTsType(funcType);
    funcType->SetVariable(res);

    if (overloadDeclarationCall) {
        res->Reset(overloadDeclaration->Declaration(), res->Flags());
    }

    UpdateDeclarationForGetterSetter(res, funcType, flags);

    return res;
}

ETSFunctionType *ETSObjectType::CreateMethodTypeForProp(util::StringView name) const
{
    ES2PANDA_ASSERT(GetRelation() != nullptr);
    return GetRelation()->GetChecker()->AsETSChecker()->CreateETSMethodType(name, {{}, Allocator()->Adapter()});
}

bool ETSObjectType::ReplaceArgumentInSignature(std::vector<Signature *> &signatures, Signature *sigToInsert,
                                               TypeRelation *relation) const
{
    for (auto *&sigToReplace : signatures) {
        if (sigToReplace->ArgCount() != sigToInsert->ArgCount()) {
            continue;
        }
        if (relation->IsSupertypeOf(sigToInsert->Owner(), sigToReplace->Owner()) &&
            relation->SignatureIsSupertypeOf(sigToInsert, sigToReplace)) {
            // Already overridden by a subtype's signature
            return true;
        }
        if (relation->IsSupertypeOf(sigToReplace->Owner(), sigToInsert->Owner()) &&
            relation->SignatureIsSupertypeOf(sigToReplace, sigToInsert)) {
            sigToReplace = sigToInsert;
            return true;
        }
    }

    return false;
}

void ETSObjectType::AddSignatureFromFunction(std::vector<Signature *> &signatures, PropertySearchFlags flags,
                                             ETSChecker *checker, varbinder::LocalVariable *found) const
{
    if (found == nullptr || !found->TsType()->IsETSFunctionType()) {
        return;
    }

    for (auto *it : found->TsType()->AsETSFunctionType()->CallSignatures()) {
        if (std::find(signatures.begin(), signatures.end(), it) != signatures.end()) {
            continue;
        }
        if (((flags & PropertySearchFlags::IGNORE_ABSTRACT) != 0) && it->HasSignatureFlag(SignatureFlags::ABSTRACT)) {
            continue;
        }
        if (ReplaceArgumentInSignature(signatures, it, checker->Relation())) {
            continue;
        }
        signatures.emplace_back(it);
    }
}

void ETSObjectType::AddSignatureFromOverload(std::vector<Signature *> &signatures, PropertySearchFlags flags,
                                             varbinder::LocalVariable *found) const
{
    if (found == nullptr || !found->HasFlag(varbinder::VariableFlags::OVERLOAD)) {
        return;
    }

    ES2PANDA_ASSERT(found->Declaration()->Node()->IsOverloadDeclaration());
    auto *overloadDeclaration = found->Declaration()->Node()->AsOverloadDeclaration();
    std::vector<Signature *> methodSignature;
    if (overloadDeclaration->Id()->IsErrorPlaceHolder()) {
        return;
    }

    if (overloadDeclaration->IsConstructorOverloadDeclaration()) {
        return AddSignatureFromConstructor(signatures, found);
    }

    for (auto *method : overloadDeclaration->OverloadedList()) {
        // Identical type cannot be obtained directly, because typeparamter has not been substitute.
        methodSignature.clear();
        util::StringView methodName =
            method->IsIdentifier() ? method->AsIdentifier()->Name() : method->AsTSQualifiedName()->Right()->Name();
        CollectSignaturesForSyntheticType(methodSignature, methodName, UpdateMethodSearchFlags(flags));
        if (!methodSignature.empty()) {
            signatures.emplace_back(methodSignature.front());
        }
    }
}

void ETSObjectType::AddSignatureFromConstructor(std::vector<Signature *> &signatures,
                                                varbinder::LocalVariable *found) const
{
    auto *overloadDeclaration = found->Declaration()->Node()->AsOverloadDeclaration();
    for (auto *method : overloadDeclaration->OverloadedList()) {
        util::StringView orderConstructorName = method->AsIdentifier()->Name();

        // Constructor will lowering to multiple Constructor if have rest parameters or optional parameters.
        // Need to modify RestTupleConstructionPhase.
        std::vector<Signature *> matches;
        std::copy_if(
            constructSignatures_.begin(), constructSignatures_.end(), std::back_inserter(matches),
            [orderConstructorName](Signature *sig) { return sig->Function()->Id()->Name() == orderConstructorName; });

        if (!matches.empty()) {
            std::copy(matches.begin(), matches.end(), std::back_inserter(signatures));
        }
    }
}

varbinder::LocalVariable *ETSObjectType::CollectSignaturesForSyntheticType(std::vector<Signature *> &signatures,
                                                                           const util::StringView &name,
                                                                           PropertySearchFlags flags) const
{
    auto *checker = GetRelation()->GetChecker()->AsETSChecker();

    if ((flags & PropertySearchFlags::SEARCH_STATIC_METHOD) != 0) {
        auto *found = GetOwnProperty<PropertyType::STATIC_METHOD>(name);
        AddSignatureFromFunction(signatures, flags, checker, found);
    }

    if ((flags & PropertySearchFlags::SEARCH_INSTANCE_METHOD) != 0) {
        auto *found = GetOwnProperty<PropertyType::INSTANCE_METHOD>(name);
        AddSignatureFromFunction(signatures, flags, checker, found);
    }

    if ((flags & PropertySearchFlags::SEARCH_STATIC_DECL) != 0) {
        auto *found = GetOwnProperty<PropertyType::STATIC_DECL>(name);
        AddSignatureFromOverload(signatures, flags, found);
    }

    if ((flags & PropertySearchFlags::SEARCH_INSTANCE_DECL) != 0) {
        auto *found = GetOwnProperty<PropertyType::INSTANCE_DECL>(name);
        AddSignatureFromOverload(signatures, flags, found);
    }

    if ((flags & PropertySearchFlags::SEARCH_METHOD) == 0 && (flags & PropertySearchFlags::SEARCH_DECL) == 0) {
        return nullptr;
    }

    if (superType_ != nullptr && ((flags & PropertySearchFlags::SEARCH_IN_BASE) != 0)) {
        superType_->CollectSignaturesForSyntheticType(signatures, name, flags);
    }

    if ((flags & PropertySearchFlags::SEARCH_IN_INTERFACES) != 0) {
        for (auto *interface : Interfaces()) {
            interface->CollectSignaturesForSyntheticType(signatures, name, flags);
        }
    }

    return nullptr;
}

void ETSObjectType::ForEachAllOwnProperties(const PropertyTraverser &cb) const
{
    EnsurePropertiesInstantiated();
    for (size_t i = 0; i < static_cast<size_t>(PropertyType::COUNT); ++i) {
        PropertyMap &map = properties_[i];
        for (const auto &[_, prop] : map) {
            (void)_;
            cb(prop);
        }
    }
}

void ETSObjectType::ForEachAllNonOwnProperties(const PropertyTraverser &cb) const
{
    if (superType_ != nullptr) {
        superType_->Iterate(cb);
    }

    for (const auto *interface : interfaces_) {
        interface->Iterate(cb);
    }
}

std::vector<varbinder::LocalVariable *> ETSObjectType::GetAllProperties() const
{
    std::vector<varbinder::LocalVariable *> allProperties;
    for (const auto &[_, prop] : InstanceFields()) {
        (void)_;
        allProperties.push_back(prop);
    }

    for (const auto &[_, prop] : StaticFields()) {
        (void)_;
        allProperties.push_back(prop);
    }

    for (const auto &[_, prop] : InstanceMethods()) {
        (void)_;
        allProperties.push_back(prop);
    }

    for (const auto &[_, prop] : StaticMethods()) {
        (void)_;
        allProperties.push_back(prop);
    }

    for (const auto &[_, prop] : InstanceDecls()) {
        (void)_;
        allProperties.push_back(prop);
    }

    for (const auto &[_, prop] : StaticDecls()) {
        (void)_;
        allProperties.push_back(prop);
    }

    return allProperties;
}

std::vector<varbinder::LocalVariable *> ETSObjectType::Overloads() const
{
    std::vector<varbinder::LocalVariable *> methods;
    for (const auto &[_, prop] : InstanceMethods()) {
        (void)_;
        if (prop->HasFlag(varbinder::VariableFlags::OVERLOAD)) {
            methods.push_back(prop);
        }
    }

    for (const auto &[_, prop] : StaticMethods()) {
        (void)_;
        if (prop->HasFlag(varbinder::VariableFlags::OVERLOAD)) {
            methods.push_back(prop);
        }
    }

    return methods;
}

std::vector<varbinder::LocalVariable *> ETSObjectType::Methods() const
{
    std::vector<varbinder::LocalVariable *> methods;
    for (const auto &[_, prop] : InstanceMethods()) {
        (void)_;
        if (prop->HasFlag(varbinder::VariableFlags::OVERLOAD)) {
            continue;
        }
        methods.push_back(prop);
    }

    for (const auto &[_, prop] : StaticMethods()) {
        (void)_;
        if (prop->HasFlag(varbinder::VariableFlags::OVERLOAD)) {
            continue;
        }
        methods.push_back(prop);
    }

    return methods;
}

std::vector<varbinder::LocalVariable *> ETSObjectType::Fields() const
{
    std::vector<varbinder::LocalVariable *> fields;
    for (const auto &[_, prop] : InstanceFields()) {
        (void)_;
        fields.push_back(prop);
    }

    for (const auto &[_, prop] : StaticFields()) {
        (void)_;
        fields.push_back(prop);
    }

    return fields;
}

std::vector<const varbinder::LocalVariable *> ETSObjectType::ForeignProperties() const
{
    std::vector<const varbinder::LocalVariable *> foreignProps;

    // spec 9.3: all names in static and, separately, non-static class declaration scopes must be unique.
    std::unordered_set<util::StringView> ownInstanceProps;
    std::unordered_set<util::StringView> ownStaticProps;

    EnsurePropertiesInstantiated();
    ownInstanceProps.reserve(properties_.size());
    ownStaticProps.reserve(properties_.size());

    ForEachAllOwnProperties([&](const varbinder::LocalVariable *prop) {
        if (prop->HasFlag(varbinder::VariableFlags::STATIC)) {
            ownStaticProps.insert(prop->Name());
        } else {
            ownInstanceProps.insert(prop->Name());
        }
    });
    ForEachAllNonOwnProperties([&](const varbinder::LocalVariable *var) {
        if (var->HasFlag(varbinder::VariableFlags::STATIC)) {
            if (ownStaticProps.find(var->Name()) == ownStaticProps.end()) {
                foreignProps.push_back(var);
            }
        } else {
            if (ownInstanceProps.find(var->Name()) == ownInstanceProps.end()) {
                foreignProps.push_back(var);
            }
        }
    });

    return foreignProps;
}

void ETSObjectType::ToString(std::stringstream &ss, bool precise) const
{
    if (IsPartial()) {
        ss << "Partial" << compiler::Signatures::GENERIC_BEGIN;
        baseType_->ToString(ss, precise);
        ss << compiler::Signatures::GENERIC_END;
        return;
    }

    if (HasTypeFlag(TypeFlag::READONLY)) {
        ss << "Readonly" << compiler::Signatures::GENERIC_BEGIN;
    }
    if (HasObjectFlag(ETSObjectFlags::REQUIRED)) {
        ss << "Required" << compiler::Signatures::GENERIC_BEGIN;
    }

    ss << (precise ? internalName_ : name_);

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

    if (HasObjectFlag(ETSObjectFlags::REQUIRED)) {
        ss << compiler::Signatures::GENERIC_END;
    }
    if (HasTypeFlag(TypeFlag::READONLY)) {
        ss << compiler::Signatures::GENERIC_END;
    }
}

void ETSObjectType::SubstitutePartialTypes(TypeRelation *relation, Type *other)
{
    ES2PANDA_ASSERT(IsPartial());

    if ((baseType_->IsGeneric() || baseType_->IsETSTypeParameter()) && effectiveSubstitution_ != nullptr) {
        auto subst = ETSChecker::ArenaSubstitutionToSubstitution(effectiveSubstitution_);
        if (auto *newBaseType = baseType_->Substitute(relation, &subst);
            newBaseType->IsETSObjectType() && !relation->IsIdenticalTo(newBaseType, baseType_)) {
            baseType_ = newBaseType->AsETSObjectType();
        }
    }

    if (other->IsETSObjectType() && other->AsETSObjectType()->IsPartial()) {
        auto *otherPartial = other->AsETSObjectType();
        if ((otherPartial->baseType_->IsGeneric() || otherPartial->baseType_->IsETSTypeParameter()) &&
            otherPartial->effectiveSubstitution_ != nullptr) {
            auto subst = ETSChecker::ArenaSubstitutionToSubstitution(otherPartial->effectiveSubstitution_);
            if (auto *newBaseType = otherPartial->baseType_->Substitute(relation, &subst);
                newBaseType->IsETSObjectType() && !relation->IsIdenticalTo(newBaseType, otherPartial->baseType_)) {
                otherPartial->baseType_ = newBaseType->AsETSObjectType();
            }
        }
    }
    relation->Result(false);  // this function spoils the relation
}

void ETSObjectType::IdenticalUptoTypeArguments(TypeRelation *relation, Type *other)
{
    relation->Result(false);
    if (!other->IsETSObjectType() || !CheckIdenticalFlags(other->AsETSObjectType())) {
        return;
    }

    if (IsPartial()) {
        SubstitutePartialTypes(relation, other);
    }

    // NOTE: (DZ) only both Partial types can be compatible.
    if (static_cast<bool>(static_cast<std::byte>(IsPartial()) ^
                          static_cast<std::byte>(other->AsETSObjectType()->IsPartial()))) {
        return;
    }

    auto *thisBase = GetOriginalBaseType();
    auto *otherBase = other->AsETSObjectType()->GetOriginalBaseType();
    if (thisBase->Variable()->Declaration()->Node() != otherBase->Variable()->Declaration()->Node()) {
        return;
    }

    if ((relation->IgnoreTypeParameters() && !HasObjectFlag(ETSObjectFlags::FUNCTIONAL)) || (this == other)) {
        relation->Result(true);
        return;
    }

    auto const sourceTypeArguments = other->AsETSObjectType()->TypeArguments();
    if (typeArguments_.empty() != sourceTypeArguments.empty()) {
        return;
    }

    relation->Result(true);
}

void ETSObjectType::Identical(TypeRelation *relation, Type *other)
{
    IdenticalUptoTypeArguments(relation, other);

    if (!relation->IsTrue() || !HasTypeFlag(TypeFlag::GENERIC) || !other->IsETSObjectType()) {
        return;
    }

    if (HasTypeFlag(TypeFlag::READONLY) != other->HasTypeFlag(TypeFlag::READONLY)) {
        relation->Result(false);
        return;
    }

    auto const otherTypeArguments = other->AsETSObjectType()->TypeArguments();

    auto const argsNumber = typeArguments_.size();
    if (argsNumber != otherTypeArguments.size()) {
        relation->Result(false);
        return;
    }

    for (size_t idx = 0U; idx < argsNumber; ++idx) {
        if (typeArguments_[idx]->IsWildcardType() || otherTypeArguments[idx]->IsWildcardType()) {
            continue;
        }
        if (!relation->IsIdenticalTo(typeArguments_[idx], otherTypeArguments[idx])) {
            return;
        }
    }

    relation->Result(true);
}

bool ETSObjectType::CheckIdenticalFlags(ETSObjectType *other) const
{
    constexpr auto FLAGS_TO_REMOVE = ETSObjectFlags::INCOMPLETE_INSTANTIATION |
                                     ETSObjectFlags::CHECKED_INVOKE_LEGITIMACY | ETSObjectFlags::EXTENSION_FUNCTION;

    auto cleanedTargetFlags = other->ObjectFlags();
    cleanedTargetFlags &= ~FLAGS_TO_REMOVE;

    auto cleanedSelfFlags = ObjectFlags();
    cleanedSelfFlags &= ~FLAGS_TO_REMOVE;

    return cleanedSelfFlags == cleanedTargetFlags;
}

bool ETSObjectType::AssignmentSource(TypeRelation *const relation, [[maybe_unused]] Type *const target)
{
    // NOTE: do not modify, to be implied by the relation
    return relation->IsSupertypeOf(target, this);
}

bool ETSObjectType::IsBoxedPrimitive() const
{
    if (this->IsETSEnumType()) {
        return false;
    }

    return this->IsETSUnboxableObject();
}

void ETSObjectType::AssignmentTarget(TypeRelation *const relation, Type *source)
{
    // NOTE: do not modify, to be implied by the relation
    relation->IsSupertypeOf(this, source);
}

ETSFunctionType *ETSObjectType::GetFunctionalInterfaceInvokeType() const
{
    ES2PANDA_ASSERT(HasObjectFlag(ETSObjectFlags::FUNCTIONAL));
    auto checker = GetRelation()->GetChecker()->AsETSChecker();

    // NOTE(vpukhov): this is still better than to retain any "functional" state in ETSObjectType
    auto [foundArity, hasRest] = [this, checker]() {
        auto baseType = GetConstOriginalBaseType();
        for (size_t arity = 0; arity <= checker->GlobalBuiltinFunctionTypeVariadicThreshold(); ++arity) {
            if (auto itf = checker->GlobalBuiltinFunctionType(arity, false); itf == baseType) {
                return std::make_pair(arity, false);
            }
            if (auto itf = checker->GlobalBuiltinFunctionType(arity, true); itf == baseType) {
                return std::make_pair(arity, true);
            }
        }
        ES2PANDA_UNREACHABLE();
    }();

    std::string invokeName = checker->FunctionalInterfaceInvokeName(foundArity, hasRest);
    auto *invoke = GetProperty(util::StringView(invokeName),
                               PropertySearchFlags::SEARCH_INSTANCE_METHOD | PropertySearchFlags::SEARCH_IN_INTERFACES);
    ES2PANDA_ASSERT(invoke != nullptr && invoke->TsType() != nullptr && invoke->TsType()->IsETSFunctionType());
    return invoke->TsType()->AsETSFunctionType();
}

bool ETSObjectType::CastWidening(TypeRelation *const relation, Type *const target, TypeFlag unboxFlags,
                                 TypeFlag wideningFlags)
{
    if (target->HasTypeFlag(unboxFlags)) {
        conversion::Unboxing(relation, this);
        return true;
    }
    if (target->HasTypeFlag(wideningFlags)) {
        conversion::UnboxingWideningPrimitive(relation, this, target);
        return true;
    }
    return false;
}

bool ETSObjectType::TryCastByte(TypeRelation *const relation, Type *const target)
{
    if (target->HasTypeFlag(TypeFlag::BYTE)) {
        conversion::Unboxing(relation, this);
        return true;
    }
    if (target->HasTypeFlag(TypeFlag::SHORT | TypeFlag::INT | TypeFlag::LONG | TypeFlag::FLOAT | TypeFlag::DOUBLE)) {
        conversion::UnboxingWideningPrimitive(relation, this, target);
        return true;
    }
    if (target->HasTypeFlag(TypeFlag::CHAR)) {
        conversion::UnboxingWideningPrimitive(relation, this, target);
        return true;
    }
    return false;
}

bool ETSObjectType::TryCastIntegral(TypeRelation *const relation, Type *const target)
{
    if (this->HasObjectFlag(ETSObjectFlags::BUILTIN_BYTE) && TryCastByte(relation, target)) {
        return true;
    }
    if (this->HasObjectFlag(ETSObjectFlags::BUILTIN_SHORT) &&
        CastWidening(relation, target, TypeFlag::SHORT,
                     TypeFlag::INT | TypeFlag::LONG | TypeFlag::FLOAT | TypeFlag::DOUBLE)) {
        return true;
    }
    if (this->HasObjectFlag(ETSObjectFlags::BUILTIN_CHAR) &&
        CastWidening(relation, target, TypeFlag::CHAR,
                     TypeFlag::SHORT | TypeFlag::INT | TypeFlag::LONG | TypeFlag::FLOAT | TypeFlag::DOUBLE)) {
        return true;
    }
    if (this->HasObjectFlag(ETSObjectFlags::BUILTIN_INT) &&
        CastWidening(relation, target, TypeFlag::INT, TypeFlag::LONG | TypeFlag::FLOAT | TypeFlag::DOUBLE)) {
        return true;
    }
    if (this->HasObjectFlag(ETSObjectFlags::BUILTIN_LONG) &&
        CastWidening(relation, target, TypeFlag::LONG, TypeFlag::FLOAT | TypeFlag::DOUBLE)) {
        return true;
    }
    return false;
}

bool ETSObjectType::TryCastFloating(TypeRelation *const relation, Type *const target)
{
    if (this->HasObjectFlag(ETSObjectFlags::BUILTIN_FLOAT) &&
        CastWidening(relation, target, TypeFlag::FLOAT, TypeFlag::DOUBLE)) {
        return true;
    }
    if (this->HasObjectFlag(ETSObjectFlags::BUILTIN_DOUBLE) &&
        CastWidening(relation, target, TypeFlag::DOUBLE, TypeFlag::NONE)) {
        return true;
    }
    return false;
}

bool ETSObjectType::TryCastUnboxable(TypeRelation *const relation, Type *const target)
{
    if (target->HasTypeFlag(TypeFlag::ETS_OBJECT)) {
        if (!target->IsETSUnboxableObject()) {
            conversion::WideningReference(relation, this, target->AsETSObjectType());
            return true;
        }
        auto unboxedTarget = relation->GetChecker()->AsETSChecker()->MaybeUnboxInRelation(target);
        CastNumericObject(relation, unboxedTarget);
        if (relation->IsTrue()) {
            conversion::Boxing(relation, unboxedTarget);
            return true;
        }
        conversion::WideningReference(relation, this, target->AsETSObjectType());
        return true;
    }

    if (target->IsETSEnumType()) {
        auto unboxedThis = relation->GetChecker()->AsETSChecker()->MaybeUnboxInRelation(this);
        return relation->IsCastableTo(unboxedThis, target);
    }

    conversion::Forbidden(relation);
    return true;
}

bool ETSObjectType::CastNumericObject(TypeRelation *const relation, Type *const target)
{
    if (!target->IsETSPrimitiveType()) {
        return false;
    }
    if (relation->IsIdenticalTo(this, target)) {
        return true;
    }
    if (TryCastIntegral(relation, target)) {
        return true;
    }
    if (TryCastFloating(relation, target)) {
        return true;
    }
    if (this->HasObjectFlag(ETSObjectFlags::BUILTIN_BOOLEAN) && target->HasTypeFlag(TypeFlag::ETS_BOOLEAN)) {
        conversion::Unboxing(relation, this);
        return true;
    }
    if (this->IsETSUnboxableObject()) {
        return TryCastUnboxable(relation, target);
    }
    return false;
}

void ETSObjectType::Cast(TypeRelation *const relation, Type *const target)
{
    conversion::Identity(relation, this, target);
    if (relation->IsTrue()) {
        return;
    }

    if (target->IsGradualType()) {
        relation->Result(true);
        return;
    }

    if (CastNumericObject(relation, target)) {
        return;
    }

    if (target->HasTypeFlag(TypeFlag::ETS_ARRAY)) {
        conversion::NarrowingReference(relation, this, target->AsETSArrayType());
        return;
    }

    if (target->HasTypeFlag(TypeFlag::ETS_TUPLE)) {
        conversion::NarrowingReference(relation, this, target->AsETSTupleType());
        return;
    }

    if (target->HasTypeFlag(TypeFlag::ETS_OBJECT)) {
        conversion::WideningReference(relation, this, target->AsETSObjectType());
        if (relation->IsTrue()) {
            return;
        }

        conversion::NarrowingReference(relation, this, target->AsETSObjectType());
        if (relation->IsTrue()) {
            return;
        }
    }

    //  #16485: Probably temporary solution for generic bridges realization. Allows casting of generic classes
    //          in the form C<T> as C<U> (where U extends T) or C<T> as D (where D extends C<U>)
    if ((relation->GetChecker()->Context().Status() & CheckerStatus::IN_BRIDGE_TEST) != 0U) {
        SavedTypeRelationFlagsContext const savedFlags(relation, relation->GetTypeRelationFlags() |
                                                                     TypeRelationFlag::IGNORE_TYPE_PARAMETERS);
        relation->IsSupertypeOf(this, target);
        return;
    }

    conversion::Forbidden(relation);
}

void ETSObjectType::IsSupertypeOf(TypeRelation *relation, Type *source)
{
    relation->Result(false);
    auto const checker = relation->GetChecker()->AsETSChecker();

    if (source->HasTypeFlag(TypeFlag::READONLY)) {
        relation->Result(false);
        return;
    }
    if (IsPartial()) {
        source->IsETSObjectType() && source->AsETSObjectType()->IsPartial() &&
            relation->IsSupertypeOf(GetBaseType(), source->AsETSObjectType()->GetBaseType());
        return;
    }
    if (!source->IsETSObjectType()) {
        return;
    }

    auto const sourceObj = source->AsETSObjectType();
    // #23072 - superType_ of the current object is not intialized in recursive generics
    if (GetConstOriginalBaseType() == checker->GlobalETSObjectType()) {  // Fastpath, all objects are subtypes of Object
        relation->Result(true);
        return;
    }
    if (sourceObj->HasObjectFlag(ETSObjectFlags::CLASS | ETSObjectFlags::INTERFACE)) {
        IdenticalUptoTypeArguments(relation, sourceObj);
        if (relation->IsTrue() && HasTypeFlag(TypeFlag::GENERIC) && !relation->IgnoreTypeParameters()) {
            IsGenericSupertypeOf(relation, sourceObj);
        }
        if (relation->IsTrue()) {
            return;
        }
    }
    //  #16485: special case for generic bridges processing.
    //          We need only to check if the type is immediate supertype of processing class.
    if ((checker->Context().Status() & CheckerStatus::IN_BRIDGE_TEST) != 0U && relation->IsBridgeCheck()) {
        if (sourceObj->Variable() == checker->Context().ContainingClass()->SuperType()->Variable()) {
            return;
        }
    }
}

void ETSObjectType::IsSubtypeOf(TypeRelation *relation, Type *target)
{
    if (target->IsETSObjectType()) {
        auto &transitives = transitiveSupertypes_;
        if (transitives.find(target->AsETSObjectType()->GetOriginalBaseType()) == transitives.end()) {
            relation->Result(false);
            return;
        }
    }

    if (auto super = SuperType(); super != nullptr) {
        if (relation->IsSupertypeOf(target, super)) {
            return;
        }
    }
    for (auto super : Interfaces()) {
        if (relation->IsSupertypeOf(target, super)) {
            return;
        }
    }
}

void ETSObjectType::IsGenericSupertypeOf(TypeRelation *relation, ETSObjectType *source)
{
    ES2PANDA_ASSERT(HasTypeFlag(TypeFlag::GENERIC));

    auto const &sourceTypeArguments = source->TypeArguments();
    auto const typeArgumentsNumber = typeArguments_.size();
    if (typeArgumentsNumber > sourceTypeArguments.size()) {
        relation->Result(false);
        return;
    }

    ES2PANDA_ASSERT(declNode_ == source->GetDeclNode());

    auto *typeParamsDecl = GetTypeParams();
    ES2PANDA_ASSERT(typeParamsDecl != nullptr || typeArguments_.empty());

    if (typeParamsDecl == nullptr) {
        return;
    }

    auto &typeParams = typeParamsDecl->Params();
    ES2PANDA_ASSERT(typeParams.size() == typeArgumentsNumber);

    for (size_t idx = 0U; idx < typeArgumentsNumber; ++idx) {
        auto *typeArg = typeArguments_[idx];
        auto *sourceTypeArg = sourceTypeArguments[idx];
        auto *typeParam = typeParams[idx];

        relation->Result(false);
        if (typeArg->IsWildcardType() || sourceTypeArg->IsWildcardType()) {
            continue;
        }
        if (typeParam->IsOut()) {
            relation->IsSupertypeOf(typeArg, sourceTypeArg);
        } else if (typeParam->IsIn()) {
            relation->IsSupertypeOf(sourceTypeArg, typeArg);
        } else {
            relation->IsIdenticalTo(typeArg, sourceTypeArg);
        }

        if (!relation->IsTrue()) {
            return;
        }
    }

    relation->Result(true);
}

Type *ETSObjectType::AsSuper(Checker *checker, varbinder::Variable *sourceVar)
{
    checker = GetETSChecker();
    if (sourceVar == nullptr) {
        return nullptr;
    }

    if (variable_ == sourceVar) {
        return this;
    }

    if (HasObjectFlag(ETSObjectFlags::INTERFACE)) {
        Type *res = nullptr;
        for (auto *const it : checker->AsETSChecker()->GetInterfaces(this)) {
            res = it->AsSuper(checker, sourceVar);
            if (res != nullptr) {
                return res;
            }
        }
        return checker->GetGlobalTypesHolder()->GlobalETSObjectType()->AsSuper(checker, sourceVar);
    }

    Type *const superType = checker->AsETSChecker()->GetSuperType(this);

    if (superType == nullptr) {
        return nullptr;
    }

    if (!superType->IsETSObjectType()) {
        return nullptr;
    }

    if (ETSObjectType *const superObj = superType->AsETSObjectType(); superObj->HasObjectFlag(ETSObjectFlags::CLASS)) {
        Type *const res = superObj->AsSuper(checker, sourceVar);
        if (res != nullptr) {
            return res;
        }
    }

    if (sourceVar->TsType()->IsETSObjectType() &&
        sourceVar->TsType()->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::INTERFACE)) {
        for (auto *const it : checker->AsETSChecker()->GetInterfaces(this)) {
            Type *const res = it->AsSuper(checker, sourceVar);
            if (res != nullptr) {
                return res;
            }
        }
    }

    return nullptr;
}

varbinder::LocalVariable *ETSObjectType::CopyProperty(varbinder::LocalVariable *prop, ArenaAllocator *allocator,
                                                      TypeRelation *relation, GlobalTypesHolder *globalTypes)
{
    auto *const copiedProp = prop->Copy(allocator, prop->Declaration());
    auto *const copiedPropType = ETSChecker::TryToInstantiate(
        relation->GetChecker()->AsETSChecker()->GetTypeOfVariable(prop), allocator, relation, globalTypes);
    // NOTE: don't change type variable if it differs from copying one!
    if (copiedPropType->Variable() == prop) {
        copiedPropType->SetVariable(copiedProp);
    }
    ES2PANDA_ASSERT(copiedProp != nullptr);
    copiedProp->SetTsType(copiedPropType);
    return copiedProp;
}

Type *ETSObjectType::Instantiate(ArenaAllocator *const allocator, TypeRelation *relation,
                                 GlobalTypesHolder *const globalTypes)
{
    relation = relation_;
    auto *const checker = relation->GetChecker()->AsETSChecker();
    std::lock_guard guard {*checker->Mutex()};
    auto *const base = GetOriginalBaseType();

    if (!relation->IsAtTypeDepthLimit(base)) {
        return this;
    }
    relation->IncreaseTypeRecursionCount(base);

    auto *const copiedType = checker->CreateETSObjectType(declNode_, flags_);
    ES2PANDA_ASSERT(copiedType->internalName_ == internalName_);
    ES2PANDA_ASSERT(copiedType->name_ == name_);
    ES2PANDA_ASSERT(copiedType != nullptr);
    copiedType->typeFlags_ = typeFlags_;
    copiedType->RemoveObjectFlag(ETSObjectFlags::INCOMPLETE_INSTANTIATION | ETSObjectFlags::CHECKED_INVOKE_LEGITIMACY);
    copiedType->SetVariable(variable_);
    copiedType->SetSuperType(superType_);

    for (auto *const it : interfaces_) {
        copiedType->AddInterface(it);
    }

    ArenaVector<Type *> typeArgs(allocator->Adapter());
    for (auto *const typeArgument : TypeArguments()) {
        typeArgs.emplace_back(typeArgument->Instantiate(allocator, relation, globalTypes));
    }
    copiedType->SetTypeArguments(std::move(typeArgs));
    copiedType->SetBaseType(this);
    copiedType->propertiesInstantiated_ = false;
    copiedType->relation_ = relation;
    copiedType->effectiveSubstitution_ = nullptr;

    relation->DecreaseTypeRecursionCount(base);

    return copiedType;
}

static Type *SubstituteVariableType(TypeRelation *relation, const Substitution *substitution, Type *const varType)
{
    auto *substitutedType = varType->Substitute(relation, substitution);

    if (varType->HasTypeFlag(TypeFlag::ETS_REQUIRED_TYPE_PARAMETER)) {
        substitutedType = relation->GetChecker()->AsETSChecker()->HandleRequiredType(substitutedType);
    }

    return substitutedType;
}

static varbinder::LocalVariable *CopyPropertyWithTypeArguments(varbinder::LocalVariable *prop, TypeRelation *relation,
                                                               const Substitution *substitution)
{
    auto *const checker = relation->GetChecker()->AsETSChecker();
    auto *const varType = ETSChecker::IsVariableGetterSetter(prop) ? prop->TsType() : checker->GetTypeOfVariable(prop);
    auto *const copiedPropType = SubstituteVariableType(relation, substitution, varType);
    auto *const copiedProp = prop->Copy(checker->Allocator(), prop->Declaration());
    // NOTE: some situation copiedPropType we get here are types cached in Checker,
    // uncontrolled SetVariable will pollute the cache.
    if (copiedPropType->Variable() == prop || copiedPropType->Variable() == nullptr) {
        copiedPropType->SetVariable(copiedProp);
    }
    ES2PANDA_ASSERT(copiedProp != nullptr);
    copiedProp->SetTsType(copiedPropType);
    return copiedProp;
}

ETSObjectType const *ETSObjectType::GetConstOriginalBaseType() const noexcept
{
    if (auto *baseIter = GetBaseType(); baseIter != nullptr) {
        auto *baseIterNext = baseIter->GetBaseType();
        while (baseIterNext != nullptr && baseIterNext != baseIter) {
            baseIter = baseIterNext;
            baseIterNext = baseIter->GetBaseType();
        }
        return baseIter;
    }
    return this;
}

bool ETSObjectType::SubstituteTypeArgs(TypeRelation *const relation, ArenaVector<Type *> &newTypeArgs,
                                       const Substitution *const substitution)
{
    bool anyChange = false;
    newTypeArgs.reserve(typeArguments_.size());

    for (auto *const arg : typeArguments_) {
        auto *const newArg = arg->Substitute(relation, substitution);
        newTypeArgs.push_back(newArg);
        anyChange = anyChange || (newArg != arg);
    }

    return anyChange;
}

static ArenaSubstitution *ComputeEffectiveSubstitution(TypeRelation *const relation,
                                                       const ArenaVector<Type *> &baseTypeParams,
                                                       ArenaVector<Type *> &newTypeArgs)
{
    ES2PANDA_ASSERT(baseTypeParams.size() == newTypeArgs.size());
    auto *const checker = relation->GetChecker()->AsETSChecker();
    auto *effectiveSubstitution = checker->NewArenaSubstitution();

    for (size_t ix = 0; ix < baseTypeParams.size(); ix++) {
        checker->EmplaceSubstituted(effectiveSubstitution, baseTypeParams[ix]->AsETSTypeParameter(), newTypeArgs[ix]);
    }

    return effectiveSubstitution;
}

void ETSObjectType::SetCopiedTypeProperties(TypeRelation *const relation, ETSObjectType *const copiedType,
                                            ArenaVector<Type *> &&newTypeArgs, ETSObjectType *base)
{
    ES2PANDA_ASSERT(copiedType != nullptr);
    copiedType->typeFlags_ = typeFlags_;
    copiedType->RemoveObjectFlag(ETSObjectFlags::INCOMPLETE_INSTANTIATION | ETSObjectFlags::CHECKED_INVOKE_LEGITIMACY);
    copiedType->SetVariable(variable_);

    // #25295 Need to do some refactor on baseType for partial
    if (IsPartial() && HasObjectFlag(ETSObjectFlags::INTERFACE)) {
        copiedType->SetBaseType(this);
    } else {
        copiedType->SetBaseType(base);
    }

    auto const &baseTypeParams = base->TypeArguments();
    copiedType->effectiveSubstitution_ = ComputeEffectiveSubstitution(relation, baseTypeParams, newTypeArgs);

    copiedType->SetTypeArguments(std::move(newTypeArgs));
    ES2PANDA_ASSERT(relation);
    copiedType->relation_ = relation;
}

void ETSObjectType::UpdateTypeProperty(varbinder::LocalVariable *const prop, PropertyType fieldType,
                                       PropertyProcesser const &func)
{
    auto const propType = prop->Declaration()->Node()->Check(GetETSChecker());

    auto *const propCopy = func(prop, propType);
    if (fieldType == PropertyType::INSTANCE_FIELD) {
        RemoveProperty<PropertyType::INSTANCE_FIELD>(prop);
        AddProperty<PropertyType::INSTANCE_FIELD>(propCopy);
    } else {
        RemoveProperty<PropertyType::STATIC_FIELD>(prop);
        AddProperty<PropertyType::STATIC_FIELD>(propCopy);
    }
}

void ETSObjectType::UpdateTypeProperties(PropertyProcesser const &func)
{
    AddTypeFlag(TypeFlag::READONLY);
    for (auto const &prop : InstanceFields()) {
        UpdateTypeProperty(prop.second, PropertyType::INSTANCE_FIELD, func);
    }

    for (auto const &prop : StaticFields()) {
        UpdateTypeProperty(prop.second, PropertyType::STATIC_FIELD, func);
    }

    if (SuperType() != nullptr) {
        auto *const superProp =
            SuperType()
                ->Instantiate(allocator_, relation_, relation_->GetChecker()->GetGlobalTypesHolder())
                ->AsETSObjectType();
        superProp->UpdateTypeProperties(func);
        SetSuperType(superProp);
    }
}

static util::StringView GetHashFromSubstitution(const Substitution *substitution, const bool extensionFuncFlag,
                                                ArenaAllocator *allocator)
{
    std::vector<std::string> fields;
    for (auto [k, v] : *substitution) {
        std::stringstream ss;
        k->ToString(ss, true);
        ss << ":";
        v->ToString(ss, true);
        // NOTE (mmartin): change bare address to something more appropriate unique representation
        ss << ":" << k << ":" << v;
        fields.push_back(ss.str());
    }
    std::sort(fields.begin(), fields.end());

    std::stringstream ss;
    for (auto &fstr : fields) {
        ss << fstr;
        ss << ";";
    }

    if (extensionFuncFlag) {
        ss << "extensionFunctionType;";
    }
    return util::UString(ss.str(), allocator).View();
}

static std::pair<util::StringView, util::StringView> GetObjectTypeDeclNames(ir::AstNode *node)
{
    if (node->IsClassDefinition()) {
        return {node->AsClassDefinition()->Ident()->Name(), node->AsClassDefinition()->InternalName()};
    }
    if (node->IsTSInterfaceDeclaration()) {
        return {node->AsTSInterfaceDeclaration()->Id()->Name(), node->AsTSInterfaceDeclaration()->InternalName()};
    }
    return {node->AsAnnotationDeclaration()->GetBaseName()->Name(), node->AsAnnotationDeclaration()->InternalName()};
}

ETSObjectType *ETSObjectType::CreateETSObjectType(ir::AstNode *declNode, ETSObjectFlags flags)
{
    auto const [name, internalName] = GetObjectTypeDeclNames(declNode);

    if (declNode->IsClassDefinition() && (declNode->AsClassDefinition()->IsEnumTransformed())) {
        if (declNode->AsClassDefinition()->IsIntEnumTransformed()) {
            return Allocator()->New<ETSIntEnumType>(Allocator(), name, internalName, declNode, GetRelation());
        }
        ES2PANDA_ASSERT(declNode->AsClassDefinition()->IsStringEnumTransformed());
        return Allocator()->New<ETSStringEnumType>(Allocator(), name, internalName, declNode, GetRelation());
    }
    if (internalName == compiler::Signatures::BUILTIN_ARRAY) {
        return Allocator()->New<ETSResizableArrayType>(Allocator(), name,
                                                       std::make_tuple(declNode, flags, GetRelation()));
    }

    return Allocator()->New<ETSObjectType>(Allocator(), name, internalName,
                                           std::make_tuple(declNode, flags, GetRelation()));
}

// #22951: remove isExtensionFunctionType flag
ETSObjectType *ETSObjectType::Substitute(TypeRelation *relation, const Substitution *substitution, bool cache,
                                         bool isExtensionFunctionType)
{
    relation = relation_;
    if (substitution == nullptr || substitution->empty()) {
        return this;
    }

    auto *base = GetOriginalBaseType();

    ArenaVector<Type *> newTypeArgs {allocator_->Adapter()};
    const bool anyChange = SubstituteTypeArgs(relation, newTypeArgs, substitution);
    // Lambda types can capture type params in their bodies, normal classes cannot.
    // NOTE: gogabr. determine precise conditions where we do not need to copy.
    // Perhaps keep track of captured type parameters for each type.
    if (!anyChange && !HasObjectFlag(ETSObjectFlags::FUNCTIONAL)) {
        return this;
    }

    const util::StringView hash = GetHashFromSubstitution(substitution, isExtensionFunctionType, allocator_);
    if (cache) {
        if (auto *inst = GetInstantiatedType(hash); inst != nullptr) {
            return inst;
        }
    }

    if (!relation->IsAtTypeDepthLimit(base)) {
        return this;
    }
    relation->IncreaseTypeRecursionCount(base);

    auto *const copiedType = CreateETSObjectType(declNode_, flags_);
    SetCopiedTypeProperties(relation, copiedType, std::move(newTypeArgs), base);
    if (isExtensionFunctionType) {
        copiedType->AddObjectFlag(checker::ETSObjectFlags::EXTENSION_FUNCTION);
    }

    if (cache) {
        ES2PANDA_ASSERT(copiedType->GetRelation());
        InsertInstantiationMap(hash, copiedType);
    }

    if (superType_ != nullptr) {
        copiedType->SetSuperType(superType_->Substitute(relation, substitution)->AsETSObjectType());
    }
    for (auto *itf : interfaces_) {
        auto *newItf = itf->Substitute(relation, substitution)->AsETSObjectType();
        copiedType->AddInterface(newItf);
    }

    relation->DecreaseTypeRecursionCount(base);

    return copiedType;
}

ETSObjectType *ETSObjectType::Substitute(TypeRelation *relation, const Substitution *substitution)
{
    return Substitute(relation, substitution, true);
}

ETSObjectType *ETSObjectType::SubstituteArguments(TypeRelation *relation, ArenaVector<Type *> const &arguments)
{
    if (arguments.empty()) {
        return this;
    }

    auto *checker = relation->GetChecker()->AsETSChecker();
    auto substitution = Substitution {};

    ES2PANDA_ASSERT(baseType_ == nullptr);
    ES2PANDA_ASSERT(typeArguments_.size() == arguments.size());

    for (size_t ix = 0; ix < typeArguments_.size(); ix++) {
        substitution.emplace(typeArguments_[ix]->AsETSTypeParameter(),
                             checker->MaybeBoxType(arguments[ix]->MaybeBaseTypeOfGradualType()));
    }

    return Substitute(relation, &substitution);
}

ETSChecker *ETSObjectType::GetETSChecker()
{
    return relation_->GetChecker()->AsETSChecker();
}

void ETSObjectType::CheckAndInstantiateProperties() const
{
    auto *checker = relation_->GetChecker()->AsETSChecker();
    auto *declNode = GetDeclNode();
    if (HasObjectFlag(ETSObjectFlags::BUILTIN_TYPE) && declNode == nullptr) {
        declNode = SuperType()->GetDeclNode();
    }
    if (declNode == nullptr) {
        ES2PANDA_ASSERT(checker->IsAnyError());
        return;
    }

    TypeStackElement tse {checker, this, {{diagnostic::CYCLIC_INHERITANCE, {this->Name()}}}, declNode->Start()};
    if (tse.HasTypeError()) {
        return;
    }
    InstantiateProperties();
}

void ETSObjectType::InstantiateProperties() const
{
    ES2PANDA_ASSERT(relation_ != nullptr);
    auto *checker = relation_->GetChecker()->AsETSChecker();

    if (baseType_ == nullptr || baseType_ == this) {
        checker->ResolveDeclaredMembersOfObject(this);
        return;
    }

    ES2PANDA_ASSERT(!propertiesInstantiated_);
    declNode_->Check(checker);

    auto subst = effectiveSubstitution_ == nullptr
                     ? Substitution {}
                     : ETSChecker::ArenaSubstitutionToSubstitution(effectiveSubstitution_);

    for (auto *const it : baseType_->ConstructSignatures()) {
        auto *newSig = it->Substitute(relation_, &subst);
        constructSignatures_.push_back(newSig);
    }

    for (auto const &[_, prop] : baseType_->InstanceFields()) {
        (void)_;
        auto *copiedProp = CopyPropertyWithTypeArguments(prop, relation_, &subst);
        properties_[static_cast<size_t>(PropertyType::INSTANCE_FIELD)].emplace(prop->Name(), copiedProp);
    }

    for (auto const &[_, prop] : baseType_->StaticFields()) {
        (void)_;
        auto *copiedProp = CopyPropertyWithTypeArguments(prop, relation_, &subst);
        properties_[static_cast<size_t>(PropertyType::STATIC_FIELD)].emplace(prop->Name(), copiedProp);
    }

    for (auto const &[_, prop] : baseType_->InstanceMethods()) {
        (void)_;
        auto *copiedProp = CopyPropertyWithTypeArguments(prop, relation_, &subst);
        properties_[static_cast<size_t>(PropertyType::INSTANCE_METHOD)].emplace(prop->Name(), copiedProp);
    }

    for (auto const &[_, prop] : baseType_->StaticMethods()) {
        (void)_;
        auto *copiedProp = CopyPropertyWithTypeArguments(prop, relation_, &subst);
        properties_[static_cast<size_t>(PropertyType::STATIC_METHOD)].emplace(prop->Name(), copiedProp);
    }

    for (auto const &[_, prop] : baseType_->InstanceDecls()) {
        (void)_;
        auto *copiedProp = CopyPropertyWithTypeArguments(prop, relation_, &subst);
        properties_[static_cast<size_t>(PropertyType::INSTANCE_DECL)].emplace(prop->Name(), copiedProp);
    }

    for (auto const &[_, prop] : baseType_->StaticDecls()) {
        (void)_;
        auto *copiedProp = CopyPropertyWithTypeArguments(prop, relation_, &subst);
        properties_[static_cast<size_t>(PropertyType::STATIC_DECL)].emplace(prop->Name(), copiedProp);
    }
}

std::string ETSObjectType::NameToDescriptor(util::StringView name)
{
    auto desc = std::string(compiler::Signatures::CLASS_REF_BEGIN)
                    .append(name.Utf8())
                    .append(std::string(compiler::Signatures::MANGLE_SEPARATOR));
    std::replace(desc.begin(), desc.end(), *compiler::Signatures::METHOD_SEPARATOR.begin(),
                 *compiler::Signatures::NAMESPACE_SEPARATOR.begin());
    return desc;
}

std::uint32_t ETSObjectType::GetPrecedence(checker::ETSChecker *checker, ETSObjectType const *type) noexcept
{
    ES2PANDA_ASSERT(type != nullptr);
    if (type->HasObjectFlag(ETSObjectFlags::BUILTIN_BYTE)) {
        return 1U;
    }
    if (type->HasObjectFlag(ETSObjectFlags::BUILTIN_CHAR)) {
        return 2U;
    }
    if (type->HasObjectFlag(ETSObjectFlags::BUILTIN_SHORT)) {
        return 3U;
    }
    if (type->HasObjectFlag(ETSObjectFlags::BUILTIN_INT)) {
        return 4U;
    }
    if (type->HasObjectFlag(ETSObjectFlags::BUILTIN_LONG)) {
        return 5U;
    }
    if (checker->Relation()->IsIdenticalTo(const_cast<ETSObjectType *>(type),
                                           checker->GetGlobalTypesHolder()->GlobalIntegralBuiltinType())) {
        return 5U;
    }
    if (type->HasObjectFlag(ETSObjectFlags::BUILTIN_FLOAT)) {
        return 6U;
    }
    if (type->HasObjectFlag(ETSObjectFlags::BUILTIN_DOUBLE)) {
        return 7U;
    }
    if (checker->Relation()->IsIdenticalTo(const_cast<ETSObjectType *>(type),
                                           checker->GetGlobalTypesHolder()->GlobalFloatingBuiltinType())) {
        return 7U;
    }
    if (type->HasObjectFlag(ETSObjectFlags::BUILTIN_BIGINT)) {
        return 8U;
    }
    return 0U;
}
void ETSObjectType::AddReExports(ETSObjectType *reExport)
{
    if (std::find(reExports_.begin(), reExports_.end(), reExport) == reExports_.end()) {
        reExports_.push_back(reExport);
    }
}

void ETSObjectType::AddReExportAlias(util::StringView const &value, util::StringView const &key)
{
    reExportAlias_.insert({key, value});
}

util::StringView ETSObjectType::GetReExportAliasValue(util::StringView const &key) const
{
    auto ret = reExportAlias_.find(key);
    if (reExportAlias_.end() == ret) {
        return key;
    }
    return ret->second;
}

bool ETSObjectType::IsReExportHaveAliasValue(util::StringView const &key) const
{
    return std::any_of(reExportAlias_.begin(), reExportAlias_.end(),
                       [&](const auto &val) { return val.second == key; });
}

const ArenaVector<ETSObjectType *> &ETSObjectType::ReExports() const
{
    return reExports_;
}

void ETSObjectType::ToAssemblerType([[maybe_unused]] std::stringstream &ss) const
{
    ss << internalName_;
}

void ETSObjectType::ToDebugInfoType(std::stringstream &ss) const
{
    ss << NameToDescriptor(internalName_);
}

void ETSObjectType::ToDebugInfoSignatureType(std::stringstream &ss) const
{
    ss << compiler::Signatures::GENERIC_BEGIN;
    ss << internalName_;
    ss << compiler::Signatures::GENERIC_END;
}

ir::TSTypeParameterDeclaration *ETSObjectType::GetTypeParams() const
{
    if (HasObjectFlag(ETSObjectFlags::ENUM) || !HasTypeFlag(TypeFlag::GENERIC)) {
        return nullptr;
    }

    if (HasObjectFlag(ETSObjectFlags::CLASS)) {
        ES2PANDA_ASSERT(declNode_->IsClassDefinition() && declNode_->AsClassDefinition()->TypeParams());
        return declNode_->AsClassDefinition()->TypeParams();
    }

    ES2PANDA_ASSERT(declNode_->IsTSInterfaceDeclaration() && declNode_->AsTSInterfaceDeclaration()->TypeParams());
    return declNode_->AsTSInterfaceDeclaration()->TypeParams();
}

bool ETSObjectType::IsSameBasedGeneric(TypeRelation *relation, Type const *other) const
{
    const_cast<ETSObjectType *>(this)->IdenticalUptoTypeArguments(relation, const_cast<Type *>(other));
    return relation->IsTrue();
}

void ETSObjectType::CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag)
{
    if (HasObjectFlag(ETSObjectFlags::FUNCTIONAL)) {
        relation->CheckVarianceRecursively(GetFunctionalInterfaceInvokeType(), varianceFlag);
        return;
    }

    // according to the spec(GENERICS chapter), only class/interface/function/
    // method/lambda and type alias can have type parameters. since
    // 1. the type of function and method is ETSFunctionType
    // 2. lambda has been checked above
    // here we just need check
    // 1. class
    // 2. interface
    // 3. type alias(which will be redirected to its real type)
    // And all of them should have declarations
    if (declNode_ == nullptr) {
        // If the type is not declared, then we do not need to check variance.
        return;
    }
    ir::TSTypeParameterDeclaration *params;
    if (GetDeclNode()->IsClassDefinition()) {
        params = GetDeclNode()->AsClassDefinition()->TypeParams();
    } else if (GetDeclNode()->IsTSInterfaceDeclaration()) {
        params = GetDeclNode()->AsTSInterfaceDeclaration()->TypeParams();
    } else {
        // If the type is not a class or interface or type alias, then we do not need to check variance.
        return;
    }

    if (params == nullptr) {
        return;
    }

    auto typeArgs = TypeArguments();
    for (size_t i = 0; i < typeArgs.size(); ++i) {
        // If the Variance of type Args is the same as the Variance of type params, then the class is Covariant.
        // If the Variance of type Args is the opposite of the Variance of type params, then the class is
        // Contravariant.
        auto param = params->Params().at(i);
        relation->CheckVarianceRecursively(
            typeArgs.at(i), relation->TransferVariant(varianceFlag, param->IsIn()    ? VarianceFlag::CONTRAVARIANT
                                                                    : param->IsOut() ? VarianceFlag::COVARIANT
                                                                                     : VarianceFlag::INVARIANT));
    }
}

ETSObjectType *ETSObjectType::GetInstantiatedType(util::StringView hash)
{
    auto &instantiationMap =
        compiler::GetPhaseManager()->Context()->GetChecker()->AsETSChecker()->GetObjectInstantiationMap();
    auto found = instantiationMap.find(this);
    if (found == instantiationMap.end()) {
        return nullptr;
    }

    auto found2 = instantiationMap.at(this).find(hash);
    if (found2 == instantiationMap.at(this).end()) {
        return nullptr;
    }

    return found2->second;
}

void ETSObjectType::InsertInstantiationMap(util::StringView key, ETSObjectType *value)
{
    auto &instantiationMap =
        compiler::GetPhaseManager()->Context()->GetChecker()->AsETSChecker()->GetObjectInstantiationMap();
    if (instantiationMap.find(this) == instantiationMap.end()) {
        ArenaUnorderedMap<util::StringView, ETSObjectType *> instantiation(
            compiler::GetPhaseManager()->Context()->GetChecker()->AsETSChecker()->Allocator()->Adapter());
        instantiation.emplace(key, value);
        instantiationMap.emplace(this, std::move(instantiation));
    }
    compiler::GetPhaseManager()
        ->Context()
        ->GetChecker()
        ->AsETSChecker()
        ->GetObjectInstantiationMap()
        .at(this)
        .try_emplace(key, value);
}

}  // namespace ark::es2panda::checker
