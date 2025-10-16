/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
using AliasesByExportedNames = std::map<util::StringView, std::pair<util::StringView, ir::AstNode const *>>;
using ModulesToExportedNamesWithAliases = std::map<util::StringView, AliasesByExportedNames>;

struct DynamicImportData {
    const ir::ETSImportDeclaration *import;
    const ir::AstNode *specifier;
    Variable *variable;
};

using DynamicImportVariables = ArenaUnorderedMap<const Variable *, DynamicImportData>;

class ETSBinder : public TypedBinder {
public:
    explicit ETSBinder(ArenaAllocator *allocator)
        : TypedBinder(allocator),
          globalRecordTable_(allocator, Program(), RecordTableFlags::NONE),
          recordTable_(&globalRecordTable_),
          externalRecordTable_(Allocator()->Adapter()),
          defaultImports_(Allocator()->Adapter()),
          dynamicImports_(Allocator()->Adapter()),
          reExportImports_(Allocator()->Adapter()),
          reexportedNames_(Allocator()->Adapter())
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

    [[nodiscard]] RecordTable *GetGlobalRecordTable() noexcept
    {
        return &globalRecordTable_;
    }

    [[nodiscard]] const RecordTable *GetGlobalRecordTable() const noexcept
    {
        return &globalRecordTable_;
    }

    [[nodiscard]] ArenaMap<parser::Program *, RecordTable *> &GetExternalRecordTable() noexcept
    {
        return externalRecordTable_;
    }

    [[nodiscard]] const ArenaMap<parser::Program *, RecordTable *> &GetExternalRecordTable() const noexcept
    {
        return externalRecordTable_;
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
    void ValidateReexportDeclaration(ir::ETSReExportDeclaration *decl);
    void ValidateReexports();
    [[nodiscard]] bool ReexportPathMatchesImportPath(const ir::ETSReExportDeclaration *const reexport,
                                                     const ir::ETSImportDeclaration *const import) const;
    Variable *ValidateImportSpecifier(const ir::ImportSpecifier *const specifier,
                                      const ir::ETSImportDeclaration *const import);
    void BuildETSNewClassInstanceExpression(ir::ETSNewClassInstanceExpression *classInstance);
    [[nodiscard]] bool DetectNameConflict(const util::StringView localName, Variable *const var,
                                          Variable *const otherVar, const ir::StringLiteral *const importPath);
    [[nodiscard]] ArenaVector<parser::Program *> GetExternalProgram(util::StringView sourceName,
                                                                    const ir::StringLiteral *importPath);

    std::pair<ir::ETSImportDeclaration *, ir::AstNode *> FindImportDeclInReExports(
        const ir::ETSImportDeclaration *const import, const util::StringView &imported,
        const ir::StringLiteral *const importPath);
    std::pair<ir::ETSImportDeclaration *, ir::AstNode *> FindImportDeclInNamedExports(
        const ir::ETSImportDeclaration *const import, const util::StringView &imported,
        const ir::StringLiteral *const importPath);
    std::pair<ir::ETSImportDeclaration *, ir::AstNode *> FindImportDeclInExports(
        const ir::ETSImportDeclaration *const import, const util::StringView &imported,
        const ir::StringLiteral *const importPath);
    ir::ETSImportDeclaration *FindImportDeclIn(const ir::ETSImportDeclaration *const import,
                                               const util::StringView &imported,
                                               const ir::StringLiteral *const importPath);
    void AddImportNamespaceSpecifiersToTopBindings(Span<parser::Program *const> records,
                                                   ir::ImportNamespaceSpecifier *namespaceSpecifier,
                                                   const ir::ETSImportDeclaration *import);
    bool AddImportSpecifiersToTopBindings(Span<parser::Program *const> records, ir::ImportSpecifier *importSpecifier,
                                          const ir::ETSImportDeclaration *import);
    void AddImportDefaultSpecifiersToTopBindings(Span<parser::Program *const> records,
                                                 ir::ImportDefaultSpecifier *importDefaultSpecifier,
                                                 const ir::ETSImportDeclaration *import);
    void ValidateImportVariable(const ir::AstNode *node, const ir::AstNode *declNode, const util::StringView &imported,
                                const ir::StringLiteral *const importPath);
    Variable *FindImportSpecifiersVariable(const util::StringView &imported,
                                           const varbinder::Scope::VariableMap &globalBindings,
                                           Span<parser::Program *const> record);
    Variable *FindStaticBinding(Span<parser::Program *const> records, const ir::StringLiteral *importPath);
    Variable *AddImportSpecifierFromReExport(ir::AstNode *importSpecifier, const ir::ETSImportDeclaration *const import,
                                             const util::StringView &imported,
                                             const ir::StringLiteral *const importPath);
    void AddSpecifiersToTopBindings(ir::AstNode *const specifier, const ir::ETSImportDeclaration *const import);

    void ResolveInterfaceDeclaration(ir::TSInterfaceDeclaration *decl);
    void ResolveMethodDefinition(ir::MethodDefinition *methodDef);
    LocalScope *ResolvePropertyReference(ir::ClassProperty *prop, ClassScope *scope);
    void ResolveEnumDeclaration(ir::TSEnumDeclaration *enumDecl);
    void InitializeInterfaceIdent(ir::TSInterfaceDeclaration *decl);
    void BuildExternalProgram(parser::Program *extProgram);
    void BuildProgram();

    void BuildFunctionName(const ir::ScriptFunction *func) const;
    bool BuildInternalNameWithCustomRecordTable(ir::ScriptFunction *scriptFunc, RecordTable *recordTable);
    void BuildProxyMethod(ir::ScriptFunction *func, const util::StringView &containingClassName, bool isExternal);
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
        reExportImports_.insert(reExport);
    }

    [[nodiscard]] const ArenaUnorderedSet<ir::ETSReExportDeclaration *> &ReExportImports() const noexcept
    {
        return reExportImports_;
    }

    [[nodiscard]] ArenaUnorderedSet<ir::ETSReExportDeclaration *> &ReExportImports() noexcept
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

    /* Returns the list of programs belonging to the same compilation unit based on a program path */
    [[nodiscard]] ArenaVector<parser::Program *> GetProgramList(const util::StringView &path) const noexcept;

    void ResolveReferenceForScope(ir::AstNode *node, Scope *scope);
    void ResolveReferencesForScope(ir::AstNode const *parent, Scope *scope);

    void ResolveReferencesForScopeWithContext(ir::AstNode *node, Scope *scope);

    [[nodiscard]] bool AddSelectiveExportAlias(parser::ETSParser *parser, util::StringView const &path,
                                               util::StringView const &key, util::StringView const &value,
                                               ir::AstNode const *decl) noexcept;

    [[nodiscard]] const ModulesToExportedNamesWithAliases &GetSelectiveExportAliasMultimap() const noexcept
    {
        return selectiveExportAliasMultimap_;
    }

    [[nodiscard]] ModulesToExportedNamesWithAliases &GetSelectiveExportAliasMultimap() noexcept
    {
        return selectiveExportAliasMultimap_;
    }

    util::StringView FindNameInAliasMap(const util::StringView &pathAsKey, const util::StringView &aliasName);
    std::pair<util::StringView, const ir::AstNode *> FindNameAndNodeInAliasMap(const util::StringView &pathAsKey,
                                                                               const util::StringView &aliasName);

    void CleanUp() override
    {
        VarBinder::CleanUp();
        externalRecordTable_.clear();
        InitImplicitThisParam();
        dynamicImports_.clear();
        reexportedNames_.clear();
        reExportImports_.clear();
        defaultExport_ = nullptr;
        globalRecordTable_.CleanUp();
    }

    void CopyTo(VarBinder *target) override
    {
        auto targetImpl = reinterpret_cast<ETSBinder *>(target);

        targetImpl->defaultImports_ = defaultImports_;
        InitImplicitThisParam();
        targetImpl->selectiveExportAliasMultimap_ = selectiveExportAliasMultimap_;
        ;

        VarBinder::CopyTo(target);
    }

private:
    void BuildClassDefinitionImpl(ir::ClassDefinition *classDef);
    void InitImplicitThisParam();
    void HandleStarImport(ir::TSQualifiedName *importName, util::StringView fullPath);
    void ImportGlobalProperties(const ir::ClassDefinition *classDef);
    bool ImportGlobalPropertiesForNotDefaultedExports(varbinder::Variable *var, const util::StringView &name,
                                                      const ir::ClassElement *classElement);
    void InsertForeignBinding(const util::StringView &name, Variable *var);
    void InsertOrAssignForeignBinding(const util::StringView &name, Variable *var);
    void ImportAllForeignBindings(const varbinder::Scope::VariableMap &globalBindings,
                                  const parser::Program *importProgram, const varbinder::GlobalScope *importGlobalScope,
                                  const ir::ETSImportDeclaration *import);
    void ThrowRedeclarationError(const lexer::SourcePosition &pos, const Variable *const var,
                                 const Variable *const variable, util::StringView localName);

    RecordTable globalRecordTable_;
    RecordTable *recordTable_;
    ArenaMap<parser::Program *, RecordTable *> externalRecordTable_;
    ArenaVector<ir::ETSImportDeclaration *> defaultImports_;  // 1
    ArenaVector<ir::ETSImportDeclaration *> dynamicImports_;
    ArenaUnorderedSet<ir::ETSReExportDeclaration *> reExportImports_;
    ArenaSet<util::StringView> reexportedNames_;
    ir::Identifier *thisParam_ {};  // 2
    ir::AstNode *defaultExport_ {};
    ModulesToExportedNamesWithAliases selectiveExportAliasMultimap_;  // 3

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
