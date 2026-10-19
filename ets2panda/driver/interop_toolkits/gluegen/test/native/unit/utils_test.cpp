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

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <random>
#include <gtest/gtest.h>

#if !defined(_WIN32)
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#endif

#include "utils.h"

using ark::es2panda::gluegen::AbsolutePath;
using ark::es2panda::gluegen::GetLastModifiedTimeStamp;
using ark::es2panda::gluegen::NormalizePath;
using ark::es2panda::gluegen::RelativePath;
using ark::es2panda::gluegen::ToLongPathIfNeeded;
using ark::es2panda::gluegen::WeaklyCanonical;

#if defined(GLUEGEN_FORCE_EXPERIMENTAL_FILESYSTEM) || !__has_include(<filesystem>)
#define GLUEGEN_TESTING_EXPERIMENTAL_FILESYSTEM 1
#else
#define GLUEGEN_TESTING_EXPERIMENTAL_FILESYSTEM 0
#endif

namespace {

class ScopedTempDir {
public:
    explicit ScopedTempDir(const std::string &prefix) : path_(fs::temp_directory_path() / (prefix + "_" + Unique()))
    {
        fs::create_directories(path_);
    }

    ~ScopedTempDir()
    {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }

    const fs::path &Path() const
    {
        return path_;
    }

private:
    static std::string Unique()
    {
        static std::atomic<uint64_t> counter {0};
        std::random_device rd;
        return std::to_string(rd()) + "_" + std::to_string(rd()) + "_" +
               // Atomic with relaxed order reason: running counter, no ordering guarantee needed
               std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
    }

    fs::path path_;
};

TEST(GluegenUtilsTest, NormalizePathReturnsAbsolutePath)
{
    auto normalized = NormalizePath(std::string("some/relative/path.ets"));
    EXPECT_TRUE(fs::path(normalized).is_absolute());
}

TEST(GluegenUtilsTest, NormalizePathUsesForwardSlashesOnly)
{
    auto normalized = NormalizePath(std::string("a/b/c.ets"));
    EXPECT_EQ(normalized.find('\\'), std::string::npos);
}

TEST(GluegenUtilsTest, NormalizePathAcceptsVariousStringLikeInputs)
{
    const char *literal = "a/b.ets";
    std::string str = "a/b.ets";
    std::string_view view = str;
    fs::path path = str;

    EXPECT_EQ(NormalizePath(literal), NormalizePath(str));
    EXPECT_EQ(NormalizePath(view), NormalizePath(str));
    EXPECT_EQ(NormalizePath(path), NormalizePath(str));
}

TEST(GluegenUtilsTest, FilesystemCompatibilityHelpersHandleCornerCases)
{
    ScopedTempDir tempDir("gluegen_filesystem_compat");
    const auto existingDir = tempDir.Path() / "existing";
    const auto baseDir = tempDir.Path() / "base";
    const auto targetDir = tempDir.Path() / "target" / "nested";
    fs::create_directories(existingDir / "child");
    fs::create_directories(baseDir);
    fs::create_directories(targetDir);

    EXPECT_EQ(AbsolutePath(fs::path(".") / "existing"), fs::absolute(fs::path(".") / "existing"));

    const auto missingPath = existingDir / "missing" / "deep" / "output.json";
    const auto pathWithParentTraversal = existingDir / "missing" / ".." / "output.json";
#if GLUEGEN_TESTING_EXPERIMENTAL_FILESYSTEM
    const auto canonicalExistingDir = fs::canonical(existingDir);
    EXPECT_EQ(WeaklyCanonical(existingDir), canonicalExistingDir);
    EXPECT_EQ(WeaklyCanonical(missingPath), canonicalExistingDir / "missing" / "deep" / "output.json");
    EXPECT_EQ(WeaklyCanonical(pathWithParentTraversal), canonicalExistingDir / "output.json");
#else
    EXPECT_EQ(WeaklyCanonical(existingDir), fs::weakly_canonical(existingDir));
    EXPECT_EQ(WeaklyCanonical(missingPath), fs::weakly_canonical(missingPath));
    EXPECT_EQ(WeaklyCanonical(pathWithParentTraversal), fs::weakly_canonical(pathWithParentTraversal));
#endif

    EXPECT_EQ(RelativePath(baseDir, baseDir), fs::path("."));
    EXPECT_EQ(RelativePath(targetDir, baseDir), fs::path("..") / "target" / "nested");
    EXPECT_EQ(RelativePath(targetDir / "..", baseDir / "."), fs::path("..") / "target");

    const auto symlinkDir = tempDir.Path() / "existing-link";
    std::error_code symlinkEc;
    fs::create_directory_symlink(existingDir, symlinkDir, symlinkEc);
    if (!symlinkEc) {
        EXPECT_EQ(WeaklyCanonical(symlinkDir / "child" / "missing"), fs::canonical(existingDir) / "child" / "missing");
        EXPECT_EQ(RelativePath(symlinkDir / "child", baseDir), fs::path("..") / "existing" / "child");
    }
}

#if !defined(_WIN32)
TEST(GluegenUtilsTest, ToLongPathIfNeededIsNoOpOnNonWindows)
{
    fs::path path = "some/relative/path.ets";
    EXPECT_EQ(ToLongPathIfNeeded(path), path);
}
#endif

TEST(GluegenUtilsTest, GetLastModifiedTimeStampReturnsNulloptForMissingFile)
{
    ScopedTempDir dir("gluegen_unit_test_missing");
    auto missing = dir.Path() / "does_not_exist.ets";
    EXPECT_EQ(GetLastModifiedTimeStamp(missing), std::nullopt);
}

TEST(GluegenUtilsTest, GetLastModifiedTimeStampReturnsValueForExistingFile)
{
    ScopedTempDir dir("gluegen_unit_test_existing");
    auto path = dir.Path() / "existing_file.ets";
    {
        std::ofstream out(path);
        out << "class Existing {}";
    }

    EXPECT_NE(GetLastModifiedTimeStamp(path), std::nullopt);
}

// The stamp backs cache hit/miss decisions, so repeated queries of an unmodified file must
// return the identical string -- even when the stored mtime sits exactly on a whole-second
// (hence millisecond) boundary, the worst case for the previous wall-clock formatting, whose
// two-`now()` clock rebasing could flip the millisecond digit between calls.
TEST(GluegenUtilsTest, GetLastModifiedTimeStampIsDeterministicAcrossRepeatedCalls)
{
    ScopedTempDir dir("gluegen_unit_test_mtime_stable");
    auto path = dir.Path() / "file.ets";
    {
        std::ofstream out(path);
        out << "class Stable {}";
    }

    const auto boundary = std::chrono::time_point_cast<std::chrono::seconds>(fs::file_time_type::clock::now());
    std::error_code ec;
    fs::last_write_time(path, boundary, ec);
    ASSERT_FALSE(ec) << "failed to set mtime: " << ec.message();

    const auto stamp = GetLastModifiedTimeStamp(path);
    ASSERT_NE(stamp, std::nullopt);
    constexpr int kRepetitions = 100;
    for (int i = 0; i < kRepetitions; ++i) {
        EXPECT_EQ(GetLastModifiedTimeStamp(path), stamp);
    }
}

TEST(GluegenUtilsTest, GetLastModifiedTimeStampChangesWhenMTimeChanges)
{
    ScopedTempDir dir("gluegen_unit_test_mtime_change");
    auto path = dir.Path() / "file.ets";
    {
        std::ofstream out(path);
        out << "class Changing {}";
    }

    const auto first = std::chrono::time_point_cast<std::chrono::seconds>(fs::file_time_type::clock::now());
    std::error_code ec;
    fs::last_write_time(path, first, ec);
    ASSERT_FALSE(ec) << "failed to set mtime: " << ec.message();
    const auto stampBefore = GetLastModifiedTimeStamp(path);
    ASSERT_NE(stampBefore, std::nullopt);

    // Move the mtime well past any filesystem timestamp granularity.
    constexpr auto kMTimeStep = std::chrono::seconds(10);
    fs::last_write_time(path, first + kMTimeStep, ec);
    ASSERT_FALSE(ec) << "failed to set mtime: " << ec.message();
    const auto stampAfter = GetLastModifiedTimeStamp(path);
    ASSERT_NE(stampAfter, std::nullopt);

    EXPECT_NE(*stampBefore, *stampAfter);
}

#if !defined(_WIN32)
// The stamp must be the OS-reported mtime in Unix-epoch nanoseconds, independent of any
// C++ library clock conventions. Uses the exact mtime from the cache-invalidation bug
// report -- the SDK stdlib placeholder 2000-12-31T16:00:00.000Z (978278400 seconds) --
// which the previous wall-clock formatting kept flipping between ".000Z" and ".001Z".
TEST(GluegenUtilsTest, GetLastModifiedTimeStampUsesUnixEpochNanoseconds)
{
    ScopedTempDir dir("gluegen_unit_test_mtime_epoch");
    auto path = dir.Path() / "file.ets";
    {
        std::ofstream out(path);
        out << "class Epoch {}";
    }

    constexpr int64_t kPlaceholderSec = 978278400;  // 2000-12-31T16:00:00.000Z
    constexpr int64_t kNanosecondsPerSecond = 1000000000LL;
    struct timespec times[2];
    times[0] = {kPlaceholderSec, 0};
    times[1] = {kPlaceholderSec, 0};
    ASSERT_EQ(utimensat(AT_FDCWD, path.c_str(), times, 0), 0) << "utimensat failed: " << strerror(errno);

    const std::string expected = std::to_string(kPlaceholderSec * kNanosecondsPerSecond);
    constexpr int kRepetitions = 10;
    for (int i = 0; i < kRepetitions; ++i) {
        EXPECT_EQ(GetLastModifiedTimeStamp(path), expected);
    }
}
#endif

}  // namespace

#undef GLUEGEN_TESTING_EXPERIMENTAL_FILESYSTEM
