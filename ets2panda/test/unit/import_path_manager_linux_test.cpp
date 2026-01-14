/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

#include "util/importPathManager.h"

namespace ark::es2panda::util {
// Forward declaration: exported from importPathManager.cpp but not in header.
void CreateDeclarationFile(const std::string &declFileName, const std::string &processed);
}  // namespace ark::es2panda::util

namespace {
std::string ReadFile(const std::filesystem::path &path)
{
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::filesystem::path MakeTempDeclPath()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    auto pid = static_cast<std::int64_t>(getpid());
    auto base = std::string("import_decl_") + std::to_string(pid) + "_" + std::to_string(now) + ".etscache";
    return std::filesystem::temp_directory_path() / base;
}
}  // namespace

using namespace ark::es2panda::util;  // NOLINT

TEST(CreateDeclarationFileLinuxTest, CreatesOnceAndKeepsContent)
{
#if defined(PANDA_TARGET_WINDOWS) || defined(USE_UNIX_SYSCALL)
    GTEST_SKIP() << "CreateDeclarationFileLinux is not used on this platform";
#endif

    const auto declPath = MakeTempDeclPath();
    const std::string first = "alpha";
    const std::string second = "beta";

    ASSERT_FALSE(std::filesystem::exists(declPath));

    CreateDeclarationFile(declPath.string(), first);
    ASSERT_TRUE(std::filesystem::exists(declPath));
    EXPECT_EQ(ReadFile(declPath), first);

    // Second call should fail exclusive create and leave file intact.
    CreateDeclarationFile(declPath.string(), second);
    EXPECT_EQ(ReadFile(declPath), first);

    std::filesystem::remove(declPath);
}
