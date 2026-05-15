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

#include "constructorInitLowering.h"

#include <algorithm>

#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"
#include "ir/base/scriptFunction.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/superExpression.h"
#include "ir/statements/expressionStatement.h"

namespace ark::es2panda::compiler {

static checker::Signature *ResolveImplicitSuperCtorSig(checker::ETSObjectType *superType)
{
    const auto &superTypeCtorSigs = superType->ConstructSignatures();
    auto anyZeroMinArg = std::find_if(superTypeCtorSigs.begin(), superTypeCtorSigs.end(),
                                      [](const checker::Signature *sig) { return sig->MinArgCount() == 0; });
    if (anyZeroMinArg == superTypeCtorSigs.end()) {
        return nullptr;
    }

    auto exactZeroArg = std::find_if(superTypeCtorSigs.begin(), superTypeCtorSigs.end(),
                                     [](const checker::Signature *sig) { return sig->ArgCount() == 0; });
    return exactZeroArg != superTypeCtorSigs.end() ? *exactZeroArg : *anyZeroMinArg;
}

static bool ShouldSkipCtor(const ir::ScriptFunction *func)
{
    if (func->Body() == nullptr || !func->Body()->IsBlockStatement()) {
        return true;
    }
    return !func->IsImplicitSuperCallNeeded();
}

static void RecheckLoweredCtorCall(public_lib::Context *ctx, ir::ScriptFunction *func, ir::Statement *stmt)
{
    auto *varBinder = ctx->GetChecker()->VarBinder()->AsETSBinder();
    auto *checker = ctx->GetChecker()->AsETSChecker();
    BindLoweredNode(varBinder, stmt);

    auto *ownerType = func->Signature()->Owner();
    ES2PANDA_ASSERT(ownerType != nullptr && ownerType->IsETSObjectType());

    checker::CheckerStatus newStatus = checker::CheckerStatus::IN_CLASS | checker::CheckerStatus::IGNORE_VISIBILITY;
    if (ownerType->GetDeclNode() != nullptr && ownerType->GetDeclNode()->IsClassDefinition() &&
        ownerType->GetDeclNode()->AsClassDefinition()->IsAbstract()) {
        newStatus = checker::CheckerStatus::IN_ABSTRACT;
    }
    if ((checker->Context().Status() & checker::CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK) != 0) {
        newStatus |= checker::CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK;
    }

    auto *scope = NearestScope(stmt);
    auto checkerCtx = checker::SavedCheckerContext(checker, newStatus, ownerType->AsETSObjectType(), func->Signature());
    auto scopeCtx = checker::ScopeContext(checker, scope);
    stmt->Check(checker);
}

static void InsertImplicitSuperCall(public_lib::Context *ctx, ir::ScriptFunction *func,
                                    checker::Signature *superCtorSig)
{
    auto *body = func->Body()->AsBlockStatement();

    ArenaVector<ir::Expression *> callArgs(ctx->Allocator()->Adapter());
    auto *superExpr = ctx->AllocNode<ir::SuperExpression>();
    auto *callExpr = ctx->AllocNode<ir::CallExpression>(superExpr, std::move(callArgs), nullptr, false);
    auto *stmt = ctx->AllocNode<ir::ExpressionStatement>(callExpr);

    superExpr->SetParent(callExpr);
    callExpr->SetSignature(superCtorSig);
    callExpr->SetParent(stmt);
    stmt->SetParent(body);
    body->StatementsForUpdates().insert(body->StatementsForUpdates().begin(), stmt);

    func->AddFlag(ir::ScriptFunctionFlags::EXPLICIT_SUPER_CALL);
    func->ClearFlag(ir::ScriptFunctionFlags::IMPLICIT_SUPER_CALL_NEEDED);
    RecheckLoweredCtorCall(ctx, func, stmt);
}

static void ProcessConstructor(public_lib::Context *ctx, ir::ScriptFunction *func)
{
    if (!func->IsConstructor()) {
        return;
    }
    if (ShouldSkipCtor(func)) {
        return;
    }

    auto *signature = func->Signature();
    ES2PANDA_ASSERT(signature != nullptr);

    auto *ownerType = signature->Owner();
    ES2PANDA_ASSERT(ownerType != nullptr && ownerType->IsETSObjectType());
    auto *ownerClassType = ownerType->AsETSObjectType();

    auto *superType = ownerClassType->SuperType();
    if (superType == nullptr) {
        return;
    }

    auto *superCtorSig = ResolveImplicitSuperCtorSig(superType);
    if (superCtorSig == nullptr) {
        return;
    }

    InsertImplicitSuperCall(ctx, func, superCtorSig);
}

bool ConstructorInitLowering::PerformForProgram(parser::Program *program)
{
    program->Ast()->IterateRecursivelyPreorder([ctx = Context()](ir::AstNode *node) {
        if (node->IsScriptFunction()) {
            ProcessConstructor(ctx, node->AsScriptFunction());
        }
    });
    return true;
}

}  // namespace ark::es2panda::compiler
