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

#include "program.h"
#include "libarkbase/macros.h"
#include "public/public.h"

#include "compiler/core/CFG.h"
#include "generated/signatures.h"
#include "varbinder/varbinder.h"
#include "varbinder/ETSBinder.h"
#include "ir/astDump.h"
#include "ir/base/classDefinition.h"
#include "ir/statements/blockStatement.h"

#include <compiler/lowering/phase.h>

namespace ark::es2panda::parser {

Program::Program(ArenaAllocator *allocator, varbinder::VarBinder *varbinder)
    : allocator_(allocator),
      externalSources_(allocator_->Adapter()),
      directExternalSources_(allocator_->Adapter()),
      extension_(varbinder != nullptr ? varbinder->Extension() : ScriptExtension::INVALID),
      etsnolintCollection_(allocator_->Adapter()),
      cfg_(allocator_->New<compiler::CFG>(allocator_)),
      varbinders_(allocator_->Adapter()),
      checkers_(allocator_->Adapter())
{
    PushVarBinder(varbinder);
}

void Program::PushVarBinder(varbinder::VarBinder *varbinder)
{
    varbinders_.insert({compiler::GetPhaseManager()->GetCurrentMajor(), varbinder});
}

const varbinder::VarBinder *Program::VarBinder() const
{
    return varbinders_.at(compiler::GetPhaseManager()->GetCurrentMajor());
}

varbinder::VarBinder *Program::VarBinder()
{
    return varbinders_.at(compiler::GetPhaseManager()->GetCurrentMajor());
}

checker::Checker *Program::Checker()
{
    return checkers_.at(compiler::GetPhaseManager()->GetCurrentMajor());
}

void Program::PushChecker(checker::Checker *checker)
{
    checkers_.push_back(checker);
}

const checker::Checker *Program::Checker() const
{
    return checkers_.at(compiler::GetPhaseManager()->GetCurrentMajor());
}

bool Program::IsGenAbcForExternal() const
{
    return compiler::GetPhaseManager()->Context()->config->options->GetCompilationMode() ==
               CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE &&
           genAbcForExternalSource_;
}

std::string Program::Dump() const
{
    ir::AstDumper dumper {ast_, SourceCode()};
    return dumper.Str();
}

void Program::DumpSilent() const
{
    [[maybe_unused]] ir::AstDumper dumper {ast_, SourceCode()};
    ES2PANDA_ASSERT(!dumper.Str().empty());
}

varbinder::ClassScope *Program::GlobalClassScope()
{
    ES2PANDA_ASSERT(GlobalClass() != nullptr);
    ES2PANDA_ASSERT(GlobalClass()->Scope() != nullptr);
    return GlobalClass()->Scope()->AsClassScope();
}

const varbinder::ClassScope *Program::GlobalClassScope() const
{
    ES2PANDA_ASSERT(GlobalClass() != nullptr);
    ES2PANDA_ASSERT(GlobalClass()->Scope() != nullptr);
    return GlobalClass()->Scope()->AsClassScope();
}

varbinder::GlobalScope *Program::GlobalScope()
{
    ES2PANDA_ASSERT(ast_->Scope()->IsGlobalScope() || ast_->Scope()->IsModuleScope());
    return static_cast<varbinder::GlobalScope *>(ast_->Scope());
}

const varbinder::GlobalScope *Program::GlobalScope() const
{
    ES2PANDA_ASSERT(ast_->Scope()->IsGlobalScope() || ast_->Scope()->IsModuleScope());
    return static_cast<const varbinder::GlobalScope *>(ast_->Scope());
}

void Program::SetPackageInfo(const util::StringView &name, util::ModuleKind kind)
{
    moduleInfo_.moduleName = name;
    moduleInfo_.modulePrefix =
        name.Empty()
            ? ""
            : util::UString(std::string(name).append(compiler::Signatures::METHOD_SEPARATOR), allocator_).View();

    moduleInfo_.kind = kind;
}

// NOTE(vpukhov): #31581: the flags should be set by the build system
void Program::MaybeTransformToDeclarationModule()
{
    ES2PANDA_ASSERT(ast_ != nullptr);
    if (IsPackage() || ast_->Statements().empty()) {
        return;
    }
    bool hasLocalDefs = false;
    for (auto stmt : ast_->Statements()) {
        if (stmt->IsETSImportDeclaration()) {
            continue;
        }
        // The existing logic is as follows:
        // * if module is empty, it is not a declaration module
        // * if there is any local non-declare definition in the module, it is not a declaration module
        // * otherwise, it is a declaration module
        hasLocalDefs = true;
        if (!(stmt->IsDeclare() || stmt->IsTSTypeAliasDeclaration())) {
            return;
        }
    }
    if (hasLocalDefs) {
        moduleInfo_.isDeclarationModule = true;
    }
}

void Program::AddNodeToETSNolintCollection(const ir::AstNode *node, const std::set<ETSWarnings> &warningsCollection)
{
    ArenaSet<ETSWarnings> tmp(allocator_->Adapter());
    tmp.insert(warningsCollection.begin(), warningsCollection.end());
    etsnolintCollection_.insert({node, tmp});
}

bool Program::NodeContainsETSNolint(const ir::AstNode *node, ETSWarnings warning)
{
    auto nodeEtsnolints = etsnolintCollection_.find(node);
    if (nodeEtsnolints == etsnolintCollection_.end()) {
        return false;
    }

    return nodeEtsnolints->second.find(warning) != nodeEtsnolints->second.end();
}

void Program::SetASTChecked()
{
    isAstChecked_ = true;
}

void Program::RemoveAstChecked()
{
    isAstChecked_ = false;
}

bool Program::IsASTChecked()
{
    return isAstChecked_;
}

Program::~Program()  // NOLINT(modernize-use-equals-default)
{
#ifndef NDEBUG
    poisonValue_ = 0;
#endif
}

compiler::CFG *Program::GetCFG()
{
    return cfg_;
}

ir::ClassDefinition *Program::GlobalClass()
{
    return ast_->AsETSModule()->GlobalClass();
}

const ir::ClassDefinition *Program::GlobalClass() const
{
    return ast_->AsETSModule()->GlobalClass();
}

void Program::SetGlobalClass(ir::ClassDefinition *globalClass)
{
    ast_->AsETSModule()->SetGlobalClass(globalClass);
}

const compiler::CFG *Program::GetCFG() const
{
    return cfg_;
}

bool Program::MergeExternalSource(const ExternalSource *externalSource)
{
    // prevent using cache for cycle import
    for (const auto &[moduleName, _] : *externalSource) {
        if (ModuleName() == moduleName) {
            return false;
        }
    }

    for (const auto &[moduleName, extProgs] : *externalSource) {
        externalSources_.emplace(moduleName, extProgs);
    }

    return true;
}

}  // namespace ark::es2panda::parser
