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

#include "lsp_api_test.h"
#include "lsp/include/completions.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/api.h"
#include "lsp/include/completions_details.h"
#include "lsp/include/quick_info.h"
#include "lsp/include/suggestion_diagnostics.h"
#include "compiler/lowering/util.h"

namespace {

class LSPCompletionsEntryDetailsTests : public LSPAPITests {};

using ark::es2panda::lsp::Initializer;

TEST_F(LSPCompletionsEntryDetailsTests, GetCompletionEntryDetails0)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("completion_entry_details.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(enum MyStrings { A = 'hello' };)");
    size_t const offset = 17;
    LSPAPI const *lspApi = GetImpl();
    const char *entryName = "MyStrings";
    auto completionEntryDetails =
        lspApi->getCompletionEntryDetails(entryName, "completion_entry_details.ets", ctx, offset);
    ASSERT_NE(completionEntryDetails, CompletionEntryDetails());
    std::vector<SymbolDisplayPart> source {};
    std::vector<SymbolDisplayPart> sourceDisplay {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind = "enum";
    const std::string kindModifiers = "final";
    const std::string expectedFileName = "completion_entry_details.ets";

    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("enum", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("MyStrings", "enumName");

    auto expectedCompletionEntryDetails = CompletionEntryDetails(entryName, kind, kindModifiers, expected, document,
                                                                 source, sourceDisplay, expectedFileName);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(completionEntryDetails, expectedCompletionEntryDetails);
}

TEST_F(LSPCompletionsEntryDetailsTests, GetCompletionEntryDetails1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("completion_entry_details1.ets", ES2PANDA_STATE_CHECKED,
                                                      "class MyClass {\n  public myProp: number = 0;\n}");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 9;
    LSPAPI const *lspApi = GetImpl();
    const char *entryName = "MyClass";
    const std::string fileName = "completion_entry_details1.ets";
    auto completionEntryDetails =
        lspApi->getCompletionEntryDetails(entryName, "completion_entry_details1.ets", ctx, offset);
    ASSERT_NE(completionEntryDetails, CompletionEntryDetails());
    std::vector<SymbolDisplayPart> source {};
    std::vector<SymbolDisplayPart> sourceDisplay {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind = "class";
    const std::string kindModifiers;
    const std::string expectedFileName = "completion_entry_details1.ets";

    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("class", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("MyClass", "className");

    auto expectedCompletionEntryDetails = CompletionEntryDetails(entryName, kind, kindModifiers, expected, document,
                                                                 source, sourceDisplay, expectedFileName);
    ASSERT_EQ(completionEntryDetails, expectedCompletionEntryDetails);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsEntryDetailsTests, GetCompletionEntryDetails2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("completion_entry_details2.ets", ES2PANDA_STATE_CHECKED,
                                  "interface objI { key : string; }\nlet obj : objI = { key:\"valueaaaaaaaaa,\" }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 7;
    LSPAPI const *lspApi = GetImpl();
    const char *entryName = "objI";
    const std::string fileName = "completion_entry_details2.ets";
    auto completionEntryDetails =
        lspApi->getCompletionEntryDetails(entryName, "completion_entry_details2.ets", ctx, offset);
    ASSERT_NE(completionEntryDetails, CompletionEntryDetails());
    std::vector<SymbolDisplayPart> source {};
    std::vector<SymbolDisplayPart> sourceDisplay {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind = "interface";
    const std::string kindModifiers = "static public";
    const std::string expectedFileName = "completion_entry_details2.ets";
    std::vector<SymbolDisplayPart> expected;

    expected.emplace_back("interface", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("objI", "className");

    auto expectedCompletionEntryDetails = CompletionEntryDetails(entryName, kind, kindModifiers, expected, document,
                                                                 source, sourceDisplay, expectedFileName);
    ASSERT_EQ(completionEntryDetails, expectedCompletionEntryDetails);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsEntryDetailsTests, GetCompletionEntryDetails3)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("completion_entry_details9.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(function name (params: string) {
  new CC().getName()
}

class CC {
  getName() {
  }
})");
    size_t const offset = 79;
    LSPAPI const *lspApi = GetImpl();
    const char *entryName = "getName";
    auto completionEntryDetails =
        lspApi->getCompletionEntryDetails(entryName, "completion_entry_details9.ets", ctx, offset);
    ASSERT_NE(completionEntryDetails, CompletionEntryDetails());
    std::vector<SymbolDisplayPart> source {};
    std::vector<SymbolDisplayPart> sourceDisplay {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind = "method";
    const std::string kindModifiers = "public";
    const std::string expectedFileName = "completion_entry_details9.ets";

    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("CC", "className");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("getName", "functionName");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("void", "returnType");

    auto expectedCompletionEntryDetails = CompletionEntryDetails(entryName, kind, kindModifiers, expected, document,
                                                                 source, sourceDisplay, expectedFileName);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(completionEntryDetails, expectedCompletionEntryDetails);
}

TEST_F(LSPCompletionsEntryDetailsTests, GetCompletionEntryDetails4)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("completion_entry_details10.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(interface DuttonInterface {
  ff(): number;
}

declare const Dutton: DuttonInterface;)");
    size_t const offset = 66;
    LSPAPI const *lspApi = GetImpl();
    const char *entryName = "Dutton";
    auto completionEntryDetails =
        lspApi->getCompletionEntryDetails(entryName, "completion_entry_details10.ets", ctx, offset);
    ASSERT_NE(completionEntryDetails, CompletionEntryDetails());
    std::vector<SymbolDisplayPart> source {};
    std::vector<SymbolDisplayPart> sourceDisplay {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind = "property";
    const std::string kindModifiers = "static public declare const";
    const std::string expectedFileName = "completion_entry_details10.ets";

    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("const", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("Dutton", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("DuttonInterface", "typeName");

    auto expectedCompletionEntryDetails = CompletionEntryDetails(entryName, kind, kindModifiers, expected, document,
                                                                 source, sourceDisplay, expectedFileName);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(completionEntryDetails, expectedCompletionEntryDetails);
}

TEST_F(LSPCompletionsEntryDetailsTests, GetCompletionEntryDetails5)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("completion_entry_details11.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(class DuttonInterface {
  ff : number;
  constructor(ff:number) {
    this.ff = ff;
  }
  static buttonRun(value: string | number | boolean):number{
    return 1;
  }
}

 function aa() {
   let Dutton: DuttonInterface = new DuttonInterface(1);
   let  a = DuttonInterface.buttonRun(1)
})");
    size_t const offset = 280;
    LSPAPI const *lspApi = GetImpl();
    const char *entryName = "buttonRun";
    auto completionEntryDetails =
        lspApi->getCompletionEntryDetails(entryName, "completion_entry_details11.ets", ctx, offset);
    ASSERT_NE(completionEntryDetails, CompletionEntryDetails());
    std::vector<SymbolDisplayPart> source {};
    std::vector<SymbolDisplayPart> sourceDisplay {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind = "method";
    const std::string kindModifiers = "static public";
    const std::string expectedFileName = "completion_entry_details11.ets";

    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("DuttonInterface", "className");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("buttonRun", "functionName");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back("value", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("string | number | boolean", "typeParameter");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "returnType");

    auto expectedCompletionEntryDetails = CompletionEntryDetails(entryName, kind, kindModifiers, expected, document,
                                                                 source, sourceDisplay, expectedFileName);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(completionEntryDetails, expectedCompletionEntryDetails);
}

TEST_F(LSPCompletionsEntryDetailsTests, GetCompletionEntryDetails6)
{
    Initializer initializer = Initializer();
    std::string expectedFileName = "completion_entry_details12.ets";
    es2panda_Context *ctx =
        initializer.CreateContext(expectedFileName.c_str(), ES2PANDA_STATE_CHECKED,
                                  R"(export const mqw1: [string, number, number] = ['Alice', 30, 10];)");
    size_t const offset = 16;
    const char *entryName = "mqw1";
    LSPAPI const *lspApi = GetImpl();
    auto completionEntryDetails = lspApi->getCompletionEntryDetails(entryName, expectedFileName.c_str(), ctx, offset);
    std::vector<SymbolDisplayPart> source {};
    std::vector<SymbolDisplayPart> sourceDisplay {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind = "property";
    const std::string kindModifiers = "static public const export";
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("const", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("mqw1", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("[string, number, number]", "typeName");
    auto expectedCompletionEntryDetails = CompletionEntryDetails(entryName, kind, kindModifiers, expected, document,
                                                                 source, sourceDisplay, expectedFileName);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(completionEntryDetails, expectedCompletionEntryDetails);
}

TEST_F(LSPCompletionsEntryDetailsTests, GetCompletionEntryDetailsForSpecialCharacters)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("completion_entry_details13.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(class DuttonInterface {
  ff : number;
  constructor(ff:number) {
    this.ff = ff;
  }
  //中文测试
  static buttonRun(value: string | number | boolean):number{
    return 1;
  }
}

 function aa() {
   let Dutton: DuttonInterface = new DuttonInterface(1);
   //中文测试
   let  a = DuttonInterface.buttonRun(1)
})");
    size_t const offset = 299;
    LSPAPI const *lspApi = GetImpl();
    const char *entryName = "buttonRun";
    auto completionEntryDetails =
        lspApi->getCompletionEntryDetails(entryName, "completion_entry_details13.ets", ctx, offset);
    ASSERT_NE(completionEntryDetails, CompletionEntryDetails());
    std::vector<SymbolDisplayPart> source {};
    std::vector<SymbolDisplayPart> sourceDisplay {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind = "method";
    const std::string kindModifiers = "static public";
    const std::string expectedFileName = "completion_entry_details13.ets";

    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("DuttonInterface", "className");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("buttonRun", "functionName");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back("value", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("string | number | boolean", "typeParameter");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "returnType");

    auto expectedCompletionEntryDetails = CompletionEntryDetails(entryName, kind, kindModifiers, expected, document,
                                                                 source, sourceDisplay, expectedFileName);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(completionEntryDetails, expectedCompletionEntryDetails);
}

TEST_F(LSPCompletionsEntryDetailsTests, CreateDisplayForClass)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("completion_entry_details3.ets", ES2PANDA_STATE_CHECKED,
                                  "class Test {\n  private _a: number = 1;\n  public get a(): number {\n    "
                                  "return this._a;\n  }\n  public static ccc:number = 1\n\n  constructor(a : "
                                  "number) {\n  }\n}\n\nlet a = 1\nlet test: Test = new Test(a)\nlet t_a = test.a");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    const char *entryName = "Test";
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [entryName](ark::es2panda::ir::AstNode *node) {
        return ark::es2panda::lsp::HasPropertyAccessExpressionWithName(node, entryName);
    };
    auto found = ast->FindChild(checkFunc);
    auto targetNode = ark::es2panda::compiler::DeclarationFromIdentifier(found->AsIdentifier());
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForClass(targetNode);
    std::vector<SymbolDisplayPart> expected;
    auto keyword1 = SymbolDisplayPart("class", "keyword");
    auto keyword2 = SymbolDisplayPart(" ", "space");
    auto keyword3 = SymbolDisplayPart("Test", "className");
    expected.push_back(keyword1);
    expected.push_back(keyword2);
    expected.push_back(keyword3);
    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsEntryDetailsTests, CreateDisplayForClassDeclarationTypeParameter)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("completion_entry_details4.ets", ES2PANDA_STATE_CHECKED,
                                                      "class Queue<T> { private items: T[] = []; }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    const char *entryName = "T";
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [entryName](ark::es2panda::ir::AstNode *node) {
        return ark::es2panda::lsp::HasPropertyAccessExpressionWithName(node, entryName);
    };
    auto found = ast->FindChild(checkFunc);
    auto targetNode = ark::es2panda::compiler::DeclarationFromIdentifier(found->AsIdentifier());
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForTypeParameter(targetNode);
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("T", "typeParameter");
    expected.emplace_back(" ", "space");
    expected.emplace_back("in", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("Queue", "className");
    expected.emplace_back("<", "punctuation");
    expected.emplace_back("T", "typeParameter");
    expected.emplace_back(">", "punctuation");
    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsEntryDetailsTests, CreateDisplayForUnionTypeAlias)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("completion_entry_details5.ets", ES2PANDA_STATE_CHECKED,
                                                      "type TestUnion = string | number;");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    const char *entryName = "TestUnion";
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [entryName](ark::es2panda::ir::AstNode *node) {
        return ark::es2panda::lsp::HasPropertyAccessExpressionWithName(node, entryName);
    };
    auto found = ast->FindChild(checkFunc);
    auto targetNode = ark::es2panda::compiler::DeclarationFromIdentifier(found->AsIdentifier());
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForTypeAlias(targetNode);
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("type", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("TestUnion", "className");
    expected.emplace_back(" ", "space");
    expected.emplace_back("=", "operator");
    expected.emplace_back(" ", "space");
    expected.emplace_back("string | number", "typeName");
    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsEntryDetailsTests, CreateDisplayForTypeAlias)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("completion_entry_details6.ets", ES2PANDA_STATE_CHECKED, "type TestType = string;");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    const char *entryName = "TestType";
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [entryName](ark::es2panda::ir::AstNode *node) {
        return ark::es2panda::lsp::HasPropertyAccessExpressionWithName(node, entryName);
    };
    auto found = ast->FindChild(checkFunc);
    auto targetNode = ark::es2panda::compiler::DeclarationFromIdentifier(found->AsIdentifier());
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForTypeAlias(targetNode);
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("type", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("TestType", "className");
    expected.emplace_back(" ", "space");
    expected.emplace_back("=", "operator");
    expected.emplace_back(" ", "space");
    expected.emplace_back("string", "typeName");
    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsEntryDetailsTests, CreateDisplayForInterface)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(
        "completion_entry_details7.ets", ES2PANDA_STATE_CHECKED,
        "interface Inner { key : string; }\ninterface Outer { inner : Inner; keyValue : number; }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    const char *entryName = "Inner";
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [entryName](ark::es2panda::ir::AstNode *node) {
        return ark::es2panda::lsp::HasPropertyAccessExpressionWithName(node, entryName);
    };
    auto found = ast->FindChild(checkFunc);
    auto targetNode = ark::es2panda::compiler::DeclarationFromIdentifier(found->AsIdentifier());
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForInterface(targetNode);
    std::vector<SymbolDisplayPart> expected;
    auto keyword1 = SymbolDisplayPart("interface", "keyword");
    auto keyword2 = SymbolDisplayPart(" ", "space");
    auto keyword3 = SymbolDisplayPart("Inner", "className");
    expected.push_back(keyword1);
    expected.push_back(keyword2);
    expected.push_back(keyword3);
    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsEntryDetailsTests, CreateDisplayForTypeAliasTypeParameter)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("completion_entry_details8.ets", ES2PANDA_STATE_CHECKED, "type list<T> = T[]");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    const char *entryName = "T";
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [entryName](ark::es2panda::ir::AstNode *node) {
        return ark::es2panda::lsp::HasPropertyAccessExpressionWithName(node, entryName);
    };
    auto found = ast->FindChild(checkFunc);
    auto targetNode = ark::es2panda::compiler::DeclarationFromIdentifier(found->AsIdentifier());
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForTypeParameter(targetNode);
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("T", "typeParameter");
    expected.emplace_back(" ", "space");
    expected.emplace_back("in", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("type", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("list", "typeName");
    expected.emplace_back("<", "punctuation");
    expected.emplace_back("T", "typeParameter");
    expected.emplace_back(">", "punctuation");
    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}

}  // namespace