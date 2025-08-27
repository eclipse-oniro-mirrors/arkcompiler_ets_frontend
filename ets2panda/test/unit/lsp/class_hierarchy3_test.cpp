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

TEST_F(LspClassHierarchyTests, GetTypeHierarchiesImpl_003)
{
    std::vector<std::string> fileNames = {"aa3.ets", "bb3.ets", "cc3.ets", "ii3.ets", "mm3.ets", "nn3.ets", "pp3.ets"};
    std::vector<std::string> fileContents = {
        R"(
        export class AAA {}
        )",
        R"(
        import { AAA } from "./aa3"
        export class BBB extends AAA {}
    )",
        R"(import { BBB } from "./bb3"
        import { NNN } from "./nn3"
        import { PPP } from "./pp3"
        class CCC extends BBB implements NNN, PPP {}
    )",
        R"(export interface III {}
    )",
        R"(
        export interface MMM {}
    )",
        R"(import { III } from "./ii3"
        import { MMM } from "./mm3"
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
    const int position = 20;
    const int fileIndex = 3;
    auto context = initializer.CreateContext(filePaths[fileIndex].c_str(), ES2PANDA_STATE_CHECKED);
    auto node = ark::es2panda::lsp::GetTargetDeclarationNodeByPosition(context, position);
    auto tokenId = ark::es2panda::lsp::GetOwnerId(node);
    auto tokenName = ark::es2panda::lsp::GetIdentifierName(node);
    const int fileIndex5 = 5;
    auto context5 = initializer.CreateContext(filePaths[fileIndex5].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = ark::es2panda::lsp::GetTypeHierarchiesImpl(context5, position, node);
    initializer.DestroyContext(context);
    const size_t childNum1 = 2;
    const size_t childNum2 = 2;
    ASSERT_EQ(res.subHierarchies.subOrSuper.size(), childNum1);
    ASSERT_EQ(res.subHierarchies.subOrSuper[1].subOrSuper.size(), childNum2);
}

}  // namespace
