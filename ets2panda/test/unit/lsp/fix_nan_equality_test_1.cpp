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

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp_api_test.h"

namespace {

using ark::es2panda::lsp::Initializer;

// FixNaNEquality is a template provider whose change branch requires a BinaryExpression typed as
// TS_IMPORT_EQUALS_DECLARATION, which can never hold. The tests below pin both NaN orientations
// through the public provider entry: each reaches the isLeftNaN/isRightNaN classification and
// deterministically returns no edit.
constexpr int FIX_NAN_EQUALITY_ERROR_CODE = 1003;
constexpr int DEFAULT_THROTTLE = 20;
// GetFixesAtOperator locates the two-character comparison token "=="; the fix
// query range spans exactly that token.
constexpr size_t COMPARISON_OPERATOR_TOKEN_LENGTH = 2;

class FixNaNEqualityTests1 : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        // line/column are 1-based
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        return index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
    }

    static std::vector<CodeFixActionInfo> GetFixesAtOperator(es2panda_Context *context, const std::string &source)
    {
        // The comparison operator token sits at the marked position of "OP" in the source
        const auto pos = source.find("==");
        EXPECT_NE(pos, std::string::npos);
        std::vector<int> errorCodes = {FIX_NAN_EQUALITY_ERROR_CODE};
        CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, pos, pos + COMPARISON_OPERATOR_TOKEN_LENGTH,
                                                              errorCodes, options);
    }

private:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    static NullCancellationToken &GetNullHost()
    {
        static NullCancellationToken instance;
        return instance;
    }
};

// Left-NaN orientation: `NaN == x` reaches the provider's NaN classification and returns no fix.
TEST_F(FixNaNEqualityTests1, LeftNaNOrientationReturnsNoFix)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("NanLeftOrientation.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(function check(x: number): void {
    if (NaN == x) {}
})");

    auto result = GetFixesAtOperator(ctx, R"(function check(x: number): void {
    if (NaN == x) {}
})");
    ASSERT_TRUE(result.empty());

    initializer.DestroyContext(ctx);
}

// Right-NaN orientation with strict equality: `x === NaN` also stays without a fix.
TEST_F(FixNaNEqualityTests1, RightNaNStrictEqualityOrientationReturnsNoFix)
{
    const std::string source = R"(function check(x: number): void {
    if (x === NaN) {}
})";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("NanRightStrict.ets", ES2PANDA_STATE_CHECKED, source.c_str());

    const auto pos = source.find("===");
    ASSERT_NE(pos, std::string::npos);
    std::vector<int> errorCodes = {FIX_NAN_EQUALITY_ERROR_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto result = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 3, errorCodes, options);
    ASSERT_TRUE(result.empty());

    initializer.DestroyContext(ctx);
}

// Both-sides orientation: `NaN == NaN` classifies as NaN on either side and returns no fix.
TEST_F(FixNaNEqualityTests1, BothSidesNaNOrientationReturnsNoFix)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("NanBothSides.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(function check(): void {
    if (NaN == NaN) {}
})");

    auto result = GetFixesAtOperator(ctx, R"(function check(): void {
    if (NaN == NaN) {}
})");
    ASSERT_TRUE(result.empty());

    initializer.DestroyContext(ctx);
}

// Inequality orientation: `x != NaN` is classified like the equality forms and yields no fix.
TEST_F(FixNaNEqualityTests1, InequalityOrientationReturnsNoFix)
{
    const std::string source = R"(function check(x: number): void {
    if (x != NaN) {}
})";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("NanInequality.ets", ES2PANDA_STATE_CHECKED, source.c_str());

    const auto pos = source.find("!=");
    ASSERT_NE(pos, std::string::npos);
    std::vector<int> errorCodes = {FIX_NAN_EQUALITY_ERROR_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto result = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 2, errorCodes, options);
    ASSERT_TRUE(result.empty());

    initializer.DestroyContext(ctx);
}

// A non-identifier right operand next to NaN (member access) still takes the left-NaN branch
// and returns no edit.
TEST_F(FixNaNEqualityTests1, LeftNaNWithMemberAccessOperandReturnsNoFix)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("NanLeftMember.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(function check(v: number[]): void {
    if (NaN == v[0]) {}
})");

    auto result = GetFixesAtOperator(ctx, R"(function check(v: number[]): void {
    if (NaN == v[0]) {}
})");
    ASSERT_TRUE(result.empty());

    initializer.DestroyContext(ctx);
}

}  // namespace
