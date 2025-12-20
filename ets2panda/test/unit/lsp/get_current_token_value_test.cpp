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

#include <gtest/gtest.h>
#include <cstddef>

#include "ir/astNode.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::Initializer;

TEST_F(LSPAPITests, GetCurrentTokenValue)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("current_token.ets", ES2PANDA_STATE_CHECKED, "ab");
    LSPAPI const *lspApi = GetImpl();
    size_t offset = 2;
    std::string result = lspApi->getCurrentTokenValue(ctx, offset);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(result, "ab");
}

TEST_F(LSPAPITests, GetCurrentTokenValue1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("file1.ets", ES2PANDA_STATE_CHECKED, "\"ab\"");
    size_t offset = 3;
    std::string result = ark::es2panda::lsp::GetCurrentTokenValueImpl(ctx, offset);
    std::string expect = "ab";
    ASSERT_EQ(result, expect);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPAPITests, GetCurrentTokenValue2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("file1.ets", ES2PANDA_STATE_CHECKED, "\'ab\'");
    size_t offset = 3;
    std::string result = ark::es2panda::lsp::GetCurrentTokenValueImpl(ctx, offset);
    std::string expect = "ab";
    ASSERT_EQ(result, expect);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPAPITests, GetCurrentTokenValue3)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("file1.ets", ES2PANDA_STATE_CHECKED, "abc");
    size_t offset = 2;
    std::string result = ark::es2panda::lsp::GetCurrentTokenValueImpl(ctx, offset);
    std::string expect = "ab";
    ASSERT_EQ(result, expect);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPAPITests, GetCurrentTokenValue4)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("current_token.ets", ES2PANDA_STATE_CHECKED, "//中文测试\nab");
    LSPAPI const *lspApi = GetImpl();
    size_t offset = 9;
    std::string result = lspApi->getCurrentTokenValue(ctx, offset);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(result, "ab");
}

TEST_F(LSPAPITests, GetTokenPosOfNode1)
{
    using ark::es2panda::ir::AstNode;

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("token-pos-identifier.ets", ES2PANDA_STATE_CHECKED,
                                                      "function A(a:number, b:number) {\n  return a + b;\n}\nA(1, 2);");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    auto targetNode = ast->FindChild([](AstNode *node) {
        return node->IsIdentifier() && node->AsIdentifier()->Name() == "A" && node->Parent()->IsCallExpression();
    });

    ASSERT_NE(targetNode, nullptr);
    auto result = ark::es2panda::lsp::GetTokenPosOfNode(targetNode);
    size_t const pos = 51;
    ASSERT_EQ(result, pos);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPAPITests, GetTokenPosOfNode2)
{
    using ark::es2panda::ir::AstNode;

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("token-pos-expression.ets", ES2PANDA_STATE_CHECKED,
                                                      "function A(a:number, b:number) {\n  return a + b;\n}\nA(1, 2);");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    auto targetNode = ast->FindChild([](AstNode *node) { return node->IsExpressionStatement(); });

    ASSERT_NE(targetNode, nullptr);
    auto result = ark::es2panda::lsp::GetTokenPosOfNode(targetNode);
    size_t const pos = 51;
    ASSERT_EQ(result, pos);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPAPITests, GetTokenPosOfNode3)
{
    using ark::es2panda::ir::AstNode;

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(
        "token-pos-literal.ets", ES2PANDA_STATE_CHECKED,
        "let number_literal: number = 1234;\nlet string_literal: string = \"hello\";\nconst str_property = "
        "\"foo\";\n");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    auto targetNode = ast->FindChild(
        [](AstNode *node) { return node->IsNumberLiteral() && node->AsNumberLiteral()->Str() == "1234"; });

    ASSERT_NE(targetNode, nullptr);
    auto result = ark::es2panda::lsp::GetTokenPosOfNode(targetNode);
    size_t const pos = 29;
    ASSERT_EQ(result, pos);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPAPITests, GetTokenPosOfNode4)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(
        "token-pos-literal.ets", ES2PANDA_STATE_CHECKED,
        "//中文测试\nlet number_literal: number = 1234;\nlet string_literal: string = \"hello\";\nconst str_property = "
        "\"foo\";\n");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    LSPAPI const *lspApi = GetImpl();
    size_t offset = 40;
    std::string result = lspApi->getCurrentTokenValue(ctx, offset);
    ASSERT_EQ(result, "1234");
    initializer.DestroyContext(ctx);
}

}  // namespace
