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
#include "parser/ETSparser.h"
#include "ir/astNode.h"

#include <gtest/gtest.h>

using ark::es2panda::compiler::ast_verifier::ReferenceTypeAnnotationIsNull;
using ark::es2panda::ir::AstNode;

TEST_F(ASTVerifierTest, DISABLED_RefAnnotationNullNegative)  // #31976
{
    char const *text = R"(
        let refNull = 10;
        let trueRef = refNull;
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        auto *type = new ark::es2panda::ir::ETSPrimitiveType(ark::es2panda::ir::PrimitiveType::CHAR, Allocator());
        GetAst()->IterateRecursively([type](AstNode *astT) {
            if (astT->IsIdentifier() && astT->AsIdentifier()->Name() == "trueRef") {
                astT->AsIdentifier()->SetTsTypeAnnotation(type);
            }
        });

        EXPECT_TRUE(Verify<ReferenceTypeAnnotationIsNull>(ExpectVerifierMessage {"TYPE_ANNOTATION_NOT_NULLPTR"}));

        delete type;
    }
}

TEST_F(ASTVerifierTest, RefAnnotationNullDefaultParam)
{
    char const *text = R"(
        function pair(defParam: number, y: number = 7.0): number {
            return defParam + y;
        }

        function main() {
            arktest.assertEQ(pair(1.0), 8.0)
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<ReferenceTypeAnnotationIsNull>());
    }
}

TEST_F(ASTVerifierTest, RefAnnotationNullInterface)
{
    char const *text = R"(
        interface Base {
            f: int
        }

        function main(): void {
            let i: Base = { // variable definition
                f : 1
            };
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<ReferenceTypeAnnotationIsNull>());
    }
}

TEST_F(ASTVerifierTest, RefAnnotationNull1)
{
    char const *text = R"(
        let refNull = 10;
        let trueRef = refNull;
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<ReferenceTypeAnnotationIsNull>());
    }
}

TEST_F(ASTVerifierTest, RefAnnotationNull2)
{
    char const *text = R"(
        let refNull = 10;
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<ReferenceTypeAnnotationIsNull>());
    }
}
