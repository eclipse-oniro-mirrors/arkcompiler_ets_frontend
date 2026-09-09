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
#include "checker/ETSchecker.h"
#include "util/apiVersion.h"
#include "ir/base/classDefinition.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/ets/etsTypeReference.h"
#include "ir/ets/etsTypeReferencePart.h"
#include "ir/expressions/arrayExpression.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/module/exportNamedDeclaration.h"
#include "ir/module/exportSpecifier.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "schemaMetadataGenerated.h"
#include "checker/types/ets/etsTupleType.h"
#include "checker/types/ets/etsAwaitedType.h"
#include "libarkfile/metadata_helper.h"
#include "generated/signatures.h"
#include "util/es2pandaMacros.h"
#include "varbinder/variable.h"
#include "utils.h"

namespace ark::es2panda::compiler {

using namespace panda_file;
using namespace panda_file::helpers;
using checker::ETSObjectFlags, checker::Type, ir::MethodDefinition, ir::ClassDefinition;

#define CUR_METADATA_LOGGER_COMPONENT METADATA_SERIALIZATION

constexpr auto NOT_BUILTIN_TYPE_KIND = static_cast<Metadata::BuiltinTypeKind>(-1);

static bool IsPartialClassName(std::string_view name)
{
    return name.rfind(checker::PARTIAL_CLASS_PREFIX, 0) == 0;
}

static bool EndsWith(std::string_view value, std::string_view suffix)
{
    return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static const ir::AnnotationDeclaration *ResolveAnnotationDeclaration(public_lib::Context *ctx,
                                                                     const ir::AnnotationUsage *anno)
{
    auto *checker = ctx != nullptr ? ctx->GetChecker()->AsETSChecker() : nullptr;
    return checker != nullptr ? checker->MaterializeAnnotationUsage(const_cast<ir::AnnotationUsage *>(anno),
                                                                    checker::AnnotationUseKind::META)
                              : nullptr;
}

static std::vector<std::string> CollectRestrictionModules(const ir::AnnotationUsage *anno)
{
    std::vector<std::string> modules;
    if (anno == nullptr) {
        return modules;
    }

    for (auto *propNode : anno->Properties()) {
        auto *prop = propNode != nullptr ? propNode->AsClassProperty() : nullptr;
        if (prop == nullptr || prop->Id() == nullptr || prop->Value() == nullptr) {
            continue;
        }
        if (!prop->Id()->Name().Is("modules") && prop->Id()->Name() != compiler::Signatures::ANNOTATION_KEY_VALUE) {
            continue;
        }
        if (!prop->Value()->IsArrayExpression()) {
            continue;
        }
        auto *arrayExpr = prop->Value()->AsArrayExpression();
        for (auto *element : arrayExpr->Elements()) {
            if (element != nullptr && element->IsStringLiteral()) {
                modules.emplace_back(element->AsStringLiteral()->Str().Utf8());
            }
        }
    }
    return modules;
}

static std::vector<std::string> CollectEffectiveRestrictionModules(public_lib::Context *ctx,
                                                                   const ir::AnnotationDeclaration *decl,
                                                                   std::unordered_set<std::string> *visiting = nullptr)
{
    std::vector<std::string> modules;
    if (decl == nullptr) {
        return modules;
    }

    std::unordered_set<std::string> localVisiting;
    if (visiting == nullptr) {
        visiting = &localVisiting;
    }

    const auto internalName = std::string(decl->InternalName().Utf8());
    if (!visiting->insert(internalName).second) {
        return modules;
    }

    for (auto *anno : decl->Annotations()) {
        auto *annoDecl = ResolveAnnotationDeclaration(ctx, anno);
        if (annoDecl == nullptr) {
            continue;
        }

        if (annoDecl->InternalName().Is("arkruntime.annotation.AccessRestriction")) {
            auto direct = CollectRestrictionModules(anno);
            modules.insert(modules.end(), direct.begin(), direct.end());
            continue;
        }

        auto nested = CollectEffectiveRestrictionModules(ctx, annoDecl, visiting);
        modules.insert(modules.end(), nested.begin(), nested.end());
    }

    visiting->erase(internalName);
    std::sort(modules.begin(), modules.end());
    modules.erase(std::unique(modules.begin(), modules.end()), modules.end());
    return modules;
}

static bool HasRestrictionMetadata(public_lib::Context *ctx, const ir::AnnotationDeclaration *decl,
                                   std::unordered_set<std::string> *visiting = nullptr)
{
    if (decl == nullptr) {
        return false;
    }

    std::unordered_set<std::string> localVisiting;
    if (visiting == nullptr) {
        visiting = &localVisiting;
    }

    const auto internalName = std::string(decl->InternalName().Utf8());
    if (!visiting->insert(internalName).second) {
        return false;
    }

    for (auto *anno : decl->Annotations()) {
        auto *annoDecl = ResolveAnnotationDeclaration(ctx, anno);
        if (annoDecl == nullptr) {
            continue;
        }
        if (annoDecl->InternalName().Is("arkruntime.annotation.AccessRestriction") ||
            HasRestrictionMetadata(ctx, annoDecl, visiting)) {
            visiting->erase(internalName);
            return true;
        }
    }

    visiting->erase(internalName);
    return false;
}

static bool IsBuiltinErrorAliasDecl(const ir::ClassDefinition *classDef)
{
    if (classDef == nullptr || classDef->Program() == nullptr || classDef->Super() == nullptr ||
        classDef->Ident() == nullptr) {
        return false;
    }

    if (!util::Helpers::IsStdLib(classDef->Program()) ||
        !EndsWith(classDef->Program()->SourceFilePath().Utf8(), "std/core/Errors.ets")) {
        return false;
    }

    const auto *superType = classDef->Super()->TsType();
    if (superType == nullptr || !superType->IsETSObjectType() || superType->AsETSObjectType()->Name() != "Error") {
        return false;
    }

    const auto className = classDef->Ident()->Name();
    return std::any_of(classDef->Body().begin(), classDef->Body().end(), [className](const ir::AstNode *member) {
        if (!member->IsMethodDefinition()) {
            return false;
        }

        const auto *method = member->AsMethodDefinition();
        return method->IsStatic() && method->Id() != nullptr &&
               method->Id()->Name() == compiler::Signatures::STATIC_INVOKE_METHOD && method->Function() != nullptr &&
               method->Function()->ReturnTypeAnnotation() != nullptr &&
               method->Function()->ReturnTypeAnnotation()->TsType() != nullptr &&
               method->Function()->ReturnTypeAnnotation()->TsType()->IsETSObjectType() &&
               method->Function()->ReturnTypeAnnotation()->TsType()->AsETSObjectType()->Name() == className;
    });
}

static const checker::Type *GetMetadataPropertyType(const ir::ClassProperty *property)
{
    const auto *propertyType = property->TsType();
    if (propertyType == nullptr || !propertyType->HasTypeFlag(checker::TypeFlag::GETTER_SETTER) ||
        !propertyType->IsETSFunctionType()) {
        return propertyType;
    }

    const auto *accessorType = propertyType->AsETSFunctionType();
    if (const auto *getter = accessorType->FindGetter(); getter != nullptr) {
        return getter->ReturnType();
    }

    const auto *setter = accessorType->FindSetter();
    if (setter != nullptr && !setter->Params().empty()) {
        return setter->Params()[0]->TsType();
    }

    return propertyType;
}

static const checker::Type *GetSerializablePropertyType(public_lib::Context *ctx, const ir::ClassProperty *property)
{
    const auto *propertyType = GetMetadataPropertyType(property);
    const auto *original = property->OriginalNode() != nullptr && property->OriginalNode()->IsClassProperty()
                               ? property->OriginalNode()->AsClassProperty()
                               : property;
    if (propertyType == nullptr || !original->IsOptionalDeclaration()) {
        return propertyType;
    }

    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *nonOptionalType = checker->RemoveUndefinedType(const_cast<checker::Type *>(propertyType));
    if (nonOptionalType == propertyType) {
        return checker->CreateETSUnionType({checker->GlobalETSUndefinedType(), nonOptionalType});
    }

    return propertyType;
}

static const ir::ClassProperty *OriginalClassProperty(const ir::ClassProperty *property)
{
    ES2PANDA_ASSERT(property != nullptr);
    const auto *original = property->OriginalNode();
    if (original == nullptr) {
        return property;
    }
    ES2PANDA_ASSERT(original->IsClassProperty());
    return original->AsClassProperty();
}

static const varbinder::Variable *ResolveImportAlias(const varbinder::Variable *variable)
{
    ES2PANDA_ASSERT(variable != nullptr);
    if (!variable->IsLocalVariable()) {
        return variable;
    }

    const auto *bindingInfo = variable->AsLocalVariable()->ImportBinding();
    if (bindingInfo == nullptr) {
        return variable;
    }
    ES2PANDA_ASSERT(bindingInfo->resolvedVariable != nullptr);
    return bindingInfo->resolvedVariable;
}

static std::optional<util::StringView> SimpleAliasTypeReferenceName(const ir::TypeNode *typeAnnotation)
{
    ES2PANDA_ASSERT(typeAnnotation != nullptr);
    if (!typeAnnotation->IsETSTypeReference()) {
        return std::nullopt;
    }

    const auto *part = typeAnnotation->AsETSTypeReference()->Part();
    ES2PANDA_ASSERT(part != nullptr);
    if (part->Previous() != nullptr || part->TypeParams() != nullptr) {
        return std::nullopt;
    }

    const auto *name = part->Name();
    ES2PANDA_ASSERT(name != nullptr);
    if (!name->IsIdentifier()) {
        return std::nullopt;
    }

    const auto *ident = name->AsIdentifier();
    if (ident->Variable() == nullptr) {
        return std::nullopt;
    }
    const auto *variable = ResolveImportAlias(ident->Variable());
    ES2PANDA_ASSERT(variable->Declaration() != nullptr);
    ES2PANDA_ASSERT(variable->Declaration()->Node() != nullptr);
    if (!variable->Declaration()->Node()->IsTSTypeAliasDeclaration()) {
        return std::nullopt;
    }
    return ident->Name();
}

std::pair<Metadata::Type, Offset<>> MetadataSerializationPhase::BuildPropertyType(FlatBufferBuilder &builder,
                                                                                  const ir::ClassProperty *property)
{
    const auto *original = OriginalClassProperty(property);
    for (const auto *annotation : {property->TypeAnnotation(), original->TypeAnnotation()}) {
        if (annotation == nullptr) {
            continue;
        }
        if (const auto aliasName = SimpleAliasTypeReferenceName(annotation); aliasName.has_value()) {
            return {Metadata::Type::Type_Ref,
                    Metadata::CreateTypeRef(builder, builder.CreateSharedString(aliasName->Utf8())).Union()};
        }
    }
    return BuildType(builder, GetSerializablePropertyType(Context(), property));
}

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

// CC-OFFNXT(G.FUN.01, huge_method) solid logic
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
    if (etsType->HasTypeFlag(checker::TypeFlag::ETS_BOOLEAN)) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_boolean;
    }
    if (etsType->HasTypeFlag(checker::TypeFlag::BYTE)) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_byte_;
    }
    if (etsType->HasTypeFlag(checker::TypeFlag::SHORT)) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_short_;
    }
    if (etsType->HasTypeFlag(checker::TypeFlag::INT)) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_int_;
    }
    if (etsType->HasTypeFlag(checker::TypeFlag::LONG)) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_long_;
    }
    if (etsType->HasTypeFlag(checker::TypeFlag::FLOAT)) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_float_;
    }
    if (etsType->HasTypeFlag(checker::TypeFlag::DOUBLE)) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_double_;
    }
    if (etsType->HasTypeFlag(checker::TypeFlag::CHAR)) {
        return Metadata::BuiltinTypeKind::BuiltinTypeKind_char_;
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
    FlatBufferBuilder &builder, const checker::Signature *signature)
{
    std::vector<Offset<Metadata::ValueParamDecl>> fbValueParams;
    for (const auto &param : signature->Params()) {
        const auto *declNode = param->Declaration() != nullptr ? param->Declaration()->Node() : nullptr;
        const auto *originalParamNode = declNode != nullptr ? declNode->OriginalNode() : nullptr;
        const auto actualParamName = originalParamNode != nullptr && originalParamNode->IsETSParameterExpression()
                                         ? originalParamNode->AsETSParameterExpression()->Name()
                                         : param->Name();
        const auto paramName = builder.CreateSharedString(std::string(actualParamName));
        const auto isOptional = (declNode != nullptr && declNode->IsETSParameterExpression() &&
                                 declNode->AsETSParameterExpression()->IsOptional()) ||
                                param->HasFlag(varbinder::VariableFlags::OPTIONAL);
        auto *serializedType = isOptional
                                   ? Context()->GetChecker()->AsETSChecker()->RemoveUndefinedType(param->TsType())
                                   : param->TsType();
        const auto *paramTypeAnnotation = declNode != nullptr && declNode->IsETSParameterExpression()
                                              ? declNode->AsETSParameterExpression()->TypeAnnotation()
                                              : nullptr;
        if (paramTypeAnnotation != nullptr && paramTypeAnnotation->IsETSPrimitiveType()) {
            serializedType = Context()->GetChecker()->AsETSChecker()->MaybeUnboxType(serializedType);
        }
        const auto [typeKind, typeOff] = BuildType(builder, serializedType);
        fbValueParams.emplace_back(Metadata::CreateValueParamDecl(builder, paramName, typeKind, typeOff,
                                                                  param->HasFlag(varbinder::VariableFlags::READONLY),
                                                                  isOptional, false));
    }

    if (signature->RestVar() != nullptr) {
        const auto *restVar = signature->RestVar();
        const auto paramName = builder.CreateSharedString(std::string(restVar->Name()));
        const auto [typeKind, typeOff] = BuildType(builder, restVar->TsType());
        fbValueParams.emplace_back(Metadata::CreateValueParamDecl(
            builder, paramName, typeKind, typeOff, restVar->HasFlag(varbinder::VariableFlags::READONLY), false, true));
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
        } else if (typeParam->Modifiers() & ir::ModifierFlags::OUT) {
            variance = Metadata::TypeParamVariance::TypeParamVariance_OUT;
        }
        const auto [constraintKind, constraintOff] = typeParam->Constraint() != nullptr
                                                         ? BuildType(builder, typeParam->Constraint()->TsType())
                                                         : std::make_pair(Metadata::Type::Type_NONE, Offset<> {});
        const auto [defaultTypeKind, defaultTypeOff] = typeParam->DefaultType() != nullptr
                                                           ? BuildType(builder, typeParam->DefaultType()->TsType())
                                                           : std::make_pair(Metadata::Type::Type_NONE, Offset<> {});
        fbTypeParams.emplace_back(
            Metadata::CreateTypeParamDecl(builder, builder.CreateSharedString(typeParam->Name()->Name().Utf8()),
                                          variance, constraintKind, constraintOff, defaultTypeKind, defaultTypeOff));
    }

    return builder.CreateVector<Offset<Metadata::TypeParamDecl>>(fbTypeParams);
}

Offset<Metadata::TypeDecl> MetadataSerializationPhase::BuildTypeDecl(FlatBufferBuilder &builder,
                                                                     const ir::TSTypeAliasDeclaration *astDecl)
{
    const auto typeName = builder.CreateSharedString(astDecl->Id()->Name().Utf8());
    const auto *typeParamsNode =
        astDecl->TypeParams() != nullptr
            ? astDecl->TypeParams()
            : (astDecl->OriginalNode() != nullptr && astDecl->OriginalNode()->IsTSTypeAliasDeclaration()
                   ? astDecl->OriginalNode()->AsTSTypeAliasDeclaration()->TypeParams()
                   : nullptr);
    const auto fbTypeParams = BuildTypeParams(builder, typeParamsNode);
    const auto &[buildType, buildTypeKind] = BuildType(builder, astDecl->TypeAnnotation()->TsType());

    return Metadata::CreateTypeDecl(builder, typeName, buildType, buildTypeKind, fbTypeParams);
}

static bool HasDirectLocalExport(const varbinder::ExportFactStore::ExportFactSnapshot &facts,
                                 util::StringView localName)
{
    for (const auto &fact : facts.locals) {
        if (!fact.isLocalAlias && !fact.isInvalid && (fact.exportedName == localName || fact.localName == localName)) {
            return true;
        }
    }
    return false;
}

static varbinder::Variable *FindLocalExportAliasVariable(const varbinder::ExportFactStore::ExportFactSnapshot &facts,
                                                         const varbinder::PendingLocalExportAlias &alias)
{
    for (const auto &fact : facts.locals) {
        if (fact.isLocalAlias && !fact.isInvalid && fact.exportedName == alias.exportedName &&
            fact.localName == alias.localName) {
            return fact.variable;
        }
    }
    return nullptr;
}

static const varbinder::ImportBindingInfo *GetNamedImportBinding(varbinder::Variable *variable)
{
    if (variable == nullptr || !variable->IsLocalVariable()) {
        return nullptr;
    }

    const auto *localVariable = variable->AsLocalVariable();
    const auto *bindingInfo = localVariable->ImportBinding();
    if (bindingInfo == nullptr || bindingInfo->kind != varbinder::ImportBindingKind::NAMED) {
        return nullptr;
    }
    return bindingInfo;
}

static bool IsSerializedAliasTarget(const parser::Program *program, const varbinder::PendingLocalExportAlias &alias)
{
    if (alias.origin != nullptr && alias.origin->HasExportAlias()) {
        return true;
    }
    const auto *globalClass = program->GlobalClass();
    if (globalClass == nullptr) {
        return false;
    }
    for (const auto *member : globalClass->Body()) {
        const ir::Identifier *id = nullptr;
        if (member->IsClassProperty()) {
            id = member->AsClassProperty()->Id();
        } else if (member->IsMethodDefinition()) {
            id = member->AsMethodDefinition()->Id();
        }
        if (id != nullptr && id->Name() == alias.localName &&
            (member->IsExported() || member->IsDefaultExported() || member->HasExportAlias())) {
            return true;
        }
    }
    return false;
}

template <typename T>
Offset<Metadata::Decls> MetadataSerializationPhase::BuildDecls(FlatBufferBuilder &builder,
                                                               const ArenaVector<T> &astDecls,
                                                               const std::string &pkgName, parser::Program *program)
{
    MetadataDecls decls;
    for (const auto stmt : astDecls) {
        ProcessStatement(builder, *stmt, decls);
    }

    if (program != nullptr) {
        AddExportDeclarations(builder, program, decls);
    }

    return BuildMetadataDecls(builder, pkgName, decls);
}

void MetadataSerializationPhase::AddExportDeclarations(FlatBufferBuilder &builder, parser::Program *program,
                                                       MetadataDecls &decls)
{
    auto *const varBinder = program->VarBinder();
    ES2PANDA_ASSERT(varBinder != nullptr);
    ES2PANDA_ASSERT(varBinder->IsETSBinder());
    const auto *const etsBinder = varBinder->AsETSBinder();
    ES2PANDA_ASSERT(etsBinder != nullptr);

    const auto &facts = etsBinder->GetExportFacts(program);

    for (const auto &fact : facts.locals) {
        if (fact.isLocalAlias || fact.isInvalid || !varbinder::IsDefaultExportName(fact.exportedName)) {
            continue;
        }
        decls.localExports.emplace_back(BuildLocalExportDecl(builder, fact));
    }

    for (const auto &alias : etsBinder->PendingLocalExportAliases(program)) {
        if (alias.kind != varbinder::LocalExportKind::ALIAS || alias.isInvalid) {
            continue;
        }

        if (const auto *bindingInfo = GetNamedImportBinding(FindLocalExportAliasVariable(facts, alias));
            bindingInfo != nullptr) {
            decls.reExports.emplace_back(
                BuildImportBindingReExportDecl(builder, bindingInfo, alias.exportedName, alias.isExplicitTypeOnly));
            continue;
        }

        if (!HasDirectLocalExport(facts, alias.localName) && !IsSerializedAliasTarget(program, alias)) {
            LOG(ERROR, ES2PANDA) << "Metadata local export alias '" << alias.exportedName << "' refers to local '"
                                 << alias.localName << "' in module '" << program->ModuleName()
                                 << "', but the local is neither directly exported nor export-aliased";
            ES2PANDA_UNREACHABLE();
        }
        decls.localExports.emplace_back(BuildLocalExportDecl(builder, alias));
    }
}

Offset<Metadata::Decls> MetadataSerializationPhase::BuildMetadataDecls(FlatBufferBuilder &builder,
                                                                       const std::string &pkgName,
                                                                       const MetadataDecls &decls)
{
    if (!decls.IsFilled()) {
        return 0;
    }

    return Metadata::CreateDecls(
        builder, decls.imports.empty() ? 0 : builder.CreateVector<Offset<Metadata::ImportDecl>>(decls.imports),
        decls.reExports.empty() ? 0 : builder.CreateVector<Offset<Metadata::ReExportDecl>>(decls.reExports),
        decls.localExports.empty() ? 0 : builder.CreateVector<Offset<Metadata::LocalExportDecl>>(decls.localExports),
        decls.classes.empty() ? 0 : builder.CreateVector<Offset<Metadata::ClassDecl>>(decls.classes),
        decls.interfaces.empty() ? 0 : builder.CreateVector<Offset<Metadata::InterfaceDecl>>(decls.interfaces),
        decls.annotations.empty() ? 0 : builder.CreateVector<Offset<Metadata::AnnotationDecl>>(decls.annotations),
        decls.types.empty() ? 0 : builder.CreateVector<Offset<Metadata::TypeDecl>>(decls.types),
        builder.CreateSharedString(pkgName));
}

std::vector<Offset<flatbuffers::String>> MetadataSerializationPhase::ExtractEnumValues(
    FlatBufferBuilder &builder, const ir::ClassDefinition *astDecl) const
{
    const bool isString = astDecl->IsStringEnumTransformed();
    const auto valuesArrayName = isString ? "#StringValuesArray" : "#ValuesArray";

    std::vector<Offset<flatbuffers::String>> values;
    for (const auto *elem : astDecl->Body()) {
        if (!elem->IsClassProperty()) {
            continue;
        }
        const auto *prop = elem->AsClassProperty();
        if (prop->Id()->Name() != valuesArrayName || !prop->Value()->IsArrayExpression()) {
            continue;
        }
        for (const auto *val : prop->Value()->AsArrayExpression()->Elements()) {
            if (val->IsStringLiteral()) {
                values.emplace_back(builder.CreateSharedString(val->AsStringLiteral()->Str().Utf8()));
            } else if (val->IsNumberLiteral()) {
                values.emplace_back(builder.CreateSharedString(val->AsNumberLiteral()->Number().Str().Utf8()));
            }
        }
        break;
    }

    return values;
}

std::pair<Metadata::Type, Offset<>> MetadataSerializationPhase::ExtractEnumUnderlyingType(
    FlatBufferBuilder &builder, const ir::ClassDefinition *astDecl)
{
    const auto *origEnumDecl = astDecl->OrigEnumDecl();
    if (origEnumDecl != nullptr && origEnumDecl->AsTSEnumDeclaration()->TypeAnnotation() != nullptr) {
        return BuildType(builder, origEnumDecl->AsTSEnumDeclaration()->TypeAnnotation()->TsType());
    }
    return {Metadata::Type::Type_NONE, Offset<> {}};
}

MetadataSerializationPhase::EnumInfo MetadataSerializationPhase::BuildEnumInfo(FlatBufferBuilder &builder,
                                                                               const ir::ClassDefinition *astDecl)
{
    if (!astDecl->IsEnumTransformed()) {
        return {Metadata::EnumKind_NONE, {}, Metadata::Type::Type_NONE, Offset<> {}};
    }

    const bool isString = astDecl->IsStringEnumTransformed();
    const auto kind = isString ? Metadata::EnumKind_STRING : Metadata::EnumKind_NUMERIC;
    auto values = ExtractEnumValues(builder, astDecl);
    auto [typeKind, typeOff] = ExtractEnumUnderlyingType(builder, astDecl);

    return {kind, std::move(values), typeKind, typeOff};
}

Offset<Metadata::ClassDecl> MetadataSerializationPhase::BuildClassDecl(FlatBufferBuilder &builder,
                                                                       const ClassDefinition *astDecl)
{
    const auto className = builder.CreateSharedString(astDecl->Ident()->ToString());
    const auto typeParams = BuildTypeParams(builder, astDecl->TypeParams());
    const auto [extendedClassKind, extendedClassOff] = astDecl->Super() != nullptr
                                                           ? BuildType(builder, astDecl->Super()->TsType())
                                                           : std::make_pair(Metadata::Type::Type_NONE, Offset<> {});
    std::vector<Offset<>> implementedInterfaces;
    std::vector<uint8_t> implementedInterfaceKinds;
    for (const auto *impl : astDecl->Implements()) {
        const auto [typeKind, typeOff] = BuildType(builder, impl->Expr()->TsType());
        implementedInterfaces.emplace_back(typeOff);
        implementedInterfaceKinds.emplace_back(typeKind);
    }

    LOG_METADATA(GetDeclKindToLog(astDecl)
                 << astDecl->Ident()->Name()
                 << (astDecl->TypeParams() ? "<" + IrDeclToString(astDecl->TypeParams()) + ">" : ""));

    LOG_METADATA_NESTING_INC();
    const auto isFromNamespaceOrTopLevel = astDecl->IsNamespaceTransformed() || astDecl->IsGlobal();
    const auto methods = BuildMethodDecls(builder, astDecl->Body(), isFromNamespaceOrTopLevel);
    const auto properties = BuildPropertyDecls(builder, astDecl->Body(), isFromNamespaceOrTopLevel);
    const auto decls = BuildDecls(builder, astDecl->Body(), {});
    LOG_METADATA_NESTING_DEC();

    auto [enumKind, enumValues, enumTypeKind, enumTypeOff] = BuildEnumInfo(builder, astDecl);
    const auto isBuiltin =
        (astDecl->Variable() != nullptr && astDecl->Variable()->HasFlag(varbinder::VariableFlags::BUILTIN_TYPE)) ||
        IsBuiltinErrorAliasDecl(astDecl);

    return Metadata::CreateClassDecl(
        builder, className, astDecl->IsNamespaceTransformed(), isBuiltin, enumKind, builder.CreateVector(enumValues),
        enumTypeKind, enumTypeOff, methods, properties, decls, typeParams, extendedClassKind, extendedClassOff,
        builder.CreateVector<uint8_t>(implementedInterfaceKinds), builder.CreateVector<Offset<>>(implementedInterfaces),
        astDecl->IsFinal(), astDecl->IsAbstract());
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
    const auto properties = BuildPropertyDecls(builder, interfaceDecl->Body()->Body(), false);
    LOG_METADATA_NESTING_DEC();
    const auto isBuiltin = interfaceDecl->Variable() != nullptr &&
                           interfaceDecl->Variable()->HasFlag(varbinder::VariableFlags::BUILTIN_TYPE);

    return Metadata::CreateInterfaceDecl(builder, interfaceName, isBuiltin, methods, typeParams,
                                         builder.CreateVector<uint8_t>(extendTypeKinds),
                                         builder.CreateVector<Offset<>>(extendTypes), properties);
}

Offset<Metadata::AnnotationDecl> MetadataSerializationPhase::BuildAnnotationDecl(
    FlatBufferBuilder &builder, const ir::AnnotationDeclaration *astDecl)
{
    const auto annotationName = builder.CreateSharedString(astDecl->GetBaseName()->ToString());
    const auto internalName = builder.CreateSharedString(astDecl->InternalName().Utf8());

    std::vector<Offset<Metadata::PropertyDecl>> properties;
    for (const auto *property : astDecl->Properties()) {
        properties.emplace_back(BuildPropertyDecl(builder, property->AsClassProperty()));
    }

    std::vector<std::string> effectiveRestrictionModules = CollectEffectiveRestrictionModules(Context(), astDecl);
    std::vector<Offset<flatbuffers::String>> accessRestrictionModules;
    std::optional<Offset<flatbuffers::String>> accessRestrictionAnnotationName;
    for (auto *anno : astDecl->Annotations()) {
        auto *annoDecl = ResolveAnnotationDeclaration(Context(), anno);
        if (annoDecl == nullptr) {
            continue;
        }

        const auto restrictionModules = CollectRestrictionModules(anno);
        const auto hasRestrictionMetadata = HasRestrictionMetadata(Context(), annoDecl);
        if (restrictionModules.empty() && !hasRestrictionMetadata) {
            continue;
        }

        if (!accessRestrictionAnnotationName.has_value()) {
            accessRestrictionAnnotationName = builder.CreateSharedString(annoDecl->GetBaseName()->Name().Utf8());
        }

        for (const auto &module : restrictionModules) {
            accessRestrictionModules.emplace_back(builder.CreateSharedString(module));
        }
    }

    if (accessRestrictionModules.empty()) {
        for (const auto &module : effectiveRestrictionModules) {
            accessRestrictionModules.emplace_back(builder.CreateSharedString(module));
        }
    }

    return Metadata::CreateAnnotationDecl(
        builder, annotationName, internalName, builder.CreateVector(properties),
        accessRestrictionModules.empty() ? 0 : builder.CreateVector(accessRestrictionModules),
        accessRestrictionAnnotationName.value_or(0));
}

static bool HasLocalExportFact(const ir::ClassProperty *property)
{
    ES2PANDA_ASSERT(property != nullptr);
    ES2PANDA_ASSERT(property->Id() != nullptr);

    auto *program = const_cast<parser::Program *>(property->Program());
    if (program == nullptr || program->VarBinder() == nullptr || !program->VarBinder()->IsETSBinder()) {
        return false;
    }

    const auto propertyName = property->Id()->Name();
    for (const auto &fact : program->VarBinder()->AsETSBinder()->GetExportFacts(program).locals) {
        if (!fact.isLocalAlias && !fact.isInvalid && fact.exportedName == propertyName &&
            fact.localName == propertyName) {
            return true;
        }
    }

    return false;
}

Offset<Vector<Offset<Metadata::PropertyDecl>>> MetadataSerializationPhase::BuildPropertyDecls(
    FlatBufferBuilder &builder, const ArenaVector<ir::AstNode *> &body, const bool isFromNamespaceOrTopLevel)
{
    std::vector<Offset<Metadata::PropertyDecl>> properties;
    std::unordered_set<std::string> seenProperties;
    const auto appendProperty = [this, &builder, &properties, &seenProperties](const ir::ClassProperty *property) {
        const auto propertyName = property->Id()->Name().Utf8();
        if (!seenProperties.emplace(std::string(property->IsStatic() ? "static " : "") + std::string(propertyName))
                 .second) {
            return;
        }

        const auto varName = builder.CreateSharedString(propertyName);
        const auto [returnTypeKind, returnTypeOff] = BuildPropertyType(builder, property);
        properties.emplace_back(Metadata::CreatePropertyDecl(builder, varName, returnTypeKind, returnTypeOff,
                                                             property->IsStatic(), property->IsConst(),
                                                             property->IsProtected(), property->IsReadonly()));
    };

    for (const auto &elem : body) {
        if (isFromNamespaceOrTopLevel && !elem->IsExported() && !elem->IsDefaultExported() && !elem->HasExportAlias() &&
            (!elem->IsClassProperty() || !HasLocalExportFact(elem->AsClassProperty()))) {
            continue;
        }
        if (elem->IsClassProperty() && !elem->IsPrivate()) {
            appendProperty(elem->AsClassProperty());
        }
    }

    return builder.CreateVector<Offset<Metadata::PropertyDecl>>(properties);
}

static std::unordered_map<std::string, std::pair<std::string, int32_t>> CollectMethodOverloadGroups(
    const ArenaVector<ir::AstNode *> &body)
{
    std::unordered_map<std::string, std::pair<std::string, int32_t>> overloadGroupOf;
    for (const auto &elem : body) {
        if (!elem->IsOverloadDeclaration() || !elem->AsOverloadDeclaration()->IsClassMethodOverloadDeclaration()) {
            continue;
        }
        const auto *overloadDecl = elem->AsOverloadDeclaration();
        int32_t index = 0;
        for (const auto *overloadedName : overloadDecl->OverloadedList()) {
            if (!overloadedName->IsIdentifier()) {
                continue;
            }
            overloadGroupOf.emplace(overloadedName->AsIdentifier()->Name().Utf8(),
                                    std::make_pair(overloadDecl->Name().Utf8(), index));
            ++index;
        }
    }
    return overloadGroupOf;
}

Offset<Vector<Offset<Metadata::FunctionDecl>>> MetadataSerializationPhase::BuildMethodDecls(
    FlatBufferBuilder &builder, const ArenaVector<ir::AstNode *> &body, const bool isFromNamespaceOrTopLevel)
{
    const auto overloadGroupOf = CollectMethodOverloadGroups(body);

    std::vector<Offset<Metadata::FunctionDecl>> methods;
    std::unordered_set<std::string> seenMethodSignatures;

    const auto appendMethod = [this, &builder, &methods, &seenMethodSignatures,
                               &overloadGroupOf](const ir::MethodDefinition *method) {
        if (!ShouldProcessMethod(method, seenMethodSignatures)) {
            return;
        }

        const auto *func = method->Function();
        std::string_view overloadGroup;
        int32_t overloadGroupIndex = 0;
        if (const auto it = overloadGroupOf.find(func->Id()->ToString()); it != overloadGroupOf.end()) {
            overloadGroup = it->second.first;
            overloadGroupIndex = it->second.second;
        }

        bool isAbstract = method->IsAbstract() || (!func->HasBody() && !method->IsNative());
        methods.emplace_back(BuildFunctionDecl(builder, func, method->IsProtected(), method->IsGetter(),
                                               method->IsSetter(), method->IsFinal(), method->IsNative(), isAbstract,
                                               overloadGroup, overloadGroupIndex));
    };

    IterateMethods(body, isFromNamespaceOrTopLevel, appendMethod);

    return builder.CreateVector<Offset<Metadata::FunctionDecl>>(methods);
}

bool MetadataSerializationPhase::ShouldProcessMethod(const ir::MethodDefinition *method,
                                                     std::unordered_set<std::string> &seenMethodSignatures)
{
    const auto *func = method->Function();

    if (method->IsPrivate()) {
        return false;
    }
    if (method->IsConstructor() && func->IsSynthetic() && func->HasRestParameter()) {
        return false;
    }
    if (func->Signature() == nullptr) {
        return false;
    }
    if (func->Signature()->ReturnType() == nullptr) {
        return false;
    }

    const auto signatureKey = func->Id()->ToString() + func->Signature()->ToString();
    if (!seenMethodSignatures.emplace(signatureKey).second) {
        return false;
    }

    return true;
}

void MetadataSerializationPhase::IterateMethods(const ArenaVector<ir::AstNode *> &body,
                                                const bool isFromNamespaceOrTopLevel,
                                                const std::function<void(const ir::MethodDefinition *)> &appendMethod)
{
    for (const auto &elem : body) {
        if (isFromNamespaceOrTopLevel && !elem->IsExported() && !elem->IsDefaultExported() && !elem->HasExportAlias()) {
            continue;
        }
        if (elem->IsMethodDefinition()) {
            const auto *method = elem->AsMethodDefinition();
            if (method->Function()->IsOverload()) {
                continue;
            }
            if (!method->IsPrivate()) {
                appendMethod(method);
            }
            for (const auto *overload : method->Overloads()) {
                if (!overload->IsPrivate()) {
                    appendMethod(overload);
                }
            }
        }
    }
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

Offset<flatbuffers::String> MetadataSerializationPhase::BuildRefTypeName(FlatBufferBuilder &builder,
                                                                         const ir::AstNode &node)
{
    std::string name;
    auto const *curNode = &node;
    while (curNode != nullptr) {
        const ir::Identifier *id = nullptr;
        if (curNode->IsClassDefinition() && !curNode->AsClassDefinition()->IsGlobal()) {
            id = curNode->AsClassDefinition()->Ident();
        } else if (curNode->IsTSInterfaceDeclaration()) {
            id = curNode->AsTSInterfaceDeclaration()->Id();
        } else if (curNode->IsClassDeclaration()) {
            curNode = curNode->Parent();
            continue;
        } else {
            break;
        }

        name = std::string(id->Name().Utf8()) + (name.empty() ? "" : "." + name);
        curNode = curNode->Parent();
    }

    return builder.CreateSharedString(name);
}

Offset<> MetadataSerializationPhase::BuildRefType(FlatBufferBuilder &builder, const checker::ETSObjectType *type)
{
    const auto decl = type->GetDeclNode();

    ES2PANDA_ASSERT(decl->IsClassDefinition() ||
                    decl->IsTSInterfaceDeclaration());  // other decls are not supported yet

    std::vector<uint8_t> typeArgKinds;
    std::vector<Offset<>> typeArgs;
    for (auto const &typeArg : type->TypeArguments()) {
        const auto [componentTypeKind, componentTypeOff] = BuildType(builder, typeArg);
        typeArgs.emplace_back(componentTypeOff);
        typeArgKinds.emplace_back(componentTypeKind);
    }

    return Metadata::CreateTypeRef(builder, BuildRefTypeName(builder, *decl), builder.CreateVector(typeArgKinds),
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
        params.emplace_back(Metadata::CreateFunctionTypeParam(builder, paramName, paramTypeKind, paramTypeOff,
                                                              param->HasFlag(varbinder::VariableFlags::OPTIONAL),
                                                              false));
    }
    if (signature->HasRestParameter()) {
        const auto [paramTypeKind, paramTypeOff] = BuildType(builder, signature->RestVar()->TsType());
        const auto paramName = builder.CreateSharedString(signature->RestVar()->Name().Utf8());
        params.emplace_back(
            Metadata::CreateFunctionTypeParam(builder, paramName, paramTypeKind, paramTypeOff, false, true));
    }
    const auto [returnTypeKind, returnTypeOff] = BuildType(builder, signature->ReturnType());
    return Metadata::CreateFunctionType(builder, builder.CreateVector(params), returnTypeKind, returnTypeOff,
                                        signature->HasSignatureFlag(checker::SignatureFlags::EXTENSION_FUNCTION))
        .Union();
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
    return Metadata::CreateTypeRef(builder, builder.CreateSharedString(std::string(type->Name())), 0, 0,
                                   type->DoAllowUnsafeVariance())
        .Union();
}

Offset<> MetadataSerializationPhase::BuildAwaitedType(FlatBufferBuilder &builder, const checker::ETSAwaitedType *type)
{
    const auto [underlyingTypeKind, underlyingTypeOff] = BuildType(builder, type->GetUnderlying());
    std::vector<uint8_t> typeArgKinds {static_cast<uint8_t>(underlyingTypeKind)};
    std::vector<Offset<>> typeArgs {underlyingTypeOff};

    return Metadata::CreateTypeRef(builder, builder.CreateSharedString("Awaited"), builder.CreateVector(typeArgKinds),
                                   builder.CreateVector(typeArgs))
        .Union();
}

std::pair<Metadata::Type, Offset<>> MetadataSerializationPhase::BuildAliasType(FlatBufferBuilder &builder,
                                                                               const checker::ETSTypeAliasType *type)
{
    const auto *resolved = type->GetTargetType();
    if (resolved != nullptr && resolved->IsETSObjectType()) {
        const auto *etsObj = resolved->AsETSObjectType();
        if (etsObj->IsPartial()) {
            return BuildType(builder, resolved);
        }
    }

    std::vector<uint8_t> typeArgKinds;
    std::vector<Offset<>> typeArgs;

    for (const auto *typeArg : type->TypeArguments()) {
        const auto [kind, offset] = BuildType(builder, typeArg);
        typeArgKinds.emplace_back(kind);
        typeArgs.emplace_back(offset);
    }

    const auto typeName = type->GetDeclNode()->AsTSTypeAliasDeclaration()->Id()->Name().Utf8();
    return {Metadata::Type::Type_Ref,
            Metadata::CreateTypeRef(builder, builder.CreateSharedString(typeName), builder.CreateVector(typeArgKinds),
                                    builder.CreateVector(typeArgs))
                .Union()};
}

Offset<> MetadataSerializationPhase::BuildPartialType(FlatBufferBuilder &builder, const checker::ETSObjectType *type)
{
    const auto *baseType = type->GetBaseType();
    ES2PANDA_ASSERT(baseType != nullptr);
    const auto [innerTypeKind, innerTypeOff] = BuildType(builder, baseType);
    return Metadata::CreatePartialType(builder, innerTypeKind, innerTypeOff).Union();
}

std::pair<Metadata::Type, Offset<>> MetadataSerializationPhase::BuildType(FlatBufferBuilder &builder, const Type *type)
{
    if (type->IsETSObjectType()) {
        const auto etsObjType = type->AsETSObjectType();
        if (etsObjType->IsPartial()) {
            return {Metadata::Type::Type_Partial, BuildPartialType(builder, etsObjType)};
        }
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
        return BuildAliasType(builder, type->AsETSTypeAliasType());
    }

    if (type->IsETSAwaitedType()) {
        return {Metadata::Type::Type_Ref, BuildAwaitedType(builder, type->AsETSAwaitedType())};
    }

    const auto builtinTypeKind = GetBuiltinTypeKind(type);

    ES2PANDA_ASSERT(builtinTypeKind != NOT_BUILTIN_TYPE_KIND);

    return {Metadata::Type::Type_Builtin, Metadata::CreateBuiltinType(builder, builtinTypeKind).Union()};
}

// CC-OFFNXT(G.FUN.01-CPP) solid logic
Offset<Metadata::FunctionDecl> MetadataSerializationPhase::BuildFunctionDecl(
    FlatBufferBuilder &builder, const ir::ScriptFunction *func, bool isProtected, bool isGetter, bool isSetter,
    bool isFinal, bool isNative, bool isAbstract, std::string_view overloadGroup, int32_t overloadGroupIndex)
{
    const auto methodName = builder.CreateSharedString(func->Id()->ToString());
    const auto overloadGroupOff = overloadGroup.empty() ? 0 : builder.CreateSharedString(std::string(overloadGroup));
    const auto valueParams = BuildValueParams(builder, func->Signature());
    const auto typeParams = BuildTypeParams(builder, func->TypeParams());
    const auto isVoidType =
        (func->ReturnTypeAnnotation() && func->ReturnTypeAnnotation()->IsETSPrimitiveType() &&
         func->ReturnTypeAnnotation()->AsETSPrimitiveType()->GetPrimitiveType() == ir::PrimitiveType::VOID) ||
        func->IsConstructor();

    auto *returnType = func->Signature()->ReturnType();
    if (func->ReturnTypeAnnotation() != nullptr && func->ReturnTypeAnnotation()->IsETSPrimitiveType()) {
        returnType = Context()->GetChecker()->AsETSChecker()->MaybeUnboxType(returnType);
    }

    const auto isThisType = func->ReturnTypeAnnotation() != nullptr && func->ReturnTypeAnnotation()->IsTSThisType();

    // Temporary fix for the void return type because at the current stage, undefined type set instead as a return type
    const auto [returnTypeKind, returnTypeOff] =
        isVoidType   ? std::make_pair(Metadata::Type::Type_Builtin,
                                      Metadata::CreateBuiltinType(builder, Metadata::BuiltinTypeKind_void_).Union())
        : isThisType ? std::make_pair(Metadata::Type::Type_This, Metadata::CreateThisType(builder).Union())
                     : BuildType(builder, returnType);

    LOG_METADATA(func->Id()->ToString() << func->Signature()->ToString());

    return Metadata::CreateFunctionDecl(builder, methodName, returnTypeKind, returnTypeOff, func->IsStatic(),
                                        valueParams, typeParams, 0, isProtected, isGetter, isSetter, overloadGroupOff,
                                        overloadGroupIndex, isFinal, isNative, isAbstract);
}

Offset<Metadata::PropertyDecl> MetadataSerializationPhase::BuildPropertyDecl(FlatBufferBuilder &builder,
                                                                             const ir::ClassProperty *var)
{
    LOG_METADATA(var->Id()->ToString() << ": " << GetMetadataPropertyType(var)->ToString());
    const auto varName = builder.CreateSharedString(var->Id()->ToString());
    const auto [returnTypeKind, returnTypeOff] = BuildPropertyType(builder, var);
    return Metadata::CreatePropertyDecl(builder, varName, returnTypeKind, returnTypeOff, var->IsStatic(),
                                        var->IsConst(), var->IsProtected(), var->IsReadonly());
}

Offset<Metadata::ImportDecl> MetadataSerializationPhase::BuildImportDecl(FlatBufferBuilder &builder,
                                                                         const ir::ImportDeclaration *importDecl) const
{
    ES2PANDA_ASSERT(importDecl->IsETSImportDeclaration());
    const auto from = importDecl->Source()->Str().Utf8();
    const auto moduleName = importDecl->AsETSImportDeclaration()->ImportInfo().ModuleName();
    ES2PANDA_ASSERT(!moduleName.empty());

    std::vector<Offset<flatbuffers::String>> specifiers;
    for (const auto &specifier : importDecl->Specifiers()) {
        if (!specifier->IsImportSpecifier()) {
            continue;
        }
        specifiers.emplace_back(builder.CreateSharedString(specifier->AsImportSpecifier()->Imported()->Name().Utf8()));
    }

    LOG_METADATA(IrDeclToString(importDecl));

    return Metadata::CreateImportDecl(builder, builder.CreateSharedString(from), builder.CreateSharedString(moduleName),
                                      builder.CreateVector(specifiers));
}

std::vector<Offset<Metadata::ReExportDecl>> MetadataSerializationPhase::BuildReExportDecls(
    FlatBufferBuilder &builder, const ir::ETSReExportDeclaration *reExportDecl) const
{
    auto *const importDecl = reExportDecl->GetETSImportDeclarations();
    ES2PANDA_ASSERT(importDecl != nullptr);
    const auto from = importDecl->Source()->Str().Utf8();
    const auto moduleName = importDecl->ImportInfo().ModuleName();
    ES2PANDA_ASSERT(!moduleName.empty());
    LOG_METADATA(IrDeclToString(reExportDecl));
    std::vector<Offset<Metadata::ReExportDecl>> reExports;
    for (const auto *specifier : importDecl->Specifiers()) {
        Metadata::ReExportKind kind;
        Offset<flatbuffers::String> exportedName;
        Offset<flatbuffers::String> importedName;
        if (specifier->IsImportSpecifier()) {
            const auto *is = specifier->AsImportSpecifier();
            kind = Metadata::ReExportKind_Named;
            exportedName = builder.CreateSharedString(is->Local()->Name().Utf8());
            importedName = builder.CreateSharedString(is->Imported()->Name().Utf8());
        } else if (specifier->IsImportNamespaceSpecifier()) {
            const auto *ns = specifier->AsImportNamespaceSpecifier();
            if (ns->Local()->Name().Empty()) {
                kind = Metadata::ReExportKind_Star;
            } else {
                kind = Metadata::ReExportKind_Namespace;
                exportedName = builder.CreateSharedString(ns->Local()->Name().Utf8());
            }
        } else {
            ES2PANDA_UNREACHABLE();
        }
        reExports.emplace_back(Metadata::CreateReExportDecl(builder, builder.CreateSharedString(from),
                                                            builder.CreateSharedString(moduleName), kind, exportedName,
                                                            importedName, importDecl->IsTypeKind()));
    }
    return reExports;
}

Offset<Metadata::ReExportDecl> MetadataSerializationPhase::BuildImportBindingReExportDecl(
    FlatBufferBuilder &builder, const varbinder::ImportBindingInfo *bindingInfo, util::StringView exportedName,
    bool isExplicitTypeOnly) const
{
    ES2PANDA_ASSERT(bindingInfo != nullptr);
    ES2PANDA_ASSERT(bindingInfo->importDecl != nullptr);
    ES2PANDA_ASSERT(bindingInfo->kind == varbinder::ImportBindingKind::NAMED);
    ES2PANDA_ASSERT(!exportedName.Empty());
    ES2PANDA_ASSERT(!bindingInfo->importedName.Empty());

    const auto *importDecl = bindingInfo->importDecl;
    const auto from = importDecl->Source()->Str().Utf8();
    const auto moduleName = importDecl->ImportInfo().ModuleName();
    ES2PANDA_ASSERT(!moduleName.empty());

    return Metadata::CreateReExportDecl(
        builder, builder.CreateSharedString(from), builder.CreateSharedString(moduleName), Metadata::ReExportKind_Named,
        builder.CreateSharedString(exportedName.Utf8()), builder.CreateSharedString(bindingInfo->importedName.Utf8()),
        isExplicitTypeOnly || bindingInfo->isTypeOnly);
}

Offset<Metadata::LocalExportDecl> MetadataSerializationPhase::BuildLocalExportDecl(
    FlatBufferBuilder &builder, const varbinder::PendingLocalExportAlias &alias) const
{
    ES2PANDA_ASSERT(alias.kind == varbinder::LocalExportKind::ALIAS);
    ES2PANDA_ASSERT(!alias.isInvalid);
    ES2PANDA_ASSERT(!alias.exportedName.Empty());
    ES2PANDA_ASSERT(!alias.localName.Empty());
    return Metadata::CreateLocalExportDecl(builder, builder.CreateSharedString(alias.exportedName.Utf8()),
                                           builder.CreateSharedString(alias.localName.Utf8()),
                                           alias.isExplicitTypeOnly);
}

Offset<Metadata::LocalExportDecl> MetadataSerializationPhase::BuildLocalExportDecl(
    FlatBufferBuilder &builder, const varbinder::ExportFact &fact) const
{
    ES2PANDA_ASSERT(!fact.isLocalAlias);
    ES2PANDA_ASSERT(!fact.isInvalid);
    ES2PANDA_ASSERT(fact.variable != nullptr);
    ES2PANDA_ASSERT(fact.origin != nullptr);
    ES2PANDA_ASSERT(!fact.exportedName.Empty());
    ES2PANDA_ASSERT(!fact.variable->Name().Empty());
    return Metadata::CreateLocalExportDecl(builder, builder.CreateSharedString(fact.exportedName.Utf8()),
                                           builder.CreateSharedString(fact.variable->Name().Utf8()),
                                           fact.isExplicitTypeOnly);
}

void MetadataSerializationPhase::ProcessStatement(FlatBufferBuilder &builder, const ir::AstNode &node,
                                                  MetadataDecls &decls)
{
    if (node.IsETSImportDeclaration()) {
        decls.imports.emplace_back(BuildImportDecl(builder, node.AsETSImportDeclaration()));
        return;
    }

    if (node.IsETSReExportDeclaration()) {
        auto reExports = BuildReExportDecls(builder, node.AsETSReExportDeclaration());
        decls.reExports.insert(decls.reExports.end(), reExports.begin(), reExports.end());
        return;
    }

    const auto isExported = node.IsExported() || node.IsDefaultExported() || node.HasExportAlias() ||
                            (node.IsClassDeclaration() && node.AsClassDeclaration()->Definition()->IsGlobal());
    if (!isExported) {
        return;
    }

    if (node.IsClassDeclaration()) {
        const auto className = node.AsClassDeclaration()->Definition()->Ident()->Name().Utf8();
        if (IsPartialClassName(className)) {
            return;
        }
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

    // Metadata should not be serialized from a semantically broken program.
    if (ctx->diagnosticEngine != nullptr && ctx->diagnosticEngine->IsAnyError()) {
        return true;
    }

    LOG_METADATA_ENABLE();

    const auto pkgName = std::string(program->ModuleName());
    const auto moduleName = std::string(program->SourceFile().GetFileName().Utf8());

    LOG_METADATA("serializing metadata of program " << pkgName << ":" << moduleName);

    FlatBufferBuilder builder;

    LOG_METADATA_NESTING_INC();
    const auto decls = BuildDecls(builder, program->Ast()->Statements(), pkgName, program);
    LOG_METADATA_NESTING_DEC();

    if (decls.IsNull()) {  // No exported decls, skip recording for such module
        LOG_METADATA_DISABLE();
        return true;
    }

    ctx->metadata[pkgName][moduleName] = GetMetadataBytes(builder, decls);

    LOG_METADATA_DISABLE();

    return true;
}

}  // namespace ark::es2panda::compiler
