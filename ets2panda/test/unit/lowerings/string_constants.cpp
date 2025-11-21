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
#include "compiler/lowering/ets/stringConstantsLowering.h"

namespace ark::es2panda {

static bool CheckConstReorderLeft(const ir::AstNode *node)
{
    if (node->IsBinaryExpression()) {
        auto const binOp = node->AsBinaryExpression();
        return (binOp->OperatorType() == lexer::TokenType::PUNCTUATOR_PLUS && binOp->Left()->IsIdentifier() &&
                binOp->Right()->IsStringLiteral());
    }
    return false;
}

static bool CheckConstReorderRight(const ir::AstNode *node)
{
    if (node->IsBinaryExpression()) {
        auto const binOp = node->AsBinaryExpression();
        return (binOp->OperatorType() == lexer::TokenType::PUNCTUATOR_PLUS && binOp->Left()->IsStringLiteral() &&
                binOp->Right()->IsIdentifier());
    }
    return false;
}

TEST_F(LoweringTest, TestStringConstansConcat)
{
    char const *text = R"(
        function main() {
            let v = "a" + ("bb" + "1") + (("ccc" + "123") + "dddd");
        }
    )";
    char const *expect = "abb1ccc123dddd";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) { return node->IsBinaryExpression(); }));
        ASSERT_TRUE(ast->IsAnyChild([expect](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect);
        }));
    }
}

TEST_F(LoweringTest, TestStringConstansReorder1)
{
    char const *text = R"(
        const constPad = '=';
        function main() {
            let resultStr = "aa";
            resultStr = constPad + constPad + constPad;
        }
    )";
    char const *expect = "===";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) { return node->IsBinaryExpression(); }));
        ASSERT_TRUE(ast->IsAnyChild([expect](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect);
        }));
    }
}

TEST_F(LoweringTest, TestStringConstansReorder2)
{
    char const *text = R"(
        const constPad = '=';
        function main() {
            let resultStr = "aa";
            resultStr = (constPad + constPad + constPad);
        }
    )";
    char const *expect = "===";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) { return node->IsBinaryExpression(); }));
        ASSERT_TRUE(ast->IsAnyChild([expect](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect);
        }));
    }
}

TEST_F(LoweringTest, TestStringConstansReorder3)
{
    char const *text = R"(
        const constPad1 = '=';
        const constPad2 = '%';
        const constPad3 = '&';
        function main() {
            let resultStr = "aa";
            resultStr = (constPad1 + "%" + constPad2) + "=" + constPad3;
        }
    )";
    char const *expect = "=%%=&";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) { return node->IsBinaryExpression(); }));
        ASSERT_TRUE(ast->IsAnyChild([expect](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect);
        }));
    }
}

TEST_F(LoweringTest, TestStringConstansReorder4)
{
    char const *text = R"(
        const constPad1 = '=';
        const constPad2 = '%';
        const constPad3 = '&';

        function main() {
            let resultStr = "aa";
            resultStr = resultStr + constPad1 + constPad2 + constPad3;
        }
    )";
    char const *expect = "=%&";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_TRUE(ast->IsAnyChild([](ir::AstNode *const node) { return CheckConstReorderLeft(node); }));
        ASSERT_TRUE(ast->IsAnyChild([expect](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect);
        }));
    }
}

TEST_F(LoweringTest, TestStringConstansReorder5)
{
    char const *text = R"(
        function main() {
            let resultStr = "aa";
            resultStr = "^" + "%" + "=" + resultStr;
        }
    )";
    char const *expect = "^%=";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_TRUE(ast->IsAnyChild([](ir::AstNode *const node) { return CheckConstReorderRight(node); }));
        ASSERT_TRUE(ast->IsAnyChild([expect](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect);
        }));
    }
}

TEST_F(LoweringTest, TestStringConstansReorder6)
{
    char const *text = R"(
        const constPad1 = '=';
        const constPad2 = '%';
        const constPad3 = '&';
        function main() {
            let resultStr = "aa";
            resultStr = resultStr + constPad1 + "%" + constPad2 + "=" + constPad3 + resultStr;
        }
    )";
    char const *expect = "=%%=&";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_TRUE(ast->IsAnyChild([](ir::AstNode *const node) { return CheckConstReorderLeft(node); }));
        ASSERT_TRUE(ast->IsAnyChild([expect](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect);
        }));
    }
}

TEST_F(LoweringTest, TestStringConstansReorder7)
{
    char const *text = R"(
        const constPad = '=';
        function main() {
            let resultStr = "aa";
            let identifierStr = "bb";
            resultStr = (identifierStr + constPad) + (constPad + identifierStr);
        }
    )";
    char const *expect = "==";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) { return CheckConstReorderLeft(node); }));
        ASSERT_TRUE(ast->IsAnyChild([](ir::AstNode *const node) { return CheckConstReorderRight(node); }));
        ASSERT_TRUE(ast->IsAnyChild([expect](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect);
        }));
    }
}

TEST_F(LoweringTest, TestStringConstansReorder8)
{
    char const *text = R"(
        function getString(str: string) {
            return "the " + str + " string";
        }

        function main(): void {
            const con = " constant";
            let str = "";
            str = str + getString("empty") + con;
        }
    )";
    char const *expect = " constant";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_FALSE(ast->IsAnyChild([](ir::AstNode *const node) { return CheckConstReorderLeft(node); }));
        ASSERT_TRUE(ast->IsAnyChild([expect](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect);
        }));
    }
}

TEST_F(LoweringTest, TestStringConstansReorder9)
{
    char const *text = R"(
        const constPad1 = '=';
        const constPad2 = '%';
        const constPad3 = '&';

        function main() {
            let str = "aa";
            str = (constPad1 + constPad2 + str + constPad3 + constPad1) + (constPad3 + str);
        }
    )";
    char const *expect1 = "=%";
    char const *expect2 = "&=&";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_TRUE(ast->IsAnyChild([](ir::AstNode *const node) { return CheckConstReorderRight(node); }));
        ASSERT_TRUE(ast->IsAnyChild([expect1](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect1);
        }));
        ASSERT_TRUE(ast->IsAnyChild([expect2](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect2);
        }));
    }
}

TEST_F(LoweringTest, TestStringConstansReorder10)
{
    char const *text = R"(
        const constPad1 = '=';
        const constPad2 = '%';
        const constPad3 = '&';

        function main() {
            let str = "aa";
            str = (str + (str + constPad1) + (constPad2 + str) + constPad1 + constPad3) + str;
        }
    )";
    char const *expect1 = "=%";
    char const *expect2 = "=&";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_TRUE(ast->IsAnyChild([](ir::AstNode *const node) { return CheckConstReorderRight(node); }));
        ASSERT_TRUE(ast->IsAnyChild([expect1](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect1);
        }));
        ASSERT_TRUE(ast->IsAnyChild([expect2](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect2);
        }));
    }
}

TEST_F(LoweringTest, TestStringConstansReorder11)
{
    char const *text = R"(
        const constPad1 = '=';
        const constPad2 = '%';
        const constPad3 = '&';

        function main() {
            let str = "aa";
            let strNew = "bb";
            str = (str + (constPad1 + strNew)) + (constPad3 + constPad2 + constPad1) + strNew;
        }
    )";
    char const *expect1 = "=";
    char const *expect2 = "&%=";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        const auto *const ast = GetAst();

        ASSERT_TRUE(ast->IsAnyChild([](ir::AstNode *const node) { return CheckConstReorderRight(node); }));
        ASSERT_TRUE(ast->IsAnyChild([expect1](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect1);
        }));
        ASSERT_TRUE(ast->IsAnyChild([expect2](ir::AstNode *const node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->Str().Is(expect2);
        }));
    }
}
}  // namespace ark::es2panda
