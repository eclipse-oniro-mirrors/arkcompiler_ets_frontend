/*
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

#include "setterLowering.h"

#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
namespace ark::es2panda::compiler {

// This lowering transforms setter calls, to preserve the parameter value, so it can be passed after that.
// The transformation can be written like this
// Original code
// ----------------
// <lhs> = <rhs> // where <lhs> is a setter property
// ----------------
// Transformed
// ----------------
// const gensym = <rhs>;
// <lhs> = gensym;
// gensym;
// ----------------
// The main purpose of the modification is to support cases like this
// (x, y are simple variables; c.x is a setter property)
// x = c.x = y
static bool IsSetterCall(const ir::Expression *const expr)
{
    if (!expr->IsMemberExpression() && !expr->IsIdentifier()) {
        return false;
    }

    auto *const variable = expr->IsMemberExpression() ? expr->AsMemberExpression()->Property()->Variable()
                                                      : expr->AsIdentifier()->Variable();
    if (!checker::ETSChecker::IsVariableGetterSetter(variable)) {
        return false;
    }

    ES2PANDA_ASSERT(variable != nullptr && variable->TsType() != nullptr);

    return variable->TsType()->HasTypeFlag(checker::TypeFlag::SETTER);
}

static ir::Expression *TransformSetterCall(public_lib::Context *ctx,
                                           ir::AssignmentExpression *const assignmentExpression)
{
    auto *const allocator = ctx->Allocator();
    auto *const parser = ctx->parser->AsETSParser();

    ir::Identifier *gensym1 = Gensym(allocator);
    auto *gensymType = ctx->AllocNode<ir::OpaqueTypeNode>(assignmentExpression->Right()->TsType(), allocator);

    std::stringstream ss;
    ss << "const @@I1: @@T2 = @@E3;";
    ss << "@@E4 = @@I5;";
    ss << "(@@I6);";

    return parser->CreateFormattedExpression(ss.str(), gensym1, gensymType, assignmentExpression->Right(),
                                             assignmentExpression->Left(), gensym1->Clone(allocator, nullptr),
                                             gensym1->Clone(allocator, nullptr));
}

bool SetterLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        [ctx](ir::AstNode *const node) {
            if (!node->IsAssignmentExpression() || !IsSetterCall(node->AsAssignmentExpression()->Left())) {
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
