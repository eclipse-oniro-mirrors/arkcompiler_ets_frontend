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

#include "expressionLambdaLowering.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::compiler {
static ir::AstNode *ConvertExpression(public_lib::Context *ctx, ir::ArrowFunctionExpression *const arrow)
{
    auto *const function = arrow->Function();
    auto *const scope = function->Scope();
    auto *const expr = function->Body()->AsExpression();
    auto *const allocator = ctx->Allocator();
    auto const adapter = allocator->Adapter();

    ArenaVector<ir::Statement *> statements(adapter);

    if ((function->ReturnTypeAnnotation() != nullptr && function->ReturnTypeAnnotation()->IsETSPrimitiveType() &&
         function->ReturnTypeAnnotation()->AsETSPrimitiveType()->GetPrimitiveType() == ir::PrimitiveType::VOID)) {
        statements.emplace_back(ctx->AllocNode<ir::ExpressionStatement>(expr));
    } else {
        statements.emplace_back(ctx->AllocNode<ir::ReturnStatement>(expr));
        function->AddFlag(ir::ScriptFunctionFlags::HAS_RETURN);
    }

    auto *const block = ctx->AllocNode<ir::BlockStatement>(allocator, std::move(statements));

    ES2PANDA_ASSERT(block);
    block->SetScope(scope);
    block->SetParent(function);

    function->SetBody(block);

    return arrow;
}

using AstNodePtr = ir::AstNode *;

bool ExpressionLambdaConstructionPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        [ctx](ir::AstNode *const node) -> AstNodePtr {
            if (node->IsArrowFunctionExpression() && node->AsArrowFunctionExpression()->Function()->HasBody() &&
                node->AsArrowFunctionExpression()->Function()->Body()->IsExpression()) {
                return ConvertExpression(ctx, node->AsArrowFunctionExpression());
            }

            return node;
        },
        Name());

    return true;
}

bool ExpressionLambdaConstructionPhase::PostconditionForModule([[maybe_unused]] public_lib::Context *ctx,
                                                               const parser::Program *program)
{
    return !program->Ast()->IsAnyChild([](const ir::AstNode *node) {
        return node->IsArrowFunctionExpression() && node->AsArrowFunctionExpression()->Function()->HasBody() &&
               node->AsArrowFunctionExpression()->Function()->Body()->IsExpression();
    });
}

}  // namespace ark::es2panda::compiler
