/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

namespace {

class LSPOffsetTests : public LSPAPITests {};

TEST_F(LSPOffsetTests, getOffsetComment)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t line = 4;
    const size_t col = 17;
    auto res = lspApi->getOffsetByColAndLine(R"delimiter(
// comment of line 2

let aaa = 'default string';
)delimiter",
                                             line, col);
    const size_t expectedOffset = 39;
    ASSERT_EQ(res, expectedOffset);
}

TEST_F(LSPOffsetTests, getLineAndColByOffset)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 28;
    auto res = lspApi->getColAndLineByOffset(R"delimiter(
// comment of line 2

let aaa = 'default string';
)delimiter",
                                             offset);
    const size_t expectedLine = 4;
    const size_t expectedCol = 6;
    ASSERT_EQ(res.first, expectedLine);
    ASSERT_EQ(res.second, expectedCol);
}

TEST_F(LSPOffsetTests, getOffsetCommentForSpecialCharacters)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t line = 5;
    const size_t col = 15;
    auto res = lspApi->getOffsetByColAndLine(R"delimiter(
// comment of line 2

//中文测试
let aaa = '中文测试';
)delimiter",
                                             line, col);
    const size_t expectedOffset = 44;
    ASSERT_EQ(res, expectedOffset);
}

TEST_F(LSPOffsetTests, getLineAndColByOffsetForSpecialCharacters)
{
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 42;
    auto res = lspApi->getColAndLineByOffset(R"delimiter(
// comment of line 2

//中文测试
let aaa = '中文测试';
)delimiter",
                                             offset);
    const size_t expectedLine = 5;
    const size_t expectedCol = 13;
    ASSERT_EQ(res.first, expectedLine);
    ASSERT_EQ(res.second, expectedCol);
}

}  // namespace
