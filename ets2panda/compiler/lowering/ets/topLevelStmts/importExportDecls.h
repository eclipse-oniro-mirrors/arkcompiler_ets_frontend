/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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
public:
    ImportExportDecls() = default;
    explicit ImportExportDecls(public_lib::Context *ctx)
        : varbinder_(ctx->parserProgram->VarBinder()->AsETSBinder()), parser_(ctx->parser->AsETSParser()), ctx_(ctx)
    {
    }

    /**
     * Add stdlib names to default imports
     */
    void IntroduceStdlibImportProgram();

    /**
     * Verifies import errors, and add Exported flag to top level variables and methods
     * @param global_stmts program global statements
     */
    void HandleGlobalStmts(parser::Program *programs);
    void ProcessProgramStatements(parser::Program *program, const ArenaVector<ir::Statement *> &statements);
    void VerifyTypeExports(parser::Program *programs);
    void VerifyType(ir::Statement *stmt, std::set<util::StringView> &exportedStatements,
                    std::map<util::StringView, ir::AstNode *> &typesMap);
    void HandleSimpleType(std::set<util::StringView> &exportedStatements, ir::Statement *stmt, util::StringView name);

    void VerifySingleExportDefault(parser::Program *programs);
    void AddExportFlags(ir::AstNode *node, bool hasAliasName);
    void AddTypeOnlyExportFlags(util::StringView originalFieldName);
    bool HandleSelectiveExportWithAlias(util::StringView originalFieldName, util::StringView exportName,
                                        lexer::SourcePosition startLoc);
    void PopulateAliasMap(parser::Program *program, const ir::ExportNamedDeclaration *decl);
    void PopulateAliasMap(parser::Program *program, const ir::TSTypeAliasDeclaration *decl);
    void VerifyCollectedExportName(const parser::Program *program);
    void CollectNamespaceDeclarations(const parser::Program *program);
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
    void VisitETSReExportDeclaration(ir::ETSReExportDeclaration *reExportDecl) override;
    void VisitAnnotationDeclaration(ir::AnnotationDeclaration *annotationDecl) override;
    void VisitETSModule(ir::ETSModule *etsModule) override;

private:
    void ProcessDestructuringElements(ir::ETSDestructuring *destructuring, ir::VariableDeclaration *varDecl);
    void CheckDuplicateExportName(util::StringView exportName, util::StringView localName,
                                  const ir::AstNode *reportNode);
    void CheckDuplicateReExportName(util::StringView exportName, util::StringView importedName,
                                    util::StringView sourceName, const ir::AstNode *reportNode);
    void ReportUnresolvedValueExport(const varbinder::PendingLocalExportAlias &alias, util::StringView originalName,
                                     lexer::SourcePosition startLoc, std::set<util::StringView> &unresolvedAliases,
                                     std::set<util::StringView> &warnedUnresolvedAliases);
    bool VerifyTypeOnlyExportAlias(const parser::Program *program, const varbinder::PendingLocalExportAlias &alias,
                                   util::StringView exportName, util::StringView originalName, ir::AstNode *localDecl,
                                   bool hasImportedSpecifier, lexer::SourcePosition startLoc,
                                   const ir::AstNode *reportOrigin);
    void VerifyCollectedExportAlias(const parser::Program *program, const varbinder::PendingLocalExportAlias &alias,
                                    std::set<util::StringView> &unresolvedAliases,
                                    std::set<util::StringView> &warnedUnresolvedAliases);

    varbinder::ETSBinder *varbinder_ {nullptr};
    std::map<util::StringView, ir::AstNode *> fieldMap_;
    std::map<util::StringView, util::StringView> exportNameMap_;
    std::map<util::StringView, std::pair<util::StringView, util::StringView>> reExportNameMap_;
    std::set<util::StringView> exportedTypes_;
    parser::ETSParser *parser_ {nullptr};
    public_lib::Context *ctx_ {nullptr};
    std::map<util::StringView, util::StringView> importedSpecifiersForExportCheck_;
    lexer::SourcePosition lastExportErrorPos_ {};
    util::StringView exportDefaultName_;
    size_t namespaceDepth_ {0};

    friend class SavedImportExportDeclsContext;
};

class SavedImportExportDeclsContext {
public:
    explicit SavedImportExportDeclsContext(ImportExportDecls *imExDecl)
        : imExDecl_(imExDecl),
          fieldMapPrev_(imExDecl_->fieldMap_),
          exportNameMapPrev_(imExDecl_->exportNameMap_),
          reExportNameMapPrev_(imExDecl_->reExportNameMap_),
          exportedTypesPrev_(imExDecl_->exportedTypes_),
          exportDefaultNamePrev_(imExDecl_->exportDefaultName_),
          namespaceDepthPrev_(imExDecl_->namespaceDepth_)
    {
        ClearImportExportDecls();
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
        imExDecl_->reExportNameMap_.clear();
        imExDecl_->exportedTypes_.clear();
        imExDecl_->exportDefaultName_ = nullptr;
        imExDecl_->namespaceDepth_ = 0;
    }

    void RestoreImportExportDecls() noexcept
    {
        imExDecl_->fieldMap_ = fieldMapPrev_;
        imExDecl_->exportNameMap_ = exportNameMapPrev_;
        imExDecl_->reExportNameMap_ = reExportNameMapPrev_;
        imExDecl_->exportedTypes_ = exportedTypesPrev_;
        imExDecl_->exportDefaultName_ = exportDefaultNamePrev_;
        imExDecl_->namespaceDepth_ = namespaceDepthPrev_;
    }

private:
    ImportExportDecls *imExDecl_;
    std::map<util::StringView, ir::AstNode *> fieldMapPrev_;
    std::map<util::StringView, util::StringView> exportNameMapPrev_;
    std::map<util::StringView, std::pair<util::StringView, util::StringView>> reExportNameMapPrev_;
    std::set<util::StringView> exportedTypesPrev_;
    util::StringView exportDefaultNamePrev_;
    size_t namespaceDepthPrev_;
};
}  // namespace ark::es2panda::compiler

#endif  // PANDA_IMPORTEXPORTDECLS_H
