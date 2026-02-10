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
#include <iomanip>
#include <ostream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "assembler/assembly-program.h"
#include "assembly-function.h"
#include "es2panda.h"
#include "test/utils/asm_test.h"

namespace ark::es2panda::compiler::test {
class ETSEmitterTest : public ::test::utils::AsmTest {};

TEST_F(ETSEmitterTest, check_emitted_items_with_different_opt_level)
{
    using ::testing::Contains, ::testing::Key, ::testing::Eq, ::testing::Not;

    std::string_view programSource = R"(
        function main() {
            return 0;
        }
    )";

    std::vector<std::string_view> emitRecordsOnlyWithNonZeroOptLevelItems = {
        "std.core.StringBuilder",
    };

    std::vector<std::string_view> emitFunctionsOnlyWithNonZeroOptLevelItems = {
        "std.core.StringBuilder.%%get-stringLength:i32;",
    };

    std::vector<std::string_view> noEmitRecordsByDefault = {
        "std.core.String",
        "std.core.String[]",
        "std.core.Object[]",
    };

    std::array args = {
        ES2PANDA_BIN_PATH,
        "--opt-level=0",
    };

    auto program = GetCurrentProgramWithArgs({args.data(), args.size()}, programSource);
    ASSERT_NE(program, nullptr);

    for (auto item : emitFunctionsOnlyWithNonZeroOptLevelItems) {
        EXPECT_THAT(program->functionInstanceTable, Not(Contains(Key(Eq(item)))));
    }

    for (auto item : emitRecordsOnlyWithNonZeroOptLevelItems) {
        EXPECT_THAT(program->recordTable, Not(Contains(Key(Eq(item)))));
    }

    for (auto item : noEmitRecordsByDefault) {
        EXPECT_THAT(program->recordTable, Not(Contains(Key(Eq(item)))));
    }

    args = {
        ES2PANDA_BIN_PATH,
        "--opt-level=2",
    };

    program = GetCurrentProgramWithArgs({args.data(), args.size()}, programSource);
    ASSERT_NE(program, nullptr);

    for (auto item : emitFunctionsOnlyWithNonZeroOptLevelItems) {
        EXPECT_THAT(program->functionInstanceTable, Contains(Key(Eq(item))));
    }

    for (auto item : emitRecordsOnlyWithNonZeroOptLevelItems) {
        EXPECT_THAT(program->recordTable, Contains(Key(Eq(item))));
    }

    for (auto item : noEmitRecordsByDefault) {
        EXPECT_THAT(program->recordTable, Not(Contains(Key(Eq(item)))));
    }
}

}  // namespace ark::es2panda::compiler::test