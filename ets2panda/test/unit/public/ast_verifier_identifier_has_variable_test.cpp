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

#include "ast_verifier_test.h"
#include "checker/ETSchecker.h"
#include "ir/expressions/identifier.h"
#include "ir/astNode.h"
#include "parser/ETSparser.h"

#include <gtest/gtest.h>

using ark::es2panda::compiler::ast_verifier::IdentifierHasVariable;

namespace {
TEST_F(ASTVerifierTest, LabelsHaveReferences)
{
    char const *text = R"(
        function main(): void {
        loop: for (let i = 0; i < 10; i++) {
        break loop;
        }
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<IdentifierHasVariable>());
    }
}

TEST_F(ASTVerifierTest, ExtensionFunction)
{
    char const *text = R"(
        class Fruit {
            name(): void {
            }
        }

        function name(this:Fruit, id: int): void {
        }

        function test() {
        let fruit = new Fruit();
        fruit.name()
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<IdentifierHasVariable>());
    }
}

// 26272
TEST_F(ASTVerifierTest, DISABLED_Imports)
{
    char const *text = R"(
        import { PI } from "std/math";
        import { A } from "dynamic_import_tests"
        import default_imported from "import_tests/modules/default_export";
        import * as Time from "std/time";
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<IdentifierHasVariable>());
    }
}

TEST_F(ASTVerifierTest, OptionalLambdas)
{
    char const *text = R"(
        function main(): void {
            let d = (c?: int) => {
            }
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<IdentifierHasVariable>());
    }
}

TEST_F(ASTVerifierTest, TSQualifiedName)
{
    char const *text = R"(
        import * as Time from "std/time";

        function main() {
        let value = new Time.Chrono();
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<IdentifierHasVariable>());
    }
}

TEST_F(ASTVerifierTest, ParametersInArrowFunctionExpression)
{
    char const *text = R"(
        let b = 1;
        let f = (p: double) => b + p;
        function main () {
            arktest.assertEQ(f(42), 43)
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<IdentifierHasVariable>());
    }
}

TEST_F(ASTVerifierTest, Lambdas)
{
    char const *text = R"(
        type asyncLambda = () => Promise<void>;

        async function asyncFunc(): Promise<boolean> {
            return true;
        }

        function callAsyncLambda(): void {
            let is_call_async_lambda: boolean = false;

            let async_lambda: asyncLambda = async (): Promise<void> => {
                await asyncFunc();
                is_call_async_lambda = true;
            };
        }

        type I2v = (i: int) => void;
        type T1 = (lambda: (arg: int) => int, arg: int) => int;
        type T2 = (c: int) => int;

        const F1: I2v = (counter: int) => {
            let funcWithLambda: T1 = (lambda: (arg: int) => int, arg: int) => {
                return lambda(arg);
            };

            let it: T2 = (c: int): int => {
                return c;
            };

            while (counter > 0) counter = funcWithLambda(it, counter);
        };

        function main() {
            F1(44);
            return 0;
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<IdentifierHasVariable>());
    }
}

TEST_F(ASTVerifierTest, PromiseUndefined)
{
    char const *text = R"(
        async function testAsyncVoidNothing() {}
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<IdentifierHasVariable>());
    }
}

TEST_F(ASTVerifierTest, EnumInts)
{
    char const *text = R"(
        enum Color {
            Red = 1,
            Blue = 2
        }

        function main() {
            let color: Color = Color.Red;
            let name = color.getName();
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<IdentifierHasVariable>());
    }
}

TEST_F(ASTVerifierTest, EnumStrings)
{
    char const *text = R"(
        enum Shape {
            Circle = "CIRCLE",
            Square = "SQUARE",
            Triangle = "TRIANGLE"
        }

        function main() {
            let shape = Shape.Circle.valueOf();
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<IdentifierHasVariable>());
    }
}
}  // namespace
