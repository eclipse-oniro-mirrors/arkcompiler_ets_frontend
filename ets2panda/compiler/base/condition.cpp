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

#include "condition.h"

#include "checker/ETSAnalyzerHelpers.h"
#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/binaryExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/unaryExpression.h"

namespace ark::es2panda::compiler {
bool Condition::CompileBinaryExpr(PandaGen *pg, const ir::BinaryExpression *binExpr, Label *falseLabel)
{
    switch (binExpr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL: {
            // This is a special case
            // These operators are expressed via cmp instructions and the following
            // if-else branches. Condition also expressed via cmp instruction and
            // the following if-else.
            // the goal of this method is to merge these two sequences of instructions.
            RegScope rs(pg);
            VReg lhs = pg->AllocReg();

            binExpr->Left()->Compile(pg);
            pg->StoreAccumulator(binExpr, lhs);
            binExpr->Right()->Compile(pg);
            pg->Condition(binExpr, binExpr->OperatorType(), lhs, falseLabel);
            return true;
        }
        case lexer::TokenType::PUNCTUATOR_LOGICAL_AND: {
            binExpr->Left()->Compile(pg);
            pg->ToBoolean(binExpr);
            pg->BranchIfFalse(binExpr, falseLabel);

            binExpr->Right()->Compile(pg);
            pg->ToBoolean(binExpr);
            pg->BranchIfFalse(binExpr, falseLabel);
            return true;
        }
        case lexer::TokenType::PUNCTUATOR_LOGICAL_OR: {
            auto *endLabel = pg->AllocLabel();

            binExpr->Left()->Compile(pg);
            pg->ToBoolean(binExpr);
            pg->BranchIfTrue(binExpr, endLabel);

            binExpr->Right()->Compile(pg);
            pg->ToBoolean(binExpr);
            pg->BranchIfFalse(binExpr, falseLabel);
            pg->SetLabel(binExpr, endLabel);
            return true;
        }
        default: {
            break;
        }
    }
    return false;
}

void Condition::Compile(PandaGen *pg, const ir::Expression *expr, Label *falseLabel)
{
    if (expr->IsBinaryExpression()) {
        if (CompileBinaryExpr(pg, expr->AsBinaryExpression(), falseLabel)) {
            return;
        }
    } else if (expr->IsUnaryExpression() &&
               expr->AsUnaryExpression()->OperatorType() == lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK) {
        expr->AsUnaryExpression()->Argument()->Compile(pg);

        pg->Negate(expr);
        pg->BranchIfFalse(expr, falseLabel);
        return;
    }

    // General case including some binExpr i.E.(a+b)
    expr->Compile(pg);
    pg->ToBoolean(expr);
    pg->BranchIfFalse(expr, falseLabel);
}

bool Condition::CompileBinaryExprForBigInt(ETSGen *etsg, const ir::BinaryExpression *expr, Label *falseLabel)
{
    if ((expr->Left()->TsType() == nullptr) || (expr->Right()->TsType() == nullptr)) {
        return false;
    }

    if (!expr->Left()->TsType()->IsETSBigIntType()) {
        return false;
    }

    if (!expr->Right()->TsType()->IsETSBigIntType()) {
        return false;
    }

    std::string_view signature = compiler::Signatures::ANY;
    switch (expr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
            signature = compiler::Signatures::BUILTIN_BIGINT_OPERATOR_LESS_THAN;
            break;
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL:
            signature = compiler::Signatures::BUILTIN_BIGINT_OPERATOR_LESS_THAN_EQUAL;
            break;
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
            signature = compiler::Signatures::BUILTIN_BIGINT_OPERATOR_GREATER_THAN;
            break;
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL:
            signature = compiler::Signatures::BUILTIN_BIGINT_OPERATOR_GREATER_THAN_EQUAL;
            break;
        default:
            // Other operations are handled in the CompileBinaryExpr function
            return false;
    }

    auto ttctx = TargetTypeContext(etsg, expr->OperationType());
    RegScope rs(etsg);
    VReg lhs = etsg->AllocReg();
    expr->Left()->Compile(etsg);
    etsg->ApplyConversionAndStoreAccumulator(expr->Left(), lhs, expr->OperationType());
    expr->Right()->Compile(etsg);
    etsg->ApplyConversion(expr->Right(), expr->OperationType());
    compiler::VReg rhs = etsg->AllocReg();
    etsg->StoreAccumulator(expr, rhs);
    etsg->CallBigIntBinaryComparison(expr, lhs, rhs, signature);
    etsg->BranchIfFalse(expr, falseLabel);

    return true;
}

void Condition::CompileInstanceofExpr(ETSGen *etsg, const ir::BinaryExpression *binExpr, Label *falseLabel)
{
    ES2PANDA_ASSERT(binExpr->OperatorType() == lexer::TokenType::KEYW_INSTANCEOF);
    binExpr->Compile(etsg);
    etsg->BranchIfFalse(binExpr, falseLabel);
}

void Condition::CompileLogical(ETSGen *etsg, const ir::BinaryExpression *binExpr, Label *falseLabel)
{
    ES2PANDA_ASSERT(binExpr->IsLogicalExtended());

    // If the Result is given, we can optimize the process.
    if (binExpr->Result() != nullptr) {
        if (binExpr->Result() != binExpr->Left()) {
            ES2PANDA_ASSERT(binExpr->Result() == binExpr->Right());
            etsg->CompileAndCheck(binExpr->Left());
        }
        Compile(etsg, binExpr->Result(), falseLabel);
        return;
    }

    compiler::RegScope rs(etsg);
    auto *endLabel = etsg->AllocLabel();

    if (binExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_AND) {
        Compile(etsg, binExpr->Left(), falseLabel);
    } else {
        ES2PANDA_ASSERT(binExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_OR);
        etsg->CompileAndCheck(binExpr->Left());
        etsg->BranchConditionalIfTrue(binExpr->Left(), endLabel);
    }

    Compile(etsg, binExpr->Right(), falseLabel);
    etsg->SetLabel(binExpr, endLabel);
}

bool Condition::CompileBinaryExpr(ETSGen *etsg, const ir::BinaryExpression *binExpr, Label *falseLabel)
{
    if (CompileBinaryExprForBigInt(etsg, binExpr, falseLabel)) {
        return true;
    }

    switch (binExpr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL: {
            auto ttctx = TargetTypeContext(etsg, binExpr->OperationType());

            RegScope rs(etsg);
            VReg lhs = etsg->AllocReg();

            binExpr->CompileOperands(etsg, lhs);
            etsg->Condition(binExpr, binExpr->OperatorType(), lhs, falseLabel);
            return true;
        }
        case lexer::TokenType::PUNCTUATOR_LOGICAL_OR:
        case lexer::TokenType::PUNCTUATOR_LOGICAL_AND: {
            CompileLogical(etsg, binExpr, falseLabel);
            return true;
        }
        case lexer::TokenType::KEYW_INSTANCEOF: {
            CompileInstanceofExpr(etsg, binExpr, falseLabel);
            return true;
        }
        default: {
            break;
        }
    }
    return false;
}

void Condition::Compile(ETSGen *etsg, const ir::Expression *expr, Label *falseLabel)
{
    if (expr->IsBinaryExpression()) {
        if (CompileBinaryExpr(etsg, expr->AsBinaryExpression(), falseLabel)) {
            return;
        }
    } else if (expr->IsUnaryExpression() &&
               expr->AsUnaryExpression()->OperatorType() == lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK) {
        expr->AsUnaryExpression()->Argument()->Compile(etsg);
        etsg->ApplyConversion(expr->AsUnaryExpression()->Argument(), etsg->Checker()->GlobalETSBooleanType());
        etsg->ResolveConditionalResultIfTrue<false, false>(expr, falseLabel);
        etsg->BranchIfTrue(expr, falseLabel);
        return;
    }
    expr->Compile(etsg);
    etsg->ApplyConversion(expr, etsg->Checker()->GlobalETSBooleanType());
    etsg->ResolveConditionalResultIfFalse(expr, falseLabel);
    etsg->BranchIfFalse(expr, falseLabel);
}
}  // namespace ark::es2panda::compiler
