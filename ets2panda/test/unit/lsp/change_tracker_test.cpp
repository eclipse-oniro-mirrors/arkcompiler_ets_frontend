/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <cstdio>
#include <gtest/gtest.h>
#include <iostream>
#include <ostream>
#include <regex>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/class_hierarchy.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "public/es2panda_lib.h"
#include <variant>

namespace {

class LspClassChangeTracker : public LSPAPITests {};

const size_t START_POS = 0;
const size_t END_POS = 9;
const auto C6 = 6;

bool g_hasTextChange = false;
bool g_hasNodeChange = false;

ark::es2panda::lsp::ChangeTracker GetTracker()
{
    const std::string defaultNewLine = "\n";
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    auto tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    return tracker;
}

TEST_F(LspClassChangeTracker, PushRaw_AddsChangeCorrectly)
{
    const char *source = R"(
function add(a: number, b: number) {
    return a + b;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("pushRaw_AddsChangeCorrectly.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto sourceFile = context->sourceFile;

    const auto *returnNode =
        context->parserProgram->Ast()->FindChild([](const auto *node) { return node->IsReturnStatement(); });
    ASSERT_NE(returnNode, nullptr);

    auto spanStart = returnNode->Start().index;
    auto spanLength = strlen("return a + b;");
    TextSpan span = {spanStart, spanLength};
    std::string retStr = "return a - b;";
    TextChange change = {span, retStr};
    FileTextChanges fileChange = {"pushRaw_AddsChangeCorrectly.ets", {change}};
    auto tracker = GetTracker();
    tracker.PushRaw(sourceFile, fileChange);

    auto list = tracker.GetChanges();
    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.size(), 1);

    const auto variant = list[0];
    const auto changeText = variant.textChanges;
    ASSERT_EQ(changeText[0].newText, retStr);

    EXPECT_EQ(variant.fileName, fileChange.fileName);

    const auto &textChange = changeText;
    EXPECT_EQ(textChange[0].span.start, spanStart);
}

TEST_F(LspClassChangeTracker, DeleteMethods_BasicTest1)
{
    const char *source = R"(
function test() {
    const a = 1;
    const b = 2;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("deleteMethods_BasicTest1.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto sourceFile = context->sourceFile;
    ark::es2panda::ir::AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    const auto ca = "a";
    const auto c0 = 0;
    const auto c1 = 1;
    const auto c2 = 2;
    ark::es2panda::ir::AstNode *aDecl;
    ast->FindChild([&ca, &aDecl](ark::es2panda::ir::AstNode *node) {
        if (node->IsVariableDeclaration() &&
            node->AsVariableDeclaration()->Declarators()[c0]->Id()->AsIdentifier()->Name() == ca) {
            aDecl = node;
        }
        return false;
    });
    ASSERT_NE(aDecl, nullptr);
    auto tracker = GetTracker();
    std::variant<const ark::es2panda::ir::AstNode *, const std::vector<const ark::es2panda::ir::AstNode *>>
        nodeVariant = aDecl;
    tracker.Delete(sourceFile, nodeVariant);
    const auto &deletedNodes = tracker.GetDeletedNodesList();
    EXPECT_EQ(deletedNodes.size(), c1);
    ark::es2panda::lsp::TextRange range = {aDecl->Start().index, aDecl->End().index};
    tracker.DeleteRange(sourceFile, range);
    const auto &changes = tracker.GetChangeList();
    EXPECT_EQ(changes.size(), c1);
    auto &change = std::get<ark::es2panda::lsp::RemoveNode>(changes[c0]);
    EXPECT_EQ(change.kind, ark::es2panda::lsp::ChangeKind::REMOVE);
    EXPECT_EQ(change.range.pos, range.pos);
    EXPECT_EQ(change.range.end, range.end);
    tracker.DeleteNode(ctx, sourceFile, aDecl);
    EXPECT_EQ(tracker.GetChangeList().size(), c2);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, DeleteHelpers_BasicTest2)
{
    const char *source = "export async function test() { \n let a = 1; \n let b = 2; \n let c = 3; \n}";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("deleteHelpers_BasicTest2.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    const auto *sourceFile = context->sourceFile;
    auto tracker = GetTracker();
    ark::es2panda::ir::AstNode *aDecl;
    ark::es2panda::ir::AstNode *bDecl;
    ark::es2panda::ir::AstNode *cDecl;
    const auto c3 = 3;

    ast->FindChild([&aDecl, &bDecl, &cDecl](ark::es2panda::ir::AstNode *node) {
        if (node->IsVariableDeclaration() &&
            node->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "a") {
            aDecl = node;
        }
        if (node->IsVariableDeclaration() &&
            node->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "b") {
            bDecl = node;
        }
        if (node->IsVariableDeclaration() &&
            node->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "c") {
            cDecl = node;
        }
        return false;
    });
    ASSERT_NE(aDecl, nullptr);
    ASSERT_NE(bDecl, nullptr);
    ASSERT_NE(cDecl, nullptr);
    tracker.DeleteNode(ctx, sourceFile, aDecl);
    tracker.DeleteNodeRange(ctx, bDecl, cDecl);
    tracker.DeleteNodeRangeExcludingEnd(ctx, const_cast<ark::es2panda::ir::AstNode *>(aDecl), bDecl);
    const auto &changes = tracker.GetChangeList();  // varsayalım internal erişim var
    EXPECT_EQ(changes.size(), c3);
    for (const auto &c : changes) {
        if (const auto *removeNode = std::get_if<ark::es2panda::lsp::RemoveNode>(&c)) {
            EXPECT_EQ(removeNode->kind, ark::es2panda::lsp::ChangeKind::REMOVE);
            EXPECT_LT(removeNode->range.pos, removeNode->range.end);
        }
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, ReplaceHelpers_BasicTest1)
{
    const char *source = "function test() {\nlet a = 1;\nlet b = 2;\n}";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("replaceHelpers_BasicTest1.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    auto *sourceFile = context->sourceFile;
    auto tracker = GetTracker();
    ark::es2panda::ir::AstNode *aDecl;
    ark::es2panda::ir::AstNode *bDecl;
    ast->FindChild([&aDecl, &bDecl](ark::es2panda::ir::AstNode *node) {
        if (node->IsVariableDeclaration() &&
            node->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "a") {
            aDecl = node;
        }
        if (node->IsVariableDeclaration() &&
            node->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "b") {
            bDecl = node;
        }
        return false;
    });
    const char *textReplace = "// replaced\n";
    const char *replaceNode = "let x = 100;";
    ark::es2panda::ir::AstNode *bClone = bDecl->Clone(context->allocator, nullptr);
    ark::es2panda::lsp::ChangeNodeOptions options = {};
    tracker.ReplaceNode(ctx, aDecl, bClone, options);
    ark::es2panda::ir::AstNode *fnNode =
        ast->FindChild([](ark::es2panda::ir::AstNode *node) { return node->IsFunctionExpression(); });
    tracker.ReplaceNodeRange(ctx, bClone, fnNode, bClone);
    std::vector<ark::es2panda::ir::AstNode *> bCloneVec {bClone};
    tracker.ReplaceNodeWithNodes(ctx, aDecl, bCloneVec);
    tracker.ReplaceNodeWithText(ctx, bClone, replaceNode);
    tracker.ReplaceRangeWithText(sourceFile, {START_POS, END_POS}, textReplace);
    tracker.ReplaceNodeRangeWithNodes(ctx, bClone, bClone, bCloneVec);
    const auto &changes = tracker.GetChangeList();  // Internal erişim
    EXPECT_EQ(changes.size(), C6);
    for (const auto &change : changes) {
        if (std::get_if<ark::es2panda::lsp::ChangeText>(&change) != nullptr) {
            g_hasTextChange = true;
        }
        if (std::get_if<ark::es2panda::lsp::ReplaceWithSingleNode>(&change) != nullptr) {
            g_hasNodeChange = true;
        }
    }
    EXPECT_TRUE(g_hasTextChange);
    EXPECT_TRUE(g_hasNodeChange);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, ReplacePropertyAssignment_BasicTest)
{
    const char *source = R"(
let obj = {
    foo: 1
};)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("replacePropertyAssignment_BasicTest.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *propertyNode = ast->FindChild([](const ark::es2panda::ir::AstNode *node) {
        return node->IsProperty() && node->AsProperty()->Key()->AsIdentifier()->Name() == "foo" &&
               node->Parent()->IsObjectExpression() && node->Parent()->Parent()->IsAssignmentExpression();
    });
    ASSERT_NE(propertyNode, nullptr);
    ark::es2panda::ir::AstNode *newProperty = propertyNode->Clone(context->allocator, nullptr);
    ASSERT_NE(newProperty, nullptr);
    auto *prop = const_cast<ark::es2panda::ir::Property *>(newProperty->AsProperty());
    auto *ident = const_cast<ark::es2panda::ir::Identifier *>(prop->Key()->AsIdentifier());
    ident->SetName("bar");
    ark::es2panda::lsp::ChangeTracker tracker = GetTracker();
    tracker.ReplacePropertyAssignment(ctx, propertyNode, newProperty);
    const auto &changes = tracker.GetChangeList();
    const size_t c1 = 1;
    const auto bar = "bar";
    const auto newLine = "";
    ASSERT_EQ(changes.size(), c1);
    const auto &change = changes[0];
    const auto *replace = std::get_if<ark::es2panda::lsp::ReplaceWithSingleNode>(&change);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ark::es2panda::lsp::ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(replace->node->AsProperty()->Key()->AsIdentifier()->Name(), bar);
    EXPECT_EQ(replace->options->suffix, newLine);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, ReplaceConstructorBody_BasicTest)
{
    const char *source = "class MyClass {\nconstructor() {\nlet a = 1;\n}\n}";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("replaceConstructorBody_BasicTest.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    ark::es2panda::ir::AstNode *ctrNode;
    ast->FindChild([&ctrNode](ark::es2panda::ir::AstNode *node) {
        if (node->IsMethodDefinition() &&
            node->AsMethodDefinition()->Kind() == ark::es2panda::ir::MethodDefinitionKind::CONSTRUCTOR) {
            ctrNode = node->AsMethodDefinition()->Function()->Body()->AsBlockStatement();
        }
        return false;
    });
    ASSERT_NE(ctrNode, nullptr);
    std::vector<ark::es2panda::ir::Statement *> dummyStatements;
    for (auto stmt : ctrNode->AsBlockStatement()->Statements()) {
        dummyStatements.push_back(stmt);
    }
    ark::es2panda::lsp::ChangeTracker tracker = GetTracker();
    tracker.ReplaceConstructorBody(ctx, ctrNode, dummyStatements);
    const auto &changes = tracker.GetChangeList();
    const size_t c1 = 1;
    ASSERT_EQ(changes.size(), c1);
    const auto &change = changes[0];
    const auto *replace = std::get_if<ark::es2panda::lsp::ReplaceWithSingleNode>(&change);
    ASSERT_NE(replace, nullptr);
    EXPECT_EQ(replace->kind, ark::es2panda::lsp::ChangeKind::REPLACEWITHSINGLENODE);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, InsertHelpers_BasicTest1)
{
    const char *source = "function test() {\nlet a = 1;\nlet b = 2;\n}";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("insertHelpers_BasicTest1.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    ark::es2panda::lsp::ChangeTracker tracker = GetTracker();
    ark::es2panda::ir::AstNode *aDecl;
    ark::es2panda::ir::AstNode *bDecl;
    ast->FindChild([&aDecl, &bDecl](ark::es2panda::ir::AstNode *node) {
        if (node->IsVariableDeclaration() &&
            node->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "a") {
            aDecl = node;
        }
        if (node->IsVariableDeclaration() &&
            node->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name() == "b") {
            bDecl = node;
        }
        return false;
    });
    ASSERT_NE(aDecl, nullptr);
    ASSERT_NE(bDecl, nullptr);
    ark::es2panda::ir::AstNode *bClone = bDecl->Clone(context->allocator, nullptr);
    ark::es2panda::lsp::InsertNodeOptions options;
    ASSERT_NE(bClone, nullptr);
    auto *ident = const_cast<ark::es2panda::ir::Identifier *>(
        bClone->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier());
    ident->SetName("bar");
    tracker.InsertNodeAt(ctx, aDecl->Start().index, bClone, options);
    ident->SetName("top");
    tracker.InsertNodeAtTopOfFile(ctx, bClone, false);
    ident->SetName("before");
    tracker.InsertNodeBefore(ctx, aDecl, bClone, false);
    ident->SetName("modify");
    tracker.InsertModifierAt(ctx, aDecl->End().index, bClone, options);
    ident->SetName("modifBefore");
    tracker.InsertModifierBefore(ctx, aDecl, bClone);
    const size_t c5 = 5;
    ASSERT_EQ(tracker.GetChangeList().size(), c5);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, InsertTextTest)
{
    const std::string sourceText = "const a = 1;";
    const std::string file = "insertTextTest_1.ets";
    auto filePaths = CreateTempFile({file}, {sourceText});
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(file.c_str(), ES2PANDA_STATE_CHECKED, sourceText.c_str());
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);

    ark::es2panda::lsp::ChangeTracker tracker = GetTracker();
    const size_t insertPosition = 0;
    const size_t c1 = 1;
    const std::string textToInsert = "// Inserted text\n";
    tracker.InsertText(context->sourceFile, insertPosition, textToInsert);
    ASSERT_EQ(tracker.GetChangeList().size(), c1);
    const auto &changeVariant = tracker.GetChangeList();
    const auto &change = std::get<ark::es2panda::lsp::ChangeText>(changeVariant[0]);
    ASSERT_EQ(change.kind, ark::es2panda::lsp::ChangeKind::TEXT);
    EXPECT_EQ(change.range.pos, insertPosition);
    EXPECT_EQ(change.text, textToInsert);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, TryInsertTypeAnnotationTest)
{
    const std::string sourceText = "const a = 1;";
    const std::string file = "tryInsertTypeAnnotationTest.ets";
    auto filePaths = CreateTempFile({file}, {sourceText});
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(file.c_str(), ES2PANDA_STATE_CHECKED, sourceText.c_str());
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);

    ark::es2panda::lsp::ChangeTracker tracker = GetTracker();
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    ark::es2panda::ir::AstNode *aDecl;
    ast->FindChild([&aDecl](ark::es2panda::ir::AstNode *node) {
        if (node->IsClassProperty() && node->AsClassProperty()->Id()->AsIdentifier()->Name() == "a") {
            aDecl = node;
        }
        return false;
    });
    ASSERT_NE(aDecl, nullptr);
    const size_t c1 = 1;
    const auto prefix = ": ";
    ark::es2panda::ir::AstNode *aClone = aDecl->Clone(context->allocator, nullptr);
    tracker.TryInsertTypeAnnotation(ctx, aDecl, aClone);
    ASSERT_EQ(tracker.GetChangeList().size(), c1);
    const auto change = std::get<ark::es2panda::lsp::ReplaceWithSingleNode>(tracker.GetChangeList()[0]);
    ASSERT_EQ(change.kind, ark::es2panda::lsp::ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(change.range.pos, aDecl->End().index);
    EXPECT_EQ(change.options->prefix, prefix);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, TryInsertThisTypeAnnotationTest)
{
    const std::string sourceText = "function foo() {}";
    const std::string file = "tryInsertThisTypeAnnotationTest.ets";
    auto filePaths = CreateTempFile({file}, {sourceText});
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(file.c_str(), ES2PANDA_STATE_CHECKED, sourceText.c_str());
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ark::es2panda::lsp::ChangeTracker tracker = GetTracker();
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    ark::es2panda::ir::AstNode *funcDecl;
    ast->FindChild([&funcDecl](ark::es2panda::ir::AstNode *node) {
        if (node->IsFunctionExpression() &&
            node->AsFunctionExpression()->Function()->Id()->AsIdentifier()->Name() == "foo") {
            funcDecl = node;
        }
        return false;
    });
    ASSERT_NE(funcDecl, nullptr);
    ark::es2panda::ir::AstNode *aClone = funcDecl->Clone(context->allocator, nullptr);
    tracker.TryInsertThisTypeAnnotation(ctx, funcDecl, aClone);
    const auto c1 = 1;
    const auto prefix = "this: ";
    ASSERT_EQ(tracker.GetChangeList().size(), c1);
    const auto &changeVariant = tracker.GetChangeList();
    const auto &change = std::get<ark::es2panda::lsp::ReplaceWithSingleNode>(changeVariant[0]);
    ASSERT_EQ(change.kind, ark::es2panda::lsp::ChangeKind::REPLACEWITHSINGLENODE);
    EXPECT_EQ(change.range.pos, funcDecl->Start().index);
    EXPECT_EQ(change.options->prefix, prefix);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, InsertTypeParametersTest)
{
    const std::string sourceText = "function foo(a: number) {}";
    const std::string file = "insertTypeParametersTest.ets";
    auto filePaths = CreateTempFile({file}, {sourceText});
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(file.c_str(), ES2PANDA_STATE_CHECKED, sourceText.c_str());
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ark::es2panda::lsp::ChangeTracker tracker = GetTracker();
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    ark::es2panda::ir::AstNode *funcDecl;
    ark::es2panda::ir::AstNode *param;
    ast->FindChild([&funcDecl](ark::es2panda::ir::AstNode *node) {
        if (node->IsFunctionExpression() &&
            node->AsFunctionExpression()->Function()->Id()->AsIdentifier()->Name() == "foo") {
            funcDecl = node;
        }
        return false;
    });
    ast->FindChild([&param](ark::es2panda::ir::AstNode *node) {
        if (node->IsETSParameterExpression()) {
            param = node;
        }
        return false;
    });
    ASSERT_NE(funcDecl, nullptr);
    const auto param1 = param->Clone(context->allocator, nullptr);
    const auto param2 = param->Clone(context->allocator, nullptr);
    std::vector<ark::es2panda::ir::AstNode *> typeParams {param1, param2};
    tracker.InsertTypeParameters(ctx, funcDecl, typeParams);
    const auto c1 = 1;
    const auto prefix = "<";
    const auto suffix = ">";
    const auto joiner = ", ";
    ASSERT_EQ(tracker.GetChangeList().size(), c1);
    const auto &changeVariant = tracker.GetChangeList();
    const auto &change = std::get<ark::es2panda::lsp::ReplaceWithMultipleNodes>(changeVariant[0]);
    ASSERT_EQ(change.kind, ark::es2panda::lsp::ChangeKind::REPLACEWITHMULTIPLENODES);
    EXPECT_EQ(change.range.pos, funcDecl->End().index);  // This may be wrong
    EXPECT_EQ(change.options->prefix, prefix);
    EXPECT_EQ(change.options->suffix, suffix);
    EXPECT_EQ(change.options->joiner, joiner);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, InsertExportModifier_BasicTest)
{
    const char *source = R"(
 class ETSGLOBAL {
     public static foo() {
         let x = 10;
     }
 }
 )";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("insertExportModifier_BasicTest.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(context->parserProgram->Ast(), nullptr);
    auto *sourceFile = context->sourceFile;
    ark::es2panda::lsp::ChangeTracker tracker = GetTracker();
    ark::es2panda::ir::AstNode *classNode = context->parserProgram->Ast()->FindChild(
        [](ark::es2panda::ir::AstNode *node) { return node->IsClassDeclaration(); });
    ASSERT_NE(classNode, nullptr);
    auto *statementNode = classNode->AsStatement();
    ASSERT_NE(statementNode, nullptr);
    tracker.InsertExportModifier(sourceFile, statementNode);
    const auto c1 = 1;
    const auto expText = "export ";
    ASSERT_EQ(tracker.GetChangeList().size(), c1);
    const auto &changeVariant = tracker.GetChangeList();
    const auto &change = std::get<ark::es2panda::lsp::ChangeText>(changeVariant[0]);
    ASSERT_NE(&change, nullptr);
    EXPECT_EQ(change.text, expText);
    EXPECT_EQ(change.range.pos, statementNode->Start().index);
    EXPECT_EQ(change.range.pos, change.range.end);  // Insert olduğu için boş aralık

    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, DeleteNode_InnerIdentifierDeletesWholeVarDecl)
{
    const char *source = R"(
function f() {
    let a = 42;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("deleteNode_InnerIdentifierDeletesWholeVarDecl.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    auto *sourceFile = context->sourceFile;
    ASSERT_TRUE(ast != nullptr);
    ark::es2panda::ir::AstNode *aIdent = nullptr;
    ark::es2panda::ir::AstNode *varDecl = nullptr;
    ast->FindChild([&](ark::es2panda::ir::AstNode *node) {
        if (aIdent == nullptr && node->IsIdentifier() && node->AsIdentifier()->Name() == "a") {
            aIdent = node;
        }
        if (varDecl == nullptr && node->IsVariableDeclaration()) {
            varDecl = node;
        }
        return false;
    });

    ASSERT_TRUE(aIdent != nullptr);
    ASSERT_TRUE(varDecl != nullptr);
    auto tracker = GetTracker();
    tracker.DeleteNode(ctx, sourceFile, aIdent);
    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto &rem = std::get<ark::es2panda::lsp::RemoveNode>(changes[0]);
    EXPECT_EQ(rem.kind, ark::es2panda::lsp::ChangeKind::REMOVE);
    EXPECT_EQ(rem.range.pos, varDecl->Start().index);
    EXPECT_EQ(rem.range.end, varDecl->End().index);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, DeleteNodeRange_InnerNodesExpandAcrossStatements)
{
    const char *source = R"(
function f() {
    let a = 1;
    let b = 2;
    let c = 3;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("deleteNodeRange_InnerNodesExpandAcrossStatements.ets",
                                                      ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();

    ASSERT_TRUE(ast != nullptr);

    ark::es2panda::ir::AstNode *aIdent = nullptr;
    ark::es2panda::ir::AstNode *cIdent = nullptr;
    ark::es2panda::ir::AstNode *aDecl = nullptr;
    ark::es2panda::ir::AstNode *cDecl = nullptr;

    ast->FindChild([&](ark::es2panda::ir::AstNode *node) {
        if (node->IsVariableDeclaration()) {
            const auto name = node->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name();
            if (name == "a") {
                aDecl = node;
            }
            if (name == "c") {
                cDecl = node;
            }
        }
        if (node->IsIdentifier() && node->AsIdentifier()->Name() == "a") {
            aIdent = node;
        }
        if (node->IsIdentifier() && node->AsIdentifier()->Name() == "c") {
            cIdent = node;
        }
        return false;
    });

    ASSERT_TRUE(aIdent != nullptr);
    ASSERT_TRUE(cIdent != nullptr);
    ASSERT_TRUE(aDecl != nullptr);
    ASSERT_TRUE(cDecl != nullptr);

    auto tracker = GetTracker();
    tracker.DeleteNodeRange(ctx, aIdent, cIdent);

    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);

    const auto &rem = std::get<ark::es2panda::lsp::RemoveNode>(changes[0]);
    EXPECT_EQ(rem.kind, ark::es2panda::lsp::ChangeKind::REMOVE);
    EXPECT_EQ(rem.range.pos, aDecl->Start().index);
    EXPECT_EQ(rem.range.end, cDecl->End().index);

    initializer.DestroyContext(ctx);
}

TEST_F(LspClassChangeTracker, DeleteNode_FromPropertyKeyDeletesWholeProperty)
{
    const char *source = R"(
const obj = {
    foo: 1,
    bar: 2
};
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("deleteNode_FromPropertyKeyDeletesWholeProperty.ets", ES2PANDA_STATE_CHECKED, source);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    auto *sourceFile = context->sourceFile;
    ASSERT_NE(ast, nullptr);
    ark::es2panda::ir::AstNode *fooKey = nullptr;
    ark::es2panda::ir::AstNode *fooProp = nullptr;
    ast->FindChild([&](ark::es2panda::ir::AstNode *node) {
        if (!node->IsProperty()) {
            return false;
        }
        const auto *key = node->AsProperty()->Key();
        if (key == nullptr || !key->IsIdentifier()) {
            return false;
        }
        const auto *id = key->AsIdentifier();
        if (id->Name() != "foo") {
            return false;
        }
        fooProp = node;
        fooKey = const_cast<ark::es2panda::ir::Identifier *>(id);
        return true;
    });
    ASSERT_NE(fooKey, nullptr);
    ASSERT_NE(fooProp, nullptr);
    auto tracker = GetTracker();
    tracker.DeleteNode(ctx, sourceFile, fooKey);
    const auto &changes = tracker.GetChangeList();
    ASSERT_EQ(changes.size(), 1U);
    const auto &rem = std::get<ark::es2panda::lsp::RemoveNode>(changes[0]);
    EXPECT_EQ(rem.kind, ark::es2panda::lsp::ChangeKind::REMOVE);
    EXPECT_EQ(rem.range.pos, fooProp->Start().index);
    EXPECT_EQ(rem.range.end, fooProp->End().index);

    initializer.DestroyContext(ctx);
}
}  // namespace