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

#include "lsp/include/quick_info.h"
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include <gtest/gtest.h>

namespace {
using ark::es2panda::lsp::Initializer;

class LspQuickInfoTests : public LSPAPITests {};

TEST_F(LspQuickInfoTests, GetQuickInfoAtPosition1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("quick_info3.ets", ES2PANDA_STATE_CHECKED, R"(enum MyStrings { A = 'hello' };)");
    size_t const offset = 17;
    LSPAPI const *lspApi = GetImpl();
    auto quickInfo = lspApi->getQuickInfoAtPosition("quick_info3.ets", ctx, offset);
    ASSERT_NE(quickInfo, QuickInfo());
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind = "enum member";
    size_t const start = 17;
    size_t const length = 1;
    TextSpan span(start, length);
    const std::string kindModifiers = "static public readonly";
    const std::string expectedFileName = "quick_info3.ets";

    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("MyStrings", "enumName");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("A", "enumMember");
    expected.emplace_back(" ", "space");
    expected.emplace_back("=", "operator");
    expected.emplace_back(" ", "space");
    expected.emplace_back("\"", "punctuation");
    expected.emplace_back("hello", "text");
    expected.emplace_back("\"", "punctuation");

    auto expectedQuickInfo = QuickInfo(kind, kindModifiers, span, expected, document, tags, expectedFileName);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(quickInfo, expectedQuickInfo);
}

TEST_F(LspQuickInfoTests, GetQuickInfoAtPosition2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("quick-info-test.ets", ES2PANDA_STATE_CHECKED,
                                                      "class MyClass {\n  public myProp: number = 0;\n}");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 9;
    const std::string fileName = "quick-info-test.ets";
    auto quickInfo = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset, fileName);
    ASSERT_NE(quickInfo, QuickInfo());
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind = "class";
    std::vector<SymbolDisplayPart> expected;
    size_t const start = 6;
    size_t const length = 7;
    TextSpan span(start, length);
    const std::string kindModifiers;

    expected.emplace_back("class", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("MyClass", "className");

    auto expectedQuickInfo = QuickInfo(kind, kindModifiers, span, expected, document, tags, fileName);
    ASSERT_EQ(quickInfo, expectedQuickInfo);

    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, GetQuickInfoAtPosition3)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("quick-info-test.ets", ES2PANDA_STATE_CHECKED,
                                  "interface objI { key : string; }\nlet obj : objI = { key:\"valueaaaaaaaaa,\" }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 54;
    const std::string fileName = "quick-info-test.ets";
    auto quickInfo = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset, fileName);
    ASSERT_NE(quickInfo, QuickInfo());
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind = "get";
    const std::string kindModifiers = "public abstract";
    size_t const start = 52;
    size_t const length = 3;
    TextSpan span(start, length);
    std::vector<SymbolDisplayPart> expected;

    expected.emplace_back("objI", "interface");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("key", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("string", "returnType");

    auto expectedQuickInfo = QuickInfo(kind, kindModifiers, span, expected, document, tags, fileName);
    ASSERT_EQ(quickInfo, expectedQuickInfo);

    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, getPropertySymbolFromContextualType1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("contextual-type-test.ets", ES2PANDA_STATE_CHECKED,
                                  "interface objI { key : string; }\nlet obj : objI = { key:\"valueaaaaaaaaa,\" }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 54;
    auto node = ark::es2panda::lsp::GetTokenForQuickInfo(ctx, offset);
    ASSERT_NE(node, nullptr);
    auto containingObjectNode = ark::es2panda::lsp::GetContainingObjectLiteralNode(node);
    ASSERT_NE(containingObjectNode, nullptr);
    auto contextualTypeNode = ark::es2panda::lsp::GetContextualTypeNode(containingObjectNode->Parent());
    ASSERT_NE(contextualTypeNode, nullptr);
    ASSERT_EQ(contextualTypeNode->Type(), ark::es2panda::ir::AstNodeType::TS_INTERFACE_DECLARATION);
    auto propertyNode = ark::es2panda::lsp::GetPropertyNodeFromContextualType(containingObjectNode, contextualTypeNode);
    ASSERT_NE(propertyNode, nullptr);
    auto propertyDef = propertyNode->AsMethodDefinition();
    ASSERT_EQ(propertyDef->Key()->AsIdentifier()->Name(), "key");

    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, getPropertySymbolFromContextualType2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("contextual-type-test.ets", ES2PANDA_STATE_CHECKED,
                                                      "const record : Record<string,number> = { \"hello\":1234 }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 44;
    auto node = ark::es2panda::lsp::GetTokenForQuickInfo(ctx, offset);
    ASSERT_NE(node, nullptr);
    auto containingObjectNode = ark::es2panda::lsp::GetContainingObjectLiteralNode(node);
    ASSERT_NE(containingObjectNode, nullptr);
    auto contextualTypeNode = ark::es2panda::lsp::GetContextualTypeNode(containingObjectNode->Parent());
    ASSERT_NE(contextualTypeNode, nullptr);
    ASSERT_EQ(contextualTypeNode->Type(), ark::es2panda::ir::AstNodeType::CLASS_DEFINITION);
    auto propertyNode = ark::es2panda::lsp::GetPropertyNodeFromContextualType(containingObjectNode, contextualTypeNode);
    ASSERT_EQ(propertyNode->Type(), ark::es2panda::ir::AstNodeType::PROPERTY);

    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, GetNodeAtLocationForQuickInfo1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("quick-info-test.ets", ES2PANDA_STATE_CHECKED,
                                  "interface objI { key : string; }\nlet obj : objI = { key:\"valueaaaaaaaaa,\" }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 54;
    auto node = ark::es2panda::lsp::GetTokenForQuickInfo(ctx, offset);
    ASSERT_NE(node, nullptr);
    auto nodeAtLocationForQuickInfo = ark::es2panda::lsp::GetNodeAtLocationForQuickInfo(node);
    ASSERT_NE(nodeAtLocationForQuickInfo, nullptr);
    auto propertyDef = nodeAtLocationForQuickInfo->AsMethodDefinition();
    ASSERT_EQ(propertyDef->Key()->AsIdentifier()->Name(), "key");

    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, GetNodeAtLocationForQuickInfo2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("quick-info-test.ets", ES2PANDA_STATE_CHECKED,
                                  "class Test {\n  private _a: number = 1;\n  public get a(): number {\n    "
                                  "return this._a;\n  }\n  public static ccc:number = 1\n\n  constructor(a : "
                                  "number) {\n  }\n}\n\nlet a = 1\nlet test: Test = new Test(a)\nlet t_a = test.a");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 8;
    auto node = ark::es2panda::lsp::GetTokenForQuickInfo(ctx, offset);
    ASSERT_NE(node, nullptr);
    auto nodeAtLocationForQuickInfo = ark::es2panda::lsp::GetNodeAtLocationForQuickInfo(node);
    ASSERT_NE(nodeAtLocationForQuickInfo, nullptr);
    ASSERT_EQ(nodeAtLocationForQuickInfo->Type(), ark::es2panda::ir::AstNodeType::CLASS_DEFINITION);

    size_t const position = 191;
    node = ark::es2panda::lsp::GetTokenForQuickInfo(ctx, position);
    ASSERT_NE(node, nullptr);
    nodeAtLocationForQuickInfo = ark::es2panda::lsp::GetNodeAtLocationForQuickInfo(node);
    ASSERT_NE(nodeAtLocationForQuickInfo, nullptr);
    ASSERT_EQ(nodeAtLocationForQuickInfo->Type(), ark::es2panda::ir::AstNodeType::CLASS_DEFINITION);

    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, GetNodeAtLocationForQuickInfo3)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("quick-info-test.ets", ES2PANDA_STATE_CHECKED,
                                  "function func():string {\n  return \"func\"\n}\nlet f = func();");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 11;
    auto node = ark::es2panda::lsp::GetTokenForQuickInfo(ctx, offset);
    ASSERT_NE(node, nullptr);
    auto nodeAtLocationForQuickInfo = ark::es2panda::lsp::GetNodeAtLocationForQuickInfo(node);
    ASSERT_NE(nodeAtLocationForQuickInfo, nullptr);
    ASSERT_EQ(nodeAtLocationForQuickInfo->Type(), ark::es2panda::ir::AstNodeType::METHOD_DEFINITION);

    size_t const position = 53;
    node = ark::es2panda::lsp::GetTokenForQuickInfo(ctx, position);
    ASSERT_NE(node, nullptr);
    nodeAtLocationForQuickInfo = ark::es2panda::lsp::GetNodeAtLocationForQuickInfo(node);
    ASSERT_NE(nodeAtLocationForQuickInfo, nullptr);
    ASSERT_EQ(nodeAtLocationForQuickInfo->Type(), ark::es2panda::ir::AstNodeType::METHOD_DEFINITION);

    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, GetNodeAtLocationForQuickInfo4)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("quick-info-test.ets", ES2PANDA_STATE_CHECKED,
                                  "type NullableObject = Object | null\nlet nullOb: NullableObject = null");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 11;
    auto node = ark::es2panda::lsp::GetTokenForQuickInfo(ctx, offset);
    ASSERT_NE(node, nullptr);
    auto nodeAtLocationForQuickInfo = ark::es2panda::lsp::GetNodeAtLocationForQuickInfo(node);
    ASSERT_NE(nodeAtLocationForQuickInfo, nullptr);
    ASSERT_EQ(nodeAtLocationForQuickInfo->Type(), ark::es2panda::ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION);

    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, GetNodeAtLocationForQuickInfo5)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(
        "quick-info-test.ets", ES2PANDA_STATE_CHECKED,
        "enum Color {\n  Red = \"red\",\n  Blue = \"blue\"\n}\n\nlet myColor: Color = Color.Red;");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 7;
    auto node = ark::es2panda::lsp::GetTokenForQuickInfo(ctx, offset);
    ASSERT_NE(node, nullptr);
    auto nodeAtLocationForQuickInfo = ark::es2panda::lsp::GetNodeAtLocationForQuickInfo(node);
    ASSERT_NE(nodeAtLocationForQuickInfo, nullptr);
    ASSERT_EQ(nodeAtLocationForQuickInfo->Type(), ark::es2panda::ir::AstNodeType::CLASS_DEFINITION);

    size_t const position = 70;
    node = ark::es2panda::lsp::GetTokenForQuickInfo(ctx, position);
    ASSERT_NE(node, nullptr);
    nodeAtLocationForQuickInfo = ark::es2panda::lsp::GetNodeAtLocationForQuickInfo(node);
    ASSERT_NE(nodeAtLocationForQuickInfo, nullptr);
    ASSERT_EQ(nodeAtLocationForQuickInfo->Type(), ark::es2panda::ir::AstNodeType::CLASS_DEFINITION);

    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForEnumMemberWithNumberLiteral)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("enum-member-test.ets", ES2PANDA_STATE_CHECKED,
                                                      "enum MyEnum { First = 1, Second = 2 }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::CLASS_PROPERTY &&
               node->AsClassProperty()->Key()->AsIdentifier()->Name() == "First";
    };
    auto found = ast->FindChild(checkFunc);
    auto parent = found->Parent();
    auto enumDecl = parent->AsClassDefinition()->OrigEnumDecl()->AsTSEnumDeclaration();
    auto enumMember = enumDecl->FindChild([&found](ark::es2panda::ir::AstNode *child) {
        return child->IsTSEnumMember() && child->AsTSEnumMember()->Key()->AsIdentifier()->Name() ==
                                              found->AsClassProperty()->Key()->AsIdentifier()->Name();
    });
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForEnumMember(enumMember);
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("MyEnum", "enumName");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("First", "enumMember");
    expected.emplace_back(" ", "space");
    expected.emplace_back("=", "operator");
    expected.emplace_back(" ", "space");
    expected.emplace_back("1", "text");
    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForEnumMemberWithStringLiteral)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("enum-member-string-test.ets", ES2PANDA_STATE_CHECKED,
                                                      "enum MyStrings { A = 'hello' }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::CLASS_PROPERTY &&
               node->AsClassProperty()->Key()->AsIdentifier()->Name() == "A";
    };
    auto found = ast->FindChild(checkFunc);
    auto parent = found->Parent();
    auto enumDecl = parent->AsClassDefinition()->OrigEnumDecl()->AsTSEnumDeclaration();
    auto enumMember = enumDecl->FindChild([&found](ark::es2panda::ir::AstNode *child) {
        return child->Type() == ark::es2panda::ir::AstNodeType::TS_ENUM_MEMBER &&
               child->AsTSEnumMember()->Key()->AsIdentifier()->Name() ==
                   found->AsClassProperty()->Key()->AsIdentifier()->Name();
    });
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForEnumMember(enumMember);
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("MyStrings", "enumName");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("A", "enumMember");
    expected.emplace_back(" ", "space");
    expected.emplace_back("=", "operator");
    expected.emplace_back(" ", "space");
    expected.emplace_back("\"", "punctuation");
    expected.emplace_back("hello", "text");
    expected.emplace_back("\"", "punctuation");
    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}
}  // namespace