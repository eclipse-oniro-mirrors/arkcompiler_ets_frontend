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

#include "lsp/include/get_adjusted_location.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include <gtest/gtest.h>

namespace {
using ark::es2panda::ir::AstNode;
using ark::es2panda::ir::AstNodeType;
using ark::es2panda::lsp::Initializer;

class LspGetAdjustedLocationHelpersTests : public LSPAPITests {};

TEST_F(LspGetAdjustedLocationHelpersTests, FindFirstIdentifier_NoIdentifier_ReturnsNull)
{
    Initializer initializer = Initializer();
    const std::string source = R"(1 + 2;)";
    es2panda_Context *ctx = initializer.CreateContext("no_id_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto *node = ark::es2panda::lsp::GetTouchingToken(ctx, 0, false);
    ASSERT_NE(node, nullptr);

    auto children = ark::es2panda::lsp::GetChildren(node);
    auto *found = ark::es2panda::lsp::FindFirstIdentifier(node, false, children);
    EXPECT_EQ(found, nullptr);

    initializer.DestroyContext(ctx);
}

// Same seam as upstream get_adjusted_location_test.cpp FindFirstExpressionTest;
// unique dimension: entry via GetTouchingToken rather than a fixed node.
TEST_F(LspGetAdjustedLocationHelpersTests, FindFirstExpression_FindsFirstExpr)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = 42 + 1;)";
    es2panda_Context *ctx = initializer.CreateContext("find_expr_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto *node = ark::es2panda::lsp::GetTouchingToken(ctx, 0, false);
    ASSERT_NE(node, nullptr);

    auto children = ark::es2panda::lsp::GetChildren(node);
    auto *found = ark::es2panda::lsp::FindFirstExpression(node, children);
    ASSERT_NE(found, nullptr);
    EXPECT_TRUE(found->IsExpression());

    initializer.DestroyContext(ctx);
}

// Extends upstream FindNodeOfTypeTest with a METHOD_DEFINITION hit and the
// no-match nullptr arm.
TEST_F(LspGetAdjustedLocationHelpersTests, FindNodeOfType_FindsMatchingType)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class TestClass { method() { return 42; } })";
    es2panda_Context *ctx = initializer.CreateContext("find_node_type_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto classDef = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::CLASS_DEFINITION; });
    ASSERT_NE(classDef, nullptr);

    auto children = ark::es2panda::lsp::GetChildren(classDef);
    auto *found = ark::es2panda::lsp::FindNodeOfType(classDef, AstNodeType::METHOD_DEFINITION, children);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->Type(), AstNodeType::METHOD_DEFINITION);

    auto *notFound = ark::es2panda::lsp::FindNodeOfType(classDef, AstNodeType::TS_INTERFACE_DECLARATION, children);
    EXPECT_EQ(notFound, nullptr);

    initializer.DestroyContext(ctx);
}

// Upstream IsModifierTest checks a conditional modifier context; this pins the
// deterministic false path on a plain identifier.
TEST_F(LspGetAdjustedLocationHelpersTests, IsModifier_FalseForNonModifier)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class Test { x: number; })";
    es2panda_Context *ctx = initializer.CreateContext("non_modifier_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    // Find a non-modifier identifier node
    auto ident = ast->FindChild(
        [](AstNode *node) { return node->Type() == AstNodeType::IDENTIFIER && node->AsIdentifier()->Name() == "x"; });
    ASSERT_NE(ident, nullptr);

    EXPECT_FALSE(ark::es2panda::lsp::IsModifier(ident));

    initializer.DestroyContext(ctx);
}

// Canonical copy: the identical intra-PR twin in get_adjusted_location_remaining_test.cpp
// was removed; the false-for-expression arm below is unique.
TEST_F(LspGetAdjustedLocationHelpersTests, CanHaveModifiers_TrueForClass)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class MyClass {})";
    es2panda_Context *ctx =
        initializer.CreateContext("can_have_mod_class_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    // CanHaveModifiers recognizes CLASS_DECLARATION (not CLASS_DEFINITION)
    auto classDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::CLASS_DECLARATION; });
    ASSERT_NE(classDecl, nullptr);

    EXPECT_TRUE(ark::es2panda::lsp::CanHaveModifiers(*classDecl));

    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocationHelpersTests, CanHaveModifiers_FalseForExpression)
{
    Initializer initializer = Initializer();
    const std::string source = R"(1 + 2;)";
    es2panda_Context *ctx =
        initializer.CreateContext("can_have_mod_expr_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto binaryExpr = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::BINARY_EXPRESSION; });
    ASSERT_NE(binaryExpr, nullptr);

    EXPECT_FALSE(ark::es2panda::lsp::CanHaveModifiers(*binaryExpr));

    initializer.DestroyContext(ctx);
}

// P1D2-GA-12: IsOuterExpression_TrueForParenExpr — BLOCKED_DESIGN_MAPPING
// Implementation IsOuterExpressionKind only accepts TS_TYPE_ASSERTION,
// TS_AS_EXPRESSION, and TS_NON_NULL_EXPRESSION, not PAREN_EXPRESSION.
// Replacing with `as` changes the designed semantics; do not change product.
// See get_adjusted_location.cpp:30-31.

// P1D2-GA-13: IsDeclarationOrModifier_TrueForDecl — BLOCKED_DESIGN_MAPPING
// Implementation only recognizes the modifier relationship and special
// node/parent same-kind/pattern pairs; the current TS_CLASS_IMPLEMENTS's
// immediate parent is CLASS_DEFINITION (not CLASS_DECLARATION), so the
// check returns false.  Do not pass a fake/non-parent parent and do not
// change product.
// See get_adjusted_location.cpp:217-231.

}  // namespace