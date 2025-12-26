/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "interfacePropertyDeclarations.h"

#include "checker/ETSchecker.h"
#include "checker/types/type.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/util.h"
#include "ir/astNode.h"
#include "ir/astNodeFlags.h"
#include "ir/expression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/literals/undefinedLiteral.h"
#include "ir/opaqueTypeNode.h"
#include "ir/statements/blockStatement.h"
#include "ir/ts/tsInterfaceBody.h"
#include "ir/base/classProperty.h"
#include "ir/ets/etsUnionType.h"
#include "ir/ets/etsNullishTypes.h"
#include "ir/visitor/AstVisitor.h"

namespace ark::es2panda::compiler {

void InterfacePropertyDeclarationsPhase::TransformOptionalFieldTypeAnnotation(ir::ClassProperty *const field,
                                                                              bool isInterface)
{
    if (!field->IsOptionalDeclaration()) {
        return;
    }

    if (field->IsETSUnionType()) {
        bool alreadyHasUndefined = false;
        auto unionTypes = field->AsETSUnionType()->Types();
        for (const auto &type : unionTypes) {
            if (type->IsETSUndefinedType()) {
                alreadyHasUndefined = true;
                break;
            }
        }
        if (!alreadyHasUndefined) {
            ArenaVector<ir::TypeNode *> types(field->AsETSUnionType()->Types(), Context()->Allocator()->Adapter());
            types.push_back(Context()->AllocNode<ir::ETSUndefinedType>(Context()->Allocator()));
            auto *const unionType = Context()->AllocNode<ir::ETSUnionType>(std::move(types), Context()->Allocator());
            field->SetTypeAnnotation(unionType);
        }
    } else {
        ArenaVector<ir::TypeNode *> types(Context()->Allocator()->Adapter());
        types.push_back(field->TypeAnnotation());
        types.push_back(Context()->AllocNode<ir::ETSUndefinedType>(Context()->Allocator()));
        auto *const unionType = Context()->AllocNode<ir::ETSUnionType>(std::move(types), Context()->Allocator());
        field->SetTypeAnnotation(unionType);
        unionType->SetParent(field);
    }
    field->ClearModifier(ir::ModifierFlags::OPTIONAL);

    if (isInterface) {
        GetPropCollector().InsertInterfaceProperty(field->Key()->ToString());
    }
}

ir::FunctionSignature InterfacePropertyDeclarationsPhase::GenerateGetterOrSetterSignature(
    ir::ClassProperty *const field, bool isSetter, varbinder::FunctionParamScope *paramScope)
{
    TransformOptionalFieldTypeAnnotation(field, true);
    ArenaVector<ir::Expression *> params(Context()->Allocator()->Adapter());
    auto *varbinder = Context()->parserProgram->VarBinder()->AsETSBinder();

    if (isSetter) {
        auto paramIdent = field->Key()->AsIdentifier()->Clone(Context()->Allocator(), nullptr);
        ES2PANDA_ASSERT(paramIdent != nullptr);
        paramIdent->SetTsTypeAnnotation(field->TypeAnnotation()->Clone(Context()->Allocator(), nullptr));
        paramIdent->TypeAnnotation()->SetParent(paramIdent);

        ClearTypesVariablesAndScopes(paramIdent);
        auto classCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, paramScope);
        InitScopesPhaseETS::RunExternalNode(paramIdent, varbinder);

        auto *const paramExpression =
            Context()->AllocNode<ir::ETSParameterExpression>(paramIdent, false, Context()->Allocator());
        ES2PANDA_ASSERT(paramExpression != nullptr);
        paramExpression->SetRange(paramIdent->Range());
        auto [paramVar, node] = paramScope->AddParamDecl(Context()->Allocator(), varbinder, paramExpression);
        if (node != nullptr) {
            varbinder->ThrowRedeclaration(node->Start(), paramVar->Name(), paramVar->Declaration()->Type());
        }

        paramIdent->SetVariable(paramVar);
        paramExpression->SetVariable(paramVar);

        params.push_back(paramExpression);
    }

    return ir::FunctionSignature(nullptr, std::move(params), isSetter ? nullptr : field->TypeAnnotation());
}

static ir::ModifierFlags GetFlags(bool isOptional, bool isDeclare)
{
    auto flags = ir::ModifierFlags::PUBLIC;

    if (!isOptional || isDeclare) {
        flags |= ir::ModifierFlags::ABSTRACT;
    }
    if (isOptional) {
        flags |= ir::ModifierFlags::OPTIONAL;
    }
    if (isDeclare && isOptional) {
        flags |= ir::ModifierFlags::DEFAULT;
    }
    return flags;
}

ir::MethodDefinition *InterfacePropertyDeclarationsPhase::GetMethodDefinition(bool isSetter, ir::ModifierFlags flags,
                                                                              ir::Identifier *methodIdent,
                                                                              ir::FunctionExpression *funcExpr,
                                                                              ir::ClassProperty *const field)
{
    auto *method = Context()->AllocNode<ir::MethodDefinition>(
        isSetter ? ir::MethodDefinitionKind::SET : ir::MethodDefinitionKind::GET, methodIdent, funcExpr, flags,
        Context()->Allocator(), false);

    method->Id()->SetMutator();
    method->SetRange(field->Range());
    method->SetParent(field->Parent());
    method->Function()->SetIdent(method->Id()->Clone(Context()->Allocator(), nullptr));
    method->Function()->AddModifier(method->Modifiers());
    return method;
}

static ir::ScriptFunctionFlags GetScriptFunctionFlags(bool isSetter)
{
    auto flags = ir::ScriptFunctionFlags::INTERFACE_PROPERTY | ir::ScriptFunctionFlags::METHOD;
    if (isSetter) {
        flags |= ir::ScriptFunctionFlags::SETTER;
    } else {
        flags |= ir::ScriptFunctionFlags::GETTER;
    }
    return flags;
}

ir::MethodDefinition *InterfacePropertyDeclarationsPhase::GenerateGetterOrSetter(ir::ClassProperty *const field,
                                                                                 bool isSetter, bool isOptional,
                                                                                 bool isDeclare)
{
    auto ctx = Context();
    auto classScope = NearestScope(field);
    auto *varbinder = ctx->parserProgram->VarBinder()->AsETSBinder();
    auto *paramScope = ctx->Allocator()->New<varbinder::FunctionParamScope>(ctx->Allocator(), classScope);
    auto *functionScope = ctx->Allocator()->New<varbinder::FunctionScope>(ctx->Allocator(), paramScope);
    ES2PANDA_ASSERT(functionScope != nullptr);

    functionScope->BindParamScope(paramScope);
    paramScope->BindFunctionScope(functionScope);

    auto flags = GetFlags(isOptional, isDeclare);

    ir::FunctionSignature signature = GenerateGetterOrSetterSignature(field, isSetter, paramScope);

    auto scriptFunctionFlags = GetScriptFunctionFlags(isSetter);
    auto *func = ctx->AllocNode<ir::ScriptFunction>(
        ctx->Allocator(),
        ir::ScriptFunction::ScriptFunctionData {
            ctx->GetChecker()->AsETSChecker()->CreateGetterOrSetterBodyForOptional(isSetter, isOptional && !isDeclare),
            std::move(signature), scriptFunctionFlags, flags,
            classScope->Node()->AsTSInterfaceDeclaration()->Language()});

    func->SetRange(field->Range());

    // Since optional prop has default body, need to set scope.
    if (isOptional && !isDeclare) {
        auto funcCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, classScope);
        InitScopesPhaseETS::RunExternalNode(func, varbinder);
    } else {
        func->SetScope(functionScope);
    }
    paramScope->BindNode(func);
    functionScope->BindNode(func);
    auto *funcExpr = Context()->AllocNode<ir::FunctionExpression>(func);
    SetSourceRangesRecursively(funcExpr, func->Range());

    auto const &name = field->Key()->AsIdentifier()->Name();
    auto methodIdent = Context()->AllocNode<ir::Identifier>(name, Context()->Allocator());
    auto *decl = Context()->Allocator()->New<varbinder::VarDecl>(name);
    auto var = functionScope->AddDecl(Context()->Allocator(), decl, ScriptExtension::ETS);
    ES2PANDA_ASSERT(var != nullptr);
    methodIdent->SetVariable(var);
    methodIdent->SetRange(field->Key()->Range());

    auto method = GetMethodDefinition(isSetter, flags, methodIdent, funcExpr, field);
    decl->BindNode(method);

    if (field->HasAnnotations()) {
        ArenaVector<ir::AnnotationUsage *> functionAnnotations(Context()->Allocator()->Adapter());
        for (auto *annotationUsage : field->Annotations()) {
            auto annoClone = annotationUsage->Clone(Context()->Allocator(), method)->AsAnnotationUsage();
            InitScopesPhaseETS::RunExternalNode(annoClone, varbinder);
            functionAnnotations.emplace_back(annoClone);
        }
        method->Function()->SetAnnotations(std::move(functionAnnotations));
    }

    return method;
}

void InterfacePropertyDeclarationsPhase::CollectPropertiesAndSuperInterfaces(ir::TSInterfaceBody *const interface)
{
    ES2PANDA_ASSERT(interface->Parent()->IsTSInterfaceDeclaration());
    auto *interfaceDecl = interface->Parent()->AsTSInterfaceDeclaration();
    GetPropCollector().SetInterfaceId(interfaceDecl->Id()->ToString());
    GetPropCollector().InitInterfacePropertyMap();
    for (const auto &superInterface : interfaceDecl->Extends()) {
        std::string superId = superInterface->Expr()->AsETSTypeReference()->Part()->Name()->ToString();
        if (!GetPropCollector().IsParentExists(GetPropCollector().GetInterfaceId())) {
            GetPropCollector().InitInterfaceParentMap();
        }
        GetPropCollector().InsertInterfaceParent(superId);
    }
}

void InterfacePropertyDeclarationsPhase::HandleInternalGetterOrSetterMethod(ir::AstNode *const ast)
{
    if (!ast->IsMethodDefinition()) {
        return;
    }
    auto *method = ast->AsMethodDefinition();
    if (method->Kind() == ir::MethodDefinitionKind::GET || method->Kind() == ir::MethodDefinitionKind::SET) {
        GetPropCollector().InsertInterfaceProperty(method->Key()->ToString());
    }
}

//  Extracted form 'UpdateInterfaceProperties(...)' to reduce its size.
static void AddOverload(ir::MethodDefinition *method, ir::MethodDefinition *overload, varbinder::Variable *variable)
{
    method->AddOverload(overload);
    overload->SetParent(method);
    ES2PANDA_ASSERT(overload->Function());
    overload->Function()->AddFlag(ir::ScriptFunctionFlags::OVERLOAD);
    overload->Function()->Id()->SetVariable(variable);
}

static void UpdateNewInterface(ir::TSInterfaceBody *newInterface, ir::TSInterfaceBody *const interface)
{
    ES2PANDA_ASSERT(newInterface != nullptr);
    newInterface->SetRange(interface->Range());
    newInterface->SetParent(interface->Parent());
    for (const auto &child : newInterface->Body()) {
        child->SetParent(newInterface);
    }
}

ir::Expression *InterfacePropertyDeclarationsPhase::UpdateInterfaceProperties(ir::TSInterfaceBody *const interface)
{
    if (interface->Body().empty()) {
        return interface;
    }

    CollectPropertiesAndSuperInterfaces(interface);

    ArenaVector<ir::AstNode *> newPropertyList(Context()->Allocator()->Adapter());

    auto scope = NearestScope(interface);
    ES2PANDA_ASSERT(scope->IsClassScope());

    for (const auto &prop : interface->Body()) {
        if (!prop->IsClassProperty()) {
            newPropertyList.emplace_back(prop);
            HandleInternalGetterOrSetterMethod(prop);
            continue;
        }
        auto *originProp = prop->Clone(Context()->allocator, nullptr);
        bool isOptional = prop->AsClassProperty()->IsOptionalDeclaration();
        bool isDeclare = interface->Parent()->IsDeclare();
        ir::MethodDefinition *getter = GenerateGetterOrSetter(prop->AsClassProperty(), false, isOptional, isDeclare);
        getter->SetOriginalNode(originProp);

        auto methodScope = scope->AsClassScope()->InstanceMethodScope();
        auto name = getter->Key()->AsIdentifier()->Name();

        auto *decl = Context()->Allocator()->New<varbinder::FunctionDecl>(Context()->Allocator(), name, getter);
        auto *variable = methodScope->AddDecl(Context()->Allocator(), decl, ScriptExtension::ETS);

        if (variable == nullptr) {
            auto prevDecl = methodScope->FindDecl(name);
            ES2PANDA_ASSERT(prevDecl->IsFunctionDecl());

            auto *const method = prevDecl->Node()->AsMethodDefinition();
            auto *const var = methodScope->FindLocal(name, varbinder::ResolveBindingOptions::BINDINGS);

            AddOverload(method, getter, var);

            if (!prop->AsClassProperty()->IsReadonly()) {
                auto setter = GenerateGetterOrSetter(prop->AsClassProperty(), true, isOptional, isDeclare);
                AddOverload(method, setter, var);
            }
            continue;
        }

        getter->Function()->Id()->SetVariable(variable);
        newPropertyList.emplace_back(getter);

        if (!prop->AsClassProperty()->IsReadonly()) {
            auto setter = GenerateGetterOrSetter(prop->AsClassProperty(), true, isOptional, isDeclare);
            AddOverload(getter, setter, variable);
        }
        scope->AsClassScope()->InstanceFieldScope()->EraseBinding(name);
    }

    auto newInterface = Context()->AllocNode<ir::TSInterfaceBody>(std::move(newPropertyList));
    UpdateNewInterface(newInterface, interface);
    return newInterface;
}

void InterfacePropertyDeclarationsPhase::CollectSuperInterfaceProperties(InterfacePropertyType &implInterfaceProperties,
                                                                         const std::string &interId)
{
    if (GetPropCollector().IsVisitedInterface(interId)) {
        return;
    }

    if (GetPropCollector().IsInterfaceHasProperty(interId)) {
        InterfacePropertyType &properties = GetPropCollector().GetInterfaceProperty(interId);
        implInterfaceProperties.insert(properties.begin(), properties.end());
    }
    if (GetPropCollector().IsParentExists(interId)) {
        for (auto &superId : GetPropCollector().GetInterfaceParent(interId)) {
            CollectSuperInterfaceProperties(implInterfaceProperties, superId);
        }
    }
}

void InterfacePropertyDeclarationsPhase::UpdateClassProperties(ir::ClassDefinition *const klass)
{
    if (klass->Body().empty()) {
        return;
    }

    InterfacePropertyType implInterfaceProperties = {};

    GetPropCollector().InitVisitedInterfaces();
    for (const auto &implement : klass->Implements()) {
        std::string interId = implement->Expr()->IsOpaqueTypeNode()
                                  ? implement->Expr()->TsType()->AsETSObjectType()->Name().Mutf8()
                                  : implement->Expr()->AsETSTypeReference()->Part()->Name()->ToString();
        CollectSuperInterfaceProperties(implInterfaceProperties, interId);
    }

    for (auto *elem : klass->Body()) {
        if (elem->IsClassProperty() &&
            (implInterfaceProperties.count(elem->AsClassProperty()->Key()->ToString()) != 0U)) {
            TransformOptionalFieldTypeAnnotation(elem->AsClassProperty());
        }
    }
}

bool InterfacePropertyDeclarationsPhase::PerformForProgram(parser::Program *program)
{
    ir::NodeTransformer handleInterfacePropertyDecl = [this](ir::AstNode *const ast) {
        return ast->IsTSInterfaceBody() ? UpdateInterfaceProperties(ast->AsTSInterfaceBody()) : ast;
    };

    ir::NodeTransformer handleClassPropertyDecl = [this](ir::AstNode *const ast) {
        if (ast->IsClassDefinition() && !ast->AsClassDefinition()->Implements().empty()) {
            UpdateClassProperties(ast->AsClassDefinition());
        }
        return ast;
    };

    program->Ast()->TransformChildrenRecursively(
        [handleClassPropertyDecl, handleInterfacePropertyDecl](ir::AstNode *const ast) {
            return handleClassPropertyDecl(handleInterfacePropertyDecl(ast));
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
