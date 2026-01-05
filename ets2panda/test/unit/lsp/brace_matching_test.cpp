/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "lsp/include/brace_matching.h"
#include <gtest/gtest.h>
#include "lsp_api_test.h"
#include "lsp/include/api.h"

using ark::es2panda::lsp::Initializer;

class BraceMatchingTests : public LSPAPITests {};

TEST_F(BraceMatchingTests, GetBraceMatchingAtPositionAtEndOfFile)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;

    // Initialize context with an empty function
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("at-eof.ets", ES2PANDA_STATE_CHECKED, "function main() {}");

    // Test position at the end of the file
    const size_t eofPos = 18;
    const int expectedMapSize = 0;  // No matching

    std::vector<TextSpan> result = ark::es2panda::lsp::GetBraceMatchingAtPosition(ctx, eofPos);

    ASSERT_EQ(result.size(), expectedMapSize);

    initializer.DestroyContext(ctx);
}

TEST_F(BraceMatchingTests, GetBraceMatchingAtPositionMiddleOfToken)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;

    // Initialize context with a function containing braces
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("middleoftoken.ets", ES2PANDA_STATE_CHECKED, "function main() { let x = 10; }");

    // Test position inside a keyword ("function" at position 5)
    const size_t insideTokenPos = 5;
    const int expectedMapSize = 0;  // No matching brace since it's inside a token

    std::vector<TextSpan> result = ark::es2panda::lsp::GetBraceMatchingAtPosition(ctx, insideTokenPos);

    ASSERT_EQ(result.size(), expectedMapSize);

    initializer.DestroyContext(ctx);
}

TEST_F(BraceMatchingTests, GetBraceMatchingAtPositionEmptyBraces)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("emptybraces.ets", ES2PANDA_STATE_CHECKED, "function main() { }");

    const size_t position = 16;  // pos of {
    const int expectedMapSize = 2;

    std::vector<TextSpan> result = ark::es2panda::lsp::GetBraceMatchingAtPosition(ctx, position);

    int const expectedStart = 16;  // pos of {
    int const expectedEnd = 19;    // pos of }

    ASSERT_EQ(result.size(), expectedMapSize);
    ASSERT_EQ(result[0].start, expectedStart);
    ASSERT_EQ(result[1].start, expectedEnd);

    // Cleanup
    initializer.DestroyContext(ctx);
}

TEST_F(BraceMatchingTests, GetBraceMatchingAtPositionValidBraces)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;

    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("validbraces.ets", ES2PANDA_STATE_CHECKED, "function main() { let x = (2 + 3); }");

    // Test opening brace '{'
    const size_t openBracePos = 16;
    const int expectedMapSize = 2;
    std::vector<TextSpan> result = ark::es2panda::lsp::GetBraceMatchingAtPosition(ctx, openBracePos);

    int const expectedStart = 16;  // '{' position
    int const expectedEnd = 36;    // Matching '}'

    ASSERT_EQ(result.size(), expectedMapSize);
    ASSERT_EQ(result[0].start, expectedStart);
    ASSERT_EQ(result[1].start, expectedEnd);

    initializer.DestroyContext(ctx);
}

TEST_F(BraceMatchingTests, GetBraceMatchingAtPositionParamInstantiation)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;

    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("paraminst.ets", ES2PANDA_STATE_CHECKED, "let z = new Map<Object, number>()");

    // Test '<'
    const size_t pos = 15;
    const int expectedMapSize = 2;
    std::vector<TextSpan> result = ark::es2panda::lsp::GetBraceMatchingAtPosition(ctx, pos);

    int const expectedStart = 15;  // '<' position
    int const expectedEnd = 31;    // Matching '>'

    ASSERT_EQ(result.size(), expectedMapSize);
    ASSERT_EQ(result[0].start, expectedStart);
    ASSERT_EQ(result[1].start, expectedEnd);

    initializer.DestroyContext(ctx);
}

TEST_F(BraceMatchingTests, GetBraceMatchingAtPositionParamDeclaration)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("paramdec.ets", ES2PANDA_STATE_CHECKED, "export class B<T>{}");

    // Test '<'
    const size_t pos = 14;
    const int expectedMapSize = 2;
    std::vector<TextSpan> result = ark::es2panda::lsp::GetBraceMatchingAtPosition(ctx, pos);

    int const expectedStart = 14;  // '<' position
    int const expectedEnd = 17;    // Matching '>'

    ASSERT_EQ(result.size(), expectedMapSize);
    ASSERT_EQ(result[0].start, expectedStart);
    ASSERT_EQ(result[1].start, expectedEnd);

    initializer.DestroyContext(ctx);
}

TEST_F(BraceMatchingTests, GetBraceMatchingAtPositionInnerBlock)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;

    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("paramdec.ets", ES2PANDA_STATE_CHECKED, "function main() { { Math.random() < 5; } }");

    // Test inner '{'
    const size_t pos = 18;
    const int expectedMapSize = 2;
    std::vector<TextSpan> result = ark::es2panda::lsp::GetBraceMatchingAtPosition(ctx, pos);

    int const expectedStart = 18;  // inner '{' position
    int const expectedEnd = 40;    // inner '}' match

    ASSERT_EQ(result.size(), expectedMapSize);
    ASSERT_EQ(result[0].start, expectedStart);
    ASSERT_EQ(result[1].start, expectedEnd);

    initializer.DestroyContext(ctx);
}

TEST_F(BraceMatchingTests, GetBraceMatchingAtPositionForSpecialCharacters)
{
    std::vector<std::string> fileNames = {"aa1.ets"};
    std::vector<std::string> fileContents = {
        R"(
        //中文测试
        export class B<T>{}
    )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_TRUE(filePaths.size() == fileContents.size());

    // Test '<'
    const size_t pos = 38;
    const int expectedMapSize = 2;

    LSPAPI const *lspApi = GetImpl();
    std::vector<TextSpan> result = lspApi->getBraceMatchingAtPosition(filePaths[0].c_str(), pos);

    int const expectedStart = 38;  // '<' position
    int const expectedEnd = 41;    // Matching '>'

    ASSERT_EQ(result.size(), expectedMapSize);
    ASSERT_EQ(result[0].start, expectedStart);
    ASSERT_EQ(result[1].start, expectedEnd);
}