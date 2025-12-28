/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "regAllocator.h"

#include "compiler/core/codeGen.h"
#include "checker/types/type.h"
#include "ir/irnode.h"

#include <algorithm>
#include <vector>

namespace ark::es2panda::compiler {
// AllocatorBase

AllocatorBase::AllocatorBase(CodeGen *const cg) noexcept : cg_(cg) {}

void AllocatorBase::PushBack(IRNode *const ins) const
{
    cg_->Insns().push_back(ins);
}

CodeGen &AllocatorBase::GetCodeGen() noexcept
{
    return *cg_;
}

const CodeGen &AllocatorBase::GetCodeGen() const noexcept
{
    return *cg_;
}

SArenaAllocator &AllocatorBase::Allocator() noexcept
{
    return *cg_->Allocator();
}

const SArenaAllocator &AllocatorBase::Allocator() const noexcept
{
    return *cg_->Allocator();
}

// SimpleAllocator

SimpleAllocator::SimpleAllocator(CodeGen *const cg) noexcept : AllocatorBase(cg) {}

Label *SimpleAllocator::AllocLabel(std::string &&id)
{
    const auto *lastInsNode =
        GetCodeGen().Insns().empty() ? FIRST_NODE_OF_FUNCTION : GetCodeGen().Insns().back()->Node();
    return Alloc<Label>(lastInsNode, std::move(id));
}

void SimpleAllocator::AddLabel(Label *const label) const
{
    PushBack(label);
}

// RegAllocatorBase

RegAllocatorBase::RegAllocatorBase(CodeGen *const cg, RegSpiller *const spiller) noexcept
    : AllocatorBase(cg), spiller_(spiller)
{
}

RegSpiller &RegAllocatorBase::Spiller() noexcept
{
    return *spiller_;
}

const RegSpiller &RegAllocatorBase::Spiller() const noexcept
{
    return *spiller_;
}

VReg RegAllocatorBase::Spill(IRNode *const ins, const VReg reg) const
{
    const auto [spill_info, origin_type] = spiller_->New();

    if (origin_type != nullptr) {
        if (auto *const mov = spiller_->MoveReg(ins->Node(), spill_info.SpillReg(), spill_info.OriginReg(), true);
            mov != nullptr) {
            PushBack(mov);
        }
    }

    if (auto *const mov = spiller_->MoveReg(ins->Node(), spill_info.OriginReg(), reg, false); mov != nullptr) {
        PushBack(mov);
        spiller_->GetCodeGen()->SetVRegType(spill_info.OriginReg(), origin_type);
    }

    return spill_info.OriginReg();
}

void RegAllocatorBase::Restore(const IRNode *const ins) const
{
    const auto spillInfo = spiller_->Restore();
    if (spiller_->GetCodeGen()->GetVRegType(spillInfo.OriginReg()) == nullptr) {
        return;
    }

    if (auto *const mov = spiller_->MoveReg(ins->Node(), spillInfo.OriginReg(), spillInfo.SpillReg(), false);
        mov != nullptr) {
        PushBack(mov);
    }
}

// RegAllocator

RegAllocator::RegAllocator(CodeGen *const cg, RegSpiller *const spiller) noexcept : RegAllocatorBase(cg, spiller) {}

static bool IsInsAllRegsValid(IRNode *ins, const Span<VReg *> &registers, uint32_t regsNum)
{
    const auto limit = ins->GetRegLimit();
    return std::all_of(registers.begin(), registers.end(),
                       [limit, regsNum](const VReg *reg) { return reg->IsRegOrParamValid(limit, regsNum); });
}

void RegAllocator::Run(IRNode *const ins, uint32_t realRegCount)
{
    ES2PANDA_ASSERT(Spiller().Restored());
    ES2PANDA_ASSERT(ins != nullptr);

    std::array<VReg *, IRNode::MAX_REG_OPERAND> regs {};
    ES2PANDA_ASSERT(ins != nullptr);
    const auto regCnt = ins->Registers(&regs);

    auto realRegCnt = std::min(realRegCount, static_cast<uint32_t>(regCnt));
    if (realRegCnt > 0) {
        const auto registers = Span<VReg *>(regs.data(), regs.data() + realRegCnt);
        Spiller().UpdateSpillRegCount(realRegCnt);

        if (!Spiller().HasSpill() && !IsInsAllRegsValid(ins, registers, GetCodeGen().GetRegsNum())) {
            Spiller().SetHasSpill();
        }
    }

    ins->SetRealRegCount(realRegCnt);
    PushBack(ins);
}

bool RegAllocator::CheckFinalInsNeedSpill()
{
    const auto &insns = GetCodeGen().GetInsns();
    return std::all_of(insns.begin(), insns.end(), [this](IRNode *ins) {
        uint32_t checkRegCnt = 0;
        if (!ins->IsRangeInst()) {
            checkRegCnt = ins->GetRealRegCount();
        } else {
            std::array<VReg *, IRNode::MAX_REG_OPERAND> regs {};
            checkRegCnt = ins->Registers(&regs);
        }
        if (checkRegCnt == 0) {
            return true;
        }
        std::array<VReg *, IRNode::MAX_REG_OPERAND> regs {};
        ins->Registers(&regs);
        const auto registers = Span<VReg *>(regs.data(), regs.data() + checkRegCnt);
        return IsInsAllRegsValid(ins, registers, GetCodeGen().TotalRegsNum());
    });
}

void RegAllocator::AdjustInsRegWhenHasSpill()
{
    const auto spillRegCount = Spiller().GetSpillRegCount();
    if (spillRegCount == 0 || (!Spiller().HasSpill() && CheckFinalInsNeedSpill())) {
        Spiller().ResetSpill();
        return;
    }

    ES2PANDA_ASSERT(spillRegCount + GetCodeGen().GetRegsNum() < VReg::REG_MAX);
    GetCodeGen().AddSpillRegsToUsedRegs(spillRegCount);

    SArenaList<IRNode *> newInsns(GetCodeGen().Allocator()->Adapter());
    auto &insns = GetCodeGen().GetInsns();
    const auto funcRegsNum = GetCodeGen().GetRegsNum();

    for (auto *ins : insns) {
        std::array<VReg *, IRNode::MAX_REG_OPERAND> regs {};
        auto regCnt = ins->Registers(&regs);
        if (regCnt == 0) {
            newInsns.push_back(ins);
            continue;
        }

        auto registersSize = std::min(regCnt, static_cast<size_t>(ins->GetRealRegCount()));
        auto registers = Span<VReg *>(regs.data(), regs.data() + registersSize);

        for (auto *reg : registers) {
            ES2PANDA_ASSERT(reg != nullptr);
            if (!reg->IsParameter()) {
                reg->SetIndex(reg->GetIndex() - static_cast<VReg::Index>(spillRegCount));
            }
        }

        if (IsInsAllRegsValid(ins, registers, funcRegsNum)) {
            newInsns.push_back(ins);
            continue;
        }

        if (ins->IsRangeInst()) {
            AdjustRangeInsSpill(ins, newInsns);
        } else {
            AdjustInsSpill(registers, ins, newInsns);
        }
    }

    GetCodeGen().SetInsns(newInsns);
    Spiller().ResetSpill();
}

// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
class DstRegSpillInfo {
public:
    VReg vd;
    VReg vs;
    OperandType type;

    DstRegSpillInfo(VReg d, VReg s, OperandType t) : vd(d), vs(s), type(t) {}
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

void RegAllocator::AdjustInsSpill(const Span<VReg *> &registers, IRNode *ins, SArenaList<IRNode *> &newInsns)
{
    VReg::Index spillIndex = VReg::REG_START;
    std::vector<DstRegSpillInfo> dstRegSpills;
    const auto realRegCount = ins->GetRealRegCount();
    const auto limit = ins->GetRegLimit();
    const auto funcRegsNum = GetCodeGen().GetRegsNum();

    uint32_t idx = 0;
    for (auto *reg : registers) {
        if (idx >= realRegCount) {
            break;
        }

        if (reg->IsRegOrParamValid(limit, funcRegsNum)) {
            ++idx;
            continue;
        }

        const VReg originReg = *reg;
        VReg spillReg(spillIndex--);

        OperandType ty;
        if (ins->IsDevirtual()) {
            if (idx == 0) {
                ty = OperandType::REF;
            } else {
                ty = ins->GetOperandRegType(idx - 1);
            }
        } else {
            if (idx == 0 && ins->FirstArgIsThis()) {
                ty = OperandType::REF;
            } else {
                ty = ins->GetOperandRegType(idx);
            }
        }

        auto kind = ins->GetOperandRegKind(idx);
        if (kind == OperandKind::SRC_VREG || kind == OperandKind::SRC_DST_VREG) {
            auto *mov = GetCodeGen().AllocSpillMov(ins->Node(), spillReg, originReg, ty);
            newInsns.push_back(mov);
        }

        if (kind == OperandKind::DST_VREG || kind == OperandKind::SRC_DST_VREG) {
            dstRegSpills.emplace_back(originReg, spillReg, ty);
        }

        reg->SetIndex(spillReg.GetIndex());
        ++idx;
    }

    newInsns.push_back(ins);

    for (auto spill : dstRegSpills) {
        auto *mov = GetCodeGen().AllocSpillMov(ins->Node(), spill.vd, spill.vs, spill.type);
        newInsns.push_back(mov);
    }
}

// RangeRegAllocator

RangeRegAllocator::RangeRegAllocator(CodeGen *const cg, RegSpiller *const spiller) noexcept
    : RegAllocatorBase(cg, spiller)
{
}

void RangeRegAllocator::Run(IRNode *const ins, [[maybe_unused]] VReg rangeStart, const std::size_t argCount)
{
    ES2PANDA_ASSERT(Spiller().Restored());
    ES2PANDA_ASSERT(ins != nullptr);

    std::array<VReg *, IRNode::MAX_REG_OPERAND> regs {};
    ES2PANDA_ASSERT(ins != nullptr);
    const auto regCnt = ins->Registers(&regs);
    ES2PANDA_ASSERT(regCnt > 0);

    const auto registers = Span<VReg *>(regs.data(), regs.data() + regCnt);

    const auto realRegCount = regCnt + argCount - 1;
    Spiller().UpdateSpillRegCount(realRegCount);

    if (!Spiller().HasSpill() && !IsInsAllRegsValid(ins, registers, GetCodeGen().GetRegsNum())) {
        Spiller().SetHasSpill();
    }

    ins->SetRealRegCount(realRegCount);
    PushBack(ins);
}

void RegAllocator::AdjustRangeInsSpill(IRNode *ins, SArenaList<IRNode *> &newInsns)
{
    const auto realRegCount = ins->GetRealRegCount();

    std::array<VReg *, IRNode::MAX_REG_OPERAND> regs {};
    const auto regCnt = ins->Registers(&regs);
    ES2PANDA_ASSERT(regCnt >= 1);

    const uint32_t insLastRegIdx = regCnt - 1;
    VReg::Index spillIndex = VReg::REG_START;
    const auto startRegIndex = regs[insLastRegIdx]->GetIndex();
    const auto funcRegsNum = GetCodeGen().GetRegsNum();
    const auto limit = ins->GetRegLimit();

    for (uint32_t idx = 0; idx < realRegCount; ++idx) {
        VReg::Index regIndex;

        if (idx <= insLastRegIdx) {
            auto *currentReg = regs[idx];
            if (!currentReg->IsRegOrParamValid(limit, funcRegsNum)) {
                regIndex = currentReg->GetIndex();
                currentReg->SetIndex(spillIndex);
            } else if (idx < insLastRegIdx) {
                continue;
            } else {
                ES2PANDA_ASSERT(idx == insLastRegIdx);
                ES2PANDA_ASSERT(currentReg->IsRegOrParamValid(limit, funcRegsNum));
                break;
            }
        } else {
            regIndex = startRegIndex + (insLastRegIdx - idx);
        }

        VReg spillReg(spillIndex);
        const auto originReg = VReg(regIndex);
        OperandType ty;
        if (idx == 0 && (ins->FirstArgIsThis() || ins->IsDevirtual())) {
            ty = OperandType::REF;
        } else {
            ty = ins->IsRangeInst() ? ins->GetParamTypeAt(idx) : ins->GetOperandRegType(idx);
        }
        auto *mov = GetCodeGen().AllocSpillMov(ins->Node(), spillReg, originReg, ty);
        newInsns.push_back(mov);
        spillIndex--;
    }

    newInsns.push_back(ins);
}
}  // namespace ark::es2panda::compiler
