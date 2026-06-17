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
#include "test/utils/asm_test.h"

#include <filesystem>

namespace test::unit {

static bool IsUserDefined(std::string_view name)
{
    return name.find("std.") == std::string_view::npos && name.find("std:") == std::string_view::npos &&
           name.find("arkruntime") == std::string_view::npos;
}

class SourceFilePathSeparatorTest : public test::utils::AsmTest {
protected:
    bool HasOnlyForwardSlashes(const std::string &path)
    {
        return path.find('\\') == std::string::npos;
    }

    void CheckAllFunctionsSourceFile(const std::unique_ptr<ark::pandasm::Program> &program)
    {
        for (const auto &[funcName, func] : program->functionInstanceTable) {
            EXPECT_TRUE(HasOnlyForwardSlashes(func.sourceFile))
                << "Instance function '" << funcName << "' has sourceFile: '" << func.sourceFile
                << "' which contains backslashes";
        }

        for (const auto &[funcName, func] : program->functionStaticTable) {
            EXPECT_TRUE(HasOnlyForwardSlashes(func.sourceFile))
                << "Static function '" << funcName << "' has sourceFile: '" << func.sourceFile
                << "' which contains backslashes";
        }
    }

    std::string CreateTempFile(std::string_view relativePath, std::string_view content)
    {
        std::filesystem::path filePath(testing::TempDir());
        filePath.append("sourcefile_test").append(relativePath);
        std::filesystem::create_directories(filePath.parent_path());
        std::ofstream ofs(filePath);
        ofs << content;
        ofs.close();
        return filePath.string();
    }

    std::string CreateFileInCwd(std::string_view relativePath, std::string_view content)
    {
        std::filesystem::path filePath(std::filesystem::current_path());
        filePath.append("sourcefile_test_cwd").append(relativePath);
        std::filesystem::create_directories(filePath.parent_path());
        std::ofstream ofs(filePath);
        ofs << content;
        ofs.close();
        return filePath.string();
    }
};

TEST_F(SourceFilePathSeparatorTest, CheckFunctionSourceFilePathSeparatorsWithBackslash)
{
    std::string src = R"(
        function main() {
            return 0;
        }
    )";

    const char *args[] = {ES2PANDA_BIN_PATH, "--ets-unnamed"};
    auto program = GetProgram(ark::Span<const char *const>(args, 2), "path\\to\\test.ets", src);
    ASSERT_NE(program, nullptr);

    CheckAllFunctionsSourceFile(program);
}

TEST_F(SourceFilePathSeparatorTest, CheckClassRecordSourceFilePathSeparatorsWithBackslash)
{
    std::string src = R"(
        class MyClass {
            myMethod(): void {
            }
        }
    )";

    const char *args[] = {ES2PANDA_BIN_PATH, "--ets-unnamed"};
    auto program = GetProgram(ark::Span<const char *const>(args, 2), "src\\classes\\MyClass.ets", src);
    ASSERT_NE(program, nullptr);

    for (const auto &[recordName, record] : program->recordTable) {
        EXPECT_TRUE(HasOnlyForwardSlashes(record.sourceFile))
            << "Record '" << recordName << "' has sourceFile: '" << record.sourceFile << "' which contains backslashes";
    }

    CheckAllFunctionsSourceFile(program);
}

TEST_F(SourceFilePathSeparatorTest, CheckInterfaceRecordSourceFilePathSeparatorsWithBackslash)
{
    std::string src = R"(
        interface MyInterface {
            myMethod(): void;
        }
    )";

    const char *args[] = {ES2PANDA_BIN_PATH, "--ets-unnamed"};
    auto program = GetProgram(ark::Span<const char *const>(args, 2), "src\\interfaces\\MyInterface.ets", src);
    ASSERT_NE(program, nullptr);

    for (const auto &[recordName, record] : program->recordTable) {
        EXPECT_TRUE(HasOnlyForwardSlashes(record.sourceFile))
            << "Record '" << recordName << "' has sourceFile: '" << record.sourceFile << "' which contains backslashes";
    }

    CheckAllFunctionsSourceFile(program);
}

TEST_F(SourceFilePathSeparatorTest, CheckAnnotationRecordSourceFilePathSeparatorsWithBackslash)
{
    std::string src = R"(
        @interface MyAnnotation {
        }
    )";

    const char *args[] = {ES2PANDA_BIN_PATH, "--ets-unnamed"};
    auto program = GetProgram(ark::Span<const char *const>(args, 2), "src\\annotations\\MyAnnotation.ets", src);
    ASSERT_NE(program, nullptr);

    for (const auto &[recordName, record] : program->recordTable) {
        EXPECT_TRUE(HasOnlyForwardSlashes(record.sourceFile))
            << "Record '" << recordName << "' has sourceFile: '" << record.sourceFile << "' which contains backslashes";
    }

    CheckAllFunctionsSourceFile(program);
}

TEST_F(SourceFilePathSeparatorTest, FunctionSourceFileContainsRelativePathUnderRootDir)
{
    std::string src = R"(
        function main() {
            return 0;
        }
    )";
    std::string realPath = CreateFileInCwd("src/module/Test.ets", src);

    const char *args[] = {ES2PANDA_BIN_PATH};
    auto program = GetProgram(ark::Span<const char *const>(args, 1), realPath.c_str(), src);
    ASSERT_NE(program, nullptr);

    for (const auto &[funcName, func] : program->functionStaticTable) {
        if (!IsUserDefined(funcName)) {
            continue;
        }
        EXPECT_NE(func.sourceFile, "Test.ets")
            << "Function '" << funcName << "' sourceFile should be relative path, not just filename";
        EXPECT_TRUE(func.sourceFile.find('/') != std::string::npos)
            << "Function '" << funcName << "' sourceFile should contain path separators: '" << func.sourceFile << "'";
        EXPECT_NE(func.sourceFile.front(), '/')
            << "Function '" << funcName << "' sourceFile should not be absolute: '" << func.sourceFile << "'";
        EXPECT_TRUE(HasOnlyForwardSlashes(func.sourceFile))
            << "Function '" << funcName << "' sourceFile contains backslashes: '" << func.sourceFile << "'";
    }
    std::filesystem::path cleanupPath(std::filesystem::current_path());
    cleanupPath.append("sourcefile_test_cwd");
    std::filesystem::remove_all(cleanupPath);
}

TEST_F(SourceFilePathSeparatorTest, SourceFileFallbackToFilenameOutsideRootDir)
{
    std::string src = R"(
        function main() {
            return 0;
        }
    )";
    std::string realPath = CreateTempFile("src/module/Test.ets", src);

    const char *args[] = {ES2PANDA_BIN_PATH};
    auto program = GetProgram(ark::Span<const char *const>(args, 1), realPath.c_str(), src);
    ASSERT_NE(program, nullptr);

    for (const auto &[funcName, func] : program->functionStaticTable) {
        if (!IsUserDefined(funcName)) {
            continue;
        }
        EXPECT_EQ(func.sourceFile, "Test.ets")
            << "Function '" << funcName << "' sourceFile should be just filename when outside rootDir: '"
            << func.sourceFile << "'";
    }
}

TEST_F(SourceFilePathSeparatorTest, DeclarationEtsFileUsesRelativePathUnderRootDir)
{
    std::string src = R"(
        declare class DeclaredApi {
            value: int
        }
    )";
    std::string realPath = CreateFileInCwd("src/module/Test.d.ets", src);

    const char *args[] = {ES2PANDA_BIN_PATH};
    auto program = GetProgram(ark::Span<const char *const>(args, 1), realPath.c_str(), src);
    ASSERT_NE(program, nullptr);

    bool checkedUserRecord = false;
    for (const auto &[recordName, record] : program->recordTable) {
        if (!IsUserDefined(recordName)) {
            continue;
        }
        checkedUserRecord = true;
        EXPECT_NE(record.sourceFile, "Test.d.ets")
            << "Record '" << recordName << "' sourceFile should be relative path, not just filename";
        EXPECT_TRUE(record.sourceFile.find('/') != std::string::npos)
            << "Record '" << recordName << "' sourceFile should contain path separators: '" << record.sourceFile << "'";
        EXPECT_NE(record.sourceFile.front(), '/')
            << "Record '" << recordName << "' sourceFile should not be absolute: '" << record.sourceFile << "'";
        EXPECT_TRUE(HasOnlyForwardSlashes(record.sourceFile))
            << "Record '" << recordName << "' sourceFile contains backslashes: '" << record.sourceFile << "'";
    }

    EXPECT_TRUE(checkedUserRecord) << "Expected at least one user-defined record in .d.ets output";

    std::filesystem::path cleanupPath(std::filesystem::current_path());
    cleanupPath.append("sourcefile_test_cwd");
    std::filesystem::remove_all(cleanupPath);
}

// Validates that the cached relative file path (GetCachedRelativeFilePath) produces a
// consistent sourceFile across all functions in a single Program.  A file with many
// functions exercises the per-function GenSourceFileDebugInfo call path — before the
// cache was added TryRelativeFilePathViaArkTsPaths scanned the entire ArkTSConfig
// Paths() table once per function.
TEST_F(SourceFilePathSeparatorTest, ManyFunctionsProduceConsistentSourceFile)
{
    std::string src = R"(
        function alpha(): int { return 1 }
        function beta(): double { return 3.14 }
        function gamma(): void {}

        class Calculator {
            x: int = 0
            add(): int { return this.x + 1 }
            sub(): int { return this.x - 1 }
        }

        class Multiplier {
            factor: int = 2
            apply(v: int): int { return v * this.factor }
        }
    )";

    std::string realPath = CreateFileInCwd("src/module/MultiFunc.ets", src);
    const char *args[] = {ES2PANDA_BIN_PATH};
    auto program = GetProgram(ark::Span<const char *const>(args, 1), realPath.c_str(), src);
    ASSERT_NE(program, nullptr);

    constexpr size_t kMinUserFuncCount = 4;
    std::string firstSourceFile;
    size_t userFuncCount = 0;

    auto checkTable = [this, &firstSourceFile, &userFuncCount](const auto &table) {
        for (const auto &[funcName, func] : table) {
            if (!IsUserDefined(funcName)) {
                continue;
            }
            if (firstSourceFile.empty()) {
                firstSourceFile = func.sourceFile;
            }
            EXPECT_EQ(func.sourceFile, firstSourceFile)
                << "Function '" << funcName << "' sourceFile differs — cache inconsistency";
            EXPECT_TRUE(HasOnlyForwardSlashes(func.sourceFile));
            userFuncCount++;
        }
    };

    checkTable(program->functionStaticTable);
    checkTable(program->functionInstanceTable);

    EXPECT_GE(userFuncCount, kMinUserFuncCount)
        << "Expected at least " << kMinUserFuncCount << " user-defined functions to exercise the cache";
    EXPECT_TRUE(firstSourceFile.find('/') != std::string::npos)
        << "sourceFile should be a relative path: '" << firstSourceFile << "'";
    EXPECT_NE(firstSourceFile.front(), '/') << "sourceFile should not be absolute: '" << firstSourceFile << "'";

    std::filesystem::path cleanupPath(std::filesystem::current_path());
    cleanupPath.append("sourcefile_test_cwd");
    std::filesystem::remove_all(cleanupPath);
}

}  // namespace test::unit
