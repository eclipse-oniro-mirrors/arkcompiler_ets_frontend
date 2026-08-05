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

#include <fstream>
#include <regex>
#include <gtest/gtest.h>

#include "utils.h"

using ark::es2panda::gluegen::FileTimeToString;
using ark::es2panda::gluegen::GetLastModifiedTime;
using ark::es2panda::gluegen::NormalizePath;
using ark::es2panda::gluegen::ToLongPathIfNeeded;

namespace {

TEST(GluegenUtilsTest, NormalizePathReturnsAbsolutePath)
{
    auto normalized = NormalizePath(std::string("some/relative/path.ets"));
    EXPECT_TRUE(std::filesystem::path(normalized).is_absolute());
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
    std::filesystem::path path = str;

    EXPECT_EQ(NormalizePath(literal), NormalizePath(str));
    EXPECT_EQ(NormalizePath(view), NormalizePath(str));
    EXPECT_EQ(NormalizePath(path), NormalizePath(str));
}

#if !defined(_WIN32)
TEST(GluegenUtilsTest, ToLongPathIfNeededIsNoOpOnNonWindows)
{
    std::filesystem::path path = "some/relative/path.ets";
    EXPECT_EQ(ToLongPathIfNeeded(path), path);
}
#endif

TEST(GluegenUtilsTest, FileTimeToStringMatchesIso8601Format)
{
    auto formatted = FileTimeToString(std::filesystem::file_time_type::clock::now());
    // e.g. "2026-07-20T15:30:45.123Z"
    static const std::regex kIso8601Millis(R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}Z$)");
    EXPECT_TRUE(std::regex_match(formatted, kIso8601Millis)) << "got: " << formatted;
}

TEST(GluegenUtilsTest, GetLastModifiedTimeReturnsNulloptForMissingFile)
{
    auto missing = std::filesystem::temp_directory_path() / "gluegen_unit_test_does_not_exist.ets";
    EXPECT_EQ(GetLastModifiedTime(missing), std::nullopt);
}

TEST(GluegenUtilsTest, GetLastModifiedTimeReturnsValueForExistingFile)
{
    auto path = std::filesystem::temp_directory_path() / "gluegen_unit_test_existing_file.ets";
    {
        std::ofstream out(path);
        out << "class Existing {}";
    }

    EXPECT_NE(GetLastModifiedTime(path), std::nullopt);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

}  // namespace
