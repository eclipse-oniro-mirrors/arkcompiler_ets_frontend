/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "variable.h"

#include "checker/types/type.h"
#include "varbinder/scope.h"

#include <utility>

namespace ark::es2panda::varbinder {

LocalVariable::LocalVariable(Decl *decl, VariableFlags flags) : Variable(decl, flags)
{
    if (decl->IsConstDecl() || decl->IsReadonlyDecl()) {
        flags_ |= VariableFlags::READONLY;
    }
}

LocalVariable::LocalVariable(VariableFlags flags) : Variable(flags) {}

const util::StringView &Variable::Name() const
{
    // Synthetic var has no decl_
    ES2PANDA_ASSERT(decl_ != nullptr);
    return decl_->Name();
}

LocalVariable *LocalVariable::Copy(ArenaAllocator *allocator, Decl *decl) const
{
    auto *var = allocator->New<LocalVariable>(decl, flags_);
    ES2PANDA_ASSERT(var != nullptr);
    var->vreg_ = vreg_;
    return var;
}

void LocalVariable::SetLexical(Scope *scope)
{
    if (LexicalBound()) {
        return;
    }

    VariableScope *varScope = scope->EnclosingVariableScope();
    ES2PANDA_ASSERT(varScope != nullptr);
    BindLexEnvSlot(varScope->NextSlot());
}

void GlobalVariable::SetLexical([[maybe_unused]] Scope *scope) {}
void ModuleVariable::SetLexical([[maybe_unused]] Scope *scope) {}
void EnumVariable::SetLexical([[maybe_unused]] Scope *scope) {}

void EnumVariable::ResetDecl(Decl *decl)
{
    decl_ = decl;
}
}  // namespace ark::es2panda::varbinder
