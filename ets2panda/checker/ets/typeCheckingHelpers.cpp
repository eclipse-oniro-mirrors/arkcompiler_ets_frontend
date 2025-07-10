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

#include "checker/checker.h"
#include "checker/checkerContext.h"
#include "checker/ets/wideningConverter.h"
#include "checker/types/globalTypesHolder.h"
#include "checker/types/gradualType.h"
#include "checker/types/ets/etsObjectType.h"
#include "checker/types/ets/etsPartialTypeParameter.h"
#include "ir/base/catchClause.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/statements/variableDeclarator.h"
#include "ir/statements/switchCaseStatement.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/memberExpression.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/statements/labelledStatement.h"
#include "ir/statements/tryStatement.h"
#include "ir/ets/etsNewClassInstanceExpression.h"
#include "ir/ets/etsParameterExpression.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "ir/ts/tsEnumMember.h"
#include "ir/ts/tsTypeParameter.h"
#include "ir/ets/etsUnionType.h"
#include "ir/ets/etsTuple.h"
#include "varbinder/declaration.h"
#include "checker/ETSchecker.h"
#include "varbinder/ETSBinder.h"
#include "checker/ets/typeRelationContext.h"
#include "checker/ets/boxingConverter.h"
#include "checker/ets/unboxingConverter.h"
#include "util/helpers.h"
#include "generated/diagnostic.h"

namespace ark::es2panda::checker {

void ETSChecker::CheckTruthinessOfType(ir::Expression *expr)
{
    auto const testType = expr->Check(this);
    auto *const conditionType = MaybeUnboxConditionalInRelation(testType);

    expr->SetTsType(conditionType);

    if (conditionType == nullptr) {
        return;
    }
    expr->SetTsType(MaybeBoxType(conditionType));

    if (conditionType->IsETSVoidType()) {
        LogError(diagnostic::VOID_IN_LOGIC, {}, expr->Start());
        return;
    }

    // For T_S compatibility
    if (conditionType->IsETSEnumType()) {
        expr->AddAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
    }
}

bool ETSChecker::CheckNonNullish(ir::Expression const *expr)
{
    if (!expr->TsType()->PossiblyETSNullish()) {
        return true;
    }

    if (expr->TsType()->IsETSPartialTypeParameter()) {
        return true;
    }

    if (HasStatus(checker::CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK)) {
        return false;
    }

    LogError(diagnostic::POSSIBLY_NULLISH, {}, expr->Start());
    return false;
}

Type *ETSChecker::GetNonNullishType(Type *type)
{
    if (type->IsGradualType()) {
        return CreateGradualType(GetNonNullishType(type->AsGradualType()->GetBaseType()),
                                 type->AsGradualType()->Language());
    }
    if (type->DefinitelyNotETSNullish()) {
        return type;
    }
    if (type->IsETSAnyType()) {
        return type;
    }
    if (type->IsETSTypeParameter()) {
        return ProgramAllocator()->New<ETSNonNullishType>(type->AsETSTypeParameter());
    }
    if (type->IsETSPartialTypeParameter()) {
        return type->AsETSPartialTypeParameter()->GetUnderlying();
    }
    if (type->IsETSNullType() || type->IsETSUndefinedType()) {
        return GetGlobalTypesHolder()->GlobalETSNeverType();
    }

    ArenaVector<Type *> copied(ProgramAllocator()->Adapter());
    for (auto const &t : type->AsETSUnionType()->ConstituentTypes()) {
        if (t->IsETSNullType() || t->IsETSUndefinedType()) {
            continue;
        }
        copied.push_back(GetNonNullishType(t));
    }
    return copied.empty() ? GetGlobalTypesHolder()->GlobalETSNeverType() : CreateETSUnionType(std::move(copied));
}

Type *ETSChecker::RemoveNullType(Type *const type)
{
    if (type->IsETSAnyType() || type->DefinitelyNotETSNullish() || type->IsETSUndefinedType()) {
        return type;
    }

    if (type->IsETSPartialTypeParameter()) {
        return type;
    }

    if (type->IsETSTypeParameter()) {
        // Strict equality produces incorrect NonNullish types #21526
        return type;
    }

    if (type->IsETSNullType()) {
        return GetGlobalTypesHolder()->GlobalETSNeverType();
    }

    ES2PANDA_ASSERT(type->IsETSUnionType());
    ArenaVector<Type *> copiedTypes(ProgramAllocator()->Adapter());

    for (auto *constituentType : type->AsETSUnionType()->ConstituentTypes()) {
        if (!constituentType->IsETSNullType()) {
            copiedTypes.push_back(RemoveNullType(constituentType));
        }
    }

    return copiedTypes.empty() ? GetGlobalTypesHolder()->GlobalETSNeverType()
                               : CreateETSUnionType(std::move(copiedTypes));
}

Type *ETSChecker::RemoveUndefinedType(Type *const type)
{
    if (type->IsETSAnyType() || type->DefinitelyNotETSNullish() || type->IsETSNullType()) {
        return type;
    }

    if (type->IsETSTypeParameter()) {
        // Strict equality produces incorrect NonNullish types #21526
        return type;
    }

    if (type->IsETSPartialTypeParameter()) {
        return type;
    }

    if (type->IsETSUndefinedType()) {
        return GetGlobalTypesHolder()->GlobalETSNeverType();
    }

    ES2PANDA_ASSERT(type->IsETSUnionType());
    ArenaVector<Type *> copiedTypes(ProgramAllocator()->Adapter());

    for (auto *constituentType : type->AsETSUnionType()->ConstituentTypes()) {
        if (!constituentType->IsETSUndefinedType()) {
            copiedTypes.push_back(RemoveUndefinedType(constituentType));
        }
    }

    return copiedTypes.empty() ? GetGlobalTypesHolder()->GlobalETSNeverType()
                               : CreateETSUnionType(std::move(copiedTypes));
}

std::pair<Type *, Type *> ETSChecker::RemoveNullishTypes(Type *type)
{
    if (type->DefinitelyNotETSNullish()) {
        return {GetGlobalTypesHolder()->GlobalETSNeverType(), type};
    }

    if (type->IsETSAnyType()) {
        return {type, type};
    }

    if (type->IsETSPartialTypeParameter()) {
        return {GetGlobalTypesHolder()->GlobalETSNeverType(),
                ProgramAllocator()->New<ETSNonNullishType>(type->AsETSPartialTypeParameter()->GetUnderlying())};
    }

    if (type->IsETSTypeParameter()) {
        return {GetGlobalTypesHolder()->GlobalETSUnionUndefinedNull(),
                ProgramAllocator()->New<ETSNonNullishType>(type->AsETSTypeParameter())};
    }

    if (type->IsETSUndefinedType() || type->IsETSNullType()) {
        return {type, GetGlobalTypesHolder()->GlobalETSNeverType()};
    }

    ES2PANDA_ASSERT(type->IsETSUnionType());
    ArenaVector<Type *> nullishTypes(ProgramAllocator()->Adapter());
    ArenaVector<Type *> notNullishTypes(ProgramAllocator()->Adapter());

    for (auto *ctype : type->AsETSUnionType()->ConstituentTypes()) {
        auto constituentType = ctype->MaybeBaseTypeOfGradualType();
        if (constituentType->IsETSUndefinedType() || constituentType->IsETSNullType()) {
            nullishTypes.push_back(constituentType);
        } else {
            notNullishTypes.push_back(
                !constituentType->IsETSTypeParameter()
                    ? constituentType
                    : ProgramAllocator()->New<ETSNonNullishType>(constituentType->AsETSTypeParameter()));
        }
    }

    Type *nullishType = nullishTypes.empty() ? GetGlobalTypesHolder()->GlobalETSNeverType()
                                             : CreateETSUnionType(std::move(nullishTypes));
    Type *notNullishType = notNullishTypes.empty() ? GetGlobalTypesHolder()->GlobalETSNeverType()
                                                   : CreateETSUnionType(std::move(notNullishTypes));
    return {nullishType, notNullishType};
}

// NOTE(vpukhov): can be implemented with relation if etscompiler will support it
template <typename Pred, typename Trv>
static bool MatchConstituentOrConstraint(const Type *type, Pred const &pred, Trv const &trv)
{
    if (type->IsGradualType()) {
        return MatchConstituentOrConstraint(type->AsGradualType()->GetBaseType(), pred, trv);
    }
    auto const traverse = [&pred, &trv](const Type *ttype) {
        return MatchConstituentOrConstraint<Pred, Trv>(ttype, pred, trv);
    };
    if (pred(type)) {
        return true;
    }
    if (!trv(type)) {
        return false;
    }
    if (type->IsETSUnionType()) {
        for (auto const &ctype : type->AsETSUnionType()->ConstituentTypes()) {
            if (traverse(ctype)) {
                return true;
            }
        }
        return false;
    }
    if (type->IsETSTypeParameter()) {
        return traverse(type->AsETSTypeParameter()->GetConstraintType());
    }
    if (type->IsETSNonNullishType()) {
        auto tparam = type->AsETSNonNullishType()->GetUnderlying();
        return traverse(tparam->GetConstraintType());
    }
    if (type->IsETSPartialTypeParameter()) {
        auto tparam = type->AsETSPartialTypeParameter()->GetUnderlying();
        return traverse(tparam->GetConstraintType());
    }
    return false;
}

template <typename Pred>
static bool MatchConstituentOrConstraint(const Type *type, Pred const &pred)
{
    return MatchConstituentOrConstraint(type, pred, []([[maybe_unused]] const Type *t) { return true; });
}

bool Type::PossiblyETSNull() const
{
    return MatchConstituentOrConstraint(
        this, [](const Type *t) { return t->IsETSAnyType() || t->IsETSNullType(); },
        [](const Type *t) { return !t->IsETSNonNullishType(); });
}

bool Type::PossiblyETSUndefined() const
{
    return MatchConstituentOrConstraint(
        this, [](const Type *t) { return t->IsETSAnyType() || t->IsETSUndefinedType(); },
        [](const Type *t) { return !t->IsETSNonNullishType(); });
}

static bool ObjectPossiblyInForeignDomain(ETSObjectType const *type)
{
    if (type->IsGlobalETSObjectType()) {
        return true;
    }
    auto dnode = type->GetDeclNode();
    if (dnode == nullptr) {
        return false;
    }
    auto lang = dnode->IsClassDefinition() ? dnode->AsClassDefinition()->Language()
                                           : dnode->AsTSInterfaceDeclaration()->Language();
    return lang != Language::Id::ETS;
}

bool Type::PossiblyInForeignDomain() const
{
    return MatchConstituentOrConstraint(
        this,
        [](const Type *t) {
            return t->IsETSAnyType() || (t->IsETSObjectType() && ObjectPossiblyInForeignDomain(t->AsETSObjectType()));
        },
        []([[maybe_unused]] const Type *t) { return true; });
}

bool Type::PossiblyETSNullish() const
{
    return MatchConstituentOrConstraint(
        this, [](const Type *t) { return t->IsETSAnyType() || t->IsETSNullType() || t->IsETSUndefinedType(); },
        [](const Type *t) { return !t->IsETSNonNullishType(); });
}

bool Type::DefinitelyETSNullish() const
{
    return !MatchConstituentOrConstraint(
        this,
        [](const Type *t) {
            return !(t->IsTypeParameter() || t->IsETSUnionType() || t->IsETSNullType() || t->IsETSUndefinedType());
        },
        [](const Type *t) { return !t->IsETSNonNullishType(); });
}

bool Type::DefinitelyNotETSNullish() const
{
    return !PossiblyETSNullish();
}

bool Type::PossiblyETSString() const
{
    return MatchConstituentOrConstraint(this, [](const Type *t) {
        return t->IsETSAnyType() || t->IsETSStringType() ||
               (t->IsETSObjectType() && t->AsETSObjectType()->IsGlobalETSObjectType());
    });
}

static bool IsValueTypedObjectType(ETSObjectType const *t)
{
    ETSObjectFlags flags = ETSObjectFlags::FUNCTIONAL_REFERENCE | ETSObjectFlags::VALUE_TYPED |
                           ETSObjectFlags::ENUM_OBJECT | ETSObjectFlags::FUNCTIONAL;
    return t->IsGlobalETSObjectType() || t->HasObjectFlag(flags);
}

bool Type::PossiblyETSValueTyped() const
{
    return MatchConstituentOrConstraint(this,
                                        [](const Type *t) { return t->IsETSNullType() || t->IsETSUndefinedType(); }) ||
           PossiblyETSValueTypedExceptNullish();
}

bool Type::PossiblyETSValueTypedExceptNullish() const
{
    return MatchConstituentOrConstraint(this, [](const Type *t) {
        return t->IsETSAnyType() || t->IsETSFunctionType() ||
               (t->IsETSObjectType() && IsValueTypedObjectType(t->AsETSObjectType()));
    });
}

bool Type::IsETSArrowType() const
{
    // Arrow types (in form `(p: A) => B`) will be isolated from Methods
    return IsETSFunctionType() && !IsETSMethodType();
}

bool Type::IsETSMethodType() const
{
    return HasTypeFlag(TypeFlag::ETS_METHOD);
}

[[maybe_unused]] static bool IsSaneETSReferenceType(Type const *type)
{
    static constexpr TypeFlag ETS_SANE_REFERENCE_TYPE =
        TypeFlag::TYPE_ERROR | TypeFlag::ETS_NULL | TypeFlag::ETS_UNDEFINED | TypeFlag::ETS_OBJECT |
        TypeFlag::ETS_TYPE_PARAMETER | TypeFlag::WILDCARD | TypeFlag::ETS_NONNULLISH |
        TypeFlag::ETS_REQUIRED_TYPE_PARAMETER | TypeFlag::ETS_ANY | TypeFlag::ETS_NEVER | TypeFlag::ETS_UNION |
        TypeFlag::ETS_ARRAY | TypeFlag::FUNCTION | TypeFlag::ETS_PARTIAL_TYPE_PARAMETER | TypeFlag::ETS_TUPLE |
        TypeFlag::ETS_ENUM | TypeFlag::ETS_READONLY | TypeFlag::GRADUAL_TYPE | TypeFlag::ETS_AWAITED;

    // Issues
    if (type->IsETSVoidType()) {  // NOTE(vpukhov): #19701 void refactoring
        return true;
    }
    if (type->IsETSTypeAliasType()) {  // NOTE(vpukhov): #20561
        return true;
    }
    return type->HasTypeFlag(ETS_SANE_REFERENCE_TYPE);
}

bool Type::IsETSPrimitiveType() const
{
    static constexpr TypeFlag ETS_PRIMITIVE = TypeFlag::ETS_NUMERIC | TypeFlag::CHAR | TypeFlag::ETS_BOOLEAN;

    // Do not modify
    ES2PANDA_ASSERT(!HasTypeFlag(ETS_PRIMITIVE) == IsSaneETSReferenceType(this));
    return HasTypeFlag(ETS_PRIMITIVE);
}

bool Type::IsETSPrimitiveOrEnumType() const
{
    return IsETSPrimitiveType() || IsETSEnumType();
}

bool Type::IsETSReferenceType() const
{
    // Do not modify
    return !IsETSPrimitiveType();
}

bool Type::IsETSUnboxableObject() const
{
    return IsETSObjectType() && AsETSObjectType()->HasObjectFlag(ETSObjectFlags::UNBOXABLE_TYPE);
}

bool ETSChecker::IsConstantExpression(ir::Expression *expr, Type *type)
{
    return (type->HasTypeFlag(TypeFlag::CONSTANT) && (expr->IsIdentifier() || expr->IsMemberExpression()));
}

Type *ETSChecker::GetNonConstantType(Type *type)
{
    ES2PANDA_ASSERT(type != nullptr);
    if (type->IsETSStringType()) {
        return GlobalBuiltinETSStringType();
    }

    if (type->IsETSBigIntType()) {
        return GlobalETSBigIntType();
    }

    if (type->IsETSUnionType()) {
        return CreateETSUnionType(type->AsETSUnionType()->GetNonConstantTypes(this));
    }

    if (!type->IsETSPrimitiveType()) {
        if (type->IsETSObjectType() && type->AsETSObjectType()->IsBoxedPrimitive()) {
            type->RemoveTypeFlag(TypeFlag::CONSTANT);
        }
        return type;
    }

    if (type->HasTypeFlag(TypeFlag::LONG)) {
        return GlobalLongType();
    }

    if (type->HasTypeFlag(TypeFlag::BYTE)) {
        return GlobalByteType();
    }

    if (type->HasTypeFlag(TypeFlag::SHORT)) {
        return GlobalShortType();
    }

    if (type->HasTypeFlag(TypeFlag::CHAR)) {
        return GlobalCharType();
    }

    if (type->HasTypeFlag(TypeFlag::INT)) {
        return GlobalIntType();
    }

    if (type->HasTypeFlag(TypeFlag::FLOAT)) {
        return GlobalFloatType();
    }

    if (type->HasTypeFlag(TypeFlag::DOUBLE)) {
        return GlobalDoubleType();
    }

    if (type->IsETSBooleanType()) {
        return GlobalETSBooleanType();
    }
    return type;
}

Type *ETSChecker::GetTypeOfSetterGetter(varbinder::Variable *const var)
{
    auto *propType = var->TsType()->AsETSFunctionType();
    if (propType->HasTypeFlag(checker::TypeFlag::GETTER)) {
        auto *getter = propType->FindGetter();
        ES2PANDA_ASSERT(getter != nullptr);
        return getter->ReturnType();
    }

    if (propType->FindSetter()->Params().empty()) {
        var->SetTsType(GlobalTypeError());
        return GlobalTypeError();
    }

    return propType->FindSetter()->Params()[0]->TsType();
}

Type *ETSChecker::CreateSyntheticTypeFromOverload(varbinder::Variable *const var)
{
    auto *overloadDeclaration = var->Declaration()->Node()->AsOverloadDeclaration();
    std::vector<Signature *> signatures;
    ETSFunctionType *syntheticFunctionType =
        CreateETSMethodType(overloadDeclaration->Id()->Name(), {{}, Allocator()->Adapter()});

    for (auto *overloadFunction : overloadDeclaration->OverloadedList()) {
        Type *functionType = overloadFunction->Check(this);
        if (functionType->IsTypeError()) {
            overloadDeclaration->SetTsType(GetGlobalTypesHolder()->GlobalTypeError());
            return GetGlobalTypesHolder()->GlobalTypeError();
        }
        ES2PANDA_ASSERT(functionType->IsETSFunctionType());
        auto *signature = functionType->AsETSFunctionType()->CallSignatures().front();
        if (std::find(signatures.begin(), signatures.end(), signature) != signatures.end()) {
            continue;
        }
        signatures.emplace_back(signature);
    }

    for (auto &s : signatures) {
        syntheticFunctionType->AddCallSignature(s);
    }

    syntheticFunctionType->SetVariable(var);
    var->SetTsType(syntheticFunctionType);
    overloadDeclaration->SetTsType(syntheticFunctionType);
    return syntheticFunctionType;
}

SavedCheckerContext ETSChecker::CreateSavedCheckerContext(varbinder::Variable *const var)
{
    // Before computing the given variables type, we have to make a new checker context frame so that the checking is
    // done in the proper context, and have to enter the scope where the given variable is declared, so reference
    // resolution works properly
    auto constexpr STATUS = CheckerStatus::NO_OPTS;
    for (auto *iter = var->Declaration()->Node()->Parent(); iter != nullptr; iter = iter->Parent()) {
        if (iter->IsMethodDefinition()) {
            auto *methodDef = iter->AsMethodDefinition();
            ES2PANDA_ASSERT(methodDef->TsType());
            auto *func = methodDef->Function();
            ES2PANDA_ASSERT(func != nullptr);
            return SavedCheckerContext(this, STATUS, nullptr, func->Signature());
        }
        if (iter->IsClassDefinition()) {
            auto *classDef = iter->AsClassDefinition();
            Type *containingClass {};
            if (classDef->TsType() == nullptr) {
                containingClass = BuildBasicClassProperties(classDef)->MaybeBaseTypeOfGradualType();
                ResolveDeclaredMembersOfObject(containingClass);
            } else {
                containingClass = classDef->TsType()->MaybeBaseTypeOfGradualType()->AsETSObjectType();
            }
            ES2PANDA_ASSERT(classDef->TsType());
            if (!containingClass->IsTypeError()) {
                return SavedCheckerContext(this, STATUS, containingClass->AsETSObjectType());
            }
        }
    }
    return SavedCheckerContext(this, STATUS);
}

static Type *GetTypeFromVarLikeVariableDeclaration(ETSChecker *checker, varbinder::Variable *const var)
{
    if (var->TsType() != nullptr) {
        return var->TsType();
    }

    auto *declNode = var->Declaration()->Node();
    if (var->Declaration()->Node()->IsIdentifier()) {
        declNode = declNode->Parent();
    }
    TypeStackElement tse(
        checker, var->Declaration(),
        // OHOS CC thinks initializer lists are statement blocks...
        // CC-OFFNXT(G.FMT.03-CPP) project code style
        {{diagnostic::CIRCULAR_DEPENDENCY, util::DiagnosticMessageParams {var->Declaration()->Name()}}},
        declNode->Start());
    if (tse.HasTypeError()) {
        var->SetTsType(checker->GlobalTypeError());
        return checker->GlobalTypeError();
    }
    return var->SetTsType(declNode->Check(checker));
}

Type *ETSChecker::GetTypeFromVariableDeclaration(varbinder::Variable *const var)
{
    Type *variableType = nullptr;

    switch (var->Declaration()->Type()) {
        case varbinder::DeclType::CLASS: {
            auto *classDef = var->Declaration()->Node()->AsClassDefinition();
            BuildBasicClassProperties(classDef);
            variableType = classDef->TsType();
            break;
        }
        case varbinder::DeclType::CONST:
            [[fallthrough]];
        case varbinder::DeclType::READONLY:
            [[fallthrough]];
        case varbinder::DeclType::LET:
            [[fallthrough]];
        case varbinder::DeclType::VAR: {
            variableType = GetTypeFromVarLikeVariableDeclaration(this, var);
            break;
        }

        case varbinder::DeclType::FUNC:
            [[fallthrough]];
        case varbinder::DeclType::IMPORT:
            variableType = var->Declaration()->Node()->Check(this);
            break;

        case varbinder::DeclType::TYPE_ALIAS:
            variableType = GetTypeFromTypeAliasReference(var);
            break;

        case varbinder::DeclType::INTERFACE:
            variableType = BuildBasicInterfaceProperties(var->Declaration()->Node()->AsTSInterfaceDeclaration());
            break;

        case varbinder::DeclType::ANNOTATIONUSAGE:
            [[fallthrough]];
        case varbinder::DeclType::ANNOTATIONDECL:
            break;

        default:
            ES2PANDA_ASSERT(IsAnyError());
            break;
    }

    return variableType != nullptr ? variableType : GlobalTypeError();
}

Type *ETSChecker::GetTypeOfVariable(varbinder::Variable *const var)
{
    if (IsVariableExtensionAccessor(var)) {
        return var->TsType();
    }

    if (IsVariableGetterSetter(var)) {
        return GetTypeOfSetterGetter(var);
    }

    if (var->TsType() != nullptr) {
        return var->TsType();
    }

    checker::SavedCheckerContext savedContext = CreateSavedCheckerContext(var);
    checker::ScopeContext scopeCtx(this, var->GetScope());

    return GetTypeFromVariableDeclaration(var);
}

// Determine if unchecked cast is needed and yield guaranteed source type
Type *ETSChecker::GuaranteedTypeForUncheckedCast(Type *base, Type *substituted)
{
    // Apparent type acts as effective representation for type.
    //  For T extends SomeClass|undefined
    //  Apparent(Int|T|null) is Int|SomeClass|undefined|null
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *appBase = GetApparentType(base);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *appSubst = GetApparentType(substituted);
    // Base is supertype of Substituted AND Substituted is supertype of Base
    return Relation()->IsIdenticalTo(appSubst, appBase) ? nullptr : appBase;
}

// Determine if substituted property access requires cast from erased type
Type *ETSChecker::GuaranteedTypeForUncheckedPropertyAccess(varbinder::Variable *const prop)
{
    if (IsVariableStatic(prop)) {
        return nullptr;
    }

    if (prop->TsType() != nullptr && prop->TsType()->IsTypeError()) {
        return nullptr;
    }

    if (IsVariableGetterSetter(prop)) {
        auto *method = prop->TsType()->AsETSFunctionType();
        if (!method->HasTypeFlag(checker::TypeFlag::GETTER)) {
            return nullptr;
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return GuaranteedTypeForUncheckedCallReturn(method->FindGetter());
    }
    // NOTE(vpukhov): mark ETSDynamicType properties
    if (prop->Declaration() == nullptr || prop->Declaration()->Node() == nullptr) {
        return nullptr;
    }

    switch (auto node = prop->Declaration()->Node(); node->Type()) {
        case ir::AstNodeType::CLASS_PROPERTY: {
            auto *id = node->AsClassProperty()->Id();
            ES2PANDA_ASSERT(id != nullptr);
            auto baseProp = id->Variable();
            if (baseProp == prop) {
                return nullptr;
            }
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            return GuaranteedTypeForUncheckedCast(GetTypeOfVariable(baseProp), GetTypeOfVariable(prop));
        }
        case ir::AstNodeType::METHOD_DEFINITION:
        case ir::AstNodeType::CLASS_DEFINITION:
            return GetTypeOfVariable(prop);
        case ir::AstNodeType::OVERLOAD_DECLARATION:
        case ir::AstNodeType::TS_ENUM_DECLARATION:
            return nullptr;
        default:
            ES2PANDA_UNREACHABLE();
    }
}

// Determine if substituted method cast requires cast from erased type
Type *ETSChecker::GuaranteedTypeForUncheckedCallReturn(Signature *sig)
{
    ES2PANDA_ASSERT(sig != nullptr);
    ES2PANDA_ASSERT(sig->HasFunction());
    if (sig->HasSignatureFlag(SignatureFlags::THIS_RETURN_TYPE)) {
        return sig->ReturnType();
    }
    auto *const baseSig = sig->Function() != nullptr ? sig->Function()->Signature() : nullptr;
    if (baseSig == nullptr || baseSig == sig) {
        return nullptr;
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return GuaranteedTypeForUncheckedCast(MaybeBoxType(baseSig->ReturnType()), MaybeBoxType(sig->ReturnType()));
}

Type *ETSChecker::ResolveUnionUncheckedType(ArenaVector<checker::Type *> &&apparentTypes)
{
    if (apparentTypes.empty()) {
        return nullptr;
    }
    auto *unionType = CreateETSUnionType(std::move(apparentTypes));
    ES2PANDA_ASSERT(unionType != nullptr);
    if (unionType->IsETSUnionType()) {
        return unionType->AsETSUnionType();
    }
    // Is case of single apparent type, just return itself
    return unionType;
}

Type *ETSChecker::GuaranteedTypeForUnionFieldAccess(ir::MemberExpression *memberExpression, ETSUnionType *etsUnionType)
{
    const auto &types = etsUnionType->ConstituentTypes();
    ArenaVector<checker::Type *> apparentTypes {ProgramAllocator()->Adapter()};
    const auto *prop = memberExpression->Property();
    util::StringView propertyName;
    if (prop->IsIdentifier()) {
        propertyName = prop->AsIdentifier()->Name();
    } else if (prop->IsStringLiteral()) {
        propertyName = prop->AsStringLiteral()->Str();
    } else {
        return GlobalTypeError();
    }

    for (auto *type : types) {
        auto searchFlags = PropertySearchFlags::SEARCH_FIELD | PropertySearchFlags::SEARCH_METHOD |
                           PropertySearchFlags::SEARCH_IN_BASE;
        if (!type->IsETSObjectType()) {
            return nullptr;
        }
        auto *fieldVar = type->AsETSObjectType()->GetProperty(propertyName, searchFlags);
        if (fieldVar == nullptr) {
            return nullptr;
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *fieldType = GuaranteedTypeForUncheckedPropertyAccess(fieldVar);
        if (fieldType != nullptr) {
            apparentTypes.push_back(fieldType);
        }
    }
    return ResolveUnionUncheckedType(std::move(apparentTypes));
}

bool ETSChecker::IsAllowedTypeAliasRecursion(const ir::TSTypeAliasDeclaration *typeAliasNode,
                                             std::unordered_set<const ir::TSTypeAliasDeclaration *> &typeAliases)
{
    bool isAllowedRerursiveType = true;

    RecursionPreserver<const ir::TSTypeAliasDeclaration> recursionPreserver(typeAliases, typeAliasNode);

    if (*recursionPreserver) {
        return false;
    }

    auto typeAliasDeclarationCheck = [this, &typeAliases](ir::ETSTypeReferencePart *part) {
        if (!part->Name()->IsIdentifier()) {
            return false;
        }

        if (part->Name()->Variable() == nullptr) {
            return true;
        }

        auto const *const decl = part->Name()->Variable()->Declaration();
        if (auto const *const node = decl->Node(); node != nullptr && node->IsTSTypeAliasDeclaration()) {
            return IsAllowedTypeAliasRecursion(node->AsTSTypeAliasDeclaration(), typeAliases);
        }

        return true;
    };

    if (typeAliasNode->TypeAnnotation()->IsETSTypeReference()) {
        isAllowedRerursiveType &=
            typeAliasDeclarationCheck(typeAliasNode->TypeAnnotation()->AsETSTypeReference()->Part());
    }

    if (isAllowedRerursiveType && typeAliasNode->TypeAnnotation()->IsETSUnionType()) {
        for (auto &type : typeAliasNode->TypeAnnotation()->AsETSUnionType()->Types()) {
            if (type->IsETSTypeReference()) {
                isAllowedRerursiveType &= typeAliasDeclarationCheck(type->AsETSTypeReference()->Part());
            }
        }
    }

    return isAllowedRerursiveType;
}

Type *ETSChecker::GetTypeFromTypeAliasReference(varbinder::Variable *var)
{
    if (var->TsType() != nullptr) {
        return var->TsType();
    }

    auto *const aliasTypeNode = var->Declaration()->Node()->AsTSTypeAliasDeclaration();
    std::unordered_set<const ir::TSTypeAliasDeclaration *> typeAliases;
    auto isAllowedRecursion = IsAllowedTypeAliasRecursion(aliasTypeNode, typeAliases);

    TypeStackElement tse(this, aliasTypeNode, {{diagnostic::CYCLIC_ALIAS}}, aliasTypeNode->Start(), isAllowedRecursion);

    if (tse.HasTypeError()) {
        var->SetTsType(GlobalTypeError());
        return GlobalTypeError();
    }

    auto *typeAliasType = tse.GetElementType();

    if (typeAliasType != nullptr) {
        typeAliasType->AsETSTypeAliasType()->SetRecursive();
        return typeAliasType;
    }

    typeAliasType = CreateETSTypeAliasType(aliasTypeNode->Id()->Name(), aliasTypeNode);
    if (aliasTypeNode->TypeParams() != nullptr) {
        auto [typeParamTypes, ok] = CreateUnconstrainedTypeParameters(aliasTypeNode->TypeParams());
        ES2PANDA_ASSERT(typeAliasType != nullptr);
        typeAliasType->AsETSTypeAliasType()->SetTypeArguments(std::move(typeParamTypes));
        if (ok) {
            AssignTypeParameterConstraints(aliasTypeNode->TypeParams());
        }
    }
    tse.SetElementType(typeAliasType);

    aliasTypeNode->Check(this);
    Type *targetType = aliasTypeNode->TypeAnnotation()->GetType(this);
    typeAliasType->AsETSTypeAliasType()->SetTargetType(targetType);

    var->SetTsType(targetType);
    return targetType;
}

Type *ETSChecker::GetTypeFromInterfaceReference(varbinder::Variable *var)
{
    if (var->TsType() != nullptr) {
        return var->TsType();
    }

    CheckerStatus status = CheckerStatus::IN_STATIC_CONTEXT;
    status &= this->Context().Status();
    this->Context().Status() &= ~CheckerStatus::IN_STATIC_CONTEXT;

    auto *interfaceType = BuildBasicInterfaceProperties(var->Declaration()->Node()->AsTSInterfaceDeclaration());
    var->SetTsType(interfaceType);

    this->Context().Status() |= status;
    return interfaceType;
}

Type *ETSChecker::GetTypeFromClassReference(varbinder::Variable *var)
{
    if (var->TsType() != nullptr) {
        return var->TsType();
    }

    auto classDef = var->Declaration()->Node()->AsClassDefinition();

    CheckerStatus status = CheckerStatus::IN_STATIC_CONTEXT;
    status &= this->Context().Status();
    this->Context().Status() &= ~CheckerStatus::IN_STATIC_CONTEXT;

    auto *classType = BuildBasicClassProperties(classDef);
    var->SetTsType(classType);
    this->Context().Status() |= status;
    return classType;
}

Type *ETSChecker::GetTypeFromTypeParameterReference(varbinder::LocalVariable *var, const lexer::SourcePosition &pos)
{
    ES2PANDA_ASSERT(var->Declaration()->Node()->IsTSTypeParameter());
    if ((var->Declaration()->Node()->AsTSTypeParameter()->Parent()->Parent()->IsClassDefinition() ||
         var->Declaration()->Node()->AsTSTypeParameter()->Parent()->Parent()->IsTSInterfaceDeclaration()) &&
        HasStatus(CheckerStatus::IN_STATIC_CONTEXT)) {
        return TypeError(var, diagnostic::STATIC_REF_TO_NONSTATIC, {var->Name()}, pos);
    }

    return var->TsType();
}

bool ETSChecker::CheckAmbientAnnotationFieldInitializer(ir::Expression *init, ir::Expression *expected)
{
    if (init->Type() != expected->Type()) {
        return false;
    }

    switch (init->Type()) {
        case ir::AstNodeType::NUMBER_LITERAL:
        case ir::AstNodeType::BOOLEAN_LITERAL:
        case ir::AstNodeType::STRING_LITERAL:
        case ir::AstNodeType::ARRAY_EXPRESSION:
        case ir::AstNodeType::MEMBER_EXPRESSION:
        case ir::AstNodeType::UNARY_EXPRESSION: {
            if (CheckAmbientAnnotationFieldInitializerValue(init, expected)) {
                break;
            }
            LogError(diagnostic::AMBIENT_ANNOT_FIELD_INIT_MISMATCH, {}, init->Start());
            return false;
        }
        default:
            ES2PANDA_UNREACHABLE();
    }

    return true;
}

static bool IsValidateUnaryExpression(lexer::TokenType operatorType)
{
    return operatorType == lexer::TokenType::PUNCTUATOR_PLUS || operatorType == lexer::TokenType::PUNCTUATOR_MINUS;
}

bool ETSChecker::CheckAmbientAnnotationFieldInitializerValue(ir::Expression *init, ir::Expression *expected)
{
    switch (init->Type()) {
        case ir::AstNodeType::NUMBER_LITERAL: {
            return init->AsNumberLiteral()->Number().GetDouble() == expected->AsNumberLiteral()->Number().GetDouble();
        }
        case ir::AstNodeType::BOOLEAN_LITERAL: {
            return init->AsBooleanLiteral()->Value() == expected->AsBooleanLiteral()->Value();
        }
        case ir::AstNodeType::STRING_LITERAL: {
            return init->AsStringLiteral()->Str() == expected->AsStringLiteral()->Str();
        }
        case ir::AstNodeType::ARRAY_EXPRESSION: {
            const auto &elements = init->AsArrayExpression()->Elements();
            const auto &expectedElements = expected->AsArrayExpression()->Elements();
            if (elements.size() != expectedElements.size()) {
                return false;
            }
            for (size_t i = 0; i < elements.size(); ++i) {
                if (!CheckAmbientAnnotationFieldInitializer(elements[i], expectedElements[i])) {
                    return false;
                }
            }
            return true;
        }
        case ir::AstNodeType::MEMBER_EXPRESSION: {
            if (!Relation()->IsIdenticalTo(init->TsType(), expected->TsType())) {
                return false;
            }
            auto elem = init->AsMemberExpression()->Property()->AsIdentifier()->Name();
            auto expectedElem = expected->AsMemberExpression()->Property()->AsIdentifier()->Name();
            return elem == expectedElem;
        }
        case ir::AstNodeType::UNARY_EXPRESSION: {
            if (!IsValidateUnaryExpression(init->AsUnaryExpression()->OperatorType()) ||
                !IsValidateUnaryExpression(expected->AsUnaryExpression()->OperatorType())) {
                LogError(diagnostic::ILLEGAL_UNARY_OP, {}, init->Start());
                return false;
            }
            if (init->AsUnaryExpression()->OperatorType() != expected->AsUnaryExpression()->OperatorType()) {
                return false;
            }
            return CheckAmbientAnnotationFieldInitializer(init->AsUnaryExpression()->Argument(),
                                                          expected->AsUnaryExpression()->Argument());
        }
        default:
            ES2PANDA_UNREACHABLE();
    }
}

void ETSChecker::CheckAmbientAnnotation(ir::AnnotationDeclaration *annoImpl, ir::AnnotationDeclaration *annoDecl)
{
    std::unordered_map<util::StringView, ir::ClassProperty *> fieldMap;

    for (auto *prop : annoDecl->Properties()) {
        auto *field = prop->AsClassProperty();
        ES2PANDA_ASSERT(field->Id() != nullptr);
        fieldMap[field->Id()->Name()] = field;
    }

    for (auto *prop : annoImpl->Properties()) {
        auto *field = prop->AsClassProperty();
        auto *id = field->Id();
        ES2PANDA_ASSERT(id != nullptr);
        auto fieldName = id->Name();
        auto fieldDeclIter = fieldMap.find(fieldName);
        if (fieldDeclIter == fieldMap.end()) {
            LogError(diagnostic::AMBIENT_ANNOT_IMPL_OF_UNDEFINED_FIELD, {fieldName, annoDecl->GetBaseName()->Name()},
                     field->Start());
            continue;
        }

        auto *fieldDecl = fieldDeclIter->second;
        fieldDecl->Check(this);
        if (!Relation()->IsIdenticalTo(field->TsType(), fieldDecl->TsType())) {
            LogError(diagnostic::AMBIENT_ANNOT_FIELD_TYPE_MISMATCH, {fieldName, annoDecl->GetBaseName()->Name()},
                     field->TypeAnnotation()->Start());
        }

        bool hasValueMismatch = (field->Value() == nullptr) != (fieldDecl->Value() == nullptr);
        bool initializerInvalid = field->Value() != nullptr && fieldDecl->Value() != nullptr &&
                                  !CheckAmbientAnnotationFieldInitializer(field->Value(), fieldDecl->Value());
        if (hasValueMismatch || initializerInvalid) {
            LogError(diagnostic::AMBIENT_ANNOT_FIELD_MISMATCH, {fieldName, annoDecl->GetBaseName()->Name()},
                     field->Start());
        }
        fieldMap.erase(fieldDeclIter);
    }

    for (auto it : fieldMap) {
        LogError(diagnostic::AMBIENT_ANNOT_FIELD_MISSING_IMPL,
                 {it.second->Key()->AsIdentifier()->Name(), annoDecl->GetBaseName()->Name()}, annoImpl->Start());
    }
}

void ETSChecker::CheckFunctionSignatureAnnotations(const ArenaVector<ir::Expression *> &params,
                                                   ir::TSTypeParameterDeclaration *typeParams,
                                                   ir::TypeNode *returnTypeAnnotation)
{
    for (auto *param : params) {
        if (param->IsETSParameterExpression()) {
            CheckAnnotations(param->AsETSParameterExpression());
            if (param->AsETSParameterExpression()->TypeAnnotation() != nullptr) {
                CheckAnnotations(param->AsETSParameterExpression()->TypeAnnotation());
            }
        }
    }

    if (typeParams != nullptr) {
        for (auto *typeParam : typeParams->Params()) {
            CheckAnnotations(typeParam);
        }
    }

    if (returnTypeAnnotation != nullptr) {
        ValidateThisUsage(returnTypeAnnotation);
        CheckAnnotations(returnTypeAnnotation);
    }
}

bool ETSChecker::CheckAndLogInvalidThisUsage(const ir::TypeNode *type, const diagnostic::DiagnosticKind &diagnostic)
{
    if (type->IsTSThisType()) {
        LogError(diagnostic, {}, type->Start());
        return true;
    }
    return false;
}

bool ETSChecker::IsFixedArray(ir::ETSTypeReferencePart *part)
{
    return part->Name()->IsIdentifier() &&
           part->Name()->AsIdentifier()->Name().Mutf8() == compiler::Signatures::FIXED_ARRAY_TYPE_NAME;
}

void ETSChecker::ValidateThisUsage(const ir::TypeNode *returnTypeAnnotation)
{
    if (returnTypeAnnotation->IsETSUnionType()) {
        auto types = returnTypeAnnotation->AsETSUnionType()->Types();
        for (auto type : types) {
            if (CheckAndLogInvalidThisUsage(type, diagnostic::NOT_ALLOWED_THIS_IN_UNION_TYPE)) {
                return;
            }
            ValidateThisUsage(type);
        }
        return;
    }
    if (returnTypeAnnotation->IsETSTuple()) {
        auto types = returnTypeAnnotation->AsETSTuple()->GetTupleTypeAnnotationsList();
        for (auto type : types) {
            if (CheckAndLogInvalidThisUsage(type, diagnostic::NOT_ALLOWED_THIS_IN_TUPLE_TYPE)) {
                return;
            }
            ValidateThisUsage(type);
        }
        return;
    }
    if (returnTypeAnnotation->IsETSTypeReference() &&
        IsFixedArray(returnTypeAnnotation->AsETSTypeReference()->Part())) {
        if (returnTypeAnnotation->AsETSTypeReference()->Part()->TypeParams() != nullptr) {
            auto elementType = returnTypeAnnotation->AsETSTypeReference()->Part()->TypeParams()->Params()[0];
            if (CheckAndLogInvalidThisUsage(elementType, diagnostic::NOT_ALLOWED_THIS_IN_ARRAY_TYPE)) {
                return;
            }
            ValidateThisUsage(elementType);
        }
        return;
    }
}

void ETSChecker::CheckAnnotations(const ArenaVector<ir::AnnotationUsage *> &annotations)
{
    ES2PANDA_ASSERT(!annotations.empty());

    std::unordered_set<util::StringView> seenAnnotations;
    for (const auto &anno : annotations) {
        anno->Check(this);
        CheckAnnotationRetention(anno);
        auto annoName = anno->GetBaseName()->Name();
        if (seenAnnotations.find(annoName) != seenAnnotations.end()) {
            LogError(diagnostic::ANNOT_DUPLICATE, {annoName}, anno->Start());
        }
        seenAnnotations.insert(annoName);
    }
}

static bool IsValidSourceRetentionUsage(ir::AnnotationUsage *anno, ir::AnnotationDeclaration *annoDecl)
{
    bool isTransformedClassProperty = anno->Parent()->IsClassProperty() &&
                                      anno->Parent()->Parent()->IsClassDefinition() &&
                                      anno->Parent()->Parent()->AsClassDefinition()->IsModule();
    return (!anno->Parent()->IsClassDefinition() && !anno->Parent()->IsScriptFunction() &&
            !anno->Parent()->IsETSModule() && !anno->Parent()->IsTSInterfaceDeclaration() &&
            !anno->Parent()->IsETSParameterExpression() && !anno->Parent()->IsClassProperty() &&
            !annoDecl->IsSourceRetention()) ||
           (isTransformedClassProperty && !annoDecl->IsSourceRetention());
}

void ETSChecker::CheckAnnotationRetention(ir::AnnotationUsage *anno)
{
    if (anno->GetBaseName()->Name().Mutf8() == compiler::Signatures::BUILTIN_RETENTION &&
        !anno->Parent()->IsAnnotationDeclaration()) {
        LogError(diagnostic::INVALID_ANNOTATION_RETENTION, {}, anno->Start());
        return;
    }
    if (anno->GetBaseName()->Variable() == nullptr ||
        !anno->GetBaseName()->Variable()->Declaration()->Node()->IsAnnotationDeclaration()) {
        return;
    }
    auto *annoDecl = anno->GetBaseName()->Variable()->Declaration()->Node()->AsAnnotationDeclaration();
    annoDecl->Check(this);
    if (IsValidSourceRetentionUsage(anno, annoDecl)) {
        LogError(diagnostic::ANNOTATION_ON_LAMBDA_LOCAL_TYPE, {}, anno->Start());
    }
}

void ETSChecker::HandleAnnotationRetention(ir::AnnotationUsage *anno, ir::AnnotationDeclaration *annoDecl)
{
    if (anno->Properties().size() != 1) {
        return;
    }
    const auto value = anno->Properties()[0]->AsClassProperty()->Value();
    if (value->IsStringLiteral()) {
        const auto policyStr = value->AsStringLiteral()->Str().Mutf8();
        if (policyStr == compiler::Signatures::SOURCE_POLICY) {
            annoDecl->SetSourceRetention();
            return;
        }
        if (policyStr == compiler::Signatures::BYTECODE_POLICY) {
            annoDecl->SetBytecodeRetention();
            return;
        }
        if (policyStr == compiler::Signatures::RUNTIME_POLICY) {
            annoDecl->SetRuntimeRetention();
            return;
        }
    }
    LogError(diagnostic::ANNOTATION_POLICY_INVALID, {}, anno->Properties()[0]->Start());
}

void ETSChecker::CheckStandardAnnotation(ir::AnnotationUsage *anno)
{
    if (anno->GetBaseName()->Variable() == nullptr || IsTypeError(anno->GetBaseName()->TsType())) {
        return;
    }
    ES2PANDA_ASSERT(anno->GetBaseName()->Variable()->Declaration()->Node()->AsAnnotationDeclaration() != nullptr);
    auto *annoDecl = anno->GetBaseName()->Variable()->Declaration()->Node()->AsAnnotationDeclaration();
    auto annoName = annoDecl->InternalName().Mutf8();
    if (annoName.rfind(compiler::Signatures::STD_ANNOTATIONS) != 0) {
        LogError(diagnostic::STANDARD_ANNOTATION_REQUIRED, {}, anno->Start());
    }
    if (annoName == compiler::Signatures::STD_ANNOTATIONS_RETENTION) {
        HandleAnnotationRetention(anno, anno->Parent()->AsAnnotationDeclaration());
    }
}

static auto IsNonArrayLiteral(ir::Expression *init)
{
    if ((init == nullptr) || init->IsLiteral()) {
        return true;
    }

    if (init->TsType() != nullptr && init->TsType()->IsETSEnumType() &&
        init->TsType()->AsETSEnumType()->NodeIsEnumLiteral(init)) {
        return true;
    }
    return false;
}

static auto IsValidAnnotationPropInitializer(ir::Expression *init)
{
    if (IsNonArrayLiteral(init)) {
        return true;
    }

    if (init->IsArrayExpression()) {
        for (auto elem : init->AsArrayExpression()->Elements()) {
            if (!IsValidAnnotationPropInitializer(elem)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

static bool ValidateAnnotationPropertyType(checker::Type *type, ETSChecker *checker)
{
    if (type == nullptr || type->IsTypeError()) {
        ES2PANDA_ASSERT(checker->IsAnyError());
        return false;
    }

    if (type->IsETSArrayType() || type->IsETSResizableArrayType()) {
        return ValidateAnnotationPropertyType(checker->GetElementTypeOfArray(type), checker);
    }
    auto relation = checker->Relation();
    return type->IsETSEnumType() || type->IsETSStringType() ||
           (type->IsETSObjectType() && (relation->IsSupertypeOf(checker->GlobalETSBooleanBuiltinType(), type) ||
                                        relation->IsSupertypeOf(checker->GlobalByteBuiltinType(), type) ||
                                        relation->IsSupertypeOf(checker->GlobalShortBuiltinType(), type) ||
                                        relation->IsSupertypeOf(checker->GlobalIntBuiltinType(), type) ||
                                        relation->IsSupertypeOf(checker->GlobalLongBuiltinType(), type) ||
                                        relation->IsSupertypeOf(checker->GlobalFloatBuiltinType(), type) ||
                                        relation->IsSupertypeOf(checker->GlobalDoubleBuiltinType(), type)));
}

void ETSChecker::CheckAnnotationPropertyType(ir::ClassProperty *property)
{
    // typeAnnotation check
    if (!ValidateAnnotationPropertyType(property->TsType(), this)) {
        LogError(diagnostic::ANNOT_FIELD_INVALID_TYPE, {}, property->Start());
    }

    if (IsValidAnnotationPropInitializer(property->Value())) {
        return;
    }

    // Avoid outputting the same syntax and type errors.
    if (property->Value()->IsBrokenExpression()) {
        ES2PANDA_ASSERT(IsAnyError());
        return;
    }

    LogError(diagnostic::ANNOTATION_FIELD_NONLITERAL, {}, property->Value()->Start());
}

void ETSChecker::CheckSinglePropertyAnnotation(ir::AnnotationUsage *st, ir::AnnotationDeclaration *annoDecl)
{
    auto *param = st->Properties().at(0)->AsClassProperty();
    if (annoDecl->Properties().size() > 1) {
        LogError(diagnostic::ANNOT_MULTIPLE_FIELD, {st->GetBaseName()->Name()}, st->Start());
    }

    ScopeContext scopeCtx(this, st->Scope());
    param->Check(this);
    CheckAnnotationPropertyType(param);
}

void ETSChecker::CheckMultiplePropertiesAnnotation(ir::AnnotationUsage *st, util::StringView const &baseName,
                                                   ArenaUnorderedMap<util::StringView, ir::ClassProperty *> &fieldMap)
{
    for (auto *it : st->Properties()) {
        auto *param = it->AsClassProperty();
        auto *id = param->Id();
        ES2PANDA_ASSERT(id != nullptr);
        auto result = fieldMap.find(id->Name());
        if (result == fieldMap.end()) {
            LogError(diagnostic::ANNOT_PROP_UNDEFINED, {id->Name(), baseName}, param->Start());
            continue;
        }

        ScopeContext scopeCtx(this, st->Scope());
        param->Check(this);
        CheckAnnotationPropertyType(param);
        fieldMap.erase(result);
    }
}

Type *ETSChecker::MaybeUnboxInRelation(Type *type)
{
    if (type == nullptr) {
        return nullptr;
    }

    if (type->IsETSPrimitiveType()) {
        return type;
    }

    if (!type->IsETSUnboxableObject()) {
        return nullptr;
    }

    auto savedResult = Relation()->IsTrue();
    Relation()->Result(false);

    UnboxingConverter converter = UnboxingConverter(AsETSChecker(), Relation(), type, type);
    Relation()->Result(savedResult);
    return converter.Result();
}

Type *ETSChecker::MaybeUnboxConditionalInRelation(Type *const objectType)
{
    if (objectType != nullptr && objectType->IsTypeError()) {
        return objectType;
    }

    if (objectType == nullptr) {
        return nullptr;
    }

    if (auto *unboxed = MaybeUnboxInRelation(objectType); unboxed != nullptr) {
        return unboxed;
    }

    return objectType;
}

Type *ETSChecker::MaybeBoxInRelation(Type *objectType)
{
    if (objectType == nullptr) {
        return nullptr;
    }

    if (objectType->IsETSUnboxableObject()) {
        return objectType;
    }

    if (!objectType->IsETSPrimitiveType()) {
        return nullptr;
    }

    auto savedResult = Relation()->IsTrue();
    Relation()->Result(false);

    BoxingConverter converter = BoxingConverter(AsETSChecker(), Relation(), objectType,
                                                Checker::GetGlobalTypesHolder()->GlobalIntegerBuiltinType());
    Relation()->Result(savedResult);
    return converter.Result();
}

Type *ETSChecker::MaybeBoxType(Type *type) const
{
    ES2PANDA_ASSERT(type != nullptr);
    return type->IsETSPrimitiveType() ? BoxingConverter::Convert(this, type) : type;
}

Type *ETSChecker::MaybeUnboxType(Type *type) const
{
    ES2PANDA_ASSERT(type != nullptr);
    return type->IsETSUnboxableObject() ? UnboxingConverter::Convert(this, type->AsETSObjectType()) : type;
}

Type const *ETSChecker::MaybeBoxType(Type const *type) const
{
    return MaybeBoxType(const_cast<Type *>(type));
}

Type const *ETSChecker::MaybeUnboxType(Type const *type) const
{
    return MaybeUnboxType(const_cast<Type *>(type));
}

void ETSChecker::CheckUnboxedTypeWidenable(TypeRelation *relation, Type *target, Type *self)
{
    checker::SavedTypeRelationFlagsContext savedTypeRelationFlagCtx(relation, TypeRelationFlag::ONLY_CHECK_WIDENING);
    // NOTE: vpukhov. handle union type
    auto unboxedType = MaybeUnboxInRelation(target);
    if (unboxedType == nullptr) {
        return;
    }
    WideningConverter(this, relation, unboxedType, self);
    if (!relation->IsTrue()) {
        relation->Result(relation->IsAssignableTo(self, unboxedType));
    }
}

void ETSChecker::CheckUnboxedTypesAssignable(TypeRelation *relation, Type *source, Type *target)
{
    auto *unboxedSourceType = relation->GetChecker()->AsETSChecker()->MaybeUnboxInRelation(source);
    auto *unboxedTargetType = relation->GetChecker()->AsETSChecker()->MaybeUnboxInRelation(target);
    if (unboxedSourceType == nullptr || unboxedTargetType == nullptr) {
        return;
    }
    relation->IsAssignableTo(unboxedSourceType, unboxedTargetType);
}

void ETSChecker::CheckBoxedSourceTypeAssignable(TypeRelation *relation, Type *source, Type *target)
{
    ES2PANDA_ASSERT(relation != nullptr);
    checker::SavedTypeRelationFlagsContext savedTypeRelationFlagCtx(
        relation, (relation->ApplyWidening() ? TypeRelationFlag::WIDENING : TypeRelationFlag::NONE) |
                      (relation->OnlyCheckBoxingUnboxing() ? TypeRelationFlag::ONLY_CHECK_BOXING_UNBOXING
                                                           : TypeRelationFlag::NONE));

    auto *boxedSourceType = relation->GetChecker()->AsETSChecker()->MaybeBoxInRelation(source);
    if (boxedSourceType == nullptr) {
        return;
    }
    ES2PANDA_ASSERT(target != nullptr);
    relation->IsAssignableTo(boxedSourceType, target);
    if (!relation->IsTrue()) {
        auto unboxedTargetType = MaybeUnboxInRelation(target);
        if (unboxedTargetType == nullptr) {
            return;
        }
        WideningConverter(this, relation, unboxedTargetType, source);
    }
}

void ETSChecker::CheckUnboxedSourceTypeWithWideningAssignable(TypeRelation *relation, Type *source, Type *target)
{
    auto *unboxedSourceType = relation->GetChecker()->AsETSChecker()->MaybeUnboxInRelation(source);
    if (unboxedSourceType == nullptr) {
        return;
    }
    relation->IsAssignableTo(unboxedSourceType, target);
    if (!relation->IsTrue() && relation->ApplyWidening()) {
        relation->GetChecker()->AsETSChecker()->CheckUnboxedTypeWidenable(relation, target, unboxedSourceType);
    }
}

// #22952: optional arrow leftovers
bool ETSChecker::CheckLambdaAssignable(ir::Expression *param, ir::ScriptFunction *lambda)
{
    ES2PANDA_ASSERT(param->IsETSParameterExpression());
    ir::AstNode *typeAnn = param->AsETSParameterExpression()->Ident()->TypeAnnotation();
    if (typeAnn == nullptr) {
        return false;
    }
    if (typeAnn->IsETSTypeReference() && !typeAnn->AsETSTypeReference()->TsType()->IsETSArrayType()) {
        typeAnn = util::Helpers::DerefETSTypeReference(typeAnn);
    }

    if (!typeAnn->IsETSFunctionType()) {
        // the surrounding function is made so we can *bypass* the typecheck in the "inference" context,
        // however the body of the function has to be checked in any case
        if (typeAnn->IsETSUnionType()) {
            return CheckLambdaAssignableUnion(typeAnn, lambda);
        }

        Type *paramType = param->AsETSParameterExpression()->Ident()->TsType();
        if (Relation()->IsSupertypeOf(paramType, GlobalBuiltinFunctionType())) {
            lambda->Parent()->Check(this);
            return true;
        }
        return false;
    }

    ir::ETSFunctionType *calleeType = typeAnn->AsETSFunctionType();
    return lambda->Params().size() <= calleeType->Params().size();
}

bool ETSChecker::CheckLambdaInfer(ir::AstNode *typeAnnotation, ir::ArrowFunctionExpression *const arrowFuncExpr,
                                  Type *const subParameterType)
{
    if (typeAnnotation->IsETSTypeReference()) {
        typeAnnotation = util::Helpers::DerefETSTypeReference(typeAnnotation);
    }

    if (!typeAnnotation->IsETSFunctionType()) {
        return false;
    }

    ir::ScriptFunction *const lambda = arrowFuncExpr->Function();
    auto calleeType = typeAnnotation->AsETSFunctionType();
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    InferTypesForLambda(lambda, calleeType, subParameterType->AsETSFunctionType()->ArrowSignature());

    return true;
}

bool ETSChecker::CheckLambdaTypeAnnotation(ir::ETSParameterExpression *param,
                                           ir::ArrowFunctionExpression *const arrowFuncExpr, Type *const parameterType,
                                           TypeRelationFlag flags)
{
    ir::AstNode *typeAnnotation = param->Ident()->TypeAnnotation();
    if (typeAnnotation->IsETSTypeReference()) {
        typeAnnotation = util::Helpers::DerefETSTypeReference(typeAnnotation);
    }
    auto checkInvocable = [&arrowFuncExpr, &parameterType, this](TypeRelationFlag functionFlags) {
        Type *const argumentType = arrowFuncExpr->Check(this);
        functionFlags |= TypeRelationFlag::NO_THROW;

        checker::InvocationContext invocationCtx(Relation(), arrowFuncExpr, argumentType, parameterType,
                                                 arrowFuncExpr->Start(), std::nullopt, functionFlags);
        return invocationCtx.IsInvocable();
    };

    //  process `single` type as usual.
    if (!typeAnnotation->IsETSUnionType()) {
        // #22952: infer optional parameter heuristics
        auto nonNullishParam = param->IsOptional() ? GetNonNullishType(parameterType) : parameterType;
        ES2PANDA_ASSERT(nonNullishParam != nullptr);
        if (!nonNullishParam->IsETSFunctionType()) {
            arrowFuncExpr->Check(this);
            return true;
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return CheckLambdaInfer(typeAnnotation, arrowFuncExpr, nonNullishParam) && checkInvocable(flags);
    }

    // Preserve actual lambda types
    ir::ScriptFunction *const lambda = arrowFuncExpr->Function();
    ArenaVector<ir::TypeNode *> lambdaParamTypes {ProgramAllocator()->Adapter()};
    for (auto *const lambdaParam : lambda->Params()) {
        lambdaParamTypes.emplace_back(lambdaParam->AsETSParameterExpression()->Ident()->TypeAnnotation());
    }
    auto *const lambdaReturnTypeAnnotation = lambda->ReturnTypeAnnotation();

    if (!parameterType->IsETSUnionType() || parameterType->AsETSUnionType()->ConstituentTypes().size() !=
                                                typeAnnotation->AsETSUnionType()->Types().size()) {
        Type *const argumentType = arrowFuncExpr->Check(this);
        return Relation()->IsSupertypeOf(parameterType, argumentType);
    }

    const auto typeAnnsOfUnion = typeAnnotation->AsETSUnionType()->Types();
    const auto typeParamOfUnion = parameterType->AsETSUnionType()->ConstituentTypes();
    for (size_t ix = 0; ix < typeAnnsOfUnion.size(); ++ix) {
        auto *typeNode = typeAnnsOfUnion[ix];
        auto *paramNode = typeParamOfUnion[ix];
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        if (CheckLambdaInfer(typeNode, arrowFuncExpr, paramNode) && checkInvocable(flags)) {
            return true;
        }

        //  Restore inferring lambda types:
        for (std::size_t i = 0U; i < lambda->Params().size(); ++i) {
            if (lambdaParamTypes[i] == nullptr) {
                lambda->Params()[i]->AsETSParameterExpression()->Ident()->SetTsTypeAnnotation(nullptr);
            }
        }
        if (lambdaReturnTypeAnnotation == nullptr) {
            lambda->SetReturnTypeAnnotation(nullptr);
        }
    }

    return false;
}

bool ETSChecker::ResolveLambdaArgumentType(Signature *signature, ir::Expression *argument, size_t paramPosition,
                                           size_t argumentPosition, TypeRelationFlag resolutionFlags)
{
    if (!argument->IsArrowFunctionExpression()) {
        return true;
    }

    auto arrowFuncExpr = argument->AsArrowFunctionExpression();
    bool typeValid = true;
    ir::ScriptFunction *const lambda = arrowFuncExpr->Function();
    // Note: (Issue27688) if lambda is trailing lambda transferred, it must be in recheck.
    // its type was cleared before the check, so here we need recheck it.
    if (!NeedTypeInference(lambda) && !lambda->IsTrailingLambda()) {
        return typeValid;
    }

    arrowFuncExpr->SetTsType(nullptr);
    auto *const param =
        signature->GetSignatureInfo()->params[paramPosition]->Declaration()->Node()->AsETSParameterExpression();
    Type *const parameterType = signature->Params()[paramPosition]->TsType();

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    bool rc = CheckLambdaTypeAnnotation(param, arrowFuncExpr, parameterType, resolutionFlags);
    if (!rc) {
        if ((resolutionFlags & TypeRelationFlag::NO_THROW) == 0) {
            Type *const argumentType = arrowFuncExpr->Check(this);
            LogError(diagnostic::TYPE_MISMATCH_AT_IDX, {argumentType, parameterType, argumentPosition + 1},
                     arrowFuncExpr->Start());
        }
        rc = false;
    } else if ((lambda->Signature() != nullptr) && !lambda->HasReturnStatement()) {
        //  Need to check void return type here if there are no return statement(s) in the body.
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        if (!AssignmentContext(Relation(), ProgramAllocNode<ir::Identifier>(ProgramAllocator()), GlobalVoidType(),
                               lambda->Signature()->ReturnType(), lambda->Start(), std::nullopt,
                               checker::TypeRelationFlag::DIRECT_RETURN | checker::TypeRelationFlag::NO_THROW)
                 .IsAssignable()) {  // CC-OFF(G.FMT.02-CPP) project code style
            LogError(diagnostic::ARROW_TYPE_MISMATCH, {GlobalVoidType(), lambda->Signature()->ReturnType()},
                     lambda->Body()->Start());
            rc = false;
        }
    }

    typeValid &= rc;

    return typeValid;
}

bool ETSChecker::TrailingLambdaTypeInference(Signature *signature, const ArenaVector<ir::Expression *> &arguments)
{
    if (arguments.empty() || signature->GetSignatureInfo()->params.empty()) {
        return false;
    }
    ES2PANDA_ASSERT(arguments.back()->IsArrowFunctionExpression());
    const size_t lastParamPos = signature->GetSignatureInfo()->params.size() - 1;
    const size_t lastArgPos = arguments.size() - 1;
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return ResolveLambdaArgumentType(signature, arguments.back(), lastParamPos, lastArgPos, TypeRelationFlag::NONE);
}

bool ETSChecker::TypeInference(Signature *signature, const ArenaVector<ir::Expression *> &arguments,
                               TypeRelationFlag inferenceFlags)
{
    bool typeConsistent = true;
    auto const argumentCount = arguments.size();
    auto const minArity = std::min(signature->ArgCount(), argumentCount);

    for (size_t idx = 0U; idx < minArity; ++idx) {
        auto const &argument = arguments[idx];

        if (idx == argumentCount - 1 && (inferenceFlags & TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA) != 0) {
            continue;
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        const bool valid = ResolveLambdaArgumentType(signature, argument, idx, idx, inferenceFlags);
        typeConsistent &= valid;
    }

    return typeConsistent;
}

// #22951 requires complete refactoring
bool ETSChecker::IsExtensionETSFunctionType(const checker::Type *type)
{
    if (type == nullptr || (!type->IsETSFunctionType() && !type->IsETSObjectType())) {
        return false;
    }

    if (type->IsETSObjectType()) {
        return type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::EXTENSION_FUNCTION);
    }

    if (type->IsETSArrowType()) {
        return type->AsETSFunctionType()->ArrowSignature()->IsExtensionFunction();
    }

    return type->AsETSFunctionType()->IsExtensionFunctionType();
}

// #22951 requires complete refactoring
bool ETSChecker::IsExtensionAccessorFunctionType(const checker::Type *type)
{
    if (type == nullptr || !type->IsETSFunctionType()) {
        return false;
    }

    if (type->IsETSArrowType()) {
        return type->AsETSFunctionType()->ArrowSignature()->IsExtensionAccessor();
    }

    return type->AsETSFunctionType()->IsExtensionAccessorType();
}

void ETSChecker::CheckExceptionClauseType(const std::vector<checker::ETSObjectType *> &exceptions,
                                          ir::CatchClause *catchClause, checker::Type *clauseType)
{
    for (auto *exception : exceptions) {
        this->Relation()->IsIdenticalTo(clauseType, exception);
        if (this->Relation()->IsTrue()) {
            LogError(diagnostic::EXCEPTION_REDECLARATION, {}, catchClause->Start());
        }
    }
}

static bool CheckAccessModifierForOverloadDeclaration(ETSChecker *const checker,
                                                      const ir::ModifierFlags &overLoadAliasFlags,
                                                      const ir::ModifierFlags &overloadedMethodFlags,
                                                      const lexer::SourcePosition &pos)
{
    if (((overLoadAliasFlags ^ overloadedMethodFlags) & (ir::ModifierFlags::STATIC | ir::ModifierFlags::ASYNC)) != 0) {
        checker->LogError(diagnostic::OVERLOAD_SAME_ACCESS_MODIFIERS_STATIC_ASYNC, {}, pos);
        return false;
    }

    if (((overLoadAliasFlags ^ overloadedMethodFlags) & (ir::ModifierFlags::CONSTRUCTOR)) != 0) {
        checker->LogError(diagnostic::OVERLOAD_MUST_BOTH_CONSTRUCT, {}, pos);
        return false;
    }

    return true;
}

static bool CheckOverloadedName(ETSChecker *checker, ir::OverloadDeclaration *node, ir::Expression *overloadedName)
{
    if (overloadedName->Variable()->Declaration() == nullptr) {
        checker->LogError(diagnostic::OVERLOADED_NAME_MUST_FUNCTION, {}, overloadedName->Start());
        overloadedName->SetTsType(checker->GlobalTypeError());
        return false;
    }

    auto *identDeclNode = overloadedName->Variable()->Declaration()->Node();
    if (!identDeclNode->IsMethodDefinition()) {
        checker->LogError(diagnostic::OVERLOADED_NAME_MUST_FUNCTION, {}, overloadedName->Start());
        overloadedName->SetTsType(checker->GlobalTypeError());
        return false;
    }
    // Constructor will lowering to multiple Constructor if have rest parameters or optional parameters.
    // Need to modify RestTupleConstructionPhase.
    if (!identDeclNode->AsMethodDefinition()->Overloads().empty() && !identDeclNode->IsConstructor()) {
        checker->LogError(diagnostic::OVERLOADED_NAME_REFER_TO_OVERLOAD_FUNCTION, {overloadedName->Variable()->Name()},
                          overloadedName->Start());
        overloadedName->SetTsType(checker->GlobalTypeError());
        return false;
    }

    return CheckAccessModifierForOverloadDeclaration(checker, node->Modifiers(), identDeclNode->Modifiers(),
                                                     overloadedName->Start());
}

void ETSChecker::CheckFunctionOverloadDeclaration(ETSChecker *checker, ir::OverloadDeclaration *node) const
{
    for (auto *overloadedName : node->OverloadedList()) {
        if (overloadedName->IsMemberExpression()) {
            overloadedName->Check(checker);
            overloadedName->SetVariable(overloadedName->AsMemberExpression()->Property()->Variable());
        } else if (overloadedName->IsIdentifier()) {
            ir::Identifier *ident = overloadedName->AsIdentifier();

            Type *classType = node->Parent()->AsClassDefinition()->TsType();
            ES2PANDA_ASSERT(classType->IsETSObjectType());

            PropertySearchFlags searchFlags = PropertySearchFlags::SEARCH_METHOD | PropertySearchFlags::SEARCH_IN_BASE |
                                              PropertySearchFlags::SEARCH_IN_INTERFACES |
                                              PropertySearchFlags::IS_GETTER | PropertySearchFlags::IGNORE_OVERLOAD;
            auto *variable =
                checker->ResolveOverloadReference(ident->AsIdentifier(), classType->AsETSObjectType(), searchFlags);
            if (variable == nullptr) {
                checker->LogError(diagnostic::OVERLOADED_NAME_MUST_FUNCTION, {}, ident->Start());
                ident->SetTsType(checker->GlobalTypeError());
                continue;
            }

            ident->SetTsType(variable->TsType());
            ident->SetVariable(variable);

            if (!CheckOverloadedName(checker, node, overloadedName)) {
                continue;
            }
        } else {
            overloadedName->SetTsType(checker->GlobalTypeError());
            continue;
        }
    }
}

void ETSChecker::CheckClassMethodOverloadDeclaration(ETSChecker *checker, ir::OverloadDeclaration *node) const
{
    PropertySearchFlags searchFlags =
        PropertySearchFlags::SEARCH_IN_BASE | PropertySearchFlags::SEARCH_IN_INTERFACES |
        PropertySearchFlags::IS_GETTER | PropertySearchFlags::IGNORE_OVERLOAD |
        (node->IsStatic() ? PropertySearchFlags::SEARCH_STATIC_METHOD : PropertySearchFlags::SEARCH_INSTANCE_METHOD);

    for (auto *overloadedName : node->OverloadedList()) {
        if (!overloadedName->IsIdentifier()) {
            overloadedName->SetTsType(checker->GlobalTypeError());
            continue;
        }
        ir::Identifier *ident = overloadedName->AsIdentifier();

        Type *classType = node->Parent()->AsClassDefinition()->TsType();
        ES2PANDA_ASSERT(classType->IsETSObjectType());

        auto *variable =
            checker->ResolveOverloadReference(ident->AsIdentifier(), classType->AsETSObjectType(), searchFlags);

        if (variable == nullptr &&
            checker->ResolveOverloadReference(ident->AsIdentifier(), classType->AsETSObjectType(),
                                              searchFlags | PropertySearchFlags::SEARCH_METHOD) != nullptr) {
            checker->LogError(diagnostic::OVERLOAD_SAME_ACCESS_MODIFIERS_STATIC_ASYNC, {}, ident->Start());
            continue;
        }
        if (variable == nullptr) {
            checker->LogError(diagnostic::OVERLOADED_NAME_MUST_FUNCTION, {}, ident->Start());
            ident->SetTsType(checker->GlobalTypeError());
            continue;
        }

        ident->SetTsType(variable->TsType());
        ident->SetVariable(variable);

        if (!CheckOverloadedName(checker, node, overloadedName)) {
            continue;
        }
    }
}

void ETSChecker::CheckInterfaceMethodOverloadDeclaration(ETSChecker *checker, ir::OverloadDeclaration *node) const
{
    PropertySearchFlags searchFlags =
        PropertySearchFlags::SEARCH_IN_BASE | PropertySearchFlags::SEARCH_IN_INTERFACES |
        PropertySearchFlags::IS_GETTER | PropertySearchFlags::IGNORE_OVERLOAD |
        (node->IsStatic() ? PropertySearchFlags::SEARCH_STATIC_METHOD : PropertySearchFlags::SEARCH_INSTANCE_METHOD);
    for (auto *overloadedName : node->OverloadedList()) {
        if (!overloadedName->IsIdentifier()) {
            overloadedName->SetTsType(checker->GlobalTypeError());
            continue;
        }
        ir::Identifier *ident = overloadedName->AsIdentifier();

        Type *interfaceType = node->Parent()->Parent()->AsTSInterfaceDeclaration()->TsType();
        ES2PANDA_ASSERT(interfaceType->IsETSObjectType());
        auto *variable =
            checker->ResolveOverloadReference(ident->AsIdentifier(), interfaceType->AsETSObjectType(), searchFlags);

        if (variable == nullptr &&
            checker->ResolveOverloadReference(ident->AsIdentifier(), interfaceType->AsETSObjectType(),
                                              searchFlags | PropertySearchFlags::SEARCH_METHOD) != nullptr) {
            checker->LogError(diagnostic::OVERLOAD_SAME_ACCESS_MODIFIERS_STATIC_ASYNC, {}, ident->Start());
            continue;
        }
        if (variable == nullptr) {
            checker->LogError(diagnostic::OVERLOADED_NAME_MUST_FUNCTION, {}, ident->Start());
            ident->SetTsType(checker->GlobalTypeError());
            continue;
        }

        ident->SetTsType(variable->TsType());
        ident->SetVariable(variable);

        if (!CheckOverloadedName(checker, node, overloadedName)) {
            continue;
        }
    }
}

void ETSChecker::CheckConstructorOverloadDeclaration(ETSChecker *checker, ir::OverloadDeclaration *node) const
{
    for (auto *overloadedName : node->OverloadedList()) {
        if (!overloadedName->IsIdentifier()) {
            overloadedName->SetTsType(checker->GlobalTypeError());
            continue;
        }
        ir::Identifier *ident = overloadedName->AsIdentifier();

        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        Type *identType = checker->ResolveIdentifier(ident->AsIdentifier());
        ident->SetTsType(identType);
        if (identType->IsTypeError()) {
            continue;
        }

        ES2PANDA_ASSERT(identType->IsETSFunctionType());
        const size_t singleSignatureSize = 1;
        if (identType->AsETSFunctionType()->CallSignatures().size() > singleSignatureSize) {
            size_t userDefinedConstructorSize =
                std::count_if(identType->AsETSFunctionType()->CallSignatures().begin(),
                              identType->AsETSFunctionType()->CallSignatures().end(),
                              [](Signature *sig) { return !sig->Function()->IsSynthetic(); });
            if (userDefinedConstructorSize > singleSignatureSize) {
                checker->LogError(diagnostic::OVERLOADED_NAME_REFER_TO_OVERLOAD_FUNCTION, {ident->Name()},
                                  node->Start());
                continue;
            }
        }

        if (!CheckOverloadedName(checker, node, overloadedName)) {
            continue;
        }
    }
}

}  // namespace ark::es2panda::checker
