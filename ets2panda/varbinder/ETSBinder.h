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

#ifndef ES2PANDA_VARBINDER_ETSBINDER_H
#define ES2PANDA_VARBINDER_ETSBINDER_H

#include "varbinder/TypedBinder.h"
#include "varbinder/exportFacts.h"
#include "varbinder/recordTable.h"
#include "ir/ets/etsImportDeclaration.h"
#include "ir/ets/etsReExportDeclaration.h"
#include "ir/expressions/identifier.h"
#include "ir/module/importSpecifier.h"
#include "ir/statements/annotationDeclaration.h"
#include "parser/ETSparser.h"

namespace ark::es2panda::ir {
class ETSImportDeclaration;
class AstNode;
class Identifier;
class ObjectExpression;
class ETSTypeReference;
class MethodDefinition;
class AnnotationUsage;
class StringLiteral;
class ETSReExportDeclaration;
class TSEnumDeclaration;
class TSQualifiedName;
class ClassElement;
class ImportSpecifier;
class ETSNewClassInstanceExpression;

}  // namespace ark::es2panda::ir

namespace ark::es2panda::varbinder {
struct DynamicImportData {
    const ir::ETSImportDeclaration *import;
    const ir::AstNode *specifier;
    Variable *variable;
};

using DynamicImportVariables = ArenaUnorderedMap<const Variable *, DynamicImportData>;

struct SelectiveExportAlias {
    parser::Program *program;
    util::StringView exportedName;
    const ir::Identifier *localIdent;
    const ir::AstNode *decl;
    const ir::AstNode *exportDecl;
    const ir::AstNode *reportOrigin;
    bool isTypeOnly;
};

class ETSBinder : public TypedBinder {
public:
    explicit ETSBinder(public_lib::Context *context)
        : TypedBinder(context),
          globalRecordTable_(Allocator()->New<RecordTable>(Allocator(), nullptr, RecordTableFlags::NONE)),
          recordTable_(globalRecordTable_),
          externalRecordTable_(Allocator()->Adapter()),
          defaultImports_(Allocator()->Adapter()),
          dynamicImports_(Allocator()->Adapter()),
          reExportImports_(Allocator()->Adapter()),
          exportFactStore_(Allocator()->New<ExportFactStore>(Allocator()))
    {
        InitImplicitThisParam();
    }

    ETSBinder() = delete;
    NO_COPY_SEMANTIC(ETSBinder);
    NO_MOVE_SEMANTIC(ETSBinder);
    ~ETSBinder() override = default;

    [[nodiscard]] ScriptExtension Extension() const noexcept override
    {
        return ScriptExtension::ETS;
    }

    [[nodiscard]] ResolveBindingOptions BindingOptions() const noexcept override
    {
        return ResolveBindingOptions::BINDINGS;
    }

    [[nodiscard]] RecordTable *GetRecordTable() noexcept
    {
        return recordTable_;
    }

    [[nodiscard]] const RecordTable *GetRecordTable() const noexcept
    {
        return recordTable_;
    }

    void SetRecordTable(RecordTable *table) noexcept
    {
        recordTable_ = table;
    }

    bool CheckRecordTablesConsistency(parser::Program *program = nullptr) const;

    void SetGlobalRecordTable(RecordTable *tbl) noexcept
    {
        globalRecordTable_ = tbl;
    }

    [[nodiscard]] RecordTable *GetGlobalRecordTable() noexcept
    {
        return globalRecordTable_;
    }

    [[nodiscard]] const RecordTable *GetGlobalRecordTable() const noexcept
    {
        return globalRecordTable_;
    }

    [[nodiscard]] ArenaMap<parser::Program *, RecordTable *> &GetExternalRecordTable() noexcept
    {
        return externalRecordTable_;
    }

    [[nodiscard]] const ArenaMap<parser::Program *, RecordTable *> &GetExternalRecordTable() const noexcept
    {
        return externalRecordTable_;
    }

    void CleanScopesAndRecordTables(parser::Program *program)
    {
        this->CleanScopes();
        externalRecordTable_.clear();
        globalRecordTable_->CleanUp();
        reExportImports_.erase(program);
    }

    void HandleCustomNodes(ir::AstNode *childNode) override;

    void IdentifierAnalysis() override;
    void BuildClassDefinition(ir::ClassDefinition *classDef) override;
    void BuildObjectExpression(ir::ObjectExpression *obj);
    void BuildETSTypeReference(ir::ETSTypeReference *typeRef);
    void BuildClassProperty(const ir::ClassProperty *prop) override;
    void LookupIdentReference(ir::Identifier *ident) override;
    [[nodiscard]] bool BuildInternalName(ir::ScriptFunction *scriptFunc) override;
    void AddCompilableFunction(ir::ScriptFunction *func) override;

    static bool IsSpecialName(const util::StringView &name);
    [[nodiscard]] bool LookupInDebugInfoPlugin(ir::Identifier *ident);
    void LookupTypeReference(ir::Identifier *ident);
    void LookupTypeArgumentReferences(ir::ETSTypeReference *typeRef);
    void BuildInterfaceDeclaration(ir::TSInterfaceDeclaration *decl);
    void BuildMemberExpression(ir::MemberExpression *memberExpr);
    void BuildMethodDefinition(ir::MethodDefinition *methodDef);
    void BuildOverloadDeclaration(ir::OverloadDeclaration *overloadDef);
    void BuildAnnotationDeclaration(ir::AnnotationDeclaration *annoDecl);
    void BuildAnnotationUsage(ir::AnnotationUsage *annoUsage);
    void BuildImportDeclaration(ir::ETSImportDeclaration *decl);
    void ValidateReexports();
    void BuildETSNewClassInstanceExpression(ir::ETSNewClassInstanceExpression *classInstance);
    parser::Program *GetExternalProgram(const ir::ETSImportDeclaration *import);
    parser::Program *RegisterImportTarget(const ir::ETSImportDeclaration *import);

    void AddImportNamespaceSpecifiersToTopBindings(parser::Program *const importedProgram,
                                                   ir::ImportNamespaceSpecifier *namespaceSpecifier,
                                                   const ir::ETSImportDeclaration *import);
    LocalVariable *CreateNamedImportBinding(util::StringView importedName, ir::Identifier *local,
                                            const ir::ETSImportDeclaration *import,
                                            ImportBindingKind kind = ImportBindingKind::NAMED);
    util::StringView GetAdjustedImportedName(ir::ImportSpecifier *const importSpecifier,
                                             const ir::ETSImportDeclaration *const import);
    void AddImportSpecifiersToTopBindings(ir::ImportSpecifier *importSpecifier, const ir::ETSImportDeclaration *import);
    void AddImportDefaultSpecifiersToTopBindings(ir::ImportDefaultSpecifier *importDefaultSpecifier,
                                                 const ir::ETSImportDeclaration *import);
    void AddSpecifiersToTopBindings(ir::AstNode *const specifier, const ir::ETSImportDeclaration *const import);
    void BindReExportSpecifierIdentifiers(ir::AstNode *specifier, const ir::ETSImportDeclaration *import);
    void BuildReExportDeclaration(ir::ETSReExportDeclaration *reExportDecl);

    void ResolveInterfaceDeclaration(ir::TSInterfaceDeclaration *decl);
    void ResolveMethodDefinition(ir::MethodDefinition *methodDef);
    LocalScope *ResolvePropertyReference(ir::ClassProperty *prop, ClassScope *scope);
    void ResolveEnumDeclaration(ir::TSEnumDeclaration *enumDecl);
    void InitializeInterfaceIdent(ir::TSInterfaceDeclaration *decl);
    void BuildExternalProgram(parser::Program *extProgram);
    void BuildProgram();

    void BuildFunctionName(const ir::ScriptFunction *func) const;
    bool BuildInternalNameWithCustomRecordTable(ir::ScriptFunction *scriptFunc, RecordTable *recordTable);
    void AddFunctionThisParam(ir::ScriptFunction *func);

    void ThrowError(const lexer::SourcePosition &pos, const diagnostic::DiagnosticKind &kind) const
    {
        ThrowError(pos, kind, util::DiagnosticMessageParams {});
    }
    void ThrowError(const lexer::SourcePosition &pos, const diagnostic::DiagnosticKind &kind,
                    const util::DiagnosticMessageParams &params) const override;
    bool IsGlobalIdentifier(const util::StringView &str) const override;

    void SetDefaultImports(ArenaVector<ir::ETSImportDeclaration *> defaultImports) noexcept
    {
        defaultImports_ = std::move(defaultImports);
    }

    void AddDynamicImport(ir::ETSImportDeclaration *import);

    [[nodiscard]] const ArenaVector<ir::ETSImportDeclaration *> &DynamicImports() const noexcept
    {
        return dynamicImports_;
    }

    void AddReExportImport(ir::ETSReExportDeclaration *reExport) noexcept
    {
        reExportImports_[reExport->Program()].push_back(reExport);
    }

    const auto &ReExportImports() const
    {
        return reExportImports_;
    }

    auto &ReExportImports()
    {
        return reExportImports_;
    }

    [[nodiscard]] const ir::AstNode *DefaultExport() noexcept
    {
        return defaultExport_;
    }

    void SetDefaultExport(ir::AstNode *defaultExport) noexcept
    {
        defaultExport_ = defaultExport;
    }

    void ResolveReferenceForScope(ir::AstNode *node, Scope *scope);
    void ResolveReferencesForScope(ir::AstNode const *parent, Scope *scope);

    void ResolveReferencesForScopeWithContext(ir::AstNode *node, Scope *scope);

    [[nodiscard]] bool AddSelectiveExportAlias(const SelectiveExportAlias &alias) noexcept;

    [[nodiscard]] const ExportFactStore::ExportFactSnapshot &GetExportFacts(parser::Program *program) const
    {
        return exportFactStore_->GetExportFacts(program);
    }

    [[nodiscard]] ExportFactStore &GetExportFactsStore()
    {
        return *exportFactStore_;
    }

    [[nodiscard]] const ExportFactStore &GetExportFactsStore() const
    {
        return *exportFactStore_;
    }

    [[nodiscard]] const ArenaVector<PendingLocalExportAlias> &PendingLocalExportAliases(parser::Program *program) const
    {
        return exportFactStore_->PendingLocalExportAliases(program);
    }

    void CollectExportFactsForCurrentProgram();

    void CleanUp() override
    {
        VarBinder::CleanUp();
        externalRecordTable_.clear();
        InitImplicitThisParam();
        dynamicImports_.clear();
        reExportImports_.clear();
        defaultExport_ = nullptr;
        exportFactStore_->ClearImportTargets(Program());
        exportFactStore_->ResetProgram(Program());
        globalRecordTable_->CleanUp();
    }

    void CopyTo(VarBinder *target) override
    {
        auto targetImpl = reinterpret_cast<ETSBinder *>(target);

        targetImpl->defaultImports_ = defaultImports_;
        InitImplicitThisParam();
        targetImpl->exportFactStore_ = exportFactStore_;

        VarBinder::CopyTo(target);
    }

private:
    void BuildClassDefinitionImpl(ir::ClassDefinition *classDef);
    void InitImplicitThisParam();
    void HandleStarImport(ir::TSQualifiedName *importName, util::StringView fullPath);
    void InsertForeignBinding(const util::StringView &name, Variable *var);
    void InsertOrAssignForeignBinding(const util::StringView &name, Variable *var);
    void ImportAllForeignBindings(const parser::Program *importedProgram);
    void ThrowRedeclarationError(const lexer::SourcePosition &pos, const Variable *const var,
                                 const Variable *const variable, util::StringView localName);

    // NOTE(dkofanov): #32418.
    RecordTable *globalRecordTable_;
    RecordTable *recordTable_;
    ArenaMap<parser::Program *, RecordTable *> externalRecordTable_;
    ArenaVector<ir::ETSImportDeclaration *> defaultImports_;  // 1
    ArenaVector<ir::ETSImportDeclaration *> dynamicImports_;
    ArenaMap<const parser::Program *, ArenaVector<ir::ETSReExportDeclaration *>> reExportImports_;
    ir::Identifier *thisParam_ {};  // 2
    ir::AstNode *defaultExport_ {};
    ExportFactStore *exportFactStore_;

    friend class RecordTableContext;
};

class RecordTableContext {
public:
    RecordTableContext(ETSBinder *varBinder, parser::Program *extProgram)
        : varBinder_(varBinder), savedRecordTable_(varBinder->recordTable_)
    {
        if (extProgram != nullptr &&
            varBinder->externalRecordTable_.find(extProgram) != varBinder->externalRecordTable_.end()) {
            varBinder->recordTable_ = varBinder->externalRecordTable_[extProgram];
        } else if (varBinder->GetGlobalRecordTable()->Program() == extProgram) {
            varBinder->recordTable_ = varBinder->GetGlobalRecordTable();
        }
    }

    NO_COPY_SEMANTIC(RecordTableContext);
    NO_MOVE_SEMANTIC(RecordTableContext);

    ~RecordTableContext()
    {
        varBinder_->recordTable_ = savedRecordTable_;
    }

private:
    ETSBinder *varBinder_;
    RecordTable *savedRecordTable_;
};

}  // namespace ark::es2panda::varbinder

#endif
