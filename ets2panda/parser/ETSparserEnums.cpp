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
#include "ir/statements/functionDeclaration.h"
#include "ir/statements/expressionStatement.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/variableDeclarator.h"
#include "ir/statements/variableDeclaration.h"
#include "ir/expressions/dummyNode.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/thisExpression.h"
#include "ir/expressions/typeofExpression.h"
#include "ir/expressions/memberExpression.h"
#include "ir/expressions/updateExpression.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/expressions/unaryExpression.h"
#include "ir/expressions/yieldExpression.h"
#include "ir/expressions/awaitExpression.h"
#include "ir/expressions/literals/nullLiteral.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/expressions/literals/undefinedLiteral.h"
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

// NOLINTNEXTLINE(google-default-arguments)
ir::Statement *ETSParser::ParseEnumDeclaration(bool isConst, bool isStatic)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::KEYW_ENUM);

    lexer::SourcePosition enumStart = Lexer()->GetToken().Start();
    Lexer()->NextToken();  // eat enum keyword

    auto *key = ExpectIdentifier(false, true);

    auto *declNode = ParseEnumMembers(key, enumStart, isConst, isStatic);
    if (declNode == nullptr) {  // Error processing.
        // Error is logged inside ParseEnumMembers
        return AllocBrokenStatement(enumStart);
    }
    return declNode;
}

ir::Statement *ETSParser::ParsePotentialConstEnum(VariableParsingFlags flags)
{
    if ((flags & VariableParsingFlags::CONST) == 0) {
        LogError(diagnostic::VAR_DEC_EXPECTED);
    }

    // According to the ArkTS specification:
    // const enum is supported for source-level compatibility with TypeScript,
    // and const is skipped as it has no impact on enum semantics in ArkTS.
    return ParseEnumDeclaration(false);
}

ir::TSEnumDeclaration *ETSParser::ParseEnumMembers(ir::Identifier *const key, const lexer::SourcePosition &enumStart,
                                                   const bool isConst, const bool isStatic)
{
    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE &&
        Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE);
    }
    ir::TypeNode *typeAnnotation = nullptr;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
        typeAnnotation = ParseTypeAnnotation(&options);

        // According to a comment on ETSparser.cpp:1598, compiler can't process ": string" correctly.
        // ParseTypeAnnotation reads ": string" as literal so it's not supported here for now.
        auto startPos = Lexer()->GetToken().Start();
        if (!typeAnnotation->IsETSPrimitiveType()) {
            LogError(diagnostic::UNSUPPORTED_ENUM_TYPE, {}, startPos);
            typeAnnotation = nullptr;
        } else {
            ir::PrimitiveType primitiveType = typeAnnotation->AsETSPrimitiveType()->GetPrimitiveType();
            if (primitiveType != ir::PrimitiveType::INT && primitiveType != ir::PrimitiveType::LONG &&
                primitiveType != ir::PrimitiveType::DOUBLE) {
                // Issue: #26024 Numeric support for enum
                LogError(diagnostic::UNSUPPORTED_ENUM_TYPE, {}, startPos);
                typeAnnotation = nullptr;
            }
        }
    }

    Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);  // eat '{'

    ArenaVector<ir::AstNode *> members(Allocator()->Adapter());

    lexer::SourcePosition enumEnd = ParseEnumMember(members);

    ir::TSEnumDeclaration *enumDeclaration;

    if (typeAnnotation == nullptr) {
        enumDeclaration = AllocNode<ir::TSEnumDeclaration>(
            Allocator(), key, std::move(members),
            ir::TSEnumDeclaration::ConstructorFlags {isConst, isStatic, InAmbientContext()},
            GetContext().GetLanguage());
    } else {
        enumDeclaration = AllocNode<ir::TSEnumDeclaration>(
            Allocator(), key, std::move(members),
            ir::TSEnumDeclaration::ConstructorFlags {isConst, isStatic, InAmbientContext()}, typeAnnotation,
            GetContext().GetLanguage());
    }

    ES2PANDA_ASSERT(enumDeclaration != nullptr);

    if (InAmbientContext()) {
        enumDeclaration->AddModifier(ir::ModifierFlags::DECLARE);
    }
    enumDeclaration->SetRange({enumStart, enumEnd});

    return enumDeclaration;
}

ir::Expression *ETSParser::ParseEnumExpression()
{
    ir::Expression *expression {};
    auto endLoc = Lexer()->GetToken().Start();
    expression = ParseExpression();
    if (expression == nullptr) {
        LogError(diagnostic::ENUM_INVALID_INIT, {}, endLoc);
        // Continue to parse the rest of Enum.
        return AllocNode<ir::NumberLiteral>(lexer::Number(0));
    }
    return expression;
}

bool ETSParser::ParseNumberEnumHelper()
{
    bool minusSign = false;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PLUS) {
        Lexer()->NextToken();
    } else if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MINUS) {
        minusSign = true;
        Lexer()->NextToken();
    }

    if (Lexer()->GetToken().Type() != lexer::TokenType::LITERAL_NUMBER) {
        // enum15.ets; will be zero by default
        LogError(diagnostic::INVALID_ENUM_TYPE, {}, Lexer()->GetToken().Start());
        Lexer()->GetToken().SetTokenType(lexer::TokenType::LITERAL_NUMBER);
        Lexer()->GetToken().SetTokenStr(ERROR_LITERAL);
    }
    return minusSign;
}

lexer::SourcePosition ETSParser::ParseEnumMember(ArenaVector<ir::AstNode *> &members)
{
    // Default enum number value
    ir::Expression *currentNumberExpr = AllocNode<ir::NumberLiteral>(lexer::Number(0));

    // Lambda to parse enum member (maybe with initializer)
    auto const parseMember = [this, &members, &currentNumberExpr](bool &) {
        auto *const ident = ExpectIdentifier(false, true);

        ir::Expression *ordinal;
        lexer::SourcePosition endLoc;
        bool isGenerated = false;

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
            // Case when user explicitly set the value for enumeration constant
            Lexer()->NextToken();

            ordinal = ParseEnumExpression();
            currentNumberExpr = ordinal;

            endLoc = ordinal->End();
        } else {
            // Default enumeration constant value. Equal to 0 for the first item and = previous_value + 1 for all
            // the others.
            ordinal = currentNumberExpr;
            ordinal->SetRange({ident->End(), ident->End()});
            isGenerated = true;

            endLoc = ident->End();
        }

        auto *const member = AllocNode<ir::TSEnumMember>(ident, ordinal, isGenerated);
        member->SetRange({ident->Start(), endLoc});
        members.emplace_back(member);

        // Increment the value by one
        auto incrementNode = AllocNode<ir::NumberLiteral>(lexer::Number(1));
        ir::Expression *dummyNode = currentNumberExpr->Clone(Allocator(), nullptr)->AsExpression();
        currentNumberExpr =
            AllocNode<ir::BinaryExpression>(dummyNode, incrementNode, lexer::TokenType::PUNCTUATOR_PLUS);
        return true;
    };

    lexer::SourcePosition enumEnd;
    ParseList(lexer::TokenType::PUNCTUATOR_RIGHT_BRACE, lexer::NextTokenFlags::KEYWORD_TO_IDENT, parseMember, &enumEnd,
              ParseListOptions::ALLOW_TRAILING_SEP);
    return enumEnd;
}

}  // namespace ark::es2panda::parser
