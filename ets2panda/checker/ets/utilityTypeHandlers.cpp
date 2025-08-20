/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ir/expressions/identifier.h"
#include "ir/ets/etsNullishTypes.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "ir/ets/etsUnionType.h"
#include "ir/expressions/literals/undefinedLiteral.h"
#include "varbinder/ETSBinder.h"
#include "checker/types/ets/etsPartialTypeParameter.h"
#include "checker/types/ets/etsAwaitedType.h"
#include "compiler/lowering/util.h"
#include "util/nameMangler.h"

#include <checker/types/gradualType.h>

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

static bool ValidBaseTypeOfRequiredAndPartial(Type *baseType)
{
    Type *type = baseType->MaybeBaseTypeOfGradualType();
    return type->IsETSObjectType() && (type->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::INTERFACE) ||
                                       type->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::CLASS));
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

    if (!baseType->IsETSReferenceType()) {
        LogError(diagnostic::UTIL_TYPE_OF_NONREFERENCE, {}, typeParams->Start());
        return GlobalTypeError();
    }

    if (baseType->IsETSAnyType()) {
        return baseType;
    }

    if ((utilityType == compiler::Signatures::PARTIAL_TYPE_NAME ||
         utilityType == compiler::Signatures::REQUIRED_TYPE_NAME) &&
        !ValidBaseTypeOfRequiredAndPartial(baseType)) {
        LogError(diagnostic::MUST_BE_CLASS_INTERFACE_TYPE, {utilityType}, typeParams->Start());
        return GlobalTypeError();
    }

    if (utilityType == compiler::Signatures::PARTIAL_TYPE_NAME) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return CreatePartialType(baseType);
    }

    if (utilityType == compiler::Signatures::READONLY_TYPE_NAME) {
        return GetReadonlyType(baseType);
    }

    if (utilityType == compiler::Signatures::REQUIRED_TYPE_NAME) {
        return HandleRequiredType(baseType);
    }

    if (utilityType == compiler::Signatures::AWAITED_TYPE_NAME) {
        return HandleAwaitedUtilityType(baseType);
    }

    LogError(diagnostic::UTILITY_TYPE_UNIMPLEMENTED, {}, typeParams->Start());
    return baseType;
}

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Awaited utility type
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
bool ETSChecker::IsPromiseType(Type *type)
{
    if (type->IsETSUnionType()) {
        for (Type *constituentType : type->AsETSUnionType()->ConstituentTypes()) {
            if (!IsPromiseType(constituentType)) {
                return false;
            }
        }
        return true;
    }
    return type->IsETSObjectType() && type->AsETSObjectType()->GetOriginalBaseType() == GlobalBuiltinPromiseType();
}

Type *ETSChecker::UnwrapPromiseType(checker::Type *type)
{
    Type *promiseType = GlobalBuiltinPromiseType();
    while (type->IsETSObjectType() && type->AsETSObjectType()->GetOriginalBaseType() == promiseType) {
        type = type->AsETSObjectType()->TypeArguments().at(0);
    }
    if (!type->IsETSUnionType()) {
        return type;
    }
    const auto &ctypes = type->AsETSUnionType()->ConstituentTypes();
    auto it = std::find_if(ctypes.begin(), ctypes.end(), [promiseType](checker::Type *t) {
        return t == promiseType || (t->IsETSObjectType() && t->AsETSObjectType()->GetBaseType() == promiseType);
    });
    if (it == ctypes.end()) {
        return type;
    }
    ArenaVector<Type *> newCTypes(ctypes);
    do {
        size_t index = it - ctypes.begin();
        newCTypes[index] = UnwrapPromiseType(ctypes[index]);
        ++it;
        it = std::find_if(it, ctypes.end(), [promiseType](checker::Type *t) {
            return t == promiseType || t->AsETSObjectType()->GetBaseType() == promiseType;
        });
    } while (it != ctypes.end());
    return CreateETSUnionType(std::move(newCTypes));
}

Type *ETSChecker::HandleAwaitedUtilityType(Type *typeToBeAwaited)
{
    if (typeToBeAwaited->IsETSTypeParameter()) {
        return ProgramAllocator()->New<ETSAwaitedType>(typeToBeAwaited->AsETSTypeParameter());
    }

    if (typeToBeAwaited->IsETSUnionType()) {
        ArenaVector<Type *> awaitedTypes(ProgramAllocator()->Adapter());
        for (Type *type : typeToBeAwaited->AsETSUnionType()->ConstituentTypes()) {
            Type *unwrapped = IsPromiseType(type) ? type->AsETSObjectType()->TypeArguments().at(0) : type;
            if (unwrapped->IsETSTypeParameter()) {
                unwrapped = HandleAwaitedUtilityType(unwrapped);
            }
            awaitedTypes.push_back(UnwrapPromiseType(unwrapped));
        }
        return CreateETSUnionType(std::move(awaitedTypes));
    }

    if (IsPromiseType(typeToBeAwaited)) {
        Type *typeArg = typeToBeAwaited->AsETSObjectType()->TypeArguments().at(0);
        auto unwrappedType = UnwrapPromiseType(typeArg);
        return unwrappedType->IsETSTypeParameter() ? HandleAwaitedUtilityType(unwrappedType) : unwrappedType;
    }

    return typeToBeAwaited;
}

Type *ETSChecker::HandleAwaitExpression(Type *typeToBeAwaited, ir::AwaitExpression *expr)
{
    Relation()->SetFlags(TypeRelationFlag::IGNORE_TYPE_PARAMETERS);
    if (!typeToBeAwaited->IsETSTypeParameter() &&
        !Relation()->IsSupertypeOf(GlobalBuiltinPromiseType(), typeToBeAwaited)) {
        LogError(diagnostic::AWAITED_TYPE_NOT_PROMISE, {typeToBeAwaited}, expr->Start());
        return typeToBeAwaited;
    }
    Relation()->RemoveFlags(TypeRelationFlag::IGNORE_TYPE_PARAMETERS);

    return HandleAwaitedUtilityType(typeToBeAwaited);
}

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Partial utility type
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

static std::pair<util::StringView, util::StringView> GetPartialClassName(ETSChecker *checker, ir::AstNode *typeNode)
{
    // Partial class name for class 'T' will be 'T$partial'
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

static std::pair<parser::Program *, varbinder::RecordTable *> GetPartialClassProgram(ETSChecker *checker,
                                                                                     ir::AstNode *typeNode)
{
    auto classDefProgram = typeNode->GetTopStatement()->AsETSModule()->Program();
    if (classDefProgram == checker->VarBinder()->AsETSBinder()->GetGlobalRecordTable()->Program()) {
        return {classDefProgram, checker->VarBinder()->AsETSBinder()->GetGlobalRecordTable()};
    }
    return {classDefProgram, checker->VarBinder()->AsETSBinder()->GetExternalRecordTable().at(classDefProgram)};
}

template <typename T>
static T *CloneNodeIfNotNullptr(T *node, ArenaAllocator *allocator)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return node != nullptr ? node->Clone(allocator, nullptr) : nullptr;
}

Type *ETSChecker::CreatePartialType(Type *const typeToBePartial)
{
    ES2PANDA_ASSERT(typeToBePartial->IsETSReferenceType());
    if (typeToBePartial->IsTypeError() || typeToBePartial->IsETSNeverType() || typeToBePartial->IsETSAnyType()) {
        return typeToBePartial;
    }

    if (typeToBePartial->IsGradualType()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return CreatePartialType(typeToBePartial->AsGradualType()->GetBaseType());
    }

    if (typeToBePartial->IsETSTypeParameter()) {
        return CreatePartialTypeParameter(typeToBePartial->AsETSTypeParameter());
    }

    if (typeToBePartial->IsETSUnionType()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return HandleUnionForPartialType(typeToBePartial->AsETSUnionType());
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return CreatePartialTypeClass(typeToBePartial->AsETSObjectType(),
                                  typeToBePartial->Variable()->Declaration()->Node());
}

Type *ETSChecker::CreatePartialTypeParameter(ETSTypeParameter *typeToBePartial)
{
    return ProgramAllocator()->New<ETSPartialTypeParameter>(typeToBePartial, this);
}

Type *ETSChecker::CreatePartialTypeClass(ETSObjectType *typeToBePartial, ir::AstNode *typeDeclNode)
{
    auto const [partialName, partialQualifiedName] = GetPartialClassName(this, typeDeclNode);
    auto const [partialProgram, recordTable] = GetPartialClassProgram(this, typeDeclNode);

    // Check if we've already generated the partial class, then don't do it again
    const auto classNameToFind =
        partialProgram == VarBinder()->Program() || VarBinder()->IsGenStdLib() || partialProgram->IsGenAbcForExternal()
            ? partialName
            : partialQualifiedName;
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
    BuildBasicClassProperties(partialClassDef);

    // If class prototype was created before, then we cached it's type. In that case return it.
    // This handles cases where a Partial<T> presents in class T, because during generating T$partial we'd need the
    // complete class T$partial which is not present at the time. Binding it's own type for it however will make it
    // possible to resolve member references later, when the full T$partial class was created.
    if (const auto found = NamedTypeStack().find(partialClassDef->TsType()); found != NamedTypeStack().end()) {
        return *found;
    }

    const varbinder::BoundContext boundCtx(recordTable, partialClassDef);

    NamedTypeStackElement ntse(this, partialClassDef->TsType());

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
    auto const [partialProgram, recordTable] = GetPartialClassProgram(this, interfaceDecl);

    auto *const partialInterDecl =
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        CreateInterfaceProto(partialName, partialProgram, interfaceDecl->IsStatic(), interfaceDecl->Modifiers());
    partialInterDecl->SetInternalName(partialQualifiedName);

    if (const auto found = NamedTypeStack().find(partialInterDecl->TsType()); found != NamedTypeStack().end()) {
        return *found;
    }

    const varbinder::BoundContext boundCtx(recordTable, partialInterDecl);

    // If class is external, put partial of it in global scope for the varbinder
    if (partialProgram != VarBinder()->Program()) {
        VarBinder()->Program()->GlobalScope()->InsertBinding(partialInterDecl->Id()->Name(),
                                                             partialInterDecl->Variable());
    }

    auto savedScope = VarBinder()->TopScope();
    VarBinder()->ResetTopScope(partialProgram->GlobalScope());
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *partialType = CreatePartialTypeInterfaceDecl(interfaceDecl, typeToBePartial, partialInterDecl);
    VarBinder()->ResetTopScope(savedScope);
    ES2PANDA_ASSERT(partialType != nullptr);
    NamedTypeStackElement ntse(this, partialType);

    return partialType;
}

ir::ClassProperty *ETSChecker::CreateNullishPropertyFromAccessor(ir::MethodDefinition *const accessor,
                                                                 ir::ClassDefinition *const newClassDefinition)
{
    auto *id = accessor->Id();
    ES2PANDA_ASSERT(id != nullptr);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *ident = id->Clone(ProgramAllocator(), nullptr);
    ES2PANDA_ASSERT(accessor->Function() != nullptr);
    auto modifierFlag = accessor->Function()->IsGetter() && accessor->Overloads().empty() ? ir::ModifierFlags::READONLY
                                                                                          : ir::ModifierFlags::NONE;

    auto *prop =
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        ProgramAllocator()->New<ir::ClassProperty>(ident, nullptr, nullptr, modifierFlag, ProgramAllocator(), false);
    ES2PANDA_ASSERT(prop != nullptr);
    prop->SetParent(newClassDefinition);
    ident->SetParent(prop);

    ES2PANDA_ASSERT(accessor->Function() != nullptr);
    prop->SetTypeAnnotation(accessor->Function()->IsGetter()
                                ? accessor->Function()->ReturnTypeAnnotation()
                                : accessor->Function()->Params()[0]->AsETSParameterExpression()->TypeAnnotation());

    if (prop->TypeAnnotation() != nullptr) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return CreateNullishProperty(prop, newClassDefinition);
    }

    ES2PANDA_ASSERT(accessor->TsType()->IsETSFunctionType());
    auto callSign = accessor->TsType()->AsETSFunctionType()->CallSignatures()[0];

    ES2PANDA_ASSERT(accessor->Function() != nullptr);
    auto tsType = accessor->Function()->IsGetter() ? callSign->ReturnType() : callSign->Params()[0]->TsType();

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    prop->SetTypeAnnotation(ProgramAllocator()->New<ir::OpaqueTypeNode>(tsType, ProgramAllocator()));

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return CreateNullishProperty(prop, newClassDefinition);
}

ir::ClassProperty *ETSChecker::CreateNullishProperty(ir::ClassProperty *const prop,
                                                     ir::TSInterfaceDeclaration *const newTSInterfaceDefinition)
{
    auto *const propSavedValue = prop->Value();

    // Set value to nullptr to prevent cloning it (as for arrow functions that is not possible yet), we set it
    // to 'undefined' anyway
    prop->SetValue(nullptr);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *const propClone = prop->Clone(ProgramAllocator(), newTSInterfaceDefinition->Body())->AsClassProperty();

    // Revert original property value
    prop->SetValue(propSavedValue);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    propClone->SetValue(ProgramAllocator()->New<ir::UndefinedLiteral>());

    auto *propTypeAnn = propClone->TypeAnnotation();
    ArenaVector<ir::TypeNode *> types(ProgramAllocator()->Adapter());

    // Handle implicit type annotation
    if (propTypeAnn == nullptr) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        propTypeAnn = ProgramAllocator()->New<ir::OpaqueTypeNode>(prop->TsType(), ProgramAllocator());
    }

    // Create new nullish type
    types.push_back(propTypeAnn);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    types.push_back(ProgramAllocNode<ir::ETSUndefinedType>(ProgramAllocator()));
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *const unionType = ProgramAllocNode<ir::ETSUnionType>(std::move(types), ProgramAllocator());
    propClone->SetTypeAnnotation(unionType);

    // Set new parents
    unionType->SetParent(propClone);
    propClone->SetParent(newTSInterfaceDefinition->Body());

    return propClone;
}

ir::ClassProperty *ETSChecker::CreateNullishProperty(ir::ClassProperty *const prop,
                                                     ir::ClassDefinition *const newClassDefinition)
{
    auto *const propSavedValue = prop->Value();

    // Set value to nullptr to prevent cloning it (as for arrow functions that is not possible yet), we set it
    // to 'undefined' anyway
    prop->SetValue(nullptr);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *const propClone = prop->Clone(ProgramAllocator(), newClassDefinition)->AsClassProperty();
    propClone->CleanCheckInformation();

    // Revert original property value
    prop->SetValue(propSavedValue);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    propClone->SetValue(ProgramAllocator()->New<ir::UndefinedLiteral>());
    propClone->AsClassProperty()->Value()->Check(this);

    ir::TypeNode *propertyTypeAnnotation = propClone->TypeAnnotation();
    if (propertyTypeAnnotation == nullptr) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        propertyTypeAnnotation = ProgramAllocator()->New<ir::OpaqueTypeNode>(prop->Check(this), ProgramAllocator());
    }

    // Create new nullish type annotation
    ArenaVector<ir::TypeNode *> types(ProgramAllocator()->Adapter());
    types.push_back(propertyTypeAnnotation);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    types.push_back(ProgramAllocNode<ir::ETSUndefinedType>(ProgramAllocator()));
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    propertyTypeAnnotation = ProgramAllocNode<ir::ETSUnionType>(std::move(types), ProgramAllocator());
    propClone->SetTypeAnnotation(propertyTypeAnnotation);
    propClone->SetTsType(nullptr);

    // Set new parents
    propertyTypeAnnotation->SetParent(propClone);
    propClone->SetParent(newClassDefinition);

    return propClone;
}

ir::TSTypeParameterDeclaration *ETSChecker::ProcessTypeParamAndGenSubstitution(
    ir::TSTypeParameterDeclaration const *const thisTypeParams,
    ArenaMap<ir::TSTypeParameter *, ir::TSTypeParameter *> *likeSubstitution,
    ir::TSTypeParameterDeclaration *newTypeParams = nullptr)
{
    ArenaVector<ir::TSTypeParameter *> typeParams(ProgramAllocator()->Adapter());
    if (newTypeParams == nullptr) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        newTypeParams = ProgramAllocNode<ir::TSTypeParameterDeclaration>(std::move(typeParams), typeParams.size());
    }
    for (auto *const classOrInterfaceDefTypeParam : thisTypeParams->Params()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *newTypeParam = ProgramAllocNode<ir::TSTypeParameter>(
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            CloneNodeIfNotNullptr(classOrInterfaceDefTypeParam->Name(), ProgramAllocator()),
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            CloneNodeIfNotNullptr(classOrInterfaceDefTypeParam->Constraint(), ProgramAllocator()),
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            CloneNodeIfNotNullptr(classOrInterfaceDefTypeParam->DefaultType(), ProgramAllocator()), ProgramAllocator());
        ES2PANDA_ASSERT(newTypeParam != nullptr);
        newTypeParams->AddParam(newTypeParam);
        newTypeParam->SetParent(newTypeParams);
        ES2PANDA_ASSERT(likeSubstitution != nullptr);
        (*likeSubstitution)[classOrInterfaceDefTypeParam] = newTypeParam;
    }
    return newTypeParams;
}

ir::TSTypeParameterInstantiation *ETSChecker::CreateNewSuperPartialRefTypeParamsDecl(
    ArenaMap<ir::TSTypeParameter *, ir::TSTypeParameter *> *likeSubstitution, const Type *const superPartialType,
    ir::Expression *superRef)
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
        auto it = likeSubstitution->find(type->AsETSTypeParameter()->GetDeclNode());
        if (it != likeSubstitution->end()) {
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

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP, G.FUD.05) solid logic
void ETSChecker::CreatePartialClassDeclaration(ir::ClassDefinition *const newClassDefinition,
                                               ir::ClassDefinition *classDef)
{
    if (classDef->TypeParams() != nullptr) {
        ArenaVector<ir::TSTypeParameter *> typeParams(ProgramAllocator()->Adapter());
        for (auto *const classDefTypeParam : classDef->TypeParams()->Params()) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            auto *const newTypeParam = ProgramAllocNode<ir::TSTypeParameter>(
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                CloneNodeIfNotNullptr(classDefTypeParam->Name(), ProgramAllocator()),
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                CloneNodeIfNotNullptr(classDefTypeParam->Constraint(), ProgramAllocator()),
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                CloneNodeIfNotNullptr(classDefTypeParam->DefaultType(), ProgramAllocator()), ProgramAllocator());
            typeParams.emplace_back(newTypeParam);
        }

        auto *const newTypeParams =
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            ProgramAllocNode<ir::TSTypeParameterDeclaration>(std::move(typeParams),
                                                             classDef->TypeParams()->RequiredParams());
        ES2PANDA_ASSERT(newTypeParams != nullptr);
        newClassDefinition->SetTypeParams(newTypeParams);
        newTypeParams->SetParent(newClassDefinition);
    }

    newClassDefinition->SetVariable(newClassDefinition->Ident()->Variable());
    newClassDefinition->AddModifier(static_cast<const ir::AstNode *>(classDef)->Modifiers());

    for (auto *const prop : classDef->Body()) {
        // Only handle class properties (members)
        // Method calls on partial classes will make the class not type safe, so we don't copy any methods
        if (prop->IsClassProperty()) {
            if (prop->AsClassProperty()->Id() == nullptr ||
                prop->AsClassProperty()->Id()->Name().Mutf8().find(compiler::Signatures::PROPERTY, 0) == 0) {
                continue;
            }

            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            auto *const newProp = CreateNullishProperty(prop->AsClassProperty(), newClassDefinition);

            // Put the new property into the class declaration
            newClassDefinition->EmplaceBody(newProp);
        }
        if (prop->IsMethodDefinition() && prop->AsMethodDefinition()->Function() != nullptr &&
            (prop->AsMethodDefinition()->Function()->IsGetter() ||
             prop->AsMethodDefinition()->Function()->IsSetter())) {
            auto *method = prop->AsMethodDefinition();
            ES2PANDA_ASSERT(method->Id() != nullptr);
            auto isBrokenSetter = method->Function()->IsSetter() && method->Function()->Params().size() != 1;
            auto isBrokenGetter = method->Function()->IsGetter() && !method->Function()->Params().empty();
            if (newClassDefinition->Scope()->FindLocal(method->Id()->Name(),
                                                       varbinder::ResolveBindingOptions::VARIABLES) != nullptr ||
                isBrokenSetter || isBrokenGetter) {
                ES2PANDA_ASSERT(IsAnyError());
                continue;
            }

            if (method->TsType() == nullptr) {
                method->Parent()->Check(this);
            }

            if (method->TsType() != nullptr && !method->TsType()->IsTypeError()) {
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                newClassDefinition->EmplaceBody(CreateNullishPropertyFromAccessor(method, newClassDefinition));
            }
        }
    }
    if (classDef->IsDeclare()) {
        newClassDefinition->AddModifier(ir::ModifierFlags::DECLARE);
    }

    // Run varbinder for new partial class to set scopes
    compiler::InitScopesPhaseETS::RunExternalNode(newClassDefinition, VarBinder());

    newClassDefinition->SetTsType(nullptr);
    newClassDefinition->Variable()->SetTsType(nullptr);
}

static void SetupFunctionParams(ir::ScriptFunction *function, varbinder::FunctionParamScope *paramScope,
                                checker::ETSChecker *checker)
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
            paramExpr->Ident()->SetTsTypeAnnotation(unionType);
            unionType->SetParent(paramExpr->Ident());
        }
        auto [paramVar, node] = paramScope->AddParamDecl(checker->ProgramAllocator(), checker->VarBinder(), paramExpr);
        if (node != nullptr) {
            checker->VarBinder()->ThrowRedeclaration(node->Start(), paramVar->Name(), paramVar->Declaration()->Type());
        }

        paramExpr->SetVariable(paramVar);
    }
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
ir::MethodDefinition *ETSChecker::CreateNullishAccessor(ir::MethodDefinition *const accessor,
                                                        ir::TSInterfaceDeclaration *interface)
{
    const auto interfaceCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(VarBinder(), interface->Scope());
    auto *paramScope = ProgramAllocator()->New<varbinder::FunctionParamScope>(ProgramAllocator(), interface->Scope());
    auto *functionScope = ProgramAllocator()->New<varbinder::FunctionScope>(ProgramAllocator(), paramScope);
    ES2PANDA_ASSERT(functionScope != nullptr);
    functionScope->BindParamScope(paramScope);
    paramScope->BindFunctionScope(functionScope);

    {
        auto paramScopeCtx = varbinder::LexicalScope<varbinder::FunctionParamScope>::Enter(VarBinder(), paramScope);
        VarBinder()->AddMandatoryParam(varbinder::TypedBinder::MANDATORY_PARAM_THIS);
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    ir::MethodDefinition *nullishAccessor = accessor->Clone(ProgramAllocator(), interface->Body());
    nullishAccessor->SetRange(accessor->Range());
    nullishAccessor->Function()->SetRange(accessor->Function()->Range());

    auto *decl = ProgramAllocator()->New<varbinder::FunctionDecl>(ProgramAllocator(), nullishAccessor->Id()->Name(),
                                                                  nullishAccessor);
    auto *var = ProgramAllocator()->New<varbinder::LocalVariable>(decl, varbinder::VariableFlags::VAR);
    var->AddFlag(varbinder::VariableFlags::METHOD);
    nullishAccessor->Id()->SetVariable(var);
    nullishAccessor->SetVariable(var);

    functionScope->BindName(interface->InternalName());

    auto *function = nullishAccessor->Function();
    function->SetVariable(var);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    function->SetIdent(nullishAccessor->Id()->Clone(ProgramAllocator(), function));
    function->SetScope(functionScope);
    paramScope->BindNode(function);
    functionScope->BindNode(function);

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
        SetupFunctionParams(function, paramScope, this);
    }

    nullishAccessor->SetOverloads(ArenaVector<ir::MethodDefinition *>(ProgramAllocator()->Adapter()));

    return nullishAccessor;
}

ir::TSInterfaceDeclaration *ETSChecker::CreateInterfaceProto(util::StringView name,
                                                             parser::Program *const interfaceDeclProgram,
                                                             const bool isStatic, const ir::ModifierFlags flags)
{
    const auto globalCtx =
        varbinder::LexicalScope<varbinder::GlobalScope>::Enter(VarBinder(), interfaceDeclProgram->GlobalScope());

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *const interfaceId = ProgramAllocNode<ir::Identifier>(name, ProgramAllocator());
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
    partialInterface->SetScope(classCtx.GetScope());
    partialInterface->SetVariable(var);
    decl->BindNode(partialInterface);

    // Put class declaration in global scope, and in program AST
    partialInterface->SetParent(interfaceDeclProgram->Ast());
    interfaceDeclProgram->Ast()->AddStatement(partialInterface);
    interfaceDeclProgram->GlobalScope()->InsertBinding(name, var);

    partialInterface->AddModifier(flags);
    partialInterface->ClearModifier(ir::ModifierFlags::EXPORTED);
    partialInterface->ClearModifier(ir::ModifierFlags::DEFAULT_EXPORT);

    return partialInterface;
}

void ETSChecker::CreatePartialTypeInterfaceMethods(ir::TSInterfaceDeclaration *const interfaceDecl,
                                                   ir::TSInterfaceDeclaration *partialInterface)
{
    auto &partialInterfaceMethods = partialInterface->Body()->Body();

    auto const addNullishAccessor = [&partialInterfaceMethods](ir::MethodDefinition *accessor) -> void {
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
            ES2PANDA_ASSERT_POS((*it)->AsMethodDefinition()->Function()->IsSetter(), (*it)->Start());
            auto setter = (*it)->AsMethodDefinition();
            accessor->AddOverload(setter);
            setter->SetParent(accessor);
            setter->Function()->AddFlag(ir::ScriptFunctionFlags::OVERLOAD);
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

        if (func->IsGetter() || func->IsSetter()) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            addNullishAccessor(CreateNullishAccessor(method, partialInterface));
        }

        for (auto *overload : method->Overloads()) {
            ES2PANDA_ASSERT(overload->Function() != nullptr);
            if (overload->Function()->IsGetter() || overload->Function()->IsSetter()) {
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                addNullishAccessor(CreateNullishAccessor(overload, partialInterface));
            }
        }
    }
}

Type *ETSChecker::CreatePartialTypeInterfaceDecl(ir::TSInterfaceDeclaration *const interfaceDecl,
                                                 ETSObjectType *const typeToBePartial,
                                                 ir::TSInterfaceDeclaration *partialInterface)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    CreatePartialTypeInterfaceMethods(interfaceDecl, partialInterface);
    // Create nullish properties of the partial class
    // Build the new Partial class based on the 'T' type parameter of 'Partial<T>'
    auto *likeSubstitution =
        ProgramAllocator()->New<ArenaMap<ir::TSTypeParameter *, ir::TSTypeParameter *>>(ProgramAllocator()->Adapter());

    if (interfaceDecl->TypeParams() != nullptr) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        ProcessTypeParamAndGenSubstitution(interfaceDecl->TypeParams(), likeSubstitution,
                                           partialInterface->TypeParams());
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
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        if (auto *superPartialType = CreatePartialType(t); superPartialType != nullptr) {
            ir::TSTypeParameterInstantiation *superPartialRefTypeParams =
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                CreateNewSuperPartialRefTypeParamsDecl(likeSubstitution, superPartialType, extend->Expr());

            ir::ETSTypeReference *superPartialRef =
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                BuildSuperPartialTypeReference(superPartialType, superPartialRefTypeParams);
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            partialInterface->EmplaceExtends(ProgramAllocNode<ir::TSInterfaceHeritage>(superPartialRef));
            partialInterface->Extends().back()->SetParent(partialInterface);
        }
    }

    auto *const partialType = partialInterface->Check(this)->AsETSObjectType();
    partialType->SetBaseType(typeToBePartial);

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
    classDecl->SetParent(classDeclProgram->Ast());

    // Class definition is scope bearer, not class declaration
    classDef->Scope()->BindNode(classDecl->Definition());
    decl->BindNode(classDef);

    // Put class declaration in global scope, and in program AST
    classDeclProgram->Ast()->AddStatement(classDecl);
    classDeclProgram->GlobalScope()->InsertBinding(name, var);

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
    ArenaVector<checker::Type *> newTypesForUnion(ProgramAllocator()->Adapter());

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

    // Run checker
    auto *const partialType = partialClassDef->Check(this)->AsETSObjectType();

    for (auto *interface : typeToBePartial->Interfaces()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        partialType->AddInterface(CreatePartialType(interface)->AsETSObjectType());
    }

    partialType->SetBaseType(typeToBePartial);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    CreateConstructorForPartialType(partialClassDef, partialType, recordTableToUse);

    // Create partial type for super type
    if (typeToBePartial != GlobalETSObjectType()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *const partialSuper = CreatePartialType((classDef->Super() == nullptr || !classDef->Super()->IsTypeNode())
                                                         ? GlobalETSObjectType()
                                                         : classDef->Super()->TsType());

        if (partialSuper == partialType) {
            LogError(diagnostic::CYCLIC_CLASS_SUPER_TYPE, {}, classDef->Start());
            return partialType;
        }
        if (partialSuper->IsTypeError()) {
            return partialType;
        }
        partialType->SetSuperType(partialSuper->AsETSObjectType());
    }

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
    auto *const signature = CreateSignature(signatureInfo, GlobalVoidType(), func);
    func->SetSignature(signature);

    VarBinder()->AsETSBinder()->BuildInternalNameWithCustomRecordTable(func, recordTable);
    VarBinder()->AsETSBinder()->BuildFunctionName(func);
    VarBinder()->Functions().push_back(functionScope);

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

Type *ETSChecker::GetReadonlyType(Type *type)
{
    if (const auto found = NamedTypeStack().find(type); found != NamedTypeStack().end()) {
        return *found;
    }

    if (type->IsGradualType()) {
        return GetReadonlyType(type->AsGradualType()->GetBaseType());
    }

    NamedTypeStackElement ntse(this, type);
    ES2PANDA_ASSERT(type != nullptr);
    if (type->IsETSArrayType()) {
        ETSArrayType *const clonedArrayType =
            ProgramAllocator()->New<ETSArrayType>(type->AsETSArrayType()->ElementType());
        ES2PANDA_ASSERT(clonedArrayType != nullptr);
        clonedArrayType->AddTypeFlag(TypeFlag::READONLY);
        return clonedArrayType;
    }
    if (type->IsETSTupleType()) {
        Type *const clonedType = type->Clone(this);
        clonedType->AddTypeFlag(TypeFlag::READONLY);
        return clonedType;
    }

    if (type->IsETSObjectType()) {
        type->AsETSObjectType()->InstanceFields();
        auto *clonedType = type->Clone(this)->AsETSObjectType();
        MakePropertiesReadonly(clonedType);
        return clonedType;
    }
    if (type->IsETSTypeParameter()) {
        return ProgramAllocator()->New<ETSReadonlyType>(type->AsETSTypeParameter());
    }
    if (type->IsETSUnionType()) {
        ArenaVector<Type *> unionTypes(ProgramAllocator()->Adapter());
        for (auto *t : type->AsETSUnionType()->ConstituentTypes()) {
            unionTypes.emplace_back(t->IsETSObjectType() ? GetReadonlyType(t) : t->Clone(this));
        }
        return CreateETSUnionType(std::move(unionTypes));
    }
    return type;
}

void ETSChecker::MakePropertiesReadonly(ETSObjectType *const classType)
{
    classType->UpdateTypeProperties([this](auto *property, auto *propType) {
        auto *newDecl =
            ProgramAllocator()->New<varbinder::ReadonlyDecl>(property->Name(), property->Declaration()->Node());
        auto *const propCopy = property->Copy(ProgramAllocator(), newDecl);
        propCopy->AddFlag(varbinder::VariableFlags::READONLY);
        propCopy->SetTsType(propType);
        return propCopy;
    });
}

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Required utility type
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
Type *ETSChecker::HandleRequiredType(Type *typeToBeRequired)
{
    if (const auto found = NamedTypeStack().find(typeToBeRequired); found != NamedTypeStack().end()) {
        return *found;
    }

    NamedTypeStackElement ntse(this, typeToBeRequired);

    if (typeToBeRequired->IsETSTypeParameter()) {
        auto *const requiredClone = typeToBeRequired->Clone(this);
        requiredClone->AddTypeFlag(TypeFlag::ETS_REQUIRED_TYPE_PARAMETER);
        return requiredClone;
    }

    if (typeToBeRequired->IsETSUnionType()) {
        ArenaVector<Type *> unionTypes(ProgramAllocator()->Adapter());
        for (auto *type : typeToBeRequired->AsETSUnionType()->ConstituentTypes()) {
            if (type->IsETSObjectType()) {
                type = type->Clone(this);
                ES2PANDA_ASSERT(type != nullptr);
                MakePropertiesNonNullish(type->AsETSObjectType());
            }

            if (type->IsETSNullType() || type->IsETSUndefinedType()) {
                continue;
            }

            unionTypes.emplace_back(type);
        }

        return CreateETSUnionType(std::move(unionTypes));
    }

    if (typeToBeRequired->IsETSObjectType()) {
        typeToBeRequired->AsETSObjectType()->InstanceFields();  // call to instantiate properties
    }

    typeToBeRequired = typeToBeRequired->Clone(this);

    MakePropertiesNonNullish(typeToBeRequired->MaybeBaseTypeOfGradualType()->AsETSObjectType());

    return typeToBeRequired;
}

void ETSChecker::MakePropertiesNonNullish(ETSObjectType *const classType)
{
    classType->AddObjectFlag(ETSObjectFlags::REQUIRED);
    classType->InstanceFields();

    for (const auto &[_, propVar] : classType->InstanceFields()) {
        MakePropertyNonNullish<PropertyType::INSTANCE_FIELD>(classType, propVar);
    }

    for (const auto &[_, propVar] : classType->StaticFields()) {
        MakePropertyNonNullish<PropertyType::STATIC_FIELD>(classType, propVar);
    }

    if (classType->SuperType() != nullptr) {
        auto *const superRequired = classType->SuperType()->Clone(this)->AsETSObjectType();
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

    for (const auto &[propName, _] : requiredType->InstanceFields()) {
        if (!initObjExprContainsField(propName)) {
            LogError(diagnostic::REQUIRED_PROP_MISSING_INIT, {propName, requiredType->Name()}, initObjExpr->Start());
        }
    }
}

}  // namespace ark::es2panda::checker
