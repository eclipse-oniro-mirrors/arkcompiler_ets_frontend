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

#include "emitter.h"
#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "schemaMetadataGenerated.h"

#include <string>

namespace ark::es2panda::compiler {

using checker::ETSObjectFlags, checker::Type;
using ir::MethodDefinition, ir::ClassDefinition;

constexpr auto NOT_BUILTIN_TYPE_KIND = static_cast<Metadata::BuiltinTypeKind>(-1);

// NOLINTNEXTLINE(cert-err58-cpp,fuchsia-statically-constructed-objects)
const std::map<ETSObjectFlags, Metadata::BuiltinTypeKind> MetadataEmittingPhase::BUILTIN_PRIMITIVE_TYPES = {
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

Metadata::BuiltinTypeKind MetadataEmittingPhase::GetBuiltinTypeKind(Type *etsType)
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
        if (auto entry = BUILTIN_PRIMITIVE_TYPES.find(etsType->AsETSObjectType()->BuiltInKind());
            entry != BUILTIN_PRIMITIVE_TYPES.end()) {
            return entry->second;
        }
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_object;
    }
    return NOT_BUILTIN_TYPE_KIND;
}

// Recording type params for methods is only supported currently
Offset<Vector<Offset<Metadata::TypeParamDecl>>> MetadataEmittingPhase::BuildTypeParams(
    FlatBufferBuilder &builder, const ArenaVector<Type *> &astTypeParams)
{
    std::vector<Offset<Metadata::TypeParamDecl>> typeParamDecls;
    for (const auto type : astTypeParams) {
        typeParamDecls.emplace_back(BuildTypeParamDecl(builder, type));
    }
    return builder.CreateVector<Offset<Metadata::TypeParamDecl>>(typeParamDecls);
}

Offset<Metadata::TypeParamDecl> MetadataEmittingPhase::BuildTypeParamDecl(FlatBufferBuilder &builder,
                                                                          const Type *typeParam)
{
    return Metadata::CreateTypeParamDecl(builder, builder.CreateSharedString(typeParam->ToString()));
}

Offset<Metadata::ClassDecl> MetadataEmittingPhase::BuildClassDecl(FlatBufferBuilder &builder,
                                                                  const ClassDefinition *astDecl)
{
    const auto className = builder.CreateSharedString(astDecl->Ident()->ToString());
    const auto methods = BuildClassMethods(builder, astDecl);  // only methods recording is supported currently
    return Metadata::CreateClassDecl(builder, className, methods);
}

Offset<Metadata::AnnotationDecl> MetadataEmittingPhase::BuildAnnotationDecl(FlatBufferBuilder &builder,
                                                                            const ir::AnnotationDeclaration *astDecl)
{
    auto annotationName = builder.CreateSharedString(astDecl->GetBaseName()->ToString());
    return Metadata::CreateAnnotationDecl(builder, annotationName);
}

Offset<Metadata::EnumDecl> MetadataEmittingPhase::BuildEnumDecl(FlatBufferBuilder &builder,
                                                                const ClassDefinition *astDecl)
{
    auto enumName = builder.CreateSharedString(astDecl->Ident()->ToString());

    std::vector<Offset<flatbuffers::String>> entries;
    for (auto member : astDecl->Body()) {
        if (member->IsClassProperty()) {
            auto prop = member->AsClassProperty();
            // Skip internal properties (starting with #)
            auto propName = prop->Key()->AsIdentifier()->Name().Utf8();
            if (!propName.empty() && propName[0] != '#') {
                entries.emplace_back(builder.CreateSharedString(propName));
            }
        }
    }

    auto entriesVector = builder.CreateVector<Offset<flatbuffers::String>>(std::move(entries));
    return Metadata::CreateEnumDecl(builder, enumName, entriesVector);
}

Offset<Vector<Offset<Metadata::FunctionDecl>>> MetadataEmittingPhase::BuildClassMethods(FlatBufferBuilder &builder,
                                                                                        const ClassDefinition *astDecl)
{
    std::vector<Offset<Metadata::FunctionDecl>> methods;

    for (const auto elem : astDecl->Body()) {
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
    const auto returnType = astDecl->Function()->Signature()->ReturnType();
    auto methodName = builder.CreateSharedString(astDecl->Id()->ToString());
    auto typeParams = BuildTypeParams(builder, astDecl->Function()->Signature()->TypeParams());

    /*
     * Check for string literal type first, then builtin types
     */
    if (returnType->IsETSObjectType() && returnType->AsETSObjectType()->IsETSStringLiteralType()) {
        auto stringLiteralType = static_cast<checker::ETSStringType *>(returnType);
        auto stringValue =
            builder.CreateString(stringLiteralType->GetValue().Bytes(), stringLiteralType->GetValue().Length());
        auto stringLiteral = Metadata::CreateStringLiteralType(builder, stringValue).Union();
        return Metadata::CreateFunctionDecl(builder, methodName, Metadata::Type::Type_StringLiteral, stringLiteral, 0,
                                            typeParams);
    }

    /*
     * Only builtin and string literal return types are supported currently,
     * If the actual return type is not builtin or string literal, it'd be recorded as a special value of
     * `BuiltinTypeKind` enum (`NOT_BUILTIN_TYPE_KIND`)
     */
    auto builtinTypeKind = GetBuiltinTypeKind(returnType);
    auto builtInType = Metadata::CreateBuiltinType(builder, builtinTypeKind).Union();
    return Metadata::CreateFunctionDecl(builder, methodName, Metadata::Type::Type_Builtin, builtInType, 0, typeParams);
}

std::vector<uint8_t> MetadataEmittingPhase::GetMetadataBytes(
    FlatBufferBuilder &builder, const std::vector<Offset<Metadata::ClassDecl>> &classes,
    const std::vector<Offset<Metadata::AnnotationDecl>> &annotations,
    const std::vector<Offset<Metadata::EnumDecl>> &enums)
{
    auto root = Metadata::CreateRoot(builder, builder.CreateVector<Offset<Metadata::ClassDecl>>(classes),
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
    std::vector<uint8_t> bytes;
    for (auto b : buf) {
        bytes.emplace_back(b);
    }
    return bytes;
}

void MetadataEmittingPhase::ProcessStatement(FlatBufferBuilder &builder, const ir::Statement &stmt,
                                             std::vector<Offset<Metadata::ClassDecl>> &classes,
                                             std::vector<Offset<Metadata::AnnotationDecl>> &annotations,
                                             std::vector<Offset<Metadata::EnumDecl>> &enums)
{
    if ((stmt.IsExported() || stmt.IsDefaultExported()) && stmt.IsTSEnumDeclaration()) {
        auto enumDecl = stmt.AsTSEnumDeclaration();
        auto enumName = builder.CreateSharedString(enumDecl->Key()->ToString());

        std::vector<Offset<flatbuffers::String>> entries;
        for (auto member : enumDecl->Members()) {
            if (member->IsTSEnumMember()) {
                auto enumMember = member->AsTSEnumMember();
                entries.emplace_back(builder.CreateSharedString(enumMember->Key()->ToString()));
            }
        }

        auto entriesVector = builder.CreateVector<Offset<flatbuffers::String>>(std::move(entries));
        enums.emplace_back(Metadata::CreateEnumDecl(builder, enumName, entriesVector));
        return;
    }

    // Handle transformed enum declarations (ClassDefinition with IsEnumTransformed)
    if ((stmt.IsExported() || stmt.IsDefaultExported()) && stmt.IsClassDeclaration() &&
        stmt.AsClassDeclaration()->Definition()->IsEnumTransformed()) {
        auto enumClass = stmt.AsClassDeclaration()->Definition();
        enums.emplace_back(BuildEnumDecl(builder, enumClass));
        return;
    }

    // Handle annotation declarations
    if ((stmt.IsExported() || stmt.IsDefaultExported()) && stmt.IsAnnotationDeclaration()) {
        auto *annotation = stmt.AsAnnotationDeclaration();
        annotations.emplace_back(BuildAnnotationDecl(builder, annotation));
        return;
    }

    const ClassDefinition *exportedClass;
    if ((stmt.IsExported() || stmt.IsDefaultExported()) && stmt.IsClassDeclaration()) {
        exportedClass = stmt.AsClassDeclaration()->Definition();
    } else {
        return;
    }

    classes.emplace_back(BuildClassDecl(builder, exportedClass));
}

bool MetadataEmittingPhase::PerformForProgram(parser::Program *program)
{
    const auto ctx = Context();
    if (!ctx->config->options->IsEmitMetadata()) {
        return true;
    }

    FlatBufferBuilder builder;
    std::vector<Offset<Metadata::ClassDecl>> classes;  // only classes recording is supported currently
    std::vector<Offset<Metadata::AnnotationDecl>> annotations;
    std::vector<Offset<Metadata::EnumDecl>> enums;
    for (auto &stmt : program->Ast()->Statements()) {
        ProcessStatement(builder, *stmt, classes, annotations, enums);
    }
    ctx->metadata = GetMetadataBytes(builder, classes, annotations, enums);
    return true;
}

}  // namespace ark::es2panda::compiler