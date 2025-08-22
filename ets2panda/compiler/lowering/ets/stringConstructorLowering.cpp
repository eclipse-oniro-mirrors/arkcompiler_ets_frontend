/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "stringConstructorLowering.h"

#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "parser/ETSparser.h"
#include "varbinder/ETSBinder.h"
#include "varbinder/scope.h"

namespace ark::es2panda::compiler {

std::string_view StringConstructorLowering::Name() const
{
    return "StringConstructorLowering";
}

// NOLINTBEGIN(modernize-avoid-c-arrays)
static constexpr char const FORMAT_CHECK_NULL_EXPRESSION[] =
    "let @@I1 = (@@E2);"
    "(@@I3 === null ? \"null\" : (@@I4 as Object).toString())";

static constexpr char const FORMAT_CHECK_UNDEFINED_EXPRESSION[] =
    "let @@I1 = (@@E2);"
    "(@@I3 === undefined ? \"undefined\" : (@@I4 as Object).toString())";

static constexpr char const FORMAT_CHECK_NULLISH_EXPRESSION[] =
    "let @@I1 = (@@E2);"
    "(@@I3 instanceof null ? \"null\" : (@@I4 instanceof undefined ? \"undefined\" : (@@I5 as Object).toString()))";

static constexpr char const FORMAT_TO_STRING_EXPRESSION[] = "((@@E1 as Object).toString())";
// NOLINTEND(modernize-avoid-c-arrays)

ir::Expression *ReplaceStringConstructor(public_lib::Context *const ctx,
                                         ir::ETSNewClassInstanceExpression *newClassInstExpr)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *parser = ctx->parser->AsETSParser();

    // Skip missing signatures
    if (newClassInstExpr->GetSignature() == nullptr || newClassInstExpr->GetSignature()->InternalName() == nullptr) {
        return newClassInstExpr;
    }

    // Case for the constructor: new String(str: string)
    if (newClassInstExpr->GetSignature()->InternalName() == Signatures::BUILTIN_STRING_FROM_STRING_CTOR) {
        auto *arg = newClassInstExpr->GetArguments()[0];
        arg->SetParent(newClassInstExpr->Parent());
        return arg;
    }

    // Case for the constructor: new String(str: Object)
    if (newClassInstExpr->GetSignature()->InternalName() == Signatures::BUILTIN_STRING_FROM_NULLISH_CTOR) {
        auto *arg = newClassInstExpr->GetArguments()[0];
        auto *argType = arg->TsType();

        // For the case when the constructor parameter is "null" or "undefined"
        if (argType->IsETSNullType() || argType->IsETSUndefinedType()) {
            auto *literal = argType->IsETSNullType() ? ctx->AllocNode<ir::StringLiteral>("null")
                                                     : ctx->AllocNode<ir::StringLiteral>("undefined");
            ES2PANDA_ASSERT(literal != nullptr);
            literal->SetParent(newClassInstExpr->Parent());

            // Run checker
            literal->Check(checker);
            return literal;
        }

        // Enter to the old scope
        auto *scope = NearestScope(newClassInstExpr);
        auto exprCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(checker->VarBinder(), scope);

        // Generate temporary variable
        auto const tmpIdentName = GenName(ctx->Allocator());

        // Create BlockExpression
        ir::Expression *blockExpr = nullptr;
        if (argType->PossiblyETSNull() && !argType->PossiblyETSUndefined()) {
            blockExpr = parser->CreateFormattedExpression(FORMAT_CHECK_NULL_EXPRESSION, tmpIdentName, arg, tmpIdentName,
                                                          tmpIdentName);
        } else if (argType->PossiblyETSUndefined() && !argType->PossiblyETSNull()) {
            blockExpr = parser->CreateFormattedExpression(FORMAT_CHECK_UNDEFINED_EXPRESSION, tmpIdentName, arg,
                                                          tmpIdentName, tmpIdentName);
        } else if (argType->PossiblyETSNullish()) {
            blockExpr = parser->CreateFormattedExpression(FORMAT_CHECK_NULLISH_EXPRESSION, tmpIdentName, arg,
                                                          tmpIdentName, tmpIdentName, tmpIdentName);
        } else {
            blockExpr = parser->CreateFormattedExpression(FORMAT_TO_STRING_EXPRESSION, arg);
        }

        blockExpr->SetParent(newClassInstExpr->Parent());

        // Run VarBinder for new BlockExpression
        InitScopesPhaseETS::RunExternalNode(blockExpr, checker->VarBinder());
        checker->VarBinder()->AsETSBinder()->ResolveReferencesForScope(blockExpr, NearestScope(blockExpr));

        // Run checker
        blockExpr->Check(checker);
        return blockExpr;
    }

    return newClassInstExpr;
}

bool StringConstructorLowering::PerformForModule(public_lib::Context *const ctx, parser::Program *const program)
{
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx](ir::AstNode *ast) -> ir::AstNode * {
            if (ast->IsETSNewClassInstanceExpression()) {
                return ReplaceStringConstructor(ctx, ast->AsETSNewClassInstanceExpression());
            }

            return ast;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
