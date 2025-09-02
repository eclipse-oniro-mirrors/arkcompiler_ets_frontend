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

#ifndef ES2PANDA_COMPILER_SCOPES_VARIABLE_H
#define ES2PANDA_COMPILER_SCOPES_VARIABLE_H

#include "varbinder/enumMemberResult.h"
#include "varbinder/variableFlags.h"
#include "ir/irnode.h"

namespace ark::es2panda::checker {
class Type;
enum class PropertyType;
// NOLINTBEGIN(readability-redundant-declaration)
bool IsTypeError(Type const *tp);
// NOLINTEND(readability-redundant-declaration)
}  // namespace ark::es2panda::checker

namespace ark::es2panda::varbinder {
class Decl;
class Scope;
class VariableScope;

// CC-OFFNXT(G.PRE.09) code gen
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_CLASSES(type, className) class className;  // CC-OFF(G.PRE.02) name part
VARIABLE_TYPES(DECLARE_CLASSES)
#undef DECLARE_CLASSES

class Variable {
public:
    virtual ~Variable() = default;
    NO_COPY_SEMANTIC(Variable);
    NO_MOVE_SEMANTIC(Variable);

    VariableType virtual Type() const = 0;

/* CC-OFFNXT(G.PRE.06) solid logic */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_CHECKS_CASTS(variableType, className)                                       \
    bool Is##className() const                                                              \
    {                                                                                       \
        /* CC-OFFNXT(G.PRE.05) The macro is used to generate a function. Return is needed*/ \
        return Type() == VariableType::variableType; /* CC-OFF(G.PRE.02) name part */       \
    }                                                                                       \
    /* CC-OFFNXT(G.PRE.02) name part */                                                     \
    className *As##className()                                                              \
    {                                                                                       \
        ES2PANDA_ASSERT(Is##className());                                                   \
        /* CC-OFFNXT(G.PRE.05) The macro is used to generate a function. Return is needed*/ \
        return reinterpret_cast<className *>(this); /* CC-OFF(G.PRE.02) name part */        \
    }                                                                                       \
    const className *As##className() const                                                  \
    {                                                                                       \
        ES2PANDA_ASSERT(Is##className());                                                   \
        /* CC-OFFNXT(G.PRE.05) The macro is used to generate a function. Return is needed*/ \
        return reinterpret_cast<const className *>(this);                                   \
    }
    VARIABLE_TYPES(DECLARE_CHECKS_CASTS)
#undef DECLARE_CHECKS_CASTS

    [[nodiscard]] const Decl *Declaration() const noexcept
    {
        return decl_;
    }

    [[nodiscard]] Decl *Declaration() noexcept
    {
        return decl_;
    }

    [[nodiscard]] VariableFlags Flags() const noexcept
    {
        return flags_;
    }

    [[nodiscard]] checker::Type *TsType() const
    {
        return tsType_;
    }

    [[nodiscard]] Scope *GetScope() const noexcept
    {
        return scope_;
    }

    checker::Type *SetTsType(checker::Type *tsType) noexcept
    {
        return (tsType_ = tsType);
    }

    void SetScope(varbinder::Scope *scope) noexcept
    {
        scope_ = scope;
    }

    void AddFlag(VariableFlags flag) noexcept
    {
        flags_ |= flag;
    }

    [[nodiscard]] bool HasFlag(VariableFlags flag) const noexcept
    {
        return (flags_ & flag) != 0;
    }

    void RemoveFlag(VariableFlags flag) noexcept
    {
        flags_ &= ~flag;
    }

    void Reset(Decl *decl, VariableFlags flags) noexcept
    {
        decl_ = decl;
        flags_ = flags;
    }

    [[nodiscard]] bool LexicalBound() const noexcept
    {
        return HasFlag(VariableFlags::LEXICAL_BOUND);
    }

    [[nodiscard]] const util::StringView &Name() const;
    virtual void SetLexical(Scope *scope) = 0;

protected:
    explicit Variable(Decl *decl, VariableFlags flags) : decl_(decl), flags_(flags) {}
    explicit Variable(VariableFlags flags) : flags_(flags) {}

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    Decl *decl_ {};
    VariableFlags flags_ {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)

private:
    checker::Type *tsType_ {};
    Scope *scope_ {};
};

class LocalVariable : public Variable {
public:
    explicit LocalVariable(Decl *decl, VariableFlags flags);
    explicit LocalVariable(const VariableFlags flags);

    VariableType Type() const override
    {
        return VariableType::LOCAL;
    }

    void BindVReg(compiler::VReg vreg)
    {
        ES2PANDA_ASSERT(!LexicalBound());
        vreg_ = vreg;
    }

    void BindLexEnvSlot(uint32_t slot)
    {
        ES2PANDA_ASSERT(!LexicalBound());
        AddFlag(VariableFlags::LEXICAL_BOUND);
        vreg_.SetIndex(slot);
    }

    compiler::VReg Vreg() const
    {
        return vreg_;
    }

    compiler::VReg &Vreg()
    {
        return vreg_;
    }

    uint32_t LexIdx() const
    {
        ES2PANDA_ASSERT(LexicalBound());
        return vreg_.GetIndex();
    }

    void SetLexical([[maybe_unused]] Scope *scope) override;
    LocalVariable *Copy(ArenaAllocator *allocator, Decl *decl) const;

private:
    compiler::VReg vreg_ {};
};

class GlobalVariable : public Variable {
public:
    explicit GlobalVariable(Decl *decl, VariableFlags flags) : Variable(decl, flags) {}

    VariableType Type() const override
    {
        return VariableType::GLOBAL;
    }

    void SetLexical([[maybe_unused]] Scope *scope) override;
};

class ModuleVariable : public Variable {
public:
    explicit ModuleVariable(Decl *decl, VariableFlags flags) : Variable(decl, flags) {}

    VariableType Type() const override
    {
        return VariableType::MODULE;
    }

    compiler::VReg &ModuleReg()
    {
        return moduleReg_;
    }

    compiler::VReg ModuleReg() const
    {
        return moduleReg_;
    }

    const util::StringView &ExoticName() const
    {
        return exoticName_;
    }

    util::StringView &ExoticName()
    {
        return exoticName_;
    }

    void SetLexical([[maybe_unused]] Scope *scope) override;

private:
    compiler::VReg moduleReg_ {};
    util::StringView exoticName_ {};
};

class EnumVariable : public Variable {
public:
    explicit EnumVariable(Decl *decl, bool backReference = false)
        : Variable(decl, VariableFlags::NONE), backReference_(backReference)
    {
    }

    VariableType Type() const override
    {
        return VariableType::ENUM;
    }

    void SetValue(EnumMemberResult value)
    {
        value_ = value;
    }

    const EnumMemberResult &Value() const
    {
        return value_;
    }

    bool BackReference() const
    {
        return backReference_;
    }

    void SetBackReference()
    {
        backReference_ = true;
    }

    void ResetDecl(Decl *decl);

    void SetLexical([[maybe_unused]] Scope *scope) override;

private:
    EnumMemberResult value_ {};
    bool backReference_ {};
};
}  // namespace ark::es2panda::varbinder
#endif
