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

#include "extensionAccessorLowering.h"
#include "checker/ETSchecker.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

static ir::AstNode *CheckAndReturnNode(checker::ETSChecker *checker, ir::AstNode *node);

static bool ResolveAssignmentExpressionToExtensionAccessorCall(ir::AssignmentExpression *assignExpr,
                                                               ir::MemberExpression *expr)
{
    auto *curParent = assignExpr->Parent()->AsExpressionStatement();
    ir::AstNode *callExpr = nullptr;
    if (expr->Object()->IsETSNewClassInstanceExpression()) {
        callExpr = assignExpr->Left()->AsMemberExpression()->Property()->Parent();
    } else {
        callExpr = assignExpr->Left()->AsMemberExpression()->Parent();
    }
    curParent->SetExpression(callExpr->AsExpression());
    callExpr->SetParent(curParent);
    callExpr->SetRange(curParent->Range());
    return true;
}

static bool IsMemberExprExtensionAccessor(ir::AstNode *node)
{
    return node->IsMemberExpression() &&
           node->AsMemberExpression()->HasMemberKind(ir::MemberExpressionKind::EXTENSION_ACCESSOR);
}

static bool IsAssignExprExtensionSetter(ir::AstNode *node)
{
    return node->IsAssignmentExpression() &&
           node->AsAssignmentExpression()->OperatorType() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION &&
           IsMemberExprExtensionAccessor(node->AsAssignmentExpression()->Left());
}

// Note: In check phase, extension accessor 'a.m' get its type from returnType of Signature. In this lowering, `a.m` is
// lowered to 'a.m()', so its Type should be switched to the functionType of extension accessor, and in the reCheck,
// `a.m` will not be checked again
static void SwitchType(ir::MemberExpression *expr)
{
    expr->SetTsType(expr->ExtensionAccessorType());
}

static void TryHandleExtensionAccessor(checker::ETSChecker *checker, ir::MemberExpression *expr)
{
    checker::SavedCheckerContextStatus ccStatusHelper(&checker->Context(),
                                                      checker::CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK);
    SwitchType(expr);
    auto oldParent = expr->Parent();
    if (IsAssignExprExtensionSetter(oldParent) && expr == oldParent->AsAssignmentExpression()->Left()) {
        auto *assignExpr = oldParent->AsAssignmentExpression();
        auto *callExpr = checker->CreateExtensionAccessorCall(
            checker, expr, ArenaVector<ir::Expression *>(checker->ProgramAllocator()->Adapter()));

        auto *rightExpr = assignExpr->AsAssignmentExpression()->Right();
        if (IsMemberExprExtensionAccessor(rightExpr)) {
            SwitchType(rightExpr->AsMemberExpression());
            checker::Type *tsType = rightExpr->AsMemberExpression()->TsType();
            checker::ETSFunctionType *eAccType = rightExpr->AsMemberExpression()->ExtensionAccessorType();
            auto *copyedRight = rightExpr->Clone(checker->ProgramAllocator(), nullptr);
            copyedRight->AsMemberExpression()->SetTsType(tsType);
            copyedRight->AsMemberExpression()->SetExtensionAccessorType(eAccType);
            copyedRight->AsMemberExpression()->Object()->SetTsType(rightExpr->AsMemberExpression()->Object()->TsType());
            rightExpr = checker->CreateExtensionAccessorCall(
                checker, copyedRight->AsMemberExpression(),
                ArenaVector<ir::Expression *>(checker->ProgramAllocator()->Adapter()));
        }
        rightExpr->SetParent(callExpr);
        callExpr->AsCallExpression()->Arguments().emplace_back(rightExpr);
        if (!ResolveAssignmentExpressionToExtensionAccessorCall(assignExpr, expr)) {
            return;
        };
        CheckLoweredNode(checker->VarBinder()->AsETSBinder(), checker, callExpr);
        return;
    }

    auto *callExpr = checker->CreateExtensionAccessorCall(
        checker, expr, ArenaVector<ir::Expression *>(checker->ProgramAllocator()->Adapter()));
    callExpr->SetParent(oldParent);
    CheckLoweredNode(checker->VarBinder()->AsETSBinder(), checker, callExpr);
}

static ir::AstNode *CheckAndReturnNode(checker::ETSChecker *checker, ir::AstNode *node)
{
    if (!IsAssignExprExtensionSetter(node) && !IsMemberExprExtensionAccessor(node)) {
        return node;
    }

    if (IsAssignExprExtensionSetter(node)) {
        node->SetParent(nullptr);
        if (node->AsAssignmentExpression()->Left()->AsMemberExpression()->Object()->IsETSNewClassInstanceExpression()) {
            return node->AsAssignmentExpression()->Left()->AsMemberExpression()->Property()->Parent();
        }
        return node->AsAssignmentExpression()->Left()->Parent();
    }

    auto *oldParent = node->Parent();
    TryHandleExtensionAccessor(checker, node->AsMemberExpression());
    if (IsAssignExprExtensionSetter(oldParent) && (node == oldParent->AsAssignmentExpression()->Left())) {
        return node;
    }

    if (node->AsMemberExpression()->Object()->IsETSNewClassInstanceExpression()) {
        return node->AsMemberExpression()->Property()->Parent();
    }
    return node->Parent();
}

using AstNodePtr = ir::AstNode *;

bool ExtensionAccessorPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    if (program->Extension() != ScriptExtension::ETS) {
        return true;
    }

    checker::ETSChecker *const checker = ctx->GetChecker()->AsETSChecker();
    program->Ast()->TransformChildrenRecursively(
        [&checker](ir::AstNode *const node) -> AstNodePtr { return CheckAndReturnNode(checker, node); }, Name());
    return true;
}

}  // namespace ark::es2panda::compiler
