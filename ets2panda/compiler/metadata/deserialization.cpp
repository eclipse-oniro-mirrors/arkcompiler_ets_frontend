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

#include "deserialization.h"
#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "ir/ts/tsQualifiedName.h"
#include "schemaMetadataGenerated.h"
#include "compiler/lowering/ets/topLevelStmts/globalClassHandler.h"
#include "varbinder/ETSBinder.h"
#include "flatbuffers/flatbuffers.h"
#include "evaluate/helpers.h"
#include "libarkbase/utils/logger.h"
#include "libarkfile/metadata_helper.h"
#include "utils.h"

#include <string>

namespace ark::es2panda::compiler {

using namespace flatbuffers;
using namespace panda_file;
using namespace panda_file::helpers;

#define CUR_METADATA_LOGGER_COMPONENT METADATA_DESERIALIZATION

// NOLINTNEXTLINE(cert-err58-cpp,fuchsia-statically-constructed-objects)
const std::map<Metadata::BuiltinTypeKind, ir::PrimitiveType> MetadataDeserializationPhase::BUILTIN_PRIMITIVE_TYPES = {
    {Metadata::BuiltinTypeKind::BuiltinTypeKind_boolean, ir::PrimitiveType::BOOLEAN},
    {Metadata::BuiltinTypeKind::BuiltinTypeKind_byte_, ir::PrimitiveType::BYTE},
    {Metadata::BuiltinTypeKind::BuiltinTypeKind_short_, ir::PrimitiveType::SHORT},
    {Metadata::BuiltinTypeKind::BuiltinTypeKind_char_, ir::PrimitiveType::CHAR},
    {Metadata::BuiltinTypeKind::BuiltinTypeKind_int_, ir::PrimitiveType::INT},
    {Metadata::BuiltinTypeKind::BuiltinTypeKind_long_, ir::PrimitiveType::LONG},
    {Metadata::BuiltinTypeKind::BuiltinTypeKind_float_, ir::PrimitiveType::FLOAT},
    {Metadata::BuiltinTypeKind::BuiltinTypeKind_double_, ir::PrimitiveType::DOUBLE},
    {Metadata::BuiltinTypeKind::BuiltinTypeKind_void_, ir::PrimitiveType::VOID}};

void MetadataDeserializationPhase::SetupGlobalClassStaticBlock(ir::ClassStaticBlock *staticBlock) const
{
    const auto ctx = Context();
    auto allocator = ctx->Allocator();

    const auto paramScope =
        ArenaAllocator::New<varbinder::FunctionParamScope>(allocator, staticBlock->Parent()->Scope());
    const auto functionScope = ArenaAllocator::New<varbinder::FunctionScope>(allocator, paramScope);
    functionScope->BindParamScope(paramScope);
    functionScope->BindName(staticBlock->Function()->Id()->Name());
    paramScope->BindFunctionScope(functionScope);
    staticBlock->Function()->SetScope(functionScope);
}

void MetadataDeserializationPhase::SetupGlobalClass() const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();

    GlobalClassHandler(ctx).SetupGlobalClass(curProgram);
    curProgram->GlobalClass()->SetScope(
        ArenaAllocator::New<varbinder::ClassScope>(allocator, curProgram->Ast()->Scope()));

    for (const auto &member : curProgram->GlobalClass()->Body()) {
        if (member->IsClassStaticBlock()) {
            SetupGlobalClassStaticBlock(member->AsClassStaticBlock());
        }
    }

    const auto binderDecl = EAllocator::New<varbinder::ClassDecl>(curProgram->GlobalClass()->Ident()->Name());
    binderDecl->BindNode(curProgram->GlobalClass());
    curProgram->GlobalClass()->Ident()->SetVariable(
        curProgram->GlobalScope()->AddDecl(allocator, binderDecl, ScriptExtension::ETS));
}

void MetadataDeserializationPhase::MarkBuiltinIfNeeded(varbinder::Variable *var) const
{
    if (var && curProgram->ModuleName() == "std.core") {
        var->AddFlag(varbinder::VariableFlags::BUILTIN_TYPE);
    }
}

void MetadataDeserializationPhase::AddExtends(const Metadata::InterfaceDecl *fbInterfaceDecl,
                                              ir::TSInterfaceDeclaration *interfaceDecl) const
{
    const auto extendTypes = fbInterfaceDecl->implemented_interfaces();
    const auto extendTypeKinds = fbInterfaceDecl->implemented_interfaces_type();
    for (size_t i = 0; i < extendTypes->size(); i++) {
        const auto type = CreateType(extendTypes->Get(i), static_cast<Metadata::Type>(extendTypeKinds->Get(i)));
        interfaceDecl->EmplaceExtends(Context()->AllocNode<ir::TSInterfaceHeritage>(type));
    }
}

void MetadataDeserializationPhase::AddMethods(const Vector<Offset<Metadata::FunctionDecl>> &methods,
                                              ArenaVector<ir::AstNode *> &body, ir::AstNode *parent)
{
    for (const auto fbMethodDecl : methods) {
        const auto methodDecl = CreateMethodDecl(fbMethodDecl);
        body.emplace_back(methodDecl);
        methodDecl->SetParent(parent);

        LOG_METADATA(methodDecl->Id()->Name() << IrDeclVectorToString(methodDecl->Function()->Params()));
    }
}

void MetadataDeserializationPhase::AddClassMembers(const Metadata::ClassDecl *fbClassDecl,
                                                   ir::ClassDefinition *classDef)
{
    AddMethods(*fbClassDecl->methods(), classDef->BodyForUpdate(), classDef);

    for (const auto fbPropDecl : *fbClassDecl->properties()) {
        const auto propDecl = CreatePropertyDecl(fbPropDecl);

        classDef->EmplaceBody(propDecl);
        propDecl->SetParent(classDef);

        LOG_METADATA(propDecl->Id()->Name() << ": " << IrDeclToString(propDecl->TypeAnnotation()));
    }

    if (fbClassDecl->decls()) {
        auto classBody = classDef->BodyForUpdate();
        const auto decls = CreateDecls(fbClassDecl->decls());
        classBody.reserve(classBody.size() + decls.size());
        classBody.insert(classBody.end(), decls.begin(), decls.end());

        auto *const staticDeclScope = classDef->Scope()->AsClassScope()->StaticDeclScope();
        for (auto *const decl : decls) {
            if (!decl->IsClassDefinition() && !decl->IsTSInterfaceDeclaration()) {
                continue;
            }
            const auto id =
                decl->IsClassDefinition() ? decl->AsClassDefinition()->Ident() : decl->AsTSInterfaceDeclaration()->Id();
            if (id->Variable() != nullptr) {
                staticDeclScope->InsertBinding(id->Name(), id->Variable());
            }
        }
    }
}

template <typename T>
void MetadataDeserializationPhase::RunBinderForMembers(T *node) const
{
    static_assert(std::is_same_v<T, ir::TSInterfaceDeclaration> || std::is_same_v<T, ir::ClassDefinition>,
                  "T must be TSInterfaceDeclaration or ClassDefinition");

    const auto etsBinder = Context()->GetChecker()->VarBinder()->AsETSBinder();
    varbinder::GlobalScopeContext gsc(etsBinder, curProgram, curProgram->GlobalScope());
    varbinder::RecordTableContext rtc(etsBinder, curProgram);
    varbinder::BoundContext boundCtx(etsBinder->GetRecordTable(), node, true);
    if constexpr (std::is_same_v<T, ir::TSInterfaceDeclaration>) {
        etsBinder->BuildInterfaceDeclaration(node);
    } else {
        etsBinder->BuildClassDefinition(node);
    }
}

template <typename T>
constexpr auto MetadataDeserializationPhase::GetLazyMembers()
{
    if constexpr (std::is_same_v<T, ir::TSInterfaceDeclaration>) {
        return lazyInterfaceMembers_;
    } else {
        return lazyClassMembers_;
    }
}

template <typename T, typename K>
void MetadataDeserializationPhase::MaterializeMembers(T *node, K const *fbDecl)
{
    static_assert(std::is_same_v<T, ir::TSInterfaceDeclaration> || std::is_same_v<T, ir::ClassDefinition>,
                  "T must be TSInterfaceDeclaration or ClassDefinition");

    const auto isLazy = fbDecl == nullptr;
    parser::Program *ownerProgram = curProgram;

    if (!fbDecl) {
        auto lazyMembers = GetLazyMembers<T>();
        const auto member = lazyMembers.find(node);
        if (member == lazyMembers.end()) {
            return;  // already materialized
        }

        fbDecl = member->second.first;
        ownerProgram = member->second.second;
        lazyMembers.erase(member);
    }

    WithProgram(ownerProgram, [this, &fbDecl, &node, &isLazy] {
        WithScope<void>(node->Scope(), [this, &fbDecl, &node]() -> void {
            LOG_METADATA_NESTING_INC();
            if constexpr (std::is_same_v<T, ir::TSInterfaceDeclaration>) {
                AddMethods(*fbDecl->methods(), node->Body()->Body(), node);
            } else {
                AddClassMembers(fbDecl, node);
            }
            LOG_METADATA_NESTING_DEC();
        });
        if (isLazy) {
            RunBinderForMembers(node);
        }
    });
}

ValueParamsInfo MetadataDeserializationPhase::CreateValueParams(
    const Vector<Offset<Metadata::ValueParamDecl>> *fbValueParams) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto paramScope = EAllocator::New<varbinder::FunctionParamScope>(allocator, Scope());

    if (fbValueParams->size() == 0) {
        return {{}, paramScope};
    }

    ArenaVector<ir::Expression *> valueParams;
    for (const auto &fbValueParam : *fbValueParams) {
        auto id = ctx->AllocNode<ir::Identifier>(fbValueParam->name()->string_view(), allocator);
        auto valueParam = ctx->AllocNode<ir::ETSParameterExpression>(id, false, allocator);
        valueParam->SetTypeAnnotation(CreateType(fbValueParam->type(), fbValueParam->type_type()));
        id->SetVariable(varbinder::FunctionParamScope::CreateVar<varbinder::ParameterDecl, varbinder::LocalVariable>(
            allocator, id->Name(), varbinder::VariableFlags::NONE, valueParam));
        if (fbValueParam->is_optional()) {
            valueParam->SetOptional(true);
            id->Variable()->AddFlag(varbinder::VariableFlags::OPTIONAL);
        }
        if (fbValueParam->is_readonly()) {
            id->Variable()->AddFlag(varbinder::VariableFlags::READONLY);
        }
        valueParams.emplace_back(valueParam);
    }

    return {valueParams, paramScope};
}

ir::TSTypeParameterDeclaration *MetadataDeserializationPhase::CreateTypeParams(
    const Vector<Offset<Metadata::TypeParamDecl>> *fbTypeParams) const
{
    if (!fbTypeParams || fbTypeParams->size() == 0) {
        return nullptr;
    }

    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto typeParamScope = ArenaAllocator::New<varbinder::LocalScope>(allocator, Scope());
    ArenaVector<ir::TSTypeParameter *> typeParams;

    for (const auto &fbTypeParam : *fbTypeParams) {
        auto id = ctx->AllocNode<ir::Identifier>(fbTypeParam->name()->string_view(), allocator);
        auto typeParam = ctx->AllocNode<ir::TSTypeParameter>(id, nullptr, nullptr, allocator);
        if (fbTypeParam->variance() == Metadata::TypeParamVariance::TypeParamVariance_IN) {
            typeParam->AddModifier(ir::ModifierFlags::IN);
        } else if (fbTypeParam->variance() == Metadata::TypeParamVariance::TypeParamVariance_OUT) {
            typeParam->AddModifier(ir::ModifierFlags::OUT);
        }
        const auto binderTypeParamDecl = EAllocator::New<varbinder::TypeParameterDecl>(id->Name());
        binderTypeParamDecl->BindNode(typeParam);
        typeParams.emplace_back(typeParam);
        id->SetVariable(typeParamScope->AddDecl(allocator, binderTypeParamDecl, ScriptExtension::ETS));
    }

    const auto typeParamsDecl =
        ctx->AllocNode<ir::TSTypeParameterDeclaration>(std::move(typeParams), typeParams.size());
    typeParamsDecl->SetScope(typeParamScope);

    return typeParamsDecl;
}

ir::TypeNode *MetadataDeserializationPhase::CreateBuiltinType(const Metadata::BuiltinTypeKind kind) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto checker = ctx->GetChecker()->AsETSChecker();

    if (kind == Metadata::BuiltinTypeKind::BuiltinTypeKind_undefined) {
        return ctx->AllocNode<ir::ETSUndefinedType>(allocator);
    }

    if (kind == Metadata::BuiltinTypeKind::BuiltinTypeKind_any) {
        return ctx->AllocNode<ir::OpaqueTypeNode>(checker->GlobalETSAnyType(), allocator);
    }

    if (kind == Metadata::BuiltinTypeKind::BuiltinTypeKind_null) {
        return ctx->AllocNode<ir::OpaqueTypeNode>(checker->GlobalETSNullType(), allocator);
    }

    if (kind == Metadata::BuiltinTypeKind::BuiltinTypeKind_never) {
        return ctx->AllocNode<ir::OpaqueTypeNode>(checker->GlobalETSNeverType(), allocator);
    }

    if (kind == Metadata::BuiltinTypeKind::BuiltinTypeKind_object) {
        return ctx->AllocNode<ir::ETSTypeReference>(
            ctx->AllocNode<ir::ETSTypeReferencePart>(
                ctx->AllocNode<ir::Identifier>(Signatures::BUILTIN_OBJECT_CLASS, allocator), allocator),
            allocator);
    }

    if (kind == Metadata::BuiltinTypeKind::BuiltinTypeKind_string_) {
        return ctx->AllocNode<ir::ETSTypeReference>(
            ctx->AllocNode<ir::ETSTypeReferencePart>(ctx->AllocNode<ir::Identifier>("string", allocator), allocator),
            allocator);
    }

    return ctx->AllocNode<ir::ETSPrimitiveType>(BUILTIN_PRIMITIVE_TYPES.at(kind), allocator);
}

ir::TypeNode *MetadataDeserializationPhase::CreateRefType(const Metadata::TypeRef *fbRefType) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto fqname = fbRefType->fqname()->string_view();

    ir::Expression *name = nullptr;
    for (size_t start = 0; start <= fqname.size();) {
        const auto dot = fqname.find('.', start);
        const auto seg = fqname.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start);
        const auto segId = ctx->AllocNode<ir::Identifier>(util::UString(seg, allocator).View(), allocator);
        name = (name == nullptr) ? static_cast<ir::Expression *>(segId)
                                 : ctx->AllocNode<ir::TSQualifiedName>(name, segId, allocator);
        if (dot == std::string_view::npos) {
            break;
        }
        start = dot + 1;
    }

    const auto typeRef =
        ctx->AllocNode<ir::ETSTypeReference>(ctx->AllocNode<ir::ETSTypeReferencePart>(name, allocator), allocator);

    if (fbRefType->type_args() && fbRefType->type_args()->size() > 0) {
        const auto fbTypeArgs = fbRefType->type_args();
        const auto fbTypeArgKinds = fbRefType->type_args_type();
        ArenaVector<ir::TypeNode *> typeArgs;
        for (size_t i = 0; i < fbTypeArgs->size(); i++) {
            typeArgs.emplace_back(CreateType(fbTypeArgs->Get(i), static_cast<Metadata::Type>(fbTypeArgKinds->Get(i))));
        }
        typeRef->Part()->SetTypeParams(ctx->AllocNode<ir::TSTypeParameterInstantiation>(std::move(typeArgs)));
    }

    return typeRef;
}

ir::TypeNode *MetadataDeserializationPhase::CreateUnionType(const Metadata::UnionType *fbUnionType) const
{
    const auto ctx = Context();
    const auto fbConstituentTypes = fbUnionType->components();
    const auto fbConstituentTypeKinds = fbUnionType->components_type();
    ArenaVector<ir::TypeNode *> constituentTypes;

    for (size_t i = 0; i < fbConstituentTypes->size(); i++) {
        constituentTypes.emplace_back(
            CreateType(fbConstituentTypes->Get(i), static_cast<Metadata::Type>(fbConstituentTypeKinds->Get(i))));
    }

    return ctx->AllocNode<ir::ETSUnionType>(std::move(constituentTypes), ctx->Allocator());
}

ir::TypeNode *MetadataDeserializationPhase::CreateArrayType(const Metadata::ArrayType *fbArrayType) const
{
    const auto ctx = Context();
    const auto elementType = CreateType(fbArrayType->element_type(), fbArrayType->element_type_type());
    return ctx->AllocNode<ir::TSArrayType>(elementType, ctx->Allocator());
}

ir::TypeNode *MetadataDeserializationPhase::CreateTupleType(const Metadata::TupleType *fbTupleType) const
{
    const auto ctx = Context();
    ArenaVector<ir::TypeNode *> constituentTypes;

    for (size_t i = 0; i < fbTupleType->elements()->size(); i++) {
        constituentTypes.emplace_back(CreateType(fbTupleType->elements()->Get(i),
                                                 static_cast<Metadata::Type>(fbTupleType->elements_type()->Get(i))));
    }

    return ctx->AllocNode<ir::ETSTuple>(std::move(constituentTypes), ctx->Allocator());
}

ir::TypeNode *MetadataDeserializationPhase::CreateFunctionType(const Metadata::FunctionType *fbFunctionType) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto paramScope = ArenaAllocator::New<varbinder::FunctionParamScope>(allocator, Scope());
    const auto fbParams = fbFunctionType->params();
    ArenaVector<ir::Expression *> params;

    for (size_t i = 0; i < fbParams->size(); i++) {
        const auto fbParam = fbParams->Get(i);
        const auto paramId = ctx->AllocNode<ir::Identifier>(
            fbParam->name()->string_view(), CreateType(fbParam->type(), fbParam->type_type()), allocator);
        const auto param = ctx->AllocNode<ir::ETSParameterExpression>(paramId, false, allocator);
        paramId->SetVariable(
            varbinder::FunctionParamScope::CreateVar<varbinder::ParameterDecl, varbinder::LocalVariable>(
                allocator, paramId->Name(), varbinder::VariableFlags::NONE, param));
        params.emplace_back(param);
    }

    const auto returnType = CreateType(fbFunctionType->return_type(), fbFunctionType->return_type_type());
    const auto funcType =
        ctx->AllocNode<ir::ETSFunctionType>(ir::FunctionSignature(nullptr, std::move(params), returnType, allocator),
                                            ir::ScriptFunctionFlags::NONE, allocator);
    funcType->SetScope(paramScope);

    return funcType;
}

ir::TypeNode *MetadataDeserializationPhase::CreateStringLiteralType(
    const Metadata::StringLiteralType *fbStringLiteralType) const
{
    const auto ctx = Context();
    return ctx->AllocNode<ir::ETSStringLiteralType>(fbStringLiteralType->value()->string_view(), ctx->Allocator());
}

ir::TypeNode *MetadataDeserializationPhase::CreateType(const void *type, const Metadata::Type kind) const
{
    switch (kind) {
        case Metadata::Type_Builtin: {
            return CreateBuiltinType(static_cast<const Metadata::BuiltinType *>(type)->kind());
        }
        case Metadata::Type_Ref: {
            return CreateRefType(static_cast<const Metadata::TypeRef *>(type));
        }
        case Metadata::Type_Union: {
            return CreateUnionType(static_cast<const Metadata::UnionType *>(type));
        }
        case Metadata::Type_Array: {
            return CreateArrayType(static_cast<const Metadata::ArrayType *>(type));
        }
        case Metadata::Type_Tuple: {
            return CreateTupleType(static_cast<const Metadata::TupleType *>(type));
        }
        case Metadata::Type_Function: {
            return CreateFunctionType(static_cast<const Metadata::FunctionType *>(type));
        }
        case Metadata::Type_StringLiteral: {
            return CreateStringLiteralType(static_cast<const Metadata::StringLiteralType *>(type));
        }
        case Metadata::Type_NONE:
            ES2PANDA_ASSERT(false);  // Deserialization of other types is not supported yet
            break;
    }
    return nullptr;
}

ir::MethodDefinition *MetadataDeserializationPhase::CreateMethodDecl(const Metadata::FunctionDecl *fbMethodDecl)
{
    const auto ctx = Context();
    const auto fbValueParams = fbMethodDecl->value_params();
    const auto fbTypeParams = fbMethodDecl->type_params();
    const auto methodName = fbMethodDecl->name()->string_view();
    const auto modifiers = ir::ModifierFlags::PUBLIC | ir::ModifierFlags::DECLARE |
                           (fbMethodDecl->is_static() ? ir::ModifierFlags::STATIC : ir::ModifierFlags::NONE);
    const auto isConstructor = methodName == "constructor";
    const auto flags = isConstructor ? ir::ScriptFunctionFlags::CONSTRUCTOR : ir::ScriptFunctionFlags::NONE;
    const auto typeParams = CreateTypeParams(fbTypeParams);

    using SigInfo = std::pair<ValueParamsInfo, ir::TypeNode *>;
    const auto [valueParamsInfo, returnType] = WithScope<SigInfo>(
        typeParams ? typeParams->Scope() : Scope(), [this, &fbValueParams, &fbMethodDecl]() -> SigInfo {
            return {CreateValueParams(fbValueParams),
                    CreateType(fbMethodDecl->return_type(), fbMethodDecl->return_type_type())};
        });
    auto [valueParams, paramsScope] = valueParamsInfo;
    const auto methodDef = ctx->GetChecker()->AsETSChecker()->CreateMethod(
        fbMethodDecl->name()->string_view(), modifiers | ir::ModifierFlags::EXPORT, flags, std::move(valueParams),
        paramsScope, returnType, nullptr,
        isConstructor ? ir::MethodDefinitionKind::CONSTRUCTOR : ir::MethodDefinitionKind::METHOD);
    if (typeParams) {
        typeParams->SetParent(methodDef);
        methodDef->Function()->SetTypeParams(typeParams);
    }

    const auto binderDecl = EAllocator::New<varbinder::MethodDecl>(methodDef->Id()->Name());
    binderDecl->BindNode(methodDef);
    const auto scopeToAdd = isConstructor || fbMethodDecl->is_static() ? Scope()->AsClassScope()->StaticMethodScope()
                                                                       : Scope()->AsClassScope()->InstanceMethodScope();
    auto var = scopeToAdd->AddDecl(ctx->Allocator(), binderDecl, ScriptExtension::ETS);
    if (var == nullptr) {
        var = scopeToAdd->Find(binderDecl->Name()).variable;
        var->Declaration()->Node()->AsMethodDefinition()->OverloadsForUpdate().emplace_back(methodDef);
    }

    var->AddFlag(varbinder::VariableFlags::METHOD);
    methodDef->Id()->SetVariable(var);
    return methodDef;
}

ir::ClassProperty *MetadataDeserializationPhase::CreatePropertyDecl(const Metadata::PropertyDecl *fbPropDecl) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();

    const auto propName = fbPropDecl->name()->string_view();
    const auto type = CreateType(fbPropDecl->return_type(), fbPropDecl->return_type_type());
    const auto modifiers = ir::ModifierFlags::PUBLIC | ir::ModifierFlags::EXPORT | ir::ModifierFlags::DECLARE |
                           (fbPropDecl->is_static() ? ir::ModifierFlags::STATIC : ir::ModifierFlags::NONE);
    const auto propDecl = ctx->AllocNode<ir::ClassProperty>(ctx->AllocNode<ir::Identifier>(propName, allocator),
                                                            nullptr, type, modifiers, allocator, false);

    const auto binderDecl = EAllocator::New<varbinder::PropertyDecl>(propDecl->Id()->Name());
    binderDecl->BindNode(propDecl);
    const auto scopeToAdd = fbPropDecl->is_static() ? Scope()->AsClassScope()->StaticFieldScope()
                                                    : Scope()->AsClassScope()->InstanceFieldScope();
    const auto var = scopeToAdd->AddDecl(allocator, binderDecl, ScriptExtension::ETS);

    var->AddFlag(varbinder::VariableFlags::PROPERTY);
    propDecl->Id()->SetVariable(var);

    return propDecl;
}

ir::ETSImportDeclaration *MetadataDeserializationPhase::CreateImportDecl(const Metadata::ImportDecl *fbImportDecl) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();

    ArenaVector<ir::AstNode *> specifiers;
    for (const auto &fbSpec : *fbImportDecl->decl_names()) {
        auto *local = ctx->AllocNode<ir::Identifier>(fbSpec->string_view(), allocator);
        auto *imported = ctx->AllocNode<ir::Identifier>(fbSpec->string_view(), allocator);
        specifiers.emplace_back(ctx->AllocNode<ir::ImportSpecifier>(imported, local));
    }

    const auto from = ctx->AllocNode<ir::StringLiteral>(fbImportDecl->from()->string_view());
    const auto depProgram = ctx->parser->GetImportPathManager()->GatherImportInfo(curProgram, from);
    const auto importDecl =
        ctx->AllocNode<ir::ETSImportDeclaration>(from, depProgram->GetImportInfo(), std::move(specifiers));

    LOG_METADATA(IrDeclToString(importDecl));

    return importDecl;
}

ir::AnnotationDeclaration *MetadataDeserializationPhase::CreateAnnotationDecl(
    const Metadata::AnnotationDecl *fbAnnotationDecl) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();

    const auto annotationName = fbAnnotationDecl->name()->string_view();
    const auto annotationDecl =
        ctx->AllocNode<ir::AnnotationDeclaration>(ctx->AllocNode<ir::Identifier>(annotationName, allocator), allocator);
    annotationDecl->AddModifier(ir::ModifierFlags::EXPORT);

    LOG_METADATA("annotation " << annotationName);

    return annotationDecl;
}

ir::TSTypeAliasDeclaration *MetadataDeserializationPhase::CreateTypeDecl(const Metadata::TypeDecl *fbTypeDecl) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto typeId = ctx->AllocNode<ir::Identifier>(fbTypeDecl->name()->string_view(), allocator);
    const auto type = CreateType(fbTypeDecl->type(), fbTypeDecl->type_type());
    const auto typeDecl = ctx->AllocNode<ir::TSTypeAliasDeclaration>(allocator, typeId, nullptr, type);
    const auto binderDecl = EAllocator::New<varbinder::TypeAliasDecl>(typeId->Name());
    binderDecl->BindNode(typeDecl);
    typeId->SetVariable(Scope()->AddDecl(allocator, binderDecl, ScriptExtension::ETS));
    typeDecl->AddModifier(ir::ModifierFlags::EXPORT);

    LOG_METADATA("type " << fbTypeDecl->name()->string_view() << " = " << IrDeclToString(type));

    return typeDecl;
}

ir::TSInterfaceDeclaration *MetadataDeserializationPhase::CreateInterfaceDecl(
    const Metadata::InterfaceDecl *fbInterfaceDecl)
{
    const auto typeParams = CreateTypeParams(fbInterfaceDecl->type_params());

    return WithScope<ir::TSInterfaceDeclaration *>(
        typeParams ? typeParams->Scope() : Scope(),
        // clang-format off
        [this, &fbInterfaceDecl, &typeParams]() -> ir::TSInterfaceDeclaration* {
            const auto ctx = Context();
            const auto allocator = ctx->Allocator();
            const auto interfaceName = fbInterfaceDecl->name()->string_view();
            const auto interfaceDeclProto = EAllocator::New<ir::TSInterfaceDeclaration>(
                allocator, ArenaVector<ir::TSInterfaceHeritage *>(),
                ir::TSInterfaceDeclaration::ConstructorData {nullptr, nullptr, nullptr, false, false,
                                                             Language::Id::COUNT});
            interfaceDeclProto->SetScope(ArenaAllocator::New<varbinder::ClassScope>(allocator, Scope()));
            const auto interfaceDecl =
                ctx->GetChecker()->AsETSChecker()->CreateInterfaceProto(interfaceName, curProgram, interfaceDeclProto);

            interfaceDecl->Scope()->BindNode(interfaceDecl);
            interfaceDecl->AddModifier(ir::ModifierFlags::EXPORT);
            MarkBuiltinIfNeeded(interfaceDecl->Variable());

            if (typeParams) {
                interfaceDecl->SetTypeParams(typeParams);
                interfaceDecl->TypeParams()->SetParent(interfaceDecl);
            }

            AddExtends(fbInterfaceDecl, interfaceDecl);

            LOG_METADATA(interfaceName << (interfaceDecl->TypeParams()
                                               ? "<" + IrDeclVectorToString(interfaceDecl->TypeParams()->Params()) + ">"
                                               : "")
                                       << (!interfaceDecl->Extends().empty()
                                               ? ": " + IrDeclVectorToString(interfaceDecl->Extends())
                                               : ""));

            lazyInterfaceMembers_[interfaceDecl] = {fbInterfaceDecl, curProgram};

            return interfaceDecl;
        });
}

ir::ClassDefinition *MetadataDeserializationPhase::CreateClassDecl(
    const Metadata::ClassDecl *fbClassDecl, const Vector<Offset<Metadata::TypeParamDecl>> *fbTypeParams)
{
    const auto typeParams = CreateTypeParams(fbTypeParams);

    return WithScope<ir::ClassDefinition *>(
        // clang-format off
        typeParams ? typeParams->Scope() : Scope(), [this, &fbClassDecl, &typeParams]() -> ir::ClassDefinition* {
            const auto isGlobalClass = fbClassDecl->name()->string_view() == "ETSGLOBAL";
            const auto classDef = isGlobalClass ? curProgram->GlobalClass()
                                                : Context()->GetChecker()->AsETSChecker()->CreateClassPrototype(
                                                    fbClassDecl->name()->string_view(), curProgram);
            classDef->Scope()->SetParent(Scope());
            classDef->Scope()->BindNode(classDef);
            classDef->Parent()->AddModifier(ir::ModifierFlags::EXPORT);
            MarkBuiltinIfNeeded(classDef->Variable());

            if (typeParams) {
                classDef->SetTypeParams(typeParams);
                classDef->TypeParams()->SetParent(classDef);
            }

            LOG_METADATA(GetDeclKindToLog(fbClassDecl)
                         << fbClassDecl->name()->string_view()
                         << (typeParams ? "<" + IrDeclToString(classDef->TypeParams()) + ">" : ""));

            if (isGlobalClass) {
                // Top-level members are materialized eagerly as their bindins are created eagerly as well
                MaterializeMembers(classDef, fbClassDecl);
            } else {
                lazyClassMembers_[classDef] = {fbClassDecl, curProgram};
            }
            return classDef;
        });
}

ir::ETSModule *MetadataDeserializationPhase::CreateModule() const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto moduleInfo = curProgram->ModuleInfo();
    const auto moduleId = ctx->AllocNode<ir::Identifier>(util::StringView {moduleInfo.moduleName}, allocator);
    const auto module = ctx->AllocNode<ir::ETSModule>(allocator, ArenaVector<ir::Statement *> {allocator->Adapter()},
                                                      moduleId, ir::ModuleFlag::ETSSCRIPT, moduleInfo.lang, curProgram);
    module->SetScope(ArenaAllocator::New<varbinder::ModuleScope>(allocator));
    module->Scope()->BindNode(module);
    return module;
}

ArenaVector<ir::AstNode *> MetadataDeserializationPhase::CreateDecls(const Metadata::Decls *decls)
{
    ArenaVector<ir::AstNode *> nodes;

    if (decls->imports()) {
        for (const auto &fbImportDecl : *decls->imports()) {
            nodes.emplace_back(CreateImportDecl(fbImportDecl));
        }
    }

    if (decls->classes()) {
        for (const auto fbClassDecl : *decls->classes()) {
            nodes.emplace_back(CreateClassDecl(fbClassDecl, fbClassDecl->type_params()));
        }
    }

    if (decls->interfaces()) {
        for (const auto &fbInterfaceDecl : *decls->interfaces()) {
            nodes.emplace_back(CreateInterfaceDecl(fbInterfaceDecl));
        }
    }

    if (decls->types()) {
        for (const auto &fbTypeDecl : *decls->types()) {
            nodes.emplace_back(CreateTypeDecl(fbTypeDecl));
        }
    }

    if (decls->annotations()) {
        for (const auto &fbAnnotationDecl : *decls->annotations()) {
            nodes.emplace_back(CreateAnnotationDecl(fbAnnotationDecl));
        }
    }

    return nodes;
}

template <typename T, typename F>
T MetadataDeserializationPhase::WithScope(varbinder::Scope *scope, F &&run)
{
    auto scopeCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(Context()->GetChecker()->VarBinder(), scope);
    if constexpr (std::is_same_v<T, void>) {
        run();
        return;
    } else {
        return run();
    }
}

template <typename F>
void MetadataDeserializationPhase::WithProgram(parser::Program *program, F &&run)
{
    LOG_METADATA_ENABLE();
    const auto prevProgram = curProgram;
    curProgram = program;
    run();
    curProgram = prevProgram;
    LOG_METADATA_DISABLE();
}

void MetadataDeserializationPhase::ProcessMetadata(MetadataByModules *metadata)
{
    for (const auto &[moduleName, moduleMetadata] : *metadata) {
        if (moduleMetadata.empty()) {
            LOG_METADATA("skipped module \"" << moduleName << "\" (no metadata)");
            continue;
        }

        LOG_METADATA("processing module \"" << moduleName << "\" (" << std::to_string(moduleMetadata.size())
                                            << " bytes)");

        const auto root = Metadata::GetDecls(moduleMetadata.data());
        WithScope<void>(curProgram->Ast()->Scope(), [this, &root] { CreateDecls(root); });
    }

    LOG_METADATA("TOTAL SIZE: " << CalculateMetadataSize(metadata) << " bytes");
}

bool MetadataDeserializationPhase::PerformForProgram(parser::Program *program)
{
    if (!Context()->config->options->IsReadMetadata()) {
        return false;  // make phase failed due to the corresponding compilation option disabled
    }

    Context()->materializeMembers = [this](ir::AstNode *node) {
        if (node->IsClassDefinition()) {
            MaterializeMembers<ir::ClassDefinition, Metadata::ClassDecl>(node->AsClassDefinition());
        } else if (node->IsTSInterfaceDeclaration()) {
            MaterializeMembers<ir::TSInterfaceDeclaration, Metadata::InterfaceDecl>(node->AsTSInterfaceDeclaration());
        }
    };

    WithProgram(program, [this] {
        const auto ctx = Context();
        const auto metadata = curProgram->GetImportInfo().DataFor<parser::CacheType::METADATA>();

        LOG_METADATA("deserializing metadata of program \"" << curProgram->ModuleName() << "\" (" << metadata->size()
                                                            << " modules)");

        ctx->GetChecker()->Initialize(ctx->parserProgram->VarBinder());
        curProgram->PushChecker(ctx->GetChecker());
        curProgram->SetAst(CreateModule());

        LOG_METADATA_NESTING_INC();
        SetupGlobalClass();
        ProcessMetadata(metadata);
        LOG_METADATA_NESTING_DEC();
    });

    return true;
}

}  // namespace ark::es2panda::compiler