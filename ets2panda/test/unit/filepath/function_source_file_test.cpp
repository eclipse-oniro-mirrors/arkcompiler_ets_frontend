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

namespace test::unit {

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

}  // namespace test::unit
