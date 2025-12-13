/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "promiseVoid.h"
#include "checker/ETSchecker.h"
#include "ir/expressions/literals/undefinedLiteral.h"

namespace ark::es2panda::compiler {
ir::BlockStatement *PromiseVoidInferencePhase::HandleAsyncScriptFunctionBody(public_lib::Context *ctx,
                                                                             ir::BlockStatement *body)
{
    (void)ctx;
    body->TransformChildrenRecursively(
        [ctx](checker::AstNodePtr ast) -> checker::AstNodePtr {
            if (ast->IsReturnStatement()) {
                auto *returnStmt = ast->AsReturnStatement();
                const auto *arg = returnStmt->Argument();
                if (arg == nullptr) {
                    auto *voidId = ctx->AllocNode<ir::UndefinedLiteral>();
                    const auto &returnLoc = returnStmt->Range();
                    voidId->SetRange(returnLoc);
                    returnStmt->SetArgument(voidId);
                }
            }
            return ast;
        },
        Name());

    return body;
}

void PromiseVoidInferencePhase::SetRangeRecursively(ir::TypeNode *node, const lexer::SourceRange &loc)
{
    node->SetRange(loc);
    node->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [loc](ir::AstNode *ast) -> ir::AstNode * {
            ast->SetRange(loc);
            return ast;
        },
        Name());
}

ir::TypeNode *PromiseVoidInferencePhase::CreatePromiseVoidType(public_lib::Context *ctx, const lexer::SourceRange &loc)
{
    auto *voidParam = [ctx]() {
        auto paramsVector = ArenaVector<ir::TypeNode *>(ctx->Allocator()->Adapter());
        paramsVector.push_back(ctx->AllocNode<ir::ETSPrimitiveType>(ir::PrimitiveType::VOID, ctx->Allocator()));
        auto *params = ctx->AllocNode<ir::TSTypeParameterInstantiation>(std::move(paramsVector));
        return params;
    }();

    auto *promiseVoidType = [ctx, voidParam]() {
        auto *promiseId = ctx->AllocNode<ir::Identifier>(compiler::Signatures::BUILTIN_PROMISE_CLASS, ctx->Allocator());
        auto *part = ctx->AllocNode<ir::ETSTypeReferencePart>(promiseId, voidParam, nullptr, ctx->Allocator());
        auto *type = ctx->AllocNode<ir::ETSTypeReference>(part, ctx->Allocator());
        return type;
    }();

    SetRangeRecursively(promiseVoidType, loc);

    return promiseVoidType;
}

static bool CheckForPromiseVoid(const ir::TypeNode *type)
{
    if (type == nullptr || !type->IsETSTypeReference()) {
        return false;
    }

    auto *typeRef = type->AsETSTypeReference();
    auto *typePart = typeRef->Part();
    if (typePart->Previous() != nullptr) {
        return false;
    }

    if (typePart->TypeParams() == nullptr) {
        return false;
    }
    const auto &params = typePart->TypeParams()->Params();
    if (params.size() != 1) {
        return false;
    }

    const auto &param = params.at(0);
    if (!param->IsETSUndefinedType()) {
        return false;
    }

    const auto isTypePromise = typePart->Name()->AsIdentifier()->Name() == compiler::Signatures::BUILTIN_PROMISE_CLASS;
    return isTypePromise;
}

using AstNodePtr = ir::AstNode *;

/*
 * Transformation is basically syntactical: it adds relevant return type and return statements to methods and function
 * NOTE: but not for lambdas, at least for now
 * So, the code
 * async function f() {}
 * transforms to
 * async function f(): Promise<void> { return Void; }
 * */

bool PromiseVoidInferencePhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    auto genTypeLocation = [](ir::ScriptFunction *function) -> lexer::SourceRange {
        const auto &params = function->Params();
        const auto &id = function->Id();
        const auto &body = function->Body();
        if (!params.empty()) {
            const auto &last = params.back();
            const auto &loc = last->Range();
            return {loc.end, loc.end};
        }

        if (id != nullptr) {
            const auto &loc = id->Range();
            return {loc.end, loc.end};
        }

        if (function->HasBody()) {
            const auto &loc = body->Range();
            return {loc.start, loc.start};
        }

        const auto &loc = function->Range();
        return {loc.end, loc.end};
    };

    const auto transformer = [this, ctx, genTypeLocation](ir::AstNode *ast) -> AstNodePtr {
        if (!(ast->IsScriptFunction() && ast->AsScriptFunction()->IsAsyncFunc())) {
            return ast;
        }

        auto *function = ast->AsScriptFunction();
        auto *returnAnn = function->ReturnTypeAnnotation();
        const auto hasReturnAnn = returnAnn != nullptr;
        const auto hasPromiseVoid = CheckForPromiseVoid(returnAnn);

        if (!hasReturnAnn) {
            if (!function->HasReturnStatement()) {
                const auto &loc = genTypeLocation(function);
                function->SetReturnTypeAnnotation(CreatePromiseVoidType(ctx, loc));
            }

            if (function->HasBody() && function->Body()->IsBlockStatement()) {
                HandleAsyncScriptFunctionBody(ctx, function->Body()->AsBlockStatement());
            }
        } else if (hasPromiseVoid && function->HasBody() && function->Body()->IsBlockStatement()) {
            HandleAsyncScriptFunctionBody(ctx, function->Body()->AsBlockStatement());
        }

        return ast;
    };

    program->Ast()->TransformChildrenRecursively(transformer, Name());

    return true;
}

bool PromiseVoidInferencePhase::PostconditionForModule(public_lib::Context *ctx, const parser::Program *program)
{
    (void)ctx;

    auto checkFunctionBody = [](const ir::BlockStatement *body) -> bool {
        if (!body->IsReturnStatement()) {
            return true;
        }
        auto *returnStmt = body->AsReturnStatement();
        const auto *arg = returnStmt->Argument();

        if (!arg->IsIdentifier()) {
            return false;
        }

        const auto *id = arg->AsIdentifier();
        return id->Name() == compiler::Signatures::UNDEFINED;
    };

    auto isOk = true;
    auto transformer = [checkFunctionBody, &isOk](ir::AstNode *ast) {
        if (!(ast->IsScriptFunction() && ast->AsScriptFunction()->IsAsyncFunc())) {
            return;
        }
        auto *function = ast->AsScriptFunction();
        auto *returnAnn = function->ReturnTypeAnnotation();
        if (!CheckForPromiseVoid(returnAnn)) {
            return;
        }
        if (function->HasBody() && function->Body()->IsBlockStatement()) {
            if (!checkFunctionBody(function->Body()->AsBlockStatement())) {
                isOk = false;
                return;
            }
        }
    };
    program->Ast()->IterateRecursively(transformer);

    return isOk;
}
}  // namespace ark::es2panda::compiler
