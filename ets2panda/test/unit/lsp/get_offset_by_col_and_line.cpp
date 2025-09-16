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

#include "lsp_api_test.h"

#include <gtest/gtest.h>
#include <cstddef>

#include "ir/astNode.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace {

class LSPOffsetTests : public LSPAPITests {};

using ark::es2panda::lsp::Initializer;

TEST_F(LSPAPITests, getOffsetComment)
{
    std::vector<std::string> files = {"getOffsetComment.ets"};
    std::vector<std::string> texts = {R"delimiter(
// comment of line 2

let aaa = 'default string';
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    const size_t line = 4;
    const size_t col = 17;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getOffsetByColAndLine(ctx, line, col);
    const size_t expectedOffset = 39;
    ASSERT_EQ(res, expectedOffset);
    initializer.DestroyContext(ctx);
}

}  // namespace
