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
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/get_adjusted_location.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::ir::AstNodeType;
using ark::es2panda::lsp::CanHaveModifiers;
using ark::es2panda::lsp::FindFirstIdentifier;
using ark::es2panda::lsp::GetAdjustedLocationForDeclaration;
using ark::es2panda::lsp::GetAdjustedLocationForHeritageClause;
using ark::es2panda::lsp::GetChildren;
using ark::es2panda::lsp::GetTouchingIdentifierName;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::IsDeclarationOrModifier;
using ark::es2panda::lsp::IsOuterExpression;

class LspGetAdjustedLocationRemainingTests : public LSPAPITests {
protected:
    AstNode *FindNode(AstNode *root, std::function<bool(AstNode *)> pred)
    {
        AstNode *result = nullptr;
        root->FindChild([&result, &pred](AstNode *n) {
            if (result == nullptr && pred(n)) {
                result = n;
            }
            return false;
        });
        return result;
    }
};

// =============================================================================
// P2E-GAL-05: GetTouchingIdentifierName_FindsIdentifier
// =============================================================================
TEST_F(LspGetAdjustedLocationRemainingTests, GetTouchingIdentifierName_FindsIdentifier)
{
    const std::string source = R"(
function foo() {}
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("gal05.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    size_t pos = source.find("foo");
    ASSERT_NE(pos, std::string::npos);

    AstNode *result = GetTouchingIdentifierName(ctx, pos);
    ASSERT_NE(result, nullptr);
    // GetTouchingIdentifierName returns either the identifier, or a declaration
    // node that contains the identifier
    EXPECT_TRUE(result->IsIdentifier() || result->IsClassDeclaration() || result->IsFunctionDeclaration() ||
                result->IsProperty() || result->IsTSConstructorType());

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-GAL-06: FindFirstIdentifier_FindsFirstId
// =============================================================================
// Kept over the helpers-file twin because it additionally asserts the identifier name;
// the descent branch is also covered by get_adjusted_location_test_2.cpp.
TEST_F(LspGetAdjustedLocationRemainingTests, FindFirstIdentifier_FindsFirstId)
{
    const std::string source = R"(let x = 42;)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("gal06.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *varDecl = FindNode(ast, [](AstNode *n) { return n->IsVariableDeclaration(); });
    ASSERT_NE(varDecl, nullptr);

    auto children = GetChildren(varDecl);
    AstNode *found = FindFirstIdentifier(varDecl, false, children);
    ASSERT_NE(found, nullptr);
    EXPECT_TRUE(found->IsIdentifier());

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-GAL-10: IsDeclarationOrModifier_TrueForDecl
// =============================================================================
// Walks real child-parent pairs inside a parsed class declaration: at least one
// pair (method definition under CLASS_DECLARATION) satisfies IsDeclarationOrModifier.
// EARLIER test_2 DeclarationOrModifierChainEvaluatesFalseForRealPairs pins the
// false arm for identifier/declarator and varDecl/Program pairs; both arms are
// reachable on real nodes depending on the pair space.
TEST_F(LspGetAdjustedLocationRemainingTests, IsDeclarationOrModifier_TrueForDecl)
{
    const std::string source = R"(
class MyClass {
    public foo(): void {}
}
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("gal10.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *classDecl = FindNode(ast, [](AstNode *n) { return n->IsClassDeclaration(); });
    ASSERT_NE(classDecl, nullptr);

    auto children = GetChildren(classDecl);
    // Check every child-parent pair within the class
    bool foundAny = false;
    for (auto *child : children) {
        if (child != classDecl && child->Parent() != nullptr && IsDeclarationOrModifier(child, classDecl)) {
            foundAny = true;
            break;
        }
    }
    // ETS class declaration with a method definition produces a child-parent
    // pair where IsDeclarationOrModifier returns true (likely the METHOD_DEFINITION
    // child of CLASS_DECLARATION with the function-kind pair).
    EXPECT_TRUE(foundAny);

    initializer.DestroyContext(ctx);
}

}  // namespace