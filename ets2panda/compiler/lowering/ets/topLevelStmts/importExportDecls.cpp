/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "compiler/lowering/ets/topLevelStmts/importExportDecls.h"
#include "compiler/lowering/util.h"
#include "generated/diagnostic.h"
#include "utils/arena_containers.h"

namespace ark::es2panda::compiler {

static bool ProgramFileNameLessThan(const parser::Program *a, const parser::Program *b)
{
    return a->FileName().Mutf8() < b->FileName().Mutf8();
}

void ImportExportDecls::ParseDefaultSources()
{
    std::string importStdlibFile;
    for (const auto &path : util::Helpers::StdLib()) {
        if (path == "std/math/consts") {
            // NOTE(dkofanov): for some reason, 'consts.ets' is imported manually in stdlib sources.
            // Without this 'if', definitions from this file are emitted to each file, polluting global namespace.
            // The 'math.ets' has the same problem, but due to it has tests which use, for instance, a global
            // 'abs()' function, it is not skipped for now.
            continue;
        }
        importStdlibFile += "import * from \"" + path + "\";";
    }
    auto imports = parser_->ParseDefaultSources(DEFAULT_IMPORT_SOURCE_FILE, importStdlibFile);
    if (UNLIKELY(ctx_->config->options->IsGenStdlib())) {
        for (const auto *import : imports) {
            if (import->ImportMetadata().HasSpecifiedDeclPath()) {
                auto resolved = import->ResolvedSource();
                ctx_->diagnosticEngine->LogDiagnostic(diagnostic::GEN_STDLIB_DECLS,
                                                      util::DiagnosticMessageParams {resolved}, import->Start());
            }
        }
    }
    varbinder_->SetDefaultImports(std::move(imports));
}

void ImportExportDecls::HandleInitModuleCallExpression(ir::CallExpression *callExpr, parser::Program *program,
                                                       GlobalClassHandler::ModuleDependencies &moduleDependencies)
{
    if (!callExpr->Callee()->IsIdentifier() ||
        callExpr->Callee()->AsIdentifier()->Name() != compiler::Signatures::INIT_MODULE_METHOD) {
        return;
    }

    auto metaData = parser_->GetImportPathManager()->GatherImportMetadata(
        program, util::ImportFlags::NONE, callExpr->Arguments().front()->AsStringLiteral());

    bool isSimultaneous = ctx_->config->options->GetCompilationMode() == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE;
    auto sources = isSimultaneous ? ctx_->parserProgram->ExternalSources() : program->DirectExternalSources();
    if (auto dependentProg = SearchExternalProgramInImport(sources, metaData); dependentProg != nullptr) {
        GlobalClassHandler::InsertModuleDependencies(&moduleDependencies, dependentProg);
    }
}

void ImportExportDecls::ProcessProgramStatements(parser::Program *program,
                                                 const ArenaVector<ir::Statement *> &statements,
                                                 GlobalClassHandler::ModuleDependencies &moduleDependencies)
{
    for (auto stmt : statements) {
        if (stmt->IsETSModule()) {
            SavedImportExportDeclsContext savedContext(this, program);
            ProcessProgramStatements(program, stmt->AsETSModule()->Statements(), moduleDependencies);
            VerifyCollectedExportName(program);
            savedContext.RecoverExportAliasMultimap();
        }
        stmt->Accept(this);
        if (stmt->IsExportNamedDeclaration()) {
            PopulateAliasMap(stmt->AsExportNamedDeclaration(), program->SourceFilePath());
        }
        if (stmt->IsTSTypeAliasDeclaration() && (stmt->IsExported() || stmt->IsDefaultExported())) {
            PopulateAliasMap(stmt->AsTSTypeAliasDeclaration(), program->SourceFilePath());
        }
        if (stmt->IsExpressionStatement() && stmt->AsExpressionStatement()->GetExpression()->IsCallExpression()) {
            HandleInitModuleCallExpression(stmt->AsExpressionStatement()->GetExpression()->AsCallExpression(), program,
                                           moduleDependencies);
        }
    }
}

GlobalClassHandler::ModuleDependencies ImportExportDecls::HandleGlobalStmts(ArenaVector<parser::Program *> &programs)
{
    VerifySingleExportDefault(programs);
    VerifyTypeExports(programs);
    GlobalClassHandler::ModuleDependencies moduleDependencies(
        ArenaVector<parser::Program *>(programs.front()->Allocator()->Adapter()),
        ArenaUnorderedSet<parser::Program *>(programs.front()->Allocator()->Adapter()));
    if (!programs.empty()) {
        std::sort(programs.begin(), programs.end(), ProgramFileNameLessThan);
    }
    for (auto const &program : programs) {
        if (!program->IsASTLowered()) {
            PreMergeNamespaces(program);
        }
        SavedImportExportDeclsContext savedContext(this, program);
        ProcessProgramStatements(program, program->Ast()->Statements(), moduleDependencies);
        VerifyCollectedExportName(program);
    }
    return moduleDependencies;
}

void ImportExportDecls::PopulateAliasMap(const ir::ExportNamedDeclaration *decl, const util::StringView &path)
{
    for (auto spec : decl->Specifiers()) {
        if (!varbinder_->AddSelectiveExportAlias(parser_, path, spec->Local()->Name(), spec->Exported()->Name(),
                                                 decl)) {
            parser_->LogError(diagnostic::CANNOT_EXPORT_DIFFERENT_OBJECTS_WITH_SAME_NAME,
                              {spec->Local()->Name().Mutf8()}, spec->Exported()->Start());
            lastExportErrorPos_ = lexer::SourcePosition();
        }
    }
}

void ImportExportDecls::AddExportFlags(ir::AstNode *node, util::StringView originalFieldName, bool exportedWithAlias)
{
    if (exportedWithAlias) {
        node->AddAstNodeFlags(ir::AstNodeFlags::HAS_EXPORT_ALIAS);
        return;
    }
    if (originalFieldName == exportDefaultName_) {
        node->AddModifier(ir::ModifierFlags::DEFAULT_EXPORT);
    } else {
        node->AddModifier(ir::ModifierFlags::EXPORT);
    }
}
void ImportExportDecls::PopulateAliasMap(const ir::TSTypeAliasDeclaration *decl, const util::StringView &path)
{
    if (!varbinder_->AddSelectiveExportAlias(parser_, path, decl->Id()->AsIdentifier()->Name(),
                                             decl->Id()->AsIdentifier()->Name(), decl)) {
        parser_->LogError(diagnostic::CANNOT_EXPORT_DIFFERENT_OBJECTS_WITH_SAME_NAME,
                          {decl->Id()->AsIdentifier()->Name().Mutf8()}, lastExportErrorPos_);
        lastExportErrorPos_ = lexer::SourcePosition();
    }
}

void ImportExportDecls::HandleSelectiveExportWithAlias(util::StringView originalFieldName, util::StringView exportName,
                                                       lexer::SourcePosition startLoc)
{
    bool exportedWithAlias = exportName != originalFieldName;

    auto fieldItem = fieldMap_.find(originalFieldName);
    ir::VariableDeclarator *variableDeclarator = nullptr;
    if (fieldItem != fieldMap_.end()) {
        ir::AstNode *field = fieldItem->second;
        if (field->IsVariableDeclaration()) {
            variableDeclarator = field->AsVariableDeclaration()->GetDeclaratorByName(originalFieldName);
            ES2PANDA_ASSERT(variableDeclarator != nullptr);
        }

        if (variableDeclarator != nullptr) {
            AddExportFlags(variableDeclarator, originalFieldName, exportedWithAlias);
        } else {
            AddExportFlags(field, originalFieldName, exportedWithAlias);
        }
    }

    if (exportedWithAlias) {
        if (auto declItem = fieldMap_.find(exportName); declItem != fieldMap_.end()) {
            // Checking for the alias might be unnecessary, because explicit exports cannot
            // have an alias yet.
            bool alreadyExported = ((declItem->second->Modifiers() & ir::ModifierFlags::EXPORTED) != 0) &&
                                   !declItem->second->HasExportAlias();
            if (!alreadyExported && declItem->second->IsVariableDeclaration()) {
                auto declarator = declItem->second->AsVariableDeclaration()->GetDeclaratorByName(exportName);
                ES2PANDA_ASSERT(declarator != nullptr);
                alreadyExported |=
                    ((declarator->Modifiers() & ir::ModifierFlags::EXPORTED) != 0) && !declarator->HasExportAlias();
            }
            if (alreadyExported) {
                parser_->LogError(diagnostic::DUPLICATE_EXPORT_NAME, {exportName.Mutf8()}, startLoc);
            }
        }
    }
}

void ImportExportDecls::VisitFunctionDeclaration(ir::FunctionDeclaration *funcDecl)
{
    fieldMap_.emplace(funcDecl->Function()->Id()->Name(), funcDecl->Function());
}

void ImportExportDecls::VisitOverloadDeclaration(ir::OverloadDeclaration *overloadDeclaration)
{
    fieldMap_.emplace(overloadDeclaration->Id()->Name(), overloadDeclaration);
}

void ImportExportDecls::VisitVariableDeclaration(ir::VariableDeclaration *varDecl)
{
    for (const auto &decl : varDecl->Declarators()) {
        fieldMap_.emplace(decl->Id()->AsIdentifier()->Name(), varDecl);
    }
}

void ImportExportDecls::VisitClassDeclaration(ir::ClassDeclaration *classDecl)
{
    fieldMap_.emplace(classDecl->Definition()->Ident()->Name(), classDecl);
}

void ImportExportDecls::VisitTSEnumDeclaration(ir::TSEnumDeclaration *enumDecl)
{
    fieldMap_.emplace(enumDecl->Key()->Name(), enumDecl);
}

void ImportExportDecls::VisitTSTypeAliasDeclaration(ir::TSTypeAliasDeclaration *typeAliasDecl)
{
    fieldMap_.emplace(typeAliasDecl->Id()->Name(), typeAliasDecl);
}

void ImportExportDecls::VisitTSInterfaceDeclaration(ir::TSInterfaceDeclaration *interfaceDecl)
{
    fieldMap_.emplace(interfaceDecl->Id()->Name(), interfaceDecl);
}

void ImportExportDecls::VisitAnnotationDeclaration(ir::AnnotationDeclaration *annotationDecl)
{
    fieldMap_.emplace(annotationDecl->GetBaseName()->Name(), annotationDecl);
}

void ImportExportDecls::VisitETSModule(ir::ETSModule *etsModule)
{
    if (etsModule->IsETSScript()) {
        return;
    }
    fieldMap_.emplace(etsModule->Ident()->Name(), etsModule);
}

void ImportExportDecls::VisitExportNamedDeclaration(ir::ExportNamedDeclaration *exportDecl)
{
    for (auto spec : exportDecl->Specifiers()) {
        auto local = spec->Local();
        // If this was enterred more than once, CTE must has been logged in parser.
        if ((exportDecl->Modifiers() & ir::ModifierFlags::DEFAULT_EXPORT) != 0) {
            if (exportDefaultName_ != nullptr) {
                parser_->LogError(diagnostic::EXPORT_DEFAULT_WITH_MUPLTIPLE_SPECIFIER, {}, local->Start());
                continue;
            }
            exportDefaultName_ = local->Name();
        }
        exportNameMap_.emplace(local->Name(), local->Start());
    }
}

void ImportExportDecls::VisitETSImportDeclaration(ir::ETSImportDeclaration *importDecl)
{
    for (ir::AstNode *spec : importDecl->AsETSImportDeclaration()->Specifiers()) {
        if (spec->IsImportSpecifier()) {
            importedSpecifiersForExportCheck_.emplace(spec->AsImportSpecifier()->Local()->Name(),
                                                      spec->AsImportSpecifier()->Imported()->Name());
        } else if (spec->IsImportDefaultSpecifier()) {
            importedSpecifiersForExportCheck_.emplace(spec->AsImportDefaultSpecifier()->Local()->Name(),
                                                      spec->AsImportDefaultSpecifier()->Local()->Name());
        } else if (spec->IsImportNamespaceSpecifier()) {
            importedSpecifiersForExportCheck_.emplace(spec->AsImportNamespaceSpecifier()->Local()->Name(),
                                                      spec->AsImportNamespaceSpecifier()->Local()->Name());
        } else {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void ImportExportDecls::HandleSimpleType(std::set<util::StringView> &exportedStatements, ir::Statement *stmt,
                                         util::StringView name)
{
    if (stmt->IsExported()) {
        exportedStatements.insert(name);
    }
}

void ImportExportDecls::VerifyTypeExports(const ArenaVector<parser::Program *> &programs)
{
    std::set<util::StringView> exportedStatements;
    std::map<util::StringView, ir::AstNode *> typesMap;

    for (const auto &program : programs) {
        for (auto stmt : program->Ast()->Statements()) {
            VerifyType(stmt, exportedStatements, typesMap);
        }
    }
}

void ImportExportDecls::VerifyType(ir::Statement *stmt, std::set<util::StringView> &exportedStatements,
                                   std::map<util::StringView, ir::AstNode *> &typesMap)
{
    if (stmt->IsClassDeclaration()) {
        if (!stmt->IsDeclare() && stmt->AsClassDeclaration()->Definition()->Language().IsDynamic()) {
            parser_->LogError(diagnostic::EXPORT_WITHOUT_DECLARE_IN_DECL_MODULE, {}, stmt->Start());
        }
        typesMap.insert({stmt->AsClassDeclaration()->Definition()->Ident()->Name(), stmt});
        return HandleSimpleType(exportedStatements, stmt, stmt->AsClassDeclaration()->Definition()->Ident()->Name());
    }

    if (stmt->IsTSInterfaceDeclaration()) {
        if (!stmt->IsDeclare() && stmt->AsTSInterfaceDeclaration()->Language().IsDynamic()) {
            parser_->LogError(diagnostic::EXPORT_WITHOUT_DECLARE_IN_DECL_MODULE, {}, stmt->Start());
        }
        typesMap.insert({stmt->AsTSInterfaceDeclaration()->Id()->Name(), stmt});
        return HandleSimpleType(exportedStatements, stmt, stmt->AsTSInterfaceDeclaration()->Id()->Name());
    }

    if (stmt->IsTSTypeAliasDeclaration()) {
        typesMap.insert({stmt->AsTSTypeAliasDeclaration()->Id()->Name(), stmt});
        return HandleSimpleType(exportedStatements, stmt, stmt->AsTSTypeAliasDeclaration()->Id()->Name());
    }
}

void ImportExportDecls::VerifySingleExportDefault(const ArenaVector<parser::Program *> &programs)
{
    bool metDefaultExport = false;
    auto &logger = parser_->DiagnosticEngine();
    auto verifyDefault = [&metDefaultExport, &logger](ir::Statement *stmt) {
        if ((stmt->Modifiers() & ir::ModifierFlags::DEFAULT_EXPORT) == 0) {
            return;
        }
        if (metDefaultExport) {
            logger.LogDiagnostic(diagnostic::MULTIPLE_DEFAULT_EXPORTS, util::DiagnosticMessageParams {}, stmt->Start());
        }
        metDefaultExport = true;
    };
    for (const auto &program : programs) {
        for (auto stmt : program->Ast()->Statements()) {
            verifyDefault(stmt);
        }
        metDefaultExport = false;
    }
}

void ImportExportDecls::VerifyCollectedExportName(const parser::Program *program)
{
    for (auto const &[exportName, startLoc] : exportNameMap_) {
        const bool isType = exportedTypes_.find(exportName) != exportedTypes_.end();
        util::StringView middleName = varbinder_->FindNameInAliasMap(program->SourceFilePath(), exportName);
        ES2PANDA_ASSERT(!middleName.Empty());
        auto originNameIt = importedSpecifiersForExportCheck_.find(middleName);
        auto originalName = originNameIt != importedSpecifiersForExportCheck_.end() ? originNameIt->second : middleName;
        auto result = fieldMap_.find(originalName);
        if (result == fieldMap_.end() && !isType && originNameIt == importedSpecifiersForExportCheck_.end()) {
            parser_->LogError(diagnostic::CAN_NOT_FIND_NAME_TO_EXPORT, {originalName}, startLoc);
        }
        if (result != fieldMap_.end() && result->second->IsAnnotationDeclaration() && exportName != originalName) {
            parser_->LogError(diagnostic::CAN_NOT_RENAME_ANNOTATION, {originalName}, startLoc);
        }
        if (!isType) {
            HandleSelectiveExportWithAlias(originalName, exportName, startLoc);
        }
    }
}

void ImportExportDecls::PreMergeNamespaces(parser::Program *program)
{
    bool isChanged = false;
    auto mergeNameSpace = [&program, &isChanged](ir::AstNode *ast) {
        if (!ast->IsETSModule()) {
            return;
        }

        ArenaVector<ir::ETSModule *> namespaces(program->Allocator()->Adapter());
        auto &body = ast->AsETSModule()->StatementsForUpdates();
        auto originalSize = body.size();
        auto end = std::remove_if(body.begin(), body.end(), [&namespaces](ir::AstNode *node) {
            if (node->IsETSModule() && node->AsETSModule()->IsNamespace()) {
                namespaces.emplace_back(node->AsETSModule());
                return true;
            }
            return false;
        });
        body.erase(end, body.end());

        GlobalClassHandler::MergeNamespace(namespaces, program);

        for (auto ns : namespaces) {
            body.emplace_back(ns);
        }

        isChanged |= (originalSize != body.size());
    };

    do {
        isChanged = false;
        mergeNameSpace(program->Ast());
        program->Ast()->IterateRecursivelyPreorder(mergeNameSpace);
    } while (isChanged);
}
}  // namespace ark::es2panda::compiler
