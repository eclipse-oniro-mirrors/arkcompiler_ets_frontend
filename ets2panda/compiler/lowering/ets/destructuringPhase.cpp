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

#include "destructuringPhase.h"

#include "compiler/lowering/util.h"
#include "ir/ets/etsDestructuring.h"

namespace ark::es2panda::compiler {

static ir::AstNode *ProcessAssignmentExpression(public_lib::Context *ctx, ir::AssignmentExpression *assignmentExpr)
{
    auto *allocator = ctx->allocator;
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto *parser = ctx->parser->AsETSParser();
    auto varBinder = checker->VarBinder()->AsETSBinder();

    if (!assignmentExpr->Left()->IsETSDestructuring()) {
        return assignmentExpr;
    }

    auto blockStmt = ArenaVector<ir::Statement *>(allocator->Adapter());
    ir::Expression *rhsArrayExp = nullptr;
    if (assignmentExpr->Right()->IsIdentifier()) {
        rhsArrayExp = assignmentExpr->Right();
    } else {
        rhsArrayExp = Gensym(allocator);
        auto rhsType = util::NodeAllocator::ForceSetParent<ir::OpaqueTypeNode>(
            allocator, assignmentExpr->Right()->TsType(), allocator);
        blockStmt.push_back(parser->CreateFormattedStatement(
            "let @@I1:@@T2 = @@E3;", rhsArrayExp->Clone(allocator, nullptr), rhsType, assignmentExpr->Right()));
    }

    ArenaVector<ir::Expression *> elements = assignmentExpr->Left()->AsETSDestructuring()->Elements();
    size_t i = 0;
    for (auto e : elements) {
        ES2PANDA_ASSERT(!e->IsRestElement() && !e->IsAssignmentPattern());
        if (e->IsOmittedExpression()) {
            i++;
            continue;
        }
        const std::string copyStr = "@@I1 = @@I2[" + std::to_string(i++) + "];";
        blockStmt.push_back(parser->CreateFormattedStatement(copyStr, e->Clone(allocator, nullptr),
                                                             rhsArrayExp->Clone(allocator, nullptr)));
    }
    blockStmt.push_back(parser->CreateFormattedStatement("@@I1", rhsArrayExp->Clone(allocator, nullptr)));

    auto *blockExpr = util::NodeAllocator::ForceSetParent<ir::BlockExpression>(allocator, std::move(blockStmt));
    blockExpr->SetParent(assignmentExpr->Parent());

    Recheck(ctx->phaseManager, varBinder, checker, blockExpr);

    return blockExpr;
}

bool DestructuringPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    program->Ast()->TransformChildrenRecursivelyPreorder(
        [ctx](ir::AstNode *node) {
            if (node->IsAssignmentExpression()) {
                return ProcessAssignmentExpression(ctx, node->AsAssignmentExpression());
            }
            return node;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
