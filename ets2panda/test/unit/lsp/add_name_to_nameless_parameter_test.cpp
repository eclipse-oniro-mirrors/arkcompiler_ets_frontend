/**
  Copyright (c) 2025 Huawei Device Co., Ltd.
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#include "gtest/gtest.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/register_code_fix/add_name_to_nameless_parameter.h"

namespace {
using ark::es2panda::lsp::codefixes::ADD_NAME_TO_NAMELESS_PARAMETER;
const size_t NO_FIX_WHEN_ALREADY_HAVE_NAME_IDX = 15;
constexpr auto ERROR_CODES = ADD_NAME_TO_NAMELESS_PARAMETER.GetSupportedCodeNumbers();
constexpr int DEFAULT_THROTTLE = 20;
const size_t ADD_NAME_KEEP_TYPE_IDX = 12;
const size_t NO_FIX_OUTSIDE_PARAM_CTX_IDX = 22;
const size_t ADD_NAME_FOR_SECOND_PARAM_IDX = 15;
class FixAddNameToNamelessParameterTests : public LSPAPITests {
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

TEST_F(FixAddNameToNamelessParameterTests, AddsNameAndKeepsType)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
function f(number) {
  return 1;
}
)";

    es2panda_Context *ctx = initializer.CreateContext("add_name_to_nameless_parameter_add_name.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    const size_t length = 1;
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(
        ctx, ADD_NAME_KEEP_TYPE_IDX, ADD_NAME_KEEP_TYPE_IDX + length, errorCodes, emptyOptions);
    ASSERT_EQ(fixResult.size(), 1);
    ASSERT_FALSE(fixResult[0].changes_.empty());
    ASSERT_FALSE(fixResult[0].changes_[0].textChanges.empty());
    const auto &change = fixResult[0].changes_[0].textChanges[0];
    EXPECT_EQ(change.newText, "arg0: number");
    initializer.DestroyContext(ctx);
}

TEST_F(FixAddNameToNamelessParameterTests, NoFixWhenParameterAlreadyHasNameAndType)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
function f(x: number) {
return x;
}
)";

    es2panda_Context *ctx = initializer.CreateContext("add_name_to_nameless_parameter_no_fix.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    const size_t length = 1;
    CodeFixOptions opts = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(
        ctx, NO_FIX_WHEN_ALREADY_HAVE_NAME_IDX, NO_FIX_WHEN_ALREADY_HAVE_NAME_IDX + length, errorCodes, opts);
    EXPECT_TRUE(fixes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(FixAddNameToNamelessParameterTests, NoFixOutsideParameterContext)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
function f(number) {
return number;
}
)";

    es2panda_Context *ctx = initializer.CreateContext("add_name_to_nameless_parameter_outside.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    const size_t length = 1;
    CodeFixOptions opts = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(
        ctx, NO_FIX_OUTSIDE_PARAM_CTX_IDX, NO_FIX_OUTSIDE_PARAM_CTX_IDX + length, errorCodes, opts);
    EXPECT_TRUE(fixes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(FixAddNameToNamelessParameterTests, AddsNameForSecondParameter)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
function g(x, string) {
return x;
}
)";

    es2panda_Context *ctx =
        initializer.CreateContext("add_name_to_nameless_parameter_2.ets", ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    const size_t length = 1;
    CodeFixOptions opts = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(
        ctx, ADD_NAME_FOR_SECOND_PARAM_IDX, ADD_NAME_FOR_SECOND_PARAM_IDX + length, errorCodes, opts);
    ASSERT_EQ(fixes.size(), 1);
    ASSERT_FALSE(fixes[0].changes_.empty());
    ASSERT_FALSE(fixes[0].changes_[0].textChanges.empty());
    const auto &change = fixes[0].changes_[0].textChanges[0];
    EXPECT_EQ(change.newText, "arg1: string");
    initializer.DestroyContext(ctx);
}
}  // namespace
