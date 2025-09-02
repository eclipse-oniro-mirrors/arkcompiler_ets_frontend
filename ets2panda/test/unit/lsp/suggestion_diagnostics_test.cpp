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

#include "lsp/include/suggestion_diagnostics.h"
#include <gtest/gtest.h>
#include "lsp/include/internal_api.h"
#include "lsp/include/classifier.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
namespace {
using ark::es2panda::lsp::Initializer;

class LspSuggestionTests : public LSPAPITests {};

TEST_F(LspSuggestionTests, canBeConvertedToAsync)
{
    const char *source = "function add(x: number, y: number): number\n {return x + y;};";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("sug-diag.ets", ES2PANDA_STATE_CHECKED, source);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto node = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->Type() == ark::es2panda::ir::AstNodeType::FUNCTION_EXPRESSION;
    });
    EXPECT_TRUE(ark::es2panda::lsp::CanBeConvertedToAsync(node));
    initializer.DestroyContext(ctx);
}

TEST_F(LspSuggestionTests, isFixablePromiseArgument)
{
    const char *source =
        "function add(x: number, y: number): number\n {return x + y;}"
        "\nconsole.log(add(1,2));";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("sug-diag.ets", ES2PANDA_STATE_CHECKED, source);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    std::unordered_map<std::string, bool> visitedNestedConvertibleFunctions;
    auto node = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->Type() == ark::es2panda::ir::AstNodeType::FUNCTION_EXPRESSION;
    });
    EXPECT_TRUE(ark::es2panda::lsp::IsFixablePromiseArgument(node, visitedNestedConvertibleFunctions));
    initializer.DestroyContext(ctx);
}
TEST_F(LspSuggestionTests, hasSupportedNumberOfArguments)
{
    const char *source =
        "function fetchData():Promise<string>{\n"
        "\nreturn Promise.resolve(\"Success\");\n}\nfetchData().then();";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("token-pos-literal.ets", ES2PANDA_STATE_CHECKED, source);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    std::vector<ark::es2panda::ir::AstNode *> callExprs;
    ast->FindChild([&callExprs](ark::es2panda::ir::AstNode *node) {
        if (node->Type() == ark::es2panda::ir::AstNodeType::CALL_EXPRESSION) {
            callExprs.push_back(node);
        }
        return false;
    });
    EXPECT_TRUE(ark::es2panda::lsp::HasSupportedNumberOfArguments(callExprs.at(1)));
    initializer.DestroyContext(ctx);
}
TEST_F(LspSuggestionTests, hasSupportedNumberOfArguments2)
{
    const char *source =
        "function fetchData():Promise<string>{\n"
        "\nreturn Promise.reject(\"Error\");\n}\nfetchData().catch();";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("token-pos-literal.ets", ES2PANDA_STATE_CHECKED, source);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    std::vector<ark::es2panda::ir::AstNode *> callExprs;
    ast->FindChild([&callExprs](ark::es2panda::ir::AstNode *node) {
        if (node->Type() == ark::es2panda::ir::AstNodeType::CALL_EXPRESSION) {
            callExprs.push_back(node);
        }

        return false;
    });
    EXPECT_TRUE(ark::es2panda::lsp::HasSupportedNumberOfArguments(callExprs.at(1)));
    initializer.DestroyContext(ctx);
}
TEST_F(LspSuggestionTests, hasSupportedNumberOfArguments3)
{
    const char *source =
        "function fetchData():Promise<string>{\nreturn Promise.resolve(\"Success\");\n}"
        "\nfetchData().finally();";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("token-pos-literal.ets", ES2PANDA_STATE_CHECKED, source);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    std::vector<ark::es2panda::ir::AstNode *> callExprs;
    ast->FindChild([&callExprs](ark::es2panda::ir::AstNode *node) {
        if (node->Type() == ark::es2panda::ir::AstNodeType::CALL_EXPRESSION) {
            callExprs.push_back(node);
        }
        return false;
    });
    EXPECT_TRUE(ark::es2panda::lsp::HasSupportedNumberOfArguments(callExprs.at(1)));
    initializer.DestroyContext(ctx);
}
TEST_F(LspSuggestionTests, hasSupportedNumberOfArguments4)
{
    const char *source =
        "function fetchData(name:string):Promise<string>{\n"
        "\nreturn Promise.resolve(\"Success\");\n}\nfetchData(\"test\").finally();";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("token-pos-literal.ets", ES2PANDA_STATE_CHECKED, source);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    std::vector<ark::es2panda::ir::AstNode *> callExprs;
    ast->FindChild([&callExprs](ark::es2panda::ir::AstNode *node) {
        if (node->Type() == ark::es2panda::ir::AstNodeType::CALL_EXPRESSION) {
            callExprs.push_back(node);
        }
        return false;
    });
    EXPECT_TRUE(ark::es2panda::lsp::HasSupportedNumberOfArguments(callExprs.at(1)));
    initializer.DestroyContext(ctx);
}

TEST_F(LspSuggestionTests, GetSuggestionDiagnostics)
{
    const char *source =
        "function fetchData(){\nreturn Promise.resolve(\"h\")\n;}\nfunction processData()"
        "{\nreturn fetchData().finally();\n};\nprocessData();\n";
    Initializer initializer = Initializer();
    es2panda_Context *context = initializer.CreateContext("sug-diag.ets", ES2PANDA_STATE_CHECKED, source);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(context);
    auto diag = ark::es2panda::lsp::GetSuggestionDiagnosticsImpl(ast, context);
    const auto message = "This_may_be_converted_to_an_async_function";
    auto severity = DiagnosticSeverity::Hint;
    int const startLine = 1;
    int const endLine = 3;
    int const startChar = 1;
    int const endChar = 3;
    ASSERT_EQ(diag.at(0).diagnostic.range_.start.line_, startLine);
    ASSERT_EQ(diag.at(0).diagnostic.range_.end.line_, endLine);
    ASSERT_EQ(diag.at(0).diagnostic.range_.start.character_, startChar);
    ASSERT_EQ(diag.at(0).diagnostic.range_.end.character_, endChar);
    ASSERT_EQ(diag.at(0).diagnostic.message_, message);
    ASSERT_EQ(diag.at(0).diagnostic.severity_, severity);
    initializer.DestroyContext(context);
}

TEST_F(LspSuggestionTests, GetSuggestionDiagnostics2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("ds1.ets", ES2PANDA_STATE_CHECKED,
                                  "function fetchData(){\nreturn Promise.resolve(\"h\")\n;}\nfunction processData()"
                                  "{\nreturn fetchData().finally();\n};\nprocessData();\n");
    LSPAPI const *lspApi = GetImpl();
    auto diag = lspApi->getSuggestionDiagnostics(ctx);
    initializer.DestroyContext(ctx);
    const auto msg = "This_may_be_converted_to_an_async_function";
    auto severity = DiagnosticSeverity::Hint;
    int const startLine = 1;
    int const endLine = 3;
    int const startChar = 1;
    int const endChar = 3;
    ASSERT_EQ(diag.diagnostic.at(0).range_.start.line_, startLine);
    ASSERT_EQ(diag.diagnostic.at(0).range_.end.line_, endLine);
    ASSERT_EQ(diag.diagnostic.at(0).range_.start.character_, startChar);
    ASSERT_EQ(diag.diagnostic.at(0).range_.end.character_, endChar);
    ASSERT_EQ(diag.diagnostic.at(0).message_, msg);
    ASSERT_EQ(diag.diagnostic.at(0).severity_, severity);
}

TEST_F(LspSuggestionTests, GetSuggestionDiagnosticsFixCrash1)
{
    Initializer initializer = Initializer();
    std::vector<std::string> files = {"GetSuggestionDiagnosticsFixCrash1.ets"};
    std::vector<std::string> texts = {R"delimiter(
build() {
    Column() {
        ForEach(
            (item: image.PixelMap, index?: number) => {
                ListItem() {
                    Column() {
                        Text('image' + index)
                    }
                    .margin(15)
                }
                .width('100%')
            }
        )
    }
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    LSPAPI const *lspApi = GetImpl();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto diag = lspApi->getSuggestionDiagnostics(ctx);
    initializer.DestroyContext(ctx);
    int const expectSize = 0;
    ASSERT_EQ(diag.diagnostic.size(), expectSize);
}

TEST_F(LspSuggestionTests, isPromiseHandler)
{
    const char *source =
        "function fetchData(name:string):Promise<string>{\n"
        "\nreturn Promise.resolve(\"Success\");\n}\nfetchData(\"test\").finally();";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("sug-diag.ets", ES2PANDA_STATE_CHECKED, source);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    std::vector<ark::es2panda::ir::AstNode *> callExprs;
    ast->FindChild([&callExprs](ark::es2panda::ir::AstNode *node) {
        if (node->Type() == ark::es2panda::ir::AstNodeType::CALL_EXPRESSION) {
            callExprs.push_back(node);
        }
        return false;
    });
    EXPECT_TRUE(ark::es2panda::lsp::IsPromiseHandler(callExprs.at(1)));
    initializer.DestroyContext(ctx);
}

TEST_F(LspSuggestionTests, isPromiseHandler2)
{
    const char *source =
        "function fetchData():Promise<string>{\n"
        "\nreturn Promise.reject(\"Error\");\n}\nfetchData().catch();";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("sug-diag.ets", ES2PANDA_STATE_CHECKED, source);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    std::vector<ark::es2panda::ir::AstNode *> callExprs;
    ast->FindChild([&callExprs](ark::es2panda::ir::AstNode *node) {
        if (node->Type() == ark::es2panda::ir::AstNodeType::CALL_EXPRESSION) {
            callExprs.push_back(node);
        }
        return false;
    });
    EXPECT_TRUE(ark::es2panda::lsp::IsPromiseHandler(callExprs.at(1)));
    initializer.DestroyContext(ctx);
}

TEST_F(LspSuggestionTests, isPromiseHandler3)
{
    const char *source =
        "function fetchData():Promise<string>{\n"
        "\nreturn Promise.resolve(\"Success\");\n}\nfetchData().then();";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("sug-diag.ets", ES2PANDA_STATE_CHECKED, source);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    std::vector<ark::es2panda::ir::AstNode *> callExprs;
    ast->FindChild([&callExprs](ark::es2panda::ir::AstNode *node) {
        if (node->Type() == ark::es2panda::ir::AstNodeType::CALL_EXPRESSION) {
            callExprs.push_back(node);
        }
        return false;
    });
    EXPECT_TRUE(ark::es2panda::lsp::IsPromiseHandler(callExprs.at(1)));
    initializer.DestroyContext(ctx);
}

TEST_F(LspSuggestionTests, isPromiseHandler4)
{
    const char *source =
        "function fetchData(name: string,name2:string):Promise<string>{\n"
        "\nreturn Promise.resolve(\"Success\");\n}\nfetchData(\"test\",\"test2\").finally();";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("sug-diag.ets", ES2PANDA_STATE_CHECKED, source);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    std::vector<ark::es2panda::ir::AstNode *> callExprs;
    ast->FindChild([&callExprs](ark::es2panda::ir::AstNode *node) {
        if (node->Type() == ark::es2panda::ir::AstNodeType::CALL_EXPRESSION) {
            callExprs.push_back(node);
        }
        return false;
    });
    EXPECT_TRUE(ark::es2panda::lsp::IsPromiseHandler(callExprs.at(1)));
    initializer.DestroyContext(ctx);
}
}  // namespace