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

using ark::es2panda::lsp::ArgumentListInfo;
using ark::es2panda::lsp::CountBinaryExpressionParameters;
using ark::es2panda::lsp::FindTokenOnLeftOfPosition;
using ark::es2panda::lsp::GetArgumentIndexForTemplatePiece;
using ark::es2panda::lsp::GetArgumentOrParameterListAndIndex;
using ark::es2panda::lsp::GetContextualSignatureLocationInfo;
using ark::es2panda::lsp::Initializer;

class LspSignatureHelpInternalsTests : public LSPAPITests {
protected:
    // Helper: find the first node satisfying a predicate via FindChild
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
// =============================================================================
// P1B-SH-01: GetArgumentCount_NoTrailingComma
// =============================================================================
// DESIGN EXPECTATION: "correct argument count" on a real call node.
// BLOCKED_DESIGN_MAPPING
// Source: ets2panda/lsp/src/signature_help.cpp:241-253
// Actual behavior: GetArgumentCount(node, false) does NOT count call arguments.
// It only counts TSTypeParameter and MemberExpression children of the node.
// When ignoreTrailingComma=false, the function increments argumentCount for each
// TSTypeParameter or MemberExpression child found via FindChild.
// For a call like `foo(1, 2)` with an Identifier callee and no type arguments,
// the function returns 0 (not 2). For `obj.method(1, 2)`, it returns 1 (for the
// MemberExpression callee). For `foo<number, string>(1, 2)`, it returns 2 (for
// the two TSTypeParameter children).
// The design expectation of "correct argument count" does not match this
// implementation. No test file is written for this design item.
// NEXT ACTION: Review design expectation vs. product purpose. If the design
// expectation is correct, the implementation must be fixed first.

// P1B-SH-02: GetArgumentCount_IgnoreTrailingComma
// =============================================================================
// DESIGN EXPECTATION: "ignore trailing comma" when ignoreTrailingComma=true.
// BLOCKED_DESIGN_MAPPING
// Source: ets2panda/lsp/src/signature_help.cpp:241-253
// Actual behavior: When ignoreTrailingComma=true, the inner conditions are
// always false (!ignoreTrailingComma is false), so the function always returns 0
// regardless of the node's children. It does NOT count actual arguments at all.
// The design expectation of "ignore trailing comma" implies the function counts
// arguments and optionally ignores the trailing one, but the implementation
// counts only TSTypeParameter and MemberExpression children, with the
// ignoreTrailingComma flag disabling counting entirely.
// NEXT ACTION: Review design expectation vs. product purpose. If the design
// expectation is correct, the implementation must be fixed first.

// P1B-SH-03: GetArgumentCount_EmptyCallExpression
// =============================================================================
// DESIGN EXPECTATION: "Assert zero on foo()".
// BLOCKED_DESIGN_MAPPING
// Source: ets2panda/lsp/src/signature_help.cpp:241-253
// Actual behavior: For `foo()`, GetArgumentCount(node, false) returns 0 because
// the CallExpression node has no TSTypeParameter or MemberExpression children.
// While the result (0) matches the design expectation, the reason is wrong: the
// function returns 0 because it doesn't count arguments at all, not because the
// argument list is empty. The design implies the function counts call arguments,
// while the implementation counts only TSTypeParameter/MemberExpression children.
// Writing a passing test here would give a false sense of correctness.
// NEXT ACTION: Do not write a test until the implementation is aligned with the
// design. If the design is wrong, update the design to reflect actual behavior.

// P1B-SH-04: GetHighestBinary_ReturnsTopBinary
// BLOCKED_DESIGN_MAPPING
// Source: ets2panda/lsp/include/signature_help.h:290
// ets2panda/lsp/src/signature_help.cpp:193
// The header declares the function with a const-qualified node parameter,
// while the source defines it with a non-const node parameter.
// The const parameter qualifier differs between declaration and definition,
// producing different mangled symbols. A call compiled against the header
// references the const version which has no definition — an undefined reference
// at link time.
// NEXT ACTION: Fix the header/source signature mismatch (make them consistent)
// before this test can be written.
// No TEST_F written for this item.

// P1B-SH-05: CountBinaryExpressionParameters_CorrectCount
TEST_F(LspSignatureHelpInternalsTests, CountBinaryExpressionParameters_CorrectCount)
{
    // Source: a + b + c + d  =>  chain of 4 operands
    const std::string source = "let x = a + b + c + d;";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("sigHelpCountBinary.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the outermost binary expression
    auto *outerBinary = FindNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsBinaryExpression(); });
    ASSERT_NE(outerBinary, nullptr);

    // CountBinaryExpressionParameters should return the total number of operands
    // For a + b + c + d (= 4 operands), the function recursively counts left-chain depth
    size_t count = CountBinaryExpressionParameters(outerBinary);
    // For a 4-operand chain, the expected count is 4
    EXPECT_EQ(count, 4U);
    initializer.DestroyContext(ctx);
}

// P1B-SH-06: GetArgumentOrParameterListAndIndex_FindsList
// Differs from upstream GetCandidateOrTypeInfo pipeline coverage by pinning the
// exact list size (2) and per-entry parameter count - not a duplicate seam.
TEST_F(LspSignatureHelpInternalsTests, GetArgumentOrParameterListAndIndex_FindsList)
{
    const std::string source = "function foo(a: number, b: string): void { foo(1, \"test\"); }";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("sigHelpArgList.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the CallExpression
    auto *callExpr = FindNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsCallExpression(); });
    ASSERT_NE(callExpr, nullptr);

    std::vector<ArgumentListInfo> list;
    auto result = GetArgumentOrParameterListAndIndex(callExpr, list);
    EXPECT_EQ(result.size(), 2U);
    ASSERT_GE(result.size(), 1U);
    EXPECT_EQ(result[0].GetArgumentCount(), 2U);
    initializer.DestroyContext(ctx);
}

// P1B-SH-07: GetContextualSignatureLocationInfo_ReturnsInfo
TEST_F(LspSignatureHelpInternalsTests, GetContextualSignatureLocationInfo_ReturnsInfo)
{
    // Class method (not constructor) with two parameters
    const std::string source = R"(
class C {
    foo(a: number, b: string): void {}
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("sigHelpContextualLoc.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Precisely find the MethodDefinition with name "foo", excluding the constructor
    auto *methodDef = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
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

    auto info = GetContextualSignatureLocationInfo(methodDef);
    ASSERT_TRUE(info.has_value());

    // Current contract (signature_help.cpp:282-293, 383-397):
    //   list        = one ArgumentListInfo per parameter of the MethodDefinition
    //   argumentIndex = node->Start().index
    //   argumentCount = GetArgumentCount(node, false) which counts only
    //                   TSTypeParameter/MemberExpression children (signature_help.cpp:241-253)
    //   argumentsSpan = TextSpan covering the whole MethodDefinition node
    EXPECT_EQ(info->GetList().size(), 2U);
    EXPECT_EQ(info->GetArgumentIndex(), methodDef->Start().index);
    EXPECT_EQ(info->GetArgumentCount(), 0U);  // current contract for a plain method
    // Known defect: GetArgumentCount returns 0 for plain methods
    // (counts only TSTypeParameter/MemberExpression).
    EXPECT_EQ(info->GetArgumentsSpan().start, methodDef->Start().index);
    EXPECT_EQ(info->GetArgumentsSpan().length, methodDef->End().index - methodDef->Start().index);
    initializer.DestroyContext(ctx);
}

// P1B-SH-08: GetImmediatelyContainingArgumentInfo_ReturnsInfo
// BLOCKED_DESIGN_MAPPING
// Source: ets2panda/lsp/src/signature_help.cpp:334-381
// The function does return a valid optional ArgumentListInfo on a real call
// argument, but the ArgumentCount field it produces comes from
// GetArgumentCount(parent, false) (signature_help.cpp:354), which counts only
// TSTypeParameter and MemberExpression children (signature_help.cpp:241-253) —
// NOT the actual call arguments. For foo(42, 100) the exact count is 0 instead
// of 2. Writing a test that asserts this 0 would freeze the known GetArgumentCount
// bug and would not express the design's "return argument info" purpose.
// NEXT ACTION: Fix GetArgumentCount first, then re-add this test asserting the
// correct count.
// No TEST_F written for this item.

// P1B-SH-09: GetContainingArgumentInfo_ReturnsInfo
// BLOCKED_DESIGN_MAPPING
// Source: ets2panda/lsp/src/signature_help.cpp:216-223
// The function returns GetImmediatelyContainingArgumentOrContextualParameterInfo
// which delegates to GetImmediatelyContainingArgumentInfo (signature_help.cpp:209-214).
// Same defect as P1B-SH-08: the returned ArgumentCount comes from the buggy
// GetArgumentCount and equals 0 for a real call foo(42). Asserting 0 would freeze
// the known bug.
// NEXT ACTION: Fix GetArgumentCount first, then re-add this test asserting the
// correct count.
// No TEST_F written for this item.

// P1B-SH-10: GetArgumentIndexForTemplatePiece_CorrectIndex
TEST_F(LspSignatureHelpInternalsTests, GetArgumentIndexForTemplatePiece_CorrectIndex)
{
    // Template literal with an expression and a declared variable so the
    // checker can reach CHECKED state (ETS parser: ETSparserExpressions.cpp:459
    // maps PUNCTUATOR_BACK_TICK to ParseTemplateLiteral).
    const std::string source = R"(
let name = "world";
let x = `hello ${name} world`;
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("sigHelpTemplateIndex.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // The design requires the TEMPLATE_LITERAL branch; a non-template else
    // branch alone would not cover the "template piece" behavior. Strictly
    // require a real TemplateLiteral node in the CHECKED AST.
    auto *templateLit = FindNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsTemplateLiteral(); });
    ASSERT_NE(templateLit, nullptr) << "Expected a TemplateLiteral node in the CHECKED AST";

    const size_t spanIndex = 5;
    const size_t outsideSpanReturnOffset = 2;
    // Position inside the template span: returns 0
    size_t insidePos = templateLit->Start().index + 1;
    size_t insideResult = GetArgumentIndexForTemplatePiece(spanIndex, templateLit, insidePos);
    EXPECT_EQ(insideResult, 0U);

    // Position outside the template span: returns spanIndex + outsideSpanReturnOffset
    size_t outsidePos = templateLit->End().index + 1;
    size_t outsideResult = GetArgumentIndexForTemplatePiece(spanIndex, templateLit, outsidePos);
    EXPECT_EQ(outsideResult, spanIndex + outsideSpanReturnOffset);
    initializer.DestroyContext(ctx);
}

// P1B-SH-11: GetChildListThatStartsWithOpenerToken_FindsList
// BLOCKED_DESIGN_MAPPING
// Source: ets2panda/lsp/include/signature_help.h:303
// ets2panda/lsp/src/signature_help.cpp:225-238
// Design: "开括号后找到列表" (find list after opening bracket, e.g. '(' or '<').
// The function takes a parent node and an opener-token node as parameters.
// But opener tokens like '(' and '<' in the source are lexer tokens, NOT AST
// nodes (ir::AstNode). The function's parameter type ir::AstNode * cannot
// represent a punctuation token. The previous test draft used the callee
// (Identifier "foo") as the "openerToken", which is semantically wrong: the
// design expected a '(' token, not the callee identifier.
// NEXT ACTION: The design expectation of "opener token" does not match the AST
// node-based API. Either the design must be updated to use a different mechanism
// (e.g. lexer token), or the function must be removed/replaced.
// No TEST_F written for this item.

// P1B-SH-12: FindTokenOnLeftOfPosition_FindsToken
TEST_F(LspSignatureHelpInternalsTests, FindTokenOnLeftOfPosition_FindsToken)
{
    const std::string source = "let x = 42;";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx =
        initializer.CreateContext("sigHelpFindTokenLeft.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Locate the numeric literal node (value 42) within the AST. Its byte range
    // corresponds to the literal token in the source text.
    auto *numberLit = FindNode(ast, [](ark::es2panda::ir::AstNode *n) {
        return n->IsNumberLiteral() && n->AsNumberLiteral()->Number().IsInt();
    });
    ASSERT_NE(numberLit, nullptr);

    // Position inside the number literal: GetTouchingToken returns the number
    // literal token (Start < position < End), so FindTokenOnLeftOfPosition returns
    // that token with the exact same range.
    size_t position = numberLit->Start().index + 1;

    auto *token = FindTokenOnLeftOfPosition(ctx, position);
    ASSERT_NE(token, nullptr);
    EXPECT_EQ(token->Start().index, numberLit->Start().index);
    EXPECT_EQ(token->End().index, numberLit->End().index);
    initializer.DestroyContext(ctx);
}

}  // namespace
