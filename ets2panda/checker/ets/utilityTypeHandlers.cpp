/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "generated/signatures.h"
#include "ir/expressions/identifier.h"
#include "ir/ets/etsNullishTypes.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "ir/ets/etsUnionType.h"
#include "ir/expressions/literals/undefinedLiteral.h"
#include "varbinder/ETSBinder.h"
#include "checker/types/ets/etsPartialTypeParameter.h"
#include "checker/types/ets/etsAwaitedType.h"
#include "checker/types/ets/etsReturnTypeUtilityType.h"
#include "compiler/lowering/util.h"
#include "checker/types/typeError.h"
#include "util/nameMangler.h"

namespace ark::es2panda::checker {

std::optional<ir::TypeNode *> ETSChecker::GetUtilityTypeTypeParamNode(
    const ir::TSTypeParameterInstantiation *const typeParams, const std::string_view &utilityTypeName)
{
    if (typeParams->Params().size() != 1) {
        LogError(diagnostic::UTIL_TYPE_INVALID_TYPE_PARAM_COUNT, {utilityTypeName}, typeParams->Start());
        return std::nullopt;
    }
    return typeParams->Params().front();
}

static bool ValidBaseTypeOfRequiredAndPartial(Type *type)
{
    return type->IsETSObjectType() &&
           type->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::INTERFACE | ETSObjectFlags::CLASS);
}

static bool InvalidBaseTypeOfRequiredPartialAndReadonly(Type *type, const std::string_view &utilityType)
{
    if (utilityType == compiler::Signatures::PARTIAL_TYPE_NAME ||
        utilityType == compiler::Signatures::REQUIRED_TYPE_NAME ||
        utilityType == compiler::Signatures::READONLY_TYPE_NAME) {
        return !(type->IsETSObjectType() &&
                 type->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::INTERFACE | ETSObjectFlags::CLASS));
    }
    return false;
}

static bool HasUnresolvedRecursiveAlias(const Type *type)
{
    bool hasUnresolvedRecursiveAlias = false;

    type->IterateRecursivelyPreorder([&hasUnresolvedRecursiveAlias](const Type *child) {
        if (hasUnresolvedRecursiveAlias || child == nullptr || !child->IsETSTypeAliasType()) {
            return;
        }

        auto *aliasType = const_cast<ETSTypeAliasType *>(child->AsETSTypeAliasType());
        hasUnresolvedRecursiveAlias = aliasType->IsRecursive() && aliasType->GetTargetType() == nullptr;
    });

    return hasUnresolvedRecursiveAlias;
}

Type *ETSChecker::HandleUtilityTypeParameterNode(const ir::TSTypeParameterInstantiation *const typeParams,
                                                 const ir::Identifier *const ident)
{
    if (typeParams == nullptr) {
        LogError(diagnostic::USING_RESERVED_NAME_AS_VARIABLE_OR_TYPE_NAME, {ident->Name().Utf8()}, ident->Start());
        return GlobalTypeError();
    }
    const std::string_view &utilityType = ident->Name().Utf8();
    std::optional<ir::TypeNode *> possiblyTypeParam = GetUtilityTypeTypeParamNode(typeParams, utilityType);
    if (!possiblyTypeParam.has_value()) {
        return GlobalTypeError();
    }

    Type *baseType = possiblyTypeParam.value()->Check(this);

    if (baseType->IsTypeError()) {
        return baseType;
    }

    if (utilityType == compiler::Signatures::AWAITED_TYPE_NAME && HasUnresolvedRecursiveAlias(baseType)) {
        LogError(diagnostic::CYCLIC_ALIAS, {}, typeParams->Start());
        return GlobalTypeError();
    }

    if ((utilityType == compiler::Signatures::PARTIAL_TYPE_NAME ||
         utilityType == compiler::Signatures::REQUIRED_TYPE_NAME) &&
        !ValidBaseTypeOfRequiredAndPartial(baseType)) {
        LogError(diagnostic::MUST_BE_CLASS_INTERFACE_TYPE, {utilityType}, typeParams->Start());
        return GlobalTypeError();
    }

    if (InvalidBaseTypeOfRequiredPartialAndReadonly(baseType, utilityType)) {
        LogError(diagnostic::MUST_BE_CLASS_INTERFACE_TYPE, {utilityType}, typeParams->Start());
        return GlobalTypeError();
    }

    if (utilityType == compiler::Signatures::PARTIAL_TYPE_NAME) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return CreatePartialType(baseType);
    }

    if (utilityType == compiler::Signatures::READONLY_TYPE_NAME) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return GetReadonlyType(baseType);
    }

    if (utilityType == compiler::Signatures::REQUIRED_TYPE_NAME) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return HandleRequiredType(baseType);
    }

    if (utilityType == compiler::Signatures::AWAITED_TYPE_NAME) {
        return HandleAwaitedUtilityType(baseType);
    }

    if (utilityType == compiler::Signatures::RETURN_TYPE_TYPE_NAME) {
        auto *returnType = HandleReturnTypeUtilityType(baseType);
        ValidateReturnTypeUtilityType(returnType, typeParams);
        return returnType;
    }

    LogError(diagnostic::UTILITY_TYPE_UNIMPLEMENTED, {}, typeParams->Start());
    return baseType;
}

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Awaited utility type
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
bool ETSChecker::IsPromiseType(Type *type)
{
    ES2PANDA_ASSERT(type);

    return Relation()->IsSupertypeOf(CreatePromiseOf(GlobalETSAnyType()), type) &&
           !Relation()->IsIdenticalTo(GlobalETSNeverType(), type);
}

/**
 * NOTE(knazarov): Needed to cover cases when spec requires type to be Promise<T> or T,
 * Since UnwrapPromiseType implements Awaited semantics, which unwrap recursively;
 */
Type *ETSChecker::PromiseTypeArg(checker::ETSObjectType *type)
{
    ES2PANDA_ASSERT(type);
    ES2PANDA_ASSERT(type->IsETSObjectType());
    ES2PANDA_ASSERT(IsPromiseType(type));
    ES2PANDA_ASSERT(type->AsETSObjectType()->TypeArguments().size() == 1);

    return type->AsETSObjectType()->TypeArguments()[0];
}

Type *ETSChecker::UnwrapPromiseType(checker::Type *type)
{
    Type *promiseType = GlobalBuiltinPromiseType();
    while (type->IsETSObjectType() && (type->AsETSObjectType()->GetOriginalBaseType() == promiseType)) {
        type = type->AsETSObjectType()->TypeArguments().at(0);
    }

    if (!type->IsETSUnionType()) {
        return type;
    }
    const auto &ctypes = type->AsETSUnionType()->ConstituentTypes();
    auto it = std::find_if(ctypes.begin(), ctypes.end(), [promiseType](checker::Type *t) {
        return t == promiseType || (t->IsETSObjectType() && (t->AsETSObjectType()->GetBaseType() == promiseType));
    });
    if (it == ctypes.end()) {
        return type;
    }
    auto newCTypes = ArenaVectorToStdVector(ctypes);
    do {
        size_t index = it - ctypes.begin();
        newCTypes[index] = UnwrapPromiseType(ctypes[index]);
        ++it;
        it = std::find_if(it, ctypes.end(), [promiseType](checker::Type *t) {
            return t == promiseType || (t->IsETSObjectType() && (t->AsETSObjectType()->GetBaseType() == promiseType));
        });
    } while (it != ctypes.end());
    return CreateETSUnionType(std::move(newCTypes));
}

Type *ETSChecker::HandleAwaitedUtilityType(Type *typeToBeAwaited)
{
    auto &typeCache = CachedUtilityTypes<static_cast<std::size_t>(UtilityType::AWAITED)>();
    if (const auto found = typeCache.find(typeToBeAwaited); found != typeCache.end()) {
        return found->second;
    }

    auto const cacheType = [&typeCache, typeToBeAwaited](Type *awaitedType) {
        typeCache.try_emplace(typeToBeAwaited, awaitedType);
        return awaitedType;
    };

    if (typeToBeAwaited->IsETSTypeParameter()) {
        return cacheType(ProgramAllocator()->New<ETSAwaitedType>(typeToBeAwaited->AsETSTypeParameter()));
    }

    if (typeToBeAwaited->IsETSUnionType()) {
        std::vector<Type *> awaitedTypes;
        for (Type *type : typeToBeAwaited->AsETSUnionType()->ConstituentTypes()) {
            Type *unwrapped = IsPromiseType(type) ? type->AsETSObjectType()->TypeArguments().at(0) : type;
            if (unwrapped->IsETSTypeParameter()) {
                unwrapped = HandleAwaitedUtilityType(unwrapped);
            }
            awaitedTypes.push_back(UnwrapPromiseType(unwrapped));
        }
        return cacheType(CreateETSUnionType(Span<Type *const>(awaitedTypes)));
    }

    if (IsPromiseType(typeToBeAwaited)) {
        Type *typeArg = typeToBeAwaited->AsETSObjectType()->TypeArguments().at(0);
        auto unwrappedType = UnwrapPromiseType(typeArg);
        return cacheType(unwrappedType->IsETSTypeParameter() ? HandleAwaitedUtilityType(unwrappedType) : unwrappedType);
    }

    return typeToBeAwaited;
}

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// ReturnType utility type
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Type *ETSChecker::HandleReturnTypeUtilityType(Type *baseType)
{
    if (baseType->IsETSFunctionType()) {
        auto *const funcTypeSig = baseType->AsETSFunctionType()->ArrowSignature();
        return funcTypeSig->ReturnType();
    }

    if (baseType->IsETSNeverType()) {
        return baseType;
    }

    auto &typeCache = CachedUtilityTypes<static_cast<std::size_t>(UtilityType::RETURN_TYPE)>();
    if (const auto found = typeCache.find(baseType); found != typeCache.end()) {
        return found->second;
    }

    auto const cacheType = [&typeCache, baseType](Type *returnType) {
        typeCache.emplace(baseType, returnType);
        return returnType;
    };

    if (baseType->IsETSTypeParameter()) {
        if (!Relation()->IsSupertypeOf(GlobalBuiltinFunctionType(),
                                       baseType->AsETSTypeParameter()->GetConstraintType())) {
            return GlobalTypeError();
        }

        return cacheType(ProgramAllocator()->New<ETSReturnTypeUtilityType>(baseType->AsETSTypeParameter()));
    }

    if (baseType->IsETSUnionType()) {
        std::vector<Type *> returnTypes;
        for (Type *type : baseType->AsETSUnionType()->ConstituentTypes()) {
            auto *constituentRetType = HandleReturnTypeUtilityType(type);
            if (constituentRetType->IsTypeError()) {
                return constituentRetType;
            }
            returnTypes.push_back(constituentRetType);
        }
        return cacheType(CreateETSUnionType(std::move(returnTypes)));
    }

    if (Relation()->IsIdenticalTo(baseType, GlobalBuiltinFunctionType())) {
        return cacheType(GlobalETSAnyType());
    }

    return GlobalTypeError();
}

void ETSChecker::ValidateReturnTypeUtilityType(const Type *typeToValidate,
                                               const ir::TSTypeParameterInstantiation *const typeParams)
{
    if (typeToValidate->IsTypeError()) {
        LogError(diagnostic::RETURN_TYPE_UTILITY_INCORRECT_PARAM, {typeParams->DumpEtsSrc()}, typeParams->Start());
    }
}

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Partial utility type
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

static std::pair<util::StringView, util::StringView> GetPartialClassName(ETSChecker *checker, ir::AstNode *typeNode)
{
    // Partial class name for class 'T' will be '%%partial-T'
    auto const addSuffix = [checker](util::StringView name) {
        std::string newName =
            util::NameMangler::GetInstance()->CreateMangledNameByTypeAndName(util::NameMangler::PARTIAL, name);
        return util::UString(newName, checker->ProgramAllocator()).View();
    };

    auto declIdent = typeNode->IsClassDefinition() ? typeNode->AsClassDefinition()->Ident()
                                                   : typeNode->AsTSInterfaceDeclaration()->Id();
    auto internalName = typeNode->IsClassDefinition() ? typeNode->AsClassDefinition()->InternalName()
                                                      : typeNode->AsTSInterfaceDeclaration()->InternalName();
    return {addSuffix(declIdent->Name()), addSuffix(internalName)};
}

static bool HasInterfaceInHierarchy(const ETSObjectType *type, const ETSObjectType *interfaceType)
{
    if (type == nullptr || interfaceType == nullptr) {
        return false;
    }

    for (auto *existingInterface : type->Interfaces()) {
        if (existingInterface->GetOriginalBaseType() == interfaceType->GetOriginalBaseType()) {
            return true;
        }
    }

    return HasInterfaceInHierarchy(type->SuperType(), interfaceType);
}

static std::pair<parser::Program *, varbinder::RecordTable *> GetPartialClassProgram(
    // CC-OFFNXT(G.FMT.06-CPP) project code style
    [[maybe_unused]] ETSChecker *checker, ir::AstNode *typeNode)
{
    auto classDefProgram = typeNode->GetTopStatement()->AsETSModule()->Program();
    ES2PANDA_ASSERT(checker->VarBinder()->AsETSBinder()->CheckRecordTablesConsistency(classDefProgram));
    return {classDefProgram, classDefProgram->GetRecordTable()};
}

Type *ETSChecker::CreatePartialType(Type *const typeToBePartial)
{
    ES2PANDA_ASSERT(typeToBePartial->IsETSReferenceType());
    if (typeToBePartial->IsTypeError() || typeToBePartial->IsETSNeverType() || typeToBePartial->IsETSAnyType()) {
        return typeToBePartial;
    }

    auto &typeCache = CachedUtilityTypes<static_cast<std::size_t>(UtilityType::PARTIAL)>();
    Type *partialType = nullptr;

    if (typeToBePartial->IsETSObjectType()) {
        ETSObjectType *objectType = typeToBePartial->AsETSObjectType();
        ETSObjectType *baseType = objectType->GetOriginalBaseType();

        if (auto found = typeCache.find(baseType); found != typeCache.end()) {
            partialType = found->second;
        } else {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            partialType = CreatePartialTypeClass(baseType, typeToBePartial->Variable()->Declaration()->Node());
            if (!partialType->IsTypeError()) {
                typeCache.emplace(baseType, partialType);
            }
        }

        if (partialType->IsETSObjectType() && !partialType->AsETSObjectType()->TypeArguments().empty()) {
            auto const &parameters = partialType->AsETSObjectType()->TypeArguments();
            auto const &arguments = objectType->TypeArguments();

            auto const paramNumber = parameters.size();
            ES2PANDA_ASSERT(paramNumber == arguments.size());

            Substitution substitution {};
            for (std::size_t i = 0U; i < paramNumber; ++i) {
                EmplaceSubstituted(&substitution, parameters[i]->AsETSTypeParameter(), arguments[i]);
            }

            if (!substitution.empty()) {
                partialType = partialType->Substitute(Relation(), &substitution);
            }
        }
    } else {
        if (auto found = typeCache.find(typeToBePartial); found != typeCache.end()) {
            return found->second;
        }

        if (typeToBePartial->IsETSTypeParameter()) {
            partialType = CreatePartialTypeParameter(typeToBePartial->AsETSTypeParameter());
        } else if (typeToBePartial->IsETSUnionType()) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            partialType = HandleUnionForPartialType(typeToBePartial->AsETSUnionType());
        }

        if (!partialType->IsTypeError()) {
            typeCache.emplace(typeToBePartial, partialType);
        }
    }
    return partialType;
}

Type *ETSChecker::CreatePartialTypeParameter(ETSTypeParameter *typeToBePartial)
{
    auto *partialType = ProgramAllocator()->New<ETSPartialTypeParameter>(typeToBePartial, this);
    return partialType;
}

Type *ETSChecker::CreatePartialTypeClass(ETSObjectType *typeToBePartial, ir::AstNode *typeDeclNode)
{
    auto const [partialName, partialQualifiedName] = GetPartialClassName(this, typeDeclNode);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto const [partialProgram, recordTable] = GetPartialClassProgram(this, typeDeclNode);

    // Check if we've already generated the partial class, then don't do it again
    bool nonQualifiedName = partialProgram == VarBinder()->Program() || VarBinder()->IsGenStdLib() ||
                            partialProgram->IsBuiltSimultaneously();
    const auto &classNameToFind = nonQualifiedName ? partialName : partialQualifiedName;
    if (auto *var =
            SearchNamesInMultiplePrograms({partialProgram, VarBinder()->Program()}, {classNameToFind, partialName});
        var != nullptr) {
        return var->TsType();
    }

    if (typeDeclNode->IsTSInterfaceDeclaration()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return HandlePartialInterface(typeDeclNode->AsTSInterfaceDeclaration(), typeToBePartial);
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    ir::ClassDefinition *partialClassDef = CreateClassPrototype(partialName, partialProgram);
    partialClassDef->SetInternalName(partialQualifiedName);

    // Create only class 'header' (no properties and methods, but base type created)
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    BuildBasicClassProperties(partialClassDef);

    compiler::SetSourceRangesRecursively(partialClassDef, typeDeclNode->Range());

    const varbinder::BoundContext boundCtx(recordTable, partialClassDef);

    // If class is external, put partial of it in global scope for the varbinder
    if (partialProgram != VarBinder()->Program()) {
        VarBinder()->Program()->GlobalScope()->InsertBinding(partialClassDef->Ident()->Name(),
                                                             partialClassDef->Variable());
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return CreatePartialTypeClassDef(partialClassDef, typeDeclNode->AsClassDefinition(), typeToBePartial, recordTable);
}

Type *ETSChecker::HandlePartialInterface(ir::TSInterfaceDeclaration *interfaceDecl, ETSObjectType *typeToBePartial)
{
    auto const [partialName, partialQualifiedName] = GetPartialClassName(this, interfaceDecl);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto const [partialProgram, recordTable] = GetPartialClassProgram(this, interfaceDecl);

    auto *const partialInterDecl =
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        CreateInterfaceProto(partialName, partialProgram, interfaceDecl);
    partialInterDecl->SetInternalName(partialQualifiedName);

    const varbinder::BoundContext boundCtx(recordTable, partialInterDecl);
    varbinder::RecordTableContext recordTableCtx {VarBinder()->AsETSBinder(), partialProgram};
    // If class is external, put partial of it in global scope for the varbinder
    if (partialProgram != VarBinder()->Program()) {
        VarBinder()->Program()->GlobalScope()->InsertBinding(partialInterDecl->Id()->Name(),
                                                             partialInterDecl->Variable());
    }

    auto savedScope = VarBinder()->TopScope();
    VarBinder()->ResetTopScope(partialProgram->GlobalScope());
    auto *partialType =
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        CreatePartialTypeInterfaceDecl(interfaceDecl, typeToBePartial, partialInterDecl, partialProgram);
    VarBinder()->ResetTopScope(savedScope);
    ES2PANDA_ASSERT(partialType != nullptr);

    return partialType;
}

ir::ClassProperty *ETSChecker::CreateNullishPropertyFromAccessor(ir::MethodDefinition *const accessor,
                                                                 ir::ClassDefinition *const newClassDefinition)
{
    bool const isGetter = accessor->Function()->IsGetter();

    ES2PANDA_ASSERT(accessor->TsType() != nullptr && accessor->TsType()->IsETSFunctionType());
    auto callSign = accessor->TsType()->AsETSFunctionType()->CallSignatures()[0];
    checker::Type *tsType = isGetter ? callSign->ReturnType() : callSign->Params()[0]->TsType();

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return CreateNullishProperty(accessor->Id(), ir::ModifierFlags::NONE, tsType, newClassDefinition);
}

ir::ClassProperty *ETSChecker::CreateNullishProperty(ir::Identifier *const id, ir::ModifierFlags const flags,
                                                     Type *const tsType, ir::ClassDefinition *const newClassDefinition)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *ident = id->Clone(ProgramAllocator(), nullptr);
    ir::ClassProperty *prop =
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        ProgramAllocNode<ir::ClassProperty>(ident, nullptr, nullptr, flags, ProgramAllocator(), false);
    prop->SetParent(newClassDefinition);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    prop->SetValue(ProgramAllocator()->New<ir::UndefinedLiteral>());
    prop->Value()->SetTsType(GlobalETSUndefinedType());

    std::vector<checker::Type *> types {tsType, GlobalETSUndefinedType()};
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *type = CreateETSUnionType(Span<Type *const>(types));
    prop->SetTsType(prop->Id()->SetTsType(type));

    return prop;
}

ir::TSTypeParameterDeclaration *ETSChecker::ProcessTypeParamAndGenSubstitution(
    ir::TSTypeParameterDeclaration const *const thisTypeParams,
    std::unordered_map<ir::TSTypeParameter *, ir::TSTypeParameter *> &substitution,
    ir::TSTypeParameterDeclaration *newTypeParams)
{
    if (newTypeParams == nullptr) {
        ArenaVector<ir::TSTypeParameter *> typeParams(ProgramAllocator()->Adapter());
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        newTypeParams = ProgramAllocNode<ir::TSTypeParameterDeclaration>(std::move(typeParams), typeParams.size());
    }

    for (auto *const typeParam : thisTypeParams->Params()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *newTypeParam = typeParam->Clone(ProgramAllocator(), newTypeParams);
        newTypeParams->AddParam(newTypeParam);
        substitution[typeParam] = newTypeParam;
    }

    return newTypeParams;
}

ir::TSTypeParameterInstantiation *ETSChecker::CreateNewSuperPartialRefTypeParamsDecl(
    std::unordered_map<ir::TSTypeParameter *, ir::TSTypeParameter *> const &substitution,
    const Type *const superPartialType, ir::Expression *superRef)
{
    ir::TSTypeParameterInstantiation *superPartialRefTypeParams = nullptr;
    if (superPartialType == nullptr || superRef == nullptr ||
        superRef->AsETSTypeReference()->Part()->TypeParams() == nullptr) {
        return superPartialRefTypeParams;
    }
    superPartialRefTypeParams =
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        superRef->AsETSTypeReference()->Part()->TypeParams()->Clone(ProgramAllocator(), nullptr);
    superPartialRefTypeParams->SetTsType(nullptr);
    auto superRefParams = superPartialRefTypeParams->Params();
    auto originRefParams = superRef->AsETSTypeReference()->Part()->TypeParams()->Params();
    for (size_t ix = 0; ix < superRefParams.size(); ++ix) {
        if (!originRefParams[ix]->IsETSTypeReference() ||
            !originRefParams[ix]->AsETSTypeReference()->Part()->TsType()->IsETSTypeParameter()) {
            continue;
        }
        auto type = originRefParams[ix]->AsETSTypeReference()->Part()->TsType();
        auto it = substitution.find(type->AsETSTypeParameter()->GetDeclNode());
        if (it != substitution.end()) {
            auto *typeParamRefPart =
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                ProgramAllocNode<ir::ETSTypeReferencePart>(it->second->Name()->Clone(ProgramAllocator(), nullptr),
                                                           ProgramAllocator());
            ES2PANDA_ASSERT(typeParamRefPart != nullptr);
            typeParamRefPart->Name()->SetParent(typeParamRefPart);
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            auto *typeParamRef = ProgramAllocNode<ir::ETSTypeReference>(typeParamRefPart, ProgramAllocator());
            ES2PANDA_ASSERT(typeParamRef != nullptr);
            typeParamRefPart->SetParent(typeParamRef);

            typeParamRef->SetParent(superPartialRefTypeParams);
            superRefParams[ix] = typeParamRef;
        }
    }
    return superPartialRefTypeParams;
}

ir::ETSTypeReference *ETSChecker::BuildSuperPartialTypeReference(
    Type *superPartialType, ir::TSTypeParameterInstantiation *superPartialRefTypeParams)
{
    ir::ETSTypeReference *superPartialRef = nullptr;
    if (superPartialType != nullptr) {
        auto *superPartialDeclNode = superPartialType->AsETSObjectType()->GetDeclNode();
        auto *clonedId =
            superPartialDeclNode->IsClassDefinition()
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                ? superPartialDeclNode->AsClassDefinition()->Ident()->Clone(ProgramAllocator(), nullptr)
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                : superPartialDeclNode->AsTSInterfaceDeclaration()->Id()->Clone(ProgramAllocator(), nullptr);
        auto *superPartialRefPart =
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            ProgramAllocNode<ir::ETSTypeReferencePart>(clonedId, superPartialRefTypeParams, nullptr,
                                                       ProgramAllocator());
        ES2PANDA_ASSERT(superPartialRefPart != nullptr);
        superPartialRefPart->Name()->SetParent(superPartialRefPart);
        if (superPartialRefTypeParams != nullptr) {
            superPartialRefTypeParams->SetParent(superPartialRefPart);
        }

        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        superPartialRef = ProgramAllocNode<ir::ETSTypeReference>(superPartialRefPart, ProgramAllocator());
        superPartialRefPart->SetParent(superPartialRef);
    }
    return superPartialRef;
}

// Extracted from 'ETSChecker::CreatePartialClassDeclaration(...)' to reduce its size and complexity
void ETSChecker::ProcessClassProperties(ir::AstNode *node, ir::ClassDefinition *const oldClassDef,
                                        ir::ClassDefinition *const newClassDef)
{
    // Only handle class properties (members) including those implemented via getters/setters.
    // Method calls on partial classes will make the class not type safe, so we don't copy any methods
    if (node->IsClassProperty()) {
        auto *const property = node->AsClassProperty();
        if (property->Id() == nullptr || (property->Modifiers() & ir::ModifierFlags::GETTER_SETTER) != 0U) {
            return;
        }

        auto const flags = static_cast<ir::ModifierFlags>(property->Modifiers() & ~ir::ModifierFlags::OVERRIDE);

        ir::TypeNode *const typeAnnotation = property->TypeAnnotation();
        checker::Type *tsType = typeAnnotation != nullptr ? typeAnnotation->GetType(this) : property->Check(this);

        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        newClassDef->EmplaceBody(CreateNullishProperty(property->Id(), flags, tsType, newClassDef));
    } else if (!oldClassDef->Implements().empty() && node->IsMethodDefinition() &&
               node->AsMethodDefinition()->Function() != nullptr &&
               node->AsMethodDefinition()->Function()->IsGetterOrSetter()) {
        auto *const method = node->AsMethodDefinition();
        ES2PANDA_ASSERT(method->Id() != nullptr);

        if (newClassDef->Scope()->FindLocal(method->Id()->Name(), varbinder::ResolveBindingOptions::VARIABLES) !=
            nullptr) {
            ES2PANDA_ASSERT(IsAnyError());
            return;
        }

        if (method->TsType() == nullptr) {
            method->Parent()->Check(this);
        }

        if (method->TsType() != nullptr && !method->TsType()->IsTypeError()) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            newClassDef->EmplaceBody(CreateNullishPropertyFromAccessor(method, newClassDef));
        }
    }
}

void ETSChecker::CreatePartialClassDeclaration(ir::ClassDefinition *const newClassDefinition,
                                               ir::ClassDefinition *const classDef)
{
    if (classDef->TypeParams() != nullptr) {
        ArenaVector<ir::TSTypeParameter *> typeParams(ProgramAllocator()->Adapter());
        for (auto *const typeParam : classDef->TypeParams()->Params()) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            auto *const newTypeParam = typeParam->Clone(ProgramAllocator(), nullptr);
            typeParams.emplace_back(newTypeParam);
        }

        auto *const newTypeParams =
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            ProgramAllocNode<ir::TSTypeParameterDeclaration>(std::move(typeParams),
                                                             classDef->TypeParams()->RequiredParams());
        newClassDefinition->SetTypeParams(newTypeParams);
        newTypeParams->SetParent(newClassDefinition);
    }

    newClassDefinition->SetVariable(newClassDefinition->Ident()->Variable());
    newClassDefinition->AddModifier(static_cast<const ir::AstNode *>(classDef)->Modifiers());

    for (auto *const node : classDef->Body()) {
        if (node->IsPublic()) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            ProcessClassProperties(node, classDef, newClassDefinition);
        }
    }

    if (classDef->IsDeclare()) {
        newClassDefinition->AddModifier(ir::ModifierFlags::DECLARE);
    }

    // The logic of IsExport() and IsDefaultExported() ASTNode methods binds them to the ClassDeclaration
    newClassDefinition->Parent()->AddModifier(static_cast<ir::ModifierFlags>(
        classDef->Parent()->Modifiers() & (ir::ModifierFlags::EXPORT | ir::ModifierFlags::DEFAULT_EXPORT)));

    // Run varbinder for new partial class to set scopes
    compiler::InitScopesPhaseETS::RunExternalNode(newClassDefinition, VarBinder());

    newClassDefinition->SetTsType(nullptr);
    newClassDefinition->Variable()->SetTsType(nullptr);
}

static void SetupFunctionParams(ir::ScriptFunction *function, checker::ETSChecker *checker)
{
    for (auto *params : function->Params()) {
        auto *paramExpr = params->AsETSParameterExpression();
        if (paramExpr->Ident()->TypeAnnotation() == nullptr) {
            paramExpr->Ident()->SetTsTypeAnnotation(nullptr);
        } else {
            auto *unionType =
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                checker->ProgramAllocNode<ir::ETSUnionType>(
                    ArenaVector<ir::TypeNode *>(
                        {paramExpr->Ident()->TypeAnnotation(),
                         // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                         checker->ProgramAllocNode<ir::ETSUndefinedType>(checker->ProgramAllocator())},
                        checker->ProgramAllocator()->Adapter()),
                    checker->ProgramAllocator());
            ES2PANDA_ASSERT(unionType != nullptr);
            paramExpr->Ident()->SetTypeAnnotation(unionType);
        }
    }
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
ir::MethodDefinition *ETSChecker::CreateNullishAccessor(ir::MethodDefinition *const accessor,
                                                        ir::TSInterfaceDeclaration *interface, parser::Program *program)
{
    const auto interfaceCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(VarBinder(), interface->Scope());

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    ir::MethodDefinition *nullishAccessor = accessor->Clone(ProgramAllocator(), interface->Body());
    auto *function = nullishAccessor->Function();
    nullishAccessor->SetRange(accessor->Range());
    function->SetRange(accessor->Function()->Range());

    if (!accessor->IsDeclare()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        ir::AstNode *newBody = CreateGetterOrSetterBodyForOptional(function->IsSetter(), true);
        function->ClearModifier(ir::ModifierFlags::ABSTRACT);
        nullishAccessor->ClearModifier(ir::ModifierFlags::ABSTRACT);
        function->SetBody(newBody);
        newBody->SetParent(function);
    } else {
        function->AddModifier(ir::ModifierFlags::DEFAULT);
        nullishAccessor->AddModifier(ir::ModifierFlags::DEFAULT);
    }

    auto *decl = ProgramAllocator()->New<varbinder::FunctionDecl>(ProgramAllocator(), nullishAccessor->Id()->Name(),
                                                                  nullishAccessor);
    auto *var = ProgramAllocator()->New<varbinder::LocalVariable>(decl, varbinder::VariableFlags::VAR);
    var->AddFlag(varbinder::VariableFlags::METHOD);
    nullishAccessor->Id()->SetVariable(var);
    nullishAccessor->SetVariable(var);

    function->SetVariable(var);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    function->SetIdent(nullishAccessor->Id()->Clone(ProgramAllocator(), function));

    if (function->IsGetter()) {
        auto *propTypeAnn = function->ReturnTypeAnnotation();

        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        function->SetReturnTypeAnnotation(ProgramAllocNode<ir::ETSUnionType>(
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            ArenaVector<ir::TypeNode *>({propTypeAnn, ProgramAllocNode<ir::ETSUndefinedType>(ProgramAllocator())},
                                        ProgramAllocator()->Adapter()),
            ProgramAllocator()));
    } else {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        SetupFunctionParams(function, this);
    }
    nullishAccessor->SetOverloads(ArenaVector<ir::MethodDefinition *>(ProgramAllocator()->Adapter()));
    nullishAccessor->AddModifier(ir::ModifierFlags::OPTIONAL);

    compiler::InitScopesPhaseETS::RunExternalNode(nullishAccessor, VarBinder());
    VarBinder()->AsETSBinder()->ResolveReferencesForScopeWithContext(nullishAccessor,
                                                                     compiler::NearestScope(nullishAccessor));
    if (!function->IsAbstract()) {
        // The synthetic accessor's source range is copied from the original accessor (SetRange above), so its
        // position-derived Program() may point at the program where the original type lives. Re-tag the node to
        // `program` (the partial type's program), the authoritative owner for code gen.
        function->SetProgram(program);
        VarBinder()->AsETSBinder()->AddCompilableFunction(function, program);
    }
    return nullishAccessor;
}

ir::TSInterfaceDeclaration *ETSChecker::CreateInterfaceProto(util::StringView name,
                                                             parser::Program *const interfaceDeclProgram,
                                                             const ir::TSInterfaceDeclaration *interfaceDecl)
{
    const bool isStatic = interfaceDecl->IsStatic();
    const ir::ModifierFlags flags = interfaceDecl->Modifiers();
    const auto globalCtx =
        varbinder::LexicalScope<varbinder::GlobalScope>::Enter(VarBinder(), interfaceDeclProgram->GlobalScope());

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *const interfaceId = ProgramAllocNode<ir::Identifier>(name, ProgramAllocator());
    ES2PANDA_ASSERT(interfaceId);
    const auto [decl, var] = VarBinder()->NewVarDecl<varbinder::InterfaceDecl>(interfaceId->Start(), ProgramAllocator(),
                                                                               interfaceId->Name());
    ES2PANDA_ASSERT(interfaceId != nullptr);
    interfaceId->SetVariable(var);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *body = ProgramAllocNode<ir::TSInterfaceBody>(ArenaVector<ir::AstNode *>(ProgramAllocator()->Adapter()));
    ArenaVector<ir::TSInterfaceHeritage *> extends(ProgramAllocator()->Adapter());

    ArenaVector<ir::TSTypeParameter *> typeParams(ProgramAllocator()->Adapter());
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *newTypeParams = ProgramAllocNode<ir::TSTypeParameterDeclaration>(std::move(typeParams), typeParams.size());
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto partialInterface = ProgramAllocNode<ir::TSInterfaceDeclaration>(
        ProgramAllocator(), std::move(extends),
        ir::TSInterfaceDeclaration::ConstructorData {interfaceId, newTypeParams, body, isStatic,
                                                     interfaceDeclProgram != VarBinder()->Program(),
                                                     Language(Language::Id::ETS)});

    const auto classCtx = varbinder::LexicalScope<varbinder::ClassScope>(VarBinder());
    ES2PANDA_ASSERT(partialInterface != nullptr);
    partialInterface->TypeParams()->SetParent(partialInterface);
    classCtx.GetScope()->SetParent(interfaceDecl->Scope()->Parent());
    partialInterface->SetScope(classCtx.GetScope());
    partialInterface->SetVariable(var);
    decl->BindNode(partialInterface);

    // Put class declaration in global scope, and in program AST
    partialInterface->SetParent(interfaceDeclProgram->Ast());
    interfaceDeclProgram->Ast()->AddStatement(partialInterface);
    interfaceDeclProgram->GlobalScope()->InsertBinding(name, var);

    partialInterface->AddModifier(flags);
    return partialInterface;
}

void ETSChecker::CreatePartialTypeInterfaceMethods(ir::TSInterfaceDeclaration *const interfaceDecl,
                                                   ir::TSInterfaceDeclaration *partialInterface,
                                                   parser::Program *partialProgram)
{
    auto &partialInterfaceMethods = partialInterface->Body()->Body();

    auto const addNullishAccessor = [this, &partialInterfaceMethods](ir::MethodDefinition *accessor) -> void {
        (void)this;
        auto const it = std::find_if(partialInterfaceMethods.begin(), partialInterfaceMethods.end(),
                                     [accessor](ir::AstNode const *node) -> bool {
                                         return node->AsMethodDefinition()->Id()->Name() == accessor->Id()->Name();
                                     });
        if (it == partialInterfaceMethods.end()) {
            accessor->Function()->ClearFlag(ir::ScriptFunctionFlags::OVERLOAD);
            partialInterfaceMethods.emplace_back(accessor);
        } else if (accessor->Function()->IsSetter()) {
            ES2PANDA_ASSERT_POS((*it)->AsMethodDefinition()->Function()->IsGetter(), (*it)->Start());
            (*it)->AsMethodDefinition()->AddOverload(accessor);
            accessor->SetParent(*it);
            accessor->Function()->AddFlag(ir::ScriptFunctionFlags::OVERLOAD);
        } else {
            ERROR_SANITY_CHECK(this, (*it)->AsMethodDefinition()->Function()->IsSetter(), return void());
            auto setter = (*it)->AsMethodDefinition();
            accessor->AddOverload(setter);
            setter->SetParent(accessor);
            setter->Function()->AddFlag(ir::ScriptFunctionFlags::OVERLOAD);
            accessor->Function()->ClearFlag(ir::ScriptFunctionFlags::OVERLOAD);
            partialInterfaceMethods.erase(it);
            partialInterfaceMethods.emplace_back(accessor);
        }
    };

    for (auto *const prop : interfaceDecl->Body()->Body()) {
        if (!prop->IsMethodDefinition()) {
            continue;
        }

        auto *const method = prop->AsMethodDefinition();
        auto *func = method->Function();
        ES2PANDA_ASSERT(func != nullptr);
        ES2PANDA_ASSERT((func->Flags() & ir::ScriptFunctionFlags::OVERLOAD) == 0U);

        if (func->IsGetterOrSetter()) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            addNullishAccessor(CreateNullishAccessor(method, partialInterface, partialProgram));
        }

        for (auto *overload : method->Overloads()) {
            ES2PANDA_ASSERT(overload->Function() != nullptr);
            if (overload->Function()->IsGetterOrSetter()) {
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                addNullishAccessor(CreateNullishAccessor(overload, partialInterface, partialProgram));
            }
        }
    }
}

ir::AstNode *ETSChecker::CreateGetterOrSetterBodyForOptional(bool isSetter, bool isOptional)
{
    if (!isOptional) {
        return nullptr;
    }

    ArenaVector<ir::Statement *> returnStatement(ProgramAllocator()->Adapter());
    if (isSetter) {
        auto errorIdent =
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            ProgramAllocNode<ir::Identifier>(compiler::Signatures::INVALID_STOREACCESS_ERROR, ProgramAllocator());
        ArenaVector<ir::Expression *> arguments(ProgramAllocator()->Adapter());
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto newExpr = ProgramAllocNode<ir::ETSNewClassInstanceExpression>(errorIdent, std::move(arguments));
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto throwStmt = ProgramAllocNode<ir::ThrowStatement>(newExpr);
        returnStatement.emplace_back(throwStmt);
    } else {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *undef = ProgramAllocNode<ir::UndefinedLiteral>();
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *rtStmt = ProgramAllocNode<ir::ReturnStatement>(undef);
        returnStatement.emplace_back(rtStmt);
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return ProgramAllocNode<ir::BlockStatement>(ProgramAllocator(), std::move(returnStatement));
}

Type *ETSChecker::CreatePartialTypeInterfaceDecl(ir::TSInterfaceDeclaration *const interfaceDecl,
                                                 ETSObjectType *const typeToBePartial,
                                                 ir::TSInterfaceDeclaration *partialInterface,
                                                 parser::Program *partialProgram)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    CreatePartialTypeInterfaceMethods(interfaceDecl, partialInterface, partialProgram);
    // Create nullish properties of the partial class
    // Build the new Partial class based on the 'T' type parameter of 'Partial<T>'
    std::unordered_map<ir::TSTypeParameter *, ir::TSTypeParameter *> substitution {};

    if (interfaceDecl->TypeParams() != nullptr) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        partialInterface->SetTypeParams(ProcessTypeParamAndGenSubstitution(interfaceDecl->TypeParams(), substitution,
                                                                           partialInterface->TypeParams()));
    }

    compiler::InitScopesPhaseETS::RunExternalNode(partialInterface, VarBinder());

    auto methodscope = partialInterface->Scope()->AsClassScope()->InstanceMethodScope();
    // Add getter methods to instancemethodscope.
    for (auto *const prop : partialInterface->Body()->Body()) {
        auto *func = prop->AsMethodDefinition()->Function();
        ES2PANDA_ASSERT(func != nullptr);
        if (prop->IsMethodDefinition() && func->IsGetter()) {
            auto *decl = ProgramAllocator()->New<varbinder::FunctionDecl>(
                ProgramAllocator(), prop->AsMethodDefinition()->Key()->AsIdentifier()->Name(), prop);
            methodscope->AddDecl(ProgramAllocator(), decl, ScriptExtension::ETS);
        }
    }

    // Create partial type for super type
    for (auto *extend : interfaceDecl->Extends()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *t = extend->Expr()->AsETSTypeReference()->Part()->GetType(this);
        if (t->IsTypeError()) {
            continue;
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        if (auto *superPartialType = CreatePartialType(t); superPartialType != nullptr) {
            ir::TSTypeParameterInstantiation *superPartialRefTypeParams =
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                CreateNewSuperPartialRefTypeParamsDecl(substitution, superPartialType, extend->Expr());

            ir::ETSTypeReference *superPartialRef =
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                BuildSuperPartialTypeReference(superPartialType, superPartialRefTypeParams);
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            partialInterface->EmplaceExtends(ProgramAllocNode<ir::TSInterfaceHeritage>(superPartialRef));
            partialInterface->Extends().back()->SetParent(partialInterface);
        }
    }

    compiler::SetSourceRangesRecursively(partialInterface, interfaceDecl->Range());

    auto *const partialType = partialInterface->Check(this)->AsETSObjectType();
    partialType->SetBaseType(typeToBePartial->GetOriginalBaseType());

    return partialType;
}

void ETSChecker::CreateConstructorForPartialType(ir::ClassDefinition *const partialClassDef,
                                                 checker::ETSObjectType *const partialType,
                                                 varbinder::RecordTable *const recordTable)
{
    // Create scopes
    auto *const scope = partialClassDef->Scope()->AsClassScope();
    const auto classCtx = varbinder::LexicalScope<varbinder::ClassScope>::Enter(VarBinder(), scope);

    // Create ctor
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *const ctor = CreateNonStaticClassInitializer(classCtx.GetScope(), recordTable);
    auto *const ctorFunc = ctor->Function();
    if (partialClassDef->IsDeclare()) {
        ctorFunc->AddFlag(ir::ScriptFunctionFlags::EXTERNAL);
    }
    auto *const ctorId = ctor->Function()->Id();

    // Handle bindings, create method decl for ctor
    ctorFunc->Scope()->Find(varbinder::VarBinder::MANDATORY_PARAM_THIS).variable->SetTsType(partialType);
    partialType->AddConstructSignature(ctorFunc->Signature());
    ctorFunc->Signature()->SetOwner(partialType);
    ctor->SetParent(partialClassDef);
    ctorId->SetVariable(ProgramAllocator()->New<varbinder::LocalVariable>(
        ProgramAllocator()->New<varbinder::MethodDecl>(ctorId->Name()), varbinder::VariableFlags::METHOD));
    ctor->Id()->SetVariable(ctorId->Variable());

    // Put ctor in partial class body
    partialClassDef->EmplaceBody(ctor);
}

ir::ClassDefinition *ETSChecker::CreateClassPrototype(util::StringView name, parser::Program *const classDeclProgram)
{
    const auto globalCtx =
        varbinder::LexicalScope<varbinder::GlobalScope>::Enter(VarBinder(), classDeclProgram->GlobalScope());

    // Create class name, and declaration variable
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *const classId = ProgramAllocNode<ir::Identifier>(name, ProgramAllocator());
    ES2PANDA_ASSERT(classId != nullptr);
    const auto [decl, var] = VarBinder()->NewVarDecl<varbinder::ClassDecl>(classId->Start(), classId->Name());
    classId->SetVariable(var);

    // Create class definition node
    const auto classCtx = varbinder::LexicalScope<varbinder::ClassScope>(VarBinder());
    auto *const classDef =
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        ProgramAllocNode<ir::ClassDefinition>(ProgramAllocator(), classId, ir::ClassDefinitionModifiers::DECLARATION,
                                              ir::ModifierFlags::NONE, Language(Language::Id::ETS));
    ES2PANDA_ASSERT(classDef != nullptr);
    classDef->SetScope(classCtx.GetScope());
    classDef->SetVariable(var);

    // Create class declaration node
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *const classDecl = ProgramAllocNode<ir::ClassDeclaration>(classDef, ProgramAllocator());
    ES2PANDA_ASSERT(classDecl != nullptr);
    classDecl->SetProgram(classDeclProgram);
    classDef->SetProgram(classDeclProgram);
    classId->SetProgram(classDeclProgram);

    // Class definition is scope bearer, not class declaration
    classDef->Scope()->BindNode(classDecl->Definition());
    decl->BindNode(classDef);

    // Put class declaration in global scope, and in program AST
    classDeclProgram->Ast()->AddStatement(classDecl);
    classDeclProgram->GlobalScope()->InsertBinding(name, var);
    classDef->SetRange(classDecl->Range());

    return classDef;
}

varbinder::Variable *ETSChecker::SearchNamesInMultiplePrograms(const std::set<const parser::Program *> &programs,
                                                               const std::set<util::StringView> &classNamesToFind)
{
    for (const auto *const program : programs) {
        for (const auto &className : classNamesToFind) {
            auto *const var = program->GlobalScope()->Find(className, varbinder::ResolveBindingOptions::ALL).variable;
            if (var == nullptr) {
                continue;
            }

            if (var->TsType() == nullptr) {
                var->Declaration()->Node()->Check(this);
            }

            return var;
        }
    }

    return nullptr;
}

Type *ETSChecker::HandleUnionForPartialType(ETSUnionType *const typeToBePartial)
{
    // Convert a union type to partial, by converting all types in it to partial, and making a new union
    // type out of them
    const auto *const unionTypeNode = typeToBePartial->AsETSUnionType();
    std::vector<Type *> newTypesForUnion;

    for (auto *const typeFromUnion : unionTypeNode->ConstituentTypes()) {
        if ((typeFromUnion->Variable() != nullptr) && (typeFromUnion->Variable()->Declaration() != nullptr)) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            newTypesForUnion.emplace_back(CreatePartialType(typeFromUnion));
        } else {
            newTypesForUnion.emplace_back(typeFromUnion);
        }
    }

    return CreateETSUnionType(std::move(newTypesForUnion));
}

Type *ETSChecker::CreatePartialTypeClassDef(ir::ClassDefinition *const partialClassDef,
                                            ir::ClassDefinition *const classDef, ETSObjectType *const typeToBePartial,
                                            varbinder::RecordTable *const recordTableToUse)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    CreatePartialClassDeclaration(partialClassDef, classDef);

    // Create partial type for super type
    if (typeToBePartial == GlobalETSObjectType()) {
        // Run checker
        auto *const partialType = partialClassDef->Check(this)->AsETSObjectType();

        partialType->SetBaseType(typeToBePartial);
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        CreateConstructorForPartialType(partialClassDef, partialType, recordTableToUse);
        return partialType;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    Type *const partialSuper = CreatePartialType((classDef->Super() == nullptr || !classDef->Super()->IsTypeNode())
                                                     ? GlobalETSObjectType()
                                                     : classDef->Super()->TsType());
    if (partialSuper->IsTypeError()) {
        return GlobalTypeError();
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    partialClassDef->SetSuper(ProgramAllocator()->New<ir::OpaqueTypeNode>(partialSuper, ProgramAllocator()));

    // Run checker
    ETSObjectType *const partialType = partialClassDef->Check(this)->AsETSObjectType();
    partialType->SetBaseType(typeToBePartial->GetOriginalBaseType());

    for (auto *interface : typeToBePartial->Interfaces()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *partialInterface = CreatePartialType(interface)->AsETSObjectType();
        if (HasInterfaceInHierarchy(partialSuper->AsETSObjectType(), partialInterface)) {
            continue;
        }
        partialType->AddInterface(partialInterface);
    }

    if (IsTypeIdenticalTo(partialSuper, partialType)) {
        LogError(diagnostic::CYCLIC_CLASS_SUPER_TYPE, {}, classDef->Start());
        return GlobalTypeError();
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    CreateConstructorForPartialType(partialClassDef, partialType, recordTableToUse);
    return partialType;
}

std::pair<ir::ScriptFunction *, ir::Identifier *> ETSChecker::CreateScriptFunctionForConstructor(
    varbinder::FunctionScope *const scope)
{
    ArenaVector<ir::Statement *> statements(ProgramAllocator()->Adapter());
    ArenaVector<ir::Expression *> params(ProgramAllocator()->Adapter());

    ir::ScriptFunction *func {};
    ir::Identifier *id {};

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *const body = ProgramAllocNode<ir::BlockStatement>(ProgramAllocator(), std::move(statements));
    ES2PANDA_ASSERT(body != nullptr);
    body->SetScope(scope);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    id = ProgramAllocNode<ir::Identifier>(util::UString(std::string("constructor"), ProgramAllocator()).View(),
                                          ProgramAllocator());
    auto funcSignature = ir::FunctionSignature(nullptr, std::move(params), nullptr);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    func = ProgramAllocNode<ir::ScriptFunction>(
        ProgramAllocator(), ir::ScriptFunction::ScriptFunctionData {body, std::move(funcSignature),
                                                                    ir::ScriptFunctionFlags::CONSTRUCTOR |
                                                                        ir::ScriptFunctionFlags::EXPRESSION,
                                                                    ir::ModifierFlags::PUBLIC});
    ES2PANDA_ASSERT(func != nullptr);
    func->SetScope(scope);
    scope->BindNode(func);
    func->SetIdent(id);
    VarBinder()->AsETSBinder()->AddFunctionThisParam(func);

    return std::make_pair(func, id);
}

ir::MethodDefinition *ETSChecker::CreateNonStaticClassInitializer(varbinder::ClassScope *classScope,
                                                                  varbinder::RecordTable *const recordTable)
{
    const auto classCtx = varbinder::LexicalScope<varbinder::ClassScope>::Enter(VarBinder(), classScope);

    auto *paramScope = ProgramAllocator()->New<varbinder::FunctionParamScope>(ProgramAllocator(), classScope);
    auto *const functionScope = ProgramAllocator()->New<varbinder::FunctionScope>(ProgramAllocator(), paramScope);
    functionScope->BindParamScope(paramScope);
    paramScope->BindFunctionScope(functionScope);

    const auto funcParamCtx = varbinder::LexicalScope<varbinder::FunctionParamScope>::Enter(VarBinder(), paramScope);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto [func, id] = CreateScriptFunctionForConstructor(functionScope);

    paramScope->BindNode(func);
    functionScope->BindNode(func);

    auto *const signatureInfo = CreateSignatureInfo();
    auto *const signature = CreateSignature(signatureInfo, GlobalETSUndefinedType(), func);
    func->SetSignature(signature);

    VarBinder()->AsETSBinder()->BuildInternalNameWithCustomRecordTable(func, recordTable);
    VarBinder()->AsETSBinder()->BuildFunctionName(func);
    if (!recordTable->IsExternal()) {
        VarBinder()->AddCompilableFunctionScope(functionScope, recordTable->Program());
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *funcExpr = ProgramAllocNode<ir::FunctionExpression>(func);
    auto *const ctor =
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        ProgramAllocNode<ir::MethodDefinition>(ir::MethodDefinitionKind::CONSTRUCTOR,
                                               // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                                               id->Clone(ProgramAllocator(), classScope->Node()), funcExpr,
                                               ir::ModifierFlags::NONE, ProgramAllocator(), false);

    auto *const funcType = CreateETSMethodType(id->Name(), {{signature}, ProgramAllocator()->Adapter()});
    ctor->SetTsType(funcType);

    return ctor;
}

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Readonly utility type
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
Type *ETSChecker::GetReadonlyType(Type *type)
{
    auto &typeCache = CachedUtilityTypes<static_cast<std::size_t>(UtilityType::READONLY)>();
    if (const auto found = typeCache.find(type); found != typeCache.end()) {
        return found->second;
    }

    auto const cacheType = [&typeCache, type](Type *readonlyType) {
        typeCache.emplace(type, readonlyType);
        return readonlyType;
    };

    ES2PANDA_ASSERT(type != nullptr);
    if (type->IsETSArrayType()) {
        auto *arrType = type->AsETSArrayType();
        ETSArrayType *const clonedArrayType =
            ProgramAllocator()->New<ETSArrayType>(arrType->ElementType(), arrType->IsValueArray());
        ES2PANDA_ASSERT(clonedArrayType != nullptr);
        clonedArrayType->AddTypeFlag(TypeFlag::READONLY);
        return cacheType(clonedArrayType);
    }
    if (type->IsETSTupleType()) {
        Type *const clonedType = type->Clone(this);
        clonedType->AddTypeFlag(TypeFlag::READONLY);
        return cacheType(clonedType);
    }

    if (type->IsETSObjectType()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        type->AsETSObjectType()->InstanceFields();
        auto *clonedType = type->Clone(this)->AsETSObjectType();
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        MakePropertiesReadonly(clonedType);
        return cacheType(clonedType);
    }

    if (type->IsETSTypeParameter()) {
        return cacheType(ProgramAllocator()->New<ETSReadonlyType>(type->AsETSTypeParameter()));
    }

    if (type->IsETSUnionType()) {
        std::vector<Type *> unionTypes;
        for (auto *t : type->AsETSUnionType()->ConstituentTypes()) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            unionTypes.emplace_back(t->IsETSObjectType() ? GetReadonlyType(t) : t->Clone(this));
        }
        return cacheType(CreateETSUnionType(std::move(unionTypes)));
    }

    return type;
}

template <PropertyType PROP_TYPE>
void ETSChecker::MakePropertyReadonly(ETSObjectType *const classType, varbinder::LocalVariable *const prop)
{
    auto const propType = prop->Declaration()->Node()->Check(this);

    auto *newDecl = ProgramAllocator()->New<varbinder::ReadonlyDecl>(prop->Name(), prop->Declaration()->Node());
    auto *const propCopy = prop->Copy(ProgramAllocator(), newDecl);
    propCopy->AddFlag(varbinder::VariableFlags::READONLY);
    propCopy->SetTsType(propType);

    classType->RemoveProperty<PROP_TYPE>(prop);
    classType->AddProperty<PROP_TYPE>(propCopy);
}

void ETSChecker::MakePropertiesReadonly(ETSObjectType *const classType)
{
    classType->AddTypeFlag(TypeFlag::READONLY);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    for (auto const &prop : classType->InstanceFields()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        MakePropertyReadonly<PropertyType::INSTANCE_FIELD>(classType, prop.second);
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    for (auto const &prop : classType->StaticFields()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        MakePropertyReadonly<PropertyType::STATIC_FIELD>(classType, prop.second);
    }

    if (classType->SuperType() != nullptr) {
        auto *const superProp = classType->SuperType()->Clone(this)->AsETSObjectType();
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        MakePropertiesReadonly(superProp);
        if (classType->SuperType() == GlobalETSObjectType()) {
            superProp->SetSuperType(GlobalETSObjectType());
        }

        classType->SetSuperType(superProp);
    }
}

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Required utility type
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
Type *ETSChecker::HandleRequiredType(Type *typeToBeRequired)
{
    auto &typeCache = CachedUtilityTypes<static_cast<std::size_t>(UtilityType::REQUIRED)>();
    if (const auto found = typeCache.find(typeToBeRequired); found != typeCache.end()) {
        return found->second;
    }

    auto const cacheType = [&typeCache, typeToBeRequired](Type *requiredType) {
        typeCache.emplace(typeToBeRequired, requiredType);
        return requiredType;
    };

    if (typeToBeRequired->IsETSTypeParameter()) {
        auto *const requiredClone = typeToBeRequired->Clone(this);
        requiredClone->AddTypeFlag(TypeFlag::ETS_REQUIRED_TYPE_PARAMETER);
        return cacheType(requiredClone);
    }

    if (typeToBeRequired->IsETSUnionType()) {
        std::vector<Type *> unionTypes;
        for (auto *type : typeToBeRequired->AsETSUnionType()->ConstituentTypes()) {
            if (type->IsETSObjectType()) {
                type = type->Clone(this);
                ES2PANDA_ASSERT(type != nullptr);
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                MakePropertiesNonNullish(type->AsETSObjectType());
            }

            if (type->IsETSNullType() || type->IsETSUndefinedType()) {
                continue;
            }

            unionTypes.emplace_back(type);
        }

        return cacheType(CreateETSUnionType(std::move(unionTypes)));
    }

    if (typeToBeRequired->IsETSObjectType()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        typeToBeRequired->AsETSObjectType()->InstanceFields();  // call to instantiate properties
    }

    typeToBeRequired = typeToBeRequired->Clone(this);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    MakePropertiesNonNullish(typeToBeRequired->AsETSObjectType());

    return cacheType(typeToBeRequired);
}

void ETSChecker::MakePropertiesNonNullish(ETSObjectType *const classType)
{
    classType->AddObjectFlag(ETSObjectFlags::REQUIRED);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    classType->InstanceFields();

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    for (const auto &[_, propVar] : classType->InstanceFields()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        MakePropertyNonNullish<PropertyType::INSTANCE_FIELD>(classType, propVar);
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    for (const auto &[_, propVar] : classType->StaticFields()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        MakePropertyNonNullish<PropertyType::STATIC_FIELD>(classType, propVar);
    }

    if (classType->SuperType() != nullptr) {
        auto *const superRequired = classType->SuperType()->Clone(this)->AsETSObjectType();
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        MakePropertiesNonNullish(superRequired);
        classType->SetSuperType(superRequired);
    }
}

template <PropertyType PROP_TYPE>
void ETSChecker::MakePropertyNonNullish(ETSObjectType *const classType, varbinder::LocalVariable *const prop)
{
    auto *const propType = prop->TsType();
    auto *const nonNullishPropType = GetNonNullishType(propType);

    auto *const propCopy = prop->Copy(ProgramAllocator(), prop->Declaration());
    ES2PANDA_ASSERT(propCopy != nullptr);
    propCopy->SetTsType(nonNullishPropType);
    classType->RemoveProperty<PROP_TYPE>(prop);
    classType->AddProperty<PROP_TYPE>(propCopy);
}

static bool StringEqualsPropertyName(const util::StringView pname1, const ir::Expression *const prop2Key)
{
    util::StringView pname2;
    if (prop2Key->IsStringLiteral()) {
        pname2 = prop2Key->AsStringLiteral()->Str();
    } else if (prop2Key->IsIdentifier()) {
        pname2 = prop2Key->AsIdentifier()->Name();
    }

    return pname1 == pname2;
}

void ETSChecker::ValidateObjectLiteralForRequiredType(const ETSObjectType *const requiredType,
                                                      const ir::ObjectExpression *const initObjExpr)
{
    auto initObjExprContainsField = [&initObjExpr](const util::StringView pname1) {
        return std::find_if(initObjExpr->Properties().begin(), initObjExpr->Properties().end(),
                            [&pname1](const ir::Expression *const initProp) {
                                return StringEqualsPropertyName(pname1, initProp->AsProperty()->Key());
                            }) != initObjExpr->Properties().end();
    };

    if (requiredType->HasObjectFlag(ETSObjectFlags::INTERFACE)) {
        for (const auto *method : requiredType->GetDeclNode()->AsTSInterfaceDeclaration()->Body()->Body()) {
            if (!method->IsMethodDefinition()) {
                continue;
            }
            auto *func = method->AsMethodDefinition()->Function();
            ES2PANDA_ASSERT(func != nullptr);
            if (!func->IsGetter()) {
                continue;
            }

            auto fieldname = method->AsMethodDefinition()->Key()->AsIdentifier()->Name();
            if (!initObjExprContainsField(fieldname)) {
                LogError(diagnostic::REQUIRED_PROP_MISSING_INIT, {fieldname, requiredType->Name()},
                         initObjExpr->Start());
            }
        }

        return;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    for (const auto &[propName, _] : requiredType->InstanceFields()) {
        if (!initObjExprContainsField(propName)) {
            LogError(diagnostic::REQUIRED_PROP_MISSING_INIT, {propName, requiredType->Name()}, initObjExpr->Start());
        }
    }
}

}  // namespace ark::es2panda::checker
