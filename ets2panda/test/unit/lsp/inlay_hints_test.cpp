/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cstddef>
#include <string>
#include <vector>
#include <lexer/lexer.h>
#include <lexer/token/letters.h>
#include <lexer/token/sourceLocation.h>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/inlay_hints.h"
#include "public/es2panda_lib.h"

namespace {
using ark::es2panda::lsp::Initializer;

class LSPInlayHintsTests : public LSPAPITests {};

TEST_F(LSPInlayHintsTests, VisitCallOrNewExpressionTest)
{
    std::vector<std::string> files = {"call_file_test.ets"};

    std::vector<std::string> fileContent = {R"(
        function foo(param1: number, param2: number) {
        }

        function bar() {
            foo(10, 20); // Call expression
        }
    )"};

    auto filePath = CreateTempFile(files, fileContent);

    using ark::es2panda::lsp::UserPreferences;
    UserPreferences preferences = UserPreferences::GetDefaultUserPreferences();

    preferences.SetQuotePreference(UserPreferences::QuotePreference::DOUBLE);
    preferences.SetIncludePackageJsonAutoImports(UserPreferences::IncludePackageJsonAutoImports::ON);
    preferences.SetDisableSuggestions(true);
    preferences.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::ALL);

    InlayHintList result = {};

    Initializer initializer;
    const auto ctx = initializer.CreateContext(filePath[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    auto *parent = reinterpret_cast<ark::es2panda::ir::AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);

    ark::es2panda::ir::AstNode *callExprNode = nullptr;
    parent->FindChild([&callExprNode, parent, &result](ark::es2panda::ir::AstNode *childNode) {
        if (childNode->IsCallExpression()) {
            callExprNode = childNode;
            ark::es2panda::lsp::GetCallExpTypeForHints(callExprNode, parent, &result);
        }
        return false;
    });
    const size_t num1 = 108;
    const size_t num2 = 112;
    const std::string p1 = "param1";
    const std::string p2 = "param2";
    ASSERT_NE(callExprNode, nullptr);
    std::vector<InlayHint> expectedHints = {{p1, num1, InlayHintKind::PARAMETER, false, true},
                                            {p2, num2, InlayHintKind::PARAMETER, false, true}};

    ASSERT_EQ(result.hints.size(), expectedHints.size());
    for (size_t i = 0; i < expectedHints.size(); i++) {
        ASSERT_EQ(result.hints[i].text, expectedHints[i].text);
        ASSERT_EQ(result.hints[i].number, expectedHints[i].number);
        ASSERT_EQ(result.hints[i].kind, expectedHints[i].kind);
        ASSERT_EQ(result.hints[i].whitespaceBefore, expectedHints[i].whitespaceBefore);
        ASSERT_EQ(result.hints[i].whitespaceAfter, expectedHints[i].whitespaceAfter);
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LSPInlayHintsTests, VisitEnumMemberTest)
{
    const std::vector<std::string> files = {"enum_file.ets"};

    const std::vector<std::string> fileContent = {R"(
enum Color {
    RED = 1,
    GREEN = 2,
    BLUE = 3
};
enum text {
    name = "test",
    className = "class",
    color = "blue"
};

let myColor = Color.GREEN;
let myColor1 = Color.BLUE;
let name1 = text.name;
let classText = text.className;

)"};

    const auto filePath = CreateTempFile(files, fileContent);

    InlayHintList result = {};
    const size_t size = 4;
    const std::string enum2 = "2";
    const std::string enum3 = "3";
    const std::string enumtest = "test";
    const std::string enumclass = "class";
    const size_t i0 = 0;
    const size_t i1 = 1;
    const size_t i2 = 2;
    const size_t i3 = 3;

    Initializer initializer;
    const auto ctx = initializer.CreateContext(filePath[i0].c_str(), ES2PANDA_STATE_CHECKED);
    const auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    const auto *parent = reinterpret_cast<ark::es2panda::ir::AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);

    parent->FindChild([parent, &result](ark::es2panda::ir::AstNode *childNode) {
        if (childNode->IsAssignmentExpression()) {
            ark::es2panda::lsp::GetEnumTypeForHints(childNode, parent, &result);
        }
        return false;
    });
    initializer.DestroyContext(ctx);
    ASSERT_EQ(result.hints.size(), size);
    ASSERT_EQ(result.hints[i0].text, enum2);
    ASSERT_EQ(result.hints[i1].text, enum3);
    ASSERT_EQ(result.hints[i2].text, enumtest);
    ASSERT_EQ(result.hints[i3].text, enumclass);
}

TEST_F(LSPInlayHintsTests, VisitFunctionLikeForParameterTypeTest)
{
    const std::vector<std::string> files = {"function_test.ets"};
    const std::vector<std::string> fileContent = {R"(
        function add(a: number, b: number): number {
            return a + b;
        }

        function greet(name: string, age: number): string {
            return `Hello, ${name}! You are ${age} years old.`;
        }

        let sum = add(5, 10);
        let message = greet("Alice", 30);
    )"};
    const std::string voidString = "void";
    const std::string numberString = "number";
    const std::string stdString = "string";
    const size_t index1 = 31;
    const size_t index2 = 42;
    const size_t index3 = 126;
    const size_t index4 = 139;
    const size_t i0 = 0;
    const size_t i1 = 1;
    const size_t i2 = 2;
    const size_t i3 = 3;
    const auto filePaths = CreateTempFile(files, fileContent);
    ASSERT_EQ(filePaths.size(), i1);
    InlayHintList result = {};
    Initializer initializer;

    const auto ctx = initializer.CreateContext(filePaths[i0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    const auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);

    const auto *parent = reinterpret_cast<ark::es2panda::ir::AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);

    parent->FindChild([&result](ark::es2panda::ir::AstNode *childNode) {
        if (childNode->IsMethodDefinition()) {
            ark::es2panda::lsp::GetFunctionParameterTypeForHints(childNode, &result);
        }
        return false;
    });

    ASSERT_EQ(result.hints[i0].text, numberString);
    ASSERT_EQ(result.hints[i0].number, index1);
    ASSERT_EQ(result.hints[i1].text, numberString);
    ASSERT_EQ(result.hints[i1].number, index2);
    ASSERT_EQ(result.hints[i2].text, stdString);
    ASSERT_EQ(result.hints[i2].number, index3);
    ASSERT_EQ(result.hints[i3].text, numberString);
    ASSERT_EQ(result.hints[i3].number, index4);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPInlayHintsTests, VisitFunctionDeclarationLikeForReturnTypeTest1)
{
    const std::vector<std::string> files = {"function_return_type_test1.ets"};
    const std::vector<std::string> fileContent = {R"(
        function add(a: number, b: number): number {
            return a + b;
        }
        const multiply = (a: number, b: number): number => {
            return a * b;
        };
    )"};

    const std::string doubleString = "Double";
    const size_t addIndex = 89;
    const size_t multiplyIndex = 186;
    const size_t i0 = 0;
    const size_t i1 = 1;
    const size_t i2 = 2;
    const auto filePaths = CreateTempFile(files, fileContent);
    ASSERT_EQ(filePaths.size(), i1);
    InlayHintList result;
    Initializer initializer;
    const auto ctx = initializer.CreateContext(filePaths[i0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    const auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    const auto *parent = reinterpret_cast<ark::es2panda::ir::AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);
    parent->FindChild([&result](ark::es2panda::ir::AstNode *childNode) {
        if (ark::es2panda::lsp::IsSignatureSupportingReturnAnnotation(childNode)) {
            ark::es2panda::lsp::GetFunctionReturnTypeForHints(childNode, &result);
        }
        return false;
    });
    ASSERT_EQ(result.hints[i1].text, doubleString);
    ASSERT_EQ(result.hints[i1].number, addIndex);
    ASSERT_EQ(result.hints[i2].text, doubleString);
    ASSERT_EQ(result.hints[i2].number, multiplyIndex);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPInlayHintsTests, VisitFunctionDeclarationLikeForReturnTypeTest2)
{
    const std::vector<std::string> files = {"function_return_type_test.ets"};
    const std::vector<std::string> fileContent = {R"(
        function greet(name: string): string {
            return `Hello, ${name}!`;
        }
        const sayHello = (): void => {
            console.log("Hello!");
        };

    )"};
    const std::string voidString = "void";
    const std::string stdString = "String";

    const size_t greetIndex = 95;
    const size_t sayHelloIndex = 179;
    const size_t i0 = 0;
    const size_t i1 = 1;
    const size_t i2 = 2;

    const auto filePaths = CreateTempFile(files, fileContent);
    ASSERT_EQ(filePaths.size(), i1);
    InlayHintList result;
    Initializer initializer;
    const auto ctx = initializer.CreateContext(filePaths[i0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    const auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    const auto *parent = reinterpret_cast<ark::es2panda::ir::AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);
    parent->FindChild([&result](ark::es2panda::ir::AstNode *childNode) {
        if (ark::es2panda::lsp::IsSignatureSupportingReturnAnnotation(childNode)) {
            ark::es2panda::lsp::GetFunctionReturnTypeForHints(childNode, &result);
        }
        return false;
    });
    ASSERT_EQ(result.hints[i1].text, stdString);
    ASSERT_EQ(result.hints[i1].number, greetIndex);
    ASSERT_EQ(result.hints[i2].text, voidString);
    ASSERT_EQ(result.hints[i2].number, sayHelloIndex);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPInlayHintsTests, VisitVariableLikeDeclarationTest)
{
    const std::vector<std::string> files = {"variable_test.ets"};
    const std::vector<std::string> fileContent = {R"(
        let x: number = 10;
        let y = "Hello";
        let z = true;
        let obj = { a: 1, b: 2 };
        let arr = [1, 2, 3];
    )"};
    const std::string numberString = "number";
    const std::string stdString = "string";
    const std::string boolString = "boolean";
    const size_t index1 = 27;
    const size_t index2 = 52;
    const size_t index3 = 74;
    const size_t i0 = 0;
    const size_t i1 = 1;
    const size_t i2 = 2;
    const auto filePaths = CreateTempFile(files, fileContent);
    ASSERT_EQ(filePaths.size(), i1);
    InlayHintList result;
    Initializer initializer;
    const auto ctx = initializer.CreateContext(filePaths[i0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    const auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    const auto *parent = reinterpret_cast<ark::es2panda::ir::AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);
    ark::es2panda::lsp::UserPreferences preferences;
    preferences.SetIncludeInlayVariableTypeHintsWhenTypeMatchesName(false);
    parent->FindChild([&result](ark::es2panda::ir::AstNode *childNode) {
        if (childNode->IsAssignmentExpression()) {
            ark::es2panda::lsp::GetVariableDeclarationTypeForHints(childNode, &result);
        }
        if (result.hints.empty()) {
        }
        return false;
    });
    ASSERT_EQ(result.hints[i0].text, numberString);
    ASSERT_EQ(result.hints[i0].number, index1);
    ASSERT_EQ(result.hints[i1].text, stdString);
    ASSERT_EQ(result.hints[i1].number, index2);
    ASSERT_EQ(result.hints[i2].text, boolString);
    ASSERT_EQ(result.hints[i2].number, index3);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPInlayHintsTests, StdLibMapGet)
{
    std::vector<std::string> files = {"getSignatureHelpItemsTest_map.ets"};
    std::vector<std::string> texts = {R"(let map = new Map<string, number>();
map.set("a", 1);
let a = map.get("a");
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    const size_t start = 66;
    const size_t length = 8;
    auto res = lspApi->provideInlayHints(ctx, new TextSpan {start, length});
    const int expectedNumber = 70;
    ASSERT_EQ(res.hints.size(), 1);
    ASSERT_EQ(res.hints[0].text, "key");
    ASSERT_EQ(res.hints[0].number, expectedNumber);
    ASSERT_EQ(res.hints[0].kind, InlayHintKind::PARAMETER);
    ASSERT_FALSE(res.hints[0].whitespaceBefore);
    ASSERT_TRUE(res.hints[0].whitespaceAfter);
    initializer.DestroyContext(ctx);
}

}  // namespace