/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#include "checker/types/typeRelation.h"
#include "compiler/lowering/util.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"

namespace ark::es2panda::compiler {

static ir::ETSParameterExpression *AddParam(public_lib::Context *ctx, util::StringView name, ir::TypeNode *type)
{
    auto *paramIdent = ctx->AllocNode<ir::Identifier>(name, ctx->Allocator());
    if (type != nullptr) {
        paramIdent->SetTypeAnnotation(type);
    }
    return ctx->AllocNode<ir::ETSParameterExpression>(paramIdent, false, ctx->Allocator());
}

using ClassInitializerBuilder = std::function<void(ArenaVector<ir::Statement *> *, ArenaVector<ir::Expression *> *)>;
using ClassBuilder = std::function<void(ArenaVector<ir::AstNode *> &)>;

static std::pair<ir::ScriptFunction *, ir::Identifier *> CreateScriptFunction(public_lib::Context *ctx,
                                                                              ClassInitializerBuilder const &builder)
{
    ArenaVector<ir::Statement *> statements(ctx->Allocator()->Adapter());
    ArenaVector<ir::Expression *> params(ctx->Allocator()->Adapter());

    ir::ScriptFunction *func;
    ir::Identifier *id;

    builder(&statements, &params);
    auto *body = ctx->AllocNode<ir::BlockStatement>(ctx->Allocator(), std::move(statements));
    id = ctx->AllocNode<ir::Identifier>(compiler::Signatures::CTOR, ctx->Allocator());
    auto funcSignature = ir::FunctionSignature(nullptr, std::move(params), nullptr);
    func = ctx->AllocNode<ir::ScriptFunction>(
        ctx->Allocator(), ir::ScriptFunction::ScriptFunctionData {body, std::move(funcSignature),
                                                                  ir::ScriptFunctionFlags::CONSTRUCTOR |
                                                                      ir::ScriptFunctionFlags::EXPRESSION,
                                                                  ir::ModifierFlags::PUBLIC});
    ES2PANDA_ASSERT(func != nullptr);
    func->SetIdent(id);

    return std::make_pair(func, id);
}

static ir::MethodDefinition *CreateClassInstanceInitializer(public_lib::Context *ctx,
                                                            const ClassInitializerBuilder &builder)
{
    auto [func, id] = CreateScriptFunction(ctx, builder);

    auto *funcExpr = ctx->AllocNode<ir::FunctionExpression>(func);

    auto *ctor = ctx->AllocNode<ir::MethodDefinition>(ir::MethodDefinitionKind::CONSTRUCTOR,
                                                      id->Clone(ctx->Allocator(), nullptr), funcExpr,
                                                      ir::ModifierFlags::NONE, ctx->Allocator(), false);
    return ctor;
}

static ir::ClassDeclaration *BuildClass(checker::ETSChecker *checker, util::StringView name,
                                        const ClassBuilder &builder)
{
    auto *allocator = checker->ProgramAllocator();
    auto *classId = checker->ProgramAllocNode<ir::Identifier>(name, allocator);

    auto *classDef =
        checker->ProgramAllocNode<ir::ClassDefinition>(allocator, classId, ir::ClassDefinitionModifiers::CLASS_DECL,
                                                       ir::ModifierFlags::NONE, Language(Language::Id::ETS));

    auto *classDecl = checker->ProgramAllocNode<ir::ClassDeclaration>(classDef, allocator);

    auto *const varBinder = checker->VarBinder()->AsETSBinder();
    auto *const program = varBinder->Program();

    program->Ast()->AddStatement(classDecl);
    classDecl->SetParent(program->Ast());

    ES2PANDA_ASSERT(varBinder->CheckRecordTablesConsistency(program));
    varbinder::BoundContext boundCtx(program->GetRecordTable(), classDef);

    ArenaVector<ir::AstNode *> classBody(allocator->Adapter());
    builder(classBody);

    classDef->AddProperties(std::move(classBody));

    compiler::InitScopesPhaseETS::RunExternalNode(classDecl, varBinder);
    varBinder->ResolveReference(classDecl);

    classDecl->Check(checker);

    return classDecl;
}

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
    auto *const parser = ctx->parser->AsETSParser();
    checker::ETSChecker::ClassInitializerBuilder initBuilder =
        [ctx, parser, readonlyFields](ArenaVector<ir::Statement *> *statements, ArenaVector<ir::Expression *> *params) {
            for (auto [anonClassFieldName, paramName, retType] : readonlyFields) {
                ir::ETSParameterExpression *param =
                    AddParam(ctx, paramName, ctx->AllocNode<ir::OpaqueTypeNode>(retType, ctx->Allocator()));
                params->push_back(param);
                auto *paramIdent = ctx->AllocNode<ir::Identifier>(paramName, ctx->Allocator());
                statements->push_back(
                    parser->CreateFormattedStatement("this.@@I1 = @@I2;", anonClassFieldName, paramIdent));
            }
            AddParam(ctx, varbinder::VarBinder::MANDATORY_PARAM_THIS, nullptr);
        };

    return CreateClassInstanceInitializer(ctx, initBuilder);
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
                                        const checker::ETSChecker::ClassBuilder &bodyBuilder, checker::Type *declTsType)
{
    ES2PANDA_ASSERT(decl != nullptr && (decl->IsTSInterfaceDeclaration() || decl->IsClassDefinition()));
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *allocator = ctx->Allocator();

    auto *scope = compiler::NearestScope(decl);
    auto scopeCtx = checker::ScopeContext(checker, scope);
    auto expressionCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(checker->VarBinder(), scope);

    auto *classDecl = BuildClass(checker, className, bodyBuilder);
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
        auto *classImplements =
            ctx->AllocNode<ir::TSClassImplements>(ctx->AllocNode<ir::OpaqueTypeNode>((declTsType), allocator));
        classImplements->SetParent(classDef);
        classDef->EmplaceImplements(classImplements);
        classType->RemoveObjectFlag(checker::ETSObjectFlags::RESOLVED_INTERFACES);
        checker->GetInterfacesOfClass(classType);
    } else {
        classType->SetSuperType(declTsType->AsETSObjectType());
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
    auto *classDecl = GenerateAnonClass(ctx, anonClassName.View(), ifaceNode, classBodyBuilder,
                                        ifaceNode->AsTSInterfaceDeclaration()->TsType());

    if (!classDecl->Definition()->TsType()->AsETSObjectType()->IsGradual()) {
        ifaceNode->SetAnonClass(classDecl);
    }

    return classDecl->Definition()->TsType();
}

static void GenerateAnonClassFromAbstractClass(public_lib::Context *ctx, ir::ClassDefinition *abstractClassNode)
{
    if (abstractClassNode->GetAnonClass() != nullptr) {
        return;
    }

    auto classBodyBuilder = [ctx](ArenaVector<ir::AstNode *> &classBody) -> void {
        checker::ETSChecker::ClassInitializerBuilder initBuilder =
            [ctx]([[maybe_unused]] ArenaVector<ir::Statement *> *statements,
                  [[maybe_unused]] ArenaVector<ir::Expression *> *params) {
                AddParam(ctx, varbinder::VarBinder::MANDATORY_PARAM_THIS, nullptr);
            };

        auto *ctor = CreateClassInstanceInitializer(ctx, initBuilder);
        classBody.emplace_back(ctor);
    };

    auto anonClassName =
        util::UString(GenerateAnonClassName(abstractClassNode->InternalName().Utf8()), ctx->Allocator());
    auto *classDecl = GenerateAnonClass(ctx, anonClassName.View(), abstractClassNode, classBodyBuilder,
                                        abstractClassNode->AsClassDefinition()->TsType());
    if (!classDecl->Definition()->TsType()->AsETSObjectType()->IsGradual()) {
        abstractClassNode->SetAnonClass(classDecl);
    }
}

using InterfaceMethod = std::tuple<util::StringView, checker::Signature *, bool>;
using InterfaceMethods = std::vector<InterfaceMethod>;

static void MethodsHaveBody(checker::TypeRelation *relation, ir::TSInterfaceDeclaration *interfaceDecl,
                            InterfaceMethods &methods)
{
    ES2PANDA_ASSERT(interfaceDecl->Body() != nullptr);

    //  Collect all the methods declared in interface and check if it has default implementation somewhere
    auto const addMethod = [&methods, relation](ir::ScriptFunction const *const function) -> void {
        auto const &name = function->Id()->Name();
        auto *const signature = const_cast<checker::Signature *>(function->Signature());
        auto const hasBody = function->HasBody();

        auto const it = std::find_if(
            methods.begin(), methods.end(), [&name, signature, relation](InterfaceMethod const &item) -> bool {
                return std::get<0U>(item) == name && relation->SignatureIsSupertypeOf(std::get<1U>(item), signature);
            });
        if (it == methods.end()) {
            methods.emplace_back(name, signature, hasBody);
        } else if (hasBody) {
            std::get<2U>(*it) = true;
        }
    };

    for (auto const *const node : interfaceDecl->Body()->Body()) {
        if (node->IsOverloadDeclaration()) {
            continue;
        }

        ES2PANDA_ASSERT(node->IsMethodDefinition());
        auto methodDef = node->AsMethodDefinition();
        ES2PANDA_ASSERT(methodDef->Function());
        if (!methodDef->Function()->IsGetterOrSetter()) {
            addMethod(methodDef->Function());
        }

        for (auto const *const overload : methodDef->Overloads()) {
            ES2PANDA_ASSERT(overload->Function());
            if (!overload->Function()->IsGetterOrSetter()) {
                addMethod(overload->Function());
            }
        }
    }
}

static bool CheckInterfaceShouldGenerateAnonClass(checker::ETSChecker *checker,
                                                  ir::TSInterfaceDeclaration *interfaceDecl)
{
    InterfaceMethods methods {};

    // Iterate through all the implemented interfaces
    auto const checkMethods = [&methods, checker](auto &&self, checker::ETSObjectType const *interfaceType) -> void {
        MethodsHaveBody(checker->Relation(), interfaceType->GetDeclNode()->AsTSInterfaceDeclaration(), methods);

        for (auto const *type : interfaceType->Interfaces()) {
            self(self, type);
        }
    };

    checker::Type const *const iType = interfaceDecl->Check(checker);
    if (iType == nullptr || !iType->IsETSObjectType() || iType->AsETSObjectType()->IsGradual()) {
        return false;
    }

    checkMethods(checkMethods, iType->AsETSObjectType());

    for (auto const &[_1, _2, hasBody] : methods) {
        if (!hasBody) {
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
    auto *classDecl = GenerateAnonClass(ctx, anonClassName.View(), interfaceDecl, classBodyBuilder,
                                        objectExpr->AsObjectExpression()->TsType());

    checker::Type *const classType = classDecl->Definition()->Check(checker);
    return classType->IsETSObjectType() && !classType->AsETSObjectType()->IsGradual() ? classType
                                                                                      : checker->GlobalTypeError();
}

static ArenaVector<ark::es2panda::checker::Signature *> GetInterfaceGenericSignature(checker::ETSObjectType *targetType,
                                                                                     util::StringView name)
{
    if (targetType != nullptr) {
        varbinder::LocalVariable *lv =
            targetType->GetProperty(name, checker::PropertySearchFlags::SEARCH_INSTANCE_FIELD |
                                              checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD |
                                              checker::PropertySearchFlags::SEARCH_INSTANCE_DECL |
                                              checker::PropertySearchFlags::SEARCH_IN_INTERFACES);
        if (lv != nullptr && lv->TsType() != nullptr && lv->TsType()->IsETSFunctionType()) {
            return lv->TsType()->AsETSFunctionType()->CallSignatures();
        }
    }
    return ArenaVector<ark::es2panda::checker::Signature *>();
}

static void CheckInterface(checker::TypeRelation *relation, ir::TSInterfaceDeclaration *interfaceDecl,
                           ir::ObjectExpression *objectExpr, InterfaceMethods &methods)
{
    //  Lambda checks if any method defined in object literal overrides empty method declared in interface.
    auto const checkOverriding = [&methods, objectExpr, relation](ir::ScriptFunction const *const function,
                                                                  checker::Signature *sig) -> void {
        auto const &name = function->Id()->Name();
        auto *const signature = const_cast<checker::Signature *>(function->Signature());
        auto const hasBody = function->HasBody();

        auto it = std::find_if(
            methods.begin(), methods.end(), [&name, signature, relation](InterfaceMethod const &item) -> bool {
                return std::get<0U>(item) == name && relation->SignatureIsSupertypeOf(std::get<1U>(item), signature);
            });
        if (it == methods.end()) {
            methods.emplace_back(name, signature, hasBody);
            it = std::prev(methods.end());
        } else if (hasBody) {
            std::get<2U>(*it) = true;
        }

        if (std::get<2U>(*it)) {
            return;
        }

        for (ir::Expression *propExpr : objectExpr->Properties()) {
            if (!propExpr->IsProperty()) {
                continue;
            }

            if (auto const *const key = propExpr->AsProperty()->Key();
                !key->IsIdentifier() || !key->AsIdentifier()->Name().Is(name.Utf8())) {
                continue;
            }

            checker::SavedTypeRelationFlagsContext savedCtx(relation, checker::TypeRelationFlag::OVERRIDING_CONTEXT);
            checker::Type *const valueType = propExpr->AsProperty()->Value()->TsType();
            if (valueType->IsETSArrowType() &&
                relation->SignatureIsSupertypeOf(sig, valueType->AsETSFunctionType()->ArrowSignature())) {
                std::get<2U>(*it) = true;
            }
        }
    };

    ES2PANDA_ASSERT(interfaceDecl->Body() != nullptr);
    for (auto const *const node : interfaceDecl->Body()->Body()) {
        if (node->IsOverloadDeclaration()) {
            continue;
        }
        auto methodDef = node->AsMethodDefinition();
        auto targetType = objectExpr->TsType()->AsETSObjectType();
        auto signatures = GetInterfaceGenericSignature(targetType, methodDef->Key()->AsIdentifier()->Name());

        for (auto sig : signatures) {
            if (!methodDef->Function()->IsGetterOrSetter()) {
                checkOverriding(methodDef->Function(), sig);
            }
        }
    }
}

static bool CheckInterfaceCanGenerateAnonClass(checker::ETSChecker *checker, ir::TSInterfaceDeclaration *interfaceDecl,
                                               ir::ObjectExpression *objectExpr)
{
    InterfaceMethods methods {};

    // Iterate through all the implemented interfaces
    auto const checkMethods = [&methods, objectExpr, checker](auto &&self,
                                                              checker::ETSObjectType const *interfaceType) -> void {
        CheckInterface(checker->Relation(), interfaceType->GetDeclNode()->AsTSInterfaceDeclaration(), objectExpr,
                       methods);

        for (auto const *type : interfaceType->Interfaces()) {
            self(self, type);
        }
    };

    checker::Type const *const iType = interfaceDecl->Check(checker);
    if (iType == nullptr || !iType->IsETSObjectType() || iType->AsETSObjectType()->IsGradual()) {
        return false;
    }

    checkMethods(checkMethods, iType->AsETSObjectType());

    for (auto const &[_1, _2, hasBody] : methods) {
        if (!hasBody) {
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

static checker::Type *GenerateAnonClassFromAbstractClassWithMethods(public_lib::Context *ctx,
                                                                    ir::ClassDefinition *abstractClassNode,
                                                                    ir::ObjectExpression *objectExpr)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();

    auto classBodyBuilder = [ctx, objectExpr](ArenaVector<ir::AstNode *> &classBody) -> void {
        AddMethodsFromLiteral(ctx, classBody, objectExpr);

        checker::ETSChecker::ClassInitializerBuilder initBuilder =
            [ctx]([[maybe_unused]] ArenaVector<ir::Statement *> *statements,
                  [[maybe_unused]] ArenaVector<ir::Expression *> *params) {
                AddParam(ctx, varbinder::VarBinder::MANDATORY_PARAM_THIS, nullptr);
            };

        auto *ctor = CreateClassInstanceInitializer(ctx, initBuilder);
        classBody.emplace_back(ctor);
    };

    auto anonClassName =
        util::UString(GenerateAnonClassName(abstractClassNode->InternalName().Utf8(), true), ctx->Allocator());
    auto *classDecl = GenerateAnonClass(ctx, anonClassName.View(), abstractClassNode, classBodyBuilder,
                                        abstractClassNode->AsClassDefinition()->TsType());

    checker::Type *const classType = classDecl->Definition()->Check(checker);
    return classType->IsETSObjectType() && !classType->AsETSObjectType()->IsGradual() ? classType
                                                                                      : checker->GlobalTypeError();
}

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

    if (objExpr->HasMethodDefinition()) {
        return GenerateAnonClassFromAbstractClassWithMethods(ctx, classDef, objExpr);
    }

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
                                   const std::unordered_set<ir::AstNode *> &requiredTypes)
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

bool InterfaceObjectLiteralLowering::Perform()
{
    std::unordered_set<ir::AstNode *> requiredTypes {};
    auto ctx = Context();

    ProgramsToBeEmittedSelector::Apply(ctx, [&requiredTypes](parser::Program *prog) {
        TraverseObjectLiteralExpressions(prog, [&requiredTypes](ir::ObjectExpression *expr) {
            requiredTypes.insert(expr->TsType()->AsETSObjectType()->GetDeclNode());
        });
    });

    auto *varbinder = ctx->parserProgram->VarBinder()->AsETSBinder();
    auto *savedProgram = varbinder->Program();
    ES2PANDA_ASSERT(savedProgram == ctx->parserProgram);
    auto *savedRecordTable = varbinder->GetRecordTable();
    auto *savedTopScope = varbinder->TopScope();
    ctx->parserProgram->GetExternalSources()->Visit([ctx, varbinder, &requiredTypes](auto *extProg) {
        if (extProg->IsASTLowered()) {
            return;
        }
        varbinder->ResetTopScope(extProg->GlobalScope());
        ES2PANDA_ASSERT(varbinder->CheckRecordTablesConsistency(extProg));
        varbinder->SetRecordTable(extProg->GetRecordTable());
        varbinder->SetProgram(extProg);
        TransfromInterfaceDecl(ctx, extProg, requiredTypes);
    });
    varbinder->SetProgram(savedProgram);
    varbinder->SetRecordTable(savedRecordTable);
    varbinder->ResetTopScope(savedTopScope);
    TransfromInterfaceDecl(ctx, savedProgram, requiredTypes);

    ProgramsToBeEmittedSelector::Apply(ctx, [ctx, &requiredTypes](parser::Program *prog) {
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
