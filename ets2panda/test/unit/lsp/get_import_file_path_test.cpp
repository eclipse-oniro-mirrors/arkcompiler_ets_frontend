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
#include <cstddef>
#include <string>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"

namespace {
using ark::es2panda::lsp::Initializer;

class LspGetImportFilePathTests : public LSPAPITests {};

TEST_F(LspGetImportFilePathTests, GetImportFilePath1)
{
    std::vector<std::string> files = {"GetImportFilePath1.ets", "GetImportFilePath2.ets"};
    std::vector<std::string> texts = {R"(export function A(a:number, b:number): number {
    return a + b;
})",
                                      R"(import {A} from './GetImportFilePath1';)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 22;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName = filePaths[0];
    size_t const expectedStart = 0;
    size_t const expectedLength = 0;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
}

TEST_F(LspGetImportFilePathTests, GetImportFilePath2)
{
    LSPAPI const *lspApi = GetImpl();
    // Test invalid position to avoid segment fault
    size_t const offset = 24;
    Initializer initializer = Initializer();
    auto ctx =
        initializer.CreateContext("invalidPositionTest.ets", ES2PANDA_STATE_CHECKED, "let invalidPositionTest = 0;");
    auto result = lspApi->getDefinitionAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);
    std::string expectedFileName;
    size_t const expectedStart = 0;
    size_t const expectedLength = 0;
    ASSERT_EQ(result.fileName, expectedFileName);
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
}
}  // namespace