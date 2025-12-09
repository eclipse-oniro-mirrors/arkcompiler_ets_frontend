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

using ark::es2panda::compiler::ast_verifier::ModifierAccessValid;

TEST_F(ASTVerifierTest, ProtectedAccessTestCorrect)
{
    char const *text = R"(
        class A {
            public a: int = 1;
        }
        class B extends A {
            public b: int;
            constructor() {
                this.b = this.a;
            }
        }
    )";
    CONTEXT(ES2PANDA_STATE_CHECKED, text, "dummy.ets")
    {
        GetAst()
            ->AsETSModule()
            ->Statements()[1]
            ->AsClassDeclaration()
            ->Definition()
            ->AsClassDefinition()
            ->Body()[0]
            ->AsClassProperty()
            ->AddModifier(ark::es2panda::ir::ModifierFlags::PROTECTED);

        EXPECT_TRUE(Verify<ModifierAccessValid>());
    }
}
