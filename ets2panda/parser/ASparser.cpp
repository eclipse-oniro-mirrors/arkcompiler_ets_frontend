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

#include "ASparser.h"

#include "parser/parserStatusContext.h"
#include "parserFlags.h"
#include "util/helpers.h"
#include "varbinder/privateBinding.h"
#include "varbinder/scope.h"
#include "varbinder/tsBinding.h"
#include "lexer/ASLexer.h"
#include "ir/base/decorator.h"
#include "ir/base/property.h"
#include "ir/base/spreadElement.h"
#include "ir/base/classElement.h"
#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/module/importDefaultSpecifier.h"
#include "ir/module/exportDefaultDeclaration.h"
#include "ir/module/exportNamedDeclaration.h"
#include "ir/module/importDeclaration.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/expressions/templateLiteral.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/taggedTemplateExpression.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/memberExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/sequenceExpression.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/expressions/literals/bigIntLiteral.h"
#include "ir/statements/emptyStatement.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/ifStatement.h"
#include "ir/statements/doWhileStatement.h"
#include "ir/statements/whileStatement.h"
#include "ir/statements/tryStatement.h"
#include "ir/statements/breakStatement.h"
#include "ir/statements/continueStatement.h"
#include "ir/statements/throwStatement.h"
#include "ir/statements/switchStatement.h"
#include "ir/statements/returnStatement.h"
#include "ir/statements/debuggerStatement.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/labelledStatement.h"
#include "ir/statements/variableDeclarator.h"
#include "ir/statements/functionDeclaration.h"
#include "ir/as/namedType.h"
#include "ir/as/prefixAssertionExpression.h"
#include "ir/ts/tsFunctionType.h"
#include "ir/ts/tsNonNullExpression.h"
#include "ir/ts/tsAsExpression.h"
#include "ir/ts/tsEnumDeclaration.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "ir/ts/tsModuleDeclaration.h"
#include "ir/ts/tsInterfaceHeritage.h"
#include "ir/base/tsIndexSignature.h"
#include "ir/base/tsMethodSignature.h"
#include "ir/base/tsPropertySignature.h"
#include "ir/ts/tsClassImplements.h"
#include "ir/ts/tsTypeParameterInstantiation.h"

namespace ark::es2panda::parser {
std::unique_ptr<lexer::Lexer> ASParser::InitLexer(const SourceFile &sourceFile)
{
    GetProgram()->SetSource(sourceFile);
    auto lexer = std::make_unique<lexer::ASLexer>(&GetContext(), DiagnosticEngine());
    SetLexer(lexer.get());
    return lexer;
}

ir::Decorator *ASParser::ParseDecorator()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_AT);

    lexer::SourcePosition start = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat '@'

    auto *expr = ParseLeftHandSideExpression();
    auto *decorator = AllocNode<ir::Decorator>(expr);
    ES2PANDA_ASSERT(decorator != nullptr);
    decorator->SetRange({start, expr->End()});
    return decorator;
}

void ASParser::AddDecorators(ir::AstNode *node, ArenaVector<ir::Decorator *> &decorators)
{
    if (decorators.empty()) {
        return;
    }

    if (!node->CanHaveDecorator(false)) {
        ThrowSyntaxError("Decorators are not valid here", decorators.front()->Start());
    }

    node->AddDecorators(std::move(decorators));
}

ir::TSTypeAliasDeclaration *ASParser::ParseTypeAliasDeclaration()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_TYPE);
    lexer::SourcePosition typeStart = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat type keyword
    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        ThrowSyntaxError("Identifier expected");
    }

    const util::StringView &ident = Lexer()->GetToken().Ident();

    auto *id = AllocNode<ir::Identifier>(ident, Allocator());
    ES2PANDA_ASSERT(id != nullptr);
    id->SetRange(Lexer()->GetToken().Loc());
    Lexer()->NextToken();

    ir::TSTypeParameterDeclaration *typeParamDecl = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        auto options = TypeAnnotationParsingOptions::REPORT_ERROR;
        typeParamDecl = ParseTypeParameterDeclaration(&options);
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        ThrowSyntaxError("'=' expected");
    }

    Lexer()->NextToken();  // eat '='

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_BITWISE_OR) {
        Lexer()->NextToken();  // eat '|'
    }

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TypeNode *typeAnnotation = ParseTypeAnnotation(&options);

    auto *typeAliasDecl = AllocNode<ir::TSTypeAliasDeclaration>(Allocator(), id, typeParamDecl, typeAnnotation);
    typeAliasDecl->SetRange({typeStart, Lexer()->GetToken().End()});

    return typeAliasDecl;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Statement *ASParser::ParseStatement(StatementParsingFlags flags)
{
    return ParseDeclareAndDecorators(flags);
}

void ASParser::ParseOptionalFunctionParameter(ir::AnnotatedExpression *returnNode, bool inRest)
{
    bool isOptional = false;

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_QUESTION_MARK) {
        if (inRest) {
            ThrowSyntaxError("A rest parameter cannot be optional");
        }

        ES2PANDA_ASSERT(returnNode->IsIdentifier());
        returnNode->AsIdentifier()->SetOptional(true);

        isOptional = true;
        Lexer()->NextToken();  // eat '?'
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        returnNode->SetTsTypeAnnotation(ParseTypeAnnotation(&options));
    } else if (!isOptional) {
        ThrowSyntaxError("':' expected");
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        return;
    }

    if (inRest) {
        ThrowSyntaxError("A rest parameter cannot have an initializer");
    }

    if (returnNode->IsIdentifier() && isOptional) {
        ThrowSyntaxError("Parameter cannot have question mark and initializer");
    }
}

ParserStatus ASParser::ValidateArrowExprIdentifier(ir::Expression *expr, bool *seenOptional)
{
    const util::StringView &identifier = expr->AsIdentifier()->Name();
    bool isOptional = expr->AsIdentifier()->IsOptional();
    if ((*seenOptional) != isOptional) {
        ThrowSyntaxError("A required parameter cannot follow an optional parameter.", expr->Start());
    }

    (*seenOptional) |= isOptional;

    if (expr->AsIdentifier()->TypeAnnotation() == nullptr) {
        ThrowSyntaxError("':' expected", expr->End());
    }

    if (identifier.Is("arguments")) {
        ThrowSyntaxError("Binding 'arguments' in strict mode is invalid");
    } else if (identifier.Is("eval")) {
        ThrowSyntaxError("Binding 'eval' in strict mode is invalid");
    }

    ValidateArrowParameterBindings(expr);
    return ParserStatus::NO_OPTS;
}

ParserStatus ASParser::ValidateArrowAssignmentExpr(ir::Expression *expr)
{
    auto *assignmentExpr = expr->AsAssignmentExpression();
    if (assignmentExpr->Right()->IsYieldExpression()) {
        ThrowSyntaxError("yield is not allowed in arrow function parameters");
    }

    if (assignmentExpr->Right()->IsAwaitExpression()) {
        ThrowSyntaxError("await is not allowed in arrow function parameters");
    }

    if (!assignmentExpr->ConvertibleToAssignmentPattern()) {
        ThrowSyntaxError("Invalid destructuring assignment target");
    }

    if (assignmentExpr->Left()->IsIdentifier() && assignmentExpr->Left()->AsIdentifier()->IsOptional()) {
        ThrowSyntaxError("Parameter cannot have question mark and initializer.", expr->Start());
    }

    ValidateArrowParameterBindings(expr);
    return ParserStatus::HAS_COMPLEX_PARAM;
}

ParserStatus ASParser::ValidateArrowParameter(ir::Expression *expr, bool *seenOptional)
{
    switch (expr->Type()) {
        case ir::AstNodeType::SPREAD_ELEMENT: {
            if (!expr->AsSpreadElement()->ConvertibleToRest(true)) {
                ThrowSyntaxError("Invalid rest element.");
            }

            [[fallthrough]];
        }
        case ir::AstNodeType::REST_ELEMENT: {
            if (expr->AsRestElement()->IsOptional()) {
                ThrowSyntaxError("A rest parameter cannot be optional.", expr->Start());
            }

            ValidateArrowParameterBindings(expr->AsRestElement()->Argument());
            return ParserStatus::HAS_COMPLEX_PARAM;
        }
        case ir::AstNodeType::IDENTIFIER: {
            return ValidateArrowExprIdentifier(expr, seenOptional);
        }
        case ir::AstNodeType::ASSIGNMENT_EXPRESSION: {
            return ValidateArrowAssignmentExpr(expr);
        }
        default: {
            break;
        }
    }
    ThrowSyntaxError("Insufficient formal parameter in arrow function.");
    return ParserStatus::NO_OPTS;
}

ArrowFunctionDescriptor ASParser::ConvertToArrowParameter(ir::Expression *expr, bool isAsync)
{
    auto arrowStatus = isAsync ? ParserStatus::ASYNC_FUNCTION : ParserStatus::NO_OPTS;
    ArenaVector<ir::Expression *> params(Allocator()->Adapter());

    if (expr == nullptr) {
        return ArrowFunctionDescriptor {std::move(params), Lexer()->GetToken().Start(), arrowStatus};
    }

    bool seenOptional = false;

    switch (expr->Type()) {
        case ir::AstNodeType::REST_ELEMENT:
        case ir::AstNodeType::IDENTIFIER:
        case ir::AstNodeType::ASSIGNMENT_EXPRESSION: {
            arrowStatus |= ValidateArrowParameter(expr, &seenOptional);

            params.push_back(expr);
            break;
        }
        case ir::AstNodeType::SEQUENCE_EXPRESSION: {
            auto &sequence = expr->AsSequenceExpression()->Sequence();

            for (auto *it : sequence) {
                arrowStatus |= ValidateArrowParameter(it, &seenOptional);
            }

            params.swap(sequence);
            break;
        }
        case ir::AstNodeType::CALL_EXPRESSION: {
            if (isAsync) {
                auto &arguments = expr->AsCallExpression()->Arguments();

                for (auto *it : arguments) {
                    arrowStatus |= ValidateArrowParameter(it, &seenOptional);
                }

                params.swap(arguments);
                break;
            }

            [[fallthrough]];
        }
        default: {
            ThrowSyntaxError("Unexpected token, arrow (=>)");
        }
    }

    return ArrowFunctionDescriptor {std::move(params), expr->Start(), arrowStatus};
}

// NOLINTNEXTLINE(google-default-arguments)
std::tuple<ir::AnnotatedExpression *, bool> ASParser::ParsePatternElementToken(ExpressionParseFlags flags)
{
    ir::AnnotatedExpression *returnNode = nullptr;
    bool isOptional = false;

    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD: {
            if ((flags & ExpressionParseFlags::IN_REST) != 0) {
                ThrowSyntaxError("Unexpected token");
            }

            returnNode = ParseSpreadElement(ExpressionParseFlags::MUST_BE_PATTERN);
            break;
        }
        case lexer::TokenType::LITERAL_IDENT: {
            returnNode = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
            ES2PANDA_ASSERT(returnNode != nullptr);
            returnNode->SetRange(Lexer()->GetToken().Loc());
            Lexer()->NextToken();

            bool questionMark = Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_QUESTION_MARK;
            if (questionMark && ((flags & ExpressionParseFlags::IN_REST) != 0)) {
                ThrowSyntaxError("A rest parameter cannot be optional");
            }

            if (questionMark) {
                isOptional = true;

                returnNode->AsIdentifier()->SetOptional(true);
                Lexer()->NextToken();
            }

            if (!isOptional && Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
                ThrowSyntaxError("':' expected");
            } else if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
                Lexer()->NextToken();  // eat ':'
                TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
                returnNode->SetTsTypeAnnotation(ParseTypeAnnotation(&options));
            }

            break;
        }
        default: {
            ThrowSyntaxError("Identifier expected");
        }
    }
    return {returnNode, isOptional};
}

ir::Expression *ASParser::ParsePatternElement(ExpressionParseFlags flags, bool allowDefault)
{
    ir::AnnotatedExpression *returnNode = nullptr;
    bool isOptional = false;
    std::tie(returnNode, isOptional) = ParsePatternElementToken(flags);

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        return returnNode;
    }

    if ((flags & ExpressionParseFlags::IN_REST) != 0) {
        ThrowSyntaxError("A rest parameter cannot have an initializer.");
    }

    if (!allowDefault) {
        ThrowSyntaxError("Invalid destructuring assignment target");
    }

    if (isOptional) {
        ThrowSyntaxError("Parameter cannot have question mark and initializer");
    }

    Lexer()->NextToken();

    if (((GetContext().Status() & ParserStatus::GENERATOR_FUNCTION) != 0) &&
        Lexer()->GetToken().Type() == lexer::TokenType::KEYW_YIELD) {
        ThrowSyntaxError("Yield is not allowed in generator parameters");
    }

    ir::Expression *rightNode = ParseExpression();

    auto *assignmentExpression = AllocNode<ir::AssignmentExpression>(
        ir::AstNodeType::ASSIGNMENT_PATTERN, returnNode, rightNode, lexer::TokenType::PUNCTUATOR_SUBSTITUTION);
    assignmentExpression->SetRange({returnNode->Start(), rightNode->End()});

    return assignmentExpression;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ASParser::ParsePropertyDefinition([[maybe_unused]] ExpressionParseFlags flags)
{
    Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);

    ir::Expression *key = nullptr;

    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_STRING) {
            ThrowSyntaxError("Identifier expected");
        }

        key = AllocNode<ir::StringLiteral>(Lexer()->GetToken().String());
    } else {
        key = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    }

    ES2PANDA_ASSERT(key != nullptr);
    key->SetRange(Lexer()->GetToken().Loc());

    Lexer()->NextToken();

    ir::Expression *value = nullptr;

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();
        value = ParseExpression();
    } else if (!key->IsStringLiteral()) {
        value = key;
    } else {
        ThrowSyntaxError("':' expected");
    }

    auto *property = AllocNode<ir::Property>(key, value);
    ES2PANDA_ASSERT(property != nullptr);
    property->SetRange({key->Start(), value->End()});
    return property;
}

bool ASParser::CurrentIsBasicType()
{
    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::LITERAL_STRING:
        case lexer::TokenType::LITERAL_FALSE:
        case lexer::TokenType::LITERAL_TRUE:
        case lexer::TokenType::LITERAL_NULL:
        case lexer::TokenType::KEYW_VOID: {
            return true;
        }
        case lexer::TokenType::LITERAL_IDENT: {
            switch (Lexer()->GetToken().KeywordType()) {
                case lexer::TokenType::KEYW_I8:
                case lexer::TokenType::KEYW_I16:
                case lexer::TokenType::KEYW_I32:
                case lexer::TokenType::KEYW_I64:
                case lexer::TokenType::KEYW_ISIZE:
                case lexer::TokenType::KEYW_U8:
                case lexer::TokenType::KEYW_U16:
                case lexer::TokenType::KEYW_U32:
                case lexer::TokenType::KEYW_U64:
                case lexer::TokenType::KEYW_USIZE:
                case lexer::TokenType::KEYW_F32:
                case lexer::TokenType::KEYW_F64:
                case lexer::TokenType::KEYW_V128:
                case lexer::TokenType::KEYW_FUNCREF:
                case lexer::TokenType::KEYW_EXTERNREF:
                case lexer::TokenType::KEYW_ANYREF:
                case lexer::TokenType::KEYW_EQREF:
                case lexer::TokenType::KEYW_I31REF:
                case lexer::TokenType::KEYW_DATAREF: {
                    return true;
                }
                default: {
                    break;
                }
            }

            break;
        }
        default: {
            break;
        }
    }
    return false;
}

ir::TypeNode *ASParser::ParseFunctionType(lexer::SourcePosition startLoc)
{
    auto params = ParseFunctionParams();

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_ARROW) {
        ThrowSyntaxError("'=>' expected");
    }

    Lexer()->NextToken();  // eat '=>'

    TypeAnnotationParsingOptions options =
        TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::CAN_BE_TS_TYPE_PREDICATE;
    ir::TypeNode *returnTypeAnnotation = ParseTypeAnnotation(&options);

    auto signature = ir::FunctionSignature(nullptr, std::move(params), returnTypeAnnotation);
    auto funcType = AllocNode<ir::TSFunctionType>(std::move(signature), Allocator());

    funcType->SetRange({startLoc, returnTypeAnnotation->End()});

    return funcType;
}

ir::TypeNode *ASParser::ParseParenthesizedOrFunctionType(bool throwError)
{
    lexer::SourcePosition typeStart = Lexer()->GetToken().Start();
    const auto startPos = Lexer()->Save();
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    Lexer()->NextToken();  // eat '('

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::NO_OPTS;
    ir::TypeNode *type = ParseTypeAnnotation(&options);

    if (type == nullptr) {
        Lexer()->Rewind(startPos);

        if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
            if (throwError) {
                ThrowSyntaxError("Identifier expected");
            }

            return nullptr;
        }

        return ParseFunctionType(typeStart);
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_QUESTION_MARK ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->Rewind(startPos);
        return ParseFunctionType(typeStart);
    }

    if (throwError && Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        ThrowSyntaxError("')' expected");
    }

    lexer::SourcePosition endLoc = Lexer()->GetToken().End();
    Lexer()->NextToken();  // eat ')'

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_ARROW) {
        Lexer()->Rewind(startPos);

        return ParseFunctionType(typeStart);
    }

    type->SetRange({typeStart, endLoc});
    return type;
}

ir::TypeNode *ASParser::ParseTypeAnnotationLiteralIdentHelper(ir::TypeNode *type, TypeAnnotationParsingOptions *options)
{
    auto *typeName = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(typeName != nullptr);
    typeName->SetRange(Lexer()->GetToken().Loc());
    type = AllocNode<ir::NamedType>(typeName, Allocator());
    ES2PANDA_ASSERT(type != nullptr);
    type->SetRange(Lexer()->GetToken().Loc());
    Lexer()->NextToken();

    ir::NamedType *current = type->AsNamedType();
    while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD) {
        Lexer()->NextToken();

        if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
            ThrowSyntaxError("Identifier expected");
        }

        typeName = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
        ES2PANDA_ASSERT(typeName != nullptr);
        typeName->SetRange(Lexer()->GetToken().Loc());
        auto *next = AllocNode<ir::NamedType>(typeName, Allocator());
        current->SetRange(Lexer()->GetToken().Loc());
        current->SetNext(next);
        current = next;
        Lexer()->NextToken();
    }

    ir::TSTypeParameterInstantiation *typeParams = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        typeParams = ParseTypeParameterInstantiation(options);
        if (typeParams == nullptr) {
            return nullptr;
        }

        type->AsNamedType()->SetTypeParams(typeParams);
    }
    return type;
}

ir::TypeNode *ASParser::ParseTypeAnnotationTokens(ir::TypeNode *type, bool throwError,
                                                  TypeAnnotationParsingOptions *options)
{
    util::StringView name = "";
    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS: {
            return ParseParenthesizedOrFunctionType(throwError);
        }
        case lexer::TokenType::KEYW_VOID: {
            name = "void";
            break;
        }
        case lexer::TokenType::KEYW_THIS: {
            name = "this";
            break;
        }
        case lexer::TokenType::LITERAL_FALSE:
        case lexer::TokenType::LITERAL_TRUE: {
            name = "bool";
            break;
        }
        case lexer::TokenType::LITERAL_NULL: {
            name = "null";
            break;
        }
        case lexer::TokenType::LITERAL_STRING: {
            name = "string";
            break;
        }
        case lexer::TokenType::LITERAL_IDENT: {
            return ParseTypeAnnotationLiteralIdentHelper(type, options);
        }
        default: {
            if (throwError) {
                ThrowSyntaxError("Type expected");
            }

            return nullptr;
        }
    }

    auto *typeName = AllocNode<ir::Identifier>(name, Allocator());
    ES2PANDA_ASSERT(typeName != nullptr);
    typeName->SetRange(Lexer()->GetToken().Loc());
    type = AllocNode<ir::NamedType>(typeName, Allocator());
    ES2PANDA_ASSERT(type != nullptr);
    type->SetRange(Lexer()->GetToken().Loc());
    Lexer()->NextToken();
    return type;
}

ir::TypeNode *ASParser::ParseTypeAnnotationTokensBitwiseOr(ir::TypeNode *type, bool throwError, bool isNullable)
{
    while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_BITWISE_OR) {
        Lexer()->NextToken();

        if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_NULL) {
            if (throwError) {
                ThrowSyntaxError("'null' expected");
            }

            return nullptr;
        }

        if (!isNullable) {
            isNullable = true;
            if (type->IsTSFunctionType()) {
                type->AsTSFunctionType()->SetNullable(isNullable);
            } else {
                ES2PANDA_ASSERT(type->IsNamedType());
                type->AsNamedType()->SetNullable(isNullable);
            }
        }

        type->SetEnd(Lexer()->GetToken().End());
        Lexer()->NextToken();
    }
    return type;
}

ir::TypeNode *ASParser::ParseTypeAnnotationTokenLeftSquareBracket(ir::TypeNode *type, bool throwError, bool isNullable)
{
    while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
        Lexer()->NextToken();

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
            if (throwError) {
                ThrowSyntaxError("']' expected");
            }

            return nullptr;
        }

        Lexer()->NextToken();

        isNullable = false;

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_BITWISE_OR) {
            Lexer()->NextToken();

            bool isLiteralNull = Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_NULL;
            if (isLiteralNull && throwError) {
                ThrowSyntaxError("'null' expected");
            }
            if (isLiteralNull) {
                return nullptr;
            }

            isNullable = true;
        }

        const lexer::SourcePosition &startPos = type->Start();

        util::StringView name = "Array";
        auto *typeName = AllocNode<ir::Identifier>(name, Allocator());
        ES2PANDA_ASSERT(typeName != nullptr);
        typeName->SetRange(Lexer()->GetToken().Loc());

        ArenaVector<ir::TypeNode *> params(Allocator()->Adapter());
        params.push_back(type);
        auto *typeParamInst = AllocNode<ir::TSTypeParameterInstantiation>(std::move(params));

        type = AllocNode<ir::NamedType>(typeName, Allocator());
        type->AsNamedType()->SetTypeParams(typeParamInst);
        type->AsNamedType()->SetNullable(isNullable);
        type->SetRange({startPos, Lexer()->GetToken().End()});

        if (isNullable) {
            Lexer()->NextToken();
            break;
        }
    }
    return type;
}

ir::TypeNode *ASParser::ParseTypeAnnotation(TypeAnnotationParsingOptions *options)
{
    bool reportError = (((*options) & TypeAnnotationParsingOptions::REPORT_ERROR) != 0);
    ir::TypeNode *type = ParseTypeAnnotationTokens(nullptr, reportError, options);
    if (type == nullptr) {
        return nullptr;
    }

    bool isNullable = false;
    type = ParseTypeAnnotationTokensBitwiseOr(type, reportError, isNullable);
    if (type == nullptr) {
        return nullptr;
    }

    type = ParseTypeAnnotationTokenLeftSquareBracket(type, reportError, isNullable);
    return type;
}

ir::ArrowFunctionExpression *ASParser::ParsePotentialArrowExpression(
    // CC-OFFNXT(G.FMT.06-CPP) project code style
    [[maybe_unused]] ir::Expression **returnExpression, [[maybe_unused]] const lexer::SourcePosition &startLoc)
{
    return nullptr;
}

bool ASParser::ParsePotentialNonNullExpression(ir::Expression **returnExpression, lexer::SourcePosition startLoc)
{
    if (returnExpression == nullptr || Lexer()->GetToken().NewLine()) {
        return true;
    }

    *returnExpression = AllocNode<ir::TSNonNullExpression>(*returnExpression);
    ES2PANDA_ASSERT(*returnExpression != nullptr);
    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    (*returnExpression)->SetRange({startLoc, Lexer()->GetToken().End()});
    Lexer()->NextToken();
    return false;
}

bool ASParser::ParsePotentialGenericFunctionCall(ir::Expression *primaryExpr, ir::Expression **returnExpression,
                                                 const lexer::SourcePosition &startLoc, bool ignoreCallExpression)
{
    if (Lexer()->Lookahead() == lexer::LEX_CHAR_LESS_THAN ||
        (!primaryExpr->IsIdentifier() && !primaryExpr->IsMemberExpression())) {
        return true;
    }

    const auto savedPos = Lexer()->Save();

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT) {
        Lexer()->BackwardToken(lexer::TokenType::PUNCTUATOR_LESS_THAN, 1);
    }

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::NO_OPTS;
    ir::TSTypeParameterInstantiation *typeParams = ParseTypeParameterInstantiation(&options);

    if (typeParams == nullptr) {
        Lexer()->Rewind(savedPos);
        return true;
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::EOS) {
        ThrowSyntaxError("'(' or '`' expected");
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
        if (!ignoreCallExpression) {
            *returnExpression = ParseCallExpression(*returnExpression, false);
            (*returnExpression)->AsCallExpression()->SetTypeParams(typeParams);
            return false;
        }

        return true;
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_BACK_TICK) {
        ir::TemplateLiteral *propertyNode = ParseTemplateLiteral();
        lexer::SourcePosition endLoc = propertyNode->End();

        *returnExpression = AllocNode<ir::TaggedTemplateExpression>(*returnExpression, propertyNode, typeParams);
        ES2PANDA_ASSERT(returnExpression != nullptr);
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        (*returnExpression)->SetRange({startLoc, endLoc});
        return false;
    }

    Lexer()->Rewind(savedPos);
    return true;
}

bool ASParser::IsNamedFunctionExpression()
{
    return Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS &&
           Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LESS_THAN;
}

ir::Expression *ASParser::ParsePotentialAsExpression(ir::Expression *primaryExpression)
{
    if (Lexer()->GetToken().KeywordType() != lexer::TokenType::KEYW_AS) {
        return nullptr;
    }

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    Lexer()->NextToken();
    ir::TypeNode *type = ParseTypeAnnotation(&options);
    auto *asExpression = AllocNode<ir::TSAsExpression>(primaryExpression, type, false);
    return asExpression;
}

ir::Identifier *ASParser::ParsePrimaryExpressionIdent([[maybe_unused]] ExpressionParseFlags flags)
{
    auto *identNode = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(identNode != nullptr);
    identNode->SetRange(Lexer()->GetToken().Loc());

    Lexer()->NextToken();

    ParsePotentialOptionalFunctionParameter(identNode);

    return identNode;
}

bool ASParser::ValidateArrowFunctionRestParameter([[maybe_unused]] ir::SpreadElement *restElement)
{
    ParseOptionalFunctionParameter(restElement, true);

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        ThrowSyntaxError("')' expected");
    }

    return true;
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
ArenaVector<ir::TSInterfaceHeritage *> ASParser::ParseInterfaceExtendsClause()
{
    Lexer()->NextToken();  // eat extends keyword

    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        ThrowSyntaxError("Identifier expected");
    }

    const lexer::SourcePosition &heritageStart = Lexer()->GetToken().Start();
    lexer::SourcePosition heritageEnd = Lexer()->GetToken().End();
    auto *extendsName = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(extendsName != nullptr);
    extendsName->SetRange(Lexer()->GetToken().Loc());
    auto *extendsClause = AllocNode<ir::NamedType>(extendsName, Allocator());
    ES2PANDA_ASSERT(extendsClause != nullptr);
    extendsClause->SetRange(Lexer()->GetToken().Loc());
    Lexer()->NextToken();

    ir::NamedType *current = extendsClause->AsNamedType();
    while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD) {
        Lexer()->NextToken();

        if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
            ThrowSyntaxError("Identifier expected");
        }

        extendsName = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
        extendsName->SetRange(Lexer()->GetToken().Loc());
        auto *next = AllocNode<ir::NamedType>(extendsName, Allocator());
        current->SetRange(Lexer()->GetToken().Loc());
        current->SetNext(next);
        current = next;
        heritageEnd = Lexer()->GetToken().End();
        Lexer()->NextToken();
    }

    if (Lexer()->Lookahead() == lexer::LEX_CHAR_LESS_THAN) {
        Lexer()->ForwardToken(lexer::TokenType::PUNCTUATOR_LESS_THAN, 1);
    } else {
        Lexer()->NextToken();
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        extendsClause->AsNamedType()->SetTypeParams(ParseTypeParameterInstantiation(&options));
        heritageEnd = Lexer()->GetToken().End();
    }

    extendsClause->SetRange({heritageStart, heritageEnd});

    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_IMPLEMENTS) {
        ThrowSyntaxError("Interface declaration cannot have 'implements' clause");
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        ThrowSyntaxError("'{' expected");
    }

    ArenaVector<ir::TSInterfaceHeritage *> extends(Allocator()->Adapter());
    auto *heritage = AllocNode<ir::TSInterfaceHeritage>(extendsClause);
    heritage->SetRange(extendsClause->Range());
    extends.push_back(heritage);
    return extends;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::TSIndexSignature *ASParser::ParseIndexSignature(const lexer::SourcePosition &startLoc, bool isReadonly)
{
    Lexer()->NextToken();  // eat '['

    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        ThrowSyntaxError("Identifier expected.");
    }

    if (!Lexer()->GetToken().Ident().Is("key")) {
        ThrowSyntaxError("'key' expected.");
    }

    auto *key = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(key != nullptr);
    key->SetRange(Lexer()->GetToken().Loc());

    Lexer()->NextToken();  // eat key

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
        ThrowSyntaxError("':' expected.");
    }

    Lexer()->NextToken();  // eat ':'

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TypeNode *keyType = ParseTypeAnnotation(&options);
    key->SetTsTypeAnnotation(keyType);

    if (!keyType->IsNamedType()) {
        ThrowSyntaxError("Type expected.");
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
        ThrowSyntaxError("']' expected.");
    }

    Lexer()->NextToken();  // eat ']'

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
        ThrowSyntaxError("':' expected.");
    }

    Lexer()->NextToken();  // eat ':'

    ir::TypeNode *typeAnnotation = ParseTypeAnnotation(&options);

    if (!typeAnnotation->IsNamedType()) {
        ThrowSyntaxError("Identifier expected.");
    }

    auto *indexSignature = AllocNode<ir::TSIndexSignature>(key, typeAnnotation, isReadonly);
    indexSignature->SetRange({startLoc, Lexer()->GetToken().End()});
    return indexSignature;
}

std::tuple<ir::Expression *, bool> ASParser::ParseInterfacePropertyKey()
{
    ir::Expression *key = nullptr;

    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::LITERAL_IDENT: {
            const util::StringView &ident = Lexer()->GetToken().Ident();
            key = AllocNode<ir::Identifier>(ident, Allocator());
            ES2PANDA_ASSERT(key != nullptr);
            key->SetRange(Lexer()->GetToken().Loc());
            break;
        }
        case lexer::TokenType::LITERAL_STRING: {
            const util::StringView &string = Lexer()->GetToken().String();
            key = AllocNode<ir::StringLiteral>(string);
            ES2PANDA_ASSERT(key != nullptr);
            key->SetRange(Lexer()->GetToken().Loc());
            break;
        }
        case lexer::TokenType::LITERAL_NUMBER: {
            if ((Lexer()->GetToken().Flags() & lexer::TokenFlags::NUMBER_BIGINT) != 0) {
                key = AllocNode<ir::BigIntLiteral>(Lexer()->GetToken().BigInt());
            } else {
                key = AllocNode<ir::NumberLiteral>(Lexer()->GetToken().GetNumber());
            }

            ES2PANDA_ASSERT(key != nullptr);
            key->SetRange(Lexer()->GetToken().Loc());
            break;
        }
        default: {
            ThrowSyntaxError("Unexpected token in property key");
        }
    }

    Lexer()->NextToken();
    return {key, false};
}

ir::AstNode *ASParser::ParsePropertyOrMethodSignature(const lexer::SourcePosition &startLoc, bool isReadonly)
{
    auto [key, isComputed] = ParseInterfacePropertyKey();

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_QUESTION_MARK) {
        ThrowSyntaxError("Optional properties are not supported.");
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        ir::TSTypeParameterDeclaration *typeParamDecl = nullptr;

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
            auto options = TypeAnnotationParsingOptions::REPORT_ERROR;
            typeParamDecl = ParseTypeParameterDeclaration(&options);
        }

        FunctionParameterContext funcParamContext(&GetContext());
        auto params = ParseFunctionParams();

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
            ThrowSyntaxError("Type expected.");
        }

        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options =
            TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::CAN_BE_TS_TYPE_PREDICATE;
        ir::TypeNode *returnType = ParseTypeAnnotation(&options);

        auto signature = ir::FunctionSignature(typeParamDecl, std::move(params), returnType);
        auto *methodSignature = AllocNode<ir::TSMethodSignature>(key, std::move(signature), isComputed, false);
        methodSignature->SetRange({startLoc, Lexer()->GetToken().End()});
        return methodSignature;
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
        ThrowSyntaxError("Type expected.");
    }

    Lexer()->NextToken();  // eat ':'
    TypeAnnotationParsingOptions options =
        TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::BREAK_AT_NEW_LINE;
    ir::TypeNode *typeAnnotation = ParseTypeAnnotation(&options);

    auto *propertySignature = AllocNode<ir::TSPropertySignature>(key, typeAnnotation, isComputed, false, isReadonly);
    propertySignature->SetRange({startLoc, Lexer()->GetToken().End()});
    return propertySignature;
}

ir::AstNode *ASParser::ParseTypeLiteralOrInterfaceMember()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_AT) {
        ThrowSyntaxError("Decorators are not allowed here");
    }

    char32_t nextCp = Lexer()->Lookahead();
    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();
    bool isReadonly = Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_READONLY &&
                      nextCp != lexer::LEX_CHAR_LEFT_PAREN && nextCp != lexer::LEX_CHAR_COLON;
    if (isReadonly) {
        Lexer()->NextToken();  // eat 'readonly"
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
        return ParseIndexSignature(startLoc, isReadonly);
    }

    return ParsePropertyOrMethodSignature(startLoc, isReadonly);
}

ArenaVector<ir::TSClassImplements *> ASParser::ParseClassImplementClause()
{
    ArenaVector<ir::TSClassImplements *> implements(Allocator()->Adapter());

    while (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
            ThrowSyntaxError("Identifier expected");
        }

        const lexer::SourcePosition &implementStart = Lexer()->GetToken().Start();
        auto *implementsName = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
        ES2PANDA_ASSERT(implementsName != nullptr);
        implementsName->SetRange(Lexer()->GetToken().Loc());
        auto *implementsClause = AllocNode<ir::NamedType>(implementsName, Allocator());
        ES2PANDA_ASSERT(implementsClause != nullptr);
        implementsClause->SetRange(Lexer()->GetToken().Loc());
        Lexer()->NextToken();

        ir::NamedType *current = implementsClause->AsNamedType();
        while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD) {
            Lexer()->NextToken();

            if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
                ThrowSyntaxError("Identifier expected");
            }

            implementsName = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
            ES2PANDA_ASSERT(implementsName != nullptr);
            implementsName->SetRange(Lexer()->GetToken().Loc());
            auto *next = AllocNode<ir::NamedType>(implementsName, Allocator());
            current->SetRange(Lexer()->GetToken().Loc());
            current->SetNext(next);
            current = next;
            Lexer()->NextToken();
        }

        ir::TSTypeParameterInstantiation *implTypeParams = nullptr;
        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT) {
            Lexer()->BackwardToken(lexer::TokenType::PUNCTUATOR_LESS_THAN, 1);
        }

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN ||
            Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT) {
            TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
            implTypeParams = ParseTypeParameterInstantiation(&options);
        }

        auto *impl = AllocNode<ir::TSClassImplements>(current, implTypeParams);
        ES2PANDA_ASSERT(impl != nullptr);
        impl->SetRange({implementStart, Lexer()->GetToken().End()});
        implements.push_back(impl);

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA) {
            Lexer()->NextToken();
            continue;
        }
        ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE, false);
    }

    if (implements.empty()) {
        ThrowSyntaxError("Implements clause can not be empty");
    }

    return implements;
}

ir::TypeNode *ASParser::ParseClassKeyAnnotation()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options =
            TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::BREAK_AT_NEW_LINE;
        return ParseTypeAnnotation(&options);
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS &&
        Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        ThrowSyntaxError("Type expected");
    }

    return nullptr;
}

void ASParser::ValidateClassMethodStart(ClassElementDescriptor *desc, ir::TypeNode *typeAnnotation)
{
    if (typeAnnotation == nullptr && (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS ||
                                      Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN)) {
        if ((desc->modifiers & ir::ModifierFlags::DECLARE) != 0) {
            ThrowSyntaxError("'declare' modifier cannot appear on class elements of this kind");
        }

        desc->classMethod = true;
    } else {
        if (((desc->modifiers & ir::ModifierFlags::ASYNC) != 0) || desc->isGenerator) {
            ThrowSyntaxError("Expected '('");
        }
        desc->classField = true;

        if (desc->invalidComputedProperty) {
            ThrowSyntaxError(
                "Computed property name must refer to a symbol or "
                "literal expression whose value is "
                "number or string");
        }
    }

    if ((desc->modifiers & ir::ModifierFlags::ASYNC) != 0) {
        desc->newStatus |= ParserStatus::ASYNC_FUNCTION;
    }

    if (desc->isGenerator) {
        desc->newStatus |= ParserStatus::GENERATOR_FUNCTION;
    }
}

void ASParser::ValidateClassSetter(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                                   ir::Expression *propName, ir::ScriptFunction *func)
{
    ValidateGetterSetter(ir::MethodDefinitionKind::SET, func);

    if ((desc->modifiers & ir::ModifierFlags::STATIC) == 0) {
        ir::ModifierFlags access = GetAccessability(desc->modifiers);
        CheckAccessorPair(properties, propName, ir::MethodDefinitionKind::GET, access);
    }
}

void ASParser::ValidateClassGetter(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                                   ir::Expression *propName, [[maybe_unused]] ir::ScriptFunction *func)
{
    if ((desc->modifiers & ir::ModifierFlags::STATIC) != 0) {
        ir::ModifierFlags access = GetAccessability(desc->modifiers);

        CheckAccessorPair(properties, propName, ir::MethodDefinitionKind::SET, access);
    }
}

ir::ClassElement *ASParser::ParseClassStaticBlock()
{
    ThrowSyntaxError("Unexpected token");
    return nullptr;
}

void ASParser::ParseOptionalClassElement([[maybe_unused]] ClassElementDescriptor *desc)
{
    ThrowSyntaxError("Optional properties are not supported");
}

void ASParser::ValidateIndexSignatureTypeAnnotation(ir::TypeNode *typeAnnotation)
{
    if (typeAnnotation == nullptr) {
        ThrowSyntaxError("':' expected");
    }

    if (!typeAnnotation->IsNamedType()) {
        ThrowSyntaxError("Identifier expected");
    }
}

bool ASParser::IsModifierKind(const lexer::Token &token)
{
    switch (token.KeywordType()) {
        case lexer::TokenType::KEYW_PUBLIC:
        case lexer::TokenType::KEYW_PRIVATE:
        case lexer::TokenType::KEYW_PROTECTED:
        case lexer::TokenType::KEYW_STATIC:
        case lexer::TokenType::KEYW_ASYNC:
        case lexer::TokenType::KEYW_DECLARE:
        case lexer::TokenType::KEYW_READONLY:
            return true;
        default:
            break;
    }

    return false;
}

void ASParser::ConsumeClassPrivateIdentifier([[maybe_unused]] ClassElementDescriptor *desc,
                                             [[maybe_unused]] char32_t *nextCp)
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_HASH_MARK) {
        ThrowSyntaxError("Invalid character");
    }
}

std::tuple<bool, bool, bool> ASParser::ParseComputedClassFieldOrIndexSignature(ir::Expression **propName)
{
    Lexer()->NextToken();  // eat left square bracket

    if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT &&
        Lexer()->Lookahead() == lexer::LEX_CHAR_COLON) {
        if (!Lexer()->GetToken().Ident().Is("key")) {
            ThrowSyntaxError("'key' expected.");
        }

        auto id = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
        ES2PANDA_ASSERT(id != nullptr);
        id->SetRange(Lexer()->GetToken().Loc());

        Lexer()->NextToken();  // eat param

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
            ThrowSyntaxError("':' expected");
        }

        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        ir::TypeNode *typeAnnotation = ParseTypeAnnotation(&options);

        if (!typeAnnotation->IsNamedType()) {
            ThrowSyntaxError("Type expected");
        }

        id->SetTsTypeAnnotation(typeAnnotation);
        *propName = id;

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
            ThrowSyntaxError("']' expected");
        }

        Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);

        return {false, false, true};
    }

    *propName = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);

    bool invalidComputedProperty =
        !(*propName)->IsNumberLiteral() && !(*propName)->IsStringLiteral() &&
        !((*propName)->IsMemberExpression() && (*propName)->AsMemberExpression()->Object()->IsIdentifier() &&
          (*propName)->AsMemberExpression()->Object()->AsIdentifier()->Name().Is("Symbol"));

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
        ThrowSyntaxError("Unexpected token, expected ']'");
    }

    return {true, invalidComputedProperty, false};
}

std::tuple<bool, ir::BlockStatement *, lexer::SourcePosition, bool> ASParser::ParseFunctionBody(
    // CC-OFFNXT(G.FMT.06-CPP) project code style
    [[maybe_unused]] const ArenaVector<ir::Expression *> &params, [[maybe_unused]] ParserStatus newStatus,
    ParserStatus contextStatus)
{
    bool isDeclare = InAmbientContext();
    bool isOverload = false;
    bool letDeclare = true;
    ir::BlockStatement *body = nullptr;
    lexer::SourcePosition endLoc = Lexer()->GetToken().End();

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        if (!isDeclare && ((contextStatus & ParserStatus::IN_METHOD_DEFINITION) == 0)) {
            ThrowSyntaxError("Unexpected token, expected '{'");
        } else {
            letDeclare = false;
        }

        isOverload = true;
    } else if (isDeclare) {
        ThrowSyntaxError("An implementation cannot be declared in ambient contexts.");
    } else {
        body = ParseBlockStatement();
        ES2PANDA_ASSERT(body != nullptr);
        endLoc = body->End();
    }

    return {letDeclare, body, endLoc, isOverload};
}

ir::AstNode *ASParser::ParseImportDefaultSpecifier(ArenaVector<ir::AstNode *> *specifiers)
{
    ir::Identifier *local = ParseNamedImport(&Lexer()->GetToken());
    Lexer()->NextToken();  // eat local name

    auto *specifier = AllocNode<ir::ImportDefaultSpecifier>(local);
    ES2PANDA_ASSERT(specifier != nullptr);
    specifier->SetRange(specifier->Local()->Range());
    specifiers->push_back(specifier);

    return nullptr;
}

ir::Expression *ASParser::ParseArrowFunctionRestParameter(lexer::SourcePosition start)
{
    ir::SpreadElement *restElement = ParseSpreadElement(ExpressionParseFlags::MUST_BE_PATTERN);

    restElement->SetGrouped();
    restElement->SetStart(start);

    ValidateArrowFunctionRestParameter(restElement);

    Lexer()->NextToken();

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
        ThrowSyntaxError(":' expected");
    }

    Lexer()->NextToken();  // eat ':'

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TypeNode *returnTypeAnnotation = ParseTypeAnnotation(&options);

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_ARROW) {
        ThrowSyntaxError("'=>' expected");
    }

    return ParseArrowFunctionExpression(restElement, nullptr, returnTypeAnnotation, false);
}

ir::Expression *ASParser::ParseArrowFunctionNoParameter(lexer::SourcePosition start)
{
    Lexer()->NextToken();

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
        ThrowSyntaxError(":' expected");
    }

    Lexer()->NextToken();  // eat ':'

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TypeNode *returnTypeAnnotation = ParseTypeAnnotation(&options);

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_ARROW) {
        ThrowSyntaxError("'=>' expected");
    }

    auto *arrowExpr = ParseArrowFunctionExpression(nullptr, nullptr, returnTypeAnnotation, false);
    arrowExpr->SetStart(start);
    arrowExpr->AsArrowFunctionExpression()->Function()->SetStart(start);

    return arrowExpr;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ASParser::ParseCoverParenthesizedExpressionAndArrowParameterList(
    [[maybe_unused]] ExpressionParseFlags flags)  // CC-OFF(G.FMT.06-CPP) project code style
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    lexer::SourcePosition start = Lexer()->GetToken().Start();
    Lexer()->NextToken();
    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD) {
        return ParseArrowFunctionRestParameter(start);
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        return ParseArrowFunctionNoParameter(start);
    }

    ir::Expression *expr = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA | ExpressionParseFlags::ACCEPT_REST |
                                           ExpressionParseFlags::POTENTIALLY_IN_PATTERN);

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        ThrowSyntaxError("')' expected");
    }

    expr->SetGrouped();
    expr->SetRange({start, Lexer()->GetToken().End()});
    Lexer()->NextToken();

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_ARROW) {
        ThrowSyntaxError("':' expected.");
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        auto savedPos = Lexer()->Save();
        Lexer()->NextToken();  // eat ':'
        options = TypeAnnotationParsingOptions::NO_OPTS;
        ir::TypeNode *returnTypeAnnotation = ParseTypeAnnotation(&options);

        if (returnTypeAnnotation == nullptr) {
            Lexer()->Rewind(savedPos);
            return expr;
        }

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_ARROW) {
            Lexer()->Rewind(savedPos);
            return expr;
        }

        return ParseArrowFunctionExpression(expr, nullptr, returnTypeAnnotation, false);
    }

    return expr;
}

ir::Expression *ASParser::ParsePrefixAssertionExpression()
{
    lexer::SourcePosition startPos = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat <
    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TypeNode *type = ParseTypeAnnotation(&options);

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_GREATER_THAN) {
        ThrowSyntaxError("'>' expected");
    }

    Lexer()->NextToken();  // eat >

    ir::Expression *expr = ParseExpression();

    auto *node = AllocNode<ir::PrefixAssertionExpression>(expr, type);
    node->SetRange({startPos, Lexer()->GetToken().End()});
    return node;
}

ir::Statement *ASParser::ParseConstStatement(StatementParsingFlags flags)
{
    lexer::SourcePosition constVarStar = Lexer()->GetToken().Start();
    Lexer()->NextToken();

    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_ENUM) {
        return ParseEnumDeclaration(true);
    }

    if ((flags & StatementParsingFlags::ALLOW_LEXICAL) == 0) {
        ThrowSyntaxError("Lexical declaration is not allowed in single statement context");
    }

    auto *variableDecl = ParseVariableDeclaration(VariableParsingFlags::CONST | VariableParsingFlags::NO_SKIP_VAR_KIND);
    ES2PANDA_ASSERT(variableDecl != nullptr);
    variableDecl->SetStart(constVarStar);
    ConsumeSemicolon(variableDecl);

    return variableDecl;
}

ir::AnnotatedExpression *ASParser::ParseVariableDeclaratorKey(VariableParsingFlags flags)
{
    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        ThrowSyntaxError("Identifier expected");
    }

    ValidateDeclaratorId();

    const util::StringView &identStr = Lexer()->GetToken().Ident();
    auto init = AllocNode<ir::Identifier>(identStr, Allocator());
    ES2PANDA_ASSERT(init != nullptr);
    init->SetRange(Lexer()->GetToken().Loc());
    Lexer()->NextToken();

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        init->SetTsTypeAnnotation(ParseTypeAnnotation(&options));
    } else if (((flags & VariableParsingFlags::IN_FOR) == 0) &&
               Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        ThrowSyntaxError("Type expected");
    }

    return init;
}

ir::Statement *ASParser::ParsePotentialConstEnum(VariableParsingFlags flags)
{
    if ((flags & VariableParsingFlags::CONST) == 0) {
        ThrowSyntaxError("Variable declaration expected.");
    }

    return ParseEnumDeclaration(true);
}

// NOLINTNEXTLINE(google-default-arguments)
ir::ExportDefaultDeclaration *ASParser::ParseExportDefaultDeclaration(const lexer::SourcePosition &startLoc,
                                                                      bool isExportEquals)
{
    Lexer()->NextToken();  // eat `default` keyword or `=`

    ir::AstNode *declNode = nullptr;
    bool eatSemicolon = false;

    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::KEYW_FUNCTION: {
            declNode = ParseFunctionDeclaration(true);
            break;
        }
        case lexer::TokenType::KEYW_CLASS: {
            declNode = ParseClassDeclaration(ir::ClassDefinitionModifiers::ID_REQUIRED);
            break;
        }
        case lexer::TokenType::KEYW_INTERFACE: {
            declNode = ParseInterfaceDeclaration(false);
            break;
        }
        case lexer::TokenType::KEYW_NAMESPACE: {
            Lexer()->NextToken();  // eat 'namespace'
            declNode = ParseModuleOrNamespaceDeclaration(startLoc);
            break;
        }
        case lexer::TokenType::KEYW_ENUM: {
            declNode = ParseEnumDeclaration();
            break;
        }
        case lexer::TokenType::KEYW_ASYNC: {
            if ((Lexer()->GetToken().Flags() & lexer::TokenFlags::HAS_ESCAPE) == 0) {
                Lexer()->NextToken();  // eat `async`
                declNode = ParseFunctionDeclaration(false, ParserStatus::ASYNC_FUNCTION);
                break;
            }

            [[fallthrough]];
        }
        default: {
            declNode = ParseExpression();
            eatSemicolon = true;
            break;
        }
    }

    lexer::SourcePosition endLoc = declNode->End();
    auto *exportDeclaration = AllocNode<ir::ExportDefaultDeclaration>(declNode, isExportEquals);
    ES2PANDA_ASSERT(exportDeclaration != nullptr);
    exportDeclaration->SetRange({startLoc, endLoc});

    if (eatSemicolon) {
        ConsumeSemicolon(exportDeclaration);
    }

    return exportDeclaration;
}

class ASParser::ParseNamedExportDeclarationHelper {
    friend ir::Statement *ASParser::ParseNamedExportDeclaration(const lexer::SourcePosition &startLoc);

private:
    static ir::Statement *GetParsedDeclaration(ASParser *parser, lexer::TokenType type)
    {
        ir::ModifierFlags flags = ir::ModifierFlags::NONE;
        if (parser->Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_ABSTRACT) {
            parser->Lexer()->NextToken();  // eat 'abstract'
            flags = ir::ModifierFlags::ABSTRACT;
        }

        switch (type) {
            case lexer::TokenType::KEYW_VAR: {
                return parser->ParseVariableDeclaration(VariableParsingFlags::VAR);
            }
            case lexer::TokenType::KEYW_CONST: {
                return parser->ParseVariableDeclaration(VariableParsingFlags::CONST);
            }
            case lexer::TokenType::KEYW_LET: {
                return parser->ParseVariableDeclaration(VariableParsingFlags::LET);
            }
            case lexer::TokenType::KEYW_FUNCTION: {
                return parser->ParseFunctionDeclaration(false, ParserStatus::NO_OPTS);
            }
            case lexer::TokenType::KEYW_CLASS: {
                return parser->ParseClassDeclaration(ir::ClassDefinitionModifiers::ID_REQUIRED, flags);
            }
            case lexer::TokenType::KEYW_ENUM: {
                return parser->ParseEnumDeclaration();
            }
            case lexer::TokenType::KEYW_INTERFACE: {
                return parser->ParseInterfaceDeclaration(false);
            }
            case lexer::TokenType::KEYW_TYPE: {
                return parser->ParseTypeAliasDeclaration();
            }
            case lexer::TokenType::KEYW_GLOBAL:
            case lexer::TokenType::KEYW_MODULE:
            case lexer::TokenType::KEYW_NAMESPACE: {
                return parser->ParseModuleDeclaration();
            }
            default: {
                parser->ExpectToken(lexer::TokenType::KEYW_ASYNC);
                return parser->ParseFunctionDeclaration(false, ParserStatus::ASYNC_FUNCTION);
            }
        }
    }
};

ir::Statement *ASParser::ParseNamedExportDeclaration(const lexer::SourcePosition &startLoc)
{
    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_DECLARE) {
        CheckDeclare();
    }

    ir::Statement *decl =
        ParseNamedExportDeclarationHelper::GetParsedDeclaration(this, Lexer()->GetToken().KeywordType());

    if (decl->IsVariableDeclaration()) {
        ConsumeSemicolon(decl);
    }

    ArenaVector<ir::ExportSpecifier *> specifiers(Allocator()->Adapter());
    auto *exportDeclaration = AllocNode<ir::ExportNamedDeclaration>(Allocator(), decl, std::move(specifiers));
    ES2PANDA_ASSERT(exportDeclaration != nullptr);
    exportDeclaration->SetRange({startLoc, decl->End()});

    return exportDeclaration;
}

ir::AstNode *ASParser::ParseImportSpecifiers(ArenaVector<ir::AstNode *> *specifiers)
{
    ES2PANDA_ASSERT(specifiers->empty());

    if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
        ParseImportDefaultSpecifier(specifiers);
        return nullptr;
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MULTIPLY) {
        ParseNameSpaceImport(specifiers);
    } else if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        ParseNamedImportSpecifiers(specifiers);
    }

    return nullptr;
}

ir::Statement *ASParser::ParseImportDeclaration([[maybe_unused]] StatementParsingFlags flags)
{
    char32_t nextChar = Lexer()->Lookahead();
    if (nextChar == lexer::LEX_CHAR_LEFT_PAREN || nextChar == lexer::LEX_CHAR_DOT) {
        return ParseExpressionStatement();
    }

    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat import

    ArenaVector<ir::AstNode *> specifiers(Allocator()->Adapter());

    ir::StringLiteral *source = nullptr;

    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_STRING) {
        ParseImportSpecifiers(&specifiers);
        source = ParseFromClause(true);
    } else {
        source = ParseFromClause(false);
    }

    ES2PANDA_ASSERT(source != nullptr);
    lexer::SourcePosition endLoc = source->End();
    auto *importDeclaration = AllocNode<ir::ImportDeclaration>(source, std::move(specifiers));
    ES2PANDA_ASSERT(importDeclaration != nullptr);
    importDeclaration->SetRange({startLoc, endLoc});

    ConsumeSemicolon(importDeclaration);

    return importDeclaration;
}

void ASParser::ReportIllegalBreakError(const lexer::SourcePosition &pos)
{
    ThrowSyntaxError("A 'break' statement can only be used within an enclosing iteration or switch statement", pos);
}

void ASParser::ReportIllegalContinueError()
{
    ThrowSyntaxError("A 'continue' statement can only be used within an enclosing iteration statement");
}

}  // namespace ark::es2panda::parser
