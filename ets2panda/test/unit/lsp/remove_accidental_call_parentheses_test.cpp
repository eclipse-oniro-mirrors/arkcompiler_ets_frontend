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
#include "lsp/include/register_code_fix/remove_accidental_call_parentheses.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "lsp_api_test.h"

namespace {
constexpr int DEFAULT_THROTTLE = 20;
const size_t GETTER_CALL_IDX = 92;
const size_t IGNORE_MALFORMED_CALL_IDX = 42;
const size_t NON_FUNC_CALLS_IDX = 83;
const size_t SKIP_VALID_METHOD_CALL_IDX = 77;

class FixRemoveCallParensTests : public LSPAPITests {
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

TEST_F(FixRemoveCallParensTests, RemovesParenthesesFromGetterCall)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
class Obj {
  get value() {
    return () => 42;
  }
}
const obj = new Obj();
const x = obj.value();
    )";

    const auto c1 = 1;
    const auto c2 = 2;
    es2panda_Context *ctx = initializer.CreateContext("rmv_accidental_call_parens_getter_call.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    ark::es2panda::lsp::FixRemoveAccidentalCallParentheses fix;
    ark::es2panda::lsp::CodeFixContext context = CreateCodeFixContext(ctx, GETTER_CALL_IDX);
    auto actions = fix.GetCodeActions(context);
    ASSERT_EQ(actions.size(), c1);
    const auto &change = actions[0].changes[0].textChanges[0];
    EXPECT_EQ(change.newText, "");
    EXPECT_EQ(change.span.length, c2);
    initializer.DestroyContext(ctx);
}

TEST_F(FixRemoveCallParensTests, ignore_malformed_call_expressions)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
const obj = { value: 42 };
const z = obj.value(;
    )";

    es2panda_Context *ctx = initializer.CreateContext("rmv_accidental_call_parens_mallformed_call_expr.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    ark::es2panda::lsp::FixRemoveAccidentalCallParentheses fix;
    ark::es2panda::lsp::CodeFixContext context = CreateCodeFixContext(ctx, IGNORE_MALFORMED_CALL_IDX);
    auto actions = fix.GetCodeActions(context);
    EXPECT_TRUE(actions.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(FixRemoveCallParensTests, remove_parens_from_non_function_property_call)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
type MyObj = {
  value: number;
};
const obj: MyObj = { value: 5 };
const z = obj.value()
    )";

    const auto c1 = 1;
    const auto c2 = 2;
    es2panda_Context *ctx = initializer.CreateContext("rmv_accidental_call_parens_nonfunc_propety_call.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    ark::es2panda::lsp::FixRemoveAccidentalCallParentheses fix;
    ark::es2panda::lsp::CodeFixContext context = CreateCodeFixContext(ctx, NON_FUNC_CALLS_IDX);
    auto actions = fix.GetCodeActions(context);
    ASSERT_EQ(actions.size(), c1);
    const auto &change = actions[0].changes[0].textChanges[0];
    EXPECT_EQ(change.newText, "");
    EXPECT_EQ(change.span.length, c2);
    initializer.DestroyContext(ctx);
}

TEST_F(FixRemoveCallParensTests, SkipsValidMethodCall)
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
    ark::es2panda::lsp::FixRemoveAccidentalCallParentheses fix;
    ark::es2panda::lsp::CodeFixContext context = CreateCodeFixContext(ctx, SKIP_VALID_METHOD_CALL_IDX);
    auto actions = fix.GetCodeActions(context);
    EXPECT_TRUE(actions.empty());
    initializer.DestroyContext(ctx);
}
}  // namespace