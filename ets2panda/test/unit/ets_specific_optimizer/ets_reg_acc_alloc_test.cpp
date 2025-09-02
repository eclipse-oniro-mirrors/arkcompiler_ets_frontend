/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "ets_opt.h"
#include "bytecode_optimizer/reg_acc_alloc.h"
#include "bytecode_optimizer/optimize_bytecode.h"

namespace ark::bytecodeopt::test {

// NOLINTBEGIN(readability-magic-numbers)

class RegAccAllocTest : public EtsOptTest {
public:
    void CheckInstructionsDestRegIsAcc(std::vector<int> &&instIds)
    {
        for (auto id : instIds) {
            ASSERT_EQ(INS(id).GetDstReg(), compiler::ACC_REG_ID);
        }
    }

    void CheckInstructionsSrcRegIsAcc(std::vector<int> &&instIds)
    {
        for (auto id : instIds) {
            uint8_t idx = 0;
            switch (INS(id).GetOpcode()) {
                case compiler::Opcode::LoadArray:
                case compiler::Opcode::StoreObject:
                case compiler::Opcode::StoreStatic:
                    idx = 1;
                    break;
                case compiler::Opcode::StoreArray:
                    idx = 2U;
                    break;
                default:
                    break;
            }

            ASSERT_EQ(INS(id).GetSrcReg(idx), compiler::ACC_REG_ID);
        }
    }
};

TEST_F(RegAccAllocTest, Ets_Ldobj)
{
    pandasm::Parser p;
    auto source = std::string(R"(
        .language eTS
        .record %%union_prop-std_core_String {
            std.core.String status
        }
        .record ETSGLOBAL <ets.abstract, ets.extends=std.core.Object, access.record=public> {
        }
        .record std.core.Object <external>
        .record std.core.String <external>

        .record std.time.ETSGLOBAL <external>
        .function void ETSGLOBAL.foo(std.core.Object[] a0) <static, access.function=public> {
            mov.obj v0, a0
            lda.obj v0
            sta.obj v0
            ldai 0x1
            ldarr.obj v0
            sta.obj v0
            ets.ldobj.name.obj v0, %%union_prop-std_core_String.status
            return.void
        }
        )");

    auto res = p.Parse(source);
    auto &program = res.Value();
    pandasm::AsmEmitter::PandaFileToPandaAsmMaps maps;
    std::string fileName = "Ets_Ldobj";
    auto pfile = pandasm::AsmEmitter::Emit(fileName, program, nullptr, &maps);
    ASSERT_NE(pfile, false);

    auto oldOptions = ark::bytecodeopt::g_options;
    ark::bytecodeopt::g_options = ark::bytecodeopt::Options("--opt-level=2");
    EXPECT_TRUE(OptimizeBytecode(&program, &maps, fileName, false, true));
    ark::bytecodeopt::g_options = oldOptions;
    for (const auto &inst : program.functionStaticTable.find("ETSGLOBAL.foo:(std.core.Object[])")->second.ins) {
        if (inst.opcode == ark::pandasm::Opcode::ETS_LDOBJ_NAME_OBJ) {
            ASSERT_FALSE(inst.HasFlag(pandasm::InstFlags::ACC_READ));
        }
    }
}

// NOLINTEND(readability-magic-numbers)

}  // namespace ark::bytecodeopt::test
