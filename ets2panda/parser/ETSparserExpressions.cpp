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

#include "ir/ets/etsTuple.h"
#include "ir/ets/etsDestructuring.h"
#include "ir/expressions/literals/undefinedLiteral.h"
#include "lexer/lexer.h"

namespace ark::es2panda::parser {
class FunctionContext;

using namespace std::literals::string_literals;

static std::string GetArgumentsSourceView(lexer::Lexer *lexer, const util::StringView::Iterator &lexerPos)
{
    std::string value = lexer->SourceView(lexerPos.Index(), lexer->Save().Iterator().Index()).Mutf8();
    while (!value.empty() && value.back() == ' ') {
        value.pop_back();
    }

    if (!value.empty() && (value.back() == ')' || value.back() == ',')) {
        value.pop_back();
    }

    return value;
}

ir::Expression *ETSParser::ParseFunctionParameterExpression(ir::AnnotatedExpression *const paramIdent, bool isOptional)
{
    ir::ETSParameterExpression *paramExpression;
    ES2PANDA_ASSERT(paramIdent != nullptr);
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        auto const lexerPos = Lexer()->Save().Iterator();
        Lexer()->NextToken();  // eat '='

        if (paramIdent->IsRestElement()) {
            LogError(diagnostic::NO_DEFAULT_FOR_REST);
        }

        if ((GetContext().Status() & ParserStatus::ALLOW_DEFAULT_VALUE) != 0) {
            LogError(diagnostic::DEFAULT_ONLY_FOR_OPTIONAL);
        }

        if (isOptional) {
            LogError(diagnostic::DEFAULT_UNDEF_NOT_ALLOWED);
        }
        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS ||
            Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA) {
            LogError(diagnostic::VALUE_IS_NOT_SET);
        }

        auto defaultValue = ParseExpression();
        if (!paramIdent->IsIdentifier()) {
            LogError(diagnostic::IDENTIFIER_EXPECTED);
            return AllocBrokenExpression(Lexer()->GetToken().Loc());
        }

        paramExpression = AllocNode<ir::ETSParameterExpression>(paramIdent->AsIdentifier(), defaultValue, Allocator());
        ES2PANDA_ASSERT(paramExpression != nullptr);

        std::string value = GetArgumentsSourceView(Lexer(), lexerPos);
        paramExpression->SetLexerSaved(util::UString(value, Allocator()).View());
        paramExpression->SetRange({paramIdent->Start(), paramExpression->Initializer()->End()});

        if (paramIdent->TypeAnnotation() == nullptr && defaultValue != nullptr) {
            LogError(diagnostic::EXPLICIT_PARAM_TYPE, util::DiagnosticMessageParams {}, paramIdent->Start());
            paramExpression->SetInitializer(nullptr);
        }
    } else if (paramIdent->IsIdentifier()) {
        paramExpression = AllocNode<ir::ETSParameterExpression>(paramIdent->AsIdentifier(), isOptional, Allocator());
        ES2PANDA_ASSERT(paramExpression != nullptr);
        paramExpression->SetRange({paramIdent->Start(), paramIdent->End()});
    } else {
        paramExpression = AllocNode<ir::ETSParameterExpression>(paramIdent->AsRestElement(), false, Allocator());
        ES2PANDA_ASSERT(paramExpression != nullptr);
        paramExpression->SetRange({paramIdent->Start(), paramIdent->End()});
    }
    return paramExpression;
}

ir::Expression *ETSParser::ResolveArgumentUnaryExpr(ExpressionParseFlags flags)
{
    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_PLUS:
        case lexer::TokenType::PUNCTUATOR_MINUS:
        case lexer::TokenType::PUNCTUATOR_TILDE:
        case lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK:
        case lexer::TokenType::PUNCTUATOR_PLUS_PLUS:
        case lexer::TokenType::PUNCTUATOR_MINUS_MINUS:
        case lexer::TokenType::KEYW_TYPEOF: {
            return ParseUnaryOrPrefixUpdateExpression();
        }
        default: {
            return ParseLeftHandSideExpression(flags);
        }
    }
}

ir::Expression *ETSParser::CreateUnaryExpressionFromArgument(ir::Expression *argument, lexer::TokenType operatorType,
                                                             char32_t beginningChar)
{
    auto checkLiteral = [argument, beginningChar]() -> bool {
        ir::NumberLiteral *literal = nullptr;
        if (argument->IsNumberLiteral()) {
            literal = argument->AsNumberLiteral();
        } else if (argument->IsCallExpression() && argument->AsCallExpression()->Callee()->IsMemberExpression() &&
                   argument->AsCallExpression()->Callee()->AsMemberExpression()->Object()->IsNumberLiteral()) {
            literal = argument->AsCallExpression()->Callee()->AsMemberExpression()->Object()->AsNumberLiteral();
        }
        return literal != nullptr && ((beginningChar >= '0' && beginningChar <= '9') || (beginningChar == '.'));
    };

    ir::Expression *returnExpr = nullptr;
    if (lexer::Token::IsUpdateToken(operatorType)) {
        returnExpr = AllocNode<ir::UpdateExpression>(argument, operatorType, true);
    } else if (operatorType == lexer::TokenType::KEYW_TYPEOF) {
        returnExpr = AllocNode<ir::TypeofExpression>(argument);
    } else if (operatorType == lexer::TokenType::PUNCTUATOR_MINUS && checkLiteral()) {
        returnExpr = argument;
    } else {
        returnExpr = AllocNode<ir::UnaryExpression>(argument, operatorType);
    }

    return returnExpr;
}

static bool IsLeftHandSideExpression(lexer::TokenType &operatorType, lexer::NextTokenFlags &tokenFlags)
{
    switch (operatorType) {
        case lexer::TokenType::PUNCTUATOR_MINUS:
            tokenFlags = lexer::NextTokenFlags::UNARY_MINUS;
            [[fallthrough]];
        case lexer::TokenType::PUNCTUATOR_PLUS_PLUS:
        case lexer::TokenType::PUNCTUATOR_MINUS_MINUS:
        case lexer::TokenType::PUNCTUATOR_PLUS:
        case lexer::TokenType::PUNCTUATOR_TILDE:
        case lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK:
        case lexer::TokenType::KEYW_TYPEOF:
        case lexer::TokenType::KEYW_AWAIT:
            return false;
        default:
            return true;
    }
}

ir::Expression *ETSParser::HandleDeepNesting()
{
    LogError(diagnostic::DEEP_NESTING);
    while (Lexer()->GetToken().Type() != lexer::TokenType::EOS) {
        Lexer()->NextToken();
    }
    return AllocBrokenExpression(Lexer()->GetToken().Start());
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ETSParser::ParseUnaryOrPrefixUpdateExpression(ExpressionParseFlags flags)
{
    TrackRecursive trackRecursive(RecursiveCtx());
    if (!trackRecursive) {
        return HandleDeepNesting();
    }
    auto tokenFlags = lexer::NextTokenFlags::NONE;
    lexer::TokenType operatorType = Lexer()->GetToken().Type();
    if (IsLeftHandSideExpression(operatorType, tokenFlags)) {
        return ParseLeftHandSideExpression(flags);
    }

    char32_t beginningChar = Lexer()->Lookahead();
    auto start = Lexer()->GetToken().Start();
    Lexer()->NextToken(tokenFlags);

    ir::Expression *argument = ResolveArgumentUnaryExpr(flags);
    if (operatorType == lexer::TokenType::KEYW_AWAIT) {
        auto *awaitExpr = AllocNode<ir::AwaitExpression>(argument);
        ES2PANDA_ASSERT(awaitExpr != nullptr);
        awaitExpr->SetRange({start, argument->End()});
        return awaitExpr;
    }

    if (argument == nullptr) {
        return nullptr;
    }

    if (lexer::Token::IsUpdateToken(operatorType)) {
        if (!argument->IsIdentifier() && !argument->IsMemberExpression()) {
            LogError(diagnostic::INVALID_LEFT_IN_PREFIX);
        }
    }

    ir::Expression *returnExpr = CreateUnaryExpressionFromArgument(argument, operatorType, beginningChar);
    ES2PANDA_ASSERT(returnExpr != nullptr);
    returnExpr->SetRange({start, argument->End()});
    return returnExpr;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ETSParser::ParsePropertyDefinition(ExpressionParseFlags flags)
{
    ir::PropertyKind propertyKind = ir::PropertyKind::INIT;
    ParserStatus methodStatus = ParserStatus::NO_OPTS;

    const auto startPos = Lexer()->Save();
    Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
    lexer::SourcePosition start = Lexer()->GetToken().Start();

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD) {
        return ParseSpreadElement(flags);
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
        if (ParsePropertyModifiers(flags, &propertyKind, &methodStatus)) {
            return ParseShorthandProperty(&startPos);
        }
    } else if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MULTIPLY) {
        ParseGeneratorPropertyModifier(flags, &methodStatus);
    }

    bool isComputed = Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET;

    ir::Expression *key = ParsePropertyKey(flags);

    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
        methodStatus |= ParserStatus::FUNCTION;
    }

    ir::Expression *value = ParsePropertyValue(propertyKind, methodStatus, flags);
    ES2PANDA_ASSERT(value != nullptr);
    lexer::SourcePosition end = value->End();

    ir::Expression *returnProperty = nullptr;
    if (propertyKind == ir::PropertyKind::INIT) {
        //  Additional processing for method [re-]definition in object literal
        if (value->IsFunctionExpression()) {
            ES2PANDA_ASSERT(key->IsIdentifier() && value->AsFunctionExpression()->Function() != nullptr);

            auto *function = value->AsFunctionExpression()->Function();
            function->AddFlag(ir::ScriptFunctionFlags::ARROW);
            function->SetRange(value->Range());
            function->SetIdent(key->AsIdentifier()->Clone(Allocator(), nullptr));

            value = AllocNode<ir::ArrowFunctionExpression>(function, Allocator());
            value->SetRange(function->Range());
        }

        returnProperty =
            AllocNode<ir::Property>(propertyKind, key, value, methodStatus != ParserStatus::NO_OPTS, isComputed);
        returnProperty->SetRange({start, end});
    } else {
        returnProperty = AllocBrokenExpression(key->Start());
        LogError(diagnostic::OBJECT_PATTER_CONTAIN_METHODS, {}, returnProperty->Start());
    }

    return returnProperty;
}

bool CheckNextTokenOfTypeof(const lexer::Token &token)
{
    bool pretendTypeof = token.KeywordType() == lexer::TokenType::KEYW_TYPEOF;
    bool pretendIdent = token.IsLiteral();
    bool pretendOperator = token.IsUpdate();
    bool pretendUnary = token.IsUnary();
    bool pretendPuctuator = token.IsTsParamToken(token.Type());
    return (pretendTypeof || pretendIdent || pretendOperator || pretendUnary || pretendPuctuator);
}

ir::Expression *ETSParser::ParsePropertyKey([[maybe_unused]] ExpressionParseFlags flags)
{
    ir::Expression *key = nullptr;

    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::LITERAL_IDENT: {
            const util::StringView &ident = Lexer()->GetToken().Ident();
            key = AllocNode<ir::Identifier>(ident, Allocator());
            ES2PANDA_ASSERT(key != nullptr);
            key->SetRange(Lexer()->GetToken().Loc());
            Lexer()->NextToken();
            return key;
        }
        case lexer::TokenType::LITERAL_STRING: {
            const util::StringView &string = Lexer()->GetToken().String();
            key = AllocNode<ir::StringLiteral>(string);
            ES2PANDA_ASSERT(key != nullptr);
            key->SetRange(Lexer()->GetToken().Loc());
            Lexer()->NextToken();
            return key;
        }
        case lexer::TokenType::LITERAL_NUMBER: {
            if ((Lexer()->GetToken().Flags() & lexer::TokenFlags::NUMBER_BIGINT) != 0) {
                key = AllocNode<ir::BigIntLiteral>(Lexer()->GetToken().BigInt());
            } else {
                key = AllocNode<ir::NumberLiteral>(Lexer()->GetToken().GetNumber());
            }
            ES2PANDA_ASSERT(key != nullptr);
            key->SetRange(Lexer()->GetToken().Loc());
            Lexer()->NextToken();
            return key;
        }
        default: {
            const auto &rangeToken = Lexer()->GetToken().Loc();
            LogError(diagnostic::UNEXPECTED_TOKEN);
            Lexer()->NextToken();
            return AllocBrokenExpression(rangeToken);
        }
    }
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ETSParser::ParseDefaultPrimaryExpression(ExpressionParseFlags flags)
{
    auto startLoc = Lexer()->GetToken().Start();
    auto savedPos = Lexer()->Save();
    TypeAnnotationParsingOptions options =
        TypeAnnotationParsingOptions::POTENTIAL_CLASS_LITERAL | TypeAnnotationParsingOptions::IGNORE_FUNCTION_TYPE |
        TypeAnnotationParsingOptions::DISALLOW_UNION | TypeAnnotationParsingOptions::IGNORE_KEYW_KEYOF;
    ir::TypeNode *potentialType = ParseTypeAnnotation(&options);

    if (potentialType != nullptr) {
        // #29702 remove special handling for TSArray.
        if (potentialType->IsTSArrayType()) {
            auto currentSavedPos = Lexer()->Save();
            Lexer()->Rewind(savedPos);
            const auto tokenNow = Lexer()->GetToken();
            LogUnexpectedToken(tokenNow);
            Lexer()->Rewind(currentSavedPos);
            return AllocBrokenExpression(tokenNow.Loc());
        }

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD) {
            Lexer()->NextToken();  // eat '.'
        }

        if ((Lexer()->GetToken().Type() == lexer::TokenType::KEYW_CLASS || IsStructKeyword()) &&
            potentialType->IsBrokenTypeNode()) {
            Lexer()->NextToken();  // eat 'class' and 'struct'
            auto *classLiteral = AllocNode<ir::ETSClassLiteral>(potentialType);
            ES2PANDA_ASSERT(classLiteral != nullptr);
            classLiteral->SetRange({startLoc, Lexer()->GetToken().End()});
            return classLiteral;
        }
    }

    Lexer()->Rewind(savedPos);

    Lexer()->NextToken();
    bool pretendArrow = Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_ARROW;
    bool checkNextTokenOfTypeof = CheckNextTokenOfTypeof(Lexer()->GetToken());
    Lexer()->Rewind(savedPos);

    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_TYPEOF && checkNextTokenOfTypeof) {
        return ParseUnaryOrPrefixUpdateExpression();
    }

    if (Lexer()->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
        if (pretendArrow) {
            return ParseArrowFunctionExpression();
        }
        return ParsePrimaryExpressionIdent(flags);
    }

    const auto tokenNow = Lexer()->GetToken();
    LogUnexpectedToken(tokenNow);
    Lexer()->NextToken();  // eat an unexpected token
    return AllocBrokenExpression(tokenNow.Loc());
}

ir::Expression *ETSParser::ParsePrimaryExpressionWithLiterals(ExpressionParseFlags flags)
{
    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::LITERAL_TRUE:
        case lexer::TokenType::LITERAL_FALSE: {
            return ParseBooleanLiteral();
        }
        case lexer::TokenType::LITERAL_NULL: {
            return ParseNullLiteral();
        }
        case lexer::TokenType::KEYW_UNDEFINED: {
            return ParseUndefinedLiteral();
        }
        case lexer::TokenType::LITERAL_NUMBER: {
            return ParseNumberLiteral();
        }
        case lexer::TokenType::LITERAL_STRING: {
            return ParseStringLiteral();
        }
        case lexer::TokenType::LITERAL_CHAR: {
            return ParseCharLiteral();
        }
        default: {
            return ParseDefaultPrimaryExpression(flags);
        }
    }
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ETSParser::ParsePrimaryExpression(ExpressionParseFlags flags)
{
    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS: {
            return ParseCoverParenthesizedExpressionAndArrowParameterList(flags);
        }
        case lexer::TokenType::KEYW_THIS: {
            return ParseThisExpression();
        }
        case lexer::TokenType::KEYW_SUPER: {
            return ParseSuperExpression();
        }
        case lexer::TokenType::KEYW_NEW: {
            return ParseNewExpression();
        }
        case lexer::TokenType::KEYW_ASYNC: {
            return ParseAsyncExpression();
        }
        case lexer::TokenType::KEYW_AWAIT: {
            return ParseAwaitExpression();
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET: {
            return ParseArrayOrDestructuringExpression(CarryPatternFlags(flags));
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_BRACE: {
            return ParseObjectExpression(CarryPatternFlags(flags));
        }
        case lexer::TokenType::PUNCTUATOR_BACK_TICK: {
            return ParseTemplateLiteral();
        }
        case lexer::TokenType::KEYW_TYPE: {
            LogError(diagnostic::TYPE_ALIAS_ONLY_TOP_LEVEL);
            ParseTypeAliasDeclaration();  // Try to parse type alias and drop the result.
            return AllocBrokenExpression(Lexer()->GetToken().Loc());
        }
        case lexer::TokenType::KEYW_FUNCTION: {
            LogError(diagnostic::FUNC_EXPR);
            ParseFunctionDeclaration(true, ir::ModifierFlags::NONE);
            return AllocBrokenExpression(Lexer()->GetToken().Loc());
        }
        case lexer::TokenType::PUNCTUATOR_FORMAT: {
            return ParseExpressionFormatPlaceholder();
        }
        case lexer::TokenType::KEYW_TYPEOF: {
            return ParseUnaryOrPrefixUpdateExpression();
        }
        default: {
            return ParsePrimaryExpressionWithLiterals(flags);
        }
    }
}

bool IsPunctuartorSpecialCharacter(lexer::TokenType tokenType)
{
    switch (tokenType) {
        case lexer::TokenType::PUNCTUATOR_COLON:
        case lexer::TokenType::PUNCTUATOR_COMMA:
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET:
        case lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR:
            return true;
        default:
            return false;
    }
}

// This function was created to reduce the size of `EatArrowFunctionParams`.
bool TypedParser::IsValidTokenTypeOfArrowFunctionStart(lexer::TokenType tokenType)
{
    return (tokenType == lexer::TokenType::LITERAL_IDENT || IsPrimitiveType(tokenType) ||
            tokenType == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD || tokenType == lexer::TokenType::KEYW_THIS);
}

bool TypedParser::EatArrowFunctionParams(lexer::Lexer *lexer)
{
    ES2PANDA_ASSERT(lexer->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    lexer->NextToken();
    auto tokenType = lexer->GetToken().Type();

    size_t openBrackets = 1;
    bool expectIdentifier = true;
    while (tokenType != lexer::TokenType::EOS && openBrackets > 0) {
        lexer::NextTokenFlags flag = lexer::NextTokenFlags::NONE;
        switch (tokenType) {
            case lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS:
                --openBrackets;
                break;
            case lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS:
                ++openBrackets;
                break;
            case lexer::TokenType::PUNCTUATOR_COMMA:
                expectIdentifier = true;
                break;
            // (DZ) need for correct processing of possible number literals
            case lexer::TokenType::PUNCTUATOR_MINUS:
                flag = lexer::NextTokenFlags::UNARY_MINUS;
                break;
            case lexer::TokenType::PUNCTUATOR_SEMI_COLON:
            case lexer::TokenType::PUNCTUATOR_BACK_TICK:
                return false;
            default:
                if (!expectIdentifier) {
                    break;
                }
                if (!IsValidTokenTypeOfArrowFunctionStart(tokenType)) {
                    return false;
                }
                expectIdentifier = false;
        }
        lexer->NextToken(flag);
        tokenType = lexer->GetToken().Type();
    }
    return true;
}

bool ETSParser::IsArrowFunctionExpressionStart()
{
    auto finalizer = [this, savedPos = Lexer()->Save()]([[maybe_unused]] void *ptr) { Lexer()->Rewind(savedPos); };
    std::unique_ptr<void, decltype(finalizer)> defer(&finalizer, finalizer);
    if (!EatArrowFunctionParams(Lexer())) {
        return false;
    }

    if (Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_COLON)) {
        TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::RETURN_TYPE;
        if (ParseTypeAnnotation(&options) == nullptr) {
            return false;
        }
    }

    auto tokenType = Lexer()->GetToken().Type();
    while (tokenType != lexer::TokenType::EOS && tokenType != lexer::TokenType::PUNCTUATOR_ARROW) {
        if (lexer::Token::IsPunctuatorToken(tokenType) && !IsPunctuartorSpecialCharacter(tokenType)) {
            break;
        }
        Lexer()->NextToken();
        tokenType = Lexer()->GetToken().Type();
    }
    return Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_ARROW;
}

ir::ArrowFunctionExpression *ETSParser::ParseArrowFunctionExpression()
{
    auto newStatus = ParserStatus::ARROW_FUNCTION | ParserStatus::ALLOW_RECEIVER;
    auto *func = ParseFunction(newStatus);
    auto *arrowFuncNode = AllocNode<ir::ArrowFunctionExpression>(func, Allocator());
    ES2PANDA_ASSERT(arrowFuncNode != nullptr);
    arrowFuncNode->SetRange(func->Range());
    return arrowFuncNode;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ETSParser::ParseCoverParenthesizedExpressionAndArrowParameterList(ExpressionParseFlags flags)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    if (IsArrowFunctionExpressionStart()) {
        return ParseArrowFunctionExpression();
    }

    lexer::SourcePosition start = Lexer()->GetToken().Start();
    Lexer()->NextToken();

    ExpressionParseFlags newFlags = ExpressionParseFlags::ACCEPT_COMMA;
    if ((flags & ExpressionParseFlags::INSTANCEOF) != 0) {
        newFlags |= ExpressionParseFlags::INSTANCEOF;
    };

    ir::Expression *expr = ParseExpression(newFlags);

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA) {
        LogError(diagnostic::ERROR_ARKTS_NO_COMMA_OUTSIDE_LOOPS);
        auto sequenceExpression = ParseSequenceExpression(expr);
        Lexer()->NextToken();
        return AllocBrokenExpression(sequenceExpression->Range());
    }
    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);
    }

    expr->SetGrouped();
    expr->SetRange({start, Lexer()->GetToken().End()});
    Lexer()->NextToken();

    return expr;
}

std::optional<ir::Expression *> ETSParser::GetPostPrimaryExpression(ir::Expression *returnExpression,
                                                                    lexer::SourcePosition startLoc,
                                                                    bool ignoreCallExpression,
                                                                    [[maybe_unused]] bool *isChainExpression)
{
    auto periodPos = Lexer()->GetToken().End();
    switch (Lexer()->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_QUESTION_DOT: {
            if (*isChainExpression) {
                return std::nullopt;  // terminate current chain
            }
            *isChainExpression = true;
            Lexer()->NextToken();  // eat ?.

            if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
                return ParseElementAccess(returnExpression, true);
            }

            if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
                return ParseCallExpression(returnExpression, true, false);
            }

            return ParsePropertyAccess(returnExpression, periodPos, true);
        }
        case lexer::TokenType::PUNCTUATOR_PERIOD: {
            Lexer()->NextToken();  // eat period

            return ParsePropertyAccess(returnExpression, periodPos);
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET:
            return ParseElementAccess(returnExpression);
        case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
            if (ParsePotentialGenericFunctionCall(returnExpression, &returnExpression, startLoc,
                                                  ignoreCallExpression)) {
                return std::nullopt;
            }

            return returnExpression;
        case lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS:
            if (ignoreCallExpression) {
                return std::nullopt;
            }
            return ParseCallExpression(returnExpression, false, false);
        case lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK: {
            const bool shouldBreak = ParsePotentialNonNullExpression(&returnExpression, startLoc);
            if (shouldBreak) {
                return std::nullopt;
            }

            return returnExpression;
        }
        case lexer::TokenType::PUNCTUATOR_FORMAT:
        case lexer::TokenType::PUNCTUATOR_ARROW:
            LogUnexpectedToken(Lexer()->GetToken());
            [[fallthrough]];
        default:
            return std::nullopt;
    }
}

ir::Expression *ETSParser::ParsePostPrimaryExpression(ir::Expression *primaryExpr, lexer::SourcePosition startLoc,
                                                      bool ignoreCallExpression,
                                                      [[maybe_unused]] bool *isChainExpression)
{
    ir::Expression *returnExpression = primaryExpr;
    WhileLoopGuard guard;

    while (guard.ShouldContinue()) {
        auto expr = GetPostPrimaryExpression(returnExpression, startLoc, ignoreCallExpression, isChainExpression);
        if (expr.has_value()) {
            returnExpression = expr.value();
            continue;
        }

        break;
    }

    if (guard.IsLimitReached()) {
        return HandleDeepNesting();
    }

    return returnExpression;
}

ir::Expression *ETSParser::ParsePotentialAsExpression(ir::Expression *primaryExpr)
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::KEYW_AS);
    Lexer()->NextToken();
    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_CONST) {
        LogError(diagnostic::AS_CONST_USAGE);
        return nullptr;
    }
    TypeAnnotationParsingOptions options =
        TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::ANNOTATION_NOT_ALLOW;
    ir::TypeNode *type = ParseTypeAnnotation(&options);
    if (type == nullptr) {
        // Error processing
        // Failed to parse type annotation for AsExpression
        return nullptr;
    }

    auto *asExpression = AllocNode<ir::TSAsExpression>(primaryExpr, type, false);
    ES2PANDA_ASSERT(asExpression != nullptr);
    asExpression->SetStart(primaryExpr->Start());
    asExpression->SetEnd(type->End());
    return asExpression;
}

//  Extracted from 'ParseNewExpression()' to reduce function's size
void ETSParser::ParseArgumentsNewExpression(ArenaVector<ir::Expression *> &arguments, ir::TypeNode *typeReference)
{
    lexer::SourcePosition endLoc = typeReference->End();

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
        Lexer()->NextToken();

        ParseList(
            lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS, lexer::NextTokenFlags::NONE,
            [this, &arguments](bool &) {
                ir::Expression *argument =
                    Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD
                        ? ParseSpreadElement(ExpressionParseFlags::POTENTIALLY_IN_PATTERN)
                        : ParseExpression();

                if (argument == nullptr) {
                    return false;
                }
                arguments.push_back(argument);
                return true;
            },
            &endLoc, ParseListOptions::ALLOW_TRAILING_SEP);
    }
}

ir::Expression *ETSParser::ParseNewExpression()
{
    lexer::SourcePosition start = Lexer()->GetToken().Start();

    Lexer()->NextToken();  // eat new

    TypeAnnotationParsingOptions options =
        TypeAnnotationParsingOptions::REPORT_ERROR | TypeAnnotationParsingOptions::IGNORE_FUNCTION_TYPE |
        TypeAnnotationParsingOptions::ALLOW_WILDCARD | TypeAnnotationParsingOptions::POTENTIAL_NEW_ARRAY;
    auto typeReference = ParseTypeAnnotation(&options);
    ES2PANDA_ASSERT(typeReference != nullptr);

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
        Lexer()->NextToken();
        ir::Expression *dimension = ParseExpression();

        auto endLoc = Lexer()->GetToken().End();
        ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET);

        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
            auto *arrInstance = AllocNode<ir::ETSNewArrayInstanceExpression>(typeReference, dimension);
            arrInstance->SetRange({start, endLoc});
            return arrInstance;
        }

        ArenaVector<ir::Expression *> dimensions(Allocator()->Adapter());
        dimensions.push_back(dimension);

        do {
            Lexer()->NextToken();
            dimensions.push_back(ParseExpression());

            endLoc = Lexer()->GetToken().End();
            ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET);
        } while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET);

        auto *multiArray = AllocNode<ir::ETSNewMultiDimArrayInstanceExpression>(typeReference, std::move(dimensions));
        ES2PANDA_ASSERT(multiArray != nullptr);
        multiArray->SetRange({start, endLoc});
        return multiArray;
    }

    ArenaVector<ir::Expression *> arguments(Allocator()->Adapter());
    ParseArgumentsNewExpression(arguments, typeReference);

    auto *newExprNode = AllocNode<ir::ETSNewClassInstanceExpression>(typeReference, std::move(arguments));
    ES2PANDA_ASSERT(newExprNode != nullptr);
    newExprNode->SetRange({start, Lexer()->GetToken().End()});

    return newExprNode;
}

ir::Expression *ETSParser::ParseAsyncExpression()
{
    Lexer()->NextToken();  // eat 'async'
    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_FUNCTION) {
        LogError(diagnostic::FUNC_EXPR);
        ParseFunctionDeclaration(true, ir::ModifierFlags::NONE);
        return AllocBrokenExpression(Lexer()->GetToken().Loc());
    }
    if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS ||
        !IsArrowFunctionExpressionStart()) {
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    }

    auto newStatus = ParserStatus::NEED_RETURN_TYPE | ParserStatus::ARROW_FUNCTION | ParserStatus::ASYNC_FUNCTION;
    auto *func = ParseFunction(newStatus);
    if (func == nullptr) {  // Error processing.
        return nullptr;
    }
    auto *arrowFuncNode = AllocNode<ir::ArrowFunctionExpression>(func, Allocator());
    ES2PANDA_ASSERT(arrowFuncNode != nullptr);
    arrowFuncNode->SetRange(func->Range());
    return arrowFuncNode;
}

ir::Expression *ETSParser::ParseAwaitExpression()
{
    lexer::SourcePosition start = Lexer()->GetToken().Start();
    Lexer()->NextToken();
    ir::Expression *argument = ParseExpression();
    auto *awaitExpression = AllocNode<ir::AwaitExpression>(argument);
    ES2PANDA_ASSERT(awaitExpression != nullptr);
    awaitExpression->SetRange({start, Lexer()->GetToken().End()});
    return awaitExpression;
}

ir::Expression *ETSParser::ParseArrayOrDestructuringExpression(const ExpressionParseFlags flags)
{
    auto *parsedArrayOrDestructuring = ParserImpl::ParseArrayExpression(flags, true);

    if (parsedArrayOrDestructuring->IsArrayPattern()) {
        return AllocNode<ir::ETSDestructuring>(parsedArrayOrDestructuring->Elements());
    }

    auto arrayElements = parsedArrayOrDestructuring->Elements();
    if (std::any_of(arrayElements.begin(), arrayElements.end(),
                    [](const ir::AstNode *node) { return node->IsOmittedExpression(); })) {
        LogError(diagnostic::OMITTED_ELEMENTS_NOT_SUPPORTED_IN_ARRAYS, {}, parsedArrayOrDestructuring->Start());
        ArenaVector<ir::Expression *> elements(Allocator()->Adapter());
        std::copy_if(arrayElements.begin(), arrayElements.end(), std::back_inserter(elements),
                     [](const ir::AstNode *node) { return !node->IsOmittedExpression(); });
        auto *const newArrayNode = AllocNode<ir::ArrayExpression>(std::move(elements), Allocator());
        return newArrayNode;
    }

    return parsedArrayOrDestructuring;
}

ir::ArrayExpression *ETSParser::ParseArrayExpression(ExpressionParseFlags flags)
{
    return ParserImpl::ParseArrayExpression(flags, false);
}

ir::Expression *ETSParser::ParsePotentialExpressionSequence(ir::Expression *expr, ExpressionParseFlags flags)
{
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA &&
        (flags & ExpressionParseFlags::ACCEPT_COMMA) != 0 && (flags & ExpressionParseFlags::IN_FOR) != 0U) {
        return ParseSequenceExpression(expr, (flags & ExpressionParseFlags::ACCEPT_REST) != 0);
    }

    return expr;
}

bool ETSParser::ParsePotentialNonNullExpression(ir::Expression **expression, const lexer::SourcePosition startLoc)
{
    if (expression == nullptr || Lexer()->GetToken().NewLine()) {
        return true;
    }

    const auto nonNullExpr = AllocNode<ir::TSNonNullExpression>(*expression);
    ES2PANDA_ASSERT(nonNullExpr != nullptr);
    nonNullExpr->SetRange({startLoc, Lexer()->GetToken().End()});

    *expression = nonNullExpr;

    Lexer()->NextToken();

    return false;
}

void ETSParser::ValidateInstanceOfExpression(ir::Expression *expr)
{
    ValidateGroupedExpression(expr);
    lexer::TokenType tokenType = Lexer()->GetToken().Type();
    if (tokenType == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        auto options = TypeAnnotationParsingOptions::NO_OPTS;

        // Run checks to validate type declarations
        // Should provide helpful messages with incorrect declarations like the following:
        // `instanceof A<String;`
        ParseTypeParameterDeclaration(&options);

        // Display error message even when type declaration is correct
        // `instanceof A<String>;`
        LogError(diagnostic::INVALID_RIGHT_INSTANCEOF);
    }
}

ir::Expression *ETSParser::ParseGenericLambdaOrTypeAssertion()
{
    ES2PANDA_ASSERT(Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN);
    const auto savedPosition = Lexer()->Save();
    const auto start = Lexer()->GetToken().Start();
    const auto arrowStart = DiagnosticEngine().Save();
    if (ir::Expression *expr = ParseGenericArrowFunction(); expr != nullptr) {
        LogError(diagnostic::GENERIC_LAMBDA_NOT_SUPPORTED, util::DiagnosticMessageParams {}, start);
        return AllocBrokenExpression(expr->Range());
    }
    const auto afterArrow = DiagnosticEngine().Save();
    Lexer()->Rewind(savedPosition);
    if (const ir::Expression *typeAssertion = ParseTypeAssertion(); typeAssertion != nullptr) {
        DiagnosticEngine().UndoRange(arrowStart, afterArrow);
        LogError(diagnostic::TS_TYPE_ASSERTION, util::DiagnosticMessageParams {}, start);
        ES2PANDA_ASSERT(DiagnosticEngine().IsAnyError());
        return AllocBrokenExpression(typeAssertion->Range());
    }
    DiagnosticEngine().Rollback(afterArrow);
    ES2PANDA_ASSERT(DiagnosticEngine().IsAnyError());
    return AllocBrokenExpression(lexer::SourceRange {start, Lexer()->GetToken().End()});
}

void ETSParser::ValidateKeywordIn()
{
    if (Lexer()->GetToken().KeywordType() == lexer::TokenType::KEYW_IN) {
        LogError(diagnostic::ERROR_ARKTS_NO_IN_OPERATOR);
        Lexer()->NextToken();  // eat 'in'
        if (auto const tokenType = Lexer()->GetToken().Type(); tokenType == lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
            ParseObjectExpression();
        } else if (tokenType == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
            ParseArrayExpression(ExpressionParseFlags::NO_OPTS);
        } else {
            Lexer()->NextToken();
        }
    }
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ETSParser::ParseExpression(ExpressionParseFlags flags)
{
    TrackRecursive trackRecursive(RecursiveCtx());
    if (!trackRecursive) {
        return HandleDeepNesting();
    }
    ArenaVector<ir::AnnotationUsage *> annotations {Allocator()->Adapter()};
    if (Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_AT)) {
        annotations = ParseAnnotations(false);
    }
    const auto start = Lexer()->GetToken().Start();
    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_YIELD &&
        (flags & ExpressionParseFlags::DISALLOW_YIELD) == 0U) {
        ir::YieldExpression *yieldExpr = ParseYieldExpression();

        return ParsePotentialExpressionSequence(yieldExpr, flags);
    }
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        return ParseGenericLambdaOrTypeAssertion();
    }

    ir::Expression *unaryExpressionNode = ParseUnaryOrPrefixUpdateExpression(flags);
    ValidateKeywordIn();
    if ((flags & ExpressionParseFlags::INSTANCEOF) != 0) {
        ValidateInstanceOfExpression(unaryExpressionNode);
    }

    ir::Expression *assignmentExpression = ParseAssignmentExpression(unaryExpressionNode, flags);
    ApplyAnnotationsToNode(assignmentExpression, std::move(annotations), start);

    if (!Lexer()->GetToken().NewLine() &&
        (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA &&
         (flags & ExpressionParseFlags::ACCEPT_COMMA) != 0U && (flags & ExpressionParseFlags::IN_FOR) != 0U)) {
        return ParseSequenceExpression(assignmentExpression, (flags & ExpressionParseFlags::ACCEPT_REST) != 0U);
    }

    return assignmentExpression;
}

}  // namespace ark::es2panda::parser
