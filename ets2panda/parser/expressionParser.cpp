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

#include "generated/tokenType.h"
#include "parser/parserFlags.h"
#include "ir/astNode.h"
#include "ir/base/classDefinition.h"
#include "ir/base/decorator.h"
#include "ir/base/metaProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/property.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/spreadElement.h"
#include "ir/base/templateElement.h"
#include "ir/expression.h"
#include "ir/expressions/arrayExpression.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/awaitExpression.h"
#include "ir/expressions/binaryExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/chainExpression.h"
#include "ir/expressions/classExpression.h"
#include "ir/expressions/conditionalExpression.h"
#include "ir/expressions/directEvalExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/importExpression.h"
#include "ir/expressions/literals/bigIntLiteral.h"
#include "ir/expressions/literals/booleanLiteral.h"
#include "ir/expressions/literals/nullLiteral.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/expressions/literals/regExpLiteral.h"
#include "ir/expressions/literals/charLiteral.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/expressions/literals/undefinedLiteral.h"
#include "ir/expressions/memberExpression.h"
#include "ir/expressions/newExpression.h"
#include "ir/expressions/objectExpression.h"
#include "ir/expressions/omittedExpression.h"
#include "ir/expressions/sequenceExpression.h"
#include "ir/expressions/superExpression.h"
#include "ir/expressions/taggedTemplateExpression.h"
#include "ir/expressions/templateLiteral.h"
#include "ir/expressions/thisExpression.h"
#include "ir/expressions/typeofExpression.h"
#include "ir/expressions/unaryExpression.h"
#include "ir/expressions/updateExpression.h"
#include "ir/expressions/yieldExpression.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/classDeclaration.h"
#include "ir/ts/tsAsExpression.h"
#include "ir/ts/tsNonNullExpression.h"
#include "ir/validationInfo.h"
#include "lexer/lexer.h"
#include "lexer/regexp/regexp.h"
#include "lexer/token/letters.h"
#include "lexer/token/sourceLocation.h"
#include "lexer/token/token.h"
#include "util/es2pandaMacros.h"
#include "util/errorRecovery.h"
#include "util/options.h"
#include "generated/diagnostic.h"

#include <memory>

#include "parser/parserStatusContext.h"
#include "parserImpl.h"

namespace ark::es2panda::parser {

static ir::ValidationInfo ValidateExpression(ir::ArrayExpression *arrExpr);
static ir::ValidationInfo ValidateExpression(ir::ObjectExpression *objExpr);
static ir::ValidationInfo ValidateExpression(ir::SpreadElement *spreadElement);

static ir::ValidationInfo ValidateExpression(ir::Identifier *ident)
{
    if ((ident->IdFlags() & ir::IdentifierFlags::OPTIONAL) != 0U) {
        return {diagnostic::UNEXPECTED_QUESTIONMARK.Message(), ident->Start()};
    }

    if (ident->TypeAnnotation() != nullptr) {
        return {diagnostic::UNEXPECTED_TOKEN.Message(), ident->TypeAnnotation()->Start()};
    }

    ir::ValidationInfo info;
    return info;
}

static ir::ValidationInfo ValidateExpression(ir::Property *prop)
{
    ir::ValidationInfo info;

    if (!prop->IsComputed() && !prop->IsMethod() && !prop->IsAccessor() && !prop->IsShorthand()) {
        bool currentIsProto = false;

        if (prop->Key()->IsIdentifier()) {
            currentIsProto = prop->Key()->AsIdentifier()->Name().Is("__proto__");
        } else if (prop->Key()->IsStringLiteral()) {
            currentIsProto = prop->Key()->AsStringLiteral()->Str().Is("__proto__");
        }

        if (currentIsProto) {
            prop->SetKind(ir::PropertyKind::PROTO);
        }
    }

    if (prop->Value() != nullptr) {
        if (prop->Value()->IsAssignmentPattern()) {
            return {diagnostic::INVALID_SHORTHAND_PROPERTY_INITIALIZER.Message(), prop->Value()->Start()};
        }

        if (prop->Value()->IsObjectExpression()) {
            info = ValidateExpression(prop->Value()->AsObjectExpression());
        } else if (prop->Value()->IsArrayExpression()) {
            info = ValidateExpression(prop->Value()->AsArrayExpression());
        }
    }

    return info;
}

static std::pair<ir::ValidationInfo, bool> ValidateProperty(ir::Property *prop, bool &foundProto)
{
    ir::ValidationInfo info = ValidateExpression(prop);
    if (prop->Kind() == ir::PropertyKind::PROTO) {
        if (foundProto) {
            return {{diagnostic::DUPLICATE_PROTO_FIELDS.Message(), prop->Key()->Start()}, true};
        }

        foundProto = true;
    }

    return {info, false};
}

static ir::ValidationInfo ValidateExpression(ir::ObjectExpression *objExpr)
{
    if (objExpr->IsOptional()) {
        return {diagnostic::UNEXPECTED_QUESTIONMARK.Message(), objExpr->Start()};
    }

    if (objExpr->TypeAnnotation() != nullptr) {
        return {diagnostic::UNEXPECTED_TOKEN.Message(), objExpr->TypeAnnotation()->Start()};
    }

    ir::ValidationInfo info;
    bool foundProto = false;

    for (auto *it : objExpr->Properties()) {
        switch (it->Type()) {
            case ir::AstNodeType::OBJECT_EXPRESSION:
            case ir::AstNodeType::ARRAY_EXPRESSION: {
                return {diagnostic::UNEXPECTED_TOKEN.Message(), it->Start()};
            }
            case ir::AstNodeType::SPREAD_ELEMENT: {
                info = ValidateExpression(it->AsSpreadElement());
                break;
            }
            case ir::AstNodeType::PROPERTY: {
                auto *prop = it->AsProperty();
                bool ret = false;
                std::tie(info, ret) = ValidateProperty(prop, foundProto);
                if (ret) {
                    return info;
                }
                break;
            }
            default: {
                break;
            }
        }

        if (info.Fail()) {
            break;
        }
    }

    return info;
}

static ir::ValidationInfo ValidateExpression(ir::SpreadElement *spreadElement)
{
    ir::ValidationInfo info;

    switch (spreadElement->Argument()->Type()) {
        case ir::AstNodeType::OBJECT_EXPRESSION: {
            info = ValidateExpression(spreadElement->Argument()->AsObjectExpression());
            break;
        }
        case ir::AstNodeType::ARRAY_EXPRESSION: {
            info = ValidateExpression(spreadElement->Argument()->AsArrayExpression());
            break;
        }
        default: {
            break;
        }
    }

    return info;
}

static ir::ValidationInfo ValidateExpression(ir::ArrayExpression *arrExpr)
{
    if (arrExpr->IsOptional()) {
        return {diagnostic::UNEXPECTED_QUESTIONMARK.Message(), arrExpr->Start()};
    }

    if (arrExpr->TypeAnnotation() != nullptr) {
        return {diagnostic::UNEXPECTED_TOKEN.Message(), arrExpr->TypeAnnotation()->Start()};
    }

    ir::ValidationInfo info;

    for (auto *it : arrExpr->Elements()) {
        switch (it->Type()) {
            case ir::AstNodeType::OBJECT_EXPRESSION: {
                info = ValidateExpression(it->AsObjectExpression());
                break;
            }
            case ir::AstNodeType::ARRAY_EXPRESSION: {
                info = ValidateExpression(it->AsArrayExpression());
                break;
            }
            case ir::AstNodeType::ASSIGNMENT_EXPRESSION: {
                auto *assignmentExpr = it->AsAssignmentExpression();

                if (assignmentExpr->Left()->IsArrayExpression()) {
                    info = ValidateExpression(assignmentExpr->Left()->AsArrayExpression());
                } else if (assignmentExpr->Left()->IsObjectExpression()) {
                    info = ValidateExpression(assignmentExpr->Left()->AsObjectExpression());
                }

                break;
            }
            case ir::AstNodeType::SPREAD_ELEMENT: {
                info = ValidateExpression(it->AsSpreadElement());
                break;
            }
            default: {
                break;
            }
        }

        if (info.Fail()) {
            break;
        }
    }

    return info;
}

ir::YieldExpression *ParserImpl::ParseYieldExpression()
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::KEYW_YIELD);

    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer::SourcePosition endLoc = lexer_->GetToken().End();

    if ((lexer_->GetToken().Flags() & lexer::TokenFlags::HAS_ESCAPE) != 0) {
        LogError(diagnostic::UNEXPECTED_ID);
    }

    lexer_->NextToken();

    bool isDelegate = false;
    ir::Expression *argument = nullptr;

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MULTIPLY && !lexer_->GetToken().NewLine()) {
        isDelegate = true;
        lexer_->NextToken();

        argument = ParseExpression();
        endLoc = argument->End();
    } else if (!lexer_->GetToken().NewLine() && lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE &&
               lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS &&
               lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET &&
               lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COMMA &&
               lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SEMI_COLON &&
               lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON &&
               lexer_->GetToken().Type() != lexer::TokenType::EOS) {
        argument = ParseExpression();
        endLoc = argument->End();
    }

    auto *yieldNode = AllocNode<ir::YieldExpression>(argument, isDelegate);
    ES2PANDA_ASSERT(yieldNode != nullptr);
    yieldNode->SetRange({startLoc, endLoc});

    return yieldNode;
}

ir::Expression *ParserImpl::ParsePotentialExpressionSequence(ir::Expression *expr, ExpressionParseFlags flags)
{
    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA &&
        (flags & ExpressionParseFlags::ACCEPT_COMMA) != 0) {
        return ParseSequenceExpression(expr, (flags & ExpressionParseFlags::ACCEPT_REST) != 0);
    }

    return expr;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ParserImpl::ParseExpression(ExpressionParseFlags flags)
{
    if (lexer_->GetToken().Type() == lexer::TokenType::KEYW_YIELD &&
        (flags & ExpressionParseFlags::DISALLOW_YIELD) == 0U) {
        ir::YieldExpression *yieldExpr = ParseYieldExpression();

        return ParsePotentialExpressionSequence(yieldExpr, flags);
    }

    ir::Expression *unaryExpressionNode = ParseUnaryOrPrefixUpdateExpression(flags);
    ir::Expression *assignmentExpression = ParseAssignmentExpression(unaryExpressionNode, flags);

    if (lexer_->GetToken().NewLine()) {
        return assignmentExpression;
    }

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA &&
        (flags & ExpressionParseFlags::ACCEPT_COMMA) != 0U) {
        return ParseSequenceExpression(assignmentExpression, (flags & ExpressionParseFlags::ACCEPT_REST) != 0U);
    }

    return assignmentExpression;
}

bool ParserImpl::ParseArrayExpressionRightBracketHelper(bool containsRest, const lexer::SourcePosition &startLoc)
{
    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA) {
        if (containsRest) {
            LogError(diagnostic::REST_PARAM_NOT_LAST, {}, startLoc);
        }

        return true;
    }
    return false;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::ArrayExpression *ParserImpl::ParseArrayExpression(ExpressionParseFlags flags, bool allowOmitted)
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();

    ArenaVector<ir::Expression *> elements(Allocator()->Adapter());

    lexer_->NextToken();

    bool trailingComma = false;
    bool inPattern = (flags & ExpressionParseFlags::MUST_BE_PATTERN) != 0;

    lexer::SourcePosition endLoc;
    ParseList(
        lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET, lexer::NextTokenFlags::NONE,
        [this, inPattern, startLoc, allowOmitted, &elements, &trailingComma]() {
            if (allowOmitted && lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA) {
                auto *omitted = AllocNode<ir::OmittedExpression>();
                omitted->SetRange(lexer_->GetToken().Loc());
                elements.push_back(omitted);
                return true;
            }

            ir::Expression *element {};
            if (inPattern) {
                element = ParsePatternElement();
            } else if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD) {
                element = ParseSpreadElement(ExpressionParseFlags::POTENTIALLY_IN_PATTERN);
            } else {
                element = ParseExpression(ExpressionParseFlags::POTENTIALLY_IN_PATTERN);
            }

            bool containsRest = element->IsRestElement();
            elements.push_back(element);
            trailingComma = ParseArrayExpressionRightBracketHelper(containsRest, startLoc);
            return true;
        },
        &endLoc, true);

    auto nodeType = inPattern ? ir::AstNodeType::ARRAY_PATTERN : ir::AstNodeType::ARRAY_EXPRESSION;
    auto *arrayExpressionNode =
        AllocNode<ir::ArrayExpression>(nodeType, std::move(elements), Allocator(), trailingComma);
    ES2PANDA_ASSERT(arrayExpressionNode != nullptr);
    arrayExpressionNode->SetRange({startLoc, endLoc});

    if (inPattern) {
        arrayExpressionNode->SetDeclaration();
    }

    ParseArrayExpressionErrorCheck(arrayExpressionNode, flags, inPattern);
    return arrayExpressionNode;
}

ir::ArrayExpression *ParserImpl::ParseArrayExpression(ExpressionParseFlags flags)
{
    return ParseArrayExpression(flags, true);
}

void ParserImpl::ParseArrayExpressionErrorCheck(ir::ArrayExpression *arrayExpressionNode,
                                                const ExpressionParseFlags flags, const bool inPattern)
{
    if ((flags & ExpressionParseFlags::POTENTIALLY_IN_PATTERN) == 0) {
        if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION &&
            !arrayExpressionNode->ConvertibleToArrayPattern()) {
            LogError(diagnostic::INVALID_LEFT_SIDE_ARRAY_DESTRUCTURING, {}, arrayExpressionNode->Start());
        } else if (!inPattern && lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
            ir::ValidationInfo info = ValidateExpression(arrayExpressionNode);
            if (info.Fail()) {
                LogSyntaxError(info.msg.Utf8(), info.pos);
            }
        }
    }
}

ParserStatus ParserImpl::ValidateArrowParameter(ir::Expression *expr, [[maybe_unused]] bool *seenOptional)
{
    switch (expr->Type()) {
        case ir::AstNodeType::SPREAD_ELEMENT: {
            if (!expr->AsSpreadElement()->ConvertibleToRest(true)) {
                LogError(diagnostic::INVALID_REST_ELEMENT);
            }

            [[fallthrough]];
        }
        case ir::AstNodeType::REST_ELEMENT: {
            ValidateArrowParameterBindings(expr->AsRestElement()->Argument());
            return ParserStatus::HAS_COMPLEX_PARAM;
        }
        case ir::AstNodeType::IDENTIFIER: {
            ValidateArrowParameterBindings(expr);
            return ParserStatus::NO_OPTS;
        }
        case ir::AstNodeType::OBJECT_EXPRESSION: {
            if (ir::ObjectExpression *objectPattern = expr->AsObjectExpression();
                !objectPattern->ConvertibleToObjectPattern()) {
                LogError(diagnostic::INVALID_DESTRUCTURING_TARGET);
            }

            ValidateArrowParameterBindings(expr);
            return ParserStatus::HAS_COMPLEX_PARAM;
        }
        case ir::AstNodeType::ARRAY_EXPRESSION: {
            if (ir::ArrayExpression *arrayPattern = expr->AsArrayExpression();
                !arrayPattern->ConvertibleToArrayPattern()) {
                LogError(diagnostic::INVALID_DESTRUCTURING_TARGET);
            }

            ValidateArrowParameterBindings(expr);
            return ParserStatus::HAS_COMPLEX_PARAM;
        }
        case ir::AstNodeType::ASSIGNMENT_EXPRESSION: {
            ValidateArrowParameterAssignment(expr->AsAssignmentExpression());
            ValidateArrowParameterBindings(expr);
            return ParserStatus::HAS_COMPLEX_PARAM;
        }
        default: {
            break;
        }
    }
    LogError(diagnostic::INSUFFICIENT_PARAM_IN_ARROW_FUN);
    return ParserStatus::NO_OPTS;
}

void ParserImpl::ValidateArrowParameterAssignment(ir::AssignmentExpression *expr)
{
    if (expr->Right()->IsYieldExpression()) {
        LogError(diagnostic::YIELD_IN_ARROW_FUN_PARAM);
    } else if (expr->Right()->IsAwaitExpression()) {
        LogError(diagnostic::AWAIT_IN_ARROW_FUN_PARAM);
    } else if (!expr->ConvertibleToAssignmentPattern()) {
        LogError(diagnostic::INVALID_DESTRUCTURING_TARGET);
    }
}

ir::ArrowFunctionExpression *ParserImpl::ParseArrowFunctionExpressionBody(ArrowFunctionContext *arrowFunctionContext,
                                                                          ArrowFunctionDescriptor *desc,
                                                                          ir::TSTypeParameterDeclaration *typeParamDecl,
                                                                          ir::TypeNode *returnTypeAnnotation)
{
    context_.Status() |= desc->newStatus;

    lexer_->NextToken();  // eat '=>'
    ir::ScriptFunction *funcNode {};

    ir::AstNode *body = nullptr;
    lexer::SourcePosition endLoc;
    lexer::SourcePosition bodyStart = lexer_->GetToken().Start();

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
        body = ParseExpression();
        endLoc = body->End();
        arrowFunctionContext->AddFlag(ir::ScriptFunctionFlags::EXPRESSION);
    } else {
        lexer_->NextToken();
        auto statements = ParseStatementList();
        body = AllocNode<ir::BlockStatement>(Allocator(), std::move(statements));
        ES2PANDA_ASSERT(body);
        body->SetRange({bodyStart, lexer_->GetToken().End()});

        // This check is redundant since we have ParseStatementList()
        // It is impossible to leave without eos/} or error.
        ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_BRACE);
        endLoc = body->End();
    }
    // clang-format off
    funcNode = AllocNode<ir::ScriptFunction>(
        Allocator(), ir::ScriptFunction::ScriptFunctionData {
                        body,
                        ir::FunctionSignature(typeParamDecl, std::move(desc->params), returnTypeAnnotation),
                        arrowFunctionContext->Flags(),
                        {},
                        context_.GetLanguage()});
    // clang-format on
    ES2PANDA_ASSERT(funcNode != nullptr);
    funcNode->SetRange({desc->startLoc, endLoc});

    auto *arrowFuncNode = AllocNode<ir::ArrowFunctionExpression>(funcNode, Allocator());
    ES2PANDA_ASSERT(arrowFuncNode != nullptr);
    arrowFuncNode->SetRange(funcNode->Range());

    return arrowFuncNode;
}

ArrowFunctionDescriptor ParserImpl::ConvertToArrowParameter(ir::Expression *expr, bool isAsync)
{
    auto arrowStatus = isAsync ? ParserStatus::ASYNC_FUNCTION : ParserStatus::NO_OPTS;
    ArenaVector<ir::Expression *> params(Allocator()->Adapter());

    if (expr == nullptr) {
        return ArrowFunctionDescriptor {std::move(params), lexer_->GetToken().Start(), arrowStatus};
    }

    bool seenOptional = false;

    switch (expr->Type()) {
        case ir::AstNodeType::REST_ELEMENT:
        case ir::AstNodeType::IDENTIFIER:
        case ir::AstNodeType::OBJECT_EXPRESSION:
        case ir::AstNodeType::ASSIGNMENT_EXPRESSION:
        case ir::AstNodeType::ARRAY_EXPRESSION: {
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
                auto *callExpression = expr->AsCallExpression();
                auto &arguments = callExpression->Arguments();

                if (callExpression->HasTrailingComma()) {
                    ES2PANDA_ASSERT(!arguments.empty());
                    LogError(diagnostic::REST_PARAM_NOT_LAST, {}, arguments.back()->End());
                }

                for (auto *it : arguments) {
                    arrowStatus |= ValidateArrowParameter(it, &seenOptional);
                }

                params.swap(arguments);
                break;
            }

            [[fallthrough]];
        }
        default: {
            LogError(diagnostic::UNEXPECTED_TOKEN);
        }
    }

    return ArrowFunctionDescriptor {std::move(params), expr->Start(), arrowStatus};
}

ir::ArrowFunctionExpression *ParserImpl::ParseArrowFunctionExpression(ir::Expression *expr,
                                                                      ir::TSTypeParameterDeclaration *typeParamDecl,
                                                                      ir::TypeNode *returnTypeAnnotation, bool isAsync)
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_ARROW);

    if (lexer_->GetToken().NewLine()) {
        LogError(diagnostic::EXPECTED_ARROW_SAME_LINE);
    }

    ArrowFunctionContext arrowFunctionContext(this, isAsync);
    FunctionParameterContext functionParamContext(&context_);
    ArrowFunctionDescriptor desc = ConvertToArrowParameter(expr, isAsync);

    return ParseArrowFunctionExpressionBody(&arrowFunctionContext, &desc, typeParamDecl, returnTypeAnnotation);
}

bool ParserImpl::ValidateArrowFunctionRestParameter([[maybe_unused]] ir::SpreadElement *restElement)
{
    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        // unexpected_token_3.js
        LogError(diagnostic::REST_PARAM_NOT_LAST);
        return false;
    }

    return true;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ParserImpl::ParseCoverParenthesizedExpressionAndArrowParameterList(
    [[maybe_unused]] ExpressionParseFlags flags)  // CC-OFF(G.FMT.06-CPP) project code style
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    lexer::SourcePosition start = lexer_->GetToken().Start();
    lexer_->NextToken();

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD) {
        ir::SpreadElement *restElement = ParseSpreadElement(ExpressionParseFlags::MUST_BE_PATTERN);
        restElement->SetGrouped();
        restElement->SetStart(start);

        if (ValidateArrowFunctionRestParameter(restElement)) {
            lexer_->NextToken();
        }

        if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_ARROW) {
            // test exists for js extension only
            LogExpectedToken(lexer::TokenType::PUNCTUATOR_ARROW);
        }

        return ParseArrowFunctionExpression(restElement, nullptr, nullptr, false);
    }

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
        lexer_->NextToken();

        if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_ARROW) {
            // test exists for js extension only
            LogExpectedToken(lexer::TokenType::PUNCTUATOR_ARROW);
        }

        auto *arrowExpr = ParseArrowFunctionExpression(nullptr, nullptr, nullptr, false);
        arrowExpr->SetStart(start);
        arrowExpr->AsArrowFunctionExpression()->Function()->SetStart(start);

        return arrowExpr;
    }

    ir::Expression *expr = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA | ExpressionParseFlags::ACCEPT_REST |
                                           ExpressionParseFlags::POTENTIALLY_IN_PATTERN);

    expr->SetGrouped();
    expr->SetRange({start, lexer_->GetToken().End()});

    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);

    return expr;
}

void ParserImpl::ValidateGroupedExpression(ir::Expression *lhsExpression)
{
    lexer::TokenType tokenType = lexer_->GetToken().Type();
    if (lhsExpression->IsGrouped() && tokenType != lexer::TokenType::PUNCTUATOR_ARROW) {
        if (lhsExpression->IsSequenceExpression()) {
            for (auto *seq : lhsExpression->AsSequenceExpression()->Sequence()) {
                ValidateParenthesizedExpression(seq);
            }
        } else {
            ValidateParenthesizedExpression(lhsExpression);
        }
    }
}

void ParserImpl::ValidateParenthesizedExpression(ir::Expression *lhsExpression)
{
    switch (lhsExpression->Type()) {
        case ir::AstNodeType::IDENTIFIER: {
            auto info = ValidateExpression(lhsExpression->AsIdentifier());
            if (info.Fail()) {
                LogSyntaxError(info.msg.Utf8(), info.pos);
            }
            break;
        }
        case ir::AstNodeType::MEMBER_EXPRESSION: {
            break;
        }
        case ir::AstNodeType::ARRAY_EXPRESSION: {
            auto info = ValidateExpression(lhsExpression->AsArrayExpression());
            if (info.Fail()) {
                LogSyntaxError(info.msg.Utf8(), info.pos);
            }
            break;
        }
        case ir::AstNodeType::OBJECT_EXPRESSION: {
            auto info = ValidateExpression(lhsExpression->AsObjectExpression());
            if (info.Fail()) {
                LogSyntaxError(info.msg.Utf8(), info.pos);
            }
            break;
        }
        case ir::AstNodeType::ASSIGNMENT_EXPRESSION: {
            if (lhsExpression->AsAssignmentExpression()->ConvertibleToAssignmentPattern(false)) {
                break;
            }
            [[fallthrough]];
        }
        case ir::AstNodeType::SPREAD_ELEMENT: {
            LogError(diagnostic::INVALID_LEFT_SIDE_IN_ASSIGNMENT);
            break;
        }
        default: {
            break;
        }
    }
}

ir::Expression *ParserImpl::ParsePrefixAssertionExpression()
{
    LogUnexpectedToken(lexer_->GetToken());
    return AllocBrokenExpression(Lexer()->GetToken().Loc());
}

ir::Expression *ParserImpl::ParseAssignmentExpressionHelper()
{
    lexer_->NextToken();
    ir::Expression *consequent = ParseExpression();

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
        if (lexer::Token::IsPunctuatorToken(lexer_->GetToken().Type())) {
            // typo happened like ';'
            // unexpected_token_41.ets - bad test
            LogExpectedToken(lexer::TokenType::PUNCTUATOR_COLON);
            lexer_->NextToken();  // eat ':'
        } else {
            // just go to the next token if we missed ':'
            LogError(diagnostic::UNEXPECTED_TOKEN);
        }
    } else {
        lexer_->NextToken();  // eat ':'
    }
    return consequent;
}

ir::Expression *ParserImpl::CreateBinaryAssignmentExpression(ir::Expression *assignmentExpression,
                                                             ir::Expression *lhsExpression, lexer::TokenType tokenType)
{
    auto *binaryAssignmentExpression =
        AllocNode<ir::AssignmentExpression>(lhsExpression, assignmentExpression, tokenType);
    ES2PANDA_ASSERT(binaryAssignmentExpression != nullptr);

    binaryAssignmentExpression->SetRange({lhsExpression->Start(), assignmentExpression->End()});
    return binaryAssignmentExpression;
}

ir::Expression *ParserImpl::ParseAssignmentExpression(ir::Expression *lhsExpression, ExpressionParseFlags flags)
{
    ValidateGroupedExpression(lhsExpression);

    lexer::TokenType tokenType = lexer_->GetToken().Type();
    switch (tokenType) {
        case lexer::TokenType::PUNCTUATOR_QUESTION_MARK: {
            ir::Expression *consequent = ParseAssignmentExpressionHelper();
            ir::Expression *alternate = ParseExpression();
            auto *conditionalExpr = AllocNode<ir::ConditionalExpression>(lhsExpression, consequent, alternate);
            ES2PANDA_ASSERT(conditionalExpr != nullptr);
            conditionalExpr->SetRange({lhsExpression->Start(), alternate->End()});
            return conditionalExpr;
        }
        case lexer::TokenType::PUNCTUATOR_ARROW:
            if (lexer_->GetToken().NewLine()) {
                LogError(diagnostic::EXPECTED_EXPRESSION_GOT_ARROW);
            }

            return ParseArrowFunctionExpression(lhsExpression, nullptr, nullptr, false);
        case lexer::TokenType::PUNCTUATOR_SUBSTITUTION: {
            ValidateAssignmentTarget(flags, lhsExpression);

            lexer_->NextToken();
            ir::Expression *assignmentExpression = ParseExpression(CarryPatternFlags(flags));
            return CreateBinaryAssignmentExpression(assignmentExpression, lhsExpression, tokenType);
        }
        case lexer::TokenType::KEYW_AS:
            if (auto asExpression = ParsePotentialAsExpression(lhsExpression); asExpression != nullptr) {
                return ParseAssignmentExpression(asExpression);
            }
            break;
        default: {
            auto expression = ParseAssignmentBinaryExpression(tokenType, lhsExpression, flags);
            if (expression == nullptr) {
                expression = ParseAssignmentEqualExpression(tokenType, lhsExpression, flags);
            }

            if (expression != nullptr) {
                return expression;
            }

            break;
        }
    }

    return lhsExpression;
}

ir::Expression *ParserImpl::ParseAssignmentBinaryExpression(const lexer::TokenType tokenType,
                                                            ir::Expression *lhsExpression,
                                                            const ExpressionParseFlags flags)
{
    switch (tokenType) {
        case lexer::TokenType::KEYW_IN: {
            if ((flags & ExpressionParseFlags::STOP_AT_IN) != 0) {
                break;
            }

            [[fallthrough]];
        }
        case lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING:
        case lexer::TokenType::PUNCTUATOR_LOGICAL_OR:
        case lexer::TokenType::PUNCTUATOR_LOGICAL_AND:
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR:
        case lexer::TokenType::PUNCTUATOR_BITWISE_XOR:
        case lexer::TokenType::PUNCTUATOR_BITWISE_AND:
        case lexer::TokenType::PUNCTUATOR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_PLUS:
        case lexer::TokenType::PUNCTUATOR_MINUS:
        case lexer::TokenType::PUNCTUATOR_MULTIPLY:
        case lexer::TokenType::PUNCTUATOR_DIVIDE:
        case lexer::TokenType::PUNCTUATOR_MOD:
        case lexer::TokenType::KEYW_INSTANCEOF:
        case lexer::TokenType::PUNCTUATOR_EXPONENTIATION: {
            return ParseAssignmentExpression(ParseBinaryExpression(lhsExpression));
        }
        default:
            break;
    }

    return nullptr;
}

ir::Expression *ParserImpl::ParseAssignmentEqualExpression(const lexer::TokenType tokenType,
                                                           ir::Expression *lhsExpression,
                                                           const ExpressionParseFlags flags)
{
    switch (tokenType) {
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_PLUS_EQUAL:
        case lexer::TokenType::PUNCTUATOR_MINUS_EQUAL:
        case lexer::TokenType::PUNCTUATOR_MULTIPLY_EQUAL:
        case lexer::TokenType::PUNCTUATOR_DIVIDE_EQUAL:
        case lexer::TokenType::PUNCTUATOR_MOD_EQUAL:
        case lexer::TokenType::PUNCTUATOR_BITWISE_AND_EQUAL:
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_BITWISE_XOR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_EXPONENTIATION_EQUAL: {
            ValidateLvalueAssignmentTarget(lhsExpression);

            lexer_->NextToken();
            ir::Expression *assignmentExpression = ParseExpression(CarryPatternFlags(flags));

            auto *binaryAssignmentExpression =
                AllocNode<ir::AssignmentExpression>(lhsExpression, assignmentExpression, tokenType);
            ES2PANDA_ASSERT(binaryAssignmentExpression != nullptr);

            binaryAssignmentExpression->SetRange({lhsExpression->Start(), assignmentExpression->End()});
            return binaryAssignmentExpression;
        }
        case lexer::TokenType::PUNCTUATOR_LOGICAL_AND_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LOGICAL_OR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LOGICAL_NULLISH_EQUAL: {
            LogUnexpectedToken(tokenType);
            lexer_->NextToken();  // eat token
            ParseExpression();    // Try to parse expression, but skip the result.
            [[fallthrough]];
        }
        default:
            break;
    }

    return nullptr;
}

ir::TemplateLiteral *ParserImpl::ParseTemplateLiteral()
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();

    auto startPos = lexer_->Save();
    auto multilineStr = Lexer()->ScanMultilineString();
    lexer_->Rewind(startPos);

    ArenaVector<ir::TemplateElement *> quasis(Allocator()->Adapter());
    ArenaVector<ir::Expression *> expressions(Allocator()->Adapter());

    while (true) {
        lexer_->ResetTokenEnd();
        startPos = lexer_->Save();
        lexer_->ScanString<lexer::LEX_CHAR_BACK_TICK>();
        const util::StringView cooked = lexer_->GetToken().String();
        lexer_->Rewind(startPos);

        const auto templateStr = lexer_->ScanTemplateString();
        if (templateStr.validSequence) {
            auto *const element = AllocNode<ir::TemplateElement>(templateStr.str.View(), cooked);
            ES2PANDA_ASSERT(element != nullptr);
            element->SetRange({lexer::SourcePosition {startPos.Iterator().Index(), startPos.Line(), GetProgram()},
                               lexer::SourcePosition {templateStr.end, lexer_->Line(), GetProgram()}});
            quasis.push_back(element);
        }

        if (!templateStr.scanExpression) {
            lexer_->ScanTemplateStringEnd();
            break;
        }

        ir::Expression *expression = nullptr;

        {
            lexer::TemplateLiteralParserContext ctx(lexer_);
            lexer_->PushTemplateContext(&ctx);
            lexer_->NextToken();
            expression = ParseExpression();
        }

        ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_BRACE, false);

        expressions.push_back(expression);
    }

    auto *templateNode = AllocNode<ir::TemplateLiteral>(std::move(quasis), std::move(expressions), multilineStr);
    ES2PANDA_ASSERT(templateNode != nullptr);
    templateNode->SetRange({startLoc, lexer_->GetToken().End()});

    lexer_->NextToken();

    return templateNode;
}

ir::Expression *ParserImpl::ParseNewExpression()
{
    lexer::SourcePosition start = lexer_->GetToken().Start();

    lexer_->NextToken();  // eat new

    // parse callee part of NewExpression
    ir::Expression *callee = ParseMemberExpression(true);
    if (callee->IsImportExpression() && !callee->IsGrouped()) {
        LogError(diagnostic::NEW_WITH_IMPORT);
    }

    ArenaVector<ir::Expression *> arguments(Allocator()->Adapter());

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
        lexer::SourcePosition endLoc = callee->End();
        auto *newExprNode = AllocNode<ir::NewExpression>(callee, std::move(arguments));
        ES2PANDA_ASSERT(newExprNode != nullptr);
        newExprNode->SetRange({start, endLoc});

        return newExprNode;
    }

    lexer_->NextToken();  // eat left parenthesis

    lexer::SourcePosition endLoc;
    // parse argument part of NewExpression
    ParseList(
        lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS, lexer::NextTokenFlags::NONE,
        [this, &arguments]() {
            ir::Expression *argument = nullptr;

            if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD) {
                argument = ParseSpreadElement();
            } else {
                argument = ParseExpression();
            }
            arguments.push_back(argument);
            return true;
        },
        &endLoc, true);

    auto *newExprNode = AllocNode<ir::NewExpression>(callee, std::move(arguments));
    ES2PANDA_ASSERT(newExprNode != nullptr);
    newExprNode->SetRange({start, endLoc});

    return newExprNode;
}

ir::Expression *ParserImpl::ParseLeftHandSideExpression(ExpressionParseFlags flags)
{
    return ParseMemberExpression(false, flags);
}

ir::MetaProperty *ParserImpl::ParsePotentialNewTarget()
{
    lexer::SourceRange loc = lexer_->GetToken().Loc();

    if (lexer_->Lookahead() == lexer::LEX_CHAR_DOT) {
        lexer_->NextToken();
        lexer_->NextToken();

        if (lexer_->GetToken().KeywordType() == lexer::TokenType::KEYW_TARGET) {
            if ((context_.Status() & ParserStatus::ALLOW_NEW_TARGET) == 0) {
                LogError(diagnostic::NEW_TARGET_IS_NOT_ALLOWED);
            }

            auto *metaProperty = AllocNode<ir::MetaProperty>(ir::MetaProperty::MetaPropertyKind::NEW_TARGET);
            ES2PANDA_ASSERT(metaProperty != nullptr);
            metaProperty->SetRange(loc);
            lexer_->NextToken();
            return metaProperty;
        }
    }

    return nullptr;
}

ir::Identifier *ParserImpl::ParsePrimaryExpressionIdent([[maybe_unused]] ExpressionParseFlags flags)
{
    auto *identNode = AllocNode<ir::Identifier>(lexer_->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(identNode != nullptr);
    identNode->SetRange(lexer_->GetToken().Loc());

    lexer_->NextToken();
    return identNode;
}

ir::BooleanLiteral *ParserImpl::ParseBooleanLiteral()
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::LITERAL_TRUE ||
                    lexer_->GetToken().Type() == lexer::TokenType::LITERAL_FALSE);

    auto *booleanNode = AllocNode<ir::BooleanLiteral>(lexer_->GetToken().Type() == lexer::TokenType::LITERAL_TRUE);
    ES2PANDA_ASSERT(booleanNode != nullptr);
    booleanNode->SetRange(lexer_->GetToken().Loc());

    lexer_->NextToken();
    return booleanNode;
}

ir::NullLiteral *ParserImpl::ParseNullLiteral()
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::LITERAL_NULL);
    auto *nullNode = AllocNode<ir::NullLiteral>();
    ES2PANDA_ASSERT(nullNode != nullptr);
    nullNode->SetRange(lexer_->GetToken().Loc());

    lexer_->NextToken();
    return nullNode;
}

ir::Literal *ParserImpl::ParseNumberLiteral()
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::LITERAL_NUMBER);

    ir::Literal *numberNode {};

    if ((lexer_->GetToken().Flags() & lexer::TokenFlags::NUMBER_BIGINT) != 0U) {
        numberNode = AllocNode<ir::BigIntLiteral>(lexer_->GetToken().BigInt());
    } else {
        numberNode = AllocNode<ir::NumberLiteral>(lexer_->GetToken().GetNumber());
    }

    ES2PANDA_ASSERT(numberNode != nullptr);
    numberNode->SetRange(lexer_->GetToken().Loc());

    lexer_->NextToken();
    return numberNode;
}

ir::CharLiteral *ParserImpl::ParseCharLiteral()
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::LITERAL_CHAR);

    auto *charNode = AllocNode<ir::CharLiteral>(lexer_->GetToken().Utf16());
    ES2PANDA_ASSERT(charNode != nullptr);
    charNode->SetRange(lexer_->GetToken().Loc());

    lexer_->NextToken();
    return charNode;
}

ir::StringLiteral *ParserImpl::ParseStringLiteral()
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::LITERAL_STRING);

    auto *stringNode = AllocNode<ir::StringLiteral>(lexer_->GetToken().String());
    ES2PANDA_ASSERT(stringNode != nullptr);
    stringNode->SetRange(lexer_->GetToken().Loc());

    lexer_->NextToken();
    return stringNode;
}

ir::UndefinedLiteral *ParserImpl::ParseUndefinedLiteral()
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::KEYW_UNDEFINED);
    auto *undefinedNode = AllocNode<ir::UndefinedLiteral>();
    ES2PANDA_ASSERT(undefinedNode != nullptr);
    undefinedNode->SetRange(lexer_->GetToken().Loc());

    lexer_->NextToken();
    return undefinedNode;
}

ir::ThisExpression *ParserImpl::ParseThisExpression()
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::KEYW_THIS);

    auto *thisExprNode = AllocNode<ir::ThisExpression>();
    ES2PANDA_ASSERT(thisExprNode);
    thisExprNode->SetRange(lexer_->GetToken().Loc());

    lexer_->NextToken();
    return thisExprNode;
}

ir::RegExpLiteral *ParserImpl::ParseRegularExpression()
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_DIVIDE ||
                    lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_DIVIDE_EQUAL);

    lexer_->ResetTokenEnd();
    auto regexp = lexer_->ScanRegExp();

    lexer::RegExpParser reParser(regexp, Allocator(), this);

    reParser.ParsePattern();

    auto *regexpNode = AllocNode<ir::RegExpLiteral>(regexp.patternStr, regexp.flags, regexp.flagsStr);
    ES2PANDA_ASSERT(regexpNode != nullptr);
    regexpNode->SetRange(lexer_->GetToken().Loc());

    lexer_->NextToken();
    return regexpNode;
}

ir::SuperExpression *ParserImpl::ParseSuperExpression()
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::KEYW_SUPER);

    auto *superExprNode = AllocNode<ir::SuperExpression>();
    ES2PANDA_ASSERT(superExprNode != nullptr);
    superExprNode->SetRange(lexer_->GetToken().Loc());

    lexer_->NextToken();  // eat super

    if ((lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD ||
         lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) &&
        (context_.Status() & ParserStatus::ALLOW_SUPER) != 0U) {
        return superExprNode;
    }

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS &&
        (context_.Status() & ParserStatus::ALLOW_SUPER_CALL) != 0U) {
        return superExprNode;
    }

    LogError(diagnostic::UNEXPECTED_SUPER);
    return superExprNode;
}

ir::Expression *ParserImpl::ParsePrimaryExpressionWithLiterals(ExpressionParseFlags flags)
{
    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::LITERAL_IDENT:
            return ParsePrimaryExpressionIdent(flags);
        case lexer::TokenType::LITERAL_TRUE:
        case lexer::TokenType::LITERAL_FALSE:
            return ParseBooleanLiteral();
        case lexer::TokenType::LITERAL_NULL:
            return ParseNullLiteral();
        case lexer::TokenType::KEYW_UNDEFINED:
            return ParseUndefinedLiteral();
        case lexer::TokenType::LITERAL_NUMBER:
            return ParseNumberLiteral();
        case lexer::TokenType::LITERAL_STRING:
            return ParseStringLiteral();
        default:
            LogUnexpectedToken(lexer_->GetToken());
            return AllocBrokenExpression(Lexer()->GetToken().Loc());
    }
}

ir::Expression *ParserImpl::ParseHashMaskOperator()
{
    if (!ValidatePrivateIdentifier()) {
        return AllocBrokenExpression(Lexer()->GetToken().Loc());
    }

    auto *privateIdent = AllocNode<ir::Identifier>(lexer_->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(privateIdent != nullptr);
    privateIdent->SetPrivate(true);
    lexer_->NextToken();

    if (lexer_->GetToken().Type() != lexer::TokenType::KEYW_IN) {
        LogError(diagnostic::UNEXPECTED_PRIVATE);
        lexer_->GetToken().SetTokenType(lexer::TokenType::KEYW_IN);
    }

    return privateIdent;
}

ir::Expression *ParserImpl::ParseClassExpression()
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    ir::ClassDefinition *classDefinition = ParseClassDefinition(ir::ClassDefinitionModifiers::ID_REQUIRED);
    if (classDefinition == nullptr) {  // Error processing.
        // Error is logged inside ParseClassDefinition
        return AllocBrokenExpression(startLoc);
    }

    auto *classExpr = AllocNode<ir::ClassExpression>(classDefinition);
    ES2PANDA_ASSERT(classExpr != nullptr);
    classExpr->SetRange({startLoc, classDefinition->End()});

    return classExpr;
}

ir::Expression *ParserImpl::ParsePunctuators(ExpressionParseFlags flags)
{
    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_DIVIDE:
        case lexer::TokenType::PUNCTUATOR_DIVIDE_EQUAL: {
            return ParseRegularExpression();
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET: {
            return ParseArrayExpression(CarryPatternFlags(flags));
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS: {
            return ParseCoverParenthesizedExpressionAndArrowParameterList();
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_BRACE: {
            return ParseObjectExpression(CarryPatternFlags(flags));
        }
        case lexer::TokenType::PUNCTUATOR_BACK_TICK: {
            return ParseTemplateLiteral();
        }
        case lexer::TokenType::PUNCTUATOR_HASH_MARK: {
            return ParseHashMaskOperator();
        }
        case lexer::TokenType::PUNCTUATOR_LESS_THAN: {
            return ParsePrefixAssertionExpression();
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ParserImpl::ParsePrimaryExpression(ExpressionParseFlags flags)
{
    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::KEYW_IMPORT:
            return ParseImportExpression();
        case lexer::TokenType::PUNCTUATOR_DIVIDE:
        case lexer::TokenType::PUNCTUATOR_DIVIDE_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET:
        case lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS:
        case lexer::TokenType::PUNCTUATOR_LEFT_BRACE:
        case lexer::TokenType::PUNCTUATOR_BACK_TICK:
        case lexer::TokenType::PUNCTUATOR_HASH_MARK:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
            return ParsePunctuators(flags);
        case lexer::TokenType::KEYW_FUNCTION:
            return ParseFunctionExpression();
        case lexer::TokenType::KEYW_CLASS:
            return ParseClassExpression();
        case lexer::TokenType::KEYW_THIS:
            return ParseThisExpression();
        case lexer::TokenType::KEYW_TYPEOF:
            return ParseUnaryOrPrefixUpdateExpression();
        case lexer::TokenType::KEYW_SUPER:
            return ParseSuperExpression();
        case lexer::TokenType::KEYW_NEW: {
            ir::MetaProperty *newTarget = ParsePotentialNewTarget();
            if (newTarget != nullptr) {
                return newTarget;
            }
            return ParseNewExpression();
        }
        default:
            return ParsePrimaryExpressionWithLiterals(flags);
    }
}

static constexpr size_t GetOperatorPrecedenceArithmeticAndComparison(const lexer::TokenType operatorType)
{
    switch (operatorType) {
        case lexer::TokenType::PUNCTUATOR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL: {
            constexpr auto PRECEDENCE = 8;
            return PRECEDENCE;
        }
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL:
        case lexer::TokenType::KEYW_INSTANCEOF:
        case lexer::TokenType::KEYW_IN: {
            constexpr auto PRECEDENCE = 9;
            return PRECEDENCE;
        }
        case lexer::TokenType::PUNCTUATOR_PLUS:
        case lexer::TokenType::PUNCTUATOR_MINUS: {
            constexpr auto PRECEDENCE = 12;
            return PRECEDENCE;
        }
        case lexer::TokenType::PUNCTUATOR_MULTIPLY:
        case lexer::TokenType::PUNCTUATOR_DIVIDE:
        case lexer::TokenType::PUNCTUATOR_MOD: {
            constexpr auto PRECEDENCE = 13;
            return PRECEDENCE;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

static constexpr size_t GetOperatorPrecedence(const lexer::TokenType operatorType)
{
    ES2PANDA_ASSERT(operatorType == lexer::TokenType::KEYW_AS || lexer::Token::IsBinaryToken(operatorType));

    switch (operatorType) {
        case lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING: {
            constexpr auto PRECEDENCE = 1;
            return PRECEDENCE;
        }
        case lexer::TokenType::PUNCTUATOR_LOGICAL_OR: {
            constexpr auto PRECEDENCE = 2;
            return PRECEDENCE;
        }
        case lexer::TokenType::PUNCTUATOR_LOGICAL_AND: {
            constexpr auto PRECEDENCE = 4;
            return PRECEDENCE;
        }
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR: {
            constexpr auto PRECEDENCE = 5;
            return PRECEDENCE;
        }
        case lexer::TokenType::PUNCTUATOR_BITWISE_XOR: {
            constexpr auto PRECEDENCE = 6;
            return PRECEDENCE;
        }
        case lexer::TokenType::PUNCTUATOR_BITWISE_AND: {
            constexpr auto PRECEDENCE = 7;
            return PRECEDENCE;
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT: {
            constexpr auto PRECEDENCE = 10;
            return PRECEDENCE;
        }
        case lexer::TokenType::KEYW_AS: {
            constexpr auto PRECEDENCE = 11;
            return PRECEDENCE;
        }
        case lexer::TokenType::PUNCTUATOR_EXPONENTIATION: {
            constexpr auto PRECEDENCE = 14;
            return PRECEDENCE;
        }
        default: {
            return GetOperatorPrecedenceArithmeticAndComparison(operatorType);
        }
    }
}

static inline bool ShouldBinaryExpressionBeAmended(const ir::BinaryExpression *const binaryExpression,
                                                   const lexer::TokenType operatorType)
{
    return GetOperatorPrecedence(binaryExpression->OperatorType()) <= GetOperatorPrecedence(operatorType) &&
           !binaryExpression->IsGrouped() &&
           (operatorType != lexer::TokenType::PUNCTUATOR_EXPONENTIATION ||
            binaryExpression->OperatorType() != lexer::TokenType::PUNCTUATOR_EXPONENTIATION);
}

static inline bool ShouldAsExpressionBeAmended(const ir::TSAsExpression *const asExpression,
                                               const lexer::TokenType operatorType)
{
    return GetOperatorPrecedence(lexer::TokenType::KEYW_AS) <= GetOperatorPrecedence(operatorType) &&
           !asExpression->IsGrouped() && operatorType != lexer::TokenType::PUNCTUATOR_EXPONENTIATION;
}

static inline bool ShouldExpressionBeAmended(const ir::Expression *const expression,
                                             const lexer::TokenType operatorType)
{
    bool shouldBeAmended = false;

    if (expression->IsBinaryExpression()) {
        shouldBeAmended = ShouldBinaryExpressionBeAmended(expression->AsBinaryExpression(), operatorType);
    } else if (expression->IsTSAsExpression()) {
        shouldBeAmended = ShouldAsExpressionBeAmended(expression->AsTSAsExpression(), operatorType);
    }

    return shouldBeAmended;
}

static inline bool AreLogicalAndNullishMixedIncorrectly(const ir::Expression *const expression,
                                                        const lexer::TokenType operatorType)
{
    return ((operatorType == lexer::TokenType::PUNCTUATOR_LOGICAL_OR ||
             operatorType == lexer::TokenType::PUNCTUATOR_LOGICAL_AND) &&
            expression->IsBinaryExpression() &&
            expression->AsBinaryExpression()->OperatorType() == lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING &&
            !expression->IsGrouped()) ||
           (operatorType == lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING && expression->IsBinaryExpression() &&
            (expression->AsBinaryExpression()->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_OR ||
             expression->AsBinaryExpression()->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_AND) &&
            !expression->IsGrouped());
}

static inline ir::Expression *GetAmendedChildExpression(ir::Expression *const expression)
{
    ir::Expression *amendedChild = nullptr;

    if (expression->IsBinaryExpression()) {
        amendedChild = expression->AsBinaryExpression()->Left();
    } else if (expression->IsTSAsExpression()) {
        amendedChild = expression->AsTSAsExpression()->Expr();
    } else {
        ES2PANDA_UNREACHABLE();
    }

    return amendedChild;
}

static inline void SetAmendedChildExpression(ir::Expression *const parent, ir::Expression *const amended)
{
    if (parent->IsBinaryExpression()) {
        parent->AsBinaryExpression()->SetLeft(amended);
        amended->SetParent(parent);
    } else if (parent->IsTSAsExpression()) {
        parent->AsTSAsExpression()->SetExpr(amended);
        amended->SetParent(parent);
    } else {
        ES2PANDA_UNREACHABLE();
    }
}

void ParserImpl::CreateAmendedBinaryExpression(ir::Expression *const left, ir::Expression *const right,
                                               const lexer::TokenType operatorType)
{
    auto *amended = GetAmendedChildExpression(right);

    amended->SetParent(nullptr);  // Next line overwrite parent
    auto *binaryExpr = AllocNode<ir::BinaryExpression>(left, amended, operatorType);

    ES2PANDA_ASSERT(binaryExpr != nullptr);
    binaryExpr->SetRange({left->Start(), amended->End()});
    SetAmendedChildExpression(right, binaryExpr);
}

ir::Expression *ParserImpl::ParseExpressionOrTypeAnnotation([[maybe_unused]] lexer::TokenType type,
                                                            [[maybe_unused]] ExpressionParseFlags flags)
{
    return ParseExpression(ExpressionParseFlags::DISALLOW_YIELD);
}

static ir::Expression *FindAndAmendChildExpression(ir::Expression *expression, const ir::Expression *left,
                                                   lexer::TokenType operatorType)
{
    bool shouldBeAmended = true;
    ir::Expression *parentExpression = nullptr;
    while (shouldBeAmended && GetAmendedChildExpression(expression)->IsBinaryExpression()) {
        parentExpression = expression;
        parentExpression->SetStart(left->Start());

        expression = GetAmendedChildExpression(expression);
        shouldBeAmended = ShouldExpressionBeAmended(expression, operatorType);
    }

    return shouldBeAmended ? expression : parentExpression;
}

ir::Expression *ParserImpl::ParseBinaryExpression(ir::Expression *left, ExpressionParseFlags flags)
{
    lexer::TokenType operatorType = lexer_->GetToken().Type();
    ES2PANDA_ASSERT(lexer::Token::IsBinaryToken(operatorType));

    if (operatorType == lexer::TokenType::PUNCTUATOR_EXPONENTIATION) {
        if ((left->IsUnaryExpression() || left->IsTypeofExpression()) && !left->IsGrouped()) {
            LogError(diagnostic::ILLEGAL_EXPRESSION_WRAP);
        }
    }

    lexer_->NextToken();

    ExpressionParseFlags newFlags = ExpressionParseFlags::DISALLOW_YIELD;
    if ((operatorType == lexer::TokenType::KEYW_INSTANCEOF) || ((flags & ExpressionParseFlags::INSTANCEOF) != 0)) {
        newFlags |= ExpressionParseFlags::INSTANCEOF;
    }

    ir::Expression *rightExpr = ParseExpressionOrTypeAnnotation(operatorType, ExpressionParseFlags::DISALLOW_YIELD);
    ir::ConditionalExpression *conditionalExpr = nullptr;
    if (rightExpr->IsConditionalExpression() && !rightExpr->IsGrouped()) {
        conditionalExpr = rightExpr->AsConditionalExpression();
        rightExpr = conditionalExpr->Test();
    }

    if (ShouldExpressionBeAmended(rightExpr, operatorType)) {
        if (AreLogicalAndNullishMixedIncorrectly(rightExpr, operatorType)) {
            LogError(diagnostic::NULLISH_COALESCING_MISSING_PARENS);
        }

        ir::Expression *expression = FindAndAmendChildExpression(rightExpr, left, operatorType);
        CreateAmendedBinaryExpression(left, expression, operatorType);
    } else {
        if (AreLogicalAndNullishMixedIncorrectly(rightExpr, operatorType)) {
            LogError(diagnostic::NULLISH_COALESCING_MISSING_PARENS);
        }
        const lexer::SourcePosition &endPos = rightExpr->End();
        rightExpr = AllocNode<ir::BinaryExpression>(left, rightExpr, operatorType);
        ES2PANDA_ASSERT(rightExpr != nullptr);
        rightExpr->SetRange({left->Start(), endPos});
    }

    if (conditionalExpr != nullptr) {
        conditionalExpr->SetStart(rightExpr->Start());
        conditionalExpr->SetTest(rightExpr);
        return conditionalExpr;
    }

    return rightExpr;
}

ArenaVector<ir::Expression *> ParserImpl::ParseCallExpressionArguments(bool &trailingComma,
                                                                       lexer::SourcePosition &endLoc)
{
    ArenaVector<ir::Expression *> arguments(Allocator()->Adapter());

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_FORMAT &&
        lexer_->Lookahead() == static_cast<char32_t>(ARRAY_FORMAT_NODE)) {
        arguments = std::move(ParseExpressionsArrayFormatPlaceholder());
        endLoc = Lexer()->GetToken().End();
        ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);
    } else {
        ParseList(
            lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS, lexer::NextTokenFlags::NONE,
            [this, &trailingComma, &arguments]() {
                trailingComma = false;
                ir::Expression *argument {};
                if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD) {
                    argument = ParseSpreadElement();
                } else {
                    argument = ParseExpression();
                }
                arguments.push_back(argument);

                if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA) {
                    trailingComma = true;
                }
                return true;
            },
            &endLoc, true);
    }

    return arguments;
}

ir::CallExpression *ParserImpl::ParseCallExpression(ir::Expression *callee, bool isOptionalChain, bool handleEval)
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);
    bool trailingComma {};

    while (true) {
        lexer_->NextToken();
        lexer::SourcePosition endLoc;
        ArenaVector<ir::Expression *> arguments = ParseCallExpressionArguments(trailingComma, endLoc);
        ir::CallExpression *callExpr {};

        if (!isOptionalChain && handleEval && callee->IsIdentifier() && callee->AsIdentifier()->Name().Is("eval")) {
            auto parserStatus = static_cast<uint32_t>(context_.Status() | ParserStatus::DIRECT_EVAL);
            callExpr = AllocNode<ir::DirectEvalExpression>(callee, std::move(arguments), nullptr, isOptionalChain,
                                                           parserStatus);
        } else {
            callExpr =
                AllocNode<ir::CallExpression>(callee, std::move(arguments), nullptr, isOptionalChain, trailingComma);
        }
        ES2PANDA_ASSERT(callExpr != nullptr);

        callExpr->SetRange({callee->Start(), endLoc});
        isOptionalChain = false;

        if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
            ParseTrailingBlock(callExpr);
            if (callExpr->TrailingBlock() != nullptr) {
                callExpr->SetRange({callee->Start(), callExpr->TrailingBlock()->End()});
            }
            return callExpr;
        }

        callee = callExpr;
    }

    ES2PANDA_UNREACHABLE();
}

ir::Expression *ParserImpl::ParseOptionalChain(ir::Expression *leftSideExpr)
{
    ir::Expression *returnExpression = nullptr;

    bool isPrivate = false;

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_HASH_MARK) {
        isPrivate = true;
        ValidatePrivateIdentifier();
    }

    const auto tokenType = lexer_->GetToken().Type();
    if (tokenType == lexer::TokenType::LITERAL_IDENT) {
        auto *identNode = AllocNode<ir::Identifier>(lexer_->GetToken().Ident(), Allocator());
        ES2PANDA_ASSERT(identNode != nullptr);
        identNode->SetPrivate(isPrivate);
        identNode->SetRange(lexer_->GetToken().Loc());

        returnExpression = AllocNode<ir::MemberExpression>(leftSideExpr, identNode,
                                                           ir::MemberExpressionKind::PROPERTY_ACCESS, false, true);
        ES2PANDA_ASSERT(returnExpression != nullptr);
        returnExpression->SetRange({leftSideExpr->Start(), identNode->End()});
        lexer_->NextToken();
    }

    if (tokenType == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET) {
        lexer_->NextToken();  // eat '['
        ir::Expression *propertyNode = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);

        auto endLoc = lexer_->GetToken().End();
        if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
            // test exists for ts extension only
            LogExpectedToken(lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET);
        }

        returnExpression = AllocNode<ir::MemberExpression>(leftSideExpr, propertyNode,
                                                           ir::MemberExpressionKind::ELEMENT_ACCESS, true, true);
        ES2PANDA_ASSERT(returnExpression != nullptr);
        returnExpression->SetRange({leftSideExpr->Start(), endLoc});
        lexer_->NextToken();
    }

    if (tokenType == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
        returnExpression = ParseCallExpression(leftSideExpr, true);
    }

    // Static semantic
    if (tokenType == lexer::TokenType::PUNCTUATOR_BACK_TICK ||
        lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_BACK_TICK) {
        LogError(diagnostic::TAGGED_TEMPLATE_LITERALS_IN_OPTIONALCHAIN);
        return AllocBrokenExpression(lexer_->GetToken().Loc());
    }

    return returnExpression;
}

ir::ArrowFunctionExpression *ParserImpl::ParsePotentialArrowExpression(ir::Expression **returnExpression,
                                                                       const lexer::SourcePosition &startLoc)
{
    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::KEYW_FUNCTION: {
            *returnExpression = ParseFunctionExpression(ParserStatus::ASYNC_FUNCTION);
            ES2PANDA_ASSERT(returnExpression != nullptr);
            (*returnExpression)->SetStart(startLoc);
            break;
        }
        case lexer::TokenType::LITERAL_IDENT: {
            ir::Expression *identRef = ParsePrimaryExpression();
            ES2PANDA_ASSERT(identRef->IsIdentifier());

            if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_ARROW) {
                // test exists for js extension only
                LogExpectedToken(lexer::TokenType::PUNCTUATOR_ARROW);
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
        case lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS: {
            ir::CallExpression *callExpression = ParseCallExpression(*returnExpression, false);

            if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_ARROW) {
                ir::ArrowFunctionExpression *arrowFuncExpr =
                    ParseArrowFunctionExpression(callExpression, nullptr, nullptr, true);
                arrowFuncExpr->SetStart(startLoc);

                return arrowFuncExpr;
            }

            *returnExpression = callExpression;
            break;
        }
        default: {
            break;
        }
    }

    return nullptr;
}

bool ParserImpl::ParsePotentialGenericFunctionCall([[maybe_unused]] ir::Expression *primaryExpr,
                                                   [[maybe_unused]] ir::Expression **returnExpression,
                                                   [[maybe_unused]] const lexer::SourcePosition &startLoc,
                                                   [[maybe_unused]] bool ignoreCallExpression)
{
    return true;
}

bool ParserImpl::ParsePotentialNonNullExpression([[maybe_unused]] ir::Expression **returnExpression,
                                                 [[maybe_unused]] lexer::SourcePosition startLoc)
{
    return true;
}

ir::Expression *ParserImpl::ParsePotentialAsExpression([[maybe_unused]] ir::Expression *primaryExpression)
{
    return nullptr;
}

ir::MemberExpression *ParserImpl::ParseElementAccess(ir::Expression *primaryExpr, bool isOptional)
{
    lexer_->NextToken();  // eat '['
    ir::Expression *propertyNode = ParseExpression(ExpressionParseFlags::ACCEPT_COMMA);

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET);
    }

    auto *memberExpr = AllocNode<ir::MemberExpression>(primaryExpr, propertyNode,
                                                       ir::MemberExpressionKind::ELEMENT_ACCESS, true, isOptional);
    ES2PANDA_ASSERT(memberExpr != nullptr);
    memberExpr->SetRange({primaryExpr->Start(), lexer_->GetToken().End()});
    lexer_->NextToken();
    return memberExpr;
}

ir::MemberExpression *ParserImpl::ParsePrivatePropertyAccess(ir::Expression *primaryExpr)
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_HASH_MARK);

    lexer::SourcePosition memberStart = lexer_->GetToken().Start();
    ValidatePrivateIdentifier();

    auto *privateIdent = AllocNode<ir::Identifier>(lexer_->GetToken().Ident(), Allocator());
    ES2PANDA_ASSERT(privateIdent != nullptr);
    if (program_->Extension() == util::gen::extension::ETS && privateIdent->Name().Is("prototype")) {
        LogError(diagnostic::PROTOTYPE_ACCESS);
    }
    privateIdent->SetRange({memberStart, lexer_->GetToken().End()});
    privateIdent->SetPrivate(true);
    lexer_->NextToken();

    auto *memberExpr = AllocNode<ir::MemberExpression>(primaryExpr, privateIdent,
                                                       ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    ES2PANDA_ASSERT(memberExpr != nullptr);
    memberExpr->SetRange({primaryExpr->Start(), privateIdent->End()});
    return memberExpr;
}

ir::MemberExpression *ParserImpl::ParsePropertyAccess(ir::Expression *primaryExpr, bool isOptional)
{
    ir::Identifier *ident = ExpectIdentifier(true);
    if (program_->Extension() == util::gen::extension::ETS && ident->Name().Is("prototype")) {
        LogError(diagnostic::PROTOTYPE_ACCESS);
    }
    auto *memberExpr = AllocNode<ir::MemberExpression>(primaryExpr, ident, ir::MemberExpressionKind::PROPERTY_ACCESS,
                                                       false, isOptional);
    ES2PANDA_ASSERT(memberExpr != nullptr);
    memberExpr->SetRange({primaryExpr->Start(), ident->End()});

    return memberExpr;
}

ir::Expression *ParserImpl::ParsePostPrimaryExpression(ir::Expression *primaryExpr, lexer::SourcePosition startLoc,
                                                       bool ignoreCallExpression, bool *isChainExpression)
{
    ir::Expression *returnExpr = primaryExpr;
    while (true) {
        switch (lexer_->GetToken().Type()) {
            case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET: {
                returnExpr = ParseElementAccess(returnExpr);
                continue;
            }
            case lexer::TokenType::LITERAL_IDENT: {
                if (auto *asExpression = ParsePotentialAsExpression(returnExpr); asExpression != nullptr) {
                    return asExpression;
                }
                break;
            }
            case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT:
            case lexer::TokenType::PUNCTUATOR_LESS_THAN: {
                if (ParsePotentialGenericFunctionCall(primaryExpr, &returnExpr, startLoc, ignoreCallExpression)) {
                    break;
                }
                continue;
            }
            case lexer::TokenType::PUNCTUATOR_BACK_TICK: {
                returnExpr = ParsePostPrimaryExpressionBackTick(returnExpr, startLoc);
                continue;
            }
            case lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS: {
                if (ignoreCallExpression) {
                    break;
                }
                returnExpr = ParseCallExpression(returnExpr, false);
                continue;
            }
            case lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK: {
                if (ParsePotentialNonNullExpression(&returnExpr, startLoc)) {
                    break;
                }
                continue;
            }
            default: {
                auto tmp = ParsePostPrimaryExpressionDot(returnExpr, lexer_->GetToken().Type(), isChainExpression);
                if (tmp != nullptr) {
                    returnExpr = tmp;
                    continue;
                }
            }
        }
        break;
    }
    return returnExpr;
}

ir::Expression *ParserImpl::ParsePostPrimaryExpressionBackTick(ir::Expression *returnExpression,
                                                               const lexer::SourcePosition startLoc)
{
    ir::TemplateLiteral *propertyNode = ParseTemplateLiteral();
    ES2PANDA_ASSERT(propertyNode != nullptr);
    lexer::SourcePosition endLoc = propertyNode->End();

    returnExpression = AllocNode<ir::TaggedTemplateExpression>(returnExpression, propertyNode, nullptr);
    ES2PANDA_ASSERT(returnExpression != nullptr);
    returnExpression->SetRange({startLoc, endLoc});

    return returnExpression;
}

ir::Expression *ParserImpl::ParsePostPrimaryExpressionDot(ir::Expression *returnExpression,
                                                          const lexer::TokenType tokenType, bool *isChainExpression)
{
    switch (tokenType) {
        case lexer::TokenType::PUNCTUATOR_QUESTION_DOT: {
            *isChainExpression = true;
            lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);  // eat ?.
            return ParseOptionalChain(returnExpression);
        }
        case lexer::TokenType::PUNCTUATOR_PERIOD: {
            lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);  // eat period

            if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_HASH_MARK) {
                return ParsePrivatePropertyAccess(returnExpression);
            }

            return ParsePropertyAccess(returnExpression);
        }
        default: {
            return nullptr;
        }
    }
}

void ParserImpl::ValidateUpdateExpression(ir::Expression *returnExpression, bool isChainExpression)
{
    if ((!returnExpression->IsMemberExpression() && !returnExpression->IsIdentifier() &&
         !returnExpression->IsTSNonNullExpression()) ||
        isChainExpression) {
        LogError(diagnostic::INVALID_LEFT_SITE_OPERATOR);
    }

    if (returnExpression->IsIdentifier()) {
        const util::StringView &returnExpressionStr = returnExpression->AsIdentifier()->Name();

        if (returnExpressionStr.Is("eval")) {
            LogError(diagnostic::ASSIGN_TO_EVAL_INVALID);
        }

        if (returnExpressionStr.Is("arguments")) {
            LogError(diagnostic::ASSIGN_TO_ARGS_INVALID);
        }
    }
}

ir::Expression *ParserImpl::SetupChainExpr(ir::Expression *const top, lexer::SourcePosition startLoc)
{
    auto expr = top;
    while (expr->IsTSNonNullExpression()) {
        expr = expr->AsTSNonNullExpression()->Expr();
    }
    auto chainParent = expr->Parent();

    lexer::SourcePosition endLoc = expr->End();
    auto chain = AllocNode<ir::ChainExpression>(expr);
    ES2PANDA_ASSERT(chain != nullptr);
    chain->SetRange({startLoc, endLoc});

    if (expr == top) {
        return chain;
    }
    chainParent->AsTSNonNullExpression()->SetExpr(chain);
    chain->SetParent(chainParent);
    return top;
}

ir::Expression *ParserImpl::ParseMemberExpression(bool ignoreCallExpression, ExpressionParseFlags flags)
{
    bool isAsync = lexer_->GetToken().IsAsyncModifier();
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    ir::Expression *returnExpression = ParsePrimaryExpression(flags);
    if (lexer_->GetToken().NewLine() && returnExpression->IsArrowFunctionExpression()) {
        return returnExpression;
    }

    if (isAsync && !lexer_->GetToken().NewLine()) {
        context_.Status() |= ParserStatus::ASYNC_FUNCTION;
        ir::ArrowFunctionExpression *arrow = ParsePotentialArrowExpression(&returnExpression, startLoc);

        if (arrow != nullptr) {
            return arrow;
        }
    }

    bool isChainExpression;
    ir::Expression *prevExpression;
    do {
        isChainExpression = false;
        prevExpression = returnExpression;
        returnExpression =
            ParsePostPrimaryExpression(returnExpression, startLoc, ignoreCallExpression, &isChainExpression);

        if (isChainExpression) {
            returnExpression = SetupChainExpr(returnExpression, startLoc);
        }
    } while (prevExpression != returnExpression);

    if (!lexer_->GetToken().NewLine() && lexer::Token::IsUpdateToken(lexer_->GetToken().Type())) {
        lexer::SourcePosition start = returnExpression->Start();

        ValidateUpdateExpression(returnExpression, isChainExpression);

        returnExpression = AllocNode<ir::UpdateExpression>(returnExpression, lexer_->GetToken().Type(), false);

        ES2PANDA_ASSERT(returnExpression != nullptr);
        returnExpression->SetRange({start, lexer_->GetToken().End()});
        lexer_->NextToken();
    }

    return returnExpression;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ParserImpl::ParsePatternElement(ExpressionParseFlags flags, bool allowDefault)
{
    ir::Expression *returnNode = nullptr;

    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET: {
            returnNode = ParseArrayExpression(ExpressionParseFlags::MUST_BE_PATTERN);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD: {
            if ((flags & ExpressionParseFlags::IN_REST) != 0) {
                LogError(diagnostic::UNEXPECTED_TOKEN);
            }
            returnNode = ParseSpreadElement(ExpressionParseFlags::MUST_BE_PATTERN);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_BRACE: {
            returnNode =
                ParseObjectExpression(ExpressionParseFlags::MUST_BE_PATTERN | ExpressionParseFlags::OBJECT_PATTERN);
            break;
        }
        case lexer::TokenType::LITERAL_IDENT: {
            returnNode = AllocNode<ir::Identifier>(lexer_->GetToken().Ident(), Allocator());
            returnNode->SetRange(lexer_->GetToken().Loc());
            lexer_->NextToken();
            break;
        }
        default: {
            LogError(diagnostic::UNEXPECTED_TOKEN);
            returnNode = AllocBrokenExpression(Lexer()->GetToken().Loc());
        }
    }

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        return returnNode;
    }

    ParsePatternElementErrorCheck(flags, allowDefault);

    ir::Expression *rightNode = ParseExpression();
    auto *assignmentExpression = AllocNode<ir::AssignmentExpression>(
        ir::AstNodeType::ASSIGNMENT_PATTERN, returnNode, rightNode, lexer::TokenType::PUNCTUATOR_SUBSTITUTION);
    ES2PANDA_ASSERT(assignmentExpression != nullptr);
    assignmentExpression->SetRange({returnNode->Start(), rightNode->End()});

    return assignmentExpression;
}

void ParserImpl::ParsePatternElementErrorCheck(const ExpressionParseFlags flags, const bool allowDefault)
{
    if ((flags & ExpressionParseFlags::IN_REST) != 0) {
        // unexpected_token_10.js
        LogExpectedToken(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);
    }

    if (!allowDefault) {
        LogError(diagnostic::INVALID_DESTRUCTURING_TARGET);
    }

    lexer_->NextToken();

    if (context_.IsGenerator() && lexer_->GetToken().Type() == lexer::TokenType::KEYW_YIELD) {
        LogError(diagnostic::YIELD_IN_GENERATOR_PARAM);
    }

    if (context_.IsAsync() && lexer_->GetToken().Type() == lexer::TokenType::KEYW_AWAIT) {
        LogError(diagnostic::ILLEGAL_AWAIT_IN_ASYNC_FUN);
    }
}

void ParserImpl::CheckPropertyKeyAsyncModifier(ParserStatus *methodStatus)
{
    const auto asyncPos = lexer_->Save();
    lexer_->NextToken();

    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS &&
        lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON &&
        lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COMMA &&
        lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
        if (lexer_->GetToken().NewLine()) {
            LogError(diagnostic::ASYNC_METHOD_LINE_TERM);
        }

        *methodStatus |= ParserStatus::ASYNC_FUNCTION;
    } else {
        lexer_->Rewind(asyncPos);
    }
}

static bool IsAccessorDelimiter(char32_t cp)
{
    switch (cp) {
        case lexer::LEX_CHAR_LEFT_PAREN:
        case lexer::LEX_CHAR_COLON:
        case lexer::LEX_CHAR_COMMA:
        case lexer::LEX_CHAR_RIGHT_BRACE: {
            return true;
        }
        default: {
            return false;
        }
    }
}

static bool IsShorthandDelimiter(char32_t cp)
{
    switch (cp) {
        case lexer::LEX_CHAR_EQUALS:
        case lexer::LEX_CHAR_COMMA:
        case lexer::LEX_CHAR_RIGHT_BRACE: {
            return true;
        }
        default: {
            return false;
        }
    }
}

void ParserImpl::ValidateAccessor(ExpressionParseFlags flags)
{
    if ((flags & ExpressionParseFlags::MUST_BE_PATTERN) != 0) {
        LogError(diagnostic::UNEXPECTED_TOKEN);
    }
}

ir::Property *ParserImpl::ParseShorthandProperty(const lexer::LexerPosition *startPos)
{
    char32_t nextCp = lexer_->Lookahead();
    lexer::TokenType keywordType = lexer_->GetToken().KeywordType();

    /* Rewind the lexer to the beginning of the ident to reparse as common
     * identifier */
    lexer_->Rewind(*startPos);
    lexer_->NextToken();
    lexer::SourcePosition start = lexer_->GetToken().Start();

    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        // for now no tests are available since the func is called once
        // after checking the same token for the identifier
        LogExpectedToken(lexer::TokenType::LITERAL_IDENT);
    }

    const util::StringView &ident = lexer_->GetToken().Ident();

    auto *key = AllocNode<ir::Identifier>(ident, Allocator());
    ES2PANDA_ASSERT(key != nullptr);
    key->SetRange(lexer_->GetToken().Loc());

    ir::Expression *value = AllocNode<ir::Identifier>(ident, Allocator());
    value->SetRange(lexer_->GetToken().Loc());

    lexer::SourcePosition end;

    if (nextCp == lexer::LEX_CHAR_EQUALS) {
        CheckRestrictedBinding(keywordType);

        lexer_->NextToken();  // substitution
        lexer_->NextToken();  // eat substitution

        ir::Expression *rightNode = ParseExpression();

        auto *assignmentExpression = AllocNode<ir::AssignmentExpression>(
            ir::AstNodeType::ASSIGNMENT_PATTERN, value, rightNode, lexer::TokenType::PUNCTUATOR_SUBSTITUTION);
        ES2PANDA_ASSERT(assignmentExpression != nullptr);
        assignmentExpression->SetRange({value->Start(), rightNode->End()});
        end = rightNode->End();
        value = assignmentExpression;
    } else {
        end = lexer_->GetToken().End();
        lexer_->NextToken();
    }

    auto *returnProperty = AllocNode<ir::Property>(key, value);
    ES2PANDA_ASSERT(returnProperty != nullptr);
    returnProperty->SetRange({start, end});

    return returnProperty;
}

bool ParserImpl::ParsePropertyModifiers(ExpressionParseFlags flags, ir::PropertyKind *propertyKind,
                                        ParserStatus *methodStatus)
{
    if (lexer_->GetToken().IsAsyncModifier()) {
        CheckPropertyKeyAsyncModifier(methodStatus);
    }

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MULTIPLY) {
        if ((flags & ExpressionParseFlags::MUST_BE_PATTERN) != 0) {
            LogError(diagnostic::UNEXPECTED_TOKEN);
        }

        lexer_->NextToken();
        *methodStatus |= ParserStatus::GENERATOR_FUNCTION;
    }

    char32_t nextCp = lexer_->Lookahead();
    lexer::TokenType keywordType = lexer_->GetToken().KeywordType();
    // Parse getter property
    if (keywordType == lexer::TokenType::KEYW_GET && !IsAccessorDelimiter(nextCp)) {
        ValidateAccessor(flags);

        *propertyKind = ir::PropertyKind::GET;
        lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);

        return false;
    }

    // Parse setter property
    if (keywordType == lexer::TokenType::KEYW_SET && !IsAccessorDelimiter(nextCp)) {
        ValidateAccessor(flags);

        *propertyKind = ir::PropertyKind::SET;
        lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);

        return false;
    }

    // Parse shorthand property or assignment pattern
    return (IsShorthandDelimiter(nextCp) && (*methodStatus & ParserStatus::ASYNC_FUNCTION) == 0);
}

void ParserImpl::ParseGeneratorPropertyModifier(ExpressionParseFlags flags, ParserStatus *methodStatus)
{
    if ((flags & ExpressionParseFlags::MUST_BE_PATTERN) != 0) {
        LogError(diagnostic::UNEXPECTED_TOKEN);
    }

    lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
    *methodStatus |= ParserStatus::GENERATOR_FUNCTION;
}

ir::Expression *ParserImpl::ParsePropertyKey(ExpressionParseFlags flags)
{
    ir::Expression *key = nullptr;

    switch (lexer_->GetToken().Type()) {
        case lexer::TokenType::LITERAL_IDENT: {
            const util::StringView &ident = lexer_->GetToken().Ident();
            key = AllocNode<ir::Identifier>(ident, Allocator());
            ES2PANDA_ASSERT(key != nullptr);
            key->SetRange(lexer_->GetToken().Loc());
            lexer_->NextToken();
            return key;
        }
        case lexer::TokenType::LITERAL_STRING: {
            const util::StringView &string = lexer_->GetToken().String();
            key = AllocNode<ir::StringLiteral>(string);
            ES2PANDA_ASSERT(key != nullptr);
            key->SetRange(lexer_->GetToken().Loc());
            lexer_->NextToken();
            return key;
        }
        case lexer::TokenType::LITERAL_NUMBER: {
            if ((lexer_->GetToken().Flags() & lexer::TokenFlags::NUMBER_BIGINT) != 0) {
                key = AllocNode<ir::BigIntLiteral>(lexer_->GetToken().BigInt());
            } else {
                key = AllocNode<ir::NumberLiteral>(lexer_->GetToken().GetNumber());
            }
            ES2PANDA_ASSERT(key != nullptr);
            key->SetRange(lexer_->GetToken().Loc());
            lexer_->NextToken();
            return key;
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET: {
            lexer_->NextToken();  // eat left square bracket

            key = ParseExpression(flags | ExpressionParseFlags::ACCEPT_COMMA);

            if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET) {
                // the same test for ts, ets and js extension
                // unexpected_token_52.ets
                LogExpectedToken(lexer::TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET);
            }
            lexer_->NextToken();
            return key;
        }
        default: {
            const auto &rangeToken = Lexer()->GetToken().Loc();
            LogError(diagnostic::UNEXPECTED_TOKEN);
            lexer_->NextToken();
            return AllocBrokenExpression(rangeToken);
        }
    }
}

ir::Expression *ParserImpl::ParsePropertyValue(const ir::PropertyKind *propertyKind, const ParserStatus *methodStatus,
                                               ExpressionParseFlags flags)
{
    bool isMethod = (*methodStatus & ParserStatus::FUNCTION) != 0;
    bool inPattern = (flags & ExpressionParseFlags::MUST_BE_PATTERN) != 0;

    if (!isMethod && !ir::Property::IsAccessorKind(*propertyKind)) {
        // If the actual property is not getter/setter nor method, the following
        // token must be ':'
        if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COLON) {
            if (lexer::Token::IsPunctuatorToken(lexer_->GetToken().Type())) {
                LogExpectedToken(lexer::TokenType::PUNCTUATOR_COLON);
                lexer_->NextToken();  // eat colon
            } else {
                // we just missed ':'
                LogError(diagnostic::UNEXPECTED_TOKEN);
            }
        } else {
            lexer_->NextToken();  // eat colon
        }

        if (!inPattern) {
            return ParseExpression(flags);
        }

        return ParsePatternElement();
    }

    if (inPattern) {
        LogError(diagnostic::OBJECT_PATTER_CONTAIN_METHODS);
    }

    ParserStatus newStatus = *methodStatus | ParserStatus::FUNCTION | ParserStatus::ALLOW_SUPER;

    if (*propertyKind != ir::PropertyKind::SET) {
        newStatus |= ParserStatus::NEED_RETURN_TYPE;
    }

    ir::ScriptFunction *methodDefinitonNode = ParseFunction(newStatus);
    ES2PANDA_ASSERT(methodDefinitonNode != nullptr);
    methodDefinitonNode->AddFlag(ir::ScriptFunctionFlags::METHOD);

    size_t paramsSize = methodDefinitonNode->Params().size();

    auto *value = AllocNode<ir::FunctionExpression>(methodDefinitonNode);
    ES2PANDA_ASSERT(value != nullptr);
    value->SetRange(methodDefinitonNode->Range());

    if (*propertyKind == ir::PropertyKind::SET && paramsSize != 1) {
        LogError(diagnostic::SETTER_FORMAL_PARAMS);
    }

    if (*propertyKind == ir::PropertyKind::GET && paramsSize != 0) {
        LogError(diagnostic::GETTER_FORMAL_PARAMS);
    }

    return value;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ParserImpl::ParsePropertyDefinition([[maybe_unused]] ExpressionParseFlags flags)
{
    ir::PropertyKind propertyKind = ir::PropertyKind::INIT;
    ParserStatus methodStatus = ParserStatus::NO_OPTS;

    const auto startPos = lexer_->Save();
    lexer_->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
    lexer::SourcePosition start = lexer_->GetToken().Start();

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD) {
        return ParseSpreadElement(flags);
    }

    if (lexer_->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
        if (ParsePropertyModifiers(flags, &propertyKind, &methodStatus)) {
            return ParseShorthandProperty(&startPos);
        }
    } else if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MULTIPLY) {
        ParseGeneratorPropertyModifier(flags, &methodStatus);
    }

    bool isComputed = lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET;
    ir::Expression *key = ParsePropertyKey(flags);

    // Parse method property
    if ((lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS ||
         lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) &&
        !ir::Property::IsAccessorKind(propertyKind)) {
        methodStatus |= ParserStatus::FUNCTION | ParserStatus::ALLOW_SUPER;
        propertyKind = ir::PropertyKind::INIT;
    } else if ((methodStatus & (ParserStatus::GENERATOR_FUNCTION | ParserStatus::ASYNC_FUNCTION)) != 0) {
        LogError(diagnostic::UNEXPECTED_ID);
    }

    ir::Expression *value = ParsePropertyValue(&propertyKind, &methodStatus, flags);
    ES2PANDA_ASSERT(value != nullptr);
    lexer::SourcePosition end = value->End();

    auto *returnProperty =
        AllocNode<ir::Property>(propertyKind, key, value, methodStatus != ParserStatus::NO_OPTS, isComputed);
    ES2PANDA_ASSERT(returnProperty != nullptr);
    returnProperty->SetRange({start, end});

    return returnProperty;
}

void ParserImpl::ParsePropertyEndErrorHendler()
{
    const auto pos = lexer_->Save();
    if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT) {
        lexer_->NextToken();
        if (lexer_->GetToken().Type() == lexer::TokenType::LITERAL_IDENT) {
            // {a. b} -> {a, b}
            lexer_->Rewind(pos);
            lexer_->GetToken().SetTokenType(lexer::TokenType::PUNCTUATOR_COMMA);
        } else {
            lexer_->Rewind(pos);
            if (lexer::Token::IsPunctuatorToken(lexer_->GetToken().Type())) {
                // {a, b) -> {a, b}
                lexer_->GetToken().SetTokenType(lexer::TokenType::PUNCTUATOR_RIGHT_BRACE);
            }
        }
    } else {
        lexer_->NextToken();  // {a b} -> we just skip the token
        if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
            lexer_->GetToken().SetTokenType(lexer::TokenType::PUNCTUATOR_RIGHT_BRACE);
        }
    }
}

bool ParserImpl::ParsePropertyEnd()
{
    // Property definiton must end with ',' or '}' otherwise we report SyntaxError
    if (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_COMMA &&
        lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
        // test_object_literal3.ts
        LogError(diagnostic::UNEXPECTED_TOKEN);
        ParsePropertyEndErrorHendler();
    }

    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COMMA &&
        lexer_->Lookahead() == lexer::LEX_CHAR_RIGHT_BRACE) {
        lexer_->NextToken();
        return true;
    }

    return false;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::ObjectExpression *ParserImpl::ParseObjectExpression(ExpressionParseFlags flags)
{
    ES2PANDA_ASSERT(lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_BRACE);
    lexer::SourcePosition start = lexer_->GetToken().Start();
    ArenaVector<ir::Expression *> properties(Allocator()->Adapter());
    bool trailingComma = false;
    bool inPattern = (flags & ExpressionParseFlags::MUST_BE_PATTERN) != 0;

    if (lexer_->Lookahead() == lexer::LEX_CHAR_RIGHT_BRACE) {
        lexer_->NextToken();
    }

    while (lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE &&
           lexer_->GetToken().Type() != lexer::TokenType::EOS) {
        util::ErrorRecursionGuard infiniteLoopBlocker(lexer_);
        ir::Expression *property = ParsePropertyDefinition(flags | ExpressionParseFlags::POTENTIALLY_IN_PATTERN);
        if (property != nullptr) {
            properties.push_back(property);
        }

        trailingComma = ParsePropertyEnd();
    }

    auto nodeType = inPattern ? ir::AstNodeType::OBJECT_PATTERN : ir::AstNodeType::OBJECT_EXPRESSION;
    auto *objectExpression =
        AllocNode<ir::ObjectExpression>(nodeType, Allocator(), std::move(properties), trailingComma);
    ES2PANDA_ASSERT(objectExpression != nullptr);
    objectExpression->SetRange({start, lexer_->GetToken().End()});
    lexer_->NextToken();

    if (inPattern) {
        objectExpression->SetDeclaration();
    }

    if ((flags & ExpressionParseFlags::POTENTIALLY_IN_PATTERN) == 0) {
        if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION &&
            !objectExpression->ConvertibleToObjectPattern()) {
            LogError(diagnostic::INVALID_LEFT_SIDE_ARRAY_DESTRUCTURING, {}, objectExpression->Start());
        } else if (!inPattern && lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
            ir::ValidationInfo info = ValidateExpression(objectExpression);
            if (info.Fail()) {
                LogSyntaxError(info.msg.Utf8(), info.pos);
            }
        }
    }

    return objectExpression;
}

ir::SequenceExpression *ParserImpl::ParseSequenceExpression(ir::Expression *startExpr, bool acceptRest)
{
    lexer::SourcePosition start = startExpr->Start();

    ArenaVector<ir::Expression *> sequence(Allocator()->Adapter());
    sequence.push_back(startExpr);

    while (lexer_->TryEatTokenType(lexer::TokenType::PUNCTUATOR_COMMA)) {
        if (acceptRest && lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD) {
            ir::SpreadElement *expr = ParseSpreadElement(ExpressionParseFlags::MUST_BE_PATTERN);
            sequence.push_back(expr);
            break;
        }

        if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS && lexer_->CheckArrow()) {
            break;
        }

        auto expr = ParseExpression();
        sequence.push_back(expr);
    }

    lexer::SourcePosition end = sequence.back()->End();
    auto *sequenceNode = AllocNode<ir::SequenceExpression>(std::move(sequence));
    ES2PANDA_ASSERT(sequenceNode != nullptr);
    sequenceNode->SetRange({start, end});

    return sequenceNode;
}

// NOLINTNEXTLINE(google-default-arguments)
ir::Expression *ParserImpl::ParseUnaryOrPrefixUpdateExpression(ExpressionParseFlags flags)
{
    if (!lexer_->GetToken().IsUnary()) {
        return ParseLeftHandSideExpression(flags);
    }

    lexer::TokenType operatorType = lexer_->GetToken().Type();
    lexer::SourcePosition start = lexer_->GetToken().Start();
    lexer_->NextToken();

    ir::Expression *argument =
        lexer_->GetToken().IsUnary() ? ParseUnaryOrPrefixUpdateExpression() : ParseLeftHandSideExpression();

    if (lexer::Token::IsUpdateToken(operatorType)) {
        if (!argument->IsIdentifier() && !argument->IsMemberExpression() && !argument->IsTSNonNullExpression()) {
            LogError(diagnostic::INVALID_LEFT_IN_PREFIX);
        }

        if (argument->IsIdentifier()) {
            const util::StringView &argumentStr = argument->AsIdentifier()->Name();

            if (argumentStr.Is("eval")) {
                LogError(diagnostic::ASSIGN_TO_EVAL_INVALID);
            } else if (argumentStr.Is("arguments")) {
                LogError(diagnostic::ASSIGN_TO_ARGS_INVALID);
            }
        }
    }

    if (operatorType == lexer::TokenType::KEYW_DELETE) {
        if (argument->IsIdentifier()) {
            LogError(diagnostic::DELETING_LOCAL_VAR);
        }

        if (argument->IsMemberExpression() && argument->AsMemberExpression()->Property()->IsIdentifier() &&
            argument->AsMemberExpression()->Property()->AsIdentifier()->IsPrivateIdent()) {
            LogError(diagnostic::DELETING_PRIVATE_FIELDS);
        }
    }

    lexer::SourcePosition end = argument->End();

    ir::Expression *returnExpr = nullptr;

    if (lexer::Token::IsUpdateToken(operatorType)) {
        returnExpr = AllocNode<ir::UpdateExpression>(argument, operatorType, true);
    } else if (operatorType == lexer::TokenType::KEYW_AWAIT) {
        returnExpr = AllocNode<ir::AwaitExpression>(argument);
    } else if (operatorType == lexer::TokenType::KEYW_TYPEOF) {
        returnExpr = AllocNode<ir::TypeofExpression>(argument);
    } else {
        returnExpr = AllocNode<ir::UnaryExpression>(argument, operatorType);
    }

    ES2PANDA_ASSERT(returnExpr != nullptr);
    returnExpr->SetRange({start, end});

    return returnExpr;
}

ir::Expression *ParserImpl::ParseImportExpression()
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    lexer::SourcePosition endLoc = lexer_->GetToken().End();
    lexer_->NextToken();  // eat import

    // parse import.Meta
    if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD) {
        if (!context_.IsModule()) {
            LogError(diagnostic::IMPORT_META_ONLY_MODULE);
        } else if (GetOptions().IsDirectEval()) {
            LogError(diagnostic::IMPORT_META_DIRECT_EVAL);
        }

        lexer_->NextToken();  // eat dot

        if (lexer_->GetToken().Type() != lexer::TokenType::LITERAL_IDENT ||
            lexer_->GetToken().KeywordType() != lexer::TokenType::KEYW_META) {
            // js tests only: import_meta_1.js, import_meta_2.js, import_meta_3.js
            LogError(diagnostic::META_PROP_FOR_IMPORT);

            lexer_->GetToken().SetTokenType(lexer::TokenType::KEYW_META);
        }

        auto *metaProperty = AllocNode<ir::MetaProperty>(ir::MetaProperty::MetaPropertyKind::IMPORT_META);
        ES2PANDA_ASSERT(metaProperty != nullptr);
        metaProperty->SetRange({startLoc, endLoc});

        lexer_->NextToken();
        return metaProperty;
    }

    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS);

    ir::Expression *source = ParseExpression();

    lexer::SourcePosition endImportLoc = lexer_->GetToken().End();
    ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS);

    auto *importExpression = AllocNode<ir::ImportExpression>(source);
    ES2PANDA_ASSERT(importExpression != nullptr);
    importExpression->SetRange({startLoc, endImportLoc});

    return importExpression;
}

bool ParserImpl::IsNamedFunctionExpression()
{
    return lexer_->GetToken().Type() != lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS;
}

ir::FunctionExpression *ParserImpl::ParseFunctionExpression(ParserStatus newStatus)
{
    lexer::SourcePosition startLoc = lexer_->GetToken().Start();
    ir::Identifier *ident = nullptr;

    if ((newStatus & ParserStatus::ARROW_FUNCTION) == 0) {
        ParserStatus savedStatus = context_.Status();
        context_.Status() |= static_cast<ParserStatus>(newStatus & ParserStatus::ASYNC_FUNCTION);

        lexer_->NextToken();

        if (lexer_->GetToken().Type() == lexer::TokenType::PUNCTUATOR_MULTIPLY) {
            newStatus |= ParserStatus::GENERATOR_FUNCTION;
            lexer_->NextToken();
        }

        if (IsNamedFunctionExpression()) {
            CheckRestrictedBinding(lexer_->GetToken().KeywordType());
            ident = ExpectIdentifier(false, true);
        }

        context_.Status() = savedStatus;
    }

    ir::ScriptFunction *functionNode = ParseFunction(newStatus);
    ES2PANDA_ASSERT(functionNode != nullptr);
    functionNode->SetStart(startLoc);

    auto *funcExpr = AllocNode<ir::FunctionExpression>(ident, functionNode);
    ES2PANDA_ASSERT(funcExpr != nullptr);
    funcExpr->SetRange(functionNode->Range());

    return funcExpr;
}

}  // namespace ark::es2panda::parser
