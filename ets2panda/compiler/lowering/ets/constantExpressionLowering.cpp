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
#include <limits>

#include "checker/ETSchecker.h"
#include "checker/types/typeError.h"
#include "compiler/lowering/util.h"
#include "ir/expressions/literals/undefinedLiteral.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "util/helpers.h"
#include "libpandabase/utils/small_vector.h"

namespace ark::es2panda::compiler {

static ir::PrimitiveType GetPrimitiveType(const lexer::Number &number)
{
    if (number.IsByte()) {
        return ir::PrimitiveType::BYTE;
    }
    if (number.IsShort()) {
        return ir::PrimitiveType::SHORT;
    }
    if (number.IsInt()) {
        return ir::PrimitiveType::INT;
    }
    if (number.IsLong()) {
        return ir::PrimitiveType::LONG;
    }
    if (number.IsFloat()) {
        return ir::PrimitiveType::FLOAT;
    }
    if (number.IsDouble()) {
        return ir::PrimitiveType::DOUBLE;
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

static bool IsBitwiseLogicalExpression(const ir::BinaryExpression *expr)
{
    auto opType = expr->OperatorType();
    return opType == lexer::TokenType::PUNCTUATOR_BITWISE_XOR || opType == lexer::TokenType::PUNCTUATOR_BITWISE_AND ||
           opType == lexer::TokenType::PUNCTUATOR_BITWISE_OR;
}

static bool IsAdditiveExpression(const ir::BinaryExpression *expr)
{
    auto opType = expr->OperatorType();

    return opType == lexer::TokenType::PUNCTUATOR_PLUS || opType == lexer::TokenType::PUNCTUATOR_MINUS;
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

static bool IsShiftExpression(const ir::BinaryExpression *expr)
{
    auto opType = expr->OperatorType();
    return opType == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT || opType == lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT ||
           opType == lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT;
}

static bool IsLogicalExpression(const ir::BinaryExpression *expr)
{
    auto opType = expr->OperatorType();
    return opType == lexer::TokenType::PUNCTUATOR_LOGICAL_AND || opType == lexer::TokenType::PUNCTUATOR_LOGICAL_OR;
}

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

class NodeCalculator {
public:
    using DAGNode = ConstantExpressionLoweringImpl::DAGNode;
    using TypeRank = lexer::Number::TypeRank;
    NodeCalculator(public_lib::Context *ctx, size_t sz) : context_ {ctx}
    {
        inputs_.resize(sz);
    }
    void SetInput(ir::Literal *node, size_t i)
    {
        ES2PANDA_ASSERT(i < inputs_.size());
        inputs_[i] = node;
    }

    ir::Literal *Calculate(DAGNode *node);

private:
    ir::Literal *SubstituteConstant()
    {
        ES2PANDA_ASSERT(inputs_.size() == 1);
        return inputs_[0]->Clone(context_->allocator, nullptr)->AsExpression()->AsLiteral();
    }

    ir::Literal *SubstituteConstantConditionally()
    {
        ES2PANDA_ASSERT(inputs_.size() == 3U);
        const auto *test = inputs_[0];
        auto *conseq = inputs_[1];
        auto *altern = inputs_[2U];
        auto res = TestLiteral(test) ? conseq : altern;
        auto resNode = res->Clone(context_->allocator, nullptr)->AsExpression()->AsLiteral();
        return resNode;
    }

    // NOTE(dkofanov) Template literals will be simplified only if each subexpression is constant.
    // It may worth to handle partial-simplification, e.g. after the algorithm stops.
    ir::Literal *Calculate(ir::TemplateLiteral *expr)
    {
        std::string tmpStr {};
        auto quasis = expr->Quasis();
        auto const num = std::max(inputs_.size(), quasis.size());
        for (std::size_t i = 0U; i < num; i++) {
            if (i < quasis.size()) {
                tmpStr += quasis[i]->Cooked().Utf8();
            }
            if (i < inputs_.size()) {
                if (inputs_[i]->IsCharLiteral()) {
                    LogError(diagnostic::CHAR_TO_STR_CONVERSION, {}, expr->Start());
                }
                if (inputs_[i]->IsNumberLiteral() || inputs_[i]->IsBooleanLiteral()) {
                    tmpStr += inputs_[i]->ToString();
                } else if (inputs_[i]->IsStringLiteral()) {
                    tmpStr += inputs_[i]->AsStringLiteral()->Str().Utf8();
                } else {
                    ES2PANDA_UNREACHABLE();
                }
            }
        }

        util::UString result(tmpStr, context_->allocator);
        return util::NodeAllocator::Alloc<ir::StringLiteral>(context_->allocator, result.View());
    }

    template <typename To>
    To ExtractFromLiteral(const ir::NumberLiteral *lit)
    {
        if (lit->Number().CanGetValue<To>()) {
            return lit->Number().GetValue<To>();
        }

        using Limits = std::numeric_limits<To>;
        // Since bitwise operations are allowed on FP, handle truncation here:
        if ((lit->Number().Is<double>() || lit->Number().Is<float>()) && std::is_integral_v<To>) {
            auto fp = lit->Number().GetValue<double>();
            if (((static_cast<double>(Limits::min()) <= fp)) && (fp <= static_cast<double>(Limits::max()))) {
                return static_cast<To>(fp);
            }
        }

        LogError(diagnostic::OVERFLOW_ARITHMETIC, {}, lit->Start());
        return {};
    }

    ir::NumberLiteral *FoldUnaryNumericConstant(const ir::UnaryExpression *unary, ir::NumberLiteral *literal)
    {
        auto rank = literal->Number().GetTypeRank();
        switch (rank) {
            case TypeRank::DOUBLE: {
                return FoldUnaryNumericConstantHelper<double>(unary, literal, rank);
            }
            case TypeRank::FLOAT: {
                return FoldUnaryNumericConstantHelper<float>(unary, literal, rank);
            }
            case TypeRank::INT64: {
                return FoldUnaryNumericConstantHelper<int64_t>(unary, literal, rank);
            }
            case TypeRank::INT32: {
                return FoldUnaryNumericConstantHelper<int32_t>(unary, literal, rank);
            }
            case TypeRank::INT16: {
                return FoldUnaryNumericConstantHelper<int16_t>(unary, literal, rank);
            }
            case TypeRank::INT8: {
                return FoldUnaryNumericConstantHelper<int8_t>(unary, literal, rank);
            }
            default: {
                ES2PANDA_UNREACHABLE();
            }
        }
    }

    template <typename InputType>
    ir::NumberLiteral *FoldUnaryNumericConstantHelper(const ir::UnaryExpression *unary, const ir::NumberLiteral *node,
                                                      TypeRank rank)
    {
        lexer::Number resNum {};
        switch (unary->OperatorType()) {
            case lexer::TokenType::PUNCTUATOR_PLUS: {
                resNum = lexer::Number(ExtractFromLiteral<InputType>(node));
                break;
            }
            case lexer::TokenType::PUNCTUATOR_MINUS: {
                resNum = lexer::Number(-ExtractFromLiteral<InputType>(node));
                break;
            }
            case lexer::TokenType::PUNCTUATOR_TILDE: {
                resNum = HandleBitwiseNegate(node, rank);
                break;
            }
            default: {
                ES2PANDA_UNREACHABLE();
            }
        }

        return CreateNumberLiteral(resNum);
    }

    lexer::Number HandleBitwiseNegate(const ir::NumberLiteral *node, TypeRank rank)
    {
        switch (rank) {
            case TypeRank::DOUBLE:
            case TypeRank::INT64: {
                return lexer::Number(~static_cast<uint64_t>(ExtractFromLiteral<int64_t>(node)));
            }
            case TypeRank::FLOAT:
            case TypeRank::INT32:
            case TypeRank::INT16:
            case TypeRank::INT8: {
                return lexer::Number(~static_cast<uint32_t>(ExtractFromLiteral<int32_t>(node)));
            }
            default: {
                ES2PANDA_UNREACHABLE();
            }
        }
    }

    ir::Literal *Calculate(ir::UnaryExpression *unary)
    {
        ES2PANDA_ASSERT(inputs_.size() == 1);
        if (unary->OperatorType() == lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK) {
            return CreateBooleanLiteral(!TestLiteral(inputs_[0]));
        }

        auto lit = inputs_[0];
        if (lit->IsNumberLiteral()) {
            return FoldUnaryNumericConstant(unary, lit->AsNumberLiteral());
        }

        LogError(diagnostic::WRONG_OPERAND_TYPE_FOR_UNARY_EXPRESSION, {}, unary->Start());
        return nullptr;
    }

    template <typename OperatorType, typename OperandType>
    void PerformArithmeticIntegral(const ir::Expression *expr, OperandType lhs, OperandType rhs, OperandType *res)
    {
        using Limits = std::numeric_limits<OperandType>;
        static_assert(std::is_integral_v<OperandType> && std::is_signed_v<OperandType>);
        bool overflowOccurred = false;
        if constexpr (std::is_same_v<OperatorType, std::divides<>> || std::is_same_v<OperatorType, std::modulus<>>) {
            if (rhs == 0) {
                LogError(diagnostic::DIVISION_BY_ZERO, {}, expr->Start());
                *res = Limits::max();
            } else if ((lhs == Limits::min()) && rhs == -1) {
                // Note: Handle corner cases
                overflowOccurred = true;
                *res = std::is_same_v<OperatorType, std::divides<>> ? Limits::min() : 0;
            } else {
                *res = OperatorType {}(lhs, rhs);
            }
        } else {
            if constexpr (sizeof(OperandType) >= sizeof(int32_t)) {
                if constexpr (std::is_same_v<OperatorType, std::multiplies<>>) {
                    overflowOccurred = __builtin_mul_overflow(lhs, rhs, res);
                } else if constexpr (std::is_same_v<OperatorType, std::plus<>>) {
                    overflowOccurred = __builtin_add_overflow(lhs, rhs, res);
                } else if constexpr (std::is_same_v<OperatorType, std::minus<>>) {
                    overflowOccurred = __builtin_sub_overflow(lhs, rhs, res);
                }
            } else {
                auto tmpRes = OperatorType {}(static_cast<int32_t>(lhs), static_cast<int32_t>(rhs));
                *res = static_cast<OperandType>(tmpRes);
                overflowOccurred = tmpRes < Limits::min() || Limits::max() < tmpRes;
            }
        }

        if (overflowOccurred) {
            LogError(diagnostic::OVERFLOW_ARITHMETIC, {}, expr->Start());
        }
    }

    template <typename OperatorType, typename OperandType>
    // CC-OFFNXT(huge_depth[C++]) solid logic
    void PerformArithmetic(const ir::Expression *expr, OperandType lhs, OperandType rhs, OperandType *res)
    {
        if constexpr (std::is_integral_v<OperandType>) {
            PerformArithmeticIntegral<OperatorType>(expr, lhs, rhs, res);
            return;
        } else if constexpr (std::is_floating_point_v<OperandType>) {
            if constexpr (std::is_same_v<OperatorType, std::divides<>>) {
                if ((rhs == 0) && (lhs == 0)) {
                    *res = std::numeric_limits<OperandType>::quiet_NaN();
                } else if ((rhs == 0) && (lhs > 0)) {
                    *res = std::numeric_limits<OperandType>::infinity();
                } else if ((rhs == 0) && (lhs < 0)) {
                    *res = -std::numeric_limits<OperandType>::infinity();
                } else {
                    *res = OperatorType {}(lhs, rhs);
                }
            } else if constexpr (std::is_same_v<OperatorType, std::modulus<>>) {
                if (rhs == 0) {
                    LogError(diagnostic::DIVISION_BY_ZERO, {}, expr->Start());
                    *res = std::numeric_limits<OperandType>::quiet_NaN();
                } else {
                    *res = std::fmod(lhs, rhs);
                }
            } else {
                *res = OperatorType {}(lhs, rhs);
            }

            return;
        }
        ES2PANDA_UNREACHABLE();
    }

    template <typename TargetType>
    ir::Literal *PerformMultiplicativeOperation(TargetType leftNum, TargetType rightNum,
                                                const ir::BinaryExpression *expr)
    {
        auto opType = expr->OperatorType();
        TargetType resNum {};
        switch (opType) {
            case lexer::TokenType::PUNCTUATOR_MULTIPLY: {
                PerformArithmetic<std::multiplies<>>(expr, leftNum, rightNum, &resNum);
                break;
            }
            case lexer::TokenType::PUNCTUATOR_DIVIDE: {
                PerformArithmetic<std::divides<>>(expr, leftNum, rightNum, &resNum);
                break;
            }
            case lexer::TokenType::PUNCTUATOR_MOD: {
                PerformArithmetic<std::modulus<>>(expr, leftNum, rightNum, &resNum);
                break;
            }
            default:
                ES2PANDA_UNREACHABLE();
        }
        return CreateNumberLiteral(resNum);
    }

    ir::Literal *HandleMultiplicativeExpression(const ir::BinaryExpression *expr, const ir::NumberLiteral *left,
                                                const ir::NumberLiteral *right)
    {
        switch (std::max(left->Number().GetTypeRank(), right->Number().GetTypeRank())) {
            case TypeRank::DOUBLE: {
                return PerformMultiplicativeOperation(ExtractFromLiteral<double>(left),
                                                      ExtractFromLiteral<double>(right), expr);
            }
            case TypeRank::FLOAT: {
                return PerformMultiplicativeOperation(ExtractFromLiteral<float>(left), ExtractFromLiteral<float>(right),
                                                      expr);
            }
            case TypeRank::INT64: {
                return PerformMultiplicativeOperation(ExtractFromLiteral<int64_t>(left),
                                                      ExtractFromLiteral<int64_t>(right), expr);
            }
            case TypeRank::INT32: {
                return PerformMultiplicativeOperation(ExtractFromLiteral<int32_t>(left),
                                                      ExtractFromLiteral<int32_t>(right), expr);
            }
            case TypeRank::INT16: {
                return PerformMultiplicativeOperation(ExtractFromLiteral<int16_t>(left),
                                                      ExtractFromLiteral<int16_t>(right), expr);
            }
            case TypeRank::INT8: {
                return PerformMultiplicativeOperation(ExtractFromLiteral<int8_t>(left),
                                                      ExtractFromLiteral<int8_t>(right), expr);
            }
            default:
                ES2PANDA_UNREACHABLE();
        }
    }

    template <typename TargetType>
    ir::Literal *PerformAdditiveOperation(TargetType left, TargetType right, const ir::BinaryExpression *expr)
    {
        TargetType res {};
        switch (expr->OperatorType()) {
            case lexer::TokenType::PUNCTUATOR_PLUS:
                PerformArithmetic<std::plus<>>(expr, left, right, &res);
                break;
            case lexer::TokenType::PUNCTUATOR_MINUS:
                PerformArithmetic<std::minus<>>(expr, left, right, &res);
                break;
            default:
                ES2PANDA_UNREACHABLE();
        }
        return CreateNumberLiteral(res);
    }

    ir::Literal *HandleNumericAdditiveExpression(const ir::BinaryExpression *expr, const ir::NumberLiteral *left,
                                                 const ir::NumberLiteral *right)
    {
        switch (std::max(left->Number().GetTypeRank(), right->Number().GetTypeRank())) {
            case TypeRank::DOUBLE: {
                return PerformAdditiveOperation(ExtractFromLiteral<double>(left), ExtractFromLiteral<double>(right),
                                                expr);
            }
            case TypeRank::FLOAT: {
                return PerformAdditiveOperation(ExtractFromLiteral<float>(left), ExtractFromLiteral<float>(right),
                                                expr);
            }
            case TypeRank::INT64: {
                return PerformAdditiveOperation(ExtractFromLiteral<int64_t>(left), ExtractFromLiteral<int64_t>(right),
                                                expr);
            }
            case TypeRank::INT32: {
                return PerformAdditiveOperation(ExtractFromLiteral<int32_t>(left), ExtractFromLiteral<int32_t>(right),
                                                expr);
            }
            case TypeRank::INT16: {
                return PerformAdditiveOperation(ExtractFromLiteral<int16_t>(left), ExtractFromLiteral<int16_t>(right),
                                                expr);
            }
            case TypeRank::INT8: {
                return PerformAdditiveOperation(ExtractFromLiteral<int8_t>(left), ExtractFromLiteral<int8_t>(right),
                                                expr);
            }
            default:
                ES2PANDA_UNREACHABLE();
        }
    }

    ir::Literal *PerformStringAdditiveOperation(const ir::BinaryExpression *expr, const ir::Literal *left,
                                                const ir::Literal *right)
    {
        if ((expr->OperatorType() != lexer::TokenType::PUNCTUATOR_PLUS) ||
            (!left->IsStringLiteral() && !right->IsStringLiteral())) {
            LogError(diagnostic::WRONG_OPERAND_TYPE_FOR_BINARY_EXPRESSION, {}, expr->Start());
            return nullptr;
        }
        if (left->IsCharLiteral() || right->IsCharLiteral()) {
            LogError(diagnostic::CHAR_TO_STR_CONVERSION, {}, expr->Start());
            return nullptr;
        }
        std::string tmpStr {};
        auto appendLiteral = [&tmpStr](const ir::Literal *lit) {
            if (lit->IsStringLiteral()) {
                tmpStr += lit->AsStringLiteral()->Str().Utf8();
            } else {
                tmpStr += lit->ToString();
            }
        };

        appendLiteral(left);
        appendLiteral(right);

        auto resStr = util::UString(tmpStr, context_->allocator);
        return util::NodeAllocator::Alloc<ir::StringLiteral>(context_->allocator, resStr.View());
    }

    template <typename SignedType>
    ir::Literal *PerformShiftOperation(SignedType left, SignedType right, lexer::TokenType opType)
    {
        using UnsignedType = std::make_unsigned_t<SignedType>;

        auto uLeft = bit_cast<UnsignedType, SignedType>(left);
        auto uRight = bit_cast<UnsignedType, SignedType>(right);

        auto mask = std::numeric_limits<UnsignedType>::digits - 1U;
        UnsignedType shift = uRight & mask;

        SignedType res {};
        switch (opType) {
            case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT: {
                static_assert(sizeof(UnsignedType) == 4 || sizeof(UnsignedType) == 8);
                res = bit_cast<SignedType, UnsignedType>(uLeft << shift);
                break;
            }
            case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT: {
                static_assert(sizeof(SignedType) == 4 || sizeof(SignedType) == 8);
                res = bit_cast<SignedType, UnsignedType>(left >> shift);  // NOLINT(hicpp-signed-bitwise)
                break;
            }
            case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT: {
                static_assert(sizeof(UnsignedType) == 4 || sizeof(UnsignedType) == 8);
                res = bit_cast<SignedType, UnsignedType>(uLeft >> shift);
                break;
            }
            default:
                ES2PANDA_UNREACHABLE();
        }
        return CreateNumberLiteral(res);
    }

    ir::Literal *HandleShiftExpression(const ir::BinaryExpression *expr, const ir::NumberLiteral *left,
                                       const ir::NumberLiteral *right)
    {
        auto opType = expr->OperatorType();
        switch (std::max(left->Number().GetTypeRank(), right->Number().GetTypeRank())) {
            case TypeRank::DOUBLE:
            case TypeRank::INT64: {
                return PerformShiftOperation(ExtractFromLiteral<int64_t>(left), ExtractFromLiteral<int64_t>(right),
                                             opType);
            }
            case TypeRank::FLOAT:
            case TypeRank::INT32:
            case TypeRank::INT16:
            case TypeRank::INT8: {
                return PerformShiftOperation(ExtractFromLiteral<int32_t>(left), ExtractFromLiteral<int32_t>(right),
                                             opType);
            }
            default:
                ES2PANDA_UNREACHABLE();
        }
    }

    ir::Literal *PerformRelationOperation(const ir::CharLiteral *left, const ir::CharLiteral *right,
                                          lexer::TokenType opType)
    {
        bool res {};
        switch (opType) {
            case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_EQUAL: {
                res = *left == *right;
                break;
            }
            case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_NOT_EQUAL: {
                res = !(*left == *right);
                break;
            }
            default: {
                return nullptr;
            }
        }
        return CreateBooleanLiteral(res);
    }

    template <typename InputType>
    auto PerformRelationOperation(InputType left, InputType right, lexer::TokenType opType)
    {
        bool res {};
        switch (opType) {
            case lexer::TokenType::PUNCTUATOR_GREATER_THAN: {
                res = left > right;
                break;
            }
            case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL: {
                res = left >= right;
                break;
            }
            case lexer::TokenType::PUNCTUATOR_LESS_THAN: {
                res = left < right;
                break;
            }
            case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL: {
                res = left <= right;
                break;
            }
            case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_EQUAL: {
                res = left == right;
                break;
            }
            case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_NOT_EQUAL: {
                res = left != right;
                break;
            }
            default: {
                ES2PANDA_UNREACHABLE();
            }
        }
        return CreateBooleanLiteral(res);
    }

    ir::Literal *HandleNumericalRelationalExpression(const ir::BinaryExpression *expr, const ir::NumberLiteral *left,
                                                     const ir::NumberLiteral *right)
    {
        auto opType = expr->OperatorType();
        switch (std::max(left->Number().GetTypeRank(), right->Number().GetTypeRank())) {
            case TypeRank::DOUBLE: {
                return PerformRelationOperation(ExtractFromLiteral<double>(left), ExtractFromLiteral<double>(right),
                                                opType);
            }
            case TypeRank::FLOAT: {
                return PerformRelationOperation(ExtractFromLiteral<float>(left), ExtractFromLiteral<float>(right),
                                                opType);
            }
            case TypeRank::INT64: {
                return PerformRelationOperation(ExtractFromLiteral<int64_t>(left), ExtractFromLiteral<int64_t>(right),
                                                opType);
            }
            case TypeRank::INT32: {
                return PerformRelationOperation(ExtractFromLiteral<int32_t>(left), ExtractFromLiteral<int32_t>(right),
                                                opType);
            }
            case TypeRank::INT16: {
                return PerformRelationOperation(ExtractFromLiteral<int16_t>(left), ExtractFromLiteral<int16_t>(right),
                                                opType);
            }
            case TypeRank::INT8: {
                return PerformRelationOperation(ExtractFromLiteral<int8_t>(left), ExtractFromLiteral<int8_t>(right),
                                                opType);
            }
            default: {
                ES2PANDA_UNREACHABLE();
            }
        }
    }

    ir::Literal *HandleNonNumericRelationalExpression(const ir::BinaryExpression *expr, const ir::Literal *left,
                                                      const ir::Literal *right)
    {
        auto opType = expr->OperatorType();

        if (left->IsStringLiteral() && right->IsStringLiteral()) {
            return PerformRelationOperation(left->AsStringLiteral()->Str(), right->AsStringLiteral()->Str(), opType);
        }

        if (left->IsBooleanLiteral() && right->IsBooleanLiteral()) {
            return PerformRelationOperation(GetVal<bool>(left), GetVal<bool>(right), opType);
        }

        if (left->IsCharLiteral() && right->IsCharLiteral()) {
            auto res = PerformRelationOperation(left->AsCharLiteral(), right->AsCharLiteral(), opType);
            if (res != nullptr) {
                return res;
            }
        }

        LogError(diagnostic::WRONG_OPERAND_TYPE_FOR_BINARY_EXPRESSION, {}, expr->Start());
        return nullptr;
    }

    template <typename SignedType>
    ir::Literal *PerformBitwiseLogicalOperation(SignedType left, SignedType right, lexer::TokenType opType)
    {
        using UnsignedType = std::make_unsigned_t<SignedType>;

        auto uLeft = bit_cast<UnsignedType, SignedType>(left);
        auto uRight = bit_cast<UnsignedType, SignedType>(right);

        SignedType res {};
        switch (opType) {
            case lexer::TokenType::PUNCTUATOR_BITWISE_AND: {
                res = uLeft & uRight;
                break;
            }
            case lexer::TokenType::PUNCTUATOR_BITWISE_OR: {
                res = uLeft | uRight;
                break;
            }
            case lexer::TokenType::PUNCTUATOR_BITWISE_XOR: {
                res = uLeft ^ uRight;
                break;
            }
            default:
                ES2PANDA_UNREACHABLE();
        }
        return CreateNumberLiteral(res);
    }

    ir::Literal *HandleNumericBitwiseLogicalExpression(const ir::BinaryExpression *expr, const ir::NumberLiteral *left,
                                                       const ir::NumberLiteral *right)
    {
        auto opType = expr->OperatorType();
        switch (std::max(left->Number().GetTypeRank(), right->Number().GetTypeRank())) {
            case TypeRank::DOUBLE: {
                return PerformBitwiseLogicalOperation(ExtractFromLiteral<int64_t>(left),
                                                      ExtractFromLiteral<int64_t>(right), opType);
            }
            case TypeRank::INT64: {
                return PerformBitwiseLogicalOperation(ExtractFromLiteral<int64_t>(left),
                                                      ExtractFromLiteral<int64_t>(right), opType);
            }
            case TypeRank::FLOAT:
            case TypeRank::INT32:
            case TypeRank::INT16:
            case TypeRank::INT8: {
                return PerformBitwiseLogicalOperation(ExtractFromLiteral<int32_t>(left),
                                                      ExtractFromLiteral<int32_t>(right), opType);
            }
            default:
                ES2PANDA_UNREACHABLE();
        }
    }

    ir::Literal *HandleNonNumericBitwiseLogicalExpression(const ir::BinaryExpression *expr, const ir::Literal *left,
                                                          const ir::Literal *right)
    {
        auto opType = expr->OperatorType();

        if (!left->IsBooleanLiteral() || !right->IsBooleanLiteral()) {
            LogError(diagnostic::WRONG_OPERAND_TYPE_FOR_BINARY_EXPRESSION, {}, expr->Start());
            return nullptr;
        }

        bool res = false;
        auto leftVal = left->AsBooleanLiteral()->Value();
        auto rightVal = right->AsBooleanLiteral()->Value();
        switch (opType) {
            case lexer::TokenType::PUNCTUATOR_BITWISE_AND: {
                res = leftVal && rightVal;
                break;
            }
            case lexer::TokenType::PUNCTUATOR_BITWISE_OR: {
                res = leftVal || rightVal;
                break;
            }
            case lexer::TokenType::PUNCTUATOR_BITWISE_XOR: {
                res = leftVal ^ rightVal;
                break;
            }
            default:
                ES2PANDA_UNREACHABLE();
        }
        return CreateBooleanLiteral(res);
    }

    ir::Literal *HandleLogicalExpression(const ir::BinaryExpression *expr, const ir::Literal *left,
                                         const ir::Literal *right)
    {
        bool lhs = TestLiteral(left);
        bool rhs = TestLiteral(right);

        bool res {};
        auto opType = expr->OperatorType();
        switch (opType) {
            case lexer::TokenType::PUNCTUATOR_LOGICAL_AND: {
                res = lhs && rhs;
                break;
            }
            case lexer::TokenType::PUNCTUATOR_LOGICAL_OR: {
                res = lhs || rhs;
                break;
            }
            default: {
                ES2PANDA_UNREACHABLE();
            }
        }
        return CreateBooleanLiteral(res);
    }

    ir::Literal *Calculate(const ir::BinaryExpression *expr)
    {
        ES2PANDA_ASSERT(inputs_.size() == 2U);
        auto left = inputs_[0];
        auto right = inputs_[1];
        if (IsLogicalExpression(expr)) {
            return HandleLogicalExpression(expr, left, right);
        }

        if (left->IsNumberLiteral() && right->IsNumberLiteral()) {
            auto leftN = left->AsNumberLiteral();
            auto rightN = right->AsNumberLiteral();
            if (IsBitwiseLogicalExpression(expr)) {
                return HandleNumericBitwiseLogicalExpression(expr, leftN, rightN);
            }
            if (IsMultiplicativeExpression(expr)) {
                return HandleMultiplicativeExpression(expr, leftN, rightN);
            }
            if (IsAdditiveExpression(expr)) {
                return HandleNumericAdditiveExpression(expr, leftN, rightN);
            }
            if (IsShiftExpression(expr)) {
                return HandleShiftExpression(expr, leftN, rightN);
            }
            if (IsRelationalExpression(expr)) {
                return HandleNumericalRelationalExpression(expr, leftN, rightN);
            }
        } else {
            if (IsAdditiveExpression(expr)) {
                return PerformStringAdditiveOperation(expr, left, right);
            }
            if (IsBitwiseLogicalExpression(expr)) {
                return HandleNonNumericBitwiseLogicalExpression(expr, left, right);
            }
            if (IsRelationalExpression(expr)) {
                return HandleNonNumericRelationalExpression(expr, left, right);
            }
        }

        // If the expression cannot be folded, it has no sence (like `1 ?? 2`), so raise type error.
        LogError(diagnostic::WRONG_OPERAND_TYPE_FOR_BINARY_EXPRESSION, {}, expr->Start());
        return nullptr;
    }

    void LogError(const diagnostic::DiagnosticKind &diagnostic, const util::DiagnosticMessageParams &diagnosticParams,
                  const lexer::SourcePosition &pos)
    {
        context_->diagnosticEngine->LogDiagnostic(diagnostic, diagnosticParams, pos);
    }

    ir::BooleanLiteral *CreateBooleanLiteral(bool val)
    {
        auto resNode = util::NodeAllocator::Alloc<ir::BooleanLiteral>(context_->allocator, val);
        ES2PANDA_ASSERT(resNode != nullptr);
        resNode->SetFolded();
        return resNode;
    }

    template <typename T>
    ir::NumberLiteral *CreateNumberLiteral(T val)
    {
        auto resNum = lexer::Number(val);
        auto *resNode = util::NodeAllocator::Alloc<ir::NumberLiteral>(context_->allocator, resNum);
        ES2PANDA_ASSERT(resNode != nullptr);

        // Some hack to set string representation of lexer::Number
        resNode->Number().SetStr(util::UString(resNode->ToString(), context_->allocator).View());

        resNode->SetFolded();
        return resNode;
    }

private:
    public_lib::Context *context_;
    SmallVector<ir::Literal *, 3U> inputs_;
};

template <typename T>
static bool TryCastInteger(lexer::Number &number)
{
    if (number.Is<T>()) {
        return true;
    }
    if (number.CanGetValue<T>()) {
        number = lexer::Number(number.GetValue<T>());
        return true;
    }
    return false;
}

auto TryExtractPrimitiveType(ir::TypeNode *constraint)
{
    if (constraint->IsETSPrimitiveType()) {
        return constraint->AsETSPrimitiveType()->GetPrimitiveType();
    }
    // NOTE(dkofanov): Check for known primitive type alias. Need to consider a 'number'->'double' lowering.
    if (auto typeRef = Cast<ir::ETSTypeReference>(constraint); typeRef != nullptr) {
        if (auto part = typeRef->Part(); part->Name()->IsIdentifier() && (part->Previous() == nullptr)) {
            const static std::map<std::string_view, ir::PrimitiveType> MAP {
                {"Number", ir::PrimitiveType::DOUBLE}, {"number", ir::PrimitiveType::DOUBLE},
                {"Double", ir::PrimitiveType::DOUBLE}, {"Float", ir::PrimitiveType::FLOAT},
                {"Long", ir::PrimitiveType::LONG},     {"Int", ir::PrimitiveType::INT},
                {"Short", ir::PrimitiveType::SHORT},   {"Char", ir::PrimitiveType::CHAR},
                {"Byte", ir::PrimitiveType::BYTE},     {"Boolean", ir::PrimitiveType::BOOLEAN},
            };

            if (auto it = MAP.find(part->Name()->AsIdentifier()->Name().Utf8()); it != MAP.end()) {
                return it->second;
            }
        }
    }
    return ir::PrimitiveType::VOID;
}

static void LogErrorUnconverted(ir::PrimitiveType dst, ir::PrimitiveType src, util::DiagnosticEngine *de,
                                lexer::SourcePosition pos)
{
    if ((dst == ir::PrimitiveType::FLOAT) && (src == ir::PrimitiveType::DOUBLE)) {
        de->LogDiagnostic(diagnostic::CONSTANT_FLOATING_POINT_COVERSION, util::DiagnosticMessageParams {}, pos);
    } else if (((dst != ir::PrimitiveType::FLOAT) && (dst != ir::PrimitiveType::DOUBLE)) &&
               ((src == ir::PrimitiveType::FLOAT) || (src == ir::PrimitiveType::DOUBLE))) {
        de->LogDiagnostic(diagnostic::CONSTANT_FLOATING_POINT_COVERSION, util::DiagnosticMessageParams {}, pos);
    } else {
        de->LogDiagnostic(diagnostic::CONSTANT_VALUE_OUT_OF_RANGE, util::DiagnosticMessageParams {}, pos);
    }
}

static bool CheckCastLiteral(util::DiagnosticEngine *de, ir::TypeNode *constraint, ir::Literal *literal)
{
    if (literal->IsStringLiteral()) {
        return true;
    }

    auto dst = TryExtractPrimitiveType(constraint);
    if (dst == ir::PrimitiveType::VOID) {
        // NOTE(dkofanov): ConstFolding supported only for primitives or strings.
        return false;
    }

    if (literal->IsBooleanLiteral() || literal->IsCharLiteral() || (dst == ir::PrimitiveType::BOOLEAN) ||
        (dst == ir::PrimitiveType::CHAR)) {
        return (literal->IsBooleanLiteral() && (dst == ir::PrimitiveType::BOOLEAN)) ||
               (literal->IsCharLiteral() && (dst == ir::PrimitiveType::CHAR));
    }

    auto &number = literal->AsNumberLiteral()->Number();
    bool converted = false;
    switch (dst) {
        case ir::PrimitiveType::DOUBLE:
            converted = TryCastInteger<double>(number);
            break;
        case ir::PrimitiveType::FLOAT:
            converted = TryCastInteger<float>(number);
            break;
        case ir::PrimitiveType::LONG:
            converted = TryCastInteger<int64_t>(number);
            break;
        case ir::PrimitiveType::INT:
            converted = TryCastInteger<int32_t>(number);
            break;
        case ir::PrimitiveType::SHORT:
            converted = TryCastInteger<int16_t>(number);
            break;
        case ir::PrimitiveType::BYTE:
            converted = TryCastInteger<int8_t>(number);
            break;
        default:
            ES2PANDA_UNREACHABLE();
    }

    if (!converted) {
        LogErrorUnconverted(dst, GetPrimitiveType(number), de, constraint->Start());
    }
    return converted;
}

static ir::TypeNode *GetTypeAnnotation(ir::AstNode *initParent)
{
    if (auto prop = Cast<ir::ClassProperty>(initParent); prop != nullptr) {
        ES2PANDA_ASSERT(prop->Key()->AsIdentifier()->TypeAnnotation() == nullptr);
        return prop->TypeAnnotation();
    }
    if (auto vardecl = Cast<ir::VariableDeclarator>(initParent);
        (vardecl != nullptr) && vardecl->Id()->IsIdentifier()) {
        return vardecl->Id()->AsIdentifier()->TypeAnnotation();
    }
    return nullptr;
}

bool ConstantExpressionLoweringImpl::CalculateAndCheck(DAGNode *user)
{
    auto *inputsIds = user->InputsIds();
    NodeCalculator nc {context_, inputsIds->size()};
    size_t i = 0;
    for (auto inputId : *inputsIds) {
        nc.SetInput(DNode(inputId)->Ir()->AsExpression()->AsLiteral(), i++);
    }

    auto *res = nc.Calculate(user);
    ES2PANDA_ASSERT(!res || res->IsLiteral());

    if (auto constr = GetTypeAnnotation(user->Ir()->Parent());
        (res == nullptr) || ((constr != nullptr) && !CheckCastLiteral(context_->diagnosticEngine, constr, res))) {
        user->UsersIds()->clear();
        return false;
    }

    RegisterReplacement(user, res);
    return true;
}

ir::Literal *NodeCalculator::Calculate(DAGNode *node)
{
    switch (node->Ir()->Type()) {
        case ir::AstNodeType::IDENTIFIER:
        case ir::AstNodeType::MEMBER_EXPRESSION:
            return SubstituteConstant();
        case ir::AstNodeType::CONDITIONAL_EXPRESSION:
            return SubstituteConstantConditionally();
        case ir::AstNodeType::UNARY_EXPRESSION:
            return Calculate(node->Ir()->AsUnaryExpression());
        case ir::AstNodeType::BINARY_EXPRESSION:
            return Calculate(node->Ir()->AsBinaryExpression());
        case ir::AstNodeType::TEMPLATE_LITERAL:
            return Calculate(node->Ir()->AsTemplateLiteral());
        default:
            ES2PANDA_UNREACHABLE();
    }
}

static ir::Expression *ExtendIdentToQualifiedName(ir::Identifier *ident)
{
    if (ident == nullptr) {
        return nullptr;
    }
    ir::Expression *rvnode = ident;
    if (auto mexp = Cast<ir::MemberExpression>(ident->Parent()); mexp != nullptr) {
        if (mexp->Property() != ident) {
            return nullptr;
        }
        // NOTE(dkofanov): 'MemberExpressionKind' should be eliminated and 'ir::MemberExpression' should be splitted
        // accordingly:
        if (mexp->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS) {
            if (auto mexpPar = Cast<ir::MemberExpression>(mexp->Parent());
                (mexpPar != nullptr) && (mexpPar->Object() == mexp)) {
                return nullptr;
            }
            rvnode = mexp;
        }
    }
    return rvnode;
}

// CC-OFFNXT(huge_cyclomatic_complexity, huge_cca_cyclomatic_complexity[C++]) solid logic
static ir::Expression *AsRValue(ir::Identifier *ident)
{
    ir::Expression *rvnode = ExtendIdentToQualifiedName(ident);
    if (rvnode == nullptr) {
        return nullptr;
    }
    auto parent = rvnode->Parent();
    ES2PANDA_ASSERT(parent != nullptr);
    if (parent->IsUnaryExpression() || parent->IsBinaryExpression() || parent->IsConditionalExpression()) {
        return rvnode;
    }
    auto isIn = [rvnode](auto &args) { return std::find(args.begin(), args.end(), rvnode) != args.end(); };
    // A list of contexts in which there will be an attempt to fold ident/mexp. May be revisited.
    if (auto vardecl = Cast<ir::VariableDeclarator>(parent); (vardecl != nullptr) && vardecl->Init() == rvnode) {
        return rvnode;
    }
    if (auto propdecl = Cast<ir::ClassProperty>(parent); (propdecl != nullptr) && propdecl->Value() == rvnode) {
        return rvnode;
    }
    if (auto enummemb = Cast<ir::TSEnumMember>(parent); (enummemb != nullptr) && enummemb->Init() == rvnode) {
        return rvnode;
    }
    if (auto casestmt = Cast<ir::SwitchCaseStatement>(parent); (casestmt != nullptr) && casestmt->Test() == rvnode) {
        return rvnode;
    }
    if (auto assignexp = Cast<ir::AssignmentExpression>(parent);
        (assignexp != nullptr) && assignexp->Right() == rvnode) {
        return rvnode;
    }
    if (auto callexp = Cast<ir::CallExpression>(parent); (callexp != nullptr) && isIn(callexp->Arguments())) {
        return rvnode;
    }
    if (auto newarr = Cast<ir::ETSNewArrayInstanceExpression>(parent);
        (newarr != nullptr) && (newarr->Dimension() == rvnode)) {
        return rvnode;
    }
    if (auto ar = Cast<ir::ETSNewMultiDimArrayInstanceExpression>(parent); (ar != nullptr) && isIn(ar->Dimensions())) {
        return rvnode;
    }
    if (auto cls = Cast<ir::ETSNewClassInstanceExpression>(parent); (cls != nullptr) && isIn(cls->GetArguments())) {
        return rvnode;
    }
    if (auto indexexp = Cast<ir::MemberExpression>(parent);
        (indexexp != nullptr) && (indexexp->Kind() == ir::MemberExpressionKind::ELEMENT_ACCESS) &&
        indexexp->Property() == rvnode) {
        return rvnode;
    }
    return nullptr;
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

// NOTE(dkofanov): Remove after enum refactoring. The reason for this function is to limit contexts where
// enum literals are folded because checker is not ready for implicit int->enum conversions required by the spec, while
// there are requirements for them being folded. Also note, that type of enum literals is not taken into account.
static bool AllowedEnumLiteralFoldingPosition(const ir::MemberExpression *enumLiteral)
{
    auto anotherEnumLitDecl = util::Helpers::FindAncestorGivenByType(enumLiteral, ir::AstNodeType::TS_ENUM_MEMBER);
    ES2PANDA_ASSERT((anotherEnumLitDecl == nullptr) || (anotherEnumLitDecl->AsTSEnumMember()->Key() != enumLiteral));

    return anotherEnumLitDecl != nullptr;
}

static varbinder::Variable *ResolveMemberExpressionProperty(const ir::MemberExpression *me)
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
        auto *classDef = declNode->IsClassDefinition()    ? declNode->AsClassDefinition()
                         : declNode->IsClassDeclaration() ? declNode->AsClassDeclaration()->Definition()
                                                          : nullptr;
        ES2PANDA_ASSERT(classDef != nullptr);

        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        scope = classDef->Scope();

        // NOTE(dkofanov): For some reason, EnumLiteralDecl relates to enum-declaration, while EnumDecl - to enum
        // members (i.e. enum literals declaration).
    } else if (decl->IsEnumLiteralDecl() && AllowedEnumLiteralFoldingPosition(me)) {
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

static varbinder::Variable *Resolve(const ir::Expression *identOrMexp)
{
    if (identOrMexp->IsIdentifier()) {
        return ResolveIdentifier(identOrMexp->AsIdentifier());
    }
    return ResolveMemberExpressionProperty(identOrMexp->AsMemberExpression());
}

// Access flags should be checked as use-site may be folded.
static bool CheckPrivateAccess(ir::ClassProperty *propDecl, ir::Expression *rval)
{
    ES2PANDA_ASSERT(propDecl->IsPrivateElement());
    auto *const cls = propDecl->Parent();
    auto *pred = rval->Parent();
    while (pred != nullptr) {
        if (pred == cls) {
            return true;
        }
        pred = pred->Parent();
    }
    return false;
}

// CC-OFFNXT(huge_cyclomatic_complexity, huge_cca_cyclomatic_complexity[C++]) solid logic
void ConstantExpressionLoweringImpl::PopulateDAGs(ir::Expression *node)
{
    if (auto literal = AsSupportedLiteral(node); literal != nullptr) {
        if (auto constr = GetTypeAnnotation(node->Parent());
            (constr == nullptr) || CheckCastLiteral(context_->diagnosticEngine, constr, literal)) {
            AddRootDNode(literal);
        }
    } else if (auto numLiteral = Cast<ir::NumberLiteral>(node); numLiteral != nullptr) {
        ES2PANDA_ASSERT(numLiteral->Number().ConversionError());
        AddDNodeToPretransform(numLiteral);
    } else if (auto identOrMExp = AsRValue(Cast<ir::Identifier>(node)); identOrMExp != nullptr) {
        auto var = Resolve(identOrMExp);
        auto decl = (var != nullptr) ? var->Declaration() : nullptr;
        if ((decl != nullptr) && (decl->IsConstDecl() || decl->IsReadonlyDecl())) {
            auto declnode = decl->Node();
            // NOTE(dkofanov): Constants initialized via static block/constructor are not supported.
            ir::Expression *init = nullptr;
            if (auto prop = Cast<ir::ClassProperty>(declnode);
                (prop != nullptr) && (!prop->IsPrivateElement() || CheckPrivateAccess(prop, identOrMExp))) {
                init = prop->Value();
                // NOTE(dkofanov): 'declnode' points to identifier instead of 'ir::VariableDeclarator'.
            } else if (auto enumdecl = Cast<ir::TSEnumMember>(declnode); enumdecl != nullptr) {
                init = enumdecl->Init();
            } else if (auto vardecl = Cast<ir::VariableDeclarator>(declnode->Parent()); vardecl != nullptr) {
                init = vardecl->Init();
            }
            if (init != nullptr) {
                AddDNode(identOrMExp, init);
            }
        }
    } else if (auto tmpl = Cast<ir::TemplateLiteral>(node); tmpl) {
        if (tmpl->Expressions().empty()) {
            AddDNodeToPretransform(tmpl);
        } else {
            AddDNode(tmpl, tmpl->Expressions());
        }
    } else if (auto unop = Cast<ir::UnaryExpression>(node); unop != nullptr) {
        AddDNode(unop, unop->Argument());
    } else if (auto binop = Cast<ir::BinaryExpression>(node); binop != nullptr) {
        AddDNode(binop, {binop->Left(), binop->Right()});
    } else if (auto condexpr = Cast<ir::ConditionalExpression>(node); condexpr != nullptr) {
        // Reduce conditional expression only if each input is known to avoid hiding errors in 'dead' code.
        AddDNode(condexpr, {condexpr->Test(), condexpr->Consequent(), condexpr->Alternate()});
    }
}

// Note: memberExpression can be constant when it is enum property access, this check will be enabled after Issue23082.
// for package, we need to check whether its every immediate-initializers is const expression.
static void CheckInitializerInPackage(public_lib::Context *context, ir::AstNode *node)
{
    auto log = [context](const auto &kind, auto pos) {
        context->diagnosticEngine->LogDiagnostic(kind, util::DiagnosticMessageParams {}, pos);
    };
    switch (node->Type()) {
        case ir::AstNodeType::EXPRESSION_STATEMENT: {
            auto assign = Cast<ir::AssignmentExpression>(node->AsExpressionStatement()->GetExpression());
            if (assign == nullptr) {
                return;
            }
            auto initTobeChecked = assign->Right();
            if ((initTobeChecked != nullptr) && (AsSupportedLiteral(initTobeChecked) == nullptr)) {
                log(diagnostic::INVALID_INIT_IN_PACKAGE, initTobeChecked->Start());
            }
            return;
        }
        case ir::AstNodeType::CLASS_PROPERTY: {
            if (auto init = node->AsClassProperty()->Value();
                (init != nullptr) && (AsSupportedLiteral(init) == nullptr)) {
                log(diagnostic::INVALID_INIT_IN_PACKAGE, init->Start());
            }
            return;
        }
        default:
            return;
    }
}

static void PostCheckGlobalIfPackage(public_lib::Context *context, ir::ClassDefinition *globalClass)
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
                          [context](ir::AstNode *node) { CheckInitializerInPackage(context, node); });
        }

        if (element->IsClassProperty() && element->AsClassProperty()->IsConst() &&
            !element->AsClassProperty()->NeedInitInStaticBlock()) {
            CheckInitializerInPackage(context, element);
        }
    }
}

bool ConstantExpressionLoweringImpl::PerformForModule(parser::Program *program, std::string_view name)
{
    if (program->GetFlag(parser::ProgramFlags::AST_CONSTANT_EXPRESSION_LOWERED)) {
        return true;
    }

    program->Ast()->IterateRecursively([this](ir::AstNode *node) {
        if (node->IsExpression()) {
            PopulateDAGs(node->AsExpression());
        }
    });

    Pretransform();
    while (PerformStep()) {
    }

    // Preorder to match the "super"-expression.
    program->Ast()->TransformChildrenRecursivelyPreorder(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [this](ir::AstNode *node) -> ir::AstNode * {
            if (!node->IsExpression()) {
                return node;
            }
            auto expr = node->AsExpression();
            if (replacements_.find(expr) == replacements_.end()) {
                return node;
            }
            auto folded = replacements_[expr];
            folded->SetParent(expr->Parent());
            folded->SetRange(expr->Range());
            return folded;
        },
        name);

    if (program->IsPackage()) {
        PostCheckGlobalIfPackage(context_, program->GlobalClass());
    }

    program->SetFlag(parser::ProgramFlags::AST_CONSTANT_EXPRESSION_LOWERED);
    return true;
}

}  // namespace ark::es2panda::compiler
