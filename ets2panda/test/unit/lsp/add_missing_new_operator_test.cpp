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
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/code_fixes/code_fix_types.h"
#include "lsp/include/register_code_fix/add_missing_new_operator.h"

namespace {
constexpr int DEFAULT_THROTTLE = 20;
const size_t MISSING_NEW_IDX = 26;
const size_t ALREADY_HAS_NEW_IDX = 48;
const size_t SKIP_NON_CLASS_FUNC_CALLS_IDX = 48;
class FixAddMissingNewOperatorTests : public LSPAPITests {
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

TEST_F(FixAddMissingNewOperatorTests, AddsNewKeywordToConstructorCall)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
class Foo {}
let a:Foo = Foo();
)";
    es2panda_Context *ctx = initializer.CreateContext("missing_new_operator_constructor_call.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    ark::es2panda::lsp::FixAddMissingNewOperator fix;
    ark::es2panda::lsp::CodeFixContext context = CreateCodeFixContext(ctx, MISSING_NEW_IDX);
    auto actions = fix.GetCodeActions(context);
    ASSERT_EQ(actions.size(), 1);
    const auto &change = actions[0].changes[0].textChanges[0];
    EXPECT_TRUE(change.newText.find("new ") != std::string::npos);
    initializer.DestroyContext(ctx);
}

TEST_F(FixAddMissingNewOperatorTests, SkipsValidNewCall)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
class Foo {
constructor() {}
}
const obj = new Foo();
    )";
    es2panda_Context *ctx =
        initializer.CreateContext("missing_new_operator_skip_valid.ets", ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    ark::es2panda::lsp::FixAddMissingNewOperator fix;
    ark::es2panda::lsp::CodeFixContext context = CreateCodeFixContext(ctx, ALREADY_HAS_NEW_IDX);
    auto actions = fix.GetCodeActions(context);
    EXPECT_TRUE(actions.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(FixAddMissingNewOperatorTests, SkipsNonClassFunctionCalls)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
function foo() {}
foo();
    )";
    es2panda_Context *ctx = initializer.CreateContext("missing_new_operator_skip_non_class_func_calls.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    ark::es2panda::lsp::FixAddMissingNewOperator fix;
    ark::es2panda::lsp::CodeFixContext context = CreateCodeFixContext(ctx, SKIP_NON_CLASS_FUNC_CALLS_IDX);
    auto actions = fix.GetCodeActions(context);
    EXPECT_TRUE(actions.empty());
    initializer.DestroyContext(ctx);
}
}  // namespace