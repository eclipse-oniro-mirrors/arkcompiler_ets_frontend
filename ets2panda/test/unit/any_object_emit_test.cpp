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

#include <iomanip>
#include <iterator>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "assembly-function.h"
#include "assembly-program.h"

#include "test/utils/asm_test.h"

namespace ark::pandasm {

// The value printer for expects.
std::ostream &operator<<(std::ostream &s, const Function &arg)
{
    return s << std::quoted(arg.name);
}

}  // namespace ark::pandasm

namespace ark::es2panda::compiler::test {

class AnyObjectAsmTest : public ::test::utils::AsmTest {
public:
    AnyObjectAsmTest() = default;

    ~AnyObjectAsmTest() override = default;

    void CheckInsInFunction(std::string_view functionName, std::string_view insString, bool result = true)
    {
        ASSERT_NE(program_.get(), nullptr);
        pandasm::Function *fn = GetFunction(functionName, program_->functionStaticTable);
        ASSERT_NE(fn, nullptr) << "Function '" << functionName << "' not found";
        bool found = false;
        for (const auto &i : fn->ins) {
            std::string iStr = i.ToString("", true);
            if (iStr.find(insString) != std::string::npos) {
                found = true;
            }
        }
        EXPECT_EQ(found, result) << "Instruction '" << insString << "' in function '" << functionName
                                 << "' not met expectations";
    }

private:
    NO_COPY_SEMANTIC(AnyObjectAsmTest);
    NO_MOVE_SEMANTIC(AnyObjectAsmTest);
};

TEST_F(AnyObjectAsmTest, AnyInstanceOfObject)
{
    SetCurrentProgram(R"(
        function f(a: Any) {
            return a instanceof object
        }
    )");

    // previous bytecode (param is "std.core.Object"!)
    // ---------------------------
    // .function u1 test.ETSGLOBAL.f(std.core.Object a0) {
    //     movi v0, 0x0
    //	   movi v1, 0x1
    // 	   ets.movnullvalue v2
    //	   lda.obj v2
    //	   jeq.obj a0, jump_label_0
    //	   lda.obj a0
    //	   jeqz.obj jump_label_0
    //	   mov v0, v1
    // jump_label_0:
    //	   lda v0
    //	   return
    // }
    // ----------------------------
    // current bytecode (param is "Y"!)
    // ----------------------------
    // .function u1 test.ETSGLOBAL.f(Y a0) {
    //     movi v0, 0x0
    //     movi v1, 0x1
    //     ets.movnullvalue v2
    //     lda.obj v2
    //     jeq.obj a0, jump_label_0
    //     lda.obj a0
    //     isinstance std.core.Object
    //     jeqz jump_label_0
    //     mov v0, v1
    // jump_label_0:
    //     lda v0
    //     return
    // }
    // ----------------------------

    CheckInsInFunction("dummy.ETSGLOBAL.f:Y;u1;", "isinstance std.core.Object", true);
}

TEST_F(AnyObjectAsmTest, ObjectNullInstanceOfObject)
{
    SetCurrentProgram(R"(
        function f(a: object | null) {
            return a instanceof object
        }
    )");

    // previous bytecode
    // ---------------------------
    // .function u1 test.ETSGLOBAL.f({Ustd.core.Null,std.core.Object} a0) {
    //     movi v0, 0x0
    //     movi v1, 0x1
    //     ets.movnullvalue v2
    //     lda.obj v2
    //     jeq.obj a0, jump_label_0
    //     lda.obj a0
    //     jeqz.obj jump_label_0
    //     mov v0, v1
    // jump_label_0:
    //     lda v0
    //     return
    // }
    // ----------------------------
    // current bytecode
    // ----------------------------
    // .function u1 test.ETSGLOBAL.f({Ustd.core.Null,std.core.Object} a0) {
    //     movi v0, 0x0
    //     movi v1, 0x1
    //     ets.movnullvalue v2
    //     lda.obj v2
    //     jeq.obj a0, jump_label_0
    //     lda.obj a0
    //     isinstance std.core.Object
    //     jeqz jump_label_0
    //     mov v0, v1
    // jump_label_0:
    //     lda v0
    //     return
    // }
    // ----------------------------

    CheckInsInFunction("dummy.ETSGLOBAL.f:{Ustd.core.Null,std.core.Object};u1;", "isinstance std.core.Object", true);
}
TEST_F(AnyObjectAsmTest, ObjectUndefinedInstanceOfObject)
{
    SetCurrentProgram(R"(
        function f(a: object | undefined) {
            return a instanceof object
        }
    )");

    // previous bytecode
    // ---------------------------
    // .function u1 test.ETSGLOBAL.f(std.core.Object a0) {
    //     movi v0, 0x0
    //     movi v1, 0x1
    //     ets.movnullvalue v2
    //     lda.obj v2
    //     jeq.obj a0, jump_label_0
    //     lda.obj a0
    //     jeqz.obj jump_label_0
    //     mov v0, v1
    // jump_label_0:
    //     lda v0
    //     return
    // }
    // ----------------------------
    // current bytecode
    // ----------------------------
    // .function u1 test.ETSGLOBAL.f(std.core.Object a0) {
    //    lda.obj a0
    //    isinstance std.core.Object
    //    return
    // }
    // ----------------------------

    CheckInsInFunction("dummy.ETSGLOBAL.f:std.core.Object;u1;", "isinstance std.core.Object", true);
}

TEST_F(AnyObjectAsmTest, AnyInstanceOfObjectUndefined)
{
    SetCurrentProgram(R"(
        function f(a: Any) {
            return a instanceof object | undefined
        }
    )");

    // previous bytecode (param is "std.core.Object"!)
    // ---------------------------
    // .function u1 test.ETSGLOBAL.f(std.core.Object a0) {
    //     movi v0, 0x1
    //     movi v1, 0x0
    //     lda.obj a0
    //     jeqz.obj jump_label_0
    //     ets.movnullvalue v2
    //     lda.obj v2
    //     jeq.obj a0, jump_label_1
    //     mov v1, v0
    //     jmp jump_label_1
    // jump_label_0:
    //     mov v1, v0
    // jump_label_1:
    //     lda v1
    //     return
    // }
    // ----------------------------
    // current bytecode (param is "Y"!)
    // ----------------------------
    // .function u1 test.ETSGLOBAL.f(Y a0) {
    //     movi v0, 0x1
    //     movi v1, 0x0
    //     lda.obj a0
    //     jeqz.obj jump_label_0
    //     ets.movnullvalue v2
    //     lda.obj v2
    //     jeq.obj a0, jump_label_1
    //     lda.obj a0
    //     isinstance std.core.Object
    //     jeqz jump_label_1
    //     mov v1, v0
    //     jmp jump_label_1
    // jump_label_0:
    //     mov v1, v0
    // jump_label_1:
    //     lda v1
    //     return
    // }
    // ----------------------------

    CheckInsInFunction("dummy.ETSGLOBAL.f:Y;u1;", "isinstance std.core.Object", true);
}

}  // namespace ark::es2panda::compiler::test
