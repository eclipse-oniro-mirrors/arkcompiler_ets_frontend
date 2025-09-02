/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "bigintLowering.h"

#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

std::string_view BigIntLowering::Name() const
{
    return "BigIntLowering";
}

// Helper function to check if a type is numeric (including boxed types)
static bool IsNumericType(public_lib::Context *ctx, checker::Type *type)
{
    if (type == nullptr) {
        return false;
    }
    auto checker = ctx->GetChecker()->AsETSChecker();
    return checker->CheckIfNumeric(type);
}

static bool IsFloatingPoint(ir::Expression *expr, public_lib::Context *ctx)
{
    if (expr->IsNumberLiteral()) {
        auto number = expr->AsNumberLiteral()->Number();
        return number.IsFloat() || number.IsDouble();
    }
    if (expr->TsType() != nullptr) {
        auto checker = ctx->GetChecker()->AsETSChecker();
        return checker->CheckIfFloatingPoint(expr->TsType());
    }
    return false;
}

ir::Expression *CreateBigInt(public_lib::Context *ctx, ir::BigIntLiteral *literal)
{
    auto parser = ctx->parser->AsETSParser();
    auto checker = ctx->GetChecker()->AsETSChecker();

    // This will change the bigint literal node into the new class instance expression:
    // 123456n => new BigInt("123456")
    std::string src {"new "};
    src += Signatures::BUILTIN_BIGINT_CLASS;
    src += "(\"";
    src += literal->Str().Utf8();
    src += "\")";
    auto loweringResult = parser->CreateExpression(src);
    loweringResult->SetParent(literal->Parent());
    InitScopesPhaseETS::RunExternalNode(loweringResult, checker->VarBinder());
    checker->VarBinder()->AsETSBinder()->ResolveReferencesForScope(loweringResult, NearestScope(loweringResult));
    loweringResult->Check(checker);
    return loweringResult;
}

ir::Expression *CreateBigIntFromNumericExpression(public_lib::Context *ctx, ir::Expression *expr)
{
    auto parser = ctx->parser->AsETSParser();
    auto checker = ctx->GetChecker()->AsETSChecker();
    // Check if the expression is a floating point type
    bool isFloatingPoint = IsFloatingPoint(expr, ctx);
    // Clone the expression first to avoid circular references
    auto *argumentExpr = expr->Clone(ctx->allocator, nullptr)->AsExpression();
    if (isFloatingPoint) {
        // For floating point numbers, truncate before passing to BigInt.
        argumentExpr = parser->CreateFormattedExpression("Math.trunc(@@E1)", argumentExpr);
    }
    ir::Expression *loweringResult = parser->CreateFormattedExpression("new BigInt(@@E1)", argumentExpr);
    // Copy source range information from the original expression
    loweringResult->SetRange(expr->Range());
    loweringResult->SetParent(expr->Parent());
    InitScopesPhaseETS::RunExternalNode(loweringResult, checker->VarBinder());
    checker->VarBinder()->AsETSBinder()->ResolveReferencesForScope(loweringResult, NearestScope(loweringResult));
    loweringResult->Check(checker);
    return loweringResult;
}

bool ConvertNumericExpressionToBigInt(public_lib::Context *ctx, ir::BinaryExpression *expr)
{
    // Don't apply BigInt conversion to logical operators - they should preserve original types
    auto op = expr->OperatorType();
    if (op == lexer::TokenType::PUNCTUATOR_LOGICAL_AND || op == lexer::TokenType::PUNCTUATOR_LOGICAL_OR ||
        op == lexer::TokenType::PUNCTUATOR_LOGICAL_AND_EQUAL || op == lexer::TokenType::PUNCTUATOR_LOGICAL_OR_EQUAL) {
        return false;
    }

    auto *left = expr->Left();
    auto *right = expr->Right();
    bool leftIsBigInt = (left->TsType() != nullptr && left->TsType()->IsETSBigIntType()) || left->IsBigIntLiteral();
    bool rightIsBigInt = (right->TsType() != nullptr && right->TsType()->IsETSBigIntType()) || right->IsBigIntLiteral();
    // Convert when one operand is BigInt and the other is numeric (including literals and expressions)
    if (leftIsBigInt && !rightIsBigInt && IsNumericType(ctx, right->TsType())) {
        ir::Expression *newRight = CreateBigIntFromNumericExpression(ctx, right);
        expr->SetRight(newRight);
        return true;
    }

    if (rightIsBigInt && !leftIsBigInt && IsNumericType(ctx, left->TsType())) {
        ir::Expression *newLeft = CreateBigIntFromNumericExpression(ctx, left);
        expr->SetLeft(newLeft);
        return true;
    }

    return false;
}

bool ReplaceStrictEqualByNormalEqual(ir::BinaryExpression *expr)
{
    auto left = expr->Left()->TsType();
    auto isBigintLeft = (left != nullptr && left->IsETSBigIntType()) || expr->Left()->IsBigIntLiteral();
    auto right = expr->Right()->TsType();
    auto isBigintRight = (right != nullptr && right->IsETSBigIntType()) || expr->Right()->IsBigIntLiteral();
    if (!isBigintLeft && !isBigintRight) {
        return false;
    }

    if (expr->OperatorType() == lexer::TokenType::PUNCTUATOR_STRICT_EQUAL) {
        expr->SetOperator(lexer::TokenType::PUNCTUATOR_EQUAL);
        return true;
    }
    if (expr->OperatorType() == lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL) {
        expr->SetOperator(lexer::TokenType::PUNCTUATOR_NOT_EQUAL);
        return true;
    }

    return false;
}

bool RemoveConst(ir::BinaryExpression *expr)
{
    bool isRemoved = false;
    auto left = expr->Left()->TsType();
    if (left != nullptr && left->IsETSBigIntType()) {
        left->RemoveTypeFlag(checker::TypeFlag::CONSTANT);
        isRemoved = true;
    }

    auto right = expr->Right()->TsType();
    if (right != nullptr && right->IsETSBigIntType()) {
        right->RemoveTypeFlag(checker::TypeFlag::CONSTANT);
        isRemoved = true;
    }

    return isRemoved;
}

bool BigIntLowering::PerformForModule(public_lib::Context *const ctx, parser::Program *const program)
{
    auto checker = ctx->GetChecker()->AsETSChecker();

    program->Ast()->TransformChildrenRecursivelyPostorder(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx, checker](ir::AstNode *ast) -> ir::AstNode * {
            if (ast->IsBigIntLiteral() && ast->Parent() != nullptr && ast->Parent()->IsClassProperty()) {
                return CreateBigInt(ctx, ast->AsBigIntLiteral());
            }

            if (ast->IsBinaryExpression()) {
                auto expr = ast->AsBinaryExpression();
                bool doCheck = ReplaceStrictEqualByNormalEqual(expr);
                doCheck |= RemoveConst(expr);
                doCheck |= ConvertNumericExpressionToBigInt(ctx, expr);
                if (doCheck) {
                    // Clear the type to force recalculation of the correct result type
                    expr->SetTsType(nullptr);
                    expr->Check(checker);
                }
            }

            return ast;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
