/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include <compiler/core/pandagen.h>
#include <ir/expressions/binaryExpression.h>
#include <ir/expressions/unaryExpression.h>

namespace panda::es2panda::compiler {

static void CompileComparison(PandaGen *pg, const ir::BinaryExpression *binExpr, Label *target, bool branchOnTrue)
{
    RegScope rs(pg);
    VReg lhs = pg->AllocReg();

    binExpr->Left()->Compile(pg);
    pg->StoreAccumulator(binExpr, lhs);
    binExpr->Right()->Compile(pg);
    pg->ConditionBranch(binExpr, binExpr->OperatorType(), lhs, target, branchOnTrue);
}

static void CompileLogical(PandaGen *pg, const ir::BinaryExpression *binExpr, Label *target, bool branchOnTrue)
{
    // Compile each operand as a condition so nested negations
    // and comparisons merge instead of materializing booleans.
    // When either operand alone decides (`&&` on false, `||` on
    // true), both share the target; otherwise the first operand
    // branches with the inverted polarity to a local exit and
    // only the second operand can reach the target.
    const bool isAnd = binExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_AND;
    if (branchOnTrue != isAnd) {
        Condition::Compile(pg, binExpr->Left(), target, branchOnTrue);
        Condition::Compile(pg, binExpr->Right(), target, branchOnTrue);
    } else {
        auto *exitLabel = pg->AllocLabel();
        Condition::Compile(pg, binExpr->Left(), exitLabel, !branchOnTrue);
        Condition::Compile(pg, binExpr->Right(), target, branchOnTrue);
        pg->SetLabel(binExpr, exitLabel);
    }
}

void Condition::Compile(PandaGen *pg, const ir::Expression *expr, Label *target, bool branchOnTrue)
{
    if (expr->IsBinaryExpression()) {
        const auto *binExpr = expr->AsBinaryExpression();

        switch (binExpr->OperatorType()) {
            case lexer::TokenType::PUNCTUATOR_EQUAL:
            case lexer::TokenType::PUNCTUATOR_NOT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_LESS_THAN:
            case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL:
            case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
            case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL: {
                // Comparison instructions yield exactly true/false, so the
                // branch can test them directly (jeqz) instead of running an
                // identity istrue/isfalse first.
                CompileComparison(pg, binExpr, target, branchOnTrue);
                return;
            }
            case lexer::TokenType::KEYW_IN: {
                // `a in b` yields exactly true/false, so it can branch
                // directly; private-name `in` has its own emission forms and
                // keeps the generic path.
                if (binExpr->Left()->IsPrivateIdentifier()) {
                    break;
                }
                CompileComparison(pg, binExpr, target, branchOnTrue);
                return;
            }
            case lexer::TokenType::PUNCTUATOR_LOGICAL_AND:
            case lexer::TokenType::PUNCTUATOR_LOGICAL_OR: {
                CompileLogical(pg, binExpr, target, branchOnTrue);
                return;
            }
            default: {
                break;
            }
        }
    } else if (expr->IsUnaryExpression() &&
               expr->AsUnaryExpression()->OperatorType() == lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK) {
        Compile(pg, expr->AsUnaryExpression()->Argument(), target, !branchOnTrue);
        return;
    }

    // General case including some binExpr i.E.(a+b)
    expr->Compile(pg);
    if (branchOnTrue) {
        pg->BranchIfTrue(expr, target);
    } else {
        pg->BranchIfFalse(expr, target);
    }
}

}  // namespace panda::es2panda::compiler
