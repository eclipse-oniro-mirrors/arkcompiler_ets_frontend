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

#include "TypedParser.h"

#include "parser/parserImpl.h"
#include "parser/parserStatusContext.h"
#include "varbinder/privateBinding.h"
#include "lexer/lexer.h"
#include "ir/base/classDefinition.h"
#include "ir/base/decorator.h"
#include "ir/base/spreadElement.h"
#include "ir/base/tsPropertySignature.h"
#include "ir/base/tsMethodSignature.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/tsIndexSignature.h"
#include "ir/base/scriptFunction.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/yieldExpression.h"
#include "ir/expressions/objectExpression.h"
#include "ir/expressions/arrayExpression.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/expressions/sequenceExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/expressions/functionExpression.h"
#include "ir/statement.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/variableDeclarator.h"
#include "ir/ts/tsModuleDeclaration.h"
#include "ir/ts/tsModuleBlock.h"
#include "ir/ts/tsInterfaceBody.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsEnumMember.h"
#include "ir/ts/tsEnumDeclaration.h"
#include "ir/ts/tsTypeParameterInstantiation.h"
#include "ir/ts/tsTypeParameterDeclaration.h"
#include "ir/ts/tsTypeParameter.h"
#include "ir/ts/tsQualifiedName.h"
#include "ir/ts/tsTypeReference.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "ir/ts/tsInterfaceHeritage.h"
#include "ir/ts/tsClassImplements.h"
#include "ir/ts/tsTypeAssertion.h"
#include "util/errorRecovery.h"
#include "generated/diagnostic.h"

namespace ark::es2panda::parser {

ir::Expression *TypedParser::ParsePotentialAsExpression([[maybe_unused]] ir::Expression *primaryExpression)
{
    return nullptr;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *TypedParser::ParseExpression(ExpressionParseFlags flags)
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_YIELD &&
        ((flags & ExpressionParseFlags::DISALLOW_YIELD) == 0)) {
        ir::YieldExpression *yieldExpr = ParseYieldExpression();

        return ParsePotentialExpressionSequence(yieldExpr, flags);
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        const auto startPos = Lexer()->Save();

        // NOTE(rsipka): ParseTsGenericArrowFunction and ParseTsTypeAssertion might be in a common function
        ir::Expression *expr = ParseGenericArrowFunction();
        // NOTE(rsipka): negative cases are not covered, probably this is not a complete solution yet
        if (expr == nullptr) {
            Lexer()->Rewind(startPos);
            expr = ParseTypeAssertion();
        }

        return expr;
    }

    ir::Expression *unaryExpressionNode = ParseUnaryOrPrefixUpdateExpression(flags);
    if (unaryExpressionNode->IsArrowFunctionExpression()) {
        return unaryExpressionNode;
    }

    ir::Expression *assignmentExpression = ParseAssignmentExpression(unaryExpressionNode, flags);

    if (Lexer()->GetToken().NewLine()) {
        return assignmentExpression;
    }

    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::LITERAL_IDENT: {
            if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_AS) {
                return ParsePotentialAsExpression(assignmentExpression);
            }
            break;
        }
        case lexer::TokenType::PUNCTUATOR_COMMA: {
            if ((flags & ExpressionParseFlags::ACCEPT_COMMA) != 0) {
                return ParseSequenceExpression(assignmentExpression,
                                               ((flags & ExpressionParseFlags::ACCEPT_REST) != 0));
            }
            break;
        }
        default: {
            break;
        }
    }

    return assignmentExpression;
}

bool TypedParser::IsNamespaceDecl()
{
    if (Lexer()->GetToken().KeywordType() != lexer::TokenType::KEYW_NAMESPACE) {
        return false;
    }
    auto savedPos = Lexer()->Save();
    Lexer()->NextToken();
    // namespace is a soft keyword, so it can be used as an identifier outside declarations
    // If followed by an identifier (literal or keyword), it's treated as a declaration
    // Using keywords as identifiers is invalid, but that error is handled later during parsing
    bool isNamespaceDecl =
        Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT || Lexer()->GetToken().IsKeyword();
    Lexer()->Rewind(savedPos);
    return isNamespaceDecl;
}

ir::Statement *TypedParser::ParsePotentialExpressionStatement(StatementParsingFlags flags)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT);

    switch (Lexer()->GetToken().KeywordType()) {
        case lexer::TokenType::KEYW_TYPE: {
            const auto maybeAlias = ParseTypeAliasStatement();
            if (maybeAlias != nullptr) {
                return maybeAlias;
            }
            break;
        }
        case lexer::TokenType::KEYW_ABSTRACT: {
            Lexer()->NextToken();  // eat abstract keyword

            if (Lexer()->GetToken().Type() != lexer::TokenType::KEYW_CLASS) {
                LogError(diagnostic::ABSTRACT_METHODS_ONLY_IN_ABSTRACT_CLASS);
                if (Lexer()->GetToken().IsKeyword()) {
                    return ParseExpressionStatement(flags);
                }
                Lexer()->GetToken().SetTokenType(lexer::TokenType::KEYW_CLASS);
            }

            return ParseClassStatement(flags, ir::ClassDefinitionModifiers::NONE, ir::ModifierFlags::ABSTRACT);
        }
        case lexer::TokenType::KEYW_GLOBAL:
        case lexer::TokenType::KEYW_MODULE: {
            return ParseModuleDeclaration();
        }
        case lexer::TokenType::KEYW_NAMESPACE: {
            if (((GetContext().Status() & ParserStatus::IN_AMBIENT_CONTEXT) != 0U) || IsNamespaceDecl()) {
                return ParseNamespace(ir::ModifierFlags::NONE);
            }
            [[fallthrough]];
        }
        default: {
            break;
        }
    }
    return ParseExpressionStatement(flags);
}

ir::TSTypeAssertion *TypedParser::ParseTypeAssertion()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN);
    lexer::SourcePosition start = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat '<'

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TypeNode *typeAnnotation = ParseTypeAnnotation(&options);

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_GREATER_THAN) {
        return nullptr;
    }

    Lexer()->NextToken();  // eat '>'
    ir::Expression *expression = ParseExpression();
    if (expression == nullptr) {
        return nullptr;
    }
    auto *typeAssertion = AllocNode<ir::TSTypeAssertion>(typeAnnotation, expression);
    ES2PANDA_ASSERT(typeAssertion != nullptr);
    typeAssertion->SetRange({start, Lexer()->GetToken().End()});

    return typeAssertion;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Statement *TypedParser::ParseModuleDeclaration([[maybe_unused]] StatementParsingFlags flags)
{
    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();
    GetContext().Status() |= ParserStatus::MODULE;

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_GLOBAL) {
        return ParseAmbientExternalModuleDeclaration(startLoc);
    }

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_NAMESPACE) {
        Lexer()->NextToken();
    } else {
        ES2PANDA_ASSERT(Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_MODULE);
        Lexer()->NextToken();
        if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_STRING) {
            return ParseAmbientExternalModuleDeclaration(startLoc);
        }
    }

    return ParseModuleOrNamespaceDeclaration(startLoc);
}

ir::Statement *TypedParser::ParseNamespace([[maybe_unused]] ir::ModifierFlags flags)
{
    return ParseModuleDeclaration();
}

ir::ArrowFunctionExpression *TypedParser::ParseGenericArrowFunction()
{
    ArrowFunctionContext arrowFunctionContext(this, false);

    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN);
    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();

    auto typeParamDeclOptions = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TSTypeParameterDeclaration *typeParamDecl = ParseTypeParameterDeclaration(&typeParamDeclOptions);

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
        return nullptr;
    }

    FunctionParameterContext funcParamContext(&GetContext());
    auto params = ParseFunctionParams();

    ParserStatus arrowStatus = ParserStatus::NO_OPTS;
    if (std::any_of(params.begin(), params.end(), [](const auto *param) { return !param->IsIdentifier(); })) {
        arrowStatus = ParserStatus::HAS_COMPLEX_PARAM;
    }

    ir::TypeNode *returnTypeAnnotation = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        returnTypeAnnotation = ParseTypeAnnotation(&options);
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_ARROW) {
        return nullptr;
    }

    ArrowFunctionDescriptor desc(std::move(params), startLoc, arrowStatus);

    return ParseArrowFunctionExpressionBody(&arrowFunctionContext, &desc, typeParamDecl, returnTypeAnnotation);
}

ir::TSModuleDeclaration *TypedParser::ParseAmbientExternalModuleDeclaration(const lexer::SourcePosition &startLoc)
{
    bool isGlobal = false;
    ir::Expression *name = nullptr;

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_GLOBAL) {
        isGlobal = true;
        name = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    } else {
        ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_STRING);

        if (!InAmbientContext()) {
            LogError(diagnostic::ONLY_AMBIENT_MODULES_QUOTED_NAMES);
        }

        name = AllocNode<ir::StringLiteral>(Lexer()->GetToken().String());
    }
    ES2PANDA_ASSERT(name != nullptr);
    name->SetRange(Lexer()->GetToken().Loc());

    Lexer()->NextToken();

    ir::Statement *body = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        body = ParseTsModuleBlock();
    } else if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
        Lexer()->NextToken();
    } else {
        // test exists for ts extension only
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_SEMI_COLON);
        Lexer()->NextToken();
    }

    auto *moduleDecl = AllocNode<ir::TSModuleDeclaration>(Allocator(), name, body,
                                                          ir::TSModuleDeclaration::ConstructorFlags {isGlobal, true});
    ES2PANDA_ASSERT(moduleDecl != nullptr);
    moduleDecl->SetRange({startLoc, Lexer()->GetToken().End()});

    return moduleDecl;
}

ir::TSModuleDeclaration *TypedParser::ParseModuleOrNamespaceDeclaration(const lexer::SourcePosition &startLoc)
{
    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        LogExpectedToken(lexer::TokenType::LITERAL_IDENT);
    }

    auto *identNode = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(identNode != nullptr);
    identNode->SetRange(Lexer()->GetToken().Loc());

    Lexer()->NextToken();

    ir::Statement *body = nullptr;

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD) {
        Lexer()->NextToken();
        lexer::SourcePosition moduleStart = Lexer()->GetToken().Start();
        body = ParseModuleOrNamespaceDeclaration(moduleStart);
    } else {
        body = ParseTsModuleBlock();
    }

    auto *moduleDecl = AllocNode<ir::TSModuleDeclaration>(Allocator(), identNode, body,
                                                          ir::TSModuleDeclaration::ConstructorFlags {false, false});
    ES2PANDA_ASSERT(moduleDecl != nullptr);
    moduleDecl->SetRange({startLoc, Lexer()->GetToken().End()});

    return moduleDecl;
}

ir::TSModuleBlock *TypedParser::ParseTsModuleBlock()
{
    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE);
    }

    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();
    Lexer()->NextToken();
    auto statements = ParseStatementList();

    auto *blockNode = AllocNode<ir::TSModuleBlock>(std::move(statements));
    ES2PANDA_ASSERT(blockNode != nullptr);
    blockNode->SetRange({startLoc, Lexer()->GetToken().End()});

    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_BRACE);
    return blockNode;
}

void TypedParser::CheckDeclare()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_DECLARE);

    if (InAmbientContext()) {
        LogError(diagnostic::DECALRE_IN_AMBIENT_CONTEXT);
    }

    GetContext().Status() |= ParserStatus::IN_AMBIENT_CONTEXT;

    Lexer()->NextToken();  // eat 'declare'

    switch (Lexer()->GetToken().KeywordType()) {
        case lexer::TokenType::KEYW_VAR:
        case lexer::TokenType::KEYW_LET:
        case lexer::TokenType::KEYW_CONST:
        case lexer::TokenType::KEYW_FUNCTION:
        case lexer::TokenType::KEYW_CLASS:
        case lexer::TokenType::KEYW_TYPE:
        case lexer::TokenType::KEYW_MODULE:
        case lexer::TokenType::KEYW_GLOBAL:
        case lexer::TokenType::KEYW_NAMESPACE:
        case lexer::TokenType::KEYW_ENUM:
        case lexer::TokenType::KEYW_ABSTRACT:
        case lexer::TokenType::KEYW_INTERFACE: {
            return;
        }
        default: {
            LogUnexpectedToken(Lexer()->GetToken());
        }
    }
}

void TypedParser::ParseDecorators(ArenaVector<ir::Decorator *> &decorators)
{
    while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_AT) {
        decorators.push_back(ParseDecorator());
    }
}

ir::TypeNode *TypedParser::ParseFunctionReturnType(ParserStatus status)
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options =
            TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::RETURN_TYPE;
        return ParseTypeAnnotation(&options);
    }

    if ((status & ParserStatus::NEED_RETURN_TYPE) != 0) {
        LogError(diagnostic::TYPE_EXPECTED);
    }

    return nullptr;
}

ir::TypeNode *TypedParser::ParseInterfaceExtendsElement()
{
    const lexer::SourcePosition &heritageStart = Lexer()->GetToken().Start();
    lexer::SourcePosition heritageEnd = Lexer()->GetToken().End();
    ir::Expression *expr = ParseQualifiedName();

    if (Lexer()->Lookahead() == lexer::LEX_CHAR_LESS_THAN) {
        Lexer()->ForwardToken(lexer::TokenType::PUNCTUATOR_LESS_THAN, 1);
    }

    ir::TSTypeParameterInstantiation *typeParamInst = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        typeParamInst = ParseTypeParameterInstantiation(&options);
        ES2PANDA_ASSERT(typeParamInst != nullptr);
        heritageEnd = typeParamInst->End();
    }

    auto *typeReference = AllocNode<ir::TSTypeReference>(expr, typeParamInst, Allocator());
    ES2PANDA_ASSERT(typeReference != nullptr);
    typeReference->SetRange({heritageStart, heritageEnd});
    return typeReference;
}

ArenaVector<ir::TSInterfaceHeritage *> TypedParser::ParseInterfaceExtendsClause()
{
    Lexer()->NextToken();  // eat extends keyword

    ArenaVector<ir::TSInterfaceHeritage *> extends(Allocator()->Adapter());

    while (true) {
        auto *typeReference = ParseInterfaceExtendsElement();
        ES2PANDA_ASSERT(typeReference != nullptr);
        auto *heritage = AllocNode<ir::TSInterfaceHeritage>(typeReference);
        ES2PANDA_ASSERT(heritage != nullptr);
        heritage->SetRange(typeReference->Range());
        extends.push_back(heritage);

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_BRACE ||
            Lexer()->GetToken().Type() == lexer::TokenType::EOS) {
            break;
        }

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COMMA) {
            LogExpectedToken(lexer::TokenType::PUNCTUATOR_COMMA);
        }

        Lexer()->NextToken();
    }

    return extends;
}

ir::TSTypeParameterDeclaration *TypedParser::ParseFunctionTypeParameters()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        auto options = TypeAnnotationParsingOptions::REPORT_ERROR;
        return ParseTypeParameterDeclaration(&options);
    }

    return nullptr;
}

ir::Statement *TypedParser::ParseInterfaceDeclaration(bool isStatic)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::KEYW_INTERFACE);
    GetContext().Status() |= ParserStatus::ALLOW_THIS_TYPE;
    lexer::SourcePosition interfaceStart = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat interface keyword

    auto id = ExpectIdentifier(true);

    ir::TSTypeParameterDeclaration *typeParamDecl = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        auto options = TypeAnnotationParsingOptions::REPORT_ERROR;
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
        ir::TSInterfaceDeclaration::ConstructorData {id, typeParamDecl, body, isStatic, isExternal,
                                                     GetContext().GetLanguage()});
    ES2PANDA_ASSERT(interfaceDecl != nullptr);
    interfaceDecl->SetRange({interfaceStart, Lexer()->GetToken().End()});

    Lexer()->NextToken();
    GetContext().Status() &= ~ParserStatus::ALLOW_THIS_TYPE;

    return interfaceDecl;
}

static util::StringView GetTSPropertyName(ir::Expression *key)
{
    switch (key->Type()) {
        case ir::AstNodeType::IDENTIFIER: {
            return key->AsIdentifier()->Name();
        }
        case ir::AstNodeType::NUMBER_LITERAL: {
            return key->AsNumberLiteral()->Str();
        }
        case ir::AstNodeType::STRING_LITERAL: {
            return key->AsStringLiteral()->Str();
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void TypedParser::CheckObjectTypeForDuplicatedProperties(ir::Expression *key, ArenaVector<ir::AstNode *> &members)
{
    if (!key->IsIdentifier() && !key->IsNumberLiteral() && !key->IsStringLiteral()) {
        return;
    }

    for (auto *it : members) {
        ir::Expression *compare = nullptr;

        switch (it->Type()) {
            case ir::AstNodeType::TS_PROPERTY_SIGNATURE: {
                compare = it->AsTSPropertySignature()->Key();
                break;
            }
            case ir::AstNodeType::TS_METHOD_SIGNATURE: {
                compare = it->AsTSMethodSignature()->Key();
                break;
            }
            default: {
                continue;
            }
        }

        if (!compare->IsIdentifier() && !compare->IsNumberLiteral() && !compare->IsStringLiteral()) {
            continue;
        }

        if (GetTSPropertyName(key) == GetTSPropertyName(compare)) {
            LogError(diagnostic::DUPLICATED_IDENTIFIER, {}, key->Start());
        }
    }
}

ArenaVector<ir::AstNode *> TypedParser::ParseTypeLiteralOrInterfaceBody()
{
    ArenaVector<ir::AstNode *> members(Allocator()->Adapter());

    while (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE &&
           Lexer()->GetToken().Type() != lexer::TokenType::EOS) {
        util::ErrorRecursionGuard infiniteLoopBlocker(Lexer());

        ir::AstNode *member = ParseTypeLiteralOrInterfaceMember();
        if (member->IsMethodDefinition() && member->AsMethodDefinition()->Function() != nullptr &&
            member->AsMethodDefinition()->Function()->IsOverload() &&
            member->AsMethodDefinition()->Function()->Body() != nullptr) {
            continue;
        }

        if (member->IsTSPropertySignature()) {
            CheckObjectTypeForDuplicatedProperties(member->AsTSPropertySignature()->Key(), members);
        } else if (member->IsTSMethodSignature()) {
            CheckObjectTypeForDuplicatedProperties(member->AsTSMethodSignature()->Key(), members);
        }

        members.push_back(member);

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
            break;
        }

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COMMA &&
            Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
            if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
                LogError(diagnostic::INTERFACE_MEMBER_INIT_NOT_ALLOWED);
                Lexer()->NextToken();
            }
            if (!Lexer()->GetToken().NewLine()) {
                LogExpectedToken(lexer::TokenType::PUNCTUATOR_COMMA);
            }

            if (Lexer()->GetToken().IsKeyword() && (Lexer()->GetToken().Type() != lexer::TokenType::KEYW_STATIC &&
                                                    Lexer()->GetToken().Type() != lexer::TokenType::KEYW_PRIVATE &&
                                                    Lexer()->GetToken().Type() != lexer::TokenType::KEYW_DEFAULT)) {
                Lexer()->GetToken().SetTokenType(lexer::TokenType::LITERAL_IDENT);
                Lexer()->GetToken().SetTokenStr(ERROR_LITERAL);
            }

            continue;
        }

        Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
    }

    return members;
}

ArenaVector<ir::AstNode *> TypedParser::ParseTypeLiteralOrInterface()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_IMPLEMENTS) {
        LogError(diagnostic::INTERFACE_DEC_IMPLEMENTS);
        Lexer()->NextToken();         // eat 'implements'
        ParseClassImplementClause();  // Try to parse implements, but drop the result;
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE);
    }

    Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);  // eat '{'

    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_OVERRIDE) {
        LogError(diagnostic::OVERRIDE_IN_INTERFACE);
        Lexer()->NextToken();  // eat 'override'
    }

    bool const formattedParsing = Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_FORMAT &&
                                  Lexer()->Lookahead() == static_cast<char32_t>(ARRAY_FORMAT_NODE);

    ArenaVector<ir::AstNode *> members =
        !formattedParsing ? ParseTypeLiteralOrInterfaceBody() : std::move(ParseAstNodesArrayFormatPlaceholder());

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
        // redundant check since we have the same check
        // in ParseTypeLiteralOrInterfaceBody() above
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_RIGHT_BRACE);
    }

    return members;
}

ir::TSEnumDeclaration *TypedParser::ParseEnumMembers(ir::Identifier *key, const lexer::SourcePosition &enumStart,
                                                     bool isConst, [[maybe_unused]] bool isStatic)
{
    ArenaVector<ir::AstNode *> members(Allocator()->Adapter());
    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE, false);
    Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);  // eat '{'

    lexer::SourcePosition endLoc;
    ParseList(
        lexer::TokenType::PUNCTUATOR_RIGHT_BRACE, lexer::NextTokenFlags::KEYWORD_TO_IDENT,
        [this, &members]() {
            ir::Expression *memberKey = nullptr;

            if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
                memberKey = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
                memberKey->SetRange(Lexer()->GetToken().Loc());
            } else if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_STRING) {
                memberKey = AllocNode<ir::StringLiteral>(Lexer()->GetToken().String());
                memberKey->SetRange(Lexer()->GetToken().Loc());
            } else {
                LogError(diagnostic::UNEXPECTED_TOKEN_ENUM);
                memberKey = AllocBrokenExpression(Lexer()->GetToken().Loc());
                // Consider that the current token is a memberKey and skip it.
            }

            Lexer()->NextToken();  // eat memberKey

            ir::Expression *memberInit = nullptr;
            lexer::SourcePosition initStart = Lexer()->GetToken().Start();

            if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
                Lexer()->NextToken();  // eat '='
                initStart = Lexer()->GetToken().Start();
                memberInit = ParseExpression();
            }

            auto *member = AllocNode<ir::TSEnumMember>(memberKey, memberInit);
            member->SetRange({initStart, Lexer()->GetToken().End()});
            members.push_back(member);
            return true;
        },
        &endLoc, true);

    auto *enumDeclaration =
        AllocNode<ir::TSEnumDeclaration>(Allocator(), key, std::move(members),
                                         ir::TSEnumDeclaration::ConstructorFlags {isConst}, GetContext().GetLanguage());
    ES2PANDA_ASSERT(enumDeclaration != nullptr);
    enumDeclaration->SetRange({enumStart, endLoc});

    return enumDeclaration;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Statement *TypedParser::ParseEnumDeclaration(bool isConst, [[maybe_unused]] bool isStatic)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::KEYW_ENUM);
    lexer::SourcePosition enumStart = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat enum keyword
    auto *key = ExpectIdentifier(true);
    auto *declNode = ParseEnumMembers(key, enumStart, isConst, false);
    return declNode;
}

ir::TSTypeParameter *TypedParser::ParseTypeParameter(TypeAnnotationParsingOptions *options)
{
    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();
    bool reportError = ((*options) & TypeAnnotationParsingOptions::REPORT_ERROR) != 0;

    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT || Lexer()->GetToken().IsDefinableTypeName()) {
        if (!reportError) {
            return nullptr;
        }

        LogError(diagnostic::TYPE_PARAM_DEC_EXPECTED);
        return nullptr;
    }

    if (reportError) {
        CheckIfTypeParameterNameIsReserved();
    }

    const auto &ident = Lexer()->GetToken().Ident();
    auto *paramIdent = AllocNode<ir::Identifier>(ident, Allocator());
    ES2PANDA_ASSERT(paramIdent != nullptr);
    paramIdent->SetRange({Lexer()->GetToken().Start(), Lexer()->GetToken().End()});

    Lexer()->NextToken();

    TypeAnnotationParsingOptions newOptions = TypeAnnotationParsingOptions::NO_OPTS;

    if (reportError) {
        newOptions |= TypeAnnotationParsingOptions::REPORT_ERROR;
    }

    ir::TypeNode *constraint = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_EXTENDS) {
        Lexer()->NextToken();
        constraint = ParseTypeAnnotation(&newOptions);
    }

    ir::TypeNode *defaultType = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        Lexer()->NextToken();
        defaultType = ParseTypeAnnotation(&newOptions);
    }

    auto *typeParam = AllocNode<ir::TSTypeParameter>(paramIdent, constraint, defaultType, Allocator());
    ES2PANDA_ASSERT(typeParam != nullptr);
    typeParam->SetRange({startLoc, Lexer()->GetToken().End()});

    return typeParam;
}

//  Auxiliary method to reduce the size of functions.
ir::AstNode *TypedParser::ParseTypeParameterDeclarationImpl(TypeAnnotationParsingOptions *options)
{
    ArenaVector<ir::TSTypeParameter *> params(Allocator()->Adapter());
    bool seenDefault = false;
    size_t requiredParams = 0U;

    while (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_GREATER_THAN) {
        auto newOptions = *options | TypeAnnotationParsingOptions::ADD_TYPE_PARAMETER_BINDING;
        ir::TSTypeParameter *currentParam = ParseTypeParameter(&newOptions);

        if (currentParam == nullptr) {
            // Maybe error processing.
            return nullptr;
        }

        if (currentParam->DefaultType() != nullptr) {
            seenDefault = true;
        } else if (seenDefault) {
            LogError(diagnostic::REQUIRED_TYPE_PARAM_AFTER_OPTIONAL);
        } else {
            requiredParams++;
        }

        params.push_back(currentParam);

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COMMA) {
            break;
        }

        Lexer()->NextToken();
    }

    if (params.empty()) {
        LogError(diagnostic::TYPE_PARAM_LIST_EMPTY);
    }

    return AllocNode<ir::TSTypeParameterDeclaration>(std::move(params), requiredParams);
}

ir::TSTypeParameterDeclaration *TypedParser::ParseTypeParameterDeclaration(TypeAnnotationParsingOptions *options)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN);

    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat '<'

    ir::AstNode *typeParamDecl;

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_FORMAT &&
        Lexer()->Lookahead() == static_cast<char32_t>(EXPRESSION_FORMAT_NODE)) {
        typeParamDecl = ParseTypeParametersFormatPlaceholder();
    } else {
        typeParamDecl = ParseTypeParameterDeclarationImpl(options);
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_GREATER_THAN) {
        if ((*options & TypeAnnotationParsingOptions::REPORT_ERROR) == 0) {
            return nullptr;
        }
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_GREATER_THAN);
    }

    lexer::SourcePosition endLoc = Lexer()->GetToken().End();
    Lexer()->NextToken();  // eat '>'

    if (typeParamDecl != nullptr) {
        typeParamDecl->SetRange({startLoc, endLoc});
        return typeParamDecl->AsTSTypeParameterDeclaration();
    }

    return nullptr;
}

ir::Expression *TypedParser::ParseSuperClassReference()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_EXTENDS) {
        Lexer()->NextToken();

        return ParseLeftHandSideExpression();
    }

    return nullptr;
}

std::tuple<ir::Expression *, ir::TSTypeParameterInstantiation *> TypedParser::ParseSuperClass()
{
    ir::Expression *superClass = ParseSuperClassReference();

    ir::TSTypeParameterInstantiation *superTypeParams = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT) {
        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT) {
            Lexer()->BackwardToken(lexer::TokenType::PUNCTUATOR_LESS_THAN, 1);
        }

        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        superTypeParams = ParseTypeParameterInstantiation(&options);
    }

    return {superClass, superTypeParams};
}

std::tuple<ir::Expression *, ir::TSTypeParameterInstantiation *> TypedParser::ParseClassImplementsElement()
{
    ir::Expression *expr = ParseQualifiedName();

    ir::TSTypeParameterInstantiation *implTypeParams = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT) {
        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT) {
            Lexer()->BackwardToken(lexer::TokenType::PUNCTUATOR_LESS_THAN, 1);
        }

        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        implTypeParams = ParseTypeParameterInstantiation(&options);
    }

    return {expr, implTypeParams};
}

ArenaVector<ir::TSClassImplements *> TypedParser::ParseClassImplementClause()
{
    ArenaVector<ir::TSClassImplements *> implements(Allocator()->Adapter());

    while (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        lexer::SourcePosition implStart = Lexer()->GetToken().Start();
        auto [expr, implTypeParams] = ParseClassImplementsElement();
        auto *impl = AllocNode<ir::TSClassImplements>(expr, implTypeParams);
        ES2PANDA_ASSERT(impl != nullptr);
        impl->SetRange({implStart, Lexer()->GetToken().End()});
        implements.push_back(impl);

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA) {
            Lexer()->NextToken();
            continue;
        }

        if (InAmbientContext()) {
            break;
        }

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
            LogExpectedToken(lexer::TokenType::PUNCTUATOR_COMMA);
            break;  // Force to leave the 'implements' context.
        }
    }

    if (implements.empty()) {
        LogError(diagnostic::IMPLEMENTS_CLAUSE_EMPTY);
    }

    return implements;
}

bool TypedParser::CheckClassElementInterfaceBody(ir::AstNode *property, ArenaVector<ir::AstNode *> &properties)
{
    for (auto *it : property->AsTSInterfaceBody()->Body()) {
        properties.push_back(it);
    }

    return true;
}

bool TypedParser::CheckClassElement(ir::AstNode *property, ir::MethodDefinition *&ctor,
                                    [[maybe_unused]] ArenaVector<ir::AstNode *> &properties)
{
    if (property->IsTSInterfaceBody()) {
        return CheckClassElementInterfaceBody(property, properties);
    }

    return ParserImpl::CheckClassElement(property, ctor, properties);
}

// NOLINTNEXTLINE(google-default-arguments)
ir::ClassDefinition *TypedParser::ParseClassDefinition(ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags)
{
    Lexer()->NextToken();

    ir::Identifier *identNode = ParseClassIdent(modifiers);

    ir::TSTypeParameterDeclaration *typeParamDecl = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        auto options = TypeAnnotationParsingOptions::REPORT_ERROR;
        typeParamDecl = ParseTypeParameterDeclaration(&options);
    }

    varbinder::PrivateBinding privateBinding(Allocator(), ClassId()++);

    // Parse SuperClass
    auto [superClass, superTypeParams] = ParseSuperClass();

    if (superClass != nullptr) {
        modifiers |= ir::ClassDefinitionModifiers::HAS_SUPER;
    }

    // Parse implements clause
    ArenaVector<ir::TSClassImplements *> implements(Allocator()->Adapter());
    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_IMPLEMENTS) {
        Lexer()->NextToken();
        implements = ParseClassImplementClause();
    }

    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE, false);

    // Parse ClassBody
    auto [ctor, properties, bodyRange] = ParseClassBody(modifiers, flags);

    if (InAmbientContext()) {
        flags |= ir::ModifierFlags::DECLARE;
    }

    auto *classDefinition = AllocNode<ir::ClassDefinition>(
        Allocator(), identNode, typeParamDecl, superTypeParams, std::move(implements), ctor, superClass,
        std::move(properties), modifiers, flags, GetContext().GetLanguage());
    ES2PANDA_ASSERT(classDefinition != nullptr);
    classDefinition->SetInternalName(privateBinding.View());

    classDefinition->SetRange(bodyRange);

    return classDefinition;
}

void TypedParser::ValidateIndexSignatureTypeAnnotation(ir::TypeNode *typeAnnotation)
{
    if (typeAnnotation == nullptr) {
        LogError(diagnostic::INDEX_MISSING_TYPE);
    }
}

ir::AstNode *TypedParser::ParseProperty(const ArenaVector<ir::AstNode *> &properties, ClassElementDescriptor &desc,
                                        ir::Expression *propName)
{
    ir::AstNode *property = nullptr;
    ir::TypeNode *typeAnnotation = ParseClassKeyAnnotation();

    if (desc.isIndexSignature) {
        if (!desc.decorators.empty()) {
            LogError(diagnostic::DECORATORS_INVALID, {}, desc.decorators.front()->Start());
        }

        ValidateIndexSignatureTypeAnnotation(typeAnnotation);

        if ((desc.modifiers & ir::ModifierFlags::DECLARE) != 0) {
            LogError(diagnostic::DECALRE_IN_AMBIENT_CONTEXT);
        }

        property =
            AllocNode<ir::TSIndexSignature>(propName, typeAnnotation, desc.modifiers & ir::ModifierFlags::READONLY);
        ES2PANDA_ASSERT(property != nullptr);
        property->SetRange({property->AsTSIndexSignature()->Param()->Start(),
                            property->AsTSIndexSignature()->TypeAnnotation()->End()});
    } else {
        ValidateClassMethodStart(&desc, typeAnnotation);
        property = ParseClassProperty(&desc, properties, propName, typeAnnotation);

        if (!desc.decorators.empty()) {
            if (desc.isPrivateIdent) {
                LogError(diagnostic::DECORATORS_INVALID);
            }
            ES2PANDA_ASSERT(property != nullptr);
            property->AddDecorators(std::move(desc.decorators));
        }
    }

    ES2PANDA_ASSERT(property != nullptr);
    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SEMI_COLON &&
        Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE &&
        ((Lexer()->GetToken().Flags() & lexer::TokenFlags::NEW_LINE) == 0) &&
        !(property->IsMethodDefinition() &&
          property->AsMethodDefinition()->Value()->AsFunctionExpression()->Function()->Body() != nullptr)) {
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_SEMI_COLON);
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
        Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
    }

    GetContext().Status() &= ~ParserStatus::ALLOW_THIS_TYPE;

    if (desc.isPrivateIdent) {
        AddPrivateElement(property->AsClassElement());
    }

    return property;
}

ir::AstNode *TypedParser::ParseClassElement(const ArenaVector<ir::AstNode *> &properties,
                                            ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags)
{
    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_STATIC &&
        Lexer()->Lookahead() == lexer::LEX_CHAR_LEFT_BRACE) {
        return ParseClassStaticBlock();
    }

    ClassElementDescriptor desc(Allocator());

    desc.methodKind = ir::MethodDefinitionKind::METHOD;
    desc.newStatus = ParserStatus::ALLOW_SUPER;
    desc.hasSuperClass = (modifiers & ir::ClassDefinitionModifiers::HAS_SUPER) != 0;
    desc.propStart = Lexer()->GetToken().Start();

    ParseDecorators(desc.decorators);

    desc.modifiers = ParseModifiers();

    if (((desc.modifiers & ir::ModifierFlags::ABSTRACT) != 0) && ((flags & ir::ModifierFlags::ABSTRACT) == 0)) {
        LogError(diagnostic::ABSTRACT_METHODS_ONLY_IN_ABSTRACT_CLASS);
    }

    char32_t nextCp = Lexer()->Lookahead();
    CheckClassGeneratorMethod(&desc, &nextCp);
    ParseClassAccessor(&desc, &nextCp);

    if ((desc.modifiers & ir::ModifierFlags::STATIC) == 0) {
        GetContext().Status() |= ParserStatus::ALLOW_THIS_TYPE;
    }

    ir::Expression *propName = ParseClassKey(&desc);
    if (desc.methodKind == ir::MethodDefinitionKind::CONSTRUCTOR && !desc.decorators.empty()) {
        LogError(diagnostic::DECORATORS_INVALID, {}, desc.decorators.front()->Start());
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_QUESTION_MARK) {
        ParseOptionalClassElement(&desc);
    } else if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK) {
        if (desc.isIndexSignature || Lexer()->Lookahead() != lexer::LEX_CHAR_COLON) {
            // test exists for ts extension only
            LogExpectedToken(lexer::TokenType::PUNCTUATOR_SEMI_COLON);
        }

        desc.modifiers |= ir::ModifierFlags::DEFINITE;
        Lexer()->NextToken();
    }

    return ParseProperty(properties, desc, propName);
}

void TypedParser::ParseOptionalClassElement(ClassElementDescriptor *desc)
{
    if (desc->isIndexSignature) {
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_SEMI_COLON);  // no test
    }

    if (desc->methodKind == ir::MethodDefinitionKind::CONSTRUCTOR) {
        // test exists for ts extension only
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    }

    desc->modifiers |= ir::ModifierFlags::OPTIONAL;
    Lexer()->NextToken();
}

static std::pair<ir::ModifierFlags, ir::ModifierFlags> ParseActualNextStatus(lexer::TokenType keywordType)
{
    constexpr auto ASYNC_STATIC_READONLY =
        ir::ModifierFlags::ASYNC | ir::ModifierFlags::STATIC | ir::ModifierFlags::READONLY;
    constexpr auto ASYNC_STATIC_READONLY_AMBIENT_ABSTRACT =
        ASYNC_STATIC_READONLY | ir::ModifierFlags::DECLARE | ir::ModifierFlags::ABSTRACT;

    switch (keywordType) {
        case lexer::TokenType::KEYW_PUBLIC:
            return {ir::ModifierFlags::PUBLIC, ASYNC_STATIC_READONLY_AMBIENT_ABSTRACT};
        case lexer::TokenType::KEYW_PRIVATE:
            return {ir::ModifierFlags::PRIVATE, ASYNC_STATIC_READONLY_AMBIENT_ABSTRACT};
        case lexer::TokenType::KEYW_PROTECTED:
            return {ir::ModifierFlags::PROTECTED, ASYNC_STATIC_READONLY_AMBIENT_ABSTRACT};
        case lexer::TokenType::KEYW_INTERNAL:
            return {ir::ModifierFlags::INTERNAL, ASYNC_STATIC_READONLY_AMBIENT_ABSTRACT | ir::ModifierFlags::PROTECTED};
        case lexer::TokenType::KEYW_STATIC:
            return {ir::ModifierFlags::STATIC, ir::ModifierFlags::ASYNC | ir::ModifierFlags::READONLY |
                                                   ir::ModifierFlags::DECLARE | ir::ModifierFlags::ABSTRACT};
        case lexer::TokenType::KEYW_ASYNC:
            return {ir::ModifierFlags::ASYNC,
                    ir::ModifierFlags::READONLY | ir::ModifierFlags::DECLARE | ir::ModifierFlags::ABSTRACT};
        case lexer::TokenType::KEYW_ABSTRACT:
            return {ir::ModifierFlags::ABSTRACT,
                    ASYNC_STATIC_READONLY | ir::ModifierFlags::ACCESS | ir::ModifierFlags::DECLARE};
        case lexer::TokenType::KEYW_DECLARE:
            return {ir::ModifierFlags::DECLARE, ASYNC_STATIC_READONLY | ir::ModifierFlags::ACCESS};
        case lexer::TokenType::KEYW_READONLY:
            return {ir::ModifierFlags::READONLY,
                    ir::ModifierFlags::ASYNC | ir::ModifierFlags::DECLARE | ir::ModifierFlags::ABSTRACT};
        default:
            ES2PANDA_UNREACHABLE();
    }
}

ir::ModifierFlags TypedParser::ParseModifiers()
{
    ir::ModifierFlags resultStatus = ir::ModifierFlags::NONE;
    ir::ModifierFlags prevStatus = ir::ModifierFlags::ALL;

    while (IsModifierKind(Lexer()->GetToken())) {
        char32_t nextCp = Lexer()->Lookahead();
        if (!(nextCp != lexer::LEX_CHAR_EQUALS && nextCp != lexer::LEX_CHAR_SEMICOLON &&
              nextCp != lexer::LEX_CHAR_COMMA && nextCp != lexer::LEX_CHAR_LEFT_PAREN)) {
            break;
        }

        lexer::TokenFlags tokenFlags = Lexer()->GetToken().Flags();
        if ((tokenFlags & lexer::TokenFlags::HAS_ESCAPE) != 0) {
            LogError(diagnostic::KEYWORD_CONTAINS_ESCAPED_CHARS);
        }

        auto [actualStatus, nextStatus] = ParseActualNextStatus(Lexer()->GetToken().KeywordType());

        nextCp = Lexer()->Lookahead();
        if (nextCp == lexer::LEX_CHAR_COLON || nextCp == lexer::LEX_CHAR_COMMA ||
            nextCp == lexer::LEX_CHAR_RIGHT_PAREN || nextCp == lexer::LEX_CHAR_QUESTION ||
            nextCp == lexer::LEX_CHAR_RIGHT_BRACE || nextCp == lexer::LEX_CHAR_LESS_THAN) {
            break;
        }

        if ((prevStatus & actualStatus) == 0) {
            LogError(diagnostic::UNEXPECTED_MODIFIER);
        }

        if ((resultStatus & actualStatus) != 0) {
            LogError(diagnostic::DUPLICATED_MODIFIER);
        }

        if ((GetContext().Status() & ParserStatus::CONSTRUCTOR_FUNCTION) != 0 &&
            (actualStatus & ~ir::ModifierFlags::ALLOWED_IN_CTOR_PARAMETER) != 0) {
            LogParameterModifierError(actualStatus);
        }

        Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);

        resultStatus |= actualStatus;
        prevStatus = nextStatus;
    }

    return resultStatus;
}

ir::Expression *TypedParser::ParseQualifiedName(ExpressionParseFlags flags)
{
    ir::Expression *expr = nullptr;

    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_FORMAT:
            expr = ParseIdentifierFormatPlaceholder(std::nullopt);
            break;
        case lexer::TokenType::LITERAL_IDENT:
            expr = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
            ES2PANDA_ASSERT(expr != nullptr);
            expr->SetRange(Lexer()->GetToken().Loc());
            Lexer()->NextToken();
            break;
        default:
            LogError(diagnostic::ID_EXPECTED);
            return AllocBrokenExpression(Lexer()->GetToken().Loc());
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD) {
        expr = ParseQualifiedReference(expr, flags);
    }

    return expr;
}

ir::Expression *TypedParser::ParseLiteralIndent(ir::Expression *typeName, ExpressionParseFlags flags,
                                                const lexer::SourcePosition &startLoc)
{
    if ((flags & ExpressionParseFlags::POTENTIAL_CLASS_LITERAL) != 0 &&
        Lexer()->GetToken().Type() == lexer::TokenType::KEYW_CLASS) {
        typeName->SetRange({startLoc, Lexer()->GetToken().End()});
        return typeName;
    }

    LogError(diagnostic::ID_EXPECTED);
    return AllocBrokenExpression(Lexer()->GetToken().Loc());
}

ir::Expression *TypedParser::ParseQualifiedReference(ir::Expression *typeName, ExpressionParseFlags flags)
{
    lexer::SourcePosition startLoc = typeName->Start();

    do {
        Lexer()->NextToken();  // eat '.'

        ir::Identifier *propName {};
        if ((flags & ExpressionParseFlags::IMPORT) != 0 &&
            Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MULTIPLY) {
            Lexer()->NextToken();  // eat '*'
            propName = AllocNode<ir::Identifier>(varbinder::VarBinder::STAR_IMPORT, Allocator());
        } else if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_FORMAT) {
            propName = ParseIdentifierFormatPlaceholder(std::nullopt);
        } else if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
            return ParseLiteralIndent(typeName, flags, startLoc);
        } else {
            propName = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
        }

        if (propName == nullptr) {
            return AllocBrokenExpression(Lexer()->GetToken().Loc());
        }
        propName->SetRange(Lexer()->GetToken().Loc());

        typeName = AllocNode<ir::TSQualifiedName>(typeName, propName, Allocator());
        ES2PANDA_ASSERT(typeName != nullptr);
        typeName->SetRange({typeName->AsTSQualifiedName()->Left()->Start(), Lexer()->GetToken().End()});

        if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
            Lexer()->NextToken();
        }
    } while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD);

    typeName->SetRange({startLoc, Lexer()->GetToken().End()});

    return typeName;
}

//  Auxiliary method to reduce the size of functions.
ir::AstNode *TypedParser::ParseTypeParameterInstantiationImpl(TypeAnnotationParsingOptions *options)
{
    ArenaVector<ir::TypeNode *> params(Allocator()->Adapter());

    while (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_GREATER_THAN) {
        TypeAnnotationParsingOptions tmpOptions = *options &= ~TypeAnnotationParsingOptions::IGNORE_FUNCTION_TYPE;
        // Need to parse correctly the cases like `x: T|C<T|U>`
        tmpOptions &= ~TypeAnnotationParsingOptions::DISALLOW_UNION;
        ir::TypeNode *currentParam = ParseTypeAnnotation(&tmpOptions);

        if (currentParam == nullptr) {
            return nullptr;
        }

        params.push_back(currentParam);

        switch (Lexer()->GetToken().Type()) {
            case lexer::TokenType::PUNCTUATOR_COMMA: {
                Lexer()->NextToken();
                continue;
            }
            case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL:
            case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT: {
                Lexer()->BackwardToken(lexer::TokenType::PUNCTUATOR_GREATER_THAN, 1);
                break;
            }
            case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT: {
                Lexer()->BackwardToken(lexer::TokenType::PUNCTUATOR_GREATER_THAN, 2U);
                break;
            }
            case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT_EQUAL: {
                Lexer()->BackwardToken(lexer::TokenType::PUNCTUATOR_GREATER_THAN, 3U);
                break;
            }
            case lexer::TokenType::PUNCTUATOR_GREATER_THAN: {
                break;
            }
            default: {
                return nullptr;
            }
        }
    }

    return AllocNode<ir::TSTypeParameterInstantiation>(std::move(params));
}

ir::TSTypeParameterInstantiation *TypedParser::ParseTypeParameterInstantiation(TypeAnnotationParsingOptions *options)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN);

    const bool inPotentialNewArray = (*options & TypeAnnotationParsingOptions::POTENTIAL_NEW_ARRAY) != 0;
    *options &= ~TypeAnnotationParsingOptions::POTENTIAL_NEW_ARRAY;
    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat '<'

    ir::AstNode *typeParamInst;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_FORMAT &&
        Lexer()->Lookahead() == static_cast<char32_t>(EXPRESSION_FORMAT_NODE)) {
        typeParamInst = ParseTypeParametersFormatPlaceholder();
    } else {
        typeParamInst = ParseTypeParameterInstantiationImpl(options);
    }

    if (inPotentialNewArray) {
        *options |= TypeAnnotationParsingOptions::POTENTIAL_NEW_ARRAY;
    }

    lexer::SourcePosition endLoc = Lexer()->GetToken().End();
    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_GREATER_THAN) {
        if ((*options & TypeAnnotationParsingOptions::REPORT_ERROR) == 0) {
            return nullptr;
        }
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_GREATER_THAN);
    } else {
        Lexer()->NextToken();  // eat '>'
    }
    if (typeParamInst != nullptr) {
        typeParamInst->SetRange({startLoc, endLoc});
        return typeParamInst->AsTSTypeParameterInstantiation();
    }

    return nullptr;
}

ir::Statement *TypedParser::ParseDeclareAndDecorators(StatementParsingFlags flags)
{
    ArenaVector<ir::Decorator *> decorators(Allocator()->Adapter());

    ParseDecorators(decorators);

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_DECLARE) {
        CheckDeclare();
    }

    ir::Statement *stmt = TypedParser::ParseStatement(flags);

    GetContext().Status() &= ~ParserStatus::IN_AMBIENT_CONTEXT;
    if (stmt != nullptr) {  // Error processing.
        AddDecorators(stmt, decorators);
    }

    return stmt;
}

void TypedParser::ConvertThisKeywordToIdentIfNecessary()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_THIS) {
        Lexer()->GetToken().SetTokenType(lexer::TokenType::LITERAL_IDENT);
        Lexer()->GetToken().SetTokenStr(ERROR_LITERAL);
    }
}

ir::VariableDeclarator *TypedParser::ParseVariableDeclarator(ir::Expression *init, lexer::SourcePosition startLoc,
                                                             VariableParsingFlags flags)
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        return ParseVariableDeclaratorInitializer(init, flags, startLoc);
    }

    if (((flags & VariableParsingFlags::IN_FOR) == 0) && (init->IsArrayPattern() || init->IsObjectPattern())) {
        LogError(diagnostic::MISSING_INIT_IN_DEST_DEC);
    }

    lexer::SourcePosition endLoc = init->End();
    auto declarator = AllocNode<ir::VariableDeclarator>(GetFlag(flags), init);
    declarator->SetRange({startLoc, endLoc});

    return declarator;
}

void TypedParser::ParsePotentialOptionalFunctionParameter(ir::AnnotatedExpression *returnNode)
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_QUESTION_MARK) {
        switch (returnNode->Type()) {
            case ir::AstNodeType::IDENTIFIER: {
                returnNode->AsIdentifier()->SetOptional(true);
                break;
            }
            case ir::AstNodeType::OBJECT_PATTERN: {
                returnNode->AsObjectPattern()->SetOptional(true);
                break;
            }
            case ir::AstNodeType::ARRAY_PATTERN: {
                returnNode->AsArrayPattern()->SetOptional(true);
                break;
            }
            case ir::AstNodeType::REST_ELEMENT: {
                returnNode->AsRestElement()->SetOptional(true);
                break;
            }
            default: {
                LogError(diagnostic::UNEXPECTED_TOKEN_FOR_PARAM);
            }
        }

        Lexer()->NextToken();  // eat '?'
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        returnNode->SetTsTypeAnnotation(ParseTypeAnnotation(&options));
    }
}

ParserStatus TypedParser::ValidateArrowParameterAssignment(ir::AssignmentExpression *assignmentExpr)
{
    if (assignmentExpr->Right()->IsYieldExpression()) {
        LogError(diagnostic::YIELD_IN_ARROW_FUN_PARAM);
    }

    if (assignmentExpr->Right()->IsAwaitExpression()) {
        LogError(diagnostic::AWAIT_IN_ARROW_FUN_PARAM);
    }

    if (!assignmentExpr->ConvertibleToAssignmentPattern()) {
        LogError(diagnostic::INVALID_DESTRUCTURING_TARGET);
    }

    if (assignmentExpr->Left()->IsIdentifier() && assignmentExpr->Left()->AsIdentifier()->IsOptional()) {
        LogError(diagnostic::PARAM_CANNOT_HAVE_QUESTION_MARK, {}, assignmentExpr->Start());
    }

    ValidateArrowParameterBindings(assignmentExpr);
    return ParserStatus::HAS_COMPLEX_PARAM;
}

ParserStatus TypedParser::ValidateArrowParameterArray(ir::ArrayExpression *arrayPattern)
{
    if (!arrayPattern->ConvertibleToArrayPattern()) {
        LogError(diagnostic::INVALID_DESTRUCTURING_TARGET);
    }

    if (!InAmbientContext() && ((GetContext().Status() & ParserStatus::FUNCTION) != 0) && arrayPattern->IsOptional()) {
        LogError(diagnostic::BINDING_PATTERN_PARAM_CANNOT_BE_OPTIONAL, {}, arrayPattern->Start());
    }

    ValidateArrowParameterBindings(arrayPattern);
    return ParserStatus::HAS_COMPLEX_PARAM;
}

ParserStatus TypedParser::ValidateArrowParameterObject(ir::ObjectExpression *objectPattern)
{
    if (!objectPattern->ConvertibleToObjectPattern()) {
        LogError(diagnostic::INVALID_DESTRUCTURING_TARGET);
    }

    if (!InAmbientContext() && ((GetContext().Status() & ParserStatus::FUNCTION) != 0) && objectPattern->IsOptional()) {
        LogError(diagnostic::BINDING_PATTERN_PARAM_CANNOT_BE_OPTIONAL, {}, objectPattern->Start());
    }

    ValidateArrowParameterBindings(objectPattern);
    return ParserStatus::HAS_COMPLEX_PARAM;
}

ParserStatus TypedParser::ValidateArrowParameter(ir::Expression *expr, bool *seenOptional)
{
    switch (expr->Type()) {
        case ir::AstNodeType::SPREAD_ELEMENT: {
            if (!expr->AsSpreadElement()->ConvertibleToRest(true)) {
                LogError(diagnostic::INVALID_REST_ELEMENT);
            }

            [[fallthrough]];
        }
        case ir::AstNodeType::IDENTIFIER: {
            const util::StringView &identifier = expr->AsIdentifier()->Name();
            bool isOptional = expr->AsIdentifier()->IsOptional();
            if ((*seenOptional) && !isOptional) {
                LogError(diagnostic::REQUIRED_PARAM_AFTER_OPTIONAL, {}, expr->Start());
            }

            (*seenOptional) |= isOptional;

            if (identifier.Is("arguments")) {
                LogError(diagnostic::BINDING_ARGS_INVALID);
            } else if (identifier.Is("eval")) {
                LogError(diagnostic::BINDING_EVAL_INVALID);
            }

            ValidateArrowParameterBindings(expr);
            return ParserStatus::NO_OPTS;
        }
        case ir::AstNodeType::OBJECT_EXPRESSION:
            return ValidateArrowParameterObject(expr->AsObjectExpression());
        case ir::AstNodeType::ARRAY_EXPRESSION:
            return ValidateArrowParameterArray(expr->AsArrayExpression());
        case ir::AstNodeType::ASSIGNMENT_EXPRESSION:
            return ValidateArrowParameterAssignment(expr->AsAssignmentExpression());
        default:
            break;
    }
    LogError(diagnostic::INSUFFICIENT_PARAM_IN_ARROW_FUN);
    return ParserStatus::NO_OPTS;
}

ir::Statement *TypedParser::ParseTypeAliasStatement()
{
    return ParseTypeAliasDeclaration();
}

}  // namespace ark::es2panda::parser
