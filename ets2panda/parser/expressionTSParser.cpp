/**
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

#include "TSparser.h"

#include "parserFlags.h"
#include "util/helpers.h"
#include "varbinder/privateBinding.h"
#include "varbinder/scope.h"
#include "varbinder/tsBinding.h"
#include "lexer/TSLexer.h"
#include "ir/base/spreadElement.h"
#include "ir/base/decorator.h"
#include "ir/base/classElement.h"
#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/module/importDefaultSpecifier.h"
#include "ir/module/exportDefaultDeclaration.h"
#include "ir/module/exportAllDeclaration.h"
#include "ir/module/exportNamedDeclaration.h"
#include "ir/module/importDeclaration.h"
#include "ir/expressions/memberExpression.h"
#include "ir/expressions/sequenceExpression.h"
#include "ir/expressions/templateLiteral.h"
#include "ir/expressions/taggedTemplateExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/expressions/yieldExpression.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/objectExpression.h"
#include "ir/expressions/arrayExpression.h"
#include "ir/expressions/literals/bigIntLiteral.h"
#include "ir/expressions/literals/booleanLiteral.h"
#include "ir/expressions/literals/nullLiteral.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/expressions/literals/stringLiteral.h"
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
#include "ir/ts/tsLiteralType.h"
#include "ir/ts/tsMappedType.h"
#include "ir/ts/tsImportType.h"
#include "ir/ts/tsThisType.h"
#include "ir/ts/tsConditionalType.h"
#include "ir/ts/tsTypeOperator.h"
#include "ir/ts/tsInferType.h"
#include "ir/ts/tsTupleType.h"
#include "ir/ts/tsNamedTupleMember.h"
#include "ir/ts/tsQualifiedName.h"
#include "ir/ts/tsIndexedAccessType.h"
#include "ir/ts/tsTypeQuery.h"
#include "ir/ts/tsTypeReference.h"
#include "ir/ts/tsTypePredicate.h"
#include "ir/ts/tsTypeLiteral.h"
#include "ir/ts/tsArrayType.h"
#include "ir/ts/tsUnionType.h"
#include "ir/ts/tsIntersectionType.h"
#include "ir/ts/tsAnyKeyword.h"
#include "ir/ts/tsUndefinedKeyword.h"
#include "ir/ts/tsVoidKeyword.h"
#include "ir/ts/tsNumberKeyword.h"
#include "ir/ts/tsStringKeyword.h"
#include "ir/ts/tsBooleanKeyword.h"
#include "ir/ts/tsBigintKeyword.h"
#include "ir/ts/tsUnknownKeyword.h"
#include "ir/ts/tsNullKeyword.h"
#include "ir/ts/tsNeverKeyword.h"
#include "ir/ts/tsObjectKeyword.h"
#include "ir/ts/tsFunctionType.h"
#include "ir/ts/tsConstructorType.h"
#include "ir/ts/tsParenthesizedType.h"
#include "ir/ts/tsTypeAssertion.h"
#include "ir/ts/tsAsExpression.h"
#include "ir/ts/tsNonNullExpression.h"
#include "ir/ts/tsEnumDeclaration.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "ir/ts/tsModuleDeclaration.h"
#include "ir/ts/tsTypeParameterInstantiation.h"
#include "ir/ts/tsInterfaceHeritage.h"
#include "ir/base/tsSignatureDeclaration.h"
#include "ir/base/tsIndexSignature.h"
#include "ir/base/tsMethodSignature.h"
#include "ir/base/tsPropertySignature.h"
#include "ir/ts/tsParameterProperty.h"
#include "ir/ts/tsClassImplements.h"
#include "ir/ts/tsImportEqualsDeclaration.h"
#include "ir/ts/tsExternalModuleReference.h"

namespace ark::es2panda::parser {
ir::Expression *TSParser::ParsePotentialAsExpression(ir::Expression *expr)
{
    if (Lexer()->GetToken().KeywordType() != lexer::TokenType::KEYW_AS) {
        return nullptr;
    }

    Lexer()->NextToken();  // eat 'as'
    TypeAnnotationParsingOptions options =
        TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::ALLOW_CONST;
    ir::TypeNode *typeAnnotation = ParseTypeAnnotation(&options);

    bool isConst = false;
    if (typeAnnotation->IsTSTypeReference() && typeAnnotation->AsTSTypeReference()->TypeName()->IsIdentifier()) {
        const util::StringView &refName = typeAnnotation->AsTSTypeReference()->TypeName()->AsIdentifier()->Name();
        if (refName.Is("const")) {
            isConst = true;
        }
    }

    lexer::SourcePosition startLoc = expr->Start();
    auto *asExpr = AllocNode<ir::TSAsExpression>(expr, typeAnnotation, isConst);
    ES2PANDA_ASSERT(asExpr != nullptr);
    asExpr->SetRange({startLoc, Lexer()->GetToken().End()});

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_AS) {
        return ParsePotentialAsExpression(asExpr);
    }

    return asExpr;
}

ir::AnnotatedExpression *TSParser::ParsePatternElementGetReturnNode(ExpressionParseFlags &flags, bool &isOptional)
{
    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET: {
            ir::AnnotatedExpression *returnNode = ParseArrayExpression(ExpressionParseFlags::MUST_BE_PATTERN);
            isOptional = returnNode->AsArrayPattern()->IsOptional();
            return returnNode;
        }
        case lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD: {
            if ((flags & ExpressionParseFlags::IN_REST) != 0) {
                ThrowSyntaxError("Unexpected token");
            }

            return ParseSpreadElement(ExpressionParseFlags::MUST_BE_PATTERN);
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_BRACE: {
            ir::AnnotatedExpression *returnNode =
                ParseObjectExpression(ExpressionParseFlags::MUST_BE_PATTERN | ExpressionParseFlags::OBJECT_PATTERN);
            isOptional = returnNode->AsObjectPattern()->IsOptional();
            return returnNode;
        }
        case lexer::TokenType::LITERAL_IDENT: {
            ir::AnnotatedExpression *returnNode = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
            ES2PANDA_ASSERT(returnNode != nullptr);

            returnNode->SetRange(Lexer()->GetToken().Loc());

            Lexer()->NextToken();

            if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_QUESTION_MARK) {
                isOptional = true;

                if ((flags & ExpressionParseFlags::IN_REST) != 0) {
                    ThrowSyntaxError("A rest parameter cannot be optional");
                }

                returnNode->AsIdentifier()->SetOptional(true);
                Lexer()->NextToken();
            }

            if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
                Lexer()->NextToken();  // eat ':'
                TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
                returnNode->SetTsTypeAnnotation(ParseTypeAnnotation(&options));
            }
            return returnNode;
        }
        default: {
            ThrowSyntaxError("Unexpected token, expected an identifier.");
        }
    }
}
// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *TSParser::ParsePatternElement(ExpressionParseFlags flags, bool allowDefault)
{
    bool isOptional = false;
    ir::AnnotatedExpression *returnNode = ParsePatternElementGetReturnNode(flags, isOptional);

    if ((returnNode->IsObjectPattern() || returnNode->IsArrayPattern()) && !InAmbientContext() &&
        ((GetContext().Status() & ParserStatus::FUNCTION) != 0) && isOptional) {
        ThrowSyntaxError("A binding pattern parameter cannot be optional in an implementation signature.");
    }

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

ir::TSParameterProperty *TSParser::CreateParameterProperty(ir::Expression *parameter, ir::ModifierFlags modifiers)
{
    auto accessibilityOption = ir::AccessibilityOption::NO_OPTS;
    bool readonly = false;
    bool isStatic = false;
    bool isExport = false;

    if ((modifiers & ir::ModifierFlags::PRIVATE) != 0) {
        accessibilityOption = ir::AccessibilityOption::PRIVATE;
    } else if ((modifiers & ir::ModifierFlags::PUBLIC) != 0) {
        accessibilityOption = ir::AccessibilityOption::PUBLIC;
    } else if ((modifiers & ir::ModifierFlags::PROTECTED) != 0) {
        accessibilityOption = ir::AccessibilityOption::PROTECTED;
    }

    if ((modifiers & ir::ModifierFlags::READONLY) != 0) {
        readonly = true;
    }

    if ((modifiers & ir::ModifierFlags::STATIC) != 0) {
        isStatic = true;
    }

    // NOTE(Csaba Repasi): Handle export property of TSParameterProperty

    return AllocNode<ir::TSParameterProperty>(accessibilityOption, parameter, readonly, isStatic, isExport);
}

ir::Expression *TSParser::ParseFunctionParameter()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_THIS) {
        Lexer()->GetToken().SetTokenType(lexer::TokenType::LITERAL_IDENT);
        Lexer()->GetToken().SetTokenStr(ERROR_LITERAL);
    }

    lexer::SourcePosition parameterStart = Lexer()->GetToken().Start();
    ir::ModifierFlags modifiers = ParseModifiers();
    // NOTE(Csaba Repasi): throw error if using strick mode reserved keyword here
    if (((GetContext().Status() & ParserStatus::CONSTRUCTOR_FUNCTION) == 0) && modifiers != ir::ModifierFlags::NONE) {
        ThrowSyntaxError("A parameter property is only allowed in a constructor implementation.", parameterStart);
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
        CheckRestrictedBinding();
    }

    ir::Expression *functionParameter = ParsePatternElement(ExpressionParseFlags::NO_OPTS, true);

    if (modifiers != ir::ModifierFlags::NONE && functionParameter->IsRestElement()) {
        ThrowSyntaxError("A parameter property cannot be declared using a rest parameter.", parameterStart);
    }

    if (modifiers != ir::ModifierFlags::NONE &&
        (functionParameter->IsArrayPattern() || functionParameter->IsObjectPattern() ||
         (functionParameter->IsAssignmentPattern() &&
          (functionParameter->AsAssignmentPattern()->Left()->IsArrayPattern() ||
           functionParameter->AsAssignmentPattern()->Left()->IsObjectPattern())))) {
        ThrowSyntaxError("A parameter property may not be declared using a binding pattern.", parameterStart);
    }

    if (modifiers != ir::ModifierFlags::NONE) {
        functionParameter = CreateParameterProperty(functionParameter, modifiers);
        functionParameter->SetRange({parameterStart, functionParameter->AsTSParameterProperty()->Parameter()->End()});
    }

    return functionParameter;
}

ir::Expression *TSParser::ParseModuleReference()
{
    ir::Expression *result = nullptr;

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_REQUIRE &&
        Lexer()->Lookahead() == lexer::LEX_CHAR_LEFT_PAREN) {
        lexer::SourcePosition start = Lexer()->GetToken().Start();
        Lexer()->NextToken();  // eat 'require'
        Lexer()->NextToken();  // eat '('

        if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_STRING) {
            ThrowSyntaxError("String literal expected.");
        }

        result = AllocNode<ir::StringLiteral>(Lexer()->GetToken().String());
        ES2PANDA_ASSERT(result != nullptr);
        result->SetRange(Lexer()->GetToken().Loc());
        Lexer()->NextToken();

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
            ThrowSyntaxError("')' expected.");
        }

        result = AllocNode<ir::TSExternalModuleReference>(result);
        result->SetRange({start, Lexer()->GetToken().End()});
        Lexer()->NextToken();  // eat ')'
    } else {
        result = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
        ES2PANDA_ASSERT(result != nullptr);
        result->SetRange(Lexer()->GetToken().Loc());
        Lexer()->NextToken();

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD) {
            result = ParseQualifiedReference(result);
        }
    }

    return result;
}

ir::TSTypeReference *TSParser::ParseConstExpression()
{
    auto *identRef = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(identRef != nullptr);
    identRef->SetRange(Lexer()->GetToken().Loc());

    auto *typeReference = AllocNode<ir::TSTypeReference>(identRef, nullptr, Allocator());
    ES2PANDA_ASSERT(typeReference);
    typeReference->SetRange(Lexer()->GetToken().Loc());

    Lexer()->NextToken();

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COMMA &&
        Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SEMI_COLON &&
        Lexer()->GetToken().Type() != lexer::TokenType::EOS &&
        Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET &&
        ((Lexer()->GetToken().Flags() & lexer::TokenFlags::NEW_LINE) == 0)) {
        ThrowSyntaxError("Unexpected token.");
    }

    return typeReference;
}

bool TSParser::ParsePotentialNonNullExpression(ir::Expression **returnExpression, lexer::SourcePosition startLoc)
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

// NOLINTNEXTLINE(google-default-arguments)
ir::ObjectExpression *TSParser::ParseObjectExpression(ExpressionParseFlags flags)
{
    ir::ObjectExpression *objExpression = ParserImpl::ParseObjectExpression(flags);
    ParsePotentialOptionalFunctionParameter(objExpression);
    return objExpression;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::ArrayExpression *TSParser::ParseArrayExpression(ExpressionParseFlags flags)
{
    ir::ArrayExpression *arrayExpression = ParserImpl::ParseArrayExpression(flags);
    ParsePotentialOptionalFunctionParameter(arrayExpression);
    return arrayExpression;
}
class TSParser::ParsePotentialArrowExpressionHelper {
    friend ir::ArrowFunctionExpression *TSParser::ParsePotentialArrowExpression(ir::Expression **returnExpression,
                                                                                const lexer::SourcePosition &startLoc);

private:
    static ir::ArrowFunctionExpression *CreateCallExpression(TSParser *parser, lexer::Lexer *lexer,
                                                             ir::Expression **returnExpression,
                                                             ir::TSTypeParameterDeclaration *typeParamDecl,
                                                             const lexer::SourcePosition &startLoc)
    {
        ir::CallExpression *callExpression = parser->ParseCallExpression(*returnExpression, false);

        ir::TypeNode *returnTypeAnnotation = nullptr;
        if (lexer->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
            lexer->NextToken();  // eat ':'
            TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
            returnTypeAnnotation = parser->ParseTypeAnnotation(&options);
        }

        if (lexer->GetToken().Type() == lexer::TokenType::PUNCTUATOR_ARROW) {
            ir::ArrowFunctionExpression *arrowFuncExpr =
                parser->ParseArrowFunctionExpression(callExpression, typeParamDecl, returnTypeAnnotation, true);
            arrowFuncExpr->SetStart(startLoc);

            return arrowFuncExpr;
        }

        if (returnTypeAnnotation != nullptr || typeParamDecl != nullptr) {
            parser->ThrowSyntaxError("'=>' expected");
        }

        *returnExpression = callExpression;
        return nullptr;
    }
};

// NOLINTNEXTLINE(readability-function-size)
// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
ir::ArrowFunctionExpression *TSParser::ParsePotentialArrowExpression(ir::Expression **returnExpression,
                                                                     const lexer::SourcePosition &startLoc)
{
    ir::TSTypeParameterDeclaration *typeParamDecl = nullptr;

    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::KEYW_FUNCTION: {
            *returnExpression = ParseFunctionExpression(ParserStatus::ASYNC_FUNCTION);
            ES2PANDA_ASSERT(*returnExpression != nullptr);
            (*returnExpression)->SetStart(startLoc);
            break;
        }
        case lexer::TokenType::LITERAL_IDENT: {
            ir::Expression *identRef = ParsePrimaryExpression();
            if (identRef == nullptr) {
                return nullptr;
            }
            ES2PANDA_ASSERT(identRef->IsIdentifier());

            if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_ARROW) {
                ThrowSyntaxError("Unexpected token, expected '=>'");
            }

            ir::ArrowFunctionExpression *arrowFuncExpr = ParseArrowFunctionExpression(identRef, nullptr, nullptr, true);
            arrowFuncExpr->SetStart(startLoc);

            return arrowFuncExpr;
        }
        case lexer::TokenType::PUNCTUATOR_ARROW: {
            ir::ArrowFunctionExpression *arrowFuncExpr =
                ParseArrowFunctionExpression(*returnExpression, nullptr, nullptr, true);
            arrowFuncExpr->SetStart(startLoc);
            return arrowFuncExpr;
        }
        case lexer::TokenType::PUNCTUATOR_LESS_THAN: {
            const auto savedPos = Lexer()->Save();

            auto options = TypeAnnotationParsingOptions::NO_OPTS;
            typeParamDecl = ParseTypeParameterDeclaration(&options);
            if (typeParamDecl == nullptr) {
                Lexer()->Rewind(savedPos);
                return nullptr;
            }

            if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
                ThrowSyntaxError("'(' expected");
            }

            [[fallthrough]];
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS: {
            return ParsePotentialArrowExpressionHelper::CreateCallExpression(this, Lexer(), returnExpression,
                                                                             typeParamDecl, startLoc);
        }
        default: {
            break;
        }
    }

    return nullptr;
}

ir::AnnotatedExpression *TSParser::ParseVariableDeclaratorKey(VariableParsingFlags flags)
{
    ir::AnnotatedExpression *init = ParserImpl::ParseVariableDeclaratorKey(flags);

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        init->SetTsTypeAnnotation(ParseTypeAnnotation(&options));
    }

    return init;
}

void TSParser::ParseOptionalFunctionParameter(ir::AnnotatedExpression *returnNode, bool isRest)
{
    bool isOptional = false;

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_QUESTION_MARK) {
        if (isRest) {
            ThrowSyntaxError("A rest parameter cannot be optional");
        }

        switch (returnNode->Type()) {
            case ir::AstNodeType::IDENTIFIER: {
                returnNode->AsIdentifier()->SetOptional(true);
                break;
            }
            case ir::AstNodeType::OBJECT_PATTERN:
            case ir::AstNodeType::ARRAY_PATTERN: {
                if (!InAmbientContext() && ((GetContext().Status() & ParserStatus::FUNCTION) != 0)) {
                    ThrowSyntaxError("A binding pattern parameter cannot be optional in an implementation signature.");
                }

                if (returnNode->IsObjectPattern()) {
                    returnNode->AsObjectPattern()->SetOptional(true);
                    break;
                }

                returnNode->AsArrayPattern()->SetOptional(true);
                break;
            }
            case ir::AstNodeType::REST_ELEMENT: {
                returnNode->AsRestElement()->SetOptional(true);
                break;
            }
            default: {
                ES2PANDA_UNREACHABLE();
            }
        }

        isOptional = true;
        Lexer()->NextToken();  // eat '?'
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        returnNode->SetTsTypeAnnotation(ParseTypeAnnotation(&options));
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        return;
    }

    if (isRest) {
        ThrowSyntaxError("A rest parameter cannot have an initializer");
    }

    if (returnNode->IsIdentifier() && isOptional) {
        ThrowSyntaxError("Parameter cannot have question mark and initializer");
    }
}

bool TSParser::ValidateArrowFunctionRestParameter(ir::SpreadElement *restElement)
{
    ParseOptionalFunctionParameter(restElement, true);

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        ThrowSyntaxError("')' expected");
    }

    return true;
}

ir::Expression *TSParser::ParseArrowFunctionRestParameter(lexer::SourcePosition start)
{
    ir::SpreadElement *restElement = ParseSpreadElement(ExpressionParseFlags::MUST_BE_PATTERN);

    restElement->SetGrouped();
    restElement->SetStart(start);

    ValidateArrowFunctionRestParameter(restElement);

    Lexer()->NextToken();

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TypeNode *returnTypeAnnotation = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        returnTypeAnnotation = ParseTypeAnnotation(&options);
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_ARROW) {
        ThrowSyntaxError("Unexpected token");
    }

    return ParseArrowFunctionExpression(restElement, nullptr, returnTypeAnnotation, false);
}

ir::Expression *TSParser::ParseArrowFunctionNoParameter(lexer::SourcePosition start)
{
    Lexer()->NextToken();

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TypeNode *returnTypeAnnotation = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        returnTypeAnnotation = ParseTypeAnnotation(&options);
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_ARROW) {
        ThrowSyntaxError("Unexpected token");
    }

    auto *arrowExpr = ParseArrowFunctionExpression(nullptr, nullptr, returnTypeAnnotation, false);
    arrowExpr->SetStart(start);
    arrowExpr->AsArrowFunctionExpression()->Function()->SetStart(start);

    return arrowExpr;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *TSParser::ParseCoverParenthesizedExpressionAndArrowParameterList(
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
        ThrowSyntaxError("Unexpected token, expected ')'");
    }

    expr->SetGrouped();
    expr->SetRange({start, Lexer()->GetToken().End()});
    Lexer()->NextToken();

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

}  // namespace ark::es2panda::parser
