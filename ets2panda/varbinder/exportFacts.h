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

#ifndef ES2PANDA_VARBINDER_EXPORT_FACTS_H
#define ES2PANDA_VARBINDER_EXPORT_FACTS_H

#include "util/eheap.h"
#include "util/ustring.h"

#include <tuple>

namespace ark::es2panda::ir {
class AstNode;
class ETSImportDeclaration;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::varbinder {
class Variable;

enum class ExportSurfaceKind { Program, Package };

struct ExportSurfaceId {
    ExportSurfaceKind kind {ExportSurfaceKind::Program};
    parser::Program *program {};
    util::StringView moduleName {};
    util::StringView resolvedSource {};
    ExportSurfaceKind effectiveKind {ExportSurfaceKind::Program};
    parser::Program *effectiveProgram {};
};

struct PackageSurfaceFact {
    explicit PackageSurfaceFact(ArenaAllocator *allocator) : fractions(allocator->Adapter()) {}

    ArenaVector<parser::Program *> fractions;
};

enum class LocalExportKind { DECLARATION, ALIAS };

struct PendingLocalExportAlias {
    parser::Program *program {};
    util::StringView exportedName {};
    util::StringView localName {};
    const ir::AstNode *origin {};
    const ir::AstNode *exportDecl {};
    const ir::AstNode *reportOrigin {};
    bool originDeclaresName {};
    bool isTypeOnly {};
    bool isInvalid {};
    LocalExportKind kind {LocalExportKind::ALIAS};
};

struct ImportTargetKey {
    parser::Program *sourceProgram {};
    const ir::ETSImportDeclaration *importDecl {};

    bool operator<(const ImportTargetKey &other) const
    {
        return std::tie(sourceProgram, importDecl) < std::tie(other.sourceProgram, other.importDecl);
    }
};

struct ExportFact {
    parser::Program *sourceProgram {};
    util::StringView exportedName {};
    util::StringView localName {};
    util::StringView importedName {};
    const ir::ETSImportDeclaration *importDecl {};
    const ir::AstNode *origin {};
    // Binder only stores a directly attached local binding here.
    // Local exports and namespace exports have one, re-export edges do not.
    Variable *variable {};
    bool isTypeOnly {};
    bool isLocalAlias {};
    bool isInvalid {};
};

class ExportFactStore {
public:
    struct ExportFactSnapshot {
        explicit ExportFactSnapshot(ArenaAllocator *allocator)
            : locals(allocator->Adapter()),
              namedReExports(allocator->Adapter()),
              starExports(allocator->Adapter()),
              namespaceExports(allocator->Adapter())
        {
        }

        ArenaVector<ExportFact> locals;
        ArenaVector<ExportFact> namedReExports;
        ArenaVector<ExportFact> starExports;
        ArenaVector<ExportFact> namespaceExports;
    };

    explicit ExportFactStore(ArenaAllocator *allocator)
        : allocator_(allocator),
          facts_(allocator->Adapter()),
          surfacesByProgram_(allocator->Adapter()),
          surfaceByResolvedSource_(allocator->Adapter()),
          importTargets_(allocator->Adapter()),
          effectiveImportTargets_(allocator->Adapter()),
          packageSurfaces_(allocator->Adapter()),
          pendingLocalExportAliases_(allocator->Adapter()),
          emptySnapshot_(allocator),
          emptyPendingLocalExportAliases_(allocator->Adapter())
    {
    }

    const ExportFactSnapshot &GetExportFacts(parser::Program *program) const;
    const ExportSurfaceId *FindSurfaceByProgram(parser::Program *program) const;
    const ExportSurfaceId *FindSurfaceByResolvedSource(util::StringView resolvedSource) const;
    const PackageSurfaceFact *FindPackageSurface(parser::Program *program) const;
    void RegisterProgramSurface(parser::Program *program);
    void RegisterPackageSurface(parser::Program *program);
    void AddImportTarget(parser::Program *sourceProgram, const ir::ETSImportDeclaration *importDecl,
                         parser::Program *targetProgram);
    void AddEffectiveImportTarget(parser::Program *sourceProgram, const ir::ETSImportDeclaration *importDecl,
                                  parser::Program *targetProgram);
    const ExportSurfaceId *FindImportTarget(parser::Program *sourceProgram,
                                            const ir::ETSImportDeclaration *importDecl) const;
    const ExportSurfaceId *FindEffectiveImportTarget(parser::Program *sourceProgram,
                                                     const ir::ETSImportDeclaration *importDecl) const;
    template <typename Callback>
    bool AnyImportTargetForEffective(const parser::Program *sourceProgram, const parser::Program *effectiveProgram,
                                     Callback cb) const
    {
        if (sourceProgram == nullptr || effectiveProgram == nullptr) {
            return false;
        }

        for (const auto &[key, effectiveSurface] : effectiveImportTargets_) {
            if (key.sourceProgram != sourceProgram || effectiveSurface.program != effectiveProgram) {
                continue;
            }
            auto exactIt = importTargets_.find(key);
            if (exactIt != importTargets_.end() && cb(exactIt->second)) {
                return true;
            }
        }
        return false;
    }
    template <typename Callback>
    bool AnyImportTargetForEffective(const parser::Program *effectiveProgram, Callback cb) const
    {
        if (effectiveProgram == nullptr) {
            return false;
        }

        for (const auto &[key, effectiveSurface] : effectiveImportTargets_) {
            if (effectiveSurface.program != effectiveProgram) {
                continue;
            }
            auto exactIt = importTargets_.find(key);
            if (exactIt != importTargets_.end() && cb(exactIt->second)) {
                return true;
            }
        }
        return false;
    }
    void ResetProgram(parser::Program *program);
    void ClearImportTargets(parser::Program *sourceProgram);
    void AddLocalExport(parser::Program *program, util::StringView exportedName, Variable *variable,
                        const ir::AstNode *origin);
    void AddLocalExport(parser::Program *program, util::StringView exportedName, util::StringView localName,
                        Variable *variable, const ir::AstNode *origin);
    void AddLocalExportAlias(parser::Program *program, util::StringView exportedName, util::StringView localName,
                             Variable *variable, const ir::AstNode *origin, bool isTypeOnly = false,
                             bool isInvalid = false);
    bool AddPendingLocalExportAlias(parser::Program *program, util::StringView exportedName, util::StringView localName,
                                    const ir::AstNode *origin, const ir::AstNode *exportDecl,
                                    const ir::AstNode *reportOrigin, bool originDeclaresName, bool isTypeOnly,
                                    LocalExportKind kind);
    void MarkPendingLocalExportAliasInvalid(parser::Program *program, util::StringView exportedName,
                                            util::StringView localName, const ir::AstNode *reportOrigin);
    const ArenaVector<PendingLocalExportAlias> &PendingLocalExportAliases(parser::Program *program) const;
    void AddNamedReExport(parser::Program *program, const ir::ETSImportDeclaration *importDecl,
                          util::StringView exportedName, util::StringView importedName, const ir::AstNode *origin,
                          bool isTypeOnly);
    void AddStarExport(parser::Program *program, const ir::ETSImportDeclaration *importDecl, const ir::AstNode *origin,
                       bool isTypeOnly);
    void AddNamespaceExport(parser::Program *program, const ir::ETSImportDeclaration *importDecl,
                            util::StringView exportedName, Variable *variable, const ir::AstNode *origin,
                            bool isTypeOnly);

private:
    ExportFactSnapshot &GetOrCreateSnapshot(parser::Program *program);
    ExportSurfaceId MakeSurface(parser::Program *program) const;
    static bool IsTypeOnlyVariable(const Variable *variable);

    ArenaAllocator *allocator_;
    ArenaMap<parser::Program *, ExportFactSnapshot> facts_;
    ArenaMap<parser::Program *, ExportSurfaceId> surfacesByProgram_;
    ArenaMap<util::StringView, parser::Program *> surfaceByResolvedSource_;
    ArenaMap<ImportTargetKey, ExportSurfaceId> importTargets_;
    ArenaMap<ImportTargetKey, ExportSurfaceId> effectiveImportTargets_;
    ArenaMap<parser::Program *, PackageSurfaceFact> packageSurfaces_;
    ArenaMap<parser::Program *, ArenaVector<PendingLocalExportAlias>> pendingLocalExportAliases_;
    ExportFactSnapshot emptySnapshot_;
    ArenaVector<PendingLocalExportAlias> emptyPendingLocalExportAliases_;
};

}  // namespace ark::es2panda::varbinder

#endif
