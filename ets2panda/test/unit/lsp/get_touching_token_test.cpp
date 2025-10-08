/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "lsp_api_test.h"

#include <gtest/gtest.h>
#include <cstddef>

#include "ir/astNode.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::Initializer;

TEST_F(LSPAPITests, GetTouchingToken1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("not-found-node.ets", ES2PANDA_STATE_CHECKED,
                                                      "function A(a:number, b:number) {\n  return a + b;\n}\nA(1, 2);");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 50;
    auto result = ark::es2panda::lsp::GetTouchingToken(ctx, offset, false);
    ASSERT_EQ(result, nullptr);

    auto result1 = ark::es2panda::lsp::GetTouchingToken(ctx, offset, true);
    ASSERT_EQ(result1, nullptr);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPAPITests, GetTouchingToken2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("nested-node.ets", ES2PANDA_STATE_CHECKED,
                                                      "function A(a:number, b:number) {\n  return a + b;\n}\nA(1, 2);");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 51;
    auto result = ark::es2panda::lsp::GetTouchingToken(ctx, offset, false);
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto expectedNode = ast->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->IsIdentifier() && node->AsIdentifier()->Name() == "A" && node->Parent()->IsCallExpression();
    });
    ASSERT_EQ(result->DumpJSON(), expectedNode->DumpJSON());
    ASSERT_EQ(result->Start().index, expectedNode->Start().index);
    ASSERT_EQ(result->End().index, expectedNode->End().index);
    initializer.DestroyContext(ctx);
}
}  // namespace
