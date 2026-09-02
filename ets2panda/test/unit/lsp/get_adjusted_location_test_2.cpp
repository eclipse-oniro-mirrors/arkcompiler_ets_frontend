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

#include <string>
#include "lsp/include/get_adjusted_location.h"
#include "gtest/gtest.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/internal_api.h"

using ark::es2panda::ir::AstNode;

namespace {

class LspGetAdjustedLocation2 : public LSPAPITests {};

// Public entry guards: null input never adjusts. Also covers the null guards of
// the module-level helpers that share the same defensive shape.
TEST_F(LspGetAdjustedLocation2, NullNodeReturnsNoAdjustedLocation)
{
    EXPECT_FALSE(ark::es2panda::lsp::GetAdjustedLocation(nullptr).has_value());
    EXPECT_FALSE(ark::es2panda::lsp::GetAdjustedLocationForClass(nullptr).has_value());
    EXPECT_EQ(ark::es2panda::lsp::GetAdjustedLocationForFunction(nullptr), std::nullopt);
    EXPECT_FALSE(ark::es2panda::lsp::GetAdjustedLocationForHeritageClause(nullptr).has_value());
    EXPECT_EQ(ark::es2panda::lsp::FindFirstIdentifier(nullptr, false, {}), nullptr);
}

// A node without a parent (the Program root) is returned unchanged: there is no
// enclosing construct to adjust to.
TEST_F(LspGetAdjustedLocation2, ProgramRootReturnsItself)
{
    const char *source = R"(
let x = 1;
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("root.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(ast);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, ast);
    initializer.DestroyContext(ctx);
}

// Touching the argument-list parenthesis of a plain call yields the call token;
// GetTouchingPropertyName unwraps it to the callee identifier with an exact span.
TEST_F(LspGetAdjustedLocation2, CallParenTouchReturnsCalleeIdentifierWithExactSpan)
{
    const char *source =
        "function fn(): void {}\n"
        "let called = fn();\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("call.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    // Locate the call site after the definition ("fn()" also occurs in the
    // declaration header).
    size_t callSite = src.find("fn();", src.find("let called"));
    ASSERT_NE(callSite, std::string::npos);
    size_t lparen = callSite + strlen("fn");
    auto *token = ark::es2panda::lsp::GetTouchingToken(ctx, lparen, false);
    ASSERT_NE(token, nullptr);
    EXPECT_TRUE(token->IsCallExpression());
    auto *nameNode = ark::es2panda::lsp::GetTouchingPropertyName(ctx, lparen);
    ASSERT_NE(nameNode, nullptr);
    EXPECT_TRUE(nameNode->IsIdentifier());
    EXPECT_EQ(nameNode->AsIdentifier()->Name(), "fn");
    EXPECT_EQ(nameNode->Range().start.index, callSite);
    EXPECT_EQ(nameNode->Range().end.index, lparen);
    initializer.DestroyContext(ctx);
}

// Class keywords resolve to their declaration node so editors can adjust from
// the keyword to the named declaration. Function keywords behave differently:
// they surface the inner SCRIPT_FUNCTION token, which no adjustment branch
// maps to a FunctionDeclaration, so GetTouchingPropertyName yields nullptr.
TEST_F(LspGetAdjustedLocation2, ClassKeywordTouchesReturnDeclarationFunctionKeywordYieldsNull)
{
    const char *source =
        "function top(): void {}\n"
        "class Cls { }\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("kw.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *classDecl = ast->FindChild([](AstNode *n) { return n->IsClassDeclaration(); });
    ASSERT_NE(classDecl, nullptr);
    size_t clsKw = src.find("class");
    auto *fromCls = ark::es2panda::lsp::GetTouchingPropertyName(ctx, clsKw + 1);
    ASSERT_NE(fromCls, nullptr);
    EXPECT_TRUE(fromCls->IsClassDeclaration());
    EXPECT_EQ(fromCls, classDecl);
    size_t fnKw = src.find("function");
    auto *tokAtFnKw = ark::es2panda::lsp::GetTouchingToken(ctx, fnKw + 1, false);
    ASSERT_NE(tokAtFnKw, nullptr);
    EXPECT_TRUE(tokAtFnKw->IsScriptFunction());
    EXPECT_EQ(ark::es2panda::lsp::GetTouchingPropertyName(ctx, fnKw + 1), nullptr);
    initializer.DestroyContext(ctx);
}

// GetTouchingIdentifierName only accepts identifier-like tokens: member/property
// positions return the identifier itself, while call punctuation and declaration
// keywords produce nullptr (no identifier name is being typed there).
TEST_F(LspGetAdjustedLocation2, IdentifierNamePositionsAndMisses)
{
    const char *source =
        "function fn(): void {}\n"
        "class Cls { prop: number = 1; }\n"
        "let called = fn();\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("idname.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    size_t propPos = src.find("prop");
    auto *atProp = ark::es2panda::lsp::GetTouchingIdentifierName(ctx, propPos);
    ASSERT_NE(atProp, nullptr);
    EXPECT_TRUE(atProp->IsIdentifier());
    EXPECT_EQ(atProp->AsIdentifier()->Name(), "prop");
    size_t lparen = src.find("fn();", src.find("let called")) + strlen("fn");
    EXPECT_EQ(ark::es2panda::lsp::GetTouchingTokenForIdentifier(ctx, lparen, false), nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetTouchingIdentifierName(ctx, lparen), nullptr);
    size_t clsKw = src.find("class");
    EXPECT_EQ(ark::es2panda::lsp::GetTouchingIdentifierName(ctx, clsKw + 1), nullptr);
    initializer.DestroyContext(ctx);
}

// SkipOuterExpressions cannot be reached with an outer-expression operand
// through GetAdjustedLocation: the mirrored-simple-kind, binary+type-operator
// and for-in/of branches of TryConvenienceExpressions feed it operands whose
// kinds (await/new/typeof/yield, iterables) never satisfy IsOuterExpressionKind,
// and ETS produces no TS-type-operator nodes at all. The helper is exercised
// directly per the suite convention for module-public seams. `v!!` builds two
// nested TSNonNullExpression nodes, driving two unwrap iterations down to the
// operand identifier.
TEST_F(LspGetAdjustedLocation2, ChainedNonNullUnwrapsTwiceToOperand)
{
    const char *source = R"(
class Base {}
let v: Base = new Base();
let w = v!!;
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("nonnull.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *outer = nullptr;
    ast->IterateRecursively([&outer](AstNode *n) {
        if (outer == nullptr && n->IsTSNonNullExpression()) {
            outer = n;
        }
    });
    ASSERT_NE(outer, nullptr);
    int nonNullCount = 0;
    ast->IterateRecursively([&nonNullCount](AstNode *n) {
        if (n->IsTSNonNullExpression()) {
            nonNullCount++;
        }
    });
    // `v!!` wraps the identifier in exactly two chained TSNonNullExpression nodes.
    constexpr int expectedNonNullCount = 2;
    EXPECT_EQ(nonNullCount, expectedNonNullCount);
    auto *operand = ark::es2panda::lsp::SkipOuterExpressions(outer);
    ASSERT_NE(operand, nullptr);
    EXPECT_TRUE(operand->IsIdentifier());
    EXPECT_EQ(operand->AsIdentifier()->Name(), "v");
    // Through the public entry the non-null expression stays as-is: its parent
    // is a variable declarator, which none of the adjustment branches handle.
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(outer);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, outer);
    initializer.DestroyContext(ctx);
}

// Same seam as above exercised with an `as` chain (`x as A as B`): two
// TSAsExpression levels are unwrapped down to the operand identifier. The
// public entry returns the as-expression itself because its parent is a plain
// declarator (the TS-only specifier/export-all parents of TryTSAsExpression do
// not exist in ArkTS import/export syntax).
TEST_F(LspGetAdjustedLocation2, AsExpressionChainUnwrapsAndPublicEntryKeepsNode)
{
    const char *source = R"(
class A {}
let x: A = new A();
let chained = x as A as A;
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("aschain.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *asExpr = ast->FindChild([](AstNode *n) { return n->IsTSAsExpression(); });
    ASSERT_NE(asExpr, nullptr);
    auto *operand = ark::es2panda::lsp::SkipOuterExpressions(asExpr);
    ASSERT_NE(operand, nullptr);
    EXPECT_TRUE(operand->IsIdentifier());
    EXPECT_EQ(operand->AsIdentifier()->Name(), "x");
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(asExpr);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, asExpr);
    initializer.DestroyContext(ctx);
}

// FindFirstIdentifier returns the node itself when handed an identifier; for
// container nodes it descends to the first identifier descendant. Under ETS the
// modifier flags sit on the declaration nodes themselves, so skipping
// "modifier" identifiers cannot change the outcome for these shapes; both sides
// are asserted to pin the behaviour.
TEST_F(LspGetAdjustedLocation2, FindFirstIdentifierSelfContainerAndFlaggedInput)
{
    const char *source = R"(
class C {
    private flagged: number = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("ffid.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *propNode = ast->FindChild([](AstNode *n) { return n->IsClassProperty(); });
    ASSERT_NE(propNode, nullptr);
    EXPECT_TRUE(ark::es2panda::lsp::IsModifier(propNode));
    AstNode *directId =
        ast->FindChild([](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name() == "flagged"; });
    ASSERT_NE(directId, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::FindFirstIdentifier(directId, true, {}), directId);
    auto *foundInProp = ark::es2panda::lsp::FindFirstIdentifier(propNode, false, {});
    ASSERT_NE(foundInProp, nullptr);
    EXPECT_TRUE(foundInProp->IsIdentifier());
    EXPECT_EQ(foundInProp->AsIdentifier()->Name(), "flagged");
    initializer.DestroyContext(ctx);
}

// IsDeclarationOrModifier walks its whole disjunction chain for ordinary node
// pairs. Every pair below evaluates the final terms of the chain and returns
// false: under ArkTS modifier keywords are flags on the declaration nodes (never
// standalone child nodes) and nested namespaces lower to ETS_MODULE rather than
// TSModuleDeclaration, so no real ETS node pair satisfies the predicate.
TEST_F(LspGetAdjustedLocation2, DeclarationOrModifierChainEvaluatesFalseForRealPairs)
{
    const char *source = R"(
export let sized: number = 1;
class C {
    member: number = 0;
}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("declmod.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *varDecl = ast->FindChild([](AstNode *n) { return n->IsVariableDeclaration(); });
    AstNode *declarator = ast->FindChild([](AstNode *n) { return n->IsVariableDeclarator(); });
    AstNode *bindingId =
        ast->FindChild([](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name() == "sized"; });
    AstNode *memberId =
        ast->FindChild([](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name() == "member"; });
    ASSERT_NE(varDecl, nullptr);
    ASSERT_NE(declarator, nullptr);
    ASSERT_NE(bindingId, nullptr);
    ASSERT_NE(memberId, nullptr);
    // Plain expression/identifier pairs walk the full chain including the final
    // TSImportEqualsDeclaration term before returning false.
    EXPECT_FALSE(ark::es2panda::lsp::IsDeclarationOrModifier(bindingId, declarator));
    EXPECT_FALSE(ark::es2panda::lsp::IsDeclarationOrModifier(memberId, declarator));
    // Even a modifier-flagged declaration node fails because its parent (the
    // Program) cannot host modifiers.
    EXPECT_FALSE(ark::es2panda::lsp::IsDeclarationOrModifier(varDecl, ast));
    initializer.DestroyContext(ctx);
}

// GetAdjustedLocationForClass rejects inputs that are not class declarations or
// class expressions.
TEST_F(LspGetAdjustedLocation2, ClassAdjustRejectsNonClassInput)
{
    const char *source = R"(
let plain = 1;
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("nocls.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *bindingId =
        ast->FindChild([](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name() == "plain"; });
    ASSERT_NE(bindingId, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetAdjustedLocationForClass(bindingId), std::nullopt);
    initializer.DestroyContext(ctx);
}

// GetAdjustedLocationForFunction accepts identifiers directly; method
// definitions are rejected because their parent is the class definition, not a
// function declaration.
TEST_F(LspGetAdjustedLocation2, FunctionAdjustAcceptsIdentifierRejectsMethodDefinition)
{
    const char *source = R"(
class C {
    method(): void {}
}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("meth.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *methodName =
        ast->FindChild([](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name() == "method"; });
    ASSERT_NE(methodName, nullptr);
    auto adjustedId = ark::es2panda::lsp::GetAdjustedLocationForFunction(methodName);
    ASSERT_TRUE(adjustedId.has_value());
    EXPECT_EQ(*adjustedId, methodName);
    AstNode *methodDef = ast->FindChild([](AstNode *n) { return n->IsMethodDefinition(); });
    ASSERT_NE(methodDef, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetAdjustedLocationForFunction(methodDef), std::nullopt);
    initializer.DestroyContext(ctx);
}

// Direct switch coverage of GetAdjustedLocationForDeclaration, adjusting arm:
// function declarations and named-export declarations adjust to their declared
// identifier.
TEST_F(LspGetAdjustedLocation2, DeclarationAdjustsFunctionAndExportToIdentifier)
{
    const char *source = R"(
function topFn(p: number): number {
    return p;
}
class C {
    handler(): void {}
}
const alias = C;
export { C };
let rest: number = 2;
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("switch_fn_export.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *funcDecl = ast->FindChild([](AstNode *n) { return n->IsFunctionDeclaration(); });
    ASSERT_NE(funcDecl, nullptr);
    auto adjustedFn = ark::es2panda::lsp::GetAdjustedLocationForDeclaration(funcDecl, {});
    ASSERT_TRUE(adjustedFn.has_value());
    EXPECT_TRUE((*adjustedFn)->IsIdentifier());
    EXPECT_EQ((*adjustedFn)->AsIdentifier()->Name(), "topFn");

    AstNode *exportDecl = ast->FindChild([](AstNode *n) { return n->IsExportNamedDeclaration(); });
    ASSERT_NE(exportDecl, nullptr);
    auto adjustedExport = ark::es2panda::lsp::GetAdjustedLocationForDeclaration(exportDecl, {});
    ASSERT_TRUE(adjustedExport.has_value());
    EXPECT_TRUE((*adjustedExport)->IsIdentifier());
    EXPECT_EQ((*adjustedExport)->AsIdentifier()->Name(), "C");
    initializer.DestroyContext(ctx);
}

// Direct switch coverage of GetAdjustedLocationForDeclaration, fallthrough arm:
// function expressions (which only appear as method values in ArkTS) reject
// because their parent is the method definition, and other declaration kinds
// (plain variable declarations) fall through to no adjustment.
TEST_F(LspGetAdjustedLocation2, DeclarationKeepsMethodValueAndVariableUnadjusted)
{
    const char *source = R"(
function topFn(p: number): number {
    return p;
}
class C {
    handler(): void {}
}
const alias = C;
export { C };
let rest: number = 2;
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("switch_value_var.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *methodDef = ast->FindChild([](AstNode *n) {
        return n->IsMethodDefinition() &&
               n->AsMethodDefinition()->Kind() != ark::es2panda::ir::MethodDefinitionKind::CONSTRUCTOR;
    });
    ASSERT_NE(methodDef, nullptr);
    AstNode *methodValue = nullptr;
    methodDef->IterateRecursively([&methodValue](AstNode *n) {
        if (methodValue == nullptr && n->IsFunctionExpression()) {
            methodValue = n;
        }
    });
    ASSERT_NE(methodValue, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetAdjustedLocationForDeclaration(methodValue, {}), std::nullopt);

    AstNode *varDecl = ast->FindChild([](AstNode *n) {
        return n->IsVariableDeclaration() && n->FindChild([](AstNode *c) {
            return c->IsIdentifier() && c->AsIdentifier()->Name() == "rest";
        }) != nullptr;
    });
    ASSERT_NE(varDecl, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetAdjustedLocationForDeclaration(varDecl, {}), std::nullopt);
    initializer.DestroyContext(ctx);
}

// ArkTS lowers imports to ETS_IMPORT_DECLARATION nodes whose type differs from
// plain IMPORT_DECLARATION. The import guards of this module accept both node
// kinds, so a real parsed ArkTS import resolves its specifier through the same
// branch that serves JS/TS imports.
TEST_F(LspGetAdjustedLocation2, DISABLED_EtsImportDeclarationMatchesImportGuards)
{
    const char *source =
        "import { Named } from './mod';\n"
        "import * as nsAll from './mod2';\n"
        "let useA = Named;\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("imports.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    // The parser orders namespace imports before named ones; select the named
    // import through its specifier's parent.
    AstNode *namedSpec = ast->FindChild([](AstNode *n) { return n->IsImportSpecifier(); });
    ASSERT_NE(namedSpec, nullptr);
    AstNode *importDecl = namedSpec->Parent();
    ASSERT_NE(importDecl, nullptr);
    EXPECT_FALSE(importDecl->IsImportDeclaration());
    EXPECT_TRUE(importDecl->IsETSImportDeclaration());
    // The lowered declaration kind is accepted and resolves to the specifier
    // name identifier of the first (named) import.
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocationForImportDeclaration(importDecl, {});
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "Named");
    size_t namedMod = src.find("'./mod'");
    auto *namedStrTok = ark::es2panda::lsp::GetTouchingToken(ctx, namedMod + 1, false);
    ASSERT_NE(namedStrTok, nullptr);
    EXPECT_TRUE(namedStrTok->IsStringLiteral());
    auto adjustedNamed = ark::es2panda::lsp::GetAdjustedLocation(namedStrTok);
    ASSERT_TRUE(adjustedNamed.has_value());
    // Touching the source string of a named import adjusts to the specifier
    // name: the import branch precedes the module-string fallback.
    EXPECT_TRUE((*adjustedNamed)->IsIdentifier());
    EXPECT_EQ((*adjustedNamed)->AsIdentifier()->Name(), "Named");
    initializer.DestroyContext(ctx);
}

// Side-effect imports (`import './mod';`) do not build an import declaration in
// ArkTS: they lower to an expression statement holding the string literal. The
// public entry therefore returns the literal untouched, which is also why the
// empty-specifier branch of GetAdjustedLocationForImportDeclaration has no ETS
// input.
TEST_F(LspGetAdjustedLocation2, SideEffectImportLowersToStringExpressionStatement)
{
    const char *source = "import './sideeffect';\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("sideimport.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->FindChild([](AstNode *n) { return n->IsImportDeclaration(); }), nullptr);
    AstNode *strLit = ast->FindChild([](AstNode *n) { return n->IsStringLiteral(); });
    ASSERT_NE(strLit, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(strLit);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, strLit);
    initializer.DestroyContext(ctx);
}

// A type parameter node sits directly under its TSTypeParameterDeclaration; the
// adjustment resolves the pair to the parameter name identifier, which is the
// name an editor would rename at that position.
TEST_F(LspGetAdjustedLocation2, TypeParameterAdjustsToParameterName)
{
    const char *source = R"(
interface Iface {}
function pick<T extends Iface>(p: T): T {
    return p;
}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("typeparam.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *typeParam = ast->FindChild(
        [](AstNode *n) { return n->Parent() != nullptr && n->Parent()->IsTSTypeParameterDeclaration(); });
    ASSERT_NE(typeParam, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(typeParam);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "T");
    initializer.DestroyContext(ctx);
}

// Braceless for-of bodies nest the inner statement directly under the outer
// for-of, producing the mirrored FOR_OF pair that TryConvenienceExpressions
// resolves to the first expression of the outer loop.
TEST_F(LspGetAdjustedLocation2, NestedBracelessForOfAdjustsToOuterIterable)
{
    const char *source = R"(
function g(v: number): void {}
let a: number[] = [1];
for (let x of a) for (let y of a) g(y);
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("fornest.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *inner = ast->FindChild(
        [](AstNode *n) { return n->IsForOfStatement() && n->Parent() != nullptr && n->Parent()->IsForOfStatement(); });
    ASSERT_NE(inner, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(inner);
    ASSERT_TRUE(adjusted.has_value());
    // FindFirstExpression walks the outer loop in pre-order, so the first
    // expression-shaped descendant wins: the binding identifier of the outer
    // left-hand side (`x`), not the iterable.
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "x");
    initializer.DestroyContext(ctx);
}

// A variable declarator under its VariableDeclaration adjusts to the binding
// identifier through the variable branch of the module-or-variable step.
TEST_F(LspGetAdjustedLocation2, DeclaratorAdjustsToBindingIdentifier)
{
    const char *source = R"(
let binding = 7;
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("decl.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *declarator = ast->FindChild([](AstNode *n) { return n->IsVariableDeclarator(); });
    ASSERT_NE(declarator, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(declarator);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "binding");
    initializer.DestroyContext(ctx);
}

// Multi-byte identifiers keep exact byte spans: touching the middle of a
// code-point sequence still resolves the full identifier, and the reported
// range covers every UTF-8 byte of the name (byte offsets are the internal
// convention of the touching-token helpers).
TEST_F(LspGetAdjustedLocation2, UnicodeIdentifierMidCodePointTouchKeepsExactSpan)
{
    const char *source = "let 变量名 = 42;\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("uni.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    const size_t identStart = strlen("let ");
    const size_t identBytes = strlen("变量名");
    // Second byte of the leading code point of the identifier.
    auto *nameNode = ark::es2panda::lsp::GetTouchingPropertyName(ctx, identStart + 1);
    ASSERT_NE(nameNode, nullptr);
    EXPECT_TRUE(nameNode->IsIdentifier());
    EXPECT_EQ(std::string(nameNode->AsIdentifier()->Name().Utf8()), "变量名");
    EXPECT_EQ(nameNode->Range().start.index, identStart);
    EXPECT_EQ(nameNode->Range().end.index, identStart + identBytes);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(nameNode);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, nameNode);
    initializer.DestroyContext(ctx);
}

// Overload-style method groups (several signatures over one name shape) are
// plain method definitions for the adjustment layer: without a function-
// declaration parent each definition adjusts to itself, keeping the editor on
// the exact signature being edited.
TEST_F(LspGetAdjustedLocation2, OverloadStyleMethodsKeepTheirOwnNodes)
{
    const char *source = R"(
interface Shape {
    area(side: number): number;
    area(unit: string): number;
}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("overload.ets", ES2PANDA_STATE_PARSED, source);
    if (ContextState(ctx) == ES2PANDA_STATE_PARSED) {
        auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
        AstNode *ast = context->parserProgram->Ast();
        ASSERT_NE(ast, nullptr);
        AstNode *firstArea =
            ast->FindChild([](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name() == "area"; });
        ASSERT_NE(firstArea, nullptr);
        auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(firstArea);
        ASSERT_TRUE(adjusted.has_value());
        EXPECT_EQ(*adjusted, firstArea);
    } else {
        ADD_FAILURE() << "duplicate interface signatures expected to parse in PARSED state";
    }
    initializer.DestroyContext(ctx);
}

}  // namespace
