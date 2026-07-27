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

#include <string_view>

#include "gtest/gtest.h"
#include "test/utils/asm_test.h"

// Counts pandasm records whose name matches the lambda-object class pattern
// "%%lambda-lambda_invoke-<N>" (see name_mangling_disasm_test.cpp::lambdaNameGen).
static int CountLambdaObjectClasses(std::unique_ptr<ark::pandasm::Program> const &program)
{
    int n = 0;
    for (auto const &entry : program->recordTable) {
        if (entry.first.find("%%lambda-lambda_invoke-") != std::string::npos) {
            ++n;
        }
    }
    return n;
}

class LambdaLoweringTest : public ::test::utils::AsmTest {};

// AC-1.1: untyped top-level lambda (path B Clone) must produce exactly ONE lambda class.
// Before the fix this is 2: the field initializer clone is lowered (dead) alongside the
// cctor assignment clone. See obscure/OH7.x_lambda_duplicate_class_analysis.md.
TEST_F(LambdaLoweringTest, singleLambdaClassForInferenceOnlyInitializer)
{
    std::string_view src = R"(
        let f = (x: int): int => x + 1
    )";
    SetCurrentProgram(src);
    EXPECT_EQ(CountLambdaObjectClasses(program_), 1);
}

// AC-1.2: typed top-level lambda (path A Move) must remain ONE (regression guard, not RED).
TEST_F(LambdaLoweringTest, typedLambdaStillSingleClass)
{
    std::string_view src = R"(
        let f: (x: int) => int = (x: int): int => x + 1
    )";
    SetCurrentProgram(src);
    EXPECT_EQ(CountLambdaObjectClasses(program_), 1);
}

// AC-1.3: const top-level lambda (path B const sub-branch) must produce exactly ONE.
TEST_F(LambdaLoweringTest, constLambdaProducesSingleClass)
{
    std::string_view src = R"(
        const g = (x: int): int => x + 1
    )";
    SetCurrentProgram(src);
    EXPECT_EQ(CountLambdaObjectClasses(program_), 1);
}

// Gap (typed + const): a top-level const lambda WITH a type annotation is still routed to
// path B (Clone) by InitTopLevelProperty (:214 condition `!IsConst()`), so the field
// initializer copy is lowered dead alongside the cctor copy. The dedup guard previously
// required `TypeAnnotation()==nullptr` and missed this case. Must produce exactly ONE.
TEST_F(LambdaLoweringTest, typedConstLambdaProducesSingleClass)
{
    std::string_view src = R"(
        const f: (x: int) => int = (x: int): int => x + 1
    )";
    SetCurrentProgram(src);
    EXPECT_EQ(CountLambdaObjectClasses(program_), 1);
}

// AC-1.4 (wrapped, array): an untyped top-level lambda nested inside an array initializer is
// still cloned (path B) into the static ctor, while the field copy is inference-only. Before
// the pre-pass detach both copies lowered and produced TWO classes; must be exactly ONE.
TEST_F(LambdaLoweringTest, arrayWrappedLambdaProducesSingleClass)
{
    std::string_view src = R"(
        let f = [(x: int): int => x + 1]
    )";
    SetCurrentProgram(src);
    EXPECT_EQ(CountLambdaObjectClasses(program_), 1);
}

// AC-1.5 (wrapped, cast): a surviving `as` cast wrapping the lambda. Same duplication path as
// the array case; must produce exactly ONE.
TEST_F(LambdaLoweringTest, castWrappedLambdaProducesSingleClass)
{
    std::string_view src = R"(
        let f = ((x: int): int => x + 1) as (x: int) => int
    )";
    SetCurrentProgram(src);
    EXPECT_EQ(CountLambdaObjectClasses(program_), 1);
}

// AC-1.6 (wrapped, multiple lambdas): two distinct lambdas inside one wrapped initializer. The
// pre-pass detaches the field copy once per property, so each distinct lambda lowers exactly
// once in the ctor clone: TWO classes (one per lambda), never the duplicated FOUR.
TEST_F(LambdaLoweringTest, arrayWrappedMultipleLambdasProduceOneEach)
{
    std::string_view src = R"(
        let f = [(x: int): int => x + 1, (x: int): int => x * 2]
    )";
    int lambCnt = 2;
    SetCurrentProgram(src);
    EXPECT_EQ(CountLambdaObjectClasses(program_), lambCnt);
}

// AC-1.7 (wrapped, nonconstant conditional): a top-level conditional whose guard is a mutable
// `let` (not foldable) keeps both branch lambdas, so path B clones the whole conditional. Before
// the pre-pass each branch lowered twice (FOUR classes); both must now lower exactly once (TWO).
// A literal `true ? a : b` is constant-folded to the consequent and would not exercise this path.
TEST_F(LambdaLoweringTest, conditionalWrappedLambdasProduceOneEach)
{
    std::string_view src = R"(
        let g = true
        let f = g ? ((x: int): int => x + 1) : ((x: int): int => x * 2)
    )";
    int lambCnt = 2;
    SetCurrentProgram(src);
    EXPECT_EQ(CountLambdaObjectClasses(program_), lambCnt);
}
