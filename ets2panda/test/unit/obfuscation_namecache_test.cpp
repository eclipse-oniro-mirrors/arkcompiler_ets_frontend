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

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

#include "ir/obfuscationNameCache.h"

namespace {

class ScopedTempDirectory {
public:
    ScopedTempDirectory()
    {
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto threadId = std::hash<std::thread::id> {}(std::this_thread::get_id());
        path_ = std::filesystem::temp_directory_path() /
                ("namecache_" + std::to_string(threadId) + "_" + std::to_string(timestamp));
        std::filesystem::create_directory(path_);
    }

    ~ScopedTempDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    ScopedTempDirectory(const ScopedTempDirectory &) = delete;
    ScopedTempDirectory &operator=(const ScopedTempDirectory &) = delete;

    const std::filesystem::path &Path() const
    {
        return path_;
    }

private:
    std::filesystem::path path_;
};

std::string ReadEntireFile(const std::filesystem::path &path)
{
    std::ifstream ifs(path);
    return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

}  // namespace

namespace ark::es2panda::ir {

class ObfuscationNameCacheFileTest : public ::testing::Test {
protected:
    static void SeedClassEntry(ObfuscationNameCache &cache, const std::string &name)
    {
        cache.AddNameEntry(name, ObfuscationNameCache::NameCacheType::CLAZZ);
    }
};

TEST(ObfuscationNameCacheSanitizeTest, ReplacesSlashInScopedOhmurlName)
{
    EXPECT_EQ(ObfuscationNameCache::SanitizeFileBaseName("@hw-hmos/animatronix.src.main.ets.api.CardArgs"),
              "@hw-hmos.animatronix.src.main.ets.api.CardArgs");
}

TEST(ObfuscationNameCacheSanitizeTest, ReplacesBackslashAndStripsEtsSuffix)
{
    EXPECT_EQ(ObfuscationNameCache::SanitizeFileBaseName("@hw-hmos\\animatronix.CardArgs.ets"),
              "@hw-hmos.animatronix.CardArgs");
}

TEST(ObfuscationNameCacheSanitizeTest, StripsDeclEtsSuffix)
{
    EXPECT_EQ(ObfuscationNameCache::SanitizeFileBaseName("entry.src.main.ets.file.d.ets"), "entry.src.main.ets.file");
}

TEST(ObfuscationNameCacheSanitizeTest, ReplacesWindowsIllegalFilenameCharacters)
{
    EXPECT_EQ(ObfuscationNameCache::SanitizeFileBaseName("a<>:\"|?*b"), "a.......b");
}

TEST(ObfuscationNameCacheSanitizeTest, LeavesSafeModuleNameUnchanged)
{
    EXPECT_EQ(ObfuscationNameCache::SanitizeFileBaseName("entry.src.main.ets.file_name"),
              "entry.src.main.ets.file_name");
}

TEST_F(ObfuscationNameCacheFileTest, WritesFlatJsonForScopedOhmurlModuleName)
{
    constexpr std::string_view MODULE_NAME = "@hw-hmos/animatronix.src.main.ets.api.CardArgs";
    constexpr std::string_view EXPECTED_FILE = "@hw-hmos.animatronix.src.main.ets.api.CardArgs.json";

    ObfuscationNameCache cache;
    cache.SetModuleName(std::string(MODULE_NAME));
    SeedClassEntry(cache, std::string(MODULE_NAME) + ".CardArgs");

    ScopedTempDirectory tempDir;
    std::string resolvedPath;
    ASSERT_TRUE(cache.GenerateJsonFile(tempDir.Path().string(), &resolvedPath));

    const std::filesystem::path jsonPath(resolvedPath);
    EXPECT_EQ(jsonPath.filename().string(), std::string(EXPECTED_FILE));
    EXPECT_TRUE(std::filesystem::equivalent(jsonPath.parent_path(), tempDir.Path()));
    ASSERT_TRUE(std::filesystem::is_regular_file(jsonPath));
    EXPECT_FALSE(std::filesystem::exists(tempDir.Path() / "@hw-hmos"));

    const std::string content = ReadEntireFile(jsonPath);
    EXPECT_NE(content.find(std::string(MODULE_NAME)), std::string::npos);
}

TEST_F(ObfuscationNameCacheFileTest, WritesFlatJsonForBackslashInModuleName)
{
    constexpr std::string_view MODULE_NAME = "@hw-hmos\\animatronix.CardArgs";
    constexpr std::string_view EXPECTED_FILE = "@hw-hmos.animatronix.CardArgs.json";

    ObfuscationNameCache cache;
    cache.SetModuleName(std::string(MODULE_NAME));
    SeedClassEntry(cache, "CardArgs");

    ScopedTempDirectory tempDir;
    std::string resolvedPath;
    ASSERT_TRUE(cache.GenerateJsonFile(tempDir.Path().string(), &resolvedPath));

    const std::filesystem::path jsonPath(resolvedPath);
    EXPECT_EQ(jsonPath.filename().string(), std::string(EXPECTED_FILE));
    ASSERT_TRUE(std::filesystem::is_regular_file(jsonPath));
    EXPECT_FALSE(std::filesystem::exists(tempDir.Path() / "@hw-hmos"));
}

}  // namespace ark::es2panda::ir
