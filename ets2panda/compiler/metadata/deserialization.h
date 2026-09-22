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

#ifndef ES2PANDA_COMPILER_METADATA_DESERIALIZER_H
#define ES2PANDA_COMPILER_METADATA_DESERIALIZER_H

#include "compiler/lowering/phase.h"
#include "schemaMetadataGenerated.h"
#include "libarkfile/metadata_helper.h"

#include <unordered_map>
#include <unordered_set>

namespace ark::es2panda::compiler {

using ValueParamsInfo = std::pair<ArenaVector<ir::Expression *>, varbinder::FunctionParamScope *>;
using TypeParamsInfo = std::pair<ir::TSTypeParameterDeclaration *, varbinder::LocalScope *>;
using FbMethodParams = std::pair<const flatbuffers::Vector<flatbuffers::Offset<Metadata::ValueParamDecl>> *,
                                 const flatbuffers::Vector<flatbuffers::Offset<Metadata::TypeParamDecl>> *>;
using MethodOptions = std::pair<ir::ScriptFunctionFlags, ir::ModifierFlags>;

class MetadataClassBuilder;

class MetadataDeserializationPhase : public PhaseForMetadataBasedPrograms {
public:
    std::string_view Name() const override
    {
        return "MetadataDeserializationPhase";
    }

    bool PerformForProgram(parser::Program *program) override;

private:
    friend class MetadataClassBuilder;
    void SetupGlobalClassStaticBlock(ir::ClassStaticBlock *staticBlock) const;
    void SetupGlobalClass() const;
    void MarkBuiltinIfNeeded(varbinder::Variable *var, bool isBuiltin) const;

    void AddClassMembers(const Metadata::ClassDecl *fbClassDecl, ir::ClassDefinition *classDef);
    void EnsureConstructorExists(ir::ClassDefinition *classDef);
    void AddProperties(const Metadata::ClassDecl *fbClassDecl, ir::ClassDefinition *classDef);
    void AddNestedDeclarations(const Metadata::ClassDecl *fbClassDecl, ir::ClassDefinition *classDef);
    ir::Identifier *GetDeclarationIdentifier(ir::AstNode *decl);
    void AddEnumMembers(const Metadata::ClassDecl *fbClassDecl, ir::ClassDefinition *classDef);
    bool DetermineIsStringEnum(const Metadata::ClassDecl *fbClassDecl) const;
    const flatbuffers::String *GetEnumValue(const Metadata::ClassDecl *fbClassDecl, uint32_t index) const;
    ArenaVector<ir::AstNode *> CreateEnumMemberNodes(const Metadata::ClassDecl *fbClassDecl, bool isStringEnum) const;
    ir::TSEnumDeclaration *CreateOrigEnumDecl(const Metadata::ClassDecl *fbClassDecl, ir::ClassDefinition *classDef,
                                              ArenaVector<ir::AstNode *> &&enumMembers) const;
    ir::ClassProperty *AddSyntheticEnumProperty(std::string_view propName, const Metadata::ClassDecl *fbClassDecl,
                                                ir::ClassDefinition *classDef,
                                                std::function<ir::Expression *(uint32_t)> &&makeElement) const;
    void AddInterfaceProperties(const Metadata::InterfaceDecl *fbInterfaceDecl,
                                ir::TSInterfaceDeclaration *interfaceDecl) const;
    void AddExtends(const Metadata::InterfaceDecl *fbInterfaceDecl, ir::TSInterfaceDeclaration *interfaceDecl) const;
    void AddMethods(const flatbuffers::Vector<flatbuffers::Offset<Metadata::FunctionDecl>> &methods,
                    ArenaVector<ir::AstNode *> &body, ir::AstNode *parent);
    void AddOverloadGroups(const flatbuffers::Vector<flatbuffers::Offset<Metadata::FunctionDecl>> &methods,
                           ArenaVector<ir::AstNode *> &body, ir::AstNode *parent) const;

    template <typename T>
    constexpr auto &GetLazyMembers();

    template <typename T, typename K>
    void MaterializeMembers(T *node, K const *fbDecl = nullptr);

    template <typename T>
    void RunBinderForMembers(T *node) const;

    ValueParamsInfo CreateValueParams(
        const flatbuffers::Vector<flatbuffers::Offset<Metadata::ValueParamDecl>> *fbValueParams) const;
    ir::TSTypeParameterDeclaration *CreateTypeParams(
        const flatbuffers::Vector<flatbuffers::Offset<Metadata::TypeParamDecl>> *fbTypeParams) const;

    ir::TypeNode *CreateBuiltinType(Metadata::BuiltinTypeKind kind) const;
    ir::TypeNode *CreateRefType(const Metadata::TypeRef *fbRefType) const;
    ir::TypeNode *CreateUnionType(const Metadata::UnionType *fbUnionType) const;
    ir::TypeNode *CreateArrayType(const Metadata::ArrayType *fbArrayType) const;
    ir::TypeNode *CreateTupleType(const Metadata::TupleType *fbTupleType) const;
    ir::TypeNode *CreateFunctionType(const Metadata::FunctionType *fbFunctionType) const;
    ir::TypeNode *CreatePartialType(const Metadata::PartialType *fbPartialType) const;
    ir::TypeNode *CreateStringLiteralType(const Metadata::StringLiteralType *fbStringLiteralType) const;
    ir::TypeNode *CreateType(const void *type, Metadata::Type kind) const;

    ir::MethodDefinition *RegisterMethodInScope(ir::MethodDefinition *methodDef, bool isStatic) const;
    ir::MethodDefinition *CreateMethodDecl(const Metadata::FunctionDecl *fbMethodDecl, bool isGlobalMember);
    ir::ClassProperty *CreatePropertyDecl(const Metadata::PropertyDecl *fbPropDecl, bool isGlobalMember) const;
    ir::OverloadDeclaration *CreateOverloadGroupDecl(std::string_view groupName, bool isStatic,
                                                     const std::vector<std::string_view> &overloadedNames) const;
    ir::ClassProperty *CreateAnnotationPropertyDecl(const Metadata::PropertyDecl *fbPropDecl) const;

    ir::AnnotationDeclaration *CreateAnnotationDecl(const Metadata::AnnotationDecl *fbAnnotationDecl, bool isNested);
    ir::ETSImportDeclaration *CreateImportDecl(const Metadata::ImportDecl *fbImportDecl);
    ir::ETSReExportDeclaration *CreateReExportDecl(const Metadata::ReExportDecl *fbReExportDecl) const;
    void MaterializeTypeDecl(const Metadata::TypeDecl *fbTypeDecl, ir::TSTypeAliasDeclaration *typeDecl);
    std::vector<ir::TSTypeAliasDeclaration *> CreateTypeDecls(
        const flatbuffers::Vector<flatbuffers::Offset<Metadata::TypeDecl>> *fbTypeDecls);
    ir::TSInterfaceDeclaration *CreateInterfaceDecl(const Metadata::InterfaceDecl *fbInterfaceDecl);
    ir::ClassDefinition *CreateClassDecl(
        const Metadata::ClassDecl *fbClassDecl,
        const flatbuffers::Vector<flatbuffers::Offset<Metadata::TypeParamDecl>> *fbTypeParams, bool isNested = false,
        bool isPreDeclare = false, bool materializeMembers = true);
    ir::ETSModule *CreateModule() const;

    void PredeclareClasses(const Metadata::Decls *decls, bool isNested = false);
    ArenaVector<ir::AstNode *> CreateDependencyDecls(const Metadata::Decls *decls);
    ArenaVector<ir::AstNode *> CreateDecls(const Metadata::Decls *decls, bool isNested = false,
                                           parser::Program *moduleProg = nullptr,
                                           varbinder::ExportFactStore *store = nullptr,
                                           varbinder::ETSBinder *etsBinder = nullptr);
    void AddDeclarations(const Metadata::Decls *decls, bool isNested, parser::Program *moduleProg,
                         varbinder::ExportFactStore *store, ArenaVector<ir::AstNode *> &nodes);
    varbinder::Variable *FindMetadataLocalVariable(parser::Program *program, util::StringView localName) const;

    varbinder::Scope *Scope() const
    {
        return Context()->GetChecker()->VarBinder()->GetScope();
    }
    template <typename T, typename F>
    T WithScope(varbinder::Scope *scope, F &&run);
    template <typename F>
    void WithProgram(parser::Program *program, F &&run);

    parser::Program *ResolveMetadataModuleProgram(const Metadata::Decls *root, std::string_view moduleName) const;
    void BindMetadataImports(const ArenaVector<ir::AstNode *> &nodes, varbinder::ETSBinder *etsBinder,
                             varbinder::ExportFactStore *moduleStore);
    void RegisterMetadataReExportSpecifier(varbinder::ExportFactStore *store, parser::Program *moduleProg,
                                           const ir::ETSImportDeclaration *importDecl, ir::AstNode *specifier) const;
    void RegisterMetadataReExports(const ArenaVector<ir::AstNode *> &nodes, parser::Program *moduleProg,
                                   varbinder::ExportFactStore *store, varbinder::ETSBinder *etsBinder);
    void RegisterMetadataLocalExports(const ArenaVector<ir::AstNode *> &nodes, parser::Program *moduleProg,
                                      varbinder::ExportFactStore *store) const;
    void RegisterMetadataLocalExportSpecifier(
        const flatbuffers::Vector<flatbuffers::Offset<Metadata::LocalExportDecl>> *fbLocalExports,
        parser::Program *moduleProg, varbinder::ExportFactStore *store) const;

    void ProcessMetadata(panda_file::MetadataByModules *metadata);
    void ProcessMetadataModule(parser::Program *moduleProg, const Metadata::Decls *root);

    parser::Program *curProgram = nullptr;

    std::unordered_map<const ir::ClassDefinition *, std::pair<const Metadata::ClassDecl *, parser::Program *>>
        lazyClassMembers_;
    std::unordered_map<const ir::TSInterfaceDeclaration *,
                       std::pair<const Metadata::InterfaceDecl *, parser::Program *>>
        lazyInterfaceMembers_;
    std::unordered_map<const parser::Program *, std::unordered_set<std::string>> mergedImportSpecifiers_;

    static const std::map<Metadata::BuiltinTypeKind, ir::PrimitiveType> BUILTIN_PRIMITIVE_TYPES;

#if defined(METADATA_VERBOSE) && METADATA_VERBOSE
    uint8_t curLogLevel_ = 0;
#endif
};
}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_METADATA_DESERIALIZER_H
