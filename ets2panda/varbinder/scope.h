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

#ifndef ES2PANDA_COMPILER_SCOPES_SCOPE_H
#define ES2PANDA_COMPILER_SCOPES_SCOPE_H

#include "es2panda.h"
#include "varbinder/declaration.h"
#include "varbinder/variable.h"

namespace ark::es2panda::public_lib {
struct Context;
}  // namespace ark::es2panda::public_lib

namespace ark::es2panda::compiler {
class IRNode;
}  // namespace ark::es2panda::compiler

namespace ark::es2panda::ir {
class AstNode;
class Expression;
class Identifier;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::varbinder {
// CC-OFFNXT(G.PRE.09) code gen
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_CLASSES(type, className) class className;  // CC-OFF(G.PRE.02) name part
SCOPE_TYPES(DECLARE_CLASSES)
#undef DECLARE_CLASSES

template <typename ScopeT,
          std::enable_if_t<std::is_pointer_v<ScopeT> && std::is_base_of_v<Scope, std::remove_pointer_t<ScopeT>>, bool> =
              true>
class ScopeFindResultT {
public:
    ScopeFindResultT() = default;
    ScopeFindResultT(util::StringView n, ScopeT s, uint32_t l, Variable *v) : ScopeFindResultT(n, s, l, l, v) {}
    ScopeFindResultT(ScopeT s, uint32_t l, uint32_t ll, Variable *v) : scope(s), level(l), lexLevel(ll), variable(v) {}
    ScopeFindResultT(util::StringView n, ScopeT s, uint32_t l, uint32_t ll, Variable *v)
        : name(n), scope(s), level(l), lexLevel(ll), variable(v)
    {
    }

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    util::StringView name {};
    ScopeT scope {};
    uint32_t level {};
    uint32_t lexLevel {};
    Variable *variable {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

using ConstScopeFindResult = ScopeFindResultT<const Scope *>;
using ScopeFindResult = ScopeFindResultT<Scope *>;

class Scope {
public:
    Scope() = delete;
    virtual ~Scope() = default;
    NO_COPY_SEMANTIC(Scope);
    NO_MOVE_SEMANTIC(Scope);

    using VariableMap = ArenaUnorderedMap<util::StringView, Variable *>;
    using InsertResult = std::pair<VariableMap::const_iterator, bool>;

    virtual ScopeType Type() const = 0;

// CC-OFFNXT(G.PRE.06) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_CHECKS_CASTS(scopeType, className)                                           \
    bool Is##className() const                                                               \
    {                                                                                        \
        /* CC-OFFNXT(G.PRE.05) The macro is used to generate a function. Return is needed*/  \
        return Type() == ScopeType::scopeType; /* CC-OFF(G.PRE.02) name part */              \
    }                                                                                        \
    /* CC-OFFNXT(G.PRE.02) name part */                                                      \
    className *As##className()                                                               \
    {                                                                                        \
        ES2PANDA_ASSERT(Is##className());                                                    \
        /* CC-OFFNXT(G.PRE.05) The macro is used to generate a function. Return is needed*/  \
        return reinterpret_cast<className *>(this); /* CC-OFF(G.PRE.02) name part */         \
    }                                                                                        \
    const className *As##className() const                                                   \
    {                                                                                        \
        ES2PANDA_ASSERT(Is##className());                                                    \
        /* CC-OFFNXT(G.PRE.05) The macro is used to generate a function. Return is needed */ \
        return reinterpret_cast<const className *>(this);                                    \
    }
    SCOPE_TYPES(DECLARE_CHECKS_CASTS)
#undef DECLARE_CHECKS_CASTS

    [[nodiscard]] bool IsVariableScope() const noexcept
    {
        return Type() > ScopeType::LOCAL;
    }

    [[nodiscard]] bool IsFunctionVariableScope() const noexcept
    {
        return Type() >= ScopeType::FUNCTION;
    }

    [[nodiscard]] FunctionScope *AsFunctionVariableScope()
    {
        ES2PANDA_ASSERT(IsFunctionVariableScope());
        return reinterpret_cast<FunctionScope *>(this);
    }

    [[nodiscard]] const FunctionScope *AsFunctionVariableScope() const
    {
        ES2PANDA_ASSERT(IsFunctionVariableScope());
        return reinterpret_cast<const FunctionScope *>(this);
    }

    [[nodiscard]] VariableScope *AsVariableScope()
    {
        ES2PANDA_ASSERT(IsVariableScope());
        return reinterpret_cast<VariableScope *>(this);
    }

    [[nodiscard]] const VariableScope *AsVariableScope() const
    {
        ES2PANDA_ASSERT(IsVariableScope());
        return reinterpret_cast<const VariableScope *>(this);
    }

    [[nodiscard]] VariableScope *EnclosingVariableScope() noexcept;
    [[nodiscard]] const VariableScope *EnclosingVariableScope() const noexcept;

    void AddFlag(ScopeFlags flag) noexcept
    {
        flags_ |= flag;
    }

    void ClearFlag(ScopeFlags flag) noexcept
    {
        flags_ &= ~flag;
    }

    [[nodiscard]] bool HasFlag(ScopeFlags flag) const noexcept
    {
        return (flags_ & flag) != 0;
    }

    [[nodiscard]] ArenaVector<Decl *> &Decls() noexcept
    {
        return decls_;
    }

    [[nodiscard]] const ArenaVector<Decl *> &Decls() const noexcept
    {
        return decls_;
    }

    void SetParent(Scope *parent) noexcept
    {
        parent_ = parent;
    }

    [[nodiscard]] Scope *Parent() noexcept
    {
        return parent_;
    }

    [[nodiscard]] const Scope *Parent() const noexcept
    {
        return parent_;
    }

    [[nodiscard]] ir::AstNode *Node() noexcept
    {
        return node_;
    }

    [[nodiscard]] const ir::AstNode *Node() const noexcept
    {
        return node_;
    }

    void BindNode(ir::AstNode *node) noexcept
    {
        node_ = node;
    }

    Variable *AddDecl(ArenaAllocator *allocator, Decl *decl, ScriptExtension extension)
    {
        ES2PANDA_ASSERT(decl != nullptr);
        auto *var =
            AddBinding(allocator, FindLocal(decl->Name(), varbinder::ResolveBindingOptions::BINDINGS), decl, extension);
        if (var != nullptr) {
            decls_.emplace_back(decl);
        }
        return var;
    }

    Variable *AddTsDecl(ArenaAllocator *allocator, Decl *decl, ScriptExtension extension)
    {
        decls_.emplace_back(decl);
        return AddBinding(allocator, FindLocal(decl->Name(), ResolveBindingOptions::ALL), decl, extension);
    }

    template <typename T, typename... Args>
    T *NewDecl(ArenaAllocator *allocator, Args &&...args);

    template <typename DeclType, typename VariableType>
    std::pair<varbinder::Variable *, bool> AddDecl(ArenaAllocator *allocator, util::StringView name,
                                                   VariableFlags flags);

    template <typename DeclType = varbinder::LetDecl, typename VariableType = varbinder::LocalVariable>
    static VariableType *CreateVar(ArenaAllocator *allocator, util::StringView name, VariableFlags flags,
                                   ir::AstNode *node);

    template <typename T, typename... Args>
    Variable *PropagateBinding(ArenaAllocator *allocator, util::StringView name, Args &&...args);

    virtual InsertResult InsertBinding(const util::StringView &name, Variable *var);
    virtual InsertResult InsertOrAssignBinding(const util::StringView &name, Variable *const var);
    virtual InsertResult TryInsertBinding(const util::StringView &name, Variable *var);
    virtual void MergeBindings(VariableMap const &bindings);
    virtual VariableMap::size_type EraseBinding(const util::StringView &name);
    virtual bool CorrectForeignBinding(const util::StringView &name, Variable *builtinVar, Variable *redefinedVar);

    [[nodiscard]] const VariableMap &Bindings() const noexcept
    {
        return bindings_;
    }

    [[nodiscard]] ArenaMap<util::StringView, Variable *> OrderedBindings(ArenaAllocator *allocator) const noexcept
    {
        ArenaMap<util::StringView, Variable *> result(allocator->Adapter());
        result.insert(bindings_.begin(), bindings_.end());
        return result;
    }

    virtual Variable *AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                                 [[maybe_unused]] ScriptExtension extension) = 0;

    virtual Variable *FindLocal(const util::StringView &name, ResolveBindingOptions options) const;

    [[nodiscard]] bool IsSuperscopeOf(const varbinder::Scope *subscope) const noexcept;

    ConstScopeFindResult Find(const util::StringView &name,
                              ResolveBindingOptions options = ResolveBindingOptions::BINDINGS) const;

    ScopeFindResult Find(const util::StringView &name, ResolveBindingOptions options = ResolveBindingOptions::BINDINGS);

    ConstScopeFindResult FindInGlobal(const util::StringView &name,
                                      ResolveBindingOptions options = ResolveBindingOptions::BINDINGS) const;

    ConstScopeFindResult FindInFunctionScope(const util::StringView &name,
                                             ResolveBindingOptions options = ResolveBindingOptions::BINDINGS) const;

    [[nodiscard]] Decl *FindDecl(const util::StringView &name) const noexcept;

protected:
    explicit Scope(ArenaAllocator *allocator, Scope *parent)
        : parent_(parent), decls_(allocator->Adapter()), bindings_(allocator->Adapter())
    {
    }

    explicit Scope(ArenaAllocator *allocator, Scope *parent, ScopeFlags flags)
        : parent_(parent), decls_(allocator->Adapter()), bindings_(allocator->Adapter()), flags_(flags)
    {
    }

    /**
     * @return true - if the variable is shadowed
     *         false - otherwise
     */
    using VariableVisitor = std::function<bool(const Variable *)>;

    /**
     * @return true - if the variable is shadowed
     *         false - otherwise
     */
    std::tuple<Scope *, bool> IterateShadowedVariables(const util::StringView &name, const VariableVisitor &visitor);

    Variable *AddLocal(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                       [[maybe_unused]] ScriptExtension extension);

    Variable *AddLocalVar(ArenaAllocator *allocator, Decl *newDecl);
    Variable *AddLocalInterfaceVariable(ArenaAllocator *allocator, Decl *newDecl);
    Variable *AddLocalTypeAliasVariable(ArenaAllocator *allocator, Decl *newDecl);
    Variable *AddLocalClassVariable(ArenaAllocator *allocator, Decl *newDecl);

private:
    template <
        typename ResultT, typename ScopeT,
        std::enable_if_t<std::is_same_v<ResultT, ConstScopeFindResult> || std::is_same_v<ResultT, ScopeFindResult>,
                         bool> = true,
        std::enable_if_t<std::is_pointer_v<ScopeT> && std::is_base_of_v<Scope, std::remove_pointer_t<ScopeT>>, bool> =
            true>
    static ResultT FindImpl(ScopeT &&scope, const util::StringView &name, const ResolveBindingOptions options)
    {
        uint32_t level = 0;
        uint32_t lexLevel = 0;
        // iter will be the EXACT type of scope with cv-qualifiers
        auto &&iter = scope;

        if (iter->IsFunctionParamScope()) {
            auto *const v = iter->FindLocal(name, options);

            if (v != nullptr) {
                return {name, iter, level, lexLevel, v};
            }

            level++;
            const auto *const funcVariableScope = iter->AsFunctionParamScope()->GetFunctionScope();

            if (funcVariableScope != nullptr && funcVariableScope->NeedLexEnv()) {
                lexLevel++;
            }

            iter = iter->Parent();
        }

        while (iter != nullptr) {
            auto *const v = iter->FindLocal(name, options);

            if (v != nullptr) {
                return {name, iter, level, lexLevel, v};
            }

            if (iter->IsVariableScope()) {
                level++;

                if (iter->AsVariableScope()->NeedLexEnv()) {
                    lexLevel++;
                }
            }

            iter = iter->Parent();
        }

        return {name, nullptr, 0, 0, nullptr};
    }

    Scope *parent_ {};
    ArenaVector<Decl *> decls_;
    VariableMap bindings_;
    ir::AstNode *node_ {};
    ScopeFlags flags_ {};
};

class VariableScope : public Scope {
public:
    ~VariableScope() override = default;
    NO_COPY_SEMANTIC(VariableScope);
    NO_MOVE_SEMANTIC(VariableScope);

    uint32_t NextSlot()
    {
        return slotIndex_++;
    }

    uint32_t LexicalSlots() const
    {
        return slotIndex_;
    }

    bool NeedLexEnv() const
    {
        return slotIndex_ != 0;
    }

    uint32_t EvalBindings() const
    {
        return evalBindings_;
    }

    void CheckDirectEval(public_lib::Context *context);

protected:
    explicit VariableScope(ArenaAllocator *allocator, Scope *parent) : Scope(allocator, parent) {}

    template <typename T>
    Variable *AddVar(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl, ScriptExtension extension);

    template <typename T>
    Variable *AddFunction(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                          [[maybe_unused]] ScriptExtension extension);

    template <typename T>
    Variable *AddTSBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl, VariableFlags flags);

    template <typename T>
    Variable *AddLexical(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl);

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    uint32_t evalBindings_ {};
    uint32_t slotIndex_ {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

class ParamScope : public Scope {
public:
    ScopeType Type() const override
    {
        return ScopeType::PARAM;
    }

    ArenaVector<LocalVariable *> &Params()
    {
        return params_;
    }

    const ArenaVector<LocalVariable *> &Params() const
    {
        return params_;
    }

    std::tuple<Variable *, ir::Expression *> AddParamDecl(ArenaAllocator *allocator, varbinder::VarBinder *vb,
                                                          ir::Expression *parameter);

protected:
    explicit ParamScope(ArenaAllocator *allocator, Scope *parent)
        : Scope(allocator, parent), params_(allocator->Adapter())
    {
    }

    Variable *AddParameter(ArenaAllocator *allocator, Decl *newDecl, VariableFlags flags);

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    ArenaVector<LocalVariable *> params_;
};

class FunctionScope;

class FunctionParamScope : public ParamScope {
public:
    explicit FunctionParamScope(ArenaAllocator *allocator, Scope *parent) : ParamScope(allocator, parent) {}

    FunctionScope *GetFunctionScope() const
    {
        return functionScope_;
    }

    void BindFunctionScope(FunctionScope *funcScope)
    {
        functionScope_ = funcScope;
    }

    [[nodiscard]] LocalVariable *NameVar() const noexcept
    {
        return nameVar_;
    }

    void BindName(ArenaAllocator *allocator, util::StringView name);

    ScopeType Type() const override
    {
        return ScopeType::FUNCTION_PARAM;
    }

    Variable *AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                         [[maybe_unused]] ScriptExtension extension) override;

    friend class FunctionScope;
    template <typename E, typename T>
    friend class ScopeWithParamScope;

private:
    FunctionScope *functionScope_ {};
    LocalVariable *nameVar_ {};
};

template <typename E, typename T>
class ScopeWithParamScope : public E {
public:
    explicit ScopeWithParamScope(ArenaAllocator *allocator, Scope *parent) : E(allocator, parent), paramScope_(nullptr)
    {
    }

    void BindParamScope(T *paramScope)
    {
        AssignParamScope(paramScope);
        this->MergeBindings(paramScope->Bindings());
    }

    void AssignParamScope(T *paramScope)
    {
        ES2PANDA_ASSERT(this->Parent() == paramScope);
        paramScope_ = paramScope;
    }

    T *ParamScope()
    {
        return paramScope_;
    }

    const T *ParamScope() const
    {
        return paramScope_;
    }

protected:
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    T *paramScope_;
};

class LocalScope : public Scope {
public:
    explicit LocalScope(ArenaAllocator *allocator, Scope *parent) : Scope(allocator, parent) {}
    explicit LocalScope(ArenaAllocator *allocator, Scope *parent, ScopeFlags flags) : Scope(allocator, parent, flags) {}

    ScopeType Type() const override
    {
        return ScopeType::LOCAL;
    }

    Variable *AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                         [[maybe_unused]] ScriptExtension extension) override;
};

class FunctionScope : public ScopeWithParamScope<VariableScope, FunctionParamScope> {
public:
    explicit FunctionScope(ArenaAllocator *allocator, Scope *parent) : ScopeWithParamScope(allocator, parent) {}

    ScopeType Type() const override
    {
        return ScopeType::FUNCTION;
    }

    void BindName(util::StringView name)
    {
        name_ = name;
    }

    void BindInternalName(util::StringView internalName)
    {
        internalName_ = internalName;
    }

    const util::StringView &Name() const
    {
        return name_;
    }

    const util::StringView &InternalName() const
    {
        return internalName_;
    }

    Variable *AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                         [[maybe_unused]] ScriptExtension extension) override;
    Variable *InsertBindingIfAbsentInScope(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                                           VariableFlags flag);

private:
    util::StringView name_ {};
    util::StringView internalName_ {};
};

class ClassScope : public LocalScope {
public:
    explicit ClassScope(ArenaAllocator *allocator, Scope *parent)
        : LocalScope(allocator, parent),
          typeAliasScope_(allocator->New<LocalScope>(allocator, this, ScopeFlags::TYPE_ALIAS)),
          staticDeclScope_(allocator->New<LocalScope>(allocator, typeAliasScope_, ScopeFlags::STATIC_DECL_SCOPE)),
          staticFieldScope_(allocator->New<LocalScope>(allocator, staticDeclScope_, ScopeFlags::STATIC_FIELD_SCOPE)),
          staticMethodScope_(allocator->New<LocalScope>(allocator, staticFieldScope_, ScopeFlags::STATIC_METHOD_SCOPE)),
          instanceDeclScope_(allocator->New<LocalScope>(allocator, staticMethodScope_, ScopeFlags::DECL_SCOPE)),
          instanceFieldScope_(allocator->New<LocalScope>(allocator, instanceDeclScope_, ScopeFlags::FIELD_SCOPE)),
          instanceMethodScope_(allocator->New<LocalScope>(allocator, instanceFieldScope_, ScopeFlags::METHOD_SCOPE))
    {
    }

    ScopeType Type() const override
    {
        return ScopeType::CLASS;
    }

    LocalScope *TypeAliasScope()
    {
        return typeAliasScope_;
    }

    const LocalScope *TypeAliasScope() const
    {
        return typeAliasScope_;
    }

    LocalScope *StaticDeclScope()
    {
        return staticDeclScope_;
    }

    const LocalScope *StaticDeclScope() const
    {
        return staticDeclScope_;
    }

    LocalScope *StaticFieldScope()
    {
        return staticFieldScope_;
    }

    const LocalScope *StaticFieldScope() const
    {
        return staticFieldScope_;
    }

    LocalScope *StaticMethodScope()
    {
        return staticMethodScope_;
    }

    const LocalScope *StaticMethodScope() const
    {
        return staticMethodScope_;
    }

    LocalScope *InstanceFieldScope()
    {
        return instanceFieldScope_;
    }

    const LocalScope *InstanceFieldScope() const
    {
        return instanceFieldScope_;
    }

    LocalScope *InstanceMethodScope()
    {
        return instanceMethodScope_;
    }

    const LocalScope *InstanceMethodScope() const
    {
        return instanceMethodScope_;
    }

    LocalScope *InstanceDeclScope()
    {
        return instanceDeclScope_;
    }

    const LocalScope *InstanceDeclScope() const
    {
        return instanceDeclScope_;
    }

    uint32_t GetAndIncrementAnonymousClassIdx() const
    {
        return anonymousClassIdx_++;
    }

    Variable *FindLocal(const util::StringView &name, ResolveBindingOptions options) const override;

    Variable *AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                         [[maybe_unused]] ScriptExtension extension) override;

    class BindingProps {
    public:
        BindingProps() = default;

        void SetFlagsType(VariableFlags flagsType)
        {
            flags_ |= flagsType;
        }
        void SetBindingProps(VariableFlags flags, ir::Identifier *ident, LocalScope *targetScope)
        {
            flags_ |= flags;
            ident_ = ident;
            targetScope_ = targetScope;
        }
        VariableFlags GetFlags() const
        {
            return flags_;
        }
        ir::Identifier *GetIdent()
        {
            return ident_;
        }
        LocalScope *GetTargetScope()
        {
            return targetScope_;
        }

    private:
        VariableFlags flags_ = VariableFlags::NONE;
        ir::Identifier *ident_ {};
        LocalScope *targetScope_ {};
    };

    void SetBindingProps(Decl *newDecl, BindingProps *props, bool isStatic);

private:
    LocalScope *typeAliasScope_;
    LocalScope *staticDeclScope_;
    LocalScope *staticFieldScope_;
    LocalScope *staticMethodScope_;
    LocalScope *instanceDeclScope_;
    LocalScope *instanceFieldScope_;
    LocalScope *instanceMethodScope_;
    mutable uint32_t anonymousClassIdx_ {1};
};

class AnnotationScope : public ClassScope {
public:
    explicit AnnotationScope(ArenaAllocator *allocator, Scope *parent) : ClassScope(allocator, parent) {}

    ScopeType Type() const override
    {
        return ScopeType::ANNOTATION;
    }
};

class AnnotationParamScope : public ParamScope {
public:
    explicit AnnotationParamScope(ArenaAllocator *allocator, Scope *parent) : ParamScope(allocator, parent) {}

    ScopeType Type() const override
    {
        return ScopeType::ANNOTATIONPARAMSCOPE;
    }

    Variable *AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                         [[maybe_unused]] ScriptExtension extension) override;
};

class CatchParamScope : public ParamScope {
public:
    explicit CatchParamScope(ArenaAllocator *allocator, Scope *parent) : ParamScope(allocator, parent) {}

    ScopeType Type() const override
    {
        return ScopeType::CATCH_PARAM;
    }

    Variable *AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                         [[maybe_unused]] ScriptExtension extension) override;

    friend class CatchScope;
};

class CatchScope : public ScopeWithParamScope<LocalScope, CatchParamScope> {
public:
    explicit CatchScope(ArenaAllocator *allocator, Scope *parent) : ScopeWithParamScope(allocator, parent) {}

    ScopeType Type() const override
    {
        return ScopeType::CATCH;
    }

    Variable *AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                         [[maybe_unused]] ScriptExtension extension) override;
    Variable *FindLocal(const util::StringView &name, ResolveBindingOptions options) const override;
};

class LoopScope;

class LoopDeclarationScope : public VariableScope {
public:
    explicit LoopDeclarationScope(ArenaAllocator *allocator, Scope *parent) : VariableScope(allocator, parent) {}

    ScopeType Type() const override
    {
        return loopType_;
    }

    Variable *AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                         [[maybe_unused]] ScriptExtension extension) override
    {
        return AddLocal(allocator, currentVariable, newDecl, extension);
    }

    Scope *InitScope()
    {
        if (NeedLexEnv()) {
            return initScope_;
        }

        return this;
    }

    void ConvertToVariableScope(ArenaAllocator *allocator);

private:
    friend class LoopScope;
    LoopScope *loopScope_ {};
    LocalScope *initScope_ {};
    ScopeType loopType_ {ScopeType::LOCAL};
};

class LoopScope : public VariableScope {
public:
    explicit LoopScope(ArenaAllocator *allocator, Scope *parent) : VariableScope(allocator, parent)
    {
        // NOTE(kkonkuznetsov): currently LoopScope type has ScopeType::LOCAL
        // therefore it does not respond to IsLoopScope() because it checks for type.
        // This LOOP_SCOPE flag can be used to check that scope is actually a loop scope.
        AddFlag(ScopeFlags::LOOP_SCOPE);
    }

    LoopDeclarationScope *DeclScope()
    {
        return declScope_;
    }

    void BindDecls(LoopDeclarationScope *declScope)
    {
        declScope_ = declScope;
        declScope_->loopScope_ = this;
    }

    ScopeType Type() const override
    {
        return loopType_;
    }

    void ConvertToVariableScope(ArenaAllocator *allocator);

    Variable *AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                         [[maybe_unused]] ScriptExtension extension) override
    {
        return AddLocal(allocator, currentVariable, newDecl, extension);
    }

protected:
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    LoopDeclarationScope *declScope_ {};
    ScopeType loopType_ {ScopeType::LOCAL};
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

class GlobalScope : public FunctionScope {
public:
    explicit GlobalScope(ArenaAllocator *allocator)
        : FunctionScope(allocator, nullptr), foreignBindings_(allocator->Adapter())
    {
        auto *paramScope = allocator->New<FunctionParamScope>(allocator, this);
        paramScope_ = paramScope;
        ES2PANDA_ASSERT(paramScope_ != nullptr);
        paramScope_->BindFunctionScope(this);
    }

    ScopeType Type() const override
    {
        return ScopeType::GLOBAL;
    }

    Variable *AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                         [[maybe_unused]] ScriptExtension extension) override;

    InsertResult InsertBinding(const util::StringView &name, Variable *var) override;
    InsertResult TryInsertBinding(const util::StringView &name, Variable *var) override;
    void MergeBindings(VariableMap const &bindings) override;
    VariableMap::size_type EraseBinding(const util::StringView &name) override;

    InsertResult InsertForeignBinding(const util::StringView &name, Variable *var);
    InsertResult InsertOrAssignForeignBinding(const util::StringView &name, Variable *var);
    [[nodiscard]] bool IsForeignBinding(const util::StringView &name) const;

    InsertResult InsertDynamicBinding(const util::StringView &name, Variable *var);
    bool CorrectForeignBinding(const util::StringView &name, Variable *builtinVar, Variable *redefinedVar) override;

private:
    enum class InsertBindingFlags : uint8_t { NONE = 0, FOREIGN = 1U << 0U, DYNAMIC = 1U << 1U, ASSIGN = 1U << 2U };
    InsertResult InsertImpl(const util::StringView &name, Variable *var, InsertBindingFlags flags);

    ArenaUnorderedMap<util::StringView, bool> foreignBindings_;
};

class ModuleScope : public GlobalScope {
public:
    template <typename K, typename V>
    using ModuleEntry = ArenaVector<std::pair<K, V>>;
    using ImportDeclList = ArenaVector<ImportDecl *>;
    using ExportDeclList = ArenaVector<ExportDecl *>;
    using LocalExportNameMap = ArenaMultiMap<varbinder::Variable *, util::StringView>;

    explicit ModuleScope(ArenaAllocator *allocator)
        : GlobalScope(allocator),
          allocator_(allocator),
          imports_(allocator_->Adapter()),
          exports_(allocator_->Adapter()),
          localExports_(allocator_->Adapter())
    {
    }

    ScopeType Type() const override
    {
        return ScopeType::MODULE;
    }

    const ModuleEntry<ir::ImportDeclaration *, ImportDeclList> &Imports() const
    {
        return imports_;
    }

    const ModuleEntry<ir::AstNode *, ExportDeclList> &Exports() const
    {
        return exports_;
    }

    const LocalExportNameMap &LocalExports() const
    {
        return localExports_;
    }

    void AddImportDecl(ir::ImportDeclaration *importDecl, ImportDeclList &&decls);

    void AddExportDecl(ir::AstNode *exportDecl, ExportDecl *decl);

    void AddExportDecl(ir::AstNode *exportDecl, ExportDeclList &&decls);

    Variable *AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                         [[maybe_unused]] ScriptExtension extension) override;

    bool ExportAnalysis();

private:
    Variable *AddImport(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl);

    ArenaAllocator *allocator_;
    ModuleEntry<ir::ImportDeclaration *, ImportDeclList> imports_;
    ModuleEntry<ir::AstNode *, ExportDeclList> exports_;
    LocalExportNameMap localExports_;
};

template <typename T, typename... Args>
T *Scope::NewDecl(ArenaAllocator *allocator, Args &&...args)
{
    T *decl = allocator->New<T>(std::forward<Args>(args)...);
    decls_.push_back(decl);

    return decl;
}

template <typename DeclType, typename VariableType>
std::pair<varbinder::Variable *, bool> Scope::AddDecl(ArenaAllocator *allocator, util::StringView name,
                                                      VariableFlags flags)
{
    varbinder::Variable *variable = FindLocal(name, varbinder::ResolveBindingOptions::BINDINGS);
    if (variable != nullptr) {
        return std::make_pair(variable, true);
    }

    auto *decl = allocator->New<DeclType>(name);
    variable = allocator->New<VariableType>(decl, flags);
    ES2PANDA_ASSERT(variable != nullptr);
    decls_.emplace_back(decl);
    bindings_.insert({decl->Name(), variable});
    variable->SetScope(this);

    return std::make_pair(variable, false);
}

template <typename DeclType, typename VariableType>
VariableType *Scope::CreateVar(ArenaAllocator *allocator, util::StringView name, VariableFlags flags, ir::AstNode *node)
{
    auto *decl = allocator->New<DeclType>(name);
    auto *variable = allocator->New<VariableType>(decl, flags);
    decl->BindNode(node);
    return variable;
}

}  // namespace ark::es2panda::varbinder

namespace enumbitops {

template <>
struct IsAllowedType<ark::es2panda::varbinder::GlobalScope::InsertBindingFlags> : std::true_type {
};

}  // namespace enumbitops

#endif
