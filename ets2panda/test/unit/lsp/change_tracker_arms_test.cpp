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

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
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

class LspChangeTrackerArmsTests : public LSPAPITests {
protected:
    ChangeTracker MakeTracker()
    {
        FormatCodeSettings settings;
        auto formatContext = GetFormatContext(settings);
        TextChangesContext changeText {{}, formatContext, {}};
        return ChangeTracker::FromContext(changeText);
    }

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

    ark::es2panda::ir::AstNode *FindPropertyByKey(ark::es2panda::ir::AstNode *root, const char *key)
    {
        return FindNode(root, [key](ark::es2panda::ir::AstNode *n) {
            return n->IsProperty() && n->AsProperty()->Key()->AsIdentifier()->Name() == key;
        });
    }
};

// P1C-PG-005 arm sweep: GetContainingList ObjectExpression arm
// (change_tracker.cpp:765-770): FindChild below the parent finds a NESTED object
// literal, so the inner properties are collected and the early-exit `return true`
// arm runs. With a flat object the list stays empty (covered by
// change_tracker_remaining_test.cpp GetContainingList_CharacterizesBug).
TEST_F(LspChangeTrackerArmsTests, GetContainingList_NestedObjectLiteral_ReturnsInnerProperties)
{
    const std::string source = R"(
let outer = {
    a: 1,
    inner: {
        x: 1,
        y: 2
    },
    c: 3
};
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("nestedObject.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *aProp = FindPropertyByKey(ast, "a");
    auto *xProp = FindPropertyByKey(ast, "x");
    auto *yProp = FindPropertyByKey(ast, "y");
    ASSERT_NE(aProp, nullptr);
    ASSERT_NE(xProp, nullptr);
    ASSERT_NE(yProp, nullptr);

    auto tracker = MakeTracker();
    std::vector<ark::es2panda::ir::AstNode *> list = tracker.GetContainingList(aProp);
    ASSERT_EQ(list.size(), 2U);
    EXPECT_EQ(list[0], xProp);
    EXPECT_EQ(list[1], yProp);
    initializer.DestroyContext(ctx);
}

// P1C-PG-005 arm sweep: InsertNodeInListAfter with a NON-empty containing list
// (change_tracker.cpp:809-821). The host property `a` is not part of the collected
// inner-object list (GetContainingList structural defect, change_tracker.cpp:761-773),
// so the search loop runs without a match, index stays 0 and the function returns
// early at change_tracker.cpp:819-821 without recording any change. This pins the
// reachable arms of the InsertNodeInListAfter guard chain; the deeper branches
// (change_tracker.cpp:826-841 incl. InsertNodeInListAfterMultiLine) are unreachable
// because a collected property's parent is always its ObjectExpression, which can
// never be a strict descendant of the host property's parent.
TEST_F(LspChangeTrackerArmsTests, InsertNodeInListAfter_NestedObjectHost_IndexZero_EarlyReturn)
{
    const std::string source = R"(
let outer = {
    a: 1,
    inner: {
        x: 1,
        y: 2
    },
    c: 3
};
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("insertInListNested.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *aProp = FindPropertyByKey(ast, "a");
    auto *xProp = FindPropertyByKey(ast, "x");
    auto *yProp = FindPropertyByKey(ast, "y");
    ASSERT_NE(aProp, nullptr);
    ASSERT_NE(xProp, nullptr);
    ASSERT_NE(yProp, nullptr);

    auto *aClone = aProp->Clone(context->allocator, nullptr);
    ASSERT_NE(aClone, nullptr);

    auto tracker = MakeTracker();
    std::vector<ark::es2panda::ir::AstNode *> containingList;
    tracker.InsertNodeInListAfter(ctx, aProp, aClone, containingList);

    // The out-parameter is overwritten with GetContainingList's result (inner properties).
    ASSERT_EQ(containingList.size(), 2U);
    EXPECT_EQ(containingList[0], xProp);
    EXPECT_EQ(containingList[1], yProp);

    // index == 0 early return: no change is recorded.
    EXPECT_TRUE(tracker.GetChangeList().empty());
    initializer.DestroyContext(ctx);
}

// P1C-PG-005 arm sweep: InsertAtTopOfFile blankLineBetween TRUE arm
// (change_tracker.cpp:202-203) via the public InsertNodeAtTopOfFile
// (change_tracker.cpp:554-557). The insertion position is the Start().index of the
// first ClassDeclaration (change_tracker.cpp:548-552); the exact prefix/suffix
// combination of the recorded change is pinned.
TEST_F(LspChangeTrackerArmsTests, InsertNodeAtTopOfFile_BlankLineBetweenTrue_SuffixDoubleNewLine)
{
    const std::string source = "function foo() {}";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("topOfFileBlank.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *sourceFile = context->sourceFile;
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *funcExpr = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsFunctionExpression() && n->AsFunctionExpression()->Function()->Id() != nullptr &&
               n->AsFunctionExpression()->Function()->Id()->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(funcExpr, nullptr);
    auto *funcClone = funcExpr->Clone(context->allocator, nullptr);
    ASSERT_NE(funcClone, nullptr);

    auto *firstClass = ast->FindChild([](ark::es2panda::ir::AstNode *n) { return n->IsClassDeclaration(); });
    ASSERT_NE(firstClass, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertNodeAtTopOfFile(ctx, funcClone, true);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    EXPECT_EQ(replace->node, funcClone);
    EXPECT_EQ(replace->range.pos, firstClass->Start().index);
    EXPECT_EQ(replace->range.end, firstClass->Start().index);
    ASSERT_TRUE(replace->options.has_value());
    EXPECT_EQ(replace->options->prefix, "");
    EXPECT_EQ(replace->options->suffix, "\n\n");
    initializer.DestroyContext(ctx);
}

// P1C-PG-005 arm sweep: InsertNodeAtConstructorEnd on a constructor without user
// statements (change_tracker.cpp:666-684). MEASURED: even for `constructor() {}` the
// statement collection at change_tracker.cpp:672-677 is NON-empty — the (empty) body
// BlockStatement itself derives from Statement and is collected as the "last
// statement" — so the else path runs InsertNodeAfter(body, newNode) and lands the
// zero-length edit at ToEditBoundary(body) == the constructor MethodDefinition end
// (change_tracker.cpp:310-314), with the default-arm suffix "\n"
// (change_tracker.cpp:342). The statements.empty() arm (change_tracker.cpp:679-681) is
// therefore effectively unreachable for real constructors; only the deeper
// ReplaceConstructorBody no-op guard (change_tracker.cpp:521) remains blocked behind
// it.
TEST_F(LspChangeTrackerArmsTests, InsertNodeAtConstructorEnd_EmptyBody_CollectsBodyBlockAsLastStatement)
{
    const std::string source = R"(
class C {
    constructor() {}
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ctorEndEmpty.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *sourceFile = context->sourceFile;
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *ctor = FindNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsConstructor(); });
    ASSERT_NE(ctor, nullptr);

    // First edit-boundary ancestor of the body (METHOD_DEFINITION, change_tracker.cpp:41).
    ark::es2panda::ir::AstNode *ctorBody = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsBlockStatement() && n->Parent() != nullptr && n->Parent()->IsConstructor();
    });
    ASSERT_NE(ctorBody, nullptr);
    ark::es2panda::ir::AstNode *ctorMethod = ctorBody;
    while (ctorMethod != nullptr && !ctorMethod->IsMethodDefinition()) {
        ctorMethod = ctorMethod->Parent();
    }
    ASSERT_NE(ctorMethod, nullptr);

    auto tracker = MakeTracker();
    tracker.InsertNodeAtConstructorEnd(ctx, ctor, nullptr);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto *replace = std::get_if<ReplaceWithSingleNode>(&changes[0]);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->sourceFile, sourceFile);
    EXPECT_EQ(replace->node, nullptr);
    EXPECT_EQ(replace->range.pos, ctorMethod->End().index);
    EXPECT_EQ(replace->range.end, ctorMethod->End().index);
    ASSERT_TRUE(replace->options.has_value());
    EXPECT_FALSE(replace->options->prefix.has_value());
    EXPECT_EQ(replace->options->suffix, "\n");
    initializer.DestroyContext(ctx);
}

// BLOCKED_PRODUCT_FIX (P1C-PG-005 arm sweep): InsertImportSpecifierAtIndex with index == 0
// (change_tracker.cpp:847): namedImports.at(index - 1) underflows to at(SIZE_MAX)
// and vector::at throws std::out_of_range. This pins the bounds-guard defect of the
// inverted index handling (same defect family as InsertFirstParameter's inverted
// empty check at change_tracker.cpp:747-748).
TEST_F(LspChangeTrackerArmsTests, DISABLED_InsertImportSpecifierAtIndex_IndexZero_ThrowsOutOfRange)
{
    const std::string source = R"(
import { foo, bar } from './mod';
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("specifierIndexZero.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    std::vector<ark::es2panda::ir::AstNode *> namedImports;
    ast->FindChild([&namedImports](ark::es2panda::ir::AstNode *n) {
        if (n->IsImportSpecifier()) {
            namedImports.push_back(n);
        }
        return false;
    });
    ASSERT_GE(namedImports.size(), 2U);

    auto tracker = MakeTracker();
    EXPECT_THROW(tracker.InsertImportSpecifierAtIndex(ctx, nullptr, namedImports, 0), std::out_of_range);
    EXPECT_TRUE(tracker.GetChangeList().empty());
    initializer.DestroyContext(ctx);
}

// P1C-PG-005 BLOCKED (GetOptionsForInsertNodeBefore change_tracker.cpp:229-230 TRUE arms):
// `before->IsStringLiteral() && before->Parent()->IsImportDeclaration()` requires a
// TS-flavoured ImportDeclaration (the ETS parser produces ETSImportDeclaration, whose
// strict IsImportDeclaration() is false; the ETS side-effect form `import "./mod";` is
// dropped from the AST entirely), and `before->IsNamedType()` requires a bare
// ir::NamedType node which only the AS parser creates (parser/ASparser.cpp). The LSP
// context factory (Initializer::CreateContext, lsp/include/internal_api.h:44-52) builds
// its config without an --extension override (lsp/src/internal_api.cpp:131-132), so no
// LSP context can parse TS or AS sources and neither TRUE arm is reachable.
// The FALSE arms of change_tracker.cpp:229 are covered transitively by the
// ImportSpecifier route (InsertNodeBefore_ImportSpecifier_SuffixVariants in
// change_tracker_insert_at_start_test.cpp).
// NEXT ACTION: product must expose a TS-flavoured context route (or drop the
// TS/AS-only arms) before they can be covered.

}  // namespace
