/*
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

#include "arithmetic.h"

#include "checker/types/ets/etsTupleType.h"
#include "checker/types/globalTypesHolder.h"
#include "checker/types/typeError.h"
#include "ir/ets/etsUnionType.h"
#include "lexer/token/token.h"

namespace ark::es2panda::checker {

struct BinaryArithmOperands {
    ir::BinaryExpression *expr;
    checker::Type *typeL;
    checker::Type *typeR;
    checker::Type *reducedL;
    checker::Type *reducedR;
};

struct BinaryOperatorParams {
    ir::Expression *left;
    ir::Expression *right;
    ir::Expression *expr;
    lexer::TokenType operationType;
    lexer::SourcePosition pos;
    bool isEqualOp;
};

struct TypeParams {
    checker::Type *leftType;
    checker::Type *rightType;
    Type *unboxedL;
    Type *unboxedR;
};

enum class BinaryExpressionValidity {
    NO_ERR = 0U,
    LHS_ERR = 1U << 0U,
    RHS_ERR = 1U << 1U,
    EITHER_ERR = 1U << 2U,
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    BOTH_ERR = LHS_ERR | RHS_ERR,
};

static BinaryArithmOperands GetBinaryOperands(ETSChecker *checker, ir::BinaryExpression *expr)
{
    auto typeL = expr->Left()->Check(checker);
    auto typeR = expr->Right()->Check(checker);
    auto unboxedL = checker->MaybeUnboxType(typeL);
    auto unboxedR = checker->MaybeUnboxType(typeR);
    return {expr, typeL, typeR, unboxedL, unboxedR};
}

static void LogOperatorCannotBeApplied(ETSChecker *checker, lexer::TokenType op, Type *typeL, Type *typeR,
                                       lexer::SourcePosition pos)
{
    checker->LogError(diagnostic::BINOP_INVALID_TYPE, {TokenToString(op), typeL, typeR}, pos);
}

static void LogOperatorCannotBeApplied(ETSChecker *checker, BinaryArithmOperands const &ops)
{
    LogOperatorCannotBeApplied(checker, ops.expr->OperatorType(), ops.typeL, ops.typeR, ops.expr->Start());
}

static inline void RepairTypeErrorsInOperands(Type **left, Type **right)
{
    if (IsTypeError(*left)) {
        *left = *right;
    }
    if (IsTypeError(*right)) {
        *right = *left;
    }
}

static inline BinaryArithmOperands RepairTypeErrorsInOperands(BinaryArithmOperands const &ops)
{
    BinaryArithmOperands res = ops;
    RepairTypeErrorsInOperands(&res.typeL, &res.typeR);
    RepairTypeErrorsInOperands(&res.reducedL, &res.reducedR);
    return res;
}

static inline void RepairTypeErrorWithDefault(Type **type, Type *dflt)
{
    if (IsTypeError(*type)) {
        *type = dflt;
    }
}

static bool CheckOpArgsTypeEq(ETSChecker *checker, Type *left, Type *right, Type *type)
{
    return ((left != nullptr) && (right != nullptr) && checker->IsTypeIdenticalTo(left, type) &&
            checker->IsTypeIdenticalTo(right, type));
}

static bool FindOpArgsType(ETSChecker *checker, Type *left, Type *right, Type *target)
{
    return (checker->Relation()->IsSupertypeOf(target, left) || checker->Relation()->IsSupertypeOf(target, right));
}

bool ETSChecker::CheckIfNumeric(Type *type)
{
    if (type == nullptr) {
        return false;
    }
    if (type->IsETSPrimitiveType()) {
        // NOTE(rsipka): Deprecated operations #28006, char should be removed from ETS_CONVERTIBLE_TO_NUMERIC flags
        return type->HasTypeFlag(TypeFlag::ETS_CONVERTIBLE_TO_NUMERIC) && !type->HasTypeFlag(TypeFlag::CHAR);
    }
    auto *unboxed = MaybeUnboxInRelation(type);
    // NOTE(rsipka): Deprecated operations #28006, char should be removed from ETS_CONVERTIBLE_TO_NUMERIC flags
    return (unboxed != nullptr) && unboxed->HasTypeFlag(TypeFlag::ETS_CONVERTIBLE_TO_NUMERIC) &&
           !unboxed->HasTypeFlag(TypeFlag::CHAR);
}

bool ETSChecker::CheckIfFloatingPoint(Type *type)
{
    if (type == nullptr) {
        return false;
    }
    auto *unboxed = MaybeUnboxInRelation(type);
    return (unboxed != nullptr) && (unboxed->IsFloatType() || unboxed->IsDoubleType());
}

static Type *EffectivePrimitiveTypeOfNumericOp(ETSChecker *checker, Type *left, Type *right)
{
    if (left->IsDoubleType() || right->IsDoubleType()) {
        return checker->GlobalDoubleType();
    }
    if (left->IsFloatType() || right->IsFloatType()) {
        return checker->GlobalFloatType();
    }
    if (left->IsLongType() || right->IsLongType()) {
        return checker->GlobalLongType();
    }
    // NOTE(dkofanov): Deprecated operations on 'char' #28006
    if (left->IsCharType() && right->IsCharType()) {
        return checker->GlobalCharType();
    }
    return checker->GlobalIntType();  // return int in case of primitive types by default
}

static Type *TryConvertToPrimitiveType(ETSChecker *checker, Type *type)
{
    if (type == nullptr) {
        return nullptr;
    }

    if (type->IsETSNumericEnumType()) {
        // Pull out the type argument to BaseEnum
        if (type->AsETSObjectType()->SuperType() != nullptr &&
            !type->AsETSObjectType()->SuperType()->TypeArguments().empty()) {
            auto *baseEnumArg = type->AsETSObjectType()->SuperType()->TypeArguments()[0];
            return checker->MaybeUnboxInRelation(baseEnumArg);
        }
        return checker->GlobalIntType();
    }

    if (type->IsETSStringEnumType()) {
        return checker->GlobalETSStringLiteralType();
    }
    return checker->MaybeUnboxInRelation(type);
}

static Type *EffectiveTypeOfNumericOp(ETSChecker *checker, Type *left, Type *right)
{
    ES2PANDA_ASSERT(checker->CheckIfNumeric(left) && checker->CheckIfNumeric(right));

    auto bothBoxed = left->IsETSUnboxableObject() && right->IsETSUnboxableObject();
    if (!bothBoxed) {
        return EffectivePrimitiveTypeOfNumericOp(checker, left, right);
    }

    auto globalTypesHolder = checker->GetGlobalTypesHolder();
    if (FindOpArgsType(checker, left, right, globalTypesHolder->GlobalDoubleBuiltinType())) {
        return globalTypesHolder->GlobalDoubleBuiltinType();
    }
    if (FindOpArgsType(checker, left, right, globalTypesHolder->GlobalFloatBuiltinType())) {
        return globalTypesHolder->GlobalFloatBuiltinType();
    }
    if (FindOpArgsType(checker, left, right, globalTypesHolder->GlobalLongBuiltinType())) {
        return globalTypesHolder->GlobalLongBuiltinType();
    }
    return globalTypesHolder->GlobalIntegerBuiltinType();  // return Int for Byte, Short, Int
}

// NOTE(dkofanov): Deprecated operations on 'char' #28006
static Type *BinaryGetPromotedType(ETSChecker *checker, Type *left, Type *right, bool const promote)
{
    Type *const unboxedL = TryConvertToPrimitiveType(checker, left);
    Type *const unboxedR = TryConvertToPrimitiveType(checker, right);
    if (unboxedL == nullptr || unboxedR == nullptr) {
        return nullptr;
    }

    Type *typeL = left;
    Type *typeR = right;

    bool const bothBoxed = !typeL->IsETSPrimitiveType() && !typeR->IsETSPrimitiveType();

    if (!promote) {
        return typeR;
    }

    if (!bothBoxed) {
        if (unboxedL->IsETSEnumType() || unboxedR->IsETSEnumType()) {
            return nullptr;
        }
        if (!typeL->IsETSPrimitiveType()) {
            typeL = checker->MaybeUnboxType(typeL);
        }
        if (!typeR->IsETSPrimitiveType()) {
            typeR = checker->MaybeUnboxType(typeR);
        }
    }

    if (checker->CheckIfNumeric(typeL) && checker->CheckIfNumeric(typeR)) {
        return EffectiveTypeOfNumericOp(checker, typeL, typeR);
    }
    if (checker->CheckIfNumeric(typeR)) {
        return typeR;
    }

    return typeL;
}

bool ETSChecker::CheckBinaryOperatorForBigInt(Type *left, Type *right, lexer::TokenType op)
{
    if ((left == nullptr) || (right == nullptr)) {
        return false;
    }

    // Allow operations between BigInt and numeric types - number will be converted to BigInt
    bool leftIsBigInt = left->IsETSBigIntType();
    bool rightIsBigInt = right->IsETSBigIntType();
    // Allow if either operand is BigInt.
    // The non-BigInt operand will be converted to BigInt during lowering.
    if ((leftIsBigInt && CheckIfNumeric(right)) || (rightIsBigInt && CheckIfNumeric(left))) {
        switch (op) {
            case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
            case lexer::TokenType::PUNCTUATOR_LESS_THAN:
            case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL:
            case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL:
            case lexer::TokenType::PUNCTUATOR_EQUAL:
            case lexer::TokenType::PUNCTUATOR_NOT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL:
                return true;
            default:
                break;
        }
    }

    if (!leftIsBigInt || !rightIsBigInt) {
        return false;
    }

    switch (op) {
        case lexer::TokenType::KEYW_INSTANCEOF:
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT:
            // This is handled in the main CheckBinaryOperator function
            return false;
        default:
            break;
    }

    return true;
}

bool ETSChecker::CheckBinaryPlusMultDivOperandsForUnionType(const Type *leftType, const Type *rightType,
                                                            const ir::Expression *left, const ir::Expression *right)
{
    std::stringstream ss;
    if (leftType->IsETSUnionType()) {
        LogError(diagnostic::BINOP_ON_UNION, {leftType}, left->Start());
        return false;
    }
    if (rightType->IsETSUnionType()) {
        LogError(diagnostic::BINOP_ON_UNION, {rightType}, right->Start());
        return false;
    }
    return true;
}

void ETSChecker::SetGenerateValueOfFlags(std::tuple<checker::Type *, checker::Type *, Type *, Type *> types,
                                         std::tuple<ir::Expression *, ir::Expression *> nodes)
{
    auto [leftType, rightType, _, __] = types;
    auto [left, right] = nodes;
    if (leftType->IsETSEnumType()) {
        left->AddAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
    }
    if (rightType->IsETSEnumType()) {
        right->AddAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
    }
}

static bool TypeIsAppropriateForArithmetic(const checker::Type *type, ETSChecker *checker)
{
    return type->HasTypeFlag(TypeFlag::ETS_CONVERTIBLE_TO_NUMERIC) ||
           (type->IsETSObjectType() &&
            checker->Relation()->IsSupertypeOf(checker->GetGlobalTypesHolder()->GlobalNumericBuiltinType(), type));
}

static checker::Type *CheckBinaryOperatorForNumericEnums(ETSChecker *checker, checker::Type *const leftType,
                                                         checker::Type *const rightType)
{
    if (!leftType->IsETSEnumType() && !rightType->IsETSEnumType()) {
        return nullptr;
    }
    if (TypeIsAppropriateForArithmetic(leftType, checker) && TypeIsAppropriateForArithmetic(rightType, checker)) {
        Type *leftNumeric;
        if (leftType->IsETSNumericEnumType()) {
            leftNumeric = checker->MaybeBoxInRelation(TryConvertToPrimitiveType(checker, leftType));
        } else {
            leftNumeric = leftType;
        }

        Type *rightNumeric;
        if (rightType->IsETSNumericEnumType()) {
            rightNumeric = checker->MaybeBoxInRelation(TryConvertToPrimitiveType(checker, rightType));
        } else {
            rightNumeric = rightType;
        }

        return EffectiveTypeOfNumericOp(checker, leftNumeric, rightNumeric);
    }

    return nullptr;
}

checker::Type *ETSChecker::CheckBinaryOperatorMulDivMod(
    std::tuple<ir::Expression *, ir::Expression *, lexer::TokenType, lexer::SourcePosition> op, bool isEqualOp,
    std::tuple<checker::Type *, checker::Type *, Type *, Type *> types)
{
    auto [left, right, operationType, pos] = op;
    auto [leftType, rightType, unboxedL, unboxedR] = types;

    // Try to handle errors on a lower level
    RepairTypeErrorsInOperands(&leftType, &rightType);
    RepairTypeErrorsInOperands(&unboxedL, &unboxedR);
    ERROR_TYPE_CHECK(this, leftType, return GlobalTypeError());

    auto const promotedType = BinaryGetPromotedType(this, leftType, rightType, !isEqualOp);
    if (!CheckBinaryPlusMultDivOperandsForUnionType(leftType, rightType, left, right)) {
        return GlobalTypeError();
    }

    if (promotedType == nullptr || !CheckIfNumeric(leftType) || !CheckIfNumeric(rightType)) {
        auto type = CheckBinaryOperatorForNumericEnums(this, leftType, rightType);
        if (type != nullptr) {
            return type;
        }
        LogError(diagnostic::OP_NONNUMERIC, {}, pos);
        return GlobalTypeError();
    }

    return promotedType;
}

checker::Type *ETSChecker::CheckBinaryBitwiseOperatorForNumericEnums(checker::Type *const leftType,
                                                                     checker::Type *const rightType)
{
    if (!leftType->IsETSEnumType() && !rightType->IsETSEnumType()) {
        return nullptr;
    }

    auto checkForEnumType = [=](auto type, bool hasLong) -> bool {
        ETSObjectFlags floatingPointType = hasLong ? ETSObjectFlags::BUILTIN_DOUBLE : ETSObjectFlags::BUILTIN_FLOAT;
        ETSObjectFlags integralType = hasLong ? ETSObjectFlags::BUILTIN_LONG : ETSObjectFlags::BUILTIN_INT;
        return (type->AsETSNumericEnumType()->CheckBuiltInType(this, floatingPointType) ||
                type->AsETSNumericEnumType()->CheckBuiltInType(this, integralType));
    };

    if (TypeIsAppropriateForArithmetic(leftType, this) && TypeIsAppropriateForArithmetic(rightType, this)) {
        if (leftType->IsETSNumericEnumType() && rightType->IsETSNumericEnumType()) {
            if (checkForEnumType(leftType, false) || checkForEnumType(rightType, false)) {
                return GlobalIntBuiltinType();
            }
            if (checkForEnumType(leftType, true) || checkForEnumType(rightType, true)) {
                return GlobalLongBuiltinType();
            }
        }
        if (leftType->IsFloatType() || rightType->IsFloatType()) {
            return GlobalIntBuiltinType();
        }
        if (leftType->IsDoubleType() || rightType->IsDoubleType()) {
            return GlobalLongBuiltinType();
        }
        if (leftType->IsLongType() || rightType->IsLongType()) {
            return GlobalLongBuiltinType();
        }
        return GlobalIntBuiltinType();
    }
    return nullptr;
}

static checker::Type *CheckBinaryOperatorPlusForEnums(ETSChecker *checker, checker::Type *const leftType,
                                                      checker::Type *const rightType)
{
    if (auto numericType = CheckBinaryOperatorForNumericEnums(checker, leftType, rightType); numericType != nullptr) {
        return numericType;
    }
    if ((leftType->IsETSStringEnumType() && (rightType->IsETSStringType() || rightType->IsETSStringEnumType())) ||
        (rightType->IsETSStringEnumType() && (leftType->IsETSStringType() || leftType->IsETSStringEnumType()))) {
        return checker->GlobalETSStringLiteralType();
    }
    return nullptr;
}

checker::Type *ETSChecker::CheckBinaryOperatorPlus(
    std::tuple<ir::Expression *, ir::Expression *, lexer::TokenType, lexer::SourcePosition> op, bool isEqualOp,
    std::tuple<checker::Type *, checker::Type *, Type *, Type *> types)
{
    auto [left, right, operationType, pos] = op;
    auto [leftType, rightType, unboxedL, unboxedR] = types;

    // Try to handle errors on a lower level
    RepairTypeErrorsInOperands(&leftType, &rightType);
    RepairTypeErrorsInOperands(&unboxedL, &unboxedR);
    ERROR_TYPE_CHECK(this, leftType, return GlobalTypeError());

    if (leftType->IsETSStringType() || rightType->IsETSStringType()) {
        if (operationType == lexer::TokenType::PUNCTUATOR_MINUS ||
            operationType == lexer::TokenType::PUNCTUATOR_MINUS_EQUAL) {
            LogError(diagnostic::OP_NONNUMERIC, {}, pos);
            return GlobalTypeError();
        }

        return HandleStringConcatenation(leftType, rightType);
    }

    if (!CheckBinaryPlusMultDivOperandsForUnionType(leftType, rightType, left, right)) {
        return GlobalTypeError();
    }
    auto const promotedType = BinaryGetPromotedType(this, leftType, rightType, !isEqualOp);
    if (promotedType == nullptr || !CheckIfNumeric(rightType) || !CheckIfNumeric(leftType)) {
        auto type = CheckBinaryOperatorPlusForEnums(this, leftType, rightType);
        if (type != nullptr) {
            return type;
        }
        LogError(diagnostic::BINOP_NONARITHMETIC_TYPE, {}, pos);
        return GlobalTypeError();
    }

    return promotedType;
}

[[maybe_unused]] static checker::Type *GetBitwiseCompatibleType(ETSChecker *checker, Type *const type)
{
    switch (checker->ETSType(type)) {
        case TypeFlag::BYTE: {
            return checker->GlobalByteType();
        }
        case TypeFlag::SHORT: {
            return checker->GlobalShortType();
        }
        case TypeFlag::CHAR: {
            return checker->GlobalCharType();
        }
        case TypeFlag::INT:
        case TypeFlag::FLOAT: {
            return checker->GlobalIntType();
        }
        case TypeFlag::LONG:
        case TypeFlag::DOUBLE: {
            return checker->GlobalLongType();
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
    return nullptr;
}

// NOTE(dkofanov): Deprecated operations on 'char' #28006
checker::Type *ETSChecker::CheckBinaryOperatorShift(
    std::tuple<ir::Expression *, ir::Expression *, lexer::TokenType, lexer::SourcePosition> op, bool isEqualOp,
    std::tuple<checker::Type *, checker::Type *, Type *, Type *> types)
{
    auto [left, right, operationType, pos] = op;
    auto [leftType, rightType, unboxedL, unboxedR] = types;

    RepairTypeErrorWithDefault(&leftType, GlobalIntBuiltinType());
    RepairTypeErrorWithDefault(&rightType, GlobalIntBuiltinType());
    RepairTypeErrorWithDefault(&unboxedL, GlobalIntType());
    RepairTypeErrorWithDefault(&unboxedR, GlobalIntType());

    if (leftType->IsETSUnionType() || rightType->IsETSUnionType()) {
        LogError(diagnostic::BINOP_UNION, {}, pos);
        return GlobalTypeError();
    }

    auto promotedLeftType = GetUnaryOperatorPromotedType(leftType, !isEqualOp);
    auto promotedRightType = GetUnaryOperatorPromotedType(rightType, !isEqualOp);
    if (promotedLeftType == nullptr || promotedRightType == nullptr || !CheckIfNumeric(promotedLeftType) ||
        !CheckIfNumeric(promotedRightType)) {
        auto type = CheckBinaryBitwiseOperatorForNumericEnums(leftType, rightType);
        if (type != nullptr) {
            return type;
        }
        LogError(diagnostic::OP_NONNUMERIC, {}, pos);
        return GlobalTypeError();
    }

    auto isPrim = promotedLeftType->IsETSPrimitiveType();
    auto unboxedProm = MaybeUnboxType(promotedLeftType);
    if (unboxedProm->IsFloatType() || unboxedProm->IsIntType()) {
        return isPrim ? GlobalIntType() : GetGlobalTypesHolder()->GlobalIntegerBuiltinType();
    }

    if (unboxedProm->IsLongType() || unboxedProm->IsDoubleType()) {
        return isPrim ? GlobalLongType() : GetGlobalTypesHolder()->GlobalLongBuiltinType();
    }

    if (unboxedProm->IsByteType() || unboxedProm->IsShortType()) {
        return promotedLeftType;
    }

    ES2PANDA_UNREACHABLE();
    return nullptr;
}

// NOTE(dkofanov): Deprecated operations on 'char' #28006
checker::Type *ETSChecker::CheckBinaryOperatorBitwise(
    std::tuple<ir::Expression *, ir::Expression *, lexer::TokenType, lexer::SourcePosition> op, bool isEqualOp,
    std::tuple<checker::Type *, checker::Type *, Type *, Type *> types)
{
    auto [left, right, operationType, pos] = op;
    auto [leftType, rightType, unboxedL, unboxedR] = types;

    RepairTypeErrorsInOperands(&leftType, &rightType);
    RepairTypeErrorsInOperands(&unboxedL, &unboxedR);
    ERROR_TYPE_CHECK(this, leftType, return GlobalTypeError());

    if (leftType->IsETSUnionType() || rightType->IsETSUnionType()) {
        LogError(diagnostic::BINOP_UNION, {}, pos);
        return GlobalTypeError();
    }

    if (CheckOpArgsTypeEq(this, unboxedL, unboxedR, GlobalETSBooleanType())) {
        return GetGlobalTypesHolder()->GlobalETSBooleanBuiltinType();
    }

    auto const promotedType = BinaryGetPromotedType(this, leftType, rightType, !isEqualOp);
    if (promotedType == nullptr || !CheckIfNumeric(rightType) || !CheckIfNumeric(leftType)) {
        auto type = CheckBinaryBitwiseOperatorForNumericEnums(leftType, rightType);
        if (type != nullptr) {
            return type;
        }
        LogError(diagnostic::OP_NONNUMERIC, {}, pos);
        return GlobalTypeError();
    }
    SetGenerateValueOfFlags(types, {left, right});

    auto isPrim = promotedType->IsETSPrimitiveType();
    auto unboxedProm = MaybeUnboxType(promotedType);
    if (unboxedProm->IsFloatType() || unboxedProm->IsIntType()) {
        return isPrim ? GlobalIntType() : GetGlobalTypesHolder()->GlobalIntegerBuiltinType();
    }

    if (unboxedProm->IsLongType() || unboxedProm->IsDoubleType()) {
        return isPrim ? GlobalLongType() : GetGlobalTypesHolder()->GlobalLongBuiltinType();
    }

    if (unboxedProm->IsByteType() || unboxedProm->IsShortType()) {
        return promotedType;
    }
    return nullptr;
}

checker::Type *ETSChecker::CheckBinaryOperatorLogical(ir::Expression *left, ir::Expression *right,
                                                      checker::Type *leftType, checker::Type *rightType, Type *unboxedL,
                                                      Type *unboxedR)
{
    RepairTypeErrorsInOperands(&leftType, &rightType);
    RepairTypeErrorsInOperands(&unboxedL, &unboxedR);
    ERROR_TYPE_CHECK(this, leftType, return GlobalTypeError());

    // Don't do any boxing for primitive type when another operand is Enum. Enum will become primitive type later.
    if (leftType->IsETSEnumType() || rightType->IsETSEnumType()) {
        left->RemoveAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
        right->RemoveAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
        return CreateETSUnionType({MaybeBoxExpression(left), MaybeBoxExpression(right)});
    }

    if (right->IsNumberLiteral() && !left->IsNumberLiteral() && leftType->IsBuiltinNumeric()) {
        return leftType;
    }
    if (left->IsNumberLiteral() && !right->IsNumberLiteral() && rightType->IsBuiltinNumeric()) {
        return rightType;
    }

    if (IsTypeIdenticalTo(leftType, rightType)) {
        return GetNonConstantType(leftType);
    }

    return CreateETSUnionType({MaybeBoxExpression(left), MaybeBoxExpression(right)});
}

static bool ContainsNumbers(ETSChecker *checker, Type *tp)
{
    auto isSubtypeOfNumeric = [checker](Type *tp2) {
        return checker->Relation()->IsSupertypeOf(checker->GetGlobalTypesHolder()->GlobalNumericBuiltinType(), tp2);
    };
    if (isSubtypeOfNumeric(tp)) {
        return true;
    }
    if (tp->IsETSUnionType()) {
        for (auto *constituent : tp->AsETSUnionType()->ConstituentTypes()) {
            if (isSubtypeOfNumeric(constituent)) {
                return true;
            }
        }
    }

    return false;
}

// CC-OFFNXT(huge_cyclomatic_complexity, huge_cca_cyclomatic_complexity[C++]) solid logic
// NOTE: code inside this function follows the broken logic
bool ETSChecker::CheckValidEqualReferenceType(checker::Type *const leftType, checker::Type *const rightType)
{
    auto isRelaxedType {[&](checker::Type *const type) -> bool {
        return (type->IsETSObjectType() && type->AsETSObjectType()->IsGlobalETSObjectType()) || type->IsETSAnyType() ||
               type->IsETSNullType() || type->IsETSUndefinedType();
    }};

    // Equality expression is always allowed for *magic types*
    if (isRelaxedType(leftType) || isRelaxedType(rightType)) {
        return true;
    }

    // Any two types that can be numeric are comparable
    if (ContainsNumbers(this, leftType) && ContainsNumbers(this, rightType)) {
        return true;
    }

    // Boolean and any type that can be numeric or char are not comparable
    if ((FindOpArgsType(this, leftType, rightType, GetGlobalTypesHolder()->GlobalNumericBuiltinType()) ||
         FindOpArgsType(this, leftType, rightType, GetGlobalTypesHolder()->GlobalCharBuiltinType())) &&
        FindOpArgsType(this, leftType, rightType, GetGlobalTypesHolder()->GlobalETSBooleanBuiltinType())) {
        return false;
    }

    // NOTE (mxlgv): Skip for unions. Required implementation of the specification section:
    // 7.25.6 Reference Equality Based on Actual Type (Union Equality Operators)
    if (leftType->IsETSUnionType()) {
        return leftType->AsETSUnionType()->IsOverlapWith(Relation(), rightType);
    }
    if (rightType->IsETSUnionType()) {
        return rightType->AsETSUnionType()->IsOverlapWith(Relation(), leftType);
    }

    // NOTE (mxlgv): Skip for generic. Required implementation of the specification section:
    // 7.25.6 Reference Equality Based on Actual Type (Type Parameter Equality Operators)
    if (leftType->HasTypeFlag(TypeFlag::GENERIC) || rightType->HasTypeFlag(TypeFlag::GENERIC)) {
        return true;
    }

    // Equality expression can only be applied to String and String, and BigInt and BigInt
    if (leftType->IsETSStringType() || rightType->IsETSStringType() || leftType->IsETSBigIntType() ||
        rightType->IsETSBigIntType()) {
        auto *const nonConstLhs = GetNonConstantType(leftType);
        auto *const nonConstRhs = GetNonConstantType(rightType);
        if (!Relation()->IsIdenticalTo(nonConstLhs, nonConstRhs) &&
            !Relation()->IsIdenticalTo(nonConstRhs, nonConstLhs)) {
            return false;
        }
    }

    if (FindOpArgsType(this, leftType, rightType, GetGlobalTypesHolder()->GlobalNumericBuiltinType()) &&
        (leftType->IsETSEnumType() || rightType->IsETSEnumType())) {
        return true;
    }

    // 7.24.5 Enumeration Relational Operators
    return leftType->IsETSEnumType() == rightType->IsETSEnumType();
}

std::tuple<Type *, Type *> ETSChecker::CheckBinaryOperatorStrictEqual(ir::Expression *left,
                                                                      lexer::TokenType operationType,
                                                                      lexer::SourcePosition pos,
                                                                      checker::Type *leftType, checker::Type *rightType)
{
    RepairTypeErrorsInOperands(&leftType, &rightType);
    // We still know that operation result should be boolean, so recover.
    ERROR_TYPE_CHECK(this, leftType, return std::make_tuple(GlobalETSBooleanBuiltinType(), GlobalETSObjectType()));

    if (!IsReferenceType(leftType) || !IsReferenceType(rightType)) {
        LogError(diagnostic::BINOP_NOT_REFERENCE, {}, pos);
        return {GlobalETSBooleanType(), GlobalETSObjectType()};
    }

    Relation()->SetNode(left);
    if (!CheckValidEqualReferenceType(leftType, rightType)) {
        LogOperatorCannotBeApplied(this, operationType, leftType, rightType, pos);
    } else if (!Relation()->IsCastableTo(leftType, rightType) && !Relation()->IsCastableTo(rightType, leftType)) {
        LogOperatorCannotBeApplied(this, operationType, leftType, rightType, pos);
    }

    return {GlobalETSBooleanType(), GlobalETSObjectType()};
}

static Type *HandelReferenceBinaryEquality(ETSChecker *checker, BinaryArithmOperands const &ops)
{
    [[maybe_unused]] auto const [expr, typeL, typeR, reducedL, reducedR] = ops;
    if ((typeR->IsETSNullType() && typeL->IsETSPrimitiveType()) ||
        (typeL->IsETSNullType() && typeR->IsETSPrimitiveType())) {
        return checker->CreateETSUnionType({typeL, typeR});
    }

    if (typeL->IsETSReferenceType() && typeR->IsETSReferenceType()) {
        checker->Relation()->SetNode(expr->Left());
        if (!checker->CheckValidEqualReferenceType(typeL, typeR)) {
            LogOperatorCannotBeApplied(checker, ops);
            return typeL;
        }
        return checker->CreateETSUnionType({typeL, typeR});
    }

    if ((reducedL->IsETSReferenceType() || reducedR->IsETSReferenceType()) &&
        !(typeL->IsETSNullType() || typeL->IsETSUndefinedType()) &&
        !(typeR->IsETSNullType() || typeR->IsETSUndefinedType())) {
        if (checker->CheckValidEqualReferenceType(checker->MaybeBoxType(typeL), checker->MaybeBoxType(typeR))) {
            return checker->CreateETSUnionType(
                {checker->MaybeBoxExpression(expr->Left()), checker->MaybeBoxExpression(expr->Right())});
        }
    }

    return nullptr;
}

static Type *CheckBinaryOperatorEqual(ETSChecker *checker, BinaryArithmOperands const &ops)
{
    [[maybe_unused]] auto const [expr, typeL, typeR, reducedL, reducedR] = ops;

    ERROR_TYPE_CHECK(checker, typeL, return checker->GlobalTypeError());

    if (reducedL->IsETSBooleanType() && reducedR->IsETSBooleanType()) {
        if (reducedL->IsConstantType() && reducedR->IsConstantType()) {
            return checker->GetGlobalTypesHolder()->GlobalETSBooleanBuiltinType();
        }
        if (checker->CheckIfNumeric(typeL) && checker->CheckIfNumeric(typeR) && typeL->IsETSUnboxableObject() &&
            typeR->IsETSUnboxableObject()) {
            return typeL;
        }
        return reducedL;
    }

    return HandelReferenceBinaryEquality(checker, ops);
}

// Satisfying the Chinese checker
static bool NonNumericTypesAreAppropriateForComparison(ETSChecker *checker, Type *leftType, Type *rightType)
{
    leftType = checker->MaybeUnboxType(leftType);
    rightType = checker->MaybeUnboxType(rightType);
    if (rightType->IsETSStringType() && leftType->IsETSStringType()) {
        return true;
    }
    if (leftType->IsETSEnumType() && rightType->IsETSEnumType()) {
        return checker->Relation()->IsIdenticalTo(leftType, rightType);
    }
    if ((leftType->IsETSStringEnumType() && rightType->IsETSStringType()) ||
        (leftType->IsETSStringType() && rightType->IsETSStringEnumType())) {
        return true;
    }
    if ((leftType->IsETSPrimitiveType() && rightType->IsETSNumericEnumType()) ||
        (leftType->IsETSNumericEnumType() && rightType->IsETSPrimitiveType())) {
        return true;
    }
    return false;
}

// NOTE(dkofanov): Deprecated operations on 'char' #28006
std::tuple<Type *, Type *> ETSChecker::CheckBinaryOperatorLessGreater(ir::Expression *left, ir::Expression *right,
                                                                      lexer::TokenType operationType,
                                                                      lexer::SourcePosition pos, bool isEqualOp,
                                                                      checker::Type *leftType, checker::Type *rightType,
                                                                      Type *unboxedL, Type *unboxedR)
{
    RepairTypeErrorsInOperands(&leftType, &rightType);
    RepairTypeErrorsInOperands(&unboxedL, &unboxedR);
    ERROR_TYPE_CHECK(this, leftType, return std::make_tuple(GlobalETSBooleanBuiltinType(), GlobalTypeError()));

    if ((leftType->IsETSUnionType() || rightType->IsETSUnionType()) &&
        operationType != lexer::TokenType::PUNCTUATOR_EQUAL &&
        operationType != lexer::TokenType::PUNCTUATOR_NOT_EQUAL &&
        operationType != lexer::TokenType::PUNCTUATOR_STRICT_EQUAL &&
        operationType != lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL) {
        LogError(diagnostic::BINOP_UNION, {}, pos);
        return {GlobalETSBooleanBuiltinType(), leftType};
    }

    auto const promotedType = BinaryGetPromotedType(this, leftType, rightType, !isEqualOp);

    if (leftType->IsETSUnionType() || rightType->IsETSUnionType()) {
        return {GlobalETSBooleanBuiltinType(),
                CreateETSUnionType({MaybeBoxExpression(left), MaybeBoxExpression(right)})};
    }

    if (promotedType != nullptr && unboxedL != nullptr && unboxedR != nullptr &&
        unboxedL->IsETSBooleanType() != unboxedR->IsETSBooleanType()) {
        LogOperatorCannotBeApplied(this, operationType, leftType, rightType, pos);
        return {GlobalETSBooleanBuiltinType(), leftType};
    }

    if (promotedType == nullptr) {
        if (!NonNumericTypesAreAppropriateForComparison(this, leftType, rightType)) {
            LogError(diagnostic::BINOP_INCOMPARABLE, {}, pos);
        }
        return {GlobalETSBooleanBuiltinType(), GlobalETSBooleanBuiltinType()};
    }

    return {GlobalETSBooleanBuiltinType(), promotedType};
}

static bool IsTypeRetainedAfterErasure(const Type *const typeToCheck)
{
    // NOTE (smartin): #30480 - Many checks are missing from this function, to be able to merge this patch in time.
    // These must be added.
    if (typeToCheck->IsETSTypeParameter()) {
        return false;
    }

    if (typeToCheck->IsETSFunctionType()) {
        auto *callSig = typeToCheck->AsETSFunctionType()->CallSignaturesOfMethodOrArrow().front();
        bool isSigRetained = IsTypeRetainedAfterErasure(callSig->ReturnType());
        for (const auto *param : callSig->Params()) {
            isSigRetained &= IsTypeRetainedAfterErasure(param->TsType());
        }

        return isSigRetained;
    }

    if (typeToCheck->IsETSUnionType()) {
        return typeToCheck->AsETSUnionType()->AllOfConstituentTypes(IsTypeRetainedAfterErasure);
    }

    return true;
}

static BinaryExpressionValidity AreTypesValidInInstanceofExpression(const ir::Expression *const right,
                                                                    const Type *const rightType)
{
    // NOTE (smartin): #30480 - many checks were removed intentionally, to be able to merge the fix in time, these will
    // need to be added
    bool isRightExprStringLiteral = right->IsETSStringLiteralType();
    if (right->IsETSUnionType()) {
        const auto &unionTypeTypes = right->AsETSUnionType()->Types();
        isRightExprStringLiteral |= std::any_of(unionTypeTypes.begin(), unionTypeTypes.end(),
                                                [](auto *type) { return type->IsETSStringLiteralType(); });
    }
    const bool isRightTypeRetained = IsTypeRetainedAfterErasure(rightType) && !isRightExprStringLiteral;
    return isRightTypeRetained ? BinaryExpressionValidity::NO_ERR : BinaryExpressionValidity::RHS_ERR;
}

std::tuple<Type *, Type *> ETSChecker::CheckBinaryOperatorInstanceOf(const ir::Expression *right,
                                                                     checker::Type *leftType, checker::Type *rightType)
{
    RepairTypeErrorsInOperands(&leftType, &rightType);
    ERROR_TYPE_CHECK(this, leftType, return std::make_tuple(GlobalETSBooleanBuiltinType(), GlobalTypeError()));

    const BinaryExpressionValidity exprValidity = AreTypesValidInInstanceofExpression(right, rightType);
    switch (exprValidity) {
        case BinaryExpressionValidity::NO_ERR: {
            break;
        }
        case BinaryExpressionValidity::RHS_ERR: {
            LogError(diagnostic::INVALID_INSTANCEOF_RHS_TYPE, {right->DumpEtsSrc()}, right->Start());
            break;
        }
        default:
            ES2PANDA_UNREACHABLE();
    }

    RemoveStatus(checker::CheckerStatus::IN_INSTANCEOF_CONTEXT);

    return {GlobalETSBooleanBuiltinType(),
            exprValidity == BinaryExpressionValidity::NO_ERR ? GlobalETSObjectType() : leftType};
}

template <typename T>
static void ConvertNumberLiteralTo(ETSChecker *checker, ir::NumberLiteral *lit, Type *toType)
{
    auto &number = lit->Number();
    number.SetValue(number.GetValueAndCastTo<T>());
    lit->SetTsType(checker->GetConstantBuiltinType(toType));
}

template <typename From, typename To>
static bool CheckNumberLiteralValue(ETSChecker *checker, ir::NumberLiteral const *const lit)
{
    auto const maxTo = static_cast<From>(std::numeric_limits<To>::max());
    auto const minTo = static_cast<From>(std::numeric_limits<To>::min());
    auto const val = lit->Number().GetValue<From>();
    if (val < minTo || val > maxTo) {
        checker->LogError(diagnostic::CONSTANT_VALUE_OUT_OF_RANGE, {}, lit->Start());
        return false;
    }
    return true;
}

//  Just to reduce the size of 'ConvertNumberLiteral'
static void ConvertIntegerNumberLiteral(ETSChecker *checker, ir::NumberLiteral *lit, ETSObjectType *fromType,
                                        ETSObjectType *toType)
{
    if (toType->HasObjectFlag(ETSObjectFlags::BUILTIN_LONG)) {
        ConvertNumberLiteralTo<int64_t>(checker, lit, checker->GlobalLongBuiltinType());
    } else if (toType->HasObjectFlag(ETSObjectFlags::BUILTIN_INT)) {
        if (!fromType->HasObjectFlag(ETSObjectFlags::BUILTIN_LONG) ||
            CheckNumberLiteralValue<int64_t, int32_t>(checker, lit)) {
            ConvertNumberLiteralTo<int32_t>(checker, lit, checker->GlobalIntBuiltinType());
        }
    } else if (toType->HasObjectFlag(ETSObjectFlags::BUILTIN_SHORT)) {
        if (fromType->HasObjectFlag(ETSObjectFlags::BUILTIN_LONG) &&
            !CheckNumberLiteralValue<int64_t, int16_t>(checker, lit)) {
            return;
        }
        if (fromType->HasObjectFlag(ETSObjectFlags::BUILTIN_INT) &&
            !CheckNumberLiteralValue<int32_t, int16_t>(checker, lit)) {
            return;
        }
        ConvertNumberLiteralTo<int16_t>(checker, lit, checker->GlobalShortBuiltinType());
    } else if (toType->HasObjectFlag(ETSObjectFlags::BUILTIN_BYTE)) {
        if (fromType->HasObjectFlag(ETSObjectFlags::BUILTIN_LONG) &&
            !CheckNumberLiteralValue<int64_t, int8_t>(checker, lit)) {
            return;
        }
        if (fromType->HasObjectFlag(ETSObjectFlags::BUILTIN_INT) &&
            !CheckNumberLiteralValue<int32_t, int8_t>(checker, lit)) {
            return;
        }
        if (fromType->HasObjectFlag(ETSObjectFlags::BUILTIN_SHORT) &&
            !CheckNumberLiteralValue<int16_t, int8_t>(checker, lit)) {
            return;
        }
        ConvertNumberLiteralTo<int8_t>(checker, lit, checker->GlobalByteBuiltinType());
    }
}

static void ConvertNumberLiteral(ETSChecker *checker, ir::NumberLiteral *lit, ETSObjectType *toType)
{
    ES2PANDA_ASSERT(toType->IsBuiltinNumeric() && lit->TsType()->IsBuiltinNumeric());

    if (auto *fromType = lit->TsType()->AsETSObjectType(); !checker->Relation()->IsIdenticalTo(fromType, toType)) {
        switch (static_cast<ETSObjectFlags>(toType->ObjectFlags() & ETSObjectFlags::BUILTIN_NUMERIC)) {
            case ETSObjectFlags::BUILTIN_DOUBLE:
                ConvertNumberLiteralTo<double>(checker, lit, checker->GlobalDoubleBuiltinType());
                break;

            case ETSObjectFlags::BUILTIN_FLOAT:
                if (fromType->HasObjectFlag(ETSObjectFlags::BUILTIN_DOUBLE)) {
                    checker->LogError(diagnostic::INVALID_ASSIGNMNENT, {fromType, toType}, lit->Start());
                } else {
                    ConvertNumberLiteralTo<float>(checker, lit, checker->GlobalFloatBuiltinType());
                }
                break;

            case ETSObjectFlags::BUILTIN_LONG:
            case ETSObjectFlags::BUILTIN_INT:
            case ETSObjectFlags::BUILTIN_SHORT:
            case ETSObjectFlags::BUILTIN_BYTE:
                if (fromType->HasObjectFlag(ETSObjectFlags::BUILTIN_FLOATING_POINT)) {
                    checker->LogError(diagnostic::INVALID_ASSIGNMNENT, {fromType, toType}, lit->Start());
                } else {
                    ConvertIntegerNumberLiteral(checker, lit, fromType, toType);
                }
                break;

            default:
                ES2PANDA_UNREACHABLE();
        }
    }
}

Type *ETSChecker::CheckBinaryOperatorNullishCoalescing(ir::Expression *left, ir::Expression *right,
                                                       lexer::SourcePosition pos)
{
    auto *leftType = left->TsType();
    leftType = GetNonNullishType(leftType);
    if (leftType->IsTypeError() && HasStatus(checker::CheckerStatus::IN_TYPE_INFER)) {
        return GlobalTypeError();
    }

    ERROR_TYPE_CHECK(this, leftType, return GlobalTypeError());

    if (leftType->IsETSPrimitiveType()) {
        LogError(diagnostic::COALESCE_NOT_REF, {}, pos);
    }

    auto *rightType = MaybeBoxType(right->TsType());
    if (IsTypeIdenticalTo(leftType, rightType)) {
        return leftType;
    }

    //  If possible and required update number literal type to the proper value (identical to left-side type)
    if (right->IsNumberLiteral() && leftType->IsBuiltinNumeric()) {
        ConvertNumberLiteral(this, right->AsNumberLiteral(), leftType->AsETSObjectType());
        return leftType;
    }

    return CreateETSUnionType({leftType, rightType});
}

using CheckBinaryFunction = std::function<checker::Type *(
    ETSChecker *, std::tuple<ir::Expression *, ir::Expression *, lexer::TokenType, lexer::SourcePosition> op,
    bool isEqualOp, std::tuple<checker::Type *, checker::Type *, Type *, Type *> types)>;

std::map<lexer::TokenType, CheckBinaryFunction> &GetCheckMap()
{
    static std::map<lexer::TokenType, CheckBinaryFunction> checkMap = {
        {lexer::TokenType::PUNCTUATOR_MULTIPLY, &ETSChecker::CheckBinaryOperatorMulDivMod},
        {lexer::TokenType::PUNCTUATOR_MULTIPLY_EQUAL, &ETSChecker::CheckBinaryOperatorMulDivMod},
        {lexer::TokenType::PUNCTUATOR_DIVIDE, &ETSChecker::CheckBinaryOperatorMulDivMod},
        {lexer::TokenType::PUNCTUATOR_DIVIDE_EQUAL, &ETSChecker::CheckBinaryOperatorMulDivMod},
        {lexer::TokenType::PUNCTUATOR_MOD, &ETSChecker::CheckBinaryOperatorMulDivMod},
        {lexer::TokenType::PUNCTUATOR_MOD_EQUAL, &ETSChecker::CheckBinaryOperatorMulDivMod},
        {lexer::TokenType::PUNCTUATOR_EXPONENTIATION, &ETSChecker::CheckBinaryOperatorMulDivMod},
        {lexer::TokenType::PUNCTUATOR_EXPONENTIATION_EQUAL, &ETSChecker::CheckBinaryOperatorMulDivMod},

        {lexer::TokenType::PUNCTUATOR_MINUS, &ETSChecker::CheckBinaryOperatorPlus},
        {lexer::TokenType::PUNCTUATOR_MINUS_EQUAL, &ETSChecker::CheckBinaryOperatorPlus},
        {lexer::TokenType::PUNCTUATOR_PLUS, &ETSChecker::CheckBinaryOperatorPlus},
        {lexer::TokenType::PUNCTUATOR_PLUS_EQUAL, &ETSChecker::CheckBinaryOperatorPlus},

        {lexer::TokenType::PUNCTUATOR_LEFT_SHIFT, &ETSChecker::CheckBinaryOperatorShift},
        {lexer::TokenType::PUNCTUATOR_LEFT_SHIFT_EQUAL, &ETSChecker::CheckBinaryOperatorShift},
        {lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT, &ETSChecker::CheckBinaryOperatorShift},
        {lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT_EQUAL, &ETSChecker::CheckBinaryOperatorShift},
        {lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT, &ETSChecker::CheckBinaryOperatorShift},
        {lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT_EQUAL, &ETSChecker::CheckBinaryOperatorShift},

        {lexer::TokenType::PUNCTUATOR_BITWISE_OR, &ETSChecker::CheckBinaryOperatorBitwise},
        {lexer::TokenType::PUNCTUATOR_BITWISE_OR_EQUAL, &ETSChecker::CheckBinaryOperatorBitwise},
        {lexer::TokenType::PUNCTUATOR_BITWISE_AND, &ETSChecker::CheckBinaryOperatorBitwise},
        {lexer::TokenType::PUNCTUATOR_BITWISE_AND_EQUAL, &ETSChecker::CheckBinaryOperatorBitwise},
        {lexer::TokenType::PUNCTUATOR_BITWISE_XOR, &ETSChecker::CheckBinaryOperatorBitwise},
        {lexer::TokenType::PUNCTUATOR_BITWISE_XOR_EQUAL, &ETSChecker::CheckBinaryOperatorBitwise},
    };

    return checkMap;
}

// CC-OFFNXT(G.FUN.01, huge_method) solid logic
static std::tuple<Type *, Type *> CheckBinaryOperatorHelper(ETSChecker *checker,
                                                            const BinaryOperatorParams &binaryParams,
                                                            const TypeParams &typeParams)
{
    ir::Expression *left = binaryParams.left;
    ir::Expression *right = binaryParams.right;
    lexer::SourcePosition pos = binaryParams.pos;
    checker::Type *const leftType = typeParams.leftType;
    checker::Type *const rightType = typeParams.rightType;
    checker::Type *tsType {};

    BinaryArithmOperands ops = GetBinaryOperands(checker, binaryParams.expr->AsBinaryExpression());
    BinaryArithmOperands opsRepaired = RepairTypeErrorsInOperands(ops);

    switch (binaryParams.operationType) {
        case lexer::TokenType::PUNCTUATOR_LOGICAL_AND:
        case lexer::TokenType::PUNCTUATOR_LOGICAL_OR: {
            tsType = checker->CheckBinaryOperatorLogical(left, right, leftType, rightType, typeParams.unboxedL,
                                                         typeParams.unboxedR);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_EQUAL: {
            if (auto res = CheckBinaryOperatorEqual(checker, opsRepaired); res != nullptr) {
                return {checker->GetGlobalTypesHolder()->GlobalETSBooleanBuiltinType(), res};
            }
            [[fallthrough]];
        }
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL: {
            return checker->CheckBinaryOperatorLessGreater(left, right, binaryParams.operationType, pos,
                                                           binaryParams.isEqualOp, leftType, rightType,
                                                           typeParams.unboxedL, typeParams.unboxedR);
        }
        case lexer::TokenType::KEYW_INSTANCEOF: {
            return checker->CheckBinaryOperatorInstanceOf(right, leftType, rightType);
        }
        case lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING: {
            tsType = checker->CheckBinaryOperatorNullishCoalescing(left, right, pos);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
            break;
        }
    }

    return {tsType, tsType};
}

static void TryAddValueOfFlagToStringEnumOperand(ir::Expression *op, const ir::Expression *otherOp)
{
    auto type = op->TsType();
    auto otherType = otherOp->TsType();
    if (type->IsETSStringEnumType() && (otherType->IsETSStringType() || otherType->IsETSStringEnumType())) {
        op->AddAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
    }
}

static void TryAddValueOfFlagToNumericEnumOperand(ir::Expression *op, const ir::Expression *otherOp)
{
    auto type = op->TsType();
    auto otherType = otherOp->TsType();
    if (type->IsETSNumericEnumType() &&
        ((otherType->IsETSObjectType() && otherType->AsETSObjectType()->IsBoxedPrimitive()) ||
         otherType->IsETSNumericEnumType())) {
        op->AddAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
    }
}

static void CheckEnumInOperatorContext(ir::Expression *expression, lexer::TokenType opType, ir::Expression *left,
                                       ir::Expression *right, ETSChecker *checker)
{
    auto [lType, rType] = std::tuple {left->TsType(), right->TsType()};

    switch (opType) {
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_EQUAL: {
            if (lType->IsETSEnumType() && rType->IsETSEnumType() && !checker->Relation()->IsIdenticalTo(lType, rType)) {
                checker->LogError(diagnostic::BINOP_INCOMPARABLE, {}, expression->Start());
                return;
            }
            [[fallthrough]];
        }
        case lexer::TokenType::PUNCTUATOR_PLUS: {
            TryAddValueOfFlagToStringEnumOperand(left, right);
            TryAddValueOfFlagToStringEnumOperand(right, left);
            [[fallthrough]];
        }
        case lexer::TokenType::PUNCTUATOR_MULTIPLY:
        case lexer::TokenType::PUNCTUATOR_DIVIDE:
        case lexer::TokenType::PUNCTUATOR_MOD:
        case lexer::TokenType::PUNCTUATOR_MINUS:
        case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_BITWISE_AND:
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR:
        case lexer::TokenType::PUNCTUATOR_BITWISE_XOR:
        case lexer::TokenType::PUNCTUATOR_LOGICAL_AND:
        case lexer::TokenType::PUNCTUATOR_LOGICAL_OR: {
            TryAddValueOfFlagToNumericEnumOperand(left, right);
            TryAddValueOfFlagToNumericEnumOperand(right, left);
            break;
        }
        default:
            // NOTE(dkofanov): What about the '+=' operation?
            break;
    }
}

std::tuple<Type *, Type *> ETSChecker::CheckArithmeticOperations(
    ir::Expression *expr, std::tuple<ir::Expression *, ir::Expression *, lexer::TokenType, lexer::SourcePosition> op,
    bool isEqualOp, std::tuple<checker::Type *, checker::Type *, Type *, Type *> types)
{
    auto [left, right, operationType, pos] = op;
    auto [leftType, rightType, unboxedL, unboxedR] = types;

    if (leftType->IsETSUnionType()) {
        leftType = GetNonConstantType(leftType);
    }

    if (rightType->IsETSUnionType()) {
        rightType = GetNonConstantType(rightType);
    }
    CheckEnumInOperatorContext(expr, operationType, left, right, this);

    auto checkMap = GetCheckMap();
    if (checkMap.find(operationType) != checkMap.end()) {
        auto check = checkMap[operationType];
        auto tsType = check(this, std::make_tuple(left, right, operationType, pos), isEqualOp,
                            std::make_tuple(leftType, rightType, unboxedL, unboxedR));
        if (tsType == nullptr) {
            return {leftType, rightType};
        }
        if (tsType->IsETSPrimitiveType()) {
            tsType = MaybeBoxType(tsType);
        }
        if (left->TsType()->IsTypeError()) {
            left->SetTsType(tsType);
        }
        if (right->TsType()->IsTypeError()) {
            right->SetTsType(tsType);
        }
        return {tsType, tsType};
    }

    return CheckBinaryOperatorHelper(this, {left, right, expr, operationType, pos, isEqualOp},
                                     {leftType, rightType, unboxedL, unboxedR});
}

static std::tuple<Type *, Type *> ResolveCheckBinaryOperatorForBigInt(ETSChecker *checker, Type *leftType,
                                                                      Type *rightType, lexer::TokenType operationType)
{
    switch (operationType) {
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL:
            return {checker->GlobalETSBooleanType(), checker->GlobalETSBooleanType()};
        default:
            return {leftType, rightType};
    }
}

std::tuple<Type *, Type *> ETSChecker::CheckBinaryOperator(ir::Expression *left, ir::Expression *right,
                                                           ir::Expression *expr, lexer::TokenType operationType,
                                                           lexer::SourcePosition pos, bool forcePromotion)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    checker::Type *leftType = left->Check(this);

    if (leftType == nullptr) {
        LogError(diagnostic::BINOP_UNEXPECTED_ERROR, {}, left->Start());
        auto rightType = right->Check(this);
        return {rightType, rightType};
    }

    if (operationType == lexer::TokenType::KEYW_INSTANCEOF) {
        AddStatus(checker::CheckerStatus::IN_INSTANCEOF_CONTEXT);
    }

    Context().CheckTestSmartCastCondition(operationType);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    checker::Type *rightType = right->Check(this);

    if (right->IsTypeNode()) {
        rightType = right->AsTypeNode()->GetType(this);
    }

    if (rightType == nullptr) {
        LogError(diagnostic::BINOP_UNEXPECTED_ERROR, {}, pos);
        return {leftType, leftType};
    }

    const bool isLogicalExtendedOperator = (operationType == lexer::TokenType::PUNCTUATOR_LOGICAL_AND) ||
                                           (operationType == lexer::TokenType::PUNCTUATOR_LOGICAL_OR);
    Type *unboxedL =
        isLogicalExtendedOperator ? MaybeUnboxConditionalInRelation(leftType) : MaybeUnboxInRelation(leftType);
    Type *unboxedR =
        isLogicalExtendedOperator ? MaybeUnboxConditionalInRelation(rightType) : MaybeUnboxInRelation(rightType);

    ES2PANDA_ASSERT(operationType != lexer::TokenType::PUNCTUATOR_SUBSTITUTION);
    bool isEqualOp = lexer::Token::IsBinaryLvalueToken(operationType) && !forcePromotion;

    if (CheckBinaryOperatorForBigInt(leftType, rightType, operationType)) {
        return ResolveCheckBinaryOperatorForBigInt(this, leftType, rightType, operationType);
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return CheckArithmeticOperations(expr, std::make_tuple(left, right, operationType, pos), isEqualOp,
                                     std::make_tuple(leftType, rightType, unboxedL, unboxedR));
}

}  // namespace ark::es2panda::checker
