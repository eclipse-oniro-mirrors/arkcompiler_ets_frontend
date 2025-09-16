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

#include <gtest/gtest.h>
#include <cstddef>
#include <cstdio>

#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/services/utilities.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/api.h"

using ark::es2panda::lsp::Initializer;

class LspGILAPTests : public LSPAPITests {};

TEST_F(LspGILAPTests, GILAPIsInitializer_Test)
{
    Initializer initializer = Initializer();
    const char *code = "class C {n: number = 0;constructor() {this.n = 12;};foo():this{return this}}";
    es2panda_Context *ctx = initializer.CreateContext("dummy-node.ets", ES2PANDA_STATE_CHECKED, code);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const offset = 20;
    auto result = ark::es2panda::lsp::GetTouchingToken(ctx, offset, false);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(ark::es2panda::lsp::IsInitializer(result));

    initializer.DestroyContext(ctx);
}

TEST_F(LspGILAPTests, GILAPIsInitializer_Test1)
{
    Initializer initializer = Initializer();
    const char *code = "class C {n: number=0;foo():this{return this}};let a: C = new C();";
    es2panda_Context *ctx = initializer.CreateContext("dummy-node.ets", ES2PANDA_STATE_CHECKED, code);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const offset = 10;
    auto result = ark::es2panda::lsp::GetTouchingToken(ctx, offset, false);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(ark::es2panda::lsp::IsInitializer(result));

    initializer.DestroyContext(ctx);
}

TEST_F(LspGILAPTests, GILAPIsReturnNode_Test)
{
    Initializer initializer = Initializer();
    const char *code = "function A(a:number, b:number):boolean{ return true;}";
    es2panda_Context *ctx = initializer.CreateContext("dummy-node.ets", ES2PANDA_STATE_CHECKED, code);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const offset = 48;
    auto result = ark::es2panda::lsp::GetTouchingToken(ctx, offset, false);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(ark::es2panda::lsp::IsReturnNode(result));

    initializer.DestroyContext(ctx);
}

TEST_F(LspGILAPTests, GILAPIsThisNode_Test)
{
    Initializer initializer = Initializer();
    const char *code = "class C {n: number = 0;foo():this{return this}}";
    es2panda_Context *ctx = initializer.CreateContext("dummy-node.ets", ES2PANDA_STATE_CHECKED, code);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const offset = 30;
    auto result = ark::es2panda::lsp::GetTouchingToken(ctx, offset, false);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(ark::es2panda::lsp::IsThis(result));

    initializer.DestroyContext(ctx);
}

TEST_F(LspGILAPTests, GILAPIsThisNode_Test1)
{
    Initializer initializer = Initializer();
    const char *code = "class C {n: number = 0;foo():this{return this}}";
    es2panda_Context *ctx = initializer.CreateContext("dummy-node.ets", ES2PANDA_STATE_CHECKED, code);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const offset = 42;
    auto result = ark::es2panda::lsp::GetTouchingToken(ctx, offset, false);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(ark::es2panda::lsp::IsThis(result));

    initializer.DestroyContext(ctx);
}

TEST_F(LspGILAPTests, GILAPIsValidImplementation_Test)
{
    Initializer initializer = Initializer();
    const char *code = "function A(){ 1+2; }";
    es2panda_Context *ctx = initializer.CreateContext("dummy-node.ets", ES2PANDA_STATE_CHECKED, code);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const offset = 15;
    auto result = ark::es2panda::lsp::GetTouchingToken(ctx, offset, false);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(ark::es2panda::lsp::IsValidImplementation(result));

    initializer.DestroyContext(ctx);
}

TEST_F(LspGILAPTests, GILAPIsAsTypeNode_Test)
{
    Initializer initializer = Initializer();
    const char *code =
        "class Shape {}class Circle extends Shape {x: number = 5}class Square extends Shape {y: string = 'a'}function "
        "createShape(): Shape {return new Circle()}let c2 = createShape() as Circle";
    es2panda_Context *ctx = initializer.CreateContext("dummy-node.ets", ES2PANDA_STATE_CHECKED, code);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const offset = 175;
    auto result = ark::es2panda::lsp::GetTouchingToken(ctx, offset, false);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->IsTSAsExpression());

    initializer.DestroyContext(ctx);
}

TEST_F(LspGILAPTests, GILAPGetImplementationAtPosition_Test)
{
    Initializer initializer = Initializer();
    static std::string source = R"(
    function Ali(a:number, b:number) {
        return a + b;
    }
    Ali(1, 2);
    function Ali(a:number) {
        return a;
    }
    Ali(1);
    )";
    es2panda_Context *ctx =
        initializer.CreateContext("implementation-at-position.ets", ES2PANDA_STATE_CHECKED, source.data());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const int callOffset = 72;

    auto result = GetImpl()->getImplementationLocationAtPosition(ctx, callOffset);

    initializer.DestroyContext(ctx);
    ASSERT_FALSE(result.empty());

    const int sL = 1;
    const int eL = 3;

    const int sI = 5;
    const int eI = 67;

    ASSERT_EQ(result[0].range_.start.line_, sL);
    ASSERT_EQ(result[0].range_.start.character_, sI);

    ASSERT_EQ(result[0].range_.end.line_, eL);
    ASSERT_EQ(result[0].range_.end.character_, eI);

    ASSERT_EQ(result[0].uri_.compare("implementation-at-position.ets"), 0);
}
