/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include "generated/signatures.h"
#include "ir/expressions/assignmentExpression.h"
#include "util/helpers.h"
#include "util/nameMangler.h"

namespace ark::es2panda::compiler {

static constexpr std::string_view OBJECT_LITERAL_SUFFIX = "$ObjectLiteral";
using ReadonlyFieldHolder =
    std::tuple<util::UString, util::StringView, checker::Type *>;  // anonClassFieldName, paramName, fieldType

std::string_view InterfaceObjectLiteralLowering::Name() const
{
    return "InterfaceObjectLiteralLowering";
}

static inline bool IsInterfaceType(const checker::Type *type)
{
    return type != nullptr && type->IsETSObjectType() &&
           type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::INTERFACE) && !type->IsGradualType();
}

static inline bool IsAbstractClassType(const checker::Type *type)
{
    return type != nullptr && type->IsETSObjectType() &&
           type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::ABSTRACT) && !type->IsGradualType();
}

static ir::AstNode *CreateAnonClassImplCtor(public_lib::Context *ctx, ArenaVector<ReadonlyFieldHolder> &readonlyFields)
{
    auto *const checker = ctx->GetChecker()->AsETSChecker();
    auto *const parser = ctx->parser->AsETSParser();
    checker::ETSChecker::ClassInitializerBuilder initBuilder =
        [ctx, checker, parser, readonlyFields](ArenaVector<ir::Statement *> *statements,
                                               ArenaVector<ir::Expression *> *params) {
            for (auto [anonClassFieldName, paramName, retType] : readonlyFields) {
                ir::ETSParameterExpression *param =
                    checker->AddParam(paramName, ctx->AllocNode<ir::OpaqueTypeNode>(retType, ctx->Allocator()));
                params->push_back(param);
                auto *paramIdent = ctx->AllocNode<ir::Identifier>(paramName, ctx->Allocator());
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
    ES2PANDA_ASSERT(ifaceMethod->Function());
    auto *fieldType = ifaceMethod->Function()->Signature()->ReturnType();
    ES2PANDA_ASSERT(fieldType != nullptr);

    std::stringstream sourceCode;

    if (isSetter) {
        // Setter body: this.<fieldName> = <callParam>;
        sourceCode << "public set @@I1 (anonParam:@@T2){" << std::endl;
        sourceCode << "this.@@I3 = anonParam" << std::endl;
        sourceCode << "}" << std::endl;
        ES2PANDA_ASSERT(ifaceMethod->Id());
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
                          const ArenaVector<ir::AstNode *> &ifaceBody, ArenaVector<ReadonlyFieldHolder> &readonlyFields,
                          checker::ETSObjectType *currentType = nullptr)
{
    for (auto *it : ifaceBody) {
        if (it->IsOverloadDeclaration()) {
            continue;
        }

        ES2PANDA_ASSERT(it->IsMethodDefinition());
        auto *ifaceMethod = it->AsMethodDefinition();

        ES2PANDA_ASSERT(ifaceMethod->Function());
        if (!ifaceMethod->Function()->IsGetter()) {
            continue;
        }

        auto iter = std::find_if(classBody->begin(), classBody->end(), [ifaceMethod](ir::AstNode *ast) -> bool {
            return ast->IsMethodDefinition() && ast->AsMethodDefinition()->Function()->IsGetter() &&
                   ast->AsMethodDefinition()->Id()->Name() == ifaceMethod->Id()->Name();
        });
        if (iter != classBody->end()) {
            continue;
        }

        auto copyIfaceMethod = ifaceMethod->Clone(ctx->Allocator(), nullptr);
        copyIfaceMethod->SetRange(ifaceMethod->Range());
        copyIfaceMethod->Function()->SetSignature(ifaceMethod->Function()->Signature());

        if (currentType != nullptr) {
            auto prop = currentType->GetOwnProperty<checker::PropertyType::INSTANCE_METHOD>(ifaceMethod->Id()->Name());
            auto funcType = (prop != nullptr) ? prop->TsType() : nullptr;
            if (funcType != nullptr) {
                ES2PANDA_ASSERT(funcType->IsETSFunctionType() &&
                                funcType->AsETSFunctionType()->FindGetter() != nullptr);
                copyIfaceMethod->Function()->SetSignature(funcType->AsETSFunctionType()->FindGetter());
            }
        }

        // Field identifier
        std::string newName = util::NameMangler::GetInstance()->CreateMangledNameByTypeAndName(
            util::NameMangler::PROPERTY, ifaceMethod->Id()->Name());
        util::UString anonClassFieldName(newName, ctx->allocator);
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

// CC-OFFNXT(G.FUN.01-CPP) solid logic
static void FillAnonClassBody(public_lib::Context *ctx, ArenaVector<ir::AstNode *> *classBody,
                              ir::TSInterfaceDeclaration *ifaceNode, ArenaVector<ReadonlyFieldHolder> &readonlyFields,
                              checker::ETSObjectType *interfaceType = nullptr)
{
    FillClassBody(ctx, classBody, ifaceNode->Body()->Body(), readonlyFields, interfaceType);
    for (auto *extendedIface : ifaceNode->TsType()->AsETSObjectType()->Interfaces()) {
        FillAnonClassBody(ctx, classBody, extendedIface->GetDeclNode()->AsTSInterfaceDeclaration(), readonlyFields,
                          extendedIface);
    }
}

// Annotate synthetic class so we can determite it's origin in a runtime
// Now implemented for the anon class generated from an interface only
static void AnnotateGeneratedAnonClass(checker::ETSChecker *checker, ir::ClassDefinition *classDef)
{
    auto *annoId =
        checker->ProgramAllocNode<ir::Identifier>(Signatures::INTERFACE_OBJ_LITERAL, checker->ProgramAllocator());
    annoId->SetAnnotationUsage();
    auto *annoUsage = checker->ProgramAllocNode<ir::AnnotationUsage>(annoId, checker->ProgramAllocator());
    ES2PANDA_ASSERT(annoUsage);
    annoUsage->AddModifier(ir::ModifierFlags::ANNOTATION_USAGE);
    annoUsage->SetParent(classDef);
    annoId->SetParent(annoUsage);
    classDef->EmplaceAnnotation(annoUsage);
    RefineSourceRanges(annoUsage);
    CheckLoweredNode(checker->VarBinder()->AsETSBinder(), checker, annoUsage);
}

static void GenerateAnonClassTypeFromInterface(public_lib::Context *ctx, ir::TSInterfaceDeclaration *ifaceNode)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();

    if (ifaceNode->GetAnonClass() != nullptr) {
        return;
    }

    auto classBodyBuilder = [ctx, checker, ifaceNode](ArenaVector<ir::AstNode *> *classBody) {
        if (ifaceNode->TsType() == nullptr) {
            ifaceNode->Check(checker);
        }
        ArenaVector<ReadonlyFieldHolder> readonlyFields(ctx->Allocator()->Adapter());
        FillAnonClassBody(ctx, classBody, ifaceNode, readonlyFields);
        classBody->push_back(CreateAnonClassImplCtor(ctx, readonlyFields));
    };

    auto originalName = std::string {ifaceNode->InternalName()};
    std::replace(originalName.begin(), originalName.end(), '.', '$');
    auto anonClassName = util::UString(originalName.append(OBJECT_LITERAL_SUFFIX), checker->ProgramAllocator());
    auto *classDecl = checker->BuildClass(anonClassName.View(), classBodyBuilder);
    RefineSourceRanges(classDecl);
    auto *classDef = classDecl->Definition();
    auto *classType = classDef->TsType()->MaybeBaseTypeOfGradualType()->AsETSObjectType();
    classDef->SetAnonymousModifier();

    classDecl->SetRange(ifaceNode->Range());
    classDef->SetRange(ifaceNode->Range());

    AnnotateGeneratedAnonClass(checker, classDef);

    // Class type params
    if (ifaceNode->TypeParams() != nullptr) {
        ArenaVector<checker::Type *> typeArgs(ctx->Allocator()->Adapter());
        for (auto param : ifaceNode->TypeParams()->Params()) {
            auto *var = param->Name()->Variable();
            ES2PANDA_ASSERT(var && var->TsType()->IsETSTypeParameter());
            typeArgs.push_back(var->TsType());
        }
        classType->SetTypeArguments(std::move(typeArgs));
    }

    // Class implements
    auto *classImplements = ctx->AllocNode<ir::TSClassImplements>(
        ctx->AllocNode<ir::OpaqueTypeNode>(ifaceNode->TsType(), ctx->Allocator()));
    ES2PANDA_ASSERT(classImplements);
    classImplements->SetParent(classDef);
    classDef->EmplaceImplements(classImplements);
    classType->RemoveObjectFlag(checker::ETSObjectFlags::RESOLVED_INTERFACES);
    checker->GetInterfacesOfClass(classType);

    ifaceNode->SetAnonClass(classDecl);
}

static void GenerateAnonClassTypeFromAbstractClass(public_lib::Context *ctx, ir::ClassDefinition *abstractClassNode)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();

    if (abstractClassNode->GetAnonClass() != nullptr) {
        return;
    }

    auto classBodyBuilder = [checker](ArenaVector<ir::AstNode *> *classBody) {
        checker::ETSChecker::ClassInitializerBuilder initBuilder =
            [checker]([[maybe_unused]] ArenaVector<ir::Statement *> *statements,
                      [[maybe_unused]] ArenaVector<ir::Expression *> *params) {
                checker->AddParam(varbinder::VarBinder::MANDATORY_PARAM_THIS, nullptr);
            };

        auto ctor = checker->CreateClassInstanceInitializer(initBuilder);
        classBody->push_back(ctor);
    };

    auto originalName = std::string {abstractClassNode->InternalName()};
    std::replace(originalName.begin(), originalName.end(), '.', '$');
    auto anonClassName = util::UString(originalName.append(OBJECT_LITERAL_SUFFIX), checker->ProgramAllocator());
    auto *classDecl = checker->BuildClass(anonClassName.View(), classBodyBuilder);
    RefineSourceRanges(classDecl);
    auto *classDef = classDecl->Definition();
    if (classDef->TsType()->IsGradualType()) {
        return;
    }
    auto *classType = classDef->TsType()->AsETSObjectType();

    classDecl->SetRange(abstractClassNode->Range());
    classDef->SetAnonymousModifier();
    classDef->SetRange(abstractClassNode->Range());

    // Class type params
    if (abstractClassNode->TypeParams() != nullptr) {
        ArenaVector<checker::Type *> typeArgs(ctx->Allocator()->Adapter());
        for (auto param : abstractClassNode->TypeParams()->Params()) {
            auto *var = param->Name()->Variable();
            ES2PANDA_ASSERT(var && var->TsType()->IsETSTypeParameter());
            typeArgs.push_back(var->TsType());
        }
        classType->SetTypeArguments(std::move(typeArgs));
    }

    abstractClassNode->SetAnonClass(classDecl);
    classType->SetSuperType(abstractClassNode->TsType()->AsETSObjectType());
}

static checker::Type *ProcessDeclNode(checker::ETSChecker *checker, checker::ETSObjectType *targetType,
                                      ir::ObjectExpression *objExpr)
{
    auto *declNode = targetType->GetDeclNode();

    if (declNode->IsTSInterfaceDeclaration()) {
        auto *ifaceNode = declNode->AsTSInterfaceDeclaration();
        if (ifaceNode->GetAnonClass() == nullptr) {
            checker->LogError(diagnostic::INTERFACE_WITH_METHOD, {}, objExpr->Start());
            return checker->GlobalTypeError();
        }
        return ifaceNode->GetAnonClass()->Definition()->TsType();
    }

    auto *classDef = declNode->AsClassDefinition();
    ES2PANDA_ASSERT(classDef->IsAbstract());

    if (classDef->GetAnonClass() == nullptr) {
        for (auto it : classDef->Body()) {
            if (!it->IsMethodDefinition() || !it->AsMethodDefinition()->IsAbstract()) {
                continue;
            }

            ES2PANDA_ASSERT(it->AsMethodDefinition()->Id());
            checker->LogError(diagnostic::ABSTRACT_METH_IN_ABSTRACT_CLASS, {it->AsMethodDefinition()->Id()->Name()},
                              objExpr->Start());
            return checker->GlobalTypeError();
        }
    }
    return classDef->GetAnonClass()->Definition()->TsType();
}

static void HandleInterfaceLowering(public_lib::Context *ctx, ir::ObjectExpression *objExpr)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *targetType = objExpr->TsType();
    checker->CheckObjectLiteralKeys(objExpr->Properties());

    auto *etsTargetType = targetType->AsETSObjectType();
    checker::Type *resultType = ProcessDeclNode(checker, etsTargetType, objExpr);

    if (resultType->IsTypeError()) {
        objExpr->SetTsType(resultType);
        return;
    }

    if (etsTargetType->IsPartial()) {
        resultType->AsETSObjectType()->SetBaseType(etsTargetType->GetBaseType());
    }

    if (!etsTargetType->TypeArguments().empty()) {
        ArenaVector<checker::Type *> typeArgTypes(etsTargetType->TypeArguments());
        checker::InstantiationContext instantiationCtx(checker, resultType->AsETSObjectType(), std::move(typeArgTypes),
                                                       objExpr->Start());
        resultType = instantiationCtx.Result();
    }

    if (const auto *const parent = objExpr->Parent();
        parent->IsArrayExpression() && !parent->AsArrayExpression()->TsType()->IsETSTupleType()) {
        for (auto *elem : parent->AsArrayExpression()->Elements()) {
            if (elem->IsObjectExpression()) {
                elem->AsObjectExpression()->SetTsType(resultType);
            }
        }
    }
    objExpr->SetTsType(resultType);
}

static bool CheckInterfaceShouldGenerateAnonClass(ir::TSInterfaceDeclaration *interfaceDecl)
{
    if (interfaceDecl->TsType() != nullptr && interfaceDecl->TsType()->IsGradualType()) {
        return false;
    }
    for (auto it : interfaceDecl->Body()->Body()) {
        if (it->IsOverloadDeclaration()) {
            continue;
        }
        ES2PANDA_ASSERT(it->IsMethodDefinition());
        auto methodDef = it->AsMethodDefinition();
        ES2PANDA_ASSERT(methodDef->Function());
        if (!methodDef->Function()->IsGetter() && !methodDef->Function()->IsSetter()) {
            return false;
        }
    }

    return true;
}

static bool CheckAbstractClassShouldGenerateAnonClass(ir::ClassDefinition *classDef)
{
    auto constructorSigs = classDef->TsType()->AsETSObjectType()->ConstructSignatures();
    if (auto res = std::find_if(constructorSigs.cbegin(), constructorSigs.cend(),
                                [](checker::Signature *sig) -> bool { return sig->MinArgCount() == 0; });
        res == constructorSigs.cend()) {
        return false;
    }
    for (auto it : classDef->Body()) {
        if (it->IsMethodDefinition() && it->AsMethodDefinition()->IsAbstract()) {
            return false;
        }
    }

    return true;
}

static void TransfromInterfaceDecl(public_lib::Context *ctx, parser::Program *program)
{
    program->Ast()->IterateRecursivelyPostorder([ctx, program](ir::AstNode *ast) -> void {
        if (ast->IsTSInterfaceDeclaration() && CheckInterfaceShouldGenerateAnonClass(ast->AsTSInterfaceDeclaration())) {
            GenerateAnonClassTypeFromInterface(ctx, ast->AsTSInterfaceDeclaration());
        } else if (ast->IsClassDefinition() && ast != program->GlobalClass() &&
                   ast->AsClassDefinition()->IsAbstract() && !ast->AsClassDefinition()->TsType()->IsGradualType() &&
                   CheckAbstractClassShouldGenerateAnonClass(ast->AsClassDefinition())) {
            GenerateAnonClassTypeFromAbstractClass(ctx, ast->AsClassDefinition());
        }
    });
}

static void TransfromInterfaceLiteral(public_lib::Context *ctx, parser::Program *program)
{
    program->Ast()->IterateRecursivelyPostorder([ctx](ir::AstNode *ast) -> void {
        if (!ast->IsObjectExpression()) {
            return;
        }
        auto objExpr = ast->AsObjectExpression();
        if ((IsInterfaceType(objExpr->TsType()) || IsAbstractClassType(objExpr->TsType())) &&
            !objExpr->TsType()->AsETSObjectType()->GetDeclNode()->AsTyped()->TsType()->IsGradualType()) {
            HandleInterfaceLowering(ctx, ast->AsObjectExpression());
        }
    });
}

bool InterfaceObjectLiteralLowering::Perform(public_lib::Context *ctx, parser::Program *program)
{
    auto *varbinder = program->VarBinder()->AsETSBinder();
    for (auto &[_, extPrograms] : program->ExternalSources()) {
        (void)_;
        for (auto *extProg : extPrograms) {
            if (extProg->IsASTLowered()) {
                continue;
            }
            auto *savedProgram = varbinder->Program();
            auto *savedRecordTable = varbinder->GetRecordTable();
            auto *savedTopScope = varbinder->TopScope();
            varbinder->ResetTopScope(extProg->GlobalScope());
            varbinder->SetRecordTable(varbinder->GetExternalRecordTable().at(extProg));
            varbinder->SetProgram(extProg);
            TransfromInterfaceDecl(ctx, extProg);
            varbinder->SetProgram(savedProgram);
            varbinder->SetRecordTable(savedRecordTable);
            varbinder->ResetTopScope(savedTopScope);
        }
    }

    TransfromInterfaceDecl(ctx, program);

    for (auto &[_, extPrograms] : program->ExternalSources()) {
        (void)_;
        for (auto *extProg : extPrograms) {
            if (extProg->IsASTLowered()) {
                continue;
            }
            TransfromInterfaceLiteral(ctx, extProg);
        }
    }

    TransfromInterfaceLiteral(ctx, program);

    return true;
}

}  // namespace ark::es2panda::compiler
