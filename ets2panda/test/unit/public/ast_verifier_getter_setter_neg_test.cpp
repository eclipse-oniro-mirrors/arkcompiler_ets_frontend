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
#include "ir/astNode.h"
#include "util/diagnosticEngine.h"
#include <gtest/gtest.h>
#include <cstddef>

using ark::es2panda::checker::ETSChecker;
using ark::es2panda::compiler::ast_verifier::GetterSetterValidation;
using ark::es2panda::ir::ETSParameterExpression;
using ark::es2panda::ir::Identifier;
using ark::es2panda::util::DiagnosticEngine;
namespace {
TEST_F(ASTVerifierTest, ValidateGetterReturnTypeAnnotation)
{
    DiagnosticEngine de {};
    ETSChecker checker {Allocator(), de};

    char const *text =
        R"(
        class A {
            private _value: number = 0;
            get value(): number {
                return this._value
            }

            set value(v: number) {
                this._value = v
            }
        }
    )";

    auto cb = [&checker](ark::es2panda::ir::AstNode *child) {
        if (child->IsMethodDefinition()) {
            auto *const method = child->AsMethodDefinition();
            if (method->IsGetter() && method->Value()->IsFunctionExpression()) {
                auto *const function = method->Value()->AsFunctionExpression()->Function();
                ASSERT_NE(function->ReturnTypeAnnotation(), nullptr);
                function->ReturnTypeAnnotation()->SetTsType(checker.GlobalVoidType());
            }
        }
    };

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        // Change annotation return type to void
        GetAst()->IterateRecursively(cb);
        EXPECT_TRUE(Verify<GetterSetterValidation>(
            ExpectVerifierMessage {"GETTER METHOD HAS VOID RETURN TYPE IN RETURN TYPE ANNOTATION"}));
    }
}

TEST_F(ASTVerifierTest, ValidateGetterHasReturnStatement)
{
    // Program with no type annotation for getter
    char const *text =
        R"(
        class A {
            private _value: number = 0;
            get value() {
                return this._value
            }

            set value(v: number) {
                this._value = v
            }
        }
    )";

    auto cb = [](ark::es2panda::ir::AstNode *child) {
        if (child->IsMethodDefinition()) {
            auto *const method = child->AsMethodDefinition();
            if (method->IsGetter() && method->Value()->IsFunctionExpression()) {
                auto *const function = method->Value()->AsFunctionExpression()->Function();
                auto &returns = function->ReturnStatementsForUpdate();
                returns.clear();
            }
        }
    };
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        // Remove return statements from getter
        GetAst()->IterateRecursively(cb);
        EXPECT_TRUE(Verify<GetterSetterValidation>(
            ExpectVerifierMessage {"MISSING RETURN TYPE ANNOTATION AND RETURN STATEMENT IN GETTER METHOD"}));
    }
}

TEST_F(ASTVerifierTest, ValidateGetterVoidReturnStatement)
{
    DiagnosticEngine de {};
    ETSChecker checker {Allocator(), de};

    char const *text =
        R"(
        class A {
            private _value: number = 0;
            get value() {
                return this._value
            }

            set value(v: number) {
                this._value = v
            }
        }
    )";
    auto cb = [&checker](ark::es2panda::ir::AstNode *child) {
        if (child->IsMethodDefinition()) {
            auto *const method = child->AsMethodDefinition();
            if (method->IsGetter() && method->Value()->IsFunctionExpression()) {
                auto *const function = method->Value()->AsFunctionExpression()->Function();
                auto &returns = function->ReturnStatements();
                ASSERT_EQ(returns.size(), 1);
                returns[0]->SetArgument(nullptr);
                returns[0]->SetReturnType(&checker, checker.GlobalVoidType());
            }
        }
    };
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        // Change return statement type to void
        GetAst()->IterateRecursively(cb);
        EXPECT_TRUE(Verify<GetterSetterValidation>(ExpectVerifierMessage {"GETTER METHOD HAS VOID RETURN TYPE"}));
    }
}

TEST_F(ASTVerifierTest, ValidateGetterArguments)
{
    DiagnosticEngine de {};
    ETSChecker checker {Allocator(), de};

    char const *text =
        R"(
        class A {
            private _value: number = 0;
            get value(): number {
                return this._value
            }

            set value(v: number) {
                this._value = v
            }
        }
    )";

    // Create argument
    auto *ident = checker.AllocNode<Identifier>("ident", Allocator());
    auto *param = checker.AllocNode<ETSParameterExpression>(ident, nullptr, Allocator());
    auto cb = [param](ark::es2panda::ir::AstNode *child) {
        if (child->IsMethodDefinition()) {
            auto *const method = child->AsMethodDefinition();
            if (method->IsGetter() && method->Value()->IsFunctionExpression()) {
                auto *const function = method->Value()->AsFunctionExpression()->Function();
                auto &params = function->ParamsForUpdate();
                ASSERT_EQ(params.size(), 0);
                params.push_back(param);
            }
        }
    };
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        // Add argument to getter
        GetAst()->IterateRecursively(cb);
        EXPECT_TRUE(
            Verify<GetterSetterValidation>(ExpectVerifierMessage {"GETTER METHOD HAS INCORRECT NUMBER OF ARGUMENTS"}));
    }
}

TEST_F(ASTVerifierTest, ValidateSetterReturnType)
{
    DiagnosticEngine de {};
    ETSChecker checker {Allocator(), de};

    char const *text =
        R"(
        class A {
            private _value: number = 0;
            set value(v: number) {
                this._value = v
            }

            get value(): number {
                return this._value
            }
        }
    )";
    auto cb = [&checker](ark::es2panda::ir::AstNode *child) {
        if (child->IsMethodDefinition()) {
            auto *const method = child->AsMethodDefinition();
            if (method->IsSetter() && method->Value()->IsFunctionExpression()) {
                auto *const function = method->Value()->AsFunctionExpression()->Function();
                ASSERT_EQ(function->ReturnTypeAnnotation(), nullptr);
                auto *const thisType = function->Signature()->Params()[0]->TsType();
                auto *const thisTypeAnnotation =
                    function->Params()[0]->AsETSParameterExpression()->Ident()->TypeAnnotation();
                function->Signature()->SetReturnType(thisType);
                function->SetReturnTypeAnnotation(thisTypeAnnotation->Clone(checker.Allocator(), function));
            }
        }
    };

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        // Change setter return type
        GetAst()->IterateRecursively(cb);
        EXPECT_TRUE(Verify<GetterSetterValidation>(ExpectVerifierMessage {"SETTER METHOD MUST NOT HAVE RETURN TYPE"}));
    }
}

TEST_F(ASTVerifierTest, ValidateSetterArguments)
{
    DiagnosticEngine de {};
    ETSChecker checker {Allocator(), de};

    char const *text =
        R"(
        class A {
            private _value: number = 0;
            set value(v: number) {
                this._value = v
            }

            get value(): number {
                return this._value
            }
        }
    )";
    auto cb = [](ark::es2panda::ir::AstNode *child) {
        if (child->IsMethodDefinition()) {
            auto *const method = child->AsMethodDefinition();
            if (method->IsSetter() && method->Value()->IsFunctionExpression()) {
                auto *const function = method->Value()->AsFunctionExpression()->Function();
                auto &params = function->ParamsForUpdate();
                ASSERT_EQ(params.size(), 1);
                params.clear();
            }
        }
    };
    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        // Change setter arguments
        GetAst()->IterateRecursively(cb);
        EXPECT_TRUE(
            Verify<GetterSetterValidation>(ExpectVerifierMessage {"SETTER METHOD HAS INCORRECT NUMBER OF ARGUMENTS"}));
    }
}

}  // anonymous namespace
