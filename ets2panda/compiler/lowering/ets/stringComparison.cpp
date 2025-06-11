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

#include "varbinder/ETSBinder.h"
#include "stringComparison.h"
#include "checker/ETSchecker.h"
#include "parser/parserImpl.h"
#include "libarkbase/utils/arena_containers.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

/**
 * Check if we got String comparison such like < , <=, >, >=, e.g.
 *
 *   let a:String = "AAAA"
 *   let b:String = "BBB"
 *   ..
 *   if (a >= b) {..}
 *
 * so such test has to be updated by our lowering to
 *
 *   if (a.CompareTo(b) >= 0)
 */

bool CheckOperatorType(ir::BinaryExpression *expr)
{
    switch (expr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL: {
            return true;
        }
        default:
            return false;
    }
}

bool StringComparisonLowering::IsStringComparison(ir::AstNode *node)
{
    if (node->IsBinaryExpression()) {
        auto *expr = node->AsBinaryExpression();

        if (!CheckOperatorType(expr)) {
            return false;
        }

        if ((expr->Left()->TsType() == nullptr) || (expr->Right()->TsType() == nullptr)) {
            return false;
        }

        if (expr->Left()->TsType()->IsETSStringType() && expr->Right()->TsType()->IsETSStringType()) {
            return true;
        }
    }
    return false;
}

void StringComparisonLowering::ProcessBinaryExpression(ir::BinaryExpression *expr, public_lib::Context *ctx)
{
    ES2PANDA_ASSERT(expr->IsBinaryExpression());
    ES2PANDA_ASSERT(expr->Left()->TsType()->IsETSStringType() && expr->Right()->TsType()->IsETSStringType());

    // reset types is any, will re-run checker to set them once again properly
    expr->SetTsType(nullptr);

    checker::ETSChecker *checker = ctx->GetChecker()->AsETSChecker();
    ArenaVector<ir::Expression *> callArgs(checker->Allocator()->Adapter());
    ir::Expression *accessor = nullptr;
    auto *zeroExpr = checker->AllocNode<ir::NumberLiteral>(lexer::Number(int32_t(0)));
    auto *const callee = checker->AllocNode<ir::Identifier>("compareTo", checker->Allocator());
    ES2PANDA_ASSERT(callee != nullptr);
    ES2PANDA_ASSERT(checker->GlobalBuiltinETSStringType() != nullptr);
    auto *var = checker->GlobalBuiltinETSStringType()->GetProperty(callee->AsIdentifier()->Name(),
                                                                   checker::PropertySearchFlags::SEARCH_METHOD);
    callee->SetVariable(var);
    accessor = checker->AllocNode<ir::MemberExpression>(expr->Left(), callee, ir::MemberExpressionKind::PROPERTY_ACCESS,
                                                        false, false);

    callArgs.push_back(expr->Right());
    auto callExpression = checker->AllocNode<ir::CallExpression>(accessor, std::move(callArgs), nullptr, false, false);
    expr->SetLeft(callExpression);
    expr->SetRight(zeroExpr);

    auto *parent = expr->Parent();
    InitScopesPhaseETS::RunExternalNode(expr, ctx->GetChecker()->VarBinder());
    checker->VarBinder()->AsETSBinder()->ResolveReferencesForScope(parent, NearestScope(parent));

    if (parent->IsBinaryExpression() || parent->IsConditionalExpression()) {
        parent->AsExpression()->SetTsType(nullptr);
        parent->Check(checker);
    } else {
        expr->Check(checker);
    }
}

bool StringComparisonLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    checker::ETSChecker *checker = ctx->GetChecker()->AsETSChecker();
    [[maybe_unused]] ArenaVector<ir::BinaryExpression *> foundNodes(checker->Allocator()->Adapter());
    // CC-OFFNXT(G.FMT.14-CPP) project code style
    program->Ast()->IterateRecursively([&foundNodes, this](ir::AstNode *ast) -> ir::AstNode * {
        if (IsStringComparison(ast)) {
            foundNodes.push_back(ast->AsBinaryExpression());
        }
        return ast;
    });

    for ([[maybe_unused]] auto &it : foundNodes) {
        ProcessBinaryExpression(it, ctx);
    }

    return true;
}
}  // namespace ark::es2panda::compiler
