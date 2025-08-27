/*
 * Copyright (c) 2023 - 2025 Huawei Device Co., Ltd.
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

#ifndef PANDA_IMPORTEXPORTDECLS_H
#define PANDA_IMPORTEXPORTDECLS_H

#include <parser/ETSparser.h>
#include "varbinder/ETSBinder.h"
#include "compiler/lowering/phase.h"
#include "ir/visitor/IterateAstVisitor.h"
#include "globalClassHandler.h"

namespace ark::es2panda::compiler {

class SavedImportExportDeclsContext;
class ImportExportDecls : ir::visitor::EmptyAstVisitor {
    static constexpr std::string_view DEFAULT_IMPORT_SOURCE_FILE = "<default_import>.ets";

public:
    ImportExportDecls() = default;
    ImportExportDecls(varbinder::ETSBinder *varbinder, parser::ETSParser *parser, public_lib::Context *ctx)
        : varbinder_(varbinder), parser_(parser), ctx_(ctx)
    {
    }

    /**
     * Add stdlib names to default imports
     */
    void ParseDefaultSources();

    /**
     * Verifies import errors, and add Exported flag to top level variables and methods
     * @param global_stmts program global statements
     */
    GlobalClassHandler::ModuleDependencies HandleGlobalStmts(ArenaVector<parser::Program *> &programs);
    void ProcessProgramStatements(parser::Program *program, const ArenaVector<ir::Statement *> &statements,
                                  GlobalClassHandler::ModuleDependencies &moduleDependencies);
    void VerifyTypeExports(const ArenaVector<parser::Program *> &programs);
    void VerifyType(ir::Statement *stmt, std::set<util::StringView> &exportedStatements,
                    std::map<util::StringView, ir::AstNode *> &typesMap);
    void HandleSimpleType(std::set<util::StringView> &exportedStatements, ir::Statement *stmt, util::StringView name);

    void VerifySingleExportDefault(const ArenaVector<parser::Program *> &programs);
    void AddExportFlags(ir::AstNode *node, util::StringView originalFieldName, bool exportedWithAlias);
    void HandleSelectiveExportWithAlias(util::StringView originalFieldName, util::StringView exportName,
                                        lexer::SourcePosition startLoc);
    void PopulateAliasMap(const ir::ExportNamedDeclaration *decl, const util::StringView &path);
    void PopulateAliasMap(const ir::TSTypeAliasDeclaration *decl, const util::StringView &path);
    void VerifyCollectedExportName(const parser::Program *program);
    void PreMergeNamespaces(parser::Program *program);

private:
    void VisitOverloadDeclaration(ir::OverloadDeclaration *overloadDeclaration) override;
    void VisitFunctionDeclaration(ir::FunctionDeclaration *funcDecl) override;
    void VisitVariableDeclaration(ir::VariableDeclaration *varDecl) override;
    void VisitExportNamedDeclaration(ir::ExportNamedDeclaration *exportDecl) override;
    void VisitClassDeclaration(ir::ClassDeclaration *classDecl) override;
    void VisitTSEnumDeclaration(ir::TSEnumDeclaration *enumDecl) override;
    void VisitTSTypeAliasDeclaration(ir::TSTypeAliasDeclaration *typeAliasDecl) override;
    void VisitTSInterfaceDeclaration(ir::TSInterfaceDeclaration *interfaceDecl) override;
    void VisitETSImportDeclaration(ir::ETSImportDeclaration *importDecl) override;
    void VisitAnnotationDeclaration(ir::AnnotationDeclaration *annotationDecl) override;
    void VisitETSModule(ir::ETSModule *etsModule) override;
    void HandleInitModuleCallExpression(ir::CallExpression *expr, parser::Program *program,
                                        GlobalClassHandler::ModuleDependencies &moduleDependencies);

private:
    varbinder::ETSBinder *varbinder_ {nullptr};
    std::map<util::StringView, ir::AstNode *> fieldMap_;
    std::map<util::StringView, lexer::SourcePosition> exportNameMap_;
    std::set<util::StringView> exportedTypes_;
    parser::ETSParser *parser_ {nullptr};
    public_lib::Context *ctx_ {nullptr};
    std::map<util::StringView, util::StringView> importedSpecifiersForExportCheck_;
    lexer::SourcePosition lastExportErrorPos_ {};
    util::StringView exportDefaultName_;

    friend class SavedImportExportDeclsContext;
};

class SavedImportExportDeclsContext {
public:
    explicit SavedImportExportDeclsContext(ImportExportDecls *imExDecl, parser::Program *program)
        : imExDecl_(imExDecl),
          program_(program),
          fieldMapPrev_(imExDecl_->fieldMap_),
          exportNameMapPrev_(imExDecl_->exportNameMap_),
          exportedTypesPrev_(imExDecl_->exportedTypes_),
          exportDefaultNamePrev_(imExDecl_->exportDefaultName_),
          exportAliasMultimapPrev_(SaveExportAliasMultimap())
    {
        ClearImportExportDecls();
        UpdateExportMap();
    }

    void RecoverExportAliasMultimap()
    {
        auto &exportMap = imExDecl_->varbinder_->GetSelectiveExportAliasMultimap();

        exportMap.erase(program_->SourceFilePath());
        exportMap.insert({program_->SourceFilePath(), exportAliasMultimapPrev_});
    }

    NO_COPY_SEMANTIC(SavedImportExportDeclsContext);
    DEFAULT_MOVE_SEMANTIC(SavedImportExportDeclsContext);

    ~SavedImportExportDeclsContext()
    {
        RestoreImportExportDecls();
    }

private:
    void ClearImportExportDecls()
    {
        imExDecl_->fieldMap_.clear();
        imExDecl_->exportNameMap_.clear();
        imExDecl_->exportedTypes_.clear();
        imExDecl_->exportDefaultName_ = nullptr;
    }

    void UpdateExportMap()
    {
        auto &exportMap = imExDecl_->varbinder_->GetSelectiveExportAliasMultimap();
        if (auto it = exportMap.find(program_->SourceFilePath()); it != exportMap.end()) {
            exportAliasMultimapPrev_ = it->second;
            exportMap.erase(it);
        }

        ArenaMap<util::StringView, std::pair<util::StringView, ir::AstNode const *>> newMap(
            program_->Allocator()->Adapter());
        exportMap.insert({program_->SourceFilePath(), newMap});
    }

    void RestoreImportExportDecls() noexcept
    {
        imExDecl_->fieldMap_ = fieldMapPrev_;
        imExDecl_->exportNameMap_ = exportNameMapPrev_;
        imExDecl_->exportedTypes_ = exportedTypesPrev_;
        imExDecl_->exportDefaultName_ = exportDefaultNamePrev_;
    }

    ArenaMap<util::StringView, std::pair<util::StringView, ir::AstNode const *>> SaveExportAliasMultimap()
    {
        auto &exportMap = imExDecl_->varbinder_->GetSelectiveExportAliasMultimap();
        const auto found = exportMap.find(program_->SourceFilePath());
        if (found == exportMap.end()) {
            return ArenaMap<util::StringView, std::pair<util::StringView, ir::AstNode const *>>(
                program_->Allocator()->Adapter());
        }
        return found->second;
    }

private:
    ImportExportDecls *imExDecl_;
    parser::Program *program_;
    std::map<util::StringView, ir::AstNode *> fieldMapPrev_;
    std::map<util::StringView, lexer::SourcePosition> exportNameMapPrev_;
    std::set<util::StringView> exportedTypesPrev_;
    util::StringView exportDefaultNamePrev_;
    ArenaMap<util::StringView, std::pair<util::StringView, ir::AstNode const *>> exportAliasMultimapPrev_;
};
}  // namespace ark::es2panda::compiler

#endif  // PANDA_IMPORTEXPORTDECLS_H
