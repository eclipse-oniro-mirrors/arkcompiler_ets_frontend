/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "interfaceObjectLiteralLowering.h"
#include "checker/ETSchecker.h"
#include "checker/ets/typeRelationContext.h"
#include "compiler/lowering/util.h"
#include "ir/expressions/assignmentExpression.h"
#include "util/helpers.h"

namespace ark::es2panda::compiler {

using ReadonlyFieldHolder =
    std::tuple<util::UString, util::StringView, checker::Type *>;  // anonClassFieldName, paramName, fieldType

std::string_view InterfaceObjectLiteralLowering::Name() const
{
    return "InterfaceObjectLiteralLowering";
}

static inline bool IsInterfaceType(const checker::Type *type)
{
    return type != nullptr && type->IsETSObjectType() &&
           type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::INTERFACE) &&
           !type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::DYNAMIC);
}

static ir::AstNode *CreateAnonClassImplCtor(public_lib::Context *ctx, ArenaVector<ReadonlyFieldHolder> &readonlyFields)
{
    auto *const checker = ctx->checker->AsETSChecker();
    auto *const parser = ctx->parser->AsETSParser();
    checker::ETSChecker::ClassInitializerBuilder initBuilder =
        [checker, parser, readonlyFields](ArenaVector<ir::Statement *> *statements,
                                          ArenaVector<ir::Expression *> *params) {
            for (auto [anonClassFieldName, paramName, retType] : readonlyFields) {
                ir::ETSParameterExpression *param =
                    checker->AddParam(paramName, checker->AllocNode<ir::OpaqueTypeNode>(retType));
                params->push_back(param);
                auto *paramIdent = checker->AllocNode<ir::Identifier>(paramName, checker->Allocator());
                statements->push_back(
                    parser->CreateFormattedStatement("this.@@I1 = @@I2;", anonClassFieldName, paramIdent));
            }
            checker->AddParam(varbinder::VarBinder::MANDATORY_PARAM_THIS, nullptr);
        };

    return checker->CreateClassInstanceInitializer(initBuilder);
}

static ir::ClassProperty *CreateAnonClassField(public_lib::Context *ctx, ir::MethodDefinition *ifaceMethod,
                                               util::UString anonClassFieldName)
{
    auto *const parser = ctx->parser->AsETSParser();
    // Field type annotation
    auto *fieldType = ifaceMethod->Function()->Signature()->ReturnType();

    std::stringstream sourceCode;
    // Field modifiers flags
    sourceCode << "private ";
    // No overloads means no setter function with the same name, so the field is readonly
    if (ifaceMethod->Overloads().empty()) {
        sourceCode << "readonly ";
    }
    sourceCode << "@@I1 : @@T2;" << std::endl;

    auto field = parser->CreateFormattedClassFieldDefinition(sourceCode.str(), anonClassFieldName, fieldType);
    field->SetRange(ifaceMethod->Range());

    return field->AsClassProperty();
}

static ir::MethodDefinition *CreateAnonClassFieldGetterSetter(public_lib::Context *ctx,
                                                              ir::MethodDefinition *ifaceMethod, bool isSetter,
                                                              util::UString anonClassFieldName)
{
    auto *const parser = ctx->parser->AsETSParser();
    // Field type annotation
    auto *fieldType = ifaceMethod->Function()->Signature()->ReturnType();
    ASSERT(fieldType != nullptr);

    std::stringstream sourceCode;

    if (isSetter) {
        // Setter body: this.<fieldName> = <callParam>;
        sourceCode << "public set @@I1 (anonParam:@@T2){" << std::endl;
        sourceCode << "this.@@I3 = anonParam" << std::endl;
        sourceCode << "}" << std::endl;

        return parser
            ->CreateFormattedClassMethodDefinition(sourceCode.str(), ifaceMethod->Id()->Name(), fieldType,
                                                   anonClassFieldName)
            ->AsMethodDefinition();
    }

    // Getter body: return this.<fieldName>;
    sourceCode << "public get @@I1():@@T2{" << std::endl;
    sourceCode << "return this.@@I3" << std::endl;
    sourceCode << "}" << std::endl;

    return parser
        ->CreateFormattedClassMethodDefinition(sourceCode.str(), ifaceMethod->Id()->Name(), fieldType,
                                               anonClassFieldName)
        ->AsMethodDefinition();
}

static void FillClassBody(public_lib::Context *ctx, ArenaVector<ir::AstNode *> *classBody,
                          const ArenaVector<ir::AstNode *> &ifaceBody, ir::ObjectExpression *objExpr,
                          ArenaVector<ReadonlyFieldHolder> &readonlyFields,
                          checker::ETSObjectType *currentType = nullptr)
{
    auto *checker = ctx->checker->AsETSChecker();

    for (auto *it : ifaceBody) {
        ASSERT(it->IsMethodDefinition());
        auto *ifaceMethod = it->AsMethodDefinition();

        if (!ifaceMethod->Function()->IsGetter() && !ifaceMethod->Function()->IsSetter()) {
            checker->LogTypeError("Interface has methods", objExpr->Start());
            objExpr->SetTsType(checker->GlobalTypeError());
            return;
        }

        if (!ifaceMethod->Function()->IsGetter()) {
            continue;
        }

        auto copyIfaceMethod = ifaceMethod->Clone(checker->Allocator(), nullptr);
        copyIfaceMethod->SetRange(ifaceMethod->Range());
        copyIfaceMethod->Function()->SetSignature(ifaceMethod->Function()->Signature());

        if (currentType != nullptr) {
            auto instanProp =
                currentType->GetOwnProperty<checker::PropertyType::INSTANCE_METHOD>(ifaceMethod->Id()->Name());
            auto funcType = (instanProp != nullptr) ? instanProp->TsType() : nullptr;
            if (funcType != nullptr) {
                ASSERT(funcType->IsETSFunctionType() && funcType->AsETSFunctionType()->FindGetter() != nullptr);
                copyIfaceMethod->Function()->SetSignature(funcType->AsETSFunctionType()->FindGetter());
            }
        }

        // Field identifier
        auto anonClassFieldName = GenName(ctx->allocator);
        auto *field = CreateAnonClassField(ctx, copyIfaceMethod, anonClassFieldName);
        if (field->IsReadonly()) {
            readonlyFields.push_back(
                std::make_tuple(anonClassFieldName, ifaceMethod->Id()->Name(), field->TypeAnnotation()->TsType()));
        }
        classBody->push_back(field);
        SetSourceRangesRecursively(field, ifaceMethod->Range());

        auto *getter = CreateAnonClassFieldGetterSetter(ctx, copyIfaceMethod, false, anonClassFieldName);
        classBody->push_back(getter);
        SetSourceRangesRecursively(getter, ifaceMethod->Range());

        if (copyIfaceMethod->Overloads().size() == 1 && copyIfaceMethod->Overloads()[0]->Function()->IsSetter()) {
            auto *setter = CreateAnonClassFieldGetterSetter(ctx, copyIfaceMethod, true, anonClassFieldName);
            classBody->push_back(setter);
            SetSourceRangesRecursively(setter, ifaceMethod->Range());
        }
    }
}

static void FillAnonClassBody(public_lib::Context *ctx, ArenaVector<ir::AstNode *> *classBody,
                              ir::TSInterfaceDeclaration *ifaceNode, ir::ObjectExpression *objExpr,
                              ArenaVector<ReadonlyFieldHolder> &readonlyFields)
{
    for (auto *extendedIface : ifaceNode->TsType()->AsETSObjectType()->Interfaces()) {
        auto extendedIfaceBody = extendedIface->GetDeclNode()->AsTSInterfaceDeclaration()->Body()->Body();
        FillClassBody(ctx, classBody, extendedIfaceBody, objExpr, readonlyFields, extendedIface);
    }

    FillClassBody(ctx, classBody, ifaceNode->Body()->Body(), objExpr, readonlyFields);
}

static checker::Type *GenerateAnonClassTypeFromInterface(public_lib::Context *ctx,
                                                         ir::TSInterfaceDeclaration *ifaceNode,
                                                         ir::ObjectExpression *objExpr)
{
    auto *checker = ctx->checker->AsETSChecker();

    if (ifaceNode->GetAnonClass() != nullptr) {
        return ifaceNode->GetAnonClass()->Definition()->TsType()->AsETSObjectType();
    }

    auto classBodyBuilder = [ctx, checker, ifaceNode, objExpr](ArenaVector<ir::AstNode *> *classBody) {
        if (ifaceNode->TsType() == nullptr) {
            ifaceNode->Check(checker);
        }
        ArenaVector<ReadonlyFieldHolder> readonlyFields(checker->Allocator()->Adapter());
        FillAnonClassBody(ctx, classBody, ifaceNode, objExpr, readonlyFields);
        classBody->push_back(CreateAnonClassImplCtor(ctx, readonlyFields));
    };

    auto anonClassName = GenName(checker->Allocator());
    auto *classDecl = checker->BuildClass(anonClassName.View(), classBodyBuilder);
    auto *classDef = classDecl->Definition();
    auto *classType = classDef->TsType()->AsETSObjectType();
    classDef->SetAnonymousModifier();

    classDecl->SetRange(ifaceNode->Range());
    classDef->SetRange(ifaceNode->Range());

    // Class type params
    if (ifaceNode->TypeParams() != nullptr) {
        ArenaVector<checker::Type *> typeArgs(checker->Allocator()->Adapter());
        for (auto param : ifaceNode->TypeParams()->Params()) {
            auto *var = param->Name()->Variable();
            ASSERT(var && var->TsType()->IsETSTypeParameter());
            typeArgs.push_back(var->TsType());
        }
        classType->SetTypeArguments(std::move(typeArgs));
    }

    // Class implements
    auto *classImplements =
        checker->AllocNode<ir::TSClassImplements>(checker->AllocNode<ir::OpaqueTypeNode>(ifaceNode->TsType()));
    classImplements->SetParent(classDef);
    classDef->Implements().emplace_back(classImplements);
    classType->RemoveObjectFlag(checker::ETSObjectFlags::RESOLVED_INTERFACES);
    checker->GetInterfacesOfClass(classType);

    ifaceNode->SetAnonClass(classDecl);
    return classType;
}

static void HandleInterfaceLowering(public_lib::Context *ctx, ir::ObjectExpression *objExpr)
{
    auto *checker = ctx->checker->AsETSChecker();

    auto *targetType = objExpr->TsType();
    ASSERT(targetType->AsETSObjectType()->GetDeclNode()->IsTSInterfaceDeclaration());
    auto *ifaceNode = targetType->AsETSObjectType()->GetDeclNode()->AsTSInterfaceDeclaration();
    checker::Type *resultType = GenerateAnonClassTypeFromInterface(ctx, ifaceNode, objExpr);

    if (targetType->AsETSObjectType()->IsPartial()) {
        resultType->AsETSObjectType()->SetBaseType(targetType->AsETSObjectType()->GetBaseType());
    }

    if (!targetType->AsETSObjectType()->TypeArguments().empty()) {
        ArenaVector<checker::Type *> typeArgTypes(targetType->AsETSObjectType()->TypeArguments());
        checker::InstantiationContext instantiationCtx(checker, resultType->AsETSObjectType(), std::move(typeArgTypes),
                                                       objExpr->Start());
        resultType = instantiationCtx.Result();
    }

    if (const auto *const parent = objExpr->Parent(); parent->IsArrayExpression()) {
        for (auto *elem : parent->AsArrayExpression()->Elements()) {
            if (!elem->IsObjectExpression()) {
                continue;
            }
            // Adjusting ts types of other object literals in array
            elem->AsObjectExpression()->SetTsType(resultType);
        }
    }
    objExpr->SetTsType(resultType);
}

bool InterfaceObjectLiteralLowering::Perform(public_lib::Context *ctx, parser::Program *program)
{
    for (auto &[_, extPrograms] : program->ExternalSources()) {
        (void)_;
        for (auto *extProg : extPrograms) {
            Perform(ctx, extProg);
        }
    }

    program->Ast()->IterateRecursivelyPostorder([ctx](ir::AstNode *ast) -> void {
        if (ast->IsObjectExpression() && IsInterfaceType(ast->AsObjectExpression()->TsType())) {
            HandleInterfaceLowering(ctx, ast->AsObjectExpression());
        }
    });

    return true;
}

bool InterfaceObjectLiteralLowering::Postcondition(public_lib::Context *ctx, const parser::Program *program)
{
    for (auto &[_, extPrograms] : program->ExternalSources()) {
        (void)_;
        for (auto *extProg : extPrograms) {
            if (!Postcondition(ctx, extProg)) {
                return false;
            }
        }
    }

    return !program->Ast()->IsAnyChild([](const ir::AstNode *ast) -> bool {
        return ast->IsObjectExpression() && IsInterfaceType(ast->AsObjectExpression()->TsType());
    });
}
}  // namespace ark::es2panda::compiler
