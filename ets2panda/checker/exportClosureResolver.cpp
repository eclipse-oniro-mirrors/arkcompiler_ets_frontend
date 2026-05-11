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

#include "checker/exportClosureResolver.h"

#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "checker/checker.h"
#include "ir/base/classProperty.h"
#include "ir/base/overloadDeclaration.h"
#include "ir/ets/etsImportDeclaration.h"
#include "ir/ets/etsModule.h"
#include "ir/module/importNamespaceSpecifier.h"
#include "ir/module/importSpecifier.h"
#include "parser/program/program.h"
#include "generated/signatures.h"
#include "util/helpers.h"
#include "varbinder/ETSBinder.h"
#include "varbinder/exportFacts.h"

namespace ark::es2panda::checker {

namespace {

using VisitingSet = std::unordered_set<NameResolutionKey, NameResolutionKeyHash>;

enum class MergeOutcome {
    UNCHANGED,
    NEW_AMBIGUOUS,
};

varbinder::ETSBinder *FindAvailableETSBinder(parser::Program *program)
{
    if (program == nullptr) {
        return nullptr;
    }

    for (const auto &[_, varBinder] : program->VarBinders()) {
        if (varBinder != nullptr && varBinder->IsETSBinder()) {
            return varBinder->AsETSBinder();
        }
    }
    return nullptr;
}

bool IsProgramLocalVariable(parser::Program *program, const varbinder::Variable *var)
{
    return var != nullptr && var->Declaration() != nullptr && var->Declaration()->Node() != nullptr &&
           var->Declaration()->Node()->Program() == program;
}

bool HasProgramLocalDeclaration(parser::Program *program, util::StringView localName,
                                const varbinder::Scope::VariableMap &bindings)
{
    auto iter = bindings.find(localName);
    return iter != bindings.end() && IsProgramLocalVariable(program, iter->second);
}

bool HasProgramLocalDeclaration(parser::Program *program, util::StringView localName)
{
    auto *ast = program == nullptr ? nullptr : program->Ast();
    auto *scope = ast == nullptr ? nullptr : ast->Scope();
    if (scope == nullptr || (!scope->IsGlobalScope() && !scope->IsModuleScope())) {
        return false;
    }

    auto *globalClass = ast->IsETSModule() ? ast->AsETSModule()->GlobalClass() : nullptr;
    auto *globalClassScope =
        globalClass != nullptr && globalClass->Scope() != nullptr ? globalClass->Scope()->AsClassScope() : nullptr;
    if (globalClassScope == nullptr) {
        return HasProgramLocalDeclaration(program, localName, static_cast<varbinder::GlobalScope *>(scope)->Bindings());
    }

    return HasProgramLocalDeclaration(program, localName, static_cast<varbinder::GlobalScope *>(scope)->Bindings()) ||
           HasProgramLocalDeclaration(program, localName, globalClassScope->StaticFieldScope()->Bindings()) ||
           HasProgramLocalDeclaration(program, localName, globalClassScope->StaticMethodScope()->Bindings()) ||
           HasProgramLocalDeclaration(program, localName, globalClassScope->StaticDeclScope()->Bindings()) ||
           HasProgramLocalDeclaration(program, localName, globalClassScope->TypeAliasScope()->Bindings());
}

ResolvedExportResult MakeResult(ExportResolutionStatus status, const ir::AstNode *reportOrigin = nullptr)
{
    ResolvedExportResult result;
    result.status = status;
    result.reportOrigin = reportOrigin;
    return result;
}

ResolvedExportResult MakeResolvedResult(varbinder::Variable *variable, const ir::AstNode *origin,
                                        parser::Program *originProgram, const ir::AstNode *reportOrigin = nullptr)
{
    ResolvedExportResult result;
    result.status = ExportResolutionStatus::RESOLVED;
    result.entry.variable = variable;
    result.entry.origin = origin;
    result.entry.originProgram = originProgram;
    result.reportOrigin = reportOrigin != nullptr ? reportOrigin : origin;
    return result;
}

ResolvedExportResult MakeResolvedSurfaceResult(const varbinder::ExportSurfaceId &surface, const ir::AstNode *origin,
                                               parser::Program *originProgram,
                                               const ir::AstNode *reportOrigin = nullptr)
{
    auto result = MakeResolvedResult(nullptr, origin, originProgram, reportOrigin);
    result.entry.surface = surface;
    return result;
}

lexer::SourcePosition ImportBindingReportPosition(const varbinder::ImportBindingInfo *bindingInfo)
{
    ES2PANDA_ASSERT(bindingInfo != nullptr && bindingInfo->origin != nullptr);
    if (bindingInfo->origin->IsImportNamespaceSpecifier()) {
        return bindingInfo->origin->AsImportNamespaceSpecifier()->Local()->Start();
    }
    return bindingInfo->origin->Start();
}

bool IsSameSurface(const varbinder::ExportSurfaceId &lhs, const varbinder::ExportSurfaceId &rhs)
{
    return lhs.kind == rhs.kind && lhs.program == rhs.program;
}

bool IsMemoKeyForSurface(const NameResolutionKey &key, const varbinder::ExportSurfaceId &surface)
{
    return key.surfaceKind == surface.kind && key.surfaceProgram == surface.program;
}

bool IsSameOrigin(const ResolvedExportEntry &lhs, const ResolvedExportEntry &rhs)
{
    if (lhs.originProgram == rhs.originProgram && lhs.variable != nullptr && lhs.variable == rhs.variable) {
        return true;
    }
    auto overloadContainsVariable = [](const ir::AstNode *origin, const varbinder::Variable *variable) {
        if (origin == nullptr || variable == nullptr || !origin->IsOverloadDeclaration()) {
            return false;
        }
        for (auto *expr : origin->AsOverloadDeclaration()->OverloadedList()) {
            if (expr != nullptr && expr->IsIdentifier() && expr->AsIdentifier()->Variable() == variable) {
                return true;
            }
        }
        return false;
    };
    if (lhs.originProgram == rhs.originProgram &&
        (overloadContainsVariable(lhs.origin, rhs.variable) || overloadContainsVariable(rhs.origin, lhs.variable))) {
        return true;
    }
    return lhs.originProgram == rhs.originProgram && lhs.origin == rhs.origin && lhs.variable == rhs.variable &&
           IsSameSurface(lhs.surface, rhs.surface);
}

ResolvedExportResult RebindReportOrigin(const ResolvedExportResult &result, const ir::AstNode *reportOrigin)
{
    auto rebound = result;
    if (reportOrigin != nullptr) {
        rebound.reportOrigin = reportOrigin;
    }
    return rebound;
}

ResolvedExportResult ResultOrNotFound(const ResolvedExportResult *result)
{
    return result != nullptr ? *result : MakeResult(ExportResolutionStatus::NOT_FOUND);
}

ResolvedExportResult RebindResultOrNotFound(const ResolvedExportResult *result, const ir::AstNode *reportOrigin)
{
    return RebindReportOrigin(ResultOrNotFound(result), reportOrigin);
}

MergeOutcome MergeResolvedResults(ResolvedExportResult *current, const ResolvedExportResult &next)
{
    if (current->status == ExportResolutionStatus::INVALID || next.status == ExportResolutionStatus::INVALID) {
        *current = MakeResult(ExportResolutionStatus::INVALID,
                              next.reportOrigin != nullptr ? next.reportOrigin : current->reportOrigin);
        return MergeOutcome::UNCHANGED;
    }

    if (next.status == ExportResolutionStatus::NOT_FOUND) {
        return MergeOutcome::UNCHANGED;
    }

    if (current->status == ExportResolutionStatus::NOT_FOUND) {
        *current = next;
        return MergeOutcome::UNCHANGED;
    }

    if (current->status == ExportResolutionStatus::AMBIGUOUS || next.status == ExportResolutionStatus::AMBIGUOUS) {
        *current = MakeResult(ExportResolutionStatus::AMBIGUOUS,
                              next.reportOrigin != nullptr ? next.reportOrigin : current->reportOrigin);
        return MergeOutcome::UNCHANGED;
    }

    if (!IsSameOrigin(current->entry, next.entry)) {
        *current = MakeResult(ExportResolutionStatus::AMBIGUOUS,
                              next.reportOrigin != nullptr ? next.reportOrigin : current->reportOrigin);
        return MergeOutcome::NEW_AMBIGUOUS;
    }

    return MergeOutcome::UNCHANGED;
}

const varbinder::ExportFactStore::ExportFactSnapshot &GetSnapshot(parser::Program *program)
{
    using ExportFactSnapshot = varbinder::ExportFactStore::ExportFactSnapshot;
    static auto *emptySnapshot = []() {
        alignas(EAllocator) static std::array<std::byte, sizeof(EAllocator)> allocatorStorage {};
        auto *allocator = new (allocatorStorage.data()) EAllocator();

        alignas(ExportFactSnapshot) static std::array<std::byte, sizeof(ExportFactSnapshot)> snapshotStorage {};
        return new (snapshotStorage.data()) ExportFactSnapshot(allocator);
    }();

    auto *etsBinder = FindAvailableETSBinder(program);
    if (etsBinder == nullptr) {
        return *emptySnapshot;
    }

    return etsBinder->GetExportFacts(program);
}

const varbinder::ExportFactStore *GetFactStore(parser::Program *program)
{
    auto *etsBinder = FindAvailableETSBinder(program);
    if (etsBinder == nullptr) {
        return nullptr;
    }

    return &(etsBinder->GetExportFactsStore());
}

template <typename Callback>
bool ForEachPackageFraction(const varbinder::ExportSurfaceId &surface, Callback callback)
{
    if (surface.kind != varbinder::ExportSurfaceKind::Package) {
        return false;
    }

    bool sawFraction = false;
    auto visitFraction = [&callback, &sawFraction](parser::Program *fraction) {
        if (fraction == nullptr || fraction->Is<util::ModuleKind::PACKAGE>()) {
            return;
        }
        sawFraction = true;
        callback(fraction);
    };

    const auto *store = GetFactStore(surface.program);
    const auto *packageSurface = store == nullptr ? nullptr : store->FindPackageSurface(surface.program);
    if (packageSurface != nullptr) {
        for (auto *fraction : packageSurface->fractions) {
            visitFraction(fraction);
        }
    } else if (surface.program != nullptr && surface.program->Is<util::ModuleKind::PACKAGE>()) {
        surface.program->MaybeIteratePackage([&visitFraction](parser::Program *fraction, bool isPackageFraction) {
            if (isPackageFraction) {
                visitFraction(fraction);
            }
        });
    }

    return sawFraction;
}

parser::Program *FindOwningProgram(const ir::AstNode *node)
{
    while (node != nullptr) {
        if (node->IsETSModule()) {
            return const_cast<parser::Program *>(node->AsETSModule()->Program());
        }
        node = node->Parent();
    }
    return nullptr;
}

ResolvedExportResult ResolveMatchingFacts(const ArenaVector<varbinder::ExportFact> &facts,
                                          util::StringView exportedName, parser::Program *originProgram)
{
    auto resolved = MakeResult(ExportResolutionStatus::NOT_FOUND);
    for (const auto &fact : facts) {
        if (fact.isInvalid || fact.exportedName != exportedName) {
            continue;
        }

        (void)MergeResolvedResults(&resolved, MakeResolvedResult(fact.variable, fact.origin, originProgram));
    }

    return resolved;
}

bool MatchesDefaultExportName(const ResolvedExportResult *resolved, util::StringView importedName)
{
    return resolved != nullptr && resolved->status == ExportResolutionStatus::RESOLVED &&
           resolved->entry.variable != nullptr && resolved->entry.variable->Name() == importedName;
}

void CheckExportedVariableTypeAnnotation(Checker *checker, const ResolvedExportEntry &entry)
{
    if (checker == nullptr || entry.origin == nullptr || !entry.origin->IsClassProperty()) {
        return;
    }

    auto *classProp = entry.origin->AsClassProperty();
    if (classProp->TypeAnnotation() != nullptr || classProp->Parent()->IsAnnotationUsage() ||
        classProp->Id()->Name() == compiler::Signatures::REEXPORT_DEFAULT_ANONYMOUSLY ||
        !util::Helpers::IsExported(classProp)) {
        return;
    }

    const std::string entityType = util::Helpers::IsGlobalClass(classProp->Parent()) ? "variable" : "class property";
    checker->LogError(diagnostic::EXPORTED_ENTITIES_DOESNOT_HAS_TYPEANNO, {entityType, classProp->Id()->Name()},
                      classProp->Start());
}

bool IsSameResolvedExport(const ResolvedExportResult &lhs, const ResolvedExportResult &rhs)
{
    return lhs.status == ExportResolutionStatus::RESOLVED && rhs.status == ExportResolutionStatus::RESOLVED &&
           IsSameOrigin(lhs.entry, rhs.entry);
}

bool IsPlainLocalDeclarationExport(const varbinder::ExportFact &fact)
{
    return fact.importDecl == nullptr && !fact.exportedName.Empty() && fact.exportedName == fact.localName &&
           !fact.isLocalAlias;
}

bool IsSameNamedReExportEdge(const varbinder::ExportFact &lhs, const varbinder::ExportFact &rhs)
{
    return lhs.importDecl != nullptr && rhs.importDecl != nullptr && lhs.exportedName == rhs.exportedName &&
           lhs.importedName == rhs.importedName && lhs.importDecl->Source()->Str() == rhs.importDecl->Source()->Str();
}

const ResolvedExportResult *CycleInvalidResult()
{
    static const auto RESULT = MakeResult(ExportResolutionStatus::INVALID);
    return &RESULT;
}

class VisitingGuard {
public:
    VisitingGuard(VisitingSet *visiting, const varbinder::ExportSurfaceId &surface, util::StringView exportedName)
        : visiting_(visiting), key_({surface.kind, surface.program, exportedName})
    {
        inserted_ = visiting_->insert(key_).second;
    }

    bool IsCycle() const
    {
        return !inserted_;
    }

    NO_COPY_SEMANTIC(VisitingGuard);
    NO_MOVE_SEMANTIC(VisitingGuard);

    ~VisitingGuard()
    {
        if (inserted_) {
            visiting_->erase(key_);
        }
    }

private:
    VisitingSet *visiting_;
    NameResolutionKey key_;
    bool inserted_ {false};
};

}  // namespace

struct ExportClosureResolver::ExplicitExportConflictState {
    std::unordered_map<std::string, ResolvedExportResult> seen;
    std::unordered_map<std::string, const varbinder::ExportFact *> seenFacts;
    std::unordered_map<std::string, ResolvedExportResult> seenDefaultByLocalName;
    std::unordered_map<std::string, ResolvedExportResult> seenNamedByLocalName;
    std::unordered_set<std::string> warnedAliases;
};

const ResolvedExportEntry *ResolvedExportCache::Find(util::StringView exportedName) const
{
    const auto *result = ResolveIfNeeded(exportedName);
    return result != nullptr && result->status == ExportResolutionStatus::RESOLVED ? &(result->entry) : nullptr;
}

bool ResolvedExportCache::Has(util::StringView exportedName) const
{
    const auto *result = ResolveIfNeeded(exportedName);
    return result != nullptr && result->status == ExportResolutionStatus::RESOLVED;
}

bool ResolvedExportCache::HasAmbiguous(util::StringView exportedName) const
{
    const auto *result = ResolveIfNeeded(exportedName);
    return result != nullptr && result->status == ExportResolutionStatus::AMBIGUOUS;
}

const ResolvedExportResult *ResolvedExportCache::ResolveIfNeeded(util::StringView exportedName) const
{
    if (auto it = entries_.find(exportedName); it != entries_.end()) {
        return &(it->second);
    }

    if (attempted_.find(exportedName) != attempted_.end()) {
        return nullptr;
    }

    const auto nameCopy = CopyName(exportedName);
    attempted_.insert(nameCopy);

    const auto *resolved = resolver_->ResolveExportName(resolver_->GetSurface(program_), exportedName);
    auto [it, _] = entries_.emplace(nameCopy, ResultOrNotFound(resolved));
    return &(it->second);
}

util::StringView ResolvedExportCache::CopyName(util::StringView exportedName) const
{
    return util::UString(exportedName, allocator_).View();
}

const ResolvedExportResult *ExportClosureResolver::ResolveExportName(const varbinder::ExportSurfaceId &surface,
                                                                     util::StringView exportedName)
{
    VisitingSet visiting;
    return ResolveSurfaceName(surface, exportedName, &visiting);
}

const ResolvedExportResult *ExportClosureResolver::ResolveExportNameWithoutAmbiguousDiagnostic(
    const varbinder::ExportSurfaceId &surface, util::StringView exportedName)
{
    const auto oldReportAmbiguousExport = reportAmbiguousExport_;
    reportAmbiguousExport_ = false;
    auto *result = ResolveExportName(surface, exportedName);
    reportAmbiguousExport_ = oldReportAmbiguousExport;
    return result;
}

void ExportClosureResolver::ValidateExportSurface(parser::Program *program)
{
    ValidateExportSurface(GetSurface(program));
}

void ExportClosureResolver::ValidateExportSurface(const varbinder::ExportSurfaceId &surface)
{
    if (surface.program == nullptr) {
        return;
    }

    const auto key = MakeKey(surface, "");
    if (validatedSurfaces_.find(key) != validatedSurfaces_.end() || !validatingSurfaces_.insert(key).second) {
        return;
    }

    VisitingSet visiting;
    ValidateExplicitExportConflicts(surface, &visiting);
    ValidateExportedDeclarations(surface, &visiting);
    validatingSurfaces_.erase(key);
    validatedSurfaces_.insert(key);
}

varbinder::ExportSurfaceId ExportClosureResolver::GetImportedSurface(const ir::ETSImportDeclaration *importDecl) const
{
    return GetImportedSurface(FindOwningProgram(importDecl), importDecl);
}

varbinder::ExportSurfaceId ExportClosureResolver::GetImportedSurface(parser::Program *sourceProgram,
                                                                     const ir::ETSImportDeclaration *importDecl) const
{
    if (importDecl == nullptr || !importDecl->IsValid()) {
        return {};
    }

    if (const auto *store = GetFactStore(sourceProgram); store != nullptr) {
        if (const auto *surface = store->FindImportTarget(sourceProgram, importDecl); surface != nullptr) {
            auto exactSurface = *surface;
            auto effectiveSurface = GetEffectiveImportedSurface(sourceProgram, importDecl);
            exactSurface.effectiveKind = effectiveSurface.kind;
            exactSurface.effectiveProgram = effectiveSurface.program;
            return exactSurface;
        }
    }

    ES2PANDA_ASSERT(false);
    return {};
}

varbinder::ExportSurfaceId ExportClosureResolver::GetImportedSurface(const varbinder::ExportFact &fact) const
{
    return GetImportedSurface(fact.sourceProgram, fact.importDecl);
}

varbinder::ExportSurfaceId ExportClosureResolver::GetEffectiveImportedSurface(
    parser::Program *sourceProgram, const ir::ETSImportDeclaration *importDecl) const
{
    if (sourceProgram == nullptr || importDecl == nullptr) {
        return {};
    }

    const auto *store = GetFactStore(sourceProgram);
    const auto *surface = store == nullptr ? nullptr : store->FindEffectiveImportTarget(sourceProgram, importDecl);
    return surface == nullptr ? varbinder::ExportSurfaceId {} : *surface;
}

varbinder::ExportSurfaceId ExportClosureResolver::GetEffectiveImportedSurface(const varbinder::ExportFact &fact) const
{
    return GetEffectiveImportedSurface(fact.sourceProgram, fact.importDecl);
}

varbinder::ExportSurfaceId ExportClosureResolver::GetEffectiveImportedSurface(
    const varbinder::ImportBindingInfo *bindingInfo) const
{
    if (bindingInfo == nullptr || bindingInfo->importDecl == nullptr) {
        return {};
    }

    return GetEffectiveImportedSurface(FindOwningProgram(bindingInfo->importDecl), bindingInfo->importDecl);
}

ResolvedExportResult ExportClosureResolver::SelectMaterializedSurfaceEntry(
    const varbinder::ExportSurfaceId &exactSurface, util::StringView exportedName,
    const ResolvedExportResult &exactResolved)
{
    if (exactResolved.status != ExportResolutionStatus::RESOLVED || exactSurface.effectiveProgram == nullptr ||
        exactSurface.effectiveProgram == exactSurface.program) {
        return exactResolved;
    }

    auto effectiveSurface = GetSurface(exactSurface.effectiveProgram);
    const auto *effectiveResolved = ResolveExportNameWithoutAmbiguousDiagnostic(effectiveSurface, exportedName);
    if (effectiveResolved == nullptr || effectiveResolved->status != ExportResolutionStatus::RESOLVED ||
        (effectiveResolved->entry.variable == nullptr && effectiveResolved->entry.surface.program == nullptr)) {
        return exactResolved;
    }

    return RebindReportOrigin(*effectiveResolved, exactResolved.reportOrigin);
}

ResolvedExportEntry ExportClosureResolver::SelectMaterializedImportEntry(
    const varbinder::ImportBindingInfo *bindingInfo, const varbinder::ExportSurfaceId &exactSurface,
    util::StringView exportedName, const ResolvedExportResult &exactResolved)
{
    auto effectiveSurface = GetEffectiveImportedSurface(bindingInfo);
    if (effectiveSurface.program == nullptr || effectiveSurface.program == exactSurface.program) {
        return exactResolved.entry;
    }
    const auto *effectiveResolved = ResolveExportNameWithoutAmbiguousDiagnostic(effectiveSurface, exportedName);
    if (effectiveResolved == nullptr || effectiveResolved->status != ExportResolutionStatus::RESOLVED) {
        return exactResolved.entry;
    }

    if (effectiveResolved->entry.variable == nullptr && effectiveResolved->entry.surface.program == nullptr) {
        return exactResolved.entry;
    }

    return effectiveResolved->entry;
}

ResolvedExportResult ExportClosureResolver::SelectMaterializedReExportResult(
    const varbinder::ExportFact &fact, const varbinder::ExportSurfaceId &exactSurface, util::StringView exportedName,
    const ResolvedExportResult &exactResolved)
{
    auto effectiveSurface = GetEffectiveImportedSurface(fact);
    auto surface = exactSurface;
    surface.effectiveKind = effectiveSurface.kind;
    surface.effectiveProgram = effectiveSurface.program;
    return SelectMaterializedSurfaceEntry(surface, exportedName, exactResolved);
}

void ExportClosureResolver::Clear()
{
    memo_.clear();
    validatingSurfaces_.clear();
    validatedSurfaces_.clear();
}

ResolvedImportBindingResult ExportClosureResolver::ResolveImportBinding(const varbinder::ImportBindingInfo *bindingInfo,
                                                                        ImportBindingResolveOptions options)
{
    if (bindingInfo == nullptr) {
        return {};
    }
    if (bindingInfo->resolvedVariable != nullptr) {
        ResolvedImportBindingResult result;
        result.status = ImportBindingResolutionStatus::RESOLVED_VARIABLE;
        result.entry.variable = bindingInfo->resolvedVariable;
        return result;
    }

    auto targetSurface = GetImportedSurface(bindingInfo->importDecl);
    if (targetSurface.program == nullptr) {
        ReportImportPathNotFound(bindingInfo, options);
        return {};
    }

    if (options.validateSurface && bindingInfo->kind != varbinder::ImportBindingKind::NAMESPACE) {
        ValidateExportSurface(targetSurface.program);
    }

    if (bindingInfo->kind == varbinder::ImportBindingKind::NAMESPACE) {
        return ResolveNamespaceImportBinding(bindingInfo, targetSurface);
    }

    const auto exportedName = bindingInfo->kind == varbinder::ImportBindingKind::DEFAULT ? util::StringView {"default"}
                                                                                         : bindingInfo->importedName;
    const auto *resolved = ResolveExportName(targetSurface, exportedName);
    return ResolveNamedImportBinding(bindingInfo, targetSurface, exportedName, resolved, options);
}

ResolvedImportBindingResult ExportClosureResolver::ResolveNamespaceImportBinding(
    const varbinder::ImportBindingInfo *bindingInfo, const varbinder::ExportSurfaceId &targetSurface)
{
    if (HasOnlyDefaultExport(targetSurface) && checker_ != nullptr && bindingInfo->origin != nullptr) {
        checker_->LogError(diagnostic::DEFAULT_EXPORT_DIRECT_IMPORTED, ImportBindingReportPosition(bindingInfo));
    }
    ResolvedImportBindingResult result;
    result.status = ImportBindingResolutionStatus::RESOLVED_SURFACE;
    result.surface = targetSurface;
    return result;
}

ResolvedImportBindingResult ExportClosureResolver::ResolveNamedImportBinding(
    const varbinder::ImportBindingInfo *bindingInfo, const varbinder::ExportSurfaceId &targetSurface,
    util::StringView exportedName, const ResolvedExportResult *resolved, const ImportBindingResolveOptions &options)
{
    const auto status = resolved == nullptr ? ExportResolutionStatus::NOT_FOUND : resolved->status;
    switch (status) {
        case ExportResolutionStatus::RESOLVED: {
            auto entry = SelectMaterializedImportEntry(bindingInfo, targetSurface, exportedName, *resolved);
            return {entry.surface.program != nullptr ? ImportBindingResolutionStatus::RESOLVED_SURFACE
                                                     : ImportBindingResolutionStatus::RESOLVED_VARIABLE,
                    entry, entry.surface, exportedName, resolved->reportOrigin};
        }
        case ExportResolutionStatus::AMBIGUOUS:
            ReportAmbiguousExport(exportedName, resolved != nullptr ? resolved->reportOrigin : bindingInfo->origin);
            return {ImportBindingResolutionStatus::AMBIGUOUS, {}, {}, exportedName};
        case ExportResolutionStatus::NOT_FOUND:
            if (bindingInfo->kind == varbinder::ImportBindingKind::NAMED &&
                MatchesDefaultExportName(ResolveExportName(targetSurface, "default"), exportedName)) {
                if (checker_ != nullptr && bindingInfo->origin != nullptr) {
                    checker_->LogError(diagnostic::DEFAULT_EXPORT_DIRECT_IMPORTED, bindingInfo->origin->Start());
                }
                return {ImportBindingResolutionStatus::INVALID, {}, {}, exportedName};
            }
            ReportImportedNameNotFound(bindingInfo, exportedName, targetSurface, options);
            return {ImportBindingResolutionStatus::NOT_FOUND, {}, {}, exportedName};
        case ExportResolutionStatus::INVALID:
            if (options.reportDiagnostics && checker_ != nullptr && bindingInfo->origin != nullptr) {
                checker_->LogError(diagnostic::CYCLIC_EXPORT, bindingInfo->origin->Start());
            }
            return {ImportBindingResolutionStatus::INVALID, {}, {}, exportedName};
        default:
            ES2PANDA_UNREACHABLE();
    }
}

varbinder::Variable *ExportClosureResolver::ResolveEffectiveImportVariable(varbinder::Variable *var,
                                                                           ImportBindingResolveOptions options)
{
    std::unordered_set<varbinder::Variable *> visited;
    while (var != nullptr && var->IsLocalVariable() && var->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
        if (!visited.insert(var).second) {
            return nullptr;
        }

        auto *bindingInfo = var->AsLocalVariable()->ImportBinding();
        if (bindingInfo == nullptr) {
            return nullptr;
        }

        if (bindingInfo->resolvedVariable != nullptr) {
            var = bindingInfo->resolvedVariable;
            continue;
        }

        auto resolved = ResolveImportBinding(bindingInfo, options);
        if (resolved.status != ImportBindingResolutionStatus::RESOLVED_VARIABLE || resolved.entry.variable == nullptr) {
            return nullptr;
        }

        bindingInfo->resolvedVariable = resolved.entry.variable;
        var = resolved.entry.variable;
    }

    return var;
}

varbinder::Variable *ExportClosureResolver::ResolveEffectiveImportVariableForDeclaration(
    const varbinder::ImportBindingInfo *bindingInfo)
{
    if (bindingInfo == nullptr) {
        return nullptr;
    }

    auto *program = FindOwningProgram(bindingInfo->origin);
    if (program == nullptr) {
        return nullptr;
    }

    ExportClosureResolver resolver(program->Allocator(), nullptr);
    auto result = resolver.ResolveImportBinding(bindingInfo, {false, false});
    if (result.status == ImportBindingResolutionStatus::RESOLVED_VARIABLE) {
        return resolver.ResolveEffectiveImportVariable(result.entry.variable, {false, false});
    }

    if (bindingInfo->kind != varbinder::ImportBindingKind::NAMED) {
        return nullptr;
    }

    auto targetSurface = resolver.GetImportedSurface(program, bindingInfo->importDecl);
    auto *resolved = targetSurface.program == nullptr
                         ? nullptr
                         : resolver.ResolveExportNameWithoutAmbiguousDiagnostic(targetSurface, "default");
    if (resolved == nullptr || resolved->status != ExportResolutionStatus::RESOLVED ||
        resolved->entry.variable == nullptr || resolved->entry.variable->Name() != bindingInfo->importedName) {
        return nullptr;
    }
    return resolver.ResolveEffectiveImportVariable(resolved->entry.variable, {false, false});
}

void ExportClosureResolver::ReportAmbiguousExport(util::StringView exportedName, const ir::AstNode *origin) const
{
    if (reportAmbiguousExport_ && checker_ != nullptr && origin != nullptr) {
        checker_->LogError(diagnostic::AMBIGUOUS_EXPORT, {exportedName}, origin->Start());
    }
}

void ExportClosureResolver::ReportStarExportAmbiguity(util::StringView exportedName, const ir::AstNode *origin) const
{
    if (checker_ != nullptr && origin != nullptr) {
        checker_->LogDiagnostic(diagnostic::ALREADY_EXPORTED, {exportedName}, origin->Start());
    }
}

void ExportClosureResolver::ReportIncorrectNamedReExport(util::StringView importedName, const ir::AstNode *origin) const
{
    if (checker_ != nullptr && origin != nullptr) {
        checker_->LogError(diagnostic::EXPORT_INCORRECT, {importedName}, origin->Start());
    }
}

void ExportClosureResolver::ReportImportPathNotFound(const varbinder::ImportBindingInfo *bindingInfo,
                                                     const ImportBindingResolveOptions &options) const
{
    if (!options.reportDiagnostics || checker_ == nullptr || bindingInfo == nullptr || bindingInfo->origin == nullptr) {
        return;
    }

    const auto importPath =
        bindingInfo->importDecl != nullptr ? bindingInfo->importDecl->ResolvedSource() : util::StringView {};
    checker_->LogError(diagnostic::IMPORT_NOT_FOUND_2, {importPath}, bindingInfo->origin->Start());
}

bool HasSyntaxDiagnosticForProgram(Checker *checker, const parser::Program *program)
{
    if (checker == nullptr || program == nullptr) {
        return false;
    }

    const auto sourceFile = program->SourceFilePath().Utf8();
    const auto sourceBase = util::BaseName(sourceFile);
    for (const auto &diagnostic : checker->DiagnosticEngine().GetDiagnosticStorage(util::DiagnosticType::SYNTAX)) {
        if (diagnostic == nullptr) {
            continue;
        }

        const auto &diagnosticFile = diagnostic->File();
        if (diagnosticFile == sourceFile || util::BaseName(diagnosticFile) == sourceBase) {
            return true;
        }
    }

    return false;
}

void ExportClosureResolver::ReportImportedNameNotFound(const varbinder::ImportBindingInfo *bindingInfo,
                                                       util::StringView exportedName,
                                                       const varbinder::ExportSurfaceId &targetSurface,
                                                       const ImportBindingResolveOptions &options) const
{
    if (!options.reportDiagnostics || checker_ == nullptr || bindingInfo == nullptr || bindingInfo->origin == nullptr) {
        return;
    }

    if (bindingInfo->kind == varbinder::ImportBindingKind::DEFAULT) {
        checker_->LogError(diagnostic::DEFAULT_IMPORT_NOT_FOUND, {}, bindingInfo->origin->Start());
        return;
    }

    if (!HasSyntaxDiagnosticForProgram(checker_, targetSurface.program) &&
        (HasHiddenLocalExportAlias(targetSurface, exportedName) ||
         !HasProgramLocalDeclaration(targetSurface.program, exportedName))) {
        checker_->LogError(diagnostic::IMPORT_NOT_FOUND, {exportedName}, bindingInfo->origin->Start());
        return;
    }

    checker_->LogError(diagnostic::IMPORTED_NOT_EXPORTED, {exportedName}, bindingInfo->origin->Start());
}

bool ExportClosureResolver::HasHiddenLocalExportAlias(const varbinder::ExportSurfaceId &surface,
                                                      util::StringView localName) const
{
    const auto &snapshot = GetSnapshot(surface.program);
    for (const auto &fact : snapshot.locals) {
        if (fact.localName == localName && fact.exportedName != localName) {
            return true;
        }
    }
    return false;
}

varbinder::ExportSurfaceId ExportClosureResolver::GetSurface(parser::Program *program) const
{
    if (const auto *store = GetFactStore(program); store != nullptr) {
        if (const auto *surface = store->FindSurfaceByProgram(program); surface != nullptr) {
            return *surface;
        }
    }

    if (program == nullptr) {
        return {};
    }

    return varbinder::ExportSurfaceId {
        program->Is<util::ModuleKind::PACKAGE>() ? varbinder::ExportSurfaceKind::Package
                                                 : varbinder::ExportSurfaceKind::Program,
        program,
        util::StringView {program->ModuleName()},
        util::StringView {program->GetImportInfo().ResolvedSource()},
    };
}

const ResolvedExportResult *ExportClosureResolver::ResolveSurfaceName(const varbinder::ExportSurfaceId &surface,
                                                                      util::StringView exportedName,
                                                                      VisitingSet *visiting)
{
    const auto key = MakeKey(surface, exportedName);
    if (auto it = memo_.find(key); it != memo_.end()) {
        return &(it->second);
    }

    if (surface.program == nullptr) {
        auto [it, _] =
            memo_.emplace(MakePersistentKey(surface, exportedName), MakeResult(ExportResolutionStatus::NOT_FOUND));
        return &(it->second);
    }

    VisitingGuard guard(visiting, surface, exportedName);
    if (guard.IsCycle()) {
        return CycleInvalidResult();
    }

    auto resolved = surface.kind == varbinder::ExportSurfaceKind::Package
                        ? ResolvePackageSurface(surface, exportedName, visiting)
                        : ResolveProgramSurface(surface, exportedName, visiting);
    if (resolved.status == ExportResolutionStatus::INVALID) {
        transientInvalidResult_ = resolved;
        return &transientInvalidResult_;
    }

    auto [it, _] = memo_.emplace(MakePersistentKey(surface, exportedName), resolved);
    return &(it->second);
}

ResolvedExportResult ExportClosureResolver::ResolvePackageSurface(const varbinder::ExportSurfaceId &surface,
                                                                  util::StringView exportedName, VisitingSet *visiting)
{
    auto resolved = MakeResult(ExportResolutionStatus::NOT_FOUND);

    auto resolveFraction = [this, exportedName, visiting, &resolved](parser::Program *fraction) {
        const auto *entry = ResolveSurfaceName(GetSurface(fraction), exportedName, visiting);
        const auto candidate = ResultOrNotFound(entry);
        if (MergeResolvedResults(&resolved, candidate) == MergeOutcome::NEW_AMBIGUOUS) {
            ReportAmbiguousExport(exportedName, candidate.reportOrigin);
        }
    };
    if (ForEachPackageFraction(surface, resolveFraction)) {
        return resolved;
    }
    return ResolveProgramSurface(surface, exportedName, visiting);
}

ResolvedExportResult ExportClosureResolver::ResolveProgramSurface(const varbinder::ExportSurfaceId &surface,
                                                                  util::StringView exportedName, VisitingSet *visiting)
{
    // Phase 1: explicit exports (local + named re-export).
    auto resolved = ResolveLocalExport(surface, exportedName);
    auto next = ResolveNamedReExport(surface, exportedName, visiting);
    if (MergeResolvedResults(&resolved, next) == MergeOutcome::NEW_AMBIGUOUS) {
        ReportAmbiguousExport(exportedName, next.reportOrigin);
    }

    // Explicit exports take precedence over star/namespace propagation.
    if (resolved.status != ExportResolutionStatus::NOT_FOUND) {
        return resolved;
    }

    // Phase 2: implicit propagation — only consulted when no explicit export matched.
    next = ResolveNamespaceExport(surface, exportedName);
    if (MergeResolvedResults(&resolved, next) == MergeOutcome::NEW_AMBIGUOUS) {
        ReportAmbiguousExport(exportedName, next.reportOrigin);
    }

    next = ResolveStarExport(surface, exportedName, visiting);
    if (MergeResolvedResults(&resolved, next) == MergeOutcome::NEW_AMBIGUOUS) {
        ReportAmbiguousExport(exportedName, next.reportOrigin);
    }

    return resolved;
}

ResolvedExportResult ExportClosureResolver::ResolveLocalExport(const varbinder::ExportSurfaceId &surface,
                                                               util::StringView exportedName)
{
    return ResolveMatchingFacts(GetSnapshot(surface.program).locals, exportedName, surface.program);
}

ResolvedExportResult ExportClosureResolver::ResolveNamedReExport(const varbinder::ExportSurfaceId &surface,
                                                                 util::StringView exportedName, VisitingSet *visiting)
{
    auto resolved = MakeResult(ExportResolutionStatus::NOT_FOUND);
    for (const auto &fact : GetSnapshot(surface.program).namedReExports) {
        if (fact.exportedName != exportedName) {
            continue;
        }

        const auto exactSurface = GetImportedSurface(fact);
        const auto *entry = ResolveSurfaceName(exactSurface, fact.importedName, visiting);
        auto candidate = RebindResultOrNotFound(entry, fact.origin);
        candidate = SelectMaterializedReExportResult(fact, exactSurface, fact.importedName, candidate);
        if (candidate.status == ExportResolutionStatus::NOT_FOUND) {
            ReportIncorrectNamedReExport(fact.importedName, fact.origin);
            continue;
        }

        if (MergeResolvedResults(&resolved, candidate) == MergeOutcome::NEW_AMBIGUOUS) {
            ReportAmbiguousExport(exportedName, fact.origin);
        }
    }

    return resolved;
}

NameResolutionKey ExportClosureResolver::MakeKey(const varbinder::ExportSurfaceId &surface,
                                                 util::StringView exportedName) const
{
    return {surface.kind, surface.program, exportedName};
}

NameResolutionKey ExportClosureResolver::MakePersistentKey(const varbinder::ExportSurfaceId &surface,
                                                           util::StringView exportedName) const
{
    ES2PANDA_ASSERT(allocator_ != nullptr);
    return MakeKey(surface, util::UString(exportedName, allocator_).View());
}

bool ExportClosureResolver::HasOnlyDefaultExport(const varbinder::ExportSurfaceId &surface)
{
    auto *defaultExport = ResolveExportName(surface, "default");
    if (defaultExport == nullptr || defaultExport->status != ExportResolutionStatus::RESOLVED) {
        return false;
    }

    VisitingSet visiting;
    ValidateExportedDeclarations(surface, &visiting);
    for (const auto &[key, resolved] : memo_) {
        if (!IsMemoKeyForSurface(key, surface) || key.exportedName.Is("default")) {
            continue;
        }

        if (resolved.status != ExportResolutionStatus::NOT_FOUND) {
            return false;
        }
    }
    return true;
}

ResolvedExportResult ExportClosureResolver::ResolveNamespaceExportMember(const varbinder::ExportSurfaceId &surface,
                                                                         util::StringView exportedName)
{
    auto resolved = MakeResult(ExportResolutionStatus::NOT_FOUND);
    VisitingSet visiting;
    for (const auto &fact : GetSnapshot(surface.program).namespaceExports) {
        const auto exactSurface = GetImportedSurface(fact);
        const auto *entry = ResolveSurfaceName(exactSurface, exportedName, &visiting);
        auto candidate = RebindResultOrNotFound(entry, fact.origin);
        candidate = SelectMaterializedReExportResult(fact, exactSurface, exportedName, candidate);
        if (candidate.status == ExportResolutionStatus::NOT_FOUND) {
            continue;
        }

        (void)MergeResolvedResults(&resolved, candidate);
    }

    return resolved;
}

void ExportClosureResolver::ValidateExplicitExportConflicts(const varbinder::ExportSurfaceId &surface,
                                                            VisitingSet *visiting)
{
    ExplicitExportConflictState state;
    const auto &snapshot = GetSnapshot(surface.program);
    for (const auto &fact : snapshot.locals) {
        ValidateExplicitExportFact(&state, fact, MakeResolvedResult(fact.variable, fact.origin, surface.program));
    }

    for (const auto &fact : snapshot.namedReExports) {
        const auto *resolved = ResolveSurfaceName(GetImportedSurface(fact), fact.importedName, visiting);
        ValidateExplicitExportFact(&state, fact, ResultOrNotFound(resolved));
    }

    for (const auto &fact : snapshot.namespaceExports) {
        ValidateExplicitExportFact(&state, fact,
                                   MakeResolvedSurfaceResult(GetImportedSurface(fact), fact.origin, surface.program));
    }
}

void ExportClosureResolver::ValidateExplicitExportFact(ExplicitExportConflictState *state,
                                                       const varbinder::ExportFact &fact,
                                                       ResolvedExportResult candidate)
{
    if (fact.isInvalid || fact.exportedName.Empty()) {
        return;
    }

    candidate = RebindReportOrigin(candidate, fact.origin);
    if (candidate.status != ExportResolutionStatus::RESOLVED) {
        return;
    }

    ValidateExplicitExportLocalAlias(state, fact, candidate);
    ValidateExplicitExportNameConflict(state, fact, candidate);
}

void ExportClosureResolver::ValidateExplicitExportLocalAlias(ExplicitExportConflictState *state,
                                                             const varbinder::ExportFact &fact,
                                                             const ResolvedExportResult &candidate) const
{
    if (fact.localName.Empty()) {
        return;
    }

    auto localName = std::string(fact.localName.Utf8());
    if (fact.exportedName.Is("default")) {
        state->seenDefaultByLocalName.emplace(localName, candidate);
        return;
    }

    auto &sameLocalExports =
        fact.exportedName.Is("default") ? state->seenNamedByLocalName : state->seenDefaultByLocalName;
    if (auto localIt = sameLocalExports.find(localName);
        localIt != sameLocalExports.end() && IsSameResolvedExport(localIt->second, candidate) &&
        state->warnedAliases.insert(localName).second && checker_ != nullptr && fact.origin != nullptr &&
        !fact.origin->IsOverloadDeclaration()) {
        checker_->LogDiagnostic(diagnostic::DUPLICATE_EXPORT_ALIASES, {fact.localName}, fact.origin->Start());
    }

    auto &currentLocalExports =
        fact.exportedName.Is("default") ? state->seenDefaultByLocalName : state->seenNamedByLocalName;
    currentLocalExports.emplace(localName, candidate);
}

void ExportClosureResolver::ValidateExplicitExportNameConflict(ExplicitExportConflictState *state,
                                                               const varbinder::ExportFact &fact,
                                                               const ResolvedExportResult &candidate)
{
    if (fact.exportedName.Is("default")) {
        return;
    }

    auto name = std::string(fact.exportedName.Utf8());
    auto [it, inserted] = state->seen.emplace(name, candidate);
    if (inserted) {
        state->seenFacts.emplace(name, &fact);
        return;
    }

    auto factIt = state->seenFacts.find(name);
    if (factIt != state->seenFacts.end() && IsPlainLocalDeclarationExport(*(factIt->second)) &&
        IsPlainLocalDeclarationExport(fact)) {
        return;
    }

    if (IsSameResolvedExport(it->second, candidate)) {
        if (factIt != state->seenFacts.end() && IsSameNamedReExportEdge(*(factIt->second), fact)) {
            return;
        }
        if (state->warnedAliases.insert(name).second && checker_ != nullptr && fact.origin != nullptr &&
            !fact.origin->IsOverloadDeclaration()) {
            checker_->LogDiagnostic(diagnostic::DUPLICATE_EXPORT_ALIASES, {fact.exportedName}, fact.origin->Start());
        }
        return;
    }

    if (state->warnedAliases.insert(name).second && checker_ != nullptr && fact.origin != nullptr) {
        checker_->LogDiagnostic(diagnostic::DUPLICATE_EXPORT_ALIASES, {fact.exportedName}, fact.origin->Start());
    }

    if (MergeResolvedResults(&(it->second), candidate) == MergeOutcome::NEW_AMBIGUOUS) {
        ReportAmbiguousExport(fact.exportedName, fact.origin);
    }
}

void ExportClosureResolver::ValidateExportedDeclarations(const varbinder::ExportSurfaceId &surface,
                                                         VisitingSet *visiting)
{
    std::unordered_set<const ir::AstNode *> checkedOrigins;
    auto validateProgram = [this, &surface, visiting, &checkedOrigins](parser::Program *program) {
        ValidateProgramExportedDeclarations(surface, program, visiting, &checkedOrigins);
    };
    if (!ForEachPackageFraction(surface, validateProgram)) {
        validateProgram(surface.program);
    }
}

void ExportClosureResolver::ValidateProgramExportedDeclarations(const varbinder::ExportSurfaceId &surface,
                                                                parser::Program *program, VisitingSet *visiting,
                                                                std::unordered_set<const ir::AstNode *> *checkedOrigins)
{
    if (program == nullptr) {
        return;
    }

    const auto &snapshot = GetSnapshot(program);
    auto validateFact = [this, &surface, visiting, checkedOrigins](const varbinder::ExportFact &fact) {
        if (!fact.isInvalid && !fact.exportedName.Empty()) {
            CheckResolvedExportDeclaration(ResolveSurfaceName(surface, fact.exportedName, visiting), checkedOrigins);
        }
    };
    std::for_each(snapshot.locals.begin(), snapshot.locals.end(), validateFact);
    std::for_each(snapshot.namedReExports.begin(), snapshot.namedReExports.end(), validateFact);
    std::for_each(snapshot.namespaceExports.begin(), snapshot.namespaceExports.end(), validateFact);

    for (const auto &fact : snapshot.starExports) {
        ValidateStarExportedDeclarations(surface, fact, visiting, checkedOrigins);
    }
}

void ExportClosureResolver::ValidateStarExportedDeclarations(const varbinder::ExportSurfaceId &surface,
                                                             const varbinder::ExportFact &fact, VisitingSet *visiting,
                                                             std::unordered_set<const ir::AstNode *> *checkedOrigins)
{
    auto targetSurface = GetImportedSurface(fact);
    ValidateExportSurface(targetSurface);
    for (const auto &entry : memo_) {
        const auto &key = entry.first;
        if (IsMemoKeyForSurface(key, targetSurface) && !key.exportedName.Is("default")) {
            CheckResolvedExportDeclaration(ResolveSurfaceName(surface, key.exportedName, visiting), checkedOrigins);
        }
    }
}

void ExportClosureResolver::CheckResolvedExportDeclaration(
    const ResolvedExportResult *resolved, std::unordered_set<const ir::AstNode *> *checkedOrigins) const
{
    if (resolved == nullptr || resolved->status != ExportResolutionStatus::RESOLVED ||
        !checkedOrigins->insert(resolved->entry.origin).second) {
        return;
    }

    CheckExportedVariableTypeAnnotation(checker_, resolved->entry);
}

ResolvedExportResult ExportClosureResolver::ResolveNamespaceExport(const varbinder::ExportSurfaceId &surface,
                                                                   util::StringView exportedName)
{
    auto resolved = MakeResult(ExportResolutionStatus::NOT_FOUND);
    for (const auto &fact : GetSnapshot(surface.program).namespaceExports) {
        if (fact.exportedName != exportedName) {
            continue;
        }

        (void)MergeResolvedResults(&resolved,
                                   MakeResolvedSurfaceResult(GetImportedSurface(fact), fact.origin, surface.program));
    }

    return resolved;
}

ResolvedExportResult ExportClosureResolver::ResolveStarExport(const varbinder::ExportSurfaceId &surface,
                                                              util::StringView exportedName, VisitingSet *visiting)
{
    if (exportedName.Is("default")) {
        return MakeResult(ExportResolutionStatus::NOT_FOUND);
    }

    auto resolved = MakeResult(ExportResolutionStatus::NOT_FOUND);
    auto cycleResult = MakeResult(ExportResolutionStatus::NOT_FOUND);
    for (const auto &fact : GetSnapshot(surface.program).starExports) {
        const auto exactSurface = GetImportedSurface(fact);
        const auto *entry = ResolveSurfaceName(exactSurface, exportedName, visiting);
        auto candidate = RebindResultOrNotFound(entry, fact.origin);
        candidate = SelectMaterializedReExportResult(fact, exactSurface, exportedName, candidate);
        if (candidate.status == ExportResolutionStatus::INVALID) {
            if (cycleResult.status == ExportResolutionStatus::NOT_FOUND) {
                cycleResult = candidate;
            }
            continue;
        }
        if (candidate.status == ExportResolutionStatus::NOT_FOUND) {
            continue;
        }

        if (MergeResolvedResults(&resolved, candidate) == MergeOutcome::NEW_AMBIGUOUS) {
            ReportStarExportAmbiguity(exportedName, fact.origin);
        }
    }

    if (resolved.status == ExportResolutionStatus::NOT_FOUND && cycleResult.status == ExportResolutionStatus::INVALID) {
        return cycleResult;
    }
    return resolved;
}

}  // namespace ark::es2panda::checker
