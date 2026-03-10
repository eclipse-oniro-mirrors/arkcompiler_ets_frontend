/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "conditionalSimplifyLowering.h"

#include "checker/ETSAnalyzerHelpers.h"
#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"
#include "ir/expressions/binaryExpression.h"
#include "ir/expressions/conditionalExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/ts/tsNonNullExpression.h"

namespace ark::es2panda::compiler {

static bool IsEnumOperandInParentBinaryExpression(ir::ConditionalExpression *const expr,
                                                  const checker::Type *const replacementType)
{
    if (!replacementType->IsETSEnumType()) {
        return false;
    }

    auto *const parent = expr->Parent();
    if (parent == nullptr || !parent->IsBinaryExpression()) {
        return false;
    }

    auto *const binary = parent->AsBinaryExpression();
    return binary->Left() == expr || binary->Right() == expr;
}

static const ir::Expression *SkipNonNullAssertions(const ir::Expression *expr)
{
    while (expr->IsTSNonNullExpression()) {
        expr = expr->AsTSNonNullExpression()->Expr();
    }
    return expr;
}

static bool IsSafeIdentifierLikeExpr(const ir::Expression *expr)
{
    expr = SkipNonNullAssertions(expr);
    if (!expr->IsIdentifier()) {
        return false;
    }
    auto *const declarationNode = DeclarationFromIdentifier(expr->AsIdentifier());
    if (declarationNode == nullptr) {
        return false;
    }

    return !declarationNode->IsETSParameterExpression();
}

static bool IsSafeToEraseConstantTest(const ir::Expression *const test)
{
    if (IsSafeIdentifierLikeExpr(test)) {
        return true;
    }

    if (test->IsLiteral()) {
        return true;
    }

    if (!test->IsBinaryExpression()) {
        return false;
    }

    auto *const binary = test->AsBinaryExpression();
    if (!binary->IsEquality()) {
        return false;
    }

    if (binary->Left()->IsNullLiteral() || binary->Left()->IsUndefinedLiteral()) {
        return IsSafeIdentifierLikeExpr(binary->Right());
    }
    if (binary->Right()->IsNullLiteral() || binary->Right()->IsUndefinedLiteral()) {
        return IsSafeIdentifierLikeExpr(binary->Left());
    }

    return false;
}

static ir::AstNode *SimplifyConditional(checker::ETSChecker *const checker, ir::ConditionalExpression *const expr)
{
    auto const testValue = checker::TryResolveConditionalTestValue(checker, expr->Test());
    if (!testValue.has_value() || !IsSafeToEraseConstantTest(expr->Test())) {
        return expr;
    }

    auto *const replacement = testValue.value() ? expr->Consequent() : expr->Alternate();
    auto *const replacementType = replacement->TsType();
    auto *const conditionalType = expr->TsType();
    if (replacementType == nullptr || conditionalType == nullptr ||
        !checker->IsTypeIdenticalTo(replacementType, conditionalType)) {
        return expr;
    }
    if (IsEnumOperandInParentBinaryExpression(expr, replacementType)) {
        return expr;
    }
    replacement->AddAstNodeFlags(expr->GetAstNodeFlags());
    replacement->SetParent(expr->Parent());
    return replacement;
}

bool ConditionalSimplifyLowering::PerformForProgram(parser::Program *program)
{
    auto *const checker = Context()->GetChecker()->AsETSChecker();
    program->Ast()->TransformChildrenRecursively(
        [checker](ir::AstNode *const node) {
            if (!node->IsConditionalExpression()) {
                return node;
            }
            return SimplifyConditional(checker, node->AsConditionalExpression());
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
