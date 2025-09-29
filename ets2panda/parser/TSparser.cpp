/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "parser/parserStatusContext.h"
#include "util/es2pandaMacros.h"
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
std::unique_ptr<lexer::Lexer> TSParser::InitLexer()
{
    ES2PANDA_ASSERT(GetProgram() == GetContext().GetProgram());
    auto lexer = std::make_unique<lexer::TSLexer>(&GetContext(), DiagnosticEngine());
    SetLexer(lexer.get());
    return lexer;
}

ir::Decorator *TSParser::ParseDecorator()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_AT);

    lexer::SourcePosition start = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat '@'

    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        ThrowSyntaxError("Identifier expected");
    }

    ir::Expression *expr = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    expr->SetRange(Lexer()->GetToken().Loc());
    Lexer()->NextToken();

    while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD) {
        Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);

        if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
            ThrowSyntaxError("Identifier expected");
        }

        auto *identNode = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
        ES2PANDA_ASSERT(identNode != nullptr);
        identNode->SetRange(Lexer()->GetToken().Loc());

        expr =
            AllocNode<ir::MemberExpression>(expr, identNode, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
        Lexer()->NextToken();
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
        expr = ParseCallExpression(expr);
    }

    auto *result = AllocNode<ir::Decorator>(expr);
    ES2PANDA_ASSERT(result != nullptr);
    result->SetRange({start, expr->End()});

    return result;
}

void TSParser::AddDecorators(ir::AstNode *node, ArenaVector<ir::Decorator *> &decorators)
{
    if (decorators.empty()) {
        return;
    }

    if (!node->CanHaveDecorator(true)) {
        ThrowSyntaxError("Decorators are not valid here", decorators.front()->Start());
    }

    node->AddDecorators(std::move(decorators));
}

ir::TSTypeAliasDeclaration *TSParser::ParseTypeAliasDeclaration()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_TYPE);
    lexer::SourcePosition typeStart = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat type keyword

    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        ThrowSyntaxError("Identifier expected");
    }

    if (Lexer()->GetToken().IsReservedTypeName()) {
        std::string errMsg("Type alias name cannot be '");
        errMsg.append(TokenToString(Lexer()->GetToken().KeywordType()));
        errMsg.append("'");
        ThrowSyntaxError(errMsg.c_str());
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

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TypeNode *typeAnnotation = ParseTypeAnnotation(&options);

    auto *typeAliasDecl = AllocNode<ir::TSTypeAliasDeclaration>(Allocator(), id, typeParamDecl, typeAnnotation);
    typeAliasDecl->SetRange({typeStart, Lexer()->GetToken().End()});

    return typeAliasDecl;
}

bool TSParser::CurrentLiteralIsBasicType() const
{
    switch (Lexer()->GetToken().KeywordType()) {
        case lexer::TokenType::KEYW_ANY:
        case lexer::TokenType::KEYW_BOOLEAN:
        case lexer::TokenType::KEYW_NUMBER:
        case lexer::TokenType::KEYW_STRING:
        case lexer::TokenType::KEYW_UNKNOWN:
        case lexer::TokenType::KEYW_UNDEFINED:
        case lexer::TokenType::KEYW_NEVER:
        case lexer::TokenType::KEYW_OBJECT:
        case lexer::TokenType::KEYW_BIGINT: {
            return true;
        }
        default: {
            break;
        }
    }

    return false;
}

bool TSParser::CurrentIsBasicType()
{
    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::LITERAL_NUMBER:
        case lexer::TokenType::LITERAL_STRING:
        case lexer::TokenType::LITERAL_FALSE:
        case lexer::TokenType::LITERAL_TRUE:
        case lexer::TokenType::LITERAL_NULL:
        case lexer::TokenType::KEYW_THIS:
        case lexer::TokenType::KEYW_VOID: {
            return true;
        }
        case lexer::TokenType::LITERAL_IDENT: {
            return CurrentLiteralIsBasicType();
        }
        default: {
            break;
        }
    }

    return false;
}

ir::TypeNode *TSParser::ParseTypeAnnotation(TypeAnnotationParsingOptions *options)
{
    ir::TypeNode *typeAnnotation = nullptr;

    while (true) {
        ir::TypeNode *element = ParseTypeAnnotationElement(typeAnnotation, options);

        *options &= ~TypeAnnotationParsingOptions::CAN_BE_TS_TYPE_PREDICATE;

        if (element == nullptr) {
            break;
        }

        typeAnnotation = element;

        if ((((*options) & TypeAnnotationParsingOptions::BREAK_AT_NEW_LINE) != 0) && Lexer()->GetToken().NewLine()) {
            break;
        }
    }

    return typeAnnotation;
}

ir::TypeNode *TSParser::ParseIdentifierReference()
{
    if (CurrentLiteralIsBasicType() && Lexer()->Lookahead() != lexer::LEX_CHAR_DOT) {
        return ParseBasicType();
    }

    return ParseTypeReferenceOrQuery();
}

bool TSParser::IsStartOfMappedType() const
{
    auto pos = Lexer()->Save();
    Lexer()->NextToken();
    bool result = false;

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MINUS ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PLUS) {
        Lexer()->NextToken();
        result = Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_READONLY;
        Lexer()->Rewind(pos);
        return result;
    }

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_READONLY) {
        Lexer()->NextToken();
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
        Lexer()->Rewind(pos);
        return false;
    }

    Lexer()->NextToken();

    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        Lexer()->Rewind(pos);
        return false;
    }

    Lexer()->NextToken();

    result = Lexer()->GetToken().Type() == lexer::TokenType::KEYW_IN;

    Lexer()->Rewind(pos);
    return result;
}

bool TSParser::IsStartOfTypePredicate() const
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT ||
                    Lexer()->GetToken().Type() == lexer::TokenType::KEYW_THIS);

    auto pos = Lexer()->Save();
    bool isAsserts = Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_ASSERTS;
    if (isAsserts) {
        Lexer()->NextToken();
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT &&
        Lexer()->GetToken().Type() != lexer::TokenType::KEYW_THIS) {
        Lexer()->Rewind(pos);
        return false;
    }

    if (isAsserts) {
        Lexer()->Rewind(pos);
        return true;
    }

    Lexer()->NextToken();

    bool result = Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_IS;
    Lexer()->Rewind(pos);
    return result;
}

bool TSParser::IsStartOfAbstractConstructorType() const
{
    if (Lexer()->GetToken().KeywordType() != lexer::TokenType::KEYW_ABSTRACT) {
        return false;
    }

    lexer::LexerPosition pos = Lexer()->Save();
    Lexer()->NextToken();  // eat 'abstract'
    bool result = Lexer()->GetToken().Type() == lexer::TokenType::KEYW_NEW;

    Lexer()->Rewind(pos);

    return result;
}

ir::TSImportType *TSParser::ParseImportType(const lexer::SourcePosition &startLoc, bool isTypeof)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::KEYW_IMPORT);

    Lexer()->NextToken();

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
        ThrowSyntaxError("'(' expected");
    }

    Lexer()->NextToken();

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TypeNode *param = ParseTypeAnnotation(&options);

    if (!param->IsTSLiteralType() || !param->AsTSLiteralType()->Literal()->IsStringLiteral()) {
        ThrowSyntaxError("String literal expected");
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        ThrowSyntaxError("')' expected");
    }

    Lexer()->NextToken();

    ir::Expression *qualifier = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD) {
        Lexer()->NextToken();
        qualifier = ParseQualifiedName();
    }

    ir::TSTypeParameterInstantiation *typeParams = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT) {
            Lexer()->BackwardToken(lexer::TokenType::PUNCTUATOR_LESS_THAN, 1);
        }

        typeParams = ParseTypeParameterInstantiation(&options);
    }

    auto *importType = AllocNode<ir::TSImportType>(param, typeParams, qualifier, isTypeof, Allocator());

    importType->SetRange({startLoc, Lexer()->GetToken().End()});

    return importType;
}

ir::TypeNode *TSParser::ParseThisType(bool throwError)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::KEYW_THIS);

    if (throwError && ((GetContext().Status() & ParserStatus::ALLOW_THIS_TYPE) == 0)) {
        ThrowSyntaxError(
            "A 'this' type is available only in a non-static member "
            "of a class or interface.");
    }

    auto *returnType = AllocNode<ir::TSThisType>(Allocator());
    ES2PANDA_ASSERT(returnType != nullptr);
    returnType->SetRange(Lexer()->GetToken().Loc());

    Lexer()->NextToken();

    return returnType;
}

ir::TypeNode *TSParser::ParseConditionalType(ir::Expression *checkType, bool restrictExtends)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::KEYW_EXTENDS);
    if (restrictExtends) {
        ThrowSyntaxError("'?' expected.");
    }

    lexer::SourcePosition startLoc = checkType->Start();

    Lexer()->NextToken();  // eat 'extends'

    ParserStatus savedStatus = GetContext().Status();
    GetContext().Status() |= ParserStatus::IN_EXTENDS;

    TypeAnnotationParsingOptions options =
        TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::RESTRICT_EXTENDS;
    auto *extendsType = ParseTypeAnnotation(&options);

    GetContext().Status() = savedStatus;

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_QUESTION_MARK) {
        ThrowSyntaxError("'?' expected.");
    }

    Lexer()->NextToken();  // eat '?'

    options &= ~TypeAnnotationParsingOptions::RESTRICT_EXTENDS;
    auto *trueType = ParseTypeAnnotation(&options);

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
        ThrowSyntaxError("':' expected.");
    }

    Lexer()->NextToken();  // eat ':'

    auto *falseType = ParseTypeAnnotation(&options);

    lexer::SourcePosition endLoc = falseType->End();

    auto *conditionalType = AllocNode<ir::TSConditionalType>(checkType, extendsType, trueType, falseType, Allocator());

    conditionalType->SetRange({startLoc, endLoc});

    return conditionalType;
}

ir::TypeNode *TSParser::ParseTypeOperatorOrTypeReference()
{
    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_READONLY) {
        lexer::SourcePosition typeOperatorStart = Lexer()->GetToken().Start();
        Lexer()->NextToken();

        ir::TypeNode *type = ParseTypeAnnotation(&options);

        if (!type->IsTSArrayType() && !type->IsTSTupleType()) {
            ThrowSyntaxError(
                "'readonly' type modifier is only permitted on array "
                "and tuple literal types.");
        }

        auto *typeOperator = AllocNode<ir::TSTypeOperator>(type, ir::TSOperatorType::READONLY, Allocator());
        ES2PANDA_ASSERT(typeOperator != nullptr);
        typeOperator->SetRange({typeOperatorStart, type->End()});

        return typeOperator;
    }

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_KEYOF) {
        lexer::SourcePosition typeOperatorStart = Lexer()->GetToken().Start();
        Lexer()->NextToken();

        ir::TypeNode *type = ParseTypeAnnotation(&options);

        auto *typeOperator = AllocNode<ir::TSTypeOperator>(type, ir::TSOperatorType::KEYOF, Allocator());
        ES2PANDA_ASSERT(typeOperator != nullptr);
        typeOperator->SetRange({typeOperatorStart, type->End()});

        return typeOperator;
    }

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_INFER) {
        if ((GetContext().Status() & ParserStatus::IN_EXTENDS) == 0) {
            ThrowSyntaxError(
                "'infer' declarations are only permitted in the "
                "'extends' clause of a conditional type.");
        }

        lexer::SourcePosition inferStart = Lexer()->GetToken().Start();
        Lexer()->NextToken();

        ir::TSTypeParameter *typeParam = ParseTypeParameter(&options);

        auto *inferType = AllocNode<ir::TSInferType>(typeParam, Allocator());

        ES2PANDA_ASSERT(inferType != nullptr);
        inferType->SetRange({inferStart, Lexer()->GetToken().End()});

        return inferType;
    }

    return ParseIdentifierReference();
}

ir::TypeNode *TSParser::ParseTupleElement(ir::TSTupleKind *kind, bool *seenOptional)
{
    lexer::SourcePosition elementStart = Lexer()->GetToken().Start();
    ir::TypeNode *element = nullptr;
    bool isOptional = false;
    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;

    if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT && !CurrentLiteralIsBasicType()) {
        auto *elementIdent = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
        elementIdent->SetRange(Lexer()->GetToken().Loc());

        if (Lexer()->Lookahead() == lexer::LEX_CHAR_COLON || Lexer()->Lookahead() == lexer::LEX_CHAR_QUESTION) {
            if (*kind == ir::TSTupleKind::DEFAULT) {
                ThrowSyntaxError("Tuple members must all have names or all not have names");
            }

            Lexer()->NextToken();  // eat ident

            if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_QUESTION_MARK) {
                Lexer()->NextToken();  // eat '?'
                isOptional = true;
                *seenOptional = true;
            } else if (*seenOptional) {
                ThrowSyntaxError("A required element cannot follow an optional element");
            }

            if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
                ThrowSyntaxError("':' expected");
            }

            Lexer()->NextToken();  // eat ':'
            auto *elementType = ParseTypeAnnotation(&options);
            *kind = ir::TSTupleKind::NAMED;

            element = AllocNode<ir::TSNamedTupleMember>(elementIdent, elementType, isOptional, Allocator());
        } else {
            element = ParseTypeReferenceOrQuery();
        }
        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COMMA &&
            Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
            element = ParseTypeAnnotationElement(element, &options);
        }
    } else {
        if (*kind == ir::TSTupleKind::NAMED) {
            ThrowSyntaxError("Tuple members must all have names or all not have names");
        }

        *kind = ir::TSTupleKind::DEFAULT;
        element = ParseTypeAnnotation(&options);
    }

    if (element != nullptr) {
        element->SetRange({elementStart, Lexer()->GetToken().End()});
    }
    return element;
}

ir::TSTupleType *TSParser::ParseTupleType()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET);
    lexer::SourcePosition tupleStart = Lexer()->GetToken().Start();
    ArenaVector<ir::TypeNode *> elements(Allocator()->Adapter());
    ir::TSTupleKind kind = ir::TSTupleKind::NONE;
    bool seenOptional = false;

    Lexer()->NextToken();  // eat '['

    while (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
        ir::TypeNode *element = ParseTupleElement(&kind, &seenOptional);
        elements.push_back(element);

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
            break;
        }

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COMMA) {
            ThrowSyntaxError("',' expected.");
        }

        Lexer()->NextToken();  // eat ','
    }

    lexer::SourcePosition tupleEnd = Lexer()->GetToken().End();
    Lexer()->NextToken();  // eat ']'

    auto *tupleType = AllocNode<ir::TSTupleType>(std::move(elements), Allocator());
    ES2PANDA_ASSERT(tupleType != nullptr);
    tupleType->SetRange({tupleStart, tupleEnd});
    return tupleType;
}

ir::TypeNode *TSParser::ParseIndexAccessType(ir::TypeNode *typeName)
{
    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;

    do {
        Lexer()->NextToken();  // eat '['

        ir::TypeNode *indexType = ParseTypeAnnotation(&options);

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
            ThrowSyntaxError("']' expected");
        }

        Lexer()->NextToken();  // eat ']'

        typeName = AllocNode<ir::TSIndexedAccessType>(typeName, indexType, Allocator());
        typeName->SetRange({typeName->AsTSIndexedAccessType()->ObjectType()->Start(), Lexer()->GetToken().End()});
    } while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET &&
             Lexer()->Lookahead() != lexer::LEX_CHAR_RIGHT_SQUARE);

    return typeName;
}

ir::TypeNode *TSParser::ParseTypeReferenceOrQuery(bool parseQuery)
{
    lexer::SourcePosition referenceStartLoc = Lexer()->GetToken().Start();

    if (parseQuery) {
        ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::KEYW_TYPEOF);
        Lexer()->NextToken();  // eat 'typeof'

        if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_IMPORT) {
            lexer::SourcePosition &startLoc = referenceStartLoc;
            return ParseImportType(startLoc, true);
        }

        if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
            ThrowSyntaxError("Identifier expected.");
        }
    }

    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT ||
                    Lexer()->GetToken().Type() == lexer::TokenType::KEYW_EXTENDS);

    ir::Expression *typeName = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(typeName != nullptr);
    typeName->SetRange(Lexer()->GetToken().Loc());

    if (Lexer()->Lookahead() == lexer::LEX_CHAR_LESS_THAN) {
        Lexer()->ForwardToken(lexer::TokenType::PUNCTUATOR_LESS_THAN, 1);
    } else {
        Lexer()->NextToken();
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD) {
        typeName = ParseQualifiedReference(typeName);
    }

    ir::TSTypeParameterInstantiation *typeParamInst = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        if (parseQuery) {
            ThrowSyntaxError("Unexpected token.");
        }

        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        typeParamInst = ParseTypeParameterInstantiation(&options);
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET &&
        Lexer()->Lookahead() != lexer::LEX_CHAR_RIGHT_SQUARE) {
        ir::TypeNode *typeRef =
            parseQuery ? AllocNode<ir::TSTypeQuery>(typeName, Allocator())->AsTypeNode()
                       : AllocNode<ir::TSTypeReference>(typeName, typeParamInst, Allocator())->AsTypeNode();

        typeRef->SetRange({referenceStartLoc, Lexer()->GetToken().End()});

        return ParseIndexAccessType(typeRef);
    }

    ir::TypeNode *returnNode = parseQuery
                                   ? AllocNode<ir::TSTypeQuery>(typeName, Allocator())->AsTypeNode()
                                   : AllocNode<ir::TSTypeReference>(typeName, typeParamInst, Allocator())->AsTypeNode();

    returnNode->SetRange({referenceStartLoc, typeName->End()});

    return returnNode;
}

ir::TSTypeParameter *TSParser::ParseMappedTypeParameter()
{
    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();

    auto *paramName = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(paramName != nullptr);
    paramName->SetRange({Lexer()->GetToken().Start(), Lexer()->GetToken().End()});

    Lexer()->NextToken();

    Lexer()->NextToken();  // eat 'in'

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TypeNode *constraint = ParseTypeAnnotation(&options);

    lexer::SourcePosition endLoc = constraint->End();

    auto *typeParameter = AllocNode<ir::TSTypeParameter>(paramName, constraint, nullptr, Allocator());
    ES2PANDA_ASSERT(typeParameter != nullptr);
    typeParameter->SetRange({startLoc, endLoc});

    return typeParameter;
}

ir::MappedOption TSParser::ParseMappedOption(lexer::TokenType tokenType)
{
    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_MINUS &&
        Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_PLUS &&
        Lexer()->GetToken().KeywordType() != tokenType && Lexer()->GetToken().Type() != tokenType) {
        return ir::MappedOption::NO_OPTS;
    }

    auto result = Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MINUS ? ir::MappedOption::MINUS
                                                                                   : ir::MappedOption::PLUS;

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MINUS ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PLUS) {
        Lexer()->NextToken();
    }

    if (Lexer()->GetToken().KeywordType() != tokenType && Lexer()->GetToken().Type() != tokenType) {
        ThrowSyntaxError({"'", TokenToString(tokenType), "' expected."});
    }

    Lexer()->NextToken();

    return result;
}

ir::TSMappedType *TSParser::ParseMappedType()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_BRACE);

    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();
    Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);  // eat '{'

    ir::MappedOption readonly = ParseMappedOption(lexer::TokenType::KEYW_READONLY);

    Lexer()->NextToken();  // eat '['

    ir::TSTypeParameter *typeParameter = ParseMappedTypeParameter();

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
        ThrowSyntaxError("']' expected");
    }

    Lexer()->NextToken();  // eat ']'

    ir::MappedOption optional = ParseMappedOption(lexer::TokenType::PUNCTUATOR_QUESTION_MARK);

    ir::TypeNode *typeAnnotation = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        typeAnnotation = ParseTypeAnnotation(&options);
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SEMI_COLON &&
        Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
        ThrowSyntaxError("';' expected");
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
        Lexer()->NextToken();  // eat ';'
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
        ThrowSyntaxError("'}' expected");
    }

    auto *mappedType = AllocNode<ir::TSMappedType>(typeParameter, typeAnnotation, readonly, optional, Allocator());

    mappedType->SetRange({startLoc, Lexer()->GetToken().End()});

    Lexer()->NextToken();  // eat '}'

    return mappedType;
}

ir::TSTypePredicate *TSParser::ParseTypePredicate()
{
    auto pos = Lexer()->Save();
    lexer::SourcePosition startPos = Lexer()->GetToken().Start();
    bool isAsserts = Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_ASSERTS;
    if (isAsserts) {
        Lexer()->NextToken();  // eat 'asserts'
        if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_IS) {
            isAsserts = false;
            Lexer()->Rewind(pos);
        }
    }

    ir::Expression *parameterName = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
        parameterName = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    } else {
        parameterName = AllocNode<ir::TSThisType>(Allocator());
    }

    parameterName->SetRange({Lexer()->GetToken().Start(), Lexer()->GetToken().End()});

    Lexer()->NextToken();

    ir::TypeNode *typeAnnotation = nullptr;
    lexer::SourcePosition endPos;
    ir::TSTypePredicate *result = nullptr;

    if (isAsserts && Lexer()->GetToken().KeywordType() != lexer::TokenType::KEYW_IS) {
        endPos = parameterName->End();
        result = AllocNode<ir::TSTypePredicate>(parameterName, typeAnnotation, isAsserts, Allocator());
        ES2PANDA_ASSERT(result != nullptr);
        result->SetRange({startPos, endPos});
        return result;
    }

    Lexer()->NextToken();  // eat 'is'

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    typeAnnotation = ParseTypeAnnotation(&options);
    endPos = typeAnnotation->End();

    result = AllocNode<ir::TSTypePredicate>(parameterName, typeAnnotation, isAsserts, Allocator());
    ES2PANDA_ASSERT(result != nullptr);
    result->SetRange({startPos, endPos});

    return result;
}

ir::TypeNode *TSParser::ParseTypeLiteralOrMappedType(ir::TypeNode *typeAnnotation)
{
    if (typeAnnotation != nullptr) {
        return nullptr;
    }

    if (IsStartOfMappedType()) {
        return ParseMappedType();
    }

    lexer::SourcePosition bodyStart = Lexer()->GetToken().Start();
    auto members = ParseTypeLiteralOrInterface();
    lexer::SourcePosition bodyEnd = Lexer()->GetToken().End();
    Lexer()->NextToken();

    auto *literalType = AllocNode<ir::TSTypeLiteral>(std::move(members), Allocator());
    ES2PANDA_ASSERT(literalType != nullptr);
    auto *typeVar = varbinder::Scope::CreateVar(Allocator(), "__type", varbinder::VariableFlags::TYPE, literalType);
    literalType->SetVariable(typeVar);
    literalType->SetRange({bodyStart, bodyEnd});
    return literalType;
}

ir::TypeNode *TSParser::ParseTypeReferenceOrTypePredicate(ir::TypeNode *typeAnnotation, bool canBeTsTypePredicate)
{
    if (typeAnnotation != nullptr) {
        return nullptr;
    }

    if (canBeTsTypePredicate && IsStartOfTypePredicate()) {
        return ParseTypePredicate();
    }

    return ParseTypeOperatorOrTypeReference();
}

ir::TypeNode *TSParser::ParseThisTypeOrTypePredicate(ir::TypeNode *typeAnnotation, bool canBeTsTypePredicate,
                                                     bool throwError)
{
    if (typeAnnotation != nullptr) {
        return nullptr;
    }

    if (canBeTsTypePredicate && IsStartOfTypePredicate()) {
        return ParseTypePredicate();
    }

    return ParseThisType(throwError);
}

ir::TSArrayType *TSParser::ParseArrayType(ir::TypeNode *elementType)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET);
    Lexer()->NextToken();  // eat '['

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
        ThrowSyntaxError("']' expected");
    }

    lexer::SourcePosition endLoc = Lexer()->GetToken().End();
    Lexer()->NextToken();  // eat ']'

    lexer::SourcePosition startLoc = elementType->Start();
    auto *arrayType = AllocNode<ir::TSArrayType>(elementType, Allocator());
    ES2PANDA_ASSERT(arrayType != nullptr);
    arrayType->SetRange({startLoc, endLoc});

    return arrayType;
}

ir::TSUnionType *TSParser::ParseUnionType(ir::TypeNode *type, bool restrictExtends)
{
    ArenaVector<ir::TypeNode *> types(Allocator()->Adapter());
    lexer::SourcePosition startLoc;

    TypeAnnotationParsingOptions options =
        TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::IN_UNION;

    if (restrictExtends) {
        options |= TypeAnnotationParsingOptions::RESTRICT_EXTENDS;
    }

    if (type != nullptr) {
        startLoc = type->Start();
        types.push_back(type);
    } else {
        startLoc = Lexer()->GetToken().Start();
    }

    while (true) {
        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_BITWISE_OR) {
            break;
        }

        Lexer()->NextToken();  // eat '|'

        types.push_back(ParseTypeAnnotation(&options));
    }

    lexer::SourcePosition endLoc = types.back()->End();

    auto *unionType = AllocNode<ir::TSUnionType>(std::move(types), Allocator());
    auto *typeVar = varbinder::Scope::CreateVar(Allocator(), "__type", varbinder::VariableFlags::TYPE, unionType);
    ES2PANDA_ASSERT(unionType != nullptr);
    unionType->SetVariable(typeVar);
    unionType->SetRange({startLoc, endLoc});

    return unionType;
}

ir::TSIntersectionType *TSParser::ParseIntersectionType(ir::Expression *type, bool inUnion, bool restrictExtends)
{
    ArenaVector<ir::Expression *> types(Allocator()->Adapter());
    lexer::SourcePosition startLoc;

    TypeAnnotationParsingOptions options =
        TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::IN_INTERSECTION;

    if (restrictExtends) {
        options |= TypeAnnotationParsingOptions::RESTRICT_EXTENDS;
    }

    if (inUnion) {
        options |= TypeAnnotationParsingOptions::IN_UNION;
    }

    if (type != nullptr) {
        startLoc = type->Start();
        types.push_back(type);
    } else {
        startLoc = Lexer()->GetToken().Start();
    }

    while (true) {
        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_BITWISE_AND) {
            break;
        }

        Lexer()->NextToken();  // eat '&'

        types.push_back(ParseTypeAnnotation(&options));
    }

    lexer::SourcePosition endLoc = types.back()->End();

    auto *intersectionType = AllocNode<ir::TSIntersectionType>(std::move(types), Allocator());
    auto *typeVar =
        varbinder::Scope::CreateVar(Allocator(), "__type", varbinder::VariableFlags::TYPE, intersectionType);
    ES2PANDA_ASSERT(intersectionType != nullptr);
    intersectionType->SetVariable(typeVar);
    intersectionType->SetRange({startLoc, endLoc});

    return intersectionType;
}

class TSParser::ParseBasicTypeHelper {
    friend ir::TypeNode *TSParser::ParseBasicType();

private:
    static ir::TypeNode *GetTypeAnnotationFromLiteral(TSParser *parser, lexer::Lexer *lexer)
    {
        switch (lexer->GetToken().KeywordType()) {
            case lexer::TokenType::LITERAL_NUMBER: {
                if ((lexer->GetToken().Flags() & lexer::TokenFlags::NUMBER_BIGINT) != 0) {
                    auto *bigintNode = parser->AllocNode<ir::BigIntLiteral>(lexer->GetToken().BigInt());
                    bigintNode->SetRange(lexer->GetToken().Loc());

                    return parser->AllocNode<ir::TSLiteralType>(bigintNode, parser->Allocator());
                }
                auto *numberNode = parser->AllocNode<ir::NumberLiteral>(lexer->GetToken().GetNumber());
                CHECK_NOT_NULL(numberNode);
                numberNode->SetRange(lexer->GetToken().Loc());

                return parser->AllocNode<ir::TSLiteralType>(numberNode, parser->Allocator());
            }
            case lexer::TokenType::LITERAL_STRING: {
                auto *stringNode = parser->AllocNode<ir::StringLiteral>(lexer->GetToken().String());
                stringNode->SetRange(lexer->GetToken().Loc());

                return parser->AllocNode<ir::TSLiteralType>(stringNode, parser->Allocator());
            }
            case lexer::TokenType::LITERAL_TRUE: {
                auto *booleanLiteral = parser->AllocNode<ir::BooleanLiteral>(true);
                booleanLiteral->SetRange(lexer->GetToken().Loc());

                return parser->AllocNode<ir::TSLiteralType>(booleanLiteral, parser->Allocator());
            }
            case lexer::TokenType::LITERAL_FALSE: {
                auto *booleanLiteral = parser->AllocNode<ir::BooleanLiteral>(false);
                booleanLiteral->SetRange(lexer->GetToken().Loc());

                return parser->AllocNode<ir::TSLiteralType>(booleanLiteral, parser->Allocator());
            }
            case lexer::TokenType::LITERAL_NULL: {
                return parser->AllocNode<ir::TSNullKeyword>(parser->Allocator());
            }
            default: {
                return nullptr;
            }
        }
    }

    static ir::TypeNode *GetTypeAnnotation(TSParser *parser, lexer::Lexer *lexer)
    {
        switch (lexer->GetToken().KeywordType()) {
            case lexer::TokenType::LITERAL_NUMBER:
            case lexer::TokenType::LITERAL_STRING:
            case lexer::TokenType::LITERAL_TRUE:
            case lexer::TokenType::LITERAL_FALSE:
            case lexer::TokenType::LITERAL_NULL: {
                return GetTypeAnnotationFromLiteral(parser, lexer);
            }
            case lexer::TokenType::KEYW_ANY: {
                return parser->AllocNode<ir::TSAnyKeyword>(parser->Allocator());
            }
            case lexer::TokenType::KEYW_BOOLEAN: {
                return parser->AllocNode<ir::TSBooleanKeyword>(parser->Allocator());
            }
            case lexer::TokenType::KEYW_NUMBER: {
                return parser->AllocNode<ir::TSNumberKeyword>(parser->Allocator());
            }
            case lexer::TokenType::KEYW_STRING: {
                return parser->AllocNode<ir::TSStringKeyword>(parser->Allocator());
            }
            case lexer::TokenType::KEYW_UNKNOWN: {
                return parser->AllocNode<ir::TSUnknownKeyword>(parser->Allocator());
            }
            case lexer::TokenType::KEYW_VOID: {
                return parser->AllocNode<ir::TSVoidKeyword>(parser->Allocator());
            }
            case lexer::TokenType::KEYW_UNDEFINED: {
                return parser->AllocNode<ir::TSUndefinedKeyword>(parser->Allocator());
            }
            case lexer::TokenType::KEYW_NEVER: {
                return parser->AllocNode<ir::TSNeverKeyword>(parser->Allocator());
            }
            case lexer::TokenType::KEYW_OBJECT: {
                return parser->AllocNode<ir::TSObjectKeyword>(parser->Allocator());
            }
            case lexer::TokenType::KEYW_BIGINT: {
                return parser->AllocNode<ir::TSBigintKeyword>(parser->Allocator());
            }
            default: {
                parser->ThrowSyntaxError("Unexpected type");
            }
        }
    }
};
ir::TypeNode *TSParser::ParseBasicType()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MINUS) {
        Lexer()->NextToken();

        if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_NUMBER) {
            ThrowSyntaxError("Type expected");
        }
    }

    ir::TypeNode *typeAnnotation = ParseBasicTypeHelper::GetTypeAnnotation(this, Lexer());

    typeAnnotation->SetRange(Lexer()->GetToken().Loc());

    Lexer()->NextToken();
    return typeAnnotation;
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
ir::TypeNode *TSParser::ParseParenthesizedOrFunctionType(ir::TypeNode *typeAnnotation, bool throwError)
{
    if (typeAnnotation != nullptr) {
        return nullptr;
    }

    lexer::SourcePosition typeStart = Lexer()->GetToken().Start();

    bool abstractConstructor = false;

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_ABSTRACT) {
        abstractConstructor = true;
        Lexer()->NextToken();  // eat 'abstract'
    }

    bool isConstructionType = false;

    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_NEW) {
        Lexer()->NextToken();  // eat 'new'
        isConstructionType = true;

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS &&
            Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LESS_THAN) {
            ThrowSyntaxError("'(' expected");
        }
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN || isConstructionType) {
        return ParseFunctionType(typeStart, isConstructionType, throwError, abstractConstructor);
    }

    const auto startPos = Lexer()->Save();
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    Lexer()->NextToken();  // eat '('

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::NO_OPTS;
    ir::TypeNode *type = ParseTypeAnnotation(&options);

    if (type == nullptr) {
        Lexer()->Rewind(startPos);
        return ParseFunctionType(typeStart, false, throwError);
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_QUESTION_MARK ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->Rewind(startPos);
        return ParseFunctionType(typeStart, false, throwError);
    }

    if (throwError && Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        ThrowSyntaxError("')' expected");
    }

    lexer::SourcePosition endLoc = Lexer()->GetToken().End();
    Lexer()->NextToken();  // eat ')'

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_ARROW) {
        Lexer()->Rewind(startPos);

        return ParseFunctionType(typeStart, false, throwError);
    }

    auto *result = AllocNode<ir::TSParenthesizedType>(type, Allocator());
    ES2PANDA_ASSERT(result != nullptr);
    result->SetRange({typeStart, endLoc});

    return result;
}

ir::TypeNode *TSParser::ParseFunctionType(lexer::SourcePosition startLoc, bool isConstructionType, bool throwError,
                                          bool abstractConstructor)
{
    ir::TSTypeParameterDeclaration *typeParamDecl = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        auto options = throwError ? TypeAnnotationParsingOptions::REPORT_ERROR : TypeAnnotationParsingOptions::NO_OPTS;
        typeParamDecl = ParseTypeParameterDeclaration(&options);

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
            if (!throwError) {
                return nullptr;
            }

            ThrowSyntaxError("'(' expected");
        }
    }

    auto params = ParseFunctionParams();

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_ARROW) {
        ThrowSyntaxError("'=>' expected");
    }

    Lexer()->NextToken();  // eat '=>'

    TypeAnnotationParsingOptions options =
        TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::CAN_BE_TS_TYPE_PREDICATE;
    ir::TypeNode *returnTypeAnnotation = ParseTypeAnnotation(&options);

    ir::TypeNode *funcType = nullptr;

    ir::FunctionSignature signature(typeParamDecl, std::move(params), returnTypeAnnotation);

    if (isConstructionType) {
        funcType = AllocNode<ir::TSConstructorType>(std::move(signature), abstractConstructor, Allocator());
    } else {
        funcType = AllocNode<ir::TSFunctionType>(std::move(signature), Allocator());
    }

    funcType->SetRange({startLoc, returnTypeAnnotation->End()});

    return funcType;
}

class TSParser::ParseTypeAnnotationElementHelper {
    friend ir::TypeNode *TSParser::ParseTypeAnnotationElement(ir::TypeNode *typeAnnotation,
                                                              TypeAnnotationParsingOptions *options);

private:
    static ir::TypeNode *ParseKeywordTokens(TSParser *parser, lexer::Lexer *lexer, ir::TypeNode *typeAnnotation,
                                            TypeAnnotationParsingOptions *options)
    {
        switch (lexer->GetToken().Type()) {
            case lexer::TokenType::KEYW_NEW: {
                return parser->ParseParenthesizedOrFunctionType(
                    typeAnnotation, ((*options) & TypeAnnotationParsingOptions::REPORT_ERROR) != 0);
            }
            case lexer::TokenType::KEYW_TYPEOF: {
                if (typeAnnotation != nullptr) {
                    break;
                }

                return parser->ParseTypeReferenceOrQuery(true);
            }
            case lexer::TokenType::KEYW_IMPORT: {
                if (typeAnnotation != nullptr) {
                    break;
                }
                return parser->ParseImportType(lexer->GetToken().Start());
            }
            case lexer::TokenType::KEYW_CONST: {
                if (((*options) & TypeAnnotationParsingOptions::ALLOW_CONST) == 0) {
                    break;
                }

                (*options) &= ~TypeAnnotationParsingOptions::ALLOW_CONST;
                return parser->ParseConstExpression();
            }
            case lexer::TokenType::KEYW_EXTENDS: {
                if (((*options) &
                     (TypeAnnotationParsingOptions::IN_UNION | TypeAnnotationParsingOptions::IN_INTERSECTION)) != 0) {
                    break;
                }

                if (typeAnnotation == nullptr) {
                    return parser->ParseIdentifierReference();
                }

                return parser->ParseConditionalType(typeAnnotation,
                                                    ((*options) & TypeAnnotationParsingOptions::RESTRICT_EXTENDS) != 0);
            }
            case lexer::TokenType::KEYW_THIS: {
                return parser->ParseThisTypeOrTypePredicate(
                    typeAnnotation, ((*options) & TypeAnnotationParsingOptions::CAN_BE_TS_TYPE_PREDICATE) != 0,
                    ((*options) & TypeAnnotationParsingOptions::REPORT_ERROR) != 0);
            }
            default: {
                break;
            }
        }
        return nullptr;
    }

    static ir::TypeNode *ParsePunctuatorTokens(TSParser *parser, lexer::Lexer *lexer, ir::TypeNode *typeAnnotation,
                                               TypeAnnotationParsingOptions *options)
    {
        switch (lexer->GetToken().Type()) {
            case lexer::TokenType::PUNCTUATOR_BITWISE_OR: {
                if (((*options) &
                     (TypeAnnotationParsingOptions::IN_UNION | TypeAnnotationParsingOptions::IN_INTERSECTION)) != 0) {
                    break;
                }

                return parser->ParseUnionType(typeAnnotation,
                                              ((*options) & TypeAnnotationParsingOptions::RESTRICT_EXTENDS) != 0);
            }
            case lexer::TokenType::PUNCTUATOR_BITWISE_AND: {
                if (((*options) & TypeAnnotationParsingOptions::IN_INTERSECTION) != 0) {
                    break;
                }

                return parser->ParseIntersectionType(
                    typeAnnotation, ((*options) & TypeAnnotationParsingOptions::IN_UNION) != 0,
                    ((*options) & TypeAnnotationParsingOptions::RESTRICT_EXTENDS) != 0);
            }
            case lexer::TokenType::PUNCTUATOR_LESS_THAN:
            case lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS: {
                return parser->ParseParenthesizedOrFunctionType(
                    typeAnnotation, ((*options) & TypeAnnotationParsingOptions::REPORT_ERROR) != 0);
            }
            case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET: {
                if (typeAnnotation != nullptr) {
                    if (lexer->Lookahead() == lexer::LEX_CHAR_RIGHT_SQUARE) {
                        return parser->ParseArrayType(typeAnnotation);
                    }

                    return parser->ParseIndexAccessType(typeAnnotation);
                }

                return parser->ParseTupleType();
            }
            case lexer::TokenType::PUNCTUATOR_LEFT_BRACE: {
                return parser->ParseTypeLiteralOrMappedType(typeAnnotation);
            }
            default: {
                break;
            }
        }

        return nullptr;
    }
};

ir::TypeNode *TSParser::ParseTypeAnnotationElement(ir::TypeNode *typeAnnotation, TypeAnnotationParsingOptions *options)
{
    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_MINUS:
        case lexer::TokenType::LITERAL_NUMBER:
        case lexer::TokenType::LITERAL_STRING:
        case lexer::TokenType::LITERAL_FALSE:
        case lexer::TokenType::LITERAL_TRUE:
        case lexer::TokenType::LITERAL_NULL:
        case lexer::TokenType::KEYW_VOID: {
            if (typeAnnotation != nullptr) {
                break;
            }

            return ParseBasicType();
        }
        case lexer::TokenType::LITERAL_IDENT: {
            if (IsStartOfAbstractConstructorType()) {
                return ParseParenthesizedOrFunctionType(typeAnnotation,
                                                        ((*options) & TypeAnnotationParsingOptions::REPORT_ERROR) != 0);
            }

            return ParseTypeReferenceOrTypePredicate(
                typeAnnotation, ((*options) & TypeAnnotationParsingOptions::CAN_BE_TS_TYPE_PREDICATE) != 0);
        }
        default: {
            ir::TypeNode *parsedValue =
                ParseTypeAnnotationElementHelper::ParseKeywordTokens(this, Lexer(), typeAnnotation, options);
            if (parsedValue != nullptr) {
                return parsedValue;
            }

            parsedValue =
                ParseTypeAnnotationElementHelper::ParsePunctuatorTokens(this, Lexer(), typeAnnotation, options);
            if (parsedValue != nullptr) {
                return parsedValue;
            }

            break;
        }
    }

    if (typeAnnotation == nullptr && (((*options) & TypeAnnotationParsingOptions::REPORT_ERROR) != 0)) {
        ThrowSyntaxError("Type expected");
    }

    return nullptr;
}

bool TSParser::ParsePotentialGenericFunctionCall(ir::Expression *primaryExpr, ir::Expression **returnExpression,
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
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        (*returnExpression)->SetRange({startLoc, endLoc});
        return false;
    }

    Lexer()->Rewind(savedPos);
    return true;
}

bool TSParser::IsNamedFunctionExpression()
{
    return Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS &&
           Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LESS_THAN;
}

ir::Identifier *TSParser::ParsePrimaryExpressionIdent([[maybe_unused]] ExpressionParseFlags flags)
{
    auto *identNode = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(identNode != nullptr);
    identNode->SetRange(Lexer()->GetToken().Loc());

    Lexer()->NextToken();

    ParsePotentialOptionalFunctionParameter(identNode);

    return identNode;
}

ir::TSSignatureDeclaration *TSParser::ParseSignatureMember(bool isCallSignature)
{
    lexer::SourcePosition memberStartLoc = Lexer()->GetToken().Start();

    if (!isCallSignature) {
        Lexer()->NextToken();  // eat 'new' keyword
    }

    ir::TSTypeParameterDeclaration *typeParamDecl = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        auto options = TypeAnnotationParsingOptions::REPORT_ERROR;
        typeParamDecl = ParseTypeParameterDeclaration(&options);

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
            ThrowSyntaxError("'(' expected");
        }
    }

    FunctionParameterContext funcParamContext(&GetContext());
    auto params = ParseFunctionParams();

    ir::TypeNode *typeAnnotation = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options =
            TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::CAN_BE_TS_TYPE_PREDICATE;
        typeAnnotation = ParseTypeAnnotation(&options);
    }

    auto kind = isCallSignature ? ir::TSSignatureDeclaration::TSSignatureDeclarationKind::CALL_SIGNATURE
                                : ir::TSSignatureDeclaration::TSSignatureDeclarationKind::CONSTRUCT_SIGNATURE;
    auto *signatureMember = AllocNode<ir::TSSignatureDeclaration>(
        kind, ir::FunctionSignature(typeParamDecl, std::move(params), typeAnnotation));
    ES2PANDA_ASSERT(signatureMember != nullptr);
    signatureMember->SetRange({memberStartLoc, Lexer()->GetToken().End()});

    return signatureMember;
}

bool TSParser::IsPotentiallyIndexSignature()
{
    const auto savedPos = Lexer()->Save();

    Lexer()->NextToken();  // eat '['

    bool isIndexSignature =
        Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT && Lexer()->Lookahead() == lexer::LEX_CHAR_COLON;

    Lexer()->Rewind(savedPos);

    return isIndexSignature;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::TSIndexSignature *TSParser::ParseIndexSignature(const lexer::SourcePosition &startLoc, bool isReadonly)
{
    Lexer()->NextToken();  // eat '['

    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT);
    auto *key = AllocNode<ir::Identifier>(Lexer()->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(key != nullptr);
    key->SetRange(Lexer()->GetToken().Loc());

    Lexer()->NextToken();  // eat key

    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON);

    Lexer()->NextToken();  // eat ':'

    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    ir::TypeNode *keyType = ParseTypeAnnotation(&options);

    if (!keyType->IsTSNumberKeyword() && !keyType->IsTSStringKeyword()) {
        ThrowSyntaxError(
            "An index signature parameter type must be either "
            "'string' or 'number'");
    }

    key->SetTsTypeAnnotation(keyType);

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
        ThrowSyntaxError("']' expected.");
    }

    Lexer()->NextToken();  // eat ']'

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
        ThrowSyntaxError("An index signature must have a type annotation.");
    }

    Lexer()->NextToken();  // eat ':'

    ir::TypeNode *typeAnnotation = ParseTypeAnnotation(&options);

    auto *indexSignature = AllocNode<ir::TSIndexSignature>(key, typeAnnotation, isReadonly);
    indexSignature->SetRange({startLoc, Lexer()->GetToken().End()});
    return indexSignature;
}

std::tuple<ir::Expression *, bool> TSParser::ParseInterfacePropertyKey()
{
    ir::Expression *key = nullptr;
    bool isComputed = false;

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
        case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET: {
            Lexer()->NextToken();  // eat left square bracket

            key = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);
            isComputed = true;

            if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
                ThrowSyntaxError("Unexpected token, expected ']'");
            }
            break;
        }
        default: {
            ThrowSyntaxError("Unexpected token in property key");
        }
    }

    Lexer()->NextToken();
    return {key, isComputed};
}

void TSParser::CreateTSVariableForProperty(ir::AstNode *node, const ir::Expression *key, varbinder::VariableFlags flags)
{
    varbinder::Variable *propVar = nullptr;
    bool isMethod = (flags & varbinder::VariableFlags::METHOD) != 0;
    util::StringView propName = "__computed";

    switch (key->Type()) {
        case ir::AstNodeType::IDENTIFIER: {
            propName = key->AsIdentifier()->Name();
            break;
        }
        case ir::AstNodeType::NUMBER_LITERAL: {
            propName = key->AsNumberLiteral()->Str();
            flags |= varbinder::VariableFlags::NUMERIC_NAME;
            break;
        }
        case ir::AstNodeType::STRING_LITERAL: {
            propName = key->AsStringLiteral()->Str();
            break;
        }
        default: {
            flags |= varbinder::VariableFlags::COMPUTED;
            break;
        }
    }

    propVar = isMethod ? varbinder::Scope::CreateVar<varbinder::MethodDecl>(Allocator(), propName, flags, node)
                       : varbinder::Scope::CreateVar<varbinder::PropertyDecl>(Allocator(), propName, flags, node);

    node->SetVariable(propVar);
}

ir::AstNode *TSParser::ParsePropertyOrMethodSignature(const lexer::SourcePosition &startLoc, bool isReadonly)
{
    auto [key, isComputed] = ParseInterfacePropertyKey();
    varbinder::VariableFlags flags = isReadonly ? varbinder::VariableFlags::READONLY : varbinder::VariableFlags::NONE;

    bool isOptional = false;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_QUESTION_MARK) {
        isOptional = true;
        flags |= varbinder::VariableFlags::OPTIONAL;
        Lexer()->NextToken();  // eat '?'
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        if (isReadonly) {
            ThrowSyntaxError("'readonly' modifier can only appear on a property declaration or index signature.",
                             startLoc);
        }

        ir::TSTypeParameterDeclaration *typeParamDecl = nullptr;
        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
            auto options = TypeAnnotationParsingOptions::REPORT_ERROR;
            typeParamDecl = ParseTypeParameterDeclaration(&options);
        }

        ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS, false);

        FunctionParameterContext funcParamContext(&GetContext());
        auto params = ParseFunctionParams();

        ir::TypeNode *returnType = nullptr;
        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
            Lexer()->NextToken();  // eat ':'
            TypeAnnotationParsingOptions options =
                TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::CAN_BE_TS_TYPE_PREDICATE;
            returnType = ParseTypeAnnotation(&options);
        }

        auto *methodSignature = AllocNode<ir::TSMethodSignature>(
            key, ir::FunctionSignature(typeParamDecl, std::move(params), returnType), isComputed, isOptional);
        CreateTSVariableForProperty(methodSignature, key, flags | varbinder::VariableFlags::METHOD);
        methodSignature->SetRange({startLoc, Lexer()->GetToken().End()});
        return methodSignature;
    }

    ir::TypeNode *typeAnnotation = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options =
            TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::BREAK_AT_NEW_LINE;
        typeAnnotation = ParseTypeAnnotation(&options);
    }

    auto *propertySignature =
        AllocNode<ir::TSPropertySignature>(key, typeAnnotation, isComputed, isOptional, isReadonly);
    CreateTSVariableForProperty(propertySignature, key, flags | varbinder::VariableFlags::PROPERTY);
    propertySignature->SetRange({startLoc, Lexer()->GetToken().End()});
    return propertySignature;
}

ir::AstNode *TSParser::ParseTypeLiteralOrInterfaceMember()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_AT) {
        ThrowSyntaxError("Decorators are not allowed here");
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        return ParseSignatureMember(true);
    }

    char32_t nextCp = Lexer()->Lookahead();
    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_NEW &&
        (nextCp == lexer::LEX_CHAR_LEFT_PAREN || nextCp == lexer::LEX_CHAR_LESS_THAN)) {
        return ParseSignatureMember(false);
    }

    lexer::SourcePosition startLoc = Lexer()->GetToken().Start();
    bool isReadonly = Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_READONLY &&
                      nextCp != lexer::LEX_CHAR_LEFT_PAREN && nextCp != lexer::LEX_CHAR_COLON &&
                      nextCp != lexer::LEX_CHAR_COMMA;
    if (isReadonly) {
        Lexer()->NextToken();  // eat 'readonly"
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET &&
        IsPotentiallyIndexSignature()) {
        return ParseIndexSignature(startLoc, isReadonly);
    }

    return ParsePropertyOrMethodSignature(startLoc, isReadonly);
}

void TSParser::ValidateFunctionParam(const ArenaVector<ir::Expression *> &params, const ir::Expression *parameter,
                                     bool *seenOptional)
{
    if (!parameter->IsIdentifier()) {
        GetContext().Status() |= ParserStatus::HAS_COMPLEX_PARAM;
        if (!parameter->IsRestElement()) {
            return;
        }

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
            ThrowSyntaxError("A rest parameter must be last in parameter list");
        }
        return;
    }

    bool currentIsOptional = parameter->AsIdentifier()->IsOptional();
    if (*seenOptional && !currentIsOptional) {
        ThrowSyntaxError("A required parameter cannot follow an optional parameter");
    }

    *seenOptional |= currentIsOptional;
    const util::StringView &paramName = parameter->AsIdentifier()->Name();

    if (paramName.Is("this")) {
        if (!params.empty()) {
            ThrowSyntaxError("A 'this' parameter must be the first parameter");
        }

        if ((GetContext().Status() & ParserStatus::CONSTRUCTOR_FUNCTION) != 0) {
            ThrowSyntaxError("A constructor cannot have a 'this' parameter");
        }

        if ((GetContext().Status() & ParserStatus::ARROW_FUNCTION) != 0) {
            ThrowSyntaxError("An arrow function cannot have a 'this' parameter");
        }

        if ((GetContext().Status() & ParserStatus::ACCESSOR_FUNCTION) != 0) {
            ThrowSyntaxError("'get' and 'set' accessors cannot declare 'this' parameters");
        }
    }

    if (paramName.Is("constructor") && ((GetContext().Status() & ParserStatus::CONSTRUCTOR_FUNCTION) != 0)) {
        ThrowSyntaxError("'constructor' cannot be used as a parameter property name");
    }
}

ArenaVector<ir::Expression *> TSParser::ParseFunctionParams()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    Lexer()->NextToken();

    ArenaVector<ir::Expression *> params(Allocator()->Adapter());
    bool seenOptional = false;

    while (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        ArenaVector<ir::Decorator *> decorators(Allocator()->Adapter());

        ParseDecorators(decorators);

        if (!decorators.empty() && ((GetContext().Status() & ParserStatus::IN_CLASS_BODY) == 0)) {
            ThrowSyntaxError("Decorators are not valid here", decorators.front()->Start());
        }

        ir::Expression *parameter = ParseFunctionParameter();
        ValidateFunctionParam(params, parameter, &seenOptional);

        if (!decorators.empty()) {
            parameter->AddDecorators(std::move(decorators));
        }

        params.push_back(parameter);

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COMMA &&
            Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
            ThrowSyntaxError(", expected");
        }

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA) {
            Lexer()->NextToken();
        }
    }

    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);
    Lexer()->NextToken();

    return params;
}

ir::TypeNode *TSParser::ParseClassKeyAnnotation()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options =
            TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::BREAK_AT_NEW_LINE;
        return ParseTypeAnnotation(&options);
    }

    return nullptr;
}

void TSParser::ValidateClassMethodStart(ClassElementDescriptor *desc, ir::TypeNode *typeAnnotation)
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS && desc->isPrivateIdent) {
        ThrowSyntaxError("A method cannot be named with a private identifier");
    }

    if (typeAnnotation == nullptr && (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS ||
                                      Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN)) {
        if (((desc->modifiers & (ir::ModifierFlags::DECLARE | ir::ModifierFlags::READONLY)) != 0)) {
            ThrowSyntaxError("Class method can not be declare nor readonly");
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

ir::MethodDefinition *TSParser::ParseClassMethod(ClassElementDescriptor *desc,
                                                 const ArenaVector<ir::AstNode *> &properties, ir::Expression *propName,
                                                 lexer::SourcePosition *propEnd)
{
    if (desc->methodKind == ir::MethodDefinitionKind::SET || desc->methodKind == ir::MethodDefinitionKind::GET) {
        desc->newStatus |= ParserStatus::ACCESSOR_FUNCTION;
    }

    desc->newStatus |= ParserStatus::IN_METHOD_DEFINITION;

    if (InAmbientContext() && (desc->newStatus & ParserStatus::ASYNC_FUNCTION) != 0) {
        ThrowSyntaxError("'async' modifier cannot be used in an ambient context.");
    }

    if (InAmbientContext() && desc->isGenerator) {
        ThrowSyntaxError("Generators are not allowed in an ambient context.");
    }

    if (desc->methodKind != ir::MethodDefinitionKind::SET &&
        ((desc->newStatus & ParserStatus::CONSTRUCTOR_FUNCTION) == 0)) {
        desc->newStatus |= ParserStatus::NEED_RETURN_TYPE;
    }

    ir::ScriptFunction *func = ParseFunction(desc->newStatus);

    ES2PANDA_ASSERT(func != nullptr);
    if (func->IsOverload() && !desc->decorators.empty()) {
        ThrowSyntaxError("A decorator can only decorate a method implementation, not an overload.",
                         desc->decorators.front()->Start());
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
    auto *method = AllocNode<ir::MethodDefinition>(desc->methodKind, propName, funcExpr, desc->modifiers, Allocator(),
                                                   desc->isComputed);
    ES2PANDA_ASSERT(method != nullptr);
    method->SetRange(funcExpr->Range());

    return method;
}

void TSParser::ValidateClassSetter(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                                   ir::Expression *propName, ir::ScriptFunction *func)
{
    ValidateGetterSetter(ir::MethodDefinitionKind::SET, func);

    if ((desc->modifiers & ir::ModifierFlags::STATIC) == 0) {
        ir::ModifierFlags access = GetAccessability(desc->modifiers);
        CheckAccessorPair(properties, propName, ir::MethodDefinitionKind::GET, access);
    }
}

void TSParser::ValidateClassGetter(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                                   ir::Expression *propName, ir::ScriptFunction *func)
{
    ValidateGetterSetter(ir::MethodDefinitionKind::GET, func);

    if ((desc->modifiers & ir::ModifierFlags::STATIC) == 0) {
        ir::ModifierFlags access = GetAccessability(desc->modifiers);

        CheckAccessorPair(properties, propName, ir::MethodDefinitionKind::SET, access);
    }
}

void TSParser::ValidateIndexSignatureTypeAnnotation(ir::TypeNode *typeAnnotation)
{
    if (typeAnnotation == nullptr) {
        ThrowSyntaxError("An index signature must have a type annotation");
    }
}

bool TSParser::IsModifierKind(const lexer::Token &token)
{
    switch (token.KeywordType()) {
        case lexer::TokenType::KEYW_PUBLIC:
        case lexer::TokenType::KEYW_PRIVATE:
        case lexer::TokenType::KEYW_PROTECTED:
        case lexer::TokenType::KEYW_STATIC:
        case lexer::TokenType::KEYW_ASYNC:
        case lexer::TokenType::KEYW_ABSTRACT:
        case lexer::TokenType::KEYW_DECLARE:
        case lexer::TokenType::KEYW_READONLY:
            return true;
        default:
            break;
    }

    return false;
}

void TSParser::CheckIfTypeParameterNameIsReserved()
{
    if (Lexer()->GetToken().IsReservedTypeName()) {
        ThrowSyntaxError("Invalid type parameter name");
    }
}

void TSParser::CheckIfStaticConstructor(ir::ModifierFlags flags)
{
    if ((flags & ir::ModifierFlags::STATIC) != 0) {
        ThrowSyntaxError("Static modifier can not appear on a constructor");
    }
}

std::tuple<bool, bool, bool> TSParser::ParseComputedClassFieldOrIndexSignature(ir::Expression **propName)
{
    Lexer()->NextToken();  // eat left square bracket

    if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT &&
        Lexer()->Lookahead() == lexer::LEX_CHAR_COLON) {
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

        if (!typeAnnotation->IsTSNumberKeyword() && !typeAnnotation->IsTSStringKeyword()) {
            ThrowSyntaxError(
                "An index signature parameter type must be either "
                "'string' or 'number'");
        }

        id->SetTsTypeAnnotation(typeAnnotation);

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
            ThrowSyntaxError("']' expected");
        }

        *propName = id;
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

ir::TypeNode *TSParser::ParseFunctionReturnType([[maybe_unused]] ParserStatus status)
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options =
            TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::CAN_BE_TS_TYPE_PREDICATE;
        return ParseTypeAnnotation(&options);
    }

    return nullptr;
}

void TSParser::ValidateFunctionOverloadParams(const ArenaVector<ir::Expression *> &params)
{
    for (auto *it : params) {
        if (it->IsAssignmentPattern()) {
            ThrowSyntaxError(
                "A parameter initializer is only allowed in a function "
                "or constructor implementation.",
                it->Start());
        }
    }
}

std::tuple<bool, ir::BlockStatement *, lexer::SourcePosition, bool> TSParser::ParseFunctionBody(
    const ArenaVector<ir::Expression *> &params, ParserStatus newStatus, ParserStatus contextStatus)
{
    bool isDeclare = InAmbientContext();
    bool isOverload = false;
    bool letDeclare = true;
    ir::BlockStatement *body = nullptr;
    lexer::SourcePosition endLoc = Lexer()->GetToken().End();

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        if ((newStatus & ParserStatus::FUNCTION_DECLARATION) != 0) {
            ValidateFunctionOverloadParams(params);
        } else if (!isDeclare && ((contextStatus & ParserStatus::IN_METHOD_DEFINITION) == 0)) {
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

ir::AstNode *TSParser::ParseImportDefaultSpecifier(ArenaVector<ir::AstNode *> *specifiers)
{
    ir::Identifier *local = ParseNamedImport(&Lexer()->GetToken());
    Lexer()->NextToken();  // eat local name

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        Lexer()->NextToken();  // eat substitution
        if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
            ThrowSyntaxError("identifier expected");
        }

        return AllocNode<ir::TSImportEqualsDeclaration>(local, ParseModuleReference(), false);
    }

    auto *specifier = AllocNode<ir::ImportDefaultSpecifier>(local);
    ES2PANDA_ASSERT(specifier != nullptr);
    specifier->SetRange(specifier->Local()->Range());
    specifiers->push_back(specifier);

    Lexer()->NextToken();  // eat specifier name

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA) {
        Lexer()->NextToken();  // eat comma
    }

    return nullptr;
}

void TSParser::ParseCatchParamTypeAnnotation([[maybe_unused]] ir::AnnotatedExpression *param)
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'

        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        param->SetTsTypeAnnotation(ParseTypeAnnotation(&options));
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        ThrowSyntaxError("Catch clause variable cannot have an initializer");
    }
}

void TSParser::ReportPossibleOutOfBoundaryJumpError(bool allowBreak)
{
    if (((GetContext().Status() & ParserStatus::FUNCTION) != 0) && !allowBreak) {
        ThrowSyntaxError("Jump target cannot cross function boundary");
    }
}

void TSParser::ReportIllegalBreakError(const lexer::SourcePosition &pos)
{
    ThrowSyntaxError("A 'break' statement can only be used within an enclosing iteration or switch statement", pos);
}

void TSParser::ReportIllegalContinueError()
{
    ThrowSyntaxError("A 'continue' statement can only be used within an enclosing iteration statement");
}

void TSParser::ReportMultipleDefaultError()
{
    ThrowSyntaxError("A 'default' clause cannot appear more than once in a 'switch' statement");
}

void TSParser::ReportIllegalNewLineErrorAfterThrow()
{
    ThrowSyntaxError("Line break not permitted here");
}

void TSParser::ReportIfBodyEmptyError(ir::Statement *consequent)
{
    if (consequent->IsEmptyStatement()) {
        ThrowSyntaxError("The body of an if statement cannot be the empty statement");
    }
}

}  // namespace ark::es2panda::parser
