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

#include <cstdint>
#include <fstream>
#include <regex>
#include <gtest/gtest.h>

#include "utils.h"

using ark::es2panda::gluegen::AbsolutePath;
using ark::es2panda::gluegen::FileTimeToString;
using ark::es2panda::gluegen::GetLastModifiedTime;
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
    explicit ScopedTempDir(const std::string &prefix)
        : path_(fs::temp_directory_path() / (prefix + "_" + std::to_string(reinterpret_cast<std::uintptr_t>(this))))
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

TEST(GluegenUtilsTest, FileTimeToStringMatchesIso8601Format)
{
    auto formatted = FileTimeToString(fs::file_time_type::clock::now());
    // e.g. "2026-07-20T15:30:45.123Z"
    static const std::regex kIso8601Millis(R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}Z$)");
    EXPECT_TRUE(std::regex_match(formatted, kIso8601Millis)) << "got: " << formatted;
}

TEST(GluegenUtilsTest, GetLastModifiedTimeReturnsNulloptForMissingFile)
{
    auto missing = fs::temp_directory_path() / "gluegen_unit_test_does_not_exist.ets";
    EXPECT_EQ(GetLastModifiedTime(missing), std::nullopt);
}

TEST(GluegenUtilsTest, GetLastModifiedTimeReturnsValueForExistingFile)
{
    auto path = fs::temp_directory_path() / "gluegen_unit_test_existing_file.ets";
    {
        std::ofstream out(path);
        out << "class Existing {}";
    }

    EXPECT_NE(GetLastModifiedTime(path), std::nullopt);

    std::error_code ec;
    fs::remove(path, ec);
}

}  // namespace

#undef GLUEGEN_TESTING_EXPERIMENTAL_FILESYSTEM
