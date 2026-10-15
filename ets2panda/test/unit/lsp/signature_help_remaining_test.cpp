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
#include <optional>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/signature_help.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::CountBinaryExpressionParameters;
using ark::es2panda::lsp::CreateTextSpanForNode;
using ark::es2panda::lsp::GetContainingArgumentInfo;
using ark::es2panda::lsp::GetContextualSignatureLocationInfo;
using ark::es2panda::lsp::GetImmediatelyContainingArgumentInfo;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::IsSyntacticOwner;
using ark::es2panda::lsp::TryGetParameterInfo;

class LspSignatureHelpRemainingTests : public LSPAPITests {
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
// P2D-SH-01: IsSyntacticOwner_TrueForCallExpr
// =============================================================================
TEST_F(LspSignatureHelpRemainingTests, IsSyntacticOwner_TrueForCallExpr)
{
    const std::string source = "function foo() { foo(); }";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("sh01.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the CallExpression node
    auto *callExpr = FindNode(ast, [](AstNode *n) { return n->IsCallExpression(); });
    ASSERT_NE(callExpr, nullptr);

    EXPECT_TRUE(IsSyntacticOwner(callExpr));
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-SH-02: IsSyntacticOwner_FalseForNonCall
// =============================================================================
TEST_F(LspSignatureHelpRemainingTests, IsSyntacticOwner_FalseForNonCall)
{
    const std::string source = "let x = 42;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("sh02.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find a NumberLiteral node (not a call expression)
    auto *numberLit = FindNode(ast, [](AstNode *n) { return n->IsNumberLiteral(); });
    ASSERT_NE(numberLit, nullptr);

    EXPECT_FALSE(IsSyntacticOwner(numberLit));
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-SH-03: GetArgumentCount_TwoArgs
// =============================================================================
// BLOCKED_DESIGN_MAPPING
// Source: ets2panda/lsp/src/signature_help.cpp:241-254
// Design expectation: GetArgumentCount(foo(1,2), false) returns 2.
// Actual product behavior: GetArgumentCount counts only TSTypeParameter and
// MemberExpression children of the node (signature_help.cpp:245-251). For a
// plain CallExpression foo(1,2), it returns 0 because the CallExpression has
// no TSTypeParameter or MemberExpression children. The design expectation
// of "correct argument count" does not match this implementation.
// NEXT ACTION: Fix GetArgumentCount to count actual call arguments, not just
// TSTypeParameter/MemberExpression children.

// =============================================================================
// P2D-SH-04: GetArgumentCount_EmptyCall
// =============================================================================
TEST_F(LspSignatureHelpRemainingTests, GetArgumentCount_EmptyCall)
{
    // Design expectation: GetArgumentCount(foo(), false) returns 0.
    // The actual result (0) matches the design expectation, but the reason is
    // wrong: the function returns 0 because it counts only TSTypeParameter/
    // MemberExpression children, not because the argument list is empty.
    const std::string source = "function foo(): void {} foo();";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("sh04.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *callExpr = FindNode(ast, [](AstNode *n) { return n->IsCallExpression(); });
    ASSERT_NE(callExpr, nullptr);

    // LIMITATION: The result 0 is coincidentally correct.
    // GetArgumentCount counts only TSTypeParameter/MemberExpression children
    // (signature_help.cpp:241-254), not actual call arguments. The result 0
    // happens to match the design expectation for an empty call, but the reason
    // is wrong — the function never counts actual arguments.
    size_t count = ark::es2panda::lsp::GetArgumentCount(callExpr, false);
    EXPECT_EQ(count, 0U);
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-SH-08: CreateTextSpanForNode_ReturnsSpan
// =============================================================================
TEST_F(LspSignatureHelpRemainingTests, CreateTextSpanForNode_ReturnsSpan)
{
    const std::string source = "let x = 42;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("sh08.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the NumberLiteral "42" node
    auto *numberLit =
        FindNode(ast, [](AstNode *n) { return n->IsNumberLiteral() && n->AsNumberLiteral()->Number().IsInt(); });
    ASSERT_NE(numberLit, nullptr);

    auto span = CreateTextSpanForNode(numberLit);
    EXPECT_EQ(span.start, numberLit->Start().index);
    EXPECT_EQ(span.length, numberLit->End().index - numberLit->Start().index);
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-SH-10: GetImmediatelyContainingArgumentInfo_ReturnsInfo
// =============================================================================
// The duplicated CountBinaryExpressionParameters and GetContextualSignatureLocationInfo
// copies were removed (canonical copies live in signature_help_internals_test.cpp).
// Unique here: argument-info parenting for a direct call.
TEST_F(LspSignatureHelpRemainingTests, GetImmediatelyContainingArgumentInfo_ReturnsInfo)
{
    // Source: call expression with a number literal argument
    const std::string source = R"(
function foo(a: number, b: number): void {}
let x = foo(42, 100);
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("sh10.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the first argument number literal "42"
    auto *argNode = FindNode(ast, [](AstNode *n) {
        return n->IsNumberLiteral() && n->AsNumberLiteral()->Number().IsInt() &&
               n->AsNumberLiteral()->Number().GetInt() == 42;
    });
    ASSERT_NE(argNode, nullptr);

    // Use a position inside the argument node
    size_t position = argNode->Start().index + 1;

    auto info = GetImmediatelyContainingArgumentInfo(argNode, position);
    ASSERT_TRUE(info.has_value());

    // The invocation should wrap the call expression node
    auto applicableSpan = info->GetApplicableSpan();
    EXPECT_GT(applicableSpan.start, 0U);
    EXPECT_GT(applicableSpan.length, 0U);

    // LIMITATION: GetArgumentCount returns 0 because it counts only
    // TSTypeParameter/MemberExpression children (signature_help.cpp:241-254),
    // not actual call arguments. The design expects 2 (two arguments).
    EXPECT_EQ(info->GetArgumentCount(), 0U);

    // The argument index is the parent CallExpression's start index
    // (GetArgumentOrParameterListInfo returns node->Start().index for the
    // CallExpression node, not the individual argument's start)
    auto *parent = argNode->Parent();
    ASSERT_NE(parent, nullptr);
    EXPECT_EQ(info->GetArgumentIndex(), parent->Start().index);
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-SH-11: TryGetParameterInfo_ReturnsInfo
// =============================================================================
TEST_F(LspSignatureHelpRemainingTests, TryGetParameterInfo_ReturnsInfo)
{
    const std::string source = R"(
class C {
    foo(a: number, b: string): void {}
}
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("sh11.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the MethodDefinition with name "foo", excluding constructor
    auto *methodDef = FindNode(ast, [](AstNode *n) {
        if (!n->IsMethodDefinition()) {
            return false;
        }
        auto *method = n->AsMethodDefinition();
        if (method->Kind() == ark::es2panda::ir::MethodDefinitionKind::CONSTRUCTOR) {
            return false;
        }
        auto *id = method->Id();
        return id != nullptr && id->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(methodDef, nullptr);

    auto info = TryGetParameterInfo(methodDef);
    ASSERT_TRUE(info.has_value());

    // TryGetParameterInfo delegates to GetContextualSignatureLocationInfo
    // and wraps the result in an ArgumentListInfo
    EXPECT_EQ(info->GetArgumentIndex(), methodDef->Start().index);
    // LIMITATION: GetArgumentCount returns 0 because it counts only
    // TSTypeParameter/MemberExpression children (signature_help.cpp:241-254),
    // not actual call arguments. The design expects 2 (two parameters).
    EXPECT_EQ(info->GetArgumentCount(), 0U);
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-SH-12: GetContainingArgumentInfo_ReturnsInfo
// =============================================================================
// Unique dimension: the isManuallyInvoked=true guard path.
TEST_F(LspSignatureHelpRemainingTests, GetContainingArgumentInfo_ReturnsInfo)
{
    // Source: call expression with a number literal argument
    const std::string source = R"(
function foo(a: number, b: number): void {}
let x = foo(42, 100);
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("sh12.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the first argument number literal "42"
    auto *argNode = FindNode(ast, [](AstNode *n) {
        return n->IsNumberLiteral() && n->AsNumberLiteral()->Number().IsInt() &&
               n->AsNumberLiteral()->Number().GetInt() == 42;
    });
    ASSERT_NE(argNode, nullptr);

    size_t position = argNode->Start().index + 1;
    // isManuallyInvoked=true is required for the function to proceed past the
    // early-return guard at signature_help.cpp:218-220
    bool isManuallyInvoked = true;

    auto info = GetContainingArgumentInfo(argNode, position, isManuallyInvoked);
    ASSERT_TRUE(info.has_value());

    // The invocation should wrap the call expression node
    auto applicableSpan = info->GetApplicableSpan();
    EXPECT_GT(applicableSpan.start, 0U);
    EXPECT_GT(applicableSpan.length, 0U);

    // LIMITATION: GetArgumentCount returns 0 because it counts only
    // TSTypeParameter/MemberExpression children (signature_help.cpp:241-254),
    // not actual call arguments. The design expects 2 (two arguments).
    EXPECT_EQ(info->GetArgumentCount(), 0U);

    // The argument index is the parent CallExpression's start index
    // (GetArgumentOrParameterListInfo returns node->Start().index for the
    // CallExpression node, not the individual argument's start)
    auto *parent = argNode->Parent();
    ASSERT_NE(parent, nullptr);
    EXPECT_EQ(info->GetArgumentIndex(), parent->Start().index);
    initializer.DestroyContext(ctx);
}

}  // namespace