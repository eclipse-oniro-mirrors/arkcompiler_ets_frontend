/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "lowering_test.h"
#include "compiler/lowering/ets/constantExpressionLowering.h"

namespace ark::es2panda {

template <typename T, T EXPECT_VAL>
static bool CheckNumberLiteral(const ir::AstNode *node)
{
    if (node->IsNumberLiteral()) {
        auto numNode = node->AsNumberLiteral()->Number();
        if (numNode.CanGetValue<T>()) {
            return numNode.GetValue<T>() == EXPECT_VAL;
        }
    }
    return false;
}

TEST_F(LoweringTest, TestConstantExpressionConcatExtendedBoolean1)
{
    char const *text = R"(
        @interface MyAnno {
            c : int
            d : int
        }
        @MyAnno({c = "a" ? 5 : 4, d = 12 ? 7 : 6})
        function main() {}
    )";

    int const expectC = 5;
    int const expectD = 7;
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();
        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsBinaryExpression() || node->IsUnaryExpression() || node->IsConditionalExpression();
        }));

        ASSERT_TRUE(
            ast->IsAnyChild([](ir::AstNode *const node) { return CheckNumberLiteral<int32_t, expectC>(node); }));

        ASSERT_TRUE(
            ast->IsAnyChild([](ir::AstNode *const node) { return CheckNumberLiteral<int32_t, expectD>(node); }));
    }
}

TEST_F(LoweringTest, TestConstantExpressionConcatExtendedBoolean2)
{
    char const *text = R"(
        @interface MyAnno {
            c : int
            d : int
        }
        @MyAnno({c = "" ? 5 : 4, d = 0 ? 7 : 6})
        function main() {}
    )";

    int const expectC = 4;
    int const expectD = 6;
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();
        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsBinaryExpression() || node->IsUnaryExpression() || node->IsConditionalExpression();
        }));

        ASSERT_TRUE(
            ast->IsAnyChild([](ir::AstNode *const node) { return CheckNumberLiteral<int32_t, expectC>(node); }));

        ASSERT_TRUE(
            ast->IsAnyChild([](ir::AstNode *const node) { return CheckNumberLiteral<int32_t, expectD>(node); }));
    }
}

TEST_F(LoweringTest, TestConstantExpressionConcatExtendedBoolean4)
{
    char const *text = R"(
        @interface MyAnno {
            a : boolean
            b : int = "aaa" ? 1 : 2
        }
        @MyAnno({a = ((("aa" >= "aa") && ("ab" < "aa")) || ("aa" != "bb")) && "aa!" == "aa!"})
        function main() {}
    )";

    bool const expectA = true;
    int const expectB = 1;
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsBinaryExpression() || node->IsUnaryExpression() || node->IsConditionalExpression();
        }));

        ASSERT_TRUE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsBooleanLiteral() && node->AsBooleanLiteral()->Value() == expectA;
        }));

        ASSERT_TRUE(
            ast->IsAnyChild([](ir::AstNode *const node) { return CheckNumberLiteral<int32_t, expectB>(node); }));
    }
}

TEST_F(LoweringTest, TestConstantExpressionConcatExtendedBoolean3)
{
    char const *text = R"(
        @interface MyAnno {
            a : int
            b : int
        }
        @MyAnno({a = !0 ? 10 : 20, b= "test" ? 1 : 2})
        function main() {}
    )";

    int const expectA = 10;
    int const expectB = 1;
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsBinaryExpression() || node->IsUnaryExpression() || node->IsConditionalExpression();
        }));

        ASSERT_TRUE(
            ast->IsAnyChild([](ir::AstNode *const node) { return CheckNumberLiteral<int32_t, expectA>(node); }));

        ASSERT_TRUE(
            ast->IsAnyChild([](ir::AstNode *const node) { return CheckNumberLiteral<int32_t, expectB>(node); }));
    }
}
TEST_F(LoweringTest, TestConstantExpressionConcat)
{
    char const *text = R"(
        @interface MyAnno {
            a : int
            b : long
            c : boolean
            d : int
        }
        @MyAnno({a = 1 + -1, b = 1554.4 ^ 10, c = 12 >= 10, d = 15 != 10 ? 1+4 : 5+1})
        function main() {}
    )";
    int const expectA = 0;
    int64_t const expectB = 1560;
    bool const expectC = true;
    int const expectD = 5;

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsBinaryExpression() || node->IsUnaryExpression() || node->IsConditionalExpression();
        }));

        ASSERT_TRUE(
            ast->IsAnyChild([](ir::AstNode *const node) { return CheckNumberLiteral<int32_t, expectA>(node); }));

        ASSERT_TRUE(
            ast->IsAnyChild([](ir::AstNode *const node) { return CheckNumberLiteral<int64_t, expectB>(node); }));

        ASSERT_TRUE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsBooleanLiteral() && node->AsBooleanLiteral()->Value() == expectC;
        }));

        ASSERT_TRUE(
            ast->IsAnyChild([](ir::AstNode *const node) { return CheckNumberLiteral<int32_t, expectD>(node); }));
    }
}

TEST_F(LoweringTest, TestConstantExpressionConcatNumeric1)
{
    char const *text = R"(
        @interface MyAnno {
            a : int = 1+1
        }
        @MyAnno()
        function main() {}
    )";

    int32_t const expectA = 2;
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsBinaryExpression() || node->IsUnaryExpression() || node->IsConditionalExpression();
        }));

        ASSERT_TRUE(
            ast->IsAnyChild([](ir::AstNode *const node) { return CheckNumberLiteral<int32_t, expectA>(node); }));
    }
}

TEST_F(LoweringTest, TestConstantExpressionConcatBoolean)
{
    char const *text = R"(
        @interface MyAnno {
            a : boolean
        }
        @MyAnno({a = ((20<=10) || true) ? !false && (10==10) && (30 != 10): false})
        function main() {}
    )";

    bool const expectA = true;
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsBinaryExpression() || node->IsUnaryExpression() || node->IsConditionalExpression();
        }));

        ASSERT_TRUE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsBooleanLiteral() && node->AsBooleanLiteral()->Value() == expectA;
        }));
    }
}

TEST_F(LoweringTest, TestConstantExpressionConcatNumeric)
{
    char const *text = R"(
        @interface MyAnno {
            a : long
        }
        @MyAnno({a = ((((1 + -1 + 10) * 123 / 5) ^ (~10.2) << 1) >> 2.6 >>> 33 & 141 | 12) % 53})
        function main() {}
    )";

    int64_t const expectA = 35;
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();
        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) {
            return node->IsBinaryExpression() || node->IsUnaryExpression() || node->IsConditionalExpression();
        }));

        ASSERT_TRUE(
            ast->IsAnyChild([](ir::AstNode *const node) { return CheckNumberLiteral<int64_t, expectA>(node); }));
    }
}
}  // namespace ark::es2panda
