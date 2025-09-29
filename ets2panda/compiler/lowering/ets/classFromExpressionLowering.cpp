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

#include "compiler/lowering/ets/classFromExpressionLowering.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

static constexpr std::string_view TYPE_REFERENCE_NAME = "typereference";

static ir::AstNode *LowerToSyntheticFromNode([[maybe_unused]] public_lib::Context *const ctx, ir::CallExpression *call)
{
    auto *allocator = ctx->Allocator();
    auto *varbinder = ctx->parserProgram->VarBinder()->AsETSBinder();
    auto *checker = ctx->GetChecker()->AsETSChecker();

    auto type = call->TypeParams()->Params()[0]->TsType();

    auto typeNode = allocator->New<ir::OpaqueTypeNode>(
        (type->IsETSVoidType() ? checker->GlobalETSAnyType() : checker->MaybeBoxType(type)), allocator);
    typeNode->SetRange(call->TypeParams()->Params()[0]->Range());

    ES2PANDA_ASSERT(call->TypeParams()->Params().size() == 1U);
    auto intrinsicExpr = util::NodeAllocator::ForceSetParent<ir::ETSIntrinsicNode>(
        allocator, TYPE_REFERENCE_NAME, ArenaVector<ir::Expression *>({typeNode}, ctx->Allocator()->Adapter()));
    intrinsicExpr->SetParent(call->Parent());
    intrinsicExpr->SetRange(call->Range());

    auto *scope = NearestScope(call);
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, scope);
    CheckLoweredNode(varbinder, ctx->GetChecker()->AsETSChecker(), intrinsicExpr);
    return intrinsicExpr;
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

bool ClassFromExpressionLowering::PerformForProgram(parser::Program *const program)
{
    program->Ast()->TransformChildrenRecursively(
        [ctx = Context()](ir::AstNode *ast) {
            if (ast->IsCallExpression() && IsCorrectFromCall(ctx, ast->AsCallExpression())) {
                return LowerToSyntheticFromNode(ctx, ast->AsCallExpression());
            }
            return ast;
        },
        Name());
    return true;
}
}  // namespace ark::es2panda::compiler