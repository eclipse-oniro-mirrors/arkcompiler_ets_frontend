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
#include "schemaMetadataGenerated.h"
#include "compiler/lowering/ets/topLevelStmts/globalClassHandler.h"
#include "flatbuffers/flatbuffers.h"
#include "evaluate/helpers.h"

#include <string>

namespace ark::es2panda::compiler {

using namespace flatbuffers;

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

ir::ETSModule *MetadataDeserializationPhase::CreateModule(parser::Program *program) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto moduleInfo = program->ModuleInfo();
    const auto moduleId = ctx->AllocNode<ir::Identifier>(util::StringView {moduleInfo.moduleName}, allocator);
    const auto module = ctx->AllocNode<ir::ETSModule>(allocator, ArenaVector<ir::Statement *> {allocator->Adapter()},
                                                      moduleId, ir::ModuleFlag::ETSSCRIPT, moduleInfo.lang, program);
    module->SetScope(ArenaAllocator::New<varbinder::ModuleScope>(allocator));
    return module;
}

void MetadataDeserializationPhase::SetupGlobalClassStaticBlock(ir::ClassStaticBlock *staticBlock) const
{
    const auto ctx = Context();
    auto allocator = ctx->Allocator();

    const auto paramScope =
        ArenaAllocator::New<varbinder::FunctionParamScope>(allocator, staticBlock->Parent()->Scope());
    const auto functionScope = ArenaAllocator::New<varbinder::FunctionScope>(allocator, paramScope);
    functionScope->BindParamScope(paramScope);
    paramScope->BindFunctionScope(functionScope);
    staticBlock->Function()->SetScope(functionScope);
}

void MetadataDeserializationPhase::SetupGlobalClass(parser::Program *program) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();

    GlobalClassHandler(ctx).SetupGlobalClass(program);
    program->GlobalClass()->SetScope(ArenaAllocator::New<varbinder::ClassScope>(allocator, program->Ast()->Scope()));

    for (const auto &member : program->GlobalClass()->Body()) {
        if (member->IsClassStaticBlock()) {
            SetupGlobalClassStaticBlock(member->AsClassStaticBlock());
        }
    }

    const auto binderDecl = allocator->New<varbinder::ClassDecl>(program->GlobalClass()->Ident()->Name());
    binderDecl->BindNode(program->GlobalClass());
    program->GlobalClass()->Ident()->SetVariable(
        program->GlobalScope()->AddDecl(allocator, binderDecl, ScriptExtension::ETS));
}

ir::ClassDefinition *MetadataDeserializationPhase::CreateClass(parser::Program *program,
                                                               const util::StringView className) const
{
    const auto classDef = Context()->GetChecker()->AsETSChecker()->CreateClassPrototype(className, program);

    classDef->SetInternalName(classDef->Ident()->Name());
    classDef->Parent()->AddModifier(ir::ModifierFlags::EXPORT);
    program->GlobalScope()->InsertBinding(classDef->Ident()->Name(), classDef->Variable());
    return classDef;
}

ir::ClassProperty *MetadataDeserializationPhase::CreateField(const ir::ClassDefinition *classDef,
                                                             util::StringView fieldName, ir::TypeNode &returnType,
                                                             const ir::ModifierFlags modifiers) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto classScope = classDef->Scope()->AsClassScope();
    const auto isStatic = (modifiers & ir::ModifierFlags::STATIC) != 0;

    const auto varDecl =
        ctx->AllocNode<ir::ClassProperty>(ctx->AllocNode<ir::Identifier>(fieldName, allocator), nullptr, &returnType,
                                          modifiers | ir::ModifierFlags::EXPORT, allocator, false);

    const auto binderDecl = allocator->New<varbinder::PropertyDecl>(varDecl->Id()->Name());
    binderDecl->BindNode(varDecl);
    const auto scopeToAdd = isStatic ? classScope->StaticFieldScope() : classScope->InstanceFieldScope();
    varDecl->Id()->SetVariable(scopeToAdd->AddDecl(allocator, binderDecl, ScriptExtension::ETS));

    return varDecl;
}

ValueParamsInfo MetadataDeserializationPhase::CreateValueParams(
    const Vector<Offset<Metadata::ValueParamDecl>> *fbValueParams, varbinder::Scope *parentScope) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto paramScope = allocator->New<varbinder::FunctionParamScope>(ctx->Allocator(), parentScope);

    if (fbValueParams->size() == 0) {
        return {{}, paramScope};
    }

    const auto checker = ctx->GetChecker()->AsETSChecker();
    auto valueParams = ArenaVector<ir::Expression *>(allocator->Adapter());

    for (const auto &fbValueParam : *fbValueParams) {
        auto id = checker->AllocNode<ir::Identifier>(fbValueParam->name()->string_view(), allocator);
        auto valueParam = checker->AllocNode<ir::ETSParameterExpression>(id, false, allocator);
        valueParam->SetTypeAnnotation(CreateType(fbValueParam->type(), fbValueParam->type_type()));
        id->SetVariable(paramScope->CreateVar<varbinder::ParameterDecl, varbinder::LocalVariable>(
            allocator, id->Name(), varbinder::VariableFlags::NONE, valueParam));
        valueParams.emplace_back(valueParam);
    }

    return {valueParams, paramScope};
}

TypeParamsInfo MetadataDeserializationPhase::CreateTypeParams(
    const Vector<Offset<Metadata::TypeParamDecl>> *fbTypeParams, varbinder::Scope *parentScope) const
{
    ES2PANDA_ASSERT(fbTypeParams->size() != 0);

    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto typeParamScope = ArenaAllocator::New<varbinder::LocalScope>(ctx->Allocator(), parentScope);
    auto typeParams = ArenaVector<ir::TSTypeParameter *>(allocator->Adapter());

    for (const auto &fbTypeParam : *fbTypeParams) {
        auto id = Context()->AllocNode<ir::Identifier>(fbTypeParam->name()->string_view(), Context()->Allocator());
        auto typeParam = Context()->AllocNode<ir::TSTypeParameter>(id, nullptr, nullptr, allocator);
        const auto binderTypeParamDecl = allocator->New<varbinder::TypeParameterDecl>(id->Name());
        binderTypeParamDecl->BindNode(typeParam);
        typeParams.emplace_back(typeParam);
        id->SetVariable(typeParamScope->AddDecl(allocator, binderTypeParamDecl, ScriptExtension::ETS));
    }

    auto typeParamsDecl =
        Context()->AllocNode<ir::TSTypeParameterDeclaration>(std::move(typeParams), typeParams.size());
    typeParamsDecl->SetScope(typeParamScope);

    return {typeParamsDecl, typeParamScope};
}

ir::MethodDefinition *MetadataDeserializationPhase::CreateMethod(const ir::ClassDefinition *classDef,
                                                                 const util::StringView methodName,
                                                                 ir::TypeNode &returnType, FbMethodParams fbParams,
                                                                 MethodOptions options) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto checker = ctx->GetChecker()->AsETSChecker();
    const auto varBinder = ctx->GetChecker()->VarBinder();
    const auto currentScope = varBinder->GetScope();
    const auto classScope = classDef->Scope()->AsClassScope();
    const auto &[fbValueParams, fbTypeParams] = fbParams;
    const auto [flags, modifiers] = options;
    const auto isConstructor = (flags & ir::ScriptFunctionFlags::CONSTRUCTOR) != 0;
    const auto isStatic = (modifiers & ir::ModifierFlags::STATIC) != 0;
    const auto hasTypeParams = fbTypeParams->size() != 0;

    varBinder->ResetAllScopes(varBinder->TopScope(), varBinder->VarScope(), classScope);

    varbinder::Scope *parentScope = classScope;
    ir::TSTypeParameterDeclaration *typeParams = nullptr;

    if (hasTypeParams) {
        const auto [typeParams_, typeParamsScope] = CreateTypeParams(fbTypeParams, parentScope);
        parentScope = typeParamsScope;
        typeParams = typeParams_;
    }

    auto [valueParams, paramsScope] = CreateValueParams(fbValueParams, parentScope);

    const auto methodDef = checker->CreateMethod(methodName, modifiers | ir::ModifierFlags::EXPORT, flags,
                                                 std::move(valueParams), paramsScope, &returnType, nullptr);

    if (hasTypeParams) {
        methodDef->Function()->SetTypeParams(std::move(typeParams));
    }

    const auto binderDecl = allocator->New<varbinder::MethodDecl>(methodDef->Id()->Name());
    binderDecl->BindNode(methodDef);
    const auto scopeToAdd =
        isConstructor || isStatic ? classScope->StaticMethodScope() : classScope->InstanceMethodScope();
    const auto var = scopeToAdd->AddDecl(allocator, binderDecl, ScriptExtension::ETS);

    var->AddFlag(varbinder::VariableFlags::METHOD);
    methodDef->Id()->SetVariable(var);

    varBinder->ResetAllScopes(varBinder->TopScope(), varBinder->VarScope(), currentScope);

    return methodDef;
}

ir::TypeNode *MetadataDeserializationPhase::CreateBuiltinType(const Metadata::BuiltinTypeKind kind) const
{
    return Context()->AllocNode<ir::ETSPrimitiveType>(BUILTIN_PRIMITIVE_TYPES.at(kind), Context()->Allocator());
}

ir::TypeNode *MetadataDeserializationPhase::CreateType(const void *type, const Metadata::Type kind) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();

    switch (kind) {
        case Metadata::Type_Builtin: {
            const auto builtinTypeKind = static_cast<const Metadata::BuiltinType *>(type)->kind();
            if (builtinTypeKind == Metadata::BuiltinTypeKind::BuiltinTypeKind_undefined) {
                return ctx->AllocNode<ir::ETSUndefinedType>(allocator);
            }

            if (builtinTypeKind == Metadata::BuiltinTypeKind::BuiltinTypeKind_void_) {
                return CreateBuiltinType(Metadata::BuiltinTypeKind::BuiltinTypeKind_void_);
            }

            if (builtinTypeKind == Metadata::BuiltinTypeKind::BuiltinTypeKind_string_) {
                auto id = ctx->AllocNode<ir::Identifier>("string", allocator);
                return ctx->AllocNode<ir::ETSTypeReference>(ctx->AllocNode<ir::ETSTypeReferencePart>(id, allocator),
                                                            allocator);
            }
            return CreateBuiltinType(builtinTypeKind);
        }
        case Metadata::Type_Ref: {
            const auto fqname = static_cast<const Metadata::TypeRef *>(type)->fqname()->string_view();
            // Currently resolution for restored AST by metadata is performed, so no need to use fqnames
            auto name = fqname.substr(fqname.find_last_of(".") + 1);
            auto id = ctx->AllocNode<ir::Identifier>(name, allocator);
            return ctx->AllocNode<ir::ETSTypeReference>(ctx->AllocNode<ir::ETSTypeReferencePart>(id, allocator),
                                                        allocator);
        }
        case Metadata::Type_Union: {
            const auto unionType = static_cast<const Metadata::UnionType *>(type);
            ES2PANDA_ASSERT(unionType->components()->size() == unionType->components_type()->size());

            ArenaVector<ir::TypeNode *> constituentTypes;
            auto fbConstituentTypes = unionType->components();
            auto fbConstituentTypeKinds = unionType->components_type();
            for (size_t i = 0; i < fbConstituentTypes->size(); i++) {
                constituentTypes.emplace_back(CreateType(fbConstituentTypes->Get(i),
                                                         static_cast<Metadata::Type>(fbConstituentTypeKinds->Get(i))));
            }

            return ctx->AllocNode<ir::ETSUnionType>(std::move(constituentTypes), allocator);
        }
        case Metadata::Type_NONE:
        case Metadata::Type_Function:
        case Metadata::Type_Tuple:
        case Metadata::Type_StringLiteral:
            ES2PANDA_ASSERT(false);  // Deserialization of types above is not supported yet
            break;
    }
    return nullptr;
}

bool MetadataDeserializationPhase::PerformForProgram(parser::Program *program)
{
    const auto metadata = program->GetImportInfo().DataFor<parser::CacheType::METADATA>();
    const auto root = Metadata::GetRoot(metadata->data());

    Context()->GetChecker()->Initialize(Context()->parserProgram->VarBinder());
    program->SetAst(CreateModule(program));
    SetupGlobalClass(program);

    for (const auto classDecl : *root->classes()) {
        const auto isGlobalClass = classDecl->name()->string_view() == "ETSGLOBAL";
        const auto classDef =
            isGlobalClass ? program->GlobalClass() : CreateClass(program, classDecl->name()->string_view());

        for (const auto methodDecl : *classDecl->methods()) {
            const auto methodName = methodDecl->name()->string_view();
            const auto returnType = CreateType(methodDecl->return_type(), methodDecl->return_type_type());
            const auto modifiers = ir::ModifierFlags::PUBLIC | ir::ModifierFlags::DECLARE |
                                   (methodDecl->is_static() ? ir::ModifierFlags::STATIC : ir::ModifierFlags::NONE);
            const auto isConstructor = methodName == "constructor";
            const auto flags = isConstructor ? ir::ScriptFunctionFlags::CONSTRUCTOR : ir::ScriptFunctionFlags::NONE;

            const auto methodDef =
                CreateMethod(classDef, methodName, *returnType, {methodDecl->value_params(), methodDecl->type_params()},
                             {flags, modifiers});

            classDef->EmplaceBody(methodDef);
            methodDef->SetParent(classDef);
        }

        for (const auto fieldDecl : *classDecl->fields()) {
            const auto fieldName = fieldDecl->name()->string_view();
            const auto type = CreateType(fieldDecl->return_type(), fieldDecl->return_type_type());
            const auto modifiers = ir::ModifierFlags::PUBLIC | ir::ModifierFlags::DECLARE |
                                   (fieldDecl->is_static() ? ir::ModifierFlags::STATIC : ir::ModifierFlags::NONE);

            const auto classPropDecl = CreateField(classDef, fieldName, *type, modifiers);

            classDef->EmplaceBody(classPropDecl);
            classPropDecl->SetParent(classDef);
        }
    }

    return true;
}

}  // namespace ark::es2panda::compiler