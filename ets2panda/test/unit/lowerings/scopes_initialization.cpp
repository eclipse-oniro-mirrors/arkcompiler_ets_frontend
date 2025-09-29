/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "libarkbase/macros.h"

#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "varbinder/tsBinding.h"
#include "varbinder/ETSBinder.h"
#include "ir/astNode.h"

#include "lowering_test.h"

namespace ark::es2panda {

using ScopesInitPhaseTest = LoweringTest;

static ir::Identifier *BodyToFirstName(ir::Statement *body)
{
    return body->AsBlockStatement()
        ->Statements()
        .front()
        ->AsVariableDeclaration()
        ->Declarators()
        .front()
        ->Id()
        ->AsIdentifier();
}

TEST_F(ScopesInitPhaseTest, TestForUpdateLoop)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    char const *text = R"(
        for (let x: int = 0; x < 10; x++) {
            let x: int;
        }
    )";

    CONTEXT(ES2PANDA_STATE_PARSED, text)
    {
        auto *const forNode = GetAst()->AsETSModule()->Statements()[0]->AsForUpdateStatement();
        compiler::InitScopesPhaseETS::RunExternalNode(forNode, GetAst()->AsETSModule()->Program());

        auto blockScope = forNode->Body()->AsBlockStatement()->Scope();
        auto loopScope = forNode->Scope();
        auto parScope = loopScope->Parent();
        ASSERT_EQ(blockScope->Parent(), loopScope);

        const auto &scopeBindings = blockScope->Bindings();
        const auto &parBindings = parScope->Bindings();

        ASSERT_EQ(scopeBindings.size(), 1);
        ASSERT_EQ(parBindings.size(), 1);

        auto parName = forNode->Init()->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier();
        auto name = BodyToFirstName(forNode->Body());
        ASSERT_EQ(scopeBindings.begin()->first, name->Name());
        ASSERT_EQ(parBindings.begin()->first, parName->Name());
        ASSERT_EQ(scopeBindings.begin()->second, name->Variable());
        ASSERT_EQ(parBindings.begin()->second, parName->Variable());
        ASSERT_NE(parName->Variable(), name->Variable());
    }
}

TEST_F(ScopesInitPhaseTest, CreateWhile)
{
    char const *text = R"(
        let x: int;
        while (x < 10) { let x:int; }
    )";

    CONTEXT(ES2PANDA_STATE_PARSED, text)
    {
        auto *const whileNode = GetAst()->AsETSModule()->Statements()[1]->AsWhileStatement();
        compiler::InitScopesPhaseETS::RunExternalNode(whileNode, GetAst()->AsETSModule()->Program());

        auto whileScope = whileNode->Scope();
        auto bodyScope = whileNode->Body()->AsBlockStatement()->Scope();
        ASSERT_EQ(bodyScope->Parent(), whileScope);

        const auto &bodyBindings = bodyScope->Bindings();
        auto name = BodyToFirstName(whileNode->Body());
        ASSERT_EQ(bodyBindings.size(), 1);
        ASSERT_EQ(bodyBindings.begin()->first, name->Name());
        ASSERT_EQ(bodyBindings.begin()->second, name->Variable());
    }
}

}  // namespace ark::es2panda
