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

using ark::es2panda::checker::ETSChecker;
using ark::es2panda::compiler::ast_verifier::GetterSetterValidation;
using ark::es2panda::util::DiagnosticEngine;

namespace {
TEST_F(ASTVerifierTest, ValidateCorrectGetterSetter)
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

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<GetterSetterValidation>());
    }
}

TEST_F(ASTVerifierTest, ValidateAbstractGettersSetters)
{
    DiagnosticEngine de {};
    ETSChecker checker {Allocator(), de};

    char const *text =
        R"(
        abstract class AbstractClass {
            private _value: int;
            abstract set value(v: int);
            abstract get value(): int;
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<GetterSetterValidation>());
    }
}

TEST_F(ASTVerifierTest, ValidateAmbientGettersSetters)
{
    DiagnosticEngine de {};
    ETSChecker checker {Allocator(), de};

    char const *text =
        R"(
        declare class A {
            get foo(): int
            set foo(x: int)
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        EXPECT_TRUE(Verify<GetterSetterValidation>());
    }
}
}  // anonymous namespace
