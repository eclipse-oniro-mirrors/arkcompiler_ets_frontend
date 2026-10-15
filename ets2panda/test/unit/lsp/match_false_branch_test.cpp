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

#include "lsp/include/node_matchers.h"
#include "lsp/include/api.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {

using ::NodeInfo;
using ark::es2panda::ir::AstNode;
using ark::es2panda::ir::AstNodeType;
using ark::es2panda::lsp::Initializer;

class LspMatchFalseBranchTests : public LSPAPITests {};

// =============================================================================
// P4-MFB-01: MatchClassDefinition_NullInput_ReturnsFalse - BLOCKED_DESIGN_MAPPING
// Design expectation: MatchClassDefinition(nullptr, info) returns false.
// Actual product behavior: MatchClassDefinition (node_matchers.cpp:59-62)
// dereferences childNode immediately via childNode->IsClassDefinition() without
// a null check. Passing nullptr would crash (nullptr dereference). The
// implementation does not guard against null input.
// Implementation evidence: node_matchers.cpp:61
//   `return childNode->IsClassDefinition() && ...`
// next_action: Add null guard at the top of MatchClassDefinition (or all
// Match* functions) before this branch can be tested.
// No TEST_F written for this item.

// P4-MFB-02: MatchClassDefinition_WrongType_ReturnsFalse
// Canonical copy of the wrong-type negative: the identical MatchClassDefinition_
// DoesNotMatchStruct test in node_matchers_test.cpp was consolidated here, so
// both a SCRIPT_FUNCTION and a STRUCT_DECLARATION node are covered.
TEST_F(LspMatchFalseBranchTests, MatchClassDefinition_WrongType_ReturnsFalse)
{
    Initializer initializer = Initializer();
    const std::string source = R"(function foo(): void {}
struct MyStruct {})";
    es2panda_Context *ctx = initializer.CreateContext("mfb02.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto scriptFunc = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::SCRIPT_FUNCTION; });
    ASSERT_NE(scriptFunc, nullptr);
    NodeInfo info("foo", AstNodeType::CLASS_DEFINITION);
    // scriptFunc is a SCRIPT_FUNCTION, not a CLASS_DEFINITION
    EXPECT_FALSE(ark::es2panda::lsp::MatchClassDefinition(scriptFunc, &info));
    auto structDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::STRUCT_DECLARATION; });
    ASSERT_NE(structDecl, nullptr);
    NodeInfo structInfo("MyStruct", AstNodeType::CLASS_DEFINITION);
    // A struct declaration is not a class definition either
    EXPECT_FALSE(ark::es2panda::lsp::MatchClassDefinition(structDecl, &structInfo));
    initializer.DestroyContext(ctx);
}

// P4-MFB-03: MatchIdentifier_NullInput_ReturnsFalse - BLOCKED_DESIGN_MAPPING
// Design expectation: MatchIdentifier(nullptr, info) returns false.
// Actual product behavior: MatchIdentifier (node_matchers.cpp:64-67)
// dereferences childNode immediately via childNode->IsIdentifier() without
// a null check. Passing nullptr would crash.
// Implementation evidence: node_matchers.cpp:66
//   `return childNode->IsIdentifier() && ...`
// next_action: Add null guard at the top of MatchIdentifier.
// No TEST_F written for this item.

// P4-MFB-04: MatchIdentifier_WrongType_ReturnsFalse
TEST_F(LspMatchFalseBranchTests, MatchIdentifier_WrongType_ReturnsFalse)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class C {})";
    es2panda_Context *ctx = initializer.CreateContext("mfb04.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto classDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::CLASS_DECLARATION; });
    ASSERT_NE(classDecl, nullptr);
    NodeInfo info("C", AstNodeType::IDENTIFIER);
    // classDecl is a CLASS_DECLARATION, not an IDENTIFIER
    EXPECT_FALSE(ark::es2panda::lsp::MatchIdentifier(classDecl, &info));
    initializer.DestroyContext(ctx);
}

// P4-MFB-05: MatchMethodDefinition_NullInput_ReturnsFalse - BLOCKED_DESIGN_MAPPING
// Design expectation: MatchMethodDefinition(nullptr, info) returns false.
// Actual product behavior: MatchMethodDefinition (node_matchers.cpp:79-83)
// dereferences childNode immediately via childNode->IsMethodDefinition()
// without a null check. Passing nullptr would crash.
// Implementation evidence: node_matchers.cpp:81
//   `return childNode->IsMethodDefinition() && ...`
// next_action: Add null guard at the top of MatchMethodDefinition.
// No TEST_F written for this item.

// P4-MFB-06: MatchMemberExpression_WrongType_ReturnsFalse
TEST_F(LspMatchFalseBranchTests, MatchMemberExpression_WrongType_ReturnsFalse)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = 42;)";
    es2panda_Context *ctx = initializer.CreateContext("mfb06.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto ident = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::IDENTIFIER; });
    ASSERT_NE(ident, nullptr);
    NodeInfo info("x", AstNodeType::MEMBER_EXPRESSION);
    // ident is an IDENTIFIER, not a MEMBER_EXPRESSION
    EXPECT_FALSE(ark::es2panda::lsp::MatchMemberExpression(ident, &info));
    initializer.DestroyContext(ctx);
}

// P4-MFB-07: MatchTSEnumDeclaration_NullInput_ReturnsFalse - BLOCKED_DESIGN_MAPPING
// Design expectation: MatchTSEnumDeclaration(nullptr, info) returns false.
// Actual product behavior: MatchTSEnumDeclaration (node_matchers.cpp:85-89)
// dereferences childNode immediately via childNode->IsTSEnumDeclaration()
// without a null check. Passing nullptr would crash.
// Implementation evidence: node_matchers.cpp:87
//   `return childNode->IsTSEnumDeclaration() && ...`
// next_action: Add null guard at the top of MatchTSEnumDeclaration.
// No TEST_F written for this item.

// P4-MFB-08: MatchTSInterfaceDeclaration_WrongType_ReturnsFalse
TEST_F(LspMatchFalseBranchTests, MatchTSInterfaceDeclaration_WrongType_ReturnsFalse)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class C {})";
    es2panda_Context *ctx = initializer.CreateContext("mfb08.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto classDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::CLASS_DECLARATION; });
    ASSERT_NE(classDecl, nullptr);
    NodeInfo info("C", AstNodeType::TS_INTERFACE_DECLARATION);
    // classDecl is a CLASS_DECLARATION, not a TS_INTERFACE_DECLARATION
    EXPECT_FALSE(ark::es2panda::lsp::MatchTSInterfaceDeclaration(classDecl, &info));
    initializer.DestroyContext(ctx);
}

// P4-MFB-09: MatchCallExpression_NullInput_ReturnsFalse - BLOCKED_DESIGN_MAPPING
// Design expectation: MatchCallExpression(nullptr, info) returns false.
// Actual product behavior: MatchCallExpression (node_matchers.cpp:144-160)
// dereferences childNode immediately via childNode->IsCallExpression()
// without a null check. Passing nullptr would crash.
// Implementation evidence: node_matchers.cpp:146
//   `if (childNode->IsCallExpression()) { ... }`
// next_action: Add null guard at the top of MatchCallExpression.
// No TEST_F written for this item.

// P4-MFB-10: MatchEtsStructDeclaration_WrongType_ReturnsFalse
TEST_F(LspMatchFalseBranchTests, MatchEtsStructDeclaration_WrongType_ReturnsFalse)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class C {})";
    es2panda_Context *ctx = initializer.CreateContext("mfb10.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto classDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::CLASS_DECLARATION; });
    ASSERT_NE(classDecl, nullptr);
    NodeInfo info("C", AstNodeType::STRUCT_DECLARATION);
    // classDecl is a CLASS_DECLARATION, not a STRUCT_DECLARATION
    EXPECT_FALSE(ark::es2panda::lsp::MatchEtsStructDeclaration(classDecl, &info));
    initializer.DestroyContext(ctx);
}

// P4-MFB-11: MatchImportSpecifier_NullInput_ReturnsFalse - BLOCKED_DESIGN_MAPPING
// Design expectation: MatchImportSpecifier(nullptr, info) returns false.
// Actual product behavior: MatchImportSpecifier (node_matchers.cpp:446-450)
// dereferences childNode immediately via childNode->IsImportSpecifier()
// without a null check. Passing nullptr would crash.
// Implementation evidence: node_matchers.cpp:448
//   `return childNode->IsImportSpecifier() && ...`
// next_action: Add null guard at the top of MatchImportSpecifier.
// No TEST_F written for this item.

// P4-MFB-12: MatchExportSpecifier_WrongType_ReturnsFalse
TEST_F(LspMatchFalseBranchTests, MatchExportSpecifier_WrongType_ReturnsFalse)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class C {})";
    es2panda_Context *ctx = initializer.CreateContext("mfb12.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto classDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::CLASS_DECLARATION; });
    ASSERT_NE(classDecl, nullptr);
    NodeInfo info("C", AstNodeType::EXPORT_SPECIFIER);
    // classDecl is a CLASS_DECLARATION, not an EXPORT_SPECIFIER
    EXPECT_FALSE(ark::es2panda::lsp::MatchExportSpecifier(classDecl, &info));
    initializer.DestroyContext(ctx);
}

// P4-MFB-13: MatchAwaitExpression_NullInput_ReturnsFalse - BLOCKED_DESIGN_MAPPING
// Design expectation: MatchAwaitExpression(nullptr, info) returns false.
// Actual product behavior: MatchAwaitExpression (node_matchers.cpp:408-419)
// dereferences childNode immediately via childNode->IsAwaitExpression()
// without a null check. Passing nullptr would crash.
// Implementation evidence: node_matchers.cpp:410
//   `if (!childNode->IsAwaitExpression()) { return false; }`
// next_action: Add null guard at the top of MatchAwaitExpression.
// No TEST_F written for this item.

// P4-MFB-14: MatchSpreadElement_WrongType_ReturnsFalse
TEST_F(LspMatchFalseBranchTests, MatchSpreadElement_WrongType_ReturnsFalse)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = 42;)";
    es2panda_Context *ctx = initializer.CreateContext("mfb14.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto ident = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::IDENTIFIER; });
    ASSERT_NE(ident, nullptr);
    NodeInfo info("x", AstNodeType::SPREAD_ELEMENT);
    // ident is an IDENTIFIER, not a SPREAD_ELEMENT
    EXPECT_FALSE(ark::es2panda::lsp::MatchSpreadElement(ident, &info));
    initializer.DestroyContext(ctx);
}

// P4-MFB-15: MatchBigIntLiteral_NullInput_ReturnsFalse - BLOCKED_DESIGN_MAPPING
// Design expectation: MatchBigIntLiteral(nullptr, info) returns false.
// Actual product behavior: MatchBigIntLiteral (node_matchers.cpp:421-432)
// dereferences childNode immediately via childNode->IsBigIntLiteral()
// without a null check. Passing nullptr would crash.
// Implementation evidence: node_matchers.cpp:423
//   `if (!childNode->IsBigIntLiteral()) { return false; }`
// next_action: Add null guard at the top of MatchBigIntLiteral.
// No TEST_F written for this item.

}  // namespace