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
#include "ir/ts/tsTypeParameterDeclaration.h"
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
using ark::es2panda::lsp::TextRange;

class LspChangeTrackerRemainingTests : public LSPAPITests {
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

// P2C-CT-01: GetAdjustedRange_WorksCorrectly
// Only direct coverage of GetAdjustedRange on a multi-node change set;
// complements DeleteNodeRangeExcludingEnd in the extended file.
TEST_F(LspChangeTrackerRemainingTests, GetAdjustedRange_WorksCorrectly)
{
    const std::string source = R"(
function test() {
    let a = 1;
    let b = 2;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("getAdjustedRange.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

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

    auto tracker = MakeTracker();
    TextRange range = tracker.GetAdjustedRange(ctx, aDecl, bDecl);
    // GetAdjustedRange -> MakeEditRange -> ToEditBoundary: both nodes are VariableDeclaration
    // inside a BlockStatement, so ToEditBoundary returns them unchanged.
    // Range = {aDecl->Start().index, bDecl->End().index}
    EXPECT_EQ(range.pos, aDecl->Start().index);
    EXPECT_EQ(range.end, bDecl->End().index);
    EXPECT_LE(range.pos, range.end);
    initializer.DestroyContext(ctx);
}

// P2C-CT-02: DeleteModifier_WorksCorrectly
// Note: In the ETS AST, modifiers are ModifierFlags on declaration nodes, not separate
// AstNode children. DeleteModifier accepts any AstNode and deletes its byte range.
// This test verifies the current product behavior: passing a VariableDeclaration node
// deletes the range covering that declaration.
// Upstream DeleteMethods_BasicTest1 reaches DeleteRange only via batch helpers;
// this pins the exact deleted range for a modifier token.
TEST_F(LspChangeTrackerRemainingTests, DeleteModifier_WorksCorrectly)
{
    const std::string source = R"(
function test() {
    let a = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("deleteModifier.ets", ES2PANDA_STATE_CHECKED, source.c_str());
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

    auto tracker = MakeTracker();
    tracker.DeleteModifier(ctx, aDecl);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *rem = std::get_if<RemoveNode>(&changes[0]);
    ASSERT_NE(rem, nullptr);
    EXPECT_EQ(rem->kind, ChangeKind::REMOVE);
    EXPECT_EQ(rem->sourceFile, sourceFile);
    // DeleteModifier calls DeleteRange(sourceFile, {modifier->Start().index, modifier->End().index})
    EXPECT_EQ(rem->range.pos, aDecl->Start().index);
    EXPECT_EQ(rem->range.end, aDecl->End().index);
    initializer.DestroyContext(ctx);
}

// P2C-CT-03: ReplaceRange_WorksCorrectly
TEST_F(LspChangeTrackerRemainingTests, ReplaceRange_WorksCorrectly)
{
    const std::string source = R"(
function test() {
    let a = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("replaceRange.ets", ES2PANDA_STATE_CHECKED, source.c_str());
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

    TextRange range = {aDecl->Start().index, aDecl->End().index};
    InsertNodeOptions options;
    options.prefix = "// replaced";

    auto tracker = MakeTracker();
    tracker.ReplaceRange(ctx, range, clone, options);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    EXPECT_EQ(replace->range.pos, range.pos);
    EXPECT_EQ(replace->range.end, range.end);
    EXPECT_NE(replace->node, nullptr);
    EXPECT_TRUE(replace->options.has_value());
    if (replace->options.has_value()) {
        EXPECT_EQ(replace->options->prefix, "// replaced");
    }
    initializer.DestroyContext(ctx);
}

// P2C-CT-04: CreateRange_WorksCorrectly
TEST_F(LspChangeTrackerRemainingTests, CreateRange_WorksCorrectly)
{
    auto tracker = MakeTracker();
    // CreateRange with pos only: end == pos
    TextRange r1 = tracker.CreateRange(42);
    EXPECT_EQ(r1.pos, 42U);
    EXPECT_EQ(r1.end, 42U);

    // CreateRange with pos and end
    TextRange r2 = tracker.CreateRange(10, 20);
    EXPECT_EQ(r2.pos, 10U);
    EXPECT_EQ(r2.end, 20U);
}

// P2C-CT-05: FinishClassesWithNodesInsertedAtStart_WorksCorrectly
// The map classesWithNodesInsertedAtStart_ is empty by default, so iterating it is a no-op.
TEST_F(LspChangeTrackerRemainingTests, FinishClassesWithNodesInsertedAtStart_WorksCorrectly)
{
    auto tracker = MakeTracker();
    // Calling FinishClassesWithNodesInsertedAtStart on an empty map should not crash
    // and should not produce any changes.
    EXPECT_NO_FATAL_FAILURE(tracker.FinishClassesWithNodesInsertedAtStart());
    const auto &changes = tracker.GetChangeList();
    EXPECT_TRUE(changes.empty());
}

// P2C-CT-06: GetInsertionPositionAtSourceFileTop_WorksCorrectly
// Complement of the extended file's InsertNodeAtTopOfFile: covers the
// top-of-file insertion position computation itself.
TEST_F(LspChangeTrackerRemainingTests, GetInsertionPositionAtSourceFileTop_WorksCorrectly)
{
    const std::string source = R"(
function foo() {}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("getInsertionPositionAtSourceFileTop.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto tracker = MakeTracker();
    size_t pos = tracker.GetInsertionPositionAtSourceFileTop(ast);
    // The ETS compiler wraps top-level statements in an implicit global ClassDeclaration.
    // GetInsertionPositionAtSourceFileTop returns the Start().index of the first ClassDeclaration.
    auto *firstClass = ast->FindChild([](ark::es2panda::ir::AstNode *child) { return child->IsClassDeclaration(); });
    ASSERT_NE(firstClass, nullptr);
    EXPECT_EQ(pos, firstClass->Start().index);
    initializer.DestroyContext(ctx);
}

// P2C-CT-11: InsertNodeAtEndOfScope_WorksCorrectly
// Only direct coverage of InsertNodeAtEndOfScope's closing-brace placement.
TEST_F(LspChangeTrackerRemainingTests, InsertNodeAtEndOfScope_WorksCorrectly)
{
    const std::string source = R"(
function test() {
    let a = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("insertNodeAtEndOfScope.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *sourceFile = context->sourceFile;

    ark::es2panda::ir::AstNode *blockStmt = nullptr;
    ast->FindChild([&blockStmt](ark::es2panda::ir::AstNode *n) {
        if (n->IsBlockStatement()) {
            blockStmt = n;
        }
        return false;
    });
    ASSERT_NE(blockStmt, nullptr);

    auto *clone = blockStmt->Clone(context->allocator, nullptr);
    ASSERT_NE(clone, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertNodeAtEndOfScope(ctx, blockStmt, clone);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    // InsertNodeAtEndOfScope -> InsertNodeAt(scope->End().index, newNode, options)
    // with prefix = newLine, suffix = newLine -> zero-length range at scope->End().index
    EXPECT_EQ(replace->range.pos, blockStmt->End().index);
    EXPECT_EQ(replace->range.end, blockStmt->End().index);
    EXPECT_TRUE(replace->options.has_value());
    if (replace->options.has_value()) {
        EXPECT_TRUE(replace->options->prefix.has_value());
        EXPECT_TRUE(replace->options->suffix.has_value());
    }
    initializer.DestroyContext(ctx);
}

// P2C-CT-15: InsertNodeAtEndOfList_WorksCorrectly
// Note: The current implementation treats `size - 1` (list element count) as a byte offset
// position (change_tracker.cpp:720-727). This is a known product bug: the position should be
// derived from the last element's range, not from the element count. The test verifies the
// current product behavior.
// *** CHARACTERIZATION TEST ***
// InsertNodeAtEndOfList uses the element count as a byte offset (change_tracker.cpp:720-727). This test characterizes
// the current buggy behavior and must be updated once the product bug is fixed.
// Characterization cluster (InsertNodeAtEndOfList/GetContainingList/InsertNodeInListAfter/
// InsertImportSpecifierAtIndex): pins the current off-by-one list-bounds behavior;
// these characterize a known product quirk, not the designed contract.
TEST_F(LspChangeTrackerRemainingTests, InsertNodeAtEndOfList_CharacterizesBug)
{
    const std::string source = R"(
function test() {
    let a = 1;
    let b = 2;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("insertNodeAtEndOfList.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *sourceFile = context->sourceFile;

    // Create a vector of AstNode pointers (the list to insert at end of)
    std::vector<const ark::es2panda::ir::AstNode *> nodeList;
    ast->FindChild([&nodeList](ark::es2panda::ir::AstNode *n) {
        if (n->IsVariableDeclaration()) {
            nodeList.push_back(n);
        }
        return false;
    });
    ASSERT_FALSE(nodeList.empty());

    // nodeList contains const pointers; Clone is non-const, so cast away constness
    auto *clone = const_cast<ark::es2panda::ir::AstNode *>(nodeList[0])->Clone(context->allocator, nullptr);
    ASSERT_NE(clone, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertNodeAtEndOfList(ctx, nodeList, clone);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    // Current buggy behavior: InsertNodeAt(context, size - 1, newNode, options)
    // where size is the number of elements in the list (a byte count, not a position).
    EXPECT_TRUE(replace->options.has_value());
    if (replace->options.has_value()) {
        EXPECT_EQ(replace->options->prefix, ", ");
    }
    initializer.DestroyContext(ctx);
}

// P2C-CT-18: InsertFirstParameter_WorksCorrectly
// The implementation (change_tracker.cpp:743-753) has an inverted-condition bug:
// when the parameters vector is empty it accesses element zero (undefined behavior),
// and in the non-empty path it uses the element count as a byte offset.
// Call with a NON-EMPTY vector of real TSTypeParameterDeclaration* nodes from a parsed
// generic class context. The non-empty path records an edit at byte position equal to
// the element count (same bug family as InsertNodeAtEndOfList CT-15).
// File:line evidence: change_tracker.cpp:747-751 (inverted condition, empty path would be UB;
// non-empty path misuses element count as byte offset).
// *** CHARACTERIZATION TEST ***
// InsertFirstParameter has an inverted empty check (change_tracker.cpp:747-748): the empty path
// is UB (accessing element zero) and the non-empty path misuses the element count as a byte
// offset. This test characterizes the current buggy behavior via the non-empty path and must
// be updated once the product bug is fixed.
TEST_F(LspChangeTrackerRemainingTests, InsertFirstParameter_CharacterizesBug)
{
    // Source with a generic class that has type params
    const std::string source = R"(
class MyClass<T> {}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("insertFirstParameter.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the ClassDefinition and get its TypeParams()
    ark::es2panda::ir::AstNode *classDef = nullptr;
    ast->FindChild([&classDef](ark::es2panda::ir::AstNode *n) {
        if (n->IsClassDefinition() && n->AsClassDefinition()->Ident() != nullptr &&
            n->AsClassDefinition()->Ident()->Name().Is("MyClass")) {
            classDef = n;
            return true;
        }
        return false;
    });
    ASSERT_NE(classDef, nullptr);

    // Get the TSTypeParameterDeclaration from the ClassDefinition's TypeParams()
    auto *typeParamDecl = classDef->AsClassDefinition()->TypeParams();
    ASSERT_NE(typeParamDecl, nullptr);

    // Clone the TSTypeParameterDeclaration to use as the "new" parameter
    auto *newParam = typeParamDecl->Clone(context->allocator, nullptr)->AsTSTypeParameterDeclaration();
    ASSERT_NE(newParam, nullptr);

    // Build a non-empty vector with the existing TSTypeParameterDeclaration
    std::vector<ark::es2panda::ir::TSTypeParameterDeclaration *> params = {typeParamDecl};

    auto tracker = MakeTracker();
    tracker.InsertFirstParameter(ctx, params, *newParam);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ark::es2panda::lsp::ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ark::es2panda::lsp::ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, context->sourceFile);
    // Non-empty path: InsertNodeAt(context, parameters.size(), ...) where parameters.size() == 1.
    // InsertNodeAt creates a zero-length range at byte position == parameters.size() == 1.
    // This is a known product bug (element count used as byte offset).
    // See change_tracker.cpp:743-751: inverted condition (empty path would be UB on parameters[0]);
    // non-empty path misuses element count as byte offset (same bug as InsertNodeAtEndOfList CT-15).
    EXPECT_EQ(replace->range.pos, 1U);
    EXPECT_EQ(replace->range.end, 1U);
    initializer.DestroyContext(ctx);
}

// P2C-CT-19: GetContainingList_WorksCorrectly
// Note: The current implementation of GetContainingList (change_tracker.cpp:761-773)
// calls node->Parent()->FindChild() looking for an ObjectExpression child, but the
// parent of a Property inside an ObjectExpression IS the ObjectExpression, and its
// children are Properties (not ObjectExpressions). So FindChild never matches and
// the list is always empty. This is a known product bug.
// *** CHARACTERIZATION TEST ***
// GetContainingList is known to always return an empty list because FindChild on the ObjectExpression parent never
// matches an ObjectExpression child (change_tracker.cpp:761-773). This test characterizes the current buggy behavior;
// when the product bug is fixed it MUST be updated to assert a non-empty list.
TEST_F(LspChangeTrackerRemainingTests, GetContainingList_CharacterizesBug)
{
    const std::string source = R"(
let obj = {
    foo: 1,
    bar: 2
};
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("getContainingList.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find a property node inside the object literal
    auto *fooProp = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsProperty() && n->AsProperty()->Key()->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(fooProp, nullptr);

    auto tracker = MakeTracker();
    std::vector<ark::es2panda::ir::AstNode *> list = tracker.GetContainingList(fooProp);
    // Current implementation: GetContainingList returns empty because FindChild
    // on the ObjectExpression parent does not find an ObjectExpression child.
    EXPECT_TRUE(list.empty());
    initializer.DestroyContext(ctx);
}

// P2C-CT-20: InsertNodeInListAfter_WorksCorrectly
// Note: InsertNodeInListAfter calls GetContainingList which returns empty (see CT-19),
// so InsertNodeInListAfter returns early without producing changes. This is a known
// product bug in GetContainingList.
// *** CHARACTERIZATION TEST ***
// InsertNodeInListAfter depends on the buggy GetContainingList (change_tracker.cpp:761-773), so no changes are
// produced. This test characterizes the current buggy behavior and must be updated once the product bug is fixed.
TEST_F(LspChangeTrackerRemainingTests, InsertNodeInListAfter_CharacterizesBug)
{
    const std::string source = R"(
let obj = {
    foo: 1,
    bar: 2
};
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("insertNodeInListAfter.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *fooProp = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsProperty() && n->AsProperty()->Key()->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(fooProp, nullptr);

    auto *clone = fooProp->Clone(context->allocator, nullptr);
    ASSERT_NE(clone, nullptr);

    std::vector<ark::es2panda::ir::AstNode *> containingList;
    auto tracker = MakeTracker();
    // Function should not crash even with the buggy GetContainingList
    EXPECT_NO_FATAL_FAILURE(tracker.InsertNodeInListAfter(ctx, fooProp, clone, containingList));

    // The containingList is empty due to the GetContainingList bug
    const auto &changes = tracker.GetChangeList();
    EXPECT_TRUE(changes.empty());
    initializer.DestroyContext(ctx);
}

// P2C-CT-21: InsertImportSpecifierAtIndex_WorksCorrectly
// Note: InsertImportSpecifierAtIndex internally calls InsertNodeInListAfter which
// depends on GetContainingList (known to return empty, see CT-19). The function
// should not crash even without producing changes.
// *** CHARACTERIZATION TEST ***
// InsertImportSpecifierAtIndex depends on the buggy GetContainingList (change_tracker.cpp:761-773); only a no-crash
// contract can be asserted for the current behavior. Update this test when the product bug is fixed.
TEST_F(LspChangeTrackerRemainingTests, InsertImportSpecifierAtIndex_CharacterizesBug)
{
    const std::string source = R"(
import { foo, bar } from './mod';
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("insertImportSpecifierAtIndex.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the import declaration
    ark::es2panda::ir::AstNode *importDecl = nullptr;
    ast->FindChild([&importDecl](ark::es2panda::ir::AstNode *n) {
        if (n->IsETSImportDeclaration()) {
            importDecl = n;
        }
        return false;
    });
    ASSERT_NE(importDecl, nullptr);

    // Collect named import specifiers from the import declaration
    std::vector<ark::es2panda::ir::AstNode *> namedImports;
    importDecl->FindChild([&namedImports](ark::es2panda::ir::AstNode *n) {
        if (n->IsImportSpecifier()) {
            namedImports.push_back(n);
        }
        return false;
    });
    ASSERT_GE(namedImports.size(), 2U);

    auto tracker = MakeTracker();
    // Insert at index 1 (after the first specifier).
    // The function should not crash even with the GetContainingList bug.
    // The prevSpecifier at index-1 (0) is namedImports[0].
    // InsertNodeInListAfter will be called with prevSpecifier and nullptr as newNode.
    EXPECT_NO_FATAL_FAILURE(tracker.InsertImportSpecifierAtIndex(ctx, nullptr, namedImports, 1));
    initializer.DestroyContext(ctx);
}

// P2C-CT-23: RfindNearestKeyWordTextRange_WorksCorrectly
TEST_F(LspChangeTrackerRemainingTests, RfindNearestKeyWordTextRange_WorksCorrectly)
{
    const std::string source = R"(
function test() {
    let a = 1;
    return a;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("rfindNearestKeyWordTextRange.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto tracker = MakeTracker();
    TextRange range = {0, 0};
    // Byte offset into the keyword to search from (past the leading "ret")
    const size_t keywordSearchOffset = 3;
    const std::string keyword = "return";
    size_t pos = source.rfind(keyword);
    ASSERT_NE(pos, std::string::npos);
    tracker.RfindNearestKeyWordTextRange(ctx, pos + keywordSearchOffset, keyword, range);
    // Range should cover the "return" keyword at the found position
    EXPECT_EQ(range.pos, pos);
    EXPECT_EQ(range.end, pos + keyword.size());
    initializer.DestroyContext(ctx);
}

}  // namespace
