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

TEST_F(LspQuickInfoTests, StdLibMapGet)
{
    std::vector<std::string> files = {"quick_info_map_get.ets"};
    std::vector<std::string> texts = {R"(let map = new Map<string, number>();
map.set("a", 1);
let a = map.get("a");
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 68;
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getQuickInfoAtPosition(filePaths[0].c_str(), ctx, offset);
    auto displayParts = res.GetDisplayParts();
    auto expectedDisplayParts = std::vector<SymbolDisplayPart> {
        SymbolDisplayPart {"Map", "className"},
        SymbolDisplayPart {".", "punctuation"},
        SymbolDisplayPart {"get", "functionName"},
        SymbolDisplayPart {"(", "punctuation"},
        SymbolDisplayPart {"key", "functionParameter"},
        SymbolDisplayPart {":", "punctuation"},
        SymbolDisplayPart {" ", "space"},
        SymbolDisplayPart {"K", "typeParameter"},
        SymbolDisplayPart {")", "punctuation"},
        SymbolDisplayPart {":", "punctuation"},
        SymbolDisplayPart {" ", "space"},
        SymbolDisplayPart {"V", "returnType"},
        SymbolDisplayPart {"|", "punctuation"},
        SymbolDisplayPart {" ", "space"},
        SymbolDisplayPart {"undefined", "returnType"},
    };
    ASSERT_EQ(displayParts, expectedDisplayParts);
    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, StdLibMapSet)
{
    std::vector<std::string> files = {"quick_info_map_set.ets"};
    std::vector<std::string> texts = {R"(let map = new Map<string, number>();
map.set("a", 1);
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 43;
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getQuickInfoAtPosition(filePaths[0].c_str(), ctx, offset);
    auto displayParts = res.GetDisplayParts();
    auto expectedDisplayParts = std::vector<SymbolDisplayPart> {
        SymbolDisplayPart {"Map", "className"},
        SymbolDisplayPart {".", "punctuation"},
        SymbolDisplayPart {"set", "functionName"},
        SymbolDisplayPart {"(", "punctuation"},
        SymbolDisplayPart {"key", "functionParameter"},
        SymbolDisplayPart {":", "punctuation"},
        SymbolDisplayPart {" ", "space"},
        SymbolDisplayPart {"K", "typeParameter"},
        SymbolDisplayPart {",", "punctuation"},
        SymbolDisplayPart {" ", "space"},
        SymbolDisplayPart {"val", "functionParameter"},
        SymbolDisplayPart {":", "punctuation"},
        SymbolDisplayPart {" ", "space"},
        SymbolDisplayPart {"V", "typeParameter"},
        SymbolDisplayPart {")", "punctuation"},
        SymbolDisplayPart {":", "punctuation"},
        SymbolDisplayPart {" ", "space"},
        SymbolDisplayPart {"this", "returnType"},
    };
    ASSERT_EQ(displayParts, expectedDisplayParts);
    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForEnum)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("enum-test.ets", ES2PANDA_STATE_CHECKED, "enum MyEnum { A, B, C }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::CLASS_DEFINITION &&
               node->AsClassDefinition()->Ident()->Name() == "MyEnum";
    };
    auto found = ast->FindChild(checkFunc);
    auto enumDecl = found->AsClassDefinition()->OrigEnumDecl()->AsTSEnumDeclaration();
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForEnum(enumDecl);
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("enum", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("MyEnum", "enumName");
    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForClass1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("quick-info-test.ets", ES2PANDA_STATE_CHECKED,
                                  "class Test {\n  private _a: number = 1;\n  public get a(): number {\n    "
                                  "return this._a;\n  }\n  public static ccc:number = 1\n\n  constructor(a : "
                                  "number) {\n  }\n}\n\nlet a = 1\nlet test: Test = new Test(a)\nlet t_a = test.a");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::CLASS_DECLARATION &&
               node->AsClassDeclaration()->Definition()->InternalName() != "quick-info-test.ETSGLOBAL";
    };
    auto found = ast->FindChild(checkFunc);
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForClass(found);
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

TEST_F(LspQuickInfoTests, CreateDisplayForUnionTypeAlias)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("union-type-alias-test.ets", ES2PANDA_STATE_CHECKED,
                                                      "type TestUnion = string | number;");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION;
    };
    auto found = ast->FindChild(checkFunc);
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForTypeAlias(found);
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

TEST_F(LspQuickInfoTests, CreateDisplayForTypeAlias)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("type-alias-test.ets", ES2PANDA_STATE_CHECKED, "type TestType = string;");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION;
    };
    auto found = ast->FindChild(checkFunc);
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForTypeAlias(found);
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

TEST_F(LspQuickInfoTests, CreateDisplayForInterface)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(
        "quick-info-test.ets", ES2PANDA_STATE_CHECKED,
        "interface Inner { key : string; }\ninterface Outer { inner : Inner; keyValue : number; }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::TS_INTERFACE_DECLARATION;
    };
    auto found = ast->FindChild(checkFunc);
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForInterface(found);
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

TEST_F(LspQuickInfoTests, CreateDisplayForTypeAliasTypeParameter)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("type-alias-type-param-test.ets", ES2PANDA_STATE_CHECKED, "type list<T> = T[]");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::TS_TYPE_PARAMETER;
    };
    auto found = ast->FindChild(checkFunc);
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForTypeParameter(found);
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

TEST_F(LspQuickInfoTests, CreateDisplayForClassDeclarationTypeParameter)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("class-type-param-test.ets", ES2PANDA_STATE_CHECKED,
                                                      "class Queue<T> { private items: T[] = []; }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::TS_TYPE_PARAMETER;
    };
    auto found = ast->FindChild(checkFunc);
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForTypeParameter(found);
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

TEST_F(LspQuickInfoTests, CreateDisplayForMethodDefinition)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("method-definition-test.ets", ES2PANDA_STATE_CHECKED,
                                                      "function func(a: number): string { return 'function1'; }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(context->GetChecker());
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::METHOD_DEFINITION &&
               node->AsMethodDefinition()->Id()->Name() != "main" &&
               node->AsMethodDefinition()->Id()->Name() != "_$init$_";
    };
    auto found = ast->FindChild(checkFunc);
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForMethodDefinition(found, "", checker);
    std::vector<SymbolDisplayPart> expected;

    // Expected parts for the method definition
    expected.emplace_back("function", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("func", "functionName");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back("a", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeParameter");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("string", "returnType");

    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForScriptFunctionTypeParameter)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("function-type-param-test.ets", ES2PANDA_STATE_CHECKED,
                                                      "function identity<T, D>(arg: T): T { return arg }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::TS_TYPE_PARAMETER;
    };
    auto found = ast->FindChild(checkFunc);
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForTypeParameter(found);
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("T", "typeParameter");
    expected.emplace_back(" ", "space");
    expected.emplace_back("in", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("identity", "functionName");
    expected.emplace_back("<", "punctuation");
    expected.emplace_back("T", "typeParameter");
    expected.emplace_back(",", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("D", "typeParameter");
    expected.emplace_back(">", "punctuation");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back("arg", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("T", "typeParameter");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("T", "returnType");
    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForClassProperty1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("class-property-test2.ets", ES2PANDA_STATE_CHECKED,
                                  "class MyClass {\n  public myProp: number = 0;\n}\n let obj = new MyClass();");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::CLASS_PROPERTY &&
               node->AsClassProperty()->Key()->AsIdentifier()->Name() != "ETSGLOBAL";
    };
    auto found = ast->FindChild(checkFunc);
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForClassProperty(found);
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("let", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("obj", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("MyClass", "typeName");

    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForClassProperty2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("class-property-test.ets", ES2PANDA_STATE_CHECKED,
                                                      "class MyClass {\n  public myProp: number = 0;\n}");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::CLASS_PROPERTY &&
               node->AsClassProperty()->Key()->AsIdentifier()->Name() != "ETSGLOBAL";
    };
    auto found = ast->FindChild(checkFunc);
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForClassProperty(found);
    std::vector<SymbolDisplayPart> expected;

    expected.emplace_back("MyClass", "className");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("myProp", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeName");

    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}

TEST_F(LspQuickInfoTests, CreateDisplayForParameterExpression)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("parameter-expression-test.ets", ES2PANDA_STATE_CHECKED,
                                                      "function func(param1: string, param2: number) {}");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    auto checkFunc = [](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::ETS_PARAMETER_EXPRESSION &&
               node->AsETSParameterExpression()->Name() != "param2";
    };
    auto found = ast->FindChild(checkFunc);
    std::vector<SymbolDisplayPart> display = ark::es2panda::lsp::CreateDisplayForETSParameterExpression(found);
    std::vector<SymbolDisplayPart> expected;

    expected.emplace_back("param1", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("string", "typeName");

    ASSERT_EQ(expected, display);
    initializer.DestroyContext(ctx);
}
}  // namespace