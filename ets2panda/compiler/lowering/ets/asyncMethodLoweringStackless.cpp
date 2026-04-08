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

static void CheckNode(public_lib::Context *ctx, ir::AstNode *node)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(node);
    ES2PANDA_ASSERT(node->Parent());

    auto checker = ctx->GetChecker()->AsETSChecker();
    auto binder = checker->VarBinder()->AsETSBinder();
    auto scope = NearestScope(node->Parent());
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(binder, scope);
    CheckLoweredNode(binder, checker, node);
}

static ir::CallExpression *CreateAsyncContextCurrentCall(public_lib::Context *ctx)
{
    ES2PANDA_ASSERT(ctx);

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

    return asyncCtxCurrentCall;
}

static ArenaVector<ir::Statement *> CreatePrologue(public_lib::Context *ctx)
{
    ES2PANDA_ASSERT(ctx);

    const auto alloc = ctx->Allocator();

    const auto asyncCtxCurrentCall = CreateAsyncContextCurrentCall(ctx);

    const auto dispatchCall = util::NodeAllocator::ForceSetParent<ir::ETSIntrinsicNode>(
        alloc, "asyncdispatch", ArenaVector<ir::Expression *>({asyncCtxCurrentCall}));
    ES2PANDA_ASSERT(dispatchCall);

    const auto dispatchStmt = util::NodeAllocator::ForceSetParent<ir::ExpressionStatement>(alloc, dispatchCall);
    ES2PANDA_ASSERT(dispatchStmt);

    return {dispatchStmt};
}

static void AddPrologueToMethodBody(public_lib::Context *ctx, ir::BlockStatement *block)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(block);

    const auto statements = block->Statements();
    const auto prologue = CreatePrologue(ctx);
    const auto prologueSize = prologue.size();

    ArenaVector<ir::Statement *> newStatements = {};
    newStatements.reserve(statements.size() + prologueSize);
    newStatements.insert(newStatements.end(), prologue.begin(), prologue.end());
    newStatements.insert(newStatements.end(), statements.begin(), statements.end());

    block->SetStatements(std::move(newStatements));
}

static checker::Type *PromiseTypeArg(public_lib::Context *ctx, checker::Type *t)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(t);
    auto checker = ctx->GetChecker()->AsETSChecker();
    ES2PANDA_ASSERT(checker->IsPromiseType(t) || t->IsETSAsyncFuncReturnType());
    if (checker->IsPromiseType(t)) {
        return checker->UnwrapPromiseType(t);
    }
    ES2PANDA_ASSERT(t->IsETSAsyncFuncReturnType());
    return t->AsETSAsyncFuncReturnType()->GetPromiseTypeArg();
}

static checker::Type *PromiseType(public_lib::Context *ctx, checker::Type *t)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(t);
    auto checker = ctx->GetChecker()->AsETSChecker();
    ES2PANDA_ASSERT(checker->IsPromiseType(t) || t->IsETSAsyncFuncReturnType());
    if (checker->IsPromiseType(t)) {
        return t;
    }
    ES2PANDA_ASSERT(t->IsETSAsyncFuncReturnType());
    return t->AsETSAsyncFuncReturnType()->PromiseType();
}

static ir::Expression *CreateAsyncContextResolveValue(public_lib::Context *ctx, ir::ReturnStatement *stmt)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(stmt);

    auto checker = ctx->GetChecker()->AsETSChecker();
    const auto parser = ctx->parser->AsETSParser();
    const auto alloc = ctx->Allocator();

    auto arg = stmt->Argument();
    if (!arg) {
        return alloc->New<ir::UndefinedLiteral>();
    }
    ES2PANDA_ASSERT(arg);
    const auto argType = arg->TsType();
    ES2PANDA_ASSERT(argType);
    auto argClone = arg->Clone(alloc, nullptr)->AsExpression();
    ES2PANDA_ASSERT(argClone);

    const auto isArgVoid = checker->Relation()->IsIdenticalTo(argType, checker->GlobalVoidType());
    const auto isArgUndef = checker->Relation()->IsIdenticalTo(argType, checker->GlobalETSUndefinedType());
    if (isArgVoid || isArgUndef) {
        const auto argStmt = util::NodeAllocator::ForceSetParent<ir::ExpressionStatement>(alloc, argClone);
        ES2PANDA_ASSERT(argStmt);

        const auto undefinedLit = alloc->New<ir::UndefinedLiteral>();
        ES2PANDA_ASSERT(undefinedLit);
        const auto undefinedLitStmt = util::NodeAllocator::ForceSetParent<ir::ExpressionStatement>(alloc, undefinedLit);
        ES2PANDA_ASSERT(undefinedLitStmt);

        return util::NodeAllocator::ForceSetParent<ir::BlockExpression>(
            alloc, ArenaVector<ir::Statement *>({argStmt, undefinedLitStmt}));
    }

    /**
     * NOTE(knazarov): need to preserve the type deduction context,
     * since we are moving them out of the initial deduction context.
     */
    const auto retType = stmt->ReturnType();
    ES2PANDA_ASSERT(retType);
    const auto forcedType = checker->IsPromiseType(argType) ? PromiseType(ctx, argType) : PromiseTypeArg(ctx, retType);
    ES2PANDA_ASSERT(forcedType);

    return parser->CreateFormattedExpression("@@E1 as @@T2", argClone, forcedType);
}

static ir::ReturnStatement *CreateReturnFromAsync(public_lib::Context *ctx, ir::Expression *val,
                                                  checker::Type *explicitTypeAnno, bool isResolve)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(val);

    const auto parser = ctx->parser->AsETSParser();
    const auto alloc = ctx->Allocator();

    const auto resolveValIdent = Gensym(alloc);
    ES2PANDA_ASSERT(resolveValIdent);
    const auto resolveValDecl = parser->CreateFormattedStatement("let @@I1 = @@E2", resolveValIdent, val);
    ES2PANDA_ASSERT(resolveValDecl);

    const auto asyncCtxCurrent = CreateAsyncContextCurrentCall(ctx);
    ES2PANDA_ASSERT(asyncCtxCurrent);

    const auto asyncCtxIdent = Gensym(alloc);
    ES2PANDA_ASSERT(asyncCtxIdent);
    const auto asyncCtxDecl = parser->CreateFormattedStatement("let @@I1 = @@E2", asyncCtxIdent, asyncCtxCurrent);
    ES2PANDA_ASSERT(asyncCtxDecl);

    const auto resolveSig = isResolve ? "resolve" : "reject";

    ir::Statement *returnArgDecl = nullptr;
    std::stringstream formattedStmt;
    if (explicitTypeAnno) {
        formattedStmt << "@@I1 ? @@I2." << resolveSig << "<@@T3>(@@I4) : Promise." << resolveSig << "<@@T5>(@@I6)";
        returnArgDecl = parser->CreateFormattedStatement(formattedStmt.str(), asyncCtxIdent->Clone(alloc, nullptr),
                                                         asyncCtxIdent->Clone(alloc, nullptr), explicitTypeAnno,
                                                         resolveValIdent->Clone(alloc, nullptr), explicitTypeAnno,
                                                         resolveValIdent->Clone(alloc, nullptr));
    } else {
        formattedStmt << "@@I1 ? @@I2." << resolveSig << "(@@I3) : Promise." << resolveSig << "(@@I4)";
        returnArgDecl = parser->CreateFormattedStatement(
            formattedStmt.str(), asyncCtxIdent->Clone(alloc, nullptr), asyncCtxIdent->Clone(alloc, nullptr),
            resolveValIdent->Clone(alloc, nullptr), resolveValIdent->Clone(alloc, nullptr));
    }
    ES2PANDA_ASSERT(returnArgDecl);

    auto returnArg = util::NodeAllocator::ForceSetParent<ir::BlockExpression>(
        alloc, ArenaVector<ir::Statement *>({resolveValDecl, asyncCtxDecl, returnArgDecl}));
    ES2PANDA_ASSERT(returnArg);

    auto returnStmt = util::NodeAllocator::ForceSetParent<ir::ReturnStatement>(alloc, returnArg);
    ES2PANDA_ASSERT(returnStmt);

    return returnStmt;
}

static ir::ReturnStatement *HandleReturnStatement(public_lib::Context *ctx, [[maybe_unused]] ir::ScriptFunction *func,
                                                  ir::ReturnStatement *stmt)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(stmt);

    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(stmt);

    const auto val = CreateAsyncContextResolveValue(ctx, stmt);
    ES2PANDA_ASSERT(val);
    const auto returnStmt = CreateReturnFromAsync(ctx, val, nullptr, true);
    ES2PANDA_ASSERT(returnStmt);

    returnStmt->SetParent(stmt->Parent());

    return returnStmt;
}

static void AddMissingReturnStatement(public_lib::Context *ctx, ir::ScriptFunction *func)
{
    ES2PANDA_ASSERT(func);
    ES2PANDA_ASSERT(func->HasBody());
    ES2PANDA_ASSERT(func->Body()->IsBlockStatement());

    if (func->HasReturnStatement()) {
        return;
    }

    auto checker = ctx->GetChecker()->AsETSChecker();
    const auto alloc = ctx->Allocator();

    const auto funcReturnType = func->Signature()->ReturnType();
    const auto promiseVoidType = checker->CreatePromiseOf(checker->GlobalVoidType());
    const auto promiseUndefinedType = checker->CreatePromiseOf(checker->GlobalETSUndefinedType());
    const auto relation = checker->Relation();
    if (!relation->IsSupertypeOf(funcReturnType, promiseVoidType) &&
        !relation->IsSupertypeOf(funcReturnType, promiseUndefinedType)) {
        return;
    }

    const auto undefinedLit = alloc->New<ir::UndefinedLiteral>();
    ES2PANDA_ASSERT(undefinedLit);
    auto returnStmt = util::NodeAllocator::ForceSetParent<ir::ReturnStatement>(alloc, undefinedLit);
    ES2PANDA_ASSERT(returnStmt);
    auto body = func->Body()->AsBlockStatement();
    ES2PANDA_ASSERT(body);
    body->AddStatement(returnStmt);
    func->AddFlag(ir::ScriptFunctionFlags::HAS_RETURN);

    CheckNode(ctx, returnStmt);
}

static ArenaVector<ir::CatchClause *> CreateCatchClauses(public_lib::Context *ctx, ir::ScriptFunction *func)
{
    ES2PANDA_ASSERT(ctx);

    const auto alloc = ctx->Allocator();

    const auto errIdent = Gensym(alloc);
    ES2PANDA_ASSERT(errIdent);
    const auto promiseT = func->Signature()->ReturnType();
    ES2PANDA_ASSERT(promiseT);
    const auto returnStmt = CreateReturnFromAsync(ctx, errIdent, promiseT, false);
    ES2PANDA_ASSERT(returnStmt);
    const auto catchBody = util::NodeAllocator::ForceSetParent<ir::BlockStatement>(
        alloc, alloc, ArenaVector<ir::Statement *>({returnStmt}));
    ES2PANDA_ASSERT(catchBody);

    const auto catchClause =
        util::NodeAllocator::ForceSetParent<ir::CatchClause>(alloc, errIdent->Clone(alloc, nullptr), catchBody);
    ES2PANDA_ASSERT(catchClause);

    return ArenaVector<ir::CatchClause *>({catchClause});
}

static void WrapBodyInTryCatchBlock(public_lib::Context *ctx, ir::ScriptFunction *func, ir::BlockStatement *body)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(func);
    ES2PANDA_ASSERT(body);
    ES2PANDA_ASSERT(func == body->Parent());
    ES2PANDA_ASSERT(func->IsAsyncFunc());

    const auto alloc = ctx->Allocator();

    ClearTypesVariablesAndScopes(body);

    const auto emptyFinalizer =
        util::NodeAllocator::Alloc<ir::BlockStatement>(alloc, alloc, ArenaVector<ir::Statement *>({}));
    ES2PANDA_ASSERT(emptyFinalizer);
    const auto tryStatement = util::NodeAllocator::ForceSetParent<ir::TryStatement>(
        alloc, body, CreateCatchClauses(ctx, func), emptyFinalizer,
        ArenaVector<std::pair<compiler::LabelPair, const ir::Statement *>>({}));
    ES2PANDA_ASSERT(tryStatement);
    const auto newBody = util::NodeAllocator::ForceSetParent<ir::BlockStatement>(
        alloc, alloc, ArenaVector<ir::Statement *>({tryStatement}));
    ES2PANDA_ASSERT(newBody);

    newBody->SetScope(func->Scope());
    newBody->SetParent(func);
    func->SetBody(newBody);

    CheckNode(ctx, newBody);
}

static void TransformAsyncFunctionBody(public_lib::Context *ctx, ir::ScriptFunction *func, ir::BlockStatement *body)
{
    ES2PANDA_ASSERT(ctx);
    ES2PANDA_ASSERT(func);
    ES2PANDA_ASSERT(body);
    ES2PANDA_ASSERT(func == body->Parent());
    ES2PANDA_ASSERT(func->IsAsyncFunc());

    AddPrologueToMethodBody(ctx, body);

    AddMissingReturnStatement(ctx, func);

    // CC-OFFNXT(G.FMT.14-CPP) project code style
    const std::function<ir::AstNode *(ir::AstNode *)> transformer = [&](ir::AstNode *node) -> ir::AstNode * {
        ES2PANDA_ASSERT(node);

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
            const auto loweringResult = HandleReturnStatement(ctx, func, node->AsReturnStatement());
            ES2PANDA_ASSERT(loweringResult);
            return loweringResult;
        }

        return node;
    };

    body->TransformChildren(transformer, LOWERING_NAME);
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
        ES2PANDA_ASSERT(scriptFunction->Signature());
        ES2PANDA_ASSERT(scriptFunction->Signature()->ReturnType());
        ES2PANDA_ASSERT(ctx->GetChecker()->AsETSChecker()->IsPromiseType(scriptFunction->Signature()->ReturnType()));

        auto scriptFunctionBody = scriptFunction->Body();
        if (scriptFunctionBody == nullptr) {
            return;
        }
        ES2PANDA_ASSERT(scriptFunctionBody->IsBlockStatement());

        auto body = scriptFunctionBody->AsBlockStatement();

        TransformAsyncFunctionBody(ctx, scriptFunction, body);

        WrapBodyInTryCatchBlock(ctx, scriptFunction, body);

        RefineSourceRanges(body);
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

bool AsyncMethodLoweringStackless::PreconditionForProgram(const parser::Program *program)
{
    return !program->Ast()->IsAnyChild([](const ir::AstNode *node) -> bool {
        /*
         * NOTE(knazarov): No asyncImpl functions should be generated by the lowerings
         */
        const auto isAsyncImplFunc = node->IsScriptFunction() && node->AsScriptFunction()->IsAsyncImplFunc();
        if (isAsyncImplFunc) {
            return true;
        }

        return false;
    });
}

}  // namespace ark::es2panda::compiler
