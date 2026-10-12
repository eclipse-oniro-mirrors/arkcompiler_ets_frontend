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

#include <cstddef>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp/include/services/text_change/text_change_context.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::ChangeKind;
using ark::es2panda::lsp::ChangeTracker;
using ark::es2panda::lsp::FormatCodeSettings;
using ark::es2panda::lsp::GetFormatContext;
using ark::es2panda::lsp::ReplaceWithSingleNode;

class LspChangeTrackerInsertAtStartTests : public LSPAPITests {
protected:
    ChangeTracker MakeTracker()
    {
        FormatCodeSettings settings;
        auto formatContext = GetFormatContext(settings);
        TextChangesContext changeText {{}, formatContext, {}};
        return ChangeTracker::FromContext(changeText);
    }

    TextChangesContext MakeContext()
    {
        FormatCodeSettings settings;
        return {{}, GetFormatContext(settings), {}};
    }

    // Helper: find the first node satisfying a predicate
    ark::es2panda::ir::AstNode *FindNode(ark::es2panda::ir::AstNode *root,
                                         std::function<bool(ark::es2panda::ir::AstNode *)> pred)
    {
        ark::es2panda::ir::AstNode *result = nullptr;
        root->FindChild([&result, &pred](ark::es2panda::ir::AstNode *n) {
            if (result == nullptr && pred(n)) {
                result = n;
            }
            return false;
        });
        return result;
    }
};

// P1C-PG-001: InsertNodeAtObjectStart with a supported newElement (MethodDefinition).
// Chain: InsertNodeAtObjectStart (change_tracker.cpp:707-711)
//        -> InsertNodeAtStartWorker (change_tracker.cpp:271-287)
//        -> GetMembersOrProperties (change_tracker.cpp:237-255)
//        -> GetInsertNodeAtStartInsertOptions (change_tracker.cpp:257-269)
// GetInsertNodeAtStartInsertOptions unconditionally evaluates
// classesWithNodesInsertedAtStart_.at(0) (change_tracker.cpp:261) but the private map
// (change_tracker.h:155) has no write path anywhere in the codebase, so std::map::at
// throws std::out_of_range before any TextChange can be produced.
// Characterization: the measured product behavior is the exact exception type and an
// empty change list; a producing test requires a product fix (BLOCKED_PRODUCT_FIX).
TEST_F(LspChangeTrackerInsertAtStartTests, DISABLED_ObjectStart_MethodElement_ThrowsOutOfRange)
{
    const std::string source = R"(
let base = { x: 1, y: 2 };
class K {
    m(): void {}
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("objectStartMethod.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *xProp = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsProperty() && n->AsProperty()->Key()->AsIdentifier()->Name() == "x";
    });
    ASSERT_NE(xProp, nullptr);
    auto *objExpr = xProp->Parent();
    ASSERT_NE(objExpr, nullptr);
    ASSERT_TRUE(objExpr->IsObjectExpression());

    auto *methodDef = FindNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsMethodDefinition(); });
    ASSERT_NE(methodDef, nullptr);

    auto tracker = MakeTracker();
    EXPECT_THROW(tracker.InsertNodeAtObjectStart(ctx, objExpr->AsObjectExpression(), methodDef), std::out_of_range);
    EXPECT_TRUE(tracker.GetChangeList().empty());

    // Same route through the ChangeTracker::With harness used by production code fixes.
    auto textChangesContext = MakeContext();
    EXPECT_THROW(ChangeTracker::With(textChangesContext,
                                     [&](ChangeTracker &t) {
                                         t.InsertNodeAtObjectStart(ctx, objExpr->AsObjectExpression(), methodDef);
                                     }),
                 std::out_of_range);
    initializer.DestroyContext(ctx);
}

// BLOCKED_PRODUCT_FIX (P1C-PG-001): InsertNodeAtObjectStart with a SpreadElement newElement
// (InsertNodeAtStartWorker newElement shape arm change_tracker.cpp:279).
// Same empty-registry std::map::at defect as DISABLED_ObjectStart_MethodElement_ThrowsOutOfRange
// (change_tracker.cpp:261, change_tracker.h:155): exact exception type is pinned.
TEST_F(LspChangeTrackerInsertAtStartTests, DISABLED_ObjectStart_SpreadElement_ThrowsOutOfRange)
{
    const std::string source = R"(
let base = { x: 1 };
let ext = { ...base, y: 2 };
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("objectStartSpread.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *xProp = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsProperty() && n->AsProperty()->Key()->AsIdentifier()->Name() == "x";
    });
    ASSERT_NE(xProp, nullptr);
    auto *baseObj = xProp->Parent();
    ASSERT_NE(baseObj, nullptr);
    ASSERT_TRUE(baseObj->IsObjectExpression());

    auto *spread = FindNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsSpreadElement(); });
    ASSERT_NE(spread, nullptr);

    auto tracker = MakeTracker();
    EXPECT_THROW(tracker.InsertNodeAtObjectStart(ctx, baseObj->AsObjectExpression(), spread), std::out_of_range);
    EXPECT_TRUE(tracker.GetChangeList().empty());
    initializer.DestroyContext(ctx);
}

// P1C-PG-001: InsertNodeAtObjectStart negative arm — a newElement that is none of
// ClassProperty/SpreadElement/MethodDefinition/TSPropertySignature
// (change_tracker.cpp:279-280) returns silently without recording any change.
TEST_F(LspChangeTrackerInsertAtStartTests, ObjectStart_UnsupportedElement_SilentNoOp)
{
    const std::string source = R"(
let base = { x: 1, y: 2 };
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("objectStartUnsupported.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *xProp = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsProperty() && n->AsProperty()->Key()->AsIdentifier()->Name() == "x";
    });
    ASSERT_NE(xProp, nullptr);
    auto *objExpr = xProp->Parent();
    ASSERT_NE(objExpr, nullptr);
    ASSERT_TRUE(objExpr->IsObjectExpression());

    ark::es2panda::ir::AstNode *identClone = nullptr;
    xProp->FindChild([&identClone, context](ark::es2panda::ir::AstNode *n) {
        if (identClone == nullptr && n->IsIdentifier() && n->AsIdentifier()->Name() == "x") {
            identClone = n->Clone(context->allocator, nullptr);
            return true;
        }
        return false;
    });
    ASSERT_NE(identClone, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertNodeAtObjectStart(ctx, objExpr->AsObjectExpression(), identClone);
    EXPECT_TRUE(tracker.GetChangeList().empty());
    initializer.DestroyContext(ctx);
}

// P1C-PG-001: InsertNodeAtObjectStart with a null newElement hits the null guard
// (change_tracker.cpp:274-276) and returns silently without changes.
TEST_F(LspChangeTrackerInsertAtStartTests, ObjectStart_NullElement_SilentNoOp)
{
    const std::string source = R"(
let base = { x: 1, y: 2 };
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("objectStartNull.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *xProp = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsProperty() && n->AsProperty()->Key()->AsIdentifier()->Name() == "x";
    });
    ASSERT_NE(xProp, nullptr);
    auto *objExpr = xProp->Parent();
    ASSERT_NE(objExpr, nullptr);
    ASSERT_TRUE(objExpr->IsObjectExpression());

    auto tracker = MakeTracker();
    tracker.InsertNodeAtObjectStart(ctx, objExpr->AsObjectExpression(), nullptr);
    EXPECT_TRUE(tracker.GetChangeList().empty());
    initializer.DestroyContext(ctx);
}

// BLOCKED_PRODUCT_FIX (P1C-PG-002): InsertMemberAtStart on a class declaration with a ClassProperty newElement
// (change_tracker.cpp:694-705) -> InsertNodeAtStartWorker class arm
// (change_tracker.cpp:277, 279) -> GetMembersOrProperties non-object arm
// (change_tracker.cpp:246-252, empty here) -> GetInsertNodeAtStartInsertOptions
// (change_tracker.cpp:257) -> classesWithNodesInsertedAtStart_.at(0) throws
// (change_tracker.cpp:261). Exact exception type and empty change list are pinned.
TEST_F(LspChangeTrackerInsertAtStartTests, DISABLED_MemberAtStart_ClassProperty_ThrowsOutOfRange)
{
    const std::string source = R"(
class C {
    p: number = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("memberAtStartClass.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *classProp = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsClassProperty() && n->AsClassProperty()->Id()->AsIdentifier()->Name() == "p";
    });
    ASSERT_NE(classProp, nullptr);
    auto *classDecl = classProp->Parent()->Parent();
    ASSERT_NE(classDecl, nullptr);
    ASSERT_TRUE(classDecl->IsClassDeclaration());

    auto tracker = MakeTracker();
    EXPECT_THROW(tracker.InsertMemberAtStart(ctx, classDecl, classProp), std::out_of_range);
    EXPECT_TRUE(tracker.GetChangeList().empty());
    initializer.DestroyContext(ctx);
}

// BLOCKED_PRODUCT_FIX (P1C-PG-002): GetMembersOrProperties non-object arm with a collected MemberExpression
// (change_tracker.cpp:247-250): the class subtree contains `this.q`, so the
// membersOrProperties emplace arm runs before the std::map::at throw at
// change_tracker.cpp:261. Exact exception type is pinned.
TEST_F(LspChangeTrackerInsertAtStartTests, DISABLED_MemberAtStart_CollectsMemberExpression_ThrowsOutOfRange)
{
    const std::string source = R"(
class C {
    p: number = this.q;
    q: number = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("memberAtStartMemberExpr.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *memberExpr = FindNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsMemberExpression(); });
    ASSERT_NE(memberExpr, nullptr);

    auto *classProp = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsClassProperty() && n->AsClassProperty()->Id()->AsIdentifier()->Name() == "p";
    });
    ASSERT_NE(classProp, nullptr);
    auto *classDecl = classProp->Parent()->Parent();
    ASSERT_NE(classDecl, nullptr);
    ASSERT_TRUE(classDecl->IsClassDeclaration());

    auto tracker = MakeTracker();
    EXPECT_THROW(tracker.InsertMemberAtStart(ctx, classDecl, classProp), std::out_of_range);
    EXPECT_TRUE(tracker.GetChangeList().empty());
    initializer.DestroyContext(ctx);
}

// BLOCKED_PRODUCT_FIX (P1C-PG-002): InsertMemberAtStart on an interface declaration.
// Two measured behaviors are pinned here:
// 1. The ETS parser represents interface members `x: number` as getter
//    MethodDefinition nodes inside a TSInterfaceDeclaration. InsertMemberAtStart's
//    shape gate (change_tracker.cpp:701) accepts only ClassProperty,
//    TSPropertySignature and TSMethodSignature — NOT MethodDefinition (which the
//    worker itself does accept, change_tracker.cpp:279) — so the natural ETS route
//    returns silently. Characterized: no throw, no change.
// 2. With a ClassProperty newElement (cloned from a class in the same file) the gate
//    passes, the IsTSInterfaceDeclaration host arm (change_tracker.cpp:699) runs and
//    the chain reaches classesWithNodesInsertedAtStart_.at(0)
//    (change_tracker.cpp:261) which throws std::out_of_range on the never-populated
//    map (change_tracker.h:155). Exact exception type and empty change list pinned.
// BLOCKED_PRODUCT_FIX: the never-populated registry blocks edit production.
TEST_F(LspChangeTrackerInsertAtStartTests, DISABLED_MemberAtStart_Interface_ClassPropThrows)
{
    const std::string source = R"(
interface I {
    x: number;
}
class K {
    p: number = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("memberAtStartInterface.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *ifaceDecl = FindNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsTSInterfaceDeclaration(); });
    ASSERT_NE(ifaceDecl, nullptr);
    auto *ifaceGetter = FindNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsMethodDefinition(); });
    ASSERT_NE(ifaceGetter, nullptr);
    auto *classProp = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsClassProperty() && n->AsClassProperty()->Id()->AsIdentifier()->Name() == "p";
    });
    ASSERT_NE(classProp, nullptr);
    auto *classPropClone = classProp->Clone(context->allocator, nullptr);
    ASSERT_NE(classPropClone, nullptr);

    auto tracker = MakeTracker();
    // 1. ETS interface getter MethodDefinition is rejected by the shape gate.
    tracker.InsertMemberAtStart(ctx, ifaceDecl, ifaceGetter);
    EXPECT_TRUE(tracker.GetChangeList().empty());
    // 2. ClassProperty newElement passes the gate and hits the empty-registry throw.
    EXPECT_THROW(tracker.InsertMemberAtStart(ctx, ifaceDecl, classPropClone), std::out_of_range);
    EXPECT_TRUE(tracker.GetChangeList().empty());
    initializer.DestroyContext(ctx);
}

// P1C-PG-002 BLOCKED (TSTypeLiteral arm, change_tracker.cpp:277 and :699):
// InsertMemberAtStart/InsertNodeAtStartWorker accept a TSTypeLiteral host node, but no
// valid ETS context can produce one:
//   1. The ETS parser lowers object-literal type syntax to ErrorType
//      (`type T = { a: number }` and `p: { a: number }` both dump as ErrorType).
//   2. TSTypeLiteral is only produced by the TS parse route, and the LSP context
//      factory (Initializers::CreateContext, lsp/include/internal_api.h:44-52) builds
//      its config from a fixed argv without an --extension override
//      (lsp/src/internal_api.cpp:131-132), so every LSP context parses with the
//      default (ETS) extension regardless of the fileName suffix.
// Driving this arm would require fabricating a detached compiler node, which the test
// rules forbid. NEXT ACTION: product must expose a TS-flavoured context route (or the
// arm is dead for ArkTS) before it can be covered.

// P1C-PG-002: InsertMemberAtStart negative arm — a host node that is neither class,
// interface, type literal nor object expression (change_tracker.cpp:699-700) returns
// silently without recording any change.
TEST_F(LspChangeTrackerInsertAtStartTests, MemberAtStart_NonClassHost_SilentNoOp)
{
    const std::string source = R"(
function f(): void {
    let a = 1;
}
class K {
    m(): void {}
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("memberAtStartNonClass.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *aDecl = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsVariableDeclaration() &&
               n->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "a";
    });
    ASSERT_NE(aDecl, nullptr);
    auto *methodDef = FindNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsMethodDefinition(); });
    ASSERT_NE(methodDef, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertMemberAtStart(ctx, aDecl, methodDef);
    EXPECT_TRUE(tracker.GetChangeList().empty());
    initializer.DestroyContext(ctx);
}

// P1C-PG-005 arm sweep: ChangeTracker::ToEditBoundary (public static,
// change_tracker.h:243) on nullptr hits the IsEditBoundaryNode null arm
// (change_tracker.cpp:33-34) and the ToEditBoundary null early return
// (change_tracker.cpp:66-68).
TEST_F(LspChangeTrackerInsertAtStartTests, ToEditBoundary_NullNode_ReturnsNull)
{
    EXPECT_EQ(ChangeTracker::ToEditBoundary(nullptr), nullptr);
}

// P1C-PG-005 arm sweep: RfindNearestKeyWordTextRange keyword-not-found arm
// (change_tracker.cpp:152-153): rfind returns npos and the range is left untouched.
TEST_F(LspChangeTrackerInsertAtStartTests, RfindNearestKeyword_AbsentKeyword_RangeUntouched)
{
    const std::string source = "function test() { let a = 1; }";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rfindAbsent.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto tracker = MakeTracker();
    ark::es2panda::lsp::TextRange range = {7U, 9U};
    tracker.RfindNearestKeyWordTextRange(ctx, 0U, "zzz", range);
    EXPECT_EQ(range.pos, 7U);
    EXPECT_EQ(range.end, 9U);
    initializer.DestroyContext(ctx);
}

// P1C-PG-005 arm sweep: InsertNodeBefore on an import specifier.
// MEASURED: ir::ImportSpecifier derives from Statement (ir/module/importSpecifier.h:24),
// so GetOptionsForInsertNodeBefore takes the IsStatement() arm at
// change_tracker.cpp:223-224 — the IsImportSpecifier() arm at change_tracker.cpp:231-232
// is dead for real specifier nodes. The suffix is therefore the statement new-line
// variant ("\n" for blankLineBetween=false, "\n\n" for true), and ToEditBoundary walks
// from the specifier up to the ETSImportDeclaration edit boundary
// (change_tracker.cpp:27-55, 89-92), so both zero-length edits land at the import
// declaration start.
// InsertNodeBefore accepts any existing AST node as newNode, so live specifier nodes
// are reused here. ImportSpecifier MUST NOT be cloned: it has no Clone override and
// the base AstNode::Clone is ES2PANDA_UNREACHABLE (ir/astNode.cpp:133), which
// segfaults in release builds (product finding).
TEST_F(LspChangeTrackerInsertAtStartTests, InsertNodeBefore_ImportSpecifier_SuffixVariants)
{
    const std::string source = R"(
import { foo, bar } from './mod';
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("insertBeforeSpecifier.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *sourceFile = context->sourceFile;
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    std::vector<ark::es2panda::ir::AstNode *> specifiers;
    ast->FindChild([&specifiers](ark::es2panda::ir::AstNode *n) {
        if (n->IsImportSpecifier()) {
            specifiers.push_back(n);
        }
        return false;
    });
    ASSERT_GE(specifiers.size(), 2U);

    const size_t anchorStart = ChangeTracker::ToEditBoundary(specifiers[0])->Start().index;

    auto tracker = MakeTracker();
    tracker.InsertNodeBefore(ctx, specifiers[0], specifiers[1], false);
    tracker.InsertNodeBefore(ctx, specifiers[1], specifiers[0], true);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 2U);
    const auto *first = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(first->sourceFile, sourceFile);
    EXPECT_EQ(first->node, specifiers[1]);
    EXPECT_EQ(first->range.pos, anchorStart);
    EXPECT_EQ(first->range.end, anchorStart);
    ASSERT_TRUE(first->options.has_value());
    EXPECT_FALSE(first->options->prefix.has_value());
    EXPECT_EQ(first->options->suffix, "\n");

    const auto *second = std::get_if<ReplaceWithSingleNode>(&changes[1]);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->sourceFile, sourceFile);
    EXPECT_EQ(second->node, specifiers[0]);
    EXPECT_EQ(second->range.pos, anchorStart);
    EXPECT_EQ(second->range.end, anchorStart);
    ASSERT_TRUE(second->options.has_value());
    EXPECT_FALSE(second->options->prefix.has_value());
    EXPECT_EQ(second->options->suffix, "\n\n");
    initializer.DestroyContext(ctx);
}

// P1C-PG-005 BLOCKED (NeedSemicolonBetween TRUE chain, change_tracker.cpp:294-295 and
// 300-306): the chain requires a TSPropertySignature or TSParameterProperty node as
// `after`, but the ETS parser never produces either kind (interface property members
// `x: number` are lowered to getter MethodDefinition nodes; constructor parameter
// properties become ETSParameterExpression with modifier flags). Both public callers
// (InsertNodeAfter and InsertNodeAfterComma pass their `after`/comma-anchor node) are
// therefore limited to node kinds that can only come from TS-flavoured parses, which
// the LSP context factory cannot create (fixed config, lsp/src/internal_api.cpp:131-132).
// Only the null-guard arm (change_tracker.cpp:291-292, covered by
// InsertNodeAfter_NullNewNode_NullGuardArm below) and the FALSE outcome of the chain
// are reachable from real contexts.
// NEXT ACTION: product must expose a TS-flavoured context route (or real ETS nodes of
// these kinds) before the semicolon-replacement arm can be covered.

// P1C-PG-005 arm sweep: NeedSemicolonBetween null-newNode arm
// (change_tracker.cpp:291-292) driven through the public InsertNodeAfter with nullptr.
// The recorded ReplaceWithSingleNode keeps node == nullptr; GetChangeList() is safe,
// but GetChanges() must NOT be called here because GetTextChangesFromChanges
// (change_tracker.cpp:876) would dereference the null node.
TEST_F(LspChangeTrackerInsertAtStartTests, InsertNodeAfter_NullNewNode_NullGuardArm)
{
    const std::string source = R"(
function test() {
    let a = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("insertAfterNull.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *sourceFile = context->sourceFile;
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *aDecl = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsVariableDeclaration() &&
               n->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "a";
    });
    ASSERT_NE(aDecl, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertNodeAfter(ctx, aDecl, nullptr);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    EXPECT_EQ(replace->node, nullptr);
    EXPECT_EQ(replace->range.pos, aDecl->End().index);
    EXPECT_EQ(replace->range.end, aDecl->End().index);
    ASSERT_TRUE(replace->options.has_value());
    EXPECT_EQ(replace->options->prefix, ", ");
    initializer.DestroyContext(ctx);
}

// P1C-PG-005 arm sweep: InsertNodeBefore on a re-export module specifier.
// MEASURED: the ETS re-export `export { C } from "./m";` parses to
// ETSReExportDeclaration, and the "./m" StringLiteral's Parent() is the
// ETSReExportDeclaration itself (verified: Type() == REEXPORT_STATEMENT), NOT the
// ImportDeclaration. GetOptionsForInsertNodeBefore therefore evaluates the
// change_tracker.cpp:229 StringLiteral guard as FALSE and falls through every arm, so no
// prefix/suffix is set. ToEditBoundary ascends through the re-export to the Program root
// (parent == nullptr arm, change_tracker.cpp:81-83), so the zero-length edit lands at the
// program start. This pins the exact fallback path and covers the ToEditBoundary
// parent-null arm.
// BLOCKED siblings: the change_tracker.cpp:229 TRUE arms need either a parentless
// StringLiteral under a TS ImportDeclaration (the ETS parser parents import/re-export
// strings to ETSReExportDeclaration/ETSImportDeclaration, and side-effect imports are
// dropped) or a bare ir::NamedType (AS parser only); the EXPORT_SPECIFIER switch case
// (change_tracker.cpp:335-336) is also unreachable from ETS — named exports
// `export { C };` produce no AST node and re-exports carry an ImportSpecifier.
TEST_F(LspChangeTrackerInsertAtStartTests, InsertNodeBefore_ReExportStringLiteral_FallsThroughOptions)
{
    const std::string source = R"(
class C {}
export { C } from "./m";
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("reExportStringLiteral.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *sourceFile = context->sourceFile;
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *strLiteral = FindNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsStringLiteral(); });
    ASSERT_NE(strLiteral, nullptr);
    ASSERT_TRUE(strLiteral->Parent() != nullptr && strLiteral->Parent()->IsETSReExportDeclaration());
    auto *reExportSpecifier = FindNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsImportSpecifier(); });
    ASSERT_NE(reExportSpecifier, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertNodeBefore(ctx, strLiteral, reExportSpecifier, false);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    EXPECT_EQ(replace->node, reExportSpecifier);
    // ToEditBoundary ascends to the Program root (parent == nullptr arm,
    // change_tracker.cpp:81-83), so the zero-length edit lands at the program start.
    EXPECT_EQ(replace->range.pos, 0U);
    EXPECT_EQ(replace->range.end, 0U);
    ASSERT_TRUE(replace->options.has_value());
    EXPECT_FALSE(replace->options->prefix.has_value());
    EXPECT_FALSE(replace->options->suffix.has_value());
    initializer.DestroyContext(ctx);
}

// P1C-PG-005 arm sweep: InsertNodeAtConstructorStart (change_tracker.cpp:626-657)
// empty-body arm — firstStatement == nullptr && statements.empty() leads to
// ReplaceConstructorBody with the new statement (change_tracker.cpp:649-652), which
// replaces the constructor body node. Null-guard arms (change_tracker.cpp:629-631)
// return silently. The recorded range is ToEditBoundary(body): the walk from the
// constructor body ascends through the ScriptFunction and FunctionExpression and stops
// at the constructor MethodDefinition, which is an edit boundary
// (change_tracker.cpp:37-55, METHOD_DEFINITION), so the replaced range is the
// constructor method range.
TEST_F(LspChangeTrackerInsertAtStartTests, InsertNodeAtConstructorStart_EmptyBody_ReplacesBody)
{
    const std::string source = R"(
function helper(): void {
    let a = 1;
}
class C {
    constructor() {}
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ctorStartEmpty.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *sourceFile = context->sourceFile;
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *aDecl = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsVariableDeclaration() &&
               n->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "a";
    });
    ASSERT_NE(aDecl, nullptr);
    auto *newStmt = aDecl->Clone(context->allocator, nullptr)->AsStatement();
    ASSERT_NE(newStmt, nullptr);

    auto *ctorBody = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsBlockStatement() && n->Parent() != nullptr && n->Parent()->IsConstructor();
    });
    ASSERT_NE(ctorBody, nullptr);

    // First edit-boundary ancestor of the body (METHOD_DEFINITION, change_tracker.cpp:41).
    ark::es2panda::ir::AstNode *ctorMethod = ctorBody;
    while (ctorMethod != nullptr && !ctorMethod->IsMethodDefinition()) {
        ctorMethod = ctorMethod->Parent();
    }
    ASSERT_NE(ctorMethod, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertNodeAtConstructorStart(ctx, nullptr, newStmt);
    tracker.InsertNodeAtConstructorStart(ctx, ctorBody, nullptr);
    EXPECT_TRUE(tracker.GetChangeList().empty());

    tracker.InsertNodeAtConstructorStart(ctx, ctorBody, newStmt);
    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    ASSERT_NE(replace->node, nullptr);
    EXPECT_TRUE(replace->node->IsBlockStatement());
    EXPECT_EQ(replace->range.pos, ctorMethod->Start().index);
    EXPECT_EQ(replace->range.end, ctorMethod->End().index);
    initializer.DestroyContext(ctx);
}

}  // namespace
