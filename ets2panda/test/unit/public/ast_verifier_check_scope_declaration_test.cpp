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

using ark::es2panda::compiler::ast_verifier::CheckScopeDeclaration;

namespace {

TEST_F(ASTVerifierTest, FunctionScope)
{
    char const *text = R"(
        function test(a: int) {
            console.log(a)
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<CheckScopeDeclaration>());
    }
}

TEST_F(ASTVerifierTest, ForUpdateLoopScope)
{
    char const *text = R"(
        function main() {
            for (let i = 0; i < 10; i++) {}
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<CheckScopeDeclaration>());
    }
}

TEST_F(ASTVerifierTest, ForInLoopScope)
{
    char const *text = R"(
        function main(): void {
            let res = 0
            let arr: int[] = [1, 2, 3]
            for (let i of arr) {
                res += i
            }
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<CheckScopeDeclaration>());
    }
}

TEST_F(ASTVerifierTest, PartialClass)
{
    char const *text = R"(
        export class A {
            private static readonly param: int = 0;
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<CheckScopeDeclaration>());
    }
}

TEST_F(ASTVerifierTest, TryCatch)
{
    char const *text = R"(
        function main(): void {
            let catchCode = 0;

            try {
                throw new NullPointerError();
            } catch (e) {
             if (e instanceof NullPointerError) {
                catchCode = 1;
             } else {
                throw e;
             }
            }
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<CheckScopeDeclaration>());
    }
}

}  // namespace
