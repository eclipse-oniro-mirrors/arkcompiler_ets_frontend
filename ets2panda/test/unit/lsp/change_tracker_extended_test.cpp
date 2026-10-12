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
#include <string>
#include <vector>
#include <variant>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp/include/services/text_change/text_change_context.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::ChangeKind;
using ark::es2panda::lsp::ChangeText;
using ark::es2panda::lsp::ChangeTracker;
using ark::es2panda::lsp::FormatCodeSettings;
using ark::es2panda::lsp::GetFormatContext;
using ark::es2panda::lsp::InsertNodeOptions;
using ark::es2panda::lsp::NewFile;
using ark::es2panda::lsp::RemoveNode;
using ark::es2panda::lsp::ReplaceWithMultipleNodes;
using ark::es2panda::lsp::ReplaceWithSingleNode;

class LspChangeTrackerExtendedTests : public LSPAPITests {
protected:
    ChangeTracker MakeTracker()
    {
        FormatCodeSettings settings;
        auto formatContext = GetFormatContext(settings);
        TextChangesContext changeText {{}, formatContext, {}};
        return ChangeTracker::FromContext(changeText);
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

// P1B-CT-02: InsertNodeAfter
// Unique anchor coverage: change_tracker_test_2.cpp exercises InsertNodeAfter only on
// interface-method/class-property anchors; this pins a function-body statement anchor.
TEST_F(LspChangeTrackerExtendedTests, InsertNodeAfter)
{
    const std::string source = R"(
function test() {
    let a = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("insertNodeAfter.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *sourceFile = context->sourceFile;

    auto *aDecl = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsVariableDeclaration() &&
               n->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "a";
    });
    ASSERT_NE(aDecl, nullptr);

    auto *clone = aDecl->Clone(context->allocator, nullptr);
    ASSERT_NE(clone, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertNodeAfter(ctx, aDecl, clone);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    EXPECT_NE(replace->node, nullptr);
    // InsertNodeAfterWorker: ToEditBoundary(aDecl) returns aDecl (VariableDeclaration inside BlockStatement)
    // -> anchor = aDecl, returns aDecl->End().index. InsertNodeAt creates zero-length range at that position.
    // GetInsertNodeAfterOptions for VARIABLE_DECLARATION returns prefix = ", ".
    EXPECT_EQ(replace->range.pos, aDecl->End().index);
    EXPECT_EQ(replace->range.end, aDecl->End().index);
    EXPECT_TRUE(replace->options.has_value());
    if (replace->options.has_value()) {
        EXPECT_EQ(replace->options->prefix, ", ");
    }
    initializer.DestroyContext(ctx);
}

// P1B-CT-04: InsertNodeAtConstructorEnd
// Only direct coverage of the insert-at-constructor-end position; the weaker twin in
// change_tracker_remaining_test.cpp was removed as a semantic duplicate.
TEST_F(LspChangeTrackerExtendedTests, InsertNodeAtConstructorEnd)
{
    const std::string source = R"(
class MyClass {
    constructor() {
        let x = 1;
    }
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("insertNodeAtConstructorEnd.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    ark::es2panda::ir::AstNode *ctrNode = nullptr;
    ast->FindChild([&ctrNode](ark::es2panda::ir::AstNode *n) {
        if (n->IsConstructor()) {
            ctrNode = n;
        }
        return false;
    });
    ASSERT_NE(ctrNode, nullptr);

    // Find the last statement in the constructor body by navigating to the BlockStatement body
    ark::es2panda::ir::AstNode *lastStmt = nullptr;
    ctrNode->FindChild([&lastStmt](ark::es2panda::ir::AstNode *n) {
        if (n->IsBlockStatement()) {
            auto *block = n->AsBlockStatement();
            if (!block->Statements().empty()) {
                lastStmt = block->Statements().back();
            }
        }
        return false;
    });
    ASSERT_NE(lastStmt, nullptr);
    auto *newStmt = lastStmt->Clone(context->allocator, nullptr)->AsStatement();
    ASSERT_NE(newStmt, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertNodeAtConstructorEnd(ctx, ctrNode, newStmt);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    // InsertNodeAtConstructorEnd -> InsertNodeAfter(lastStatement, newStatement)
    // -> InsertNodeAfterWorker: ToEditBoundary(lastStmt) returns lastStmt
    // -> endPosition = lastStmt->End().index -> zero-length range at lastStmt->End().index
    EXPECT_EQ(replace->range.pos, lastStmt->End().index);
    EXPECT_EQ(replace->range.end, lastStmt->End().index);
    initializer.DestroyContext(ctx);
}

// P1B-CT-05: InsertMemberAtStart
// BLOCKED_DESIGN_MAPPING
// Source: ets2panda/lsp/src/services/text_change/change_tracker.cpp:257-269, 271-287, 694-705
// GetInsertNodeAtStartInsertOptions() unconditionally evaluates
//   classesWithNodesInsertedAtStart_.at(0).node == node   (change_tracker.cpp:261)
// but the map std::map<int, ClassInsertInfo> classesWithNodesInsertedAtStart_
// (change_tracker.h:155) has NO write path anywhere in the codebase — there is no
// emplace/insert/operator[] on it, and the getter (change_tracker.h:172) returns a
// copy. Any call to InsertNodeAtStartWorker therefore throws std::out_of_range from
// std::map::at on an empty map. Additionally, GetMembersOrProperties() for a class
// node only collects IsMemberExpression()/IsTSEnumMember() children
// (change_tracker.cpp:246-252), which do not match ClassProperty/MethodDefinition
// class members, so even the membersOrProperties.at(size-1) at line 284 is wrong.
// Exact run evidence: test throws std::out_of_range before producing any change.
// NEXT ACTION: Product must populate the map (or guard against empty) before this
// can be tested; also GetMembersOrProperties must handle class members.
// No TEST_F written for this item.

// P1B-CT-06: InsertNodeAtObjectStart
// BLOCKED_DESIGN_MAPPING
// Source: ets2panda/lsp/src/services/text_change/change_tracker.cpp:707-711
//          → InsertNodeAtStartWorker (change_tracker.cpp:271-287)
//          → GetInsertNodeAtStartInsertOptions (change_tracker.cpp:257-269)
//            → classesWithNodesInsertedAtStart_.at(0)  (line 261)
// This is the same product defect as P1B-CT-05: the private map
// std::map<int, ClassInsertInfo> classesWithNodesInsertedAtStart_
// (change_tracker.h:155) has no write path anywhere in the codebase.
// std::map::at on an empty map throws std::out_of_range at runtime.
// Even with a real SpreadElement from source (as attempted in the previous
// test draft), the exception is unavoidable without a product code change.
// Exact run evidence: std::out_of_range before any change is produced.
// NEXT ACTION: Product must populate the map (or guard against empty map
// access) before this test can be written.
// No TEST_F written for this item.

// P1B-CT-07: InsertNodeAfterComma
// Only direct coverage of InsertNodeAfterComma's comma-prefix insertion arm.
TEST_F(LspChangeTrackerExtendedTests, InsertNodeAfterComma)
{
    const std::string source = R"(
let obj = {
    foo: 1,
    bar: 2
};
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("insertNodeAfterComma.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *sourceFile = context->sourceFile;

    auto *fooProp = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsProperty() && n->AsProperty()->Key()->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(fooProp, nullptr);

    auto *clone = fooProp->Clone(context->allocator, nullptr);
    ASSERT_NE(clone, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertNodeAfterComma(ctx, fooProp, clone);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    // InsertNodeAfterComma: NextCommaToken finds first descendant after node->Start().index,
    // InsertNodeAfterWorker -> ToEditBoundary walks up to the Property (edit boundary)
    // -> returns fooProp->End().index. GetInsertNodeAfterOptions for PROPERTY returns suffix = "," + NL.
    EXPECT_EQ(replace->range.pos, fooProp->End().index);
    EXPECT_EQ(replace->range.end, fooProp->End().index);
    EXPECT_TRUE(replace->options.has_value());
    if (replace->options.has_value()) {
        EXPECT_EQ(replace->options->suffix,
                  "," + ark::es2panda::lsp::GetDefaultFormatCodeSettings().GetNewLineCharacter());
    }
    initializer.DestroyContext(ctx);
}

// P1B-CT-12: InsertNodeAtTopOfFile
// Upstream reaches InsertNodeAtTopOfFile only inside unasserted batch tests;
// this pins the exact insertion range (intentional overlap, deeper assertions).
TEST_F(LspChangeTrackerExtendedTests, InsertNodeAtTopOfFile)
{
    const std::string source = R"(
function foo() {}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("insertNodeAtTopOfFile.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *sourceFile = context->sourceFile;

    auto *funcExpr = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsFunctionExpression() && n->AsFunctionExpression()->Function()->Id() != nullptr &&
               n->AsFunctionExpression()->Function()->Id()->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(funcExpr, nullptr);

    auto *clone = funcExpr->Clone(context->allocator, nullptr);
    ASSERT_NE(clone, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertNodeAtTopOfFile(ctx, clone, false);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    // InsertNodeAtTopOfFile -> InsertAtTopOfFile -> GetInsertionPositionAtSourceFileTop:
    // position is the Start().index of the first ClassDeclaration found via FindChild.
    // For source "\nfunction foo() {}\n", the ETS compiler wraps top-level statements in an
    // implicit global class (ClassDeclaration). FindChild with IsClassDeclaration() locates
    // this global class, whose Start().index is 0 (beginning of file).
    auto *firstClass = ast->FindChild([](ark::es2panda::ir::AstNode *child) { return child->IsClassDeclaration(); });
    ASSERT_NE(firstClass, nullptr);
    EXPECT_EQ(replace->range.pos, firstClass->Start().index);
    EXPECT_EQ(replace->range.pos, replace->range.end);  // zero-length insert
    initializer.DestroyContext(ctx);
}

// P1B-CT-15: InsertModifierAt
// Upstream batch tests call InsertModifierAt/InsertModifierBefore without asserting;
// here the inserted modifier text and its zero-length range are pinned.
TEST_F(LspChangeTrackerExtendedTests, InsertModifierAt)
{
    const std::string source = R"(
function foo() {}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("insertModifierAt.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *sourceFile = context->sourceFile;

    // Find the function expression
    auto *funcExpr = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsFunctionExpression() && n->AsFunctionExpression()->Function()->Id() != nullptr &&
               n->AsFunctionExpression()->Function()->Id()->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(funcExpr, nullptr);

    // Clone the function expression to use as a "modifier" node
    // (InsertModifierAt accepts any AstNode as modifier)
    auto *clone = funcExpr->Clone(context->allocator, nullptr);
    ASSERT_NE(clone, nullptr);

    InsertNodeOptions options;
    options.prefix = "public ";

    auto tracker = MakeTracker();
    tracker.InsertModifierAt(ctx, funcExpr->Start().index, clone, options);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    EXPECT_TRUE(replace->options.has_value());
    // InsertModifierAt -> InsertNodeAt(pos) -> zero-length range at the passed position.
    EXPECT_EQ(replace->range.pos, funcExpr->Start().index);
    EXPECT_EQ(replace->range.end, funcExpr->Start().index);
    if (replace->options.has_value()) {
        EXPECT_EQ(replace->options->prefix, "public ");
    }
    initializer.DestroyContext(ctx);
}

// P1B-CT-16: InsertModifierBefore
TEST_F(LspChangeTrackerExtendedTests, InsertModifierBefore)
{
    const std::string source = R"(
function foo() {}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("insertModifierBefore.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *sourceFile = context->sourceFile;

    // Find the function expression
    auto *funcExpr = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsFunctionExpression() && n->AsFunctionExpression()->Function()->Id() != nullptr &&
               n->AsFunctionExpression()->Function()->Id()->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(funcExpr, nullptr);

    auto *clone = funcExpr->Clone(context->allocator, nullptr);
    ASSERT_NE(clone, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertModifierBefore(ctx, clone, funcExpr);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    // InsertModifierBefore -> InsertModifierAt(before->Start().index) -> zero-length range.
    EXPECT_EQ(replace->range.pos, funcExpr->Start().index);
    EXPECT_EQ(replace->range.end, funcExpr->Start().index);
    initializer.DestroyContext(ctx);
}

// P1B-CT-17: ReplaceNodeRangeWithNodes
// Upstream ReplaceHelpers/DeleteHelpers batch tests invoke these four APIs without
// range assertions; these tests pin the exact edit ranges (intentional overlap).
TEST_F(LspChangeTrackerExtendedTests, ReplaceNodeRangeWithNodes)
{
    const std::string source = R"(
function test() {
    let a = 1;
    let b = 2;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("replaceNodeRangeWithNodes.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *sourceFile = context->sourceFile;

    auto *aDecl = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsVariableDeclaration() &&
               n->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "a";
    });
    auto *bDecl = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsVariableDeclaration() &&
               n->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "b";
    });
    ASSERT_NE(aDecl, nullptr);
    ASSERT_NE(bDecl, nullptr);

    auto *clone = aDecl->Clone(context->allocator, nullptr);
    ASSERT_NE(clone, nullptr);
    std::vector<ark::es2panda::ir::AstNode *> newNodes = {clone};

    auto tracker = MakeTracker();
    tracker.ReplaceNodeRangeWithNodes(ctx, aDecl, bDecl, newNodes);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *multi = std::get_if<ReplaceWithMultipleNodes>(&changes[0]);
    ASSERT_NE(multi, nullptr);
    EXPECT_EQ(multi->kind, ChangeKind::REPLACEWITHMULTIPLENODES);
    EXPECT_EQ(multi->sourceFile, sourceFile);
    EXPECT_EQ(multi->nodes.size(), 1U);
    EXPECT_EQ(multi->range.pos, aDecl->Start().index);
    EXPECT_EQ(multi->range.end, bDecl->End().index);
    initializer.DestroyContext(ctx);
}

// P1B-CT-18: ReplaceNodeWithNodes
TEST_F(LspChangeTrackerExtendedTests, ReplaceNodeWithNodes)
{
    const std::string source = R"(
function test() {
    let a = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("replaceNodeWithNodes.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *sourceFile = context->sourceFile;

    auto *aDecl = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsVariableDeclaration() &&
               n->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "a";
    });
    ASSERT_NE(aDecl, nullptr);

    auto *clone1 = aDecl->Clone(context->allocator, nullptr);
    auto *clone2 = aDecl->Clone(context->allocator, nullptr);
    ASSERT_NE(clone1, nullptr);
    ASSERT_NE(clone2, nullptr);
    std::vector<ark::es2panda::ir::AstNode *> newNodes = {clone1, clone2};

    auto tracker = MakeTracker();
    tracker.ReplaceNodeWithNodes(ctx, aDecl, newNodes);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *multi = std::get_if<ReplaceWithMultipleNodes>(&changes[0]);
    ASSERT_NE(multi, nullptr);
    EXPECT_EQ(multi->kind, ChangeKind::REPLACEWITHMULTIPLENODES);
    EXPECT_EQ(multi->sourceFile, sourceFile);
    EXPECT_EQ(multi->nodes.size(), 2U);
    EXPECT_EQ(multi->range.pos, aDecl->Start().index);
    EXPECT_EQ(multi->range.end, aDecl->End().index);
    initializer.DestroyContext(ctx);
}

// P1B-CT-19: ReplaceRangeWithText
TEST_F(LspChangeTrackerExtendedTests, ReplaceRangeWithText)
{
    const std::string source = "function test() { let a = 1; }";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("replaceRangeWithText.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *sourceFile = context->sourceFile;

    constexpr size_t startPos = 0;
    constexpr size_t endPos = 8;  // "function"
    ark::es2panda::lsp::TextRange range = {startPos, endPos};
    const std::string newText = "// replaced";

    auto tracker = MakeTracker();
    tracker.ReplaceRangeWithText(sourceFile, range, newText);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *change = std::get_if<ChangeText>(&changes[0]);
    ASSERT_NE(change, nullptr);
    EXPECT_EQ(change->kind, ChangeKind::TEXT);
    EXPECT_EQ(change->sourceFile, sourceFile);
    EXPECT_EQ(change->range.pos, startPos);
    EXPECT_EQ(change->range.end, endPos);
    EXPECT_EQ(change->text, newText);
    initializer.DestroyContext(ctx);
}

// P1B-CT-20: DeleteModifier
// BLOCKED_DESIGN_MAPPING
// Source: ets2panda/lsp/include/services/text_change/change_tracker.h:189
// ets2panda/lsp/src/services/text_change/change_tracker.cpp:430-435
// The function accepts a context and an AST node representing the modifier. It
// deletes the byte range spanning the node from start to end.
// The design expects deleting a modifier keyword (export/static/final) only. In the
// ETS AST these modifiers are NOT independent AstNode children with their own
// ranges: they are ModifierFlags stored on the declaration node. There is no
// modifier AstNode to pass. Passing the whole declaration (as the previous test
// draft did) would delete the entire declaration, which is semantically wrong and
// is NOT the design purpose.
// NEXT ACTION: The product must expose a modifier keyword range (or the design must
// be changed) before this test can be written.
// No TEST_F written for this item.

// P1B-CT-21: DeleteNodeRangeExcludingEnd
TEST_F(LspChangeTrackerExtendedTests, DeleteNodeRangeExcludingEnd)
{
    const std::string source = R"(
function test() {
    let a = 1;
    let b = 2;
    let c = 3;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("deleteNodeRangeExcludingEnd.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *sourceFile = context->sourceFile;

    auto *aDecl = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsVariableDeclaration() &&
               n->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "a";
    });
    auto *cDecl = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsVariableDeclaration() &&
               n->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "c";
    });
    ASSERT_NE(aDecl, nullptr);
    ASSERT_NE(cDecl, nullptr);

    auto tracker = MakeTracker();
    tracker.DeleteNodeRangeExcludingEnd(ctx, aDecl, cDecl);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *remNode = std::get_if<RemoveNode>(&changes[0]);
    ASSERT_NE(remNode, nullptr);
    EXPECT_EQ(remNode->kind, ChangeKind::REMOVE);
    EXPECT_EQ(remNode->sourceFile, sourceFile);
    // DeleteNodeRangeExcludingEnd calls
    // DeleteRange(sourceFile, GetAdjustedRange(context, startNode, afterEndNode))
    // where GetAdjustedRange -> MakeEditRange(startNode, afterEndNode)
    // (change_tracker.cpp:437-444, 412-415, 101-108).
    // Both nodes are VariableDeclarations inside a BlockStatement, so
    // ToEditBoundary returns them unchanged (change_tracker.cpp:71-73).
    // The resulting range is exactly {aDecl->Start().index, cDecl->End().index}.
    EXPECT_EQ(remNode->range.pos, aDecl->Start().index);
    EXPECT_EQ(remNode->range.end, cDecl->End().index);
    initializer.DestroyContext(ctx);
}

}  // namespace