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

#include "compiler/lowering/ets/classFromExpressionLowering.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

static ir::AstNode *LowerToSyntheticFromNode([[maybe_unused]] public_lib::Context *const ctx, ir::CallExpression *call)
{
    auto *allocator = ctx->Allocator();
    auto *varbinder = ctx->parserProgram->VarBinder()->AsETSBinder();

    ES2PANDA_ASSERT(call->TypeParams()->Params().size() == 1U);
    auto classFromExpr = util::NodeAllocator::ForceSetParent<ir::ClassFromExpression>(
        allocator, call->TypeParams()->Params()[0]->Clone(allocator, nullptr));
    classFromExpr->SetParent(call->Parent());

    auto *scope = NearestScope(call);
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, scope);
    CheckLoweredNode(varbinder, ctx->GetChecker()->AsETSChecker(), classFromExpr);
    return classFromExpr;
}

static bool IsCorrectFromCall(public_lib::Context *const ctx, ir::CallExpression *call)
{
    if (!call->Callee()->IsMemberExpression() || call->TypeParams() == nullptr ||
        call->TypeParams()->Params().size() != 1 || !call->Arguments().empty()) {
        return false;
    }
    auto *callee = call->Callee()->AsMemberExpression();

    return ctx->GetChecker()->IsTypeIdenticalTo(callee->Object()->TsType(),
                                                ctx->GetChecker()->GetGlobalTypesHolder()->GlobalClassBuiltinType()) &&
           callee->Property()->IsIdentifier() &&
           callee->Property()->AsIdentifier()->Name().Is(compiler::Signatures::FROM);
}

bool ClassFromExpressionLowering::PerformForModule(public_lib::Context *const ctx, parser::Program *const program)
{
    program->Ast()->TransformChildrenRecursively(
        [ctx](ir::AstNode *ast) {
            if (ast->IsCallExpression() && IsCorrectFromCall(ctx, ast->AsCallExpression())) {
                return LowerToSyntheticFromNode(ctx, ast->AsCallExpression());
            }
            return ast;
        },
        Name());
    return true;
}
}  // namespace ark::es2panda::compiler