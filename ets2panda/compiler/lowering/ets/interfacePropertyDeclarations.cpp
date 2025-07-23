/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "ir/expression.h"
#include "ir/expressions/identifier.h"
#include "ir/opaqueTypeNode.h"
#include "ir/statements/blockStatement.h"
#include "ir/ts/tsInterfaceBody.h"
#include "ir/base/classProperty.h"
#include "ir/ets/etsUnionType.h"
#include "ir/ets/etsNullishTypes.h"

namespace ark::es2panda::compiler {

void InterfacePropertyDeclarationsPhase::TransformOptionalFieldTypeAnnotation(public_lib::Context *ctx,
                                                                              ir::ClassProperty *const field,
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
            ArenaVector<ir::TypeNode *> types(field->AsETSUnionType()->Types(), ctx->Allocator()->Adapter());
            types.push_back(ctx->AllocNode<ir::ETSUndefinedType>(ctx->Allocator()));
            auto *const unionType = ctx->AllocNode<ir::ETSUnionType>(std::move(types), ctx->Allocator());
            field->SetTypeAnnotation(unionType);
        }
    } else {
        ArenaVector<ir::TypeNode *> types(ctx->Allocator()->Adapter());
        types.push_back(field->TypeAnnotation());
        types.push_back(ctx->AllocNode<ir::ETSUndefinedType>(ctx->Allocator()));
        auto *const unionType = ctx->AllocNode<ir::ETSUnionType>(std::move(types), ctx->Allocator());
        field->SetTypeAnnotation(unionType);
        unionType->SetParent(field);
    }
    field->ClearModifier(ir::ModifierFlags::OPTIONAL);

    if (isInterface) {
        GetPropCollector().InsertInterfaceProperty(field->Key()->ToString());
    }
}

ir::FunctionSignature InterfacePropertyDeclarationsPhase::GenerateGetterOrSetterSignature(
    public_lib::Context *ctx, varbinder::ETSBinder *varbinder, ir::ClassProperty *const field, bool isSetter,
    varbinder::FunctionParamScope *paramScope)
{
    TransformOptionalFieldTypeAnnotation(ctx, field, true);
    ArenaVector<ir::Expression *> params(ctx->Allocator()->Adapter());

    if (isSetter) {
        auto paramIdent = field->Key()->AsIdentifier()->Clone(ctx->Allocator(), nullptr);
        ES2PANDA_ASSERT(paramIdent != nullptr);
        paramIdent->SetTsTypeAnnotation(field->TypeAnnotation()->Clone(ctx->Allocator(), nullptr));
        paramIdent->TypeAnnotation()->SetParent(paramIdent);

        ClearTypesVariablesAndScopes(paramIdent);
        auto classCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, paramScope);
        InitScopesPhaseETS::RunExternalNode(paramIdent, varbinder);

        auto *const paramExpression = ctx->AllocNode<ir::ETSParameterExpression>(paramIdent, false, ctx->Allocator());
        ES2PANDA_ASSERT(paramExpression != nullptr);
        paramExpression->SetRange(paramIdent->Range());
        auto [paramVar, node] = paramScope->AddParamDecl(ctx->Allocator(), varbinder, paramExpression);
        if (node != nullptr) {
            varbinder->ThrowRedeclaration(node->Start(), paramVar->Name(), paramVar->Declaration()->Type());
        }

        paramIdent->SetVariable(paramVar);
        paramExpression->SetVariable(paramVar);

        params.push_back(paramExpression);
    }

    return ir::FunctionSignature(nullptr, std::move(params), isSetter ? nullptr : field->TypeAnnotation());
}

ir::MethodDefinition *InterfacePropertyDeclarationsPhase::GenerateGetterOrSetter(public_lib::Context *ctx,
                                                                                 varbinder::ETSBinder *varbinder,
                                                                                 ir::ClassProperty *const field,
                                                                                 bool isSetter)
{
    auto classScope = NearestScope(field);
    auto *paramScope = ctx->Allocator()->New<varbinder::FunctionParamScope>(ctx->Allocator(), classScope);
    auto *functionScope = ctx->Allocator()->New<varbinder::FunctionScope>(ctx->Allocator(), paramScope);
    ES2PANDA_ASSERT(functionScope != nullptr);

    functionScope->BindParamScope(paramScope);
    paramScope->BindFunctionScope(functionScope);

    auto flags = ir::ModifierFlags::PUBLIC;
    flags |= ir::ModifierFlags::ABSTRACT;

    ir::FunctionSignature signature = GenerateGetterOrSetterSignature(ctx, varbinder, field, isSetter, paramScope);

    auto *func = ctx->AllocNode<ir::ScriptFunction>(
        ctx->Allocator(), ir::ScriptFunction::ScriptFunctionData {
                              nullptr, std::move(signature),  // CC-OFF(G.FMT.02) project code style
                              // CC-OFFNXT(G.FMT.02) project code style
                              isSetter ? ir::ScriptFunctionFlags::SETTER : ir::ScriptFunctionFlags::GETTER, flags,
                              classScope->Node()->AsTSInterfaceDeclaration()->Language()});

    func->SetRange(field->Range());
    func->SetScope(functionScope);

    auto const &name = field->Key()->AsIdentifier()->Name();
    auto methodIdent = ctx->AllocNode<ir::Identifier>(name, ctx->Allocator());

    auto *decl = ctx->Allocator()->New<varbinder::VarDecl>(name);
    auto var = functionScope->AddDecl(ctx->Allocator(), decl, ScriptExtension::ETS);
    ES2PANDA_ASSERT(var != nullptr);
    methodIdent->SetVariable(var);

    auto *funcExpr = ctx->AllocNode<ir::FunctionExpression>(func);
    funcExpr->SetRange(func->Range());
    func->AddFlag(ir::ScriptFunctionFlags::METHOD);

    auto *method =
        ctx->AllocNode<ir::MethodDefinition>(isSetter ? ir::MethodDefinitionKind::SET : ir::MethodDefinitionKind::GET,
                                             methodIdent, funcExpr, flags, ctx->Allocator(), false);

    method->Id()->SetMutator();
    method->SetRange(field->Range());
    method->Function()->SetIdent(method->Id()->Clone(ctx->Allocator(), nullptr));
    method->Function()->AddModifier(method->Modifiers());
    paramScope->BindNode(func);
    functionScope->BindNode(func);

    if (field->HasAnnotations()) {
        ArenaVector<ir::AnnotationUsage *> functionAnnotations(ctx->Allocator()->Adapter());
        for (auto *annotationUsage : field->Annotations()) {
            auto annoClone = annotationUsage->Clone(ctx->Allocator(), method)->AsAnnotationUsage();
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

ir::Expression *InterfacePropertyDeclarationsPhase::UpdateInterfaceProperties(public_lib::Context *ctx,
                                                                              varbinder::ETSBinder *varbinder,
                                                                              ir::TSInterfaceBody *const interface)
{
    if (interface->Body().empty()) {
        return interface;
    }

    CollectPropertiesAndSuperInterfaces(interface);

    auto propertyList = interface->Body();
    ArenaVector<ir::AstNode *> newPropertyList(ctx->Allocator()->Adapter());

    auto scope = NearestScope(interface);
    ES2PANDA_ASSERT(scope->IsClassScope());

    for (const auto &prop : propertyList) {
        if (!prop->IsClassProperty()) {
            newPropertyList.emplace_back(prop);
            HandleInternalGetterOrSetterMethod(prop);
            continue;
        }
        auto *originProp = prop->Clone(ctx->allocator, nullptr);
        auto getter = GenerateGetterOrSetter(ctx, varbinder, prop->AsClassProperty(), false);
        getter->SetOriginalNode(originProp);

        auto methodScope = scope->AsClassScope()->InstanceMethodScope();
        auto name = getter->Key()->AsIdentifier()->Name();

        auto *decl = ctx->Allocator()->New<varbinder::FunctionDecl>(ctx->Allocator(), name, getter);
        auto *variable = methodScope->AddDecl(ctx->Allocator(), decl, ScriptExtension::ETS);

        if (variable == nullptr) {
            auto prevDecl = methodScope->FindDecl(name);
            ES2PANDA_ASSERT(prevDecl->IsFunctionDecl());

            auto *const method = prevDecl->Node()->AsMethodDefinition();
            auto *const var = methodScope->FindLocal(name, varbinder::ResolveBindingOptions::BINDINGS);

            AddOverload(method, getter, var);

            if (!prop->AsClassProperty()->IsReadonly()) {
                auto setter = GenerateGetterOrSetter(ctx, varbinder, prop->AsClassProperty(), true);
                AddOverload(method, setter, var);
            }
            continue;
        }

        getter->Function()->Id()->SetVariable(variable);
        newPropertyList.emplace_back(getter);

        if (!prop->AsClassProperty()->IsReadonly()) {
            auto setter = GenerateGetterOrSetter(ctx, varbinder, prop->AsClassProperty(), true);
            AddOverload(getter, setter, variable);
        }
        scope->AsClassScope()->InstanceFieldScope()->EraseBinding(name);
    }

    auto newInterface = ctx->AllocNode<ir::TSInterfaceBody>(std::move(newPropertyList));
    ES2PANDA_ASSERT(newInterface != nullptr);
    newInterface->SetRange(interface->Range());
    newInterface->SetParent(interface->Parent());

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

void InterfacePropertyDeclarationsPhase::UpdateClassProperties(public_lib::Context *ctx,
                                                               ir::ClassDefinition *const klass)
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
            TransformOptionalFieldTypeAnnotation(ctx, elem->AsClassProperty());
        }
    }
}

bool InterfacePropertyDeclarationsPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    varbinder::ETSBinder *const varbinder = ctx->parserProgram->VarBinder()->AsETSBinder();

    ir::NodeTransformer handleInterfacePropertyDecl = [this, ctx, varbinder](ir::AstNode *const ast) {
        return ast->IsTSInterfaceBody() ? UpdateInterfaceProperties(ctx, varbinder, ast->AsTSInterfaceBody()) : ast;
    };

    ir::NodeTransformer handleClassPropertyDecl = [this, ctx](ir::AstNode *const ast) {
        if (ast->IsClassDefinition() && !ast->AsClassDefinition()->Implements().empty()) {
            UpdateClassProperties(ctx, ast->AsClassDefinition());
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
