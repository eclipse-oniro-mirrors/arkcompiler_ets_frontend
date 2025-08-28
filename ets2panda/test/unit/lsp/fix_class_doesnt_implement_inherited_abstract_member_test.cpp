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
#include "generated/code_fix_register.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/types.h"
#include "lsp/include/register_code_fix/fix_class_doesnt_implement_inherited_abstract_member.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/code_fixes/code_fix_types.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetFixId();
constexpr auto ERROR_CODES = FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetSupportedCodeNumbers();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Add missing inherited abstract members";
constexpr int DEFAULT_THROTTLE = 20;

class LspFixAbstractMemberTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        return index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
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

TEST_F(LspFixAbstractMemberTests, TestFixClassNotImplementingInheritedMembers1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("LspFixAbstractMemberTests_001.ets", ES2PANDA_STATE_CHECKED, R"(abstract class A {
  abstract foo(a:number, b:number): number;
  abstract foo1(a:number, b:number);
}

class B extends A {
  foo(a: number, b: number): number {}
})");
    std::string expectedFileName = "LspFixAbstractMemberTests_001.ets";
    const size_t start = LineColToPos(ctx, 6, 19);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 122;
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto result = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, start, length, errorCodes, options);

    ASSERT_EQ(result[0].changes_[0].fileName, expectedFileName);
    ASSERT_EQ(result[0].changes_[0].textChanges[0].newText, "  foo1(a: number, b: number) {}\n");
    ASSERT_EQ(result[0].changes_[0].textChanges[0].span.start, expectedTextChangeStart);
    ASSERT_EQ(result[0].fixName_, EXPECTED_FIX_NAME);
    ASSERT_EQ(result[0].fixId_, EXPECTED_FIX_NAME);
    ASSERT_EQ(result[0].description_, EXPECTED_FIX_DESCRIPTION);
    initializer.DestroyContext(ctx);
}

TEST_F(LspFixAbstractMemberTests, TestFixClassNotImplementingInheritedMembers2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("LspFixAbstractMemberTests_002.ets", ES2PANDA_STATE_CHECKED, R"(abstract class A {
  abstract foo(a:number, b:number): number;
  abstract foo1(a:number, b:number);
}

class B extends A {
};
class C extends A {
})");

    std::string expectedNewText = "  foo(a: number, b: number): number {}\n  foo1(a: number, b: number) {}\n";
    std::string expectedFileName = "LspFixAbstractMemberTests_002.ets";
    const size_t expectedTextChangeStart1 = 122;
    const size_t expectedTextChangeStart2 = 145;
    const int expectedFixResultSize = 2;
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    CombinedCodeActionsInfo result = ark::es2panda::lsp::GetCombinedCodeFixImpl(ctx, EXPECTED_FIX_NAME.data(), options);
    ASSERT_EQ(result.changes_[0].textChanges.size(), expectedFixResultSize);
    ASSERT_EQ(result.changes_[0].fileName, expectedFileName);
    ASSERT_EQ(result.changes_[0].textChanges[0].newText, expectedNewText);
    ASSERT_EQ(result.changes_[0].textChanges[1].newText, expectedNewText);
    ASSERT_EQ(result.changes_[0].textChanges[0].span.start, expectedTextChangeStart1);
    ASSERT_EQ(result.changes_[0].textChanges[1].span.start, expectedTextChangeStart2);
    initializer.DestroyContext(ctx);
}

}  // namespace