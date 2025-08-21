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

#include "scope.h"

#include "public/public.h"
#include "varbinder/tsBinding.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::varbinder {
VariableScope *Scope::EnclosingVariableScope() noexcept
{
    return const_cast<VariableScope *>(const_cast<Scope const *>(this)->EnclosingVariableScope());
}

const VariableScope *Scope::EnclosingVariableScope() const noexcept
{
    const auto *iter = this;

    while (iter != nullptr) {
        if (iter->IsVariableScope()) {
            return iter->AsVariableScope();
        }

        iter = iter->Parent();
    }

    return nullptr;
}

bool Scope::IsSuperscopeOf(const varbinder::Scope *subscope) const noexcept
{
    while (subscope != nullptr) {
        if (subscope == this) {
            return true;
        }
        subscope = ir::AstNode::EnclosingScope(subscope->Node()->Parent());
    }
    return false;
}

Variable *Scope::FindLocal(const util::StringView &name, ResolveBindingOptions options) const
{
    if ((options & ResolveBindingOptions::INTERFACES) != 0) {
        std::string tsBindingName = varbinder::TSBinding::ToTSBinding(name);
        util::StringView interfaceNameView(tsBindingName);

        auto res = bindings_.find(interfaceNameView);
        if (res != bindings_.end()) {
            return res->second;
        }

        if ((options & ResolveBindingOptions::BINDINGS) == 0) {
            return nullptr;
        }
    }

    auto res = bindings_.find(name);
    if (res == bindings_.end()) {
        return nullptr;
    }

    return res->second;
}

bool Scope::CorrectForeignBinding(const util::StringView &name, Variable *builtinVar, Variable *redefinedVar)
{
    ES2PANDA_ASSERT(builtinVar != redefinedVar);
    auto bindingTobeErase = bindings_.find(name);
    if (bindingTobeErase == bindings_.end()) {
        return false;
    }

    Variable *varTobeErase = bindingTobeErase->second;
    if (varTobeErase != redefinedVar) {
        return false;
    }

    auto declTobeErase = redefinedVar->Declaration();
    bindings_.erase(name);
    auto it = std::find_if(decls_.begin(), decls_.end(), [&](auto *decl) { return decl == declTobeErase; });
    ES2PANDA_ASSERT(it != decls_.end());
    decls_.erase(it);
    return Scope::InsertBinding(name, builtinVar).second;
}

Scope::InsertResult Scope::InsertBinding(const util::StringView &name, Variable *const var)
{
    ES2PANDA_ASSERT(var != nullptr);
    auto insertResult = bindings_.emplace(name, var);
    if (insertResult.second) {
        decls_.emplace_back(var->Declaration());
    }
    return insertResult;
}

Scope::InsertResult Scope::InsertOrAssignBinding(const util::StringView &name, Variable *const var)
{
    ES2PANDA_ASSERT(var != nullptr);
    auto insertResult = bindings_.insert_or_assign(name, var);
    if (insertResult.second) {
        decls_.emplace_back(var->Declaration());
    }
    return insertResult;
}

Scope::InsertResult Scope::TryInsertBinding(const util::StringView &name, Variable *const var)
{
    ES2PANDA_ASSERT(var != nullptr);
    return bindings_.try_emplace(name, var);
}

void Scope::MergeBindings(VariableMap const &bindings)
{
    for (auto &[k, v] : bindings) {
        bindings_.try_emplace(k, v);
    }
}

Scope::VariableMap::size_type Scope::EraseBinding(const util::StringView &name)
{
    if (auto toBeErased = bindings_.find(name);
        toBeErased == bindings_.end() ||
        (toBeErased->second->IsLocalVariable() &&
         toBeErased->second->AsLocalVariable()->Declaration()->Node()->IsImportNamespaceSpecifier())) {
        return 0U;
    }

    return bindings_.erase(name);
}

ConstScopeFindResult Scope::FindInGlobal(const util::StringView &name, const ResolveBindingOptions options) const
{
    const auto *scopeIter = this;
    Variable *resolved = nullptr;
    while (scopeIter != nullptr && !scopeIter->IsGlobalScope()) {
        bool isModule = scopeIter->Node() != nullptr && scopeIter->Node()->IsClassDefinition() &&
                        scopeIter->Node()->AsClassDefinition()->IsModule();
        if (isModule) {
            resolved = scopeIter->FindLocal(name, options);
            if (resolved != nullptr) {
                break;
            }
        }
        scopeIter = scopeIter->Parent();
    }
    if (resolved == nullptr && scopeIter != nullptr && scopeIter->IsGlobalScope()) {
        resolved = scopeIter->FindLocal(name, options);
    }
    return {name, scopeIter, 0, 0, resolved};
}

ConstScopeFindResult Scope::FindInFunctionScope(const util::StringView &name, const ResolveBindingOptions options) const
{
    const auto *scopeIter = this;
    while (scopeIter != nullptr && !scopeIter->IsGlobalScope()) {
        if (!scopeIter->IsClassScope()) {
            if (auto *const resolved = scopeIter->FindLocal(name, options); resolved != nullptr) {
                return ConstScopeFindResult(name, scopeIter, 0, 0, resolved);
            }
        }
        scopeIter = scopeIter->Parent();
    }

    return ConstScopeFindResult(name, scopeIter, 0, 0, nullptr);
}

ScopeFindResult Scope::Find(const util::StringView &name, const ResolveBindingOptions options)
{
    return FindImpl<ScopeFindResult>(this, name, options);
}

ConstScopeFindResult Scope::Find(const util::StringView &name, const ResolveBindingOptions options) const
{
    return FindImpl<ConstScopeFindResult>(this, name, options);
}

Decl *Scope::FindDecl(const util::StringView &name) const noexcept
{
    for (auto *it : decls_) {
        if (it->Name() == name) {
            return it;
        }
    }

    return nullptr;
}

std::tuple<Scope *, bool> Scope::IterateShadowedVariables(const util::StringView &name, const VariableVisitor &visitor)
{
    auto *iter = this;

    while (iter != nullptr) {
        auto *v = iter->FindLocal(name, varbinder::ResolveBindingOptions::BINDINGS);

        if (v != nullptr && visitor(v)) {
            return {iter, true};
        }

        if (iter->IsFunctionVariableScope()) {
            break;
        }

        iter = iter->Parent();
    }

    return {iter, false};
}

Variable *Scope::AddLocalVar(ArenaAllocator *allocator, Decl *newDecl)
{
    auto [scope, shadowed] =
        IterateShadowedVariables(newDecl->Name(), [](const Variable *v) { return !v->HasFlag(VariableFlags::VAR); });

    if (shadowed) {
        return nullptr;
    }

    VariableFlags varFlags = VariableFlags::HOIST_VAR | VariableFlags::LEXICAL_VAR;
    if (scope->IsGlobalScope()) {
        return scope->InsertBinding(newDecl->Name(), allocator->New<GlobalVariable>(newDecl, varFlags)).first->second;
    }

    return PropagateBinding<LocalVariable>(allocator, newDecl->Name(), newDecl, varFlags);
}

Variable *Scope::AddLocalInterfaceVariable(ArenaAllocator *allocator, Decl *newDecl)
{
    auto [iter, inserted] =
        bindings_.try_emplace(newDecl->Name(), allocator->New<LocalVariable>(newDecl, VariableFlags::INTERFACE));
    if (!inserted) {
        return nullptr;
    }

    auto *var = iter->second;
    if (newDecl->Node() != nullptr) {
        newDecl->Node()->AsTSInterfaceDeclaration()->Id()->SetVariable(var);
    }
    return var;
}

Variable *Scope::AddLocalTypeAliasVariable(ArenaAllocator *allocator, Decl *newDecl)
{
    auto *var = bindings_.insert({newDecl->Name(), allocator->New<LocalVariable>(newDecl, VariableFlags::TYPE_ALIAS)})
                    .first->second;
    newDecl->Node()->AsTSTypeAliasDeclaration()->Id()->SetVariable(var);
    return var;
}

Variable *Scope::AddLocalClassVariable(ArenaAllocator *allocator, Decl *newDecl)
{
    auto isNamespaceTransformed = newDecl->Node()->AsClassDefinition()->IsNamespaceTransformed();
    auto isEnumTransformed = newDecl->Node()->AsClassDefinition()->IsEnumTransformed();
    VariableFlags flag = isNamespaceTransformed ? VariableFlags::NAMESPACE
                         : isEnumTransformed    ? VariableFlags::ENUM_LITERAL
                                                : VariableFlags::CLASS;
    auto [iter, inserted] = bindings_.try_emplace(newDecl->Name(), allocator->New<LocalVariable>(newDecl, flag));
    if (!inserted) {
        return nullptr;
    }

    auto *var = iter->second;
    newDecl->Node()->AsClassDefinition()->Ident()->SetVariable(var);
    return var;
}

Variable *Scope::AddLocal(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                          [[maybe_unused]] ScriptExtension extension)
{
    VariableFlags flags = VariableFlags::LEXICAL;
    switch (newDecl->Type()) {
        case DeclType::VAR: {
            return AddLocalVar(allocator, newDecl);
        }
        case DeclType::ENUM: {
            return bindings_.insert({newDecl->Name(), allocator->New<EnumVariable>(newDecl, false)}).first->second;
        }
        case DeclType::ENUM_LITERAL: {
            auto *var =
                bindings_.insert({newDecl->Name(), allocator->New<LocalVariable>(newDecl, VariableFlags::ENUM_LITERAL)})
                    .first->second;
            newDecl->Node()->AsTSEnumDeclaration()->Key()->SetVariable(var);
            return var;
        }
        case DeclType::INTERFACE: {
            return AddLocalInterfaceVariable(allocator, newDecl);
        }
        case DeclType::CLASS: {
            return AddLocalClassVariable(allocator, newDecl);
        }
        case DeclType::TYPE_ALIAS: {
            return AddLocalTypeAliasVariable(allocator, newDecl);
        }
        case DeclType::TYPE_PARAMETER: {
            return bindings_
                .insert({newDecl->Name(), allocator->New<LocalVariable>(newDecl, VariableFlags::TYPE_PARAMETER)})
                .first->second;
        }
        case DeclType::FUNC: {
            flags = VariableFlags::HOIST;
            [[fallthrough]];
        }
        default: {
            if (currentVariable != nullptr) {
                return nullptr;
            }

            auto [_, shadowed] = IterateShadowedVariables(
                newDecl->Name(), [](const Variable *v) { return v->HasFlag(VariableFlags::LEXICAL_VAR); });
            (void)_;

            if (shadowed) {
                return nullptr;
            }

            return bindings_.insert({newDecl->Name(), allocator->New<LocalVariable>(newDecl, flags)}).first->second;
        }
    }
}

void VariableScope::CheckDirectEval(public_lib::Context *context)
{
    ES2PANDA_ASSERT(context);
    const auto &varMap = Bindings();

    if (!HasFlag(ScopeFlags::NO_REG_STORE) || varMap.empty()) {
        evalBindings_ = compiler::INVALID_LITERAL_BUFFER_ID;
        return;
    }

    size_t constBindings = 0;
    for (const auto &[name, var] : varMap) {
        (void)name;
        var->SetLexical(this);

        if (var->LexicalBound() && var->Declaration()->IsConstDecl()) {
            constBindings++;
        }
    }

    std::vector<compiler::Literal> literals(LexicalSlots() + constBindings, compiler::Literal(util::StringView()));

    if (constBindings == 0U) {
        for (const auto &[name, variable] : varMap) {
            if (!variable->LexicalBound()) {
                continue;
            }

            literals[variable->AsLocalVariable()->LexIdx()] = compiler::Literal(name);
        }
    } else {
        std::vector<varbinder::Variable *> bindings(LexicalSlots());

        for (const auto &[name, variable] : varMap) {
            (void)name;
            if (!variable->LexicalBound()) {
                continue;
            }

            bindings[variable->AsLocalVariable()->LexIdx()] = variable;
        }

        uint32_t buffIndex = 0;
        for (const auto *variable : bindings) {
            if (variable == nullptr) {
                ES2PANDA_ASSERT(literals[buffIndex].GetString().empty());
                buffIndex++;
                continue;
            }
            if (variable->Declaration()->IsConstDecl()) {
                literals[buffIndex++] = compiler::Literal(true);
            }
            literals[buffIndex++] = compiler::Literal(variable->Name());
        }
    }
    context->contextLiterals.emplace_back(literals);
    evalBindings_ = context->contextLiterals.size() - 1;
}

template <typename T>
Variable *VariableScope::AddVar(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                                [[maybe_unused]] ScriptExtension extension)
{
    if (!currentVariable) {
        return InsertBinding(newDecl->Name(), allocator->New<T>(newDecl, VariableFlags::HOIST_VAR)).first->second;
    }

    switch (currentVariable->Declaration()->Type()) {
        case DeclType::VAR: {
            currentVariable->Reset(newDecl, VariableFlags::HOIST_VAR);
            [[fallthrough]];
        }
        case DeclType::PARAM:
        case DeclType::FUNC: {
            return currentVariable;
        }
        default: {
            ES2PANDA_ASSERT(extension == ScriptExtension::JS);
            return nullptr;
        }
    }
}

template <typename T>
Variable *VariableScope::AddFunction(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                                     ScriptExtension extension)
{
    VariableFlags flags = (extension == ScriptExtension::JS) ? VariableFlags::HOIST_VAR : VariableFlags::HOIST;

    if (!currentVariable) {
        return InsertBinding(newDecl->Name(), allocator->New<T>(newDecl, flags)).first->second;
    }

    if (extension != ScriptExtension::JS || IsModuleScope()) {
        return nullptr;
    }

    switch (currentVariable->Declaration()->Type()) {
        case DeclType::VAR:
        case DeclType::FUNC: {
            currentVariable->Reset(newDecl, VariableFlags::HOIST_VAR);
            return currentVariable;
        }
        default: {
            ES2PANDA_ASSERT(extension == ScriptExtension::JS);
            return nullptr;
        }
    }
}

template <typename T>
Variable *VariableScope::AddTSBinding(ArenaAllocator *allocator, [[maybe_unused]] Variable *currentVariable,
                                      Decl *newDecl, VariableFlags flags)
{
    ES2PANDA_ASSERT(!currentVariable);
    return InsertBinding(newDecl->Name(), allocator->New<T>(newDecl, flags)).first->second;
}

template <typename T>
Variable *VariableScope::AddLexical(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl)
{
    if (currentVariable) {
        return nullptr;
    }

    return InsertBinding(newDecl->Name(), allocator->New<T>(newDecl, VariableFlags::NONE)).first->second;
}

Variable *ParamScope::AddParameter(ArenaAllocator *allocator, Decl *newDecl, VariableFlags flags)
{
    ES2PANDA_ASSERT(newDecl->IsParameterDecl());

    auto *param = allocator->New<LocalVariable>(newDecl, flags);
    ES2PANDA_ASSERT(param != nullptr);
    param->SetScope(this);

    params_.emplace_back(param);
    InsertBinding(newDecl->Name(), param);

    return param;
}

std::tuple<Variable *, ir::Expression *> ParamScope::AddParamDecl(ArenaAllocator *allocator, varbinder::VarBinder *vb,
                                                                  ir::Expression *parameter)
{
    auto [name, pattern] = util::Helpers::ParamName(allocator, parameter, params_.size());
    if (name.Is(ERROR_LITERAL)) {
        name = compiler::GenName(allocator).View();
    }

    auto *variable = FindLocal(name, varbinder::ResolveBindingOptions::BINDINGS);
    if (variable != nullptr) {
        return std::make_tuple(variable, parameter);
    }

    if (pattern) {
        std::vector<ir::Identifier *> bindings = util::Helpers::CollectBindingNames(vb, parameter);

        for (auto *binding : bindings) {
            auto *varDecl = NewDecl<VarDecl>(allocator, binding->Name());
            varDecl->BindNode(binding);

            if (variable = FindLocal(varDecl->Name(), varbinder::ResolveBindingOptions::BINDINGS);
                variable != nullptr) {
                return std::make_tuple(variable, binding);
            }

            auto *paramVar = allocator->New<LocalVariable>(varDecl, VariableFlags::VAR | VariableFlags::LOCAL);
            TryInsertBinding(varDecl->Name(), paramVar);
        }
    }

    auto *const decl = NewDecl<ParameterDecl>(allocator, name);
    variable = AddParameter(allocator, decl, VariableFlags::VAR | VariableFlags::LOCAL);
    decl->BindNode(parameter);

    return std::make_tuple(variable, nullptr);
}

void FunctionParamScope::BindName(ArenaAllocator *allocator, util::StringView name)
{
    auto [variable, _] = AddDecl<ConstDecl, LocalVariable>(allocator, name, VariableFlags::INITIALIZED);
    nameVar_ = functionScope_->InsertBinding(name, variable).second ? variable->AsLocalVariable() : nullptr;
}

Variable *FunctionParamScope::AddBinding([[maybe_unused]] ArenaAllocator *allocator,
                                         [[maybe_unused]] Variable *currentVariable, [[maybe_unused]] Decl *newDecl,
                                         [[maybe_unused]] ScriptExtension extension)
{
    ES2PANDA_UNREACHABLE();
}

Variable *AnnotationParamScope::AddBinding([[maybe_unused]] ArenaAllocator *allocator,
                                           [[maybe_unused]] Variable *currentVariable, [[maybe_unused]] Decl *newDecl,
                                           [[maybe_unused]] ScriptExtension extension)
{
    auto *ident = newDecl->Node()->AsClassProperty()->Id();
    ES2PANDA_ASSERT(ident != nullptr);
    auto annoVar = allocator->New<LocalVariable>(newDecl, VariableFlags::PROPERTY);
    auto var = InsertBinding(ident->Name(), annoVar).first->second;
    if (var != nullptr) {
        var->SetScope(this);
        ident->SetVariable(var);
    }
    return var;
}

Variable *FunctionScope::InsertBindingIfAbsentInScope(ArenaAllocator *allocator, Variable *currentVariable,
                                                      Decl *newDecl, VariableFlags flag)
{
    if (currentVariable != nullptr) {
        return nullptr;
    }
    return InsertBinding(newDecl->Name(), allocator->New<LocalVariable>(newDecl, flag)).first->second;
}

Variable *FunctionScope::AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                                    [[maybe_unused]] ScriptExtension extension)
{
    ir::Identifier *ident {};
    Variable *var {};
    switch (newDecl->Type()) {
        case DeclType::VAR: {
            return AddVar<LocalVariable>(allocator, currentVariable, newDecl, extension);
        }
        case DeclType::FUNC: {
            return AddFunction<LocalVariable>(allocator, currentVariable, newDecl, extension);
        }
        case DeclType::ENUM: {
            return InsertBinding(newDecl->Name(), allocator->New<EnumVariable>(newDecl, false)).first->second;
        }
        case DeclType::ENUM_LITERAL: {
            var = AddTSBinding<LocalVariable>(allocator, currentVariable, newDecl, VariableFlags::ENUM_LITERAL);
            ident = newDecl->Node()->AsTSEnumDeclaration()->Key();
            break;
        }
        // NOTE(psiket):Duplication
        case DeclType::INTERFACE: {
            ident = newDecl->Node()->AsTSInterfaceDeclaration()->Id();
            var = InsertBindingIfAbsentInScope(allocator, currentVariable, newDecl, VariableFlags::INTERFACE);
            break;
        }
        case DeclType::CLASS: {
            auto isNamespaceTransformed = newDecl->Node()->AsClassDefinition()->IsNamespaceTransformed();
            VariableFlags flag = isNamespaceTransformed ? VariableFlags::NAMESPACE : VariableFlags::CLASS;
            ident = newDecl->Node()->AsClassDefinition()->Ident();
            var = InsertBindingIfAbsentInScope(allocator, currentVariable, newDecl, flag);
            break;
        }
        case DeclType::TYPE_ALIAS: {
            ident = newDecl->Node()->AsTSTypeAliasDeclaration()->Id();
            var = InsertBindingIfAbsentInScope(allocator, currentVariable, newDecl, VariableFlags::TYPE_ALIAS);
            break;
        }
        default: {
            return AddLexical<LocalVariable>(allocator, currentVariable, newDecl);
        }
    }
    if (var != nullptr) {
        var->SetScope(this);
        if (ident != nullptr) {
            ident->SetVariable(var);
        }
    }
    return var;
}

Variable *GlobalScope::AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                                  [[maybe_unused]] ScriptExtension extension)
{
    switch (newDecl->Type()) {
        case DeclType::VAR: {
            return AddVar<GlobalVariable>(allocator, currentVariable, newDecl, extension);
        }
        case DeclType::FUNC: {
            return AddFunction<GlobalVariable>(allocator, currentVariable, newDecl, extension);
        }
        case DeclType::ENUM: {
            return InsertBinding(newDecl->Name(), allocator->New<EnumVariable>(newDecl, false)).first->second;
        }
        case DeclType::ENUM_LITERAL: {
            return AddTSBinding<LocalVariable>(allocator, currentVariable, newDecl, VariableFlags::ENUM_LITERAL);
        }
        case DeclType::INTERFACE: {
            return AddTSBinding<LocalVariable>(allocator, currentVariable, newDecl, VariableFlags::INTERFACE);
        }
        default: {
            return AddLexical<LocalVariable>(allocator, currentVariable, newDecl);
        }
    }
}

Scope::InsertResult GlobalScope::InsertBinding(const util::StringView &name, Variable *const var)
{
    return GlobalScope::InsertImpl(name, var, InsertBindingFlags::NONE);
}

Scope::InsertResult GlobalScope::TryInsertBinding(const util::StringView &name, Variable *const var)
{
    const auto insRes = Scope::TryInsertBinding(name, var);
    if (insRes.second) {
        [[maybe_unused]] const bool insertSuccess = std::get<1>(foreignBindings_.try_emplace(name, var));
        ES2PANDA_ASSERT(insertSuccess);
    }

    return insRes;
}

void GlobalScope::MergeBindings([[maybe_unused]] const VariableMap &bindings)
{
    ES2PANDA_UNREACHABLE();
}

Scope::VariableMap::size_type GlobalScope::EraseBinding(const util::StringView &name)
{
    const auto erased = Scope::EraseBinding(name);
    if (erased != 0) {
        [[maybe_unused]] const auto erasedForeign = foreignBindings_.erase(name);
        ES2PANDA_ASSERT(erasedForeign != 0);
    }

    return erased;
}

Scope::InsertResult GlobalScope::InsertForeignBinding(const util::StringView &name, Variable *const var)
{
    return GlobalScope::InsertImpl(name, var, InsertBindingFlags::FOREIGN);
}

Scope::InsertResult GlobalScope::InsertOrAssignForeignBinding(const util::StringView &name, Variable *const var)
{
    return GlobalScope::InsertImpl(name, var, InsertBindingFlags::FOREIGN | InsertBindingFlags::ASSIGN);
}

bool GlobalScope::CorrectForeignBinding(const util::StringView &name, Variable *builtinVar, Variable *redefinedVar)
{
    const bool deleteRes = Scope::CorrectForeignBinding(name, builtinVar, redefinedVar);
    if (deleteRes) {
        foreignBindings_[name] = true;
    }
    return deleteRes;
}

Scope::InsertResult GlobalScope::InsertImpl(const util::StringView &name, Variable *const var,
                                            const InsertBindingFlags flags)
{
    bool isAssign = (flags & InsertBindingFlags::ASSIGN) != 0;
    bool isDynamic = (flags & InsertBindingFlags::DYNAMIC) != 0;
    bool isForeign = (flags & InsertBindingFlags::FOREIGN) != 0;

    if (!isDynamic && isForeign && !var->Declaration()->Name().Is(compiler::Signatures::ETS_GLOBAL)) {
        ES2PANDA_ASSERT(var->Declaration()->Name().Utf8().find(compiler::Signatures::ETS_GLOBAL) == std::string::npos);
        const auto *const node = var->Declaration()->Node();

        if (!(node->IsExported() || node->IsDefaultExported() || node->HasExportAlias())) {
            return Scope::InsertResult {Bindings().end(), false};
        }
    }

    if (isAssign) {
        const auto insRes = Scope::InsertOrAssignBinding(name, var);
        foreignBindings_.insert_or_assign(name, isForeign);
        return insRes;
    }

    const auto insRes = Scope::InsertBinding(name, var);
    if (insRes.second) {
        [[maybe_unused]] const bool insertSuccess = std::get<1>(foreignBindings_.emplace(name, isForeign));
        ES2PANDA_ASSERT(insertSuccess);
    }
    return insRes;
}

bool GlobalScope::IsForeignBinding(const util::StringView &name) const
{
    // Asserts make sure that the passed in key comes from this scope
    ES2PANDA_ASSERT(Bindings().find(name) != Bindings().end());
    ES2PANDA_ASSERT(foreignBindings_.find(name) != foreignBindings_.end());

    return foreignBindings_.at(name);
}

Scope::InsertResult GlobalScope::InsertDynamicBinding(const util::StringView &name, Variable *const var)
{
    return InsertImpl(name, var, InsertBindingFlags::FOREIGN | InsertBindingFlags::DYNAMIC);
}

// ModuleScope

Variable *ModuleScope::AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                                  [[maybe_unused]] ScriptExtension extension)
{
    switch (newDecl->Type()) {
        case DeclType::VAR: {
            return AddVar<LocalVariable>(allocator, currentVariable, newDecl, extension);
        }
        case DeclType::FUNC: {
            return AddFunction<LocalVariable>(allocator, currentVariable, newDecl, extension);
        }
        case DeclType::ENUM: {
            return InsertBinding(newDecl->Name(), allocator->New<EnumVariable>(newDecl, false)).first->second;
        }
        case DeclType::ENUM_LITERAL: {
            return AddTSBinding<LocalVariable>(allocator, currentVariable, newDecl, VariableFlags::ENUM_LITERAL);
        }
        case DeclType::INTERFACE: {
            return AddTSBinding<LocalVariable>(allocator, currentVariable, newDecl, VariableFlags::INTERFACE);
        }
        case DeclType::IMPORT: {
            return AddImport(allocator, currentVariable, newDecl);
        }
        case DeclType::EXPORT: {
            return allocator->New<LocalVariable>(newDecl, VariableFlags::NONE);
        }
        default: {
            return AddLexical<LocalVariable>(allocator, currentVariable, newDecl);
        }
    }
}

void ModuleScope::AddImportDecl(ir::ImportDeclaration *importDecl, ImportDeclList &&decls)
{
    auto res = imports_.emplace_back(importDecl, decls);

    for (auto &decl : res.second) {
        decl->BindNode(importDecl);
    }
}

void ModuleScope::AddExportDecl(ir::AstNode *exportDecl, ExportDecl *decl)
{
    ES2PANDA_ASSERT(decl != nullptr);
    decl->BindNode(exportDecl);

    ArenaVector<ExportDecl *> decls(allocator_->Adapter());
    decls.push_back(decl);

    AddExportDecl(exportDecl, std::move(decls));
}

void ModuleScope::AddExportDecl(ir::AstNode *exportDecl, ExportDeclList &&decls)
{
    auto res = exports_.emplace_back(exportDecl, decls);

    for (auto &decl : res.second) {
        decl->BindNode(exportDecl);
    }
}

Variable *ModuleScope::AddImport(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl)
{
    if (currentVariable != nullptr && currentVariable->Declaration()->Type() != DeclType::VAR) {
        return nullptr;
    }

    if (newDecl->Node()->IsImportNamespaceSpecifier()) {
        return InsertBinding(newDecl->Name(), allocator->New<LocalVariable>(newDecl, VariableFlags::READONLY))
            .first->second;
    }

    auto *variable = allocator->New<ModuleVariable>(newDecl, VariableFlags::NONE);
    ES2PANDA_ASSERT(variable != nullptr);
    variable->ExoticName() = newDecl->AsImportDecl()->ImportName();
    InsertBinding(newDecl->Name(), variable);
    return variable;
}

bool ModuleScope::ExportAnalysis()
{
    std::set<util::StringView> exportedNames;

    for (const auto &[exportDecl, decls] : exports_) {
        if (exportDecl->IsExportAllDeclaration()) {
            const auto *exportAllDecl = exportDecl->AsExportAllDeclaration();

            if (exportAllDecl->Exported() == nullptr) {
                continue;
            }

            auto result = exportedNames.insert(exportAllDecl->Exported()->Name());
            if (!result.second) {
                return false;
            }

            continue;
        }

        if (exportDecl->IsExportNamedDeclaration()) {
            const auto *exportNamedDecl = exportDecl->AsExportNamedDeclaration();

            if (exportNamedDecl->Source() != nullptr) {
                continue;
            }
        }

        for (const auto *decl : decls) {
            varbinder::Variable *variable = FindLocal(decl->LocalName(), varbinder::ResolveBindingOptions::BINDINGS);

            if (variable == nullptr) {
                continue;
            }

            auto result = exportedNames.insert(decl->ExportName());
            if (!result.second) {
                return false;
            }

            if (!variable->IsModuleVariable()) {
                variable->AddFlag(VariableFlags::LOCAL_EXPORT);
                localExports_.insert({variable, decl->ExportName()});
            }
        }
    }

    return true;
}

// LocalScope

Variable *LocalScope::AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                                 [[maybe_unused]] ScriptExtension extension)
{
    return AddLocal(allocator, currentVariable, newDecl, extension);
}

Variable *ClassScope::FindLocal(const util::StringView &name, ResolveBindingOptions options) const
{
    if ((options & ResolveBindingOptions::TYPE_ALIASES) != 0) {
        auto found = typeAliasScope_->Bindings().find(name);
        if (found != typeAliasScope_->Bindings().end()) {
            return found->second;
        }
    }

    if ((options & ResolveBindingOptions::VARIABLES) != 0) {
        auto found = instanceFieldScope_->Bindings().find(name);
        if (found != instanceFieldScope_->Bindings().end()) {
            return found->second;
        }
    }

    if ((options & ResolveBindingOptions::STATIC_VARIABLES) != 0) {
        auto found = staticFieldScope_->Bindings().find(name);
        if (found != staticFieldScope_->Bindings().end()) {
            return found->second;
        }
    }

    if ((options & ResolveBindingOptions::DECLARATION) != 0) {
        auto found = instanceDeclScope_->Bindings().find(name);
        if (found != instanceDeclScope_->Bindings().end()) {
            return found->second;
        }
    }

    if ((options & ResolveBindingOptions::STATIC_DECLARATION) != 0) {
        auto found = staticDeclScope_->Bindings().find(name);
        if (found != staticDeclScope_->Bindings().end()) {
            return found->second;
        }
    }

    if ((options & ResolveBindingOptions::METHODS) != 0) {
        auto found = instanceMethodScope_->Bindings().find(name);
        if (found != instanceMethodScope_->Bindings().end()) {
            return found->second;
        }
    }

    if ((options & ResolveBindingOptions::STATIC_METHODS) != 0) {
        auto found = staticMethodScope_->Bindings().find(name);
        if (found != staticMethodScope_->Bindings().end()) {
            return found->second;
        }
    }

    return nullptr;
}

void ClassScope::SetBindingProps(Decl *newDecl, BindingProps *props, bool isStatic)
{
    if (newDecl->IsImportDecl()) {
        return;
    }

    switch (newDecl->Type()) {
        case DeclType::CONST:
            [[fallthrough]];
        case DeclType::READONLY:
            [[fallthrough]];
        case DeclType::LET:
            if (newDecl->Node()->IsClassProperty()) {
                props->SetBindingProps(VariableFlags::PROPERTY, newDecl->Node()->AsClassProperty()->Id(),
                                       isStatic ? staticFieldScope_ : instanceFieldScope_);
            }
            break;

        case DeclType::INTERFACE:
            props->SetBindingProps(VariableFlags::INTERFACE, newDecl->Node()->AsTSInterfaceDeclaration()->Id(),
                                   isStatic ? staticDeclScope_ : instanceDeclScope_);
            break;

        case DeclType::CLASS: {
            VariableFlags flag = VariableFlags::CLASS;
            if (newDecl->Node()->AsClassDefinition()->IsNamespaceTransformed()) {
                flag = VariableFlags::NAMESPACE;
            }
            props->SetBindingProps(flag, newDecl->Node()->AsClassDefinition()->Ident(),
                                   isStatic ? staticDeclScope_ : instanceDeclScope_);
            break;
        }

        case DeclType::ENUM_LITERAL:
            props->SetBindingProps(VariableFlags::ENUM_LITERAL, newDecl->Node()->AsTSEnumDeclaration()->Key(),
                                   isStatic ? staticDeclScope_ : instanceDeclScope_);
            break;

        case DeclType::TYPE_ALIAS:
            props->SetBindingProps(VariableFlags::TYPE_ALIAS, newDecl->Node()->AsTSTypeAliasDeclaration()->Id(),
                                   typeAliasScope_);
            break;

        case DeclType::ANNOTATIONDECL:
            props->SetBindingProps(VariableFlags::ANNOTATIONDECL,
                                   newDecl->Node()->AsAnnotationDeclaration()->GetBaseName(),
                                   isStatic ? staticDeclScope_ : instanceDeclScope_);
            break;

        case DeclType::ANNOTATIONUSAGE:
            props->SetBindingProps(VariableFlags::ANNOTATIONUSAGE, newDecl->Node()->AsAnnotationUsage()->GetBaseName(),
                                   isStatic ? staticDeclScope_ : instanceDeclScope_);
            break;

        default:
            ES2PANDA_UNREACHABLE();
    }
}

Variable *ClassScope::AddBinding(ArenaAllocator *allocator, [[maybe_unused]] Variable *currentVariable, Decl *newDecl,
                                 [[maybe_unused]] ScriptExtension extension)
{
    bool isStatic = newDecl->Node()->IsStatic();
    BindingProps props;

    if (isStatic) {
        props.SetFlagsType(VariableFlags::STATIC);
    }

    SetBindingProps(newDecl, &props, isStatic);
    if (props.GetTargetScope() == nullptr) {
        return nullptr;
    }

    // First search
    const auto *foundVar = FindLocal(newDecl->Name(), ResolveBindingOptions::ALL);
    if (foundVar != nullptr) {
        // Found potential conflict in the current scope
        if (!newDecl->IsLetOrConstDecl() && !newDecl->Node()->IsStatement()) {
            return nullptr;
        }

        // Non-static variables can coexist with static variables and static methods and vice versa.
        // Run second search with filtered options.
        const auto excludeOptions =
            isStatic ? ResolveBindingOptions::VARIABLES | ResolveBindingOptions::METHODS
                     : ResolveBindingOptions::STATIC_VARIABLES | ResolveBindingOptions::STATIC_METHODS;
        const auto options = ResolveBindingOptions::ALL ^ excludeOptions;
        foundVar = FindLocal(newDecl->Name(), options);
        if (foundVar != nullptr) {
            // Conflict in bindings
            return nullptr;
        }
    }

    auto *var = props.GetTargetScope()->AddBinding(allocator, nullptr, newDecl, extension);
    if (var == nullptr) {
        return nullptr;
    }

    if (auto node = newDecl->Node(); node->IsStatement() && (node->AsStatement()->IsMethodDefinition() ||
                                                             node->IsClassProperty() || node->IsClassStaticBlock())) {
        if (node->AsStatement()->AsClassElement()->Value() != nullptr) {
            props.SetFlagsType(VariableFlags::INITIALIZED);
        }

        if (node->IsClassProperty() && node->AsClassProperty()->NeedInitInStaticBlock()) {
            props.SetFlagsType(VariableFlags::INIT_IN_STATIC_BLOCK);
        }
    }

    var->SetScope(this);
    var->AddFlag(props.GetFlags());

    if (props.GetIdent() != nullptr) {
        props.GetIdent()->SetVariable(var);
    }

    return var;
}

void LoopDeclarationScope::ConvertToVariableScope(ArenaAllocator *allocator)
{
    if (NeedLexEnv()) {
        return;
    }

    const auto &bindings = Bindings();
    for (auto &[name, var] : bindings) {
        if (!var->LexicalBound() || !var->Declaration()->IsLetOrConstDecl()) {
            continue;
        }

        slotIndex_++;
        loopType_ = ScopeType::LOOP_DECL;
        auto *copiedVar = var->AsLocalVariable()->Copy(allocator, var->Declaration());
        ES2PANDA_ASSERT(copiedVar != nullptr);
        copiedVar->AddFlag(VariableFlags::INITIALIZED | VariableFlags::PER_ITERATION);
        var->AddFlag(VariableFlags::LOOP_DECL);
        loopScope_->InsertBinding(name, copiedVar);
    }

    if (loopType_ == ScopeType::LOOP_DECL) {
        auto *parentVarScope = Parent()->EnclosingVariableScope();
        ES2PANDA_ASSERT(parentVarScope != nullptr);
        slotIndex_ = std::max(slotIndex_, parentVarScope->LexicalSlots());
        evalBindings_ = parentVarScope->EvalBindings();
        initScope_ = allocator->New<LocalScope>(allocator, Parent());
        ES2PANDA_ASSERT(initScope_ != nullptr);
        initScope_->BindNode(Node());
        initScope_->MergeBindings(bindings);
    }
}

void LoopScope::ConvertToVariableScope(ArenaAllocator *allocator)
{
    declScope_->ConvertToVariableScope(allocator);

    if (loopType_ != ScopeType::LOCAL) {
        return;
    }

    for (const auto &[_, var] : Bindings()) {
        (void)_;
        if (var->LexicalBound() && var->Declaration()->IsLetDecl()) {
            ES2PANDA_ASSERT(declScope_->NeedLexEnv());
            loopType_ = ScopeType::LOOP;
            break;
        }
    }

    if (loopType_ == ScopeType::LOOP) {
        slotIndex_ = std::max(slotIndex_, declScope_->LexicalSlots());
        evalBindings_ = declScope_->EvalBindings();
    }
}

Variable *CatchParamScope::AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                                      [[maybe_unused]] ScriptExtension extension)
{
    return currentVariable != nullptr ? nullptr : AddParameter(allocator, newDecl, VariableFlags::INITIALIZED);
}

Variable *CatchScope::AddBinding(ArenaAllocator *allocator, Variable *currentVariable, Decl *newDecl,
                                 [[maybe_unused]] ScriptExtension extension)
{
    if (!newDecl->IsVarDecl() &&
        (paramScope_->FindLocal(newDecl->Name(), varbinder::ResolveBindingOptions::BINDINGS) != nullptr)) {
        return nullptr;
    }

    return AddLocal(allocator, currentVariable, newDecl, extension);
}

Variable *CatchScope::FindLocal(const util::StringView &name, ResolveBindingOptions options) const
{
    auto res = Bindings().find(name);
    if (res == Bindings().end()) {
        return paramScope_->FindLocal(name, options);
    }
    return res->second;
}

template <typename T, typename... Args>
Variable *Scope::PropagateBinding(ArenaAllocator *allocator, util::StringView name, Args &&...args)
{
    auto res = bindings_.find(name);
    if (res == bindings_.end()) {
        return bindings_.insert({name, allocator->New<T>(std::forward<Args>(args)...)}).first->second;
    }

    res->second->Reset(std::forward<Args>(args)...);
    return res->second;
}
}  // namespace ark::es2panda::varbinder
