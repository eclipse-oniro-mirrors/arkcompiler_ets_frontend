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

static void ProcessVariableDeclarator(public_lib::Context *ctx, ir::VariableDeclarator *varDecl,
                                      ArenaVector<ir::VariableDeclarator *> *varDeclarators)
{
    auto *allocator = ctx->allocator;

    // Other cases must be catched in the checker
    auto *dstr = varDecl->Id();
    auto *init = varDecl->Init();
    ES2PANDA_ASSERT(dstr->IsETSDestructuring() && init != nullptr);

    auto checker = ctx->GetChecker()->AsETSChecker();

    auto *tmpVar = Gensym(allocator);
    auto *tmpTypeAnnotation = ctx->AllocNode<ir::OpaqueTypeNode>(init->TsType(), allocator);
    tmpVar->SetTypeAnnotation(tmpTypeAnnotation);
    tmpTypeAnnotation->SetParent(tmpVar);
    auto *tmpDeclarator = ctx->AllocNode<ir::VariableDeclarator>(varDecl->Flag(), tmpVar, init);

    tmpDeclarator->SetParent(varDecl->Parent());
    varDeclarators->emplace_back(tmpDeclarator);
    auto varBinder = checker->VarBinder()->AsETSBinder();
    Recheck(ctx->phaseManager, varBinder, checker, tmpDeclarator);

    uint32_t i = 0U;
    for (auto *elem : dstr->AsETSDestructuring()->Elements()) {
        if (elem->IsOmittedExpression()) {
            i++;
            continue;
        }

        auto *number = allocator->New<ir::NumberLiteral>(lexer::Number(i++));
        auto *clone = tmpVar->Clone(allocator, nullptr);
        clone->SetTypeAnnotation(nullptr);
        ir::Expression *newInit =
            ctx->AllocNode<ir::MemberExpression>(clone, number, ir::MemberExpressionKind::ELEMENT_ACCESS, true, false);
        auto *newDeclarator = ctx->AllocNode<ir::VariableDeclarator>(varDecl->Flag(), elem, newInit);

        newDeclarator->SetParent(varDecl->Parent());
        newDeclarator->Check(checker);
        varDeclarators->emplace_back(newDeclarator);
    }
}

static ir::AstNode *ProcessVariableDeclaration(public_lib::Context *ctx, ir::VariableDeclaration *varDecl)
{
    auto *allocator = ctx->allocator;

    ArenaVector<ir::VariableDeclarator *> declarators(allocator->Adapter());
    for (auto *it : varDecl->Declarators()) {
        if (!it->Id()->IsETSDestructuring()) {
            declarators.emplace_back(it);
            continue;
        }

        ProcessVariableDeclarator(ctx, it, &declarators);
    }

    varDecl->SetDeclarators(std::move(declarators));

    return varDecl;
}

bool DestructuringPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    program->Ast()->TransformChildrenRecursivelyPreorder(
        [ctx](ir::AstNode *node) {
            if (node->IsAssignmentExpression()) {
                return ProcessAssignmentExpression(ctx, node->AsAssignmentExpression());
            }
            if (node->IsVariableDeclaration()) {
                return ProcessVariableDeclaration(ctx, node->AsVariableDeclaration());
            }
            return node;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
