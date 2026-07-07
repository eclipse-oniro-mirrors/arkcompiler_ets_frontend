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

#ifndef ES2PANDA_CHECKER_EXPORT_CLOSURE_RESOLVER_H
#define ES2PANDA_CHECKER_EXPORT_CLOSURE_RESOLVER_H

#include <unordered_set>

#include "util/eheap.h"
#include "util/ustring.h"
#include "varbinder/exportFacts.h"

namespace ark::es2panda::ir {
class AstNode;
class ETSImportDeclaration;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::varbinder {
struct ImportBindingInfo;
class Variable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::checker {

class Checker;
class ExportClosureResolver;

struct NameResolutionKey {
    varbinder::ExportSurfaceKind surfaceKind {varbinder::ExportSurfaceKind::Program};
    parser::Program *surfaceProgram {};
    util::StringView exportedName {};

    bool operator==(const NameResolutionKey &other) const
    {
        return surfaceKind == other.surfaceKind && surfaceProgram == other.surfaceProgram &&
               exportedName == other.exportedName;
    }
};

struct NameResolutionKeyHash {
    size_t operator()(const NameResolutionKey &key) const
    {
        auto hash = std::hash<parser::Program *> {}(key.surfaceProgram);
        hash ^= std::hash<size_t> {}(static_cast<size_t>(key.surfaceKind)) << 1U;
        hash ^= std::hash<util::StringView> {}(key.exportedName) << 2U;
        return hash;
    }
};

struct NameResolutionKeyLess {
    bool operator()(const NameResolutionKey &lhs, const NameResolutionKey &rhs) const
    {
        if (lhs.surfaceKind != rhs.surfaceKind) {
            return lhs.surfaceKind < rhs.surfaceKind;
        }
        if (lhs.surfaceProgram != rhs.surfaceProgram) {
            return lhs.surfaceProgram < rhs.surfaceProgram;
        }
        return lhs.exportedName < rhs.exportedName;
    }
};

struct ResolvedExportEntry {
    varbinder::Variable *variable {};
    varbinder::ExportSurfaceId surface {};
    const ir::AstNode *origin {};
    parser::Program *originProgram {};
    // Checker-facing annotation-use result. `import type` is not a general value-use barrier in ArkTS, but annotations
    // do not define types, so an annotation path becomes invalid when it crosses `export type` or `import type`.
    // When multiple paths resolve to the same declaration, this remains true only if every path is type-only.
    bool isTypeOnlyUse {};
};

enum class ExportResolutionStatus {
    RESOLVED,
    INVALID,
    AMBIGUOUS,
    NOT_FOUND,
};

struct ResolvedExportResult {
    ExportResolutionStatus status {ExportResolutionStatus::NOT_FOUND};
    ResolvedExportEntry entry {};
    const ir::AstNode *reportOrigin {};
};

enum class ImportBindingResolutionStatus {
    RESOLVED_VARIABLE,
    RESOLVED_SURFACE,
    AMBIGUOUS,
    NOT_FOUND,
    INVALID,
};

struct ImportBindingResolveOptions {
    bool reportDiagnostics {true};
    bool validateSurface {true};
};

struct ResolvedImportBindingResult {
    ImportBindingResolutionStatus status {ImportBindingResolutionStatus::INVALID};
    ResolvedExportEntry entry {};
    varbinder::ExportSurfaceId surface {};
    util::StringView exportedName {};
    const ir::AstNode *reportOrigin {};
};

class ResolvedExportCache {
public:
    explicit ResolvedExportCache(ArenaAllocator *allocator, ExportClosureResolver *resolver, parser::Program *program)
        : allocator_(allocator),
          resolver_(resolver),
          program_(program),
          entries_(allocator->Adapter()),
          attempted_(allocator->Adapter())
    {
    }

    const ResolvedExportEntry *Find(util::StringView exportedName) const;
    bool Has(util::StringView exportedName) const;
    bool HasAmbiguous(util::StringView exportedName) const;

private:
    const ResolvedExportResult *ResolveIfNeeded(util::StringView exportedName) const;
    util::StringView CopyName(util::StringView exportedName) const;

    ArenaAllocator *allocator_;
    ExportClosureResolver *resolver_;
    parser::Program *program_;
    mutable ArenaMap<util::StringView, ResolvedExportResult> entries_;
    mutable ArenaSet<util::StringView> attempted_;
};

class ExportClosureResolver {
public:
    explicit ExportClosureResolver(ArenaAllocator *allocator, Checker *checker)
        : allocator_(allocator),
          checker_(checker),
          memo_(allocator->Adapter()),
          validatingSurfaces_(allocator->Adapter()),
          validatedSurfaces_(allocator->Adapter())
    {
    }

    const ResolvedExportResult *ResolveExportName(const varbinder::ExportSurfaceId &surface,
                                                  util::StringView exportedName);
    const ResolvedExportResult *ResolveExportNameWithoutAmbiguousDiagnostic(const varbinder::ExportSurfaceId &surface,
                                                                            util::StringView exportedName);
    ResolvedExportResult SelectMaterializedSurfaceEntry(const varbinder::ExportSurfaceId &exactSurface,
                                                        util::StringView exportedName,
                                                        const ResolvedExportResult &exactResolved);
    ResolvedExportResult ResolveNamespaceExportMember(const varbinder::ExportSurfaceId &surface,
                                                      util::StringView exportedName);
    void ValidateExportSurface(parser::Program *program);
    varbinder::ExportSurfaceId GetImportedSurface(const ir::ETSImportDeclaration *importDecl) const;
    varbinder::ExportSurfaceId GetImportedSurface(parser::Program *sourceProgram,
                                                  const ir::ETSImportDeclaration *importDecl) const;
    varbinder::ExportSurfaceId GetSurface(parser::Program *program) const;
    ResolvedImportBindingResult ResolveImportBinding(const varbinder::ImportBindingInfo *bindingInfo,
                                                     ImportBindingResolveOptions options = {});
    varbinder::Variable *ResolveEffectiveImportVariable(varbinder::Variable *var,
                                                        ImportBindingResolveOptions options = {});
    void Clear();
    static varbinder::Variable *ResolveEffectiveImportVariableForDeclaration(
        const varbinder::ImportBindingInfo *bindingInfo);

private:
    struct ExplicitExportConflictState;
    struct NamedImportBindingResolveContext;

    const ResolvedExportResult *ResolveSurfaceName(
        const varbinder::ExportSurfaceId &surface, util::StringView exportedName,
        std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting);
    ResolvedExportResult ResolvePackageSurface(const varbinder::ExportSurfaceId &surface, util::StringView exportedName,
                                               std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting);
    ResolvedExportResult ResolveProgramSurface(const varbinder::ExportSurfaceId &surface, util::StringView exportedName,
                                               std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting);
    ResolvedExportResult ResolveExplicitExports(const varbinder::ExportSurfaceId &surface,
                                                util::StringView exportedName,
                                                std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting);
    ResolvedExportResult ResolveImplicitExports(const varbinder::ExportSurfaceId &surface,
                                                util::StringView exportedName,
                                                std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting);
    ResolvedExportResult ResolveLocalExport(const varbinder::ExportSurfaceId &surface, util::StringView exportedName,
                                            std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting);
    ResolvedExportResult ResolveLocalExportFact(const varbinder::ExportFact &fact, parser::Program *originProgram,
                                                std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting);
    ResolvedExportResult ResolveNamedReExport(const varbinder::ExportSurfaceId &surface, util::StringView exportedName,
                                              std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting);
    ResolvedExportResult ResolveNamespaceExport(const varbinder::ExportSurfaceId &surface,
                                                util::StringView exportedName);
    ResolvedExportResult ResolveStarExport(const varbinder::ExportSurfaceId &surface, util::StringView exportedName,
                                           std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting);
    void ValidateExportSurface(const varbinder::ExportSurfaceId &surface);
    void ValidateExplicitExportConflicts(const varbinder::ExportSurfaceId &surface,
                                         std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting);
    void ValidateExportedDeclarations(const varbinder::ExportSurfaceId &surface,
                                      std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting);
    void ValidateProgramExportedDeclarations(const varbinder::ExportSurfaceId &surface, parser::Program *program,
                                             std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting,
                                             std::unordered_set<const ir::AstNode *> *checkedOrigins);
    void ValidateStarExportedDeclarations(const varbinder::ExportSurfaceId &surface, const varbinder::ExportFact &fact,
                                          std::unordered_set<NameResolutionKey, NameResolutionKeyHash> *visiting,
                                          std::unordered_set<const ir::AstNode *> *checkedOrigins);
    void CheckResolvedExportDeclaration(const ResolvedExportResult *resolved,
                                        std::unordered_set<const ir::AstNode *> *checkedOrigins) const;
    NameResolutionKey MakeKey(const varbinder::ExportSurfaceId &surface, util::StringView exportedName) const;
    NameResolutionKey MakePersistentKey(const varbinder::ExportSurfaceId &surface, util::StringView exportedName) const;
    bool HasOnlyDefaultExport(const varbinder::ExportSurfaceId &surface);
    varbinder::ExportSurfaceId GetImportedSurface(const varbinder::ExportFact &fact) const;
    varbinder::ExportSurfaceId GetEffectiveImportedSurface(parser::Program *sourceProgram,
                                                           const ir::ETSImportDeclaration *importDecl) const;
    varbinder::ExportSurfaceId GetEffectiveImportedSurface(const varbinder::ExportFact &fact) const;
    varbinder::ExportSurfaceId GetEffectiveImportedSurface(const varbinder::ImportBindingInfo *bindingInfo) const;
    ResolvedExportEntry SelectMaterializedImportEntry(const varbinder::ImportBindingInfo *bindingInfo,
                                                      const varbinder::ExportSurfaceId &exactSurface,
                                                      util::StringView exportedName,
                                                      const ResolvedExportResult &exactResolved);
    ResolvedExportResult SelectMaterializedReExportResult(const varbinder::ExportFact &fact,
                                                          const varbinder::ExportSurfaceId &exactSurface,
                                                          util::StringView exportedName,
                                                          const ResolvedExportResult &exactResolved);
    ResolvedImportBindingResult ResolveNamespaceImportBinding(const varbinder::ImportBindingInfo *bindingInfo,
                                                              const varbinder::ExportSurfaceId &targetSurface);
    ResolvedImportBindingResult ResolveNamedImportBinding(const varbinder::ImportBindingInfo *bindingInfo,
                                                          const varbinder::ExportSurfaceId &targetSurface,
                                                          util::StringView exportedName,
                                                          const ResolvedExportResult *resolved,
                                                          const ImportBindingResolveOptions &options);
    void ValidateExplicitExportFact(ExplicitExportConflictState *state, const varbinder::ExportFact &fact,
                                    ResolvedExportResult candidate);
    void ValidateExplicitExportLocalAlias(ExplicitExportConflictState *state, const varbinder::ExportFact &fact,
                                          const ResolvedExportResult &candidate) const;
    void ValidateExplicitExportNameConflict(ExplicitExportConflictState *state, const varbinder::ExportFact &fact,
                                            const ResolvedExportResult &candidate);
    void ReportAmbiguousExport(util::StringView exportedName, const ir::AstNode *origin) const;
    void ReportStarExportAmbiguity(util::StringView exportedName, const ir::AstNode *origin) const;
    void ReportIncorrectNamedReExport(util::StringView importedName, const ir::AstNode *origin) const;
    void ReportImportPathNotFound(const varbinder::ImportBindingInfo *bindingInfo,
                                  const ImportBindingResolveOptions &options) const;
    void ReportImportedNameNotFound(const varbinder::ImportBindingInfo *bindingInfo, util::StringView exportedName,
                                    const varbinder::ExportSurfaceId &targetSurface,
                                    const ImportBindingResolveOptions &options) const;
    bool HasHiddenLocalExportAlias(const varbinder::ExportSurfaceId &surface, util::StringView localName) const;

    ArenaAllocator *allocator_ {};
    Checker *checker_ {};
    ArenaMap<NameResolutionKey, ResolvedExportResult, NameResolutionKeyLess> memo_;
    ArenaSet<NameResolutionKey, NameResolutionKeyLess> validatingSurfaces_;
    ArenaSet<NameResolutionKey, NameResolutionKeyLess> validatedSurfaces_;
    ResolvedExportResult transientInvalidResult_;
    bool reportAmbiguousExport_ {true};
};

}  // namespace ark::es2panda::checker

#endif
