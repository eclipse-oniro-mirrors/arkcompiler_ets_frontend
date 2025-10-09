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

#ifndef ES2PANDA_COMPILER_IR_IRNODE_H
#define ES2PANDA_COMPILER_IR_IRNODE_H

#include "compiler/base/literals.h"
#include "compiler/core/vReg.h"
#include "compiler/core/programElement.h"
#include "lexer/token/sourceLocation.h"
#include "util/es2pandaMacros.h"
#include "util/ustring.h"
#include "libarkbase/utils/span.h"

#include <cstdint>
#include <list>
#include <limits>
#include <sstream>
#include <utility>
#include <variant>
#include <vector>

namespace ark::es2panda::ir {
class AstNode;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::compiler {
enum class OperandKind {
    // the least significant bit indicates vreg
    // the second bit indicates src or dst
    SRC_VREG,
    DST_VREG,
    SRC_DST_VREG,
    IMM,
    ID,
    STRING_ID,
    LABEL,
    NONE
};

enum class OperandType {
    REF,  // ref
    B32,  // u1 u2 u4 i8 u8 i16 u16 i32 u32 b32 f32
    B64,  // i64, f64, b64
    ANY,  // any
    NONE
};

struct OutVReg {
    const VReg *reg;
    OperandType type;
};

class FormatItem {
public:
    constexpr FormatItem(OperandKind kind, uint32_t bitWidth) : kind_(kind), bitWidth_(bitWidth) {}

    OperandKind Kind() const
    {
        return kind_;
    };

    bool constexpr IsVReg() const
    {
        return kind_ == OperandKind::SRC_VREG || kind_ == OperandKind::DST_VREG || kind_ == OperandKind::SRC_DST_VREG;
    }

    uint32_t BitWidth() const
    {
        return bitWidth_;
    };

private:
    OperandKind kind_;
    uint32_t bitWidth_;
};

class Format {
public:
    constexpr Format(const FormatItem *item, size_t size) : item_(item), size_(size) {}

    ark::Span<const FormatItem> GetFormatItem() const
    {
        return ark::Span<const FormatItem>(item_, size_);
    }

private:
    const FormatItem *item_;
    size_t size_;
};

using Formats = ark::Span<const Format>;

class Label;
class IRNode;

using Operand = std::variant<compiler::VReg, double, int64_t, util::StringView, Label *>;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define FIRST_NODE_OF_FUNCTION (reinterpret_cast<ir::AstNode *>(0x1))

class IRNode {
public:
    explicit IRNode(const ir::AstNode *node) : node_(node) {};
    virtual ~IRNode() = default;

    NO_COPY_SEMANTIC(IRNode);
    NO_MOVE_SEMANTIC(IRNode);

    const ir::AstNode *Node() const
    {
        return node_;
    }

    static uint16_t MapRegister(uint32_t reg, uint32_t totalRegs)
    {
        ES2PANDA_ASSERT(reg != VReg::Invalid().GetIndex());

        uint32_t regCount = VReg::REG_START - totalRegs;
        uint16_t newReg = 0;

        if (reg >= VReg::PARAM_START) {
            newReg = reg - VReg::PARAM_START + regCount;
            // NOTE: dbatiz. Remove this else if, and fix the regIndexes
        } else if (reg <= regCount + VReg::MANDATORY_PARAM_NUM) {
            newReg = VReg::REG_START - totalRegs + VReg::MANDATORY_PARAM_NUM + reg;
        } else {
            uint32_t regOffset = reg - totalRegs;
            newReg = std::abs(static_cast<int32_t>(regOffset - regCount));
        }

        return newReg;
    }

    static constexpr auto MAX_REG_OPERAND = 5;
    virtual OperandType GetOperandRegType([[maybe_unused]] size_t idx) const = 0;
    virtual OperandType GetParamTypeAt(size_t idx) const
    {
        return GetOperandRegType(idx);
    }
    virtual uint32_t GetRegLimit() const noexcept = 0;
    virtual OperandKind GetOperandRegKind(size_t idx) const = 0;
    virtual bool FirstArgIsThis() const noexcept
    {
        return false;
    }

    virtual Formats GetFormats() const = 0;
    virtual size_t Registers([[maybe_unused]] std::array<VReg *, MAX_REG_OPERAND> *regs) = 0;
    virtual size_t Registers([[maybe_unused]] std::array<const VReg *, MAX_REG_OPERAND> *regs) const = 0;
    virtual size_t OutRegisters([[maybe_unused]] std::array<OutVReg, MAX_REG_OPERAND> *regs) const = 0;
    virtual void Transform(ark::pandasm::Ins *ins, [[maybe_unused]] ProgramElement *programElement,
                           [[maybe_unused]] uint32_t totalRegs) const = 0;
    bool IsDevirtual() const
    {
        return isDevirtual_;
    }

    void SetDevirtual()
    {
        isDevirtual_ = true;
    }

    // for non-range ins, realRegCount_ is the number of non-dummy regs contained in the ins
    // for range ins, realRegCount_ is the number of regs contained in the ins and range regs
    uint32_t GetRealRegCount() const
    {
        return realRegCount_;
    }

    void SetRealRegCount(uint32_t s)
    {
        realRegCount_ = s;
    }

    bool IsRangeInst() const
    {
        return isRange_;
    }

    void SetIsRangeInst()
    {
        isRange_ = true;
    }

private:
    const ir::AstNode *node_;
    bool isDevirtual_ = false;
    uint32_t realRegCount_ = 0;
    bool isRange_ = false;
};
}  // namespace ark::es2panda::compiler

#endif
