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

#ifndef ES2PANDA_COMPILER_METADATA_EMITTER_H
#define ES2PANDA_COMPILER_METADATA_EMITTER_H

#include "compiler/lowering/phase.h"
#include "schemaMetadataGenerated.h"
#include "ir/statements/annotationDeclaration.h"
#include "libarkfile/metadata_helper.h"
#include "varbinder/exportFacts.h"

namespace ark::es2panda::varbinder {
struct ImportBindingInfo;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::compiler {

using flatbuffers::Offset, flatbuffers::Vector, flatbuffers::FlatBufferBuilder;

struct MetadataDecls {
    std::vector<Offset<Metadata::ImportDecl>> imports;
    std::vector<Offset<Metadata::ReExportDecl>> reExports;
    std::vector<Offset<Metadata::LocalExportDecl>> localExports;
    std::vector<Offset<Metadata::InterfaceDecl>> interfaces;
    std::vector<Offset<Metadata::ClassDecl>> classes;
    std::vector<Offset<Metadata::AnnotationDecl>> annotations;
    std::vector<Offset<Metadata::TypeDecl>> types;

    [[nodiscard]] bool IsFilled() const
    {
        return !classes.empty() || !annotations.empty() || !interfaces.empty() || !types.empty() ||
               !reExports.empty() || !localExports.empty();
    }
};

class MetadataSerializationPhase : public PhaseForProgramsWithBodies {
public:
    std::string_view Name() const override
    {
        return "MetadataSerializationPhase";
    }

    bool PerformForProgram(parser::Program *program) override;

private:
    Offset<Metadata::TypeDecl> BuildTypeDecl(FlatBufferBuilder &builder, const ir::TSTypeAliasDeclaration *astDecl);
    Offset<Metadata::ClassDecl> BuildClassDecl(FlatBufferBuilder &builder, const ir::ClassDefinition *astDecl);
    Offset<Metadata::InterfaceDecl> BuildInterfaceDecl(FlatBufferBuilder &builder,
                                                       const ir::TSInterfaceDeclaration *interfaceDecl);
    Offset<Vector<Offset<Metadata::FunctionDecl>>> BuildMethodDecls(FlatBufferBuilder &builder,
                                                                    const ArenaVector<ir::AstNode *> &body,
                                                                    bool isFromNamespaceOrTopLevel = false,
                                                                    bool isInterface = false);
    bool ShouldProcessMethod(const ir::MethodDefinition *method, std::unordered_set<std::string> &seenMethodSignatures);
    void IterateMethods(const ArenaVector<ir::AstNode *> &body, const bool isFromNamespaceOrTopLevel,
                        const std::function<void(const ir::MethodDefinition *)> &appendMethod);
    Offset<Vector<Offset<Metadata::PropertyDecl>>> BuildPropertyDecls(FlatBufferBuilder &builder,
                                                                      const ArenaVector<ir::AstNode *> &body,
                                                                      bool isFromNamespaceOrTopLevel = false);
    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    Offset<Metadata::FunctionDecl> BuildFunctionDecl(FlatBufferBuilder &builder, const ir::ScriptFunction *func,
                                                     bool isProtected = false, bool isGetter = false,
                                                     bool isSetter = false, bool isFinal = false, bool isNative = false,
                                                     bool isAbstract = false, std::string_view overloadGroup = {},
                                                     int32_t overloadGroupIndex = 0);
    Offset<> BuildAwaitedType(FlatBufferBuilder &builder, const checker::ETSAwaitedType *type);
    static Offset<> BuildTypeParameterType(FlatBufferBuilder &builder, const checker::ETSTypeParameter *type);
    static Offset<flatbuffers::String> BuildRefTypeName(FlatBufferBuilder &builder, const ir::AstNode &node);
    Offset<> BuildRefType(FlatBufferBuilder &builder, const checker::ETSObjectType *type);
    Offset<> BuildUnionType(FlatBufferBuilder &builder, const checker::ETSUnionType *type);
    static Offset<> BuildStringLiteralType(FlatBufferBuilder &builder, const checker::ETSStringType *type);
    Offset<> BuildArrayType(FlatBufferBuilder &builder, const checker::ETSArrayType *type);
    Offset<> BuildTupleType(FlatBufferBuilder &builder, const checker::ETSTupleType *type);
    Offset<> BuildFunctionType(FlatBufferBuilder &builder, const checker::ETSFunctionType *type);
    Offset<> BuildPartialType(FlatBufferBuilder &builder, const checker::ETSObjectType *type);
    std::pair<Metadata::Type, Offset<>> BuildType(FlatBufferBuilder &builder, const checker::Type *type);
    std::pair<Metadata::Type, Offset<>> BuildAliasType(FlatBufferBuilder &builder,
                                                       const checker::ETSTypeAliasType *type);
    Offset<Vector<Offset<Metadata::TypeParamDecl>>> BuildTypeParams(
        FlatBufferBuilder &builder, const ir::TSTypeParameterDeclaration *astTypeParams);
    std::pair<Metadata::Type, Offset<>> BuildPropertyType(FlatBufferBuilder &builder,
                                                          const ir::ClassProperty *property);
    Offset<Metadata::PropertyDecl> BuildPropertyDecl(FlatBufferBuilder &builder, const ir::ClassProperty *var);
    Offset<Metadata::ImportDecl> BuildImportDecl(FlatBufferBuilder &builder,
                                                 const ir::ImportDeclaration *importDecl) const;
    std::vector<Offset<Metadata::ReExportDecl>> BuildReExportDecls(
        FlatBufferBuilder &builder, const ir::ETSReExportDeclaration *reExportDecl) const;
    Offset<Metadata::ReExportDecl> BuildImportBindingReExportDecl(FlatBufferBuilder &builder,
                                                                  const varbinder::ImportBindingInfo *bindingInfo,
                                                                  util::StringView exportedName,
                                                                  bool isExplicitTypeOnly) const;
    Offset<Metadata::LocalExportDecl> BuildLocalExportDecl(FlatBufferBuilder &builder,
                                                           const varbinder::PendingLocalExportAlias &alias) const;
    Offset<Metadata::LocalExportDecl> BuildLocalExportDecl(FlatBufferBuilder &builder,
                                                           const varbinder::ExportFact &fact) const;
    template <typename T>
    Offset<Metadata::Decls> BuildDecls(FlatBufferBuilder &builder, const ArenaVector<T> &astDecls,
                                       const std::string &pkgName, parser::Program *program = nullptr);
    void AddExportDeclarations(FlatBufferBuilder &builder, parser::Program *program, MetadataDecls &decls);
    Offset<Metadata::Decls> BuildMetadataDecls(FlatBufferBuilder &builder, const std::string &pkgName,
                                               const MetadataDecls &decls);
    std::pair<std::vector<Offset<>>, std::vector<uint8_t>> BuildExtends(
        FlatBufferBuilder &builder, const ArenaVector<ir::TSInterfaceHeritage *> &extends);
    Offset<Vector<Offset<Metadata::ValueParamDecl>>> BuildValueParams(FlatBufferBuilder &builder,
                                                                      const checker::Signature *signature);
    struct EnumInfo {
        Metadata::EnumKind kind;
        std::vector<Offset<flatbuffers::String>> values;
        Metadata::Type underlyingTypeKind;
        Offset<> underlyingTypeOff;
    };
    EnumInfo BuildEnumInfo(FlatBufferBuilder &builder, const ir::ClassDefinition *astDecl);
    std::vector<Offset<flatbuffers::String>> ExtractEnumValues(FlatBufferBuilder &builder,
                                                               const ir::ClassDefinition *astDecl) const;
    std::pair<Metadata::Type, Offset<>> ExtractEnumUnderlyingType(FlatBufferBuilder &builder,
                                                                  const ir::ClassDefinition *astDecl);
    static Metadata::BuiltinTypeKind GetBuiltinTypeKind(const checker::Type *etsType);
    Offset<Metadata::AnnotationDecl> BuildAnnotationDecl(FlatBufferBuilder &builder,
                                                         const ir::AnnotationDeclaration *astDecl);
    static std::vector<uint8_t> GetMetadataBytes(FlatBufferBuilder &builder, const Offset<Metadata::Decls> &fbDecls);
    void ProcessStatement(FlatBufferBuilder &builder, const ir::AstNode &node, MetadataDecls &decls);
    // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
    static const std::map<checker::ETSObjectFlags, Metadata::BuiltinTypeKind> BUILTIN_PRIMITIVE_TYPES;

#if defined(METADATA_VERBOSE) && METADATA_VERBOSE
    uint8_t curLogLevel_ = 0;
#endif
};

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_METADATA_EMITTER_H
