/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "formatting/rules.h"
#include <vector>
#include "formatting/formatting_context.h"
#include "generated/tokenType.h"
#include "ir/astNode.h"
#include "internal_api.h"

namespace ark::es2panda::lsp {

static bool IsOnSameLineContext(FormattingContext *ctx)
{
    return ctx->TokensAreOnSameLine();
}

static bool NodeIsTypeScriptDeclWithBlockContext(ir::AstNode *node)
{
    if (node == nullptr) {
        return false;
    }
    switch (node->Type()) {
        case ir::AstNodeType::CLASS_DECLARATION:
        case ir::AstNodeType::CLASS_EXPRESSION:
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
        case ir::AstNodeType::TS_ENUM_DECLARATION:
        case ir::AstNodeType::TS_TYPE_LITERAL:
        case ir::AstNodeType::TS_MODULE_DECLARATION:
        case ir::AstNodeType::EXPORT_NAMED_DECLARATION:
        case ir::AstNodeType::IMPORT_DECLARATION:
            return true;
        default:
            return false;
    }
}

static bool NodeIsBlockContext(ir::AstNode *node)
{
    if (node == nullptr) {
        return false;
    }
    if (NodeIsTypeScriptDeclWithBlockContext(node)) {
        return true;
    }
    switch (node->Type()) {
        case ir::AstNodeType::BLOCK_STATEMENT:
        case ir::AstNodeType::OBJECT_EXPRESSION:
        case ir::AstNodeType::TS_MODULE_BLOCK:
        case ir::AstNodeType::SWITCH_STATEMENT:
            return true;
        default:
            return false;
    }
}

static bool NodeIsInDecoratorContext(ir::AstNode *node)
{
    while (node != nullptr && node->IsExpression()) {
        node = node->Parent();
    }
    return node != nullptr && node->Type() == ir::AstNodeType::DECORATOR;
}

static bool IsBinaryOpContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    if (parent == nullptr) {
        return false;
    }
    const auto &currentToken = ctx->GetCurrentToken();
    const auto &nextToken = ctx->GetNextToken();

    switch (parent->Type()) {
        case ir::AstNodeType::BINARY_EXPRESSION:
            return parent->AsBinaryExpression()->OperatorType() != lexer::TokenType::PUNCTUATOR_COMMA;
        case ir::AstNodeType::CONDITIONAL_EXPRESSION:
        case ir::AstNodeType::TS_CONDITIONAL_TYPE:
        case ir::AstNodeType::TS_AS_EXPRESSION:
        case ir::AstNodeType::EXPORT_SPECIFIER:
        case ir::AstNodeType::IMPORT_SPECIFIER:
        case ir::AstNodeType::TS_TYPE_PREDICATE:
        case ir::AstNodeType::TS_UNION_TYPE:
        case ir::AstNodeType::ETS_UNION_TYPE:
        case ir::AstNodeType::TS_INTERSECTION_TYPE:
            return true;
        case ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION:
        case ir::AstNodeType::TS_IMPORT_EQUALS_DECLARATION:
        case ir::AstNodeType::VARIABLE_DECLARATION:
        case ir::AstNodeType::TS_PARAMETER_PROPERTY:
        case ir::AstNodeType::TS_ENUM_MEMBER:
        case ir::AstNodeType::CLASS_PROPERTY:
        case ir::AstNodeType::TS_PROPERTY_SIGNATURE:
            return currentToken.Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION ||
                   nextToken.Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION;
        case ir::AstNodeType::EXPORT_DEFAULT_DECLARATION:
            if (parent->AsExportDefaultDeclaration()->IsExportEquals()) {
                return currentToken.Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION ||
                       nextToken.Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION;
            }
            return false;
        case ir::AstNodeType::FOR_IN_STATEMENT:
        case ir::AstNodeType::TS_TYPE_PARAMETER:
            return currentToken.Type() == lexer::TokenType::KEYW_IN || nextToken.Type() == lexer::TokenType::KEYW_IN ||
                   currentToken.Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION ||
                   nextToken.Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION;
        case ir::AstNodeType::FOR_OF_STATEMENT:
            return currentToken.Type() == lexer::TokenType::KEYW_OF || nextToken.Type() == lexer::TokenType::KEYW_OF;
        default:
            return false;
    }
}

static bool IsNotBinaryOpContext(FormattingContext *ctx)
{
    return !IsBinaryOpContext(ctx);
}

static bool IsFunctionLikeDeclarationKind(ir::AstNodeType kind)
{
    switch (kind) {
        case ark::es2panda::ir::AstNodeType::FUNCTION_DECLARATION:
        case ark::es2panda::ir::AstNodeType::METHOD_DEFINITION:
        case ark::es2panda::ir::AstNodeType::TS_CONSTRUCTOR_TYPE:
        case ark::es2panda::ir::AstNodeType::FUNCTION_EXPRESSION:
        case ark::es2panda::ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
            return true;
        default:
            return false;
    }
}

static bool IsFunctionLikeKind(ir::AstNodeType kind)
{
    switch (kind) {
        case ark::es2panda::ir::AstNodeType::METHOD_DEFINITION:
        case ark::es2panda::ir::AstNodeType::CALL_EXPRESSION:
        case ark::es2panda::ir::AstNodeType::TS_CONSTRUCTOR_TYPE:
        case ark::es2panda::ir::AstNodeType::TS_INDEX_SIGNATURE:
        case ark::es2panda::ir::AstNodeType::FUNCTION_DECLARATION:
            return true;
        default:
            return IsFunctionLikeDeclarationKind(kind);
    }
}

static bool IsTypeAnnotationContext(FormattingContext *context)
{
    const auto contextKind = context->GetCurrentTokenParent()->Type();
    return contextKind == ir::AstNodeType::PROPERTY || contextKind == ir::AstNodeType::TS_PROPERTY_SIGNATURE ||
           contextKind == ir::AstNodeType::TS_TYPE_PARAMETER_DECLARATION ||
           contextKind == ir::AstNodeType::VARIABLE_DECLARATION || IsFunctionLikeKind(contextKind);
}

static bool IsNotTypeAnnotationContext(FormattingContext *ctx)
{
    return !IsTypeAnnotationContext(ctx);
}

static bool IsOptionalPropertyContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    if (parent == nullptr) {
        return false;
    }
    switch (parent->Type()) {
        case ir::AstNodeType::CLASS_PROPERTY:
        case ir::AstNodeType::TS_PROPERTY_SIGNATURE:
        case ir::AstNodeType::TS_PARAMETER_PROPERTY:
            return (parent->Modifiers() & ir::ModifierFlags::OPTIONAL) != 0;
        default:
            return false;
    }
}

static bool IsNonOptionalPropertyContext(FormattingContext *ctx)
{
    return !IsOptionalPropertyContext(ctx);
}

static bool IsConditionalOperatorContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    return parent != nullptr && (parent->Type() == ir::AstNodeType::CONDITIONAL_EXPRESSION ||
                                 parent->Type() == ir::AstNodeType::TS_CONDITIONAL_TYPE);
}

static bool IsImportTypeContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    return parent != nullptr && parent->Type() == ir::AstNodeType::TS_IMPORT_TYPE;
}

static bool IsNotPropertyAccessOnIntegerLiteral(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    if (parent == nullptr || parent->Type() != ir::AstNodeType::MEMBER_EXPRESSION) {
        return true;
    }
    auto *memberExpr = parent->AsMemberExpression();
    if (memberExpr == nullptr || memberExpr->Object() == nullptr || !memberExpr->Object()->IsNumberLiteral()) {
        return true;
    }
    auto *numLiteral = memberExpr->Object()->AsNumberLiteral();
    if (numLiteral == nullptr) {
        return true;
    }

    auto &numStr = numLiteral->Str();
    std::string str(numStr);
    return str.find('.') != std::string::npos;
}

static bool IsBlockContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    return NodeIsBlockContext(parent);
}

static bool IsMultilineBlockContext(FormattingContext *ctx)
{
    return IsBlockContext(ctx) && !ctx->ContextNodeBlockIsOnOneLine();
}

static bool IsAfterCodeBlockContext(FormattingContext *ctx)
{
    auto *currentParent = ctx->GetCurrentTokenParent();
    if (currentParent == nullptr) {
        return false;
    }
    switch (currentParent->Type()) {
        case ir::AstNodeType::CLASS_DECLARATION:
        case ir::AstNodeType::TS_MODULE_DECLARATION:
        case ir::AstNodeType::TS_ENUM_DECLARATION:
        case ir::AstNodeType::CATCH_CLAUSE:
        case ir::AstNodeType::TS_MODULE_BLOCK:
        case ir::AstNodeType::SWITCH_STATEMENT:
            return true;
        case ir::AstNodeType::BLOCK_STATEMENT: {
            auto *blockParent = currentParent->Parent();
            if (blockParent == nullptr || (blockParent->Type() != ir::AstNodeType::ARROW_FUNCTION_EXPRESSION &&
                                           blockParent->Type() != ir::AstNodeType::FUNCTION_EXPRESSION)) {
                return true;  // NOLINT(readability-simplify-boolean-expr)
            }
            return false;
        }
        default:
            return false;
    }
}

static bool IsControlDeclContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    if (parent == nullptr) {
        return false;
    }
    switch (parent->Type()) {
        case ir::AstNodeType::IF_STATEMENT:
        case ir::AstNodeType::SWITCH_STATEMENT:
        case ir::AstNodeType::FOR_UPDATE_STATEMENT:
        case ir::AstNodeType::FOR_IN_STATEMENT:
        case ir::AstNodeType::FOR_OF_STATEMENT:
        case ir::AstNodeType::WHILE_STATEMENT:
        case ir::AstNodeType::TRY_STATEMENT:
        case ir::AstNodeType::DO_WHILE_STATEMENT:
        case ir::AstNodeType::CATCH_CLAUSE:
            return true;
        default:
            return false;
    }
}

static bool IsObjectContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    return parent != nullptr && parent->Type() == ir::AstNodeType::OBJECT_EXPRESSION;
}

static bool IsFunctionDeclContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    if (parent == nullptr) {
        return false;
    }
    switch (parent->Type()) {
        case ir::AstNodeType::FUNCTION_DECLARATION:
        case ir::AstNodeType::FUNCTION_EXPRESSION:
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
        case ir::AstNodeType::TS_METHOD_SIGNATURE:
        case ir::AstNodeType::TS_SIGNATURE_DECLARATION:
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
        case ir::AstNodeType::TS_CONSTRUCTOR_TYPE:
        case ir::AstNodeType::METHOD_DEFINITION:
        case ir::AstNodeType::CLASS_PROPERTY:
            return true;
        default:
            return false;
    }
}

static bool IsNotFunctionDeclContext(FormattingContext *ctx)
{
    return !IsFunctionDeclContext(ctx);
}

static bool IsFunctionDeclarationOrFunctionExpressionContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    return parent != nullptr && (parent->Type() == ir::AstNodeType::FUNCTION_DECLARATION ||
                                 parent->Type() == ir::AstNodeType::FUNCTION_EXPRESSION);
}

static bool IsFunctionCallOrNewContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    if (parent == nullptr) {
        return false;
    }
    switch (parent->Type()) {
        case ir::AstNodeType::CALL_EXPRESSION:
        case ir::AstNodeType::NEW_EXPRESSION:
        case ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION:
        case ir::AstNodeType::ETS_NEW_ARRAY_INSTANCE_EXPRESSION:
        case ir::AstNodeType::ETS_NEW_MULTI_DIM_ARRAY_INSTANCE_EXPRESSION:
            return true;
        default:
            return false;
    }
}

static bool IsPreviousTokenNotComma(FormattingContext *ctx)
{
    return ctx->GetCurrentToken().Type() != lexer::TokenType::PUNCTUATOR_COMMA;
}

static bool IsArrowFunctionContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    return parent != nullptr && parent->Type() == ir::AstNodeType::ARROW_FUNCTION_EXPRESSION;
}

static bool IsStartOfVariableDeclarationList(FormattingContext *ctx)
{
    auto *currentParent = ctx->GetCurrentTokenParent();
    const auto &currentSpan = ctx->GetCurrentTokenSpan();
    if (currentParent == nullptr) {
        return false;
    }
    return currentParent->Type() == ir::AstNodeType::VARIABLE_DECLARATION &&
           currentParent->Start().index == currentSpan.start.index;
}

static bool IsModuleDeclContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    return parent != nullptr &&
           (parent->Type() == ir::AstNodeType::TS_MODULE_DECLARATION || parent->Type() == ir::AstNodeType::ETS_MODULE);
}

static bool IsObjectTypeContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    return parent != nullptr &&
           (parent->Type() == ir::AstNodeType::TS_TYPE_LITERAL || parent->Type() == ir::AstNodeType::TS_INTERFACE_BODY);
}

static bool IsConstructorSignatureContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    if (parent == nullptr) {
        return false;
    }
    if (parent->Type() == ir::AstNodeType::TS_SIGNATURE_DECLARATION) {
        auto *sigDecl = parent->AsTSSignatureDeclaration();
        return sigDecl != nullptr;
    }
    if (parent->Type() == ir::AstNodeType::METHOD_DEFINITION) {
        auto *methodDef = parent->AsMethodDefinition();
        return methodDef->IsConstructor();
    }
    return false;
}

static bool IsTypeArgumentOrParameterOrAssertion(const lexer::Token &token, ir::AstNode *parent)
{
    if (token.Type() != lexer::TokenType::PUNCTUATOR_LESS_THAN &&
        token.Type() != lexer::TokenType::PUNCTUATOR_GREATER_THAN) {
        return false;
    }
    if (parent == nullptr) {
        return false;
    }
    switch (parent->Type()) {
        case ir::AstNodeType::TS_TYPE_REFERENCE:
        case ir::AstNodeType::ETS_TYPE_REFERENCE:
        case ir::AstNodeType::TS_TYPE_ASSERTION:
        case ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION:
        case ir::AstNodeType::CLASS_DECLARATION:
        case ir::AstNodeType::CLASS_EXPRESSION:
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
        case ir::AstNodeType::FUNCTION_DECLARATION:
        case ir::AstNodeType::FUNCTION_EXPRESSION:
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
        case ir::AstNodeType::METHOD_DEFINITION:
        case ir::AstNodeType::TS_METHOD_SIGNATURE:
        case ir::AstNodeType::TS_SIGNATURE_DECLARATION:
        case ir::AstNodeType::CALL_EXPRESSION:
        case ir::AstNodeType::NEW_EXPRESSION:
        case ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION:
            return true;
        default:
            return false;
    }
}

static bool IsTypeArgumentOrParameterOrAssertionContext(FormattingContext *ctx)
{
    const auto &currentToken = ctx->GetCurrentToken();
    const auto &nextToken = ctx->GetNextToken();
    auto *currentParent = ctx->GetCurrentTokenParent();
    auto *nextParent = ctx->GetNextTokenParent();
    return IsTypeArgumentOrParameterOrAssertion(currentToken, currentParent) ||
           IsTypeArgumentOrParameterOrAssertion(nextToken, nextParent);
}

static bool IsTypeAssertionContext(FormattingContext *context)
{
    return context->GetCurrentTokenParent()->Type() == ir::AstNodeType::TS_TYPE_ASSERTION;
}
static bool IsNonTypeAssertionContext(FormattingContext *ctx)
{
    return !IsTypeAssertionContext(ctx);
}

static bool IsVoidOpContext(FormattingContext *ctx)
{
    const auto &currentToken = ctx->GetCurrentToken();
    auto *parent = ctx->GetCurrentTokenParent();
    return currentToken.Type() == lexer::TokenType::KEYW_VOID && parent != nullptr &&
           parent->Type() == ir::AstNodeType::UNARY_EXPRESSION;
}

static bool IsYieldOrYieldStarWithOperand(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    if (parent == nullptr || parent->Type() != ir::AstNodeType::YIELD_EXPRESSION) {
        return false;
    }
    auto *yieldExpr = parent->AsYieldExpression();
    return yieldExpr != nullptr && yieldExpr->Argument() != nullptr;
}

static bool IsEndOfDecoratorContextOnSameLine(FormattingContext *ctx)
{
    if (!ctx->TokensAreOnSameLine()) {
        return false;
    }
    auto *currentParent = ctx->GetCurrentTokenParent();
    auto *nextParent = ctx->GetNextTokenParent();
    return currentParent != nullptr && NodeIsInDecoratorContext(currentParent) &&
           (nextParent == nullptr || !NodeIsInDecoratorContext(nextParent));
}

static bool IsNonNullAssertionContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    return parent != nullptr && parent->Type() == ir::AstNodeType::TS_NON_NULL_EXPRESSION;
}

static bool IsStatementConditionContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    if (parent == nullptr) {
        return false;
    }
    switch (parent->Type()) {
        case ir::AstNodeType::IF_STATEMENT:
        case ir::AstNodeType::FOR_UPDATE_STATEMENT:
        case ir::AstNodeType::FOR_IN_STATEMENT:
        case ir::AstNodeType::FOR_OF_STATEMENT:
        case ir::AstNodeType::DO_WHILE_STATEMENT:
        case ir::AstNodeType::WHILE_STATEMENT:
            return true;
        default:
            return false;
    }
}

static bool IsNotStatementConditionContext(FormattingContext *ctx)
{
    return !IsStatementConditionContext(ctx);
}

static TokenRange CreateTokenRange(const std::vector<lexer::TokenType> &tokens)
{
    return TokenRange {TokenRange(const_cast<std::vector<lexer::TokenType> &>(tokens), true)};
}

static TokenRange CreateAnyTokenExcept(const std::vector<lexer::TokenType> &excludeTokens)
{
    std::vector<lexer::TokenType> allTokens;
    for (int token = static_cast<int>(lexer::TokenType::EOS); token <= static_cast<int>(lexer::TokenType::FIRST_KEYW);
         token++) {
        auto tokenType = static_cast<lexer::TokenType>(token);
        bool exclude = false;
        for (auto excludeToken : excludeTokens) {
            if (tokenType == excludeToken) {
                exclude = true;
                break;
            }
        }
        if (!exclude) {
            allTokens.push_back(tokenType);
        }
    }
    return CreateTokenRange(allTokens);
}

static void AddPunctuationRules(std::vector<RuleSpec> &rules)
{
    auto anyTokenRange = CreateTokenRange({});
    auto colon = CreateTokenRange({lexer::TokenType::PUNCTUATOR_COLON});
    auto question = CreateTokenRange({lexer::TokenType::PUNCTUATOR_QUESTION_MARK});
    auto dots = CreateTokenRange({lexer::TokenType::PUNCTUATOR_PERIOD, lexer::TokenType::PUNCTUATOR_QUESTION_DOT});
    auto ellipsis = CreateTokenRange({lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD});
    auto ident = CreateTokenRange({lexer::TokenType::LITERAL_IDENT});

    Rule rule1({IsOnSameLineContext, IsNotBinaryOpContext, IsNotTypeAnnotationContext}, RuleAction::DELETE_SPACE,
               RuleFlags::NONE);
    rules.emplace_back(rule1, anyTokenRange, colon);

    Rule rule2({IsOnSameLineContext, IsNotBinaryOpContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule2, colon, anyTokenRange);

    Rule rule3({IsOnSameLineContext, IsNotBinaryOpContext, IsNotTypeAnnotationContext}, RuleAction::DELETE_SPACE,
               RuleFlags::NONE);
    rules.emplace_back(rule3, anyTokenRange, question);

    Rule rule4({IsOnSameLineContext, IsConditionalOperatorContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule4, question, anyTokenRange);

    Rule rule5({IsOnSameLineContext, IsNonOptionalPropertyContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule5, question, anyTokenRange);

    Rule rule6({IsOnSameLineContext, IsNotPropertyAccessOnIntegerLiteral}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule6, anyTokenRange, dots);

    Rule rule7({IsOnSameLineContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule7, dots, anyTokenRange);

    Rule rule8({IsOnSameLineContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule8, ellipsis, ident);
}

static void AddUnaryOperatorRules(std::vector<RuleSpec> &rules)
{
    auto unaryPrefixOperators =
        CreateTokenRange({lexer::TokenType::PUNCTUATOR_PLUS_PLUS, lexer::TokenType::PUNCTUATOR_MINUS_MINUS,
                          lexer::TokenType::PUNCTUATOR_TILDE, lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK});
    auto unaryPrefixExpressions = CreateTokenRange(
        {lexer::TokenType::LITERAL_NUMBER, lexer::TokenType::LITERAL_IDENT,
         lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS, lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET,
         lexer::TokenType::PUNCTUATOR_LEFT_BRACE, lexer::TokenType::KEYW_THIS, lexer::TokenType::KEYW_NEW});
    auto unaryPreincrementExpressions =
        CreateTokenRange({lexer::TokenType::LITERAL_IDENT, lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS,
                          lexer::TokenType::KEYW_THIS, lexer::TokenType::KEYW_NEW});
    auto unaryPostincrementExpressions =
        CreateTokenRange({lexer::TokenType::LITERAL_IDENT, lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS,
                          lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET, lexer::TokenType::KEYW_NEW});
    auto plusPlus = CreateTokenRange({lexer::TokenType::PUNCTUATOR_PLUS_PLUS});
    auto minusMinus = CreateTokenRange({lexer::TokenType::PUNCTUATOR_MINUS_MINUS});
    auto plus = CreateTokenRange({lexer::TokenType::PUNCTUATOR_PLUS});
    auto minus = CreateTokenRange({lexer::TokenType::PUNCTUATOR_MINUS});

    Rule rule1({IsOnSameLineContext, IsNotBinaryOpContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule1, unaryPrefixOperators, unaryPrefixExpressions);

    Rule rule2({IsOnSameLineContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule2, plusPlus, unaryPreincrementExpressions);

    Rule rule3({IsOnSameLineContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule3, minusMinus, unaryPreincrementExpressions);

    Rule rule4({IsOnSameLineContext, IsNotStatementConditionContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule4, unaryPostincrementExpressions, plusPlus);

    Rule rule5({IsOnSameLineContext, IsNotStatementConditionContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule5, unaryPostincrementExpressions, minusMinus);

    Rule rule6({IsOnSameLineContext, IsBinaryOpContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule6, plusPlus, plus);

    Rule rule7({IsOnSameLineContext, IsBinaryOpContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule7, plus, plus);

    Rule rule8({IsOnSameLineContext, IsBinaryOpContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule8, plus, plusPlus);

    Rule rule9({IsOnSameLineContext, IsBinaryOpContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule9, minusMinus, minus);

    Rule rule10({IsOnSameLineContext, IsBinaryOpContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule10, minus, minus);

    Rule rule11({IsOnSameLineContext, IsBinaryOpContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule11, minus, minusMinus);
}

static void AddBraceRules(std::vector<RuleSpec> &rules)
{
    auto anyTokenRange = CreateTokenRange({});
    auto rightBrace = CreateTokenRange({lexer::TokenType::PUNCTUATOR_RIGHT_BRACE});
    auto leftBrace = CreateTokenRange({lexer::TokenType::PUNCTUATOR_LEFT_BRACE});
    auto commaColon = CreateTokenRange({lexer::TokenType::PUNCTUATOR_COMMA, lexer::TokenType::PUNCTUATOR_SEMI_COLON});
    auto elseKeyword = CreateTokenRange({lexer::TokenType::KEYW_ELSE});
    auto whileKeyword = CreateTokenRange({lexer::TokenType::KEYW_WHILE});

    Rule rule1({IsOnSameLineContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule1, rightBrace, commaColon);

    Rule rule2({IsMultilineBlockContext}, RuleAction::INSERT_NEWLINE, RuleFlags::NONE);
    rules.emplace_back(rule2, anyTokenRange, rightBrace);

    Rule rule3({IsOnSameLineContext, IsAfterCodeBlockContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    auto tokenExcept = CreateAnyTokenExcept({lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS});
    rules.emplace_back(rule3, rightBrace, tokenExcept);

    Rule rule4({IsOnSameLineContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule4, rightBrace, elseKeyword);

    Rule rule5({IsOnSameLineContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule5, rightBrace, whileKeyword);

    Rule rule6({IsOnSameLineContext, IsObjectContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule6, leftBrace, rightBrace);

    Rule rule7({IsMultilineBlockContext}, RuleAction::INSERT_NEWLINE, RuleFlags::NONE);
    rules.emplace_back(rule7, leftBrace, anyTokenRange);

    Rule rule8({IsOnSameLineContext, IsObjectTypeContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule8, leftBrace, rightBrace);
}

static void AddFunctionRules(std::vector<RuleSpec> &rules)
{
    auto anyTokenRange = CreateTokenRange({});
    auto functionKeyword = CreateTokenRange({lexer::TokenType::KEYW_FUNCTION});
    auto multiply = CreateTokenRange({lexer::TokenType::PUNCTUATOR_MULTIPLY});
    auto ident = CreateTokenRange({lexer::TokenType::LITERAL_IDENT});
    auto getSet = CreateTokenRange({lexer::TokenType::KEYW_GET, lexer::TokenType::KEYW_SET});
    auto rightParen = CreateTokenRange({lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS});
    auto leftBracket = CreateTokenRange({lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET});
    auto leftParen = CreateTokenRange({lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS});

    Rule rule1({IsFunctionDeclarationOrFunctionExpressionContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule1, functionKeyword, multiply);

    Rule rule2({IsFunctionDeclarationOrFunctionExpressionContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule2, multiply, ident);

    Rule rule3({IsFunctionDeclContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule3, functionKeyword, anyTokenRange);

    Rule rule4({IsFunctionDeclContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule4, getSet, ident);

    Rule rule5({IsControlDeclContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule5, rightParen, leftBracket);

    Rule rule6({IsOnSameLineContext, IsFunctionCallOrNewContext, IsPreviousTokenNotComma}, RuleAction::DELETE_SPACE,
               RuleFlags::NONE);
    rules.emplace_back(rule6, anyTokenRange, leftParen);
}

static void AddKeywordRules(std::vector<RuleSpec> &rules)
{
    auto anyTokenRange = CreateTokenRange({});
    auto binaryKeywordOperators =
        CreateTokenRange({lexer::TokenType::KEYW_IN, lexer::TokenType::KEYW_INSTANCEOF, lexer::TokenType::KEYW_OF,
                          lexer::TokenType::KEYW_AS, lexer::TokenType::KEYW_IS});
    auto spaceAfterKeywords =
        CreateTokenRange({lexer::TokenType::KEYW_VAR, lexer::TokenType::KEYW_THROW, lexer::TokenType::KEYW_NEW,
                          lexer::TokenType::KEYW_DELETE, lexer::TokenType::KEYW_RETURN, lexer::TokenType::KEYW_TYPEOF,
                          lexer::TokenType::KEYW_AWAIT});
    auto letConst = CreateTokenRange({lexer::TokenType::KEYW_LET, lexer::TokenType::KEYW_CONST});
    auto yield = CreateTokenRange({lexer::TokenType::KEYW_YIELD});
    auto multiply = CreateTokenRange({lexer::TokenType::PUNCTUATOR_MULTIPLY});
    auto yieldMultiply = CreateTokenRange({lexer::TokenType::KEYW_YIELD, lexer::TokenType::PUNCTUATOR_MULTIPLY});
    auto returnKeyword = CreateTokenRange({lexer::TokenType::KEYW_RETURN});
    auto semicolon = CreateTokenRange({lexer::TokenType::PUNCTUATOR_SEMI_COLON});
    auto voidKeyword = CreateTokenRange({lexer::TokenType::KEYW_VOID});
    auto import = CreateTokenRange({lexer::TokenType::KEYW_IMPORT});
    auto leftParen = CreateTokenRange({lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS});

    Rule rule1({IsOnSameLineContext, IsBinaryOpContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule1, anyTokenRange, binaryKeywordOperators);

    Rule rule2({IsOnSameLineContext, IsBinaryOpContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule2, binaryKeywordOperators, anyTokenRange);

    Rule rule3({IsOnSameLineContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule3, spaceAfterKeywords, anyTokenRange);

    Rule rule4({IsOnSameLineContext, IsStartOfVariableDeclarationList}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule4, letConst, anyTokenRange);

    Rule rule5({IsOnSameLineContext, IsYieldOrYieldStarWithOperand}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule5, yield, multiply);

    Rule rule6({IsOnSameLineContext, IsYieldOrYieldStarWithOperand}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule6, yieldMultiply, anyTokenRange);

    Rule rule7({IsOnSameLineContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule7, returnKeyword, semicolon);

    Rule rule8({IsOnSameLineContext, IsVoidOpContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule8, voidKeyword, anyTokenRange);

    Rule rule9({IsOnSameLineContext, IsImportTypeContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule9, import, leftParen);
}

static void AddTypeScriptRules(std::vector<RuleSpec> &rules)
{
    auto anyTokenRange = CreateTokenRange({});
    auto typeScriptKeywords = CreateTokenRange(
        {lexer::TokenType::KEYW_ABSTRACT,  lexer::TokenType::KEYW_CLASS,     lexer::TokenType::KEYW_DECLARE,
         lexer::TokenType::KEYW_DEFAULT,   lexer::TokenType::KEYW_ENUM,      lexer::TokenType::KEYW_EXPORT,
         lexer::TokenType::KEYW_EXTENDS,   lexer::TokenType::KEYW_GET,       lexer::TokenType::KEYW_IMPLEMENTS,
         lexer::TokenType::KEYW_IMPORT,    lexer::TokenType::KEYW_INTERFACE, lexer::TokenType::KEYW_MODULE,
         lexer::TokenType::KEYW_NAMESPACE, lexer::TokenType::KEYW_PRIVATE,   lexer::TokenType::KEYW_PUBLIC,
         lexer::TokenType::KEYW_PROTECTED, lexer::TokenType::KEYW_READONLY,  lexer::TokenType::KEYW_SET,
         lexer::TokenType::KEYW_STATIC,    lexer::TokenType::KEYW_TYPE,      lexer::TokenType::KEYW_FROM,
         lexer::TokenType::KEYW_KEYOF,     lexer::TokenType::KEYW_INFER});
    auto extendsImplementsFromKeywords = CreateTokenRange(
        {lexer::TokenType::KEYW_EXTENDS, lexer::TokenType::KEYW_IMPLEMENTS, lexer::TokenType::KEYW_FROM});
    auto moduleRequire = CreateTokenRange({lexer::TokenType::KEYW_MODULE, lexer::TokenType::KEYW_REQUIRE});
    auto stringLiteral = CreateTokenRange({lexer::TokenType::LITERAL_STRING});
    auto leftBrace = CreateTokenRange({lexer::TokenType::PUNCTUATOR_LEFT_BRACE});
    auto leftParen = CreateTokenRange({lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS});
    auto async = CreateTokenRange({lexer::TokenType::KEYW_ASYNC});
    auto functionIdent = CreateTokenRange({lexer::TokenType::KEYW_FUNCTION, lexer::TokenType::LITERAL_IDENT});

    Rule rule1({IsOnSameLineContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule1, typeScriptKeywords, anyTokenRange);

    Rule rule2({IsOnSameLineContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule2, anyTokenRange, extendsImplementsFromKeywords);

    Rule rule3({IsOnSameLineContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule3, moduleRequire, leftParen);

    Rule rule4({IsModuleDeclContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule4, stringLiteral, leftBrace);

    Rule rule5({IsArrowFunctionContext, IsOnSameLineContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule5, async, leftParen);

    Rule rule6({IsOnSameLineContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule6, async, functionIdent);
}

static void AddArrowAndGenericRules(std::vector<RuleSpec> &rules)
{
    auto anyTokenRange = CreateTokenRange({});
    auto arrow = CreateTokenRange({lexer::TokenType::PUNCTUATOR_ARROW});
    auto typeNames =
        CreateTokenRange({lexer::TokenType::LITERAL_IDENT, lexer::TokenType::KEYW_STRING, lexer::TokenType::KEYW_NUMBER,
                          lexer::TokenType::KEYW_BOOLEAN, lexer::TokenType::KEYW_OBJECT, lexer::TokenType::KEYW_ANY});
    auto lessThan = CreateTokenRange({lexer::TokenType::PUNCTUATOR_LESS_THAN});
    auto greaterThan = CreateTokenRange({lexer::TokenType::PUNCTUATOR_GREATER_THAN});
    auto rightParen = CreateTokenRange({lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS});
    auto parenBracketComma = CreateTokenRange(
        {lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS, lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET,
         lexer::TokenType::PUNCTUATOR_GREATER_THAN, lexer::TokenType::PUNCTUATOR_COMMA});
    auto question = CreateTokenRange({lexer::TokenType::PUNCTUATOR_QUESTION_MARK});
    auto rightParenComma =
        CreateTokenRange({lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS, lexer::TokenType::PUNCTUATOR_COMMA});

    Rule rule1({IsOnSameLineContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule1, anyTokenRange, arrow);

    Rule rule2({IsOnSameLineContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule2, arrow, anyTokenRange);

    Rule rule3({IsOnSameLineContext, IsTypeArgumentOrParameterOrAssertionContext}, RuleAction::DELETE_SPACE,
               RuleFlags::NONE);
    rules.emplace_back(rule3, typeNames, lessThan);

    Rule rule4({IsOnSameLineContext, IsTypeArgumentOrParameterOrAssertionContext}, RuleAction::DELETE_SPACE,
               RuleFlags::NONE);
    rules.emplace_back(rule4, rightParen, lessThan);

    Rule rule5({IsOnSameLineContext, IsTypeArgumentOrParameterOrAssertionContext}, RuleAction::DELETE_SPACE,
               RuleFlags::NONE);
    rules.emplace_back(rule5, lessThan, anyTokenRange);

    Rule rule6({IsOnSameLineContext, IsTypeArgumentOrParameterOrAssertionContext}, RuleAction::DELETE_SPACE,
               RuleFlags::NONE);
    rules.emplace_back(rule6, anyTokenRange, greaterThan);

    Rule rule7({IsOnSameLineContext, IsTypeArgumentOrParameterOrAssertionContext, IsNotFunctionDeclContext,
                IsNonTypeAssertionContext},
               RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule7, greaterThan, parenBracketComma);

    Rule rule8({IsOnSameLineContext, IsNotBinaryOpContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule8, question, rightParenComma);

    Rule rule9({IsOnSameLineContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule9, lessThan, lessThan);
}

static void AddDecoratorRules(std::vector<RuleSpec> &rules)
{
    auto anyTokenRange = CreateTokenRange({});
    auto rightParenIdent =
        CreateTokenRange({lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS, lexer::TokenType::LITERAL_IDENT});
    auto at = CreateTokenRange({lexer::TokenType::PUNCTUATOR_AT});
    auto afterDecoratorTokens =
        CreateTokenRange({lexer::TokenType::KEYW_ABSTRACT, lexer::TokenType::LITERAL_IDENT,
                          lexer::TokenType::KEYW_EXPORT, lexer::TokenType::KEYW_DEFAULT, lexer::TokenType::KEYW_CLASS,
                          lexer::TokenType::KEYW_STATIC, lexer::TokenType::KEYW_PUBLIC, lexer::TokenType::KEYW_PRIVATE,
                          lexer::TokenType::KEYW_PROTECTED, lexer::TokenType::KEYW_GET, lexer::TokenType::KEYW_SET,
                          lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET, lexer::TokenType::PUNCTUATOR_MULTIPLY});
    auto exclamation = CreateTokenRange({lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK});
    auto newKeyword = CreateTokenRange({lexer::TokenType::KEYW_NEW});
    auto leftParen = CreateTokenRange({lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS});

    Rule rule1({IsOnSameLineContext}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule1, rightParenIdent, at);

    Rule rule2({IsOnSameLineContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule2, at, anyTokenRange);

    Rule rule3({IsEndOfDecoratorContextOnSameLine}, RuleAction::INSERT_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule3, anyTokenRange, afterDecoratorTokens);

    Rule rule4({IsOnSameLineContext, IsNonNullAssertionContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule4, anyTokenRange, exclamation);

    Rule rule5({IsOnSameLineContext, IsConstructorSignatureContext}, RuleAction::DELETE_SPACE, RuleFlags::NONE);
    rules.emplace_back(rule5, newKeyword, leftParen);
}

static bool IsTypeScriptDeclWithBlockContext(FormattingContext *context)
{
    return NodeIsTypeScriptDeclWithBlockContext(context->GetCurrentTokenParent());
}

static bool IsBeforeBlockContext(FormattingContext *context)
{
    return NodeIsBlockContext(context->GetCurrentTokenParent());
}

static bool IsSameLineTokenOrBeforeBlockContext(FormattingContext *context)
{
    return context->TokensAreOnSameLine() || IsBeforeBlockContext(context);
}

static bool IsForContext(FormattingContext *context)
{
    return context->GetNextTokenParent()->Type() == ir::AstNodeType::FOR_IN_STATEMENT;
}

// This check is done before an open brace in a control construct, a function, or a typescript block declaration
static bool IsBeforeMultilineBlockContext(FormattingContext *context)
{
    return IsBeforeBlockContext(context) && !(context->TokensAreOnSameLine() || context->ContextNodeBlockIsOnOneLine());
}

std::function<bool(const FormattingContext &)> IsOptionDisabledOrUndefinedOrTokensOnSameLine(
    bool (FormatCodeSettings::*getter)() const)
{
    return [getter](const FormattingContext &context) {
        if (!(context.GetOptions().*getter)()) {
            return true;
        }

        // token'lar aynı satırda ise
        return context.TokensAreOnSameLine();
    };
}

std::vector<lexer::TokenType> GetAllTokenTypes()
{
    std::vector<lexer::TokenType> result;
    for (int i = static_cast<int>(lexer::TokenType::EOS); i < static_cast<int>(lexer::TokenType::FIRST_KEYW); ++i) {
        result.push_back(static_cast<lexer::TokenType>(i));
    }
    return result;
}

static bool IsNextTokenNotCloseBracket(FormattingContext *context)
{
    return context->GetCurrentToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE;
}

static bool IsNextTokenNotCloseParen(FormattingContext *context)
{
    return context->GetCurrentToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS;
}

const TokenRange &GetBinaryOperators()
{
    static const TokenRange BINARY_OPERATORS = [] {
        static const std::vector<lexer::TokenType> COMPARISON_OPERATORS = {
            lexer::TokenType::PUNCTUATOR_LESS_THAN,       lexer::TokenType::PUNCTUATOR_GREATER_THAN,
            lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL, lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL,
            lexer::TokenType::PUNCTUATOR_EQUAL,           lexer::TokenType::PUNCTUATOR_NOT_EQUAL};

        return TokenRange {COMPARISON_OPERATORS, true};
    }();
    return BINARY_OPERATORS;
}

// Place a space before open brace in a control flow construct
const TokenRange &GetControlOpenBraceLeftTokenRange()
{
    static const TokenRange CONTROL_OPEN_BRACE_LEFT_TOKEN_RANGE {
        std::vector<lexer::TokenType> {lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS, lexer::TokenType::KEYW_DO,
                                       lexer::TokenType::KEYW_TRY, lexer::TokenType::KEYW_FINALLY,
                                       lexer::TokenType::KEYW_ELSE},
        true};
    return CONTROL_OPEN_BRACE_LEFT_TOKEN_RANGE;
}

const TokenRange &GetAnyTokenIncludingMultilineComments()
{
    static const TokenRange RANGE {GetAllTokenTypes(),  // vector<lexer::TokenType>
                                   true};
    return RANGE;
}

const TokenRange &GetKeywordsTokenRange()
{
    static const TokenRange RANGE {{lexer::TokenType::FIRST_KEYW, lexer::TokenType::KEYW_OF}, true};
    return RANGE;
}

const TokenRange &GetTypeScriptOpenBraceLeftTokenRange()
{
    static const TokenRange RANGE {{lexer::TokenType::LITERAL_IDENT, lexer::TokenType::PUNCTUATOR_LEFT_BRACE,
                                    lexer::TokenType::KEYW_CLASS, lexer::TokenType::KEYW_STRUCT,
                                    lexer::TokenType::KEYW_EXPORT, lexer::TokenType::KEYW_IMPORT},
                                   true};
    return RANGE;
}

const TokenRange &GetFunctionOpenBraceLeftTokenRange()
{
    return GetAnyTokenIncludingMultilineComments();
}

const TokenRange &GetAnyTokenRange()
{
    static const TokenRange RANGE {{},  // empty token list
                                   true};
    return RANGE;
}

struct UserConfigRule {
    TokenRange left;
    TokenRange right;
    Rule rule;
};

static void AddUserConfigRules(std::vector<RuleSpec> &rules, const std::vector<UserConfigRule> &configRules)
{
    for (auto &cfg : configRules) {
        rules.emplace_back(cfg.rule, cfg.left, cfg.right);
    }
}
// === Part 1: Constructor & Comma rules ===
void AddConstructorAndCommaRules(std::vector<UserConfigRule> &configRules, const TokenRange &leftConstructor,
                                 const TokenRange &rightParen, const TokenRange &leftComma, const TokenRange &any)
{
    configRules.insert(
        configRules.end(),
        {{leftConstructor, rightParen,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceAfterConstructor(); }},
               RuleAction::INSERT_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {leftConstructor, rightParen,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceAfterConstructor(); }},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {leftComma, any,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceAfterConstructor(); }},
               RuleAction::INSERT_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {leftComma, any,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceAfterCommaDelimiter(); },
                IsNextTokenNotCloseBracket, IsNextTokenNotCloseParen},
               RuleAction::INSERT_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {leftComma, any,
          Rule({[](FormattingContext *c) { return !c->GetOptions().GetInsertSpaceAfterCommaDelimiter(); }},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)}});
}

// === Part 2: Parentheses rules ===
void AddParenthesesRules(std::vector<UserConfigRule> &configRules, const TokenRange &leftParen,
                         const TokenRange &rightParenTok, const TokenRange &any)
{
    configRules.insert(
        configRules.end(),
        {{any, rightParenTok,
          Rule({[](FormattingContext *c) {
                    return !c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis();
                },
                IsControlDeclContext},
               RuleAction::INSERT_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {leftParen, any,
          Rule({[](FormattingContext *c) {
                   return !c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis();
               }},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {any, rightParenTok,
          Rule({[](FormattingContext *c) {
                    return !c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis();
                },
                IsControlDeclContext},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)}});
}

// === Part 3: Brackets rules ===
void AddBracketRules(std::vector<UserConfigRule> &configRules, const TokenRange &leftBracket,
                     const TokenRange &rightBracket, const TokenRange &any)
{
    configRules.insert(configRules.end(),
                       {{leftBracket, any,
                         Rule({[](FormattingContext *c) {
                                  return c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets();
                              }},
                              RuleAction::INSERT_SPACE, RuleFlags::NONE)},

                        {any, rightBracket,
                         Rule({[](FormattingContext *c) {
                                   return c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets();
                               },
                               IsControlDeclContext},
                              RuleAction::INSERT_SPACE, RuleFlags::NONE)},

                        {leftBracket, any,
                         Rule({[](FormattingContext *c) {
                                  return !c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets();
                              }},
                              RuleAction::DELETE_SPACE, RuleFlags::NONE)},

                        {any, rightBracket,
                         Rule({[](FormattingContext *c) {
                                   return !c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets();
                               },
                               IsControlDeclContext},
                              RuleAction::DELETE_SPACE, RuleFlags::NONE)}});
}

void AddFunctionAndControlFlowRules(std::vector<UserConfigRule> &configRules, const TokenRange &any,
                                    const TokenRange &leftParen, const TokenRange &rightParenTok,
                                    const TokenRange &functionTokenRange)
{
    configRules.insert(
        configRules.end(),
        {{functionTokenRange, rightParenTok,
          Rule({[](FormattingContext *c) {
                    return !c->GetOptions().GetInsertSpaceAfterFunctionKeywordForAnonymousFunctions();
                },
                IsFunctionDeclContext},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {GetKeywordsTokenRange(), rightParenTok,
          Rule({[](FormattingContext *c) {
                    return c->GetOptions().GetInsertSpaceAfterKeywordsInControlFlowStatements();
                },
                IsControlDeclContext},
               RuleAction::INSERT_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {GetKeywordsTokenRange(), rightParenTok,
          Rule({[](FormattingContext *c) {
                    return !c->GetOptions().GetInsertSpaceAfterKeywordsInControlFlowStatements();
                },
                IsControlDeclContext},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {leftParen, any,
          Rule({[](FormattingContext *c) {
                   return c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis();
               }},
               RuleAction::INSERT_SPACE, RuleFlags::NONE)}});
}

void AddBraceRules(std::vector<UserConfigRule> &configRules, const TokenRange &leftBrace, const TokenRange &rightBrace,
                   const TokenRange &any)
{
    configRules.insert(configRules.end(),
                       {{leftBrace, rightBrace,
                         Rule({[](FormattingContext *c) {
                                   return !c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces();
                               },
                               IsObjectContext},
                              RuleAction::DELETE_SPACE, RuleFlags::NONE)},

                        {leftBrace, any,
                         Rule({[](FormattingContext *c) {
                                  return !c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces();
                              }},
                              RuleAction::DELETE_SPACE, RuleFlags::NONE)},

                        {any, rightBrace,
                         Rule({[](FormattingContext *c) {
                                  return !c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces();
                              }},
                              RuleAction::DELETE_SPACE, RuleFlags::NONE)},

                        {leftBrace, rightBrace,
                         Rule({[](FormattingContext *c) {
                                  return !c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces();
                              }},
                              RuleAction::INSERT_SPACE, RuleFlags::NONE)}});
}

void AddSemicolonAndBinaryOpRules(std::vector<UserConfigRule> &configRules, const TokenRange &any)
{
    configRules.insert(
        configRules.end(),
        {{CreateTokenRange({lexer::TokenType::PUNCTUATOR_SEMI_COLON}), any,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceAfterSemicolonInForStatements(); },
                IsForContext},
               RuleAction::INSERT_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {CreateTokenRange({lexer::TokenType::PUNCTUATOR_SEMI_COLON}), any,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceAfterSemicolonInForStatements(); },
                IsForContext},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {GetBinaryOperators(), any,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceBeforeAndAfterBinaryOperators(); },
                IsBinaryOpContext},
               RuleAction::INSERT_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {any, GetBinaryOperators(),
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceBeforeAndAfterBinaryOperators(); },
                IsBinaryOpContext},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)}});
}
void AddFunctionParensAndControlBlocksRules(std::vector<UserConfigRule> &configRules, const TokenRange &any,
                                            const TokenRange &rightParenTok, const TokenRange &leftBrace)
{
    configRules.insert(
        configRules.end(),
        {{any, rightParenTok,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceBeforeFunctionParenthesis(); },
                IsFunctionDeclContext},
               RuleAction::INSERT_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {any, rightParenTok,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceBeforeFunctionParenthesis(); },
                IsFunctionDeclContext},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {GetControlOpenBraceLeftTokenRange(), leftBrace,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetPlaceOpenBraceOnNewLineForControlBlocks(); },
                IsControlDeclContext, IsBeforeMultilineBlockContext},
               RuleAction::INSERT_NEWLINE, RuleFlags::CAN_DELETE_NEWLINES)},
         // CC-OFFNXT(G.FMT.02) project code style
         {GetFunctionOpenBraceLeftTokenRange(), leftBrace,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetPlaceOpenBraceOnNewLineForFunctions(); },
                IsControlDeclContext, IsBeforeMultilineBlockContext},
               RuleAction::INSERT_NEWLINE, RuleFlags::CAN_DELETE_NEWLINES)}});
}

void AddTypeAssertionsAndAnnotationsRules(std::vector<UserConfigRule> &configRules, const TokenRange &greaterThan,
                                          const TokenRange &colonOrQM, const TokenRange &semiOrColon,
                                          const TokenRange &any)
{
    auto allList = GetAllTokenTypes();
    allList.push_back(lexer::TokenType::EOS);
    auto anyWithEOF = CreateTokenRange(allList);
    configRules.insert(
        configRules.end(),
        {{greaterThan, any,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceAfterTypeAssertion(); },
                IsTypeAssertionContext},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {any, colonOrQM,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceBeforeTypeAnnotation(); },
                IsTypeAnnotationContext},
               RuleAction::INSERT_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {colonOrQM, any,
          Rule({[](FormattingContext *c) { return !c->GetOptions().GetInsertSpaceBeforeTypeAnnotation(); },
                IsTypeAnnotationContext},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {semiOrColon, anyWithEOF,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetSemicolons() == SemicolonPreference::REMOVE; },
                IsTypeAnnotationContext},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)}});
}

void AddRemainingBraceAndBinaryOpsRules(std::vector<UserConfigRule> &configRules, const TokenRange &leftBrace,
                                        const TokenRange &rightParenTok, const TokenRange &greaterThan,
                                        const TokenRange &any)
{
    configRules.insert(
        configRules.end(),
        {{GetControlOpenBraceLeftTokenRange(), leftBrace,
          Rule({[](FormattingContext *c) {
                    return c->GetOptions().GetPlaceOpenBraceOnNewLineForControlBlocks() || c->TokensAreOnSameLine();
                },
                IsSameLineTokenOrBeforeBlockContext},
               RuleAction::INSERT_SPACE, RuleFlags::CAN_DELETE_NEWLINES)},
         // CC-OFFNXT(G.FMT.02) project code style
         {GetTypeScriptOpenBraceLeftTokenRange(), leftBrace,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetPlaceOpenBraceOnNewLineForFunctions(); },
                IsTypeScriptDeclWithBlockContext, IsBeforeMultilineBlockContext},
               RuleAction::INSERT_NEWLINE, RuleFlags::CAN_DELETE_NEWLINES)},
         // CC-OFFNXT(G.FMT.02) project code style
         {CreateTokenRange({lexer::TokenType::KEYW_FUNCTION, lexer::TokenType::PUNCTUATOR_MULTIPLY}), rightParenTok,
          Rule({[](FormattingContext *c) {
                    return c->GetOptions().GetInsertSpaceAfterFunctionKeywordForAnonymousFunctions();
                },
                IsFunctionDeclContext},
               RuleAction::INSERT_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {GetBinaryOperators(), any,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceBeforeAndAfterBinaryOperators(); },
                IsBinaryOpContext},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {greaterThan, any,
          Rule({[](FormattingContext *c) { return c->GetOptions().GetInsertSpaceAfterTypeAssertion(); },
                IsTypeAssertionContext},
               RuleAction::INSERT_NEWLINE, RuleFlags::NONE)},
         // CC-OFFNXT(G.FMT.02) project code style
         {leftBrace, leftBrace,
          Rule({[](FormattingContext *c) {
                   return !c->GetOptions().GetInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces();
               }},
               RuleAction::DELETE_SPACE, RuleFlags::NONE)}});
}

void GetUserConfigRules(std::vector<RuleSpec> &rules,
                        const std::function<TokenRange(const std::vector<lexer::TokenType> &)> &createTokenRange)
{
    auto leftConstructor = createTokenRange({lexer::TokenType::KEYW_CONSTRUCTOR});
    auto rightParen = createTokenRange({lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS});
    auto leftComma = createTokenRange({lexer::TokenType::PUNCTUATOR_COMMA});
    auto &any = GetAnyTokenRange();

    auto leftParen = createTokenRange({lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS});
    auto rightParenTok = createTokenRange({lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS});
    auto leftBracket = createTokenRange({lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET});
    auto rightBracket = createTokenRange({lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET});
    auto leftBrace = createTokenRange({lexer::TokenType::PUNCTUATOR_LEFT_BRACE});
    auto rightBrace = createTokenRange({lexer::TokenType::PUNCTUATOR_RIGHT_BRACE});

    auto greaterThan = createTokenRange({lexer::TokenType::PUNCTUATOR_GREATER_THAN});
    auto colonOrQM = createTokenRange({lexer::TokenType::PUNCTUATOR_QUESTION_MARK, lexer::TokenType::PUNCTUATOR_COLON});
    auto semiOrColon = createTokenRange({lexer::TokenType::PUNCTUATOR_SEMI_COLON, lexer::TokenType::PUNCTUATOR_COLON});

    std::vector<UserConfigRule> configRules;

    AddConstructorAndCommaRules(configRules, leftConstructor, rightParen, leftComma, any);
    AddParenthesesRules(configRules, leftParen, rightParenTok, any);
    AddBracketRules(configRules, leftBracket, rightBracket, any);
    AddFunctionAndControlFlowRules(
        configRules, any, leftParen, rightParenTok,
        createTokenRange({lexer::TokenType::KEYW_FUNCTION, lexer::TokenType::PUNCTUATOR_MULTIPLY}));
    AddBraceRules(configRules, leftBrace, rightBrace, any);
    AddSemicolonAndBinaryOpRules(configRules, any);
    AddFunctionParensAndControlBlocksRules(configRules, any, rightParenTok, leftBrace);
    AddTypeAssertionsAndAnnotationsRules(configRules, greaterThan, colonOrQM, semiOrColon, any);
    AddRemainingBraceAndBinaryOpsRules(configRules, leftBrace, rightParenTok, greaterThan, any);

    AddUserConfigRules(rules, configRules);
}

std::vector<RuleSpec> GetAllRules()
{
    std::vector<RuleSpec> rules;

    auto createTokenRange = [](const std::vector<lexer::TokenType> &tokens) {
        return TokenRange {TokenRange(const_cast<std::vector<lexer::TokenType> &>(tokens), true)};
    };

    {
        std::vector<ContextPredicate> p = {[](FormattingContext *ctx) { return ctx->TokensAreOnSameLine(); },
                                           IsConditionalOperatorContext};
        Rule rule(p, RuleAction::INSERT_SPACE, RuleFlags::NONE);
        auto left = createTokenRange({lexer::TokenType::PUNCTUATOR_QUESTION_MARK});
        auto right = GetAnyTokenRange();
        rules.emplace_back(rule, left, right);
    }

    AddBraceRules(rules);
    AddArrowAndGenericRules(rules);
    AddDecoratorRules(rules);
    AddFunctionRules(rules);
    AddKeywordRules(rules);
    AddTypeScriptRules(rules);
    AddUnaryOperatorRules(rules);
    AddPunctuationRules(rules);

    GetUserConfigRules(rules, createTokenRange);

    return rules;
}

}  // namespace ark::es2panda::lsp