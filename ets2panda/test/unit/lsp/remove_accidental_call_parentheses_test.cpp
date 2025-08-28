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

#include "generated/code_fix_register.h"
#include "gtest/gtest.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/register_code_fix/remove_accidental_call_parentheses.h"
#include <cstddef>

namespace {
constexpr int DEFAULT_THROTTLE = 20;
const size_t GETTER_CALL_IDX = 92;
const size_t IGNORE_MALFORMED_CALL_IDX = 42;
const size_t SKIP_VALID_METHOD_CALL_IDX = 77;
using ark::es2panda::lsp::codefixes::REMOVE_ACCIDENTAL_CALL_PARENTHESES;
constexpr auto ERROR_CODES = REMOVE_ACCIDENTAL_CALL_PARENTHESES.GetSupportedCodeNumbers();
class FixRemoveCallParens_AtPos_Tests : public LSPAPITests {
public:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    static ark::es2panda::lsp::CancellationToken CreateToken()
    {
        static NullCancellationToken nullToken;
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &nullToken);
    }
};

TEST_F(FixRemoveCallParens_AtPos_Tests, RemovesParenthesesFromGetterCall)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
class User {
get name() {return "Alice;"}
}
const user = new User();
const name = user.name();
    )";

    es2panda_Context *ctx = initializer.CreateContext("rmv_accidental_call_parens_getter_call.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    const size_t length = 0;
    const size_t c1 = 1;
    const size_t c2 = 2;
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, GETTER_CALL_IDX, GETTER_CALL_IDX + length,
                                                                errorCodes, options);
    ASSERT_EQ(fixes.size(), c1);
    const auto &change = fixes[0].changes_[0].textChanges[0];
    EXPECT_EQ(change.newText, "");
    EXPECT_EQ(change.span.length, c2);
    initializer.DestroyContext(ctx);
}

TEST_F(FixRemoveCallParens_AtPos_Tests, IgnoreMalformedCallExpressions)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
const obj = { value: 42 };
const z = obj.value(;
    )";

    es2panda_Context *ctx = initializer.CreateContext("rmv_accidental_call_parens_malformed_call_expr.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    const size_t length = 0;
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(
        ctx, IGNORE_MALFORMED_CALL_IDX, IGNORE_MALFORMED_CALL_IDX + length, errorCodes, options);
    EXPECT_TRUE(fixes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(FixRemoveCallParens_AtPos_Tests, RemoveParensFromNonFunctionPropertyCall)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
class MyObj {
  value: number = 5;
}
const obj = new MyObj();
const z = obj.value()
    )";

    es2panda_Context *ctx = initializer.CreateContext("rmv_accidental_call_parens_nonfunc_property_call.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    const size_t c1 = 1;
    const size_t c2 = 2;
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    const std::string needle = "obj.value()";
    const auto posInStr = sourceCode.find(needle);
    ASSERT_NE(posInStr, std::string::npos);
    const size_t callPos = posInStr + std::string("obj.value").size();
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, callPos, callPos, errorCodes, options);
    ASSERT_EQ(fixes.size(), c1);
    const auto &change = fixes[0].changes_[0].textChanges[0];
    EXPECT_EQ(change.newText, "");
    EXPECT_EQ(change.span.length, c2);

    initializer.DestroyContext(ctx);
}

TEST_F(FixRemoveCallParens_AtPos_Tests, SkipsValidMethodCall)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
class A {
  calc(): number {
    return 1;
  }
}
const obj = new A();
const y = obj.calc();
    )";

    es2panda_Context *ctx = initializer.CreateContext("rmv_accidental_call_parens_valid_call.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    const size_t length = 0;
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(
        ctx, SKIP_VALID_METHOD_CALL_IDX, SKIP_VALID_METHOD_CALL_IDX + length, errorCodes, options);
    EXPECT_TRUE(fixes.empty());
    initializer.DestroyContext(ctx);
}
}  // namespace