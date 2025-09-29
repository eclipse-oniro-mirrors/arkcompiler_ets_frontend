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

#include "defaultParametersLowering.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

static ir::Statement *TransformInitializer(ArenaAllocator *allocator, parser::ETSParser *parser,
                                           ir::ETSParameterExpression *param)
{
    //  NOTE (DZ): temporary solution until node history starts working properly
    auto *oldParam = param->Clone(allocator, param);
    param->SetOriginalNode(oldParam);

    auto const ident = param->Ident();
    auto const init = param->Initializer();
    auto const typeAnnotation = param->TypeAnnotation();

    param->SetIdent(Gensym(allocator));

    param->Ident()->SetTypeAnnotation(typeAnnotation);

    param->SetInitializer(nullptr);
    ES2PANDA_ASSERT(param->IsOptional());

    return parser->CreateFormattedStatement("let @@I1: @@T2 = (@@I3 !== undefined) ? @@I4 : @@E5", ident,
                                            typeAnnotation->Clone(allocator, nullptr), param->Ident()->Name(),
                                            param->Ident()->Name(), init);
}

static void TransformDefaultParameters(public_lib::Context *ctx, ir::ScriptFunction *function,
                                       const std::vector<ir::ETSParameterExpression *> &params)
{
    auto validateDefaultParamInDeclare = [ctx, function, &params]() {
        for (auto param : params) {
            ES2PANDA_ASSERT(param->Initializer() != nullptr);
            param->SetInitializer(nullptr);
            if ((function->Flags() & ir::ScriptFunctionFlags::EXTERNAL) != 0U) {
                ctx->GetChecker()->AsETSChecker()->LogError(diagnostic::DEFAULT_PARAM_IN_DECLARE, param->Start());
            }
        }
    };

    if (!function->HasBody()) {  // #23134
        validateDefaultParamInDeclare();
        return;
    }

    auto const body = function->Body()->AsBlockStatement();
    auto const allocator = ctx->allocator;
    auto const parser = ctx->parser->AsETSParser();
    auto &bodyStmt = body->StatementsForUpdates();

    bodyStmt.insert(bodyStmt.begin(), params.size(), nullptr);

    for (std::size_t dfltIdx = 0U; dfltIdx < params.size(); ++dfltIdx) {
        auto *const param = params[dfltIdx];
        auto stmt = TransformInitializer(allocator, parser, param);
        bodyStmt[dfltIdx] = stmt;
        // From a developer's perspective, this locational information is more intuitive.
        stmt->SetParent(param);
        RefineSourceRanges(stmt);
        stmt->SetParent(body);
    }
}

static void TransformFunction(public_lib::Context *ctx, ir::ScriptFunction *function)
{
    auto const &params = function->Params();
    std::vector<ir::ETSParameterExpression *> defaultParams;

    for (auto *param : params) {
        if (!param->IsETSParameterExpression()) {  // #23134
            ES2PANDA_ASSERT(ctx->diagnosticEngine->IsAnyError());
            continue;
        }
        if (param->AsETSParameterExpression()->Initializer() == nullptr) {
            continue;
        }
        if (param->AsETSParameterExpression()->TypeAnnotation() == nullptr) {  // #23134
            ES2PANDA_ASSERT(ctx->diagnosticEngine->IsAnyError());
            param->AsETSParameterExpression()->SetInitializer(nullptr);
            continue;
        }
        defaultParams.emplace_back(param->AsETSParameterExpression());
    }

    if (defaultParams.empty()) {
        return;
    }

    TransformDefaultParameters(ctx, function, defaultParams);
}

bool DefaultParametersLowering::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursivelyPreorder(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx = Context()](ir::AstNode *const node) -> ir::AstNode * {
            if (node->IsScriptFunction()) {
                TransformFunction(ctx, node->AsScriptFunction());
            }
            return node;
        },
        Name());

    return true;
}

bool DefaultParametersLowering::PostconditionForProgram(parser::Program const *program)
{
    return !program->Ast()->IsAnyChild([ctx = Context()](ir::AstNode const *node) {
        if (!node->IsScriptFunction()) {
            return false;
        }
        for (auto p : node->AsScriptFunction()->Params()) {
            if (!p->IsETSParameterExpression()) {  // #23134
                ES2PANDA_ASSERT(ctx->diagnosticEngine->IsAnyError());
                (void)ctx;
                continue;
            }
            if (p->AsETSParameterExpression()->Initializer() != nullptr) {
                return true;
            }
        }
        return false;
    });
}

}  // namespace ark::es2panda::compiler
