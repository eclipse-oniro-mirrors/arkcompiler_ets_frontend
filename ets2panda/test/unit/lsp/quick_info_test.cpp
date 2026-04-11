/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

TEST_F(LspQuickInfoTests, GetQuickInfoAtPosition4)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("quick-info-test.ets", ES2PANDA_STATE_CHECKED,
                                                      "//中文注释\ninterface objI {\n//中文注释\n key : string; }\nlet "
                                                      "obj : objI = {\n//中文注释\n key:\"valueaaaaaaaaa,\" }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    LSPAPI const *lspApi = GetImpl();

    size_t const offset = 76;
    const std::string fileName = "quick-info-test.ets";
    auto quickInfo = lspApi->getQuickInfoAtPosition(fileName.c_str(), ctx, offset);
    ASSERT_NE(quickInfo, QuickInfo());
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind = "get";
    const std::string kindModifiers = "public abstract";
    size_t const start = 99;
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

TEST_F(LspQuickInfoTests, GetQuickInfoAtPositionForKeywords1)
{
    const std::string src = R"(
function func() {};
)";
    auto files = CreateTempFile({"keywords-test1.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string fileName = "keywords-test1.ets";

    size_t const offset = 4;
    auto quickInfo = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset, fileName);
    ASSERT_NE(quickInfo, QuickInfo());
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("function", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("func", "functionName");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("undefined", "returnType");
    const std::string kind = "method";
    const std::string kindModifiers = "static public";
    const size_t start = 1;
    const size_t length = 18;
    TextSpan span(start, length);

    auto expectedQuickInfo = QuickInfo(kind, kindModifiers, span, expected, document, tags, fileName);
    ASSERT_EQ(quickInfo, expectedQuickInfo);

    init.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, GetQuickInfoAtPositionForKeywords2)
{
    const std::string src = R"(
class A {};
)";
    auto files = CreateTempFile({"keywords-test2.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string fileName = "keywords-test2.ets";

    size_t const offset = 4;
    auto quickInfo = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset, fileName);
    ASSERT_NE(quickInfo, QuickInfo());
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("class", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("A", "className");
    const std::string kind = "class";
    const std::string kindModifiers = "static public";
    const size_t start = 1;
    const size_t length = 10;
    TextSpan span(start, length);

    auto expectedQuickInfo = QuickInfo(kind, kindModifiers, span, expected, document, tags, fileName);
    ASSERT_EQ(quickInfo, expectedQuickInfo);

    init.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, GetQuickInfoAtPositionForKeywords3)
{
    const std::string src = R"(
namespace A {};
interface B {};
enum C {};
type D = string;
)";
    auto files = CreateTempFile({"keywords-test3.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string fileName = "keywords-test3.ets";

    size_t const offset1 = 4;
    auto quickInfo1 = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset1, fileName);
    ASSERT_EQ(quickInfo1, QuickInfo());

    size_t const offset2 = 20;
    auto quickInfo2 = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset2, fileName);
    ASSERT_EQ(quickInfo2, QuickInfo());

    size_t const offset3 = 35;
    auto quickInfo3 = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset3, fileName);
    ASSERT_EQ(quickInfo3, QuickInfo());

    size_t const offset4 = 46;
    auto quickInfo4 = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset4, fileName);
    ASSERT_EQ(quickInfo4, QuickInfo());

    init.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForDocument)
{
    const std::string src = R"(
    /**
     * This is a test document.
     *
     * @param param1 The first parameter.
     * @since 23
     */
     class AAA {}
)";
    auto files = CreateTempFile({"document-test.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string fileName = "document-test.ets";
    const auto offset = src.find("AAA");
    ASSERT_NE(offset, std::string::npos);

    auto quickInfo = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset, fileName);
    ASSERT_NE(quickInfo, QuickInfo());
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_EQ(quickInfo.GetFileName(), std::string(context->parserProgram->SourceFilePath()));

    std::vector<SymbolDisplayPart> expectedDocument;
    expectedDocument.emplace_back("This is a test document.", "plaintext");
    ASSERT_EQ(quickInfo.GetDocument(), expectedDocument);

    std::vector<DocTagInfo> expectedTags;
    expectedTags.emplace_back("param", "param1 The first parameter.");
    expectedTags.emplace_back("since", "23");
    ASSERT_EQ(quickInfo.GetTags(), expectedTags);

    init.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForDocumentWithMultiJsdocIndex)
{
    const std::string src = R"(
    /**
     * This is a test document 1.
     *
     * @param param1 The first parameter.
     * @since 23
     */
    /**
     * This is a test document 0.
     *
     * @param param1 The first parameter.
     * @since 23
     */
    class AAA {}
)";
    auto files = CreateTempFile({"document-multi-jsdoc-index-test.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string fileName = "document-multi-jsdoc-index-test.ets";
    const auto offset = src.find("AAA");
    ASSERT_NE(offset, std::string::npos);

    auto quickInfo = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset, fileName);
    ASSERT_NE(quickInfo, QuickInfo());

    std::vector<SymbolDisplayPart> expectedDocument;
    expectedDocument.emplace_back("This is a test document 1.", "plaintext", 0);
    expectedDocument.emplace_back("This is a test document 0.", "plaintext", 1);
    ASSERT_EQ(quickInfo.GetDocument(), expectedDocument);

    std::vector<DocTagInfo> expectedTags;
    expectedTags.emplace_back("param", "param1 The first parameter.", 0);
    expectedTags.emplace_back("since", "23", 0);
    expectedTags.emplace_back("param", "param1 The first parameter.", 1);
    expectedTags.emplace_back("since", "23", 1);
    ASSERT_EQ(quickInfo.GetTags(), expectedTags);

    init.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForDocumentWithMultiParagraphs)
{
    const std::string src = R"(
    /**
     * First paragraph line 1.
     * Second line.
     *
     *
     *
     * Third paragraph.
     */
    class BBB {}
)";
    auto files = CreateTempFile({"document-multi-paragraph-test.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string fileName = "document-multi-paragraph-test.ets";
    const auto offset = src.find("BBB");
    ASSERT_NE(offset, std::string::npos);

    auto quickInfo = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset, fileName);
    ASSERT_NE(quickInfo, QuickInfo());

    std::vector<SymbolDisplayPart> expectedDocument;
    expectedDocument.emplace_back("First paragraph line 1.\nSecond line.\n\nThird paragraph.", "plaintext");
    ASSERT_EQ(quickInfo.GetDocument(), expectedDocument);
    ASSERT_TRUE(quickInfo.GetTags().empty());

    init.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForDocumentWithMultilineTags)
{
    const std::string src = R"(
    /**
     * Function docs.
     * @param value first line
     * second line of param.
     * @returns result line1
     * result line2
     */
    class CCC {}
)";
    auto files = CreateTempFile({"document-multiline-tags-test.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string fileName = "document-multiline-tags-test.ets";
    const auto offset = src.find("CCC");
    ASSERT_NE(offset, std::string::npos);

    auto quickInfo = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset, fileName);
    ASSERT_NE(quickInfo, QuickInfo());

    std::vector<SymbolDisplayPart> expectedDocument;
    expectedDocument.emplace_back("Function docs.", "plaintext");
    ASSERT_EQ(quickInfo.GetDocument(), expectedDocument);

    std::vector<DocTagInfo> expectedTags;
    expectedTags.emplace_back("param", "value first line second line of param.");
    expectedTags.emplace_back("returns", "result line1 result line2");
    ASSERT_EQ(quickInfo.GetTags(), expectedTags);

    init.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForDocumentWithMultilineTags1)
{
    std::vector<std::string> files = {"document-multiline-tags-test-export.ets",
                                      "document-multiline-tags-test-import.ets"};
    std::vector<std::string> texts = {R"(
/**
* Function docs.
* @param value first line
* second line of param.
* @returns result line1
* result line2
*/
export class CCC {}
)",
                                      R"(
import {CCC} from './document-multiline-tags-test-export'
let a : CCC = new CCC()
)"};
    auto filePaths = CreateTempFile(files, texts);

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string fileName = "document-multiline-tags-test-import.ets";
    const auto offset = 68;

    auto quickInfo = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(ctx, offset, fileName);
    ASSERT_NE(quickInfo, QuickInfo());
    ASSERT_EQ(quickInfo.GetFileName(), filePaths[0]);

    std::vector<SymbolDisplayPart> expectedDocument;
    expectedDocument.emplace_back("Function docs.", "plaintext");
    ASSERT_EQ(quickInfo.GetDocument(), expectedDocument);

    std::vector<DocTagInfo> expectedTags;
    expectedTags.emplace_back("param", "value first line second line of param.");
    expectedTags.emplace_back("returns", "result line1 result line2");
    ASSERT_EQ(quickInfo.GetTags(), expectedTags);

    init.DestroyContext(ctx);
}

}  // namespace
