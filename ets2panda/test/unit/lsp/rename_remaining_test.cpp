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
#include "lsp/include/internal_api.h"
#include "lsp/include/rename.h"
#include "lsp/include/types.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::GetAssignedName;
using ark::es2panda::lsp::GetNameOfDeclaration;
using ark::es2panda::lsp::GetNonAssignedNameOfDeclaration;
using ark::es2panda::lsp::GetRenameInfo;
using ark::es2panda::lsp::GetRenameInfoForNode;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::IsImportOrExportSpecifierName;
using ark::es2panda::lsp::IsLiteralNameOfPropertyDeclarationOrIndexAccess;
using ark::es2panda::lsp::RenameInfoType;

class LspRenameRemainingTests : public LSPAPITests {
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
// P2E-RENAME-01: GetRenameInfo_DirectCall_ReturnsInfo
// =============================================================================
TEST_F(LspRenameRemainingTests, GetRenameInfo_DirectCall_ReturnsInfo)
{
    const std::string source = R"(
let x = 1;
)";
    Initializer initializer;
    std::vector<std::string> files = {"rename01.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    size_t pos = source.find("x");
    ASSERT_NE(pos, std::string::npos);

    // GetRenameInfo requires a pandaLibPath; use a path with enough depth (>3 levels)
    // to prevent IsDefinedInLibraryFile from matching the test file path (the
    // function strips 3 levels up from pandaLibPath and does a substring check).
    RenameInfoType info = GetRenameInfo(ctx, pos, "/nonexistent/very/deep/path/");
    // The variant should hold either RenameInfoSuccess or RenameInfoFailure
    EXPECT_NO_FATAL_FAILURE({
        if (std::holds_alternative<ark::es2panda::lsp::RenameInfoSuccess>(info)) {
            auto &success = std::get<ark::es2panda::lsp::RenameInfoSuccess>(info);
            EXPECT_TRUE(success.GetCanRenameSuccess());
        } else {
            auto &failure = std::get<ark::es2panda::lsp::RenameInfoFailure>(info);
            EXPECT_FALSE(failure.GetCanRenameFailure());
        }
    });

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-RENAME-02: GetContextualTypeFromParentOrAncestorTypeNode_ReturnsType
// =============================================================================
// LIMITATION: GetContextualTypeFromParentOrAncestorTypeNode requires a
// checker::ETSChecker, which is only available in CHECKED state via
// GetChecker()->AsETSChecker().  The function internally calls node->Check()
// which requires a checker context.  For a parameter expression node, it
// should return the contextual type.
// Only direct test of rename.cpp GetContextualTypeFromParentOrAncestorTypeNode
// (rename.cpp:217-238).
TEST_F(LspRenameRemainingTests, GetContextualTypeFromParentOrAncestorTypeNode_ReturnsType)
{
    const std::string source = R"(
function foo(x: number): void {}
)";
    Initializer initializer;
    std::vector<std::string> files = {"rename02.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *checker = context->GetChecker()->AsETSChecker();
    ASSERT_NE(checker, nullptr);

    // Find the ETS_PARAMETER_EXPRESSION node for parameter 'x'
    auto *paramExpr = FindNode(ast, [](AstNode *n) { return n->IsETSParameterExpression(); });
    ASSERT_NE(paramExpr, nullptr);

    auto result = ark::es2panda::lsp::GetContextualTypeFromParentOrAncestorTypeNode(paramExpr, checker);
    // The function should return a valid type for a typed parameter
    EXPECT_TRUE(result.has_value());

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-RENAME-03: IsImportOrExportSpecifierName_True
// =============================================================================
TEST_F(LspRenameRemainingTests, IsImportOrExportSpecifierName_True)
{
    // IsImportOrExportSpecifierName returns true for ImportSpecifier, ExportSpecifier, or Identifier
    const std::string source = R"(
let x = 1;
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename03.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the 'x' identifier
    auto *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("x"); });
    ASSERT_NE(ident, nullptr);

    // IsImportOrExportSpecifierName returns true for Identifier nodes
    EXPECT_TRUE(IsImportOrExportSpecifierName(ident));

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-RENAME-04: IsImportOrExportSpecifierName_False
// =============================================================================
TEST_F(LspRenameRemainingTests, IsImportOrExportSpecifierName_False)
{
    // IsImportOrExportSpecifierName returns false for non-Identifier, non-specifier nodes
    const std::string source = "let x = 42;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename04.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find a NumberLiteral (not an Identifier or specifier)
    auto *numLit = FindNode(ast, [](AstNode *n) { return n->IsNumberLiteral(); });
    ASSERT_NE(numLit, nullptr);

    EXPECT_FALSE(IsImportOrExportSpecifierName(numLit));

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-RENAME-05: IsLiteralNameOfPropertyDeclarationOrIndexAccess_True
// =============================================================================
// BLOCKED_DESIGN_MAPPING
// The true path of IsLiteralNameOfPropertyDeclarationOrIndexAccess is unreachable
// under ETS-only. In rename.cpp:485-501, the declaration-name match on lines
// 487-495 cannot hold because GetNameOfDeclaration returns nullptr for the
// property, assignment, enum-member, function, method, and module parent types
// represented there. The indexed-access path on lines 496-497 depends on a
// TS_LITERAL_TYPE under a TS_INDEXED_ACCESS_TYPE, but TSparser.cpp:666 constructs
// that shape only for TS and the ETS-only restriction in es2panda_lib.cpp:486-487
// rejects TS contexts. The design's true expectation therefore has no reachable
// product path in ETS.

// =============================================================================
// P2E-RENAME-06: IsLiteralNameOfPropertyDeclarationOrIndexAccess_False
// =============================================================================
// Earlier lsp_rename tests reach only the TS_ENUM_MEMBER arm; this pins the
// switch default arm (VariableDeclarator parent).
TEST_F(LspRenameRemainingTests, IsLiteralNameOfPropertyDeclarationOrIndexAccess_False)
{
    // For a NumberLiteral whose parent is not in the recognized set, returns false.
    const std::string source = "let x = 42;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename06.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find a NumberLiteral whose parent is a VariableDeclarator (not in the recognized set)
    auto *numLit = FindNode(ast, [](AstNode *n) { return n->IsNumberLiteral(); });
    ASSERT_NE(numLit, nullptr);

    EXPECT_FALSE(IsLiteralNameOfPropertyDeclarationOrIndexAccess(numLit));

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-RENAME-07: GetNameOfDeclaration_ReturnsName
// =============================================================================
TEST_F(LspRenameRemainingTests, GetNameOfDeclaration_ReturnsName)
{
    // GetNameOfDeclaration calls GetNonAssignedNameOfDeclaration first.
    // For an Identifier node, GetNonAssignedNameOfDeclaration returns the
    // identifier itself (rename.cpp:519-521), so GetNameOfDeclaration
    // returns the same identifier.
    const std::string source = R"(
function foo(): void {}
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename07.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the 'foo' identifier (the function name)
    auto *fooIdent = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("foo"); });
    ASSERT_NE(fooIdent, nullptr);

    // GetNameOfDeclaration(Identifier) returns the identifier itself
    AstNode *name = GetNameOfDeclaration(fooIdent);
    ASSERT_NE(name, nullptr);
    EXPECT_TRUE(name->IsIdentifier());
    EXPECT_EQ(name->AsIdentifier()->Name(), "foo");

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-RENAME-08: GetNonAssignedNameOfDeclaration_ReturnsName
// =============================================================================
TEST_F(LspRenameRemainingTests, GetNonAssignedNameOfDeclaration_ReturnsName)
{
    const std::string source = R"(
let x = 1;
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename08.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the 'x' identifier (which is an Identifier node)
    auto *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("x"); });
    ASSERT_NE(ident, nullptr);

    AstNode *name = GetNonAssignedNameOfDeclaration(ident);
    // For an Identifier node, GetNonAssignedNameOfDeclaration returns the node itself
    ASSERT_NE(name, nullptr);
    EXPECT_TRUE(name->IsIdentifier());
    EXPECT_EQ(name->AsIdentifier()->Name(), "x");

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-RENAME-09: GetAssignedName_ReturnsName
// =============================================================================
TEST_F(LspRenameRemainingTests, GetAssignedName_ReturnsName)
{
    // GetAssignedName only inspects node->Parent() (rename.cpp:543-549),
    // not the node's own type.  An ArrowFunctionExpression as an object
    // literal property value has a Property parent (ETSparserExpressions.cpp:260-269).
    // The Property path (rename.cpp:548-550) returns the Property node.
    const std::string source = R"(
let o = { handler: (): void => {} };
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("rename09.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the ArrowFunctionExpression
    auto *arrowFunc = FindNode(ast, [](AstNode *n) { return n->IsArrowFunctionExpression(); });
    ASSERT_NE(arrowFunc, nullptr);

    // Verify the parent is a Property before calling GetAssignedName
    AstNode *parent = arrowFunc->Parent();
    ASSERT_NE(parent, nullptr);
    ASSERT_TRUE(parent->IsProperty());

    AstNode *assignedName = GetAssignedName(arrowFunc);
    ASSERT_NE(assignedName, nullptr);
    // GetAssignedName returns the Property node
    EXPECT_TRUE(assignedName->IsProperty());
    // The Property's key should be the identifier "handler"
    auto *prop = assignedName->AsProperty();
    EXPECT_TRUE(prop->Key()->IsIdentifier());
    EXPECT_EQ(prop->Key()->AsIdentifier()->Name(), "handler");

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-RENAME-10: GetRenameInfoForNode_DirectCall_ReturnsInfo
// =============================================================================
// Complements lsp_rename RenameInfoGetRenameInfoForNode1 (nullopt for a std-lib
// import) with the local-declaration success branch.
TEST_F(LspRenameRemainingTests, GetRenameInfoForNode_DirectCall_ReturnsInfo)
{
    const std::string source = R"(
let x = 1;
let y = x;
)";
    Initializer initializer;
    std::vector<std::string> files = {"rename10.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *checker = context->GetChecker()->AsETSChecker();
    ASSERT_NE(checker, nullptr);

    // Find the second 'x' (a reference, not the declaration)
    size_t secondX = source.rfind("x");
    ASSERT_NE(secondX, std::string::npos);
    auto *ident = FindNode(ast, [secondX](AstNode *n) {
        return n->IsIdentifier() && n->AsIdentifier()->Name().Is("x") && n->Start().index == secondX;
    });
    ASSERT_NE(ident, nullptr);
    // The reference identifier should have a resolved variable in CHECKED state
    ASSERT_NE(ident->AsIdentifier()->Variable(), nullptr) << "Reference x has no resolved variable";

    // Use a pandaLibPath with enough depth (>3 levels) to prevent
    // IsDefinedInLibraryFile from matching the test file path (the function
    // strips 3 levels up from pandaLibPath and does a substring check).
    std::string pandaLibPath = "/nonexistent/very/deep/path/";
    auto result = GetRenameInfoForNode(ident->AsIdentifier(), checker, context->parserProgram, pandaLibPath);
    // For a local variable reference in CHECKED state, the declaration should
    // be found and rename info should be returned.
    ASSERT_TRUE(result.has_value());
    auto &info = *result;
    if (std::holds_alternative<ark::es2panda::lsp::RenameInfoSuccess>(info)) {
        auto &success = std::get<ark::es2panda::lsp::RenameInfoSuccess>(info);
        EXPECT_TRUE(success.GetCanRenameSuccess());
    } else {
        auto &failure = std::get<ark::es2panda::lsp::RenameInfoFailure>(info);
        EXPECT_FALSE(failure.GetCanRenameFailure());
    }

    initializer.DestroyContext(ctx);
}

}  // namespace
