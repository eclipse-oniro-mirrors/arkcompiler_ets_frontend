/*
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

#include "enumLowering.h"

#include "checker/ETSchecker.h"
#include "checker/types/type.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

namespace {

[[nodiscard]] ir::ETSParameterExpression *MakeFunctionParam(public_lib::Context *ctx, const util::StringView &name,
                                                            ir::TypeNode *const typeAnnotation)
{
    auto *const paramIdent = ctx->AllocNode<ir::Identifier>(name, typeAnnotation, ctx->Allocator());
    paramIdent->SetRange(typeAnnotation->Range());
    auto *const param = ctx->AllocNode<ir::ETSParameterExpression>(paramIdent, false, ctx->Allocator());

    return param;
}

[[nodiscard]] ir::Identifier *MakeParamRefIdent(public_lib::Context *ctx, ir::ETSParameterExpression *paramExpr)
{
    auto *const refIdent = ctx->AllocNode<ir::Identifier>(paramExpr->Ident()->Name(), ctx->Allocator());
    ES2PANDA_ASSERT(refIdent);
    refIdent->SetRange(paramExpr->Ident()->Range());
    refIdent->SetVariable(paramExpr->Ident()->Variable());
    return refIdent;
}

[[nodiscard]] ir::ETSTypeReference *MakeTypeReference(public_lib::Context *ctx, const util::StringView &name)
{
    auto *const ident = ctx->AllocNode<ir::Identifier>(name, ctx->Allocator());
    auto *const referencePart = ctx->AllocNode<ir::ETSTypeReferencePart>(ident, ctx->Allocator());
    return ctx->AllocNode<ir::ETSTypeReference>(referencePart, ctx->Allocator());
}

ir::MethodDefinition *MakeMethodDef(public_lib::Context *ctx, ir::ClassDefinition *enumClass,
                                    ir::Identifier *const ident, ir::ScriptFunction *const function)
{
    auto *const functionExpr = ctx->AllocNode<ir::FunctionExpression>(function);
    auto *const identClone = ident->Clone(ctx->Allocator(), nullptr);

    auto *const methodDef = ctx->AllocNode<ir::MethodDefinition>(
        ir::MethodDefinitionKind::METHOD, identClone, functionExpr, function->Modifiers(), ctx->Allocator(), false);
    methodDef->SetParent(enumClass);
    enumClass->EmplaceBody(methodDef);

    return methodDef;
}

}  // namespace

void EnumLoweringPhase::LogError(const diagnostic::DiagnosticKind &diagnostic,
                                 const util::DiagnosticMessageParams &diagnosticParams,
                                 const lexer::SourcePosition &pos)
{
    context_->diagnosticEngine->LogDiagnostic(diagnostic, diagnosticParams, pos);
}

template <EnumLoweringPhase::EnumType TYPE_NODE>
bool EnumLoweringPhase::CheckEnumMemberType(const ArenaVector<ir::AstNode *> &enumMembers, bool &hasLoggedError,
                                            bool &hasLongLiteral)
{
    for (auto *member : enumMembers) {
        auto *init = member->AsTSEnumMember()->Init();
        if constexpr (TYPE_NODE == EnumLoweringPhase::EnumType::INT) {
            if (!init->IsNumberLiteral() || !init->AsNumberLiteral()->Number().IsInteger()) {
                LogError(diagnostic::ERROR_ARKTS_NO_ENUM_MIXED_TYPES, {}, init->Start());
                hasLoggedError = true;
                continue;
            }

            if (!init->AsNumberLiteral()->Number().IsLong()) {
                continue;
            }

            hasLongLiteral = true;
            // Invalid generated value.
            if (member->AsTSEnumMember()->IsGenerated() &&
                init->AsNumberLiteral()->Number().GetLong() == std::numeric_limits<int64_t>::min()) {
                LogError(diagnostic::ERROR_ARKTS_NO_ENUM_MIXED_TYPES, {}, init->Start());
                hasLoggedError = true;
            }
        } else if constexpr (TYPE_NODE == EnumLoweringPhase::EnumType::DOUBLE) {
            if (!init->IsNumberLiteral()) {
                LogError(diagnostic::ERROR_ARKTS_NO_ENUM_MIXED_TYPES, {}, init->Start());
                hasLoggedError = true;
                continue;
            }
            if (member->AsTSEnumMember()->IsGenerated()) {
                LogError(diagnostic::ERROR_ARKTS_NO_ENUM_MIXED_TYPES, {}, init->Start());
                hasLoggedError = true;
            }
        } else if constexpr (TYPE_NODE == EnumLoweringPhase::EnumType::STRING) {
            if (!init->IsStringLiteral()) {
                LogError(diagnostic::ERROR_ARKTS_NO_ENUM_MIXED_TYPES, {}, init->Start());
                hasLoggedError = true;
            }
            if (member->AsTSEnumMember()->IsGenerated()) {
                LogError(diagnostic::ENUM_STRING_TYPE_ALL_ITEMS_INIT, {}, init->Start());
                hasLoggedError = true;
            }
        } else {
            static_assert(TYPE_NODE ==
                          EnumLoweringPhase::EnumType::NOT_SPECIFIED);  // Unsupported TypeNode in CheckEnumMemberType.
        }
    }
    return !hasLoggedError;
}

[[nodiscard]] ir::ScriptFunction *EnumLoweringPhase::MakeFunction(FunctionInfo &&functionInfo)
{
    ir::BlockStatement *bodyBlock = nullptr;

    if (functionInfo.enumDecl->IsDeclare()) {
        functionInfo.flags |= ir::ModifierFlags::DECLARE;
    } else {
        bodyBlock = AllocNode<ir::BlockStatement>(Allocator(), std::move(functionInfo.body));
    }
    // clang-format off
    auto *const function = AllocNode<ir::ScriptFunction>(
        Allocator(), ir::ScriptFunction::ScriptFunctionData {
            bodyBlock,
            ir::FunctionSignature(nullptr, std::move(functionInfo.params), functionInfo.returnTypeAnnotation),
            ir::ScriptFunctionFlags::METHOD, functionInfo.flags});
    // clang-format on

    return function;
}

template <typename ElementMaker>
[[nodiscard]] ir::Identifier *EnumLoweringPhase::MakeArray(const ir::TSEnumDeclaration *const enumDecl,
                                                           ir::ClassDefinition *const enumClass,
                                                           const util::StringView &name,
                                                           ir::TypeNode *const typeAnnotation,
                                                           ElementMaker &&elementMaker)
{
    ArenaVector<ir::Expression *> elements(Allocator()->Adapter());
    elements.reserve(enumDecl->Members().size());
    for (const auto *const member : enumDecl->Members()) {
        elements.push_back(elementMaker(member->AsTSEnumMember()));
    }
    auto *const arrayExpr = AllocNode<ir::ArrayExpression>(std::move(elements), Allocator());
    auto *const arrayIdent = AllocNode<ir::Identifier>(name, Allocator());
    auto *const arrayClassProp = AllocNode<ir::ClassProperty>(
        arrayIdent, arrayExpr, typeAnnotation,
        ir::ModifierFlags::STATIC | ir::ModifierFlags::PRIVATE | ir::ModifierFlags::READONLY, Allocator(), false);
    ES2PANDA_ASSERT(arrayClassProp != nullptr);
    arrayClassProp->SetParent(enumClass);
    enumClass->EmplaceBody(arrayClassProp);

    return arrayIdent;
}

ir::Expression *EnumLoweringPhase::CheckEnumTypeForItemFields(EnumType enumType, ir::TSEnumMember *const member)
{
    ir::Expression *valueArgument = nullptr;

    switch (enumType) {
        case EnumType::INT: {
            auto enumFieldValue =
                member->AsTSEnumMember()->Init()->AsNumberLiteral()->Number().GetValue<std::int32_t>();
            valueArgument = AllocNode<ir::NumberLiteral>(lexer::Number(enumFieldValue));
            break;
        }
        case EnumType::LONG: {
            auto enumFieldValue =
                member->AsTSEnumMember()->Init()->AsNumberLiteral()->Number().GetValue<std::int64_t>();
            valueArgument = AllocNode<ir::NumberLiteral>(lexer::Number(enumFieldValue));
            break;
        }
        case EnumType::DOUBLE: {
            auto enumFieldValue = member->AsTSEnumMember()->Init()->AsNumberLiteral()->Number().GetValue<double>();
            valueArgument = AllocNode<ir::NumberLiteral>(lexer::Number(enumFieldValue));
            break;
        }
        case EnumType::STRING: {
            auto enumFieldValue = member->AsTSEnumMember()->Init()->AsStringLiteral()->Str();
            valueArgument = AllocNode<ir::StringLiteral>(enumFieldValue);
            break;
        }
        case EnumType::NOT_SPECIFIED: {
            LogError(diagnostic::ENUM_INVALID_INIT, {}, member->AsTSEnumMember()->Init()->Start());
            break;
        }
    }

    return valueArgument;
}

void EnumLoweringPhase::CreateEnumItemFields(const ir::TSEnumDeclaration *const enumDecl,
                                             ir::ClassDefinition *const enumClass, EnumType enumType)
{
    static_assert(ORDINAL_TYPE == ir::PrimitiveType::INT);
    int32_t ordinal = 0;
    auto createEnumItemField = [this, enumClass, enumType, &ordinal](ir::TSEnumMember *const member) {
        auto *const enumMemberIdent =
            AllocNode<ir::Identifier>(member->AsTSEnumMember()->Key()->AsIdentifier()->Name(), Allocator());
        auto *const enumTypeAnnotation = MakeTypeReference(context_, enumClass->Ident()->Name());

        auto *const ordinalLiteral = AllocNode<ir::NumberLiteral>(lexer::Number(ordinal));
        ordinal++;
        ArenaVector<ir::Expression *> newExprArgs(Allocator()->Adapter());
        newExprArgs.push_back(ordinalLiteral);

        ir::Expression *valueArgument = CheckEnumTypeForItemFields(enumType, member);

        newExprArgs.push_back(valueArgument);

        auto enumTypeAnnotation1 = enumTypeAnnotation->Clone(Allocator(), nullptr);
        auto *const newExpression =
            AllocNode<ir::ETSNewClassInstanceExpression>(enumTypeAnnotation1, std::move(newExprArgs));

        auto *field = AllocNode<ir::ClassProperty>(
            enumMemberIdent, newExpression, enumTypeAnnotation,
            ir::ModifierFlags::PUBLIC | ir::ModifierFlags::STATIC | ir::ModifierFlags::READONLY, Allocator(), false);
        enumMemberIdent->SetRange(member->Key()->Range());
        newExpression->SetRange(member->Init()->Range());
        field->SetRange(member->Range());
        field->SetOrigEnumMember(member->AsTSEnumMember());
        field->SetParent(enumClass);
        return field;
    };
    for (auto *const member : enumDecl->Members()) {
        enumClass->EmplaceBody(createEnumItemField(member->AsTSEnumMember()));
    }
}

ir::Identifier *EnumLoweringPhase::CreateEnumNamesArray(const ir::TSEnumDeclaration *const enumDecl,
                                                        ir::ClassDefinition *const enumClass)
{
    auto *const stringTypeAnnotation = MakeTypeReference(context_, STRING_REFERENCE_TYPE);  // NOTE String -> Builtin?
    auto *const arrayTypeAnnotation = AllocNode<ir::TSArrayType>(stringTypeAnnotation, Allocator());

    // clang-format off
    return MakeArray(enumDecl, enumClass, NAMES_ARRAY_NAME, arrayTypeAnnotation,
                     [this](const ir::TSEnumMember *const member) {
                        auto *const enumNameStringLiteral =
                            AllocNode<ir::StringLiteral>(member->Key()->AsIdentifier()->Name());
                        return enumNameStringLiteral;
                    });
    // clang-format on
}

static ir::TypeNode *CreateType(public_lib::Context *ctx, EnumLoweringPhase::EnumType enumType)
{
    switch (enumType) {
        case EnumLoweringPhase::EnumType::INT: {
            return ctx->AllocNode<ir::ETSPrimitiveType>(ir::PrimitiveType::INT, ctx->Allocator());
        }
        case EnumLoweringPhase::EnumType::LONG: {
            return ctx->AllocNode<ir::ETSPrimitiveType>(ir::PrimitiveType::LONG, ctx->Allocator());
        }
        case EnumLoweringPhase::EnumType::DOUBLE: {
            return ctx->AllocNode<ir::ETSPrimitiveType>(ir::PrimitiveType::DOUBLE, ctx->Allocator());
        }
        case EnumLoweringPhase::EnumType::STRING: {
            return MakeTypeReference(ctx, EnumLoweringPhase::STRING_REFERENCE_TYPE);
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    return nullptr;
}

ir::ClassDeclaration *EnumLoweringPhase::CreateClass(ir::TSEnumDeclaration *const enumDecl,
                                                     const DeclarationFlags flags, EnumType enumType)
{
    auto *ident = Allocator()->New<ir::Identifier>(enumDecl->Key()->Name(), Allocator());
    ident->SetRange(enumDecl->Key()->Range());

    auto baseClassDefinitionFlag = ir::ClassDefinitionModifiers::CLASS_DECL;
    if (enumType == EnumType::INT || enumType == EnumType::LONG) {
        baseClassDefinitionFlag |= ir::ClassDefinitionModifiers::INT_ENUM_TRANSFORMED;
    } else if (enumType == EnumType::DOUBLE) {
        baseClassDefinitionFlag |= ir::ClassDefinitionModifiers::DOUBLE_ENUM_TRANSFORMED;
    } else if (enumType == EnumType::STRING) {
        baseClassDefinitionFlag |= ir::ClassDefinitionModifiers::STRING_ENUM_TRANSFORMED;
    } else if (enumType == EnumType::NOT_SPECIFIED) {
        ES2PANDA_UNREACHABLE();
    }

    auto typeParamsVector = ArenaVector<ir::TypeNode *>(Allocator()->Adapter());
    typeParamsVector.push_back(CreateType(context_, enumType));
    auto *typeParam = AllocNode<ir::TSTypeParameterInstantiation>(std::move(typeParamsVector));

    auto *identRef = AllocNode<ir::Identifier>(util::StringView(BASE_CLASS_NAME), Allocator());
    auto *typeRefPart = AllocNode<ir::ETSTypeReferencePart>(identRef, typeParam, nullptr, Allocator());
    auto *superClass = AllocNode<ir::ETSTypeReference>(typeRefPart, Allocator());

    auto *classDef = AllocNode<ir::ClassDefinition>(
        Allocator(), ident,
        flags.isLocal ? baseClassDefinitionFlag | ir::ClassDefinitionModifiers::LOCAL : baseClassDefinitionFlag,
        enumDecl->IsDeclare() ? ir::ModifierFlags::FINAL | ir::ModifierFlags::DECLARE : ir::ModifierFlags::FINAL,
        enumDecl->Language());

    classDef->SetSuper(superClass);
    auto *classDecl = AllocNode<ir::ClassDeclaration>(classDef, Allocator());

    if (enumDecl->IsExported()) {
        classDecl->AddModifier(ir::ModifierFlags::EXPORT);
        program_->GlobalClass()->AddToExportedClasses(classDecl);
    } else if (enumDecl->IsDefaultExported()) {
        classDecl->AddModifier(ir::ModifierFlags::DEFAULT_EXPORT);
    } else if (enumDecl->HasExportAlias()) {
        classDecl->AddAstNodeFlags(ir::AstNodeFlags::HAS_EXPORT_ALIAS);
    }

    classDef->SetOrigEnumDecl(enumDecl);

    CreateOrdinalField(classDef);
    if (!enumDecl->IsDeclare()) {
        CreateCCtorForEnumClass(classDef);
    }
    CreateCtorForEnumClass(classDef, enumType);
    classDecl->SetRange(enumDecl->Range());

    return classDecl;
}

void EnumLoweringPhase::CreateCCtorForEnumClass(ir::ClassDefinition *const enumClass)
{
    ArenaVector<ir::Expression *> params(Allocator()->Adapter());
    auto *id = AllocNode<ir::Identifier>(compiler::Signatures::CCTOR, Allocator());

    ArenaVector<ir::Statement *> statements(Allocator()->Adapter());

    auto *body = AllocNode<ir::BlockStatement>(Allocator(), std::move(statements));
    auto *func = AllocNode<ir::ScriptFunction>(
        Allocator(),
        ir::ScriptFunction::ScriptFunctionData {body, ir::FunctionSignature(nullptr, std::move(params), nullptr),
                                                ir::ScriptFunctionFlags::STATIC_BLOCK | ir::ScriptFunctionFlags::HIDDEN,
                                                ir::ModifierFlags::STATIC, Language(Language::Id::ETS)});

    ES2PANDA_ASSERT(func != nullptr);
    func->SetIdent(id);

    auto *funcExpr = AllocNode<ir::FunctionExpression>(func);

    auto *const identClone = id->Clone(Allocator(), nullptr);
    auto *const methodDef =
        AllocNode<ir::MethodDefinition>(ir::MethodDefinitionKind::METHOD, identClone, funcExpr,
                                        ir::ModifierFlags::PRIVATE | ir::ModifierFlags::STATIC, Allocator(), false);
    ES2PANDA_ASSERT(methodDef != nullptr);
    methodDef->SetParent(enumClass);
    enumClass->EmplaceBody(methodDef);
}

ir::ClassProperty *EnumLoweringPhase::CreateOrdinalField(ir::ClassDefinition *const enumClass)
{
    auto *const fieldIdent = Allocator()->New<ir::Identifier>(ORDINAL_NAME, Allocator());
    auto *const intTypeAnnotation = Allocator()->New<ir::ETSPrimitiveType>(ORDINAL_TYPE, Allocator());
    auto *field =
        AllocNode<ir::ClassProperty>(fieldIdent, nullptr, intTypeAnnotation,
                                     ir::ModifierFlags::PRIVATE | ir::ModifierFlags::READONLY, Allocator(), false);

    enumClass->EmplaceBody(field);
    field->SetParent(enumClass);
    return field;
}

ir::ScriptFunction *EnumLoweringPhase::CreateFunctionForCtorOfEnumClass(ir::ClassDefinition *const enumClass,
                                                                        EnumType enumType)
{
    ArenaVector<ir::Expression *> params(Allocator()->Adapter());

    auto *const intTypeAnnotation = AllocNode<ir::ETSPrimitiveType>(ORDINAL_TYPE, Allocator());
    auto *const inputOrdinalParam = MakeFunctionParam(context_, PARAM_ORDINAL, intTypeAnnotation);
    params.push_back(inputOrdinalParam);

    ir::TypeNode *typeAnnotation = CreateType(context_, enumType);
    auto *const inputValueParam = MakeFunctionParam(context_, PARAM_VALUE, typeAnnotation);
    params.push_back(inputValueParam);

    auto *id = AllocNode<ir::Identifier>("constructor", Allocator());
    ArenaVector<ir::Statement *> statements(Allocator()->Adapter());

    auto *body = AllocNode<ir::BlockStatement>(Allocator(), std::move(statements));

    auto scriptFlags = ir::ScriptFunctionFlags::CONSTRUCTOR;
    scriptFlags |= enumClass->IsDeclare() ? ir::ScriptFunctionFlags::EXTERNAL : ir::ScriptFunctionFlags::NONE;

    auto *func = AllocNode<ir::ScriptFunction>(
        Allocator(),
        ir::ScriptFunction::ScriptFunctionData {body, ir::FunctionSignature(nullptr, std::move(params), nullptr),
                                                scriptFlags,  // CC-OFF(G.FMT.02) project code style
                                                ir::ModifierFlags::CONSTRUCTOR |
                                                    ir::ModifierFlags::PUBLIC,  // CC-OFF(G.FMT.02) project code style
                                                Language(Language::Id::ETS)});  // CC-OFF(G.FMT.02) project code style

    func->SetIdent(id);

    if (enumClass->IsDeclare()) {
        // NOTE: In aliveAnalyzer call to super is processed and leads to error. Need to invetigate it
        return func;
    }

    auto *valueIdentifier = AllocNode<ir::Identifier>(PARAM_VALUE, Allocator());
    valueIdentifier->SetVariable(inputValueParam->Ident()->Variable());

    ArenaVector<ir::Expression *> callArguments(Allocator()->Adapter());
    auto *callee = AllocNode<ir::SuperExpression>();
    callArguments.push_back(valueIdentifier);
    auto *superConstructorCall = AllocNode<ir::CallExpression>(callee, std::move(callArguments), nullptr, false);
    auto *superCallStatement = AllocNode<ir::ExpressionStatement>(superConstructorCall);
    superCallStatement->SetParent(body);
    body->AddStatement(superCallStatement);

    auto *thisExpr = Allocator()->New<ir::ThisExpression>();
    auto *fieldIdentifier = Allocator()->New<ir::Identifier>(ORDINAL_NAME, Allocator());
    auto *leftHandSide = AllocNode<ir::MemberExpression>(thisExpr, fieldIdentifier,
                                                         ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    auto *rightHandSide = AllocNode<ir::Identifier>(PARAM_ORDINAL, Allocator());
    rightHandSide->SetVariable(inputOrdinalParam->Ident()->Variable());
    if (!enumClass->IsDeclare()) {
        auto *initializer =
            AllocNode<ir::AssignmentExpression>(leftHandSide, rightHandSide, lexer::TokenType::PUNCTUATOR_SUBSTITUTION);
        auto initStatement = AllocNode<ir::ExpressionStatement>(initializer);
        initStatement->SetParent(body);
        body->AddStatement(initStatement);
    }

    return func;
}

void EnumLoweringPhase::CreateCtorForEnumClass(ir::ClassDefinition *const enumClass, EnumType enumType)
{
    auto *func = CreateFunctionForCtorOfEnumClass(enumClass, enumType);
    auto *funcExpr = AllocNode<ir::FunctionExpression>(func);

    auto *const identClone = func->Id()->Clone(Allocator(), nullptr);
    // NOTE(gogabr): constructor should be private, but interop_js complains, see test_js_use_ets_enum.ts
    auto *const methodDef = AllocNode<ir::MethodDefinition>(ir::MethodDefinitionKind::CONSTRUCTOR, identClone, funcExpr,
                                                            ir::ModifierFlags::PUBLIC, Allocator(), false);
    methodDef->SetParent(enumClass);
    enumClass->EmplaceBody(methodDef);
}

void EnumLoweringPhase::ProcessEnumClassDeclaration(ir::TSEnumDeclaration *const enumDecl,
                                                    const DeclarationFlags &flags, ir::ClassDeclaration *enumClassDecl)
{
    varbinder::Variable *var = nullptr;
    auto *ident = enumClassDecl->Definition()->Ident();
    if (flags.isLocal) {
        auto *scope = NearestScope(enumDecl->Parent());
        ES2PANDA_ASSERT(scope);
        auto localCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder_, scope);
        ES2PANDA_ASSERT(scope);
        scope->EraseBinding(ident->Name());
        InitScopesPhaseETS::RunExternalNode(enumClassDecl, varbinder_);
        var = varbinder_->GetScope()->FindLocal(ident->Name(), varbinder::ResolveBindingOptions::ALL);
    } else if (flags.isTopLevel) {
        auto *scope = program_->GlobalClassScope();
        ES2PANDA_ASSERT(scope);
        auto localCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder_, scope);
        ES2PANDA_ASSERT(scope);
        scope->StaticDeclScope()->EraseBinding(ident->Name());
        InitScopesPhaseETS::RunExternalNode(enumClassDecl, varbinder_);
        var = varbinder_->GetScope()->FindLocal(ident->Name(), varbinder::ResolveBindingOptions::ALL);
        if (var != nullptr) {
            program_->GlobalScope()->InsertBinding(ident->Name(), var);
        }
    } else if (flags.isNamespace) {
        auto *scope = enumDecl->Parent()->Scope();
        auto localCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder_, scope);
        scope->AsClassScope()->StaticDeclScope()->EraseBinding(ident->Name());
        InitScopesPhaseETS::RunExternalNode(enumClassDecl, varbinder_);
        var = varbinder_->GetScope()->FindLocal(ident->Name(), varbinder::ResolveBindingOptions::DECLARATION);
    } else {
        ES2PANDA_UNREACHABLE();
    }
    if (var != nullptr) {
        // Although it enum was transformed to class, it should still be regarded as enum.
        var->RemoveFlag(varbinder::VariableFlags::CLASS);
        var->AddFlag(varbinder::VariableFlags::ENUM_LITERAL);
    }
}

template <ir::PrimitiveType TYPE>
ir::ClassDeclaration *EnumLoweringPhase::CreateEnumIntClassFromEnumDeclaration(ir::TSEnumDeclaration *const enumDecl,
                                                                               const DeclarationFlags flags)
{
    EnumType enumType = EnumType::INT;
    if constexpr (TYPE == ir::PrimitiveType::LONG) {
        enumType = EnumType::LONG;
    }

    auto *const enumClassDecl = CreateClass(enumDecl, flags, enumType);
    auto *const enumClass = enumClassDecl->Definition();

    CreateEnumItemFields(enumDecl, enumClass, enumType);
    auto *const namesArrayIdent = CreateEnumNamesArray(enumDecl, enumClass);
    auto *const valuesArrayIdent = CreateEnumValuesArray<TYPE>(enumDecl, enumClass);
    auto *const stringValuesArrayIdent = CreateEnumStringValuesArray(enumDecl, enumClass);
    auto *const itemsArrayIdent = CreateEnumItemsArray(enumDecl, enumClass);

    CreateEnumGetNameMethod(enumDecl, enumClass, namesArrayIdent);

    CreateEnumGetValueOfMethod(enumDecl, enumClass, namesArrayIdent, itemsArrayIdent);

    CreateEnumFromValueMethod(enumDecl, enumClass, valuesArrayIdent, itemsArrayIdent, TYPE);

    CreateEnumValueOfMethod(enumDecl, enumClass, valuesArrayIdent, TYPE);

    CreateEnumToStringMethod(enumDecl, enumClass, stringValuesArrayIdent);

    CreateEnumValuesMethod(enumDecl, enumClass, itemsArrayIdent);

    CreateEnumGetOrdinalMethod(enumDecl, enumClass);

    CreateEnumDollarGetMethod(enumDecl, enumClass);

    SetDefaultPositionInUnfilledClassNodes(enumClassDecl, enumDecl);

    enumClassDecl->SetParent(enumDecl->Parent());
    ProcessEnumClassDeclaration(enumDecl, flags, enumClassDecl);

    return enumClassDecl;
}

template <ir::PrimitiveType TYPE>
ir::ClassDeclaration *EnumLoweringPhase::CreateEnumFloatClassFromEnumDeclaration(ir::TSEnumDeclaration *const enumDecl,
                                                                                 const DeclarationFlags flags)
{
    EnumType enumType = EnumType::DOUBLE;

    auto *const enumClassDecl = CreateClass(enumDecl, flags, enumType);
    auto *const enumClass = enumClassDecl->Definition();

    CreateEnumItemFields(enumDecl, enumClass, enumType);
    auto *const namesArrayIdent = CreateEnumNamesArray(enumDecl, enumClass);
    auto *const valuesArrayIdent = CreateEnumValuesArray<TYPE>(enumDecl, enumClass);
    auto *const stringValuesArrayIdent = CreateEnumStringValuesArray(enumDecl, enumClass);
    auto *const itemsArrayIdent = CreateEnumItemsArray(enumDecl, enumClass);

    CreateEnumGetNameMethod(enumDecl, enumClass, namesArrayIdent);

    CreateEnumGetValueOfMethod(enumDecl, enumClass, namesArrayIdent, itemsArrayIdent);

    CreateEnumFromValueMethod(enumDecl, enumClass, valuesArrayIdent, itemsArrayIdent, TYPE);

    CreateEnumValueOfMethod(enumDecl, enumClass, valuesArrayIdent, TYPE);

    CreateEnumToStringMethod(enumDecl, enumClass, stringValuesArrayIdent);

    CreateEnumValuesMethod(enumDecl, enumClass, itemsArrayIdent);

    CreateEnumGetOrdinalMethod(enumDecl, enumClass);

    CreateEnumDollarGetMethod(enumDecl, enumClass);

    SetDefaultPositionInUnfilledClassNodes(enumClassDecl, enumDecl);

    enumClassDecl->SetParent(enumDecl->Parent());
    ProcessEnumClassDeclaration(enumDecl, flags, enumClassDecl);

    return enumClassDecl;
}

ir::ClassDeclaration *EnumLoweringPhase::CreateEnumStringClassFromEnumDeclaration(ir::TSEnumDeclaration *const enumDecl,
                                                                                  const DeclarationFlags flags)
{
    auto *const enumClassDecl = CreateClass(enumDecl, flags, EnumType::STRING);
    auto *const enumClass = enumClassDecl->Definition();

    CreateEnumItemFields(enumDecl, enumClass, EnumType::STRING);
    auto *const namesArrayIdent = CreateEnumNamesArray(enumDecl, enumClass);
    auto *const stringValuesArrayIdent = CreateEnumStringValuesArray(enumDecl, enumClass);
    auto *const itemsArrayIdent = CreateEnumItemsArray(enumDecl, enumClass);

    CreateEnumGetNameMethod(enumDecl, enumClass, namesArrayIdent);

    CreateEnumGetValueOfMethod(enumDecl, enumClass, namesArrayIdent, itemsArrayIdent);

    CreateEnumFromValueMethod(enumDecl, enumClass, stringValuesArrayIdent, itemsArrayIdent, std::nullopt);

    CreateEnumValueOfMethod(enumDecl, enumClass, stringValuesArrayIdent, std::nullopt);

    CreateEnumToStringMethod(enumDecl, enumClass, stringValuesArrayIdent);

    CreateEnumValuesMethod(enumDecl, enumClass, itemsArrayIdent);

    CreateEnumGetOrdinalMethod(enumDecl, enumClass);

    CreateEnumDollarGetMethod(enumDecl, enumClass);

    SetDefaultPositionInUnfilledClassNodes(enumClassDecl, enumDecl);

    enumClassDecl->SetParent(enumDecl->Parent());
    ProcessEnumClassDeclaration(enumDecl, flags, enumClassDecl);
    return enumClassDecl;
}

static EnumLoweringPhase::DeclarationFlags GetDeclFlags(ir::TSEnumDeclaration *const enumDecl)
{
    return {enumDecl->Parent() != nullptr && enumDecl->Parent()->IsETSModule() &&
                enumDecl->Parent()->AsETSModule()->IsETSScript(),
            enumDecl->Parent() != nullptr && enumDecl->Parent()->IsBlockStatement(),
            enumDecl->Parent() != nullptr && enumDecl->Parent()->IsClassDefinition() &&
                enumDecl->Parent()->AsClassDefinition()->IsNamespaceTransformed()};
}

ir::AstNode *EnumLoweringPhase::TransformAnnotatedEnumChildrenRecursively(ir::TSEnumDeclaration *const enumDecl)
{
    auto const flags = GetDeclFlags(enumDecl);
    if (!flags.IsValid() || enumDecl->Members().empty()) {
        return enumDecl;
    }

    bool hasLoggedError = false;
    bool hasLongLiteral = false;
    auto *const itemInit = enumDecl->Members().front()->AsTSEnumMember()->Init();
    ir::TypeNode *typeAnnotation = enumDecl->TypeNodes();

    ir::PrimitiveType primitiveType = typeAnnotation->AsETSPrimitiveType()->GetPrimitiveType();
    if ((primitiveType == ir::PrimitiveType::INT || primitiveType == ir::PrimitiveType::LONG) &&
        CheckEnumMemberType<EnumType::INT>(enumDecl->Members(), hasLoggedError, hasLongLiteral)) {
        auto res = hasLongLiteral ? CreateEnumIntClassFromEnumDeclaration<ir::PrimitiveType::LONG>(enumDecl, flags)
                                  : CreateEnumIntClassFromEnumDeclaration<ir::PrimitiveType::INT>(enumDecl, flags);
        return res;
    }

    if ((primitiveType == ir::PrimitiveType::DOUBLE) &&
        CheckEnumMemberType<EnumType::DOUBLE>(enumDecl->Members(), hasLoggedError, hasLongLiteral)) {
        return CreateEnumFloatClassFromEnumDeclaration<ir::PrimitiveType::DOUBLE>(enumDecl, flags);
    }

    if (!hasLoggedError) {
        LogError(diagnostic::UNSUPPORTED_ENUM_TYPE, {}, itemInit->Start());
    }

    return enumDecl;
}

ir::AstNode *EnumLoweringPhase::TransformEnumChildrenRecursively(ir::TSEnumDeclaration *const enumDecl)
{
    auto const flags = GetDeclFlags(enumDecl);
    if (!flags.IsValid()) {
        return enumDecl;
    }

    if (enumDecl->Members().empty()) {
        return CreateEnumIntClassFromEnumDeclaration<ir::PrimitiveType::INT>(enumDecl, flags);
    }

    bool hasLoggedError = false;
    bool hasLongLiteral = false;
    auto *const itemInit = enumDecl->Members().front()->AsTSEnumMember()->Init();

    if (itemInit->IsNumberLiteral() &&
        ((itemInit->AsNumberLiteral()->Number().IsInteger() || itemInit->AsNumberLiteral()->Number().IsLong()) &&
         CheckEnumMemberType<EnumType::INT>(enumDecl->Members(), hasLoggedError, hasLongLiteral))) {
        auto res = hasLongLiteral ? CreateEnumIntClassFromEnumDeclaration<ir::PrimitiveType::LONG>(enumDecl, flags)
                                  : CreateEnumIntClassFromEnumDeclaration<ir::PrimitiveType::INT>(enumDecl, flags);
        return res;
    }
    if (itemInit->IsNumberLiteral() &&
        ((itemInit->AsNumberLiteral()->Number().IsDouble() || itemInit->AsNumberLiteral()->Number().IsFloat()) &&
         CheckEnumMemberType<EnumType::DOUBLE>(enumDecl->Members(), hasLoggedError, hasLongLiteral))) {
        return CreateEnumFloatClassFromEnumDeclaration<ir::PrimitiveType::DOUBLE>(enumDecl, flags);
    }

    if (itemInit->IsStringLiteral() &&
        CheckEnumMemberType<EnumType::STRING>(enumDecl->Members(), hasLoggedError, hasLongLiteral)) {
        return CreateEnumStringClassFromEnumDeclaration(enumDecl, flags);
    }

    if (!hasLoggedError) {
        LogError(diagnostic::ERROR_ARKTS_NO_ENUM_MIXED_TYPES, {}, itemInit->Start());
    }

    return enumDecl;
}

bool EnumLoweringPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    if (program->Extension() != ScriptExtension::ETS) {
        return true;
    }

    context_ = ctx;
    checker_ = ctx->GetChecker()->AsETSChecker();
    varbinder_ = ctx->parserProgram->VarBinder()->AsETSBinder();
    program_ = program;

    program->Ast()->TransformChildrenRecursively(
        [this](checker::AstNodePtr ast) -> checker::AstNodePtr {
            if (ast->IsTSEnumDeclaration()) {
                auto *const enumDecl = ast->AsTSEnumDeclaration();
                return enumDecl->TypeNodes() != nullptr ? TransformAnnotatedEnumChildrenRecursively(enumDecl)
                                                        : TransformEnumChildrenRecursively(enumDecl);
            }
            return ast;
        },
        Name());

    return true;
}

template <ir::PrimitiveType TYPE>
ir::Identifier *EnumLoweringPhase::CreateEnumValuesArray(const ir::TSEnumDeclaration *const enumDecl,
                                                         ir::ClassDefinition *const enumClass)
{
    auto *const type = AllocNode<ir::ETSPrimitiveType>(TYPE, Allocator());
    auto *const arrayTypeAnnotation = AllocNode<ir::TSArrayType>(type, Allocator());
    // clang-format off
    return MakeArray(enumDecl, enumClass, VALUES_ARRAY_NAME, arrayTypeAnnotation,
                     [this](const ir::TSEnumMember *const member) {
                        auto *const enumValueLiteral = AllocNode<ir::NumberLiteral>(
                            lexer::Number(member->AsTSEnumMember()
                                                ->Init()
                                                ->AsNumberLiteral()
                                                ->Number()));
                        return enumValueLiteral;
                    });
    // clang-format on
}

ir::Identifier *EnumLoweringPhase::CreateEnumStringValuesArray(const ir::TSEnumDeclaration *const enumDecl,
                                                               ir::ClassDefinition *const enumClass)
{
    auto *const stringTypeAnnotation = MakeTypeReference(context_, STRING_REFERENCE_TYPE);  // NOTE String -> Builtin?
    auto *const arrayTypeAnnotation = AllocNode<ir::TSArrayType>(stringTypeAnnotation, Allocator());

    // clang-format off
    return MakeArray(enumDecl, enumClass, STRING_VALUES_ARRAY_NAME, arrayTypeAnnotation,
                     [this](const ir::TSEnumMember *const member) {
                        auto *const init = member->AsTSEnumMember()->Init();
                        util::StringView stringValue;

                        if (init->IsStringLiteral()) {
                            stringValue = init->AsStringLiteral()->Str();
                        } else {
                            std::string str = init->AsNumberLiteral()->ToString();
                            stringValue = util::UString(str, Allocator()).View();
                        }
                        auto *const enumValueStringLiteral = AllocNode<ir::StringLiteral>(stringValue);
                        return enumValueStringLiteral;
                    });
    // clang-format on
}

ir::Identifier *EnumLoweringPhase::CreateEnumItemsArray(const ir::TSEnumDeclaration *const enumDecl,
                                                        ir::ClassDefinition *const enumClass)
{
    auto *const enumTypeAnnotation = MakeTypeReference(context_, enumClass->Ident()->Name());
    auto *const arrayTypeAnnotation = AllocNode<ir::TSArrayType>(enumTypeAnnotation, Allocator());
    // clang-format off
    return MakeArray(enumDecl, enumClass, ITEMS_ARRAY_NAME, arrayTypeAnnotation,
                     [this, enumClass, enumDecl](const ir::TSEnumMember *const member) {
                        auto *const enumTypeIdent =
                            AllocNode<ir::Identifier>(enumClass->Ident()->Name(),
					                               Allocator());
                        enumTypeIdent->SetRange(enumDecl->Key()->Range());
                        auto *const enumMemberIdent = AllocNode<ir::Identifier>(
                            member->AsTSEnumMember()->Key()->AsIdentifier()->Name(), context_->Allocator());
                        enumMemberIdent->SetRange(member->AsTSEnumMember()->Key()->Range());
                        auto *const enumMemberExpr = AllocNode<ir::MemberExpression>(
                            enumTypeIdent, enumMemberIdent, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
                        return enumMemberExpr;
                    });
    // clang-format on
}

ir::MemberExpression *EnumLoweringPhase::CreateOrdinalAccessExpression()
{
    auto *thisExpr = AllocNode<ir::ThisExpression>();
    auto *fieldIdentifier = AllocNode<ir::Identifier>(ORDINAL_NAME, Allocator());
    auto *ordinalAccessExpr = AllocNode<ir::MemberExpression>(thisExpr, fieldIdentifier,
                                                              ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    return ordinalAccessExpr;
}

namespace {

ir::MemberExpression *CreateStaticAccessMemberExpression(public_lib::Context *ctx,
                                                         ir::Identifier *const enumClassIdentifier,
                                                         ir::Identifier *const arrayIdentifier)
{
    auto *const enumClassIdentifierClone = enumClassIdentifier->Clone(ctx->Allocator(), nullptr);
    auto *const arrayIdentifierClone = arrayIdentifier->Clone(ctx->Allocator(), nullptr);

    return ctx->AllocNode<ir::MemberExpression>(enumClassIdentifierClone, arrayIdentifierClone,
                                                ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
}

ir::ThrowStatement *CreateThrowStatement(public_lib::Context *ctx, ir::ETSParameterExpression *const parameter,
                                         const util::UString &messageString)
{
    auto *const paramRefIdent = MakeParamRefIdent(ctx, parameter);
    auto *const message = ctx->AllocNode<ir::StringLiteral>(messageString.View());
    auto *const newExprArg =
        ctx->AllocNode<ir::BinaryExpression>(message, paramRefIdent, lexer::TokenType::PUNCTUATOR_PLUS);

    paramRefIdent->SetParent(newExprArg);
    ArenaVector<ir::Expression *> newExprArgs(ctx->Allocator()->Adapter());
    newExprArgs.push_back(newExprArg);

    auto *const exceptionReference = MakeTypeReference(ctx, "Error");
    auto *const newExpr = ctx->AllocNode<ir::ETSNewClassInstanceExpression>(exceptionReference, std::move(newExprArgs));
    return ctx->AllocNode<ir::ThrowStatement>(newExpr);
}

ir::ReturnStatement *CreateReturnStatement(public_lib::Context *ctx, ir::Identifier *const enumClassIdentifier,
                                           ir::Identifier *const arrayIdentifier, ir::Expression *const parameter)
{
    auto *const propertyAccessExpr = CreateStaticAccessMemberExpression(ctx, enumClassIdentifier, arrayIdentifier);

    auto *const arrayAccessExpr = ctx->AllocNode<ir::MemberExpression>(
        propertyAccessExpr, parameter, ir::MemberExpressionKind::ELEMENT_ACCESS, true, false);

    return ctx->AllocNode<ir::ReturnStatement>(arrayAccessExpr);
}

ir::CallExpression *CreateCallInstanceMethod(public_lib::Context *ctx, std::string_view methodName,
                                             ir::Expression *thisArg)
{
    auto methodId = ctx->AllocNode<ir::Identifier>(methodName, ctx->Allocator());
    auto callee = ctx->AllocNode<ir::MemberExpression>(thisArg, methodId, ir::MemberExpressionKind::PROPERTY_ACCESS,
                                                       false, false);

    ArenaVector<ir::Expression *> callArguments({}, ctx->Allocator()->Adapter());
    return ctx->AllocNode<ir::CallExpression>(callee, std::move(callArguments), nullptr, false);
}

}  // namespace

void EnumLoweringPhase::CreateEnumToStringMethod(const ir::TSEnumDeclaration *const enumDecl,
                                                 ir::ClassDefinition *const enumClass,
                                                 ir::Identifier *const stringValuesArrayIdent)
{
    auto *ordinalAccessExpr = CreateOrdinalAccessExpression();
    auto *const returnStmt =
        CreateReturnStatement(context_, enumClass->Ident(), stringValuesArrayIdent, ordinalAccessExpr);

    ArenaVector<ir::Statement *> body(Allocator()->Adapter());
    body.push_back(returnStmt);

    ArenaVector<ir::Expression *> params(Allocator()->Adapter());
    auto *const stringTypeAnnotation = MakeTypeReference(context_, STRING_REFERENCE_TYPE);  // NOTE String -> Builtin?
    auto *const function =
        MakeFunction({std::move(params), std::move(body), stringTypeAnnotation, enumDecl, ir::ModifierFlags::PUBLIC});

    auto *const functionIdent = AllocNode<ir::Identifier>(checker::ETSEnumType::TO_STRING_METHOD_NAME, Allocator());

    function->SetIdent(functionIdent);
    MakeMethodDef(context_, enumClass, functionIdent, function);
}

void EnumLoweringPhase::CreateEnumValueOfMethod(const ir::TSEnumDeclaration *const enumDecl,
                                                ir::ClassDefinition *const enumClass,
                                                ir::Identifier *const valuesArrayIdent,
                                                std::optional<ir::PrimitiveType> primitiveType)
{
    auto *ordinalAccessExpr = CreateOrdinalAccessExpression();
    auto *const returnStmt = CreateReturnStatement(context_, enumClass->Ident(), valuesArrayIdent, ordinalAccessExpr);

    ArenaVector<ir::Statement *> body(Allocator()->Adapter());
    body.push_back(returnStmt);

    ArenaVector<ir::Expression *> params(Allocator()->Adapter());
    auto *const typeAnnotation = primitiveType.has_value()
                                     ? AllocNode<ir::ETSPrimitiveType>(primitiveType.value(), Allocator())->AsTypeNode()
                                     : MakeTypeReference(context_, STRING_REFERENCE_TYPE)->AsTypeNode();
    auto *const function =
        MakeFunction({std::move(params), std::move(body), typeAnnotation, enumDecl, ir::ModifierFlags::PUBLIC});
    auto *const functionIdent = AllocNode<ir::Identifier>(checker::ETSEnumType::VALUE_OF_METHOD_NAME, Allocator());
    function->SetIdent(functionIdent);

    MakeMethodDef(context_, enumClass, functionIdent, function);
}

void EnumLoweringPhase::CreateEnumGetNameMethod(const ir::TSEnumDeclaration *const enumDecl,
                                                ir::ClassDefinition *const enumClass,
                                                ir::Identifier *const namesArrayIdent)
{
    auto ordinalAccessExpr = CreateOrdinalAccessExpression();
    auto *const returnStmt = CreateReturnStatement(context_, enumClass->Ident(), namesArrayIdent, ordinalAccessExpr);

    ArenaVector<ir::Statement *> body(Allocator()->Adapter());
    body.push_back(returnStmt);

    ArenaVector<ir::Expression *> params(Allocator()->Adapter());
    auto *const stringTypeAnnotation = MakeTypeReference(context_, STRING_REFERENCE_TYPE);  // NOTE String -> Builtin?

    auto *const function =
        MakeFunction({std::move(params), std::move(body), stringTypeAnnotation, enumDecl, ir::ModifierFlags::PUBLIC});
    auto *const functionIdent = AllocNode<ir::Identifier>(checker::ETSEnumType::GET_NAME_METHOD_NAME, Allocator());

    function->SetIdent(functionIdent);

    MakeMethodDef(context_, enumClass, functionIdent, function);
}

namespace {

ir::VariableDeclaration *CreateForLoopInitVariableDeclaration(public_lib::Context *ctx,
                                                              ir::Identifier *const enumClassIdentifier,
                                                              ir::Identifier *const namesArrayIdentifier,
                                                              ir::Identifier *const loopIdentifier)
{
    static_assert(EnumLoweringPhase::ORDINAL_TYPE == ir::PrimitiveType::INT);
    auto *const lengthIdent = ctx->AllocNode<ir::Identifier>("length", ctx->Allocator());
    auto *const propertyAccessExpr = CreateStaticAccessMemberExpression(ctx, enumClassIdentifier, namesArrayIdentifier);
    auto *const arrayLengthExpr = ctx->AllocNode<ir::MemberExpression>(
        propertyAccessExpr, lengthIdent, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    auto *const oneLiteral = ctx->AllocNode<ir::NumberLiteral>(lexer::Number((int32_t)1));
    auto *const init =
        ctx->AllocNode<ir::BinaryExpression>(arrayLengthExpr, oneLiteral, lexer::TokenType::PUNCTUATOR_MINUS);
    auto *const decl = ctx->AllocNode<ir::VariableDeclarator>(ir::VariableDeclaratorFlag::LET, loopIdentifier, init);

    ES2PANDA_ASSERT(loopIdentifier);
    loopIdentifier->SetParent(decl);
    ArenaVector<ir::VariableDeclarator *> decls(ctx->Allocator()->Adapter());
    decls.push_back(decl);
    auto *const declaration = ctx->AllocNode<ir::VariableDeclaration>(
        ir::VariableDeclaration::VariableDeclarationKind::LET, ctx->Allocator(), std::move(decls));
    ES2PANDA_ASSERT(decl);
    decl->SetParent(declaration);
    return declaration;
}

ir::BinaryExpression *CreateForLoopTest(public_lib::Context *ctx, ir::Identifier *const loopIdentifier)
{
    auto *const zeroLiteral = ctx->AllocNode<ir::NumberLiteral>(lexer::Number((int32_t)0));

    auto *const forLoopIdentClone = loopIdentifier->Clone(ctx->Allocator(), nullptr);
    auto *const binaryExpr = ctx->AllocNode<ir::BinaryExpression>(forLoopIdentClone, zeroLiteral,
                                                                  lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL);

    return binaryExpr;
}

ir::UpdateExpression *CreateForLoopUpdate(public_lib::Context *ctx, ir::Identifier *const loopIdentifier)
{
    auto *const forLoopIdentClone = loopIdentifier->Clone(ctx->Allocator(), nullptr);
    auto *const incrementExpr =
        ctx->AllocNode<ir::UpdateExpression>(forLoopIdentClone, lexer::TokenType::PUNCTUATOR_MINUS_MINUS, true);
    return incrementExpr;
}

ir::IfStatement *CreateIf(public_lib::Context *ctx, ir::MemberExpression *propertyAccessExpr,
                          ir::MemberExpression *itemAccessExpr, ir::Identifier *const loopIdentifier,
                          ir::ETSParameterExpression *const parameter)
{
    auto *const forLoopIdentClone1 = loopIdentifier->Clone(ctx->Allocator(), nullptr);
    auto *const namesArrayElementExpr = ctx->AllocNode<ir::MemberExpression>(
        propertyAccessExpr, forLoopIdentClone1, ir::MemberExpressionKind::ELEMENT_ACCESS, true, false);

    auto *const paramRefIdent = MakeParamRefIdent(ctx, parameter);
    auto *const namesEqualExpr =
        ctx->AllocNode<ir::BinaryExpression>(paramRefIdent, namesArrayElementExpr, lexer::TokenType::PUNCTUATOR_EQUAL);
    paramRefIdent->SetParent(namesEqualExpr);

    auto *const forLoopIdentClone2 = loopIdentifier->Clone(ctx->Allocator(), nullptr);

    auto *const itemsArrayElementExpr = ctx->AllocNode<ir::MemberExpression>(
        itemAccessExpr, forLoopIdentClone2, ir::MemberExpressionKind::ELEMENT_ACCESS, true, false);

    auto *const returnStmt = ctx->AllocNode<ir::ReturnStatement>(itemsArrayElementExpr);
    return ctx->AllocNode<ir::IfStatement>(namesEqualExpr, returnStmt, nullptr);
}

}  // namespace

void EnumLoweringPhase::CreateEnumGetValueOfMethod(const ir::TSEnumDeclaration *const enumDecl,
                                                   ir::ClassDefinition *const enumClass,
                                                   ir::Identifier *const namesArrayIdent,
                                                   ir::Identifier *const itemsArrayIdent)
{
    auto *const forLoopIIdent = AllocNode<ir::Identifier>(IDENTIFIER_I, Allocator());
    auto *const forLoopInitVarDecl =
        CreateForLoopInitVariableDeclaration(context_, enumClass->Ident(), namesArrayIdent, forLoopIIdent);
    auto *const forLoopTest = CreateForLoopTest(context_, forLoopIIdent);
    auto *const forLoopUpdate = CreateForLoopUpdate(context_, forLoopIIdent);
    auto *const stringTypeAnnotation = MakeTypeReference(context_, STRING_REFERENCE_TYPE);  // NOTE String -> Builtin?
    auto *const inputNameIdent = MakeFunctionParam(context_, PARAM_NAME, stringTypeAnnotation);
    auto *const ifStmt =
        CreateIf(context_, CreateStaticAccessMemberExpression(context_, enumClass->Ident(), namesArrayIdent),
                 CreateStaticAccessMemberExpression(context_, enumClass->Ident(), itemsArrayIdent), forLoopIIdent,
                 inputNameIdent);

    auto *const forLoop = AllocNode<ir::ForUpdateStatement>(forLoopInitVarDecl, forLoopTest, forLoopUpdate, ifStmt);

    util::UString messageString(util::StringView("No enum constant "), Allocator());
    messageString.Append(enumClass->Ident()->Name());
    messageString.Append('.');

    auto *const throwStmt = CreateThrowStatement(context_, inputNameIdent, messageString);

    ArenaVector<ir::Statement *> body(Allocator()->Adapter());
    body.push_back(forLoop);
    body.push_back(throwStmt);

    ArenaVector<ir::Expression *> params(Allocator()->Adapter());
    params.push_back(inputNameIdent);
    auto *const enumTypeAnnotation = MakeTypeReference(context_, enumClass->Ident()->Name());

    auto *const function = MakeFunction({std::move(params), std::move(body), enumTypeAnnotation, enumDecl,
                                         ir::ModifierFlags::PUBLIC | ir::ModifierFlags::STATIC});
    auto *const functionIdent = AllocNode<ir::Identifier>(checker::ETSEnumType::GET_VALUE_OF_METHOD_NAME, Allocator());

    function->SetIdent(functionIdent);
    MakeMethodDef(context_, enumClass, functionIdent, function);
}

void EnumLoweringPhase::CreateEnumFromValueMethod(ir::TSEnumDeclaration const *const enumDecl,
                                                  ir::ClassDefinition *const enumClass,
                                                  ir::Identifier *const valuesArrayIdent,
                                                  ir::Identifier *const itemsArrayIdent,
                                                  std::optional<ir::PrimitiveType> primitiveType)
{
    auto *const forLoopIIdent = AllocNode<ir::Identifier>(IDENTIFIER_I, Allocator());
    auto *const forLoopInitVarDecl =
        CreateForLoopInitVariableDeclaration(context_, enumClass->Ident(), valuesArrayIdent, forLoopIIdent);
    auto *const forLoopTest = CreateForLoopTest(context_, forLoopIIdent);
    auto *const forLoopUpdate = CreateForLoopUpdate(context_, forLoopIIdent);
    auto *const typeAnnotation = primitiveType.has_value()
                                     ? AllocNode<ir::ETSPrimitiveType>(primitiveType.value(), Allocator())->AsTypeNode()
                                     : MakeTypeReference(context_, STRING_REFERENCE_TYPE)->AsTypeNode();
    auto *const inputValueIdent = MakeFunctionParam(context_, PARAM_VALUE, typeAnnotation);
    auto *const ifStmt =
        CreateIf(context_, CreateStaticAccessMemberExpression(context_, enumClass->Ident(), valuesArrayIdent),
                 CreateStaticAccessMemberExpression(context_, enumClass->Ident(), itemsArrayIdent), forLoopIIdent,
                 inputValueIdent);

    auto *const forLoop = AllocNode<ir::ForUpdateStatement>(forLoopInitVarDecl, forLoopTest, forLoopUpdate, ifStmt);

    util::UString messageString(util::StringView("No enum "), Allocator());
    messageString.Append(enumClass->Ident()->Name());
    messageString.Append(" with value ");

    auto *const throwStmt = CreateThrowStatement(context_, inputValueIdent, messageString);

    ArenaVector<ir::Statement *> body(Allocator()->Adapter());
    body.push_back(forLoop);
    body.push_back(throwStmt);

    ArenaVector<ir::Expression *> params(Allocator()->Adapter());
    params.push_back(inputValueIdent);
    auto *const enumTypeAnnotation = MakeTypeReference(context_, enumClass->Ident()->Name());

    auto *const function = MakeFunction({std::move(params), std::move(body), enumTypeAnnotation, enumDecl,
                                         ir::ModifierFlags::PUBLIC | ir::ModifierFlags::STATIC});
    auto *const functionIdent = AllocNode<ir::Identifier>(checker::ETSEnumType::FROM_VALUE_METHOD_NAME, Allocator());

    function->SetIdent(functionIdent);
    MakeMethodDef(context_, enumClass, functionIdent, function);
}

void EnumLoweringPhase::CreateEnumValuesMethod(const ir::TSEnumDeclaration *const enumDecl,
                                               ir::ClassDefinition *const enumClass,
                                               ir::Identifier *const itemsArrayIdent)
{
    auto *const propertyAccessExpr = CreateStaticAccessMemberExpression(context_, enumClass->Ident(), itemsArrayIdent);
    auto *const returnStmt = AllocNode<ir::ReturnStatement>(propertyAccessExpr);
    ArenaVector<ir::Statement *> body(Allocator()->Adapter());
    body.push_back(returnStmt);

    ArenaVector<ir::Expression *> params(Allocator()->Adapter());
    auto *const enumArrayTypeAnnotation =
        AllocNode<ir::TSArrayType>(MakeTypeReference(context_, enumClass->Ident()->Name()), Allocator());

    auto *const function = MakeFunction({std::move(params), std::move(body), enumArrayTypeAnnotation, enumDecl,
                                         ir::ModifierFlags::PUBLIC | ir::ModifierFlags::STATIC});
    auto *const functionIdent = AllocNode<ir::Identifier>(checker::ETSEnumType::VALUES_METHOD_NAME, Allocator());
    function->SetIdent(functionIdent);

    MakeMethodDef(context_, enumClass, functionIdent, function);
}

void EnumLoweringPhase::CreateEnumGetOrdinalMethod(const ir::TSEnumDeclaration *const enumDecl,
                                                   ir::ClassDefinition *const enumClass)
{
    auto ordinalAccessExpr = CreateOrdinalAccessExpression();
    auto *const returnStmt = AllocNode<ir::ReturnStatement>(ordinalAccessExpr);
    ArenaVector<ir::Statement *> body(Allocator()->Adapter());
    body.push_back(returnStmt);

    ArenaVector<ir::Expression *> params(Allocator()->Adapter());
    auto *const intTypeAnnotation = Allocator()->New<ir::ETSPrimitiveType>(ORDINAL_TYPE, Allocator());

    auto *const function =
        MakeFunction({std::move(params), std::move(body), intTypeAnnotation, enumDecl, ir::ModifierFlags::PUBLIC});

    auto *const functionIdent = AllocNode<ir::Identifier>(checker::ETSEnumType::GET_ORDINAL_METHOD_NAME, Allocator());
    function->SetIdent(functionIdent);

    MakeMethodDef(context_, enumClass, functionIdent, function);
}

void EnumLoweringPhase::CreateEnumDollarGetMethod(ir::TSEnumDeclaration const *const enumDecl,
                                                  ir::ClassDefinition *const enumClass)
{
    auto *const inputTypeAnnotation = MakeTypeReference(context_, enumClass->Ident()->Name());
    auto *const inputNameIdent = MakeFunctionParam(context_, "e", inputTypeAnnotation);
    auto *const paramRefIdent = MakeParamRefIdent(context_, inputNameIdent);
    auto *const callExpr =
        CreateCallInstanceMethod(context_, checker::ETSEnumType::GET_NAME_METHOD_NAME, paramRefIdent);
    auto *const returnStmt = AllocNode<ir::ReturnStatement>(callExpr);
    ArenaVector<ir::Statement *> body(Allocator()->Adapter());
    body.push_back(returnStmt);

    ArenaVector<ir::Expression *> params(Allocator()->Adapter());
    params.push_back(inputNameIdent);
    auto *const stringTypeAnnotation = MakeTypeReference(context_, STRING_REFERENCE_TYPE);
    auto *const function = MakeFunction({std::move(params), std::move(body), stringTypeAnnotation, enumDecl,
                                         ir::ModifierFlags::PUBLIC | ir::ModifierFlags::STATIC});
    auto *const functionIdent = AllocNode<ir::Identifier>(checker::ETSEnumType::DOLLAR_GET_METHOD_NAME, Allocator());
    function->SetIdent(functionIdent);

    MakeMethodDef(context_, enumClass, functionIdent, function);
}

void EnumLoweringPhase::SetDefaultPositionInUnfilledClassNodes(const ir::ClassDeclaration *enumClassDecl,
                                                               ir::TSEnumDeclaration const *const enumDecl)
{
    // Range of "default" value is one point, because that code is not exist in initial source code
    // but createad by enum at this point
    const auto &defautlRange = lexer::SourceRange(enumDecl->Range().start, enumDecl->Range().start);
    enumClassDecl->IterateRecursively([&defautlRange](ir::AstNode *ast) -> void {
        // If SourcePostion is not set before, we set "default" which point to start of enum
        if (ast->Range().start.Program() == nullptr) {
            ast->SetRange(defautlRange);
        }
    });
}

ArenaAllocator *EnumLoweringPhase::Allocator()
{
    return context_->Allocator();
}

template <typename T, typename... Args>
T *EnumLoweringPhase::AllocNode(Args &&...args)
{
    return context_->AllocNode<T>(std::forward<Args>(args)...);
}

}  // namespace ark::es2panda::compiler
