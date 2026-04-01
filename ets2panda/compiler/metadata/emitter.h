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

namespace ark::es2panda::compiler {

using flatbuffers::Offset, flatbuffers::Vector, flatbuffers::FlatBufferBuilder;

class MetadataEmittingPhase : public PhaseForProgramsWithBodies {
public:
    std::string_view Name() const override
    {
        return "MetadataEmittingPhase";
    }

    bool PerformForProgram(parser::Program *program) override;

private:
    static Offset<Metadata::ClassDecl> BuildClassDecl(FlatBufferBuilder &builder, const ir::ClassDefinition *astDecl);
    static Offset<Vector<Offset<Metadata::FunctionDecl>>> BuildClassMethods(FlatBufferBuilder &builder,
                                                                            const ir::ClassDefinition *astDecl);
    static Offset<Metadata::FunctionDecl> BuildFunctionDecl(FlatBufferBuilder &builder, ir::MethodDefinition *astDecl);
    static Offset<Vector<Offset<Metadata::TypeParamDecl>>> BuildTypeParams(
        FlatBufferBuilder &builder, const ArenaVector<checker::Type *> &astTypeParams);
    static Offset<Metadata::TypeParamDecl> BuildTypeParamDecl(FlatBufferBuilder &builder,
                                                              const checker::Type *typeParam);
    static Metadata::BuiltinTypeKind GetBuiltinTypeKind(checker::Type *etsType);
    static Offset<Metadata::AnnotationDecl> BuildAnnotationDecl(FlatBufferBuilder &builder,
                                                                const ir::AnnotationDeclaration *astDecl);
    static std::vector<uint8_t> GetMetadataBytes(FlatBufferBuilder &builder,
                                                 const std::vector<Offset<Metadata::ClassDecl>> &classes,
                                                 const std::vector<Offset<Metadata::AnnotationDecl>> &annotations,
                                                 const std::vector<Offset<Metadata::EnumDecl>> &enums);
    static Offset<Metadata::EnumDecl> BuildEnumDecl(FlatBufferBuilder &builder, const ir::ClassDefinition *astDecl);
    static void ProcessStatement(FlatBufferBuilder &builder, const ir::Statement &stmt,
                                 std::vector<Offset<Metadata::ClassDecl>> &classes,
                                 std::vector<Offset<Metadata::AnnotationDecl>> &annotations,
                                 std::vector<Offset<Metadata::EnumDecl>> &enums);
    // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
    static const std::map<checker::ETSObjectFlags, Metadata::BuiltinTypeKind> BUILTIN_PRIMITIVE_TYPES;
};

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_METADATA_EMITTER_H