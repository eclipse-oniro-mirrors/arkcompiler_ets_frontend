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

#ifndef ES2PANDA_VARBINDER_VARBINDER_H
#define ES2PANDA_VARBINDER_VARBINDER_H

#include "lexer/token/sourceLocation.h"
#include "scope.h"
#include "varbinder/scope.h"

namespace ark::es2panda::parser {
class Program;
enum class ScriptKind;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::ir {
class AstNode;
class BlockStatement;
class CatchClause;
class ClassDefinition;
class Expression;
class ForUpdateStatement;
class Identifier;
class ScriptFunction;
class Statement;
class VariableDeclarator;
class TSFunctionType;
class ThisExpression;
class MemberExpression;
class ClassStaticBlock;
class Identifier;
class TSTypeAliasDeclaration;
class ClassProperty;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::public_lib {
struct Context;
}  // namespace ark::es2panda::public_lib

namespace ark::es2panda::varbinder {
class ETSBinder;

class VarBinder {
public:
    explicit VarBinder(ArenaAllocator *allocator) : allocator_(allocator), functionScopes_(allocator_->Adapter()) {}

    VarBinder() = delete;
    NO_COPY_SEMANTIC(VarBinder);
    NO_MOVE_SEMANTIC(VarBinder);
    virtual ~VarBinder() = default;

    void InitTopScope();
    virtual void IdentifierAnalysis();

    template <typename T, typename... Args>
    T *AddDecl(const lexer::SourcePosition &pos, Args &&...args);

    template <typename T, typename... Args>
    T *AddTsDecl(const lexer::SourcePosition &pos, Args &&...args);

    template <typename T, typename... Args>
    std::tuple<T *, varbinder::Variable *> NewVarDecl(const lexer::SourcePosition &pos, Args &&...args);

    Variable *AddParamDecl(ir::Expression *param);

    void SetProgram(parser::Program *program) noexcept
    {
        program_ = program;
    }

    [[nodiscard]] parser::Program *Program() noexcept
    {
        return program_;
    }

    [[nodiscard]] const parser::Program *Program() const
    {
        ES2PANDA_ASSERT(program_);
        return program_;
    }

    void SetContext(public_lib::Context *context)
    {
        context_ = context;
    }

    [[nodiscard]] public_lib::Context *GetContext() const
    {
        ES2PANDA_ASSERT(context_);
        return context_;
    }

    void SetGenStdLib(bool genStdLib) noexcept
    {
        genStdLib_ = genStdLib;
    }

    [[nodiscard]] bool IsGenStdLib() noexcept
    {
        return genStdLib_;
    }

    [[nodiscard]] Scope *GetScope() const noexcept
    {
        return scope_;
    }

    void ResetAllScopes(GlobalScope *topScope, VariableScope *varScope, Scope *scope) noexcept
    {
        topScope_ = topScope;
        varScope_ = varScope;
        scope_ = scope;
    }

    void ResetTopScope(GlobalScope *topScope)
    {
        ES2PANDA_ASSERT(topScope_ == scope_);
        topScope_ = topScope;
        varScope_ = topScope_;
        scope_ = topScope_;
    }

    void SetTopScope(GlobalScope *topScope) noexcept
    {
        topScope_ = topScope;
    }

    [[nodiscard]] GlobalScope *TopScope() const noexcept
    {
        return topScope_;
    }

    [[nodiscard]] VariableScope *VarScope() const noexcept
    {
        return varScope_;
    }

    [[nodiscard]] bool IsETSBinder() const noexcept
    {
        return Extension() == ScriptExtension::ETS;
    }

    [[nodiscard]] const ETSBinder *AsETSBinder() const
    {
        ES2PANDA_ASSERT(Extension() == ScriptExtension::ETS);
        return reinterpret_cast<const ETSBinder *>(this);
    }

    [[nodiscard]] ETSBinder *AsETSBinder()
    {
        ES2PANDA_ASSERT(Extension() == ScriptExtension::ETS);
        return reinterpret_cast<ETSBinder *>(this);
    }

    void ThrowPrivateFieldMismatch(const lexer::SourcePosition &pos, const util::StringView &name) const;
    void ThrowRedeclaration(const lexer::SourcePosition &pos, const util::StringView &name, DeclType declType) const;
    void ThrowLocalRedeclaration(const lexer::SourcePosition &pos, const util::StringView &name) const;
    void ThrowUnresolvableType(const lexer::SourcePosition &pos, const util::StringView &name) const;
    void ThrowTDZ(const lexer::SourcePosition &pos, const util::StringView &name) const;
    void ThrowInvalidCapture(const lexer::SourcePosition &pos, const util::StringView &name) const;
    void ThrowError(const lexer::SourcePosition &pos, const diagnostic::DiagnosticKind &kind) const
    {
        ThrowError(pos, kind, util::DiagnosticMessageParams {});
    }
    virtual void ThrowError(const lexer::SourcePosition &pos, const diagnostic::DiagnosticKind &kind,
                            const util::DiagnosticMessageParams &params) const;
    virtual bool IsGlobalIdentifier(const util::StringView &str) const;

    void PropagateDirectEval() const;

    template <typename T>
    friend class LexicalScope;
    friend class TopScopeContext;

    [[nodiscard]] ArenaAllocator *Allocator() const noexcept
    {
        return allocator_;
    }

    [[nodiscard]] const ArenaVector<FunctionScope *> &Functions() const noexcept
    {
        return functionScopes_;
    }

    [[nodiscard]] ArenaVector<FunctionScope *> &Functions() noexcept
    {
        return functionScopes_;
    }

    [[nodiscard]] virtual ScriptExtension Extension() const noexcept
    {
        return ScriptExtension::JS;
    }

    [[nodiscard]] virtual ResolveBindingOptions BindingOptions() const noexcept
    {
        return ResolveBindingOptions::BINDINGS;
    }

    LocalVariable *AddMandatoryParam(const std::string_view &name);

    inline static constexpr std::string_view const FUNCTION_ARGUMENTS = "arguments";
    inline static constexpr std::string_view const MANDATORY_PARAM_FUNC = "=f";
    inline static constexpr std::string_view const MANDATORY_PARAM_NEW_TARGET = "=nt";
    inline static constexpr std::string_view const MANDATORY_PARAM_THIS = "=t";

    inline static constexpr std::uint32_t const MANDATORY_PARAM_FUNC_REG = 0U;
    inline static constexpr std::uint32_t const MANDATORY_PARAMS_NUMBER = 3U;

    inline static constexpr std::string_view const LEXICAL_MANDATORY_PARAM_FUNC = "!f";
    inline static constexpr std::string_view const LEXICAL_MANDATORY_PARAM_NEW_TARGET = "!nt";
    inline static constexpr std::string_view const LEXICAL_MANDATORY_PARAM_THIS = "!t";

    inline static constexpr std::string_view const LEXICAL_CONTEXT_PARAM = "=eval";
    inline static constexpr std::string_view const MAIN = "main";
    inline static constexpr std::uint32_t const LEXICAL_CONTEXT_PARAM_REG = MANDATORY_PARAMS_NUMBER;
    inline static constexpr std::string_view const STAR_IMPORT = "*";

    void ResolveReferenceDoWhileHelper(ir::AstNode *childNode);
    void ResolveReferenceWhileHelper(ir::AstNode *childNode);
    void ResolveReference(ir::AstNode *childNode);

protected:
    template <size_t N>
    using MandatoryParams = std::array<std::string_view, N>;

    static constexpr MandatoryParams<MANDATORY_PARAMS_NUMBER> FUNCTION_MANDATORY_PARAMS = {
        MANDATORY_PARAM_FUNC, MANDATORY_PARAM_NEW_TARGET, MANDATORY_PARAM_THIS};

    static constexpr MandatoryParams<MANDATORY_PARAMS_NUMBER + 1> EVAL_SCRIPT_MANDATORY_PARAMS = {
        MANDATORY_PARAM_FUNC, MANDATORY_PARAM_NEW_TARGET, MANDATORY_PARAM_THIS, LEXICAL_CONTEXT_PARAM};

    static constexpr MandatoryParams<MANDATORY_PARAMS_NUMBER> ARROW_MANDATORY_PARAMS = {
        MANDATORY_PARAM_FUNC, LEXICAL_MANDATORY_PARAM_NEW_TARGET, LEXICAL_MANDATORY_PARAM_THIS};

    static constexpr MandatoryParams<MANDATORY_PARAMS_NUMBER> CTOR_ARROW_MANDATORY_PARAMS = {
        LEXICAL_MANDATORY_PARAM_FUNC, LEXICAL_MANDATORY_PARAM_NEW_TARGET, LEXICAL_MANDATORY_PARAM_THIS};

    void LookUpMandatoryReferences(const FunctionScope *funcScope, bool needLexicalFuncObj);
    template <size_t N>
    void AddMandatoryParams(const MandatoryParams<N> &params);
    void AddMandatoryParams();
    void LookupReference(const util::StringView &name);
    void InstantiateArguments();
    [[nodiscard]] bool InstantiateArgumentsImpl(Scope **scope, Scope *iter, const ir::AstNode *node);
    void InstantiatePrivateContext(const ir::Identifier *ident) const;
    void BuildVarDeclarator(ir::VariableDeclarator *varDecl);
    void BuildVarDeclaratorId(ir::AstNode *childNode);
    void BuildForUpdateLoop(ir::ForUpdateStatement *forUpdateStmt);
    void BuildForInOfLoop(varbinder::LoopScope *loopScope, ir::AstNode *left, ir::Expression *right,
                          ir::Statement *body);
    void BuildCatchClause(ir::CatchClause *catchClauseStmt);
    void BuildTypeAliasDeclaration(ir::TSTypeAliasDeclaration *const typeAliasDecl);
    void ResolveReferences(const ir::AstNode *parent);
    void VisitScriptFunctionWithPotentialTypeParams(ir::ScriptFunction *func);
    void VisitScriptFunction(ir::ScriptFunction *func);
    [[nodiscard]] util::StringView BuildFunctionName(util::StringView name, uint32_t idx);

    void AddCompilableFunctionScope(varbinder::FunctionScope *funcScope);

    void InitializeClassBinding(ir::ClassDefinition *classDef);
    void InitializeClassIdent(ir::ClassDefinition *classDef);

    virtual void LookupIdentReference(ir::Identifier *ident);
    virtual void HandleCustomNodes(ir::AstNode *childNode)
    {
        ResolveReferences(childNode);
    }
    virtual void BuildSignatureDeclarationBaseParams([[maybe_unused]] ir::AstNode *typeNode) {};
    virtual void BuildClassDefinition(ir::ClassDefinition *classDef);
    virtual void BuildClassProperty(const ir::ClassProperty *prop);
    [[nodiscard]] virtual bool BuildInternalName(ir::ScriptFunction *scriptFunc);
    virtual void AddCompilableFunction(ir::ScriptFunction *func);

    virtual void CleanUp()
    {
        topScope_ = nullptr;
        scope_ = nullptr;
        varScope_ = nullptr;
        functionScopes_.clear();
    }

    virtual void CopyTo(VarBinder *target)
    {
        target->program_ = program_;
        target->allocator_ = allocator_;
        target->context_ = context_;
        target->bindingOptions_ = bindingOptions_;
        target->genStdLib_ = genStdLib_;
    }

private:
    parser::Program *program_ {};
    ArenaAllocator *allocator_ {};
    public_lib::Context *context_ {};
    GlobalScope *topScope_ {};
    Scope *scope_ {};
    VariableScope *varScope_ {};
    ArenaVector<FunctionScope *> functionScopes_;
    ResolveBindingOptions bindingOptions_ {};
    bool genStdLib_ {false};
};

template <typename T>
class LexicalScope {
public:
    template <typename... Args>
    explicit LexicalScope(VarBinder *varbinder, Args &&...args)
        : LexicalScope(
              varbinder->Allocator()->New<T>(varbinder->Allocator(), varbinder->scope_, std::forward<Args>(args)...),
              varbinder)
    {
    }

    T *GetScope() const
    {
        return scope_;
    }

    ~LexicalScope()
    {
        ES2PANDA_ASSERT(varbinder_);
        varbinder_->scope_ = prevScope_;
        varbinder_->varScope_ = prevVarScope_;
    }

    [[nodiscard]] static LexicalScope<T> Enter(VarBinder *varbinder, T *scope, bool checkEval = true)
    {
        LexicalScope<T> lexScope(scope, varbinder);
        if (!checkEval || varbinder->Extension() == ScriptExtension::TS) {
            return lexScope;
        }

        // NOLINTNEXTLINE(readability-braces-around-statements)
        if constexpr (std::is_same_v<T, FunctionParamScope>) {
            varbinder->varScope_ = scope->GetFunctionScope();
            varbinder->varScope_->CheckDirectEval(varbinder->context_);
            // NOLINTNEXTLINE(readability-braces-around-statements,readability-misleading-indentation)
        } else if constexpr (std::is_same_v<T, FunctionScope>) {
            varbinder->varScope_ = scope;
            varbinder->varScope_->CheckDirectEval(varbinder->context_);
            // NOLINTNEXTLINE(readability-braces-around-statements,readability-misleading-indentation)
        } else if constexpr (std::is_same_v<T, LoopScope>) {
            if (scope->IsLoopScope()) {
                varbinder->varScope_ = scope;
                varbinder->varScope_->CheckDirectEval(varbinder->context_);
            }
            // NOLINTNEXTLINE(readability-braces-around-statements,readability-misleading-indentation)
        } else if constexpr (std::is_same_v<T, LoopDeclarationScope>) {
            if (scope->IsLoopDeclarationScope()) {
                varbinder->varScope_ = scope;
                varbinder->varScope_->CheckDirectEval(varbinder->context_);
            }
        }

        return lexScope;
    }

    DEFAULT_MOVE_SEMANTIC(LexicalScope);

private:
    NO_COPY_SEMANTIC(LexicalScope);

    explicit LexicalScope(T *scope, VarBinder *varbinder)
        : varbinder_(varbinder), scope_(scope), prevScope_(varbinder->scope_), prevVarScope_(varbinder->varScope_)
    {
        varbinder_->scope_ = scope_;
    }

    VarBinder *varbinder_ {};
    T *scope_ {};
    Scope *prevScope_ {};
    VariableScope *prevVarScope_ {};
};

template <size_t N>
void VarBinder::AddMandatoryParams(const MandatoryParams<N> &params)
{
    ES2PANDA_ASSERT(scope_->IsFunctionVariableScope());

    auto scopeCtx = LexicalScope<FunctionParamScope>::Enter(this, scope_->AsFunctionVariableScope()->ParamScope());

    for (auto iter = params.rbegin(); iter != params.rend(); ++iter) {
        AddMandatoryParam(*iter);
    }
}

template <typename T, typename... Args>
T *VarBinder::AddTsDecl(const lexer::SourcePosition &pos, Args &&...args)
{
    T *decl = Allocator()->New<T>(std::forward<Args>(args)...);

    if (scope_->AddTsDecl(Allocator(), decl, Extension()) == nullptr) {
        ThrowRedeclaration(pos, decl->Name(), decl->Type());
    }

    return decl;
}

template <typename T, typename... Args>
T *VarBinder::AddDecl(const lexer::SourcePosition &pos, Args &&...args)
{
    T *decl = Allocator()->New<T>(std::forward<Args>(args)...);

    if (scope_->AddDecl(Allocator(), decl, Extension()) == nullptr) {
        ThrowRedeclaration(pos, decl->Name(), decl->Type());
    }

    return decl;
}

template <typename T, typename... Args>
std::tuple<T *, varbinder::Variable *> VarBinder::NewVarDecl(const lexer::SourcePosition &pos, Args &&...args)
{
    T *decl = Allocator()->New<T>(std::forward<Args>(args)...);
    auto *allocator = Allocator();
    auto extension = Extension();
    varbinder::Variable *var = scope_->AddDecl(allocator, decl, extension);

    if (var == nullptr) {
        ThrowRedeclaration(pos, decl->Name(), decl->Type());
        var = scope_->FindLocal(decl->Name(), ResolveBindingOptions::BINDINGS);
    }

    ES2PANDA_ASSERT(var != nullptr);
    return {decl, var};
}

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class TopScopeContext {
public:
    TopScopeContext(VarBinder *varbinder, GlobalScope *globalScope)
        : varbinder_(varbinder),
          prevTopscope_(varbinder->TopScope()),
          prevScope_(varbinder->GetScope()),
          prevVarScope_(varbinder->VarScope())
    {
        varbinder->topScope_ = globalScope;
        varbinder->scope_ = globalScope;
        varbinder->varScope_ = globalScope;
    }

    ~TopScopeContext()
    {
        varbinder_->topScope_ = prevTopscope_;
        varbinder_->scope_ = prevScope_;
        varbinder_->varScope_ = prevVarScope_;
    }

private:
    VarBinder *varbinder_ {};
    GlobalScope *prevTopscope_ {};
    Scope *prevScope_ {};
    VariableScope *prevVarScope_ {};
};

class GlobalScopeContext {
public:
    GlobalScopeContext(VarBinder *varbinder, parser::Program *extProgram, GlobalScope *globalScope)
        : varbinder_(varbinder), savedProgram_(varbinder->Program()), prevTopscope_(varbinder->TopScope())
    {
        varbinder_->SetProgram(extProgram);
        varbinder_->SetTopScope(globalScope);
    }

    NO_COPY_SEMANTIC(GlobalScopeContext);
    NO_MOVE_SEMANTIC(GlobalScopeContext);

    ~GlobalScopeContext()
    {
        varbinder_->SetProgram(savedProgram_);
        varbinder_->SetTopScope(prevTopscope_);
    }

private:
    VarBinder *varbinder_ {};
    parser::Program *savedProgram_ {};
    GlobalScope *prevTopscope_ {};
};

}  // namespace ark::es2panda::varbinder
#endif
