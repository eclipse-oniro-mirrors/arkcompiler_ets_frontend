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
#include <memory>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/api.h"
#include "lsp/include/quick_info.h"
#include "public/es2panda_lib.h"
#include "compiler/lowering/util.h"
#include "varbinder/scope.h"
#include "varbinder/declaration.h"
#include "util/diagnostic.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::ir::ModifierFlags;
using ark::es2panda::lsp::ComputeRelativeImportPath;
using ark::es2panda::lsp::CreateDiagnosticForError;
using ark::es2panda::lsp::CreateDiagnosticWithoutFile;
using ark::es2panda::lsp::FindAllChild;
using ark::es2panda::lsp::FindAncestor;
using ark::es2panda::lsp::FindDeclInFunctionScope;
using ark::es2panda::lsp::FindDeclInGlobalScope;
using ark::es2panda::lsp::FindDeclInScopeWithFallback;
using ark::es2panda::lsp::FindRightToken;
using ark::es2panda::lsp::GetDefinitionAtPositionImpl;
using ark::es2panda::lsp::GetDocumentHighlightsImpl;
using ark::es2panda::lsp::GetIdentFromNewClassExprPart;
using ark::es2panda::lsp::GetIdentifier;
using ark::es2panda::lsp::GetIdentifierFromSuper;
using ark::es2panda::lsp::GetOptimumNodeByRange;
using ark::es2panda::lsp::GetOriginalNode;
using ark::es2panda::lsp::GetOwner;
using ark::es2panda::lsp::GetRangeOfEnclosingComment;
using ark::es2panda::lsp::GetTokenTypes;
using ark::es2panda::lsp::GetTouchingToken;
using ark::es2panda::lsp::GetTouchingTokenByRange;
using ark::es2panda::lsp::GetTouchingTokenForIdentifier;
using ark::es2panda::lsp::GetTouchingTokenRightMatch;
using ark::es2panda::lsp::MakeDiagnosticReferences;
using ark::es2panda::lsp::NodeHasTokens;
using ark::es2panda::lsp::ResolveIdentifier;
using ark::es2panda::lsp::TextRange;
// api.h declares these in the global (non-namespaced) scope
using ::CommentKind;
using ::CommentRange;
using ::DiagnosticReferences;
using ::DiagnosticSeverity;
using ::DocumentHighlights;

class LspInternalApiHelpersTests : public LSPAPITests {
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

// P2C-IA-01: GetTouchingTokenRightMatch_ReturnsCorrectly
// Cluster: GetTouchingTokenRightMatch/ByRange/ForIdentifier/GetOptimumNodeByRange have
// no prior direct coverage; the miss branches live in get_adjusted_location_test_2.cpp.
TEST_F(LspInternalApiHelpersTests, GetTouchingTokenRightMatch_ReturnsCorrectly)
{
    const std::string source = R"(
function foo() {}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia01.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    // Position at 'foo' identifier
    size_t pos = source.find("foo");
    ASSERT_NE(pos, std::string::npos);

    AstNode *token = GetTouchingTokenRightMatch(ctx, pos);
    ASSERT_NE(token, nullptr);
    // Should find the identifier node at the position
    EXPECT_TRUE(token->IsIdentifier() || token->IsFunctionExpression());
    initializer.DestroyContext(ctx);
}

// P2C-IA-02: GetTouchingTokenByRange_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, GetTouchingTokenByRange_ReturnsCorrectly)
{
    const std::string source = R"(
function foo() {}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia02.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    size_t pos = source.find("foo");
    ASSERT_NE(pos, std::string::npos);
    TextRange span = {pos, pos + 3};  // "foo"

    AstNode *token = GetTouchingTokenByRange(ctx, span, false);
    // Should find a node that covers the range
    ASSERT_NE(token, nullptr);
    EXPECT_LE(token->Start().index, span.pos);
    EXPECT_GE(token->End().index, span.end);
    initializer.DestroyContext(ctx);
}

// P2C-IA-03: GetTouchingTokenForIdentifier_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, GetTouchingTokenForIdentifier_ReturnsCorrectly)
{
    const std::string source = R"(
function foo() {}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia03.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    size_t pos = source.find("foo");
    ASSERT_NE(pos, std::string::npos);

    AstNode *token = GetTouchingTokenForIdentifier(ctx, pos, false);
    // Should find the identifier node at the position
    ASSERT_NE(token, nullptr);
    initializer.DestroyContext(ctx);
}

// P2C-IA-04: GetOptimumNodeByRange_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, GetOptimumNodeByRange_ReturnsCorrectly)
{
    const std::string source = R"(
function foo() {}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia04.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    size_t pos = source.find("foo");
    ASSERT_NE(pos, std::string::npos);
    TextRange span = {pos, pos + 3};

    AstNode *optimum = GetOptimumNodeByRange(ast, span);
    // Should find the optimum node for the range
    ASSERT_NE(optimum, nullptr);
    initializer.DestroyContext(ctx);
}

// P2C-IA-05: GetIdentifierFromSuper_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, GetIdentifierFromSuper_ReturnsCorrectly)
{
    const std::string source = R"(
class Base {}
class Derived extends Base {}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia05.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the ClassDefinition for Derived and get its super
    AstNode *derivedClass = FindNode(ast, [](AstNode *n) {
        return n->IsClassDefinition() && n->AsClassDefinition()->Ident() != nullptr &&
               n->AsClassDefinition()->Ident()->Name().Is("Derived");
    });
    ASSERT_NE(derivedClass, nullptr);

    auto *super = derivedClass->AsClassDefinition()->Super();
    if (super != nullptr) {
        AstNode *ident = GetIdentifierFromSuper(super);
        // The super reference should resolve to an identifier "Base"
        EXPECT_NE(ident, nullptr);
        if (ident != nullptr && ident->IsIdentifier()) {
            EXPECT_EQ(ident->AsIdentifier()->Name(), "Base");
        }
    }
    initializer.DestroyContext(ctx);
}

// P2C-IA-06: GetOriginalNode_ReturnsCorrectly
// Identity case only; OriginalNode chain unwrapping requires a lowered node that a
// PARSED context cannot produce.
TEST_F(LspInternalApiHelpersTests, GetOriginalNode_ReturnsCorrectly)
{
    const std::string source = R"(
let x = 1;
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia06.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *varDecl = FindNode(ast, [](AstNode *n) { return n->IsVariableDeclaration(); });
    ASSERT_NE(varDecl, nullptr);

    AstNode *original = GetOriginalNode(varDecl);
    // GetOriginalNode traverses the OriginalNode chain
    ASSERT_NE(original, nullptr);
    // For a node without OriginalNode set, it should return the same node
    EXPECT_TRUE(original == varDecl || original->IsVariableDeclaration());
    initializer.DestroyContext(ctx);
}

// P2C-IA-07: GetDefinitionAtPositionImpl_ReturnsCorrectly
// Two-parameter (nullptr-resolver) overload; public getDefinitionAtPosition tests
// drive the 3-param resolver overload.
TEST_F(LspInternalApiHelpersTests, GetDefinitionAtPositionImpl_ReturnsCorrectly)
{
    const std::string source = R"(
let x = 1;
let y = x;
)";
    ark::es2panda::lsp::Initializer initializer;
    std::vector<std::string> files = {"ia07.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // Position at the second 'x' in "let y = x"
    size_t pos = source.rfind("x");
    ASSERT_NE(pos, std::string::npos);

    auto result = GetDefinitionAtPositionImpl(ctx, pos);
    // Should return a non-null node and a non-empty name
    EXPECT_NE(result.first, nullptr);
    EXPECT_FALSE(result.second.Empty());
    initializer.DestroyContext(ctx);
}

// P2C-IA-08: GetDocumentHighlightsImpl_ReturnsCorrectly
// Querying the declaration of `x` must return the definition span (WRITTEN_REFERENCE) plus the
// read reference in `let y = x` (REFERENCE), sorted by start, for the queried file only.
TEST_F(LspInternalApiHelpersTests, GetDocumentHighlightsImpl_ReturnsCorrectly)
{
    const std::string source = R"(
let x = 1;
let y = x;
)";
    ark::es2panda::lsp::Initializer initializer;
    std::vector<std::string> files = {"ia08.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    // GetDocumentHighlightsImpl reads the symbol reference index; build it for this context first.
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(ctx));

    // Position at the first 'x' (the declaration)
    size_t pos = source.find("x");
    ASSERT_NE(pos, std::string::npos);
    // Position of the read reference in "let y = x"
    size_t usePos = source.rfind("x");
    ASSERT_NE(usePos, std::string::npos);
    ASSERT_NE(usePos, pos);

    DocumentHighlights highlights = GetDocumentHighlightsImpl(ctx, pos);
    initializer.DestroyContext(ctx);

    // Exact structure: the queried file with the definition span followed by the use span.
    EXPECT_EQ(highlights.fileName_, filePaths[0]);
    ASSERT_EQ(highlights.highlightSpans_.size(), 2U);
    EXPECT_EQ(highlights.highlightSpans_[0].textSpan_.start, pos);
    EXPECT_EQ(highlights.highlightSpans_[0].textSpan_.length, 1U);
    EXPECT_EQ(highlights.highlightSpans_[0].kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    EXPECT_EQ(highlights.highlightSpans_[1].textSpan_.start, usePos);
    EXPECT_EQ(highlights.highlightSpans_[1].textSpan_.length, 1U);
    EXPECT_EQ(highlights.highlightSpans_[1].kind_, HighlightSpanKind::REFERENCE);
}

// P2C-IA-09: GetIdentifier_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, GetIdentifier_ReturnsCorrectly)
{
    const std::string source = R"(
let x = 1;
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia09.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the 'x' identifier
    AstNode *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("x"); });
    ASSERT_NE(ident, nullptr);

    // GetIdentifier should return the identifier node itself
    AstNode *result = GetIdentifier(ident);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->IsIdentifier());

    // GetIdentifier with nullptr returns nullptr
    EXPECT_EQ(GetIdentifier(nullptr), nullptr);
    initializer.DestroyContext(ctx);
}

// P2C-IA-10: ResolveIdentifier_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, ResolveIdentifier_ReturnsCorrectly)
{
    const std::string source = R"(
let x = 1;
)";
    ark::es2panda::lsp::Initializer initializer;
    std::vector<std::string> files = {"ia10.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the 'x' identifier
    AstNode *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("x"); });
    ASSERT_NE(ident, nullptr);

    auto *var = ResolveIdentifier(ident->AsIdentifier());
    // In CHECKED state, the variable should be resolved
    EXPECT_NE(var, nullptr);
    if (var != nullptr) {
        EXPECT_NE(var->Declaration(), nullptr);
    }
    initializer.DestroyContext(ctx);
}

// P2C-IA-11: FindRightToken_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, FindRightToken_ReturnsCorrectly)
{
    const std::string source = R"(
let x = 1;
let y = 2;
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia11.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Get children of AST
    std::vector<AstNode *> children;
    ast->Iterate([&children](AstNode *child) { children.push_back(child); });

    // Find the position after 'x' and look for the next token
    size_t pos = source.find("x");
    ASSERT_NE(pos, std::string::npos);

    AstNode *rightToken = FindRightToken(pos, children);
    // Should find a token to the right of the position
    ASSERT_NE(rightToken, nullptr);
    EXPECT_GE(rightToken->Start().index, pos);
    initializer.DestroyContext(ctx);
}

// P2C-IA-12: GetOwner_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, GetOwner_ReturnsCorrectly)
{
    const std::string source = R"(
let x = 1;
)";
    ark::es2panda::lsp::Initializer initializer;
    std::vector<std::string> files = {"ia12.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the 'x' identifier
    AstNode *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("x"); });
    ASSERT_NE(ident, nullptr);

    AstNode *owner = GetOwner(ident);
    // GetOwner should return the identifier of the declaration's node
    ASSERT_NE(owner, nullptr);
    EXPECT_TRUE(owner->IsIdentifier());
    initializer.DestroyContext(ctx);
}

// P2C-IA-13: NodeHasTokens_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, NodeHasTokens_ReturnsCorrectly)
{
    const std::string source = R"(
let x = 1;
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia13.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // A real identifier has tokens (start != end)
    AstNode *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("x"); });
    ASSERT_NE(ident, nullptr);
    EXPECT_TRUE(NodeHasTokens(ident));
    EXPECT_NE(ident->Start().index, ident->End().index);
    // LIMITATION: the false branch (start == end, internal_api.cpp:479-481) cannot be
    // exercised because all AST nodes produced from real parsed source have non-zero
    // ranges, and no synthetic zero-range node is constructed (that would fake
    // unreachable product state). Only the true branch is covered by this test.
    initializer.DestroyContext(ctx);
}

// P2C-IA-14: FindAllChild_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, FindAllChild_ReturnsCorrectly)
{
    const std::string source = R"(
let x = 1;
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia14.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    std::vector<AstNode *> results;
    FindAllChild(
        ast, [](AstNode *node) { return node->IsIdentifier(); }, results);
    // Should find at least one identifier node ('x')
    EXPECT_FALSE(results.empty());
    bool foundX = false;
    for (auto *n : results) {
        if (n->IsIdentifier() && n->AsIdentifier()->Name().Is("x")) {
            foundX = true;
            break;
        }
    }
    EXPECT_TRUE(foundX);
    initializer.DestroyContext(ctx);
}

// P2C-IA-15: FindAncestor_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, FindAncestor_ReturnsCorrectly)
{
    const std::string source = R"(
function test() {
    let x = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia15.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the 'x' identifier
    AstNode *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("x"); });
    ASSERT_NE(ident, nullptr);

    // Find ancestor that is a VariableDeclaration
    AstNode *ancestor = FindAncestor(ident, [](AstNode *n) { return n->IsVariableDeclaration(); });
    ASSERT_NE(ancestor, nullptr);
    EXPECT_TRUE(ancestor->IsVariableDeclaration());

    // Find ancestor that doesn't exist (should return nullptr)
    AstNode *noAncestor = FindAncestor(ident, [](AstNode *n) { return n->IsClassDeclaration(); });
    EXPECT_EQ(noAncestor, nullptr);
    initializer.DestroyContext(ctx);
}

// P2C-IA-16: GetIdentFromNewClassExprPart_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, GetIdentFromNewClassExprPart_ReturnsCorrectly)
{
    const std::string source = R"(
class MyClass {}
let obj = new MyClass();
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia16.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the ETSNewClassInstanceExpression
    AstNode *newExpr = FindNode(ast, [](AstNode *n) { return n->IsETSNewClassInstanceExpression(); });
    ASSERT_NE(newExpr, nullptr);

    auto *ident = GetIdentFromNewClassExprPart(newExpr->AsExpression());
    // Should find the identifier 'MyClass' from the new expression's type reference
    EXPECT_NE(ident, nullptr);
    if (ident != nullptr) {
        EXPECT_EQ(ident->Name(), "MyClass");
    }
    initializer.DestroyContext(ctx);
}

// P2C-IA-17: FindDeclInFunctionScope_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, FindDeclInFunctionScope_ReturnsCorrectly)
{
    const std::string source = R"(
function test() {
    let x = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    std::vector<std::string> files = {"ia17.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the 'x' identifier
    AstNode *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("x"); });
    ASSERT_NE(ident, nullptr);

    auto *scope = ark::es2panda::compiler::NearestScope(ident);
    ASSERT_NE(scope, nullptr);

    auto *decl = FindDeclInFunctionScope(scope, ident->AsIdentifier()->Name());
    // Should find the declaration of 'x' in the function scope
    EXPECT_NE(decl, nullptr);
    if (decl != nullptr) {
        EXPECT_EQ(decl->Name(), "x");
    }
    initializer.DestroyContext(ctx);
}

// P2C-IA-18: FindDeclInGlobalScope_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, FindDeclInGlobalScope_ReturnsCorrectly)
{
    const std::string source = R"(
let x = 1;
)";
    ark::es2panda::lsp::Initializer initializer;
    std::vector<std::string> files = {"ia18.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the 'x' identifier
    AstNode *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("x"); });
    ASSERT_NE(ident, nullptr);

    auto *scope = ark::es2panda::compiler::NearestScope(ident);
    ASSERT_NE(scope, nullptr);

    auto *decl = FindDeclInGlobalScope(scope, ident->AsIdentifier()->Name());
    // Should find the declaration of 'x' in the global scope
    EXPECT_NE(decl, nullptr);
    if (decl != nullptr) {
        EXPECT_EQ(decl->Name(), "x");
    }
    initializer.DestroyContext(ctx);
}

// P2C-IA-19: FindDeclInScopeWithFallback_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, FindDeclInScopeWithFallback_ReturnsCorrectly)
{
    const std::string source = R"(
let x = 1;
function test() {
    let y = x;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    std::vector<std::string> files = {"ia19.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    auto *ast = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the 'y' identifier (local to function)
    AstNode *yIdent = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("y"); });
    ASSERT_NE(yIdent, nullptr);

    auto *scope = ark::es2panda::compiler::NearestScope(yIdent);
    ASSERT_NE(scope, nullptr);

    // FindDeclInScopeWithFallback should find 'y' in function scope first
    auto *decl = FindDeclInScopeWithFallback(scope, yIdent->AsIdentifier()->Name());
    EXPECT_NE(decl, nullptr);
    if (decl != nullptr) {
        EXPECT_EQ(decl->Name(), "y");
    }
    initializer.DestroyContext(ctx);
}

// P2C-IA-20: ComputeRelativeImportPath_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, ComputeRelativeImportPath_ReturnsCorrectly)
{
    // Same directory
    std::string result = ComputeRelativeImportPath("/project/src/a.ets", "/project/src/b.ets");
    EXPECT_EQ(result, "./b");

    // Subdirectory
    result = ComputeRelativeImportPath("/project/src/a.ets", "/project/src/sub/c.ets");
    EXPECT_EQ(result, "./sub/c");

    // Parent directory
    result = ComputeRelativeImportPath("/project/src/sub/a.ets", "/project/src/b.ets");
    EXPECT_EQ(result, "../b");

    // Grandparent directory
    result = ComputeRelativeImportPath("/project/src/sub/deep/a.ets", "/project/src/b.ets");
    EXPECT_EQ(result, "../../b");

    // Different branch
    result = ComputeRelativeImportPath("/project/src/a.ets", "/project/lib/b.ets");
    EXPECT_EQ(result, "../lib/b");

    // .ets extension stripped
    result = ComputeRelativeImportPath("/project/src/a.ets", "/project/src/b.ets");
    EXPECT_EQ(result, "./b");

    // .d.ets extension stripped
    result = ComputeRelativeImportPath("/project/src/a.ets", "/project/src/b.d.ets");
    EXPECT_EQ(result, "./b");

    // .ts extension stripped
    result = ComputeRelativeImportPath("/project/src/a.ets", "/project/src/b.ts");
    EXPECT_EQ(result, "./b");

    // Same file path
    result = ComputeRelativeImportPath("/project/src/a.ets", "/project/src/a.ets");
    EXPECT_EQ(result, "./a");
}

// P2C-IA-21: GetTokenTypes_ReturnsCorrectly
// Public fixtures reach only the empty-modifiers case (node_token_offset_boundary_test_1.cpp);
// this pins the static/public modifier string path.
TEST_F(LspInternalApiHelpersTests, GetTokenTypes_ReturnsCorrectly)
{
    // No flags
    std::string result = GetTokenTypes(ModifierFlags::NONE);
    EXPECT_TRUE(result.empty());

    // Single flag
    result = GetTokenTypes(ModifierFlags::STATIC);
    EXPECT_EQ(result, "static");

    // Multiple flags
    result = GetTokenTypes(ModifierFlags::PUBLIC | ModifierFlags::STATIC);
    EXPECT_EQ(result, "static public");

    // Export flag
    result = GetTokenTypes(ModifierFlags::EXPORT);
    EXPECT_EQ(result, "export");

    // Async flag
    result = GetTokenTypes(ModifierFlags::ASYNC);
    EXPECT_EQ(result, "async");

    // Private flag
    result = GetTokenTypes(ModifierFlags::PRIVATE);
    EXPECT_EQ(result, "private");
}

// P2C-IA-22: MakeDiagnosticReferences_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, MakeDiagnosticReferences_ReturnsCorrectly)
{
    // Create a context with a known source file name
    ark::es2panda::lsp::Initializer initializer;
    const std::string fileName = "ia22_test.ets";
    const std::string source = "let x = 1;";
    es2panda_Context *ctx = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    // Create a diagnostic storage with a diagnostic that matches the file name
    ark::es2panda::util::DiagnosticStorage storage;
    auto diag = std::make_shared<ark::es2panda::util::ThrowableDiagnostic>(ark::es2panda::util::DiagnosticType::WARNING,
                                                                           "test warning", fileName, 1, 1);
    storage.push_back(diag);

    DiagnosticReferences result;
    MakeDiagnosticReferences(ctx, storage, result);
    // Should produce at least one diagnostic since the file name matches
    EXPECT_GE(result.diagnostic.size(), 1U);
    if (!result.diagnostic.empty()) {
        EXPECT_EQ(result.diagnostic[0].message_, "test warning");
    }
    initializer.DestroyContext(ctx);
}

// P2C-IA-23: GetRangeOfEnclosingComment_ReturnsCorrectly
// Direct CommentRange seam vs the public GetSpanOfEnclosingComment wrapper tested in
// get_span_of_enclosing_comment_test.cpp.
TEST_F(LspInternalApiHelpersTests, GetRangeOfEnclosingComment_ReturnsCorrectly)
{
    const std::string source = R"(
let x = 1; // some comment
let y = 2;
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ia23.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    // Position inside the comment
    size_t commentPos = source.find("// some comment");
    ASSERT_NE(commentPos, std::string::npos);
    size_t posInside = commentPos + 5;  // inside "some"

    CommentRange result;
    GetRangeOfEnclosingComment(ctx, posInside, &result);
    // The comment range should be found
    EXPECT_EQ(result.pos_, commentPos);
    EXPECT_EQ(result.kind_, CommentKind::SINGLE_LINE);

    // Position outside any comment (on a token)
    size_t tokenPos = source.find("x");
    ASSERT_NE(tokenPos, std::string::npos);
    CommentRange result2(0, 0, CommentKind::SINGLE_LINE);
    GetRangeOfEnclosingComment(ctx, tokenPos, &result2);
    // The function returns early when the position is on a token (IsToken true),
    // so the result struct should remain unchanged (0,0,SINGLE_LINE).
    EXPECT_EQ(result2.pos_, 0U);
    EXPECT_EQ(result2.end_, 0U);
    initializer.DestroyContext(ctx);
}

// P2C-IA-24: CreateDiagnosticForError_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, CreateDiagnosticForError_ReturnsCorrectly)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string fileName = "ia24_test.ets";
    const std::string source = "let x = 1;";
    es2panda_Context *ctx = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    // Use PLUGIN_ERROR type to avoid the static_cast<util::Diagnostic*> in CreateCodeForDiagnostic
    ark::es2panda::util::ThrowableDiagnostic error(ark::es2panda::util::DiagnosticType::PLUGIN_ERROR,
                                                   "plugin error message", fileName, 1, 1);

    ::Diagnostic diag = CreateDiagnosticForError(ctx, error);
    // Verify the diagnostic fields
    EXPECT_EQ(diag.message_, "plugin error message");
    EXPECT_EQ(diag.severity_, DiagnosticSeverity::Error);
    initializer.DestroyContext(ctx);
}

// P2C-IA-25: CreateDiagnosticWithoutFile_ReturnsCorrectly
TEST_F(LspInternalApiHelpersTests, CreateDiagnosticWithoutFile_ReturnsCorrectly)
{
    // Use a diagnostic type that doesn't need file context
    ark::es2panda::util::ThrowableDiagnostic error(ark::es2panda::util::DiagnosticType::WARNING, "warning without file",
                                                   "", 0, 0);

    ::Diagnostic diag = CreateDiagnosticWithoutFile(error);
    // Verify the diagnostic fields
    EXPECT_EQ(diag.message_, "warning without file");
    EXPECT_EQ(diag.severity_, DiagnosticSeverity::Warning);
    // Source should be empty for CreateDiagnosticWithoutFile
    EXPECT_TRUE(diag.source_.empty());
}

}  // namespace
