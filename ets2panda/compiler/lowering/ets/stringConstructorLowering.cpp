/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#include "ir/opaqueTypeNode.h"

namespace ark::es2panda::compiler {

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

static constexpr char const FORMAT_TO_STRING_PRIMITIVE_EXPRESSION[] = "@@E1.toString(@@E2)";
// NOLINTEND(modernize-avoid-c-arrays)

static bool IsBuiltinStringConstruction(public_lib::Context *const ctx,
                                        const ir::ETSNewClassInstanceExpression *newClassInstExpr)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *constructedType = newClassInstExpr->GetTypeRef()->TsType();
    return constructedType != nullptr &&
           checker->Relation()->IsIdenticalTo(constructedType, checker->GlobalBuiltinETSStringType());
}

static bool IsBuiltinStringCopyConstructor(public_lib::Context *const ctx, const checker::Signature *signature)
{
    ES2PANDA_ASSERT(signature != nullptr);
    auto *checker = ctx->GetChecker()->AsETSChecker();
    return signature->Params().size() == 1 &&
           checker->Relation()->IsIdenticalTo(signature->Params()[0]->TsType(), checker->GlobalBuiltinETSStringType());
}

static bool IsBuiltinStringCharArrayConstructor(const checker::Signature *signature, bool valueArray)
{
    ES2PANDA_ASSERT(signature != nullptr);
    if (signature->Params().size() != 1) {
        return false;
    }

    auto *paramType = signature->Params()[0]->TsType();
    if (!paramType->IsETSArrayType()) {
        return false;
    }
    if (!paramType->AsETSArrayType()->ElementType()->IsETSCharType()) {
        return false;
    }
    return paramType->AsETSArrayType()->IsValueArray() == valueArray;
}

static bool IsBuiltinStringCharValueArrayConstructor(const checker::Signature *signature)
{
    return IsBuiltinStringCharArrayConstructor(signature, true);
}

static bool IsBuiltinStringCharFixedArrayConstructor(const checker::Signature *signature)
{
    return IsBuiltinStringCharArrayConstructor(signature, false);
}

static ir::Expression *ReplaceConstructorNullish(public_lib::Context *const ctx,
                                                 ir::ETSNewClassInstanceExpression *newClassInstExpr)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *parser = ctx->parser->AsETSParser();

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
    if (argType->IsETSObjectType() && argType->AsETSObjectType()->IsBoxedPrimitive()) {
        blockExpr = parser->CreateFormattedExpression(FORMAT_TO_STRING_PRIMITIVE_EXPRESSION, argType->ToString(), arg);
    } else if (argType->PossiblyETSNull() && !argType->PossiblyETSUndefined()) {
        blockExpr = parser->CreateFormattedExpression(FORMAT_CHECK_NULL_EXPRESSION, tmpIdentName, arg, tmpIdentName,
                                                      tmpIdentName);
    } else if (argType->PossiblyETSUndefined() && !argType->PossiblyETSNull()) {
        blockExpr = parser->CreateFormattedExpression(FORMAT_CHECK_UNDEFINED_EXPRESSION, tmpIdentName, arg,
                                                      tmpIdentName, tmpIdentName);
    } else if (argType->PossiblyETSNullish()) {
        blockExpr = parser->CreateFormattedExpression(FORMAT_CHECK_NULLISH_EXPRESSION, tmpIdentName, arg, tmpIdentName,
                                                      tmpIdentName, tmpIdentName);
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

static ir::Expression *ReplaceConstructorFixedArray(public_lib::Context *const ctx,
                                                    ir::ETSNewClassInstanceExpression *newClassInstExpr)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *parser = ctx->parser->AsETSParser();

    auto *arg = newClassInstExpr->GetArguments()[0];
    auto *allocator = checker->Allocator();
    auto *argTypeNode = checker->AllocNode<ir::OpaqueTypeNode>(arg->TsType(), allocator);
    auto *convertedArg = parser->CreateFormattedExpression(
        std::string(ARKRUNTIME_IMPORT_ALIAS_PREFIX) + "stub.toValueArray(@@E1 as @@T2)", arg, argTypeNode);
    auto *newExpr = parser->CreateFormattedExpression("new String(@@E1)", convertedArg);
    newExpr->SetParent(newClassInstExpr->Parent());
    auto *scope = NearestScope(newClassInstExpr);
    auto exprCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(checker->VarBinder(), scope);
    InitScopesPhaseETS::RunExternalNode(newExpr, checker->VarBinder());
    checker->VarBinder()->AsETSBinder()->ResolveReferencesForScope(newExpr, NearestScope(newExpr));
    newExpr->Check(checker);
    return newExpr;
}

static ir::Expression *ReplaceStringConstructor(public_lib::Context *const ctx,
                                                ir::ETSNewClassInstanceExpression *newClassInstExpr)
{
    // Skip non-string constructors and invalid signatures.
    const auto *signature = newClassInstExpr->Signature();
    if (!IsBuiltinStringConstruction(ctx, newClassInstExpr) || signature == nullptr) {
        return newClassInstExpr;
    }

    if (IsBuiltinStringCopyConstructor(ctx, signature)) {
        auto *arg = newClassInstExpr->GetArguments()[0];
        arg->SetParent(newClassInstExpr->Parent());
        return arg;
    }

    if (IsBuiltinStringCharFixedArrayConstructor(signature)) {
        return ReplaceConstructorFixedArray(ctx, newClassInstExpr);
    }

    if (IsBuiltinStringCharValueArrayConstructor(signature)) {
        return newClassInstExpr;
    }

    if (signature->Params().size() == 1) {
        return ReplaceConstructorNullish(ctx, newClassInstExpr);
    }

    return newClassInstExpr;
}

bool StringConstructorLowering::PerformForProgram(parser::Program *const program)
{
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx = Context()](ir::AstNode *ast) -> ir::AstNode * {
            if (ast->IsETSNewClassInstanceExpression()) {
                return ReplaceStringConstructor(ctx, ast->AsETSNewClassInstanceExpression());
            }

            return ast;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
