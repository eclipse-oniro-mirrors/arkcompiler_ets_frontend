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
#include "gtest/gtest.h"
#include "ir/astNodeFlags.h"
#include "lsp/include/get_adjusted_location.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

using ark::es2panda::ir::AstNode;
using ark::es2panda::ir::ModifierFlags;

namespace {
template <typename Pred>
AstNode *FindNode(AstNode *ast, Pred pred)
{
    AstNode *found = nullptr;
    ast->FindChild([&found, &pred](AstNode *node) {
        if (pred(node)) {
            found = node;
            return true;
        }
        return false;
    });
    return found;
}

class LspGetAdjustedLocationEts : public LSPAPITests {};

// PJ-001 (get_adjusted_location.cpp:217-231, 68-87): single access modifiers.
// The class property carries the exact PRIVATE / PROTECTED flag word, so
// IsModifier accepts it, but IsDeclarationOrModifier stays false because the
// parent ClassDeclaration carries the fixed combined word STATIC|PUBLIC whose
// overlap with the member flag is 0. GetAdjustedLocation therefore falls
// through every Try* step and returns the property itself with its exact span.
TEST_F(LspGetAdjustedLocationEts, SingleModifierPropertyPinsToSelfSpan)
{
    const char *source = "\nclass A {\n    private x: number = 1;\n    protected p: number = 2;\n}\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj001_single.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *classDecl = FindNode(ast, [](AstNode *n) { return n->IsClassDeclaration(); });
    ASSERT_NE(classDecl, nullptr);

    AstNode *privateProperty = FindNode(ast, [](AstNode *n) {
        return n->IsClassProperty() && n->AsClassProperty()->Key()->IsIdentifier() &&
               n->AsClassProperty()->Key()->AsIdentifier()->Name().Utf8() == "x";
    });
    ASSERT_NE(privateProperty, nullptr);
    AstNode *protectedProperty = FindNode(ast, [](AstNode *n) {
        return n->IsClassProperty() && n->AsClassProperty()->Key()->IsIdentifier() &&
               n->AsClassProperty()->Key()->AsIdentifier()->Name().Utf8() == "p";
    });
    ASSERT_NE(protectedProperty, nullptr);

    EXPECT_EQ(privateProperty->Modifiers(), ModifierFlags::PRIVATE);
    EXPECT_TRUE(ark::es2panda::lsp::IsModifier(privateProperty));
    EXPECT_EQ(protectedProperty->Modifiers(), ModifierFlags::PROTECTED);
    EXPECT_TRUE(ark::es2panda::lsp::IsModifier(protectedProperty));
    EXPECT_FALSE(ark::es2panda::lsp::IsDeclarationOrModifier(privateProperty, classDecl));
    EXPECT_FALSE(ark::es2panda::lsp::IsDeclarationOrModifier(protectedProperty, classDecl));

    auto adjustedPrivate = ark::es2panda::lsp::GetAdjustedLocation(privateProperty);
    ASSERT_TRUE(adjustedPrivate.has_value());
    EXPECT_EQ(*adjustedPrivate, privateProperty);
    auto adjustedProtected = ark::es2panda::lsp::GetAdjustedLocation(protectedProperty);
    ASSERT_TRUE(adjustedProtected.has_value());
    EXPECT_EQ(*adjustedProtected, protectedProperty);

    const size_t memberStart = src.find("private x");
    ASSERT_NE(memberStart, std::string::npos);
    const size_t keyStart = src.find("x:", memberStart);
    ASSERT_NE(keyStart, std::string::npos);
    EXPECT_EQ(privateProperty->Range().start.index, keyStart);
    EXPECT_EQ(privateProperty->Range().end.index, src.find(';', memberStart));
    initializer.DestroyContext(ctx);
}

// PJ-001 (get_adjusted_location.cpp:68-87): combined modifier words fall to the
// switch default. A `public static` method carries the combined word
// STATIC|PUBLIC and an `override` method carries PUBLIC|OVERRIDE; neither word
// equals a single case label, so both report IsModifier == false and the
// declarations route through the plain fallback (self).
TEST_F(LspGetAdjustedLocationEts, CombinedModifierWordsFallToDefaultArm)
{
    const char *source =
        "\nclass Base {\n    public static m(): void {}\n}\n"
        "class Derived extends Base {\n    override m(): void {}\n}\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj001_combined.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *staticMethod = FindNode(ast, [](AstNode *n) {
        return n->IsMethodDefinition() && n->AsMethodDefinition()->Key()->IsIdentifier() &&
               n->AsMethodDefinition()->Key()->AsIdentifier()->Name().Utf8() == "m" &&
               static_cast<unsigned>(n->Modifiers() & ModifierFlags::STATIC) != 0U;
    });
    ASSERT_NE(staticMethod, nullptr);
    EXPECT_EQ(staticMethod->Modifiers(), ModifierFlags::STATIC | ModifierFlags::PUBLIC);
    EXPECT_FALSE(ark::es2panda::lsp::IsModifier(staticMethod));
    EXPECT_EQ(static_cast<unsigned>(staticMethod->Modifiers() & ModifierFlags::OVERRIDE), 0U);

    AstNode *overrideMethod = FindNode(ast, [](AstNode *n) {
        return n->IsMethodDefinition() && n->AsMethodDefinition()->Key()->IsIdentifier() &&
               n->AsMethodDefinition()->Key()->AsIdentifier()->Name().Utf8() == "m" &&
               static_cast<unsigned>(n->Modifiers() & ModifierFlags::OVERRIDE) != 0U;
    });
    ASSERT_NE(overrideMethod, nullptr);
    EXPECT_EQ(overrideMethod->Modifiers(), ModifierFlags::PUBLIC | ModifierFlags::OVERRIDE);
    EXPECT_FALSE(ark::es2panda::lsp::IsModifier(overrideMethod));

    AstNode *classDecl = FindNode(ast, [](AstNode *n) { return n->IsClassDeclaration(); });
    ASSERT_NE(classDecl, nullptr);
    EXPECT_FALSE(ark::es2panda::lsp::IsDeclarationOrModifier(staticMethod, classDecl));
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(staticMethod);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, staticMethod);
    initializer.DestroyContext(ctx);
}

// PJ-001 (get_adjusted_location.cpp:69, 214 of astNodeFlags.h): the abstract
// word is carried by the ClassDefinition, not the ClassDeclaration; the
// definition is accepted by IsModifier, while the parent declaration still has
// no overlapping flags, so the declaration-or-modifier disjunct stays false.
TEST_F(LspGetAdjustedLocationEts, AbstractFlagCarriedByClassDefinition)
{
    const char *source = "\nabstract class Shape {\n    abstract area(): number;\n}\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj001_abstract.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *classDecl = FindNode(ast, [](AstNode *n) { return n->IsClassDeclaration(); });
    ASSERT_NE(classDecl, nullptr);
    AstNode *classDefinition = FindNode(ast, [](AstNode *n) { return n->IsClassDefinition(); });
    ASSERT_NE(classDefinition, nullptr);
    EXPECT_EQ(classDefinition->Modifiers(), ModifierFlags::ABSTRACT);
    EXPECT_TRUE(ark::es2panda::lsp::IsModifier(classDefinition));
    EXPECT_FALSE(ark::es2panda::lsp::IsDeclarationOrModifier(classDefinition, classDecl));
    initializer.DestroyContext(ctx);
}

// PJ-001 (get_adjusted_location.cpp:68-87): the FINAL word is absent from the
// IsModifier switch (astNodeFlags.h:55), so a `final class` definition reports
// IsModifier == false even though the flag is stored on the definition.
TEST_F(LspGetAdjustedLocationEts, FinalClassFlagNotRecognizedAsModifier)
{
    const char *source = "\nfinal class Sealed {}\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj001_final.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *classDefinition = FindNode(ast, [](AstNode *n) { return n->IsClassDefinition(); });
    ASSERT_NE(classDefinition, nullptr);
    EXPECT_EQ(classDefinition->Modifiers(), ModifierFlags::FINAL);
    EXPECT_FALSE(ark::es2panda::lsp::IsModifier(classDefinition));
    AstNode *classDecl = FindNode(ast, [](AstNode *n) { return n->IsClassDeclaration(); });
    ASSERT_NE(classDecl, nullptr);
    EXPECT_FALSE(ark::es2panda::lsp::IsDeclarationOrModifier(classDefinition, classDecl));
    initializer.DestroyContext(ctx);
}

// PJ-001/PJ-002 (get_adjusted_location.cpp:224, 402-411): the implements
// clause of a real ETS class satisfies the TSClassImplements disjunct when the
// predicate is given the ClassDeclaration parent. In the parsed ETS tree the
// clause's actual parent is the ClassDefinition and the definition's range
// starts at the body brace, so the public entry returns the clause itself.
TEST_F(LspGetAdjustedLocationEts, ImplementsClausePredicateAndFlowPins)
{
    const char *source = "\ninterface I {}\nclass B implements I {}\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj002_impl.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *implClause = FindNode(ast, [](AstNode *n) { return n->IsTSClassImplements(); });
    ASSERT_NE(implClause, nullptr);
    AstNode *classDecl = FindNode(ast, [](AstNode *n) { return n->IsClassDeclaration(); });
    ASSERT_NE(classDecl, nullptr);
    ASSERT_NE(implClause->Parent(), nullptr);
    ASSERT_TRUE(implClause->Parent()->IsClassDefinition());
    EXPECT_TRUE(ark::es2panda::lsp::IsDeclarationOrModifier(implClause, classDecl));
    EXPECT_FALSE(ark::es2panda::lsp::IsDeclarationOrModifier(implClause, implClause->Parent()));

    const size_t nameStart = src.rfind("I");
    ASSERT_NE(nameStart, std::string::npos);
    EXPECT_EQ(implClause->Range().start.index, nameStart);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(implClause);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, implClause);

    AstNode *typeRef = FindNode(ast, [](AstNode *n) {
        return n->IsETSTypeReference() && n->Parent() != nullptr && n->Parent()->IsTSClassImplements();
    });
    ASSERT_NE(typeRef, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetAdjustedLocationForHeritageClause(implClause), std::nullopt);
    auto adjustedRef = ark::es2panda::lsp::GetAdjustedLocation(typeRef);
    ASSERT_TRUE(adjustedRef.has_value());
    EXPECT_EQ(*adjustedRef, typeRef);
    initializer.DestroyContext(ctx);
}

// PJ-002 (get_adjusted_location.cpp:342-353): interface heritage (extends)
// resolves to the inherited ETSTypeReference; the helper rejects null and
// non-heritage nodes exactly.
TEST_F(LspGetAdjustedLocationEts, InterfaceHeritageResolvesTypeReference)
{
    const char *source = "\ninterface I {}\ninterface J extends I {}\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj002_ext.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *heritage = FindNode(ast, [](AstNode *n) { return n->IsTSInterfaceHeritage(); });
    ASSERT_NE(heritage, nullptr);
    AstNode *typeRef = FindNode(ast, [](AstNode *n) {
        return n->IsETSTypeReference() && n->Parent() != nullptr && n->Parent()->IsTSInterfaceHeritage();
    });
    ASSERT_NE(typeRef, nullptr);

    auto adjusted = ark::es2panda::lsp::GetAdjustedLocationForHeritageClause(heritage);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, typeRef);
    const size_t inheritedStart = src.rfind("I");
    ASSERT_NE(inheritedStart, std::string::npos);
    EXPECT_EQ(typeRef->Range().start.index, inheritedStart);
    EXPECT_EQ(typeRef->Range().end.index, inheritedStart + 1);

    EXPECT_EQ(ark::es2panda::lsp::GetAdjustedLocationForHeritageClause(nullptr), std::nullopt);
    AstNode *interfaceDecl = FindNode(ast, [](AstNode *n) { return n->IsTSInterfaceDeclaration(); });
    ASSERT_NE(interfaceDecl, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetAdjustedLocationForHeritageClause(interfaceDecl), std::nullopt);
    initializer.DestroyContext(ctx);
}

// PJ-001 (get_adjusted_location.cpp:229): a dotted nested namespace in ETS is
// represented by ETSModule nodes, not TSModuleDeclaration, so the module
// disjunct of IsDeclarationOrModifier stays false for the real node pair and
// the public entry returns the inner module itself.
TEST_F(LspGetAdjustedLocationEts, NestedNamespaceUsesEtsModuleKind)
{
    const char *source = "\nnamespace Outer.Inner {\n    const q = 1;\n}\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj001_ns.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    EXPECT_EQ(FindNode(ast, [](AstNode *n) { return n->IsTSModuleDeclaration(); }), nullptr);
    AstNode *outer = nullptr;
    AstNode *inner = nullptr;
    ast->IterateRecursively([&outer, &inner](const AstNode *n) {
        if (n->IsETSModule()) {
            if (outer == nullptr) {
                outer = const_cast<AstNode *>(n);
            } else if (inner == nullptr) {
                inner = const_cast<AstNode *>(n);
            }
        }
    });
    ASSERT_NE(outer, nullptr);
    ASSERT_NE(inner, nullptr);
    ASSERT_TRUE(outer->Range().start.index < inner->Range().start.index);
    ASSERT_TRUE(outer->Range().end.index >= inner->Range().end.index);
    EXPECT_FALSE(ark::es2panda::lsp::IsDeclarationOrModifier(inner, outer));

    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(inner);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, inner);
    const size_t innerStart = src.find("Inner");
    ASSERT_NE(innerStart, std::string::npos);
    EXPECT_EQ(inner->Range().start.index, innerStart);
    initializer.DestroyContext(ctx);
}

// PJ-003 (get_adjusted_location.cpp:39, 511-525): ETS nests `typeof typeof`
// into a parent-child TypeofExpression pair. The mirrored-kind convenience arm
// fires and resolves to FindFirstExpression of the parent, which is the parent
// itself because a typeof expression is an expression.
TEST_F(LspGetAdjustedLocationEts, NestedTypeofMirroredPairResolvesParentExpression)
{
    const char *source = "\nlet t = typeof typeof 1;\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj003_typeof.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *outer = nullptr;
    AstNode *inner = nullptr;
    ast->IterateRecursively([&outer, &inner](const AstNode *n) {
        if (n->IsTypeofExpression()) {
            if (outer == nullptr) {
                outer = const_cast<AstNode *>(n);
            } else if (inner == nullptr) {
                inner = const_cast<AstNode *>(n);
            }
        }
    });
    ASSERT_NE(outer, nullptr);
    ASSERT_NE(inner, nullptr);
    ASSERT_EQ(inner->Parent(), outer);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(inner);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, outer);
    auto adjustedOuter = ark::es2panda::lsp::GetAdjustedLocation(outer);
    ASSERT_TRUE(adjustedOuter.has_value());
    EXPECT_EQ(*adjustedOuter, outer);
    initializer.DestroyContext(ctx);
}

// PJ-003 (get_adjusted_location.cpp:40, 511-525): nested await also forms the
// mirrored AWAIT_EXPRESSION pair and resolves to the outer await expression.
TEST_F(LspGetAdjustedLocationEts, NestedAwaitMirroredPairResolvesOuterExpression)
{
    const char *source = "\nasync function f(p: Promise<number>): Promise<number> {\n    return await await p;\n}\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj003_await.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *outer = nullptr;
    AstNode *inner = nullptr;
    ast->IterateRecursively([&outer, &inner](const AstNode *n) {
        if (n->IsAwaitExpression()) {
            if (outer == nullptr) {
                outer = const_cast<AstNode *>(n);
            } else if (inner == nullptr) {
                inner = const_cast<AstNode *>(n);
            }
        }
    });
    ASSERT_NE(outer, nullptr);
    ASSERT_NE(inner, nullptr);
    ASSERT_EQ(inner->Parent(), outer);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(inner);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, outer);
    initializer.DestroyContext(ctx);
}

// PJ-003 (get_adjusted_location.cpp:37, 511-548): ETS `new C()` allocates
// ETSNewClassInstanceExpression, not the plain NEW_EXPRESSION mirrored kind, so
// the convenience pair cannot fire; the public entry returns the node itself.
TEST_F(LspGetAdjustedLocationEts, NewExpressionUsesEtsClassInstanceKindAndPinsToSelf)
{
    const char *source = "\nclass C {}\nlet v = new C();\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj003_new.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *newExpr = FindNode(ast, [](AstNode *n) { return n->IsETSNewClassInstanceExpression(); });
    ASSERT_NE(newExpr, nullptr);
    const size_t newExprStart = src.find("new C");
    ASSERT_NE(newExprStart, std::string::npos);
    EXPECT_EQ(newExpr->Range().start.index, newExprStart);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(newExpr);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, newExpr);
    initializer.DestroyContext(ctx);
}

// PJ-003 (get_adjusted_location.cpp:539-546): braceless nested for-of nests the
// inner statement directly under the outer loop, forming the FOR_OF pair that
// resolves to the first expression of the outer loop.
TEST_F(LspGetAdjustedLocationEts, BracelessForOfPairResolvesOuterFirstExpression)
{
    const char *source = "\nlet a: number[] = [1];\nfor (let x of a) for (let y of a) {};\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj003_forof.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *inner = FindNode(ast, [](AstNode *n) {
        return n->IsForOfStatement() && n->Parent() != nullptr && n->Parent()->IsForOfStatement();
    });
    ASSERT_NE(inner, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(inner);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "x");
    initializer.DestroyContext(ctx);
}

// PJ-004 (get_adjusted_location.cpp:355-378): ETS allocates TSAsExpression for
// `as` (ETSparserExpressions.cpp:839). The import/export specifier parent arms
// are TS-only shapes, so the helper resolves nothing and the public entry
// returns the as-expression itself.
TEST_F(LspGetAdjustedLocationEts, AsExpressionPinsToSelfThroughFallback)
{
    const char *source = "\nclass C {}\nlet c = new C() as C;\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj004_as.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *asExpr = FindNode(ast, [](AstNode *n) { return n->IsTSAsExpression(); });
    ASSERT_NE(asExpr, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(asExpr);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_EQ(*adjusted, asExpr);
    initializer.DestroyContext(ctx);
}

// PJ-004 (get_adjusted_location.cpp:309-324, 326-340): ETS imports are
// ETSImportDeclaration and `export *` is ETSReExportDeclaration; neither has
// type IMPORT_DECLARATION nor EXPORT_ALL_DECLARATION, so both declaration
// helpers return nullopt on every real ETS form and the public entries return
// the specifier nodes unchanged.
TEST_F(LspGetAdjustedLocationEts, EtsImportAndReExportHelpersReturnNullopt)
{
    const char *importSource = "import { Named } from './mod';\nlet u = Named;\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj004_import.ets", ES2PANDA_STATE_PARSED, importSource);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    AstNode *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *importDecl = FindNode(ast, [](AstNode *n) { return n->IsETSImportDeclaration(); });
    ASSERT_NE(importDecl, nullptr);
    AstNode *specifier = FindNode(ast, [](AstNode *n) { return n->IsImportSpecifier(); });
    ASSERT_NE(specifier, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetAdjustedLocationForImportDeclaration(importDecl, {}), std::nullopt);
    auto adjustedSpec = ark::es2panda::lsp::GetAdjustedLocation(specifier);
    ASSERT_TRUE(adjustedSpec.has_value());
    EXPECT_EQ(*adjustedSpec, specifier);
    auto adjustedDecl = ark::es2panda::lsp::GetAdjustedLocation(importDecl);
    ASSERT_TRUE(adjustedDecl.has_value());
    EXPECT_EQ(*adjustedDecl, importDecl);
    initializer.DestroyContext(ctx);

    const char *reExportSource = "export * from './stars';\n";
    es2panda_Context *ctxRe = initializer.CreateContext("pj004_exportall.ets", ES2PANDA_STATE_PARSED, reExportSource);
    ASSERT_EQ(ContextState(ctxRe), ES2PANDA_STATE_PARSED);
    AstNode *astRe = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctxRe)->parserProgram->Ast();
    ASSERT_NE(astRe, nullptr);
    AstNode *reExport = FindNode(astRe, [](AstNode *n) { return n->IsETSReExportDeclaration(); });
    ASSERT_NE(reExport, nullptr);
    EXPECT_EQ(FindNode(astRe, [](AstNode *n) { return n->IsExportAllDeclaration(); }), nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetAdjustedLocationForImportDeclaration(reExport, {}), std::nullopt);
    EXPECT_EQ(ark::es2panda::lsp::GetAdjustedLocationForExportDeclaration(reExport, {}), std::nullopt);
    initializer.DestroyContext(ctxRe);
}

// PJ-003 (get_adjusted_location.cpp:609-630): GetTouchingIdentifierName returns
// identifiers, properties and nullptr only. A member property resolves to its
// key identifier with the exact span; positions without an identifier origin
// (class keyword, whitespace) yield nullptr.
TEST_F(LspGetAdjustedLocationEts, TouchingIdentifierNameArmsExact)
{
    const char *source = "\nclass C {\n    prop = 1;\n}\nfunction main() {\n    let c = new C();\n    c.prop;\n}\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("pj003_touch.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    std::string src(source);
    const size_t propPos = src.find("prop");
    const size_t memberPos = src.find("c.prop") + strlen("c.");
    const size_t classPos = src.find("class");
    ASSERT_NE(propPos, std::string::npos);
    ASSERT_NE(memberPos, std::string::npos);
    AstNode *propNode = ark::es2panda::lsp::GetTouchingIdentifierName(ctx, propPos);
    ASSERT_NE(propNode, nullptr);
    EXPECT_TRUE(propNode->IsIdentifier());
    EXPECT_EQ(propNode->AsIdentifier()->Name(), "prop");
    EXPECT_EQ(propNode->Range().start.index, propPos);
    EXPECT_EQ(propNode->Range().end.index, propPos + strlen("prop"));
    AstNode *memberNode = ark::es2panda::lsp::GetTouchingIdentifierName(ctx, memberPos);
    ASSERT_NE(memberNode, nullptr);
    EXPECT_TRUE(memberNode->IsIdentifier());
    EXPECT_EQ(memberNode->AsIdentifier()->Name(), "prop");
    AstNode *classKeywordNode = ark::es2panda::lsp::GetTouchingIdentifierName(ctx, classPos);
    EXPECT_EQ(classKeywordNode, nullptr);
    AstNode *whitespaceNode = ark::es2panda::lsp::GetTouchingIdentifierName(ctx, src.find('\n', classPos));
    EXPECT_EQ(whitespaceNode, nullptr);
    initializer.DestroyContext(ctx);
}
}  // namespace
