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
#include "lsp/include/quick_info.h"
#include "lsp/include/types.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::GetKindModifiers;
using ark::es2panda::lsp::GetNameForTypeNode;
using ark::es2panda::lsp::GetNameForTypeReference;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::IsClass;
using ark::es2panda::lsp::IsDeclaration;
using ark::es2panda::lsp::IsIncludedToken;
using ark::es2panda::lsp::IsObjectLiteralElement;
using ark::es2panda::lsp::IsSpace;
using ark::es2panda::lsp::ModifiersToString;

class LspQuickInfoRemainingTests : public LSPAPITests {
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
// P2D-QI-01: IsSpace_TrueForSpace
// =============================================================================
TEST_F(LspQuickInfoRemainingTests, IsSpace_TrueForSpace)
{
    EXPECT_TRUE(IsSpace(' '));
    EXPECT_TRUE(IsSpace('\t'));
    EXPECT_TRUE(IsSpace('\n'));
}

// =============================================================================
// P2D-QI-02: IsSpace_FalseForLetter
// =============================================================================
TEST_F(LspQuickInfoRemainingTests, IsSpace_FalseForLetter)
{
    EXPECT_FALSE(IsSpace('a'));
    EXPECT_FALSE(IsSpace('Z'));
    EXPECT_FALSE(IsSpace('1'));
}

// =============================================================================
// P2D-QI-03: IsIncludedToken_TrueForIdentifier
// =============================================================================
TEST_F(LspQuickInfoRemainingTests, IsIncludedToken_TrueForIdentifier)
{
    const std::string source = "let x = 1;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("qi03.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the 'x' identifier
    auto *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("x"); });
    ASSERT_NE(ident, nullptr);

    EXPECT_TRUE(IsIncludedToken(ident));
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-QI-04: IsIncludedToken_FalseForNonIncludedNode
// =============================================================================
// Note: The design says "Comment→false" but comments are lexer tokens, not
// AstNode types. The function checks AstNodeType against a set of included
// types. We use a BinaryExpression which is not in the set.
TEST_F(LspQuickInfoRemainingTests, IsIncludedToken_FalseForNonIncludedNode)
{
    const std::string source = "let x = 1 + 2;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("qi04.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find a BinaryExpression (not in the INCLUDED_TOKEN_TYPES set)
    auto *binExpr = FindNode(ast, [](AstNode *n) { return n->IsBinaryExpression(); });
    ASSERT_NE(binExpr, nullptr);

    EXPECT_FALSE(IsIncludedToken(binExpr));
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-QI-05: IsObjectLiteralElement_True
// =============================================================================
TEST_F(LspQuickInfoRemainingTests, IsObjectLiteralElement_True)
{
    const std::string source = "let obj = { key: 42 };";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("qi05.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find a Property node (the only type in the INCLUDED_OBJECT_LITERAL_ELEMENT_TYPES set)
    auto *prop = FindNode(ast, [](AstNode *n) { return n->IsProperty(); });
    ASSERT_NE(prop, nullptr);

    EXPECT_TRUE(IsObjectLiteralElement(prop));
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-QI-06: IsObjectLiteralElement_False
// =============================================================================
TEST_F(LspQuickInfoRemainingTests, IsObjectLiteralElement_False)
{
    const std::string source = "let x = 42;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("qi06.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find a NumberLiteral (not a Property)
    auto *numLit = FindNode(ast, [](AstNode *n) { return n->IsNumberLiteral(); });
    ASSERT_NE(numLit, nullptr);

    EXPECT_FALSE(IsObjectLiteralElement(numLit));
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-QI-07: GetNameForTypeReference_ReturnsName
// =============================================================================
TEST_F(LspQuickInfoRemainingTests, GetNameForTypeReference_ReturnsName)
{
    // Use a class property with a type annotation to get a TypeNode
    const std::string source = R"(
class C {
    prop: number = 1;
}
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("qi07.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the ClassProperty with name "prop"
    auto *classProp = FindNode(ast, [](AstNode *n) {
        if (!n->IsClassProperty()) {
            return false;
        }
        auto *key = n->AsClassProperty()->Key();
        return key != nullptr && key->IsIdentifier() && key->AsIdentifier()->Name() == "prop";
    });
    ASSERT_NE(classProp, nullptr);

    // ClassProperty has TypeAnnotation()
    auto *typeAnnotation = classProp->AsClassProperty()->TypeAnnotation();
    ASSERT_NE(typeAnnotation, nullptr);

    // For "number" type, GetNameForTypeReference should return the type name
    std::string name = GetNameForTypeReference(typeAnnotation);
    EXPECT_EQ(name, "number");
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-QI-08: GetNameForTypeNode_ReturnsName
// =============================================================================
TEST_F(LspQuickInfoRemainingTests, GetNameForTypeNode_ReturnsName)
{
    // Use a class property with a type annotation to get a TypeNode
    const std::string source = R"(
class C {
    prop: number = 1;
}
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("qi08.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the ClassProperty with name "prop"
    auto *classProp = FindNode(ast, [](AstNode *n) {
        if (!n->IsClassProperty()) {
            return false;
        }
        auto *key = n->AsClassProperty()->Key();
        return key != nullptr && key->IsIdentifier() && key->AsIdentifier()->Name() == "prop";
    });
    ASSERT_NE(classProp, nullptr);

    // ClassProperty has TypeAnnotation()
    auto *typeAnnotation = classProp->AsClassProperty()->TypeAnnotation();
    ASSERT_NE(typeAnnotation, nullptr);

    // For "number" type, GetNameForTypeNode should return the type name
    std::string name = GetNameForTypeNode(typeAnnotation);
    EXPECT_EQ(name, "number");
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-QI-09: CreateDisplayForImportDeclaration_ReturnsDisplay
// =============================================================================
// BLOCKED_DESIGN_MAPPING
// Source: ets2panda/lsp/src/quick_info.cpp:1543-1567
// The function checks for node->Type() == ir::AstNodeType::IMPORT_DECLARATION.
// The es2panda plugin API rejects non-ETS extensions (es2panda_lib.cpp InitContext
// guard: "Plugin API supports only ETS"), so TS contexts yield ERROR state with
// null parserProgram, making IMPORT_DECLARATION unattainable through the ETS-only
// plugin API. ETS imports use ETS_IMPORT_DECLARATION instead.
// NEXT ACTION: Review design vs. plugin API limitation. If the function should
// support ETS imports, update it to also handle ETS_IMPORT_DECLARATION.
// No TEST_F written for this item.

// =============================================================================
// P2D-QI-10: ModifiersToString_ReturnsString
// =============================================================================
TEST_F(LspQuickInfoRemainingTests, ModifiersToString_ReturnsString)
{
    using ark::es2panda::ir::ModifierFlags;

    // No flags
    EXPECT_TRUE(ModifiersToString(ModifierFlags::NONE).empty());

    // Single flag
    EXPECT_EQ(ModifiersToString(ModifierFlags::STATIC), "static");
    EXPECT_EQ(ModifiersToString(ModifierFlags::PUBLIC), "public");
    EXPECT_EQ(ModifiersToString(ModifierFlags::PRIVATE), "private");
    EXPECT_EQ(ModifiersToString(ModifierFlags::PROTECTED), "protected");
    EXPECT_EQ(ModifiersToString(ModifierFlags::ASYNC), "async");
    EXPECT_EQ(ModifiersToString(ModifierFlags::READONLY), "readonly");
    EXPECT_EQ(ModifiersToString(ModifierFlags::ABSTRACT), "abstract");
    EXPECT_EQ(ModifiersToString(ModifierFlags::CONST), "const");
    EXPECT_EQ(ModifiersToString(ModifierFlags::FINAL), "final");
    EXPECT_EQ(ModifiersToString(ModifierFlags::DECLARE), "declare");

    // Multiple flags
    std::string combined = ModifiersToString(ModifierFlags::PUBLIC | ModifierFlags::STATIC);
    EXPECT_EQ(combined, "static public");

    combined = ModifiersToString(ModifierFlags::PRIVATE | ModifierFlags::READONLY);
    EXPECT_EQ(combined, "private readonly");
}

// =============================================================================
// P2D-QI-11: GetKindModifiers_ReturnsString
// =============================================================================
TEST_F(LspQuickInfoRemainingTests, GetKindModifiers_ReturnsString)
{
    const std::string source = R"(
class MyClass {
    static foo(): void {}
}
)";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("qi11.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the MethodDefinition with name "foo"
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

    std::string modifiers = GetKindModifiers(methodDef);
    // The method is declared static: pin the exact modifier string instead of
    // mirroring the production call ModifiersToString(...) tautologically.
    EXPECT_EQ(modifiers, "static public");
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-QI-12: IsClass_TrueForClassDef
// =============================================================================
TEST_F(LspQuickInfoRemainingTests, IsClass_TrueForClassDef)
{
    const std::string source = "class MyClass {}";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("qi12.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the ClassDefinition node
    auto *classDef = FindNode(ast, [](AstNode *n) { return n->IsClassDefinition(); });
    ASSERT_NE(classDef, nullptr);

    EXPECT_TRUE(IsClass(classDef));
    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2D-QI-13: IsDeclaration_TrueForDecl
// =============================================================================
TEST_F(LspQuickInfoRemainingTests, IsDeclaration_TrueForDecl)
{
    const std::string source = "let x = 1;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("qi13.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find a VariableDeclaration node
    auto *varDecl = FindNode(ast, [](AstNode *n) { return n->IsVariableDeclaration(); });
    ASSERT_NE(varDecl, nullptr);

    // VariableDeclaration is in the IsDeclaration set
    EXPECT_TRUE(IsDeclaration(varDecl));
    initializer.DestroyContext(ctx);
}

}  // namespace