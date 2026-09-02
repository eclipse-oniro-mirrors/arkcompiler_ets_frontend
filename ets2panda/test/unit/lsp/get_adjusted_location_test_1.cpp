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

#include "lsp/include/get_adjusted_location.h"
#include "gtest/gtest.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "test/utils/ast_verifier_test.h"
#include "lsp/include/internal_api.h"

using ark::es2panda::ir::AstNode;

namespace {

class LspGetAdjustedLocation1 : public LSPAPITests {};

// `IsOuterExpression` is a public predicate of the module, but no production
// code path calls it in this tree: `SkipOuterExpressions` uses the static
// `IsOuterExpressionKind` helper directly, and no LSP definition-query API
// routes through it. The predicate is exercised directly on a real
// TS_AS_EXPRESSION node obtained from source.
TEST_F(LspGetAdjustedLocation1, IsOuterExpressionTest)
{
    const char *source = R"(
class MyType {
}
let v: MyType;
let a = v as MyType;
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *asExpr = ast->FindChild([](AstNode *n) { return n->IsTSAsExpression(); });
    ASSERT_NE(asExpr, nullptr);
    EXPECT_TRUE(ark::es2panda::lsp::IsOuterExpression(asExpr));
    EXPECT_FALSE(ark::es2panda::lsp::IsOuterExpression(nullptr));
    initializer.DestroyContext(ctx);
}

// `FindTypeReference` is only referenced by the TS-only `TryTypeOperatorFamily`
// branch. The ETS parser produces ETSTypeReference (never TSTypeReference), so
// the function's IsTSTypeReference checks always miss for ETS sources. It is
// exercised on a real ETSTypeReference node to document this behaviour.
TEST_F(LspGetAdjustedLocation1, FindTypeReferenceTest)
{
    const char *source = R"(
class MyType {
}
let v: MyType;
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *etsTypeRef = ast->FindChild([](AstNode *n) { return n->IsETSTypeReference(); });
    ASSERT_NE(etsTypeRef, nullptr);
    auto children = ark::es2panda::lsp::GetChildren(etsTypeRef);
    auto *found = ark::es2panda::lsp::FindTypeReference(etsTypeRef, children);
    // ETS type references are ETSTypeReference, so the TSTypeReference lookup
    // intentionally returns nullptr for ETS sources.
    EXPECT_EQ(found, nullptr);
    initializer.DestroyContext(ctx);
}

// `GetAdjustedLocationForDeclaration` is only reachable through
// `TryDeclaration` (requires `IsDeclarationOrModifier` to be true, which no ETS
// node pair satisfies) or the TS-only import-type branch. It is exercised
// directly on a real class declaration from source.
TEST_F(LspGetAdjustedLocation1, GetAdjustedLocationForDeclarationTest)
{
    const char *source = R"(
class MyType {
}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *classDecl = ast->FindChild([](AstNode *n) { return n->IsClassDeclaration(); });
    ASSERT_NE(classDecl, nullptr);
    auto children = ark::es2panda::lsp::GetChildren(classDecl);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocationForDeclaration(classDecl, children);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "MyType");
    initializer.DestroyContext(ctx);
}

// The ETS parser never builds ExportAllDeclaration (`export *` / `export {}`
// from clauses are lowered to ETSReExportDeclaration), so the export-all branch
// of GetAdjustedLocation is unreachable through definition-query APIs. Passing
// a non-export node exercises the guard and returns nullopt, documenting the
// ETS behaviour.
TEST_F(LspGetAdjustedLocation1, GetAdjustedLocationForExportDeclarationTest)
{
    const char *source = R"(
class MyType {
}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *classDecl = ast->FindChild([](AstNode *n) { return n->IsClassDeclaration(); });
    ASSERT_NE(classDecl, nullptr);
    auto children = ark::es2panda::lsp::GetChildren(classDecl);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocationForExportDeclaration(classDecl, children);
    EXPECT_FALSE(adjusted.has_value());
    initializer.DestroyContext(ctx);
}

// `GetAdjustedLocationForHeritageClause` is only called from
// `TryTSHeritageAndInfer` when node and parent are both TSClassImplements
// (which never happens: a TSClassImplements parent is always the
// ClassDefinition). The function is exercised directly on a real
// TSInterfaceHeritage node from `interface I2 extends Iface`.
TEST_F(LspGetAdjustedLocation1, GetAdjustedLocationForHeritageClauseTest)
{
    const char *source = R"(
interface Iface {
    x: number;
}
interface I2 extends Iface {
}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *heritage = ast->FindChild([](AstNode *n) { return n->IsTSInterfaceHeritage(); });
    ASSERT_NE(heritage, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocationForHeritageClause(heritage);
    ASSERT_TRUE(adjusted.has_value());
    // The heritage clause finds its first expression child (the ETSTypeReference
    // wrapping `Iface`) before reaching the inner identifier.
    EXPECT_TRUE((*adjusted)->IsExpression() || (*adjusted)->IsIdentifier());
    initializer.DestroyContext(ctx);
}

}  // namespace
