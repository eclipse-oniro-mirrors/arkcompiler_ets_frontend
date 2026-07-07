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

#include "compiler/lowering/ets/topLevelStmts/importExportDecls.h"
#include "generated/diagnostic.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

static std::string GetArkruntimeImports()
{
    static std::array importedNames = {std::string_view("stub"), std::string_view("annotation")};

    std::string importString = "import {";
    for (const auto &name : importedNames) {
        importString +=
            std::string(name) + " as " + std::string(ARKRUNTIME_IMPORT_ALIAS_PREFIX) + std::string(name) + ", ";
    }
    importString += "} from \'arkruntime\';";
    return importString;
}

void ImportExportDecls::IntroduceStdlibImportProgram()
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
        if (path == "arkruntime") {
            /*
             * NOTE(knazarov): workaround to be able to work with arkruntime entities
             * in lowerings. Needs to be reworked, since it may lead to user name collisions
             * or exploitations by users. Currently, we are unable to:
             * 1. import * as Ident from 'arkruntime', since `AddImportNamespaceSpecifiersToTopBindings`
             *    does not account correctly for this case
             * 2. import * as %%gensym%%, since parser will reject such symbols
             * When both these blockers are resolved, this line should be reworked to the
             * importStdlibFile += "import * as %%arkruntime%% from \'arkruntime\';";
             */
            importStdlibFile += GetArkruntimeImports();
            continue;
        }
        importStdlibFile += "import * from \"" + path + "\";";
    }
    auto stdlibImportProgram = parser_->IntroduceStdlibImportProgram(std::move(importStdlibFile));
    ArenaVector<ir::ETSImportDeclaration *> imports(varbinder_->Allocator()->Adapter());
    const auto *importManager = parser_->GetImportPathManager();
    for (auto *statement : stdlibImportProgram->Ast()->Statements()) {
        if (statement->IsETSImportDeclaration()) {
            imports.push_back(statement->AsETSImportDeclaration());
        }
    }

    if (UNLIKELY(ctx_->config->options->IsGenStdlib())) {
        for (const auto *import : imports) {
            auto prog = importManager->SearchResolved(import->ImportInfo());
            if (prog->GetModuleKind() == util::ModuleKind::ETSCACHE_DECL) {
                auto resolved = import->ResolvedSource();
                ctx_->diagnosticEngine->LogDiagnostic(diagnostic::GEN_STDLIB_DECLS,
                                                      util::DiagnosticMessageParams {resolved}, import->Start());
            }
        }
    }
    varbinder_->SetDefaultImports(std::move(imports));
}

void ImportExportDecls::ProcessProgramStatements(parser::Program *program,
                                                 const ArenaVector<ir::Statement *> &statements)
{
    for (auto stmt : statements) {
        if (stmt->IsETSModule()) {
            SavedImportExportDeclsContext savedContext(this);
            const bool isNamespace = stmt->AsETSModule()->IsNamespace();
            if (isNamespace) {
                namespaceDepth_++;
            }
            ProcessProgramStatements(program, stmt->AsETSModule()->Statements());
            // Namespace bodies use a saved local field map to mark namespace declarations immediately.
            // Program-wide pending export aliases must be verified only outside that temporary context.
            if (!isNamespace) {
                VerifyCollectedExportName(program);
            }
            if (isNamespace) {
                namespaceDepth_--;
            }
        }
        stmt->Accept(this);
        if (stmt->IsExportNamedDeclaration()) {
            PopulateAliasMap(program, stmt->AsExportNamedDeclaration());
        }
    }
}

static bool ProgramFileNameLessThan(const parser::Program *a, const parser::Program *b)
{
    return a->FileName().Mutf8() < b->FileName().Mutf8();
}

void ImportExportDecls::HandleGlobalStmts(parser::Program *program)
{
    VerifySingleExportDefault(program);
    VerifyTypeExports(program);

    if (program->Is<util::ModuleKind::PACKAGE>()) {
        auto fractions = program->As<util::ModuleKind::PACKAGE>()->GetUnmergedPackagePrograms();
        std::sort(fractions.begin(), fractions.end(), ProgramFileNameLessThan);
    }

    program->MaybeIteratePackage([this](parser::Program *prog) {
        if (!prog->IsASTLowered()) {
            PreMergeNamespaces(prog);
        }
        SavedImportExportDeclsContext savedContext(this);
        ProcessProgramStatements(prog, prog->Ast()->Statements());
        VerifyCollectedExportName(prog);
    });
}

void ImportExportDecls::PopulateAliasMap(parser::Program *program, const ir::ExportNamedDeclaration *decl)
{
    const bool isDefault = (decl->Modifiers() & ir::ModifierFlags::DEFAULT_EXPORT) != 0;
    if (namespaceDepth_ != 0) {
        for (auto spec : decl->Specifiers()) {
            auto originalName = spec->Exported()->Name();
            auto exportName = isDefault ? util::StringView {"default"} : spec->Local()->Name();
            if (fieldMap_.find(originalName) == fieldMap_.end()) {
                parser_->LogError(diagnostic::CAN_NOT_FIND_NAME_TO_EXPORT, {originalName}, spec->Exported()->Start());
                continue;
            }
            HandleSelectiveExportWithAlias(originalName, exportName, spec->Exported()->Start());
        }
        return;
    }

    const bool isExplicitTypeOnly = (decl->Modifiers() & ir::ModifierFlags::EXPORT_TYPE) != 0U;
    for (auto spec : decl->Specifiers()) {
        const ir::AstNode *origin = spec->Local();
        if (auto field = fieldMap_.find(spec->Exported()->Name()); field != fieldMap_.end()) {
            origin = field->second;
        }
        const auto exportedName = isDefault ? util::StringView {"default"} : spec->Local()->Name();
        const varbinder::SelectiveExportAlias alias {program, exportedName,  spec->Exported(),  origin,
                                                     decl,    spec->Local(), isExplicitTypeOnly};
        if (!varbinder_->AddSelectiveExportAlias(alias)) {
            parser_->LogError(diagnostic::CANNOT_EXPORT_DIFFERENT_OBJECTS_WITH_SAME_NAME, {exportedName.Mutf8()},
                              spec->Local()->Start());
            lastExportErrorPos_ = lexer::SourcePosition();
        }
    }
}

void ImportExportDecls::CheckDuplicateExportName(util::StringView exportName, util::StringView localName,
                                                 const ir::AstNode *reportNode)
{
    if (exportName.Empty() || localName.Empty() || reportNode == nullptr) {
        return;
    }

    auto [iter, inserted] = exportNameMap_.emplace(exportName, localName);
    if (!inserted && iter->second == localName && lastExportErrorPos_ != reportNode->Start()) {
        ctx_->diagnosticEngine->LogDiagnostic(diagnostic::DUPLICATE_EXPORT_ALIASES,
                                              util::DiagnosticMessageParams {exportName}, reportNode->Start());
        lastExportErrorPos_ = reportNode->Start();
    }
}

void ImportExportDecls::CheckDuplicateReExportName(util::StringView exportName, util::StringView importedName,
                                                   util::StringView sourceName, const ir::AstNode *reportNode)
{
    if (exportName.Empty() || importedName.Empty() || sourceName.Empty() || reportNode == nullptr) {
        return;
    }

    auto [iter, inserted] = reExportNameMap_.emplace(exportName, std::make_pair(importedName, sourceName));
    if (!inserted && iter->second.first == importedName && iter->second.second == sourceName &&
        lastExportErrorPos_ != reportNode->Start()) {
        ctx_->diagnosticEngine->LogDiagnostic(diagnostic::DUPLICATE_EXPORT_ALIASES,
                                              util::DiagnosticMessageParams {exportName}, reportNode->Start());
        lastExportErrorPos_ = reportNode->Start();
    }
}

void ImportExportDecls::AddExportFlags(ir::AstNode *node, bool hasAliasName)
{
    auto flags = hasAliasName ? ir::ModifierFlags::EXPORT_WITH_ALIAS : ir::ModifierFlags::EXPORT;
    node->AddModifier(flags);
    if (node->IsScriptFunction() && node->Parent() != nullptr && node->Parent()->IsFunctionDeclaration()) {
        node->Parent()->AddModifier(flags);
    }
}

void ImportExportDecls::AddTypeOnlyExportFlags(util::StringView originalFieldName)
{
    auto fieldItem = fieldMap_.find(originalFieldName);
    if (fieldItem == fieldMap_.end()) {
        return;
    }

    auto *field = fieldItem->second;
    if (field->IsVariableDeclaration()) {
        auto *variableDeclarator = field->AsVariableDeclaration()->GetDeclaratorByName(originalFieldName);
        ES2PANDA_ASSERT(variableDeclarator != nullptr);
        AddExportFlags(variableDeclarator, true);
        return;
    }
    AddExportFlags(field, true);
}

static bool IsTypeOnlyExportTarget(const ir::AstNode *node)
{
    if (node == nullptr) {
        return false;
    }
    if (node->IsClassDeclaration()) {
        return !node->AsClassDeclaration()->Definition()->IsNamespaceTransformed();
    }
    if (node->IsClassDefinition()) {
        return !node->AsClassDefinition()->IsNamespaceTransformed();
    }
    return node->IsTSInterfaceDeclaration() || node->IsTSEnumDeclaration() || node->IsTSTypeAliasDeclaration();
}

static bool ValidateTypeOnlyExportTarget(parser::ETSParser *parser, const ir::AstNode *target, util::StringView name,
                                         const lexer::SourcePosition &pos)
{
    if (IsTypeOnlyExportTarget(target)) {
        return true;
    }
    if (target != nullptr && target->IsAnnotationDeclaration()) {
        parser->LogError(diagnostic::ANNOTATION_AS_TYPE, {}, pos);
    } else {
        parser->LogError(diagnostic::TYPE_NOT_FOUND, {name}, pos);
    }
    return false;
}

void ImportExportDecls::PopulateAliasMap(parser::Program *program, const ir::TSTypeAliasDeclaration *decl)
{
    const varbinder::SelectiveExportAlias alias {
        program, decl->Id()->AsIdentifier()->Name(), decl->Id()->AsIdentifier(), decl, nullptr, decl->Id(), false};
    if (!varbinder_->AddSelectiveExportAlias(alias)) {
        parser_->LogError(diagnostic::CANNOT_EXPORT_DIFFERENT_OBJECTS_WITH_SAME_NAME,
                          {decl->Id()->AsIdentifier()->Name().Mutf8()}, lastExportErrorPos_);
        lastExportErrorPos_ = lexer::SourcePosition();
    }
}

bool ImportExportDecls::HandleSelectiveExportWithAlias(util::StringView originalFieldName, util::StringView exportName,
                                                       lexer::SourcePosition startLoc)
{
    bool hasAliasName = exportName != originalFieldName;

    if (hasAliasName) {
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
                return false;
            }
        }
    }

    auto fieldItem = fieldMap_.find(originalFieldName);
    ir::VariableDeclarator *variableDeclarator = nullptr;
    if (fieldItem != fieldMap_.end()) {
        ir::AstNode *field = fieldItem->second;
        if (field->IsVariableDeclaration()) {
            variableDeclarator = field->AsVariableDeclaration()->GetDeclaratorByName(originalFieldName);
            ES2PANDA_ASSERT(variableDeclarator != nullptr);
        }

        if (variableDeclarator != nullptr) {
            AddExportFlags(variableDeclarator, hasAliasName);
        } else {
            AddExportFlags(field, hasAliasName);
        }
    }

    return true;
}

void ImportExportDecls::VisitFunctionDeclaration(ir::FunctionDeclaration *funcDecl)
{
    fieldMap_.emplace(funcDecl->Function()->Id()->Name(), funcDecl->Function());
}

void ImportExportDecls::ProcessDestructuringElements(ir::ETSDestructuring *destructuring,
                                                     ir::VariableDeclaration *varDecl)
{
    for (auto *elem : destructuring->Elements()) {
        if (elem->IsOmittedExpression()) {
            continue;
        }
        if (elem->IsRestElement() || elem->IsAssignmentPattern() || elem->IsArrayPattern()) {
            continue;
        }
        ES2PANDA_ASSERT(elem->IsIdentifier());
        fieldMap_.emplace(elem->AsIdentifier()->Name(), varDecl);
    }
}

void ImportExportDecls::VisitOverloadDeclaration(ir::OverloadDeclaration *overloadDeclaration)
{
    fieldMap_.emplace(overloadDeclaration->Id()->Name(), overloadDeclaration);
}

void ImportExportDecls::VisitVariableDeclaration(ir::VariableDeclaration *varDecl)
{
    for (const auto &decl : varDecl->Declarators()) {
        if (decl->Id()->IsETSDestructuring()) {
            ProcessDestructuringElements(decl->Id()->AsETSDestructuring(), varDecl);
            continue;
        }

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
    if (exportDecl->Specifiers().empty()) {
        parser_->LogError(diagnostic::EMPTY_EXPORT_SPECIFIER_LIST, {}, exportDecl->Start());
        return;
    }

    // Lambda function to avoid extra nested level.
    auto const logError = [this, exportDecl](ir::Identifier const *const local) -> void {
        if (!exportDefaultName_.Is(local->Name().Utf8())) {
            parser_->LogError(diagnostic::EXPORT_DEFAULT_WITH_MUPLTIPLE_SPECIFIER, {}, exportDecl->Start());
        }
    };

    bool const isDefault = (exportDecl->Modifiers() & ir::ModifierFlags::DEFAULT_EXPORT) !=
                           static_cast<std::underlying_type_t<ir::ModifierFlags>>(0U);

    for (auto spec : exportDecl->Specifiers()) {
        auto const *const local = spec->Local();
        // If this was enterred more than once, CTE must has been logged in parser.
        if (isDefault) {
            if (exportDefaultName_ != nullptr) {
                logError(local);
                continue;
            }
            exportDefaultName_ = local->Name();
        }
        auto const *const exported = spec->Exported();
        auto exportName = isDefault ? util::StringView {"default"} : local->Name();
        CheckDuplicateExportName(exportName, exported->Name(), exportDecl);
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
        } else if (spec->IsExpression() && spec->AsExpression()->IsBrokenExpression()) {
            continue;
        } else {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void ImportExportDecls::VisitETSReExportDeclaration(ir::ETSReExportDeclaration *reExportDecl)
{
    auto *importDecl = reExportDecl->GetETSImportDeclarations();
    const auto sourceName = importDecl->Source()->Str();
    for (auto *spec : importDecl->Specifiers()) {
        if (spec->IsImportSpecifier()) {
            const auto *specifier = spec->AsImportSpecifier();
            CheckDuplicateReExportName(specifier->Local()->Name(), specifier->Imported()->Name(), sourceName,
                                       reExportDecl);
            continue;
        }
        if (spec->IsImportNamespaceSpecifier() && spec->AsImportNamespaceSpecifier()->Local() != nullptr) {
            CheckDuplicateReExportName(spec->AsImportNamespaceSpecifier()->Local()->Name(),
                                       spec->AsImportNamespaceSpecifier()->Local()->Name(), sourceName, reExportDecl);
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

void ImportExportDecls::VerifyTypeExports(parser::Program *program)
{
    std::set<util::StringView> exportedStatements;
    std::map<util::StringView, ir::AstNode *> typesMap;

    program->MaybeIteratePackage([this, &exportedStatements, &typesMap](parser::Program *prog) {
        for (auto stmt : prog->Ast()->Statements()) {
            VerifyType(stmt, exportedStatements, typesMap);
        }
    });
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

void ImportExportDecls::VerifySingleExportDefault(parser::Program *program)
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

    program->MaybeIteratePackage([&verifyDefault, &metDefaultExport](parser::Program *prog) {
        for (auto stmt : prog->Ast()->Statements()) {
            verifyDefault(stmt);
        }
        metDefaultExport = false;
    });
}

void ImportExportDecls::CollectNamespaceDeclarations(const parser::Program *program)
{
    for (auto *stmt : program->Ast()->Statements()) {
        if (stmt->IsETSModule() && stmt->AsETSModule()->IsNamespace()) {
            fieldMap_.emplace(stmt->AsETSModule()->Ident()->Name(), stmt);
            continue;
        }

        if (!stmt->IsClassDeclaration()) {
            continue;
        }

        auto *classDecl = stmt->AsClassDeclaration();
        if (classDecl->Definition()->IsNamespaceTransformed()) {
            fieldMap_.emplace(classDecl->Definition()->Ident()->Name(), stmt);
        }
    }
}

static bool IsUnresolvedValueExport(const varbinder::PendingLocalExportAlias &alias, bool isType,
                                    bool hasLocalDeclaration, bool hasImportedSpecifier)
{
    return !hasLocalDeclaration && !alias.originDeclaresName && !isType && !hasImportedSpecifier;
}

void ImportExportDecls::ReportUnresolvedValueExport(const varbinder::PendingLocalExportAlias &alias,
                                                    util::StringView originalName, lexer::SourcePosition startLoc,
                                                    std::set<util::StringView> &unresolvedAliases,
                                                    std::set<util::StringView> &warnedUnresolvedAliases)
{
    if (!alias.localName.Empty() && !unresolvedAliases.insert(alias.localName).second &&
        warnedUnresolvedAliases.insert(alias.localName).second) {
        ctx_->diagnosticEngine->LogDiagnostic(diagnostic::DUPLICATE_EXPORT_ALIASES,
                                              util::DiagnosticMessageParams {alias.localName}, startLoc);
    }
    parser_->LogError(diagnostic::CAN_NOT_FIND_NAME_TO_EXPORT, {originalName}, startLoc);
}

bool ImportExportDecls::VerifyTypeOnlyExportAlias(const parser::Program *program,
                                                  const varbinder::PendingLocalExportAlias &alias,
                                                  util::StringView exportName, util::StringView originalName,
                                                  ir::AstNode *localDecl, bool hasImportedSpecifier,
                                                  lexer::SourcePosition startLoc, const ir::AstNode *reportOrigin)
{
    const bool isValidTypeOnlyExport = localDecl != nullptr
                                           ? ValidateTypeOnlyExportTarget(parser_, localDecl, originalName, startLoc)
                                           : hasImportedSpecifier;
    if (isValidTypeOnlyExport) {
        AddTypeOnlyExportFlags(originalName);
        return true;
    }

    if (localDecl == nullptr && !hasImportedSpecifier) {
        parser_->LogError(diagnostic::TYPE_NOT_FOUND, {originalName}, startLoc);
    }
    varbinder_->GetExportFactsStore().MarkPendingLocalExportAliasInvalid(const_cast<parser::Program *>(program),
                                                                         exportName, alias.localName, reportOrigin);
    AddTypeOnlyExportFlags(originalName);
    return false;
}

void ImportExportDecls::VerifyCollectedExportAlias(const parser::Program *program,
                                                   const varbinder::PendingLocalExportAlias &alias,
                                                   std::set<util::StringView> &unresolvedAliases,
                                                   std::set<util::StringView> &warnedUnresolvedAliases)
{
    const auto exportName = alias.exportedName;
    const auto *reportOrigin = alias.reportOrigin != nullptr ? alias.reportOrigin : alias.origin;
    const auto startLoc = reportOrigin != nullptr ? reportOrigin->Start() : lexer::SourcePosition {};
    const bool isType = exportedTypes_.find(exportName) != exportedTypes_.end();
    auto originNameIt = importedSpecifiersForExportCheck_.find(alias.localName);
    auto originalName =
        originNameIt != importedSpecifiersForExportCheck_.end() ? originNameIt->second : alias.localName;
    auto result = fieldMap_.find(originalName);
    const bool hasLocalDeclaration = result != fieldMap_.end();
    const bool hasImportedSpecifier = originNameIt != importedSpecifiersForExportCheck_.end();
    auto *localDecl = hasLocalDeclaration ? result->second : nullptr;

    if (exportName.Is("default") && originalName.Is("default") && !hasLocalDeclaration) {
        return;
    }
    if (IsUnresolvedValueExport(alias, isType, hasLocalDeclaration, hasImportedSpecifier)) {
        ReportUnresolvedValueExport(alias, originalName, startLoc, unresolvedAliases, warnedUnresolvedAliases);
    }
    if (localDecl != nullptr && localDecl->IsAnnotationDeclaration() && exportName != originalName) {
        parser_->LogError(diagnostic::CAN_NOT_RENAME_ANNOTATION, {originalName}, startLoc);
    }
    if (alias.isExplicitTypeOnly) {
        VerifyTypeOnlyExportAlias(program, alias, exportName, originalName, localDecl, hasImportedSpecifier, startLoc,
                                  reportOrigin);
        return;
    }
    if (!isType && !HandleSelectiveExportWithAlias(originalName, exportName, startLoc)) {
        varbinder_->GetExportFactsStore().MarkPendingLocalExportAliasInvalid(const_cast<parser::Program *>(program),
                                                                             exportName, alias.localName, reportOrigin);
    }
}

void ImportExportDecls::VerifyCollectedExportName(const parser::Program *program)
{
    CollectNamespaceDeclarations(program);
    std::set<util::StringView> unresolvedAliases;
    std::set<util::StringView> warnedUnresolvedAliases;
    const auto &exportFacts = varbinder_->GetExportFactsStore();
    for (const auto &alias : exportFacts.PendingLocalExportAliases(const_cast<parser::Program *>(program))) {
        VerifyCollectedExportAlias(program, alias, unresolvedAliases, warnedUnresolvedAliases);
    }
}

void ImportExportDecls::PreMergeNamespaces(parser::Program *program)
{
    bool hasChange = true;

    std::function<void(ir::AstNode *)> merge = [ctx = ctx_, &program, &hasChange, &merge](ir::AstNode *ast) {
        if (ast->IsClassDeclaration() && ast->AsClassDeclaration()->Definition()->IsNamespaceTransformed()) {
            ast->Iterate(merge);
            return;
        }
        if (!ast->IsETSModule()) {
            return;
        }

        ArenaVector<ir::ETSModule *> namespaces(program->Allocator()->Adapter());
        auto &body = ast->AsETSModule()->StatementsForUpdates();
        auto originalSize = body.size();

        EjectNamespacesFromStatementsVector(&body, &namespaces);
        GlobalClassHandler::MergeNamespace(namespaces, ctx);

        for (auto ns : namespaces) {
            body.emplace_back(ns);
        }
        hasChange |= (originalSize != body.size());

        ast->Iterate(merge);
    };

    while (hasChange) {
        hasChange = false;
        merge(program->Ast());
    }
}
}  // namespace ark::es2panda::compiler
