/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#include "ir/astNode.h"

#include <gtest/gtest.h>

using ark::es2panda::compiler::ast_verifier::VariableHasScope;

namespace {
TEST_F(ASTVerifierTest, ParametersInAsyncFunction)
{
    char const *text = R"(
        async function bar(flag: boolean): Promise<double> {
            if (flag) {
                return 5.0;
            } else {
                return 1.0;
            }
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<VariableHasScope>());
    }
}

TEST_F(ASTVerifierTest, TestUnions)
{
    char const *text = R"(
        function assert_ccexc(f: () => void) {
            try {
                f();
                arktest.assertTrue(false, "exception expected");
            } catch (e) {
                arktest.assertTrue(e instanceof ClassCastError);
            }
        }

        class A { }

        function test_unions() {
            assert_ccexc(() => { let f = ((x: A | undefined) => x as A | null); f(undefined); });
        }

        function main() {
            test_unions();
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<VariableHasScope>());
    }
}

TEST_F(ASTVerifierTest, LambdasHaveCorrectScope)
{
    char const *text = R"(
        type BenchmarkFunc = () => void;

        function main() {
            const arr: number[] = [1, 2, 3, 4];
            const ITERATE_FUNC: BenchmarkFunc = () => {
                const length = arr.length;
            };
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<VariableHasScope>());
    }
}

TEST_F(ASTVerifierTest, AsyncLambda1)
{
    char const *text = R"(
        let fs: ((p: int) => int)[] = [];
        async function foo(i: int): Promise<((p: int) => int)> {
            return fs[i]
        }

        function main() {
            fs = [
                (p: int): int => p + 1,
            ]

            let ps: Object = new Object()
            ps = foo(0)

            let cnt = 0
            waitForCompletion(async () => { cnt += (await (ps as Promise<(p: int) => int>))(0) })
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<VariableHasScope>());
    }
}

TEST_F(ASTVerifierTest, AsyncLambda2)
{
    char const *text = R"(
        let global: int;
        async function func(param: int): Promise<String | null> {
            let local: int;
            let async_lambda: () => Promise<Object | null> = async (): Promise<Object | null> => {
                param;
                local;
                global;
                let x = 0;
                return null;
            }

            return null;
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<VariableHasScope>());
    }
}

}  // namespace
