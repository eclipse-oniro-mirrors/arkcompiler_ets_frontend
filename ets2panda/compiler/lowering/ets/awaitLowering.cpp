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

#include "compiler/lowering/ets/awaitLowering.h"

#include "compiler/lowering/util.h"
#include "ir/expressions/awaitExpression.h"
#include "parser/ETSparser.h"

namespace ark::es2panda::compiler {

// Transform: await expr  -->  ({ let tmp = expr; await (tmp instanceof Promise ? tmp : Promise.resolve(tmp)); })
static ir::Expression *TransformAwaitExpression(public_lib::Context *ctx, ir::AwaitExpression *awaitExpr)
{
    auto *allocator = ctx->Allocator();
    auto *parser = ctx->parser->AsETSParser();
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *varbinder = checker->VarBinder()->AsETSBinder();
    auto *argument = awaitExpr->Argument();

    // Generate a unique temporary variable identifier
    auto *tmpIdent = Gensym(allocator);
    ES2PANDA_ASSERT(tmpIdent != nullptr);
    auto *tmpIdentClone1 = tmpIdent->Clone(allocator, nullptr);
    auto *tmpIdentClone2 = tmpIdent->Clone(allocator, nullptr);
    auto *tmpIdentClone3 = tmpIdent->Clone(allocator, nullptr);

    // Create the transformation using parser template
    auto *blockExpr = parser->CreateFormattedExpression(
        "let @@I1 = @@E2; (await (@@I3 instanceof Promise ? @@I4 as Promise<Any> : Promise.resolve(@@I5))) as @@T6",
        tmpIdent, argument, tmpIdentClone1, tmpIdentClone2, tmpIdentClone3, awaitExpr->TsType());

    blockExpr->SetParent(awaitExpr->Parent());

    // Set source ranges recursively on all generated nodes
    SetSourceRangesRecursively(blockExpr, awaitExpr->Range());

    // Enter the nearest scope and run binder/checker on the new expression
    auto *scope = NearestScope(awaitExpr->Parent());
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, scope);
    CheckLoweredNode(varbinder, checker, blockExpr);

    return blockExpr;
}

bool AwaitLoweringPhase::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        [ctx = Context()](checker::AstNodePtr ast) -> checker::AstNodePtr {
            if (ast->IsAwaitExpression()) {
                auto *awaitExpr = ast->AsAwaitExpression();
                auto *argType = awaitExpr->Argument()->TsType();

                // Only transform if the type is not definitely a Promise
                auto *checker = ctx->GetChecker()->AsETSChecker();
                if (!checker->IsPromiseType(argType)) {
                    return TransformAwaitExpression(ctx, awaitExpr);
                }
            }
            return ast;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
