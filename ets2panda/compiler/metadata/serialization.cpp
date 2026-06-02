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
#include "util/apiVersion.h"
#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "schemaMetadataGenerated.h"
#include "checker/types/ets/etsTupleType.h"
#include "checker/types/ets/etsAwaitedType.h"
#include "libarkfile/metadata_helper.h"
#include "utils.h"

#include <string>

namespace ark::es2panda::compiler {

using namespace panda_file;
using namespace panda_file::helpers;
using checker::ETSObjectFlags, checker::Type, ir::MethodDefinition, ir::ClassDefinition;

#define CUR_METADATA_LOGGER_COMPONENT METADATA_SERIALIZATION

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
    std::vector<Offset<Metadata::ValueParamDecl>> fbValueParams;
    for (const auto &param : astValueParams) {
        const auto paramName = builder.CreateSharedString(std::string(param->Name()));
        const auto [typeKind, typeOff] = BuildType(builder, param->TsType());
        fbValueParams.emplace_back(Metadata::CreateValueParamDecl(builder, paramName, typeKind, typeOff,
                                                                  param->HasFlag(varbinder::VariableFlags::READONLY),
                                                                  param->HasFlag(varbinder::VariableFlags::OPTIONAL)));
    }

    return builder.CreateVector<Offset<Metadata::ValueParamDecl>>(fbValueParams);
}

Offset<Vector<Offset<Metadata::TypeParamDecl>>> MetadataSerializationPhase::BuildTypeParams(
    FlatBufferBuilder &builder, const ir::TSTypeParameterDeclaration *astTypeParams)
{
    if (!astTypeParams || astTypeParams->Params().empty()) {
        return 0;
    }

    std::vector<Offset<Metadata::TypeParamDecl>> fbTypeParams;
    for (const auto &typeParam : astTypeParams->Params()) {
        Metadata::TypeParamVariance variance = Metadata::TypeParamVariance::TypeParamVariance_INV;
        if (typeParam->Modifiers() & ir::ModifierFlags::IN) {
            variance = Metadata::TypeParamVariance::TypeParamVariance_IN;
        } else if (typeParam->Modifiers() & ir::ModifierFlags::IN) {
            variance = Metadata::TypeParamVariance::TypeParamVariance_OUT;
        }
        fbTypeParams.emplace_back(Metadata::CreateTypeParamDecl(
            builder, builder.CreateSharedString(typeParam->Name()->Name().Utf8()), variance));
    }

    return builder.CreateVector<Offset<Metadata::TypeParamDecl>>(fbTypeParams);
}

Offset<Metadata::TypeDecl> MetadataSerializationPhase::BuildTypeDecl(FlatBufferBuilder &builder,
                                                                     const ir::TSTypeAliasDeclaration *astDecl)
{
    const auto typeName = builder.CreateSharedString(astDecl->Id()->Name().Utf8());
    const auto &[buildType, buildTypeKind] = BuildType(builder, astDecl->TypeAnnotation()->TsType());

    LOG_METADATA("type " << astDecl->Id()->Name().Utf8() << " = " << astDecl->TypeAnnotation()->TsType()->ToString());

    return Metadata::CreateTypeDecl(builder, typeName, buildType, buildTypeKind);
}

template <typename T>
Offset<Metadata::Decls> MetadataSerializationPhase::BuildDecls(FlatBufferBuilder &builder,
                                                               const ArenaVector<T> &astDecls)
{
    MetadataDecls decls;
    for (const auto stmt : astDecls) {
        ProcessStatement(builder, *stmt, decls);
    }
    if (!decls.IsFilled()) {
        return 0;
    }
    return Metadata::CreateDecls(
        builder, decls.imports.empty() ? 0 : builder.CreateVector<Offset<Metadata::ImportDecl>>(decls.imports),
        decls.classes.empty() ? 0 : builder.CreateVector<Offset<Metadata::ClassDecl>>(decls.classes),
        decls.interfaces.empty() ? 0 : builder.CreateVector<Offset<Metadata::InterfaceDecl>>(decls.interfaces),
        decls.annotations.empty() ? 0 : builder.CreateVector<Offset<Metadata::AnnotationDecl>>(decls.annotations),
        decls.types.empty() ? 0 : builder.CreateVector<Offset<Metadata::TypeDecl>>(decls.types));
}

Offset<Metadata::ClassDecl> MetadataSerializationPhase::BuildClassDecl(FlatBufferBuilder &builder,
                                                                       const ClassDefinition *astDecl)
{
    const auto className = builder.CreateSharedString(astDecl->Ident()->ToString());
    const auto typeParams = BuildTypeParams(builder, astDecl->TypeParams());

    LOG_METADATA(GetDeclKindToLog(astDecl)
                 << astDecl->Ident()->Name()
                 << (astDecl->TypeParams() ? "<" + IrDeclToString(astDecl->TypeParams()) + ">" : ""));

    LOG_METADATA_NESTING_INC();
    const auto isFromNamespaceOrTopLevel = astDecl->IsNamespaceTransformed() || astDecl->IsGlobal();
    const auto methods = BuildMethodDecls(builder, astDecl->Body(), isFromNamespaceOrTopLevel);
    const auto properties = BuildPropertyDecls(builder, astDecl->Body(), isFromNamespaceOrTopLevel);
    const auto decls = BuildDecls(builder, astDecl->Body());
    LOG_METADATA_NESTING_DEC();

    return Metadata::CreateClassDecl(builder, className, astDecl->IsNamespaceTransformed(),
                                     astDecl->IsEnumTransformed(), methods, properties, decls, typeParams);
}

std::pair<std::vector<Offset<>>, std::vector<uint8_t>> MetadataSerializationPhase::BuildExtends(
    FlatBufferBuilder &builder, const ArenaVector<ir::TSInterfaceHeritage *> &extends)
{
    std::vector<Offset<>> extendTypes;
    std::vector<uint8_t> extendTypeKinds;
    for (const auto &ext : extends) {
        const auto [componentTypeKind, componentTypeOff] = BuildType(builder, ext->Expr()->TsType());
        extendTypes.emplace_back(componentTypeOff);
        extendTypeKinds.emplace_back(componentTypeKind);
    }
    return {extendTypes, extendTypeKinds};
}

Offset<Metadata::InterfaceDecl> MetadataSerializationPhase::BuildInterfaceDecl(
    FlatBufferBuilder &builder, const ir::TSInterfaceDeclaration *interfaceDecl)
{
    const auto interfaceName = builder.CreateSharedString(interfaceDecl->Id()->ToString());
    const auto typeParams = BuildTypeParams(builder, interfaceDecl->TypeParams());
    const auto [extendTypes, extendTypeKinds] = BuildExtends(builder, interfaceDecl->Extends());

    LOG_METADATA(
        "interface " << interfaceDecl->Id()->Name()
                     << (interfaceDecl->TypeParams() ? "<" + IrDeclToString(interfaceDecl->TypeParams()) + ">" : "")
                     << (!interfaceDecl->Extends().empty() ? ": " + IrDeclVectorToString(interfaceDecl->Extends())
                                                           : ""));

    LOG_METADATA_NESTING_INC();
    const auto methods = BuildMethodDecls(builder, interfaceDecl->Body()->Body());
    LOG_METADATA_NESTING_DEC();

    return Metadata::CreateInterfaceDecl(builder, interfaceName, methods, typeParams,
                                         builder.CreateVector<uint8_t>(extendTypeKinds),
                                         builder.CreateVector<Offset<>>(extendTypes));
}

Offset<Metadata::AnnotationDecl> MetadataSerializationPhase::BuildAnnotationDecl(
    FlatBufferBuilder &builder, const ir::AnnotationDeclaration *astDecl)
{
    const auto annotationName = builder.CreateSharedString(astDecl->GetBaseName()->ToString());
    return Metadata::CreateAnnotationDecl(builder, annotationName);
}

Offset<Vector<Offset<Metadata::PropertyDecl>>> MetadataSerializationPhase::BuildPropertyDecls(
    FlatBufferBuilder &builder, const ArenaVector<ir::AstNode *> &body, const bool isFromNamespaceOrTopLevel)
{
    std::vector<Offset<Metadata::PropertyDecl>> properties;

    for (const auto &elem : body) {
        if (isFromNamespaceOrTopLevel && !elem->IsExported() && !elem->IsDefaultExported()) {
            continue;
        }
        if (elem->IsClassProperty() && !elem->IsProtected() && !elem->IsPrivate()) {
            properties.emplace_back(BuildPropertyDecl(builder, elem->AsClassProperty()));
        }
    }

    return builder.CreateVector<Offset<Metadata::PropertyDecl>>(properties);
}

Offset<Vector<Offset<Metadata::FunctionDecl>>> MetadataSerializationPhase::BuildMethodDecls(
    FlatBufferBuilder &builder, const ArenaVector<ir::AstNode *> &body, const bool isFromNamespaceOrTopLevel)
{
    std::vector<Offset<Metadata::FunctionDecl>> methods;

    for (const auto &elem : body) {
        if (isFromNamespaceOrTopLevel && !elem->IsExported() && !elem->IsDefaultExported()) {
            continue;
        }
        if (elem->IsMethodDefinition() && !elem->IsProtected() && !elem->IsPrivate()) {
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
    const auto decl = type->GetDeclNode();

    ES2PANDA_ASSERT(decl->IsClassDefinition() ||
                    decl->IsTSInterfaceDeclaration());  // other decls are not supported yet

    std::string declName;
    if (decl->IsClassDefinition()) {
        declName = decl->AsClassDefinition()->InternalName().Utf8();
    } else if (decl->IsTSInterfaceDeclaration()) {
        declName = decl->AsTSInterfaceDeclaration()->InternalName().Utf8();
    } else {
        return 0;
    }

    std::vector<uint8_t> typeArgKinds;
    std::vector<Offset<>> typeArgs;
    for (auto const &typeArg : type->TypeArguments()) {
        const auto [componentTypeKind, componentTypeOff] = BuildType(builder, typeArg);
        typeArgs.emplace_back(componentTypeOff);
        typeArgKinds.emplace_back(componentTypeKind);
    }

    return Metadata::CreateTypeRef(builder, builder.CreateSharedString(declName), builder.CreateVector(typeArgKinds),
                                   builder.CreateVector(typeArgs))
        .Union();
}

Offset<> MetadataSerializationPhase::BuildArrayType(FlatBufferBuilder &builder, const checker::ETSArrayType *type)
{
    const auto [componentTypeKind, componentTypeOff] = BuildType(builder, type->ElementType());
    return Metadata::CreateArrayType(builder, componentTypeKind, componentTypeOff,
                                     type->HasTypeFlag(checker::TypeFlag::READONLY), type->IsValueArray())
        .Union();
}

Offset<> MetadataSerializationPhase::BuildFunctionType(FlatBufferBuilder &builder, const checker::ETSFunctionType *type)
{
    const auto signature = type->ArrowSignature();
    std::vector<Offset<Metadata::FunctionTypeParam>> params;
    for (auto const &param : signature->Params()) {
        const auto [paramTypeKind, paramTypeOff] = BuildType(builder, param->TsType());
        const auto paramName = builder.CreateSharedString(param->Name().Utf8());
        params.emplace_back(Metadata::CreateFunctionTypeParam(builder, paramName, paramTypeKind, paramTypeOff));
    }
    const auto [returnTypeKind, returnTypeOff] = BuildType(builder, signature->ReturnType());
    return Metadata::CreateFunctionType(builder, builder.CreateVector(params), returnTypeKind, returnTypeOff).Union();
}

Offset<> MetadataSerializationPhase::BuildTupleType(FlatBufferBuilder &builder, const checker::ETSTupleType *type)
{
    std::vector<uint8_t> typeKinds;
    std::vector<Offset<>> types;
    for (auto const &componentType : type->GetTupleTypesList()) {
        const auto [componentTypeKind, componentTypeOff] = BuildType(builder, componentType);
        types.emplace_back(componentTypeOff);
        typeKinds.emplace_back(componentTypeKind);
    }
    return Metadata::CreateTupleType(builder, builder.CreateVector(typeKinds), builder.CreateVector(types)).Union();
}

Offset<> MetadataSerializationPhase::BuildTypeParameterType(FlatBufferBuilder &builder,
                                                            const checker::ETSTypeParameter *type)
{
    return Metadata::CreateTypeRef(builder, builder.CreateSharedString(std::string(type->Name()))).Union();
}

Offset<> MetadataSerializationPhase::BuildTypeAliasType(FlatBufferBuilder &builder,
                                                        const checker::ETSTypeAliasType *type)
{
    const auto typeName = type->GetDeclNode()->AsTSTypeAliasDeclaration()->Id()->Name().Utf8();
    return Metadata::CreateTypeRef(builder, builder.CreateSharedString(typeName)).Union();
}

std::pair<Metadata::Type, Offset<>> MetadataSerializationPhase::BuildType(FlatBufferBuilder &builder, const Type *type)
{
    if (type->IsETSObjectType()) {
        const auto etsObjType = type->AsETSObjectType();
        if (etsObjType->IsETSStringLiteralType()) {
            return {Metadata::Type::Type_StringLiteral, BuildStringLiteralType(builder, type->AsETSStringType())};
        }
        return {Metadata::Type::Type_Ref, BuildRefType(builder, etsObjType)};
    }

    if (type->IsETSTypeParameter()) {
        return {Metadata::Type::Type_Ref, BuildTypeParameterType(builder, type->AsETSTypeParameter())};
    }

    if (type->IsETSUnionType()) {
        return {Metadata::Type::Type_Union, BuildUnionType(builder, type->AsETSUnionType())};
    }

    if (type->IsETSArrayType()) {
        return {Metadata::Type::Type_Array, BuildArrayType(builder, type->AsETSArrayType())};
    }

    if (type->IsETSFunctionType()) {
        return {Metadata::Type::Type_Function, BuildFunctionType(builder, type->AsETSFunctionType())};
    }

    if (type->IsETSTupleType()) {
        return {Metadata::Type::Type_Tuple, BuildTupleType(builder, type->AsETSTupleType())};
    }

    if (type->IsETSTypeAliasType()) {
        return {Metadata::Type::Type_Ref, BuildTypeAliasType(builder, type->AsETSTypeAliasType())};
    }

    if (type->IsETSAwaitedType()) {
        return {Metadata::Type::Type_Ref, BuildTypeParameterType(builder, type->AsETSAwaitedType()->GetUnderlying())};
    }

    const auto builtinTypeKind = GetBuiltinTypeKind(type);

    ES2PANDA_ASSERT(builtinTypeKind != NOT_BUILTIN_TYPE_KIND);

    return {Metadata::Type::Type_Builtin, Metadata::CreateBuiltinType(builder, builtinTypeKind).Union()};
}

Offset<Metadata::FunctionDecl> MetadataSerializationPhase::BuildFunctionDecl(FlatBufferBuilder &builder,
                                                                             const ir::ScriptFunction *func)
{
    const auto methodName = builder.CreateSharedString(func->Id()->ToString());
    const auto valueParams = BuildValueParams(builder, func->Signature()->Params());
    const auto typeParams = BuildTypeParams(builder, func->TypeParams());
    const auto isVoidReturnType =
        (func->ReturnTypeAnnotation() && func->ReturnTypeAnnotation()->IsETSPrimitiveType() &&
         func->ReturnTypeAnnotation()->AsETSPrimitiveType()->GetPrimitiveType() == ir::PrimitiveType::VOID) ||
        func->IsConstructor();

    // Temporary fix for the void return type because at the current stage, undefined type set instead as a return type
    const auto [returnTypeKind, returnTypeOff] =
        isVoidReturnType ? std::make_pair(Metadata::Type::Type_Builtin,
                                          Metadata::CreateBuiltinType(builder, Metadata::BuiltinTypeKind_void_).Union())
                         : BuildType(builder, func->Signature()->ReturnType());

    LOG_METADATA(func->Id()->ToString() << func->Signature()->ToString());

    return Metadata::CreateFunctionDecl(builder, methodName, returnTypeKind, returnTypeOff, func->IsStatic(),
                                        valueParams, typeParams);
}

Offset<Metadata::PropertyDecl> MetadataSerializationPhase::BuildPropertyDecl(FlatBufferBuilder &builder,
                                                                             const ir::ClassProperty *var)
{
    const auto returnType = var->TsType();
    const auto varName = builder.CreateSharedString(var->Id()->ToString());
    const auto [returnTypeKind, returnTypeOff] = BuildType(builder, returnType);

    LOG_METADATA(var->Id()->ToString() << ": " << returnType->ToString());

    return Metadata::CreatePropertyDecl(builder, varName, returnTypeKind, returnTypeOff, var->IsStatic());
}

Offset<Metadata::ImportDecl> MetadataSerializationPhase::BuildImportDecl(FlatBufferBuilder &builder,
                                                                         const ir::ImportDeclaration *importDecl) const
{
    const auto from = importDecl->Source()->Str().Utf8();

    std::vector<Offset<flatbuffers::String>> specifiers;
    for (const auto &specifier : importDecl->Specifiers()) {
        if (!specifier->IsImportSpecifier()) {
            continue;
        }
        specifiers.emplace_back(builder.CreateSharedString(specifier->AsImportSpecifier()->Imported()->Name().Utf8()));
    }

    LOG_METADATA(IrDeclToString(importDecl));

    return Metadata::CreateImportDecl(builder, builder.CreateSharedString(from), builder.CreateVector(specifiers));
}

void MetadataSerializationPhase::ProcessStatement(FlatBufferBuilder &builder, const ir::AstNode &node,
                                                  MetadataDecls &decls)
{
    if (node.IsETSImportDeclaration()) {
        decls.imports.emplace_back(BuildImportDecl(builder, node.AsETSImportDeclaration()));
    }

    const auto isExported = node.IsExported() || node.IsDefaultExported() ||
                            (node.IsClassDeclaration() && node.AsClassDeclaration()->Definition()->IsGlobal());
    if (!isExported) {
        return;
    }

    if (node.IsClassDeclaration()) {
        decls.classes.emplace_back(BuildClassDecl(builder, node.AsClassDeclaration()->Definition()));
    } else if (node.IsTSInterfaceDeclaration()) {
        decls.interfaces.emplace_back(BuildInterfaceDecl(builder, node.AsTSInterfaceDeclaration()));
    } else if (node.IsTSTypeAliasDeclaration()) {
        decls.types.emplace_back(BuildTypeDecl(builder, node.AsTSTypeAliasDeclaration()));
    } else if (node.IsAnnotationDeclaration()) {
        decls.annotations.emplace_back(BuildAnnotationDecl(builder, node.AsAnnotationDeclaration()));
    }
}

std::vector<uint8_t> MetadataSerializationPhase::GetMetadataBytes(FlatBufferBuilder &builder,
                                                                  const Offset<Metadata::Decls> &fbDecls)
{
    if (fbDecls.IsNull()) {
        return {};
    }
    builder.Finish(fbDecls);
    const auto buf = builder.GetBufferSpan();
    return {buf.begin(), buf.end()};
}

bool MetadataSerializationPhase::PerformForProgram(parser::Program *program)
{
    const auto ctx = Context();
    if (!ctx->config->options->IsEmitMetadata()) {
        return true;
    }

    LOG_METADATA_ENABLE();

    const auto pkgName = std::string(program->ModuleName());
    const auto moduleName = std::string(program->SourceFile().GetFileName().Utf8());

    LOG_METADATA("serializing metadata of program " << pkgName << ":" << moduleName);

    FlatBufferBuilder builder;

    LOG_METADATA_NESTING_INC();
    const auto decls = BuildDecls(builder, program->Ast()->Statements());
    LOG_METADATA_NESTING_DEC();

    ctx->metadata[MetadataModuleId(pkgName, moduleName)] = GetMetadataBytes(builder, decls);

    LOG_METADATA_DISABLE();

    return true;
}

}  // namespace ark::es2panda::compiler