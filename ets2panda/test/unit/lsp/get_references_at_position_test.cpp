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

#include <cstddef>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/references.h"
#include "public/es2panda_lib.h"

using ark::es2panda::lsp::Initializer;

class LSPGetReferencesAtPositionTests : public LSPAPITests {
public:
    LSPGetReferencesAtPositionTests() = default;
    ~LSPGetReferencesAtPositionTests() override = default;

    NO_COPY_SEMANTIC(LSPGetReferencesAtPositionTests);
    NO_MOVE_SEMANTIC(LSPGetReferencesAtPositionTests);

    References MockGetReferencesAtPosition(char const *fileName, size_t position,
                                           const std::vector<std::string> &filePaths)
    {
        Initializer initializer = Initializer();
        auto context = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED);
        auto astNode = ark::es2panda::lsp::GetTouchingToken(context, position, false);
        auto declInfo = ark::es2panda::lsp::GetDeclInfoImpl(astNode);
        initializer.DestroyContext(context);

        References result {};
        for (auto const &file : filePaths) {
            auto fileContext = initializer.CreateContext(file.c_str(), ES2PANDA_STATE_CHECKED);
            auto refInfo = ark::es2panda::lsp::GetReferencesAtPositionImpl(fileContext, declInfo);
            result.referenceInfos.insert(result.referenceInfos.end(), refInfo.referenceInfos.begin(),
                                         refInfo.referenceInfos.end());
            initializer.DestroyContext(fileContext);
        }
        auto comp = [](const ReferenceInfo &lhs, const ReferenceInfo &rhs) {
            if (lhs.fileName != rhs.fileName) {
                return lhs.fileName < rhs.fileName;
            }
            if (lhs.start != rhs.start) {
                return lhs.start < rhs.start;
            }
            return lhs.length < rhs.length;
        };
        ark::es2panda::lsp::RemoveDuplicates(result.referenceInfos, comp);
        return result;
    }
};

TEST_F(LSPGetReferencesAtPositionTests, GetReferencesAtPosition1)
{
    std::vector<std::string> files = {"references1.ets"};
    std::vector<std::string> texts = {R"(let a = 1;;)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 4;
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), position, filePaths);
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<ReferenceInfo> expectedResult {};
    // NOLINTEND(readability-magic-numbers)
    ASSERT_EQ(result.referenceInfos.size(), expectedResult.size());
    for (size_t i = 0; i < expectedResult.size(); i++) {
        ASSERT_EQ(result.referenceInfos[i].fileName, expectedResult[i].fileName);
        ASSERT_EQ(result.referenceInfos[i].start, expectedResult[i].start);
        ASSERT_EQ(result.referenceInfos[i].length, expectedResult[i].length);
    }
}

TEST_F(LSPGetReferencesAtPositionTests, GetReferencesAtPosition2)
{
    std::vector<std::string> files = {"references2.ets", "references3.ets"};
    std::vector<std::string> texts = {R"(export let a = 1;
let b = a;
function C() {
    let c = a;
};
function D() {
    let a = 1;
};)",
                                      R"(import {a} from './references2';
console.log(a);)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 26;
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), position, filePaths);
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<ReferenceInfo> expectedResult {
        {filePaths[0], 26, 1}, {filePaths[0], 56, 1}, {filePaths[1], 8, 1}, {filePaths[1], 45, 1}};
    // NOLINTEND(readability-magic-numbers)
    ASSERT_EQ(result.referenceInfos.size(), expectedResult.size());
    for (size_t i = 0; i < expectedResult.size(); i++) {
        ASSERT_EQ(result.referenceInfos[i].fileName, expectedResult[i].fileName);
        ASSERT_EQ(result.referenceInfos[i].start, expectedResult[i].start);
        ASSERT_EQ(result.referenceInfos[i].length, expectedResult[i].length);
    }
}

TEST_F(LSPGetReferencesAtPositionTests, GetReferencesAtPosition3)
{
    std::vector<std::string> files = {"references4.ets", "references5.ets"};
    std::vector<std::string> texts = {R"(export function A(){};)", R"(import {A} from './references4';
A();)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 16;
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), position, filePaths);
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<ReferenceInfo> expectedResult {{filePaths[1], 8, 1}, {filePaths[1], 33, 1}};
    // NOLINTEND(readability-magic-numbers)
    ASSERT_EQ(result.referenceInfos.size(), expectedResult.size());
    for (size_t i = 0; i < expectedResult.size(); i++) {
        ASSERT_EQ(result.referenceInfos[i].fileName, expectedResult[i].fileName);
        ASSERT_EQ(result.referenceInfos[i].start, expectedResult[i].start);
        ASSERT_EQ(result.referenceInfos[i].length, expectedResult[i].length);
    }
}
