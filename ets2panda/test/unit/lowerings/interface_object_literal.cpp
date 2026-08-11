/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include "lowering_test.h"
#include "compiler/lowering/ets/topLevelStmts/topLevelStmts.h"

namespace ark::es2panda {

TEST_F(LoweringTest, TestInterfaceObjectLiteral)
{
    char const *text = R"(
        interface I1 {
            a: number
        }

        interface I2 {
            a: string
        }

        function foo1(a: I1) { }
        function foo2(a: I2) { }

        function main() {
            foo1({ a: 1 })
            foo2({ a: "2" })
        })";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        const auto *const ast = GetAst();
        [[maybe_unused]] auto *classDef1 = ast->FindChild([](ir::AstNode *child) {
            return child->IsClassDefinition() &&
                   (child->AsClassDefinition()->InternalName().Mutf8() == "dummy.dummy$I1$ObjectLiteral");
        });
        ASSERT_TRUE(classDef1 != nullptr);
        [[maybe_unused]] auto *classDef2 = ast->FindChild([](ir::AstNode *child) {
            return child->IsClassDefinition() &&
                   (child->AsClassDefinition()->InternalName().Mutf8() == "dummy.dummy$I2$ObjectLiteral");
        });
        ASSERT_TRUE(classDef1 != nullptr);
    }
}

}  // namespace ark::es2panda
