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

#include "asyncMethodLoweringStackless.h"

#include "checker/ETSchecker.h"
#include "checker/types/ets/etsAsyncFuncReturnType.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

static constexpr std::string_view LOWERING_NAME = "async-method-lowering-stackless";

std::string_view AsyncMethodLoweringStackless::Name() const
{
    return "AsyncMethodLoweringStackless";
}

static ArenaVector<ir::Statement *> CreatePrologue(public_lib::Context *ctx, ir::Identifier *currentAsyncCtxIdent)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(currentAsyncCtxIdent);

    const auto alloc = ctx->Allocator();

    const auto asyncCtxImportAlias = util::UString {
        std::string(ARKRUNTIME_IMPORT_ALIAS_PREFIX) + std::string(Signatures::BUILTIN_ASYNCCONTEXT_CLASS), alloc};
    const auto asyncCtxIdent = alloc->New<ir::Identifier>(asyncCtxImportAlias.View(), alloc);
    ES2PANDA_ASSERT(asyncCtxIdent);

    const auto asyncCtxCurrentIdent = alloc->New<ir::Identifier>("current", alloc);
    ES2PANDA_ASSERT(asyncCtxIdent);

    const auto asyncCtxCurrentMemberExpression = util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
        alloc, asyncCtxIdent, asyncCtxCurrentIdent, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    ES2PANDA_ASSERT(asyncCtxCurrentMemberExpression);

    const auto asyncCtxCurrentCall = util::NodeAllocator::ForceSetParent<ir::CallExpression>(
        alloc, asyncCtxCurrentMemberExpression, ArenaVector<ir::Expression *>({}), nullptr, false, false);
    ES2PANDA_ASSERT(asyncCtxCurrentCall);

    const auto asyncCtxVariableDeclarator = util::NodeAllocator::ForceSetParent<ir::VariableDeclarator>(
        alloc, ir::VariableDeclaratorFlag::CONST, currentAsyncCtxIdent->Clone(alloc, nullptr), asyncCtxCurrentCall);
    ES2PANDA_ASSERT(asyncCtxVariableDeclarator);

    const auto asyncCtxVariableDeclaration = util::NodeAllocator::ForceSetParent<ir::VariableDeclaration>(
        alloc, ir::VariableDeclaration::VariableDeclarationKind::CONST, alloc,
        ArenaVector<ir::VariableDeclarator *>({asyncCtxVariableDeclarator}));
    ES2PANDA_ASSERT(asyncCtxVariableDeclaration);

    const auto dispatchCall = util::NodeAllocator::ForceSetParent<ir::ETSIntrinsicNode>(
        alloc, "asyncdispatch", ArenaVector<ir::Expression *>({currentAsyncCtxIdent->Clone(alloc, nullptr)}));
    ES2PANDA_ASSERT(dispatchCall);

    const auto dispatchStmt = util::NodeAllocator::ForceSetParent<ir::ExpressionStatement>(alloc, dispatchCall);
    ES2PANDA_ASSERT(dispatchStmt);

    return {asyncCtxVariableDeclaration, dispatchStmt};
}

static void AddPrologueToMethodBody(public_lib::Context *ctx, ir::BlockStatement *block, ir::Identifier *asyncCtxIdent)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(block);

    const auto statements = block->Statements();
    const auto prologue = CreatePrologue(ctx, asyncCtxIdent);
    const auto prologueSize = prologue.size();

    ArenaVector<ir::Statement *> newStatements = {};
    newStatements.reserve(statements.size() + prologueSize);
    newStatements.insert(newStatements.end(), prologue.begin(), prologue.end());
    newStatements.insert(newStatements.end(), statements.begin(), statements.end());

    block->SetStatements(std::move(newStatements));

    for (size_t i = 0; i < prologueSize; ++i) {
        auto checker = ctx->GetChecker()->AsETSChecker();
        auto binder = checker->VarBinder()->AsETSBinder();
        auto scope = NearestScope(block);
        auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(binder, scope);
        auto stmts = block->Statements();
        CheckLoweredNode(binder, checker, stmts[i]);
    }
}

static ArenaVector<ir::Statement *> CreateEpilogue(public_lib::Context *ctx)
{
    ES2PANDA_ASSERT(ctx);

    const auto alloc = ctx->Allocator();
    const auto undefinedLit = alloc->New<ir::UndefinedLiteral>();
    ES2PANDA_ASSERT(undefinedLit);

    auto returnStmt = util::NodeAllocator::ForceSetParent<ir::ReturnStatement>(alloc, undefinedLit);
    ES2PANDA_ASSERT(returnStmt);

    return {returnStmt};
}

static void AddEpilogueToMethodBody(public_lib::Context *ctx, ir::BlockStatement *block)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(block);

    const auto statements = block->Statements();
    const auto statementsSize = statements.size();
    const auto epilogue = CreateEpilogue(ctx);
    const auto epilogueSize = epilogue.size();

    block->AddStatements(std::move(epilogue));

    for (size_t i = statementsSize; i < statementsSize + epilogueSize; ++i) {
        auto checker = ctx->GetChecker()->AsETSChecker();
        auto binder = checker->VarBinder()->AsETSBinder();
        auto scope = NearestScope(block);
        auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(binder, scope);
        auto stmts = block->Statements();
        CheckLoweredNode(binder, checker, stmts[i]);
    }
}

static checker::Type *PromiseTypeArg(checker::ETSChecker *checker, checker::Type *t)
{
    ES2PANDA_ASSERT(t);
    ES2PANDA_ASSERT(checker->IsPromiseType(t) || t->IsETSAsyncFuncReturnType());
    if (checker->IsPromiseType(t)) {
        return checker->UnwrapPromiseType(t);
    }
    ES2PANDA_ASSERT(t->IsETSAsyncFuncReturnType());
    return t->AsETSAsyncFuncReturnType()->GetPromiseTypeArg();
}

static void HandleLoweringResult(ir::AstNode *loweringResult, ir::AstNode *orig, checker::ETSChecker *checker)
{
    ES2PANDA_ASSERT(loweringResult);
    ES2PANDA_ASSERT(orig);
    ES2PANDA_ASSERT(checker);
    ES2PANDA_ASSERT(orig->Parent());

    loweringResult->SetRange(orig->Range());
    loweringResult->SetParent(orig->Parent());

    auto binder = checker->VarBinder()->AsETSBinder();
    auto scope = NearestScope(orig->Parent());
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(binder, scope);
    /*
     * NOTE(knazarov): workaround for complex expressions in child expressions.
     * Without this call CheckLoweredNode fails to resolve variables correctly.
     * E.g. StreamReadableTestPart1.ets
     */
    ClearTypesVariablesAndScopes(loweringResult);
    CheckLoweredNode(binder, checker, loweringResult);
}

static ir::Expression *PrepareAsyncContextResolveValue(public_lib::Context *ctx, ir::ReturnStatement *stmt)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(stmt);

    auto checker = ctx->GetChecker()->AsETSChecker();
    const auto parser = ctx->parser->AsETSParser();
    const auto alloc = ctx->Allocator();

    const auto arg = stmt->Argument();
    const auto isArgVoid = arg && checker->Relation()->IsIdenticalTo(arg->TsType(), checker->GlobalVoidType());
    const auto isArgUndef = arg && checker->Relation()->IsIdenticalTo(arg->TsType(), checker->GlobalETSUndefinedType());
    if (!arg || isArgVoid || isArgUndef) {
        ArenaVector<ir::Statement *> stmts = {};

        if (arg) {
            const auto argClone = arg->Clone(alloc, nullptr)->AsExpression();
            ES2PANDA_ASSERT(argClone);
            const auto argStmt = util::NodeAllocator::ForceSetParent<ir::ExpressionStatement>(alloc, argClone);
            ES2PANDA_ASSERT(argStmt);
            stmts.push_back(argStmt);
        }

        const auto undefinedLit = alloc->New<ir::UndefinedLiteral>();
        ES2PANDA_ASSERT(undefinedLit);
        const auto undefinedLitStmt = util::NodeAllocator::ForceSetParent<ir::ExpressionStatement>(alloc, undefinedLit);
        ES2PANDA_ASSERT(undefinedLitStmt);
        stmts.push_back(undefinedLitStmt);

        return util::NodeAllocator::ForceSetParent<ir::BlockExpression>(alloc, std::move(stmts));
    }

    ES2PANDA_ASSERT(arg);
    const auto argClone = arg->Clone(alloc, nullptr)->AsExpression();
    ES2PANDA_ASSERT(argClone);

    if (checker->IsPromiseType(arg->TsType())) {
        return argClone;
    }

    /**
     * For non-promise values, need to preserve the type deduction context,
     * since we are moving them out of the initial deduction context.
     */
    checker::Type *promiseTypeArg = PromiseTypeArg(checker, stmt->ReturnType());
    ES2PANDA_ASSERT(promiseTypeArg);

    return parser->CreateFormattedExpression("@@E1 as @@T2", argClone, promiseTypeArg);
}

static ir::ReturnStatement *HandleReturnStatement(public_lib::Context *ctx, ir::ReturnStatement *stmt,
                                                  ir::Identifier *asyncCtxIdent)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(stmt);
    ES2PANDA_ASSERT(asyncCtxIdent);

    auto checker = ctx->GetChecker()->AsETSChecker();
    const auto parser = ctx->parser->AsETSParser();
    const auto alloc = ctx->Allocator();

    const auto asyncContextResolveValue = PrepareAsyncContextResolveValue(ctx, stmt);
    ES2PANDA_ASSERT(asyncContextResolveValue);

    const auto resolveValSym = Gensym(alloc);
    ES2PANDA_ASSERT(resolveValSym);
    const auto resolveValDecl = parser->CreateFormattedStatement(
        "let @@I1 = @@E2", resolveValSym->Clone(alloc, nullptr), asyncContextResolveValue);
    ES2PANDA_ASSERT(resolveValDecl);

    const auto returnArgDecl =
        parser->CreateFormattedStatement("@@I1 ? @@I2.resolve(@@I3) : Promise.resolve(@@I4)",
                                         asyncCtxIdent->Clone(alloc, nullptr), asyncCtxIdent->Clone(alloc, nullptr),
                                         resolveValSym->Clone(alloc, nullptr), resolveValSym->Clone(alloc, nullptr));
    ES2PANDA_ASSERT(returnArgDecl);

    auto returnArg = util::NodeAllocator::ForceSetParent<ir::BlockExpression>(
        alloc, ArenaVector<ir::Statement *>({resolveValDecl, returnArgDecl}));
    ES2PANDA_ASSERT(returnArg);

    auto loweringResult = util::NodeAllocator::ForceSetParent<ir::ReturnStatement>(alloc, returnArg);

    HandleLoweringResult(loweringResult, stmt, checker);

    return loweringResult->AsReturnStatement();
}

static void HandleAsyncFunctionBody(public_lib::Context *ctx, ir::BlockStatement *body)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(body);
    ES2PANDA_ASSERT(body->Parent());
    ES2PANDA_ASSERT(body->Parent()->IsScriptFunction());
    ES2PANDA_ASSERT(body->Parent()->AsScriptFunction()->IsAsyncFunc());

    const auto asyncCtxIdent = Gensym(ctx->Allocator());
    ES2PANDA_ASSERT(asyncCtxIdent);

    /*
     * NOTE(knazarov): prologue does not need to be transformed, since it  does not contain
     * neither `await`, nor `return`.
     */
    AddPrologueToMethodBody(ctx, body, asyncCtxIdent);

    /*
     * NOTE(knazarov): assume that in case that no return statement is present, the only possible
     * return statement is `return undefined`, since otherwise checker would have already reported an error
     */
    const auto containingFunc = body->Parent()->AsScriptFunction();
    if (!containingFunc->HasReturnStatement()) {
        AddEpilogueToMethodBody(ctx, body);
    }

    // CC-OFFNXT(G.FMT.14-CPP) project code style
    const std::function<ir::AstNode *(ir::AstNode *)> transformer = [&](ir::AstNode *node) -> ir::AstNode * {
        /*
         * NOTE(knazarov): since we iterate nodes in postorder, deepest
         * ScriptFunction will already be transformed. Thus, break early.
         */
        if (node->IsScriptFunction()) {
            return node;
        }

        /*
         * NOTE(knazarov): first, transform children, since `return` and
         * `await` can be nested.
         */
        node->TransformChildren(transformer, LOWERING_NAME);

        if (node->IsReturnStatement()) {
            return HandleReturnStatement(ctx, node->AsReturnStatement(), asyncCtxIdent);
        }

        return node;
    };

    body->TransformChildren(transformer, LOWERING_NAME);

    RefineSourceRanges(body);
}

static void IteratorCallback(public_lib::Context *ctx, ir::AstNode *node)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(node);

    if (node->IsScriptFunction()) {
        auto scriptFunction = node->AsScriptFunction();
        if (!scriptFunction->IsAsyncFunc()) {
            return;
        }
        auto scriptFunctionBody = scriptFunction->Body();
        if (scriptFunctionBody == nullptr) {
            return;
        }
        ES2PANDA_ASSERT(scriptFunctionBody->IsBlockStatement());

        HandleAsyncFunctionBody(ctx, scriptFunctionBody->AsBlockStatement());

        return;
    }
}

bool AsyncMethodLoweringStackless::PerformForProgram(parser::Program *program)
{
    if (!Context()->config->options->IsStacklessCoros()) {
        return true;
    }

    /*
     * NOTE(knazarov): iterate postorder, since we want to transform
     * deepest ScriptFunctions first.
     */
    program->Ast()->IterateRecursivelyPostorder([ctx = Context()](ir::AstNode *node) { IteratorCallback(ctx, node); });

    return true;
}

bool AsyncMethodLoweringStackless::PostconditionForProgram(const parser::Program *program)
{
    return !program->Ast()->IsAnyChild([](const ir::AstNode *node) -> bool {
        return node->IsScriptFunction() && node->AsScriptFunction()->IsAsyncImplFunc();
    });
}

}  // namespace ark::es2panda::compiler
