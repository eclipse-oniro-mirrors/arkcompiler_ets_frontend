/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

using ark::es2panda::lsp::Initializer;

class LSPGetFileReferencesTests : public LSPAPITests {
public:
    LSPGetFileReferencesTests() = default;
    ~LSPGetFileReferencesTests() override = default;

    NO_COPY_SEMANTIC(LSPGetFileReferencesTests);
    NO_MOVE_SEMANTIC(LSPGetFileReferencesTests);
};

TEST_F(LSPGetFileReferencesTests, GetFileReferences1)
{
    std::vector<std::string> files = {"refer-1.ets", "file-references1.ets"};
    std::vector<std::string> texts = {
        R"(export function A(a:number, b:number): number {
  return a + b;
}
export function B(a:number, b:number): number {
  return a + b;
})",
        R"(import {A} from "./refer-1";
import {B} from "./refer-1.ets";
A(1, 2);
B(1, 2);)"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    char const *searchFileName = filePaths[0].c_str();
    char const *referenceFileName = filePaths[1].c_str();
    Initializer initializer = Initializer();
    auto ctx1 = initializer.CreateContext(referenceFileName, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx1), ES2PANDA_STATE_CHECKED);

    LSPAPI const *lspApi = GetImpl();
    lspApi->buildSymbolReferenceIndexForContextWithExternal(ctx1);
    auto result = lspApi->getFileReferences(searchFileName, ctx1, false);
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<ReferenceInfo> expectedResult {{filePaths[1], 16, 11}, {filePaths[1], 45, 15}};
    // NOLINTEND(readability-magic-numbers)
    ASSERT_EQ(result.referenceInfos.size(), expectedResult.size());
    for (size_t i = 0; i < expectedResult.size(); i++) {
        ASSERT_EQ(result.referenceInfos[i].fileName, expectedResult[i].fileName);
        ASSERT_EQ(result.referenceInfos[i].start, expectedResult[i].start);
        ASSERT_EQ(result.referenceInfos[i].length, expectedResult[i].length);
    }
}

TEST_F(LSPGetFileReferencesTests, GetFileReferences2)
{
    std::vector<std::string> files = {"refer-2.ts", "file-references2.ets"};
    std::vector<std::string> texts = {
        R"(export function A(a:number, b:number): number {
  return a + b;
}
export function B(a:number, b:number): number {
  return a + b;
})",
        R"(import {A} from "./refer-2";
import {B} from "./refer-2.ts";
A(1, 2);
B(1, 2);)"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    char const *searchFileName = filePaths[0].c_str();
    char const *referenceFileName = filePaths[1].c_str();
    Initializer initializer = Initializer();
    auto ctx1 = initializer.CreateContext(referenceFileName, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx1), ES2PANDA_STATE_CHECKED);

    LSPAPI const *lspApi = GetImpl();
    lspApi->buildSymbolReferenceIndexForContextWithExternal(ctx1);
    auto result = lspApi->getFileReferences(searchFileName, ctx1, false);
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<ReferenceInfo> expectedResult {{filePaths[1], 16, 11}, {filePaths[1], 45, 14}};
    // NOLINTEND(readability-magic-numbers)
    ASSERT_EQ(result.referenceInfos.size(), expectedResult.size());
    for (size_t i = 0; i < expectedResult.size(); i++) {
        ASSERT_EQ(result.referenceInfos[i].fileName, expectedResult[i].fileName);
        ASSERT_EQ(result.referenceInfos[i].start, expectedResult[i].start);
        ASSERT_EQ(result.referenceInfos[i].length, expectedResult[i].length);
    }
}
