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

#include <cstring>
#include <string>
#include "lsp/include/get_adjusted_location.h"
#include "gtest/gtest.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/internal_api.h"

using ark::es2panda::ir::AstNode;

namespace {

class LspGetAdjustedLocationImport : public LSPAPITests {};

// Named import: the declaration adjusts to the specifier name identifier and
// keeps its exact source span; the public entry resolves the specifier child
// through the same import branch.
TEST_F(LspGetAdjustedLocationImport, DISABLED_NamedImportSpecifierAdjustsToNameWithIdentifierSpan)
{
    const char *source = "import { Named } from './mod';\nlet use = Named;\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("named_import.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *importDecl = ast->FindChild([](AstNode *n) { return n->IsETSImportDeclaration(); });
    ASSERT_NE(importDecl, nullptr);
    EXPECT_TRUE(importDecl->IsETSImportDeclaration());
    // The declaration span starts at the `import` keyword and ends after the
    // trailing semicolon consumed by the parser.
    const size_t importStart = src.find("import");
    const size_t semicolonPos = src.find(';', importStart);
    ASSERT_NE(importStart, std::string::npos);
    ASSERT_NE(semicolonPos, std::string::npos);
    EXPECT_EQ(importDecl->Range().start.index, importStart);
    EXPECT_EQ(importDecl->Range().end.index, semicolonPos + 1);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocationForImportDeclaration(importDecl, {});
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "Named");
    const size_t nameStart = src.find("Named");
    ASSERT_NE(nameStart, std::string::npos);
    EXPECT_EQ((*adjusted)->Range().start.index, nameStart);
    EXPECT_EQ((*adjusted)->Range().end.index, nameStart + strlen("Named"));
    // The specifier child of the declaration adjusts to the same identifier
    // through the public entry.
    AstNode *specifier = ast->FindChild([](AstNode *n) { return n->IsImportSpecifier(); });
    ASSERT_NE(specifier, nullptr);
    auto adjustedSpec = ark::es2panda::lsp::GetAdjustedLocation(specifier);
    ASSERT_TRUE(adjustedSpec.has_value());
    EXPECT_EQ(*adjustedSpec, *adjusted);
    initializer.DestroyContext(ctx);
}

// Aliased named import: adjustment lands on the local alias (the binding an
// editor renames), not on the imported original name, with the alias span.
TEST_F(LspGetAdjustedLocationImport, DISABLED_AliasedImportAdjustsToLocalAliasWithSpan)
{
    const char *source = "import { Orig as Alias } from './aliasmod';\nlet u = Alias;\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("alias_import.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *importDecl = ast->FindChild([](AstNode *n) { return n->IsETSImportDeclaration(); });
    ASSERT_NE(importDecl, nullptr);
    AstNode *specifier = ast->FindChild([](AstNode *n) { return n->IsImportSpecifier(); });
    ASSERT_NE(specifier, nullptr);
    auto adjustedSpec = ark::es2panda::lsp::GetAdjustedLocation(specifier);
    ASSERT_TRUE(adjustedSpec.has_value());
    EXPECT_TRUE((*adjustedSpec)->IsIdentifier());
    EXPECT_EQ((*adjustedSpec)->AsIdentifier()->Name(), "Alias");
    const size_t aliasStart = src.find("Alias");
    ASSERT_NE(aliasStart, std::string::npos);
    EXPECT_EQ((*adjustedSpec)->Range().start.index, aliasStart);
    EXPECT_EQ((*adjustedSpec)->Range().end.index, aliasStart + strlen("Alias"));
    auto adjustedDecl = ark::es2panda::lsp::GetAdjustedLocationForImportDeclaration(importDecl, {});
    ASSERT_TRUE(adjustedDecl.has_value());
    EXPECT_EQ(*adjustedDecl, *adjustedSpec);
    initializer.DestroyContext(ctx);
}

// Namespace import (`import * as ns`): the namespace specifier carries only the
// local alias, so both the helper and the public entry resolve to it.
TEST_F(LspGetAdjustedLocationImport, DISABLED_NamespaceImportAdjustsToAliasIdentifier)
{
    const char *source = "import * as nsAll from './nsmod';\nlet x = nsAll;\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("ns_import.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *specifier = ast->FindChild([](AstNode *n) { return n->IsImportNamespaceSpecifier(); });
    ASSERT_NE(specifier, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(specifier);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "nsAll");
    const size_t nsStart = src.find("nsAll");
    ASSERT_NE(nsStart, std::string::npos);
    EXPECT_EQ((*adjusted)->Range().start.index, nsStart);
    EXPECT_EQ((*adjusted)->Range().end.index, nsStart + strlen("nsAll"));
    initializer.DestroyContext(ctx);
}

// Default-only import: no ImportSpecifier/ImportNamespaceSpecifier child exists,
// so the specifier branch yields nothing and the module-string fallback (now
// accepting the lowered ETS declaration kind) returns the source literal itself
// with its exact quoted span.
TEST_F(LspGetAdjustedLocationImport, DefaultImportSourceStringFallbackKeepsLiteralSpan)
{
    const char *source = "import Def from './defmod';\nlet d = Def;\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("default_import.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *importDecl = ast->FindChild([](AstNode *n) { return n->IsETSImportDeclaration(); });
    ASSERT_NE(importDecl, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetAdjustedLocationForImportDeclaration(importDecl, {}), std::nullopt);
    AstNode *strLit = ast->FindChild([](AstNode *n) { return n->IsStringLiteral(); });
    ASSERT_NE(strLit, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(strLit);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, strLit);
    const size_t strStart = src.find("'./defmod'");
    ASSERT_NE(strStart, std::string::npos);
    EXPECT_EQ(strLit->Range().start.index, strStart);
    EXPECT_EQ(strLit->Range().end.index, strStart + strlen("'./defmod'"));
    EXPECT_EQ((*adjusted)->Range().start.index, strLit->Range().start.index);
    EXPECT_EQ((*adjusted)->Range().end.index, strLit->Range().end.index);
    initializer.DestroyContext(ctx);
}

}  // namespace
