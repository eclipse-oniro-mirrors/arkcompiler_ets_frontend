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
#include "ir/astNode.h"

#include <gtest/gtest.h>

using ark::es2panda::compiler::ast_verifier::VariableHasEnclosingScope;

namespace {
TEST_F(ASTVerifierTest, CatchClause)
{
    char const *text = R"(
        let a = 10;
        try {
            a / 0;
        } catch (e) {
            if (e instanceof Error) {
            }
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<VariableHasEnclosingScope>());
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
        EXPECT_TRUE(Verify<VariableHasEnclosingScope>());
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
        EXPECT_TRUE(Verify<VariableHasEnclosingScope>());
    }
}

TEST_F(ASTVerifierTest, LambdaAsParameter)
{
    char const *text = R"(
        function foo(callback: (resolve: (val: int) => void) => void): void {}

        function main(): void {
            foo((resolve: (val: int) => void): void => {});
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<VariableHasEnclosingScope>());
    }
}

TEST_F(ASTVerifierTest, PartialClassDeclaration)
{
    char const *text = R"(
        export class IncrementalNode {
            protected onChildInserted: ((node: IncrementalNode) => void) | undefined = undefined
            private static readonly ESCAPED_CHARS: char[] = [c'\"', c'\\']
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<VariableHasEnclosingScope>());
    }
}
}  // namespace
