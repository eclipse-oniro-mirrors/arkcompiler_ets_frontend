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

#include "initModuleLowering.h"

#include "compiler/lowering/util.h"
#include "generated/diagnostic.h"
#include "generated/signatures.h"
#include "ir/astNode.h"
#include "ir/expression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/expressions/memberExpression.h"
#include "parser/innerSourceParser.h"
#include "util/es2pandaMacros.h"
#include "util/helpers.h"

namespace ark::es2panda::compiler {

static bool IsInitModuleCall(ir::AstNode *node)
{
    if (!node->IsCallExpression()) {
        return false;
    }
    auto *callee = node->AsCallExpression()->Callee();
    if (callee->IsIdentifier() && callee->AsIdentifier()->Name() == compiler::Signatures::INIT_MODULE_METHOD) {
        return true;  // NOLINT(readability-simplify-boolean-expr)
    }
    return false;
}

static ir::AstNode *TransformInitModuleCallExpression(ir::CallExpression *callExpr, public_lib::Context *ctx,
                                                      parser::Program *program)
{
    auto *parser = ctx->parser->AsETSParser();
    auto *allocator = ctx->allocator;
    auto dependentProg =
        parser->GetImportPathManager()->GatherImportInfo(program, callExpr->Arguments().front()->AsStringLiteral());
    if (dependentProg == nullptr) {
        // Replace the broken "InitModule" expression with error node. The error message has been logged in parser.
        ES2PANDA_ASSERT(ctx->diagnosticEngine->IsAnyError());
        auto node = util::NodeAllocator::Alloc<ir::Identifier>(allocator, allocator);
        node->SetRange(callExpr->Range());
        node->SetParent(callExpr->Parent());
        return node;
    }

    ArenaVector<ir::Expression *> params(allocator->Adapter());
    auto moduleStr = util::UString {
        std::string(dependentProg->ModuleInfo().modulePrefix).append(compiler::Signatures::ETS_GLOBAL), allocator};
    auto moduleName = util::NodeAllocator::Alloc<ir::StringLiteral>(allocator, moduleStr.View());

    params.emplace_back(moduleName);
    // Note (daizihan): #27086, we should not use stringLiteral as argument in ETSIntrinsicNode, should be TypeNode.
    auto moduleNode = util::NodeAllocator::Alloc<ir::ETSIntrinsicNode>(allocator, "typereference", std::move(params));
    auto initIdent =
        util::NodeAllocator::Alloc<ir::Identifier>(allocator, compiler::Signatures::CLASS_INITIALIZE_METHOD, allocator);
    auto callee = util::NodeAllocator::Alloc<ir::MemberExpression>(
        allocator, moduleNode, initIdent, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    auto const newCallExpr = util::NodeAllocator::Alloc<ir::CallExpression>(
        allocator, callee, ArenaVector<ir::Expression *>(allocator->Adapter()), nullptr, false, false);
    newCallExpr->SetParent(callExpr->Parent());
    return newCallExpr;
}

bool InitModuleLowering::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        [ctx = Context(), program](checker::AstNodePtr node) -> checker::AstNodePtr {
            if (IsInitModuleCall(node)) {
                return TransformInitModuleCallExpression(node->AsCallExpression(), ctx, program);
            }
            return node;
        },
        Name());
    return true;
}

}  // namespace ark::es2panda::compiler