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

namespace {

using ark::es2panda::compiler::ast_verifier::ModifierAccessValid;

TEST_F(ASTVerifierTest, ProtectedAccessTestNegative1)
{
    char const *text = R"(
        class Base {
            public a: int = 1;
        }
        function main(): void {
            let base: Base = new Base();
            let a = base.a;
        }
    )";
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
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

        EXPECT_TRUE(Verify<ModifierAccessValid>(ExpectVerifierMessage {"PROPERTY_NOT_VISIBLE_HERE"}));
    }
}

TEST_F(ASTVerifierTest, ProtectedAccessTestNegative2)
{
    char const *text = R"(
        class Base {
            public a: int = 1;
        }
        class Derived extends Base {}
        function main(): void {
            let derived: Derived = new Derived();
            let a = derived.a;
        }
    )";
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
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

        EXPECT_TRUE(Verify<ModifierAccessValid>(ExpectVerifierMessage {"PROPERTY_NOT_VISIBLE_HERE"}));
    }
}

TEST_F(ASTVerifierTest, ProtectedAccessTestNegative3)
{
    char const *text = R"(
        class Base {
            public a: int = 1;
        }
        class Derived extends Base {}
        function main(): void {
            let derived: Base = new Derived();
            let a = derived.a;
        }
    )";
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
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

        EXPECT_TRUE(Verify<ModifierAccessValid>(ExpectVerifierMessage {"PROPERTY_NOT_VISIBLE_HERE"}));
    }
}

TEST_F(ASTVerifierTest, ProtectedAccessTestNegative4)
{
    char const *text = R"(
        class Base {
            public a: int = 1;
            public protectedMethod() {
                this.a = 2;
            }
        }
        function main(): void {
            let base: Base = new Base();
            base.protectedMethod();
        }
    )";
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        GetAst()
            ->AsETSModule()
            ->Statements()[0]
            ->AsClassDeclaration()
            ->Definition()
            ->AsClassDefinition()
            ->Body()[0]
            ->AsClassElement()
            ->Value()
            ->AsFunctionExpression()
            ->Function()
            ->AsScriptFunction()
            ->Body()
            ->AsBlockStatement()
            ->Statements()[1]
            ->AsExpressionStatement()
            ->GetExpression()
            ->AsCallExpression()
            ->Signature()
            ->AddSignatureFlag(ark::es2panda::checker::SignatureFlags::PROTECTED);

        EXPECT_TRUE(Verify<ModifierAccessValid>(ExpectVerifierMessage {"PROPERTY_NOT_VISIBLE_HERE"}));
    }
}

TEST_F(ASTVerifierTest, ProtectedAccessTestNegative5)
{
    char const *text = R"(
        class Base {
            public a: int = 1;
            public protectedMethod() {
                this.a = 2;
            }
        }
        class Derived extends Base {}
        function main(): void {
            let derived: Derived = new Derived();
            derived.protectedMethod();
        }
    )";
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        GetAst()
            ->AsETSModule()
            ->Statements()[0]
            ->AsClassDeclaration()
            ->Definition()
            ->AsClassDefinition()
            ->Body()[0]
            ->AsClassElement()
            ->Value()
            ->AsFunctionExpression()
            ->Function()
            ->AsScriptFunction()
            ->Body()
            ->AsBlockStatement()
            ->Statements()[1]
            ->AsExpressionStatement()
            ->GetExpression()
            ->AsCallExpression()
            ->Signature()
            ->AddSignatureFlag(ark::es2panda::checker::SignatureFlags::PROTECTED);

        EXPECT_TRUE(Verify<ModifierAccessValid>(ExpectVerifierMessage {"PROPERTY_NOT_VISIBLE_HERE"}));
    }
}

TEST_F(ASTVerifierTest, ProtectedAccessTestNegative6)
{
    char const *text = R"(
        class Base {
            public a: int = 1;
            public protectedMethod() {
                this.a = 2;
            }
        }
        class Derived extends Base {}
        function main(): void {
            let derived: Base = new Derived();
            derived.protectedMethod();
        }
    )";
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        auto *x = GetAst();
        x->AsETSModule()
            ->AsETSModule()
            ->Statements()[0]
            ->AsClassDeclaration()
            ->Definition()
            ->AsClassDefinition()
            ->Body()[0]
            ->AsClassElement()
            ->Value()
            ->AsFunctionExpression()
            ->Function()
            ->AsScriptFunction()
            ->Body()
            ->AsBlockStatement()
            ->Statements()[1]
            ->AsExpressionStatement()
            ->GetExpression()
            ->AsCallExpression()
            ->Signature()
            ->AddSignatureFlag(ark::es2panda::checker::SignatureFlags::PROTECTED);

        EXPECT_TRUE(Verify<ModifierAccessValid>(ExpectVerifierMessage {"PROPERTY_NOT_VISIBLE_HERE"}));
    }
}

}  // namespace
