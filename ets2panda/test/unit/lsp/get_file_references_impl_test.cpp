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

#include "lsp_api_test.h"

#include <gtest/gtest.h>
#include <cstddef>

#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "public/public.h"

namespace {

using ark::es2panda::lsp::Initializer;

TEST_F(LSPAPITests, GetFileReferencesImpl1)
{
    using ark::es2panda::public_lib::Context;
    std::vector<std::string> files = {"lsp_api_test_export_1.ets", "lsp_api_test_file_1.ets"};
    std::vector<std::string> texts = {
        R"(export function A(a:number, b:number): number {
  return a + b;
}
export function B(a:number, b:number): number {
  return a + b;
})",
        R"(import {A} from "./lsp_api_test_export_1";
import {B} from "./lsp_api_test_export_1.ets";
A(1, 2);
B(1, 2);)"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    char const *searchFileName = filePaths[0].c_str();
    char const *referenceFileName = filePaths[1].c_str();
    Initializer initializer = Initializer();
    es2panda_Context *ctx1 = initializer.CreateContext(referenceFileName, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx1), ES2PANDA_STATE_CHECKED);

    LSPAPI const *lspApi = GetImpl();
    lspApi->buildSymbolReferenceIndexForContextWithExternal(ctx1);
    auto result = lspApi->getFileReferences(searchFileName, ctx1, false);
    auto expectedFileName1 = filePaths[1];
    size_t const expectedStartPos1 = 16;
    size_t const expectedLength1 = 25;
    auto expectedFileName2 = filePaths[1];
    size_t const expectedStartPos2 = 59;
    size_t const expectedLength2 = 29;
    ASSERT_EQ(result.referenceInfos.at(0).fileName, expectedFileName1);
    ASSERT_EQ(result.referenceInfos.at(0).start, expectedStartPos1);
    ASSERT_EQ(result.referenceInfos.at(0).length, expectedLength1);
    ASSERT_EQ(result.referenceInfos.at(1).fileName, expectedFileName2);
    ASSERT_EQ(result.referenceInfos.at(1).start, expectedStartPos2);
    ASSERT_EQ(result.referenceInfos.at(1).length, expectedLength2);
    initializer.DestroyContext(ctx1);
}

TEST_F(LSPAPITests, GetFileReferencesImpl2)
{
    using ark::es2panda::public_lib::Context;
    std::vector<std::string> files = {"lsp_api_test_export_2.ts", "lsp_api_test_file_2.ets"};
    std::vector<std::string> texts = {
        R"(export function A(a:number, b:number): number {
  return a + b;
}
export function B(a:number, b:number): number {
  return a + b;
})",
        R"(import {A} from "./lsp_api_test_export_2";
import {B} from "./lsp_api_test_export_2.ts";
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
    auto expectedFileName1 = filePaths[1];
    size_t const expectedStartPos1 = 16;
    size_t const expectedLength1 = 25;
    auto expectedFileName2 = filePaths[1];
    size_t const expectedStartPos2 = 59;
    size_t const expectedLength2 = 28;
    ASSERT_EQ(result.referenceInfos.at(0).fileName, expectedFileName1);
    ASSERT_EQ(result.referenceInfos.at(0).start, expectedStartPos1);
    ASSERT_EQ(result.referenceInfos.at(0).length, expectedLength1);
    ASSERT_EQ(result.referenceInfos.at(1).fileName, expectedFileName2);
    ASSERT_EQ(result.referenceInfos.at(1).start, expectedStartPos2);
    ASSERT_EQ(result.referenceInfos.at(1).length, expectedLength2);
    initializer.DestroyContext(ctx1);
}

}  // namespace
