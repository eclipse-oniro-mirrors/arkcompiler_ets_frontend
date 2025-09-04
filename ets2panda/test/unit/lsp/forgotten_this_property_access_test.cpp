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

#include <gtest/gtest.h>
#include <iostream>
#include "lsp/include/register_code_fix/forgotten_this_property_access.h"
#include "generated/code_fix_register.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FORGOTTEN_THIS_PROPERTY_ACCESS;

constexpr std::string_view EXPECTED_FIX_NAME = FORGOTTEN_THIS_PROPERTY_ACCESS.GetFixId();
constexpr auto ERROR_CODES = FORGOTTEN_THIS_PROPERTY_ACCESS.GetSupportedCodeNumbers();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Add 'this.' to property access";
constexpr int DEFAULT_THROTTLE = 20;
constexpr int FORGOTTEN_THIS_PROPERTY_ACCESS_ID = 145;
class ForgottenThisPropertyAccessTests : public LSPAPITests {
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

TEST_F(ForgottenThisPropertyAccessTests, TestForgottenThisPropertyAccess_BasicCase)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("ForgottenThisPropertyAccess_Basic.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(class Person {
name = "Alice";
greet() {
console.log(name);
}
})");

    auto ctxInternal = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto &diagnostics =
        ctxInternal->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);

    bool foundPropAccessError = false;
    for (const auto &diagnostic : diagnostics) {
        if (diagnostic->GetId() == FORGOTTEN_THIS_PROPERTY_ACCESS_ID) {
            foundPropAccessError = true;
            break;
        }
    }

    ASSERT_TRUE(foundPropAccessError);

    std::string expectedFileName = "ForgottenThisPropertyAccess_Basic.ets";
    const size_t start = LineColToPos(ctx, 4, 13);
    const size_t length = 4;
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto result = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, start, length, errorCodes, options);
    if (result.empty()) {
        initializer.DestroyContext(ctx);
        return;
    }

    if (!result.empty() && !result[0].changes_.empty()) {
        if (!result[0].changes_[0].textChanges.empty()) {
            ASSERT_TRUE(result[0].changes_[0].textChanges[0].newText.find("this.name") != std::string::npos);
        }
        ASSERT_EQ(result[0].fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result[0].fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(result[0].description_, EXPECTED_FIX_DESCRIPTION);
    }

    initializer.DestroyContext(ctx);
}

TEST_F(ForgottenThisPropertyAccessTests, TestForgottenThisPropertyAccess_MultipleProperties)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("ForgottenThisPropertyAccess_Multiple.ets", ES2PANDA_STATE_CHECKED,
                                  R"(class Person {
name = "Alice";
age = 25;
greet() {
console.log(name + " is " + age);
}
})");

    auto ctxInternal = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto &diagnostics =
        ctxInternal->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);

    constexpr size_t expectedDiagnosticCount = 2;
    ASSERT_GE(diagnostics.size(), expectedDiagnosticCount);

    std::string expectedFileName = "ForgottenThisPropertyAccess_Multiple.ets";
    const size_t start1 = LineColToPos(ctx, 5, 13);
    const size_t start2 = LineColToPos(ctx, 5, 29);
    const size_t length = 4;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto result1 = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, start1, length, errorCodes, options);
    ASSERT_FALSE(result1.empty()) << "Fix should be found for 'name' at position (5,13)";
    ASSERT_FALSE(result1[0].changes_.empty()) << "Changes should exist for 'name' fix";
    ASSERT_FALSE(result1[0].changes_[0].textChanges.empty()) << "Text changes should exist for 'name' fix";

    ASSERT_TRUE(result1[0].changes_[0].textChanges[0].newText.find("this.name") != std::string::npos);
    ASSERT_EQ(result1[0].fixName_, EXPECTED_FIX_NAME);
    ASSERT_EQ(result1[0].fixId_, EXPECTED_FIX_NAME);
    ASSERT_EQ(result1[0].description_, EXPECTED_FIX_DESCRIPTION);

    auto result2 = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, start2, length, errorCodes, options);
    ASSERT_FALSE(result2.empty()) << "Fix should be found for 'age' at position (5,29)";
    ASSERT_FALSE(result2[0].changes_.empty()) << "Changes should exist for 'age' fix";
    ASSERT_FALSE(result2[0].changes_[0].textChanges.empty()) << "Text changes should exist for 'age' fix";

    ASSERT_TRUE(result2[0].changes_[0].textChanges[0].newText.find("this.age") != std::string::npos);
    ASSERT_EQ(result2[0].fixName_, EXPECTED_FIX_NAME);
    ASSERT_EQ(result2[0].fixId_, EXPECTED_FIX_NAME);
    ASSERT_EQ(result2[0].description_, EXPECTED_FIX_DESCRIPTION);

    initializer.DestroyContext(ctx);
}

}  // namespace