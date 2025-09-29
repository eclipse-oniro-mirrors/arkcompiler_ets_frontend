/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "setterLowering.h"

#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
namespace ark::es2panda::compiler {

// This lowering transforms setter calls and indexable object set calls, to preserve the parameter value, so it can be
// passed after that. The transformation can be written like this Original code
// ----------------
// <lhs> = <rhs> // where <lhs> is a setter property or a set call on an indexable object
// ----------------
// Transformed
// ----------------
// const gensym: <rhs_type>;
// <lhs> = gensym := <rhs> ; // the whole rhs is is block expression here, to not introduce a new chained assignment
// gensym;
// ----------------
// The main purpose of the modification is to support cases like this
// (x, y are simple variables; c.x is a setter property)
// x = c.x = y
static bool IsSetterCallOrSetExpression(const ir::Expression *const expr)
{
    if (!expr->IsMemberExpression() && !expr->IsIdentifier()) {
        return false;
    }

    auto *const variable = expr->IsMemberExpression() ? expr->AsMemberExpression()->Property()->Variable()
                                                      : expr->AsIdentifier()->Variable();
    if (checker::ETSChecker::IsVariableGetterSetter(variable)) {
        ES2PANDA_ASSERT(variable != nullptr && variable->TsType() != nullptr);
        return variable->TsType()->HasTypeFlag(checker::TypeFlag::SETTER);
    }

    if (expr->IsIdentifier()) {
        return false;
    }
    const auto *memberExpr = expr->AsMemberExpression();
    // We are already checking the left side of an assignment expression, so the condition below will only match for
    // set expressions, but not get expressions
    const auto isSetExpression = [](const ir::MemberExpression *const possibleSetExpr) {
        return possibleSetExpr->Kind() == ir::MemberExpressionKind::ELEMENT_ACCESS &&
               possibleSetExpr->ObjType() != nullptr;
    };

    return isSetExpression(memberExpr);
}

static ir::Expression *TransformSetterCall(public_lib::Context *ctx,
                                           ir::AssignmentExpression *const assignmentExpression)
{
    auto *const allocator = ctx->Allocator();
    auto *const parser = ctx->parser->AsETSParser();

    ir::Identifier *gensymValue = Gensym(allocator);
    auto *gensymValueType = ctx->AllocNode<ir::OpaqueTypeNode>(assignmentExpression->Right()->TsType(), allocator);

    auto *setGensymAndEvalValue = parser->CreateFormattedExpression(
        "@@I1 = @@E2", gensymValue->Clone(allocator, nullptr), assignmentExpression->Right());

    std::stringstream ss;
    ss << "let @@I1: @@T2;";
    ss << "@@E3 = @@E4;";
    ss << "(@@I5);";

    return parser->CreateFormattedExpression(ss.str(), gensymValue, gensymValueType, assignmentExpression->Left(),
                                             setGensymAndEvalValue, gensymValue->Clone(allocator, nullptr));
}

bool SetterLowering::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        [ctx = Context()](ir::AstNode *const node) {
            if (!node->IsAssignmentExpression() ||
                !IsSetterCallOrSetExpression(node->AsAssignmentExpression()->Left())) {
                return node;
            }

            // NOTE (smartin): heuristics could be applied here, to only make the transformation when the result of the
            // call expression is reused. Now we'll transform every setter call, often BCO will take care of them.

            auto *checker = ctx->GetChecker()->AsETSChecker();
            auto *assignmentExpr = node->AsAssignmentExpression();
            ir::AstNode *loweringResult = TransformSetterCall(ctx, assignmentExpr);
            loweringResult->SetParent(assignmentExpr->Parent());

            auto *const scope = NearestScope(assignmentExpr);
            auto expressionCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(checker->VarBinder(), scope);
            InitScopesPhaseETS::RunExternalNode(loweringResult, checker->VarBinder());
            checker->VarBinder()->AsETSBinder()->ResolveReferencesForScopeWithContext(loweringResult, scope);
            checker::SavedCheckerContext scc {checker, checker::CheckerStatus::IGNORE_VISIBILITY};
            loweringResult->Check(checker);

            return loweringResult;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
