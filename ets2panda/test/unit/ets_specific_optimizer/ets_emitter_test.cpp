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
#include "compiler/core/emitterExternalPass.h"
#include "es2panda.h"
#include "test/utils/asm_test.h"

namespace {

class ExternalEmitPassDependencies {
public:
    void SetTrackExternalDeps(bool val)
    {
        trackExternalDeps_ = val;
    }

    void UpdateLastToEmitSize()
    {
        lastReachableSize_ = reachableSize_;
    }

    bool MaybeRetryExternalPass() const
    {
        return reachableSize_ > lastReachableSize_;
    }

    void AddReachableDependency()
    {
        reachableSize_++;
    }

    bool IsTrackingExternalDeps() const
    {
        return trackExternalDeps_;
    }

private:
    bool trackExternalDeps_ {false};
    size_t reachableSize_ {0U};
    size_t lastReachableSize_ {0U};
};

}  // namespace

namespace ark::es2panda::compiler::test {
class ETSEmitterTest : public ::test::utils::AsmTest {};

TEST(ETSEmitterExternalPassTest, uses_one_external_emit_pass_when_no_new_dependency_is_found)
{
    ExternalEmitPassDependencies dependencies;
    size_t traversals = 0U;

    auto passes = detail::RunExternalEmitPasses(&dependencies, [&traversals]() { traversals++; });

    EXPECT_TRUE(dependencies.IsTrackingExternalDeps());
    EXPECT_EQ(passes, 1U);
    EXPECT_EQ(traversals, 1U);
}

TEST(ETSEmitterExternalPassTest, uses_second_external_emit_pass_when_new_dependency_is_found)
{
    ExternalEmitPassDependencies dependencies;
    size_t traversals = 0U;
    bool isFirstPass = true;

    auto passes = detail::RunExternalEmitPasses(&dependencies, [&dependencies, &traversals, &isFirstPass]() {
        traversals++;
        if (isFirstPass) {
            dependencies.AddReachableDependency();
            isFirstPass = false;
        }
    });

    EXPECT_TRUE(dependencies.IsTrackingExternalDeps());
    EXPECT_EQ(passes, 2U);
    EXPECT_EQ(traversals, 2U);
}

TEST(ETSEmitterExternalPassTest, caps_external_emit_passes_at_two)
{
    ExternalEmitPassDependencies dependencies;
    size_t traversals = 0U;

    auto passes = detail::RunExternalEmitPasses(&dependencies, [&dependencies, &traversals]() {
        traversals++;
        dependencies.AddReachableDependency();
    });

    EXPECT_TRUE(dependencies.IsTrackingExternalDeps());
    EXPECT_EQ(passes, 2U);
    EXPECT_EQ(traversals, 2U);
}

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
        "std.core.Object[]",
    };

    std::vector<std::string_view> emitFunctionsOnlyWithNonZeroOptLevelItems = {
        "std.core.StringBuilder.%%get-stringLength:i32;",
    };

    std::vector<std::string_view> noEmitRecordsByDefault = {
        "std.core.String",
        "std.core.String[]",
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