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
#include <string>
#include <system_error>
#include <thread>

#include "util/fsQueryCache.h"

namespace {

class ScopedTempDirectory {
public:
    ScopedTempDirectory()
    {
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto threadId = std::hash<std::thread::id> {}(std::this_thread::get_id());
        path_ = std::filesystem::temp_directory_path() /
                ("fs_query_cache_" + std::to_string(threadId) + "_" + std::to_string(timestamp));
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

TEST(FsQueryCacheTest, ClearInvalidatesCachedNegativeFileLookup)
{
    ScopedTempDirectory tempDir;
    const auto sourcePath = tempDir.Path() / "created_after_first_lookup.ets";
    ark::es2panda::util::FsQueryCache cache;

    ASSERT_FALSE(cache.IsRegularFile(sourcePath.string()));

    std::ofstream(sourcePath).put('\n');
    ASSERT_TRUE(std::filesystem::is_regular_file(sourcePath));
    EXPECT_FALSE(cache.IsRegularFile(sourcePath.string()));

    cache.Clear();

    EXPECT_TRUE(cache.IsRegularFile(sourcePath.string()));
}

TEST(FsQueryCacheTest, ClearInvalidatesCachedNegativeDirectoryLookup)
{
    ScopedTempDirectory tempDir;
    const auto directoryPath = tempDir.Path() / "created_after_first_lookup";
    ark::es2panda::util::FsQueryCache cache;

    ASSERT_FALSE(cache.IsDirectory(directoryPath.string()));

    ASSERT_TRUE(std::filesystem::create_directory(directoryPath));
    EXPECT_FALSE(cache.IsDirectory(directoryPath.string()));

    cache.Clear();

    EXPECT_TRUE(cache.IsDirectory(directoryPath.string()));
}

#if defined(PANDA_TARGET_WINDOWS)
TEST(FsQueryCacheTest, CaseSensitiveLookupDoesNotReuseCaseInsensitiveResult)
{
    ScopedTempDirectory tempDir;
    const auto actualPath = tempDir.Path() / "caseSensitive.ets";
    const auto differentlyCasedPath = tempDir.Path() / "CASESENSITIVE.ets";
    ark::es2panda::util::FsQueryCache cache;

    std::ofstream(actualPath).put('\n');
    ASSERT_TRUE(std::filesystem::is_regular_file(actualPath));

    EXPECT_TRUE(cache.IsRegularFile(actualPath.string(), true));
    EXPECT_TRUE(cache.IsRegularFile(differentlyCasedPath.string()));
    EXPECT_FALSE(cache.IsRegularFile(differentlyCasedPath.string(), true));
}
#endif

}  // namespace
