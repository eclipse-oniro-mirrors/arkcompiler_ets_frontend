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

namespace ark::es2panda::compiler {

static constexpr std::string_view OBJECT_LITERAL_SUFFIX = "$ObjectLiteral";

using ReadonlyFieldHolder =
    std::tuple<util::UString, util::StringView, checker::Type *>;  // anonClassFieldName, paramName, fieldType

using ReadonlyFields = std::vector<ReadonlyFieldHolder>;

using CapturedVariable = std::tuple<varbinder::Variable const *, util::StringView const, ir::Identifier *>;

static std::string_view LoweringName() noexcept
{
    return "InterfaceObjectLiteralLowering";
}

std::string_view InterfaceObjectLiteralLowering::Name() const
{
    return LoweringName();
}

static inline bool IsInterfaceType(const checker::Type *type)
{
    return type != nullptr && type->IsETSObjectType() &&
           type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::INTERFACE);
}

static inline bool IsAbstractClassType(const checker::Type *type)
{
    return type != nullptr && type->IsETSObjectType() &&
           type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::ABSTRACT);
}

static ir::AstNode *CreateAnonClassImplCtor(public_lib::Context *ctx, ReadonlyFields &readonlyFields)
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
                                               checker::Type *fieldType, bool isSetter,
                                               util::UString anonClassFieldName)
{
    auto *const parser = ctx->parser->AsETSParser();
    // Field type annotation
    std::stringstream sourceCode;
    // Field modifiers flags
    sourceCode << "private ";
    // No overloads and the method is not setter, means no setter function with the same name so the field is readonly
    if (ifaceMethod->Overloads().empty() && !isSetter) {
        sourceCode << "readonly ";
    }
    sourceCode << "@@I1 : @@T2;" << std::endl;

    auto field = parser->CreateFormattedClassFieldDefinition(sourceCode.str(), anonClassFieldName, fieldType);
    field->SetRange(ifaceMethod->Range());

    return field->AsClassProperty();
}

static ir::MethodDefinition *CreateAnonClassFieldGetterSetter(public_lib::Context *ctx,
                                                              ir::MethodDefinition *ifaceMethod,
                                                              checker::Type *fieldType, bool isSetter,
                                                              util::UString anonClassFieldName)
{
    auto *const parser = ctx->parser->AsETSParser();
    // Field type annotation
    ES2PANDA_ASSERT(ifaceMethod->Function());
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

static void AddAnonClassFieldAndAccessors(public_lib::Context *ctx, ArenaVector<ir::AstNode *> &classBody,
                                          ReadonlyFields &readonlyFields, ir::MethodDefinition *ifaceMethod,
                                          ir::MethodDefinition *copyIfaceMethod)
{
    bool isSetter = copyIfaceMethod->Function()->IsSetter();
    auto *fieldType = isSetter ? copyIfaceMethod->Function()->Signature()->Params()[0]->TsType()
                               : copyIfaceMethod->Function()->Signature()->ReturnType();

    std::string newName = util::NameMangler::GetInstance()->CreateMangledNameByTypeAndName(util::NameMangler::PROPERTY,
                                                                                           ifaceMethod->Id()->Name());
    util::UString anonClassFieldName(newName, ctx->allocator);

    auto *field = CreateAnonClassField(ctx, copyIfaceMethod, fieldType, isSetter, anonClassFieldName);
    if (field->IsReadonly()) {
        readonlyFields.emplace_back(
            std::make_tuple(anonClassFieldName, ifaceMethod->Id()->Name(), field->TypeAnnotation()->TsType()));
    }
    classBody.emplace_back(field);
    SetSourceRangesRecursively(field, ifaceMethod->Range());

    auto *accessor = CreateAnonClassFieldGetterSetter(ctx, copyIfaceMethod, fieldType, isSetter, anonClassFieldName);
    classBody.emplace_back(accessor);
    SetSourceRangesRecursively(accessor, ifaceMethod->Range());

    if (copyIfaceMethod->Overloads().size() == 1) {
        auto *anotherAccessor =
            CreateAnonClassFieldGetterSetter(ctx, copyIfaceMethod, fieldType, !isSetter, anonClassFieldName);
        classBody.emplace_back(anotherAccessor);
        SetSourceRangesRecursively(anotherAccessor, ifaceMethod->Range());
    }
}

static void FillClassBody(public_lib::Context *ctx, ArenaVector<ir::AstNode *> &classBody,
                          const ArenaVector<ir::AstNode *> &ifaceBody, ReadonlyFields &readonlyFields,
                          checker::ETSObjectType *currentType = nullptr)
{
    for (auto *it : ifaceBody) {
        if (it->IsOverloadDeclaration()) {
            continue;
        }

        ES2PANDA_ASSERT(it->IsMethodDefinition());
        auto *ifaceMethod = it->AsMethodDefinition();

        ES2PANDA_ASSERT(ifaceMethod->Function());
        if (!ifaceMethod->Function()->IsGetterOrSetter()) {
            continue;
        }
        bool isSetter = ifaceMethod->Function()->IsSetter();

        auto iter = std::find_if(classBody.begin(), classBody.end(), [ifaceMethod](ir::AstNode *ast) -> bool {
            return ast->IsMethodDefinition() && ast->AsMethodDefinition()->Function()->IsGetterOrSetter() &&
                   ast->AsMethodDefinition()->Id()->Name() == ifaceMethod->Id()->Name();
        });
        if (iter != classBody.end()) {
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
                                (funcType->AsETSFunctionType()->FindGetter() != nullptr ||
                                 funcType->AsETSFunctionType()->FindSetter() != nullptr));
                auto *sig = isSetter ? funcType->AsETSFunctionType()->FindSetter()
                                     : funcType->AsETSFunctionType()->FindGetter();
                copyIfaceMethod->Function()->SetSignature(sig);
            }
        }

        AddAnonClassFieldAndAccessors(ctx, classBody, readonlyFields, ifaceMethod, copyIfaceMethod);
    }
}

// CC-OFFNXT(G.FUN.01-CPP) solid logic
static void FillAnonClassBody(public_lib::Context *ctx, ArenaVector<ir::AstNode *> &classBody,
                              ir::TSInterfaceDeclaration *ifaceNode, ReadonlyFields &readonlyFields,
                              checker::ETSObjectType *interfaceType = nullptr)
{
    FillClassBody(ctx, classBody, ifaceNode->Body()->Body(), readonlyFields, interfaceType);
    for (auto *extendedIface : ifaceNode->TsType()->AsETSObjectType()->Interfaces()) {
        auto *const subInterfaceNode = extendedIface->GetDeclNode()->AsTSInterfaceDeclaration();
        subInterfaceNode->Check(ctx->GetChecker()->AsETSChecker());
        FillAnonClassBody(ctx, classBody, subInterfaceNode, readonlyFields, extendedIface);
    }
}

static std::string GenerateAnonClassName(std::string_view const originalName, bool const addUniqueID = false)
{
    auto anonClassName = std::string {originalName};
    std::replace(anonClassName.begin(), anonClassName.end(), '.', '$');
    anonClassName.append(OBJECT_LITERAL_SUFFIX);
    if (addUniqueID) {
        anonClassName.append(GenName());
    }
    return anonClassName;
}

// Annotate synthetic class so we can determine it's origin in a runtime
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

ir::ClassDeclaration *GenerateAnonClass(public_lib::Context *ctx, util::StringView const className,
                                        ir::AstNode const *const decl,
                                        const checker::ETSChecker::ClassBuilder &bodyBuilder)
{
    ES2PANDA_ASSERT(decl != nullptr && (decl->IsTSInterfaceDeclaration() || decl->IsClassDefinition()));
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *allocator = ctx->Allocator();

    auto *scope = compiler::NearestScope(decl);
    auto scopeCtx = checker::ScopeContext(checker, scope);
    auto expressionCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(checker->VarBinder(), scope);

    auto *classDecl = checker->BuildClass(className, bodyBuilder);
    RefineSourceRanges(classDecl);

    auto *classDef = classDecl->Definition();
    auto *classType = classDef->TsType()->AsETSObjectType();
    if (classType->IsGradual()) {
        return classDecl;
    }

    classDef->SetAnonymousModifier();

    auto const range = decl->Range();
    classDecl->SetRange(range);
    classDef->SetRange(range);

    // Class type params
    auto const *const typeParams = decl->IsTSInterfaceDeclaration() ? decl->AsTSInterfaceDeclaration()->TypeParams()
                                                                    : decl->AsClassDefinition()->TypeParams();
    if (typeParams != nullptr) {
        ArenaVector<checker::Type *> typeArgs(allocator->Adapter());
        for (auto const *const param : typeParams->Params()) {
            auto const *const var = param->Name()->Variable();
            ES2PANDA_ASSERT(var != nullptr && var->TsType()->IsETSTypeParameter());
            typeArgs.emplace_back(var->TsType());
        }
        classType->SetTypeArguments(std::move(typeArgs));
    }

    if (decl->IsTSInterfaceDeclaration()) {
        AnnotateGeneratedAnonClass(checker, classDef);
        // Class implements
        auto *classImplements = ctx->AllocNode<ir::TSClassImplements>(ctx->AllocNode<ir::OpaqueTypeNode>(
            const_cast<checker::Type *>(decl->AsTSInterfaceDeclaration()->TsType()), allocator));
        classImplements->SetParent(classDef);
        classDef->EmplaceImplements(classImplements);
        classType->RemoveObjectFlag(checker::ETSObjectFlags::RESOLVED_INTERFACES);
        checker->GetInterfacesOfClass(classType);
    } else {
        classType->SetSuperType(const_cast<checker::Type *>(decl->AsClassDefinition()->TsType())->AsETSObjectType());
    }

    return classDecl;
}

static checker::Type *GenerateAnonClassFromInterface(public_lib::Context *ctx, ir::TSInterfaceDeclaration *ifaceNode)
{
    if (ifaceNode->GetAnonClass() != nullptr) {
        return ifaceNode->GetAnonClass()->Definition()->TsType();
    }

    auto const classBodyBuilder = [ctx, ifaceNode](ArenaVector<ir::AstNode *> &classBody) -> void {
        if (ifaceNode->TsType() == nullptr) {
            ifaceNode->Check(ctx->GetChecker()->AsETSChecker());
        }
        ReadonlyFields readonlyFields {};
        FillAnonClassBody(ctx, classBody, ifaceNode, readonlyFields);
        classBody.emplace_back(CreateAnonClassImplCtor(ctx, readonlyFields));
    };

    auto anonClassName = util::UString(GenerateAnonClassName(ifaceNode->InternalName().Utf8()), ctx->Allocator());
    auto *classDecl = GenerateAnonClass(ctx, anonClassName.View(), ifaceNode, classBodyBuilder);

    if (!classDecl->Definition()->TsType()->AsETSObjectType()->IsGradual()) {
        ifaceNode->SetAnonClass(classDecl);
    }

    return classDecl->Definition()->TsType();
}

static void GenerateAnonClassFromAbstractClass(public_lib::Context *ctx, ir::ClassDefinition *abstractClassNode)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();

    if (abstractClassNode->GetAnonClass() != nullptr) {
        return;
    }

    auto classBodyBuilder = [checker](ArenaVector<ir::AstNode *> &classBody) -> void {
        checker::ETSChecker::ClassInitializerBuilder initBuilder =
            [checker]([[maybe_unused]] ArenaVector<ir::Statement *> *statements,
                      [[maybe_unused]] ArenaVector<ir::Expression *> *params) {
                checker->AddParam(varbinder::VarBinder::MANDATORY_PARAM_THIS, nullptr);
            };

        auto *ctor = checker->CreateClassInstanceInitializer(initBuilder);
        classBody.emplace_back(ctor);
    };

    auto anonClassName =
        util::UString(GenerateAnonClassName(abstractClassNode->InternalName().Utf8()), ctx->Allocator());
    auto *classDecl = GenerateAnonClass(ctx, anonClassName.View(), abstractClassNode, classBodyBuilder);

    if (!classDecl->Definition()->TsType()->AsETSObjectType()->IsGradual()) {
        abstractClassNode->SetAnonClass(classDecl);
    }
}

static bool AllMethodsHaveBody(ir::TSInterfaceDeclaration *interfaceDecl)
{
    ES2PANDA_ASSERT(interfaceDecl->Body() != nullptr);

    for (auto it : interfaceDecl->Body()->Body()) {
        if (it->IsOverloadDeclaration()) {
            continue;
        }

        ES2PANDA_ASSERT(it->IsMethodDefinition());
        auto methodDef = it->AsMethodDefinition();
        ES2PANDA_ASSERT(methodDef->Function());
        if (!methodDef->Function()->HasBody() && !methodDef->Function()->IsGetter() &&
            !methodDef->Function()->IsSetter()) {
            return false;
        }

        for (auto const *const overload : methodDef->Overloads()) {
            ES2PANDA_ASSERT(overload->Function());
            if (!overload->Function()->HasBody() && !overload->Function()->IsGetter() &&
                !overload->Function()->IsSetter()) {
                return false;
            }
        }
    }

    return true;
}

static bool CheckInterfaceShouldGenerateAnonClass(checker::ETSChecker *checker,
                                                  ir::TSInterfaceDeclaration *interfaceDecl)
{
    checker::Type const *const interfaceType = interfaceDecl->Check(checker);
    if (interfaceType == nullptr || interfaceType->IsTypeError() || interfaceType->AsETSObjectType()->IsGradual()) {
        return false;
    }

    if (!AllMethodsHaveBody(interfaceDecl)) {
        return false;
    }

    for (auto const *type : interfaceType->AsETSObjectType()->Interfaces()) {
        if (!AllMethodsHaveBody(type->GetDeclNode()->AsTSInterfaceDeclaration())) {
            return false;
        }
    }

    return true;
}

//==========[ Processing of object interface literals with re-defined methods => begin ]==========//

static ir::AstNode *TransformThisExpression(public_lib::Context *ctx, ir::ThisExpression *thisExpression,
                                            checker::Type *const objectType)
{
    auto *const parent = thisExpression->Parent();

    auto *const typeNode = ctx->AllocNode<ir::OpaqueTypeNode>(objectType, ctx->Allocator());
    auto *const asExpression = ctx->AllocNode<ir::TSAsExpression>(thisExpression, typeNode, false);

    asExpression->SetParent(parent);
    asExpression->SetRange(thisExpression->Range());

    return static_cast<ir::AstNode *>(asExpression);
}

static ir::AstNode *TransformIdentifier(public_lib::Context *ctx, ir::Identifier *ident,
                                        varbinder::FunctionScope const *const functionScope,
                                        std::vector<CapturedVariable> &capturedVariables)
{
    auto const *const variable = ident->Variable();
    if (variable->IsLocalVariable() && variable->HasFlag(varbinder::VariableFlags::LOCAL) &&
        !variable->Name().StartsWith(compiler::GENSYM_CORE) &&
        functionScope->FindLocal(variable->Name(), varbinder::ResolveBindingOptions::BINDINGS) == nullptr) {
        auto *const parent = ident->Parent();

        util::StringView newName;
        auto const it = std::find_if(capturedVariables.cbegin(), capturedVariables.cend(),
                                     [variable](auto const &item) { return std::get<0U>(item) == variable; });
        if (it == capturedVariables.cend()) {
            newName = compiler::GenName(ctx->Allocator()).View();
            capturedVariables.emplace_back(variable, newName, ident);
        } else {
            newName = std::get<1U>(*it);
        }

        auto *const memberExpression = ctx->AllocNode<ir::MemberExpression>(
            ctx->AllocNode<ir::ThisExpression>(), ctx->AllocNode<ir::Identifier>(newName, ctx->Allocator()),
            ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);

        memberExpression->SetParent(parent);
        memberExpression->SetRange(ident->Range());

        return static_cast<ir::AstNode *>(memberExpression);
    }

    return ident;
}

static void AddCapturedVariables(public_lib::Context *ctx, ArenaVector<ir::AstNode *> &classBody,
                                 ArenaVector<ir::Expression *> &properties,
                                 std::vector<CapturedVariable> &capturedVariables)
{
    for (auto const &[variable, fieldName, ident] : capturedVariables) {
        constexpr auto const FIELD_DECLARATION = "public @@I1 : @@T2;";

        auto *const field = ctx->parser->AsETSParser()->CreateFormattedClassFieldDefinition(
            FIELD_DECLARATION, fieldName, variable->TsType());
        classBody.emplace_back(field);

        auto *const property = ctx->AllocNode<ir::Property>(
            ir::PropertyKind::INIT, ctx->AllocNode<ir::Identifier>(fieldName, ctx->Allocator()), ident, false, false);
        properties.emplace_back(property);
    }
}

static void TransformMethodBody(public_lib::Context *ctx, ir::AstNode *const body, checker::Type *const objectType,
                                varbinder::FunctionScope const *const functionScope,
                                std::vector<CapturedVariable> &capturedVariables)
{
    ES2PANDA_ASSERT(body != nullptr && functionScope != nullptr);

    body->TransformChildrenRecursively(
        [=, &capturedVariables](ir::AstNode *node) {
            // Cast `this` expression to original literal type (otherwise it will refer to the newly generated class)
            if (node->IsThisExpression()) {
                return TransformThisExpression(ctx, node->AsThisExpression(), objectType);
            }

            // Process captured variables.
            if (node->IsIdentifier() && node->Variable() != nullptr) {
                return TransformIdentifier(ctx, node->AsIdentifier(), functionScope, capturedVariables);
            }

            return node;
        },
        LoweringName());
}

static void AddMethodsFromLiteral(public_lib::Context *ctx, ArenaVector<ir::AstNode *> &classBody,
                                  ir::ObjectExpression *const objectExpr)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *allocator = ctx->Allocator();

    std::vector<CapturedVariable> capturedVariables {};
    auto &properties = objectExpr->Properties();

    auto it = properties.begin();
    while (it != properties.end()) {
        ES2PANDA_ASSERT((*it)->IsProperty());
        if (auto *const value = (*it)->AsProperty()->Value(); !value->IsArrowFunctionExpression()) {
            ++it;
        } else {
            auto *const key = (*it)->AsProperty()->Key();
            ES2PANDA_ASSERT(key->IsIdentifier());

            ir::ScriptFunction *function = value->AsArrowFunctionExpression()->Function();
            varbinder::FunctionScope const *const scope = function->Scope();

            function = function->Clone(allocator, nullptr);
            function->ClearFlag(ir::ScriptFunctionFlags::ARROW);
            function->AddModifier(ir::ModifierFlags::PUBLIC);

            auto *const ident = key->AsIdentifier()->Clone(allocator, function);
            function->SetIdent(ident);

            TransformMethodBody(ctx, function->Body(), objectExpr->TsType(), scope, capturedVariables);

            auto *const funcExpr = checker->AllocNode<ir::FunctionExpression>(function);
            funcExpr->SetRange(function->Range());

            auto *const method = checker->AllocNode<ir::MethodDefinition>(
                ir::MethodDefinitionKind::METHOD, ident->Clone(allocator, nullptr), funcExpr,
                ir::ModifierFlags::PUBLIC | ir::ModifierFlags::OVERRIDE, allocator, false);
            method->SetRange((*it)->Range());

            compiler::ClearTypesVariablesAndScopes(method);

            classBody.emplace_back(method);

            it = properties.erase(it);
        }
    }

    // Add auxiliary class fields for captured variables and initialize them in literal object
    AddCapturedVariables(ctx, classBody, properties, capturedVariables);
}

static checker::Type *GenerateAnonClassFromInterfaceWithMethods(public_lib::Context *ctx,
                                                                ir::TSInterfaceDeclaration *const interfaceDecl,
                                                                ir::ObjectExpression *const objectExpr)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();

    auto const classBodyBuilder = [=](ArenaVector<ir::AstNode *> &classBody) -> void {
        if (interfaceDecl->TsType() == nullptr) {
            interfaceDecl->Check(checker);
        }
        ReadonlyFields readonlyFields {};
        FillAnonClassBody(ctx, classBody, interfaceDecl, readonlyFields);
        AddMethodsFromLiteral(ctx, classBody, objectExpr);
        classBody.emplace_back(CreateAnonClassImplCtor(ctx, readonlyFields));
    };

    auto anonClassName =
        util::UString(GenerateAnonClassName(interfaceDecl->InternalName().Utf8(), true), ctx->Allocator());
    auto *classDecl = GenerateAnonClass(ctx, anonClassName.View(), interfaceDecl, classBodyBuilder);

    checker::Type *const classType = classDecl->Definition()->Check(checker);
    return classType->IsETSObjectType() && !classType->AsETSObjectType()->IsGradual() ? classType
                                                                                      : checker->GlobalTypeError();
}

static bool CheckInterface(checker::ETSChecker *checker, ir::TSInterfaceDeclaration *interfaceDecl,
                           ir::ObjectExpression *objectExpr)
{
    //  Lambda checks if any method defined in object literal overrides empty method declared in interface.
    auto const checkOverriding = [checker, objectExpr](ir::ScriptFunction *function) -> bool {
        if (function->HasBody() || function->IsGetter() || function->IsSetter()) {
            return true;
        }

        for (ir::Expression *propExpr : objectExpr->Properties()) {
            if (!propExpr->IsProperty()) {
                continue;
            }

            if (auto const *const key = propExpr->AsProperty()->Key();
                !key->IsIdentifier() || !key->AsIdentifier()->Name().Is(function->Id()->Name().Utf8())) {
                continue;
            }

            checker::SavedTypeRelationFlagsContext savedFlagsCtx(checker->Relation(),
                                                                 checker::TypeRelationFlag::OVERRIDING_CONTEXT);
            checker::Type *const valueType = propExpr->AsProperty()->Value()->TsType();
            if (!valueType->IsETSArrowType() ||
                !checker->Relation()->SignatureIsSupertypeOf(
                    function->Signature(), valueType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow()[0U])) {
                continue;
            }

            return true;
        }

        return false;
    };

    ES2PANDA_ASSERT(interfaceDecl->Body() != nullptr);

    for (auto it : interfaceDecl->Body()->Body()) {
        if (it->IsOverloadDeclaration()) {
            continue;
        }
        auto methodDef = it->AsMethodDefinition();
        if (!checkOverriding(methodDef->Function())) {
            return false;
        }
        for (auto *const overload : methodDef->Overloads()) {
            if (!checkOverriding(overload->Function())) {
                return false;
            }
        }
    }

    return true;
}

static bool CheckInterfaceCanGenerateAnonClass(checker::ETSChecker *checker, ir::TSInterfaceDeclaration *interfaceDecl,
                                               ir::ObjectExpression *objectExpr)
{
    checker::Type const *const interfaceType = interfaceDecl->Check(checker);
    if (interfaceType == nullptr || interfaceType->IsTypeError() || interfaceType->AsETSObjectType()->IsGradual()) {
        return false;
    }

    if (!CheckInterface(checker, interfaceDecl, objectExpr)) {
        return false;
    }

    for (auto const *type : interfaceType->AsETSObjectType()->Interfaces()) {
        if (!CheckInterface(checker, type->GetDeclNode()->AsTSInterfaceDeclaration(), objectExpr)) {
            return false;
        }
    }

    return true;
}

static checker::Type *ProcessInterfaceWithMethods(public_lib::Context *ctx, ir::TSInterfaceDeclaration *interfaceDecl,
                                                  ir::ObjectExpression *objectExpr)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *const helperClass = interfaceDecl->GetAnonClass();

    if (objectExpr->HasMethodDefinition()) {
        //  If object literal has method [re-]definition(s) create unique auxilary class for it.
        if (helperClass != nullptr || CheckInterfaceCanGenerateAnonClass(checker, interfaceDecl, objectExpr)) {
            return GenerateAnonClassFromInterfaceWithMethods(ctx, interfaceDecl, objectExpr);
        }
    } else {
        if (helperClass != nullptr) {
            return helperClass->Definition()->TsType();
        }
        // because of lazy checker auxilary classes can be no created here
        interfaceDecl->Check(checker);
        if (CheckInterfaceShouldGenerateAnonClass(checker, interfaceDecl)) {
            return GenerateAnonClassFromInterface(ctx, interfaceDecl);
        }
    }

    checker->LogError(diagnostic::INTERFACE_WITH_METHOD, {}, interfaceDecl->Start());
    return checker->GlobalTypeError();
}

//==========[ Processing of object interface literals with re-defined methods =>  end  ]==========//

static checker::Type *ProcessDeclNode(public_lib::Context *ctx, checker::ETSObjectType *targetType,
                                      ir::ObjectExpression *objExpr)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *declNode = targetType->GetDeclNode();

    if (declNode->IsTSInterfaceDeclaration()) {
        return ProcessInterfaceWithMethods(ctx, declNode->AsTSInterfaceDeclaration(), objExpr);
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
        ES2PANDA_UNREACHABLE();
    }
    return classDef->GetAnonClass()->Definition()->TsType();
}

static void HandleInterfaceLowering(public_lib::Context *ctx, ir::ObjectExpression *objExpr)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *targetType = objExpr->TsType()->AsETSObjectType();
    checker->CheckObjectLiteralKeys(objExpr->Properties());

    checker::Type *resultType = ProcessDeclNode(ctx, targetType, objExpr);

    if (resultType->IsTypeError()) {
        objExpr->SetTsType(resultType);
        return;
    }

    if (!targetType->TypeArguments().empty()) {
        ArenaVector<checker::Type *> typeArgTypes(targetType->TypeArguments());
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

static void TransfromInterfaceDecl(public_lib::Context *ctx, parser::Program *program,
                                   std::unordered_set<ir::AstNode *> &requiredTypes)
{
    auto const cmode = ctx->config->options->GetCompilationMode();
    bool isLocal = program == ctx->parserProgram || cmode == CompilationMode::GEN_STD_LIB ||
                   (cmode == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE && program->IsGenAbcForExternal());

    auto const isRequired = [&requiredTypes, isLocal](checker::ETSObjectType *type) {
        if (isLocal && (type->GetDeclNode()->IsExported() || type->GetDeclNode()->IsDefaultExported())) {
            return true;
        }
        return requiredTypes.find(type->GetDeclNode()) != requiredTypes.end();
    };

    program->Ast()->IterateRecursivelyPostorder([ctx, program, isRequired](ir::AstNode *ast) -> void {
        if (!ast->IsTyped() || ast->AsTyped()->TsType() == nullptr) {
            return;
        }
        if (ast->IsTSInterfaceDeclaration() &&
            isRequired(ast->AsTSInterfaceDeclaration()->TsType()->AsETSObjectType()) &&
            CheckInterfaceShouldGenerateAnonClass(ctx->GetChecker()->AsETSChecker(), ast->AsTSInterfaceDeclaration())) {
            GenerateAnonClassFromInterface(ctx, ast->AsTSInterfaceDeclaration());
        } else if (ast->IsClassDefinition() && ast != program->GlobalClass() &&
                   ast->AsClassDefinition()->IsAbstract() &&
                   !ast->AsClassDefinition()->TsType()->AsETSObjectType()->IsGradual() &&
                   isRequired(ast->AsClassDefinition()->TsType()->AsETSObjectType()) &&
                   CheckAbstractClassShouldGenerateAnonClass(ast->AsClassDefinition())) {
            GenerateAnonClassFromAbstractClass(ctx, ast->AsClassDefinition());
        }
    });
}

template <typename F>
static void TraverseObjectLiteralExpressions(parser::Program *program, F const &cb)
{
    program->Ast()->IterateRecursivelyPostorder([&cb](ir::AstNode *ast) -> void {
        if (!ast->IsObjectExpression()) {
            return;
        }
        auto objExpr = ast->AsObjectExpression();
        if ((IsInterfaceType(objExpr->TsType()) || IsAbstractClassType(objExpr->TsType())) &&
            !objExpr->TsType()->AsETSObjectType()->IsGradual()) {
            cb(ast->AsObjectExpression());
        }
    });
}

bool InterfaceObjectLiteralLowering::Perform(public_lib::Context *ctx, parser::Program *program)
{
    std::unordered_set<ir::AstNode *> requiredTypes {};

    ForEachCompiledProgram(ctx, [&requiredTypes](parser::Program *prog) {
        TraverseObjectLiteralExpressions(prog, [&requiredTypes](ir::ObjectExpression *expr) {
            requiredTypes.insert(expr->TsType()->AsETSObjectType()->GetDeclNode());
        });
    });

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
            TransfromInterfaceDecl(ctx, extProg, requiredTypes);
            varbinder->SetProgram(savedProgram);
            varbinder->SetRecordTable(savedRecordTable);
            varbinder->ResetTopScope(savedTopScope);
        }
    }

    TransfromInterfaceDecl(ctx, program, requiredTypes);

    ForEachCompiledProgram(ctx, [ctx, &requiredTypes](parser::Program *prog) {
        TraverseObjectLiteralExpressions(prog, [ctx, &requiredTypes](ir::ObjectExpression *expr) {
            (void)requiredTypes;
            ES2PANDA_ASSERT(requiredTypes.find(expr->TsType()->AsETSObjectType()->GetDeclNode()) !=
                            requiredTypes.end());
            HandleInterfaceLowering(ctx, expr);
        });
    });

    return true;
}

}  // namespace ark::es2panda::compiler
