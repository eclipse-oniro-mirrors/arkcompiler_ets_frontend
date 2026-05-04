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

namespace ark::es2panda::compiler {

using ValueParamsInfo = std::pair<ArenaVector<ir::Expression *>, varbinder::FunctionParamScope *>;
using TypeParamsInfo = std::pair<ir::TSTypeParameterDeclaration *, varbinder::LocalScope *>;
using FbMethodParams = std::pair<const flatbuffers::Vector<flatbuffers::Offset<Metadata::ValueParamDecl>> *,
                                 const flatbuffers::Vector<flatbuffers::Offset<Metadata::TypeParamDecl>> *>;
using MethodOptions = std::pair<ir::ScriptFunctionFlags, ir::ModifierFlags>;

class MetadataDeserializationPhase : public PhaseForMetadataBasedPrograms {
public:
    std::string_view Name() const override
    {
        return "MetadataDeserializationPhase";
    }

    bool PerformForProgram(parser::Program *program) override;

private:
    void SetupGlobalClassStaticBlock(ir::ClassStaticBlock *staticBlock) const;
    void SetupGlobalClass(parser::Program *program) const;
    ir::ETSModule *CreateModule(parser::Program *program) const;
    ValueParamsInfo CreateValueParams(
        const flatbuffers::Vector<flatbuffers::Offset<Metadata::ValueParamDecl>> *fbValueParams,
        varbinder::Scope *parentScope) const;
    TypeParamsInfo CreateTypeParams(
        const flatbuffers::Vector<flatbuffers::Offset<Metadata::TypeParamDecl>> *fbTypeParams,
        varbinder::Scope *parentScope) const;
    ir::MethodDefinition *CreateMethod(const ir::ClassDefinition *classDef, util::StringView methodName,
                                       ir::TypeNode &returnType, FbMethodParams fbParams, MethodOptions options) const;
    ir::ClassDefinition *CreateClass(parser::Program *program, util::StringView className) const;
    ir::TypeNode *CreateBuiltinType(Metadata::BuiltinTypeKind kind) const;
    ir::TypeNode *CreateType(const void *type, Metadata::Type kind) const;
    ir::ClassProperty *CreateField(const ir::ClassDefinition *classDef, util::StringView fieldName,
                                   ir::TypeNode &returnType, ir::ModifierFlags modifiers) const;
    static const std::map<Metadata::BuiltinTypeKind, ir::PrimitiveType> BUILTIN_PRIMITIVE_TYPES;
};
}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_METADATA_DESERIALIZER_H