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

#include "lsp/include/formatting/formatting.h"
#include "lsp/include/formatting/formatting_settings.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>

namespace {

class LSPFormattingTests : public LSPAPITests {};

TEST_F(LSPFormattingTests, GetFormatContextTest)
{
    ark::es2panda::lsp::FormatCodeSettings settings;

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    EXPECT_NE(&formatContext, nullptr);
}

TEST_F(LSPFormattingTests, FormatDocumentQuestionMarkTest)
{
    std::string testCode = R"(
     function conditionalTest(value:number):number{
         return value>0?value:-value;
     }
 )";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_question_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_CHECKED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

}  // namespace