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
#include "lsp/include/register_code_fix/constructor_for_derived_need_super_call.h"

namespace {
constexpr int DEFAULT_THROTTLE = 20;
const size_t MISSING_SUPER_CALL_IDX = 80;
const size_t SKIP_BASE_CLASS_WITHOUT_SUPER_IDX = 20;
const size_t SKIP_IF_ALREADY_SUPER_EXISTS_IDX = 44;
using ark::es2panda::lsp::codefixes::CONSTRUCTOR_DERIVED_NEED_SUPER;
constexpr auto ERROR_CODES = CONSTRUCTOR_DERIVED_NEED_SUPER.GetSupportedCodeNumbers();
class FixConstructorForDerivedNeedSuperCallTests : public LSPAPITests {
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

TEST_F(FixConstructorForDerivedNeedSuperCallTests, AddsSuperCallToDerivedConstructor)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
class Animal {
constructor(public name:string) {}
}
class Dog extends Animal {
constructor(name:string) {
}
}
)";
    es2panda_Context *ctx = initializer.CreateContext("constructor_need_super_missing_super_call.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    const size_t length = 1;
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(
        ctx, MISSING_SUPER_CALL_IDX, MISSING_SUPER_CALL_IDX + length, errorCodes, emptyOptions);
    ASSERT_EQ(fixResult.size(), 1);
    const auto &change = fixResult[0].changes_[0].textChanges[0];
    EXPECT_EQ(change.newText, "super();");
    initializer.DestroyContext(ctx);
}

TEST_F(FixConstructorForDerivedNeedSuperCallTests, SkipsBaseClassWithoutSuper)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
class NotDerived {
constructor(name:string) {}
}
)";
    es2panda_Context *ctx =
        initializer.CreateContext("constructor_need_super_not_derived.ets", ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    const size_t length = 1;
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(
        ctx, SKIP_BASE_CLASS_WITHOUT_SUPER_IDX, SKIP_BASE_CLASS_WITHOUT_SUPER_IDX + length, errorCodes, emptyOptions);
    EXPECT_TRUE(fixResult.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(FixConstructorForDerivedNeedSuperCallTests, SkipsIfSuperAlreadyPresent)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string sourceCode = R"(
class Animal {}
class Dog extends Animal {
constructor(name:string) {
super();
}
}
)";
    es2panda_Context *ctx = initializer.CreateContext("constructor_need_super_already_has_super.ets",
                                                      ES2PANDA_STATE_CHECKED, sourceCode.c_str());
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    CodeFixOptions emptyOptions = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    const size_t length = 1;
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(
        ctx, SKIP_IF_ALREADY_SUPER_EXISTS_IDX, SKIP_IF_ALREADY_SUPER_EXISTS_IDX + length, errorCodes, emptyOptions);
    EXPECT_TRUE(fixResult.empty());
    initializer.DestroyContext(ctx);
}
}  // namespace