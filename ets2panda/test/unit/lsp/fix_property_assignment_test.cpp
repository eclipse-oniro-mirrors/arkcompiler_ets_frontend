/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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

namespace {
constexpr int DEFAULT_THROTTLE = 20;
using ark::es2panda::lsp::codefixes::FIX_PROPERTY_ASSIGNMENT;
constexpr auto ERROR_CODES = FIX_PROPERTY_ASSIGNMENT.GetSupportedCodeNumbers();

class FixPropertyAssignmentTests : public LSPAPITests {
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

TEST_F(FixPropertyAssignmentTests, FixesInvalidProperty_WhenCursorOnEquals)
{
    const std::string code = R"(
interface Temp {
    name: string;
    age: number;
}
const obj: Temp = {
    name: "Alice",
    age = 30
}
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("fix_equals.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    const size_t cursor = code.find("age = 30") + std::string("age ").size();
    ASSERT_NE(cursor, std::string::npos);
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions opts = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, cursor, cursor + 1, errorCodes, opts);
    ASSERT_EQ(fixes.size(), 1);
    ASSERT_FALSE(fixes[0].changes_.empty());
    const auto &change = fixes[0].changes_[0].textChanges[0];
    EXPECT_NE(change.newText.find("age: 30"), std::string::npos);
    initializer.DestroyContext(ctx);
}

TEST_F(FixPropertyAssignmentTests, SkipsNormalObjectLiteral)
{
    const std::string code = R"(
interface Temp {
    a: number;
}
const obj: Temp = { a: 1 };
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("fix3.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    size_t cursor = code.find("a: 1");
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    const size_t length = 1;
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, cursor, cursor + length, errorCodes, emptyOptions);
    EXPECT_TRUE(fixResult.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(FixPropertyAssignmentTests, SkipsIrrelevantContext)
{
    const std::string code = R"(
let x = 5;
)";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("fix5.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    size_t cursor = code.find('x');
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    const size_t length = 1;
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, cursor, cursor + length, errorCodes, emptyOptions);
    EXPECT_TRUE(fixResult.empty());
    initializer.DestroyContext(ctx);
}
}  // namespace