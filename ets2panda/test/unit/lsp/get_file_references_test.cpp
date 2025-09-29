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

    References MockGetFileReferences(char const *fileName, const std::vector<std::string> &filePaths)
    {
        Initializer initializer = Initializer();
        auto context = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED);
        bool isPackageModule = reinterpret_cast<ark::es2panda::public_lib::Context *>(context)
                                   ->parserProgram->Is<ark::es2panda::util::ModuleKind::PACKAGE>();
        initializer.DestroyContext(context);
        References result {};
        for (auto const &file : filePaths) {
            auto referenceContext = initializer.CreateContext(file.c_str(), ES2PANDA_STATE_CHECKED);
            auto refInfo = ark::es2panda::lsp::GetFileReferencesImpl(referenceContext, fileName, isPackageModule);
            result.referenceInfos.insert(result.referenceInfos.end(), refInfo.referenceInfos.begin(),
                                         refInfo.referenceInfos.end());
            initializer.DestroyContext(referenceContext);
        }
        return result;
    }
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

    References result = MockGetFileReferences(filePaths[0].c_str(), filePaths);
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

    References result = MockGetFileReferences(filePaths[0].c_str(), filePaths);
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
