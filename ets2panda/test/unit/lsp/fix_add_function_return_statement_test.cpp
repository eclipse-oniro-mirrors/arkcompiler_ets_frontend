/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "gtest/gtest.h"
#include "lsp_api_test.h"
#include <cstddef>
#include <iostream>
#include "lsp/include/register_code_fix/fix_add_function_return_statement.h"

namespace {
class FixAddFunctionReturnStatementTests : public LSPAPITests {};

TEST_F(FixAddFunctionReturnStatementTests, AddMissingReturnStatement_GetInfo)
{
    const char *source = R"(
function multiply(a: number, b: number): number {
a * b;
}
    )";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("AddMissingReturnStatement_GetInfo.ets", ES2PANDA_STATE_CHECKED, source);
    const size_t position = 20;  // Position of the function body start
    const auto wordA = "a";
    const auto wordB = "b";
    const auto wordNum = "number";
    auto info = ark::es2panda::lsp::GetInfo(ctx, position);
    const auto type = info.GetStatements().at(0)->AsExpressionStatement()->GetExpression()->Type();
    EXPECT_EQ(type, ark::es2panda::ir::AstNodeType::BINARY_EXPRESSION);
    const auto left =
        info.GetStatements().at(0)->AsExpressionStatement()->GetExpression()->AsBinaryExpression()->Left()->ToString();
    EXPECT_EQ(left, wordA);
    const auto right =
        info.GetStatements().at(0)->AsExpressionStatement()->GetExpression()->AsBinaryExpression()->Right()->ToString();
    EXPECT_EQ(right, wordB);
    const auto returnTypeNode = info.GetReturnTypeNode()->AsETSTypeReference()->BaseName()->Name();
    EXPECT_EQ(returnTypeNode, wordNum);
    initializer.DestroyContext(ctx);
}

TEST_F(FixAddFunctionReturnStatementTests, AddMissingReturnStatement_ReplaceReturnType)
{
    const char *source = R"(
function multiply(a: number, b: number): string {
a;
}
    )";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test1.ets", ES2PANDA_STATE_CHECKED, source);
    const size_t position = 20;  // Position of the function body start
    const size_t size1 = 1;
    auto info = ark::es2panda::lsp::GetInfo(ctx, position);
    EXPECT_TRUE(info.GetReturnTypeNode()->IsETSTypeReference());
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    ark::es2panda::lsp::ReplaceReturnType(tracker, ctx, info);
    auto changes = tracker.GetChangeList();
    EXPECT_EQ(changes.size(), size1);
    initializer.DestroyContext(ctx);
}

TEST_F(FixAddFunctionReturnStatementTests, AddMissingReturnStatement_AddReturnStatement)
{
    const char *source = R"(
function multiply(a: number, b: number): string {
a;
}
    )";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("AddMissingReturnStatement_AddReturnStatement.ets", ES2PANDA_STATE_CHECKED, source);
    const size_t position = 20;  // Position of the function body start
    const size_t size1 = 1;
    auto info = ark::es2panda::lsp::GetInfo(ctx, position);
    EXPECT_TRUE(info.GetReturnTypeNode()->IsETSTypeReference());
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    ark::es2panda::lsp::AddReturnStatement(tracker, ctx, info.GetStatements(), info.GetBody());
    auto changes = tracker.GetChangeList();
    EXPECT_EQ(changes.size(), size1);
    initializer.DestroyContext(ctx);
}

}  // namespace