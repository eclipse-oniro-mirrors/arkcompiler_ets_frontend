/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "varbinder/exportFacts.h"

#include "ir/astNode.h"
#include "parser/program/program.h"
#include "varbinder/declaration.h"
#include "varbinder/variable.h"

namespace ark::es2panda::varbinder {

const ExportFactStore::ExportFactSnapshot &ExportFactStore::GetExportFacts(parser::Program *program) const
{
    if (auto it = facts_.find(program); it != facts_.end()) {
        return it->second;
    }

    return emptySnapshot_;
}

const ExportSurfaceId *ExportFactStore::FindSurfaceByProgram(parser::Program *program) const
{
    if (auto it = surfacesByProgram_.find(program); it != surfacesByProgram_.end()) {
        return &it->second;
    }

    return nullptr;
}

const ExportSurfaceId *ExportFactStore::FindSurfaceByResolvedSource(util::StringView resolvedSource) const
{
    if (auto it = surfaceByResolvedSource_.find(resolvedSource); it != surfaceByResolvedSource_.end()) {
        return FindSurfaceByProgram(it->second);
    }

    return nullptr;
}

const PackageSurfaceFact *ExportFactStore::FindPackageSurface(parser::Program *program) const
{
    if (auto it = packageSurfaces_.find(program); it != packageSurfaces_.end()) {
        return &it->second;
    }

    return nullptr;
}

void ExportFactStore::RegisterProgramSurface(parser::Program *program)
{
    if (program == nullptr) {
        return;
    }

    auto [it, inserted] = surfacesByProgram_.try_emplace(program, MakeSurface(program));
    if (!inserted) {
        it->second = MakeSurface(program);
    }

    const auto &surface = it->second;
    if (!surface.resolvedSource.Empty()) {
        surfaceByResolvedSource_[surface.resolvedSource] = program;
    }
}

void ExportFactStore::RegisterPackageSurface(parser::Program *program)
{
    if (program == nullptr || !program->Is<util::ModuleKind::PACKAGE>()) {
        return;
    }

    RegisterProgramSurface(program);

    auto &surface = packageSurfaces_.try_emplace(program, allocator_).first->second;
    surface.fractions.clear();

    program->MaybeIteratePackage([this, &surface](parser::Program *fraction, bool isPackageFraction) {
        if (!isPackageFraction) {
            return;
        }

        RegisterProgramSurface(fraction);
        surface.fractions.push_back(fraction);
    });
}

void ExportFactStore::AddImportTarget(parser::Program *sourceProgram, const ir::ETSImportDeclaration *importDecl,
                                      parser::Program *targetProgram)
{
    if (sourceProgram == nullptr || importDecl == nullptr || targetProgram == nullptr) {
        return;
    }

    RegisterProgramSurface(targetProgram);
    importTargets_[ImportTargetKey {sourceProgram, importDecl}] = MakeSurface(targetProgram);
}

void ExportFactStore::AddEffectiveImportTarget(parser::Program *sourceProgram,
                                               const ir::ETSImportDeclaration *importDecl,
                                               parser::Program *targetProgram)
{
    if (sourceProgram == nullptr || importDecl == nullptr || targetProgram == nullptr) {
        return;
    }

    RegisterProgramSurface(targetProgram);
    effectiveImportTargets_[ImportTargetKey {sourceProgram, importDecl}] = MakeSurface(targetProgram);
}

const ExportSurfaceId *ExportFactStore::FindImportTarget(parser::Program *sourceProgram,
                                                         const ir::ETSImportDeclaration *importDecl) const
{
    if (auto it = importTargets_.find(ImportTargetKey {sourceProgram, importDecl}); it != importTargets_.end()) {
        return &(it->second);
    }

    return nullptr;
}

const ExportSurfaceId *ExportFactStore::FindEffectiveImportTarget(parser::Program *sourceProgram,
                                                                  const ir::ETSImportDeclaration *importDecl) const
{
    if (auto it = effectiveImportTargets_.find(ImportTargetKey {sourceProgram, importDecl});
        it != effectiveImportTargets_.end()) {
        return &(it->second);
    }

    return nullptr;
}

void ExportFactStore::ResetProgram(parser::Program *program)
{
    facts_.erase(program);
}

void ExportFactStore::ClearImportTargets(parser::Program *sourceProgram)
{
    for (auto it = importTargets_.begin(); it != importTargets_.end();) {
        if (it->first.sourceProgram == sourceProgram) {
            it = importTargets_.erase(it);
            continue;
        }
        ++it;
    }
    for (auto it = effectiveImportTargets_.begin(); it != effectiveImportTargets_.end();) {
        if (it->first.sourceProgram == sourceProgram) {
            it = effectiveImportTargets_.erase(it);
            continue;
        }
        ++it;
    }
}

void ExportFactStore::AddLocalExport(parser::Program *program, util::StringView exportedName, Variable *variable,
                                     const ir::AstNode *origin)
{
    AddLocalExport(program, exportedName, exportedName, variable, origin);
}

void ExportFactStore::AddLocalExport(parser::Program *program, util::StringView exportedName,
                                     util::StringView localName, Variable *variable, const ir::AstNode *origin)
{
    RegisterProgramSurface(program);
    auto &snapshot = GetOrCreateSnapshot(program);
    for (auto &fact : snapshot.locals) {
        if (fact.exportedName != exportedName || fact.localName != localName || fact.origin != origin) {
            continue;
        }
        if (fact.variable == nullptr) {
            fact.variable = variable;
            fact.isTypeOnly = IsTypeOnlyVariable(variable);
        }
        return;
    }
    snapshot.locals.push_back(ExportFact {
        program,
        exportedName,
        localName,
        util::StringView {},
        nullptr,
        origin,
        variable,
        IsTypeOnlyVariable(variable),
        false,
        false,
    });
}

void ExportFactStore::AddLocalExportAlias(parser::Program *program, util::StringView exportedName,
                                          util::StringView localName, Variable *variable, const ir::AstNode *origin,
                                          bool isTypeOnly, bool isInvalid)
{
    RegisterProgramSurface(program);
    auto &snapshot = GetOrCreateSnapshot(program);
    snapshot.locals.push_back(ExportFact {
        program,
        exportedName,
        localName,
        util::StringView {},
        nullptr,
        origin,
        variable,
        isTypeOnly || IsTypeOnlyVariable(variable),
        true,
        isInvalid,
    });
}

bool ExportFactStore::AddPendingLocalExportAlias(parser::Program *program, util::StringView exportedName,
                                                 util::StringView localName, const ir::AstNode *origin,
                                                 const ir::AstNode *exportDecl, const ir::AstNode *reportOrigin,
                                                 bool originDeclaresName, bool isTypeOnly, LocalExportKind kind)
{
    auto [it, _] = pendingLocalExportAliases_.try_emplace(program, allocator_->Adapter());
    auto &aliases = it->second;
    for (const auto &alias : aliases) {
        if (alias.exportedName != exportedName) {
            continue;
        }
        if (alias.localName != localName) {
            return false;
        }
    }

    aliases.push_back(PendingLocalExportAlias {program, exportedName, localName, origin, exportDecl, reportOrigin,
                                               originDeclaresName, isTypeOnly, false, kind});
    return true;
}

void ExportFactStore::MarkPendingLocalExportAliasInvalid(parser::Program *program, util::StringView exportedName,
                                                         util::StringView localName, const ir::AstNode *reportOrigin)
{
    auto it = pendingLocalExportAliases_.find(program);
    if (it == pendingLocalExportAliases_.end()) {
        return;
    }

    for (auto &alias : it->second) {
        if (alias.exportedName == exportedName && alias.localName == localName && alias.reportOrigin == reportOrigin) {
            alias.isInvalid = true;
            return;
        }
    }
}

const ArenaVector<PendingLocalExportAlias> &ExportFactStore::PendingLocalExportAliases(parser::Program *program) const
{
    if (auto it = pendingLocalExportAliases_.find(program); it != pendingLocalExportAliases_.end()) {
        return it->second;
    }
    return emptyPendingLocalExportAliases_;
}

void ExportFactStore::AddNamedReExport(parser::Program *program, const ir::ETSImportDeclaration *importDecl,
                                       util::StringView exportedName, util::StringView importedName,
                                       const ir::AstNode *origin, bool isTypeOnly)
{
    RegisterProgramSurface(program);
    auto &snapshot = GetOrCreateSnapshot(program);
    snapshot.namedReExports.push_back(ExportFact {
        program,
        exportedName,
        util::StringView {},
        importedName,
        importDecl,
        origin,
        nullptr,
        isTypeOnly,
        false,
        false,
    });
}

void ExportFactStore::AddStarExport(parser::Program *program, const ir::ETSImportDeclaration *importDecl,
                                    const ir::AstNode *origin, bool isTypeOnly)
{
    RegisterProgramSurface(program);
    auto &snapshot = GetOrCreateSnapshot(program);
    snapshot.starExports.push_back(ExportFact {
        program,
        util::StringView {},
        util::StringView {},
        util::StringView {},
        importDecl,
        origin,
        nullptr,
        isTypeOnly,
        false,
        false,
    });
}

void ExportFactStore::AddNamespaceExport(parser::Program *program, const ir::ETSImportDeclaration *importDecl,
                                         util::StringView exportedName, Variable *variable, const ir::AstNode *origin,
                                         bool isTypeOnly)
{
    RegisterProgramSurface(program);
    auto &snapshot = GetOrCreateSnapshot(program);
    snapshot.namespaceExports.push_back(ExportFact {
        program,
        exportedName,
        exportedName,
        util::StringView {},
        importDecl,
        origin,
        variable,
        isTypeOnly || IsTypeOnlyVariable(variable),
        false,
        false,
    });
}

ExportFactStore::ExportFactSnapshot &ExportFactStore::GetOrCreateSnapshot(parser::Program *program)
{
    if (auto it = facts_.find(program); it != facts_.end()) {
        return it->second;
    }

    return facts_.try_emplace(program, allocator_).first->second;
}

ExportSurfaceId ExportFactStore::MakeSurface(parser::Program *program) const
{
    if (program == nullptr) {
        return {};
    }

    return ExportSurfaceId {
        program->Is<util::ModuleKind::PACKAGE>() ? ExportSurfaceKind::Package : ExportSurfaceKind::Program,
        program,
        util::StringView {program->ModuleName()},
        util::StringView {program->GetImportInfo().ResolvedSource()},
    };
}

bool ExportFactStore::IsTypeOnlyVariable(const Variable *variable)
{
    if (variable == nullptr || variable->Declaration() == nullptr || variable->Declaration()->Node() == nullptr) {
        return false;
    }

    const auto *node = variable->Declaration()->Node();
    return node->IsTSInterfaceDeclaration() || node->IsTSTypeAliasDeclaration() || node->IsAnnotationDeclaration();
}

}  // namespace ark::es2panda::varbinder
