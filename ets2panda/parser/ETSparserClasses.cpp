/**
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

#include "ETSparser.h"
#include "ETSNolintParser.h"
#include <utility>
#include <tuple>
#include "util/es2pandaMacros.h"
#include "parser/parserFlags.h"
#include "parser/parserStatusContext.h"
#include "util/helpers.h"
#include "util/language.h"
#include "utils/arena_containers.h"
#include "varbinder/varbinder.h"
#include "varbinder/ETSBinder.h"
#include "lexer/lexer.h"
#include "lexer/ETSLexer.h"
#include "ir/astNode.h"
#include "ir/brokenTypeNode.h"
#include "ir/base/classDefinition.h"
#include "ir/base/decorator.h"
#include "ir/base/catchClause.h"
#include "ir/base/classProperty.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/classStaticBlock.h"
#include "ir/base/spreadElement.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/dummyNode.h"
#include "ir/module/importDeclaration.h"
#include "ir/module/importDefaultSpecifier.h"
#include "ir/module/importSpecifier.h"
#include "ir/module/exportSpecifier.h"
#include "ir/module/exportNamedDeclaration.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/ifStatement.h"
#include "ir/statements/labelledStatement.h"
#include "ir/statements/switchStatement.h"
#include "ir/statements/throwStatement.h"
#include "ir/statements/tryStatement.h"
#include "ir/statements/whileStatement.h"
#include "ir/statements/forOfStatement.h"
#include "ir/statements/doWhileStatement.h"
#include "ir/statements/breakStatement.h"
#include "ir/statements/debuggerStatement.h"
#include "ir/ets/etsClassLiteral.h"
#include "ir/ets/etsPrimitiveType.h"
#include "ir/ets/etsPackageDeclaration.h"
#include "ir/ets/etsReExportDeclaration.h"
#include "ir/ets/etsWildcardType.h"
#include "ir/ets/etsNewArrayInstanceExpression.h"
#include "ir/ets/etsTuple.h"
#include "ir/ets/etsFunctionType.h"
#include "ir/ets/etsNewClassInstanceExpression.h"
#include "ir/ets/etsNewMultiDimArrayInstanceExpression.h"
#include "ir/ets/etsModule.h"
#include "ir/ets/etsTypeReference.h"
#include "ir/ets/etsTypeReferencePart.h"
#include "ir/ets/etsNullishTypes.h"
#include "ir/ets/etsUnionType.h"
#include "ir/ets/etsImportDeclaration.h"
#include "ir/ets/etsStructDeclaration.h"
#include "ir/ets/etsParameterExpression.h"
#include "ir/module/importNamespaceSpecifier.h"
#include "ir/ts/tsAsExpression.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsEnumDeclaration.h"
#include "ir/ts/tsTypeParameterInstantiation.h"
#include "ir/ts/tsInterfaceBody.h"
#include "ir/ts/tsImportEqualsDeclaration.h"
#include "ir/ts/tsArrayType.h"
#include "ir/ts/tsQualifiedName.h"
#include "ir/ts/tsTypeReference.h"
#include "ir/ts/tsTypeParameter.h"
#include "ir/ts/tsInterfaceHeritage.h"
#include "ir/ts/tsFunctionType.h"
#include "ir/ts/tsClassImplements.h"
#include "ir/ts/tsEnumMember.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "ir/ts/tsTypeParameterDeclaration.h"
#include "ir/ts/tsNonNullExpression.h"
#include "ir/ts/tsThisType.h"
#include "generated/signatures.h"
#include "generated/diagnostic.h"

namespace ark::es2panda::parser {
class FunctionContext;

using namespace std::literals::string_literals;

static bool IsClassModifier(lexer::TokenType type)
{
    return type == lexer::TokenType::KEYW_STATIC || type == lexer::TokenType::KEYW_ABSTRACT ||
           type == lexer::TokenType::KEYW_FINAL;
}

ir::ModifierFlags ETSParser::ParseClassModifiers()
{
    ir::ModifierFlags flags = ir::ModifierFlags::NONE;

    while (IsClassModifier(Lexer()->GetToken().KeywordType())) {
        ir::ModifierFlags currentFlag = ir::ModifierFlags::NONE;

        switch (Lexer()->GetToken().KeywordType()) {
            case lexer::TokenType::KEYW_STATIC: {
                currentFlag = ir::ModifierFlags::STATIC;
                break;
            }
            case lexer::TokenType::KEYW_FINAL: {
                currentFlag = ir::ModifierFlags::FINAL;
                break;
            }
            case lexer::TokenType::KEYW_ABSTRACT: {
                currentFlag = ir::ModifierFlags::ABSTRACT;
                break;
            }
            default: {
                ES2PANDA_UNREACHABLE();
            }
        }

        if ((flags & currentFlag) != 0) {
            LogError(diagnostic::DUPLICATED_MODIFIER);
        }

        Lexer()->NextToken();
        flags |= currentFlag;
    }

    return flags;
}

std::tuple<ir::Expression *, ir::TSTypeParameterInstantiation *> ETSParser::ParseClassImplementsElement()
{
    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR |
                                           TypeAnnotationParsingOptions::IGNORE_FUNCTION_TYPE |
                                           TypeAnnotationParsingOptions::ALLOW_WILDCARD;
    return {ParseTypeReference(&options), nullptr};
}

ir::Expression *ETSParser::ParseSuperClassReference()
{
    if (Lexer()->GetToken().Type() != lexer::TokenType::KEYW_EXTENDS) {
        return nullptr;
    }
    Lexer()->NextToken();  // eat "extends"
    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR |
                                           TypeAnnotationParsingOptions::IGNORE_FUNCTION_TYPE |
                                           TypeAnnotationParsingOptions::ALLOW_WILDCARD;
    return ParseTypeReference(&options);
}

ir::TypeNode *ETSParser::ParseInterfaceExtendsElement()
{
    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR |
                                           TypeAnnotationParsingOptions::IGNORE_FUNCTION_TYPE |
                                           TypeAnnotationParsingOptions::ALLOW_WILDCARD;
    return ParseTypeReference(&options);
}

static bool IsClassMemberAccessModifier(lexer::TokenType type)
{
    return type == lexer::TokenType::KEYW_PUBLIC || type == lexer::TokenType::KEYW_PRIVATE ||
           type == lexer::TokenType::KEYW_PROTECTED || type == lexer::TokenType::KEYW_INTERNAL;
}

void ETSParser::ReportAccessModifierError(const lexer::Token &token)
{
    const auto keywordType = token.KeywordType();
    if (InAmbientContext() && keywordType != lexer::TokenType::KEYW_PUBLIC &&
        keywordType != lexer::TokenType::KEYW_PROTECTED) {
        LogError(diagnostic::PROHIBITED_ACCESS_MODIFIER_IN_AMBIENT_CLASS, {token.ToString()}, token.Start());
    }
}

std::tuple<ir::ModifierFlags, bool, bool> ETSParser::ParseClassMemberAccessModifiers()
{
    if (!IsClassMemberAccessModifier(Lexer()->GetToken().Type())) {
        return {ir::ModifierFlags::PUBLIC, false, true};
    }

    char32_t nextCp = Lexer()->Lookahead();
    if (nextCp == lexer::LEX_CHAR_EQUALS || nextCp == lexer::LEX_CHAR_COLON || nextCp == lexer::LEX_CHAR_LEFT_PAREN) {
        return {ir::ModifierFlags::NONE, false, false};
    }

    ir::ModifierFlags accessFlag = ir::ModifierFlags::NONE;

    const auto token = Lexer()->GetToken();

    switch (token.KeywordType()) {
        case lexer::TokenType::KEYW_PUBLIC: {
            accessFlag = ir::ModifierFlags::PUBLIC;
            break;
        }
        case lexer::TokenType::KEYW_PRIVATE: {
            accessFlag = ir::ModifierFlags::PRIVATE;
            break;
        }
        case lexer::TokenType::KEYW_PROTECTED: {
            accessFlag = ir::ModifierFlags::PROTECTED;
            break;
        }
        case lexer::TokenType::KEYW_INTERNAL: {
            Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
            if (Lexer()->GetToken().KeywordType() != lexer::TokenType::KEYW_PROTECTED) {
                ReportAccessModifierError(token);
                return {ir::ModifierFlags::INTERNAL, true, false};
            }
            accessFlag = ir::ModifierFlags::INTERNAL_PROTECTED;
            break;
        }
        case lexer::TokenType::EOS: {  // process invalid tokenType
            return {ir::ModifierFlags::NONE, false, false};
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    ReportAccessModifierError(token);

    if (((GetContext().Status() & ParserStatus::FUNCTION) != 0) &&
        (accessFlag == ir::ModifierFlags::PUBLIC || accessFlag == ir::ModifierFlags::PRIVATE ||
         accessFlag == ir::ModifierFlags::PROTECTED)) {
        LogError(diagnostic::LOCAL_CLASS_ACCESS_MOD, {}, Lexer()->GetToken().Start());
    }

    Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
    return {accessFlag, true, false};
}

static bool IsClassFieldModifier(lexer::TokenType type)
{
    return type == lexer::TokenType::KEYW_STATIC || type == lexer::TokenType::KEYW_READONLY;
}

ir::ModifierFlags ETSParser::ParseClassFieldModifiers(bool seenStatic)
{
    ir::ModifierFlags flags = seenStatic ? ir::ModifierFlags::STATIC : ir::ModifierFlags::NONE;

    while (IsClassFieldModifier(Lexer()->GetToken().KeywordType())) {
        char32_t nextCp = Lexer()->Lookahead();
        if (!(nextCp != lexer::LEX_CHAR_EQUALS && nextCp != lexer::LEX_CHAR_COLON)) {
            return flags;
        }

        ir::ModifierFlags currentFlag;

        switch (Lexer()->GetToken().KeywordType()) {
            case lexer::TokenType::KEYW_STATIC: {
                currentFlag = ir::ModifierFlags::STATIC;
                break;
            }
            case lexer::TokenType::KEYW_READONLY: {
                currentFlag = ir::ModifierFlags::READONLY;
                break;
            }
            default: {
                ES2PANDA_UNREACHABLE();
            }
        }

        if ((flags & currentFlag) != 0) {
            LogError(diagnostic::DUPLICATED_MODIFIER);
        }

        Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
        flags |= currentFlag;
    }

    return flags;
}

bool ETSParser::IsClassMethodModifier(lexer::TokenType type) noexcept
{
    switch (type) {
        case lexer::TokenType::KEYW_STATIC:
        case lexer::TokenType::KEYW_FINAL:
        case lexer::TokenType::KEYW_NATIVE:
        case lexer::TokenType::KEYW_ASYNC:
        case lexer::TokenType::KEYW_OVERRIDE:
        case lexer::TokenType::KEYW_ABSTRACT: {
            return true;
        }
        default: {
            break;
        }
    }

    return false;
}

ir::AstNode *ETSParser::HandleAmbientDeclaration(ir::ModifierFlags &memberModifiers,
                                                 const std::function<ir::AstNode *(ir::Identifier *)> &parseClassMethod)
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
        // Special case for processing of special '(param: type): returnType` identifier using in ambient context
        auto ident = CreateInvokeIdentifier();
        memberModifiers |= ir::ModifierFlags::STATIC;
        return parseClassMethod(ident);
    }
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
        const auto startPosAmbient = Lexer()->GetToken().Start();
        auto const savePos = Lexer()->Save();
        Lexer()->NextToken();
        if (Lexer()->GetToken().Ident().Is(compiler::Signatures::SYMBOL)) {
            Lexer()->Rewind(savePos);
            auto *memberName = ExpectIdentifier(false, false, TypeAnnotationParsingOptions::NO_OPTS);
            if (memberName == nullptr) {
                LogUnexpectedToken(Lexer()->GetToken());
                const auto &rangeToken = Lexer()->GetToken().Loc();
                Lexer()->NextToken();
                return AllocBrokenStatement(rangeToken);
            }

            if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS ||
                Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
                return parseClassMethod(memberName);
            }
        } else {
            return ParseAmbientSignature(startPosAmbient);
        }
    }
    return nullptr;
}

// Helper method for ir::ModifierFlags ETSParser::ParseClassMethodModifiers(bool seenStatic)
ir::ModifierFlags ETSParser::ParseClassMethodModifierFlag()
{
    ir::ModifierFlags currentFlag = ir::ModifierFlags::NONE;

    switch (Lexer()->GetToken().KeywordType()) {
        case lexer::TokenType::KEYW_STATIC: {
            currentFlag = ir::ModifierFlags::STATIC;
            break;
        }
        case lexer::TokenType::KEYW_FINAL: {
            currentFlag = ir::ModifierFlags::FINAL;
            break;
        }
        case lexer::TokenType::KEYW_NATIVE: {
            currentFlag = ir::ModifierFlags::NATIVE;
            break;
        }
        case lexer::TokenType::KEYW_ASYNC: {
            currentFlag = ir::ModifierFlags::ASYNC;
            break;
        }
        case lexer::TokenType::KEYW_OVERRIDE: {
            currentFlag = ir::ModifierFlags::OVERRIDE;
            break;
        }
        case lexer::TokenType::KEYW_ABSTRACT: {
            currentFlag = ir::ModifierFlags::ABSTRACT;
            break;
        }
        case lexer::TokenType::KEYW_DECLARE: {
            currentFlag = ir::ModifierFlags::DECLARE;
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    return currentFlag;
}

ir::ModifierFlags ETSParser::ParseClassMethodModifiers(bool seenStatic)
{
    ir::ModifierFlags flags = seenStatic ? ir::ModifierFlags::STATIC : ir::ModifierFlags::NONE;

    while (IsClassMethodModifier(Lexer()->GetToken().KeywordType())) {
        char32_t nextCp = Lexer()->Lookahead();
        if (!(nextCp != lexer::LEX_CHAR_LEFT_PAREN)) {
            return flags;
        }

        ir::ModifierFlags currentFlag = ir::ModifierFlags::NONE;

        currentFlag = ParseClassMethodModifierFlag();
        if ((flags & currentFlag) != 0) {
            LogError(diagnostic::DUPLICATED_MODIFIER);
        }

        Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
        flags |= currentFlag;
        if ((flags & ir::ModifierFlags::ASYNC) != 0) {
            if ((flags & ir::ModifierFlags::NATIVE) != 0) {
                LogError(diagnostic::NATIVE_METHOD_ASYNC);
            } else if ((flags & ir::ModifierFlags::ABSTRACT) != 0) {
                LogError(diagnostic::ABSTRACT_METHOD_ASYNC);
            }
        }
    }

    return flags;
}

ir::TypeNode *ETSParser::ConvertToOptionalUnionType(ir::TypeNode *typeAnno)
{
    if (typeAnno == nullptr) {
        return nullptr;
    }

    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    if (!typeAnno->IsETSUnionType()) {
        ArenaVector<ir::TypeNode *> types(Allocator()->Adapter());
        types.push_back(typeAnno);
        types.push_back(AllocNode<ir::ETSUndefinedType>(Allocator()));
        types.back()->SetRange(typeAnno->Range());
        auto *newTypeAnno = AllocNode<ir::ETSUnionType>(std::move(types), Allocator());
        ES2PANDA_ASSERT(newTypeAnno != nullptr);
        newTypeAnno->SetRange(typeAnno->Range());
        return newTypeAnno;
    }

    auto unionTypes = typeAnno->AsETSUnionType()->Types();
    for (const auto &type : unionTypes) {
        if (type->IsETSUndefinedType()) {
            return typeAnno;
        }
    }

    ArenaVector<ir::TypeNode *> types(typeAnno->AsETSUnionType()->Types(), Allocator()->Adapter());
    types.push_back(AllocNode<ir::ETSUndefinedType>(Allocator()));
    types.back()->SetRange(typeAnno->Range());
    auto *newTypeAnno = AllocNode<ir::ETSUnionType>(std::move(types), Allocator());
    ES2PANDA_ASSERT(newTypeAnno != nullptr);
    newTypeAnno->SetRange(typeAnno->Range());
    return newTypeAnno;
}

void ETSParser::ValidateFieldModifiers(ir::ModifierFlags modifiers, bool optionalField, ir::Expression *initializer,
                                       lexer::SourcePosition pos)
{
    const bool isDeclare = (modifiers & ir::ModifierFlags::DECLARE) != 0;
    const bool isDefinite = (modifiers & ir::ModifierFlags::DEFINITE) != 0;
    const bool isStatic = (modifiers & ir::ModifierFlags::STATIC) != 0;

    if (isDeclare && initializer != nullptr) {
        LogError(diagnostic::INITIALIZERS_IN_AMBIENT_CONTEXTS);
        return;
    }

    if (isDefinite) {
        if (isStatic) {
            LogError(diagnostic::STATIC_LATE_INITIALIZATION_FIELD_INVALID_MODIFIER, {}, pos);
        }
        if (initializer != nullptr) {
            LogError(diagnostic::LATE_INITIALIZATION_FIELD_HAS_DEFAULT_VALUE, {}, pos);
        }
        if (optionalField) {
            LogError(diagnostic::CONFLICTING_FIELD_MODIFIERS, {}, pos);
        }
    }
}

// NOLINTNEXTLINE(google-default-arguments)
void ETSParser::ParseClassFieldDefinition(ir::Identifier *fieldName, ir::ModifierFlags modifiers,
                                          ArenaVector<ir::AstNode *> *declarations, bool isDefault)
{
    lexer::SourcePosition endLoc = fieldName->End();
    ir::TypeNode *typeAnnotation = nullptr;
    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    bool optionalField = false;

    auto start = Lexer()->GetToken().Start();
    if (Lexer()->GetToken().Type() == (lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK)) {
        endLoc = Lexer()->GetToken().End();
        modifiers |= ir::ModifierFlags::DEFINITE;
        Lexer()->NextToken();
    }
    if (Lexer()->GetToken().Type() == (lexer::TokenType::PUNCTUATOR_QUESTION_MARK)) {
        endLoc = Lexer()->GetToken().End();
        optionalField = true;
        Lexer()->NextToken();
    }
    if (Lexer()->GetToken().Type() == (lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK)) {
        endLoc = Lexer()->GetToken().End();
        modifiers |= ir::ModifierFlags::DEFINITE;
        Lexer()->NextToken();
    }
    if (Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_COLON)) {
        typeAnnotation = ParseTypeAnnotation(&options);
        if (typeAnnotation == nullptr) {
            LogError(diagnostic::ID_EXPECTED);
            return;
        }

        endLoc = typeAnnotation->End();
    }

    typeAnnotation = optionalField ? ConvertToOptionalUnionType(typeAnnotation) : typeAnnotation;

    ir::Expression *initializer = nullptr;
    if (Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_SUBSTITUTION)) {
        initializer = ParseExpression();
    } else if (typeAnnotation == nullptr) {
        typeAnnotation = AllocNode<ir::BrokenTypeNode>(Allocator());
        typeAnnotation->SetRange({endLoc, endLoc});
        LogError(diagnostic::FIELD_TPYE_ANNOTATION_MISSING, {}, endLoc);
    }

    ValidateFieldModifiers(modifiers, optionalField, initializer, start);

    auto *field = AllocNode<ir::ClassProperty>(fieldName, initializer, typeAnnotation, modifiers, Allocator(), false);
    ES2PANDA_ASSERT(field != nullptr);
    field->SetDefaultAccessModifier(isDefault);
    if (optionalField) {
        field->AddModifier(ir::ModifierFlags::OPTIONAL);
    }
    field->SetRange({fieldName->Start(), initializer != nullptr ? initializer->End() : endLoc});

    declarations->push_back(field);
}

void ETSParser::ValidateOverloadList(ArenaVector<ir::Expression *> const &overloadList)
{
    for (ir::Expression *overloadedName : overloadList) {
        if (!overloadedName->IsIdentifier()) {
            LogError(diagnostic::CLASS_INTERFACE_METHOD_OVERLOADED_NAME_MUST_IDENT, {}, overloadedName->Start());
        }
    }
}

ir::OverloadDeclaration *ETSParser::ParseClassOverloadDeclaration(ir::ModifierFlags modifiers)
{
    ValidateOverloadDeclarationModifiers(modifiers);
    ir::Identifier *overloadName = nullptr;
    // To avoid duplicate names with anonymous constructors, overload constructor name is
    // "constructorOverloadDeclaration", instead of "constructor"
    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_CONSTRUCTOR) {
        overloadName = AllocNode<ir::Identifier>(compiler::Signatures::CONSTRUCTOR_NAME, Allocator());
        modifiers |= ir::ModifierFlags::CONSTRUCTOR;
        Lexer()->NextToken();
    } else {
        overloadName = ExpectIdentifier(false, true, TypeAnnotationParsingOptions::REPORT_ERROR);
    }

    auto *overloadDef = AllocNode<ir::OverloadDeclaration>(overloadName->Clone(Allocator(), nullptr)->AsExpression(),
                                                           modifiers, Allocator());
    overloadDef->AddOverloadDeclFlag(ir::OverloadDeclFlags::CLASS_METHOD);

    auto startLoc = Lexer()->GetToken().Start();
    if (!Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_LEFT_BRACE)) {
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE);
    }
    ArenaVector<ir::Expression *> overloads(Allocator()->Adapter());
    lexer::SourcePosition endLoc;

    ParseList(
        lexer::TokenType::PUNCTUATOR_RIGHT_BRACE, lexer::NextTokenFlags::NONE,
        [this, &overloads, overloadDef]() { return ParseOverloadListElement(overloads, overloadDef); }, &endLoc, true);
    overloadDef->SetOverloadedList(std::move(overloads));
    overloadDef->SetRange({startLoc, endLoc});
    ValidateOverloadList(overloadDef->OverloadedList());
    return overloadDef;
}

ir::MethodDefinition *ETSParser::ParseClassMethodDefinition(ir::Identifier *methodName, ir::ModifierFlags modifiers,
                                                            bool isDefault)
{
    auto newStatus = ParserStatus::NEED_RETURN_TYPE | ParserStatus::ALLOW_SUPER;
    auto methodKind = ir::MethodDefinitionKind::METHOD;

    if ((modifiers & ir::ModifierFlags::CONSTRUCTOR) != 0) {
        newStatus = ParserStatus::CONSTRUCTOR_FUNCTION | ParserStatus::ALLOW_SUPER | ParserStatus::ALLOW_SUPER_CALL;
        methodKind = ir::MethodDefinitionKind::CONSTRUCTOR;
    }

    if ((modifiers & ir::ModifierFlags::ASYNC) != 0) {
        newStatus |= ParserStatus::ASYNC_FUNCTION;
    }

    if ((modifiers & ir::ModifierFlags::STATIC) == 0) {
        newStatus |= ParserStatus::ALLOW_THIS_TYPE;
    }

    ir::ScriptFunction *func = ParseFunction(newStatus);
    ES2PANDA_ASSERT(func != nullptr);
    func->SetIdent(methodName);
    auto *funcExpr = AllocNode<ir::FunctionExpression>(func);
    ES2PANDA_ASSERT(funcExpr != nullptr);
    funcExpr->SetRange(func->Range());
    func->AddModifier(modifiers);

    ES2PANDA_ASSERT(methodName->Clone(Allocator(), nullptr) != nullptr);
    auto *method = AllocNode<ir::MethodDefinition>(methodKind, methodName->Clone(Allocator(), nullptr)->AsExpression(),
                                                   funcExpr, modifiers, Allocator(), false);
    ES2PANDA_ASSERT(method != nullptr);
    method->SetDefaultAccessModifier(isDefault);
    method->SetRange(funcExpr->Range());
    return method;
}

ir::MethodDefinition *ETSParser::ParseClassMethod(ClassElementDescriptor *desc,
                                                  const ArenaVector<ir::AstNode *> &properties,
                                                  ir::Expression *propName, lexer::SourcePosition *propEnd)
{
    if (desc->methodKind != ir::MethodDefinitionKind::SET &&
        (desc->newStatus & ParserStatus::CONSTRUCTOR_FUNCTION) == 0) {
        desc->newStatus |= ParserStatus::NEED_RETURN_TYPE;
    }

    ir::ScriptFunction *func = ParseFunction(desc->newStatus);
    ES2PANDA_ASSERT(func != nullptr);
    if (propName->IsIdentifier()) {
        func->SetIdent(propName->AsIdentifier()->Clone(Allocator(), nullptr));
    }

    auto *funcExpr = AllocNode<ir::FunctionExpression>(func);
    ES2PANDA_ASSERT(funcExpr != nullptr);
    funcExpr->SetRange(func->Range());

    if (desc->methodKind == ir::MethodDefinitionKind::SET) {
        ValidateClassSetter(desc, properties, propName, func);
    } else if (desc->methodKind == ir::MethodDefinitionKind::GET) {
        ValidateClassGetter(desc, properties, propName, func);
    }

    *propEnd = func->End();
    func->AddFlag(ir::ScriptFunctionFlags::METHOD);
    auto *method =
        AllocNode<ir::MethodDefinition>(desc->methodKind, propName->Clone(Allocator(), nullptr)->AsExpression(),
                                        funcExpr, desc->modifiers, Allocator(), desc->isComputed);
    ES2PANDA_ASSERT(method != nullptr);
    method->SetRange(funcExpr->Range());

    return method;
}

void ETSParser::UpdateMemberModifiers(ir::ModifierFlags &memberModifiers, bool &seenStatic)
{
    if (InAmbientContext()) {
        memberModifiers |= ir::ModifierFlags::DECLARE;
    }

    char32_t nextCp = Lexer()->Lookahead();
    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_STATIC && nextCp != lexer::LEX_CHAR_EQUALS &&
        nextCp != lexer::LEX_CHAR_COLON && nextCp != lexer::LEX_CHAR_LEFT_PAREN &&
        nextCp != lexer::LEX_CHAR_LESS_THAN) {
        Lexer()->NextToken();
        memberModifiers |= ir::ModifierFlags::STATIC;
        seenStatic = true;
    } else {
        seenStatic = false;
    }

    if (IsClassFieldModifier(Lexer()->GetToken().KeywordType())) {
        memberModifiers |= ParseClassFieldModifiers(seenStatic);
    } else if (IsClassMethodModifier(Lexer()->GetToken().Type())) {
        memberModifiers |= ParseClassMethodModifiers(seenStatic);
    }
}

std::tuple<bool, bool, bool> ETSParser::HandleClassElementModifiers(ir::ModifierFlags &memberModifiers)
{
    auto [modifierFlags, isStepToken, isDefault] = ParseClassMemberAccessModifiers();
    memberModifiers |= modifierFlags;

    bool seenStatic = false;
    UpdateMemberModifiers(memberModifiers, seenStatic);

    return {seenStatic, isStepToken, isDefault};
}

ir::AstNode *ETSParser::ParseClassElement(const ArenaVector<ir::AstNode *> &properties,
                                          ir::ClassDefinitionModifiers modifiers,
                                          [[maybe_unused]] ir::ModifierFlags flags)
{
    auto startLoc = Lexer()->GetToken().Start();

    ArenaVector<ir::AnnotationUsage *> annotations(Allocator()->Adapter());
    if (Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_AT)) {
        annotations = ParseAnnotations(false);
    }

    ir::ModifierFlags memberModifiers = ir::ModifierFlags::NONE;
    auto savedPos = Lexer()->Save();  // NOLINT(clang-analyzer-deadcode.DeadStores)

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_STATIC &&
        Lexer()->Lookahead() == lexer::LEX_CHAR_LEFT_BRACE) {
        return ParseClassStaticBlock();
    }

    auto [seenStatic, isStepToken, isDefault] = HandleClassElementModifiers(memberModifiers);

    ir::AstNode *result = nullptr;
    auto delcStartLoc = Lexer()->GetToken().Start();
    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::KEYW_INTERFACE:
        case lexer::TokenType::KEYW_CLASS:
        case lexer::TokenType::KEYW_ENUM:
            result = ParseInnerTypeDeclaration(memberModifiers, savedPos, isStepToken, seenStatic);
            break;
        case lexer::TokenType::LITERAL_IDENT: {
            if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_STRUCT) {
                result = ParseInnerTypeDeclaration(memberModifiers, savedPos, isStepToken, seenStatic);
            } else if (IsNamespaceDecl()) {
                result = ParseNamespace(flags);
            } else {
                result = ParseInnerRest(properties, modifiers, memberModifiers, startLoc, isDefault);
            }
            break;
        }
        case lexer::TokenType::KEYW_CONSTRUCTOR:
            result = ParseInnerConstructorDeclaration(memberModifiers, startLoc, isDefault);
            break;
        case lexer::TokenType::KEYW_PUBLIC:
        case lexer::TokenType::KEYW_PRIVATE:
        case lexer::TokenType::KEYW_PROTECTED: {
            LogError(diagnostic::ACCESS_BEFORE_FIELD_METHOD);
            Lexer()->NextToken();
            return AllocBrokenStatement(delcStartLoc);
        }
        default: {
            result = ParseInnerRest(properties, modifiers, memberModifiers, startLoc, isDefault);
            break;
        }
    }
    ApplyAnnotationsToClassElement(result, std::move(annotations), delcStartLoc);
    return result;
}

void *ETSParser::ApplyAnnotationsToClassElement(ir::AstNode *property, ArenaVector<ir::AnnotationUsage *> &&annotations,
                                                lexer::SourcePosition pos)
{
    if (property == nullptr) {
        return nullptr;
    }

    if (annotations.empty()) {
        return property;
    }

    if (property->IsTSInterfaceBody()) {
        for (auto *node : property->AsTSInterfaceBody()->Body()) {
            ArenaVector<ir::AnnotationUsage *> cloneAnnotations(Allocator()->Adapter());
            for (auto *annotationUsage : annotations) {
                auto cloneAnnotationUsage = annotationUsage->Clone(Allocator(), node);
                ES2PANDA_ASSERT(cloneAnnotationUsage != nullptr);
                cloneAnnotations.push_back(cloneAnnotationUsage->AsAnnotationUsage());
            }
            ApplyAnnotationsToNode(node, std::move(cloneAnnotations), pos);
        }
    } else {
        ApplyAnnotationsToNode(property, std::move(annotations), pos);
    }

    return property;
}

ir::MethodDefinition *ETSParser::ParseClassGetterSetterMethod(const ArenaVector<ir::AstNode *> &properties,
                                                              const ir::ClassDefinitionModifiers modifiers,
                                                              const ir::ModifierFlags memberModifiers, bool isDefault)
{
    ClassElementDescriptor desc(Allocator());
    desc.methodKind = Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_GET ? ir::MethodDefinitionKind::GET
                                                                                      : ir::MethodDefinitionKind::SET;
    desc.propStart = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat get/set
    auto *methodName = ExpectIdentifier();
    ES2PANDA_ASSERT(methodName != nullptr);
    if (desc.methodKind == ir::MethodDefinitionKind::GET) {
        methodName->SetAccessor();
    } else {
        methodName->SetMutator();
    }

    desc.newStatus = ParserStatus::ALLOW_SUPER;
    desc.hasSuperClass = (modifiers & ir::ClassDefinitionModifiers::HAS_SUPER) != 0U;
    desc.modifiers = memberModifiers;

    lexer::SourcePosition propEnd = methodName->End();
    ir::MethodDefinition *method = ParseClassMethod(&desc, properties, methodName, &propEnd);
    ES2PANDA_ASSERT(method != nullptr);
    method->SetDefaultAccessModifier(isDefault);
    auto *func = method->Function();
    ES2PANDA_ASSERT(func != nullptr);
    func->AddModifier(desc.modifiers);
    method->SetRange({desc.propStart, propEnd});
    if (desc.methodKind == ir::MethodDefinitionKind::GET) {
        func->AddFlag(ir::ScriptFunctionFlags::GETTER);
    } else {
        func->AddFlag(ir::ScriptFunctionFlags::SETTER);
    }

    return method;
}

ir::MethodDefinition *ETSParser::ParseInterfaceGetterSetterMethod(const ir::ModifierFlags modifiers)
{
    auto methodKind = Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_GET ? ir::MethodDefinitionKind::GET
                                                                                      : ir::MethodDefinitionKind::SET;
    Lexer()->NextToken();  // eat get/set
    ExpectToken(lexer::TokenType::LITERAL_IDENT, false);
    ir::MethodDefinition *method = ParseInterfaceMethod(modifiers, methodKind);
    if (method == nullptr) {
        return nullptr;
    }
    method->AddModifier(ir::ModifierFlags::PUBLIC);
    auto *id = method->Id();
    auto *func = method->Function();
    ES2PANDA_ASSERT(id != nullptr);
    ES2PANDA_ASSERT(func != nullptr);
    if (methodKind == ir::MethodDefinitionKind::GET) {
        id->SetAccessor();
        func->AddFlag(ir::ScriptFunctionFlags::GETTER);
    } else {
        id->SetMutator();
        func->AddFlag(ir::ScriptFunctionFlags::SETTER);
    }
    method->AddModifier(ir::ModifierFlags::PUBLIC);

    func->SetIdent(id->Clone(Allocator(), nullptr));
    func->AddModifier(method->Modifiers());

    bool hasReturn = func->ReturnTypeAnnotation() != nullptr;
    if (hasReturn && methodKind == ir::MethodDefinitionKind::SET) {
        LogError(diagnostic::SETTER_NO_RETURN_TYPE, {}, func->Range().start);
    }

    return method;
}

ir::TSInterfaceDeclaration *ETSParser::ParseInterfaceBody(ir::Identifier *name, bool isStatic)
{
    GetContext().Status() |= ParserStatus::ALLOW_THIS_TYPE;

    ir::TSTypeParameterDeclaration *typeParamDecl = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        auto options =
            TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::ALLOW_DECLARATION_SITE_VARIANCE;
        typeParamDecl = ParseTypeParameterDeclaration(&options);
    }

    ArenaVector<ir::TSInterfaceHeritage *> extends(Allocator()->Adapter());
    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_EXTENDS) {
        extends = ParseInterfaceExtendsClause();
    }

    lexer::SourcePosition bodyStart = Lexer()->GetToken().Start();
    auto members = ParseTypeLiteralOrInterface();
    auto *body = AllocNode<ir::TSInterfaceBody>(std::move(members));
    ES2PANDA_ASSERT(body != nullptr);
    body->SetRange({bodyStart, Lexer()->GetToken().End()});

    const auto isExternal = IsExternal();
    auto *interfaceDecl = AllocNode<ir::TSInterfaceDeclaration>(
        Allocator(), std::move(extends),
        ir::TSInterfaceDeclaration::ConstructorData {name, typeParamDecl, body, isStatic, isExternal,
                                                     GetContext().GetLanguage()});

    Lexer()->NextToken();
    GetContext().Status() &= ~ParserStatus::ALLOW_THIS_TYPE;

    return interfaceDecl;
}

ir::Statement *ETSParser::ParseInterfaceDeclaration(bool isStatic)
{
    lexer::SourcePosition interfaceStart = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat interface keyword

    auto *id = ExpectIdentifier(false, true);

    auto *declNode = ParseInterfaceBody(id, isStatic);
    ES2PANDA_ASSERT(declNode != nullptr);

    declNode->SetRange({interfaceStart, declNode->Body()->Range().end});
    return declNode;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::ClassDefinition *ETSParser::ParseClassDefinition(ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags)
{
    Lexer()->NextToken();

    ir::Identifier *identNode = ParseClassIdent(modifiers);
    ir::TSTypeParameterDeclaration *typeParamDecl = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        auto options =
            TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::ALLOW_DECLARATION_SITE_VARIANCE;
        typeParamDecl = ParseTypeParameterDeclaration(&options);
    }

    // Parse SuperClass
    auto [superClass, superTypeParams] = ParseSuperClass();

    if (superClass != nullptr) {
        modifiers |= ir::ClassDefinitionModifiers::HAS_SUPER;
        GetContext().Status() |= ParserStatus::ALLOW_SUPER;
    }

    if (InAmbientContext()) {
        flags |= ir::ModifierFlags::DECLARE;
    }

    // Parse implements clause
    ArenaVector<ir::TSClassImplements *> implements(Allocator()->Adapter());
    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_IMPLEMENTS) {
        Lexer()->NextToken();
        implements = ParseClassImplementClause();
    }

    ArenaVector<ir::AstNode *> properties(Allocator()->Adapter());
    ir::MethodDefinition *ctor = nullptr;
    lexer::SourceRange bodyRange;

    if ((flags & ir::ModifierFlags::DECLARE) != 0U &&
        Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        // without ClassBody
        bodyRange = lexer::SourceRange {Lexer()->GetToken().Start(), Lexer()->GetToken().Start()};
        LogError(diagnostic::AMBIENT_CLASS_MISSING_BODY);
    } else {
        ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE, false);

        // Parse ClassBody
        std::tie(ctor, properties, bodyRange) = ParseClassBody(modifiers, flags);
    }

    auto *classDefinition =
        AllocNode<ir::ClassDefinition>(identNode, typeParamDecl, superTypeParams, std::move(implements), ctor,
                                       superClass, std::move(properties), modifiers, flags, GetContext().GetLanguage());
    ES2PANDA_ASSERT(classDefinition != nullptr);

    classDefinition->SetRange(bodyRange);

    GetContext().Status() &= ~ParserStatus::ALLOW_SUPER;

    return classDefinition;
}

ir::ModifierFlags ETSParser::ParseInterfaceMethodModifiers()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS ||
        Lexer()->GetToken().Type() == lexer::TokenType::KEYW_OVERLOAD) {
        return ir::ModifierFlags::PUBLIC;
    }

    if ((GetContext().Status() & ParserStatus::FUNCTION) != 0) {
        if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_PRIVATE) {
            LogError(diagnostic::LOCAL_CLASS_ACCESS_MOD, {}, Lexer()->GetToken().Start());
        }
    }
    const auto keywordType = Lexer()->GetToken().KeywordType();
    const bool isPrivate = (keywordType == lexer::TokenType::KEYW_PRIVATE);
    const bool isDefaultInAmbient = (keywordType == lexer::TokenType::KEYW_DEFAULT) && InAmbientContext();
    if (!isPrivate) {
        if (!isDefaultInAmbient) {
            LogError(diagnostic::UNEXPECTED_TOKEN_PRIVATE_ID);
        }
        if (keywordType == lexer::TokenType::KEYW_NEW) {
            LogError(diagnostic::ERROR_ARKTS_NO_INTERFACE_CONSTRUCTOR_SIGNATURES);
        }
    }
    Lexer()->NextToken();
    return isDefaultInAmbient ? ir::ModifierFlags::DEFAULT : ir::ModifierFlags::PRIVATE;
}

ir::TypeNode *ETSParser::ParseInterfaceTypeAnnotation(ir::Identifier *name)
{
    if (!Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_COLON) &&
        Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT &&
        Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
        LogError(diagnostic::INTERFACE_FIELDS_TYPE_ANNOTATION);
        Lexer()->GetToken().SetTokenType(lexer::TokenType::PUNCTUATOR_COLON);
        Lexer()->NextToken();
    }

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    auto *type = ParseTypeAnnotation(&options);
    name->SetTsTypeAnnotation(type);
    type->SetParent(name);
    return type;
}

void ETSParser::ParseInterfaceModifiers(ir::ModifierFlags &fieldModifiers, bool &optionalField)
{
    auto processDefinite = [this, &fieldModifiers]() {
        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK) {
            Lexer()->NextToken();
            fieldModifiers |= ir::ModifierFlags::DEFINITE;
            return true;
        }
        return false;
    };
    auto start = Lexer()->GetToken().Start();
    processDefinite();

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_QUESTION_MARK) {
        Lexer()->NextToken();
        optionalField = true;
    }

    processDefinite();

    if ((fieldModifiers & ir::ModifierFlags::DEFINITE) != 0 && optionalField) {
        LogError(diagnostic::CONFLICTING_FIELD_MODIFIERS, {}, start);
    }
}

void ETSParser::ParseIndexedSignature()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET);
    Lexer()->NextToken();
    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        return;
    }
    auto *name = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    Lexer()->NextToken();
    ParseInterfaceTypeAnnotation(name);

    if (!Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET)) {
        return;
    }
    Lexer()->NextToken();
    ParseInterfaceTypeAnnotation(name);
}

ir::AstNode *ETSParser::ParseInterfaceField()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT ||
                    Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET ||
                    Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);

    ir::ModifierFlags fieldModifiers = ir::ModifierFlags::PUBLIC;
    auto startLoc = Lexer()->GetToken().Start();

    auto *name = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());

    if (name->IsErrorPlaceHolder()) {
        Lexer()->NextToken();
        return AllocBrokenExpression(Lexer()->GetToken().Loc());
    }

    auto parseClassMethod = [&fieldModifiers, &startLoc, this](ir::Identifier *methodName) {
        auto *classMethod = ParseClassMethodDefinition(methodName, fieldModifiers, false);
        ES2PANDA_ASSERT(classMethod != nullptr);
        classMethod->SetStart(startLoc);
        return classMethod;
    };
    if (InAmbientContext()) {
        fieldModifiers |= ir::ModifierFlags::DECLARE;
        auto property = HandleAmbientDeclaration(fieldModifiers, parseClassMethod);
        if (property != nullptr) {
            return property;
        }
    }
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
        ParseIndexedSignature();
        LogError(diagnostic::ERROR_ARKTS_NO_PROPERTIES_BY_INDEX, {}, startLoc);
        return AllocBrokenExpression(Lexer()->GetToken().Start());
    }

    ES2PANDA_ASSERT(name != nullptr);
    name->SetRange(Lexer()->GetToken().Loc());
    Lexer()->NextToken();
    bool optionalField = false;

    ParseInterfaceModifiers(fieldModifiers, optionalField);
    auto *typeAnnotation = ParseInterfaceTypeAnnotation(name);
    auto *field = AllocNode<ir::ClassProperty>(name, nullptr, typeAnnotation->Clone(Allocator(), nullptr),
                                               fieldModifiers, Allocator(), false);
    ES2PANDA_ASSERT(field != nullptr);
    if (optionalField) {
        field->AddModifier(ir::ModifierFlags::OPTIONAL);
    }
    field->SetEnd(typeAnnotation->End());

    return field;
}

static lexer::SourcePosition GetEndLoc(ir::BlockStatement *body, ir::ScriptFunction *func, lexer::Lexer *lexer)
{
    if (body != nullptr) {
        return body->End();
    }

    if (func->ReturnTypeAnnotation() != nullptr) {
        return func->ReturnTypeAnnotation()->End();
    }

    if (!func->Params().empty()) {
        return func->Params().back()->End();
    }

    return lexer->GetToken().End();
}

ir::OverloadDeclaration *ETSParser::ParseInterfaceOverload(ir::ModifierFlags modifiers)
{
    ValidateOverloadDeclarationModifiers(modifiers);
    auto *overloadName = ExpectIdentifier(false, true, TypeAnnotationParsingOptions::REPORT_ERROR);
    auto *overloadDef = AllocNode<ir::OverloadDeclaration>(overloadName->Clone(Allocator(), nullptr)->AsExpression(),
                                                           modifiers, Allocator());
    overloadDef->AddOverloadDeclFlag(ir::OverloadDeclFlags::INTERFACE_METHOD);

    auto startLoc = Lexer()->GetToken().Start();
    if (!Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_LEFT_BRACE)) {
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE);
    }
    ArenaVector<ir::Expression *> overloads(Allocator()->Adapter());
    lexer::SourcePosition endLoc;

    ParseList(
        lexer::TokenType::PUNCTUATOR_RIGHT_BRACE, lexer::NextTokenFlags::NONE,
        [this, &overloads, overloadDef]() { return ParseOverloadListElement(overloads, overloadDef); }, &endLoc, true);

    overloadDef->SetOverloadedList(std::move(overloads));
    overloadDef->SetRange({startLoc, endLoc});
    ValidateOverloadList(overloadDef->OverloadedList());
    return overloadDef;
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
ir::MethodDefinition *ETSParser::ParseInterfaceMethod(ir::ModifierFlags flags, ir::MethodDefinitionKind methodKind)
{
    ir::Identifier *name = nullptr;
    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        LogError(diagnostic::EXPECTED_PARAM_GOT_PARAM,
                 {"method name", lexer::TokenToString(Lexer()->GetToken().Type())});
        name = AllocBrokenExpression(Lexer()->GetToken().Loc());
    } else {
        name = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
        ES2PANDA_ASSERT(name != nullptr);
        name->SetRange(Lexer()->GetToken().Loc());
        Lexer()->NextToken();
    }
    FunctionContext functionContext(this, ParserStatus::FUNCTION);

    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();

    auto [signature, throwMarker] = ParseFunctionSignature(ParserStatus::NEED_RETURN_TYPE);

    ir::BlockStatement *body = nullptr;

    bool isDeclare = InAmbientContext();
    if (isDeclare) {
        flags |= ir::ModifierFlags::DECLARE;
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        if (methodKind == ir::MethodDefinitionKind::SET || methodKind == ir::MethodDefinitionKind::GET) {
            LogError(diagnostic::GETTER_SETTER_NOT_ABSTRACT, {}, startLoc);
        }
        body = ParseBlockStatement();
    } else if ((flags & (ir::ModifierFlags::PRIVATE)) != 0 && !isDeclare) {
        LogError(diagnostic::PRIVATE_INTERFACE_MISSING_BODY, {}, startLoc);
    }

    functionContext.AddFlag(throwMarker);

    if ((GetContext().Status() & ParserStatus::FUNCTION_HAS_RETURN_STATEMENT) != 0) {
        functionContext.AddFlag(ir::ScriptFunctionFlags::HAS_RETURN);
        GetContext().Status() ^= ParserStatus::FUNCTION_HAS_RETURN_STATEMENT;
    }

    auto *func = AllocNode<ir::ScriptFunction>(
        Allocator(), ir::ScriptFunction::ScriptFunctionData {body, std::move(signature), functionContext.Flags(), flags,
                                                             GetContext().GetLanguage()});
    ES2PANDA_ASSERT(func != nullptr);
    if ((flags & ir::ModifierFlags::STATIC) == 0 && body == nullptr) {
        func->AddModifier(ir::ModifierFlags::ABSTRACT);
    }
    ValidateGetterSetter(methodKind, func->Params().size());
    func->SetRange({name->Start(), GetEndLoc(body, func, Lexer())});

    auto *funcExpr = AllocNode<ir::FunctionExpression>(func);
    ES2PANDA_ASSERT(funcExpr != nullptr);
    funcExpr->SetRange(func->Range());
    func->AddFlag(ir::ScriptFunctionFlags::METHOD);

    func->SetIdent(name);
    ES2PANDA_ASSERT(name->Clone(Allocator(), nullptr) != nullptr);
    auto *method = AllocNode<ir::MethodDefinition>(methodKind, name->Clone(Allocator(), nullptr)->AsExpression(),
                                                   funcExpr, flags, Allocator(), false);
    ES2PANDA_ASSERT(method != nullptr);
    method->SetRange(funcExpr->Range());

    ConsumeSemicolon(method);

    return method;
}

ir::AstNode *ETSParser::ParseAnnotationsInInterfaceBody()
{
    Lexer()->NextToken();  // eat '@'

    auto annotations = ParseAnnotations(false);
    auto savePos = Lexer()->GetToken().Start();
    ir::AstNode *result = ParseTypeLiteralOrInterfaceMember();
    if (result != nullptr) {
        ApplyAnnotationsToNode(result, std::move(annotations), savePos);
    }
    return result;
}

bool ETSParser::IsFieldStartToken(lexer::TokenType tokenType)
{
    return tokenType == lexer::TokenType::LITERAL_IDENT ||
           tokenType == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET ||
           tokenType == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS;
}

ir::AstNode *ETSParser::ParseTypeLiteralOrInterfaceMember()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_AT) {
        return ParseAnnotationsInInterfaceBody();
    }

    if (Lexer()->Lookahead() != lexer::LEX_CHAR_LEFT_PAREN && Lexer()->Lookahead() != lexer::LEX_CHAR_LESS_THAN &&
        (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_GET ||
         Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_SET)) {
        return ParseInterfaceGetterSetterMethod(ir::ModifierFlags::PUBLIC);
    }

    ir::ModifierFlags modifiers = ParseInterfaceMethodModifiers();
    char32_t nextCp = Lexer()->Lookahead();
    auto startLoc = Lexer()->GetToken().Start();
    auto readonlyTok = Lexer()->TryEatTokenKeyword(lexer::TokenType::KEYW_READONLY);
    bool isReadonly = readonlyTok.has_value();

    if (Lexer()->TryEatTokenFromKeywordType(lexer::TokenType::KEYW_OVERLOAD)) {
        auto *overloadDeclaration = ParseInterfaceOverload(modifiers);
        overloadDeclaration->SetStart(startLoc);
        return overloadDeclaration;
    }

    if (nextCp == lexer::LEX_CHAR_LEFT_PAREN || nextCp == lexer::LEX_CHAR_LESS_THAN) {
        if (isReadonly) {
            LogError(diagnostic::READONLY_INTERFACE_METHOD, {}, startLoc);
        }
        auto *method = ParseInterfaceMethod(modifiers, ir::MethodDefinitionKind::METHOD);
        ES2PANDA_ASSERT(method != nullptr);
        method->SetStart(startLoc);
        return method;
    }

    auto tok = Lexer()->GetToken().Type();
    if (!IsFieldStartToken(tok)) {
        LogError(diagnostic::ID_EXPECTED, {}, startLoc);
        return AllocBrokenExpression(Lexer()->GetToken().Loc());
    }

    auto *field = ParseInterfaceField();
    if (field != nullptr) {
        field->SetStart(startLoc);
        if (isReadonly) {
            field->AddModifier(ir::ModifierFlags::READONLY);
        }
        return field;
    }

    return ParseTypeDeclaration(true);
}

bool ETSParser::CheckClassElement(ir::AstNode *property, [[maybe_unused]] ir::MethodDefinition *&ctor,
                                  [[maybe_unused]] ArenaVector<ir::AstNode *> &properties)
{
    if (property->IsClassStaticBlock()) {
        if (std::any_of(properties.cbegin(), properties.cend(),
                        [](const auto *prop) { return prop->IsClassStaticBlock(); })) {
            LogError(diagnostic::MULTIPLE_STATIC_BLOCK, {}, property->Start());
        }

        auto *id = AllocNode<ir::Identifier>(compiler::Signatures::CCTOR, Allocator());
        property->AsClassStaticBlock()->Function()->SetIdent(id);
    }

    if (property->IsTSInterfaceBody()) {
        return CheckClassElementInterfaceBody(property, properties);
    }

    if (!property->IsMethodDefinition()) {
        return false;
    }

    auto const *const method = property->AsMethodDefinition();
    auto const *const function = method->Function();

    //  Check the special '$_get' and '$_set' methods using for object's index access
    if (method->Kind() == ir::MethodDefinitionKind::METHOD) {
        CheckPredefinedMethods(function, property->Start());
    }

    return false;  // resolve overloads later on scopes stage
}

void ETSParser::CheckPredefinedMethods(ir::ScriptFunction const *function, const lexer::SourcePosition &position)
{
    ES2PANDA_ASSERT(function != nullptr);
    auto const name = function->Id()->Name();

    auto const checkAsynchronous = [this, function, &name, &position]() -> void {
        if (function->IsAsyncFunc()) {
            LogError(diagnostic::SPECIAL_PREDEFINED_METHOD_CANNOT_BE_ASYNC, {std::string(name)}, position);
        }
    };

    if (name.Is(compiler::Signatures::GET_INDEX_METHOD)) {
        checkAsynchronous();

        bool isValid = function->Params().size() == 1U;
        if (isValid) {
            if (function->Params()[0]->IsBrokenExpression()) {
                return;
            }
            auto const *const param = function->Params()[0]->AsETSParameterExpression();
            isValid = !param->IsOptional() && !param->IsRestParameter();
        }

        if (!isValid) {
            LogError(diagnostic::SPECIAL_PREDEFINED_METHOD_SHOULD_HAVE_ONE_PARAM, {std::string(name)}, position);
        }
    } else if (name.Is(compiler::Signatures::SET_INDEX_METHOD)) {
        checkAsynchronous();

        bool isValid = function->Params().size() == 2U;
        if (isValid) {
            if (function->Params()[0]->IsBrokenExpression() || function->Params()[1]->IsBrokenExpression()) {
                return;
            }
            auto const *const param1 = function->Params()[0]->AsETSParameterExpression();
            auto const *const param2 = function->Params()[1]->AsETSParameterExpression();
            isValid = !param1->IsOptional() && !param1->IsRestParameter() && !param2->IsOptional() &&
                      !param2->IsRestParameter();
        }

        if (!isValid) {
            LogError(diagnostic::SPECIAL_PREDEFINED_METHOD_SHOULD_HAVE_TWO_PARAMS, {std::string(name)}, position);
        }
    } else if (name.Is(compiler::Signatures::ITERATOR_METHOD)) {
        checkAsynchronous();

        if (!function->Params().empty()) {
            LogError(diagnostic::SPECIAL_PREDEFINED_METHOD_SHOULD_NOT_HAVE_PARAMS, {std::string(name)}, position);
        }
    }
}

void ETSParser::CreateImplicitConstructor([[maybe_unused]] ir::MethodDefinition *&ctor,
                                          ArenaVector<ir::AstNode *> &properties,
                                          [[maybe_unused]] ir::ClassDefinitionModifiers modifiers,
                                          ir::ModifierFlags flags, const lexer::SourcePosition &startLoc)
{
    if (std::any_of(properties.cbegin(), properties.cend(), [](ir::AstNode *prop) {
            return prop->IsMethodDefinition() && prop->AsMethodDefinition()->IsConstructor();
        })) {
        return;
    }

    if ((modifiers & ir::ClassDefinitionModifiers::ANONYMOUS) != 0) {
        return;
    }

    auto *methodDef = BuildImplicitConstructor(ir::ClassDefinitionModifiers::SET_CTOR_ID, startLoc);
    ES2PANDA_ASSERT(methodDef != nullptr);
    if ((flags & ir::ModifierFlags::DECLARE) != 0) {
        auto func = methodDef->Function();
        ES2PANDA_ASSERT(func != nullptr);
        func->AddFlag(ir::ScriptFunctionFlags::EXTERNAL);
    }
    properties.push_back(methodDef);
}

static bool DeclareIsModifier(lexer::Lexer *lexer)
{
    bool result = false;
    auto currentPos = lexer->Save();
    if (lexer->GetToken().KeywordType() == lexer::TokenType::KEYW_DECLARE) {
        lexer->NextToken();  // eat 'declare'
        switch (lexer->GetToken().KeywordType()) {
            case lexer::TokenType::KEYW_LET:
            case lexer::TokenType::KEYW_CONST:
            case lexer::TokenType::KEYW_FUNCTION:
            case lexer::TokenType::KEYW_CLASS:
            case lexer::TokenType::KEYW_NAMESPACE:
            case lexer::TokenType::KEYW_ENUM:
            case lexer::TokenType::KEYW_ABSTRACT:
            case lexer::TokenType::KEYW_FINAL:
            case lexer::TokenType::KEYW_INTERFACE:
            case lexer::TokenType::KEYW_TYPE:
            case lexer::TokenType::KEYW_ASYNC:
            case lexer::TokenType::KEYW_STRUCT: {
                result = true;
                break;
            }
            default: {
                if (lexer->GetToken().Type() == lexer::TokenType::PUNCTUATOR_AT) {
                    result = true;
                    break;
                }
                if (lexer->GetToken().Type() == lexer::TokenType::LITERAL_IDENT &&
                    lexer->GetToken().Ident().Is("module")) {
                    result = true;
                    break;
                }
            }
        }
    }
    lexer->Rewind(currentPos);
    return result;
}

std::pair<ir::ModifierFlags, lexer::SourcePosition> ETSParser::ParseMemberModifiers()
{
    auto memberModifiers = ir::ModifierFlags::STATIC | ir::ModifierFlags::PUBLIC;

    if (Lexer()->TryEatTokenType(lexer::TokenType::KEYW_EXPORT)) {
        const auto savedPos = Lexer()->Save();
        if (Lexer()->TryEatTokenKeyword(lexer::TokenType::KEYW_DEFAULT)) {
            memberModifiers |= ir::ModifierFlags::DEFAULT_EXPORT;
        } else if (Lexer()->TryEatTokenKeyword(lexer::TokenType::KEYW_TYPE)) {
            if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
                Lexer()->Rewind(savedPos);
            }
            memberModifiers |= ir::ModifierFlags::EXPORT;
            memberModifiers |= ir::ModifierFlags::EXPORT_TYPE;
        } else if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
            LogError(diagnostic::ERROR_ARKTS_NO_EXPORT_ASSIGNMENT);
        } else {
            memberModifiers |= ir::ModifierFlags::EXPORT;
        }
    }

    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();

    if (DeclareIsModifier(Lexer())) {
        CheckDeclare();
        memberModifiers |= ir::ModifierFlags::DECLARE;
    }
    const auto tokenType = Lexer()->GetToken().KeywordType();
    if (tokenType == lexer::TokenType::KEYW_ASYNC || tokenType == lexer::TokenType::KEYW_NATIVE) {
        bool isAsync = tokenType == lexer::TokenType::KEYW_ASYNC;

        if (isAsync) {
            memberModifiers |= ir::ModifierFlags::ASYNC;
        } else {
            memberModifiers |= ir::ModifierFlags::NATIVE;
        }
        Lexer()->NextToken();

        if (Lexer()->GetToken().Type() != lexer::TokenType::KEYW_FUNCTION &&
            Lexer()->GetToken().KeywordType() != lexer::TokenType::KEYW_OVERLOAD) {
            // async_function_bas.ets
            if (isAsync) {
                LogError(diagnostic::ASYNC_FLAG_ONLY_FOR_TOP_FUN);
            } else {
                LogError(diagnostic::NATIVE_FLAG_ONLY_FOR_TOP_FUN);
            }
            Lexer()->GetToken().SetTokenType(lexer::TokenType::KEYW_FUNCTION);
        }
    }
    return std::make_pair(memberModifiers, startLoc);
}

}  // namespace ark::es2panda::parser
