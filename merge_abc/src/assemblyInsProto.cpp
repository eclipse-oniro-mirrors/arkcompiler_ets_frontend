/**
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

#include "assemblyInsProto.h"

namespace panda::proto {
void Ins::Serialize(const panda::pandasm::InsPtr &insn, protoPanda::Ins &protoInsn)
{
    protoInsn.set_opcode(static_cast<uint32_t>(insn->GetOpcode()));
    for (const auto &reg : insn->Regs()) {
        protoInsn.add_regs(static_cast<uint32_t>(reg));
    }
    for (const auto &str : insn->Ids()) {
        protoInsn.add_ids(str);
    }
    for (const auto &imm : insn->Imms()) {
        auto *protoImm = protoInsn.add_imms();
        switch (static_cast<protoPanda::Ins_IType::TypeCase>(imm.index() + 1)) {  // 1: enum TypeCase start from 1
            case protoPanda::Ins_IType::kValueInt:
                protoImm->set_valueint(std::get<int64_t>(imm));
                break;
            case protoPanda::Ins_IType::kValueDouble:
                protoImm->set_valuedouble(std::get<double>(imm));
                break;
            default:
                UNREACHABLE();
        }
    }
    protoInsn.set_label(insn->IsLabel() ? insn->Label() : "");
    protoInsn.set_setlabelval(insn->IsLabel());
    auto *protoDebug = protoInsn.mutable_insdebug();
    DebuginfoIns::Serialize(insn->ins_debug, *protoDebug);
}

void Ins::Deserialize(const protoPanda::Ins &protoInsn, panda::pandasm::Ins *&insn)
{
    auto opcode = static_cast<panda::pandasm::Opcode>(protoInsn.opcode());
    std::vector<uint16_t> regs;
    regs.reserve(protoInsn.regs_size());
    for (const auto &protoReg : protoInsn.regs()) {
        regs.push_back(static_cast<uint16_t>(protoReg));
    }
    std::vector<std::string> ids;
    ids.reserve(protoInsn.ids_size());
    for (const auto &protoId : protoInsn.ids()) {
        ids.push_back(protoId);
    }
    std::vector<panda::pandasm::IType> imms;
    imms.reserve(protoInsn.imms_size());
    for (const auto &protoImm : protoInsn.imms()) {
        switch (protoImm.type_case()) {
            case protoPanda::Ins_IType::kValueInt: {
                imms.push_back(protoImm.valueint());
                break;
            }
            case protoPanda::Ins_IType::kValueDouble: {
                imms.push_back(protoImm.valuedouble());
                break;
            }
            default:
                UNREACHABLE();
        }
    }

    auto set_label = protoInsn.setlabelval();
    if (set_label) {
        auto label = protoInsn.label();
        insn = new pandasm::LabelIns(label);
    } else {
        insn = pandasm::Ins::CreateIns(opcode, regs, imms, ids);
    }

    const protoPanda::DebuginfoIns &protoDebugInfoIns = protoInsn.insdebug();
    DebuginfoIns::Deserialize(protoDebugInfoIns, insn->ins_debug);
}
} // panda::proto
