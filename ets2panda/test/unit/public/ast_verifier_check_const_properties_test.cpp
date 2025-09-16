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
#include "ir/base/classProperty.h"
#include <gtest/gtest.h>
#include "checker/ETSchecker.h"

using ark::es2panda::compiler::ast_verifier::CheckConstProperties;

namespace {

TEST_F(ASTVerifierTest, CheckConstProperties)
{
    ark::es2panda::util::DiagnosticEngine de {};
    ark::es2panda::checker::ETSChecker checker {Allocator(), de};

    char const *text = R"(
        class Test
        {
            readonly value = "readonly";
        }

        function main(): void {
            const test = new Test();
            console.log(test.value);
        }
    )";

    auto addConst = [](ark::es2panda::ir::AstNode *node) {
        if (node->IsClassProperty()) {
            auto property = node->AsClassProperty();
            if (property->IsReadonly()) {
                property->AddModifier(ark::es2panda::ir::ModifierFlags::CONST);
            }
        }
    };

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        GetAst()->IterateRecursively(addConst);

        EXPECT_TRUE(Verify<CheckConstProperties>(ExpectVerifierMessage {"Class property cannot be const"}));
    }
}

}  // namespace