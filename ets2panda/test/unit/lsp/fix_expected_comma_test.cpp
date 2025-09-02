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

#include "gtest/gtest.h"
#include "lsp/include/code_fixes/code_fix_types.h"
#include "lsp/include/register_code_fix/fix_expected_comma.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "lsp_api_test.h"

namespace {
constexpr int DEFAULT_THROTTLE = 20;
const size_t GETTER_CALL_IDX = 92;

class FixExpectedCommaTests : public LSPAPITests {
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

    static ark::es2panda::lsp::CodeFixContext CreateCodeFixContext(es2panda_Context *ctx, size_t pos)
    {
        ark::es2panda::lsp::RulesMap rules;
        ark::es2panda::lsp::FormatCodeSettings formatSettings;
        ark::es2panda::lsp::UserPreferences preferences;
        LanguageServiceHost host;
        ark::es2panda::lsp::FormatContext fmtCtx {formatSettings, rules};
        TextChangesContext textCtx {host, fmtCtx, preferences};
        return ark::es2panda::lsp::CodeFixContext {{textCtx, ctx, CreateToken()}, 0, TextSpan {pos, 0}};
    }
};

TEST_F(FixExpectedCommaTests, FixObjectExpression)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
interface A {
name: string;
age: number;
}
const a: A = {
name: "foo";
age: 123;
}
    )";

    const auto c1 = 1;
    const auto textChange = R"({
  name: "foo",
  age: 123,
})";
    es2panda_Context *ctx =
        initializer.CreateContext("fec_fix_object_expression.ets", ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    ark::es2panda::lsp::FixExpectedComma fix;
    ark::es2panda::lsp::CodeFixContext context = CreateCodeFixContext(ctx, GETTER_CALL_IDX);
    auto actions = fix.GetCodeActions(context);
    ASSERT_EQ(actions.size(), c1);
    ASSERT_EQ(actions[0].changes[0].textChanges[0].newText, textChange);
    initializer.DestroyContext(ctx);
}
}  // namespace
