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

#include "serialization.h"
#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "schemaMetadataGenerated.h"
#include "checker/types/ts/unionType.h"

#include <string>

namespace ark::es2panda::compiler {

using checker::ETSObjectFlags, checker::Type;
using ir::MethodDefinition, ir::ClassDefinition;

constexpr auto NOT_BUILTIN_TYPE_KIND = static_cast<Metadata::BuiltinTypeKind>(-1);

// NOLINTNEXTLINE(cert-err58-cpp,fuchsia-statically-constructed-objects)
const std::map<ETSObjectFlags, Metadata::BuiltinTypeKind> MetadataSerializationPhase::BUILTIN_PRIMITIVE_TYPES = {
    {ETSObjectFlags::BUILTIN_BOOLEAN, Metadata::BuiltinTypeKind::BuiltinTypeKind_boolean},
    {ETSObjectFlags::BUILTIN_BYTE, Metadata::BuiltinTypeKind::BuiltinTypeKind_byte_},
    {ETSObjectFlags::BUILTIN_SHORT, Metadata::BuiltinTypeKind::BuiltinTypeKind_short_},
    {ETSObjectFlags::BUILTIN_CHAR, Metadata::BuiltinTypeKind::BuiltinTypeKind_char_},
    {ETSObjectFlags::BUILTIN_INT, Metadata::BuiltinTypeKind::BuiltinTypeKind_int_},
    {ETSObjectFlags::BUILTIN_LONG, Metadata::BuiltinTypeKind::BuiltinTypeKind_long_},
    {ETSObjectFlags::BUILTIN_FLOAT, Metadata::BuiltinTypeKind::BuiltinTypeKind_float_},
    {ETSObjectFlags::BUILTIN_DOUBLE, Metadata::BuiltinTypeKind::BuiltinTypeKind_double_},
    {ETSObjectFlags::BUILTIN_ARRAY, Metadata::BuiltinTypeKind::BuiltinTypeKind_array},
};

Metadata::BuiltinTypeKind MetadataSerializationPhase::GetBuiltinTypeKind(const Type *etsType)
{
    if (etsType->IsETSNeverType()) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_never;
    }
    if (etsType->IsETSVoidType()) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_void_;
    }
    if (etsType->IsETSBigIntType()) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_bigint;
    }
    if (etsType->IsETSStringType()) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_string_;
    }
    if (etsType->IsETSUndefinedType()) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_undefined;
    }
    if (etsType->IsETSNullType()) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_null;
    }
    if (etsType->IsETSAnyType()) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_any;
    }
    if (etsType->IsETSObjectType()) {
        if (const auto entry = BUILTIN_PRIMITIVE_TYPES.find(etsType->AsETSObjectType()->BuiltInKind());
            entry != BUILTIN_PRIMITIVE_TYPES.end()) {
            return entry->second;
        }
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_object;
    }
    return NOT_BUILTIN_TYPE_KIND;
}

Offset<Vector<Offset<Metadata::ValueParamDecl>>> MetadataSerializationPhase::BuildValueParams(
    FlatBufferBuilder &builder, const ArenaVector<varbinder::LocalVariable *> &astValueParams)
{
    std::vector<Offset<Metadata::ValueParamDecl>> valueParams;
    for (const auto &param : astValueParams) {
        const auto paramName = builder.CreateSharedString(std::string(param->Name()));
        const auto [preturnTypeKind, preturnTypeOff] = BuildType(builder, param->TsType());
        valueParams.emplace_back(Metadata::CreateValueParamDecl(builder, paramName, preturnTypeKind, preturnTypeOff));
    }
    return builder.CreateVector<Offset<Metadata::ValueParamDecl>>(valueParams);
}

Offset<Vector<Offset<Metadata::TypeParamDecl>>> MetadataSerializationPhase::BuildTypeParams(
    FlatBufferBuilder &builder, const ArenaVector<Type *> &astTypeParams)
{
    std::vector<Offset<Metadata::TypeParamDecl>> typeParams;
    for (const auto &type : astTypeParams) {
        typeParams.emplace_back(BuildTypeParamDecl(builder, type));
    }
    return builder.CreateVector<Offset<Metadata::TypeParamDecl>>(typeParams);
}

Offset<Metadata::TypeParamDecl> MetadataSerializationPhase::BuildTypeParamDecl(FlatBufferBuilder &builder,
                                                                               const Type *typeParam)
{
    return Metadata::CreateTypeParamDecl(builder, builder.CreateSharedString(typeParam->ToString()));
}

Offset<Metadata::ClassDecl> MetadataSerializationPhase::BuildClassDecl(FlatBufferBuilder &builder,
                                                                       const ClassDefinition *astDecl)
{
    const auto className = builder.CreateSharedString(astDecl->Ident()->ToString());
    const auto methods = BuildClassMethods(builder, astDecl);
    const auto fields = BuildClassProperties(builder, astDecl);
    return Metadata::CreateClassDecl(builder, className, methods, fields);
}

Offset<Metadata::AnnotationDecl> MetadataSerializationPhase::BuildAnnotationDecl(
    FlatBufferBuilder &builder, const ir::AnnotationDeclaration *astDecl)
{
    const auto annotationName = builder.CreateSharedString(astDecl->GetBaseName()->ToString());
    return Metadata::CreateAnnotationDecl(builder, annotationName);
}

Offset<Metadata::EnumDecl> MetadataSerializationPhase::BuildEnumDecl(FlatBufferBuilder &builder,
                                                                     const ClassDefinition *astDecl)
{
    const auto enumName = builder.CreateSharedString(astDecl->Ident()->ToString());

    std::vector<Offset<flatbuffers::String>> entries;
    for (const auto member : astDecl->Body()) {
        if (!member->IsClassProperty()) {
            continue;
        }
        if (auto propName = member->AsClassProperty()->Key()->AsIdentifier()->ToString();
            !propName.empty() && propName[0] != '#') {
            entries.emplace_back(builder.CreateSharedString(propName));
        }
    }

    return Metadata::CreateEnumDecl(builder, enumName, builder.CreateVector<Offset<flatbuffers::String>>(entries));
}

Offset<Vector<Offset<Metadata::VarDecl>>> MetadataSerializationPhase::BuildClassProperties(
    FlatBufferBuilder &builder, const ClassDefinition *astDecl)
{
    std::vector<Offset<Metadata::VarDecl>> fields;

    for (const auto elem : astDecl->Body()) {
        if (elem->IsClassProperty()) {
            fields.emplace_back(BuildVarDecl(builder, elem->AsClassProperty()));
        }
    }

    return builder.CreateVector<Offset<Metadata::VarDecl>>(fields);
}

Offset<Vector<Offset<Metadata::FunctionDecl>>> MetadataSerializationPhase::BuildClassMethods(
    FlatBufferBuilder &builder, const ClassDefinition *astDecl)
{
    std::vector<Offset<Metadata::FunctionDecl>> methods;

    for (const auto elem : astDecl->Body()) {
        if (elem->IsMethodDefinition()) {
            methods.emplace_back(BuildFunctionDecl(builder, elem->AsMethodDefinition()->Function()));
        }
    }

    return builder.CreateVector<Offset<Metadata::FunctionDecl>>(methods);
}

Offset<> MetadataSerializationPhase::BuildStringLiteralType(FlatBufferBuilder &builder,
                                                            const checker::ETSStringType *type)
{
    const auto stringValue = builder.CreateString(type->GetValue().Bytes(), type->GetValue().Length());
    return Metadata::CreateStringLiteralType(builder, stringValue).Union();
}

Offset<> MetadataSerializationPhase::BuildUnionType(FlatBufferBuilder &builder, const checker::ETSUnionType *type)
{
    std::vector<uint8_t> typeKinds;
    std::vector<Offset<>> types;
    for (auto const &componentType : type->ConstituentTypes()) {
        const auto [componentTypeKind, componentTypeOff] = BuildType(builder, componentType);
        types.emplace_back(componentTypeOff);
        typeKinds.emplace_back(componentTypeKind);
    }
    return Metadata::CreateUnionType(builder, builder.CreateVector(typeKinds), builder.CreateVector(types)).Union();
}

Offset<> MetadataSerializationPhase::BuildRefType(FlatBufferBuilder &builder, const checker::ETSObjectType *type)
{
    // Non-class type references are not supported yet
    ES2PANDA_ASSERT(type->GetDeclNode()->IsClassDefinition());
    const auto declNode = type->GetDeclNode()->AsClassDefinition();
    return Metadata::CreateTypeRef(builder, builder.CreateSharedString(std::string(declNode->InternalName()))).Union();
}

Offset<> MetadataSerializationPhase::BuildTypeParameterType(FlatBufferBuilder &builder,
                                                            const checker::ETSTypeParameter *type)
{
    return Metadata::CreateTypeRef(builder, builder.CreateSharedString(std::string(type->Name()))).Union();
}

std::pair<Metadata::Type, Offset<>> MetadataSerializationPhase::BuildType(FlatBufferBuilder &builder, const Type *type)
{
    if (type->IsETSObjectType() && type->AsETSObjectType()->IsETSStringLiteralType()) {
        return {Metadata::Type::Type_StringLiteral, BuildStringLiteralType(builder, type->AsETSStringType())};
    }

    if (type->IsETSUnionType()) {
        return {Metadata::Type::Type_Union, BuildUnionType(builder, type->AsETSUnionType())};
    }

    if (type->IsETSObjectType()) {
        return {Metadata::Type::Type_Ref, BuildRefType(builder, type->AsETSObjectType())};
    }

    if (type->IsETSTypeParameter()) {
        return {Metadata::Type::Type_Ref, BuildTypeParameterType(builder, type->AsETSTypeParameter())};
    }

    const auto builtinTypeKind = GetBuiltinTypeKind(type);
    ES2PANDA_ASSERT(builtinTypeKind != NOT_BUILTIN_TYPE_KIND);
    return {Metadata::Type::Type_Builtin, Metadata::CreateBuiltinType(builder, builtinTypeKind).Union()};
}

Offset<Metadata::FunctionDecl> MetadataSerializationPhase::BuildFunctionDecl(FlatBufferBuilder &builder,
                                                                             const ir::ScriptFunction *func)
{
    ES2PANDA_ASSERT(func->Signature() && func->Signature()->ReturnType());
    const auto methodName = builder.CreateSharedString(func->Id()->ToString());
    const auto valueParams = BuildValueParams(builder, func->Signature()->Params());
    const auto typeParams = BuildTypeParams(builder, func->Signature()->TypeParams());
    const auto isVoidReturnType =
        func->ReturnTypeAnnotation() && func->ReturnTypeAnnotation()->IsETSPrimitiveType() &&
        func->ReturnTypeAnnotation()->AsETSPrimitiveType()->GetPrimitiveType() == ir::PrimitiveType::VOID;

    // A temporary fix for the void return type because at the current stage, undefined type set instead, as a return
    // type
    const auto [returnTypeKind, returnTypeOff] =
        isVoidReturnType ? std::make_pair(Metadata::Type::Type_Builtin,
                                          Metadata::CreateBuiltinType(builder, Metadata::BuiltinTypeKind_void_).Union())
                         : BuildType(builder, func->Signature()->ReturnType());

    return Metadata::CreateFunctionDecl(builder, methodName, returnTypeKind, returnTypeOff, func->IsStatic(),
                                        valueParams, typeParams);
}

Offset<Metadata::VarDecl> MetadataSerializationPhase::BuildVarDecl(FlatBufferBuilder &builder,
                                                                   const ir::ClassProperty *var)
{
    const auto returnType = var->TsType();
    const auto varName = builder.CreateSharedString(var->Id()->ToString());
    const auto [returnTypeKind, returnTypeOff] = BuildType(builder, returnType);
    return Metadata::CreateVarDecl(builder, varName, returnTypeKind, returnTypeOff, var->IsStatic());
}

std::vector<uint8_t> MetadataSerializationPhase::GetMetadataBytes(
    FlatBufferBuilder &builder, const std::vector<Offset<Metadata::ClassDecl>> &classes,
    const std::vector<Offset<Metadata::AnnotationDecl>> &annotations,
    const std::vector<Offset<Metadata::EnumDecl>> &enums)
{
    const auto root = Metadata::CreateRoot(builder, builder.CreateVector<Offset<Metadata::ClassDecl>>(classes),
                                           0,  // interfaces
                                           builder.CreateVector<Offset<Metadata::EnumDecl>>(enums),
                                           builder.CreateVector<Offset<Metadata::AnnotationDecl>>(annotations),
                                           0,  // types
                                           0,  // functions
                                           0,  // properties
                                           0   // variables
    );
    builder.Finish(root);
    const auto buf = builder.GetBufferSpan();
    return {buf.begin(), buf.end()};
}

void MetadataSerializationPhase::ProcessStatement(FlatBufferBuilder &builder, const ir::Statement &stmt,
                                                  std::vector<Offset<Metadata::ClassDecl>> &classes,
                                                  std::vector<Offset<Metadata::AnnotationDecl>> &annotations,
                                                  std::vector<Offset<Metadata::EnumDecl>> &enums)
{
    if (stmt.IsClassDeclaration() && stmt.AsClassDeclaration()->Definition()->IsGlobal()) {
        classes.emplace_back(BuildClassDecl(builder, stmt.AsClassDeclaration()->Definition()));
        return;
    }
    if (!stmt.IsExported() && !stmt.IsDefaultExported()) {
        return;
    }

    if (stmt.IsTSEnumDeclaration()) {
        const auto enumDecl = stmt.AsTSEnumDeclaration();
        const auto enumName = builder.CreateSharedString(enumDecl->Key()->ToString());

        std::vector<Offset<flatbuffers::String>> entries;
        for (const auto member : enumDecl->Members()) {
            if (member->IsTSEnumMember()) {
                entries.emplace_back(builder.CreateSharedString(member->AsTSEnumMember()->Key()->ToString()));
            }
        }

        const auto entriesVector = builder.CreateVector<Offset<flatbuffers::String>>(entries);
        enums.emplace_back(Metadata::CreateEnumDecl(builder, enumName, entriesVector));
    } else if (stmt.IsClassDeclaration() && stmt.AsClassDeclaration()->Definition()->IsEnumTransformed()) {
        enums.emplace_back(BuildEnumDecl(builder, stmt.AsClassDeclaration()->Definition()));
    } else if (stmt.IsAnnotationDeclaration()) {
        annotations.emplace_back(BuildAnnotationDecl(builder, stmt.AsAnnotationDeclaration()));
    } else if (stmt.IsClassDeclaration()) {
        classes.emplace_back(BuildClassDecl(builder, stmt.AsClassDeclaration()->Definition()));
    }
}

bool MetadataSerializationPhase::PerformForProgram(parser::Program *program)
{
    if (!Context()->config->options->IsEmitMetadata()) {
        return true;
    }

    FlatBufferBuilder builder;
    std::vector<Offset<Metadata::ClassDecl>> classes;
    std::vector<Offset<Metadata::AnnotationDecl>> annotations;
    std::vector<Offset<Metadata::EnumDecl>> enums;

    for (auto &stmt : program->Ast()->Statements()) {
        ProcessStatement(builder, *stmt, classes, annotations, enums);
    }

    Context()->metadata = GetMetadataBytes(builder, classes, annotations, enums);

    return true;
}

}  // namespace ark::es2panda::compiler