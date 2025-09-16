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
#include "ir/expressions/callExpression.h"
#include "checker/ETSchecker.h"
#include "checker/types/signature.h"
#include "util/diagnosticEngine.h"
#include <gtest/gtest.h>

using ark::es2panda::checker::SignatureFlags;
using ark::es2panda::compiler::ast_verifier::CheckAbstractMethod;
using ark::es2panda::ir::Identifier;
using ark::es2panda::ir::MemberExpression;
using ark::es2panda::ir::MemberExpressionKind;
using ark::es2panda::ir::SuperExpression;

namespace {

TEST_F(ASTVerifierTest, LabelsHaveReferences)
{
    ark::es2panda::util::DiagnosticEngine de;
    ark::es2panda::checker::ETSChecker checker {Allocator(), de};

    char const *text = R"(
        abstract class A {
            abstract foo (): void
            bar(): void {}
        }

        class B extends A {
            foo () {
                super.bar()
            }
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        // Setup call to abstract method via super
        GetAst()->IterateRecursively([&checker, this](ark::es2panda::ir::AstNode *child) {
            if (child->IsCallExpression()) {
                auto *const call = child->AsCallExpression();
                auto *super = checker.AllocNode<SuperExpression>();
                auto *id = checker.AllocNode<Identifier>("foo", Allocator());

                auto *callee =
                    checker.AllocNode<MemberExpression>(super, id, MemberExpressionKind::PROPERTY_ACCESS, false, false);
                call->SetCallee(callee);

                // For testing just copy signature from original callee and add abstract flag
                auto *const signature = call->Signature();
                signature->AddSignatureFlag(SignatureFlags::ABSTRACT);
                call->SetSignature(signature);
            }
        });

        EXPECT_TRUE(Verify<CheckAbstractMethod>(ExpectVerifierMessage {"CALL TO ABSTRACT METHOD VIA SUPER"}));
    }
}

}  // namespace
