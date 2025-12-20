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

#include <gtest/gtest.h>
#include <cstddef>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/api.h"
#include "public/es2panda_lib.h"

namespace {
using ark::es2panda::lsp::Initializer;

class LspGetSafeDeleteInfoTest : public LSPAPITests {};

TEST_F(LspGetSafeDeleteInfoTest, GetSafeDeleteInfoCase1)
{
    std::vector<std::string> files = {"get-safe-delete-info-case1.ets"};
    std::vector<std::string> texts = {R"(function a(): Number { return 1; } a())"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offset = 16;
    bool result = lspApi->getSafeDeleteInfo(ctx, offset);
    ASSERT_EQ(result, false);

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offsetT2 = 35;
    result = lspApi->getSafeDeleteInfo(ctx, offsetT2);
    ASSERT_EQ(result, true);

    initializer.DestroyContext(ctx);
}

TEST_F(LspGetSafeDeleteInfoTest, GetSafeDeleteInfoCase2)
{
    std::vector<std::string> files = {"get-safe-delete-info-case2.ets"};
    std::vector<std::string> texts = {R"(export const PI = 3.1415926;)"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offset = 14;
    bool result = lspApi->getSafeDeleteInfo(ctx, offset);
    ASSERT_EQ(result, true);

    initializer.DestroyContext(ctx);
}

TEST_F(LspGetSafeDeleteInfoTest, GetSafeDeleteInfoCase3)
{
    std::vector<std::string> files = {"get-safe-delete-info-case3.ets"};
    std::vector<std::string> texts = {R"(
    function setAlarmDefaultTime(alarmItem?: AlarmItem) {
      let hour;
      let minute;
    })"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offset = 71;
    bool result = lspApi->getSafeDeleteInfo(ctx, offset);
    ASSERT_EQ(result, true);

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offsetT2 = 89;
    result = lspApi->getSafeDeleteInfo(ctx, offsetT2);
    ASSERT_EQ(result, true);

    initializer.DestroyContext(ctx);
}

TEST_F(LspGetSafeDeleteInfoTest, GetSafeDeleteInfoCase4)
{
    std::vector<std::string> files = {"get-safe-delete-info-namespace.ets"};
    std::vector<std::string> texts = {R"(
    namespace Foo {}
    class Foo2 {}
    )"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offset = 14;
    bool result = lspApi->getSafeDeleteInfo(ctx, offset);
    ASSERT_EQ(result, false);

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offsetT2 = 31;
    result = lspApi->getSafeDeleteInfo(ctx, offsetT2);
    ASSERT_EQ(result, true);

    initializer.DestroyContext(ctx);
}

TEST_F(LspGetSafeDeleteInfoTest, GetSafeDeleteInfoCase5)
{
    std::vector<std::string> files = {"get-safe-delete-info-typeparam.ets"};
    std::vector<std::string> texts = {R"(
    function foo<T>(a: T) { return a; }
    )"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offset = 17;
    bool result = lspApi->getSafeDeleteInfo(ctx, offset);
    ASSERT_EQ(result, false);

    initializer.DestroyContext(ctx);
}

TEST_F(LspGetSafeDeleteInfoTest, GetSafeDeleteInfoCase6)
{
    std::vector<std::string> files = {"get-safe-delete-info-import.ets"};
    std::vector<std::string> texts = {R"(import { foo } from "./mod";)"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offset = 9;
    bool result = lspApi->getSafeDeleteInfo(ctx, offset);
    ASSERT_EQ(result, true);

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offsetT2 = 23;
    result = lspApi->getSafeDeleteInfo(ctx, offsetT2);
    ASSERT_EQ(result, true);

    initializer.DestroyContext(ctx);
}

TEST_F(LspGetSafeDeleteInfoTest, GetSafeDeleteInfoCase7)
{
    std::vector<std::string> files = {"get-safe-delete-info-arrow.ets"};
    std::vector<std::string> texts = {R"(
    export const calcDistance = (time: number) => {
        const ret = 0.5 * time * time;
        return ret;
    };
    )"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offset = 28;
    bool result = lspApi->getSafeDeleteInfo(ctx, offset);
    ASSERT_EQ(result, true);

    initializer.DestroyContext(ctx);
}

TEST_F(LspGetSafeDeleteInfoTest, GetSafeDeleteInfoCase8)
{
    std::vector<std::string> files = {"get-safe-delete-info-interface.ets"};
    std::vector<std::string> texts = {R"(
    export interface VideoPlayer {
        prepare(): Promise<void>;
    }

    function bar(parameter: VideoPlayer) {
        parameter.prepare();
    }
    )"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offset = 31;
    bool result = lspApi->getSafeDeleteInfo(ctx, offset);
    ASSERT_EQ(result, true);

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offsetT2 = 143;
    result = lspApi->getSafeDeleteInfo(ctx, offsetT2);
    ASSERT_EQ(result, true);

    initializer.DestroyContext(ctx);
}

TEST_F(LspGetSafeDeleteInfoTest, GetSafeDeleteInfoCase9)
{
    std::vector<std::string> files = {"get-safe-delete-info-importdefault.ets"};
    std::vector<std::string> texts = {R"(
    import BuildProfile from 'BuildProfile';
    BuildProfile.bundleName
    )"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offset = 15;
    bool result = lspApi->getSafeDeleteInfo(ctx, offset);
    ASSERT_EQ(result, true);

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offsetT2 = 52;
    result = lspApi->getSafeDeleteInfo(ctx, offsetT2);
    ASSERT_EQ(result, true);

    initializer.DestroyContext(ctx);
}

TEST_F(LspGetSafeDeleteInfoTest, GetSafeDeleteInfoCase10)
{
    std::vector<std::string> files = {"get-safe-delete-info-interface.ets"};
    std::vector<std::string> texts = {R"(
    //中文测试
    export interface VideoPlayer {
        prepare(): Promise<void>;
    }

    function bar(parameter: VideoPlayer) {
        //中文测试
        parameter.prepare();
    }
    )"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offset = 42;
    bool result = lspApi->getSafeDeleteInfo(ctx, offset);
    ASSERT_EQ(result, true);

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offsetT2 = 169;
    result = lspApi->getSafeDeleteInfo(ctx, offsetT2);
    ASSERT_EQ(result, true);

    initializer.DestroyContext(ctx);
}

TEST_F(LspGetSafeDeleteInfoTest, GetSafeDeleteInfoCase11)
{
    std::vector<std::string> files = {"get-safe-delete-info-importdefault.ets"};
    std::vector<std::string> texts = {R"(
    //中文测试
    import BuildProfile from 'BuildProfile';
    //中文测试
    BuildProfile.bundleName
    )"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offset = 26;
    bool result = lspApi->getSafeDeleteInfo(ctx, offset);
    ASSERT_EQ(result, true);

    // NOLINTNEXTLINE(readability-identifier-naming,-warnings-as-errors)
    constexpr size_t offsetT2 = 74;
    result = lspApi->getSafeDeleteInfo(ctx, offsetT2);
    ASSERT_EQ(result, true);

    initializer.DestroyContext(ctx);
}
}  // namespace
