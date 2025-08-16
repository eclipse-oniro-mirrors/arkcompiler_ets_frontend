/**
 * Copyright (c) 2021-2025 - Huawei Device Co., Ltd.
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

#include <string_view>
#include "checker/ETSchecker.h"
#include "checker/checkerContext.h"
#include "checker/ets/typeRelationContext.h"
#include "checker/types/ets/etsObjectType.h"
#include "checker/types/ets/etsTupleType.h"
#include "checker/types/ets/etsPartialTypeParameter.h"
#include "checker/types/ets/etsAwaitedType.h"
#include "checker/types/gradualType.h"
#include "compiler/lowering/phase.h"
#include "ir/base/classDefinition.h"
#include "ir/base/classElement.h"
#include "ir/base/classProperty.h"
#include "ir/base/classStaticBlock.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/ets/etsNewClassInstanceExpression.h"
#include "ir/ets/etsTypeReference.h"
#include "ir/ets/etsTypeReferencePart.h"
#include "ir/ets/etsUnionType.h"
#include "ir/expression.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/memberExpression.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/expressionStatement.h"
#include "ir/statements/variableDeclarator.h"
#include "ir/ts/tsClassImplements.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsInterfaceHeritage.h"
#include "ir/ts/tsTypeParameter.h"
#include "ir/ts/tsTypeParameterDeclaration.h"
#include "util/diagnostic.h"
#include "varbinder/declaration.h"
#include "varbinder/variableFlags.h"
#include "generated/signatures.h"
#include "generated/diagnostic.h"
#include "util/helpers.h"

namespace ark::es2panda::checker {

static constexpr std::string_view SET_PROTOTYPE_OF = "setPrototypeOf";

static bool CheckGetterSetterDecl(varbinder::LocalVariable const *child, varbinder::LocalVariable const *parent)
{
    auto readonlyCheck = [](varbinder::LocalVariable const *var, bool isParent, bool isReadonly) {
        if (!var->TsType()->IsETSMethodType()) {
            return true;
        }

        auto *functionType = var->TsType()->AsETSFunctionType();
        auto getter = functionType->FindGetter();
        if (getter == nullptr) {
            return false;
        }

        auto setter = functionType->FindSetter();
        if (!isParent && setter == nullptr && !isReadonly) {
            return false;
        }

        if (isParent && setter != nullptr && isReadonly) {
            return false;
        }

        return true;
    };

    bool checkChild = readonlyCheck(child, false, parent->Declaration()->Type() == varbinder::DeclType::READONLY);
    bool checkParent = readonlyCheck(parent, true, child->Declaration()->Type() == varbinder::DeclType::READONLY);
    return checkChild && checkParent && (child->TsType()->IsETSFunctionType() || parent->TsType()->IsETSFunctionType());
}

static bool CheckOverloadDecl(varbinder::LocalVariable *child, varbinder::LocalVariable *parent)
{
    if (!child->Declaration()->Node()->IsOverloadDeclaration() &&
        !parent->Declaration()->Node()->IsOverloadDeclaration()) {
        return false;
    }

    if (!child->Declaration()->Node()->IsOverloadDeclaration() ||
        !parent->Declaration()->Node()->IsOverloadDeclaration()) {
        return true;
    }

    auto *childOverload = child->Declaration()->Node()->AsOverloadDeclaration();
    auto *parentOverload = parent->Declaration()->Node()->AsOverloadDeclaration();
    for (auto *baseMethodName : parentOverload->OverloadedList()) {
        ES2PANDA_ASSERT(baseMethodName->IsIdentifier());
        auto res = std::find_if(childOverload->OverloadedList().begin(), childOverload->OverloadedList().end(),
                                [baseMethodName](ir::Expression *subMethodName) {
                                    return subMethodName->IsIdentifier() && subMethodName->AsIdentifier()->Name() ==
                                                                                baseMethodName->AsIdentifier()->Name();
                                });
        if (res == childOverload->OverloadedList().end()) {
            return false;
        }
    }
    return true;
}

static bool CheckFunctionDecl(varbinder::LocalVariable *child, varbinder::LocalVariable *parent)
{
    if (child->Declaration()->Node()->IsOverloadDeclaration() ||
        parent->Declaration()->Node()->IsOverloadDeclaration()) {
        return false;
    }
    ES2PANDA_ASSERT(child->Declaration()->Type() == parent->Declaration()->Type());
    if (!child->TsType()->IsETSMethodType() || !parent->TsType()->IsETSMethodType()) {
        return true;
    }

    const auto *childType = child->TsType()->AsETSFunctionType();
    const auto *parentType = parent->TsType()->AsETSFunctionType();
    bool childIsField = (childType->FindGetter() != nullptr) || (childType->FindSetter() != nullptr);
    bool parentIsField = (parentType->FindGetter() != nullptr) || (parentType->FindSetter() != nullptr);
    return childIsField == parentIsField;
}

ETSObjectType *ETSChecker::GetSuperType(ETSObjectType *type)
{
    ComputeSuperType(type);
    if (type == GlobalETSObjectType()) {
        return GlobalETSObjectType();
    }
    if (type->SuperType() == nullptr) {
        return nullptr;
    }
    return type->SuperType();
}

static bool CheckObjectTypeAndSuperType(ETSChecker *checker, ETSObjectType *type)
{
    if (type->HasObjectFlag(ETSObjectFlags::RESOLVED_SUPER)) {
        return true;
    }

    ES2PANDA_ASSERT(type->Variable() && type->GetDeclNode()->IsClassDefinition());
    auto *classDef = type->GetDeclNode()->AsClassDefinition();
    auto cName = classDef->Ident()->Name();
    if (cName == compiler::Signatures::PARTIAL_TYPE_NAME || cName == compiler::Signatures::READONLY_TYPE_NAME ||
        cName == compiler::Signatures::REQUIRED_TYPE_NAME || cName == compiler::Signatures::FIXED_ARRAY_TYPE_NAME ||
        cName == compiler::Signatures::AWAITED_TYPE_NAME) {
        checker->LogError(diagnostic::USING_RESERVED_NAME_AS_VARIABLE_OR_TYPE_NAME, {cName},
                          type->GetDeclNode()->Start());
        type->SetSuperType(checker->GlobalETSObjectType());
        return true;
    }

    if (classDef->Super() == nullptr || !classDef->Super()->IsTypeNode()) {
        type->AddObjectFlag(ETSObjectFlags::RESOLVED_SUPER);
        if (type != checker->GlobalETSObjectType()) {
            type->SetSuperType(checker->GlobalETSObjectType());
        }
        return true;
    }

    return false;
}

bool ETSChecker::ComputeSuperType(ETSObjectType *type)
{
    if (CheckObjectTypeAndSuperType(this, type)) {
        return true;
    }
    auto *classDef = type->GetDeclNode()->AsClassDefinition();

    TypeStackElement tse(this, type, {{diagnostic::CYCLIC_INHERITANCE, {type->Name()}}}, classDef->Ident()->Start());
    if (tse.HasTypeError()) {
        type->AddObjectFlag(ETSObjectFlags::RESOLVED_SUPER);
        return false;
    }

    auto superName = classDef->Super()->AsETSTypeReference()->Part()->GetIdent()->Name();
    if (superName == compiler::Signatures::PARTIAL_TYPE_NAME || superName == compiler::Signatures::READONLY_TYPE_NAME ||
        superName == compiler::Signatures::REQUIRED_TYPE_NAME) {
        LogError(diagnostic::EXTENDING_UTILITY_TYPE, {classDef->Ident()->Name()}, classDef->Super()->Start());
        return false;
    }

    auto *superType = classDef->Super()->AsTypeNode()->GetType(this)->MaybeBaseTypeOfGradualType();
    if (superType == nullptr) {
        return true;
    }
    if (!superType->IsETSObjectType() || !superType->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::CLASS)) {
        LogError(diagnostic::EXTENDING_UTILITY_TYPE, {classDef->Ident()->Name()}, classDef->Super()->Start());
        type->SetSuperType(GlobalETSObjectType());
        return true;
    }

    ETSObjectType *superObj = superType->MaybeBaseTypeOfGradualType()->AsETSObjectType();

    // struct node has class definition, too
    if (superObj->GetDeclNode()->Parent()->IsETSStructDeclaration()) {
        LogError(diagnostic::EXTENDING_STRUCT, {classDef->Ident()->Name()}, classDef->Super()->Start());
    }

    if (superObj->GetDeclNode()->IsFinal()) {
        LogError(diagnostic::EXTENDING_FINAL, {}, classDef->Super()->Start());
        /* It still makes sense to treat superObj as the supertype in future checking */
    }
    if (GetSuperType(superObj) == nullptr) {
        superObj = GlobalETSObjectType();
    }
    type->SetSuperType(superObj);
    type->AddObjectFlag(ETSObjectFlags::RESOLVED_SUPER);
    return true;
}

void ETSChecker::ValidateImplementedInterface(ETSObjectType *type, Type *interface,
                                              std::unordered_set<Type *> *extendsSet, const lexer::SourcePosition &pos)
{
    ES2PANDA_ASSERT(interface != nullptr);
    interface = interface->MaybeBaseTypeOfGradualType();
    if (interface->IsETSObjectType() && interface->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::CLASS)) {
        LogError(diagnostic::INTERFACE_EXTENDS_CLASS, {}, pos);
        return;
    }
    if (!interface->IsETSObjectType() || !interface->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::INTERFACE)) {
        LogError(diagnostic::NOT_INTERFACE, {}, pos);
        return;
    }

    if (!extendsSet->insert(interface).second) {
        LogError(diagnostic::REPEATED_INTERFACE, {}, pos);
    }

    auto *baseType = GetOriginalBaseType(interface);
    if (baseType != interface && !extendsSet->insert(baseType).second) {
        LogError(diagnostic::CONFLICTING_GENERIC_INTERFACE_IMPLS, {baseType}, pos);
    }

    GetInterfaces(interface->AsETSObjectType());
    auto *declNode = interface->AsETSObjectType()->GetDeclNode()->AsTSInterfaceDeclaration();
    if (declNode->TsType() != nullptr && declNode->TsType()->IsTypeError()) {
        return;
    }
    type->AddInterface(interface->AsETSObjectType());
}

void ETSChecker::GetInterfacesOfClass(ETSObjectType *type)
{
    if (type->HasObjectFlag(ETSObjectFlags::RESOLVED_INTERFACES)) {
        return;
    }

    const auto *declNode = type->GetDeclNode()->AsClassDefinition();

    std::unordered_set<Type *> extendsSet;
    for (auto *it : declNode->Implements()) {
        ValidateImplementedInterface(type, it->Expr()->AsTypeNode()->GetType(this), &extendsSet, it->Start());
    }
    type->AddObjectFlag(ETSObjectFlags::RESOLVED_INTERFACES);
}

void ETSChecker::GetInterfacesOfInterface(ETSObjectType *type)
{
    if (type->HasObjectFlag(ETSObjectFlags::RESOLVED_INTERFACES)) {
        return;
    }

    auto *declNode = type->GetDeclNode()->AsTSInterfaceDeclaration();

    TypeStackElement tse(this, type, {{diagnostic::CYCLIC_INHERITANCE, {type->Name()}}}, declNode->Id()->Start());
    if (tse.HasTypeError()) {
        type->AddObjectFlag(ETSObjectFlags::RESOLVED_INTERFACES);
        declNode->SetTsType(GlobalTypeError());
        return;
    }

    std::unordered_set<Type *> extendsSet;
    for (auto *it : declNode->Extends()) {
        ValidateImplementedInterface(type, it->Expr()->AsTypeNode()->GetType(this), &extendsSet, it->Start());
    }
    type->AddObjectFlag(ETSObjectFlags::RESOLVED_INTERFACES);
}

ArenaVector<ETSObjectType *> ETSChecker::GetInterfaces(ETSObjectType *type)
{
    ES2PANDA_ASSERT(type->GetDeclNode()->IsClassDefinition() || type->GetDeclNode()->IsTSInterfaceDeclaration());

    if (type->GetDeclNode()->IsClassDefinition()) {
        GetInterfacesOfClass(type);
    } else {
        GetInterfacesOfInterface(type);
    }

    return type->Interfaces();
}

std::pair<ArenaVector<Type *>, bool> ETSChecker::CreateUnconstrainedTypeParameters(
    ir::TSTypeParameterDeclaration const *typeParams)
{
    bool ok = true;
    ArenaVector<Type *> result {ProgramAllocator()->Adapter()};
    checker::ScopeContext scopeCtx(this, typeParams->Scope());

    // Note: we have to run pure check loop first to avoid endless loop because of possible circular dependencies
    Type2TypeMap extends {};
    TypeSet typeParameterDecls {};
    for (auto *const typeParam : typeParams->Params()) {
        ok &= CheckDefaultTypeParameter(typeParam, typeParameterDecls);
        if (auto *const constraint = typeParam->Constraint();
            constraint != nullptr && constraint->IsETSTypeReference() &&
            constraint->AsETSTypeReference()->Part()->Name()->IsIdentifier()) {
            ok &= CheckTypeParameterConstraint(typeParam, extends);
        }
    }

    for (auto *const typeParam : typeParams->Params()) {
        result.emplace_back(SetUpParameterType(typeParam));
    }

    return {result, ok};
}

void ETSChecker::AssignTypeParameterConstraints(ir::TSTypeParameterDeclaration const *typeParams)
{
    ConstraintCheckScope ctScope(this);
    // The type parameter might be used in the constraint, like 'K extend Comparable<K>',
    // so we need to create their type first, then set up the constraint
    for (auto *const param : typeParams->Params()) {
        SetUpTypeParameterConstraint(param);
    }
    ctScope.TryCheckConstraints();
}

bool ETSChecker::CheckDefaultTypeParameter(const ir::TSTypeParameter *param, TypeSet &typeParameterDecls)
{
    bool ok = true;
    const auto typeParamVar = param->Name()->Variable();
    if (typeParameterDecls.count(typeParamVar) != 0U) {
        LogError(diagnostic::DUPLICATE_TYPE_PARAM, {param->Name()->Name().Utf8()}, param->Start());
        return false;
    }

    std::function<void(ir::AstNode *)> checkDefault = [&typeParameterDecls, this, &checkDefault,
                                                       &ok](ir::AstNode *node) -> void {
        if (node->IsETSTypeReferencePart()) {
            ir::ETSTypeReferencePart *defaultTypePart = node->AsETSTypeReferencePart();
            if (defaultTypePart->Name()->IsTSQualifiedName()) {
                defaultTypePart->Name()->Check(this);
            }
            if (IsFixedArray(defaultTypePart)) {
                defaultTypePart->Check(this);
            }
            auto *const variable = defaultTypePart->GetIdent()->Variable();
            auto defaultType = variable == nullptr ? defaultTypePart->TsType() : variable->TsType();
            if (defaultType == nullptr && variable != nullptr &&
                (variable->Flags() & varbinder::VariableFlags::TYPE_PARAMETER) != 0U &&
                typeParameterDecls.count(variable) == 0U) {
                LogError(diagnostic::TYPE_PARAM_USE_BEFORE_DEFINE,
                         {defaultTypePart->Name()->AsIdentifier()->Name().Utf8()}, node->Start());
                variable->SetTsType(GlobalTypeError());
                ok = false;
            } else if (defaultType != nullptr && defaultType->IsTypeError()) {
                ok = false;
            }
        }
        node->Iterate(checkDefault);
    };

    if (param->DefaultType() != nullptr) {
        TypeStackElement tse(this, param->DefaultType(), {{diagnostic::TYPE_PARAM_CIRCULAR_DEFAULT_TYPE}},
                             param->DefaultType()->Start());
        if (tse.HasTypeError()) {
            return false;
        }

        param->DefaultType()->Iterate(checkDefault);
    }

    typeParameterDecls.emplace(typeParamVar);
    return ok;
}

bool ETSChecker::CheckTypeParameterConstraint(ir::TSTypeParameter *param, Type2TypeMap &extends)
{
    const auto typeParamVar = param->Name()->Variable();
    const auto constraintVar = param->Constraint()->AsETSTypeReference()->Part()->GetIdent()->Variable();
    extends.emplace(typeParamVar, constraintVar);
    auto it = extends.find(constraintVar);
    while (it != extends.cend()) {
        if (it->second == typeParamVar) {
            LogError(diagnostic::TYPE_PARAM_CIRCULAR_CONSTRAINT, {param->Name()->Name().Utf8()},
                     param->Constraint()->Start());
            return false;
        }
        it = extends.find(it->second);
    }

    return true;
}

void ETSChecker::SetUpTypeParameterConstraint(ir::TSTypeParameter *const param)
{
    ETSTypeParameter *const paramType = param->Name()->Variable()->TsType()->AsETSTypeParameter();
    auto const traverseReferenced =
        [this, scope = param->Parent()->AsTSTypeParameterDeclaration()->Scope()](ir::TypeNode *typeNode) {
            if (!typeNode->IsETSTypeReference()) {
                return;
            }
            const auto typeName = typeNode->AsETSTypeReference()->Part()->GetIdent()->Name();
            auto *const found = scope->FindLocal(typeName, varbinder::ResolveBindingOptions::BINDINGS);
            if (found != nullptr) {
                SetUpTypeParameterConstraint(found->Declaration()->Node()->AsTSTypeParameter());
            }
        };

    if (param->Constraint() != nullptr) {
        traverseReferenced(param->Constraint());
        paramType->SetConstraintType(param->Constraint()->GetType(this));
    } else {
        paramType->SetConstraintType(GlobalETSAnyType());
    }

    if (param->DefaultType() != nullptr) {
        traverseReferenced(param->DefaultType());
        // NOTE: #14993 ensure default matches constraint
        paramType->SetDefaultType(MaybeBoxType(param->DefaultType()->GetType(this)));
    }
}

ETSTypeParameter *ETSChecker::SetUpParameterType(ir::TSTypeParameter *const param)
{
    auto *const var = param->Name()->Variable();
    ES2PANDA_ASSERT(var != nullptr);

    if (var->TsType() != nullptr) {
        return var->TsType()->AsETSTypeParameter();
    }

    auto *const paramType = CreateTypeParameter();
    ES2PANDA_ASSERT(paramType != nullptr);
    paramType->AddTypeFlag(TypeFlag::GENERIC);
    paramType->SetDeclNode(param);
    paramType->SetVariable(param->Variable());
    // NOTE: #15026 recursive type parameter workaround
    paramType->SetConstraintType(GlobalETSAnyType());
    var->SetTsType(paramType);
    return paramType;
}

void ETSChecker::CreateTypeForClassOrInterfaceTypeParameters(ETSObjectType *type)
{
    if (type->HasObjectFlag(ETSObjectFlags::RESOLVED_TYPE_PARAMS)) {
        return;
    }

    ir::TSTypeParameterDeclaration *typeParams = type->GetDeclNode()->IsClassDefinition()
                                                     ? type->GetDeclNode()->AsClassDefinition()->TypeParams()
                                                     : type->GetDeclNode()->AsTSInterfaceDeclaration()->TypeParams();
    auto [typeParamTypes, ok] = CreateUnconstrainedTypeParameters(typeParams);
    type->SetTypeArguments(std::move(typeParamTypes));
    if (ok) {
        AssignTypeParameterConstraints(typeParams);
    }
    type->AddObjectFlag(ETSObjectFlags::RESOLVED_TYPE_PARAMS);
    type->AddObjectFlag(ETSObjectFlags::INCOMPLETE_INSTANTIATION);
}

Type *ETSChecker::MaybeGradualType(ir::AstNode *node, ETSObjectType *type)
{
    ES2PANDA_ASSERT(node->IsClassDefinition() || node->IsTSInterfaceDeclaration());
    auto isDynamic = node->IsClassDefinition() ? node->AsClassDefinition()->Language().IsDynamic()
                                               : node->AsTSInterfaceDeclaration()->Language().IsDynamic();
    // Temporary solution, the struct loses 'language' while being converted to a class through the plugin API.
    return isDynamic || Program()->IsDeclForDynamicStaticInterop() ? CreateGradualType(type) : type;
}

Type *ETSChecker::BuildBasicInterfaceProperties(ir::TSInterfaceDeclaration *interfaceDecl)
{
    auto *var = interfaceDecl->Id()->Variable();
    if (var == nullptr) {
        ES2PANDA_ASSERT(IsAnyError());
        return GlobalTypeError();
    }

    checker::ETSObjectType *interfaceType {};
    checker::Type *type {};
    if (var->TsType() == nullptr) {
        interfaceType = CreateETSObjectTypeOrBuiltin(interfaceDecl, checker::ETSObjectFlags::INTERFACE);
        interfaceType->SetVariable(var);
        type = MaybeGradualType(interfaceDecl, interfaceType);
        var->SetTsType(type);
    } else if (var->TsType()->MaybeBaseTypeOfGradualType()->IsETSObjectType()) {
        interfaceType = var->TsType()->MaybeBaseTypeOfGradualType()->AsETSObjectType();
        type = MaybeGradualType(interfaceDecl, interfaceType);
    } else {
        ES2PANDA_ASSERT(IsAnyError());
        return GlobalTypeError();
    }

    // Save before we mess with savedContext.
    bool builtinsInitialized = HasStatus(CheckerStatus::BUILTINS_INITIALIZED);

    auto *enclosingClass = Context().ContainingClass();
    interfaceType->SetEnclosingType(enclosingClass);
    CheckerStatus newStatus = CheckerStatus::IN_INTERFACE;
    auto savedContext = checker::SavedCheckerContext(this, newStatus, interfaceType);
    ConstraintCheckScope ctScope(this);
    if (interfaceDecl->TypeParams() != nullptr) {
        interfaceType->AddTypeFlag(TypeFlag::GENERIC);
        CreateTypeForClassOrInterfaceTypeParameters(interfaceType);
    }

    GetInterfaces(interfaceType);
    interfaceType->SetSuperType(GlobalETSObjectType());
    ctScope.TryCheckConstraints();

    // Skip this check if the builtins are not initialized.
    // They will be initialized in different order,
    // and it is possible that the FunctionType interface is not yet created.
    if (builtinsInitialized) {
        CheckInterfaceFunctions(interfaceType);
    }

    return type;
}

void ETSChecker::CheckDynamicInheritanceAndImplement(ETSObjectType *const interfaceOrClassType)
{
    auto getTypeString = [](ETSObjectType *type) { return type->IsInterface() ? "interface" : "class"; };
    auto extendsOrImplements = [](ETSObjectType *type) { return type->IsInterface() ? "extends" : "implements"; };
    auto isFromDynamicDecl = [](ETSObjectType *type) {
        auto declNode = type->GetDeclNode();
        if (declNode->IsTSInterfaceDeclaration()) {
            return declNode->AsTSInterfaceDeclaration()->Language().IsDynamic();
        }
        if (declNode->IsClassDefinition()) {
            return declNode->AsClassDefinition()->Language().IsDynamic();
        }
        return false;
    };
    if (isFromDynamicDecl(interfaceOrClassType)) {
        return;
    }
    for (ETSObjectType *interType : interfaceOrClassType->Interfaces()) {
        if (isFromDynamicDecl(interType)) {
            LogError(diagnostic::INTERFACE_OR_CLASS_CANNOT_IMPL_OR_EXTEND_DYNAMIC,
                     {getTypeString(interfaceOrClassType), interfaceOrClassType->Name(),
                      extendsOrImplements(interfaceOrClassType), getTypeString(interType), interType->Name()},
                     interfaceOrClassType->GetDeclNode()->Start());
        }
    }
    if (interfaceOrClassType->SuperType() != nullptr && isFromDynamicDecl(interfaceOrClassType->SuperType())) {
        LogError(diagnostic::INTERFACE_OR_CLASS_CANNOT_IMPL_OR_EXTEND_DYNAMIC,
                 {getTypeString(interfaceOrClassType), interfaceOrClassType->Name(), "extends",
                  getTypeString(interfaceOrClassType->SuperType()), interfaceOrClassType->SuperType()->Name()},
                 interfaceOrClassType->GetDeclNode()->Start());
    }
}

Type *ETSChecker::BuildBasicClassProperties(ir::ClassDefinition *classDef)
{
    if (classDef->IsFinal() && classDef->IsAbstract()) {
        LogError(diagnostic::ABSTRACT_IS_FINAL, {}, classDef->Start());
    }

    auto *var = classDef->Ident()->Variable();
    if (var == nullptr) {
        ES2PANDA_ASSERT(IsAnyError());
        return GlobalTypeError();
    }

    checker::ETSObjectType *classType {};
    checker::Type *type {};
    if (var->TsType() == nullptr) {
        classType = CreateETSObjectTypeOrBuiltin(classDef, checker::ETSObjectFlags::CLASS);
        type = MaybeGradualType(classDef, classType);
        classType->SetVariable(var);
        var->SetTsType(type);
        if (classDef->IsAbstract()) {
            classType->AddObjectFlag(checker::ETSObjectFlags::ABSTRACT);
        }
    } else if (var->TsType()->MaybeBaseTypeOfGradualType()->IsETSObjectType()) {
        classType = var->TsType()->MaybeBaseTypeOfGradualType()->AsETSObjectType();
        type = MaybeGradualType(classDef, classType);
    } else {
        ES2PANDA_ASSERT(IsAnyError());
        return GlobalTypeError();
    }

    classDef->SetTsType(type);

    ConstraintCheckScope ctScope(this);
    if (classDef->TypeParams() != nullptr) {
        classType->AddTypeFlag(TypeFlag::GENERIC);
        CreateTypeForClassOrInterfaceTypeParameters(classType);
    }

    auto *enclosingClass = Context().ContainingClass();
    classType->SetEnclosingType(enclosingClass);
    CheckerStatus newStatus = CheckerStatus::IN_CLASS;

    if (classDef->IsInner()) {
        newStatus |= CheckerStatus::INNER_CLASS;
        classType->AddObjectFlag(checker::ETSObjectFlags::INNER);
    }

    auto savedContext = checker::SavedCheckerContext(this, newStatus, classType);

    if (!classType->HasObjectFlag(ETSObjectFlags::RESOLVED_SUPER)) {
        GetSuperType(classType);
        GetInterfaces(classType);
    }
    ctScope.TryCheckConstraints();
    return type;
}

ETSObjectType *ETSChecker::BuildAnonymousClassProperties(ir::ClassDefinition *classDef, ETSObjectType *superType)
{
    auto classType = CreateETSObjectType(classDef, checker::ETSObjectFlags::CLASS);
    classDef->SetTsType(classType);
    ES2PANDA_ASSERT(classType != nullptr);
    classType->SetSuperType(superType);
    classType->AddObjectFlag(checker::ETSObjectFlags::RESOLVED_SUPER);

    return classType;
}

static void ResolveDeclaredFieldsOfObject(ETSChecker *checker, const ETSObjectType *type, varbinder::ClassScope *scope)
{
    for (auto &[_, it] : scope->InstanceFieldScope()->Bindings()) {
        (void)_;
        ES2PANDA_ASSERT(it->Declaration()->Node()->IsClassProperty());
        auto *classProp = it->Declaration()->Node()->AsClassProperty();
        it->AddFlag(checker->GetAccessFlagFromNode(classProp));
        type->AddProperty<PropertyType::INSTANCE_FIELD>(it->AsLocalVariable());
    }

    for (auto &[_, it] : scope->StaticFieldScope()->Bindings()) {
        (void)_;
        ES2PANDA_ASSERT(it->Declaration()->Node()->IsClassProperty());
        auto *classProp = it->Declaration()->Node()->AsClassProperty();
        it->AddFlag(checker->GetAccessFlagFromNode(classProp));
        type->AddProperty<PropertyType::STATIC_FIELD>(it->AsLocalVariable());
    }
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
static void ResolveDeclaredMethodsOfObject(ETSChecker *checker, const ETSObjectType *type, varbinder::ClassScope *scope)
{
    for (auto &[_, it] : scope->InstanceMethodScope()->Bindings()) {
        (void)_;
        if (it->Declaration()->Node()->IsMethodDefinition()) {
            auto *method = it->Declaration()->Node()->AsMethodDefinition();
            auto *function = method->Function();
            ES2PANDA_ASSERT(function != nullptr);
            if (function->IsProxy()) {
                continue;
            }

            it->AddFlag(checker->GetAccessFlagFromNode(method));
            auto *funcType = checker->BuildMethodSignature(method);
            if (!funcType->IsTypeError()) {
                funcType->SetVariable(it);
            }
            it->SetTsType(funcType);
            method->SetTsType(funcType);
            type->AddProperty<PropertyType::INSTANCE_METHOD>(it->AsLocalVariable());
        } else if (it->Declaration()->Node()->IsOverloadDeclaration()) {
            type->AddProperty<PropertyType::INSTANCE_METHOD>(it->AsLocalVariable());
        } else {
            ES2PANDA_UNREACHABLE();
        }
    }

    for (auto &[_, it] : scope->StaticMethodScope()->Bindings()) {
        (void)_;
        if (it->Declaration()->Node()->IsMethodDefinition()) {
            auto *method = it->Declaration()->Node()->AsMethodDefinition();
            auto *function = method->Function();
            ES2PANDA_ASSERT(function != nullptr);
            if (function->IsProxy()) {
                continue;
            }

            it->AddFlag(checker->GetAccessFlagFromNode(method));
            auto *funcType = checker->BuildMethodSignature(method);
            if (!funcType->IsTypeError()) {
                funcType->SetVariable(it);
            }
            it->SetTsType(funcType);
            method->SetTsType(funcType);

            if (method->IsConstructor() && funcType->IsETSFunctionType()) {
                type->AddConstructSignature(funcType->AsETSFunctionType()->CallSignatures());
                continue;
            }

            type->AddProperty<PropertyType::STATIC_METHOD>(it->AsLocalVariable());
        } else if (it->Declaration()->Node()->IsOverloadDeclaration()) {
            type->AddProperty<PropertyType::STATIC_METHOD>(it->AsLocalVariable());
        } else {
            continue;
        }
    }
}

static void ResolveDeclaredDeclsOfObject(ETSChecker *checker, const ETSObjectType *type, varbinder::ClassScope *scope)
{
    for (auto &[_, it] : scope->InstanceDeclScope()->Bindings()) {
        (void)_;
        it->AddFlag(checker->GetAccessFlagFromNode(it->Declaration()->Node()));
        type->AddProperty<PropertyType::INSTANCE_DECL>(it->AsLocalVariable());
    }

    for (auto &[_, it] : scope->StaticDeclScope()->Bindings()) {
        (void)_;
        it->AddFlag(checker->GetAccessFlagFromNode(it->Declaration()->Node()));
        type->AddProperty<PropertyType::STATIC_DECL>(it->AsLocalVariable());
    }
    for (auto &[_, it] : scope->TypeAliasScope()->Bindings()) {
        (void)_;
        it->AddFlag(checker->GetAccessFlagFromNode(it->Declaration()->Node()));
        type->AddProperty<PropertyType::STATIC_DECL>(it->AsLocalVariable());
    }
}

void ETSChecker::ResolveDeclaredMembersOfObject(const Type *type)
{
    if (type->IsGradualType()) {
        return ResolveDeclaredMembersOfObject(type->AsGradualType()->GetBaseType());
    }

    if (!type->IsETSObjectType() || type->AsETSObjectType()->IsPropertiesInstantiated()) {
        return;
    }

    auto *objectType = type->AsETSObjectType();
    auto *declNode = objectType->GetDeclNode();

    // Note: the DeclNode of ETSStringLiteralType is totally the same as its super type: `ETSObjectType-String`.
    if (objectType->IsETSStringLiteralType() || declNode == nullptr ||
        !(declNode->IsClassDefinition() || declNode->IsTSInterfaceDeclaration())) {
        return;
    }

    if (objectType->IsGeneric() && objectType != objectType->GetOriginalBaseType()) {
        const auto *baseType = objectType->GetOriginalBaseType();
        auto *baseDeclNode = baseType->GetDeclNode();
        checker::CheckerStatus baseStatus = baseDeclNode->IsTSInterfaceDeclaration()
                                                ? checker::CheckerStatus::IN_INTERFACE
                                                : checker::CheckerStatus::IN_CLASS;
        auto baseScope = baseDeclNode->IsTSInterfaceDeclaration() ? baseDeclNode->AsTSInterfaceDeclaration()->Scope()
                                                                  : baseDeclNode->AsClassDefinition()->Scope();
        auto savedContext = checker::SavedCheckerContext(this, baseStatus, baseType);
        checker::ScopeContext scopeCtx(this, baseScope);
        ResolveDeclaredMembersOfObject(baseType);
        return;
    }

    checker::CheckerStatus status =
        declNode->IsTSInterfaceDeclaration() ? checker::CheckerStatus::IN_INTERFACE : checker::CheckerStatus::IN_CLASS;
    auto *scope = declNode->IsTSInterfaceDeclaration() ? declNode->AsTSInterfaceDeclaration()->Scope()
                                                       : declNode->AsClassDefinition()->Scope();
    auto savedContext = checker::SavedCheckerContext(this, status, objectType);
    checker::ScopeContext scopeCtx(this, scope);

    ResolveDeclaredDeclsOfObject(this, objectType, scope->AsClassScope());
    ResolveDeclaredFieldsOfObject(this, objectType, scope->AsClassScope());
    ResolveDeclaredMethodsOfObject(this, objectType, scope->AsClassScope());
}

bool ETSChecker::HasETSFunctionType(ir::TypeNode *typeAnnotation)
{
    if (typeAnnotation->IsETSFunctionType()) {
        return true;
    }
    std::unordered_set<ir::TypeNode *> childrenSet;

    if (!typeAnnotation->IsETSTypeReference()) {
        return false;
    }

    auto const addTypeAlias = [&childrenSet, &typeAnnotation](varbinder::Decl *typeDecl) {
        typeAnnotation = typeDecl->Node()->AsTSTypeAliasDeclaration()->TypeAnnotation();
        if (!typeAnnotation->IsETSUnionType()) {
            childrenSet.insert(typeAnnotation);
            return;
        }
        for (auto *type : typeAnnotation->AsETSUnionType()->Types()) {
            if (type->IsETSTypeReference()) {
                childrenSet.insert(type);
            }
        }
    };

    auto *typeDecl = typeAnnotation->AsETSTypeReference()->Part()->GetIdent()->Variable()->Declaration();
    if (typeDecl != nullptr && typeDecl->IsTypeAliasDecl()) {
        addTypeAlias(typeDecl);
    }

    for (auto *child : childrenSet) {
        if (HasETSFunctionType(child)) {
            return true;
        }
    }
    return false;
}

std::vector<Signature *> ETSChecker::CollectAbstractSignaturesFromObject(const ETSObjectType *objType)
{
    std::vector<Signature *> abstracts;
    for (const auto &prop : objType->Methods()) {
        if (prop->Declaration()->Node()->IsOverloadDeclaration()) {
            continue;
        }

        GetTypeOfVariable(prop);

        if (!prop->TsType()->IsETSFunctionType()) {
            continue;
        }

        for (auto *sig : prop->TsType()->AsETSFunctionType()->CallSignatures()) {
            if (sig->HasSignatureFlag(SignatureFlags::ABSTRACT) && !sig->HasSignatureFlag(SignatureFlags::PRIVATE)) {
                abstracts.push_back(sig);
            }
        }
    }

    return abstracts;
}

void ETSChecker::CreateFunctionTypesFromAbstracts(const std::vector<Signature *> &abstracts,
                                                  ArenaVector<ETSFunctionType *> *target)
{
    for (auto *it : abstracts) {
        auto name = it->Function()->Id()->Name();
        auto *found = FindFunctionInVectorGivenByName(name, *target);
        if (found != nullptr) {
            found->AddCallSignature(it);
        } else {
            target->push_back(CreateETSMethodType(name, {{it}, ProgramAllocator()->Adapter()}));
        }
    }
}

void ETSChecker::ComputeAbstractsFromInterface(ETSObjectType *interfaceType)
{
    auto cachedComputedAbstracts = GetCachedComputedAbstracts();
    ES2PANDA_ASSERT(cachedComputedAbstracts != nullptr);
    auto cached = cachedComputedAbstracts->find(interfaceType);
    if (cached != cachedComputedAbstracts->end()) {
        return;
    }

    for (auto *it : interfaceType->Interfaces()) {
        ComputeAbstractsFromInterface(it);
    }

    ArenaVector<ETSFunctionType *> merged(ProgramAllocator()->Adapter());
    CreateFunctionTypesFromAbstracts(CollectAbstractSignaturesFromObject(interfaceType), &merged);
    ArenaUnorderedSet<ETSObjectType *> abstractInheritanceTarget(ProgramAllocator()->Adapter());

    for (auto *interface : interfaceType->Interfaces()) {
        auto found = cachedComputedAbstracts->find(interface);
        ES2PANDA_ASSERT(found != cachedComputedAbstracts->end());

        if (!abstractInheritanceTarget.insert(found->first).second) {
            continue;
        }

        MergeComputedAbstracts(merged, found->second.first);

        for (auto *base : found->second.second) {
            abstractInheritanceTarget.insert(base);
        }
    }

    cachedComputedAbstracts->insert({interfaceType, {merged, abstractInheritanceTarget}});
}

ArenaVector<ETSFunctionType *> &ETSChecker::GetAbstractsForClass(ETSObjectType *classType)
{
    ArenaVector<ETSFunctionType *> merged(ProgramAllocator()->Adapter());
    CreateFunctionTypesFromAbstracts(CollectAbstractSignaturesFromObject(classType), &merged);

    ArenaUnorderedSet<ETSObjectType *> abstractInheritanceTarget(ProgramAllocator()->Adapter());
    if (classType->SuperType() != nullptr) {
        auto base = GetCachedComputedAbstracts()->find(classType->SuperType());
        ES2PANDA_ASSERT(base != GetCachedComputedAbstracts()->end());
        MergeComputedAbstracts(merged, base->second.first);

        abstractInheritanceTarget.insert(base->first);
        for (auto *it : base->second.second) {
            abstractInheritanceTarget.insert(it);
        }
    }

    for (auto *it : classType->Interfaces()) {
        ComputeAbstractsFromInterface(it);
        auto found = GetCachedComputedAbstracts()->find(it);
        ES2PANDA_ASSERT(found != GetCachedComputedAbstracts()->end());

        if (!abstractInheritanceTarget.insert(found->first).second) {
            continue;
        }

        MergeComputedAbstracts(merged, found->second.first);

        for (auto *interface : found->second.second) {
            abstractInheritanceTarget.insert(interface);
        }
    }

    return GetCachedComputedAbstracts()->insert({classType, {merged, abstractInheritanceTarget}}).first->second.first;
}

[[maybe_unused]] static bool DoObjectImplementInterface(const ETSObjectType *interfaceType, const ETSObjectType *target,
                                                        const ETSChecker *checker)
{
    auto &interfaces = interfaceType->Interfaces();
    return std::any_of(interfaces.begin(), interfaces.end(), [&target, checker](auto *it) {
        return it->IsSameBasedGeneric(checker->Relation(), target) || DoObjectImplementInterface(it, target, checker);
    });
}

static bool CheckIfInterfaceCanBeFoundOnDifferentPaths(const ETSObjectType *classType,
                                                       const ETSObjectType *interfaceType, const ETSChecker *checker)
{
    return std::count_if(classType->Interfaces().begin(), classType->Interfaces().end(),
                         [&interfaceType, checker](auto *it) {
                             return DoObjectImplementInterface(it, interfaceType, checker);
                         }) == 1;
}

void ETSChecker::GetInterfacesOfClass(ETSObjectType *type, ArenaVector<ETSObjectType *> &interfaces)
{
    for (auto &classInterface : type->Interfaces()) {
        if (std::find(interfaces.begin(), interfaces.end(), classInterface) == interfaces.end()) {
            interfaces.emplace_back(classInterface);
            GetInterfacesOfClass(classInterface, interfaces);
        }
    }
}

void ETSChecker::CheckIfOverrideIsValidInInterface(ETSObjectType *classType, Signature *sig, Signature *sigFunc)
{
    bool throwError = false;
    if (AreOverrideCompatible(sigFunc, sig) && sigFunc->Function()->IsStatic() == sig->Function()->IsStatic()) {
        SavedTypeRelationFlagsContext const savedFlags(Relation(), Relation()->GetTypeRelationFlags() |
                                                                       TypeRelationFlag::IGNORE_TYPE_PARAMETERS);
        if (CheckIfInterfaceCanBeFoundOnDifferentPaths(classType, sigFunc->Owner(), this) &&
            (Relation()->IsSupertypeOf(sigFunc->Owner(), sig->Owner()) ||
             Relation()->IsSupertypeOf(sig->Owner(), sigFunc->Owner()))) {
            return;
        }
        throwError = true;
    } else if (sigFunc->Function()->IsStatic() == sig->Function()->IsStatic()) {
        Relation()->Result(false);
        SavedTypeRelationFlagsContext savedFlagsCtx(Relation(), TypeRelationFlag::NO_RETURN_TYPE_CHECK);

        Relation()->SignatureIsIdenticalTo(sig, sigFunc);
        if ((Relation()->IsTrue() &&
             (sig->GetSignatureInfo()->restVar == nullptr) == (sigFunc->GetSignatureInfo()->restVar == nullptr)) ||
            (HasSameAssemblySignature(sigFunc, sig))) {
            throwError = true;
        }
    }
    if (throwError) {
        LogError(diagnostic::INTERFACE_METHOD_COLLISION,
                 {sig->Function()->Id()->Name(), sig->Owner()->Name(), sigFunc->Owner()->Name()},
                 classType->GetDeclNode()->Start());
    }
}

void ETSChecker::CheckFunctionRedeclarationInInterface(ETSObjectType *classType,
                                                       ArenaVector<Signature *> &similarSignatures, Signature *sigFunc)
{
    for (auto *const sig : similarSignatures) {
        if (sig == sigFunc) {
            return;
        }
        ES2PANDA_ASSERT(sigFunc != nullptr);
        if (sigFunc->Function()->Id()->Name() == sig->Function()->Id()->Name()) {
            if (classType->IsSameBasedGeneric(Relation(), sig->Owner())) {
                return;
            }
            if (Relation()->IsIdenticalTo(sig->Owner(), sigFunc->Owner())) {
                continue;
            }
            CheckIfOverrideIsValidInInterface(classType, sig, sigFunc);
        }
    }

    similarSignatures.push_back(sigFunc);
}

static void CallRedeclarationCheckForCorrectSignature(ir::MethodDefinition *method, ETSFunctionType *funcType,
                                                      ArenaVector<Signature *> &similarSignatures,
                                                      ETSObjectType *classType, ETSChecker *checker)
{
    ir::ScriptFunction *func = method->Function();
    ES2PANDA_ASSERT(func != nullptr);
    if (!func->IsAbstract()) {
        auto *sigFunc = funcType->FindSignature(func);
        checker->CheckFunctionRedeclarationInInterface(classType, similarSignatures, sigFunc);
    }
}

void ETSChecker::CheckInterfaceFunctions(ETSObjectType *classType)
{
    ArenaVector<ETSObjectType *> interfaces(ProgramAllocator()->Adapter());
    ArenaVector<Signature *> similarSignatures(ProgramAllocator()->Adapter());
    interfaces.emplace_back(classType);
    GetInterfacesOfClass(classType, interfaces);

    for (auto *const &interface : interfaces) {
        for (auto *const &prop : interface->Methods()) {
            if (prop->Declaration()->Node()->IsOverloadDeclaration()) {
                continue;
            }

            ir::MethodDefinition *node = prop->Declaration()->Node()->AsMethodDefinition();
            if (prop->TsType()->IsTypeError()) {
                continue;
            }
            auto *funcType = prop->TsType()->AsETSFunctionType();
            CallRedeclarationCheckForCorrectSignature(node, funcType, similarSignatures, classType, this);
            for (auto *const &overload : node->Overloads()) {
                CallRedeclarationCheckForCorrectSignature(overload, funcType, similarSignatures, classType, this);
            }
        }
    }
}

/// Traverse the interface inheritance tree and collects implemented methods
void ETSChecker::CollectImplementedMethodsFromInterfaces(ETSObjectType *classType,
                                                         std::vector<Signature *> *implementedSignatures,
                                                         const ArenaVector<ETSFunctionType *> &abstractsToBeImplemented)
{
    std::vector<ETSObjectType *> collectedInterfaces;

    for (auto &classInterface : classType->Interfaces()) {
        collectedInterfaces.emplace_back(classInterface);
    }

    size_t index = 0;

    while (index < collectedInterfaces.size()) {
        for (auto &it : abstractsToBeImplemented) {
            for (const auto &prop : collectedInterfaces[index]->Methods()) {
                GetTypeOfVariable(prop);
                AddImplementedSignature(implementedSignatures, prop, it);
            }
        }

        for (auto &currentInterfaceChild : collectedInterfaces[index]->Interfaces()) {
            collectedInterfaces.emplace_back(currentInterfaceChild);
        }

        index++;
    }
}

void ETSChecker::ValidateAbstractSignature(ArenaVector<ETSFunctionType *>::iterator &it,
                                           ArenaVector<ETSFunctionType *> &abstractsToBeImplemented,
                                           const std::vector<Signature *> &implementedSignatures,
                                           bool &functionOverridden, Accessor &isGetSetExternal)
{
    for (auto abstractSignature = (*it)->CallSignatures().begin();
         abstractSignature != (*it)->CallSignatures().end();) {
        bool foundSignature = false;
        isGetSetExternal.isGetter = (*abstractSignature)->HasSignatureFlag(SignatureFlags::GETTER);
        isGetSetExternal.isSetter = (*abstractSignature)->HasSignatureFlag(SignatureFlags::SETTER);
        isGetSetExternal.isExternal = (*abstractSignature)->Function()->IsExternal();
        for (auto *const implemented : implementedSignatures) {
            if (implemented->HasSignatureFlag(SignatureFlags::NEED_RETURN_TYPE)) {
                implemented->OwnerVar()->Declaration()->Node()->Check(this);
            }
            Signature *substImplemented = AdjustForTypeParameters(*abstractSignature, implemented);

            if (substImplemented == nullptr) {
                continue;
            }

            if (!AreOverrideCompatible(*abstractSignature, substImplemented) ||
                !IsReturnTypeSubstitutable(substImplemented, *abstractSignature)) {
                continue;
            }

            if ((*it)->CallSignatures().size() > 1) {
                abstractSignature = (*it)->CallSignatures().erase(abstractSignature);
                foundSignature = true;
            } else {
                it = abstractsToBeImplemented.erase(it);
                functionOverridden = true;
            }

            break;
        }

        if (functionOverridden) {
            break;
        }

        if (!foundSignature) {
            ++abstractSignature;
        }
    }
}

void ETSChecker::ValidateNonOverriddenFunction(ETSObjectType *classType, ArenaVector<ETSFunctionType *>::iterator &it,
                                               ArenaVector<ETSFunctionType *> &abstractsToBeImplemented,
                                               bool &functionOverridden, const Accessor &isGetSet)
{
    auto superClassType = classType->SuperType();
    while (!functionOverridden && superClassType != nullptr) {
        for (auto *field : superClassType->Fields()) {
            if (field->Name() == (*it)->Name()) {
                auto *newProp = field->Declaration()
                                    ->Node()
                                    ->Clone(ProgramAllocator(), classType->GetDeclNode())
                                    ->AsClassProperty();
                newProp->AddModifier(ir::ModifierFlags::SUPER_OWNER);
                newProp->AddModifier(isGetSet.isGetter && isGetSet.isSetter ? ir::ModifierFlags::GETTER_SETTER
                                     : isGetSet.isGetter                    ? ir::ModifierFlags::GETTER
                                                                            : ir::ModifierFlags::SETTER);
                auto *newFieldDecl =
                    ProgramAllocator()->New<varbinder::LetDecl>(newProp->Key()->AsIdentifier()->Name());
                newFieldDecl->BindNode(newProp);

                auto newFieldVar = classType->GetDeclNode()
                                       ->Scope()
                                       ->AsClassScope()
                                       ->InstanceFieldScope()
                                       ->AddDecl(ProgramAllocator(), newFieldDecl, ScriptExtension::ETS)
                                       ->AsLocalVariable();
                newFieldVar->AddFlag(varbinder::VariableFlags::PROPERTY);
                newFieldVar->AddFlag(varbinder::VariableFlags::PUBLIC);
                classType->AddProperty<PropertyType::INSTANCE_FIELD>(newFieldVar);
                it = abstractsToBeImplemented.erase(it);
                functionOverridden = true;
                break;
            }
        }

        superClassType = superClassType->SuperType();
    }
}

void ETSChecker::ApplyModifiersAndRemoveImplementedAbstracts(ArenaVector<ETSFunctionType *>::iterator &it,
                                                             ArenaVector<ETSFunctionType *> &abstractsToBeImplemented,
                                                             ETSObjectType *classType, bool &functionOverridden,
                                                             const Accessor &isGetSetExternal)
{
    for (auto *field : classType->Fields()) {
        if (field->Declaration()->Node()->AsClassProperty()->IsStatic()) {
            continue;
        }

        if (field->Name() == (*it)->Name()) {
            field->Declaration()->Node()->AddModifier(isGetSetExternal.isGetter && isGetSetExternal.isSetter
                                                          ? ir::ModifierFlags::GETTER_SETTER
                                                      : isGetSetExternal.isGetter ? ir::ModifierFlags::GETTER
                                                                                  : ir::ModifierFlags::SETTER);
            it = abstractsToBeImplemented.erase(it);
            functionOverridden = true;
            break;
        }
    }
}

void ETSChecker::ValidateAbstractMethodsToBeImplemented(ArenaVector<ETSFunctionType *> &abstractsToBeImplemented,
                                                        ETSObjectType *classType,
                                                        const std::vector<Signature *> &implementedSignatures)
{
    SavedTypeRelationFlagsContext savedFlagsCtx(Relation(), TypeRelationFlag::NO_RETURN_TYPE_CHECK);
    for (auto it = abstractsToBeImplemented.begin(); it != abstractsToBeImplemented.end();) {
        bool functionOverridden = false;
        Accessor isGetSetExternal;

        ValidateAbstractSignature(it, abstractsToBeImplemented, implementedSignatures, functionOverridden,
                                  isGetSetExternal);

        if (functionOverridden) {
            continue;
        }

        if (!isGetSetExternal.isGetter && !isGetSetExternal.isSetter) {
            it++;
            continue;
        }

        ApplyModifiersAndRemoveImplementedAbstracts(it, abstractsToBeImplemented, classType, functionOverridden,
                                                    isGetSetExternal);

        if (functionOverridden) {
            continue;
        }

        ValidateNonOverriddenFunction(classType, it, abstractsToBeImplemented, functionOverridden, isGetSetExternal);

        if (!functionOverridden) {
            it++;
        }
    }
}

void ETSChecker::MaybeReportErrorsForOverridingValidation(ArenaVector<ETSFunctionType *> &abstractsToBeImplemented,
                                                          ETSObjectType *classType, const lexer::SourcePosition &pos,
                                                          bool reportError)
{
    if (!abstractsToBeImplemented.empty() && reportError) {
        auto unimplementedSignature = abstractsToBeImplemented.front()->CallSignatures().front();
        auto containingObjectName = GetContainingObjectNameFromSignature(unimplementedSignature);
        if (unimplementedSignature->HasSignatureFlag(SignatureFlags::DEFAULT)) {
            return;
        }
        if (unimplementedSignature->HasSignatureFlag(SignatureFlags::GETTER)) {
            LogError(diagnostic::GETTER_MISSING_IMPL,
                     {classType->Name(), unimplementedSignature->Function()->Id()->Name(), containingObjectName}, pos);
            return;
        }
        if (unimplementedSignature->HasSignatureFlag(SignatureFlags::SETTER)) {
            LogError(diagnostic::SETTER_MISSING_IMPL,
                     {classType->Name(), unimplementedSignature->Function()->Id()->Name(), containingObjectName}, pos);
            return;
        }
        LogError(diagnostic::MISSING_OVERRIDE_OF_ABSTRACT_METH,
                 {classType->Name(), unimplementedSignature->Function()->Id()->Name(), unimplementedSignature,
                  containingObjectName},
                 pos);
    }
}

void ETSChecker::ValidateOverriding(ETSObjectType *classType, const lexer::SourcePosition &pos)
{
    if (GetCachedComputedAbstracts()->find(classType) != GetCachedComputedAbstracts()->end()) {
        return;
    }

    bool throwError = true;
    if (classType->HasObjectFlag(ETSObjectFlags::ABSTRACT)) {
        throwError = false;
    }

    if (classType->SuperType() != nullptr) {
        ValidateOverriding(classType->SuperType(), classType->SuperType()->GetDeclNode()->Start());
    }

    auto &abstractsToBeImplemented = GetAbstractsForClass(classType);
    std::vector<Signature *> implementedSignatures;

    // Since interfaces can define function bodies we have to collect the implemented ones first
    CollectImplementedMethodsFromInterfaces(classType, &implementedSignatures, abstractsToBeImplemented);
    CheckInterfaceFunctions(classType);

    auto *superIter = classType;
    do {
        for (auto &it : abstractsToBeImplemented) {
            for (const auto &prop : superIter->Methods()) {
                GetTypeOfVariable(prop);
                AddImplementedSignature(&implementedSignatures, prop, it);
            }
        }
        superIter = superIter->SuperType();
    } while (superIter != nullptr);
    ValidateAbstractMethodsToBeImplemented(abstractsToBeImplemented, classType, implementedSignatures);
    MaybeReportErrorsForOverridingValidation(abstractsToBeImplemented, classType, pos, throwError);
}

void ETSChecker::AddImplementedSignature(std::vector<Signature *> *implementedSignatures,
                                         varbinder::LocalVariable *function, ETSFunctionType *it)
{
    if (!function->TsType()->IsETSFunctionType()) {
        return;
    }

    for (auto signature : function->TsType()->AsETSFunctionType()->CallSignatures()) {
        if (signature->Function()->IsAbstract() || signature->Function()->IsStatic()) {
            continue;
        }

        if (signature->Function()->Id()->Name() == it->Name()) {
            implementedSignatures->emplace_back(signature);
        }
    }
}

void ETSChecker::CheckLocalClass(ir::ClassDefinition *classDef, CheckerStatus &checkerStatus)
{
    if (!classDef->IsLocal()) {
        return;
    }
    checkerStatus |= CheckerStatus::IN_LOCAL_CLASS;
    if (!classDef->Parent()->Parent()->IsBlockStatement()) {
        LogError(diagnostic::LOCAL_CLASS_INVALID_CTX, {}, classDef->Start());
    }
    // NOTE(dkofanov): Related to spec 17.9.3 expecting CTE (native methods in local classes).
    // Actually, if I'm not mistaken, the only reason to forbid this is problematic binding of native method to the
    // mangled local class method, which is not really a reason for such restrictions. The spec should be revisited in
    // the future.
    if (classDef->HasNativeMethod()) {
        LogError(diagnostic::LOCAL_CLASS_NATIVE_METHOD, {classDef->Ident()->Name()}, classDef->Start());
    }
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
void ETSChecker::CheckClassDefinition(ir::ClassDefinition *classDef)
{
    classDef->SetClassDefinitionChecked();

    if (classDef->TsType() == nullptr) {
        ES2PANDA_ASSERT(IsAnyError());
        classDef->SetTsType(GlobalTypeError());
    }

    if (classDef->TsType()->IsTypeError()) {
        return;
    }

    auto *classType = classDef->TsType()->MaybeBaseTypeOfGradualType()->AsETSObjectType();
    if (classType->SuperType() != nullptr) {
        classType->SuperType()->GetDeclNode()->Check(this);
    }

    if (classType->GetDeclNode() != classDef) {
        ES2PANDA_ASSERT(IsAnyError());
        classDef->SetTsType(GlobalTypeError());
        return;
    }

    auto newStatus = checker::CheckerStatus::IN_CLASS;
    if (Context().ContainingClass() != classType) {
        classType->SetEnclosingType(Context().ContainingClass());
    }

    if (classDef->IsInner()) {
        newStatus |= CheckerStatus::INNER_CLASS;
        classType->AddObjectFlag(checker::ETSObjectFlags::INNER);
    }

    classDef->IsModule() ? classType->AddObjectFlag(checker::ETSObjectFlags::GLOBAL)
                         : CheckLocalClass(classDef, newStatus);

    checker::ScopeContext scopeCtx(this, classDef->Scope());
    auto savedContext = SavedCheckerContext(this, newStatus, classType);

    ResolveDeclaredMembersOfObject(classType);

    if (classDef->IsAbstract()) {
        AddStatus(checker::CheckerStatus::IN_ABSTRACT);
        classType->AddObjectFlag(checker::ETSObjectFlags::ABSTRACT);
    }

    if (classDef->IsStatic() && !Context().ContainingClass()->HasObjectFlag(ETSObjectFlags::GLOBAL)) {
        AddStatus(checker::CheckerStatus::IN_STATIC_CONTEXT);
    }

    // NOTE(gogabr): temporary, until we have proper bridges, see #16485
    // Don't check overriding for synthetic functional classes.
    if ((static_cast<ir::AstNode *>(classDef)->Modifiers() & ir::ModifierFlags::FUNCTIONAL) == 0) {
        ValidateOverriding(classType, classDef->Start());
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    TransformProperties(classType);
    CheckClassElement(classDef);

    if (classType->SuperType() == nullptr) {
        return;
    }

    CheckClassAnnotations(classDef);

    if (classDef->IsGlobal()) {
        return;
    }

    CheckDynamicInheritanceAndImplement(classType);
    CheckConstructors(classDef, classType);
    CheckValidInheritance(classType, classDef);
    CheckConstFields(classType);
    CheckGetterSetterProperties(classType);
    CheckInvokeMethodsLegitimacy(classType);
    CheckTypeParameterVariance(classDef);
}

void ETSChecker::CheckClassElement(ir::ClassDefinition *classDef)
{
    for (auto *it : classDef->Body()) {
        if (!it->IsClassProperty()) {
            if (it->IsETSModule() && it->AsETSModule()->IsNamespace()) {
                ES2PANDA_ASSERT(IsAnyError());
                continue;
            }
        }
        it->Check(this);
    }
}

void ETSChecker::CheckClassAnnotations(ir::ClassDefinition *classDef)
{
    CheckAnnotations(classDef->Annotations());
    if (classDef->TypeParams() != nullptr) {
        for (auto *param : classDef->TypeParams()->Params()) {
            CheckAnnotations(param->Annotations());
        }
    }
}

void ETSChecker::CheckInterfaceAnnotations(ir::TSInterfaceDeclaration *interfaceDecl)
{
    CheckAnnotations(interfaceDecl->Annotations());
    if (interfaceDecl->TypeParams() != nullptr) {
        for (auto *param : interfaceDecl->TypeParams()->Params()) {
            CheckAnnotations(param->Annotations());
        }
    }
}

void ETSChecker::CheckConstructors(ir::ClassDefinition *classDef, ETSObjectType *classType)
{
    if (!classDef->IsDeclare()) {
        for (auto *it : classType->ConstructSignatures()) {
            if (it->Function()->Body() == nullptr) {
                continue;
            }
            CheckCyclicConstructorCall(it);
            CheckImplicitSuper(classType, it);
            CheckThisOrSuperCallInConstructor(classType, it);
        }
    }
}

void ETSChecker::CheckImplicitSuper(ETSObjectType *classType, Signature *ctorSig)
{
    if (classType == GlobalETSObjectType()) {
        return;
    }

    if (ctorSig->Function()->IsNative() && ctorSig->Function()->IsConstructor()) {
        return;
    }

    auto &stmts = ctorSig->Function()->Body()->AsBlockStatement()->Statements();
    const auto thisCall = std::find_if(stmts.begin(), stmts.end(), [](const ir::Statement *stmt) {
        return stmt->IsExpressionStatement() && stmt->AsExpressionStatement()->GetExpression()->IsCallExpression() &&
               stmt->AsExpressionStatement()->GetExpression()->AsCallExpression()->Callee()->IsThisExpression();
    });
    // There is an alternate constructor invocation, no need for super constructor invocation
    if (thisCall != stmts.end()) {
        return;
    }

    const auto superExpr = std::find_if(stmts.begin(), stmts.end(), [](const ir::Statement *stmt) {
        return stmt->IsExpressionStatement() && stmt->AsExpressionStatement()->GetExpression()->IsCallExpression() &&
               stmt->AsExpressionStatement()->GetExpression()->AsCallExpression()->Callee()->IsSuperExpression();
    });
    // There is no super expression
    if (superExpr == stmts.end()) {
        const auto superTypeCtorSigs = classType->SuperType()->ConstructSignatures();
        const auto superTypeCtorSig = std::find_if(superTypeCtorSigs.begin(), superTypeCtorSigs.end(),
                                                   [](const Signature *sig) { return sig->MinArgCount() == 0; });
        // Super type has no parameterless ctor
        if (superTypeCtorSig == superTypeCtorSigs.end()) {
            LogError(diagnostic::CTOR_MISSING_SUPER_CALL, {}, ctorSig->Function()->Start());
        }

        ctorSig->Function()->AddFlag(ir::ScriptFunctionFlags::IMPLICIT_SUPER_CALL_NEEDED);
    }
}

void ETSChecker::CheckThisOrSuperCallInConstructor(ETSObjectType *classType, Signature *ctorSig)
{
    if (classType == GlobalETSObjectType()) {
        return;
    }

    if (ctorSig->Function()->IsNative() && ctorSig->Function()->IsConstructor()) {
        return;
    }

    for (auto it : ctorSig->Function()->Body()->AsBlockStatement()->Statements()) {
        if (it->IsExpressionStatement() && it->AsExpressionStatement()->GetExpression()->IsCallExpression() &&
            (it->AsExpressionStatement()->GetExpression()->AsCallExpression()->Callee()->IsThisExpression() ||
             it->AsExpressionStatement()->GetExpression()->AsCallExpression()->Callee()->IsSuperExpression())) {
            ArenaVector<const ir::Expression *> expressions =
                ArenaVector<const ir::Expression *>(ProgramAllocator()->Adapter());
            expressions.insert(expressions.end(),
                               it->AsExpressionStatement()->GetExpression()->AsCallExpression()->Arguments().begin(),
                               it->AsExpressionStatement()->GetExpression()->AsCallExpression()->Arguments().end());
            CheckExpressionsInConstructor(expressions);
        }
    }
}

void ETSChecker::CheckExpressionsInConstructor(const ArenaVector<const ir::Expression *> &arguments)
{
    for (auto *arg : arguments) {
        auto expressions = CheckMemberOrCallOrObjectExpressionInConstructor(arg);

        if (arg->IsETSNewClassInstanceExpression()) {
            expressions.insert(expressions.end(), arg->AsETSNewClassInstanceExpression()->GetArguments().begin(),
                               arg->AsETSNewClassInstanceExpression()->GetArguments().end());
        } else if (arg->IsArrayExpression()) {
            expressions.insert(expressions.end(), arg->AsArrayExpression()->Elements().begin(),
                               arg->AsArrayExpression()->Elements().end());
        } else if (arg->IsBinaryExpression()) {
            expressions.push_back(arg->AsBinaryExpression()->Left());
            expressions.push_back(arg->AsBinaryExpression()->Right());
        } else if (arg->IsAssignmentExpression()) {
            expressions.push_back(arg->AsAssignmentExpression()->Left());
            expressions.push_back(arg->AsAssignmentExpression()->Right());
        } else if (arg->IsTSAsExpression()) {
            expressions.push_back(arg->AsTSAsExpression()->Expr());
        } else if (arg->IsConditionalExpression()) {
            expressions.push_back(arg->AsConditionalExpression()->Test());
            expressions.push_back(arg->AsConditionalExpression()->Consequent());
            expressions.push_back(arg->AsConditionalExpression()->Alternate());
        } else if (arg->IsTypeofExpression()) {
            expressions.push_back(arg->AsTypeofExpression()->Argument());
        } else if (arg->IsTSNonNullExpression()) {
            expressions.push_back(arg->AsTSNonNullExpression()->Expr());
        } else if (arg->IsUnaryExpression()) {
            expressions.push_back(arg->AsUnaryExpression()->Argument());
        } else if (arg->IsUpdateExpression()) {
            expressions.push_back(arg->AsUpdateExpression()->Argument());
        }

        if (!expressions.empty()) {
            CheckExpressionsInConstructor(expressions);
        }
    }
}

ArenaVector<const ir::Expression *> ETSChecker::CheckMemberOrCallOrObjectExpressionInConstructor(
    const ir::Expression *arg)
{
    ArenaVector<const ir::Expression *> expressions =
        ArenaVector<const ir::Expression *>(ProgramAllocator()->Adapter());

    if (arg->IsMemberExpression()) {
        if ((arg->AsMemberExpression()->Object()->IsSuperExpression() ||
             arg->AsMemberExpression()->Object()->IsThisExpression())) {
            const auto what = (arg->AsMemberExpression()->Object()->IsSuperExpression() ? "super" : "this");
            LogError(diagnostic::THIS_OR_SUPER_IN_CTOR, {what}, arg->Start());
        }

        expressions.push_back(arg->AsMemberExpression()->Property());
        expressions.push_back(arg->AsMemberExpression()->Object());
    } else if (arg->IsCallExpression()) {
        expressions.insert(expressions.end(), arg->AsCallExpression()->Arguments().begin(),
                           arg->AsCallExpression()->Arguments().end());

        if (arg->AsCallExpression()->Callee()->IsMemberExpression() &&
            (arg->AsCallExpression()->Callee()->AsMemberExpression()->Object()->IsSuperExpression() ||
             arg->AsCallExpression()->Callee()->AsMemberExpression()->Object()->IsThisExpression()) &&
            !arg->AsCallExpression()->Callee()->AsMemberExpression()->Property()->IsStatic()) {
            const auto what =
                (arg->AsCallExpression()->Callee()->AsMemberExpression()->Object()->IsSuperExpression() ? "super"
                                                                                                        : "this");
            LogError(diagnostic::THIS_OR_SUPER_IN_CTOR, {what}, arg->Start());
        }
    } else if (arg->IsObjectExpression()) {
        for (auto *prop : arg->AsObjectExpression()->Properties()) {
            expressions.push_back(prop->AsProperty()->Value());
        }
    }

    return expressions;
}

void ETSChecker::CheckConstFields(const ETSObjectType *classType)
{
    for (const auto &prop : classType->Fields()) {
        if (!(prop->Declaration()->IsConstDecl() || prop->Declaration()->IsReadonlyDecl()) ||
            !prop->HasFlag(varbinder::VariableFlags::EXPLICIT_INIT_REQUIRED)) {
            continue;
        }
        CheckConstFieldInitialized(classType, prop);
    }
}

void ETSChecker::CheckConstFieldInitialized(const ETSObjectType *classType, varbinder::LocalVariable *classVar)
{
    const bool classVarStatic = classVar->Declaration()->Node()->AsClassProperty()->IsStatic();
    for (const auto &prop : classType->Methods()) {
        if (!prop->TsType()->IsETSFunctionType()) {
            continue;
        }

        const auto &callSigs = prop->TsType()->AsETSFunctionType()->CallSignatures();
        for (const auto *signature : callSigs) {
            if ((signature->Function()->IsConstructor() && !classVarStatic) ||
                (signature->Function()->IsStaticBlock() && classVarStatic)) {
                CheckConstFieldInitialized(signature, classVar);
            }
        }
    }
}

void ETSChecker::FindAssignment(const ir::AstNode *node, const varbinder::LocalVariable *classVar, bool &initialized)
{
    if (node->IsAssignmentExpression() && node->AsAssignmentExpression()->Target() == classVar) {
        if (initialized) {
            LogError(diagnostic::MAYBE_DOUBLE_INIT, {classVar->Declaration()->Name()}, node->Start());
        }

        initialized = true;
        return;
    }

    FindAssignments(node, classVar, initialized);
}

void ETSChecker::FindAssignments(const ir::AstNode *node, const varbinder::LocalVariable *classVar, bool &initialized)
{
    node->Iterate(
        [this, classVar, &initialized](ir::AstNode *childNode) { FindAssignment(childNode, classVar, initialized); });
}

void ETSChecker::CheckConstFieldInitialized(const Signature *signature, varbinder::LocalVariable *classVar)
{
    bool initialized = false;
    const auto &stmts = signature->Function()->Body()->AsBlockStatement()->Statements();
    const auto it = stmts.begin();
    if (it != stmts.end()) {
        if (const auto *first = *it;
            first->IsExpressionStatement() && first->AsExpressionStatement()->GetExpression()->IsCallExpression() &&
            first->AsExpressionStatement()->GetExpression()->AsCallExpression()->Callee()->IsThisExpression()) {
            initialized = true;
        }
    }

    // NOTE: szd. control flow
    FindAssignments(signature->Function()->Body(), classVar, initialized);
    if (!initialized) {
        LogError(diagnostic::MAYBE_MISSING_INIT, {classVar->Declaration()->Name()}, signature->Function()->End());
    }

    classVar->RemoveFlag(varbinder::VariableFlags::EXPLICIT_INIT_REQUIRED);
}

void ETSChecker::CheckInnerClassMembers(const ETSObjectType *classType)
{
    for (const auto &[_, it] : classType->StaticMethods()) {
        (void)_;
        LogError(diagnostic::INNER_CLASS_WITH_STATIC_METH, {}, it->Declaration()->Node()->Start());
    }

    for (const auto &[_, it] : classType->StaticFields()) {
        (void)_;
        if (!it->Declaration()->IsReadonlyDecl()) {
            LogError(diagnostic::INNER_CLASS_MUTABLE_STATIC_PROP, {}, it->Declaration()->Node()->Start());
        }
    }
}

bool ETSChecker::ValidateArrayIndex(ir::Expression *const expr, bool relaxed)
{
    auto const expressionType = expr->Check(this)->MaybeBaseTypeOfGradualType();
    if (expressionType->IsTypeError()) {
        return false;
    }

    if (!expressionType->IsETSObjectType() ||
        (!expressionType->AsETSObjectType()->HasObjectFlag(relaxed ? ETSObjectFlags::BUILTIN_ARRAY_NUMERIC
                                                                   : ETSObjectFlags::BUILTIN_ARRAY_INDEX))) {
        LogError(diagnostic::INVALID_INDEX_TYPE, {expressionType->ToString()}, expr->Start());
        return false;
    }

    if (!relaxed || !expressionType->IsConstantType() || !expr->IsNumberLiteral()) {
        return true;
    }

    ES2PANDA_ASSERT(expr->IsNumberLiteral());
    double value = expr->AsNumberLiteral()->Number().GetDouble();

    double intPart;
    if (std::modf(value, &intPart) != 0.0) {
        LogError(diagnostic::INDEX_NONINTEGRAL_FLOAT, {}, expr->Start());
        return false;
    }

    if (intPart < 0.0) {
        LogError(diagnostic::NEGATIVE_INDEX, {}, expr->Start());
        return false;
    }

    return true;
}

std::optional<std::size_t> ETSChecker::GetTupleElementAccessValue(const ir::Expression *expr)
{
    auto checkLongValBounds = [this](int64_t val, const lexer::SourcePosition &p) -> std::optional<size_t> {
        if (val < 0) {
            LogError(diagnostic::TUPLE_INDEX_OOB, {}, p);
            return std::nullopt;
        }
        return static_cast<size_t>(val);
    };

    if (expr->IsNumberLiteral()) {
        auto num = expr->AsNumberLiteral()->Number();
        if (num.IsInt()) {
            return checkLongValBounds(num.GetInt(), expr->Start());
        }
        if (num.IsLong()) {
            return checkLongValBounds(num.GetLong(), expr->Start());
        }
        ES2PANDA_UNREACHABLE();
    }

    // Below code should be unreachable after removing primitives
    auto type = expr->TsType();

    ES2PANDA_ASSERT(type->HasTypeFlag(TypeFlag::CONSTANT | TypeFlag::ETS_CONVERTIBLE_TO_NUMERIC));

    switch (ETSType(type)) {
        case TypeFlag::BYTE: {
            return type->AsByteType()->GetValue();
        }
        case TypeFlag::SHORT: {
            return type->AsShortType()->GetValue();
        }
        case TypeFlag::INT: {
            return type->AsIntType()->GetValue();
        }
        case TypeFlag::LONG: {
            if (auto val = type->AsLongType()->GetValue();
                val <= std::numeric_limits<int32_t>::max() && val >= std::numeric_limits<int32_t>::min()) {
                return static_cast<std::size_t>(val);
            }
            return std::nullopt;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

bool ETSChecker::ValidateTupleIndex(const ETSTupleType *const tuple, ir::MemberExpression *const expr,
                                    const bool reportError)
{
    auto const exprType = expr->Property()->Check(this);
    auto const *const unboxedExpressionType = MaybeUnboxInRelation(exprType);

    ES2PANDA_ASSERT(exprType != nullptr);

    if (!exprType->HasTypeFlag(TypeFlag::CONSTANT)) {
        if (exprType->IsETSObjectType() && (unboxedExpressionType != nullptr)) {
            return ValidateTupleIndexFromEtsObject(tuple, expr);
        }
        if (reportError) {
            LogError(diagnostic::TUPLE_INDEX_NONCONST, {}, expr->Property()->Start());
        }
        return false;
    }

    if (!Relation()->IsSupertypeOf(GlobalIntBuiltinType(), exprType) &&
        !Relation()->IsSupertypeOf(GlobalLongBuiltinType(), exprType)) {
        LogError(diagnostic::TUPLE_INDEX_NOT_INT, {}, expr->Property()->Start());
        return false;
    }

    if (expr->Property()->IsAssignmentExpression()) {
        if (reportError) {
            LogError(diagnostic::TUPLE_INDEX_NONCONST, {}, expr->Property()->Start());
        }
        return false;
    }

    auto exprValue = GetTupleElementAccessValue(expr->Property());
    if (!exprValue.has_value() || (*exprValue >= tuple->GetTupleSize())) {
        if (reportError) {
            LogError(diagnostic::TUPLE_INDEX_OOB, {}, expr->Property()->Start());
        }
        return false;
    }

    return true;
}

bool ETSChecker::ValidateTupleIndexFromEtsObject(const ETSTupleType *const tuple, ir::MemberExpression *const expr)
{
    if (expr->Property() == nullptr || expr->Property()->Variable() == nullptr ||
        expr->Property()->Variable()->Declaration() == nullptr ||
        expr->Property()->Variable()->Declaration()->Node() == nullptr ||
        !(expr->Property()->Variable()->Declaration()->Node()->IsMethodDefinition() ||
          expr->Property()->Variable()->Declaration()->Node()->IsClassProperty() ||
          expr->Property()->Variable()->Declaration()->Node()->IsClassStaticBlock() ||
          expr->Property()->Variable()->Declaration()->Node()->IsOverloadDeclaration())) {
        LogError(diagnostic::TUPLE_INDEX_NONCONST, {}, expr->Start());
        return false;
    }
    auto *value = expr->Property()->Variable()->Declaration()->Node()->AsClassElement()->Value();
    if (value == nullptr) {
        LogError(diagnostic::TUPLE_INDEX_NONCONST, {}, expr->Property()->Start());
        return false;
    }
    auto *exprType = value->TsType();
    ES2PANDA_ASSERT(exprType != nullptr);
    if (!exprType->HasTypeFlag(TypeFlag::CONSTANT)) {
        LogError(diagnostic::TUPLE_INDEX_NONCONST, {}, expr->Property()->Start());
        return false;
    }

    if (!exprType->HasTypeFlag(TypeFlag::ETS_ARRAY_INDEX | TypeFlag::LONG)) {
        LogError(diagnostic::TUPLE_INDEX_NOT_INT, {}, expr->Property()->Start());
        return false;
    }

    auto exprValue = GetTupleElementAccessValue(expr);
    if (!exprValue.has_value() || (*exprValue >= tuple->GetTupleSize())) {
        LogError(diagnostic::TUPLE_INDEX_OOB, {}, expr->Property()->Start());
        return false;
    }

    return true;
}

namespace {

bool IsExpressionInClassProperty(ir::Expression const *expr)
{
    for (ir::AstNode const *node = expr; node != nullptr && !node->IsClassDefinition(); node = node->Parent()) {
        if (node->IsClassProperty()) {
            return true;
        }
    }
    return false;
}

}  // namespace

ETSObjectType *ETSChecker::CheckThisOrSuperAccess(ir::Expression *node, ETSObjectType *classType, std::string_view msg)
{
    if ((Context().Status() & CheckerStatus::IGNORE_VISIBILITY) != 0U) {
        return classType;
    }

    if (node->Parent()->IsCallExpression() && (node->Parent()->AsCallExpression()->Callee() == node) &&
        !node->Parent()->HasAstNodeFlags(ir::AstNodeFlags::RESIZABLE_REST)) {
        if (Context().ContainingSignature() == nullptr) {
            LogError(diagnostic::CTOR_CLASS_NOT_FIRST, {msg}, node->Start());
            return classType;
        }

        auto *sig = Context().ContainingSignature();
        ES2PANDA_ASSERT(sig->Function()->Body() && sig->Function()->Body()->IsBlockStatement());

        if (!sig->HasSignatureFlag(SignatureFlags::CONSTRUCT)) {
            LogError(diagnostic::CTOR_CLASS_NOT_FIRST, {msg}, node->Start());
            return classType;
        }

        if (sig->Function()->Body()->AsBlockStatement()->Statements().front() != node->Parent()->Parent()) {
            LogError(diagnostic::CTOR_CLASS_NOT_FIRST, {msg}, node->Start());
            return classType;
        }
    }

    if (HasStatus(CheckerStatus::IN_STATIC_CONTEXT)) {
        LogError(diagnostic::CTOR_REF_IN_STATIC_CTX, {msg}, node->Start());
        return classType;
    }

    if (node->IsThisExpression() && IsExpressionInClassProperty(node)) {
        LogDiagnostic(diagnostic::THIS_IN_FIELD_INITIALIZER, {}, node->Start());
    }

    if (classType == nullptr ||
        (classType->GetDeclNode()->IsClassDefinition() && classType->GetDeclNode()->AsClassDefinition()->IsGlobal())) {
        LogError(diagnostic::CTOR_REF_INVALID_CTX_GLOBAL, {msg}, node->Start());
        return GlobalBuiltinErrorType();
    }

    return classType;
}

void ETSChecker::CheckCyclicConstructorCall(Signature *signature)
{
    ES2PANDA_ASSERT(signature->Function());

    if (signature->Function()->IsExternal()) {
        return;
    }

    // This is a condition set up to handle error scenarios.
    if (signature->Function()->Body() == nullptr) {
        return;
    }

    auto *funcBody = signature->Function()->Body()->AsBlockStatement();

    TypeStackElement tse(this, signature, {{diagnostic::RECURSIVE_CTOR}}, signature->Function()->Start());
    if (tse.HasTypeError()) {
        return;
    }

    if (!funcBody->Statements().empty() && funcBody->Statements()[0]->IsExpressionStatement() &&
        funcBody->Statements()[0]->AsExpressionStatement()->GetExpression()->IsCallExpression() &&
        funcBody->Statements()[0]
            ->AsExpressionStatement()  // CC-OFF(G.FMT.06-CPP,G.FMT.02-CPP) project code style
            ->GetExpression()
            ->AsCallExpression()
            ->Callee()
            ->IsThisExpression()) {
        auto *constructorCall = funcBody->Statements()[0]->AsExpressionStatement()->GetExpression()->AsCallExpression();
        if (constructorCall->TsType() == nullptr || constructorCall->TsType()->HasTypeFlag(TypeFlag::TYPE_ERROR)) {
            LogError(diagnostic::NO_SUCH_CTOR_SIG, {}, constructorCall->Start());
            return;
        }
        ES2PANDA_ASSERT(constructorCall->Signature());
        CheckCyclicConstructorCall(constructorCall->Signature());
    }
}

ETSObjectType *ETSChecker::CheckExceptionOrErrorType(checker::Type *type, const lexer::SourcePosition pos)
{
    if (type->IsGradualType()) {
        return CheckExceptionOrErrorType(type->AsGradualType()->GetBaseType(), pos);
    }
    ES2PANDA_ASSERT(type != nullptr);
    if (!type->IsETSObjectType() || (!Relation()->IsAssignableTo(type, GlobalBuiltinExceptionType()) &&
                                     !Relation()->IsAssignableTo(type, GlobalBuiltinErrorType()))) {
        LogError(diagnostic::CATCH_OR_THROW_OF_INVALID_TYPE,
                 {compiler::Signatures::BUILTIN_EXCEPTION_CLASS, compiler::Signatures::BUILTIN_ERROR_CLASS}, pos);
        return GlobalETSObjectType();
    }

    return type->AsETSObjectType();
}

Type *ETSChecker::TryToInstantiate(Type *const type, ArenaAllocator *const allocator, TypeRelation *const relation,
                                   GlobalTypesHolder *const globalTypes)
{
    // NOTE: Handle generic functions
    auto *returnType = type;
    const bool isIncomplete =
        type->IsETSObjectType() && type->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::INCOMPLETE_INSTANTIATION);
    if (const bool isFunctionType = type->IsETSFunctionType(); isFunctionType || isIncomplete) {
        returnType = type->Instantiate(allocator, relation, globalTypes);
    }

    return returnType;
}

void ETSChecker::ValidateNamespaceProperty(varbinder::Variable *property, const ETSObjectType *target,
                                           const ir::Identifier *ident)
{
    ir::AstNode *parent = nullptr;
    if (property->TsType() != nullptr && !property->TsType()->IsTypeError()) {
        if (property->TsType()->IsETSMethodType()) {
            auto funcType = property->TsType()->AsETSFunctionType();
            property = !property->HasFlag(varbinder::VariableFlags::OVERLOAD)
                           ? funcType->CallSignatures()[0]->OwnerVar()
                           : property;
            ES2PANDA_ASSERT(property != nullptr);
        } else if (ident->Parent()->IsMemberExpression() &&
                   ident->Parent()->AsMemberExpression()->Object()->IsSuperExpression()) {
            LogError(diagnostic::SUPER_NOT_ACCESSIBLE, {ident->Name()}, ident->Start());
        }
    }

    if (property->Declaration() == nullptr) {
        return;
    }

    auto node = property->Declaration()->Node();
    if (node == nullptr) {
        return;
    }

    if (node->IsClassDefinition()) {
        parent = node->Parent()->Parent();
    } else if (node->Parent() != nullptr) {
        parent = node->Parent();
    }

    bool isExported = node->IsExported() || node->IsDefaultExported();
    if (parent != nullptr && parent->IsClassDefinition() && parent->AsClassDefinition()->IsNamespaceTransformed() &&
        !parent->AsClassDefinition()->IsDeclare() && !isExported) {
        LogError(diagnostic::NOT_EXPORTED, {ident->Name(), target->Name()}, ident->Start());
    }
}

void ETSChecker::ValidateResolvedProperty(varbinder::LocalVariable **property, const ETSObjectType *const target,
                                          const ir::Identifier *const ident, const PropertySearchFlags flags)
{
    if (*property != nullptr) {
        ValidateNamespaceProperty(*property, target, ident);
        return;
    }

    if (ident->Name().Is(ERROR_LITERAL)) {
        return;
    }

    using Utype = std::underlying_type_t<PropertySearchFlags>;
    static constexpr uint32_t CORRECT_PROPERTY_SEARCH_ORDER_INSTANCE = 7U;
    static_assert(static_cast<Utype>(PropertySearchFlags::SEARCH_INSTANCE) == CORRECT_PROPERTY_SEARCH_ORDER_INSTANCE,
                  "PropertySearchFlags order changed");
    static constexpr uint32_t CORRECT_PROPERTY_SEARCH_ORDER_STATIC = 56U;
    static_assert(static_cast<Utype>(PropertySearchFlags::SEARCH_STATIC) == CORRECT_PROPERTY_SEARCH_ORDER_STATIC,
                  "PropertySearchFlags order changed");
    const auto flagsNum = static_cast<Utype>(flags);
    // This algorithm swaps the first 3 bits of a number with it's consecutive 3 bits, example: 0b110001 -> 0b001110
    // Effectively it changes PropertySearchFlags to search for the appropriate declarations
    const Utype x = (flagsNum ^ (flagsNum >> 3U)) & 7U;
    const auto newFlags = PropertySearchFlags {flagsNum ^ (x | (x << 3U))};

    auto *newProp = target->GetProperty(ident->Name(), newFlags);
    if (newProp == nullptr) {
        if (ident->Name() == SET_PROTOTYPE_OF) {
            LogError(diagnostic::ERROR_ARKTS_NO_RUNTIME_PROTOTYPE_INHERITANCE, {}, ident->Start());
        } else {
            LogError(diagnostic::PROPERTY_NONEXISTENT, {ident->Name(), target->Name()}, ident->Start());
        }
        return;
    }

    *property = newProp;  // trying to recover as much as possible; log the error but treat the property as legal

    if (IsVariableStatic(newProp)) {
        LogError(diagnostic::PROP_IS_STATIC, {ident->Name(), target->Name()}, ident->Start());
        return;
    }

    LogError(diagnostic::PROP_NOT_STATIC, {ident->Name(), target->Name()}, ident->Start());
}

using VO = varbinder::ResolveBindingOptions;
varbinder::Variable *ETSChecker::GetExtensionFuncVarInGlobalFunction(const ir::MemberExpression *const memberExpr)
{
    auto propertyName = memberExpr->Property()->AsIdentifier()->Name();
    auto *globalFunctionVar = Scope()->FindInGlobal(propertyName, VO::STATIC_METHODS).variable;
    if (globalFunctionVar == nullptr) {
        return nullptr;
    }

    if (!IsExtensionETSFunctionType(GetTypeOfVariable(globalFunctionVar))) {
        return nullptr;
    }

    return globalFunctionVar;
}

varbinder::Variable *ETSChecker::GetExtensionFuncVarInGlobalField(const ir::MemberExpression *const memberExpr)
{
    auto propertyName = memberExpr->Property()->AsIdentifier()->Name();
    auto *globalFieldVar = Scope()->FindInGlobal(propertyName, VO::STATIC_VARIABLES).variable;
    if (globalFieldVar == nullptr || !IsExtensionETSFunctionType(globalFieldVar->TsType())) {
        return nullptr;
    }

    return globalFieldVar;
}

varbinder::Variable *ETSChecker::GetExtensionFuncVarInFunctionScope(const ir::MemberExpression *const memberExpr)
{
    auto propertyName = memberExpr->Property()->AsIdentifier()->Name();
    auto *funcScopeVar = Scope()->FindInFunctionScope(propertyName, VO::ALL).variable;
    if (funcScopeVar == nullptr || !IsExtensionETSFunctionType(funcScopeVar->TsType())) {
        return nullptr;
    }
    return funcScopeVar;
}

varbinder::Variable *ETSChecker::ResolveInstanceExtension(const ir::MemberExpression *const memberExpr)
{
    auto *globalFunctionVar = GetExtensionFuncVarInGlobalFunction(memberExpr);
    if (globalFunctionVar != nullptr) {
        return globalFunctionVar;
    }

    auto *globalFieldVar = GetExtensionFuncVarInGlobalField(memberExpr);
    if (globalFieldVar != nullptr) {
        return globalFieldVar;
    }

    // extension function maybe a parameter, or some lambda function defined in function.
    return GetExtensionFuncVarInFunctionScope(memberExpr);
}

PropertySearchFlags ETSChecker::GetInitialSearchFlags(const ir::MemberExpression *const memberExpr)
{
    constexpr auto FUNCTIONAL_FLAGS = PropertySearchFlags::SEARCH_METHOD | PropertySearchFlags::SEARCH_FIELD;
    constexpr auto GETTER_FLAGS = PropertySearchFlags::SEARCH_METHOD | PropertySearchFlags::IS_GETTER;
    constexpr auto SETTER_FLAGS = PropertySearchFlags::SEARCH_METHOD | PropertySearchFlags::IS_SETTER;

    switch (memberExpr->Parent()->Type()) {
        case ir::AstNodeType::CALL_EXPRESSION: {
            if (memberExpr->Parent()->AsCallExpression()->Callee() == memberExpr) {
                return PropertySearchFlags::SEARCH_ALL;
            }
            break;
        }
        case ir::AstNodeType::OVERLOAD_DECLARATION: {
            return FUNCTIONAL_FLAGS | GETTER_FLAGS;
            break;
        }
        case ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION: {
            if (memberExpr->Parent()->AsETSNewClassInstanceExpression()->GetTypeRef() == memberExpr) {
                return PropertySearchFlags::SEARCH_DECL;
            }
            break;
        }
        case ir::AstNodeType::MEMBER_EXPRESSION: {
            return PropertySearchFlags::SEARCH_FIELD | PropertySearchFlags::SEARCH_DECL | GETTER_FLAGS;
        }
        case ir::AstNodeType::UPDATE_EXPRESSION:
        case ir::AstNodeType::UNARY_EXPRESSION:
        case ir::AstNodeType::BINARY_EXPRESSION: {
            return PropertySearchFlags::SEARCH_FIELD | GETTER_FLAGS;
        }
        case ir::AstNodeType::ASSIGNMENT_EXPRESSION: {
            const auto *const assignmentExpr = memberExpr->Parent()->AsAssignmentExpression();

            if (assignmentExpr->Left() == memberExpr) {
                if (assignmentExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
                    return PropertySearchFlags::SEARCH_FIELD | SETTER_FLAGS;
                }
                return PropertySearchFlags::SEARCH_FIELD | GETTER_FLAGS | SETTER_FLAGS;
            }

            return PropertySearchFlags::SEARCH_FIELD | GETTER_FLAGS;
        }
        default: {
            break;
        }
    }

    return PropertySearchFlags::SEARCH_FIELD | FUNCTIONAL_FLAGS | GETTER_FLAGS;
}

static bool ShouldRemoveStaticSearchFlag(const ir::MemberExpression *const memberExpr,
                                         const varbinder::Variable *targetRef)
{
    if ((targetRef != nullptr && targetRef->Declaration() != nullptr && targetRef->Declaration()->IsLetOrConstDecl()) ||
        !memberExpr->Object()->TsType()->IsETSEnumType()) {
        auto object = memberExpr->Object();
        if (object->IsMemberExpression()) {
            object = object->AsMemberExpression()->Property();
        }
        if (object->IsTypeNode()) {
            return false;
        }
        if (!object->IsIdentifier() || (object->AsIdentifier()->Variable() == nullptr) ||
            object->AsIdentifier()->Variable()->HasFlag(varbinder::VariableFlags::INITIALIZED)) {
            return true;
        }
    }
    return false;
}

PropertySearchFlags ETSChecker::GetSearchFlags(const ir::MemberExpression *const memberExpr,
                                               const varbinder::Variable *targetRef)
{
    auto searchFlag = GetInitialSearchFlags(memberExpr);
    searchFlag |= PropertySearchFlags::SEARCH_IN_BASE | PropertySearchFlags::SEARCH_IN_INTERFACES;
    if (targetRef != nullptr && targetRef->Declaration() != nullptr &&
        targetRef->Declaration()->Node()->IsClassDefinition() &&
        targetRef->Declaration()->Node()->AsClassDefinition()->IsNamespaceTransformed()) {
        return searchFlag;
    }

    if (targetRef != nullptr &&
        (targetRef->HasFlag(varbinder::VariableFlags::CLASS_OR_INTERFACE_OR_ENUM) ||
         //  NOTE (DZ):  need to investigate when and why `targetRef->TsType()->Variable()` can be `nullptr`
         //              (see ast/parser/ets/union_static_method.ets)
         (targetRef->HasFlag(varbinder::VariableFlags::TYPE_ALIAS) && targetRef->TsType()->Variable() != nullptr &&
          targetRef->TsType()->Variable()->HasFlag(varbinder::VariableFlags::CLASS_OR_INTERFACE)))) {
        searchFlag &= ~PropertySearchFlags::SEARCH_INSTANCE;
    } else if (ShouldRemoveStaticSearchFlag(memberExpr, targetRef)) {
        searchFlag &= ~PropertySearchFlags::SEARCH_STATIC;
    }
    return searchFlag;
}

const varbinder::Variable *ETSChecker::GetTargetRef(const ir::MemberExpression *const memberExpr)
{
    if (memberExpr->Object()->IsIdentifier()) {
        return memberExpr->Object()->AsIdentifier()->Variable();
    }
    if (memberExpr->Object()->IsMemberExpression()) {
        return memberExpr->Object()->AsMemberExpression()->PropVar();
    }
    if (memberExpr->Object()->IsTypeNode() && memberExpr->Object()->TsType()->IsETSObjectType()) {
        return memberExpr->Object()->TsType()->Variable();
    }
    return nullptr;
}

void ETSChecker::ValidateReadonlyProperty(const ir::MemberExpression *const memberExpr, const ETSFunctionType *propType,
                                          const lexer::SourcePosition sourcePos)
{
    ir::ClassProperty *classProp = nullptr;
    ETSObjectType *currentObj = memberExpr->ObjType();
    bool foundInThis = false;
    while (classProp == nullptr && currentObj != nullptr) {
        classProp = FindClassProperty(currentObj, propType);
        if (classProp != nullptr && currentObj == memberExpr->ObjType()) {
            foundInThis = true;
        }

        currentObj = currentObj->SuperType();
    }

    if (classProp != nullptr && this->Context().ContainingSignature() != nullptr && classProp->IsReadonly()) {
        if (!foundInThis || (!this->Context().ContainingSignature()->Function()->IsConstructor())) {
            LogError(diagnostic::ASSIGN_TO_READONLY_PROP, {}, sourcePos);
            return;
        }

        if (IsInitializedProperty(memberExpr->ObjType()->GetDeclNode()->AsClassDefinition(), classProp)) {
            LogError(diagnostic::READONLY_FIELD_MULTIPLE_INIT, {}, sourcePos);
        }
    }
}

ETSFunctionType *ETSChecker::ResolveAccessorTypeByFlag(ir::MemberExpression *const memberExpr,
                                                       ETSFunctionType *propType, ETSFunctionType *funcType,
                                                       PropertySearchFlags searchFlag)
{
    ETSFunctionType *finalRes = nullptr;
    auto const &sourcePos = memberExpr->Property()->Start();
    if ((searchFlag & PropertySearchFlags::IS_GETTER) != 0) {
        if (propType != nullptr && propType->HasTypeFlag(TypeFlag::GETTER)) {
            ValidateSignatureAccessibility(memberExpr->ObjType(), propType->FindGetter(), sourcePos);
            finalRes = propType;
        }

        if (funcType != nullptr && funcType->IsExtensionAccessorType() &&
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            FindRelativeExtensionGetter(memberExpr, funcType) != nullptr) {
            finalRes = funcType;
        }

        if (finalRes == nullptr) {
            LogError(diagnostic::READ_FROM_WRITEONLY_PROP, {}, sourcePos);
            return finalRes;
        }
    }

    if ((searchFlag & PropertySearchFlags::IS_SETTER) != 0) {
        finalRes = nullptr;
        if (propType != nullptr) {
            ValidateReadonlyProperty(memberExpr, propType, sourcePos);
            if (propType->FindSetter() != nullptr) {
                ValidateSignatureAccessibility(memberExpr->ObjType(), propType->FindSetter(), sourcePos);
                finalRes = propType;
            }
        }

        if (funcType != nullptr && funcType->IsExtensionAccessorType() &&
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            FindRelativeExtensionSetter(memberExpr, funcType) != nullptr) {
            finalRes = funcType;
        }

        if (finalRes == nullptr) {
            LogError(diagnostic::ASSIGN_TO_READONLY_PROP, {}, sourcePos);
        }
    }

    return finalRes;
}

// Note: Need to validate extension Accessor and original Accessor.
std::vector<ResolveResult *> ETSChecker::ValidateAccessor(ir::MemberExpression *const memberExpr,
                                                          varbinder::LocalVariable *const oAcc,
                                                          varbinder::Variable *const eAcc,
                                                          PropertySearchFlags searchFlag)
{
    auto *funcType = eAcc != nullptr ? eAcc->TsType()->AsETSFunctionType() : nullptr;
    auto *propType = oAcc != nullptr ? oAcc->TsType()->AsETSFunctionType() : nullptr;
    searchFlag = memberExpr->Parent()->IsUpdateExpression() ? searchFlag | PropertySearchFlags::IS_SETTER : searchFlag;
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    ETSFunctionType *finalRes = ResolveAccessorTypeByFlag(memberExpr, propType, funcType, searchFlag);
    std::vector<ResolveResult *> resolveRes = {};
    if (finalRes == nullptr) {
        return resolveRes;
    }

    if (finalRes == propType) {
        resolveRes.emplace_back(ProgramAllocator()->New<ResolveResult>(oAcc, ResolvedKind::PROPERTY));
        return resolveRes;
    }
    resolveRes.emplace_back(ProgramAllocator()->New<ResolveResult>(eAcc, ResolvedKind::EXTENSION_ACCESSOR));
    return resolveRes;
}

ir::ClassProperty *ETSChecker::FindClassProperty(const ETSObjectType *const objectType, const ETSFunctionType *propType)
{
    auto propName =
        util::UString(std::string(compiler::Signatures::PROPERTY) + propType->Name().Mutf8(), ProgramAllocator())
            .View();

    ir::ClassProperty *classProp = nullptr;
    if (objectType->GetDeclNode()->IsClassDefinition()) {
        auto body = objectType->GetDeclNode()->AsClassDefinition()->Body();
        auto foundValue = std::find_if(body.begin(), body.end(), [propName](ir::AstNode *node) {
            return node->IsClassProperty() && node->AsClassProperty()->Key()->AsIdentifier()->Name() == propName;
        });
        if (foundValue != body.end()) {
            classProp = (*foundValue)->AsClassProperty();
        }
    }

    return classProp;
}

bool ETSChecker::IsInitializedProperty(const ir::ClassDefinition *classDefinition, const ir::ClassProperty *prop)
{
    std::string targetName = prop->Key()->AsIdentifier()->Name().Mutf8();
    if (targetName.find(compiler::Signatures::PROPERTY) == 0) {
        targetName = targetName.substr(compiler::Signatures::PROPERTY.size());
    }

    for (auto *it : classDefinition->Body()) {
        if (it->IsClassProperty() && it->AsClassProperty()->Value() != nullptr) {
            return FindPropertyInAssignment(it, targetName);
        }
    }

    return false;
}

bool ETSChecker::FindPropertyInAssignment(const ir::AstNode *it, const std::string &targetName)
{
    return it->AsClassProperty()->Value()->FindChild([&targetName](ir::AstNode *node) {
        return node->IsIdentifier() && node->AsIdentifier()->Name().Is(targetName) && node->Parent() != nullptr &&
               node->Parent()->IsMemberExpression();
    }) != nullptr;
}

static bool IsExtensionAccessorCallUse(checker::ETSChecker *checker, const ir::MemberExpression *const memberExpr,
                                       ResolvedKind resolvedKind)
{
    return resolvedKind == ResolvedKind::EXTENSION_ACCESSOR &&
           !checker->HasStatus(CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK) &&
           memberExpr->Parent()->IsCallExpression() && memberExpr->Parent()->AsCallExpression()->Callee() == memberExpr;
}

static ResolvedKind DecideResolvedKind(Type *typeOfGlobalFunctionVar)
{
    ES2PANDA_ASSERT(typeOfGlobalFunctionVar != nullptr);
    ES2PANDA_ASSERT(typeOfGlobalFunctionVar->IsETSObjectType() || typeOfGlobalFunctionVar->IsETSFunctionType());
    if (typeOfGlobalFunctionVar->IsETSObjectType()) {
        return ResolvedKind::EXTENSION_FUNCTION;
    }

    if (typeOfGlobalFunctionVar->AsETSFunctionType()->IsExtensionAccessorType()) {
        return ResolvedKind::EXTENSION_ACCESSOR;
    }

    return ResolvedKind::EXTENSION_FUNCTION;
}

void ETSChecker::CheckAnnotationReference(const ir::MemberExpression *memberExpr, const varbinder::LocalVariable *prop)
{
    // Note: there might be a better way to handle annotations
    if (prop != nullptr && prop->Declaration() != nullptr && prop->Declaration()->IsAnnotationDecl() &&
        memberExpr->Parent()->IsCallExpression()) {
        LogError(diagnostic::ANNOTATION_INSTANTIATION, {prop->Declaration()->Name()}, memberExpr->Start());
    }
}

// NOLINTNEXTLINE(readability-function-size)
std::vector<ResolveResult *> ETSChecker::ResolveMemberReference(const ir::MemberExpression *const memberExpr,
                                                                const ETSObjectType *const target)
{
    std::vector<ResolveResult *> resolveRes {};

    if (target->GetDeclNode() != nullptr && target->GetDeclNode()->IsClassDefinition() &&
        !target->GetDeclNode()->AsClassDefinition()->IsClassDefinitionChecked()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        this->CheckClassDefinition(target->GetDeclNode()->AsClassDefinition());
    }
    const auto *const targetRef = GetTargetRef(memberExpr);
    auto searchFlag = GetSearchFlags(memberExpr, targetRef);
    if (target->HasObjectFlag(ETSObjectFlags::LAZY_IMPORT_OBJECT)) {
        searchFlag |= PropertySearchFlags::SEARCH_INSTANCE;
    }
    auto searchName = target->GetReExportAliasValue(memberExpr->Property()->AsIdentifier()->Name());
    auto *prop = target->GetProperty(searchName, searchFlag);

    CheckAnnotationReference(memberExpr, prop);

    varbinder::Variable *const globalFunctionVar = ResolveInstanceExtension(memberExpr);
    if (targetRef != nullptr && targetRef->HasFlag(varbinder::VariableFlags::CLASS_OR_INTERFACE)) {
        // Note: extension function only for instance.
        ValidateResolvedProperty(&prop, target, memberExpr->Property()->AsIdentifier(), searchFlag);
        if (prop != nullptr) {
            resolveRes.emplace_back(ProgramAllocator()->New<ResolveResult>(prop, ResolvedKind::PROPERTY));
        }
        return resolveRes;
    }

    if (HasStatus(CheckerStatus::IN_GETTER)) {
        WarnForEndlessLoopInGetterSetter(memberExpr);
    }

    // Note: validate originalAccessor and extensionAccessor.
    if ((IsVariableGetterSetter(prop) || IsVariableExtensionAccessor(globalFunctionVar)) &&
        ((searchFlag & PropertySearchFlags::IS_GETTER) != 0 || (searchFlag & PropertySearchFlags::IS_SETTER) != 0)) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return ValidateAccessor(const_cast<ir::MemberExpression *>(memberExpr), prop, globalFunctionVar, searchFlag);
    }

    if (globalFunctionVar != nullptr) {
        ResolvedKind resolvedKind = DecideResolvedKind(globalFunctionVar->TsType());
        if (IsExtensionAccessorCallUse(this, memberExpr, resolvedKind)) {
            LogError(diagnostic::EXTENSION_ACCESSOR_INVALID_CALL, {}, memberExpr->Start());
            return resolveRes;
        }
        resolveRes.emplace_back(ProgramAllocator()->New<ResolveResult>(globalFunctionVar, resolvedKind));
    } else {
        ValidateResolvedProperty(&prop, target, memberExpr->Property()->AsIdentifier(), searchFlag);
    }

    if (prop != nullptr) {
        resolveRes.emplace_back(ProgramAllocator()->New<ResolveResult>(prop, ResolvedKind::PROPERTY));
    }
    return resolveRes;
}

varbinder::LocalVariable *ETSChecker::ResolveOverloadReference(const ir::Identifier *ident, ETSObjectType *objType,
                                                               PropertySearchFlags searchFlags)
{
    auto *var = objType->GetProperty(ident->Name(), searchFlags);
    if (var == nullptr) {
        return nullptr;
    }
    ValidatePropertyAccess(var, objType, ident->Start());
    return var;
}

void ETSChecker::WarnForEndlessLoopInGetterSetter(const ir::MemberExpression *const memberExpr)
{
    if (!memberExpr->Object()->IsThisExpression() || memberExpr->Property() == nullptr ||
        !memberExpr->Property()->IsIdentifier()) {
        return;
    }
    auto ident = memberExpr->Property()->AsIdentifier();
    auto parent = memberExpr->Parent();
    while (parent != nullptr && (!parent->IsMethodDefinition() || parent->AsMethodDefinition()->Function() == nullptr ||
                                 (!parent->AsMethodDefinition()->Function()->IsGetter() &&
                                  !parent->AsMethodDefinition()->Function()->IsSetter()))) {
        parent = parent->Parent();
    }
    if (parent != nullptr && parent->AsMethodDefinition()->Function() != nullptr &&
        ident->Name() == parent->AsMethodDefinition()->Function()->Id()->Name()) {
        if (parent->AsMethodDefinition()->Function()->IsGetter()) {
            LogDiagnostic(diagnostic::GETTER_LOOP, memberExpr->Property()->AsIdentifier()->Start());
        } else {
            LogDiagnostic(diagnostic::SETTER_LOOP, memberExpr->Property()->AsIdentifier()->Start());
        }
    }
}

void ETSChecker::CheckValidInheritance(ETSObjectType *classType, ir::ClassDefinition *classDef)
{
    if (classType->SuperType() == nullptr) {
        return;
    }

    const auto &allProps = classType->GetAllProperties();

    for (auto *it : allProps) {
        auto *node = it->Declaration()->Node();
        if (node->IsClassProperty() && node->AsClassProperty()->TsType() != nullptr &&
            node->AsClassProperty()->TsType()->IsTypeError()) {
            continue;
        }

        const auto searchFlag = PropertySearchFlags::SEARCH_ALL | PropertySearchFlags::SEARCH_IN_BASE |
                                PropertySearchFlags::SEARCH_IN_INTERFACES |
                                PropertySearchFlags::DISALLOW_SYNTHETIC_METHOD_CREATION;
        auto *foundInSuper = classType->SuperType()->GetProperty(it->Name(), searchFlag);

        ETSObjectType *interfaceFound = nullptr;
        if (foundInSuper != nullptr) {
            CheckProperties(classType, classDef, it, foundInSuper, interfaceFound);
        }

        auto interfaceList = GetInterfaces(classType);
        varbinder::LocalVariable *foundInInterface = nullptr;
        for (auto *interface : interfaceList) {
            auto *propertyFound = interface->GetProperty(it->Name(), searchFlag);
            if (propertyFound == nullptr) {
                continue;
            }
            foundInInterface = propertyFound;
            interfaceFound = interface;
            break;
        }
        if (foundInInterface == nullptr) {
            continue;
        }

        CheckProperties(classType, classDef, it, foundInInterface, interfaceFound);
    }
}

void ETSChecker::CheckProperties(ETSObjectType *classType, ir::ClassDefinition *classDef, varbinder::LocalVariable *it,
                                 varbinder::LocalVariable *found, ETSObjectType *interfaceFound)
{
    if (found->TsType() == nullptr) {
        GetTypeOfVariable(found);
    }

    if (IsVariableStatic(it) != IsVariableStatic(found)) {
        return;
    }

    if (!IsSameDeclarationType(it, found)) {
        if (it->Declaration()->Type() == varbinder::DeclType::LET &&
            found->Declaration()->Type() == varbinder::DeclType::READONLY) {
            return;
        }

        if (CheckGetterSetterDecl(it, found)) {
            return;
        }
    } else if (CheckOverloadDecl(it, found)) {
        return;
    } else if (CheckFunctionDecl(it, found)) {
        return;
    }

    const char *targetType {};

    if (it->HasFlag(varbinder::VariableFlags::PROPERTY)) {
        targetType = "field";
    } else if (it->HasFlag(varbinder::VariableFlags::METHOD)) {
        targetType = "method";
    } else if (it->HasFlag(varbinder::VariableFlags::CLASS)) {
        targetType = "class";
    } else if (it->HasFlag(varbinder::VariableFlags::INTERFACE)) {
        targetType = "interface";
    } else if (it->HasFlag(varbinder::VariableFlags::NAMESPACE)) {
        targetType = "namespace";
    } else if (it->HasFlag(varbinder::VariableFlags::ENUM_LITERAL)) {
        targetType = "enum";
    } else if (it->HasFlag(varbinder::VariableFlags::OVERLOAD)) {
        targetType = "overload";
    } else {
        ES2PANDA_UNREACHABLE();
    }

    if (interfaceFound != nullptr) {
        LogError(diagnostic::INHERITED_INTERFACE_TYPE_MISMATCH, {interfaceFound->Name(), targetType, it->Name()},
                 interfaceFound->GetDeclNode()->Start());
        return;
    }
    auto pos = classDef->Super() == nullptr ? classDef->Ident()->Start() : classDef->Super()->Start();
    LogError(diagnostic::INHERITED_CLASS_TYPE_MISMATCH, {classType->SuperType()->Name(), targetType, it->Name()}, pos);
}

void ETSChecker::CheckReadonlyClassPropertyInImplementedInterface(ETSObjectType *classType,
                                                                  varbinder::LocalVariable *field)
{
    const auto searchFlag = PropertySearchFlags::SEARCH_ALL | PropertySearchFlags::SEARCH_IN_BASE |
                            PropertySearchFlags::SEARCH_IN_INTERFACES |
                            PropertySearchFlags::DISALLOW_SYNTHETIC_METHOD_CREATION;
    const auto &interfaceList = GetInterfaces(classType);
    if (field == nullptr || field->Declaration() == nullptr || field->Declaration()->Node() == nullptr ||
        !field->Declaration()->Node()->IsClassProperty() ||
        !field->Declaration()->Node()->AsClassProperty()->IsReadonly()) {
        return;
    }

    for (auto *interface : interfaceList) {
        ES2PANDA_ASSERT(interface != nullptr);
        auto *propertyFound = interface->GetProperty(field->Name(), searchFlag);
        if (propertyFound == nullptr) {
            continue;
        }

        ES2PANDA_ASSERT(propertyFound->TsType() != nullptr);
        if (!propertyFound->TsType()->IsETSFunctionType()) {
            continue;
        }

        auto setter = propertyFound->TsType()->AsETSFunctionType()->FindSetter();
        if (setter != nullptr) {
            LogError(diagnostic::INTERFACE_PROPERTY_REQUIRES_SETTER, {interface->Name(), field->Name()},
                     field->Declaration()->Node()->Start());
        }
    }
}

void ETSChecker::TransformProperties(ETSObjectType *classType)
{
    auto propertyList = classType->Fields();
    auto *const classDef = classType->GetDeclNode()->AsClassDefinition();

    for (auto *const field : propertyList) {
        ES2PANDA_ASSERT(field->Declaration()->Node()->IsClassProperty());
        auto *const originalProp = field->Declaration()->Node()->AsClassProperty();

        if ((originalProp->Modifiers() & ir::ModifierFlags::GETTER_SETTER) == 0U) {
            continue;
        }

        if (!field->HasFlag(varbinder::VariableFlags::PUBLIC)) {
            LogError(diagnostic::INTERFACE_PROP_NOT_PUBLIC, {}, field->Declaration()->Node()->Start());
        }
        CheckReadonlyClassPropertyInImplementedInterface(classType, field);
        classType->RemoveProperty<checker::PropertyType::INSTANCE_FIELD>(field);
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        GenerateGetterSetterPropertyAndMethod(originalProp, classType);
    }

    auto &body = classDef->Body();
    if (!std::any_of(body.cbegin(), body.cend(), [](const ir::AstNode *node) {
            return node->IsClassProperty() && (node->Modifiers() & ir::ModifierFlags::GETTER_SETTER) != 0U;
        })) {
        return;
    }
    auto it = classDef->BodyForUpdate().begin();
    while (it != classDef->BodyForUpdate().end()) {
        if ((*it)->IsClassProperty() && ((*it)->Modifiers() & ir::ModifierFlags::GETTER_SETTER) != 0U) {
            it = classDef->BodyForUpdate().erase(it);
        } else {
            ++it;
        }
    }
}

void ETSChecker::CheckGetterSetterProperties(ETSObjectType *classType)
{
    auto const checkGetterSetter = [this](varbinder::LocalVariable *var, util::StringView name) {
        auto const *type = var->TsType()->AsETSFunctionType();
        auto const *sigGetter = type->FindGetter();
        auto const *sigSetter = type->FindSetter();

        for (auto const *sig : type->CallSignatures()) {
            if (!sig->Function()->IsGetter() && !sig->Function()->IsSetter()) {
                LogError(diagnostic::METHOD_ACCESSOR_COLLISION, {name}, sig->Function()->Start());
                return;
            }
            if (sig != sigGetter && sig != sigSetter) {
                LogError(diagnostic::DUPLICATE_ACCESSOR, {}, sig->Function()->Start());
                return;
            }
        }
        if ((sigSetter != nullptr && sigGetter != nullptr) &&
            ((sigGetter->Function()->Modifiers() ^ sigSetter->Function()->Modifiers()) &
             ir::ModifierFlags::ACCESSOR_MODIFIERS) != 0) {
            LogError(diagnostic::ACCESSORS_MOD_MISMATCH, {}, sigGetter->Function()->Start());
        }
    };

    for (const auto &[name, var] : classType->InstanceMethods()) {
        if (IsVariableGetterSetter(var)) {
            checkGetterSetter(var, name);
        }
    }

    for (const auto &[name, var] : classType->StaticMethods()) {
        if (IsVariableGetterSetter(var)) {
            checkGetterSetter(var, name);
        }
    }
}

void ETSChecker::AddElementsToModuleObject(ETSObjectType *moduleObj, const util::StringView &str)
{
    for (const auto &[name, var] : VarBinder()->GetScope()->Bindings()) {
        if (name.Is(str.Mutf8()) || util::Helpers::IsGlobalVar(var)) {
            continue;
        }

        if (var->HasFlag(varbinder::VariableFlags::METHOD)) {
            moduleObj->AddProperty<checker::PropertyType::STATIC_METHOD>(var->AsLocalVariable());
        } else if (var->HasFlag(varbinder::VariableFlags::PROPERTY)) {
            moduleObj->AddProperty<checker::PropertyType::STATIC_FIELD>(var->AsLocalVariable());
        } else {
            moduleObj->AddProperty<checker::PropertyType::STATIC_DECL>(var->AsLocalVariable());
        }
    }
}

static Type *GetApparentTypeUtilityTypes(checker::ETSChecker *checker, Type *type)
{
    if (type->IsETSReadonlyType()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return checker->GetApparentType(type->AsETSReadonlyType()->GetUnderlying()->GetConstraintType());
    }
    if (type->IsETSAwaitedType()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return checker->GetApparentType(type->AsETSAwaitedType()->GetUnderlying()->GetConstraintType());
    }
    if (type->IsETSPartialTypeParameter()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return checker->CreatePartialType(
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            checker->GetApparentType(type->AsETSPartialTypeParameter()->GetUnderlying()->GetConstraintType()));
    }
    if (type->IsETSNonNullishType()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return checker->GetApparentType(type->AsETSNonNullishType()->GetUnderlying()->GetConstraintType());
    }
    return type;
}

// This function computes effective runtime view of type
Type *ETSChecker::GetApparentType(Type *type)
{
    auto currChecker = compiler::GetPhaseManager()->Context()->GetChecker()->AsETSChecker();
    auto &apparentTypes = currChecker->apparentTypes_;

    if (auto it = apparentTypes.find(type); LIKELY(it != apparentTypes.end())) {
        return it->second;
    }

    auto cached = [&apparentTypes, type](Type *res) {
        if (type != res) {
            apparentTypes.insert({type, res});
        }
        return apparentTypes.insert({res, res}).first->second;
    };

    ES2PANDA_ASSERT(type != nullptr);
    if (type->IsGradualType()) {
        return cached(type->AsGradualType()->GetBaseType());
    }

    if (type->IsETSTypeParameter()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return cached(GetApparentType(type->AsETSTypeParameter()->GetConstraintType()));
    }

    if (type->IsETSTypeAliasType()) {
        //  For the recursive type aliases its target type is not ready at the moment of union type creation
        //  (and assemblyLUB type constructing).
        if (auto *targetType = type->AsETSTypeAliasType()->GetTargetType(); targetType != nullptr) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            return cached(GetApparentType(targetType));
        }
        return type;
    }

    if (type->IsETSNonNullishType()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return cached(
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            GetNonNullishType(GetApparentType(type->AsETSNonNullishType()->GetUnderlying()->GetConstraintType())));
    }
    if (type->IsETSArrayType()) {
        return cached(type);
    }
    if (type->IsETSStringType()) {
        return GlobalBuiltinETSStringType();
    }
    if (type->IsETSUnionType()) {
        bool differ = false;
        ArenaVector<checker::Type *> newConstituent(ProgramAllocator()->Adapter());
        for (auto const &ct : type->AsETSUnionType()->ConstituentTypes()) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            newConstituent.push_back(GetApparentType(ct));
            differ |= (newConstituent.back() != ct);
        }
        return cached(differ ? CreateETSUnionType(std::move(newConstituent)) : type);
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return cached(GetApparentTypeUtilityTypes(this, type));
}

Type const *ETSChecker::GetApparentType(Type const *type) const
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return const_cast<Type const *>(const_cast<ETSChecker *>(this)->GetApparentType(const_cast<Type *>(type)));
}

ETSObjectType *ETSChecker::GetClosestCommonAncestor(ETSObjectType *source, ETSObjectType *target)
{
    if (source->AsETSObjectType()->GetDeclNode() == target->AsETSObjectType()->GetDeclNode()) {
        return source;
    }
    if (target->SuperType() == nullptr) {
        return GlobalETSObjectType();
    }

    auto *targetBase = GetOriginalBaseType(target->SuperType());
    auto *targetType = targetBase == nullptr ? target->SuperType() : targetBase;

    auto *sourceBase = GetOriginalBaseType(source);
    auto *sourceType = sourceBase == nullptr ? source : sourceBase;

    if (Relation()->IsSupertypeOf(targetType, sourceType)) {
        // NOTE: TorokG. Extending the search to find intersection types
        return targetType;
    }

    return GetClosestCommonAncestor(sourceType, targetType);
}

void ETSChecker::CheckInvokeMethodsLegitimacy(ETSObjectType *const classType)
{
    if (classType->HasObjectFlag(ETSObjectFlags::CHECKED_INVOKE_LEGITIMACY)) {
        return;
    }

    auto searchFlag = PropertySearchFlags::SEARCH_IN_INTERFACES | PropertySearchFlags::SEARCH_IN_BASE |
                      PropertySearchFlags::SEARCH_STATIC_METHOD;

    auto *const invokeMethod = classType->GetProperty(compiler::Signatures::STATIC_INVOKE_METHOD, searchFlag);
    if (invokeMethod == nullptr) {
        classType->AddObjectFlag(ETSObjectFlags::CHECKED_INVOKE_LEGITIMACY);
        return;
    }

    auto *const instantiateMethod = classType->GetProperty(compiler::Signatures::STATIC_INSTANTIATE_METHOD, searchFlag);
    if (instantiateMethod != nullptr) {
        LogError(diagnostic::STATIC_METH_IN_CLASS_AND_INTERFACE,
                 {compiler::Signatures::STATIC_INVOKE_METHOD, compiler::Signatures::STATIC_INSTANTIATE_METHOD,
                  classType->Name()},
                 classType->GetDeclNode()->Start());
    }
    classType->AddObjectFlag(ETSObjectFlags::CHECKED_INVOKE_LEGITIMACY);
}

}  // namespace ark::es2panda::checker
