/*
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

#include "constantExpressionLowering.h"
#include <cmath>
#include <cstdint>

#include "checker/ETSchecker.h"
#include "checker/types/typeError.h"
#include "compiler/lowering/util.h"
#include "ir/expressions/literals/undefinedLiteral.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "util/helpers.h"

namespace ark::es2panda::compiler {

constexpr static char32_t MAX_CHAR = 0xFFFF;

static ir::BooleanLiteral *CreateBooleanLiteral(bool val, ir::AstNode *parent, const lexer::SourceRange &loc,
                                                ArenaAllocator *allocator)
{
    auto resNode = util::NodeAllocator::Alloc<ir::BooleanLiteral>(allocator, val);
    resNode->SetParent(parent);
    resNode->SetRange(loc);
    resNode->SetFolded();
    return resNode;
}

template <typename T>
static ir::NumberLiteral *CreateNumberLiteral(T val, ir::AstNode *parent, const lexer::SourceRange &loc,
                                              ArenaAllocator *allocator)
{
    auto resNum = lexer::Number(val);

    auto *resNode = util::NodeAllocator::Alloc<ir::NumberLiteral>(allocator, resNum);

    // Some hack to set string representation of lexer::Number
    resNode->Number().SetStr(util::UString(resNode->ToString(), allocator).View());

    resNode->SetParent(parent);
    resNode->SetRange(loc);
    resNode->SetFolded();
    return resNode;
}

static ir::Identifier *CreateErrorIdentifier(const ir::AstNode *node, ArenaAllocator *allocator)
{
    // Creating Identifier without passing any arguments leads to creating Error Identifier with *ERROR_LITERAL*
    auto res = util::NodeAllocator::Alloc<ir::Identifier>(allocator, allocator);

    res->SetParent(const_cast<ir::AstNode *>(node)->Parent());
    res->SetRange(node->Range());
    return res;
}

static ir::CharLiteral *CreateCharLiteral(char16_t val, ir::AstNode *parent, const lexer::SourceRange &loc,
                                          ArenaAllocator *allocator)
{
    auto *result = util::NodeAllocator::Alloc<ir::CharLiteral>(allocator, val);
    result->SetParent(parent);
    result->SetRange(loc);
    result->SetFolded();
    return result;
}

static ir::PrimitiveType TypeRankToPrimitiveType(TypeRank tr)
{
    switch (tr) {
        case TypeRank::CHAR:
            return ir::PrimitiveType::CHAR;
        case TypeRank::INT8:
            return ir::PrimitiveType::BYTE;
        case TypeRank::INT16:
            return ir::PrimitiveType::SHORT;
        case TypeRank::INT32:
            return ir::PrimitiveType::INT;
        case TypeRank::INT64:
            return ir::PrimitiveType::LONG;
        case TypeRank::FLOAT:
            return ir::PrimitiveType::FLOAT;
        case TypeRank::DOUBLE:
            return ir::PrimitiveType::DOUBLE;
    }
    ES2PANDA_UNREACHABLE();
}

static TypeRank GetTypeRank(const ir::Literal *literal)
{
    if (literal->IsCharLiteral()) {
        return TypeRank::CHAR;
    }
    if (literal->IsNumberLiteral()) {
        auto number = literal->AsNumberLiteral()->Number();
        if (number.IsByte()) {
            return TypeRank::INT8;
        }
        if (number.IsShort()) {
            return TypeRank::INT16;
        }
        if (number.IsInt()) {
            return TypeRank::INT32;
        }
        if (number.IsLong()) {
            return TypeRank::INT64;
        }
        if (number.IsFloat()) {
            return TypeRank::FLOAT;
        }
        if (number.IsDouble()) {
            return TypeRank::DOUBLE;
        }
    }
    ES2PANDA_UNREACHABLE();
}

template <typename TargetType>
static TargetType GetVal(const ir::Literal *node)
{
    if constexpr (std::is_same_v<TargetType, bool>) {
        ES2PANDA_ASSERT(node->IsBooleanLiteral());
        return node->AsBooleanLiteral()->Value();
    }

    if constexpr (std::is_same_v<TargetType, char16_t>) {
        ES2PANDA_ASSERT(node->IsCharLiteral());
        return node->AsCharLiteral()->Char();
    }

    ES2PANDA_ASSERT(node->IsNumberLiteral());

    auto numNode = node->AsNumberLiteral();
    if constexpr (std::is_same_v<TargetType, int8_t>) {
        ES2PANDA_ASSERT(numNode->Number().IsByte());
        return numNode->Number().GetByte();
    }
    if constexpr (std::is_same_v<TargetType, int16_t>) {
        ES2PANDA_ASSERT(numNode->Number().IsShort());
        return numNode->Number().GetShort();
    }
    if constexpr (std::is_same_v<TargetType, int32_t>) {
        ES2PANDA_ASSERT(numNode->Number().IsInt());
        return numNode->Number().GetInt();
    }
    if constexpr (std::is_same_v<TargetType, int64_t>) {
        ES2PANDA_ASSERT(numNode->Number().IsLong());
        return numNode->Number().GetLong();
    }
    if constexpr (std::is_same_v<TargetType, float>) {
        ES2PANDA_ASSERT(numNode->Number().IsFloat());
        return numNode->Number().GetFloat();
    }
    if constexpr (std::is_same_v<TargetType, double>) {
        ES2PANDA_ASSERT(numNode->Number().IsDouble());
        return numNode->Number().GetDouble();
    }
    ES2PANDA_UNREACHABLE();
}

template <typename To>
static To CastValTo(const ir::Literal *lit)
{
    if (lit->IsBooleanLiteral()) {
        return static_cast<To>(GetVal<bool>(lit));
    }

    ES2PANDA_ASSERT(lit->IsNumberLiteral() || lit->IsCharLiteral());

    auto rank = GetTypeRank(lit);
    switch (rank) {
        case TypeRank::DOUBLE:
            return static_cast<To>(GetVal<double>(lit));
        case TypeRank::FLOAT:
            return static_cast<To>(GetVal<float>(lit));
        case TypeRank::INT64:
            return static_cast<To>(GetVal<int64_t>(lit));
        case TypeRank::INT32:
            return static_cast<To>(GetVal<int32_t>(lit));
        case TypeRank::INT16:
            return static_cast<To>(GetVal<int16_t>(lit));
        case TypeRank::INT8:
            return static_cast<To>(GetVal<int8_t>(lit));
        case TypeRank::CHAR:
            return static_cast<To>(GetVal<char16_t>(lit));
    }

    ES2PANDA_UNREACHABLE();
}

static bool IsConvertibleToNumericType(const ir::Literal *lit)
{
    return lit->IsCharLiteral() || lit->IsNumberLiteral();
}

static void LogError(public_lib::Context *context, const diagnostic::DiagnosticKind &diagnostic,
                     const util::DiagnosticMessageParams &diagnosticParams, const lexer::SourcePosition &pos)
{
    context->diagnosticEngine->LogDiagnostic(diagnostic, diagnosticParams, pos);
}

static bool IsCorrectNumberLiteral(const ir::AstNode *lit)
{
    if (!lit->IsNumberLiteral()) {
        return false;
    }

    return !lit->AsNumberLiteral()->Number().ConversionError();
}

static bool IsSupportedLiteral(ir::Expression *const node)
{
    if (!node->IsLiteral()) {
        return false;
    }

    auto literal = node->AsLiteral();
    return IsCorrectNumberLiteral(literal) || literal->IsCharLiteral() || literal->IsBooleanLiteral() ||
           literal->IsStringLiteral();
}

template <typename To>
static ir::AstNode *CommonCastNumberLiteralTo(const ir::Literal *num, ArenaAllocator *allocator)
{
    auto parent = const_cast<ir::Literal *>(num)->Parent();

    if constexpr (std::is_same_v<To, char16_t>) {
        return CreateCharLiteral(CastValTo<char16_t>(num), parent, num->Range(), allocator);
    }

    return CreateNumberLiteral(CastValTo<To>(num), parent, num->Range(), allocator);
}

template <typename From, typename To>
static ir::AstNode *FloatingPointNumberLiteralCast(const ir::Literal *num, public_lib::Context *context)
{
    if (sizeof(From) > sizeof(To)) {
        // double -> float
        auto doubleVal = GetVal<double>(num);
        if (doubleVal < std::numeric_limits<float>::min() || doubleVal > std::numeric_limits<float>::max()) {
            LogError(context, diagnostic::CONSTANT_VALUE_OUT_OF_RANGE, {}, num->Start());
            return const_cast<ir::Literal *>(num);
        }

        auto floatVal = static_cast<float>(doubleVal);
        if (static_cast<double>(floatVal) == doubleVal) {
            auto parent = const_cast<ir::Literal *>(num)->Parent();
            return CreateNumberLiteral(floatVal, parent, num->Range(), context->allocator);
        }

        LogError(context, diagnostic::CONSTANT_FLOATING_POINT_COVERSION, {}, num->Start());
        return const_cast<ir::Literal *>(num);
    }

    // float -> double
    return CommonCastNumberLiteralTo<To>(num, context->allocator);
}

template <typename From, typename To>
static ir::AstNode *NarrowingNumberLiteralCast(const ir::Literal *num, public_lib::Context *context)
{
    auto maxTo = std::numeric_limits<To>::max();
    auto minTo = std::numeric_limits<To>::min();
    auto val = GetVal<From>(num);
    if (val < minTo || val > maxTo) {
        LogError(context, diagnostic::CONSTANT_VALUE_OUT_OF_RANGE, {}, num->Start());
        return const_cast<ir::Literal *>(num);
    }

    return CommonCastNumberLiteralTo<To>(num, context->allocator);
}

template <typename From, typename To>
static ir::AstNode *IntegralNumberLiteralCast(const ir::Literal *num, public_lib::Context *context)
{
    if (sizeof(From) > sizeof(To)) {
        return NarrowingNumberLiteralCast<From, To>(num, context);
    }

    // Widening
    return CommonCastNumberLiteralTo<To>(num, context->allocator);
}

template <typename From, typename To>
static ir::AstNode *CastNumberOrCharLiteralFromTo(const ir::Literal *num, public_lib::Context *context)
{
    if constexpr (std::is_same_v<From, To>) {
        return const_cast<ir::Literal *>(num);
    }

    if constexpr (std::is_floating_point_v<From> && std::is_floating_point_v<To>) {
        return FloatingPointNumberLiteralCast<From, To>(num, context);
    }

    if constexpr (std::is_integral_v<From> && std::is_integral_v<To>) {
        return IntegralNumberLiteralCast<From, To>(num, context);
    }

    if constexpr (std::is_integral_v<From> && std::is_floating_point_v<To>) {
        // integral -> floating point (widening)
        return CommonCastNumberLiteralTo<To>(num, context->allocator);
    }

    if constexpr (std::is_floating_point_v<From> && std::is_integral_v<To>) {
        // Constant narrowing floating point conversion is not permitted
        LogError(context, diagnostic::CONSTANT_FLOATING_POINT_COVERSION, {}, num->Start());
        return const_cast<ir::Literal *>(num);
    }

    ES2PANDA_UNREACHABLE();
}

template <typename From>
static ir::AstNode *CastNumberOrCharLiteralFrom(const ir::Literal *lit, ir::PrimitiveType type,
                                                public_lib::Context *context)
{
    switch (type) {
        case ir::PrimitiveType::BOOLEAN:
            // Note: we do nothing for `class A {b5 : boolean = 7;}` here, type error will be thrown in checker.
            return const_cast<ir::Literal *>(lit);
        case ir::PrimitiveType::CHAR:
            return CastNumberOrCharLiteralFromTo<From, char16_t>(lit, context);
        case ir::PrimitiveType::BYTE:
            return CastNumberOrCharLiteralFromTo<From, int8_t>(lit, context);
        case ir::PrimitiveType::SHORT:
            return CastNumberOrCharLiteralFromTo<From, int16_t>(lit, context);
        case ir::PrimitiveType::INT:
            return CastNumberOrCharLiteralFromTo<From, int32_t>(lit, context);
        case ir::PrimitiveType::LONG:
            return CastNumberOrCharLiteralFromTo<From, int64_t>(lit, context);
        case ir::PrimitiveType::FLOAT:
            return CastNumberOrCharLiteralFromTo<From, float>(lit, context);
        case ir::PrimitiveType::DOUBLE:
            return CastNumberOrCharLiteralFromTo<From, double>(lit, context);
        default:
            ES2PANDA_UNREACHABLE();
    }
}

static ir::AstNode *CorrectNumberOrCharLiteral(const ir::Literal *lit, ir::PrimitiveType type,
                                               public_lib::Context *context)
{
    if (TypeRankToPrimitiveType(GetTypeRank(lit)) == type) {
        return const_cast<ir::Literal *>(lit);
    }

    switch (GetTypeRank(lit)) {
        case TypeRank::CHAR:
            return CastNumberOrCharLiteralFrom<char16_t>(lit, type, context);
        case TypeRank::INT8:
            return CastNumberOrCharLiteralFrom<int8_t>(lit, type, context);
        case TypeRank::INT16:
            return CastNumberOrCharLiteralFrom<int16_t>(lit, type, context);
        case TypeRank::INT32:
            return CastNumberOrCharLiteralFrom<int32_t>(lit, type, context);
        case TypeRank::INT64:
            return CastNumberOrCharLiteralFrom<int64_t>(lit, type, context);
        case TypeRank::FLOAT:
            return CastNumberOrCharLiteralFrom<float>(lit, type, context);
        case TypeRank::DOUBLE:
            return CastNumberOrCharLiteralFrom<double>(lit, type, context);
        default:
            ES2PANDA_UNREACHABLE();
    }
}

ir::TypeNode *GetTypeAnnotationFromVarDecl(const ir::Literal *lit)
{
    auto *parent = lit->Parent();
    if (!parent->IsVariableDeclarator()) {
        return nullptr;
    }
    auto vd = parent->AsVariableDeclarator();
    if (!vd->Id()->IsIdentifier()) {
        return nullptr;
    }
    return vd->Id()->AsIdentifier()->TypeAnnotation();
}

static ir::PrimitiveType GetRightTypeOfNumberOrCharLiteral(const ir::Literal *lit)
{
    auto *parent = lit->Parent();
    if (parent->IsVariableDeclarator()) {
        auto vb = parent->AsVariableDeclarator();
        if (!vb->Id()->IsIdentifier()) {
            return TypeRankToPrimitiveType(GetTypeRank(lit));
        }

        if (vb->Id()->AsIdentifier()->TypeAnnotation() == nullptr) {
            return TypeRankToPrimitiveType(GetTypeRank(lit));
        }

        if (vb->Id()->AsIdentifier()->TypeAnnotation()->IsETSPrimitiveType()) {
            return vb->Id()->AsIdentifier()->TypeAnnotation()->AsETSPrimitiveType()->GetPrimitiveType();
        }
    } else if (parent->IsClassProperty()) {
        auto cp = parent->AsClassProperty();
        if (cp->TypeAnnotation() == nullptr) {
            return TypeRankToPrimitiveType(GetTypeRank(lit));
        }

        if (cp->TypeAnnotation()->IsETSPrimitiveType()) {
            return cp->TypeAnnotation()->AsETSPrimitiveType()->GetPrimitiveType();
        }
    }

    return TypeRankToPrimitiveType(GetTypeRank(lit));
}

static ir::AstNode *TryToCorrectNumberOrCharLiteral(ir::AstNode *node, public_lib::Context *context)
{
    if (IsCorrectNumberLiteral(node) || node->IsCharLiteral()) {
        auto lit = node->AsExpression()->AsLiteral();
        return CorrectNumberOrCharLiteral(lit, GetRightTypeOfNumberOrCharLiteral(lit), context);
    }

    return node;
}

// NOLINTBEGIN(readability-else-after-return)
static bool TestLiteral(const ir::Literal *lit)
{
    // 15.10.1 Extended Conditional Expression
    if (lit->IsBooleanLiteral()) {
        return lit->AsBooleanLiteral()->Value();
    }
    if (lit->IsStringLiteral()) {
        return !lit->AsStringLiteral()->Str().Empty();
    }
    if (lit->IsCharLiteral()) {
        return lit->AsCharLiteral()->Char() != 0;
    }
    if (lit->IsNumberLiteral()) {
        return !lit->AsNumberLiteral()->Number().IsZero();
    }
    ES2PANDA_UNREACHABLE();
}
// NOLINTEND(readability-else-after-return)

ir::AstNode *ConstantExpressionLowering::FoldTernaryConstant(ir::ConditionalExpression *cond)
{
    auto const test = cond->Test()->AsLiteral();
    auto res = TestLiteral(test) ? cond->Consequent() : cond->Alternate();
    auto resNode = res->Clone(context_->allocator, cond->Parent());
    auto *scope = NearestScope(resNode->Parent());
    auto localCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder_, scope);
    InitScopesPhaseETS::RunExternalNode(resNode, varbinder_);
    resNode->SetRange(cond->Range());
    return resNode;
}

template <typename InputType>
static bool PerformRelationOperation(InputType left, InputType right, lexer::TokenType opType)
{
    switch (opType) {
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN: {
            return left > right;
        }
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL: {
            return left >= right;
        }
        case lexer::TokenType::PUNCTUATOR_LESS_THAN: {
            return left < right;
        }
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL: {
            return left <= right;
        }
        case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_EQUAL: {
            return left == right;
        }
        case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_EQUAL: {
            return left != right;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

static ir::AstNode *HandleNumericalRelationalExpression(const ir::BinaryExpression *expr, ArenaAllocator *allocator)
{
    auto left = expr->Left()->AsLiteral();
    auto right = expr->Right()->AsLiteral();
    auto opType = expr->OperatorType();

    ES2PANDA_ASSERT(left->IsNumberLiteral() || left->IsCharLiteral());
    ES2PANDA_ASSERT(right->IsNumberLiteral() || right->IsCharLiteral());

    TypeRank targetRank = std::max(GetTypeRank(left), GetTypeRank(right));

    bool res = false;
    switch (targetRank) {
        case TypeRank::DOUBLE: {
            res = PerformRelationOperation(CastValTo<double>(left), CastValTo<double>(right), opType);
            break;
        }
        case TypeRank::FLOAT: {
            res = PerformRelationOperation(CastValTo<float>(left), CastValTo<float>(right), opType);
            break;
        }
        case TypeRank::INT64: {
            res = PerformRelationOperation(CastValTo<int64_t>(left), CastValTo<int64_t>(right), opType);
            break;
        }
        case TypeRank::INT32:
        case TypeRank::INT16:
        case TypeRank::INT8:
        case TypeRank::CHAR: {
            res = PerformRelationOperation(CastValTo<int32_t>(left), CastValTo<int32_t>(right), opType);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    return CreateBooleanLiteral(res, const_cast<ir::BinaryExpression *>(expr)->Parent(), expr->Range(), allocator);
}

static ir::AstNode *HandleRelationalExpression(const ir::BinaryExpression *expr, public_lib::Context *context)
{
    auto left = expr->Left()->AsLiteral();
    auto right = expr->Right()->AsLiteral();
    auto opType = expr->OperatorType();

    if (IsConvertibleToNumericType(left) && IsConvertibleToNumericType(right)) {
        return HandleNumericalRelationalExpression(expr, context->allocator);
    }

    if (left->IsStringLiteral() && right->IsStringLiteral()) {
        auto res = PerformRelationOperation(left->AsStringLiteral()->Str(), right->AsStringLiteral()->Str(), opType);
        return CreateBooleanLiteral(res, const_cast<ir::BinaryExpression *>(expr)->Parent(), expr->Range(),
                                    context->allocator);
    }

    if (left->IsBooleanLiteral() && right->IsBooleanLiteral()) {
        auto res = PerformRelationOperation(GetVal<bool>(left), GetVal<bool>(right), opType);
        return CreateBooleanLiteral(res, const_cast<ir::BinaryExpression *>(expr)->Parent(), expr->Range(),
                                    context->allocator);
    }

    LogError(context, diagnostic::WRONG_OPERAND_TYPE_FOR_BINARY_EXPRESSION, {}, expr->Start());
    return CreateErrorIdentifier(expr, context->allocator);
}

static bool IsMultiplicativeExpression(const ir::BinaryExpression *expr)
{
    auto opType = expr->OperatorType();
    return opType == lexer::TokenType::PUNCTUATOR_MULTIPLY || opType == lexer::TokenType::PUNCTUATOR_DIVIDE ||
           opType == lexer::TokenType::PUNCTUATOR_MOD;
}

static bool IsRelationalExpression(const ir::BinaryExpression *expr)
{
    auto opType = expr->OperatorType();
    return opType == lexer::TokenType::PUNCTUATOR_GREATER_THAN ||
           opType == lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL ||
           opType == lexer::TokenType::PUNCTUATOR_LESS_THAN || opType == lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL ||
           opType == lexer::TokenType::PUNCTUATOR_EQUAL || opType == lexer::TokenType::PUNCTUATOR_NOT_EQUAL ||
           opType == lexer::TokenType::PUNCTUATOR_STRICT_EQUAL ||
           opType == lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL;
}

static bool IsAdditiveExpression(const ir::BinaryExpression *expr)
{
    auto opType = expr->OperatorType();

    return opType == lexer::TokenType::PUNCTUATOR_PLUS || opType == lexer::TokenType::PUNCTUATOR_MINUS;
}

static double CalculateFloatZeroDevision(double leftNum)
{
    if (leftNum == 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (leftNum > 0) {
        return std::numeric_limits<double>::infinity();
    }
    return -std::numeric_limits<double>::infinity();
}

template <typename TargetType>
static TargetType PerformMultiplicativeOperation(TargetType leftNum, TargetType rightNum,
                                                 const ir::BinaryExpression *expr, public_lib::Context *context)
{
    auto isForbiddenZeroDivision = [&rightNum]() { return std::is_integral_v<TargetType> && rightNum == 0; };
    auto isFloatZeroDevision = [&rightNum]() { return std::is_floating_point_v<TargetType> && rightNum == 0; };
    auto isIntegralDivideResOverflow = [&rightNum, &leftNum]() {
        // Note: Handle corner cases
        return std::is_integral_v<TargetType> && leftNum == std::numeric_limits<TargetType>::min() && rightNum == -1;
    };
    auto opType = expr->OperatorType();
    switch (opType) {
        case lexer::TokenType::PUNCTUATOR_MULTIPLY: {
            return leftNum * rightNum;
        }
        case lexer::TokenType::PUNCTUATOR_DIVIDE: {
            if (isForbiddenZeroDivision()) {
                LogError(context, diagnostic::DIVISION_BY_ZERO, {}, expr->Start());
                // Max integral value
                return std::numeric_limits<TargetType>::max();
            }
            if (isFloatZeroDevision()) {
                return CalculateFloatZeroDevision(leftNum);
            }

            ES2PANDA_ASSERT(rightNum != 0);
            if (isIntegralDivideResOverflow()) {
                return std::numeric_limits<TargetType>::min();
            }
            return leftNum / rightNum;
        }
        case lexer::TokenType::PUNCTUATOR_MOD: {
            if (isForbiddenZeroDivision()) {
                LogError(context, diagnostic::DIVISION_BY_ZERO, {}, expr->Start());
                // Max integral value
                return std::numeric_limits<TargetType>::max();
            }
            if constexpr (std::is_integral_v<TargetType>) {
                if (isIntegralDivideResOverflow()) {
                    return 0;
                }
                return leftNum % rightNum;
            } else {
                return std::fmod(leftNum, rightNum);
            }
        }
        default:
            ES2PANDA_UNREACHABLE();
    }
}

static ir::AstNode *HandleMultiplicativeExpression(const ir::BinaryExpression *expr, public_lib::Context *context)
{
    auto left = expr->Left()->AsLiteral();
    auto right = expr->Right()->AsLiteral();
    if (!IsConvertibleToNumericType(left) || !IsConvertibleToNumericType(right)) {
        LogError(context, diagnostic::WRONG_OPERAND_TYPE_FOR_BINARY_EXPRESSION, {}, expr->Start());
        return CreateErrorIdentifier(expr, context->allocator);
    }

    auto allocator = context->allocator;
    auto parent = const_cast<ir::BinaryExpression *>(expr)->Parent();
    auto loc = expr->Range();

    TypeRank targetRank = std::max(GetTypeRank(left), GetTypeRank(right));
    switch (targetRank) {
        case TypeRank::DOUBLE: {
            double res =
                PerformMultiplicativeOperation(CastValTo<double>(left), CastValTo<double>(right), expr, context);
            return CreateNumberLiteral(res, parent, loc, allocator);
        }
        case TypeRank::FLOAT: {
            float res = PerformMultiplicativeOperation(CastValTo<float>(left), CastValTo<float>(right), expr, context);
            return CreateNumberLiteral(res, parent, loc, allocator);
        }
        case TypeRank::INT64: {
            int64_t res =
                PerformMultiplicativeOperation(CastValTo<int64_t>(left), CastValTo<int64_t>(right), expr, context);
            return CreateNumberLiteral(res, parent, loc, allocator);
        }
        case TypeRank::INT32:
        case TypeRank::INT16:
        case TypeRank::INT8:
        case TypeRank::CHAR: {
            int32_t res =
                PerformMultiplicativeOperation(CastValTo<int32_t>(left), CastValTo<int32_t>(right), expr, context);
            return CreateNumberLiteral(res, parent, loc, allocator);
        }
        default:
            ES2PANDA_UNREACHABLE();
    }
}

template <typename TargetType>
static TargetType PerformAdditiveOperation(TargetType left, TargetType right, lexer::TokenType opType)
{
    if constexpr (std::is_floating_point_v<TargetType>) {
        switch (opType) {
            case lexer::TokenType::PUNCTUATOR_PLUS:
                return left + right;
            case lexer::TokenType::PUNCTUATOR_MINUS:
                return left - right;
            default:
                ES2PANDA_UNREACHABLE();
        }
    } else {
        //  Integral types
        // try bit cast to unsigned counterpart to avoid signed integer overflow
        auto uLeft = bit_cast<std::make_unsigned_t<TargetType>, TargetType>(left);
        auto uRight = bit_cast<std::make_unsigned_t<TargetType>, TargetType>(right);

        switch (opType) {
            case lexer::TokenType::PUNCTUATOR_PLUS: {
                return bit_cast<TargetType, std::make_unsigned_t<TargetType>>(uLeft + uRight);
            }
            case lexer::TokenType::PUNCTUATOR_MINUS: {
                return bit_cast<TargetType, std::make_unsigned_t<TargetType>>(uLeft - uRight);
            }
            default:
                ES2PANDA_UNREACHABLE();
        }
    }
}

static ir::AstNode *PerformStringAdditiveOperation(const ir::BinaryExpression *expr, public_lib::Context *context)
{
    auto const lhs = expr->Left()->AsLiteral();
    auto const rhs = expr->Right()->AsLiteral();
    auto resStr = util::UString(context->allocator);

    auto appendLiteral = [&resStr, allocator = context->allocator](const ir::Literal *lit) {
        if (lit->IsCharLiteral()) {
            resStr.Append(static_cast<char32_t>(lit->AsCharLiteral()->Char()) & MAX_CHAR);
            return;
        }
        if (lit->IsStringLiteral()) {
            // No need to create new temporary string (util::UString) for string literal
            resStr.Append(lit->AsStringLiteral()->Str());
            return;
        }
        resStr.Append(util::UString(lit->ToString(), allocator).View());
    };

    appendLiteral(lhs);
    appendLiteral(rhs);

    auto resNode = util::NodeAllocator::Alloc<ir::StringLiteral>(context->allocator, resStr.View());
    resNode->SetParent(const_cast<ir::BinaryExpression *>(expr)->Parent());
    resNode->SetRange(expr->Range());
    return resNode;
}

static ir::AstNode *HandleAdditiveExpression(const ir::BinaryExpression *expr, public_lib::Context *context)
{
    auto left = expr->Left()->AsLiteral();
    auto right = expr->Right()->AsLiteral();
    auto opType = expr->OperatorType();
    if ((opType == lexer::TokenType::PUNCTUATOR_PLUS) && (left->IsStringLiteral() || right->IsStringLiteral())) {
        return PerformStringAdditiveOperation(expr, context);
    }

    if (!IsConvertibleToNumericType(left) || !IsConvertibleToNumericType(right)) {
        LogError(context, diagnostic::WRONG_OPERAND_TYPE_FOR_BINARY_EXPRESSION, {}, expr->Start());
        return CreateErrorIdentifier(expr, context->allocator);
    }

    auto allocator = context->allocator;
    auto parent = const_cast<ir::BinaryExpression *>(expr)->Parent();
    auto loc = expr->Range();

    TypeRank targetRank = std::max(GetTypeRank(left), GetTypeRank(right));
    switch (targetRank) {
        case TypeRank::DOUBLE: {
            auto res = PerformAdditiveOperation<double>(CastValTo<double>(left), CastValTo<double>(right), opType);
            return CreateNumberLiteral(res, parent, loc, allocator);
        }
        case TypeRank::FLOAT: {
            auto res = PerformAdditiveOperation<float>(CastValTo<float>(left), CastValTo<float>(right), opType);
            return CreateNumberLiteral(res, parent, loc, allocator);
        }
        case TypeRank::INT64: {
            int64_t res = PerformAdditiveOperation(CastValTo<int64_t>(left), CastValTo<int64_t>(right), opType);
            return CreateNumberLiteral(res, parent, loc, allocator);
        }
        case TypeRank::INT32:
        case TypeRank::INT16:
        case TypeRank::INT8:
        case TypeRank::CHAR: {
            int32_t res = PerformAdditiveOperation(CastValTo<int32_t>(left), CastValTo<int32_t>(right), opType);
            return CreateNumberLiteral(res, parent, loc, allocator);
        }
        default:
            ES2PANDA_UNREACHABLE();
    }
}

static bool IsShiftExpression(const ir::BinaryExpression *expr)
{
    auto opType = expr->OperatorType();
    return opType == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT || opType == lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT ||
           opType == lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT;
}

template <typename SignedType>
static SignedType PerformShiftOperation(SignedType left, SignedType right, lexer::TokenType opType)
{
    using UnsignedType = std::make_unsigned_t<SignedType>;

    SignedType result = 0;
    auto uLeft = bit_cast<UnsignedType, SignedType>(left);
    auto uRight = bit_cast<UnsignedType, SignedType>(right);

    auto mask = std::numeric_limits<UnsignedType>::digits - 1U;
    UnsignedType shift = uRight & mask;

    switch (opType) {
        case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT: {
            static_assert(sizeof(UnsignedType) == 4 || sizeof(UnsignedType) == 8);
            return bit_cast<SignedType, UnsignedType>(uLeft << shift);
        }
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT: {
            static_assert(sizeof(SignedType) == 4 || sizeof(SignedType) == 8);
            return bit_cast<SignedType, UnsignedType>(left >> shift);  // NOLINT(hicpp-signed-bitwise)
        }
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT: {
            static_assert(sizeof(UnsignedType) == 4 || sizeof(UnsignedType) == 8);
            return bit_cast<SignedType, UnsignedType>(uLeft >> shift);
        }
        default:
            ES2PANDA_UNREACHABLE();
    }
    return result;
}

static ir::AstNode *HandleShiftExpression(const ir::BinaryExpression *expr, public_lib::Context *context)
{
    auto left = expr->Left()->AsLiteral();
    auto right = expr->Right()->AsLiteral();
    auto opType = expr->OperatorType();

    if (!IsConvertibleToNumericType(left) || !IsConvertibleToNumericType(right)) {
        LogError(context, diagnostic::WRONG_OPERAND_TYPE_FOR_BINARY_EXPRESSION, {}, expr->Start());
        return CreateErrorIdentifier(expr, context->allocator);
    }

    auto allocator = context->allocator;
    auto parent = const_cast<ir::BinaryExpression *>(expr)->Parent();
    auto loc = expr->Range();

    TypeRank targetRank = std::max(GetTypeRank(left), GetTypeRank(right));
    switch (targetRank) {
        case TypeRank::DOUBLE:
        case TypeRank::INT64: {
            int64_t res = PerformShiftOperation(CastValTo<int64_t>(left), CastValTo<int64_t>(right), opType);
            return CreateNumberLiteral(res, parent, loc, allocator);
        }
        case TypeRank::FLOAT:
        case TypeRank::INT32:
        case TypeRank::INT16:
        case TypeRank::INT8:
        case TypeRank::CHAR: {
            int32_t res = PerformShiftOperation(CastValTo<int32_t>(left), CastValTo<int32_t>(right), opType);
            return CreateNumberLiteral(res, parent, loc, allocator);
        }
        default:
            ES2PANDA_UNREACHABLE();
    }
}

static bool IsBitwiseLogicalExpression(const ir::BinaryExpression *expr)
{
    auto opType = expr->OperatorType();
    return opType == lexer::TokenType::PUNCTUATOR_BITWISE_XOR || opType == lexer::TokenType::PUNCTUATOR_BITWISE_AND ||
           opType == lexer::TokenType::PUNCTUATOR_BITWISE_OR;
}

template <typename SignedType>
static SignedType PerformBitwiseLogicalOperation(SignedType left, SignedType right, lexer::TokenType opType)
{
    using UnsignedType = std::make_unsigned_t<SignedType>;

    auto uLeft = bit_cast<UnsignedType, SignedType>(left);
    auto uRight = bit_cast<UnsignedType, SignedType>(right);

    switch (opType) {
        case lexer::TokenType::PUNCTUATOR_BITWISE_AND: {
            return uLeft & uRight;
        }
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR: {
            return uLeft | uRight;
        }
        case lexer::TokenType::PUNCTUATOR_BITWISE_XOR: {
            return uLeft ^ uRight;
        }
        default:
            ES2PANDA_UNREACHABLE();
    }
}

static ir::AstNode *HandleNumericBitwiseLogicalExpression(const ir::BinaryExpression *expr,
                                                          public_lib::Context *context)
{
    auto left = expr->Left()->AsLiteral();
    auto right = expr->Right()->AsLiteral();
    auto opType = expr->OperatorType();

    auto allocator = context->allocator;
    auto parent = const_cast<ir::BinaryExpression *>(expr)->Parent();
    auto loc = expr->Range();

    TypeRank targetRank = std::max(GetTypeRank(left), GetTypeRank(right));
    switch (targetRank) {
        case TypeRank::DOUBLE:
        case TypeRank::INT64: {
            int64_t res = PerformBitwiseLogicalOperation(CastValTo<int64_t>(left), CastValTo<int64_t>(right), opType);
            return CreateNumberLiteral(res, parent, loc, allocator);
        }
        case TypeRank::FLOAT:
        case TypeRank::INT32:
        case TypeRank::INT16:
        case TypeRank::INT8:
        case TypeRank::CHAR: {
            int32_t res = PerformBitwiseLogicalOperation(CastValTo<int32_t>(left), CastValTo<int32_t>(right), opType);
            return CreateNumberLiteral(res, parent, loc, allocator);
        }
        default:
            ES2PANDA_UNREACHABLE();
    }
}

static ir::AstNode *HandleBitwiseLogicalExpression(const ir::BinaryExpression *expr, public_lib::Context *context)
{
    auto left = expr->Left()->AsLiteral();
    auto right = expr->Right()->AsLiteral();
    auto opType = expr->OperatorType();

    if (IsConvertibleToNumericType(left) && IsConvertibleToNumericType(right)) {
        return HandleNumericBitwiseLogicalExpression(expr, context);
    }

    if (!left->IsBooleanLiteral() || !right->IsBooleanLiteral()) {
        LogError(context, diagnostic::WRONG_OPERAND_TYPE_FOR_BINARY_EXPRESSION, {}, expr->Start());
        return CreateErrorIdentifier(expr, context->allocator);
    }

    auto allocator = context->allocator;
    auto parent = const_cast<ir::BinaryExpression *>(expr)->Parent();
    auto loc = expr->Range();
    bool res = false;

    auto leftVal = left->AsBooleanLiteral()->Value();
    auto rightVal = right->AsBooleanLiteral()->Value();
    switch (opType) {
        case lexer::TokenType::PUNCTUATOR_BITWISE_AND: {
            res = ((static_cast<uint32_t>(leftVal) & static_cast<uint32_t>(rightVal)) != 0);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR: {
            res = ((static_cast<uint32_t>(leftVal) | static_cast<uint32_t>(rightVal)) != 0);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_BITWISE_XOR: {
            res = leftVal ^ rightVal;
            break;
        }
        default:
            ES2PANDA_UNREACHABLE();
    }
    return CreateBooleanLiteral(res, parent, loc, allocator);
}

static bool IsConditionalExpression(const ir::BinaryExpression *expr)
{
    auto opType = expr->OperatorType();
    return opType == lexer::TokenType::PUNCTUATOR_LOGICAL_AND || opType == lexer::TokenType::PUNCTUATOR_LOGICAL_OR;
}

static ir::AstNode *HandleConditionalExpression(const ir::BinaryExpression *expr, public_lib::Context *context)
{
    auto left = const_cast<ir::BinaryExpression *>(expr)->Left()->AsLiteral();
    auto right = const_cast<ir::BinaryExpression *>(expr)->Right()->AsLiteral();

    auto allocator = context->allocator;
    auto parent = const_cast<ir::BinaryExpression *>(expr)->Parent();
    auto loc = expr->Range();

    bool lhs = TestLiteral(left);
    bool rhs = TestLiteral(right);

    auto opType = expr->OperatorType();
    switch (opType) {
        case lexer::TokenType::PUNCTUATOR_LOGICAL_AND: {
            return CreateBooleanLiteral(lhs && rhs, parent, loc, allocator);
        }
        case lexer::TokenType::PUNCTUATOR_LOGICAL_OR: {
            return CreateBooleanLiteral(lhs || rhs, parent, loc, allocator);
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
    ES2PANDA_UNREACHABLE();
}

static ir::AstNode *FoldBinaryExpression(const ir::BinaryExpression *expr, public_lib::Context *context)
{
    if (IsMultiplicativeExpression(expr)) {
        return HandleMultiplicativeExpression(expr, context);
    }
    if (IsAdditiveExpression(expr)) {
        return HandleAdditiveExpression(expr, context);
    }
    if (IsShiftExpression(expr)) {
        return HandleShiftExpression(expr, context);
    }
    if (IsRelationalExpression(expr)) {
        return HandleRelationalExpression(expr, context);
    }
    if (IsBitwiseLogicalExpression(expr)) {
        return HandleBitwiseLogicalExpression(expr, context);
    }
    if (IsConditionalExpression(expr)) {
        return HandleConditionalExpression(expr, context);
    }
    ES2PANDA_UNREACHABLE();
}

template <typename InputType>
static lexer::Number HandleBitwiseNegate(InputType value, TypeRank rank)
{
    switch (rank) {
        case TypeRank::DOUBLE:
        case TypeRank::INT64: {
            return lexer::Number(static_cast<int64_t>(~static_cast<uint64_t>(value)));
        }
        case TypeRank::FLOAT:
        case TypeRank::INT32:
        case TypeRank::INT16:
        case TypeRank::INT8:
        case TypeRank::CHAR: {
            return lexer::Number(static_cast<int32_t>(~static_cast<uint32_t>(value)));
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

template <typename InputType>
static ir::AstNode *FoldUnaryNumericConstantHelper(const ir::UnaryExpression *unary, const ir::Literal *node,
                                                   TypeRank rank, ArenaAllocator *allocator)
{
    auto value = CastValTo<InputType>(node);

    lexer::Number resNum {};
    switch (unary->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_PLUS: {
            resNum = lexer::Number(value);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_MINUS: {
            resNum = lexer::Number(-value);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_TILDE: {
            resNum = std::move(HandleBitwiseNegate(value, rank));
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    ir::TypedAstNode *resNode = util::NodeAllocator::Alloc<ir::NumberLiteral>(allocator, resNum);
    resNode->SetParent(const_cast<ir::UnaryExpression *>(unary)->Parent());
    resNode->SetRange(unary->Range());
    return resNode;
}

static ir::AstNode *FoldUnaryNumericConstant(const ir::UnaryExpression *unary, ArenaAllocator *allocator)
{
    auto literal = unary->Argument()->AsLiteral();
    TypeRank rank = GetTypeRank(literal);

    switch (rank) {
        case TypeRank::DOUBLE: {
            return FoldUnaryNumericConstantHelper<double>(unary, literal, rank, allocator);
        }
        case TypeRank::FLOAT: {
            return FoldUnaryNumericConstantHelper<float>(unary, literal, rank, allocator);
        }
        case TypeRank::INT64: {
            return FoldUnaryNumericConstantHelper<int64_t>(unary, literal, rank, allocator);
        }
        case TypeRank::INT32:
        case TypeRank::INT16:
        case TypeRank::INT8:
        case TypeRank::CHAR: {
            return FoldUnaryNumericConstantHelper<int32_t>(unary, literal, rank, allocator);
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

static ir::AstNode *FoldLogicalUnaryExpression(const ir::UnaryExpression *unary, ArenaAllocator *allocator)
{
    auto resNode =
        util::NodeAllocator::Alloc<ir::BooleanLiteral>(allocator, !TestLiteral(unary->Argument()->AsLiteral()));
    ES2PANDA_ASSERT(resNode != nullptr);
    resNode->SetParent(const_cast<ir::UnaryExpression *>(unary)->Parent());
    resNode->SetRange(unary->Range());
    return resNode;
}

static ir::AstNode *FoldUnaryExpression(const ir::UnaryExpression *unary, public_lib::Context *context)
{
    if (unary->OperatorType() == lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK) {
        return FoldLogicalUnaryExpression(unary, context->allocator);
    }

    auto lit = unary->Argument()->AsLiteral();
    if (lit->IsNumberLiteral() || lit->IsCharLiteral()) {
        return FoldUnaryNumericConstant(unary, context->allocator);
    }

    LogError(context, diagnostic::WRONG_OPERAND_TYPE_FOR_UNARY_EXPRESSION, {}, unary->Start());
    return CreateErrorIdentifier(unary, context->allocator);
}

static ir::AstNode *FoldTemplateLiteral(ir::TemplateLiteral *expr, ArenaAllocator *allocator)
{
    auto litToString = [allocator](const ir::Literal *lit) {
        if (lit->IsNumberLiteral()) {
            return util::UString(lit->AsNumberLiteral()->ToString(), allocator).View();
        }
        if (lit->IsCharLiteral()) {
            return util::UString(lit->AsCharLiteral()->ToString(), allocator).View();
        }
        if (lit->IsBooleanLiteral()) {
            return util::UString(lit->AsBooleanLiteral()->ToString(), allocator).View();
        }
        if (lit->IsStringLiteral()) {
            return lit->AsStringLiteral()->Str();
        }
        ES2PANDA_UNREACHABLE();
    };

    util::UString result(allocator);
    auto quasis = expr->Quasis();
    auto expressions = expr->Expressions();

    auto const num = std::max(expressions.size(), quasis.size());
    for (std::size_t i = 0U; i < num; i++) {
        if (i < quasis.size()) {
            result.Append(quasis[i]->Cooked());
        }
        if (i < expressions.size()) {
            result.Append(litToString(expressions[i]->AsLiteral()));
        }
    }

    auto *strLit = util::NodeAllocator::Alloc<ir::StringLiteral>(allocator, result.View());
    strLit->SetParent(expr->Parent());
    strLit->SetRange(expr->Range());
    return strLit;
}

static varbinder::Variable *ResolveIdentifier(const ir::Identifier *ident)
{
    if (ident->Variable() != nullptr) {
        return ident->Variable();
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr varbinder::ResolveBindingOptions const option =
        varbinder::ResolveBindingOptions::ALL_DECLARATION | varbinder::ResolveBindingOptions::ALL_VARIABLES;

    varbinder::Scope *scope = NearestScope(ident);
    return scope != nullptr ? scope->Find(ident->Name(), option).variable : nullptr;
}

static varbinder::Variable *ResolveMemberExpressionProperty(ir::MemberExpression *me)
{
    varbinder::Variable *var = nullptr;
    auto meObject = me->Object();
    if (meObject->IsMemberExpression()) {
        var = ResolveMemberExpressionProperty(meObject->AsMemberExpression());
    } else if (meObject->IsIdentifier()) {
        var = ResolveIdentifier(meObject->AsIdentifier());
    }

    if (var == nullptr) {
        return nullptr;
    }

    auto decl = var->Declaration();
    varbinder::LocalScope *scope = nullptr;
    if (decl->IsClassDecl()) {
        // NOTE(gogabr) : for some reason, ETSGLOBAL points to class declaration instead of definition.
        auto *declNode = decl->AsClassDecl()->Node();
        if (declNode->IsClassDefinition()) {
            scope = declNode->AsClassDefinition()->Scope();
        } else if (declNode->IsClassDeclaration()) {
            auto *classDef = declNode->AsClassDeclaration()->Definition();
            if (classDef != nullptr) {
                // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
                scope = classDef->Scope();
            }
        }

        if (scope == nullptr) {
            return nullptr;
        }
    } else if (decl->IsEnumLiteralDecl()) {
        scope = decl->AsEnumLiteralDecl()->Node()->AsTSEnumDeclaration()->Scope();
    } else {
        return nullptr;
    }

    if (!me->Property()->IsIdentifier()) {
        return nullptr;
    }

    auto option =
        varbinder::ResolveBindingOptions::STATIC_DECLARATION | varbinder::ResolveBindingOptions::STATIC_VARIABLES;
    return scope->FindLocal(me->Property()->AsIdentifier()->Name(), option);
}

static bool IsConstantExpression(ir::AstNode *expr)
{
    if (!expr->IsExpression()) {
        if (expr->IsETSTypeReference()) {
            return false;
        }
    }

    if (expr->IsETSPrimitiveType()) {
        return true;
    }

    if (expr->IsIdentifier()) {
        auto var = ResolveIdentifier(expr->AsIdentifier());
        return var != nullptr && var->Declaration()->IsConstDecl();
    }

    if (expr->IsMemberExpression()) {
        auto me = expr->AsMemberExpression();
        if (me->Kind() != ir::MemberExpressionKind::PROPERTY_ACCESS) {
            return false;
        }

        auto var = ResolveMemberExpressionProperty(me);
        return var != nullptr && var->Declaration()->IsReadonlyDecl();
    }

    if (IsSupportedLiteral(expr->AsExpression())) {
        return true;
    }

    auto isNotConstantExpression = [](ir::AstNode *node) { return !IsConstantExpression(node); };

    return (expr->IsBinaryExpression() || expr->IsUnaryExpression() || expr->IsTSAsExpression() ||
            expr->IsConditionalExpression() || expr->IsTemplateLiteral()) &&
           !expr->IsAnyChild(isNotConstantExpression);
}

static bool IsInTSEnumMemberInit(const ir::AstNode *n)
{
    auto enumMember = util::Helpers::FindAncestorGivenByType(n, ir::AstNodeType::TS_ENUM_MEMBER);
    if (enumMember == nullptr) {
        return false;
    }

    auto init = enumMember->AsTSEnumMember()->Init();
    return (init == n) || (init->FindChild([n](auto *child) { return child == n; }) != nullptr);
}

ir::AstNode *ConstantExpressionLowering::UnfoldResolvedReference(ir::AstNode *resolved, ir::AstNode *node)
{
    checker::RecursionPreserver<const ir::AstNode> rPreserver(unfoldingSet_, resolved);
    if (*rPreserver) {
        isSelfDependence_ = true;
        return node;
    }

    ir::AstNode *resNode = nullptr;
    if (resolved->IsClassProperty()) {
        auto propVal = resolved->AsClassElement()->Value();
        if (propVal != nullptr && IsConstantExpression(propVal)) {
            resNode = propVal->Clone(context_->allocator, node->Parent());
            resNode->SetRange(node->Range());
        }
    } else if (resolved->Parent()->IsVariableDeclarator()) {
        auto init = resolved->Parent()->AsVariableDeclarator()->Init();
        if (init != nullptr && IsConstantExpression(init) && !init->IsMemberExpression()) {
            resNode = init->Clone(context_->allocator, node->Parent());
            resNode->SetRange(node->Range());
        }
    } else if (resolved->IsTSEnumMember() && IsInTSEnumMemberInit(node)) {
        auto init = resolved->AsTSEnumMember()->Init();
        if (init != nullptr && IsConstantExpression(init)) {
            resNode = init->Clone(context_->allocator, node->Parent());
            resNode->SetRange(node->Range());
        }
    }

    if (resNode != nullptr) {
        auto res = MaybeUnfold(resNode);
        if (isSelfDependence_) {
            isSelfDependence_ = false;
            return node;
        }

        return res;
    }

    // failed to unfold
    return node;
}

ir::AstNode *ConstantExpressionLowering::MaybeUnfoldIdentifier(ir::Identifier *node)
{
    if (!node->IsReference(varbinder_->Extension())) {
        return node;
    }

    // Left-Hand-Side identifiers in UpdateExpression or BinaryExpression cannot be unfolded
    if (node->Parent()->IsUpdateExpression() && node->Parent()->AsUpdateExpression()->Argument() == node) {
        return node;
    }

    if (node->Parent()->IsAssignmentExpression() && node->Parent()->AsAssignmentExpression()->Left() == node) {
        return node;
    }

    auto *resolved = ResolveIdentifier(node);
    if (resolved == nullptr || !(resolved->Declaration()->IsConstDecl() || resolved->Declaration()->IsReadonlyDecl())) {
        return node;
    }

    auto *parent = node->Parent();
    while (parent != nullptr && (parent->IsMemberExpression() || parent->IsTSQualifiedName())) {
        parent = parent->Parent();
    }
    if (parent != nullptr && (parent->IsETSTypeReferencePart() || parent->IsETSTypeReference())) {
        return node;
    }
    return UnfoldResolvedReference(resolved->Declaration()->Node(), node);
}

ir::AstNode *ConstantExpressionLowering::MaybeUnfoldMemberExpression(ir::MemberExpression *node)
{
    if (node->Kind() != ir::MemberExpressionKind::PROPERTY_ACCESS) {
        return node;
    }

    auto resolved = ResolveMemberExpressionProperty(node);
    if (resolved == nullptr || !resolved->Declaration()->IsReadonlyDecl()) {
        return node;
    }
    return UnfoldResolvedReference(resolved->Declaration()->Node(), node);
}

ir::AstNode *ConstantExpressionLowering::MaybeUnfold(ir::AstNode *node)
{
    ir::NodeTransformer handleMaybeUnfold = [this](ir::AstNode *const n) {
        if (n->IsIdentifier() && (!n->Parent()->IsMemberExpression() || n->Parent()->AsMemberExpression()->Kind() ==
                                                                            ir::MemberExpressionKind::ELEMENT_ACCESS)) {
            return MaybeUnfoldIdentifier(n->AsIdentifier());
        }

        if (n->IsMemberExpression()) {
            return MaybeUnfoldMemberExpression(n->AsMemberExpression());
        }

        return n;
    };

    node->TransformChildrenRecursivelyPostorder(handleMaybeUnfold, Name());
    return handleMaybeUnfold(node);
}

static bool IsPotentialConstant(const ir::AstNodeType type)
{
    return type == ir::AstNodeType::TEMPLATE_LITERAL || type == ir::AstNodeType::TS_AS_EXPRESSION ||
           type == ir::AstNodeType::UNARY_EXPRESSION || type == ir::AstNodeType::BINARY_EXPRESSION ||
           type == ir::AstNodeType::CONDITIONAL_EXPRESSION || type == ir::AstNodeType::IDENTIFIER;
}

ir::AstNode *ConstantExpressionLowering::Fold(ir::AstNode *constantNode)
{
    ir::NodeTransformer handleFoldConstant = [this](ir::AstNode *const node) {
        if (node->IsTemplateLiteral()) {
            auto tmpLiteral = node->AsTemplateLiteral();
            auto exprs = tmpLiteral->Expressions();
            auto notSupportedLit = std::find_if(exprs.begin(), exprs.end(),
                                                [](ir::Expression *maybeLit) { return !IsSupportedLiteral(maybeLit); });
            // Cannot fold TemplateLiteral containing unsupported literal
            if (notSupportedLit != exprs.end()) {
                return node;
            }
            return FoldTemplateLiteral(tmpLiteral, context_->allocator);
        }
        if (node->IsUnaryExpression()) {
            auto unaryOp = node->AsUnaryExpression();
            if (IsSupportedLiteral(unaryOp->Argument())) {
                return FoldUnaryExpression(unaryOp, context_);
            }
        }
        if (node->IsBinaryExpression()) {
            auto binop = node->AsBinaryExpression();
            if (IsSupportedLiteral(binop->Left()) && IsSupportedLiteral(binop->Right())) {
                ERROR_SANITY_CHECK(context_->diagnosticEngine,
                                   binop->OperatorType() != lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING,
                                   return node);
                return FoldBinaryExpression(binop, context_);
            }
        }
        if (node->IsConditionalExpression()) {
            auto condExp = node->AsConditionalExpression();
            if (IsSupportedLiteral(condExp->Test())) {
                return FoldTernaryConstant(condExp);
            }
        }
        if (node->IsTSNonNullExpression() && IsSupportedLiteral(node->AsTSNonNullExpression()->Expr())) {
            auto expr = node->AsTSNonNullExpression()->Expr()->Clone(context_->allocator, node->Parent());
            expr->SetRange(node->Range());
            return expr;
        }
        return node;
    };
    constantNode->TransformChildrenRecursivelyPostorder(handleFoldConstant, Name());
    return TryToCorrectNumberOrCharLiteral(handleFoldConstant(constantNode), context_);
}

// Note: memberExpression can be constant when it is enum property access, this check will be enabled after Issue23082.
// for package, we need to check whether its every immediate-initializers is const expression.
void ConstantExpressionLowering::IsInitByConstant(ir::AstNode *node)
{
    ir::AstNode *initTobeChecked = nullptr;
    if (node->IsExpressionStatement() && node->AsExpressionStatement()->GetExpression()->IsAssignmentExpression()) {
        auto assignExpr = node->AsExpressionStatement()->GetExpression()->AsAssignmentExpression();
        initTobeChecked = assignExpr->Right();
        if (initTobeChecked->IsExpression() && IsSupportedLiteral(initTobeChecked->AsExpression())) {
            return;
        }

        if (!IsPotentialConstant(initTobeChecked->Type())) {
            LogError(context_, diagnostic::INVALID_INIT_IN_PACKAGE, {}, initTobeChecked->Start());
            return;
        }
        assignExpr->SetRight(Fold(MaybeUnfold(initTobeChecked))->AsExpression());
    }

    if (node->IsClassProperty()) {
        auto classProp = node->AsClassProperty();
        initTobeChecked = classProp->Value();
        if (initTobeChecked == nullptr) {
            return;
        }

        if (initTobeChecked->IsExpression() && IsSupportedLiteral(initTobeChecked->AsExpression())) {
            return;
        }

        if (!IsPotentialConstant(initTobeChecked->Type())) {
            LogError(context_, diagnostic::INVALID_INIT_IN_PACKAGE, {}, initTobeChecked->Start());
            return;
        }
        classProp->SetValue(Fold(MaybeUnfold(initTobeChecked))->AsExpression());
    }
}

void ConstantExpressionLowering::TryFoldInitializerOfPackage(ir::ClassDefinition *globalClass)
{
    for (auto element : globalClass->Body()) {
        if (element->IsMethodDefinition()) {
            auto const *classMethod = element->AsMethodDefinition();
            if (!classMethod->Key()->IsIdentifier() ||
                !classMethod->Key()->AsIdentifier()->Name().Is(compiler::Signatures::INIT_METHOD)) {
                continue;
            }

            auto const *methodBody = classMethod->Value()->AsFunctionExpression()->Function()->Body();
            if (methodBody == nullptr || !methodBody->IsBlockStatement()) {
                continue;
            }
            auto const &initStatements = methodBody->AsBlockStatement()->Statements();
            std::for_each(initStatements.begin(), initStatements.end(),
                          [this](ir::AstNode *node) { IsInitByConstant(node); });
        }

        if (element->IsClassProperty() && element->AsClassProperty()->IsConst() &&
            !element->AsClassProperty()->NeedInitInStaticBlock()) {
            IsInitByConstant(element);
        }
    }
}

bool ConstantExpressionLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    if (program->GetFlag(parser::ProgramFlags::AST_CONSTANT_EXPRESSION_LOWERED)) {
        return true;
    }

    context_ = ctx;
    program_ = program;
    varbinder_ = ctx->parserProgram->VarBinder()->AsETSBinder();

    program->Ast()->TransformChildrenRecursively(
        [this](ir::AstNode *const node) -> checker::AstNodePtr {
            // Note: Package need to check whether its immediate initializer is const expression.
            if (this->program_->IsPackage() && node->IsClassDefinition() && node->AsClassDefinition()->IsGlobal()) {
                TryFoldInitializerOfPackage(node->AsClassDefinition());
            }
            return Fold(MaybeUnfold(node));
        },
        Name());

    program->SetFlag(parser::ProgramFlags::AST_CONSTANT_EXPRESSION_LOWERED);
    return true;
}

}  // namespace ark::es2panda::compiler
