/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "compiler/lowering/phase.h"

#include "util/importPathManager.h"

namespace ark::es2panda::parser {

Program::Program(const util::ImportMetadata &importMetadata, ArenaAllocator *allocator, varbinder::VarBinder *varbinder)
    : importMetadata_(importMetadata),
      allocator_(allocator),
      sourceFile_(util::Path {allocator}),
      extension_(varbinder != nullptr ? varbinder->Extension() : ScriptExtension::INVALID),
      cfg_(allocator_->New<compiler::CFG>(allocator_))
{
    PushVarBinder(varbinder);

    // NOTE(dkofanov): #32416 remove 'SourceFile' in favor of 'ImportMetadata`.
    std::string_view textView {};
    switch (importMetadata_.Text().Kind()) {
        case util::ModuleKind::PACKAGE:
        case util::ModuleKind::UNKNOWN:
            textView = "";
            break;
        default:
            textView = importMetadata_.Text().Text();
    }
    bool isDynamic = importMetadata_.Lang() != Language::Id::ETS;
    es2panda::SourceFile sf {importMetadata_.TextSource(), textView, importMetadata_.ResolvedSource(), false,
                             isDynamic};
    SetSource(sf);
}

std::string Program::RelativeFilePath(const public_lib::Context *context) const
{
    if (importMetadata_.Lang() != Language::Id::ETS) {
        return std::string {importMetadata_.TextSource()};
    }
    if (!Is<util::ModuleKind::MODULE>()) {
        return std::string {ModuleName()};
    }
    // NOTE(dkofanov): there should be rebasing abspath to baseurl.
    return util::Path(importMetadata_.TextSource(), context->Allocator()).GetFileNameWithExtension().Mutf8();
}

void Program::PushVarBinder(varbinder::VarBinder *varbinder)
{
    varbinders_.insert_or_assign(compiler::GetPhaseManager()->GetCurrentMajor(), varbinder);
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
    if (checkers_.size() > static_cast<size_t>(compiler::GetPhaseManager()->GetCurrentMajor())) {
        checkers_.at(compiler::GetPhaseManager()->GetCurrentMajor()) = checker;
        return;
    }
    checkers_.push_back(checker);
}

const checker::Checker *Program::Checker() const
{
    return checkers_.at(compiler::GetPhaseManager()->GetCurrentMajor());
}

bool Program::IsGenAbcForExternal() const
{
    if (genAbcForExternalSource_) {
        [[maybe_unused]] auto ctx = compiler::GetPhaseManager()->Context();
        ES2PANDA_ASSERT(ctx->config->options->IsSimultaneous());
        ES2PANDA_ASSERT(ctx->config->options->GetCompilationMode() == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE);
        return true;
    }
    return false;
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

// Obsolete interface
// NOTE(dkofanov): #32416 enforce '=='-consistency between 'moduleInfo_' and 'importMetadata_', then remove
// 'moduleInfo_'.
void Program::SetPackageInfo(std::string_view mname, util::ModuleKind kind)
{
    // NOTE(vpukhov): the *unnamed* modules are to be removed entirely
    ES2PANDA_ASSERT((importMetadata_.ModuleName() == mname) || mname.empty());
    moduleInfo_.moduleName = std::string(mname);
    moduleInfo_.modulePrefix = mname.empty() ? "" : std::string(mname).append(compiler::Signatures::METHOD_SEPARATOR);
    moduleInfo_.kind = kind;
}

// NOTE(vpukhov): #31581: the flags should be set by the build system
// NOTE(dkofanov): Ensures declaration module contain only declarations.
// However, this condition may be broken by '->Ast()->AddStatement' for example.
void Program::VerifyDeclarationModule()
{
    ES2PANDA_ASSERT(ast_ != nullptr);
    if (!IsDeclarationModule()) {
        return;
    }
    ES2PANDA_ASSERT(!Is<util::ModuleKind::PACKAGE>());
    for (auto stmt : Ast()->Statements()) {
        // NOTE(dkofanov): asserts should be replaced with diagnostics.
        if (stmt->IsExpressionStatement()) {
            ES2PANDA_ASSERT(stmt->AsExpressionStatement()->GetExpression()->AsStringLiteral()->Str() ==
                            compiler::Signatures::STATIC_PROGRAM_FLAG);
            continue;
        }

        if (stmt->IsDeclare() || stmt->IsTSTypeAliasDeclaration() || stmt->IsETSImportDeclaration() ||
            stmt->IsExportNamedDeclaration() || stmt->IsETSReExportDeclaration() || stmt->IsTSInterfaceDeclaration()) {
            continue;
        }
        stmt->AddModifier(ir::ModifierFlags::DECLARE);
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

void Program::PromoteToMainProgram(public_lib::Context *ctx)
{
    auto *oldMain = ctx->parserProgram;
    // NOTE(dkofanov): externals sources should be bound to context, not programs.
    ES2PANDA_ASSERT(Is<util::ModuleKind::PACKAGE>());
    ES2PANDA_ASSERT(GetExternalSources()->Empty());

    auto *packages = &oldMain->externalSources_.Get<util::ModuleKind::PACKAGE>();
    auto toRemove = std::find(packages->begin(), packages->end(), this);
    // NOTE(dkofanov): the later 'if' should be an assert. The case handled by 'if' relates to the broken functionality
    // of 'ETSPackageDeclaration' expressed in `EnsurePackageIsRegisteredByPackageFraction`.
    if (toRemove != packages->end()) {
        packages->erase(toRemove);
    }

    externalSources_.transitiveExternals_ = std::move(oldMain->externalSources_.transitiveExternals_);
    externalSources_.direct_ = std::move(oldMain->externalSources_.direct_);

    oldMain->externalSources_ = ExternalSources();
    ctx->parserProgram = this;
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

}  // namespace ark::es2panda::parser
