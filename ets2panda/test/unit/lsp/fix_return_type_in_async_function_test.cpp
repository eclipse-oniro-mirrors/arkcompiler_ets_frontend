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
#include "lsp_api_test.h"
#include <gtest/gtest.h>
#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_return_type_in_async_function.h"

namespace {

constexpr auto ERROR_CODES = ark::es2panda::lsp::codefixes::FIX_RETURN_TYPE_IN_ASYNC_FUNCTION.GetSupportedCodeNumbers();
constexpr int DEFAULT_THROTTLE = 20;
class FixReturnTypeInAsyncFunctionTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static size_t GetPosition(es2panda_Context *context, size_t line, size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        return index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
    }

    static ark::es2panda::ir::AstNode *GetEnclosingFunction(ark::es2panda::ir::AstNode *node)
    {
        while (node != nullptr) {
            if (node->IsFunctionDeclaration() || node->IsFunctionExpression()) {
                return node;
            }
            node = node->Parent();
        }
        return nullptr;
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

TEST_F(FixReturnTypeInAsyncFunctionTests, WrapsReturnTypeInPromiseWhenAsync)
{
    std::vector<std::string> fileNames = {"TestAsyncReturnTypeFix.ets"};
    std::vector<std::string> fileContents = {R"(
async function getData(): string {
    return "hello";
}
)"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    ark::es2panda::lsp::Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const size_t pos = GetPosition(context, 2, 17);
    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());

    CodeFixOptions emptyOptions = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, pos, pos + 6, errorCodes, emptyOptions);

    ASSERT_EQ(fixResult.size(), 1);

    initializer.DestroyContext(context);
}

}  // namespace