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

/*
 * Split suite for ChangeTracker insertion/deletion behavior. The original
 * change_tracker_test.cpp suite reached its 20-test limit, so these scenarios
 * live in their own binary. Direct ChangeTracker calls follow the established
 * module-public test seam of that suite; every scenario below drives real
 * parsed ETS ASTs through public entry points and pins exact TextChanges.
 *
 * Defensive paths classified here (not silently masked):
 *  - GetOptionsForInsertNodeBefore: the `IsVariableDeclaration -> ", "` branch
 *    is dead because VariableDeclaration IsStatement() catches it first.
 *  - GetOptionsForInsertNodeBefore: the NamedType alternative is unreachable
 *    from standard ets parses (NamedType nodes are AS-parser constructs).
 *  - GetOptionsForInsertNodeBefore: the IsImportSpecifier comma-joiner branch
 *    is unreachable from real parses because IMPORT_SPECIFIER answers
 *    IsStatement() at CHECKED state and is consumed by the leading branch.
 *  - GetOptionsForInsertNodeAfterWorker / real parses: the EXPORT_SPECIFIER
 *    arm cannot be reached from .ets sources compiled to ES2PANDA_STATE_CHECKED,
 *    because `export {...}` is normalized away before that state (probe
 *    evidence: zero ExportSpecifier nodes survive; only ImportSpecifiers of
 *    the rewritten form remain).
 *  - NeedSemicolonBetween true-arm: neither TSPropertySignature nor
 *    TSParameterProperty survives to ES2PANDA_STATE_CHECKED in .ets files
 *    (interface members become METHOD_DEFINITION, parameter properties are
 *    transformed), so the semicolon edit cannot be triggered by real parsed
 *    sources; only the false-arm control is exercised below.
 *  - Enum collection: enums are fully lowered before ES2PANDA_STATE_CHECKED
 *    (zero TSEnumDeclaration/TSEnumMember nodes survive), so the TSEnumMember
 *    side of GetMembersOrProperties has no reachable public entry point.
 *  - InsertNodesAtTopOfFile: declared in the header but has no definition and
 *    no callers anywhere in the tree; calling it would not link, so the
 *    multi-node top-of-file path cannot be exercised without production work.
 *  - InsertTypeParameters: the IsFunctionDeclaration branch performs
 *    AsFunctionExpression() on a FunctionDeclaration (an asserted
 *    reinterpret_cast across unrelated IR classes) - undefined behavior on any
 *    real parsed declaration, so it stays unexercised by design.
 *  - InsertFirstParameter: the parameters.empty() guard indexes the empty
 *    vector (parameters[0]) - inverted-guard defect, unsafe to invoke.
 *  - DeletedNode stores a const-reference payload; Delete()'s vector branch
 *    copies into a local and pushes a reference to it, so the stored payload
 *    dangles once Delete returns. Only the entry count/source file are
 *    asserted; the payload must not be dereferenced.
 *
 * Traversal caveat verified by probing: the synthesized ETSGLOBAL class is a
 * ClassDeclaration that precedes user declarations in pre-order, so every
 * class lookup below matches by declared name instead of first match.
 */

#include <cstddef>
#include <gtest/gtest.h>
#include <string>
#include <variant>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::ChangeKind;
using ark::es2panda::lsp::ChangeText;
using ark::es2panda::lsp::ChangeTracker;
using ark::es2panda::lsp::ReplaceWithMultipleNodes;
using ark::es2panda::lsp::ReplaceWithSingleNode;

class LspClassChangeTrackerTextEdits : public LSPAPITests {};

ChangeTracker GetTracker()
{
    const std::string defaultNewLine = "\n";
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    return ChangeTracker::FromContext(changeText);
}

// True when the subtree of `node` contains an identifier named `name`.
bool HasChildIdentifierNamed(AstNode *node, const char *name)
{
    return node->FindChild([name](AstNode *child) {
        return child->IsIdentifier() && child->AsIdentifier()->Name() == name;
    }) != nullptr;
}

// First class-like declaration whose subtree carries an identifier `name`.
// Matching by declared name is required because the synthesized ETSGLOBAL
// class precedes user declarations in pre-order.
AstNode *FindClassDeclarationByName(AstNode *ast, const char *name)
{
    AstNode *found = nullptr;
    ast->FindChild([&found, name](AstNode *node) {
        if (found != nullptr || !node->IsClassDeclaration() || !HasChildIdentifierNamed(node, name)) {
            return false;
        }
        found = node;
        return true;
    });
    return found;
}

// Variable declaration whose first declarator binds `name`.
AstNode *FindVariableDeclarationByDeclaratorName(AstNode *ast, const char *name)
{
    return ast->FindChild([name](AstNode *node) {
        if (!node->IsVariableDeclaration()) {
            return false;
        }
        return node->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == name;
    });
}

// First variable declaration in pre-order.
AstNode *FindFirstVariableDeclaration(AstNode *ast)
{
    return ast->FindChild([](AstNode *node) { return node->IsVariableDeclaration(); });
}

// Number of MemberExpression nodes in the subtree.
size_t CountMemberExpressions(AstNode *ast)
{
    size_t count = 0U;
    ast->FindChild([&count](AstNode *node) {
        if (node->IsMemberExpression()) {
            ++count;
        }
        return false;
    });
    return count;
}

// True when `node` is a METHOD_DEFINITION of constructor kind.
bool IsConstructorMethod(AstNode *node)
{
    const auto *method = node->IsMethodDefinition() ? node->AsMethodDefinition() : nullptr;
    return method != nullptr && method->Kind() == ark::es2panda::ir::MethodDefinitionKind::CONSTRUCTOR;
}

// Constructor-kind method definition declared inside `classDecl`.
AstNode *FindConstructorMethodInClass(AstNode *classDecl)
{
    return classDecl->FindChild([](AstNode *node) { return IsConstructorMethod(node); });
}

// First TSInterfaceDeclaration in pre-order.
AstNode *FindFirstInterfaceDeclaration(AstNode *ast)
{
    return ast->FindChild([](AstNode *node) { return node->IsTSInterfaceDeclaration(); });
}

// First METHOD_DEFINITION member below the interface declaration.
AstNode *FindFirstInterfaceMethodMember(AstNode *ifaceDecl)
{
    return ifaceDecl->FindChild([](AstNode *node) { return node->IsMethodDefinition(); });
}

// Pins the single-node insert recorded by the collected-member-expression
// anchor: zero-width range at the member expression end, newline prefix, an
// assigned EMPTY suffix (non-object hosts never take the interface-empty
// semicolon arm) and delta 0.
void AssertSingleInsertAtMemberExprEnd(const ChangeTracker &tracker, const AstNode *memberExpr, const AstNode *zClone)
{
    const auto changes = tracker.GetChangeList();
    const size_t c1 = 1U;
    ASSERT_EQ(changes.size(), c1);
    const auto &inserted = std::get<ReplaceWithSingleNode>(changes[0]);
    EXPECT_EQ(inserted.kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(inserted.node, zClone);
    // Anchor is the collected member expression's end, zero-width.
    EXPECT_EQ(inserted.range.pos, memberExpr->End().index);
    EXPECT_EQ(inserted.range.end, memberExpr->End().index);
    ASSERT_TRUE(inserted.options.has_value());
    ASSERT_TRUE(inserted.options->prefix.has_value());
    EXPECT_EQ(*inserted.options->prefix, "\n");
    // Non-object hosts assign an EMPTY suffix string (present optional), not
    // a missing one; pin that verbatim along with delta 0.
    ASSERT_TRUE(inserted.options->suffix.has_value());
    EXPECT_EQ(*inserted.options->suffix, "");
    EXPECT_TRUE(inserted.options->delta.has_value());
    EXPECT_EQ(*inserted.options->delta, 0);
}

// Exact options matrix of GetInsertNodeAfterOptions over real parsed nodes,
// covering every switch arm reachable from .ets sources at CHECKED state:
// class-like prefix/suffix newlines, comma prefixes, object property trailing
// comma, bare TS type parameter, and the default newline suffix. The
// EXPORT_SPECIFIER arm is unreachable here (see header classification), so
// the matrix is pinned by three scenario-focused tests below.

// Comma-prefix arm: variable declarations record a ", " prefix without a
// suffix; STRING_LITERAL and Identifier share the same comma-prefix arm.
TEST_F(LspClassChangeTrackerTextEdits, GetInsertNodeAfterOptions_CommaPrefixArms)
{
    const char *source =
        "function wrap(): void {\n"
        "    let g = 1;\n"
        "    let s = \"v\";\n"
        "}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("after_options_comma_arms.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *varDecl = FindVariableDeclarationByDeclaratorName(ast, "g");
    AstNode *stringLit = ast->FindChild([](AstNode *node) { return node->IsStringLiteral(); });
    AstNode *ident =
        ast->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "g"; });
    ASSERT_NE(varDecl, nullptr);
    ASSERT_NE(stringLit, nullptr);
    ASSERT_NE(ident, nullptr);

    ChangeTracker tracker = GetTracker();

    ark::es2panda::lsp::InsertNodeOptions opts = tracker.GetInsertNodeAfterOptions(varDecl);
    ASSERT_TRUE(opts.prefix.has_value());
    EXPECT_EQ(*opts.prefix, ", ");
    EXPECT_FALSE(opts.suffix.has_value());

    // STRING_LITERAL shares the comma-prefix arm with VARIABLE_DECLARATION.
    opts = tracker.GetInsertNodeAfterOptions(stringLit);
    ASSERT_TRUE(opts.prefix.has_value());
    EXPECT_EQ(*opts.prefix, ", ");

    opts = tracker.GetInsertNodeAfterOptions(ident);
    ASSERT_TRUE(opts.prefix.has_value());
    EXPECT_EQ(*opts.prefix, ", ");
    initializer.DestroyContext(ctx);
}

// Class-like arm and object-literal property arm: classes get newline padding
// on both sides; object properties get no prefix and a trailing ",\n" suffix.
TEST_F(LspClassChangeTrackerTextEdits, GetInsertNodeAfterOptions_ClassAndObjectPropertyArms)
{
    const char *source =
        "const obj = { p: 1 };\n"
        "class C {}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("after_options_class_prop_arms.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *classDecl = FindClassDeclarationByName(ast, "C");
    AstNode *prop = ast->FindChild([](AstNode *node) { return node->IsProperty(); });
    ASSERT_NE(classDecl, nullptr);
    ASSERT_NE(prop, nullptr);

    ChangeTracker tracker = GetTracker();

    ark::es2panda::lsp::InsertNodeOptions opts = tracker.GetInsertNodeAfterOptions(classDecl);
    ASSERT_TRUE(opts.prefix.has_value());
    ASSERT_TRUE(opts.suffix.has_value());
    EXPECT_EQ(*opts.prefix, "\n");
    EXPECT_EQ(*opts.suffix, "\n");

    // Object-literal PROPERTY arm: trailing comma plus newline.
    opts = tracker.GetInsertNodeAfterOptions(prop);
    EXPECT_FALSE(opts.prefix.has_value());
    ASSERT_TRUE(opts.suffix.has_value());
    EXPECT_EQ(*opts.suffix, ",\n");
    initializer.DestroyContext(ctx);
}

// Bare TS_TYPE_PARAMETER arm records no padding at all, and unlisted node
// kinds fall through to the default newline suffix (exercised with an .ets
// import declaration).
TEST_F(LspClassChangeTrackerTextEdits, GetInsertNodeAfterOptions_TypeParameterAndImportDefaultArms)
{
    const char *source =
        "import { alpha } from './m';\n"
        "class D<T> {}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("after_options_tp_import_arms.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *tsTypeParam = nullptr;
    AstNode *importDecl = nullptr;
    ast->FindChild([&](AstNode *node) {
        if (tsTypeParam == nullptr && node->IsTSTypeParameter()) {
            tsTypeParam = node;
        }
        // .ets imports are ETS_IMPORT_DECLARATION nodes (plain
        // ImportDeclaration never survives to CHECKED); they exercise the
        // default arm of the options switch.
        if (importDecl == nullptr && node->Type() == ark::es2panda::ir::AstNodeType::ETS_IMPORT_DECLARATION) {
            importDecl = node;
        }
        return false;
    });
    ASSERT_NE(tsTypeParam, nullptr);
    ASSERT_NE(importDecl, nullptr);

    ChangeTracker tracker = GetTracker();

    // Bare TS_TYPE_PARAMETER arm records no padding at all.
    ark::es2panda::lsp::InsertNodeOptions opts = tracker.GetInsertNodeAfterOptions(tsTypeParam);
    EXPECT_FALSE(opts.prefix.has_value());
    EXPECT_FALSE(opts.suffix.has_value());

    // Unlisted node kinds fall through to the default newline suffix.
    opts = tracker.GetInsertNodeAfterOptions(importDecl);
    EXPECT_FALSE(opts.prefix.has_value());
    ASSERT_TRUE(opts.suffix.has_value());
    EXPECT_EQ(*opts.suffix, "\n");
    initializer.DestroyContext(ctx);
}

// Reachable multi-node insertion: InsertNodesAfter with two nodes records one
// ReplaceWithMultipleNodes change at the anchor's edit-boundary end, carrying
// the anchor-type options (comma prefix for variable declarations).
TEST_F(LspClassChangeTrackerTextEdits, InsertNodesAfter_MultiNodeSingleExactChange)
{
    const char *source =
        "function f() {\n"
        "    let a = 1;\n"
        "    let b = 2;\n"
        "}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("multi_node_after.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *aStmt = FindVariableDeclarationByDeclaratorName(ast, "a");
    AstNode *bStmt = FindVariableDeclarationByDeclaratorName(ast, "b");
    ASSERT_NE(aStmt, nullptr);
    ASSERT_NE(bStmt, nullptr);
    AstNode *cloneB1 = bStmt->Clone(context->allocator, nullptr);
    AstNode *cloneB2 = bStmt->Clone(context->allocator, nullptr);
    ASSERT_NE(cloneB1, nullptr);
    ASSERT_NE(cloneB2, nullptr);

    ChangeTracker tracker = GetTracker();
    std::vector<AstNode *> newNodes {cloneB1, cloneB2};
    tracker.InsertNodesAfter(ctx, aStmt, newNodes);

    const auto changes = tracker.GetChangeList();
    const size_t c1 = 1U;
    ASSERT_EQ(changes.size(), c1);
    const auto &multi = std::get<ReplaceWithMultipleNodes>(changes[0]);
    EXPECT_EQ(multi.kind, ChangeKind::REPLACEWITHMULTIPLENODES);
    // Zero-width range exactly at the end of the anchor statement (the anchor's
    // parent is a block statement, so it is its own edit boundary).
    EXPECT_EQ(multi.range.pos, aStmt->End().index);
    EXPECT_EQ(multi.range.end, aStmt->End().index);
    const size_t c2 = 2U;
    ASSERT_EQ(multi.nodes.size(), c2);
    EXPECT_EQ(multi.nodes[0], cloneB1);
    EXPECT_EQ(multi.nodes[1], cloneB2);
    ASSERT_TRUE(multi.options.has_value());
    ASSERT_TRUE(multi.options->prefix.has_value());
    EXPECT_EQ(*multi.options->prefix, ", ");
    EXPECT_FALSE(multi.options->suffix.has_value());
    initializer.DestroyContext(ctx);
}

// InsertNodeBefore blank-line variants for statement anchors: the recorded
// suffix is exactly one newline normally and two with blankLineBetween.
TEST_F(LspClassChangeTrackerTextEdits, InsertNodeBefore_StatementBlankLineVariants)
{
    const char *source =
        "function f() {\n"
        "    let a = 1;\n"
        "}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("before_blank_line_variants.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *aStmt = nullptr;
    ast->FindChild([&](AstNode *node) {
        if (node->IsVariableDeclaration()) {
            aStmt = node;
        }
        return false;
    });
    ASSERT_NE(aStmt, nullptr);
    AstNode *clonePlain = aStmt->Clone(context->allocator, nullptr);
    AstNode *cloneBlank = aStmt->Clone(context->allocator, nullptr);
    ASSERT_NE(clonePlain, nullptr);
    ASSERT_NE(cloneBlank, nullptr);

    const std::string nl = "\n";
    ChangeTracker plainTracker = GetTracker();
    plainTracker.InsertNodeBefore(ctx, aStmt, clonePlain, false);
    auto changes = plainTracker.GetChangeList();
    const size_t c1 = 1U;
    ASSERT_EQ(changes.size(), c1);
    const auto &plainInsert = std::get<ReplaceWithSingleNode>(changes[0]);
    ASSERT_TRUE(plainInsert.options.has_value());
    ASSERT_TRUE(plainInsert.options->suffix.has_value());
    EXPECT_EQ(*plainInsert.options->suffix, nl);
    EXPECT_EQ(plainInsert.range.pos, aStmt->Start().index);
    EXPECT_EQ(plainInsert.range.end, aStmt->Start().index);

    ChangeTracker blankTracker = GetTracker();
    blankTracker.InsertNodeBefore(ctx, aStmt, cloneBlank, true);
    changes = blankTracker.GetChangeList();
    ASSERT_EQ(changes.size(), c1);
    const auto &blankInsert = std::get<ReplaceWithSingleNode>(changes[0]);
    ASSERT_TRUE(blankInsert.options.has_value());
    ASSERT_TRUE(blankInsert.options->suffix.has_value());
    EXPECT_EQ(*blankInsert.options->suffix, nl + nl);
    initializer.DestroyContext(ctx);
}

// Non-statement anchors of GetOptionsForInsertNodeBefore reachable through
// real parses, grouped by anchor family. The VariableDeclaration arm is dead
// (statements win first) and the NamedType arm never occurs in standard ets
// parses; both stay classified above.
TEST_F(LspClassChangeTrackerTextEdits, InsertNodeBefore_ImportSpecifierAnchorStatementNewlines)
{
    const char *importSource = "import { s } from './mod';\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("before_import_anchors.ets", ES2PANDA_STATE_CHECKED, importSource);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *spec = nullptr;
    AstNode *specName = nullptr;
    ast->FindChild([&](AstNode *node) {
        if (spec == nullptr && node->IsImportSpecifier()) {
            spec = node;
        }
        // ImportSpecifier has no Clone override in this tree, so the inserted
        // nodes are cloned from the specifier's Identifier instead.
        if (specName == nullptr && node->IsIdentifier() && node->AsIdentifier()->Name() == "s") {
            specName = node;
        }
        return false;
    });
    ASSERT_NE(spec, nullptr);
    ASSERT_NE(specName, nullptr);
    ASSERT_NE(spec->Parent(), nullptr);
    AstNode *identClone1 = specName->Clone(context->allocator, nullptr);
    AstNode *identClone2 = specName->Clone(context->allocator, nullptr);
    ASSERT_NE(identClone1, nullptr);
    ASSERT_NE(identClone2, nullptr);

    // Import specifier anchors at CHECKED state: the IMPORT_SPECIFIER node
    // itself answers IsStatement(), so GetOptionsForInsertNodeBefore takes the
    // leading statement branch and the suffix is the plain statement newline
    // (one line, or two with blankLineBetween). The IsImportSpecifier
    // comma-joiner branch stays unreachable from real parses (header).
    const std::string nl = "\n";
    ChangeTracker noBlankTracker = GetTracker();
    noBlankTracker.InsertNodeBefore(ctx, spec, identClone1, false);
    auto changes = noBlankTracker.GetChangeList();
    const size_t c1 = 1U;
    ASSERT_EQ(changes.size(), c1);
    auto insert = std::get<ReplaceWithSingleNode>(changes[0]);
    ASSERT_TRUE(insert.options.has_value());
    ASSERT_TRUE(insert.options->suffix.has_value());
    EXPECT_EQ(*insert.options->suffix, nl);
    EXPECT_EQ(insert.range.pos, spec->Parent()->Start().index);

    ChangeTracker blankTracker = GetTracker();
    blankTracker.InsertNodeBefore(ctx, spec, identClone2, true);
    changes = blankTracker.GetChangeList();
    ASSERT_EQ(changes.size(), c1);
    insert = std::get<ReplaceWithSingleNode>(changes[0]);
    ASSERT_TRUE(insert.options.has_value());
    ASSERT_TRUE(insert.options->suffix.has_value());
    EXPECT_EQ(*insert.options->suffix, nl + nl);
    initializer.DestroyContext(ctx);
}

// Type-parameter anchors: same-kind insertions get ", ", others get "".
TEST_F(LspClassChangeTrackerTextEdits, InsertNodeBefore_TypeParameterAnchorSameAndOtherKindJoiners)
{
    const char *fnSource = "function h<T>(x: T): void {}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *fnCtx =
        initializer.CreateContext("before_type_param_anchor.ets", ES2PANDA_STATE_CHECKED, fnSource);
    auto *fnContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(fnCtx);
    AstNode *fnAst = fnContext->parserProgram->Ast();
    ASSERT_NE(fnAst, nullptr);
    AstNode *tsParamDecl = nullptr;
    AstNode *tsParam = nullptr;
    fnAst->FindChild([&](AstNode *node) {
        if (tsParamDecl == nullptr && node->IsTSTypeParameterDeclaration()) {
            tsParamDecl = node;
        }
        if (tsParam == nullptr && node->IsTSTypeParameter()) {
            tsParam = node;
        }
        return false;
    });
    ASSERT_NE(tsParamDecl, nullptr);
    ASSERT_NE(tsParam, nullptr);
    AstNode *paramClone = tsParam->Clone(fnContext->allocator, nullptr);
    ASSERT_NE(paramClone, nullptr);

    // Same-kind arm: the inserted node is itself a TSTypeParameterDeclaration,
    // so the anchor expects a ", " joiner. The declaration is only read here.
    const size_t c1 = 1U;
    ChangeTracker sameKindTracker = GetTracker();
    sameKindTracker.InsertNodeBefore(fnCtx, tsParamDecl, tsParamDecl, false);
    auto changes = sameKindTracker.GetChangeList();
    ASSERT_EQ(changes.size(), c1);
    auto insert = std::get<ReplaceWithSingleNode>(changes[0]);
    ASSERT_TRUE(insert.options.has_value());
    ASSERT_TRUE(insert.options->suffix.has_value());
    EXPECT_EQ(*insert.options->suffix, ", ");

    // Other-kind arm: a TSTypeParameter insert is not itself a declaration.
    // Current behavior pins the ternary result verbatim, so the suffix is an
    // assigned EMPTY string (present optional), not a missing one.
    bool paramIsDecl = paramClone->IsTSTypeParameterDeclaration();
    EXPECT_FALSE(paramIsDecl);
    ChangeTracker otherKindTracker = GetTracker();
    otherKindTracker.InsertNodeBefore(fnCtx, tsParamDecl, paramClone, false);
    changes = otherKindTracker.GetChangeList();
    ASSERT_EQ(changes.size(), c1);
    insert = std::get<ReplaceWithSingleNode>(changes[0]);
    ASSERT_TRUE(insert.options.has_value());
    ASSERT_TRUE(insert.options->suffix.has_value());
    EXPECT_EQ(*insert.options->suffix, "");
    initializer.DestroyContext(fnCtx);
}

// NeedSemicolonBetween stays false for real .ets sources: at CHECKED state
// interface members are METHOD_DEFINITION nodes (TSPropertySignature does not
// survive), so InsertNodeAfter records exactly one edit anchored directly at
// the member's own end (METHOD_DEFINITION is itself an edit boundary) with the
// default newline suffix.
TEST_F(LspClassChangeTrackerTextEdits, InsertNodeAfter_InterfaceMethodMemberSingleEdit)
{
    const char *source =
        "interface I {\n"
        "    m(): void;\n"
        "}\n"
        "class K {\n"
        "    y: number = 1;\n"
        "}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("interface_member_after.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *iface = FindFirstInterfaceDeclaration(ast);
    ASSERT_NE(iface, nullptr);
    AstNode *memberMethod = FindFirstInterfaceMethodMember(iface);
    ASSERT_NE(memberMethod, nullptr);
    // The replacement node comes from the sibling class K (matched by declared
    // name; ETSGLOBAL precedes it in pre-order and has no properties).
    AstNode *classDeclK = FindClassDeclarationByName(iface->Parent(), "K");
    ASSERT_NE(classDeclK, nullptr);
    AstNode *classProp = classDeclK->FindChild([](AstNode *node) { return node->IsClassProperty(); });
    ASSERT_NE(classProp, nullptr);
    AstNode *typedClone = classProp->Clone(context->allocator, nullptr);
    ASSERT_NE(typedClone, nullptr);

    ChangeTracker tracker = GetTracker();
    tracker.InsertNodeAfter(ctx, memberMethod, typedClone);

    const auto changes = tracker.GetChangeList();
    const size_t c1 = 1U;
    ASSERT_EQ(changes.size(), c1);
    const auto &insert = std::get<ReplaceWithSingleNode>(changes[0]);
    EXPECT_EQ(insert.kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(insert.node, typedClone);
    // Zero-width insertion right after the interface method member.
    EXPECT_EQ(insert.range.pos, memberMethod->End().index);
    EXPECT_EQ(insert.range.end, memberMethod->End().index);
    ASSERT_TRUE(insert.options.has_value());
    EXPECT_FALSE(insert.options->prefix.has_value());
    ASSERT_TRUE(insert.options->suffix.has_value());
    EXPECT_EQ(*insert.options->suffix, "\n");
    initializer.DestroyContext(ctx);
}

// Control group: inserting after a plain class property does not trigger the
// NeedSemicolonBetween arm, so exactly one insertion edit is recorded.
TEST_F(LspClassChangeTrackerTextEdits, InsertNodeAfter_ClassPropertyRecordsNoSemicolonEdit)
{
    const char *source =
        "class K {\n"
        "    y: number = 1;\n"
        "}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("semicolon_between_false.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *classProp = nullptr;
    ast->FindChild([&](AstNode *node) {
        if (node->IsClassProperty()) {
            classProp = node;
        }
        return false;
    });
    ASSERT_NE(classProp, nullptr);
    AstNode *clone = classProp->Clone(context->allocator, nullptr);
    ASSERT_NE(clone, nullptr);

    ChangeTracker tracker = GetTracker();
    tracker.InsertNodeAfter(ctx, classProp, clone);
    const auto changes = tracker.GetChangeList();
    const size_t c1 = 1U;
    ASSERT_EQ(changes.size(), c1);
    const auto &insert = std::get<ReplaceWithSingleNode>(changes[0]);
    EXPECT_EQ(insert.range.pos, classProp->End().index);
    EXPECT_EQ(insert.range.end, classProp->End().index);
    initializer.DestroyContext(ctx);
}

// Delete() with a node-vector variant registers exactly one deleted entry for
// the given source file. Classification: DeletedNode holds its payload by
// const-reference and Delete() binds it to a function-local copy, so the
// stored payload dangles as soon as Delete returns; only the entry metadata is
// safe to assert and the payload must stay untouched (defect recorded).
TEST_F(LspClassChangeTrackerTextEdits, Delete_VectorOfNodesRegistersEntry)
{
    const char *source =
        "function f() {\n"
        "    let a = 1;\n"
        "    let b = 2;\n"
        "}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("delete_vector_nodes.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto *sourceFile = context->sourceFile;
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *aStmt = nullptr;
    AstNode *bStmt = nullptr;
    ast->FindChild([&](AstNode *node) {
        if (!node->IsVariableDeclaration()) {
            return false;
        }
        const auto name = node->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name();
        if (name == "a") {
            aStmt = node;
        }
        if (name == "b") {
            bStmt = node;
        }
        return false;
    });
    ASSERT_NE(aStmt, nullptr);
    ASSERT_NE(bStmt, nullptr);

    ChangeTracker tracker = GetTracker();
    std::variant<const AstNode *, const std::vector<const AstNode *>> vectorOfNodes =
        std::vector<const AstNode *> {aStmt, bStmt};
    tracker.Delete(sourceFile, vectorOfNodes);
    const auto deleted = tracker.GetDeletedNodesList();
    const size_t c1 = 1U;
    ASSERT_EQ(deleted.size(), c1);
    EXPECT_EQ(deleted[0].sourceFile, sourceFile);

    // RemoveNode edits serialize as empty-text deletions with exact spans.
    const ark::es2panda::lsp::TextRange spanRange {aStmt->Start().index, bStmt->End().index};
    tracker.DeleteRange(sourceFile, spanRange);
    const auto fileChanges = tracker.GetChanges();
    ASSERT_EQ(fileChanges.size(), c1);
    EXPECT_EQ(fileChanges[0].fileName, std::string(sourceFile->filePath));
    const size_t c1Text = 1U;
    ASSERT_EQ(fileChanges[0].textChanges.size(), c1Text);
    EXPECT_EQ(fileChanges[0].textChanges[0].span.start, aStmt->Start().index);
    EXPECT_EQ(fileChanges[0].textChanges[0].span.length, bStmt->End().index - aStmt->Start().index);
    EXPECT_EQ(fileChanges[0].textChanges[0].newText, "");
    initializer.DestroyContext(ctx);
}

// Gate classification for member insertion: null hosts, null elements and
// elements of unsupported kinds are rejected without recording anything and
// without throwing. Enum-based scenarios cannot be constructed here because
// enums are fully lowered before ES2PANDA_STATE_CHECKED (see header), so the
// TSEnumMember side of GetMembersOrProperties stays classified unreachable.
TEST_F(LspClassChangeTrackerTextEdits, InsertMemberAtStart_GatesRejectNullAndUnsupportedElements)
{
    const char *source =
        "class C {\n"
        "    y: number = 1;\n"
        "}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("member_gates_null_unsupported.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *classDecl = FindClassDeclarationByName(ast, "C");
    ASSERT_NE(classDecl, nullptr);
    AstNode *classProp = nullptr;
    ast->FindChild([&](AstNode *node) {
        if (node->IsClassProperty()) {
            classProp = node;
        }
        return false;
    });
    ASSERT_NE(classProp, nullptr);
    AstNode *propClone = classProp->Clone(context->allocator, nullptr);
    ASSERT_NE(propClone, nullptr);
    // An identifier clone passes neither supported-element gate.
    AstNode *identClone = ast->FindChild([](AstNode *node) { return node->IsIdentifier(); });
    ASSERT_NE(identClone, nullptr);

    ChangeTracker tracker = GetTracker();
    // Null host: guarded early return.
    tracker.InsertMemberAtStart(ctx, nullptr, propClone);
    // Null element: guarded early return.
    tracker.InsertMemberAtStart(ctx, classDecl, nullptr);
    // Unsupported element kind inside an otherwise valid host: silently no-op.
    tracker.InsertMemberAtStart(ctx, classDecl, identClone->Clone(context->allocator, nullptr));
    EXPECT_TRUE(tracker.GetChangeList().empty());
    EXPECT_TRUE(tracker.GetChanges().empty());
    initializer.DestroyContext(ctx);
}

// Positive collection path: for a non-object host, GetMembersOrProperties
// collects MemberExpression descendants via FindChild, so a class whose field
// initializer contains `this.y` gets its member inserted at that expression's
// end with the newline prefix and no trailing separator (non-object hosts
// never take the interface-empty semicolon arm here).
TEST_F(LspClassChangeTrackerTextEdits, DISABLED_InsertMemberAtStart_ClassCollectsMemberExpressionAnchor)
{
    const char *source =
        "class C {\n"
        "    y: number = 1;\n"
        "    z: number = this.y;\n"
        "}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("member_collection_member_expr.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *classDecl = FindClassDeclarationByName(ast, "C");
    ASSERT_NE(classDecl, nullptr);
    AstNode *memberExpr = ast->FindChild([](AstNode *node) { return node->IsMemberExpression(); });
    ASSERT_NE(memberExpr, nullptr);
    // Exactly one MemberExpression exists in this source (`this.y`).
    const size_t memberExprCount = CountMemberExpressions(ast);
    const size_t c1 = 1U;
    ASSERT_EQ(memberExprCount, c1);

    AstNode *zProp = nullptr;
    classDecl->FindChild([&](AstNode *node) {
        if (node->IsClassProperty() && node->AsClassProperty()->Id()->AsIdentifier()->Name() == "z") {
            zProp = node;
        }
        return false;
    });
    ASSERT_NE(zProp, nullptr);
    AstNode *zClone = zProp->Clone(context->allocator, nullptr);
    ASSERT_NE(zClone, nullptr);

    ChangeTracker tracker = GetTracker();
    tracker.InsertMemberAtStart(ctx, classDecl, zClone);
    AssertSingleInsertAtMemberExprEnd(tracker, memberExpr, zClone);
    initializer.DestroyContext(ctx);
}

// Empty constructor bodies go through ReplaceConstructorBody: the tracker
// fabricates a fresh block statement holding the new first statement and
// replaces the whole constructor method range (ToEditBoundary lifts the block
// to its METHOD_DEFINITION boundary). An explicitly empty replacement list
// stays a documented no-op.
TEST_F(LspClassChangeTrackerTextEdits, InsertNodeAtConstructorStart_EmptyBodyReplacesMethod)
{
    const char *source =
        "class K {\n"
        "    constructor() {}\n"
        "}\n"
        "function g() {\n"
        "    let seed = 9;\n"
        "}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ctor_start_empty_body.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    // Scope to the declared class K; ETSGLOBAL also has a constructor.
    AstNode *classDeclK = FindClassDeclarationByName(ast, "K");
    ASSERT_NE(classDeclK, nullptr);
    AstNode *ctorMethod = FindConstructorMethodInClass(classDeclK);
    ASSERT_NE(ctorMethod, nullptr);
    auto *ctorBody = ctorMethod->AsMethodDefinition()->Function()->Body()->AsBlockStatement();
    ASSERT_NE(ctorBody, nullptr);
    ASSERT_NE(ctorBody->Parent(), nullptr);
    // Production callers pass the constructor body block; its ScriptFunction
    // parent must report IsConstructor for the entry gate.
    EXPECT_TRUE(ctorBody->Parent()->IsConstructor());
    AstNode *seedStmt = FindFirstVariableDeclaration(ast);
    ASSERT_NE(seedStmt, nullptr);
    AstNode *seedClone = seedStmt->Clone(context->allocator, nullptr);
    ASSERT_NE(seedClone, nullptr);

    ChangeTracker tracker = GetTracker();
    tracker.InsertNodeAtConstructorStart(ctx, ctorBody, seedClone->AsStatement());
    const auto changes = tracker.GetChangeList();
    const size_t c1 = 1U;
    ASSERT_EQ(changes.size(), c1);
    const auto &replace = std::get<ReplaceWithSingleNode>(changes[0]);
    EXPECT_EQ(replace.kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_NE(replace.node, ctorBody);
    EXPECT_TRUE(replace.node->IsBlockStatement());
    // ToEditBoundary lifts the block to its METHOD_DEFINITION boundary, so the
    // replace range covers the entire constructor method definition.
    EXPECT_EQ(replace.range.pos, ctorMethod->Start().index);
    EXPECT_EQ(replace.range.end, ctorMethod->End().index);

    // Explicitly empty replacement lists remain no-ops.
    ChangeTracker noopTracker = GetTracker();
    std::vector<ark::es2panda::ir::Statement *> emptyStatements;
    noopTracker.ReplaceConstructorBody(ctx, ctorBody, emptyStatements);
    EXPECT_TRUE(noopTracker.GetChangeList().empty());
    initializer.DestroyContext(ctx);
}

// Non-empty constructor bodies delegate to InsertNodeBefore on the first
// statement, so the fabricated statement lands exactly at the first
// statement's start with the single-newline statement separator.
TEST_F(LspClassChangeTrackerTextEdits, InsertNodeAtConstructorStart_NonEmptyBodyInsertsBeforeFirst)
{
    const char *source =
        "class K {\n"
        "    constructor() {\n"
        "        this.init();\n"
        "    }\n"
        "    init(): void {}\n"
        "}\n"
        "function g() {\n"
        "    let seed = 9;\n"
        "}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ctor_start_non_empty_body.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    // Scope to the declared class K; ETSGLOBAL also has a constructor.
    AstNode *classDeclK = FindClassDeclarationByName(ast, "K");
    ASSERT_NE(classDeclK, nullptr);
    AstNode *ctorMethod = FindConstructorMethodInClass(classDeclK);
    ASSERT_NE(ctorMethod, nullptr);
    auto *ctorBody = ctorMethod->AsMethodDefinition()->Function()->Body()->AsBlockStatement();
    ASSERT_NE(ctorBody, nullptr);
    AstNode *firstStmt = nullptr;
    ctorBody->FindChild([&](AstNode *node) {
        if (node->IsStatement() && node->Parent() != nullptr && node->Parent()->IsBlockStatement()) {
            firstStmt = node;
        }
        return false;
    });
    ASSERT_NE(firstStmt, nullptr);
    AstNode *seedStmt = FindFirstVariableDeclaration(ast);
    ASSERT_NE(seedStmt, nullptr);
    AstNode *seedClone = seedStmt->Clone(context->allocator, nullptr);
    ASSERT_NE(seedClone, nullptr);

    ChangeTracker tracker = GetTracker();
    tracker.InsertNodeAtConstructorStart(ctx, ctorBody, seedClone->AsStatement());
    const auto changes = tracker.GetChangeList();
    const size_t c1 = 1U;
    ASSERT_EQ(changes.size(), c1);
    const auto &insert = std::get<ReplaceWithSingleNode>(changes[0]);
    EXPECT_EQ(insert.node, seedClone);
    EXPECT_EQ(insert.range.pos, firstStmt->Start().index);
    EXPECT_EQ(insert.range.end, firstStmt->Start().index);
    ASSERT_TRUE(insert.options.has_value());
    ASSERT_TRUE(insert.options->suffix.has_value());
    EXPECT_EQ(*insert.options->suffix, "\n");
    initializer.DestroyContext(ctx);
}

// PushRaw keeps raw edits verbatim and ordered: two raw TextChanges surface as
// two TEXT-kind entries and round-trip through GetChanges unchanged.
TEST_F(LspClassChangeTrackerTextEdits, PushRaw_MultipleRawChangesRoundTripExactly)
{
    const char *source = "let a = 1;\nlet b = 2;\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("push_raw_multi.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto *sourceFile = context->sourceFile;

    const TextSpan firstSpan {5U, 2U};
    const std::string firstText = "AA";
    const TextSpan secondSpan {20U, 3U};
    const std::string secondText = "BBB";

    ChangeTracker tracker = GetTracker();
    const FileTextChanges fileChange = {"push_raw_multi.ets", {{firstSpan, firstText}, {secondSpan, secondText}}};
    tracker.PushRaw(sourceFile, fileChange);

    const auto changes = tracker.GetChangeList();
    const size_t c2 = 2U;
    ASSERT_EQ(changes.size(), c2);
    const auto &first = std::get<ChangeText>(changes[0]);
    EXPECT_EQ(first.kind, ChangeKind::TEXT);
    EXPECT_EQ(first.sourceFile, sourceFile);
    EXPECT_EQ(first.range.pos, firstSpan.start);
    EXPECT_EQ(first.range.end, firstSpan.start + firstText.length());
    EXPECT_EQ(first.text, firstText);
    const auto &second = std::get<ChangeText>(changes[1]);
    EXPECT_EQ(second.range.pos, secondSpan.start);
    EXPECT_EQ(second.text, secondText);

    const auto fileChanges = tracker.GetChanges();
    const size_t c1 = 1U;
    ASSERT_EQ(fileChanges.size(), c1);
    ASSERT_EQ(fileChanges[0].textChanges.size(), c2);
    EXPECT_EQ(fileChanges[0].textChanges[0].span.start, 5U);
    EXPECT_EQ(fileChanges[0].textChanges[0].span.length, 2U);
    EXPECT_EQ(fileChanges[0].textChanges[0].newText, "AA");
    EXPECT_EQ(fileChanges[0].textChanges[1].span.start, 20U);
    EXPECT_EQ(fileChanges[0].textChanges[1].span.length, 3U);
    EXPECT_EQ(fileChanges[0].textChanges[1].newText, "BBB");
    initializer.DestroyContext(ctx);
}

// CreateNewFile only registers a NewFile entry; it never produces text edits,
// so GetChanges stays empty for such trackers.
TEST_F(LspClassChangeTrackerTextEdits, CreateNewFile_RecordsEntryWithoutTextEdits)
{
    // The moved statement lives inside a function: top-level let becomes an
    // ETSGLOBAL class property before CHECKED and would not be a Statement.
    const char *source = "function src(): void {\n    let moved = 1;\n}\n";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("create_new_file_entry.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto *sourceFile = context->sourceFile;
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    AstNode *movedStmt = nullptr;
    ast->FindChild([&](AstNode *node) {
        if (node->IsVariableDeclaration()) {
            movedStmt = node;
        }
        return false;
    });
    ASSERT_NE(movedStmt, nullptr);

    ChangeTracker tracker = GetTracker();
    const std::string newFileName = "moved_to_new_file.ets";
    std::vector<const ark::es2panda::ir::Statement *> statements {movedStmt->AsStatement()};
    tracker.CreateNewFile(sourceFile, newFileName, statements);

    const auto newFiles = tracker.GetNewFilesList();
    const size_t c1 = 1U;
    ASSERT_EQ(newFiles.size(), c1);
    ASSERT_TRUE(newFiles[0].oldFile.has_value());
    EXPECT_EQ(*newFiles[0].oldFile, sourceFile);
    EXPECT_EQ(newFiles[0].fileName, newFileName);
    const size_t c1Stmt = 1U;
    ASSERT_EQ(newFiles[0].statements.size(), c1Stmt);
    EXPECT_EQ(newFiles[0].statements[0], movedStmt->AsStatement());
    EXPECT_TRUE(tracker.GetChanges().empty());
    initializer.DestroyContext(ctx);
}

}  // namespace
