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

namespace ark::es2panda::compiler {

using flatbuffers::Offset, flatbuffers::Vector, flatbuffers::FlatBufferBuilder;

struct MetadataDecls {
    std::vector<Offset<Metadata::ImportDecl>> imports;
    std::vector<Offset<Metadata::InterfaceDecl>> interfaces;
    std::vector<Offset<Metadata::ClassDecl>> classes;
    std::vector<Offset<Metadata::AnnotationDecl>> annotations;
    std::vector<Offset<Metadata::TypeDecl>> types;

    [[nodiscard]] bool IsFilled() const
    {
        return !classes.empty() || !annotations.empty() || !interfaces.empty() || !types.empty();
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
                                                                    bool isFromNamespaceOrTopLevel = false);
    Offset<Vector<Offset<Metadata::PropertyDecl>>> BuildPropertyDecls(FlatBufferBuilder &builder,
                                                                      const ArenaVector<ir::AstNode *> &body,
                                                                      bool isFromNamespaceOrTopLevel = false);
    Offset<Metadata::FunctionDecl> BuildFunctionDecl(FlatBufferBuilder &builder, const ir::ScriptFunction *func);
    static Offset<> BuildTypeParameterType(FlatBufferBuilder &builder, const checker::ETSTypeParameter *type);
    static Offset<flatbuffers::String> BuildRefTypeName(FlatBufferBuilder &builder, const ir::AstNode &node);
    Offset<> BuildRefType(FlatBufferBuilder &builder, const checker::ETSObjectType *type);
    Offset<> BuildUnionType(FlatBufferBuilder &builder, const checker::ETSUnionType *type);
    static Offset<> BuildStringLiteralType(FlatBufferBuilder &builder, const checker::ETSStringType *type);
    Offset<> BuildArrayType(FlatBufferBuilder &builder, const checker::ETSArrayType *type);
    Offset<> BuildTupleType(FlatBufferBuilder &builder, const checker::ETSTupleType *type);
    Offset<> BuildFunctionType(FlatBufferBuilder &builder, const checker::ETSFunctionType *type);
    std::pair<Metadata::Type, Offset<>> BuildType(FlatBufferBuilder &builder, const checker::Type *type);
    static Offset<> BuildTypeAliasType(FlatBufferBuilder &builder, const checker::ETSTypeAliasType *type);
    static Offset<Vector<Offset<Metadata::TypeParamDecl>>> BuildTypeParams(
        FlatBufferBuilder &builder, const ir::TSTypeParameterDeclaration *astTypeParams);
    Offset<Metadata::PropertyDecl> BuildPropertyDecl(FlatBufferBuilder &builder, const ir::ClassProperty *var);
    Offset<Metadata::ImportDecl> BuildImportDecl(FlatBufferBuilder &builder,
                                                 const ir::ImportDeclaration *importDecl) const;
    template <typename T>
    Offset<Metadata::Decls> BuildDecls(FlatBufferBuilder &builder, const ArenaVector<T> &astDecls);
    std::pair<std::vector<Offset<>>, std::vector<uint8_t>> BuildExtends(
        FlatBufferBuilder &builder, const ArenaVector<ir::TSInterfaceHeritage *> &extends);
    Offset<Vector<Offset<Metadata::ValueParamDecl>>> BuildValueParams(
        FlatBufferBuilder &builder, const ArenaVector<varbinder::LocalVariable *> &astValueParams);
    static Metadata::BuiltinTypeKind GetBuiltinTypeKind(const checker::Type *etsType);
    static Offset<Metadata::AnnotationDecl> BuildAnnotationDecl(FlatBufferBuilder &builder,
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