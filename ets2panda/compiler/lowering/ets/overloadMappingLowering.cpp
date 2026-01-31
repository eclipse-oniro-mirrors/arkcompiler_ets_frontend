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

#include "overloadMappingLowering.h"
#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

using AstNodePtr = ir::AstNode *;

std::string_view OverloadMappingLowering::Name() const
{
    return "OverloadMappingLowering";
}

static ir::CallExpression *MethodMapping(public_lib::Context *ctx, ir::CallExpression *callExpression)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *allocator = ctx->Allocator();

    auto *signature = callExpression->Signature();
    ES2PANDA_ASSERT(signature->OwnerVar()->Declaration()->Node()->IsMethodDefinition());
    auto *methodDef = signature->OwnerVar()->Declaration()->Node()->AsMethodDefinition();

    ir::AstNode *callee = callExpression->Callee();
    while (callee->IsMemberExpression()) {
        callee = callee->AsMemberExpression()->Property();
    }
    auto *ident = callee->AsIdentifier();
    ident->SetName(methodDef->Id()->Name());

    varbinder::LocalVariable *syntheticVariable = allocator->New<varbinder::LocalVariable>(
        varbinder::VariableFlags::SYNTHETIC | varbinder::VariableFlags::METHOD);
    ident->SetVariable(syntheticVariable);
    checker::ETSFunctionType *funcType =
        checker->CreateETSMethodType(methodDef->Id()->Name(), {{}, allocator->Adapter()});
    funcType->AddCallSignature(signature);
    syntheticVariable->SetTsType(funcType);

    return callExpression;
}

static bool IsOverloadDeclarationCall(ir::Expression *expr)
{
    while (expr->IsMemberExpression()) {
        expr = expr->AsMemberExpression()->Property();
    }

    if (expr->IsIdentifier() && expr->AsIdentifier()->Variable() != nullptr) {
        return expr->AsIdentifier()->Variable()->HasFlag(varbinder::VariableFlags::OVERLOAD);
    }
    return false;
}

bool OverloadMappingLowering::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx = Context()](ir::AstNode *const node) -> AstNodePtr {
            if (node->IsCallExpression() && IsOverloadDeclarationCall(node->AsCallExpression()->Callee())) {
                return MethodMapping(ctx, node->AsCallExpression());
            }
            return node;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
