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

#include "optionalLowering.h"
#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "ir/statements/blockStatement.h"
#include "ir/expressions/memberExpression.h"
#include "parser/ETSparser.h"
#include "varbinder/ETSBinder.h"

namespace ark::es2panda::compiler {

std::string_view OptionalLowering::Name() const
{
    return "OptionalLowering";
}

static ir::AstNode *LowerOptionalExpr(public_lib::Context *ctx, ir::Expression *const expr,
                                      ir::ChainExpression *const chain)
{
    ES2PANDA_ASSERT(expr->IsMemberExpression() || expr->IsCallExpression());

    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    auto *const varbinder = ctx->parserProgram->VarBinder()->AsETSBinder();

    auto expressionCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, NearestScope(expr));

    auto *ident = Gensym(allocator);
    ir::Expression *source =
        expr->IsMemberExpression() ? expr->AsMemberExpression()->Object() : expr->AsCallExpression()->Callee();

    // '0' act as placeholder
    auto *sequenceExpr = parser->CreateFormattedExpression("let @@I1 = @@E2; @@I3 == null ? undefined : 0", ident,
                                                           source, ident->Clone(allocator, nullptr));

    sequenceExpr->SetParent(chain->Parent());
    SetSourceRangesRecursively(sequenceExpr, chain->Range());
    InitScopesPhaseETS::RunExternalNode(sequenceExpr, varbinder);

    auto const &stmts = sequenceExpr->AsBlockExpression()->Statements();
    stmts[1]->AsExpressionStatement()->GetExpression()->AsConditionalExpression()->SetAlternate(chain->GetExpression());

    auto *identClone = ident->Clone(allocator, nullptr);
    expr->IsMemberExpression() ? expr->AsMemberExpression()->SetObject(identClone)
                               : expr->AsCallExpression()->SetCallee(identClone);

    return sequenceExpr;
}

static ir::Expression *FindOptionalInChain(ir::Expression *expr)
{
    if (expr->IsMemberExpression()) {
        auto typed = expr->AsMemberExpression();
        return typed->IsOptional() ? typed : FindOptionalInChain(typed->Object());
    }
    if (expr->IsCallExpression()) {
        auto typed = expr->AsCallExpression();
        return typed->IsOptional() ? typed : FindOptionalInChain(typed->Callee());
    }
    if (expr->IsTSNonNullExpression()) {
        return FindOptionalInChain(expr->AsTSNonNullExpression()->Expr());
    }
    ES2PANDA_UNREACHABLE();
}

static ir::AstNode *LowerChain(public_lib::Context *ctx, ir::ChainExpression *const chain)
{
    auto *optional = FindOptionalInChain(chain->GetExpression());
    if (optional->IsMemberExpression()) {
        auto *expr = optional->AsMemberExpression();
        ES2PANDA_ASSERT(expr->IsOptional());
        expr->ClearOptional();
        return LowerOptionalExpr(ctx, expr, chain);
    }

    if (optional->IsCallExpression()) {
        auto *expr = optional->AsCallExpression();
        ES2PANDA_ASSERT(expr->IsOptional());
        expr->ClearOptional();
        return LowerOptionalExpr(ctx, expr, chain);
    }

    ES2PANDA_UNREACHABLE();
}

bool OptionalLowering::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx = Context()](ir::AstNode *const node) -> ir::AstNode * {
            if (node->IsChainExpression()) {
                return RefineSourceRanges(LowerChain(ctx, node->AsChainExpression()));
            }
            return node;
        },
        Name());

    return true;
}

bool OptionalLowering::PostconditionForProgram(const parser::Program *program)
{
    return !program->Ast()->IsAnyChild([](const ir::AstNode *node) {
        return node->IsChainExpression() || (node->IsMemberExpression() && node->AsMemberExpression()->IsOptional()) ||
               (node->IsCallExpression() && node->AsCallExpression()->IsOptional());
    });
}

}  // namespace ark::es2panda::compiler
