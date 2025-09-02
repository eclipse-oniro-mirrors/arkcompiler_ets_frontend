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
#include "ir/base/decorator.h"
#include "ir/base/catchClause.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/spreadElement.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/dummyNode.h"
#include "ir/module/importDeclaration.h"
#include "ir/module/importDefaultSpecifier.h"
#include "ir/module/importSpecifier.h"
#include "ir/module/exportSpecifier.h"
#include "ir/module/exportNamedDeclaration.h"
#include "ir/ets/etsPrimitiveType.h"
#include "ir/ets/etsPackageDeclaration.h"
#include "ir/ets/etsReExportDeclaration.h"
#include "ir/ets/etsWildcardType.h"
#include "ir/ets/etsTuple.h"
#include "ir/ets/etsFunctionType.h"
#include "ir/ets/etsModule.h"
#include "ir/ets/etsStringLiteralType.h"
#include "ir/ets/etsTypeReference.h"
#include "ir/ets/etsTypeReferencePart.h"
#include "ir/ets/etsNullishTypes.h"
#include "ir/ets/etsUnionType.h"
#include "ir/ets/etsImportDeclaration.h"
#include "ir/ets/etsStructDeclaration.h"
#include "ir/module/importNamespaceSpecifier.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsTypeParameterInstantiation.h"
#include "ir/ts/tsInterfaceBody.h"
#include "ir/ts/tsImportEqualsDeclaration.h"
#include "ir/ts/tsArrayType.h"
#include "ir/ts/tsQualifiedName.h"
#include "ir/ts/tsTypeReference.h"
#include "ir/ts/tsTypeParameter.h"
#include "ir/ts/tsInterfaceHeritage.h"
#include "ir/ts/tsFunctionType.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "ir/ts/tsTypeParameterDeclaration.h"
#include "ir/ts/tsThisType.h"
#include "generated/signatures.h"
#include "util/errorRecovery.h"
#include "generated/diagnostic.h"

namespace ark::es2panda::parser {
class FunctionContext;

using namespace std::literals::string_literals;

ir::TypeNode *ETSParser::ParseFunctionReturnType([[maybe_unused]] ParserStatus status)
{
    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
        return nullptr;
    }
    if ((status & ParserStatus::CONSTRUCTOR_FUNCTION) != 0U) {
        LogError(diagnostic::TYPE_ANNOTATION_FOR_CONSTRUCTOR);
    }
    Lexer()->NextToken();  // eat ':'
    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR |
                                           TypeAnnotationParsingOptions::CAN_BE_TS_TYPE_PREDICATE |
                                           TypeAnnotationParsingOptions::RETURN_TYPE;
    return ParseTypeAnnotation(&options);
}

ir::TypeNode *ETSParser::ParsePrimitiveType(TypeAnnotationParsingOptions *options, ir::PrimitiveType type)
{
    if (((*options) & TypeAnnotationParsingOptions::DISALLOW_PRIMARY_TYPE) != 0) {
        LogError(diagnostic::PRIMITIVE_NOT_ALLOWED);
    }

    auto *const typeAnnotation = AllocNode<ir::ETSPrimitiveType>(type, Allocator());
    ES2PANDA_ASSERT(typeAnnotation != nullptr);
    typeAnnotation->SetRange(Lexer()->GetToken().Loc());
    Lexer()->NextToken();
    return typeAnnotation;
}

ir::TypeNode *ETSParser::ParseUnionType(ir::TypeNode *const firstType)
{
    ArenaVector<ir::TypeNode *> types(Allocator()->Adapter());
    types.push_back(firstType->AsTypeNode());

    while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_BITWISE_OR) {
        Lexer()->NextToken();  // eat '|'

        auto options = TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::DISALLOW_UNION;
        auto typeAnnotation = ParseTypeAnnotation(&options);
        if (!typeAnnotation->IsBrokenTypeNode()) {
            types.push_back(typeAnnotation);
        }
    }

    auto const endLoc = types.back()->End();
    auto *const unionType = AllocNode<ir::ETSUnionType>(std::move(types), Allocator());
    ES2PANDA_ASSERT(unionType != nullptr);
    unionType->SetRange({firstType->Start(), endLoc});
    return unionType;
}

ir::TypeNode *ETSParser::GetTypeAnnotationOfPrimitiveType([[maybe_unused]] lexer::TokenType tokenType,
                                                          TypeAnnotationParsingOptions *options)
{
    ir::TypeNode *typeAnnotation = nullptr;
    switch (tokenType) {
        case lexer::TokenType::KEYW_BOOLEAN:
            typeAnnotation = ParsePrimitiveType(options, ir::PrimitiveType::BOOLEAN);
            break;
        case lexer::TokenType::KEYW_DOUBLE:
            typeAnnotation = ParsePrimitiveType(options, ir::PrimitiveType::DOUBLE);
            break;
        case lexer::TokenType::KEYW_BYTE:
            typeAnnotation = ParsePrimitiveType(options, ir::PrimitiveType::BYTE);
            break;
        case lexer::TokenType::KEYW_FLOAT:
            typeAnnotation = ParsePrimitiveType(options, ir::PrimitiveType::FLOAT);
            break;
        case lexer::TokenType::KEYW_SHORT:
            typeAnnotation = ParsePrimitiveType(options, ir::PrimitiveType::SHORT);
            break;
        case lexer::TokenType::KEYW_INT:
            typeAnnotation = ParsePrimitiveType(options, ir::PrimitiveType::INT);
            break;
        case lexer::TokenType::KEYW_CHAR:
            typeAnnotation = ParsePrimitiveType(options, ir::PrimitiveType::CHAR);
            break;
        case lexer::TokenType::KEYW_LONG:
            typeAnnotation = ParsePrimitiveType(options, ir::PrimitiveType::LONG);
            break;
        case lexer::TokenType::KEYW_VOID:
            typeAnnotation = ParsePrimitiveType(options, ir::PrimitiveType::VOID);
            break;
        default:
            typeAnnotation = ParseTypeReference(options);
            break;
    }
    return typeAnnotation;
}

ir::TypeNode *ETSParser::ParseWildcardType(TypeAnnotationParsingOptions *options)
{
    const auto varianceStartLoc = Lexer()->GetToken().Start();
    const auto varianceEndLoc = Lexer()->GetToken().End();
    const auto varianceModifier = ParseTypeVarianceModifier(options);

    bool isUnboundOut = varianceModifier == ir::ModifierFlags::OUT &&
                        (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_GREATER_THAN ||
                         Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA);
    ir::ETSTypeReference *typeReference = nullptr;
    if (!isUnboundOut) {
        auto reference = ParseTypeReference(options);
        ES2PANDA_ASSERT(reference != nullptr);
        typeReference = reference->AsETSTypeReference();
    }

    auto *wildcardType = AllocNode<ir::ETSWildcardType>(typeReference, varianceModifier, Allocator());
    ES2PANDA_ASSERT(wildcardType != nullptr);
    wildcardType->SetRange({varianceStartLoc, typeReference == nullptr ? varianceEndLoc : typeReference->End()});

    return wildcardType;
}

ir::TypeNode *ETSParser::ParseFunctionType(TypeAnnotationParsingOptions *options)
{
    auto startLoc = Lexer()->GetToken().Start();
    auto params = ParseFunctionParams();
    bool hasReceiver = !params.empty() && params[0]->IsETSParameterExpression() &&
                       params[0]->AsETSParameterExpression()->Ident()->IsReceiver();
    if (!Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_ARROW)) {
        if (((*options) & TypeAnnotationParsingOptions::REPORT_ERROR) != 0) {
            LogExpectedToken(lexer::TokenType::PUNCTUATOR_ARROW);
        }
        return nullptr;
    }
    ir::TypeNode *returnTypeAnnotation = nullptr;
    if (hasReceiver) {
        SavedParserContext contextAfterParseParams(this, GetContext().Status() | ParserStatus::HAS_RECEIVER);
        returnTypeAnnotation = ParseTypeAnnotation(options);
    } else {
        returnTypeAnnotation = ParseTypeAnnotation(options);
    }
    if (returnTypeAnnotation == nullptr) {
        return nullptr;
    }

    auto *funcType = AllocNode<ir::ETSFunctionType>(
        ir::FunctionSignature(nullptr, std::move(params), returnTypeAnnotation, hasReceiver),
        ir::ScriptFunctionFlags::NONE, Allocator());
    funcType->SetRange({startLoc, returnTypeAnnotation->End()});

    return funcType;
}

bool ETSParser::ParseTriplePeriod(bool spreadTypePresent)
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD) {
        if (spreadTypePresent) {
            LogError(diagnostic::ONLY_SPREAD_AT_LAST_INDEX);
        }

        spreadTypePresent = true;
        Lexer()->NextToken();  // eat '...'
    } else if (spreadTypePresent) {
        // This can't be implemented to any index, with type consistency. If a spread type is in the middle of
        // the tuple, then bounds check can't be made for element access, so the type of elements after the
        // spread can't be determined in compile time.
        LogError(diagnostic::SPREAD_MUST_BE_LAST_IN_TUPLE);
    }

    return spreadTypePresent;
}

ir::TypeNode *ETSParser::ParseETSTupleType(TypeAnnotationParsingOptions *const options)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET);

    const auto startLoc = Lexer()->GetToken().Start();
    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET);

    ArenaVector<ir::TypeNode *> tupleTypeList(Allocator()->Adapter());
    auto *const tupleType = AllocNode<ir::ETSTuple>(Allocator());

    auto parseElem = [this, options, &tupleTypeList, &tupleType]() {
        auto *const currentTypeAnnotation = ParseTypeAnnotation(options);
        if (currentTypeAnnotation == nullptr) {  // Error processing.
            Lexer()->NextToken();
            return false;
        }

        currentTypeAnnotation->SetParent(tupleType);
        tupleTypeList.push_back(currentTypeAnnotation);

        return true;
    };

    lexer::SourcePosition endLoc;
    ParseList(lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET, lexer::NextTokenFlags::NONE, parseElem, &endLoc, true);

    ES2PANDA_ASSERT(tupleType != nullptr);
    tupleType->SetTypeAnnotationsList(std::move(tupleTypeList));
    tupleType->SetRange({startLoc, endLoc});

    return tupleType;
}

// Helper function for  ETSParser::GetTypeAnnotationFromToken(...) method
ir::TypeNode *ETSParser::ParsePotentialFunctionalType(TypeAnnotationParsingOptions *options)
{
    auto savePos = Lexer()->Save();
    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);

    auto isPotentiallyOptionalWithTypeAnnotation = [this]() {
        bool res = false;

        if (Lexer()->Lookahead() != lexer::LEX_CHAR_QUESTION) {
            return res;
        }

        auto currentPos = Lexer()->Save();
        // jump to '?' token
        Lexer()->NextToken();

        if (Lexer()->Lookahead() == lexer::LEX_CHAR_COLON) {
            res = true;
        }

        Lexer()->Rewind(currentPos);
        return res;
    };
    if (((*options) & TypeAnnotationParsingOptions::IGNORE_FUNCTION_TYPE) == 0 &&
        (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS ||
         Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD ||
         Lexer()->Lookahead() == lexer::LEX_CHAR_COLON || isPotentiallyOptionalWithTypeAnnotation())) {
        GetContext().Status() |= (ParserStatus::ALLOW_DEFAULT_VALUE | ParserStatus::ALLOW_RECEIVER);
        // '(' is consumed in `ParseFunctionType`
        Lexer()->Rewind(savePos);
        auto typeAnnotation = ParseFunctionType(options);
        GetContext().Status() ^= (ParserStatus::ALLOW_DEFAULT_VALUE | ParserStatus::ALLOW_RECEIVER);
        return typeAnnotation;
    }
    Lexer()->Rewind(savePos);
    return nullptr;
}

std::pair<ir::TypeNode *, bool> ETSParser::ParseNonNullableType(TypeAnnotationParsingOptions *options)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_NON_NULLABLE);
    Lexer()->NextToken();  // eat NonNullable

    ExpectToken(lexer::TokenType::PUNCTUATOR_LESS_THAN, true);
    auto *const typeAnnotation = ParseTypeAnnotationNoPreferParam(options);
    ParsePunctuatorGreaterThan();
    return std::make_pair(AllocNode<ir::ETSNonNullishTypeNode>(typeAnnotation, Allocator()), true);
}

// Helper function to reduce line count of GetTypeAnnotationFromToken(...)
void ETSParser::CheckForConditionalTypeError(TypeAnnotationParsingOptions options, lexer::TokenType tokenType)
{
    if ((options & TypeAnnotationParsingOptions::TYPE_ALIAS_CONTEXT) != 0 &&
        (tokenType == lexer::TokenType::KEYW_EXTENDS || tokenType == lexer::TokenType::PUNCTUATOR_QUESTION_MARK)) {
        LogError(diagnostic::ERROR_ARKTS_NO_CONDITIONAL_TYPES);
    }
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
// Just to reduce the size of ParseTypeAnnotation(...) method
std::pair<ir::TypeNode *, bool> ETSParser::GetTypeAnnotationFromToken(TypeAnnotationParsingOptions *options)
{
    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_NON_NULLABLE) {
        return ParseNonNullableType(options);
    }

    if (IsPrimitiveType(Lexer()->GetToken().KeywordType()) ||
        Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
        auto typeAnnotation = ParseLiteralIdent(options);
        CheckForConditionalTypeError(*options, Lexer()->GetToken().Type());
        if (((*options) & TypeAnnotationParsingOptions::POTENTIAL_CLASS_LITERAL) != 0 &&
            (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_CLASS || IsStructKeyword())) {
            return std::make_pair(typeAnnotation, false);
        }
        return std::make_pair(typeAnnotation, true);
    }

    ir::TypeNode *typeAnnotation;
    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::LITERAL_NULL: {
            typeAnnotation = AllocNode<ir::ETSNullType>(Allocator());
            break;
        }
        case lexer::TokenType::KEYW_UNDEFINED: {
            typeAnnotation = AllocNode<ir::ETSUndefinedType>(Allocator());
            break;
        }
        case lexer::TokenType::LITERAL_STRING: {
            typeAnnotation = AllocNode<ir::ETSStringLiteralType>(Lexer()->GetToken().String(), Allocator());
            break;
        }
        case lexer::TokenType::PUNCTUATOR_BACK_TICK: {
            return std::make_pair(ParseMultilineString(), true);
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS: {
            return GetTypeAnnotationFromParentheses(options);
        }
        case lexer::TokenType::PUNCTUATOR_FORMAT:
            return std::make_pair(ParseTypeFormatPlaceholder(), true);
        case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET:
            return std::make_pair(ParseETSTupleType(options), true);
        case lexer::TokenType::KEYW_THIS:
            return std::make_pair(ParseThisType(options), true);
        case lexer::TokenType::PUNCTUATOR_LEFT_BRACE:
            if (((*options) & TypeAnnotationParsingOptions::REPORT_ERROR) != 0) {
                auto startPos = Lexer()->GetToken().Start();
                auto modifiers = ir::ClassDefinitionModifiers::ID_REQUIRED | ir::ClassDefinitionModifiers::CLASS_DECL |
                                 ir::ClassDefinitionModifiers::DECLARATION;
                auto flags = ir::ModifierFlags::NONE;
                ParseClassBody(modifiers, flags);
                LogError(diagnostic::ERROR_ARKTS_NO_OBJ_LITERAL_TO_DECL_TYPE, {}, startPos);
                return {AllocBrokenType({startPos, Lexer()->GetToken().End()}), false};
            }
            [[fallthrough]];
        default: {
            return {nullptr, true};
        }
    }
    ES2PANDA_ASSERT(typeAnnotation != nullptr);
    typeAnnotation->SetRange(Lexer()->GetToken().Loc());
    Lexer()->NextToken();
    return std::make_pair(typeAnnotation, true);
}

std::pair<ir::TypeNode *, bool> ETSParser::GetTypeAnnotationFromParentheses(TypeAnnotationParsingOptions *options)
{
    auto startLoc = Lexer()->GetToken().Start();

    ir::TypeNode *typeAnnotation = ParsePotentialFunctionalType(options);
    if (typeAnnotation != nullptr) {
        typeAnnotation->SetStart(startLoc);
        return std::make_pair(typeAnnotation, true);
    }

    lexer::LexerPosition savedPos = Lexer()->Save();
    if (!Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS)) {
        if (((*options) & TypeAnnotationParsingOptions::REPORT_ERROR) != 0) {
            LogExpectedToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
        }
        return {nullptr, false};
    }

    typeAnnotation = ParseTypeAnnotation(options);
    if (typeAnnotation == nullptr) {
        return std::make_pair(typeAnnotation, true);
    }

    typeAnnotation->SetStart(startLoc);

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_BITWISE_OR) {
        typeAnnotation = ParseUnionType(typeAnnotation);
    }

    ParseRightParenthesis(options, typeAnnotation, savedPos);

    return std::make_pair(typeAnnotation, true);
}

bool IsSimpleReturnThis(lexer::Token const &tokenAfterThis)
{
    return (tokenAfterThis.Type() == lexer::TokenType::PUNCTUATOR_ARROW) ||
           (tokenAfterThis.Type() == lexer::TokenType::PUNCTUATOR_COMMA) ||
           (tokenAfterThis.Type() == lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) ||
           (tokenAfterThis.Type() == lexer::TokenType::PUNCTUATOR_LEFT_BRACE) ||
           (tokenAfterThis.Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) ||
           ((tokenAfterThis.Flags() & lexer::TokenFlags::NEW_LINE) != 0);
}

ir::TypeNode *ETSParser::ParseThisType(TypeAnnotationParsingOptions *options)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::KEYW_THIS);

    auto startPos = Lexer()->GetToken().Start();
    auto const tokenLoc = Lexer()->GetToken().Loc();
    Lexer()->NextToken();  // eat 'this'

    // A syntax error should be thrown if
    // - the usage of 'this' as a type is not allowed in the current context, or
    // - 'this' is not used as a return type, or
    // - the current context is an arrow function (might be inside a method of a class where 'this' is allowed).
    // - Specially, Function with Receiver, Lambda with Receiver and Function type with Receiver can return 'this'.
    bool reportErr = (*options & TypeAnnotationParsingOptions::REPORT_ERROR) != 0;
    bool allowThisType = ((GetContext().Status() & ParserStatus::ALLOW_THIS_TYPE) != 0);
    bool isArrowFunc = (GetContext().Status() & ParserStatus::ARROW_FUNCTION) != 0;
    bool parseReturnType = (*options & TypeAnnotationParsingOptions::RETURN_TYPE) != 0;
    bool isExtensionFunction = (GetContext().Status() & ParserStatus::HAS_RECEIVER) != 0;
    bool isInUnion = (*options & TypeAnnotationParsingOptions::DISALLOW_UNION) != 0;
    bool isInstance = (*options & TypeAnnotationParsingOptions::INSTANCEOF) != 0;
    if (isExtensionFunction) {
        if (reportErr && (!IsSimpleReturnThis(Lexer()->GetToken()) || isInUnion)) {
            LogError(diagnostic::THIS_IN_NON_STATIC_METHOD, {}, startPos);
        }
    } else if (reportErr && (isInstance)) {
        LogError(diagnostic::INVALID_RIGHT_INSTANCEOF, {}, startPos);
    } else if (reportErr && (!allowThisType || !parseReturnType || isArrowFunc)) {
        LogError(diagnostic::THIS_IN_NON_STATIC_METHOD, {}, startPos);
    }

    auto *const thisType = AllocNode<ir::TSThisType>(Allocator());
    ES2PANDA_ASSERT(thisType != nullptr);
    thisType->SetRange(tokenLoc);

    return thisType;
}

ir::TypeNode *ETSParser::ParseTsArrayType(ir::TypeNode *typeNode, TypeAnnotationParsingOptions *options)
{
    while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
        if (((*options) & TypeAnnotationParsingOptions::POTENTIAL_NEW_ARRAY) != 0) {
            return typeNode;
        }

        lexer::SourcePosition startPos = Lexer()->GetToken().Start();

        Lexer()->NextToken();  // eat '['

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
            if ((*options & TypeAnnotationParsingOptions::REPORT_ERROR) != 0) {
                LogExpectedToken(lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET);
                return AllocBrokenType({Lexer()->GetToken().Start(), Lexer()->GetToken().End()});
            }
            return AllocBrokenType(startPos);
        }

        typeNode = AllocNode<ir::TSArrayType>(typeNode, Allocator());
        typeNode->SetRange({startPos, Lexer()->GetToken().End()});
        Lexer()->NextToken();  // eat ']'
    }
    return typeNode;
}

ir::TypeNode *ETSParser::ParseTypeAnnotationNoPreferParam(TypeAnnotationParsingOptions *options)
{
    bool const reportError = ((*options) & TypeAnnotationParsingOptions::REPORT_ERROR) != 0;
    ArenaVector<ir::AnnotationUsage *> annotations {Allocator()->Adapter()};
    if (Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_AT)) {
        annotations = ParseAnnotations(false);
    }

    auto startPos = Lexer()->GetToken().Start();
    auto [typeAnnotation, needFurtherProcessing] = GetTypeAnnotationFromToken(options);

    if (typeAnnotation == nullptr) {
        if (reportError) {
            LogError(diagnostic::INVALID_TYPE);
        }
        return AllocBrokenType({Lexer()->GetToken().Start(), Lexer()->GetToken().End()});
    }

    if (!needFurtherProcessing || typeAnnotation->IsBrokenTypeNode()) {
        return typeAnnotation;
    }

    typeAnnotation = ParseTsArrayType(typeAnnotation, options);

    ApplyAnnotationsToNode(typeAnnotation, std::move(annotations), startPos, *options);
    return typeAnnotation;
}

bool ETSParser::ParseReadonlyInTypeAnnotation()
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        Lexer()->NextToken();  // eat '<'
        Lexer()->NextToken();  // eat 'identifier'
        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_GREATER_THAN) {
            Lexer()->NextToken();  // eat '>'
            return true;
        }
    }

    return false;
}

static bool IsReadonlyApplicableType(ir::TypeNode *typeNode)
{
    return typeNode->IsTSArrayType() || typeNode->IsETSTuple() ||
           (typeNode->IsETSTypeReference() &&
            typeNode->AsETSTypeReference()->BaseName()->Name() == compiler::Signatures::ARRAY);
}

ir::TypeNode *ETSParser::ParseTypeAnnotation(TypeAnnotationParsingOptions *options)
{
    const auto startPos = Lexer()->GetToken().Start();
    if ((*options &
         (TypeAnnotationParsingOptions::DISALLOW_UNION | TypeAnnotationParsingOptions::ANNOTATION_NOT_ALLOW)) == 0 &&
        Lexer()->TryEatTokenFromKeywordType(lexer::TokenType::KEYW_TYPEOF)) {
        LogError(diagnostic::TYPEOF_IN_ANNOTATION);
        return AllocBrokenType(Lexer()->GetToken().Loc());
    }

    bool hasReadonly = Lexer()->TryEatTokenFromKeywordType(lexer::TokenType::KEYW_READONLY);

    auto typeAnnotation = ParseTypeAnnotationNoPreferParam(options);
    if (typeAnnotation->IsBrokenTypeNode()) {
        return typeAnnotation;
    }

    if (hasReadonly) {
        if (!IsReadonlyApplicableType(typeAnnotation)) {
            if (!ParseReadonlyInTypeAnnotation()) {
                LogError(diagnostic::READONLY_ONLY_ON_ARRAY_OR_TUPLE);
            } else {
                LogError(diagnostic::READONLY_TYPE_EXPECTED);
            }
        }
        typeAnnotation->SetStart(startPos);
        typeAnnotation->AddModifier(ir::ModifierFlags::READONLY_PARAMETER);
    }

    if (((*options) & TypeAnnotationParsingOptions::DISALLOW_UNION) == 0 &&
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_BITWISE_OR) {
        return ParseUnionType(typeAnnotation);
    }

    return typeAnnotation;
}

ir::TypeNode *ETSParser::ParseMultilineString()
{
    const auto startPos = Lexer()->GetToken().Start();
    const auto multilineStr = Lexer()->ScanMultilineString();
    Lexer()->ScanTemplateStringEnd();

    auto typeAnnotation = AllocNode<ir::ETSStringLiteralType>(multilineStr, Allocator());
    ES2PANDA_ASSERT(typeAnnotation != nullptr);
    typeAnnotation->SetRange({startPos, Lexer()->GetToken().End()});
    Lexer()->NextToken();

    return typeAnnotation;
}

}  // namespace ark::es2panda::parser
