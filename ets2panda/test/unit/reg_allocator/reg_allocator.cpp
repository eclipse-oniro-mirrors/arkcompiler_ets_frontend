/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "assembly-emitter.h"
#include "assembly-program.h"
#include "test/utils/asm_test.h"

namespace ark::es2panda::compiler::test {

class RegAllocator : public ::test::utils::AsmTest {
public:
    RegAllocator() = default;

    ~RegAllocator() override = default;

    void RunAnnotationEmitTest(const std::string_view text)
    {
        auto program = GetCurrentProgram(text);
        EXPECT_NE(program, nullptr);
        pandasm::AsmEmitter::PandaFileToPandaAsmMaps maps;
        static const std::string fileName = "reg_allocator_test";
        auto pfile = pandasm::AsmEmitter::Emit(fileName, *program, nullptr, &maps);
        EXPECT_NE(pfile, false);
    }

private:
    NO_COPY_SEMANTIC(RegAllocator);
    NO_MOVE_SEMANTIC(RegAllocator);
};

TEST_F(RegAllocator, TryCatch)
{
    std::string_view text = R"(
    function throwable(x: int, throwError: boolean): int {
        if (throwError) {
            throw new Error('err')
        }
        return x
    }
    function foo() {
        let x0 = 0
        let x1 = 1
        let x2 = 2
        let x3 = 3
        let x4 = 4
        let x5 = 5
        let x6 = 6
        let x7 = 7
        let x8 = 8
        let x9 = 9
        let x10 = 10
        let x11 = 11
        let x12 = 12
        let x13 = 13
        let x14 = 14
        let x15 = 15
        let ret: int = 16
        try { ret = throwable(x15, true) } catch (e) {}
        console.log(ret);
    }
    )";

    RunAnnotationEmitTest(text);
}

}  // namespace ark::es2panda::compiler::test
