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
#include <cstdio>
#include <gtest/gtest.h>
#include <regex>
#include <string>
#include <vector>

#include "lsp_api_test.h"
#include "lsp/include/class_hierarchy.h"
#include "lsp/include/internal_api.h"

namespace {

class LspClassHierarchyTests : public LSPAPITests {};

TEST_F(LspClassHierarchyTests, GetTypeHierarchiesImpl_002)
{
    std::vector<std::string> fileNames = {"aa2.ets", "bb2.ets", "cc2.ets", "ii2.ets", "mm2.ets", "nn2.ets", "pp2.ets"};
    std::vector<std::string> fileContents = {
        R"(
        export class AAA {}
        )",
        R"(
        import { AAA } from "./aa2"
        export class BBB extends AAA {}
    )",
        R"(import { BBB } from "./bb2"
        import { NNN } from "./nn2"
        import { PPP } from "./pp2"
        class CCC extends BBB implements NNN, PPP {}
    )",
        R"(export interface III {}
    )",
        R"(
        export interface MMM {}
    )",
        R"(import { III } from "./ii2"
        import { MMM } from "./mm2"
        export interface NNN extends III, MMM {}
        export interface NNN2 extends III {}
        export interface NNN3 extends NNN2 {}
        export interface NNN4 extends NNN2 {}
    )",
        R"(export interface PPP {}
    )"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_TRUE(filePaths.size() == fileContents.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    const int position = 94;
    const int fileIndex = 5;
    auto context = initializer.CreateContext(filePaths[fileIndex].c_str(), ES2PANDA_STATE_CHECKED);
    auto node = ark::es2panda::lsp::GetTouchingToken(context, position, false);
    auto tokenId = ark::es2panda::lsp::GetOwnerId(node);
    auto tokenName = ark::es2panda::lsp::GetIdentifierName(node);
    auto res = ark::es2panda::lsp::GetTypeHierarchiesImpl(context, position);
    initializer.DestroyContext(context);
    const size_t parentNum1 = 2;
    ASSERT_EQ(res.superHierarchies.subOrSuper.size(), parentNum1);
}

}  // namespace
