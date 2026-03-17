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

#include "metadataEmitter.h"
#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "metadata_generated.h"

#include <string>

namespace ark::es2panda::compiler {

using checker::ETSObjectFlags, checker::Type;
using ir::MethodDefinition, ir::ClassDefinition;

const Metadata::BuiltinTypeKind NOT_BUILTIN_TYPE_KIND = static_cast<Metadata::BuiltinTypeKind>(-1);

// NOLINTNEXTLINE(cert-err58-cpp,fuchsia-statically-constructed-objects)
const std::map<checker::ETSObjectFlags, Metadata::BuiltinTypeKind> MetadataEmittingPhase::BUILTIN_PRIMITIVE_TYPES = {
    {ETSObjectFlags::BUILTIN_BOOLEAN, Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_BOOLEAN},
    {ETSObjectFlags::BUILTIN_BYTE, Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_BYTE},
    {ETSObjectFlags::BUILTIN_SHORT, Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_SHORT},
    {ETSObjectFlags::BUILTIN_CHAR, Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_CHAR},
    {ETSObjectFlags::BUILTIN_INT, Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_INT},
    {ETSObjectFlags::BUILTIN_LONG, Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_LONG},
    {ETSObjectFlags::BUILTIN_FLOAT, Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_FLOAT},
    {ETSObjectFlags::BUILTIN_DOUBLE, Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_DOUBLE},
    {ETSObjectFlags::BUILTIN_ARRAY, Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_ARRAY},
};

Metadata::BuiltinTypeKind MetadataEmittingPhase::GetBuiltinTypeKind(Type *etsType)
{
    if (etsType->IsETSNeverType()) {
        return Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_NEVER;
    }
    if (etsType->IsETSVoidType()) {
        return Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_VOID;
    }
    if (etsType->IsETSBigIntType()) {
        return Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_BIGINT;
    }
    if (etsType->IsETSStringType()) {
        return Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_STRING;
    }
    if (etsType->IsETSUndefinedType()) {
        return Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_UNDEFINED;
    }
    if (etsType->IsETSNullType()) {
        return Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_NULL;
    }
    if (etsType->IsETSAnyType()) {
        return Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_ANY;
    }
    if (etsType->IsETSObjectType()) {
        if (auto entry = BUILTIN_PRIMITIVE_TYPES.find(etsType->AsETSObjectType()->BuiltInKind());
            entry != BUILTIN_PRIMITIVE_TYPES.end()) {
            return entry->second;
        }
        return Metadata::BuiltinTypeKind::BUILTIN_TYPE_KIND_OBJECT;
    }
    return NOT_BUILTIN_TYPE_KIND;
}

// Recording type params for methods is only supported currently
Offset<Vector<Offset<Metadata::TypeParamDecl>>> MetadataEmittingPhase::BuildTypeParams(
    FlatBufferBuilder &builder, const ArenaVector<checker::Type *> &astTypeParams)
{
    std::vector<Offset<Metadata::TypeParamDecl>> typeParamDecls;
    for (auto type : astTypeParams) {
        typeParamDecls.emplace_back(BuildTypeParamDecl(builder, type));
    }
    return builder.CreateVector<Offset<Metadata::TypeParamDecl>>(typeParamDecls);
}

Offset<Metadata::TypeParamDecl> MetadataEmittingPhase::BuildTypeParamDecl(FlatBufferBuilder &builder,
                                                                          checker::Type *typeParam)
{
    return Metadata::CreateTypeParamDecl(builder, builder.CreateSharedString(typeParam->ToString()));
}

Offset<Metadata::ClassDecl> MetadataEmittingPhase::BuildClassDecl(FlatBufferBuilder &builder, ClassDefinition *astDecl)
{
    auto className = builder.CreateSharedString(astDecl->Ident()->ToString());
    auto methods = BuildClassMethods(builder, astDecl);  // only methods recording is supported currently
    return Metadata::CreateClassDecl(builder, className, methods);
}

Offset<Vector<Offset<Metadata::FunctionDecl>>> MetadataEmittingPhase::BuildClassMethods(FlatBufferBuilder &builder,
                                                                                        ClassDefinition *astDecl)
{
    std::vector<Offset<Metadata::FunctionDecl>> methods;

    for (auto elem : astDecl->Body()) {
        if (!elem->IsMethodDefinition()) {
            continue;
        }
        methods.emplace_back(BuildFunctionDecl(builder, elem->AsMethodDefinition()));
    }

    return builder.CreateVector<Offset<Metadata::FunctionDecl>>(methods);
}

Offset<Metadata::FunctionDecl> MetadataEmittingPhase::BuildFunctionDecl(FlatBufferBuilder &builder,
                                                                        MethodDefinition *astDecl)
{
    ES2PANDA_ASSERT(astDecl->Function() && astDecl->Function()->Signature() &&
                    astDecl->Function()->Signature()->ReturnType());
    auto returnType = astDecl->Function()->Signature()->ReturnType();
    /*
     * Only builtin return types are supported currently,
     * If the actual return type is not builtin, it'd be recorded as a special value of `BuiltinTypeKind` enum
     * (`NOT_BUILTIN_TYPE_KIND`)
     */
    auto builtinTypeKind = GetBuiltinTypeKind(returnType);
    auto methodName = builder.CreateSharedString(astDecl->Id()->ToString());
    auto builtInType = Metadata::CreateBuiltinType(builder, builtinTypeKind).Union();
    auto typeParams = BuildTypeParams(builder, astDecl->Function()->Signature()->TypeParams());

    return Metadata::CreateFunctionDecl(builder, methodName, Metadata::Type::TYPE_BUILTIN, builtInType, 0, typeParams);
}

bool MetadataEmittingPhase::PerformForProgram(parser::Program *program)
{
    auto ctx = Context();
    if (!ctx->config->options->IsEmitMetadata()) {
        return true;
    }

    FlatBufferBuilder builder;
    std::vector<Offset<Metadata::ClassDecl>> classes;  // only classes recording is supported curently
    for (auto &stmt : program->Ast()->Statements()) {
        ClassDefinition *exportedClass;
        if (stmt->IsExported() && stmt->IsClassDeclaration()) {
            exportedClass = stmt->AsClassDeclaration()->Definition();
        } else if (stmt->IsDefaultExported() && stmt->IsClassDeclaration()) {
            exportedClass = stmt->AsClassDeclaration()->Definition();
        } else {
            continue;
        }

        classes.emplace_back(BuildClassDecl(builder, exportedClass));
    }
    auto root = Metadata::CreateRoot(builder, builder.CreateVector<Offset<Metadata::ClassDecl>>(classes));
    builder.Finish(root);
    auto buf = builder.GetBufferSpan();
    std::vector<uint8_t> bytes;
    for (auto b : buf) {
        bytes.emplace_back(b);
    }
    ctx->metadata = std::move(bytes);
    return true;
}

}  // namespace ark::es2panda::compiler
