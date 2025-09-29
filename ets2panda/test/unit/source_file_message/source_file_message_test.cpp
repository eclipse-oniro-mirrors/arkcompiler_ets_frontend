/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
#include <string>
#include <utility>
#include <vector>
#include "assembly-program.h"
#include "test/utils/asm_test.h"

namespace ark::es2panda::compiler::test {
class SourceFileMsgTest : public ::test::utils::AsmTest {
public:
    SourceFileMsgTest() = default;

    ~SourceFileMsgTest() override = default;

private:
    NO_COPY_SEMANTIC(SourceFileMsgTest);
    NO_MOVE_SEMANTIC(SourceFileMsgTest);
};

TEST_F(SourceFileMsgTest, source_file_msg_test)
{
    std::string_view text = R"(
    class A {}
    )";

    auto program = GetCurrentProgram(text);
    const auto &recordTable = program->recordTable;
    auto sourceFile = recordTable.find("ETSGLOBAL");
    ASSERT_EQ(sourceFile->second.sourceFile, "dummy.ets");
}

}  // namespace ark::es2panda::compiler::test
