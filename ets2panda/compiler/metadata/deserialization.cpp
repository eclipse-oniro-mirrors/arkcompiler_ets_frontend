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
#include "generated/signatures.h"
#include "ir/base/classDefinition.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/overloadDeclaration.h"
#include "ir/expressions/arrayExpression.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/ts/tsEnumDeclaration.h"
#include "ir/ts/tsEnumMember.h"
#include "ir/ts/tsQualifiedName.h"
#include "ir/ts/tsThisType.h"
#include "schemaMetadataGenerated.h"
#include "compiler/lowering/ets/topLevelStmts/globalClassHandler.h"
#include "util/es2pandaMacros.h"
#include "varbinder/ETSBinder.h"
#include "evaluate/helpers.h"
#include "libarkfile/metadata_helper.h"
#include "varbinder/recordTable.h"
#include "utils.h"

#include <algorithm>
#include <cstdlib>
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

namespace {

bool MetadataTypeIncludesUndefined(const void *type, Metadata::Type kind)
{
    switch (kind) {
        case Metadata::Type_Builtin:
            return static_cast<const Metadata::BuiltinType *>(type)->kind() ==
                   Metadata::BuiltinTypeKind::BuiltinTypeKind_undefined;
        case Metadata::Type_Union: {
            const auto *fbUnionType = static_cast<const Metadata::UnionType *>(type);
            const auto *components = fbUnionType->components();
            const auto *componentKinds = fbUnionType->components_type();
            for (size_t i = 0; i < components->size(); i++) {
                if (MetadataTypeIncludesUndefined(components->Get(i),
                                                  static_cast<Metadata::Type>(componentKinds->Get(i)))) {
                    return true;
                }
            }
            return false;
        }
        default:
            return false;
    }
}

bool IsOptionalMetadataAccessor(const Metadata::FunctionDecl *fbMethodDecl)
{
    if (!(fbMethodDecl->is_getter() || fbMethodDecl->is_setter())) {
        return false;
    }

    if (fbMethodDecl->is_getter()) {
        return MetadataTypeIncludesUndefined(fbMethodDecl->return_type(), fbMethodDecl->return_type_type());
    }

    const auto *params = fbMethodDecl->value_params();
    return params != nullptr && params->size() > 0 &&
           MetadataTypeIncludesUndefined(params->Get(0)->type(), params->Get(0)->type_type());
}

bool HasDeclarationNodeKind(const varbinder::Variable *var, bool (ir::AstNode::*check)() const)
{
    return var != nullptr && var->Declaration() != nullptr && var->Declaration()->Node() != nullptr &&
           (var->Declaration()->Node()->*check)();
}

// CC-OFFNXT(G.FUN.01-CPP) solid logic
void RegisterMetadataDirectLocalExport(parser::Program *moduleProg, varbinder::ExportFactStore *store,
                                       util::StringView name, varbinder::Variable *var, const ir::AstNode *origin,
                                       bool addDeclarationAlias)
{
    ES2PANDA_ASSERT(moduleProg != nullptr);
    ES2PANDA_ASSERT(store != nullptr);
    ES2PANDA_ASSERT(!name.Empty());
    ES2PANDA_ASSERT(var != nullptr);
    ES2PANDA_ASSERT(var->Declaration() != nullptr);
    ES2PANDA_ASSERT(var->Declaration()->Node() != nullptr);
    ES2PANDA_ASSERT(origin != nullptr);

    store->AddLocalExport(moduleProg, name, var, origin);
    if (!addDeclarationAlias) {
        return;
    }

    const auto added [[maybe_unused]] = store->AddPendingLocalExportAlias(
        moduleProg, name, name, origin, origin, origin, true, false, varbinder::LocalExportKind::DECLARATION);
    ES2PANDA_ASSERT(added);
}

ir::ClassDefinition *GetMetadataClassDefinition(ir::AstNode *node)
{
    ES2PANDA_ASSERT(node != nullptr);
    if (node->IsClassDeclaration()) {
        return node->AsClassDeclaration()->Definition();
    }
    if (node->IsClassDefinition()) {
        return node->AsClassDefinition();
    }
    return nullptr;
}

void RegisterMetadataGlobalClassExports(parser::Program *scopeProgram, parser::Program *moduleProg,
                                        varbinder::ExportFactStore *store, ir::ClassDefinition *classDef)
{
    ES2PANDA_ASSERT(scopeProgram != nullptr);
    ES2PANDA_ASSERT(scopeProgram->GlobalClassScope() != nullptr);
    ES2PANDA_ASSERT(classDef != nullptr);
    ES2PANDA_ASSERT(classDef->IsGlobal());

    for (auto *member : classDef->Body()) {
        if (member->IsMethodDefinition()) {
            auto *method = member->AsMethodDefinition();
            ES2PANDA_ASSERT(method->Id() != nullptr);
            auto *var = scopeProgram->GlobalClassScope()->StaticMethodScope()->FindLocal(
                method->Id()->Name(), varbinder::ResolveBindingOptions::ALL);
            ES2PANDA_ASSERT(HasDeclarationNodeKind(var, &ir::AstNode::IsMethodDefinition));
            RegisterMetadataDirectLocalExport(moduleProg, store, method->Id()->Name(), var, method, false);
            continue;
        }

        if (member->IsClassProperty()) {
            auto *property = member->AsClassProperty();
            ES2PANDA_ASSERT(property->Id() != nullptr);
            auto *var = property->Id()->Variable();
            ES2PANDA_ASSERT(HasDeclarationNodeKind(var, &ir::AstNode::IsClassProperty));
            RegisterMetadataDirectLocalExport(moduleProg, store, property->Id()->Name(), var, property, false);
        }
    }
}

void RegisterMetadataClassExport(parser::Program *scopeProgram, parser::Program *moduleProg,
                                 varbinder::ExportFactStore *store, ir::ClassDefinition *classDef)
{
    ES2PANDA_ASSERT(scopeProgram != nullptr);
    ES2PANDA_ASSERT(scopeProgram->GlobalScope() != nullptr);
    ES2PANDA_ASSERT(classDef != nullptr);
    ES2PANDA_ASSERT(classDef->Ident() != nullptr);

    if (classDef->IsGlobal()) {
        RegisterMetadataGlobalClassExports(scopeProgram, moduleProg, store, classDef);
        return;
    }

    auto result = scopeProgram->GlobalScope()->Find(classDef->Ident()->Name(), varbinder::ResolveBindingOptions::ALL);
    auto *var = result.variable;
    ES2PANDA_ASSERT(HasDeclarationNodeKind(var, &ir::AstNode::IsClassDefinition));
    RegisterMetadataDirectLocalExport(moduleProg, store, classDef->Ident()->Name(), var, classDef, false);
}

void RegisterMetadataInterfaceExport(parser::Program *scopeProgram, parser::Program *moduleProg,
                                     varbinder::ExportFactStore *store, ir::TSInterfaceDeclaration *interfaceDecl)
{
    ES2PANDA_ASSERT(scopeProgram != nullptr);
    ES2PANDA_ASSERT(scopeProgram->GlobalScope() != nullptr);
    ES2PANDA_ASSERT(interfaceDecl != nullptr);

    auto *id = interfaceDecl->Id();
    ES2PANDA_ASSERT(id != nullptr);
    auto result = scopeProgram->GlobalScope()->Find(id->Name(), varbinder::ResolveBindingOptions::ALL);
    auto *var = result.variable;
    ES2PANDA_ASSERT(var == id->Variable());
    ES2PANDA_ASSERT(HasDeclarationNodeKind(var, &ir::AstNode::IsTSInterfaceDeclaration));
    RegisterMetadataDirectLocalExport(moduleProg, store, id->Name(), var, interfaceDecl, true);
}

void RegisterMetadataTypeAliasExport(parser::Program *moduleProg, varbinder::ExportFactStore *store,
                                     ir::TSTypeAliasDeclaration *typeAliasDecl)
{
    ES2PANDA_ASSERT(typeAliasDecl != nullptr);

    auto *id = typeAliasDecl->Id();
    ES2PANDA_ASSERT(id != nullptr);
    auto *var = id->Variable();
    ES2PANDA_ASSERT(HasDeclarationNodeKind(var, &ir::AstNode::IsTSTypeAliasDeclaration));
    RegisterMetadataDirectLocalExport(moduleProg, store, id->Name(), var, typeAliasDecl, false);
}
void RegisterMetadataAnnotationExport(parser::Program *moduleProg, varbinder::ExportFactStore *store,
                                      ir::AnnotationDeclaration *annotationDecl)
{
    ES2PANDA_ASSERT(annotationDecl != nullptr);

    auto *id = annotationDecl->GetBaseName();
    ES2PANDA_ASSERT(id != nullptr);
    auto *var = id->Variable();
    ES2PANDA_ASSERT(HasDeclarationNodeKind(var, &ir::AstNode::IsAnnotationDeclaration));
    RegisterMetadataDirectLocalExport(moduleProg, store, id->Name(), var, annotationDecl, true);
}

varbinder::ETSBinder *FindMetadataETSBinder(parser::Program *program)
{
    ES2PANDA_ASSERT(program != nullptr);

    for (const auto &[_, varBinder] : program->VarBinders()) {
        if (varBinder != nullptr && varBinder->IsETSBinder()) {
            return varBinder->AsETSBinder();
        }
    }

    ES2PANDA_UNREACHABLE();  // Metadata module program must have an ETS binder
}

void RegisterMetadataImportTarget(public_lib::Context *ctx, varbinder::ExportFactStore *moduleStore,
                                  parser::Program *sourceProgram, const ir::ETSImportDeclaration *importDecl,
                                  parser::Program *depProgram)
{
    ES2PANDA_ASSERT(ctx != nullptr);
    ES2PANDA_ASSERT(moduleStore != nullptr);
    ES2PANDA_ASSERT(sourceProgram != nullptr);
    ES2PANDA_ASSERT(importDecl != nullptr);
    ES2PANDA_ASSERT(depProgram != nullptr);

    auto *const exactDepProgram = ctx->parser->GetImportPathManager()->SearchResolvedExact(importDecl->ImportInfo());
    moduleStore->AddImportTarget(sourceProgram, importDecl, exactDepProgram != nullptr ? exactDepProgram : depProgram);
    moduleStore->AddEffectiveImportTarget(sourceProgram, importDecl, depProgram);
}

class MetadataCheckerVarBinderContext {
public:
    MetadataCheckerVarBinderContext(checker::Checker *checker, varbinder::VarBinder *varBinder) : checker_(checker)
    {
        ES2PANDA_ASSERT(checker_ != nullptr);
        ES2PANDA_ASSERT(varBinder != nullptr);
        prevVarBinder_ = checker_->VarBinder();
        checker_->Initialize(varBinder);
    }

    ~MetadataCheckerVarBinderContext()
    {
        if (prevVarBinder_ != nullptr) {
            checker_->Initialize(prevVarBinder_);
        }
    }

    NO_COPY_SEMANTIC(MetadataCheckerVarBinderContext);
    NO_MOVE_SEMANTIC(MetadataCheckerVarBinderContext);

private:
    checker::Checker *checker_;
    varbinder::VarBinder *prevVarBinder_ {nullptr};
};

}  // namespace

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

void MetadataDeserializationPhase::MarkBuiltinIfNeeded(varbinder::Variable *var, bool isBuiltin) const
{
    if (var == nullptr) {
        return;
    }

    auto *checker = Context()->GetChecker()->AsETSChecker();
    const auto hasGlobalBuiltinId = checker->GetGlobalTypesHolder()->NameToId(var->Name()).has_value();
    if (!isBuiltin && !hasGlobalBuiltinId) {
        return;
    }

    var->AddFlag(varbinder::VariableFlags::BUILTIN_TYPE);
    if (hasGlobalBuiltinId && var->TsType() != nullptr) {
        checker->GetGlobalTypesHolder()->InitializeBuiltin(var->Name(), var->TsType());
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
    const auto isGlobalMember = parent->IsClassDefinition() && parent->AsClassDefinition()->IsGlobal();
    for (const auto fbMethodDecl : methods) {
        const auto methodDecl = CreateMethodDecl(fbMethodDecl, isGlobalMember);
        const auto *func = methodDecl->Function();
        if (func->IsGetter() || func->IsSetter()) {
            const auto sibling = std::find_if(body.begin(), body.end(), [&methodDecl](const ir::AstNode *node) {
                return node->IsMethodDefinition() &&
                       node->AsMethodDefinition()->Id()->Name() == methodDecl->Id()->Name();
            });
            if (sibling != body.end()) {
                methodDecl->SetParent(*sibling);
                LOG_METADATA(methodDecl->Id()->Name() << IrDeclVectorToString(methodDecl->Function()->Params()));
                continue;
            }
        }

        body.emplace_back(methodDecl);
        methodDecl->SetParent(parent);

        LOG_METADATA(methodDecl->Id()->Name() << IrDeclVectorToString(methodDecl->Function()->Params()));
    }
}

void MetadataDeserializationPhase::AddOverloadGroups(const Vector<Offset<Metadata::FunctionDecl>> &methods,
                                                     ArenaVector<ir::AstNode *> &body, ir::AstNode *parent) const
{
    using IndexedName = std::pair<int32_t, std::string_view>;
    std::vector<std::string_view> order;
    std::unordered_map<std::string_view, std::pair<bool, std::vector<IndexedName>>> groups;

    for (const auto fbMethodDecl : methods) {
        const auto *fbGroup = fbMethodDecl->overload_group();
        if (fbGroup == nullptr || fbGroup->size() == 0) {
            continue;
        }

        const auto groupName = fbGroup->string_view();
        auto [it, inserted] = groups.try_emplace(groupName, fbMethodDecl->is_static(), std::vector<IndexedName> {});
        if (inserted) {
            order.emplace_back(groupName);
        }
        it->second.second.emplace_back(fbMethodDecl->overload_group_index(), fbMethodDecl->name()->string_view());
    }

    for (const auto &groupName : order) {
        auto &[isStatic, indexedNames] = groups.at(groupName);
        std::sort(indexedNames.begin(), indexedNames.end(),
                  [](const IndexedName &lhs, const IndexedName &rhs) { return lhs.first < rhs.first; });

        std::vector<std::string_view> overloadedNames;
        overloadedNames.reserve(indexedNames.size());
        for (const auto &[index, name] : indexedNames) {
            overloadedNames.emplace_back(name);
        }

        auto *overloadDecl = CreateOverloadGroupDecl(groupName, isStatic, overloadedNames);
        body.emplace_back(overloadDecl);
        overloadDecl->SetParent(parent);

        LOG_METADATA(overloadDecl->Name());
    }
}

static ir::ModifierFlags ToVarianceModifier(Metadata::TypeParamVariance variance)
{
    switch (variance) {
        case Metadata::TypeParamVariance::TypeParamVariance_IN:
            return ir::ModifierFlags::IN;
        case Metadata::TypeParamVariance::TypeParamVariance_OUT:
            return ir::ModifierFlags::OUT;
        default:
            return ir::ModifierFlags::NONE;
    }
}

static bool HasConstructor(const ArenaVector<ir::AstNode *> &body)
{
    return std::any_of(body.begin(), body.end(), [](const ir::AstNode *node) {
        return node->IsMethodDefinition() && node->AsMethodDefinition()->IsConstructor();
    });
}

static ir::Expression *CreateQualifiedTypeName(public_lib::Context *ctx, ArenaAllocator *allocator,
                                               std::string_view fqname)
{
    const auto createIdentifier = [ctx, allocator](std::string_view name) {
        return ctx->AllocNode<ir::Identifier>(util::StringView {std::string_view {name}}, allocator);
    };

    ir::Expression *typeName = nullptr;
    for (size_t begin = 0; begin < fqname.size();) {
        const auto dot = fqname.find('.', begin);
        const auto end = dot == std::string_view::npos ? fqname.size() : dot;
        ES2PANDA_ASSERT(end > begin);

        auto *const ident = createIdentifier(fqname.substr(begin, end - begin));
        if (typeName == nullptr) {
            typeName = ident;
        } else {
            auto *const qualified = ctx->AllocNode<ir::TSQualifiedName>(typeName, ident, allocator);
            typeName->SetParent(qualified);
            ident->SetParent(qualified);
            typeName = qualified;
        }

        if (dot == std::string_view::npos) {
            break;
        }
        begin = dot + 1;
    }

    ES2PANDA_ASSERT(typeName != nullptr);
    return typeName;
}

class MetadataClassBuilder {
public:
    MetadataClassBuilder(MetadataDeserializationPhase &phase, const Metadata::ClassDecl *fbClassDecl,
                         ir::TSTypeParameterDeclaration *typeParams, varbinder::Scope *parentScope, bool isNested)
        : phase_(phase),
          fbClassDecl_(fbClassDecl),
          typeParams_(typeParams),
          parentScope_(parentScope),
          isNested_(isNested)
    {
    }

    varbinder::Scope *ScopeForClass() const
    {
        return typeParams_ ? typeParams_->Scope() : parentScope_;
    }

    bool IsNested() const
    {
        return isNested_;
    }

private:
    auto *Context() const
    {
        return phase_.Context();
    }

public:
    ir::ClassDefinition *GetOrCreateClassDef() const
    {
        auto const name = fbClassDecl_->name()->string_view();
        if (name == "ETSGLOBAL") {
            return phase_.curProgram->GlobalClass();
        }
        if (!isNested_) {
            auto *const existingVar =
                phase_.curProgram->GlobalScope()->Find(name, varbinder::ResolveBindingOptions::ALL).variable;
            if (existingVar != nullptr) {
                auto *const existingNode = existingVar->Declaration()->Node();
                if (existingNode->IsClassDefinition()) {
                    return existingNode->AsClassDefinition();
                }
            }
            return Context()->GetChecker()->AsETSChecker()->CreateClassPrototype(fbClassDecl_->name()->string_view(),
                                                                                 phase_.curProgram);
        }
        // isNested_
        auto *const staticDeclScope = parentScope_->AsClassScope()->StaticDeclScope();
        auto *const existingNestedVar = staticDeclScope->FindLocal(name, varbinder::ResolveBindingOptions::ALL);
        if (existingNestedVar != nullptr) {
            auto *const existingNestedNode = existingNestedVar->Declaration()->Node();
            if (existingNestedNode->IsClassDefinition()) {
                return existingNestedNode->AsClassDefinition();
            }
        }
        return CreateNestedClassDef(name);
    }

    ir::ClassDefinition *CreateNestedClassDef(std::string_view name) const
    {
        auto *const allocator = Context()->Allocator();
        auto *const classId = Context()->AllocNode<ir::Identifier>(name, allocator);
        auto *const classDefLocal = Context()->AllocNode<ir::ClassDefinition>(
            allocator, classId, ir::ClassDefinitionModifiers::CLASS_DECL | ir::ClassDefinitionModifiers::DECLARATION,
            ir::ModifierFlags::NONE, Language(Language::Id::ETS));
        auto *const classDecl = Context()->AllocNode<ir::ClassDeclaration>(classDefLocal, allocator);
        classDefLocal->SetParent(classDecl);
        auto *const clsScope = ArenaAllocator::New<varbinder::ClassScope>(allocator, parentScope_);
        classDefLocal->SetScope(clsScope);
        clsScope->BindNode(classDefLocal);
        auto *const binderDecl = ArenaAllocator::New<varbinder::ClassDecl>(classId->Name());
        binderDecl->BindNode(classDefLocal);
        auto *const staticDeclScope = parentScope_->AsClassScope()->StaticDeclScope();
        auto *const var = staticDeclScope->AddDecl(allocator, binderDecl, ScriptExtension::ETS);
        classId->SetVariable(var);
        classDefLocal->SetVariable(var);
        var->AddFlag(varbinder::VariableFlags::CLASS);
        return classDefLocal;
    }

    void AddNestedClassStaticProperty(ir::ClassDefinition *classDef) const
    {
        if (classDef->Variable() == nullptr || parentScope_->Node() == nullptr ||
            !parentScope_->Node()->IsClassDefinition()) {
            return;
        }

        auto *const parentVar = parentScope_->Node()->AsClassDefinition()->Variable();
        if (parentVar == nullptr || parentVar->TsType() == nullptr || !parentVar->TsType()->IsETSObjectType()) {
            return;
        }

        parentVar->TsType()->AsETSObjectType()->AddProperty<checker::PropertyType::STATIC_DECL>(
            classDef->Variable()->AsLocalVariable());
    }

    void SetupClassDecl(ir::ClassDefinition *classDef)
    {
        const auto isSyntheticGlobalClass = fbClassDecl_->name()->string_view() == "ETSGLOBAL";
        if (!isSyntheticGlobalClass && !isNested_ && classDef->Variable() != nullptr) {
            classDef->Variable()->AddFlag(varbinder::VariableFlags::CLASS);
        }
        classDef->Scope()->SetParent(typeParams_ ? typeParams_->Scope() : phase_.Scope());
        classDef->Scope()->BindNode(classDef);
        classDef->AddModifier(ir::ModifierFlags::DECLARE);
        if (fbClassDecl_->is_final()) {
            classDef->AddModifier(ir::ModifierFlags::FINAL);
        }
        if (fbClassDecl_->is_abstract()) {
            classDef->AddModifier(ir::ModifierFlags::ABSTRACT);
        }
        if (!isSyntheticGlobalClass) {
            classDef->Parent()->AddModifier(ir::ModifierFlags::EXPORT);
        }
        phase_.MarkBuiltinIfNeeded(classDef->Variable(), fbClassDecl_->is_builtin());

        if (fbClassDecl_->is_namespace()) {
            classDef->SetNamespaceTransformed();
            if (classDef->Variable() != nullptr) {
                classDef->Variable()->AddFlag(varbinder::VariableFlags::NAMESPACE);
            }
        }

        if (typeParams_) {
            classDef->SetTypeParams(typeParams_);
            classDef->TypeParams()->SetParent(classDef);
            if (auto *type = classDef->Variable() != nullptr ? classDef->Variable()->TsType() : nullptr;
                type != nullptr && type->IsETSObjectType()) {
                Context()->GetChecker()->AsETSChecker()->CreateTypeForClassOrInterfaceTypeParameters(
                    type->AsETSObjectType());
            }
        }
    }

    void AddClassHeritage(ir::ClassDefinition *classDef) const
    {
        if (fbClassDecl_->extended_class_type() != Metadata::Type::Type_NONE) {
            auto *superType = phase_.CreateType(fbClassDecl_->extended_class(), fbClassDecl_->extended_class_type());
            classDef->SetSuper(superType);
        }

        if (fbClassDecl_->implemented_interfaces() != nullptr) {
            for (size_t i = 0; i < fbClassDecl_->implemented_interfaces()->size(); i++) {
                auto *interfaceType =
                    phase_.CreateType(fbClassDecl_->implemented_interfaces()->Get(i),
                                      static_cast<Metadata::Type>(fbClassDecl_->implemented_interfaces_type()->Get(i)));
                auto *implements = Context()->AllocNode<ir::TSClassImplements>(interfaceType);
                implements->SetParent(classDef);
                classDef->EmplaceImplements(implements);
            }
        }
    }

private:
    MetadataDeserializationPhase &phase_;
    const Metadata::ClassDecl *fbClassDecl_;
    ir::TSTypeParameterDeclaration *typeParams_;
    varbinder::Scope *parentScope_;
    bool isNested_;
};

bool MetadataDeserializationPhase::DetermineIsStringEnum(const Metadata::ClassDecl *fbClassDecl) const
{
    const auto *props = fbClassDecl->properties();
    const auto propCount = (props != nullptr) ? props->size() : 0;
    const auto enumKind = fbClassDecl->enum_kind();
    if (enumKind != Metadata::EnumKind_NONE) {
        return enumKind == Metadata::EnumKind_STRING;
    }

    if (propCount > 0) {
        const auto *firstProp = props->Get(0);
        const auto *returnType = firstProp->return_type_as_Builtin();
        return firstProp->return_type_type() == Metadata::Type_Builtin && returnType != nullptr &&
               returnType->kind() == Metadata::BuiltinTypeKind_string_;
    }

    return false;
}

const flatbuffers::String *MetadataDeserializationPhase::GetEnumValue(const Metadata::ClassDecl *fbClassDecl,
                                                                      uint32_t index) const
{
    const auto *enumValues = fbClassDecl->enum_values();
    return (enumValues != nullptr && index < enumValues->size()) ? enumValues->Get(index) : nullptr;
}

ir::TSEnumMember *FindOrigEnumMember(ir::ClassDefinition *classDef, util::StringView memberName)
{
    ES2PANDA_ASSERT(classDef != nullptr);
    auto *origEnumDecl = classDef->OrigEnumDecl();
    ES2PANDA_ASSERT(origEnumDecl != nullptr);

    for (auto *member : origEnumDecl->Members()) {
        auto *enumMember = member->AsTSEnumMember();
        ES2PANDA_ASSERT(enumMember->Key() != nullptr);
        ES2PANDA_ASSERT(enumMember->Key()->IsIdentifier());
        if (enumMember->Key()->AsIdentifier()->Name() == memberName) {
            return enumMember;
        }
    }

    ES2PANDA_UNREACHABLE();
}

ArenaVector<ir::AstNode *> MetadataDeserializationPhase::CreateEnumMemberNodes(const Metadata::ClassDecl *fbClassDecl,
                                                                               bool isStringEnum) const
{
    auto *ctx = Context();
    auto *allocator = ctx->Allocator();
    const auto *props = fbClassDecl->properties();
    const auto propCount = (props != nullptr) ? props->size() : 0;

    ArenaVector<ir::AstNode *> enumMembers(allocator->Adapter());
    for (uint32_t i = 0; i < propCount; i++) {
        const auto name = props->Get(i)->name()->string_view();
        auto *key = ctx->AllocNode<ir::Identifier>(name, allocator);
        ir::Expression *init = nullptr;

        const auto *v = GetEnumValue(fbClassDecl, i);
        if (isStringEnum) {
            init = ctx->AllocNode<ir::StringLiteral>(v != nullptr ? v->string_view() : name);
        } else {
            int64_t num = (v != nullptr) ? std::strtoll(v->c_str(), nullptr, 10) : static_cast<int64_t>(i);
            init = ctx->AllocNode<ir::NumberLiteral>(lexer::Number(num));
        }

        enumMembers.emplace_back(ctx->AllocNode<ir::TSEnumMember>(key, init));
    }

    return enumMembers;
}

ir::TSEnumDeclaration *MetadataDeserializationPhase::CreateOrigEnumDecl(const Metadata::ClassDecl *fbClassDecl,
                                                                        ir::ClassDefinition *classDef,
                                                                        ArenaVector<ir::AstNode *> &&enumMembers) const
{
    auto *ctx = Context();
    auto *allocator = ctx->Allocator();

    ir::TypeNode *underlyingType = nullptr;
    if (fbClassDecl->enum_underlying_type_type() != Metadata::Type::Type_NONE) {
        underlyingType = CreateType(fbClassDecl->enum_underlying_type(), fbClassDecl->enum_underlying_type_type());
    }

    auto *origEnumDecl = ctx->AllocNode<ir::TSEnumDeclaration>(
        allocator, ctx->AllocNode<ir::Identifier>(classDef->Ident()->Name(), allocator), std::move(enumMembers),
        ir::TSEnumDeclaration::ConstructorFlags {false}, underlyingType, Language(Language::Id::ETS));
    origEnumDecl->SetScope(allocator->New<varbinder::LocalScope>(allocator, classDef->Scope()));
    return origEnumDecl;
}

ir::ClassProperty *MetadataDeserializationPhase::AddSyntheticEnumProperty(
    std::string_view propName, const Metadata::ClassDecl *fbClassDecl, ir::ClassDefinition *classDef,
    std::function<ir::Expression *(uint32_t)> &&makeElement) const
{
    auto *ctx = Context();
    auto *allocator = ctx->Allocator();
    auto &body = classDef->BodyForUpdate();
    const auto *props = fbClassDecl->properties();
    const auto propCount = (props != nullptr) ? props->size() : 0;

    ArenaVector<ir::Expression *> elements(allocator->Adapter());
    for (uint32_t i = 0; i < propCount; i++) {
        elements.emplace_back(makeElement(i));
    }

    auto *arr = ctx->AllocNode<ir::ArrayExpression>(std::move(elements), allocator);
    auto *ident = ctx->AllocNode<ir::Identifier>(propName, allocator);
    auto *prop = ctx->AllocNode<ir::ClassProperty>(
        ident, arr, nullptr, ir::ModifierFlags::PRIVATE | ir::ModifierFlags::STATIC | ir::ModifierFlags::DECLARE,
        allocator, false);

    prop->SetParent(classDef);
    auto *binderDecl = EAllocator::New<varbinder::PropertyDecl>(ident->Name());
    binderDecl->BindNode(prop);
    auto *var = Scope()->AsClassScope()->StaticFieldScope()->AddDecl(allocator, binderDecl, ScriptExtension::ETS);
    var->AddFlag(varbinder::VariableFlags::PROPERTY);
    var->AddFlag(varbinder::VariableFlags::STATIC);
    ident->SetVariable(var);
    body.emplace_back(prop);

    return prop;
}

void MetadataDeserializationPhase::AddEnumMembers(const Metadata::ClassDecl *fbClassDecl, ir::ClassDefinition *classDef)
{
    const bool isStringEnum = DetermineIsStringEnum(fbClassDecl);

    classDef->SetModifiers(classDef->Modifiers() |
                           (isStringEnum ? ir::ClassDefinitionModifiers::STRING_ENUM_TRANSFORMED
                                         : ir::ClassDefinitionModifiers::NUMERIC_ENUM_TRANSFORMED));

    auto enumMembers = CreateEnumMemberNodes(fbClassDecl, isStringEnum);
    auto *origEnumDecl = CreateOrigEnumDecl(fbClassDecl, classDef, std::move(enumMembers));
    classDef->SetOrigEnumDecl(origEnumDecl);

    // CC-OFFNXT(G.FMT.14-CPP) project code style
    AddSyntheticEnumProperty("#NamesArray", fbClassDecl, classDef, [this, fbClassDecl](uint32_t i) -> ir::Expression * {
        const auto *props = fbClassDecl->properties();
        return Context()->AllocNode<ir::StringLiteral>(props->Get(i)->name()->string_view());
    });

    if (isStringEnum) {
        AddSyntheticEnumProperty("#StringValuesArray", fbClassDecl, classDef,
                                 // CC-OFFNXT(G.FMT.14-CPP) project code style
                                 [this, fbClassDecl](uint32_t i) -> ir::Expression * {
                                     const auto *v = GetEnumValue(fbClassDecl, i);
                                     const auto *props = fbClassDecl->properties();
                                     return Context()->AllocNode<ir::StringLiteral>(
                                         v != nullptr ? v->string_view() : props->Get(i)->name()->string_view());
                                 });  // CC-OFF(G.FMT.02-CPP) project code style
    } else {
        AddSyntheticEnumProperty(
            // CC-OFFNXT(G.FMT.14-CPP) project code style
            "#ValuesArray", fbClassDecl, classDef, [this, fbClassDecl](uint32_t i) -> ir::Expression * {
                const auto *v = GetEnumValue(fbClassDecl, i);
                int64_t num = (v != nullptr) ? std::strtoll(v->c_str(), nullptr, 10) : static_cast<int64_t>(i);
                return Context()->AllocNode<ir::NumberLiteral>(lexer::Number(num));
            });
    }
}

void MetadataDeserializationPhase::AddClassMembers(const Metadata::ClassDecl *fbClassDecl,
                                                   ir::ClassDefinition *classDef)
{
    if (fbClassDecl->enum_kind() != Metadata::EnumKind_NONE) {
        AddEnumMembers(fbClassDecl, classDef);
    }

    AddMethods(*fbClassDecl->methods(), classDef->BodyForUpdate(), classDef);
    EnsureConstructorExists(classDef);
    AddProperties(fbClassDecl, classDef);
    AddOverloadGroups(*fbClassDecl->methods(), classDef->BodyForUpdate(), classDef);
    AddNestedDeclarations(fbClassDecl, classDef);
}

void MetadataDeserializationPhase::EnsureConstructorExists(ir::ClassDefinition *classDef)
{
    if (HasConstructor(classDef->Body())) {
        return;
    }

    auto *ctor = Context()->GetChecker()->AsETSChecker()->CreateMethod(
        "constructor", ir::ModifierFlags::PUBLIC | ir::ModifierFlags::DECLARE | ir::ModifierFlags::EXPORT,
        ir::ScriptFunctionFlags::CONSTRUCTOR, {},
        EAllocator::New<varbinder::FunctionParamScope>(Context()->Allocator(), Scope()), nullptr, nullptr,
        ir::MethodDefinitionKind::CONSTRUCTOR);

    const auto binderDecl = EAllocator::New<varbinder::MethodDecl>(ctor->Id()->Name());
    binderDecl->BindNode(ctor);
    auto *var =
        Scope()->AsClassScope()->StaticMethodScope()->AddDecl(Context()->Allocator(), binderDecl, ScriptExtension::ETS);
    if (var == nullptr) {
        var = Scope()->AsClassScope()->StaticMethodScope()->Find(binderDecl->Name()).variable;
        var->Declaration()->Node()->AsMethodDefinition()->OverloadsForUpdate().emplace_back(ctor);
    }
    var->AddFlag(varbinder::VariableFlags::METHOD);
    ctor->Id()->SetVariable(var);
    classDef->BodyForUpdate().emplace_back(ctor);
    ctor->SetParent(classDef);
}

void MetadataDeserializationPhase::AddProperties(const Metadata::ClassDecl *fbClassDecl, ir::ClassDefinition *classDef)
{
    for (const auto fbPropDecl : *fbClassDecl->properties()) {
        const auto propDecl = CreatePropertyDecl(fbPropDecl, classDef->IsGlobal());
        if (fbClassDecl->enum_kind() != Metadata::EnumKind_NONE) {
            propDecl->SetOrigEnumMember(FindOrigEnumMember(classDef, propDecl->Id()->Name()));
        }

        classDef->EmplaceBody(propDecl);
        propDecl->SetParent(classDef);

        LOG_METADATA(propDecl->Id()->Name() << ": " << IrDeclToString(propDecl->TypeAnnotation()));
    }
}

void MetadataDeserializationPhase::AddNestedDeclarations(const Metadata::ClassDecl *fbClassDecl,
                                                         ir::ClassDefinition *classDef)
{
    if (!fbClassDecl->decls()) {
        return;
    }

    auto &classBody = classDef->BodyForUpdate();
    const auto decls = CreateDecls(fbClassDecl->decls(), true);
    classBody.reserve(classBody.size() + decls.size());
    classBody.insert(classBody.end(), decls.begin(), decls.end());

    auto *const staticDeclScope = classDef->Scope()->AsClassScope()->StaticDeclScope();
    for (auto *const decl : decls) {
        decl->SetParent(classDef);
        ir::Identifier *id = GetDeclarationIdentifier(decl);
        if (id != nullptr && id->Variable() != nullptr) {
            staticDeclScope->InsertBinding(id->Name(), id->Variable());
        }
    }
}

ir::Identifier *MetadataDeserializationPhase::GetDeclarationIdentifier(ir::AstNode *decl)
{
    if (decl->IsClassDeclaration()) {
        return decl->AsClassDeclaration()->Definition()->Ident();
    }
    if (decl->IsClassDefinition()) {
        return decl->AsClassDefinition()->Ident();
    }
    if (decl->IsTSInterfaceDeclaration()) {
        return decl->AsTSInterfaceDeclaration()->Id();
    }
    return nullptr;
}

void MetadataDeserializationPhase::AddInterfaceProperties(const Metadata::InterfaceDecl *fbInterfaceDecl,
                                                          ir::TSInterfaceDeclaration *interfaceDecl) const
{
    if (fbInterfaceDecl->properties() == nullptr) {
        return;
    }

    for (const auto fbPropDecl : *fbInterfaceDecl->properties()) {
        auto *propDecl = CreatePropertyDecl(fbPropDecl, false);
        interfaceDecl->Body()->Body().emplace_back(propDecl);
        propDecl->SetParent(interfaceDecl->Body());

        if (!propDecl->IsOptionalDeclaration()) {
            continue;
        }

        for (auto *member : interfaceDecl->Body()->Body()) {
            if (!member->IsMethodDefinition()) {
                continue;
            }

            auto *method = member->AsMethodDefinition();
            if (!(method->IsGetter() || method->IsSetter()) || method->Id()->Name() != propDecl->Id()->Name()) {
                continue;
            }

            method->AddModifier(ir::ModifierFlags::OPTIONAL);
            if (method->OriginalNode() == nullptr) {
                method->SetOriginalNode(propDecl);
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
    auto scopeCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(etsBinder, curProgram->GlobalScope());
    if constexpr (std::is_same_v<T, ir::TSInterfaceDeclaration>) {
        etsBinder->BuildInterfaceDeclaration(node);
    } else {
        etsBinder->BuildClassDefinition(node);
    }
}

template <typename T>
constexpr auto &MetadataDeserializationPhase::GetLazyMembers()
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
        auto &lazyMembers = GetLazyMembers<T>();
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
                AddMethods(*fbDecl->methods(), node->Body()->Body(), node->Body());
                AddInterfaceProperties(fbDecl, node);
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
        auto *typeAnnotation = CreateType(fbValueParam->type(), fbValueParam->type_type());
        ir::ETSParameterExpression *valueParam = nullptr;
        if (fbValueParam->is_rest()) {
            auto *spread = ctx->AllocNode<ir::SpreadElement>(ir::AstNodeType::REST_ELEMENT, allocator, id);
            spread->SetTypeAnnotation(typeAnnotation);
            typeAnnotation->SetParent(spread);
            id->SetParent(spread);
            valueParam = ctx->AllocNode<ir::ETSParameterExpression>(spread, nullptr, allocator);
        } else {
            valueParam = ctx->AllocNode<ir::ETSParameterExpression>(id, false, allocator);
            valueParam->SetTypeAnnotation(typeAnnotation);
            typeAnnotation->SetParent(id);
        }
        id->SetVariable(varbinder::FunctionParamScope::CreateVar<varbinder::ParameterDecl, varbinder::LocalVariable>(
            allocator, id->Name(), varbinder::VariableFlags::NONE, valueParam));
        paramScope->Params().emplace_back(id->Variable()->AsLocalVariable());
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
    const auto checker = ctx->GetChecker()->AsETSChecker();
    const auto typeParamScope = ArenaAllocator::New<varbinder::LocalScope>(allocator, Scope());
    auto scopeCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(ctx->GetChecker()->VarBinder(), typeParamScope);
    ArenaVector<ir::TSTypeParameter *> typeParams;

    for (const auto &fbTypeParam : *fbTypeParams) {
        auto id = ctx->AllocNode<ir::Identifier>(fbTypeParam->name()->string_view(), allocator);
        auto typeParam = ctx->AllocNode<ir::TSTypeParameter>(id, nullptr, nullptr, allocator);
        typeParam->AddModifier(ToVarianceModifier(fbTypeParam->variance()));

        const auto checkerTypeParam = allocator->New<checker::ETSTypeParameter>();
        checkerTypeParam->AddTypeFlag(checker::TypeFlag::GENERIC);
        checkerTypeParam->SetDeclNode(typeParam);
        const auto binderTypeParamDecl = EAllocator::New<varbinder::TypeParameterDecl>(id->Name());
        binderTypeParamDecl->BindNode(typeParam);
        typeParams.emplace_back(typeParam);
        const auto var = typeParamScope->AddDecl(allocator, binderTypeParamDecl, ScriptExtension::ETS);
        checkerTypeParam->SetVariable(var);
        checkerTypeParam->SetConstraintType(checker->GlobalETSAnyType());
        var->SetTsType(checkerTypeParam);
        id->SetVariable(var);

        const auto attachType = [this, typeParam](const void *fbType, Metadata::Type fbTypeKind, auto setter) {
            if (fbTypeKind == Metadata::Type::Type_NONE) {
                return;
            }
            auto *type = CreateType(fbType, fbTypeKind);
            (typeParam->*setter)(type);
            type->SetParent(typeParam);
        };
        attachType(fbTypeParam->constraint(), fbTypeParam->constraint_type(), &ir::TSTypeParameter::SetConstraint);
        attachType(fbTypeParam->default_type(), fbTypeParam->default_type_type(), &ir::TSTypeParameter::SetDefaultType);
    }

    const auto typeParamsDecl =
        ctx->AllocNode<ir::TSTypeParameterDeclaration>(std::move(typeParams), typeParams.size());
    for (auto *param : typeParamsDecl->Params()) {
        param->SetParent(typeParamsDecl);
    }
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
        auto *ident = ctx->AllocNode<ir::Identifier>(Signatures::BUILTIN_OBJECT_CLASS, allocator);
        auto *part = ctx->AllocNode<ir::ETSTypeReferencePart>(ident, allocator);
        auto *typeRef = ctx->AllocNode<ir::ETSTypeReference>(part, allocator);
        ident->SetParent(part);
        part->SetParent(typeRef);
        return typeRef;
    }

    if (kind == Metadata::BuiltinTypeKind::BuiltinTypeKind_string_) {
        auto *ident = ctx->AllocNode<ir::Identifier>("string", allocator);
        auto *part = ctx->AllocNode<ir::ETSTypeReferencePart>(ident, allocator);
        auto *typeRef = ctx->AllocNode<ir::ETSTypeReference>(part, allocator);
        ident->SetParent(part);
        part->SetParent(typeRef);
        return typeRef;
    }

    return ctx->AllocNode<ir::ETSPrimitiveType>(BUILTIN_PRIMITIVE_TYPES.at(kind), allocator);
}

ir::TypeNode *MetadataDeserializationPhase::CreateRefType(const Metadata::TypeRef *fbRefType) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto fqname = fbRefType->fqname()->string_view();
    auto *typeName = CreateQualifiedTypeName(ctx, allocator, fqname);
    if (typeName->IsIdentifier()) {
        auto *const variable =
            Scope()->Find(typeName->AsIdentifier()->Name(), varbinder::ResolveBindingOptions::ALL).variable;
        if (variable != nullptr) {
            typeName->AsIdentifier()->SetVariable(variable);
        }
    }

    auto *part = ctx->AllocNode<ir::ETSTypeReferencePart>(typeName, allocator);
    const auto typeRef = ctx->AllocNode<ir::ETSTypeReference>(part, allocator);
    typeName->SetParent(part);
    part->SetParent(typeRef);
    typeRef->SetForceAllowUnsafeVariance(fbRefType->allow_unsafe_variance());

    if (fbRefType->type_args() && fbRefType->type_args()->size() > 0) {
        const auto fbTypeArgs = fbRefType->type_args();
        const auto fbTypeArgKinds = fbRefType->type_args_type();
        ArenaVector<ir::TypeNode *> typeArgs;
        for (size_t i = 0; i < fbTypeArgs->size(); i++) {
            typeArgs.emplace_back(CreateType(fbTypeArgs->Get(i), static_cast<Metadata::Type>(fbTypeArgKinds->Get(i))));
        }
        auto *typeParams = ctx->AllocNode<ir::TSTypeParameterInstantiation>(std::move(typeArgs));
        typeRef->Part()->SetTypeParams(typeParams);
        typeParams->SetParent(typeRef->Part());
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
    const auto allocator = ctx->Allocator();
    const auto elementType = CreateType(fbArrayType->element_type(), fbArrayType->element_type_type());
    ArenaVector<ir::TypeNode *> typeArgs(allocator->Adapter());
    typeArgs.emplace_back(elementType);

    auto *typeParams = ctx->AllocNode<ir::TSTypeParameterInstantiation>(std::move(typeArgs));
    auto *typeName = ctx->AllocNode<ir::Identifier>(
        fbArrayType->is_value_array() ? "ValueArray" : (fbArrayType->is_readonly() ? "ReadonlyArray" : "FixedArray"),
        allocator);
    auto *typeRefPart = ctx->AllocNode<ir::ETSTypeReferencePart>(typeName, typeParams, nullptr, allocator);
    auto *typeRef = ctx->AllocNode<ir::ETSTypeReference>(typeRefPart, allocator);
    typeName->SetParent(typeRefPart);
    typeParams->SetParent(typeRefPart);
    typeRefPart->SetParent(typeRef);
    return typeRef;
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
        ir::ETSParameterExpression *param = nullptr;
        varbinder::VariableFlags flags = varbinder::VariableFlags::NONE;
        if (fbParam->is_rest()) {
            auto *spread = ctx->AllocNode<ir::SpreadElement>(ir::AstNodeType::REST_ELEMENT, allocator, paramId);
            auto *typeAnnotation = paramId->TypeAnnotation();
            spread->SetTypeAnnotation(typeAnnotation);
            if (typeAnnotation != nullptr) {
                typeAnnotation->SetParent(spread);
            }
            paramId->SetParent(spread);
            param = ctx->AllocNode<ir::ETSParameterExpression>(spread, nullptr, allocator);
        } else {
            param = ctx->AllocNode<ir::ETSParameterExpression>(paramId, false, allocator);
            if (fbParam->is_optional()) {
                param->SetOptional(true);
                flags |= varbinder::VariableFlags::OPTIONAL;
            }
        }
        paramId->SetVariable(
            varbinder::FunctionParamScope::CreateVar<varbinder::ParameterDecl, varbinder::LocalVariable>(
                allocator, paramId->Name(), flags, param));
        paramScope->Params().emplace_back(paramId->Variable()->AsLocalVariable());
        params.emplace_back(param);
    }

    const auto returnType = CreateType(fbFunctionType->return_type(), fbFunctionType->return_type_type());
    const auto funcType = ctx->AllocNode<ir::ETSFunctionType>(
        ir::FunctionSignature(nullptr, std::move(params), returnType, fbFunctionType->has_receiver()),
        ir::ScriptFunctionFlags::NONE, allocator);
    funcType->SetScope(paramScope);
    if (returnType != nullptr) {
        returnType->SetParent(funcType);
    }
    for (auto *param : funcType->Params()) {
        param->SetParent(funcType);
    }

    return funcType;
}

ir::TypeNode *MetadataDeserializationPhase::CreatePartialType(const Metadata::PartialType *fbPartialType) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    auto *typeName = ctx->AllocNode<ir::Identifier>(compiler::Signatures::PARTIAL_TYPE_NAME, allocator);
    auto *part = ctx->AllocNode<ir::ETSTypeReferencePart>(typeName, allocator);
    auto *typeRef = ctx->AllocNode<ir::ETSTypeReference>(part, allocator);
    typeName->SetParent(part);
    part->SetParent(typeRef);

    const auto innerType = CreateType(fbPartialType->inner_type(), fbPartialType->inner_type_type());
    ArenaVector<ir::TypeNode *> typeArgs(allocator->Adapter());
    typeArgs.emplace_back(innerType);
    auto *typeParams = ctx->AllocNode<ir::TSTypeParameterInstantiation>(std::move(typeArgs));
    typeRef->Part()->SetTypeParams(typeParams);
    typeParams->SetParent(typeRef->Part());

    return typeRef;
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
        case Metadata::Type_Partial: {
            return CreatePartialType(static_cast<const Metadata::PartialType *>(type));
        }
        case Metadata::Type_This: {
            return Context()->AllocNode<ir::TSThisType>(Context()->Allocator());
        }
        case Metadata::Type_NONE:
            ES2PANDA_UNREACHABLE();  // Deserialization of other types is not supported yet
    }
    return nullptr;
}

ir::MethodDefinition *MetadataDeserializationPhase::RegisterMethodInScope(ir::MethodDefinition *methodDef,
                                                                          bool isStatic) const
{
    const auto ctx = Context();
    const auto binderDecl =
        EAllocator::New<varbinder::FunctionDecl>(ctx->Allocator(), methodDef->Id()->Name(), methodDef);
    const auto scopeToAdd =
        isStatic ? Scope()->AsClassScope()->StaticMethodScope() : Scope()->AsClassScope()->InstanceMethodScope();
    auto *var = scopeToAdd->AddDecl(ctx->Allocator(), binderDecl, ScriptExtension::ETS);
    if (var == nullptr) {
        var = scopeToAdd->Find(binderDecl->Name()).variable;
        var->Declaration()->Node()->AsMethodDefinition()->OverloadsForUpdate().emplace_back(methodDef);
    }

    var->AddFlag(varbinder::VariableFlags::METHOD);
    methodDef->Id()->SetVariable(var);
    return methodDef;
}

ir::MethodDefinition *MetadataDeserializationPhase::CreateMethodDecl(const Metadata::FunctionDecl *fbMethodDecl,
                                                                     bool isGlobalMember)
{
    const auto ctx = Context();
    const auto fbValueParams = fbMethodDecl->value_params();
    const auto fbTypeParams = fbMethodDecl->type_params();
    const auto methodName = fbMethodDecl->name()->string_view();
    const auto modifiers =
        (fbMethodDecl->is_protected() ? ir::ModifierFlags::PROTECTED : ir::ModifierFlags::PUBLIC) |
        ir::ModifierFlags::DECLARE | (isGlobalMember ? ir::ModifierFlags::EXPORT : ir::ModifierFlags::NONE) |
        (fbMethodDecl->is_static() ? ir::ModifierFlags::STATIC : ir::ModifierFlags::NONE) |
        (IsOptionalMetadataAccessor(fbMethodDecl) ? ir::ModifierFlags::OPTIONAL : ir::ModifierFlags::NONE) |
        (fbMethodDecl->is_final() ? ir::ModifierFlags::FINAL : ir::ModifierFlags::NONE) |
        (fbMethodDecl->is_native() ? ir::ModifierFlags::NATIVE : ir::ModifierFlags::NONE) |
        (fbMethodDecl->is_abstract() ? ir::ModifierFlags::ABSTRACT : ir::ModifierFlags::NONE);
    const auto isConstructor = methodName == "constructor";
    const auto flags = isConstructor               ? ir::ScriptFunctionFlags::CONSTRUCTOR
                       : fbMethodDecl->is_getter() ? ir::ScriptFunctionFlags::GETTER
                       : fbMethodDecl->is_setter() ? ir::ScriptFunctionFlags::SETTER
                                                   : ir::ScriptFunctionFlags::NONE;
    const auto typeParams = CreateTypeParams(fbTypeParams);

    using SigInfo = std::pair<ValueParamsInfo, ir::TypeNode *>;
    const auto [valueParamsInfo, returnType] = WithScope<SigInfo>(
        typeParams ? typeParams->Scope() : Scope(), [this, &fbValueParams, &fbMethodDecl]() -> SigInfo {
            return {CreateValueParams(fbValueParams),
                    CreateType(fbMethodDecl->return_type(), fbMethodDecl->return_type_type())};
        });
    auto [valueParams, paramsScope] = valueParamsInfo;
    const auto methodKind = isConstructor               ? ir::MethodDefinitionKind::CONSTRUCTOR
                            : fbMethodDecl->is_getter() ? ir::MethodDefinitionKind::GET
                            : fbMethodDecl->is_setter() ? ir::MethodDefinitionKind::SET
                                                        : ir::MethodDefinitionKind::METHOD;
    const auto methodDef = ctx->GetChecker()->AsETSChecker()->CreateMethod(
        fbMethodDecl->name()->string_view(), modifiers, flags, std::move(valueParams), paramsScope, returnType, nullptr,
        methodKind);
    if (methodDef->Function()->ReturnTypeAnnotation() != nullptr) {
        methodDef->Function()->ReturnTypeAnnotation()->SetParent(methodDef->Function());
    }
    for (auto *param : methodDef->Function()->Params()) {
        param->SetParent(methodDef->Function());
    }
    if (typeParams) {
        typeParams->SetParent(methodDef);
        methodDef->Function()->SetTypeParams(typeParams);
    }

    return RegisterMethodInScope(methodDef, isConstructor || fbMethodDecl->is_static());
}

ir::ClassProperty *MetadataDeserializationPhase::CreatePropertyDecl(const Metadata::PropertyDecl *fbPropDecl,
                                                                    bool isGlobalMember) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();

    const auto propName = fbPropDecl->name()->string_view();
    const auto type = CreateType(fbPropDecl->return_type(), fbPropDecl->return_type_type());
    const auto isOptional = MetadataTypeIncludesUndefined(fbPropDecl->return_type(), fbPropDecl->return_type_type());
    const auto modifiers = (fbPropDecl->is_protected() ? ir::ModifierFlags::PROTECTED : ir::ModifierFlags::PUBLIC) |
                           (isGlobalMember ? ir::ModifierFlags::EXPORT : ir::ModifierFlags::NONE) |
                           ir::ModifierFlags::DECLARE |
                           (fbPropDecl->is_static() ? ir::ModifierFlags::STATIC : ir::ModifierFlags::NONE) |
                           (isOptional ? ir::ModifierFlags::OPTIONAL : ir::ModifierFlags::NONE) |
                           (fbPropDecl->is_readonly() ? ir::ModifierFlags::READONLY : ir::ModifierFlags::NONE);
    const auto propDecl = ctx->AllocNode<ir::ClassProperty>(ctx->AllocNode<ir::Identifier>(propName, allocator),
                                                            nullptr, type, modifiers, allocator, false);
    if (type != nullptr) {
        type->SetParent(propDecl);
    }

    varbinder::Decl *binderDecl =
        fbPropDecl->is_const()
            ? static_cast<varbinder::Decl *>(EAllocator::New<varbinder::ConstDecl>(propDecl->Id()->Name()))
            : EAllocator::New<varbinder::PropertyDecl>(propDecl->Id()->Name());
    binderDecl->BindNode(propDecl);
    const auto scopeToAdd = fbPropDecl->is_static() ? Scope()->AsClassScope()->StaticFieldScope()
                                                    : Scope()->AsClassScope()->InstanceFieldScope();
    const auto var = scopeToAdd->AddDecl(allocator, binderDecl, ScriptExtension::ETS);

    var->AddFlag(varbinder::VariableFlags::PROPERTY);
    var->AddFlag(varbinder::VariableFlags::INITIALIZED);
    if (fbPropDecl->is_static()) {
        var->AddFlag(varbinder::VariableFlags::STATIC);
    }
    propDecl->Id()->SetVariable(var);

    return propDecl;
}

ir::OverloadDeclaration *MetadataDeserializationPhase::CreateOverloadGroupDecl(
    std::string_view groupName, bool isStatic, const std::vector<std::string_view> &overloadedNames) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    const auto modifiers = ir::ModifierFlags::PUBLIC | (isStatic ? ir::ModifierFlags::STATIC : ir::ModifierFlags::NONE);

    auto *key = ctx->AllocNode<ir::Identifier>(groupName, allocator);
    auto *overloadDecl = ctx->AllocNode<ir::OverloadDeclaration>(key, modifiers, allocator);
    overloadDecl->AddOverloadDeclFlag(ir::OverloadDeclFlags::CLASS_METHOD);

    auto *const clsScope = Scope()->AsClassScope();
    auto *const methodScope = isStatic ? clsScope->StaticMethodScope() : clsScope->InstanceMethodScope();

    ArenaVector<ir::Expression *> overloadedList(allocator->Adapter());
    for (const auto &fbName : overloadedNames) {
        auto *ident = ctx->AllocNode<ir::Identifier>(fbName, allocator);
        auto found = methodScope->FindLocal(ident->Name(), varbinder::ResolveBindingOptions::BINDINGS);
        if (found != nullptr) {
            ident->SetVariable(found);
        }
        overloadedList.emplace_back(ident);
    }
    overloadDecl->SetOverloadedList(std::move(overloadedList));

    auto *const targetScope = isStatic ? clsScope->StaticDeclScope() : clsScope->InstanceDeclScope();
    auto *const varBinder = ctx->GetChecker()->VarBinder();
    auto scopeCtx = varbinder::LexicalScope<varbinder::LocalScope>::Enter(varBinder, targetScope);
    auto *const var =
        std::get<1>(varBinder->NewVarDecl<varbinder::FunctionDecl>(key->Start(), allocator, key->Name(), overloadDecl));
    var->SetScope(clsScope);
    if (isStatic) {
        var->AddFlag(varbinder::VariableFlags::STATIC);
    }
    var->AddFlag(varbinder::VariableFlags::OVERLOAD);
    key->SetVariable(var);

    return overloadDecl;
}

ir::ClassProperty *MetadataDeserializationPhase::CreateAnnotationPropertyDecl(
    const Metadata::PropertyDecl *fbPropDecl) const
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();

    const auto propName = fbPropDecl->name()->string_view();
    const auto type = CreateType(fbPropDecl->return_type(), fbPropDecl->return_type_type());
    constexpr auto MODIFIERS =
        ir::ModifierFlags::PUBLIC | ir::ModifierFlags::ABSTRACT | ir::ModifierFlags::ANNOTATION_DECLARATION;
    const auto propDecl = ctx->AllocNode<ir::ClassProperty>(ctx->AllocNode<ir::Identifier>(propName, allocator),
                                                            nullptr, type, MODIFIERS, allocator, false);
    if (type != nullptr) {
        type->SetParent(propDecl);
    }

    const auto binderDecl = EAllocator::New<varbinder::LetDecl>(propDecl->Id()->Name());
    binderDecl->BindNode(propDecl);
    propDecl->Id()->SetVariable(Scope()->AddDecl(allocator, binderDecl, ScriptExtension::ETS));

    return propDecl;
}

std::string BuildMetadataImportPath(std::string_view from, std::string_view moduleName)
{
    if (from.empty() || moduleName.empty()) {
        return {};
    }

    std::string relativePath;
    const size_t relativePathOffset = 2;
    if (from.front() == '/') {
        relativePath = std::string(from.substr(1));
    } else if (from.size() > relativePathOffset && from.substr(0, relativePathOffset) == "./") {
        relativePath = std::string(from.substr(relativePathOffset));
    } else {
        return {};
    }

    auto modulePath = relativePath;
    std::replace(modulePath.begin(), modulePath.end(), '/', '.');
    const std::string moduleSuffix = std::string(".") + modulePath;
    if (moduleName.size() <= moduleSuffix.size() ||
        moduleName.substr(moduleName.size() - moduleSuffix.size()) != moduleSuffix) {
        return {};
    }

    const auto packageName = moduleName.substr(0, moduleName.size() - moduleSuffix.size());
    std::string importPath;
    importPath.reserve(packageName.size() + 1 + relativePath.size());
    importPath.append(packageName);
    importPath.push_back('/');
    importPath.append(relativePath);
    return importPath;
}

ir::ETSImportDeclaration *MetadataDeserializationPhase::CreateImportDecl(const Metadata::ImportDecl *fbImportDecl)
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();

    auto &mergedSpecifiers = mergedImportSpecifiers_[curProgram];
    ArenaVector<ir::AstNode *> specifiers;
    for (const auto &fbSpec : *fbImportDecl->decl_names()) {
        auto specKey =
            std::string(fbImportDecl->module_name()->string_view()) + '\0' + std::string(fbSpec->string_view());
        if (!mergedSpecifiers.insert(std::move(specKey)).second) {
            continue;
        }
        auto *local = ctx->AllocNode<ir::Identifier>(fbSpec->string_view(), allocator);
        auto *imported = ctx->AllocNode<ir::Identifier>(fbSpec->string_view(), allocator);
        specifiers.emplace_back(ctx->AllocNode<ir::ImportSpecifier>(imported, local));
    }

    const auto rawFrom = fbImportDecl->from()->string_view();
    const auto moduleName = fbImportDecl->module_name()->string_view();
    const auto metadataImportPath = BuildMetadataImportPath(rawFrom, moduleName);
    auto *from = metadataImportPath.empty()
                     ? ctx->AllocNode<ir::StringLiteral>(rawFrom)
                     : ctx->AllocNode<ir::StringLiteral>(util::UString(metadataImportPath, allocator).View());
    auto *depProgram = ctx->parser->GetImportPathManager()->GatherImportInfo(curProgram, from);
    ES2PANDA_ASSERT(depProgram != nullptr);
    const auto importDecl =
        ctx->AllocNode<ir::ETSImportDeclaration>(from, depProgram->GetImportInfo(), std::move(specifiers));
    importDecl->SetParent(curProgram->Ast());

    LOG_METADATA(IrDeclToString(importDecl));

    return importDecl;
}

ir::ETSReExportDeclaration *MetadataDeserializationPhase::CreateReExportDecl(
    const Metadata::ReExportDecl *fbReExportDecl) const
{
    ES2PANDA_ASSERT(fbReExportDecl->from() != nullptr);
    ES2PANDA_ASSERT(fbReExportDecl->module_name() != nullptr);
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    ArenaVector<ir::AstNode *> specifiers;
    switch (fbReExportDecl->kind()) {
        case Metadata::ReExportKind_Star: {
            auto *local = ctx->AllocNode<ir::Identifier>(util::StringView {}, allocator);
            specifiers.emplace_back(ctx->AllocNode<ir::ImportNamespaceSpecifier>(local));
            break;
        }
        case Metadata::ReExportKind_Namespace: {
            ES2PANDA_ASSERT(fbReExportDecl->exported_name() != nullptr);
            auto *local = ctx->AllocNode<ir::Identifier>(fbReExportDecl->exported_name()->string_view(), allocator);
            specifiers.emplace_back(ctx->AllocNode<ir::ImportNamespaceSpecifier>(local));
            break;
        }
        case Metadata::ReExportKind_Named: {
            ES2PANDA_ASSERT(fbReExportDecl->exported_name() != nullptr);
            ES2PANDA_ASSERT(fbReExportDecl->imported_name() != nullptr);
            auto *local = ctx->AllocNode<ir::Identifier>(fbReExportDecl->exported_name()->string_view(), allocator);
            auto *imported = ctx->AllocNode<ir::Identifier>(fbReExportDecl->imported_name()->string_view(), allocator);
            specifiers.emplace_back(ctx->AllocNode<ir::ImportSpecifier>(imported, local));
            break;
        }
        default:
            ES2PANDA_UNREACHABLE();
    }
    const auto rawFrom = fbReExportDecl->from()->string_view();
    const auto moduleName = fbReExportDecl->module_name()->string_view();
    const auto metadataImportPath = BuildMetadataImportPath(rawFrom, moduleName);
    auto *from = metadataImportPath.empty()
                     ? ctx->AllocNode<ir::StringLiteral>(rawFrom)
                     : ctx->AllocNode<ir::StringLiteral>(util::UString(metadataImportPath, allocator).View());
    auto *depProgram = ctx->parser->GetImportPathManager()->GatherImportInfo(curProgram, from);
    ES2PANDA_ASSERT(depProgram != nullptr);
    const auto importKind = fbReExportDecl->is_type() ? ir::ImportKinds::TYPES : ir::ImportKinds::ALL;
    auto *importDecl =
        ctx->AllocNode<ir::ETSImportDeclaration>(from, depProgram->GetImportInfo(), std::move(specifiers), importKind);
    auto *reExportDecl = ctx->AllocNode<ir::ETSReExportDeclaration>(importDecl, std::vector<std::string> {},
                                                                    curProgram->SourceFilePath(), allocator);
    importDecl->SetParent(reExportDecl);
    LOG_METADATA(IrDeclToString(reExportDecl));
    return reExportDecl;
}

ir::AnnotationDeclaration *MetadataDeserializationPhase::CreateAnnotationDecl(
    const Metadata::AnnotationDecl *fbAnnotationDecl, bool isNested)
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();

    const auto annotationName = fbAnnotationDecl->name()->string_view();
    const auto annotationDecl =
        ctx->AllocNode<ir::AnnotationDeclaration>(ctx->AllocNode<ir::Identifier>(annotationName, allocator), allocator);
    annotationDecl->SetInternalName(fbAnnotationDecl->internal_name()->string_view());
    annotationDecl->SetScope(ArenaAllocator::New<varbinder::AnnotationScope>(allocator, Scope()));
    annotationDecl->Scope()->BindNode(annotationDecl);
    annotationDecl->AddModifier(ir::ModifierFlags::EXPORT | ir::ModifierFlags::DECLARE);

    const auto binderDecl = allocator->New<varbinder::AnnotationDecl>(annotationDecl->GetBaseName()->Name());
    binderDecl->BindNode(annotationDecl);
    annotationDecl->GetBaseName()->SetVariable(Scope()->AddDecl(allocator, binderDecl, ScriptExtension::ETS));

    if (!isNested) {
        annotationDecl->SetParent(curProgram->Ast());
        curProgram->Ast()->AddStatement(annotationDecl);
    }

    LOG_METADATA("annotation " << annotationName);

    ArenaVector<ir::AstNode *> properties(allocator->Adapter());
    if (fbAnnotationDecl->properties()) {
        WithScope<void>(annotationDecl->Scope(), [this, &fbAnnotationDecl, &properties, &annotationDecl]() -> void {
            for (const auto fbPropDecl : *fbAnnotationDecl->properties()) {
                auto *property = CreateAnnotationPropertyDecl(fbPropDecl);
                property->SetParent(annotationDecl);
                properties.emplace_back(property);
            }
        });
    }
    annotationDecl->AddProperties(std::move(properties));

    if (fbAnnotationDecl->access_restriction_modules() != nullptr) {
        ArenaVector<util::StringView> modules(allocator->Adapter());
        for (const auto *fbModule : *fbAnnotationDecl->access_restriction_modules()) {
            if (fbModule != nullptr) {
                modules.emplace_back(fbModule->string_view());
            }
        }
        annotationDecl->SetMetadataAccessRestrictionModules(std::move(modules));
    }

    if (fbAnnotationDecl->access_restriction_annotation_name() != nullptr) {
        annotationDecl->SetMetadataAccessRestrictionAnnotationName(
            fbAnnotationDecl->access_restriction_annotation_name()->string_view());
    }
    return annotationDecl;
}

void MetadataDeserializationPhase::MaterializeTypeDecl(const Metadata::TypeDecl *fbTypeDecl,
                                                       ir::TSTypeAliasDeclaration *typeDecl)
{
    ES2PANDA_ASSERT(fbTypeDecl->type() != nullptr);
    ES2PANDA_ASSERT(fbTypeDecl->type_type() != Metadata::Type_NONE);
    const auto typeParams = CreateTypeParams(fbTypeDecl->type_params());
    const auto type =
        WithScope<ir::TypeNode *>(typeParams != nullptr ? typeParams->Scope() : Scope(), [this, fbTypeDecl] {
            return CreateType(fbTypeDecl->type(), fbTypeDecl->type_type());
        });
    typeDecl->SetTypeParameters(typeParams);
    if (typeParams != nullptr) {
        typeParams->SetParent(typeDecl);
    }
    ES2PANDA_ASSERT(type != nullptr);
    typeDecl->SetTypeAnnotation(type);
    typeDecl->AddModifier(ir::ModifierFlags::EXPORT);

    LOG_METADATA("type " << fbTypeDecl->name()->string_view() << " = " << IrDeclToString(type));
}

std::vector<ir::TSTypeAliasDeclaration *> MetadataDeserializationPhase::CreateTypeDecls(
    const flatbuffers::Vector<flatbuffers::Offset<Metadata::TypeDecl>> *fbTypeDecls)
{
    const auto ctx = Context();
    const auto allocator = ctx->Allocator();
    std::vector<ir::TSTypeAliasDeclaration *> typeDecls {};

    for (const auto &fbTypeDecl : *fbTypeDecls) {
        const auto name = fbTypeDecl->name()->string_view();
        auto *const typeId = ctx->AllocNode<ir::Identifier>(name, allocator);
        auto *const typeDecl = ctx->AllocNode<ir::TSTypeAliasDeclaration>(allocator, typeId);
        typeId->SetParent(typeDecl);
        auto *const binderDecl = EAllocator::New<varbinder::TypeAliasDecl>(name);
        binderDecl->BindNode(typeDecl);
        auto *const variable = Scope()->AddDecl(allocator, binderDecl, ScriptExtension::ETS);
        ES2PANDA_ASSERT(variable != nullptr);
        variable->AddFlag(varbinder::VariableFlags::TYPE_ALIAS);
        typeId->SetVariable(variable);
        typeDecls.emplace_back(typeDecl);
    }

    for (size_t i = 0; i < fbTypeDecls->size(); i++) {
        MaterializeTypeDecl(fbTypeDecls->Get(i), typeDecls[i]);
    }
    return typeDecls;
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
            auto *enclosingScope = typeParams ? typeParams->Scope() : Scope();
            interfaceDeclProto->SetScope(ArenaAllocator::New<varbinder::ClassScope>(allocator, enclosingScope));
            const auto interfaceDecl =
                ctx->GetChecker()->AsETSChecker()->CreateInterfaceProto(interfaceName, curProgram, interfaceDeclProto);

            interfaceDecl->Scope()->BindNode(interfaceDecl);
            interfaceDecl->AddModifier(ir::ModifierFlags::DECLARE);
            interfaceDecl->AddModifier(ir::ModifierFlags::EXPORT);
            MarkBuiltinIfNeeded(interfaceDecl->Variable(), fbInterfaceDecl->is_builtin());

            if (typeParams) {
                interfaceDecl->SetTypeParams(typeParams);
                interfaceDecl->TypeParams()->SetParent(interfaceDecl);
                if (auto *type = interfaceDecl->Variable() != nullptr ? interfaceDecl->Variable()->TsType() : nullptr;
                    type != nullptr && type->IsETSObjectType()) {
                    ctx->GetChecker()->AsETSChecker()->CreateTypeForClassOrInterfaceTypeParameters(
                        type->AsETSObjectType());
                }
            }

            AddExtends(fbInterfaceDecl, interfaceDecl);

            LOG_METADATA(interfaceName << (interfaceDecl->TypeParams()
                                               ? "<" + IrDeclVectorToString(interfaceDecl->TypeParams()->Params()) + ">"
                                               : "")
                                       << (!interfaceDecl->Extends().empty()
                                               ? ": " + IrDeclVectorToString(interfaceDecl->Extends())
                                               : ""));

            // External-program ASTs are cached beyond this phase's lifetime.
            if (Context()->isExternal) {
                MaterializeMembers(interfaceDecl, fbInterfaceDecl);
            } else {
                lazyInterfaceMembers_[interfaceDecl] = {fbInterfaceDecl, curProgram};
            }

            return interfaceDecl;
        });
}

ir::ClassDefinition *MetadataDeserializationPhase::CreateClassDecl(const Metadata::ClassDecl *fbClassDecl,
    const Vector<Offset<Metadata::TypeParamDecl>> *fbTypeParams, const bool isNested, bool isPreDeclare,
    bool materializeMembers)
{
    const auto typeParams = CreateTypeParams(fbTypeParams);
    MetadataClassBuilder builder(*this, fbClassDecl, typeParams, Scope(), isNested);

    return WithScope<ir::ClassDefinition *>(builder.ScopeForClass(), [this, &builder, fbClassDecl, &isPreDeclare,
                                                                      materializeMembers]() -> ir::ClassDefinition* {
        auto *const classDef = builder.GetOrCreateClassDef();
        if (builder.IsNested()) {
            builder.AddNestedClassStaticProperty(classDef);
        }

        builder.SetupClassDecl(classDef);

        if (!isPreDeclare) {
            builder.AddClassHeritage(classDef);
        }

        const bool isEnum = fbClassDecl->enum_kind() != Metadata::EnumKind_NONE;
        if (materializeMembers || (!isPreDeclare && isEnum)) {
            MaterializeMembers(classDef, fbClassDecl);
        } else if (!isPreDeclare) {
            lazyClassMembers_[classDef] = {fbClassDecl, curProgram};
        }

        LOG_METADATA(GetDeclKindToLog(fbClassDecl)
                     << fbClassDecl->name()->string_view()
                     << (classDef->TypeParams() != nullptr ? "<" + IrDeclToString(classDef->TypeParams()) + ">" : ""));

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
void MetadataDeserializationPhase::PredeclareClasses(const Metadata::Decls *decls, bool isNested)
{
    if (decls == nullptr || decls->classes() == nullptr) {
        return;
    }

    for (const auto fbClassDecl : *decls->classes()) {
        auto *const classDef = CreateClassDecl(fbClassDecl, fbClassDecl->type_params(), isNested, true, false);
        if (fbClassDecl->decls() != nullptr) {
            WithScope<void>(classDef->Scope(), [this, &fbClassDecl] { PredeclareClasses(fbClassDecl->decls(), true); });
        }
    }
}

ArenaVector<ir::AstNode *> MetadataDeserializationPhase::CreateDependencyDecls(const Metadata::Decls *decls)
{
    ArenaVector<ir::AstNode *> nodes;
    if (decls->imports()) {
        for (const auto &fbImportDecl : *decls->imports()) {
            auto *importDecl = CreateImportDecl(fbImportDecl);
            ES2PANDA_ASSERT(importDecl != nullptr);
            nodes.emplace_back(importDecl);
        }
    }
    if (decls->re_exports()) {
        for (const auto &fbReExportDecl : *decls->re_exports()) {
            auto *reExportDecl = CreateReExportDecl(fbReExportDecl);
            ES2PANDA_ASSERT(reExportDecl != nullptr);
            nodes.emplace_back(reExportDecl);
        }
    }

    return nodes;
}

ArenaVector<ir::AstNode *> MetadataDeserializationPhase::CreateDecls(const Metadata::Decls *decls, bool isNested,
                                                                     parser::Program *moduleProg,
                                                                     varbinder::ExportFactStore *store,
                                                                     varbinder::ETSBinder *etsBinder)
{
    ES2PANDA_ASSERT(isNested || moduleProg != nullptr);
    ES2PANDA_ASSERT(isNested || store != nullptr);

    ArenaVector<ir::AstNode *> nodes;
    if (!isNested) {
        nodes = CreateDependencyDecls(decls);
        // Type annotations may refer to imported declarations, so imports must be bound before materializing.
        BindMetadataImports(nodes, etsBinder, store);
    }

    AddDeclarations(decls, isNested, moduleProg, store, nodes);

    return nodes;
}

void MetadataDeserializationPhase::AddDeclarations(const Metadata::Decls *decls, bool isNested,
                                                   parser::Program *moduleProg,
                                                   varbinder::ExportFactStore *store,
                                                   ArenaVector<ir::AstNode *> &nodes)
{
    if (decls->types()) {
        for (auto *typeDecl : CreateTypeDecls(decls->types())) {
            nodes.emplace_back(typeDecl);
            if (isNested) {
                continue;
            }

            ES2PANDA_ASSERT(moduleProg->Ast() != nullptr);
            typeDecl->SetParent(moduleProg->Ast());

            auto *id = typeDecl->Id();
            ES2PANDA_ASSERT(id != nullptr);
            auto *var = id->Variable();
            ES2PANDA_ASSERT(var != nullptr);
            ES2PANDA_ASSERT(HasDeclarationNodeKind(var, &ir::AstNode::IsTSTypeAliasDeclaration));
            const auto& name = id->Name();

            store->AddLocalExport(moduleProg, id->Name(), var, typeDecl);
            [[maybe_unused]] const auto added [[maybe_unused]] = store->AddPendingLocalExportAlias(moduleProg, name,
                name, typeDecl, typeDecl, typeDecl, true, false,
                varbinder::LocalExportKind::DECLARATION);
            ES2PANDA_ASSERT(added);
        }
    }

    if (decls->classes()) {
        for (const auto fbClassDecl : *decls->classes()) {
            const bool materializeMembers = Context()->isExternal || fbClassDecl->name()->str() == "ETSGLOBAL";
            auto *const classDef =
                CreateClassDecl(fbClassDecl, fbClassDecl->type_params(), isNested, false, materializeMembers);
            nodes.emplace_back(isNested ? classDef->Parent() : classDef);
        }
    }

    if (decls->interfaces()) {
        for (const auto &fbInterfaceDecl : *decls->interfaces()) {
            auto *interfaceDecl = CreateInterfaceDecl(fbInterfaceDecl);
            ES2PANDA_ASSERT(interfaceDecl != nullptr);
            if (isNested) {
                nodes.emplace_back(interfaceDecl);
                continue;
            }
            ES2PANDA_ASSERT(moduleProg->Ast() != nullptr);
            ES2PANDA_ASSERT(interfaceDecl->Parent() == moduleProg->Ast());
            ES2PANDA_ASSERT(interfaceDecl->Id() != nullptr);
            ES2PANDA_ASSERT(interfaceDecl->Id()->Variable() != nullptr);
            nodes.emplace_back(interfaceDecl);
        }
    }

    if (decls->annotations()) {
        for (const auto &fbAnnotationDecl : *decls->annotations()) {
            auto *annotationDecl = CreateAnnotationDecl(fbAnnotationDecl, isNested);
            ES2PANDA_ASSERT(annotationDecl != nullptr);
            nodes.emplace_back(annotationDecl);
        }
    }
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

parser::Program *MetadataDeserializationPhase::ResolveMetadataModuleProgram(const Metadata::Decls *root,
                                                                            std::string_view moduleName) const
{
    const auto fbPkgName = root->package_name();
    const auto moduleId = fbPkgName != nullptr ? fbPkgName->string_view() : moduleName;
    if (curProgram->ModuleName() == moduleId) {
        return curProgram;
    }

    const auto ctx = Context();
    auto importPath = std::string(moduleId);
    std::replace(importPath.begin(), importPath.end(), '.', '/');
    auto *pathLiteral = ctx->AllocNode<ir::StringLiteral>(util::UString(importPath, ctx->Allocator()).View());
    auto *moduleProg = ctx->parser->GetImportPathManager()->GatherImportInfo(curProgram, pathLiteral);
    ES2PANDA_ASSERT(moduleProg != nullptr);
    return moduleProg;
}

void MetadataDeserializationPhase::BindMetadataImports(const ArenaVector<ir::AstNode *> &nodes,
                                                       varbinder::ETSBinder *etsBinder,
                                                       varbinder::ExportFactStore *moduleStore)
{
    ES2PANDA_ASSERT(etsBinder != nullptr);
    ES2PANDA_ASSERT(moduleStore != nullptr);
    ES2PANDA_ASSERT(curProgram != nullptr);
    ES2PANDA_ASSERT(curProgram->Ast() != nullptr);

    const auto ctx = Context();
    for (auto *node : nodes) {
        ES2PANDA_ASSERT(node != nullptr);
        if (!node->IsETSImportDeclaration()) {
            continue;
        }

        auto *const importDecl = node->AsETSImportDeclaration();
        ES2PANDA_ASSERT(importDecl->Parent() == curProgram->Ast());

        auto *const depProgram = ctx->parser->GetImportPathManager()->SearchResolved(importDecl->ImportInfo());
        ES2PANDA_ASSERT(depProgram != nullptr);
        if (depProgram != curProgram && !depProgram->IsMetadataLoadedIfApplicable()) {
            PerformForProgram(depProgram);
        }

        etsBinder->BuildImportDeclaration(importDecl);

        RegisterMetadataImportTarget(ctx, moduleStore, curProgram, importDecl, depProgram);
    }
}

void MetadataDeserializationPhase::RegisterMetadataReExportSpecifier(varbinder::ExportFactStore *store,
                                                                     parser::Program *moduleProg,
                                                                     const ir::ETSImportDeclaration *importDecl,
                                                                     ir::AstNode *specifier) const
{
    ES2PANDA_ASSERT(store != nullptr);
    ES2PANDA_ASSERT(moduleProg != nullptr);
    ES2PANDA_ASSERT(importDecl != nullptr);
    ES2PANDA_ASSERT(specifier != nullptr);

    if (specifier->IsImportNamespaceSpecifier()) {
        auto *namespaceSpecifier = specifier->AsImportNamespaceSpecifier();
        if (namespaceSpecifier->Local()->Name().Empty()) {
            store->AddStarExport(moduleProg, importDecl, namespaceSpecifier, importDecl->IsTypeKind());
        } else {
            store->AddNamespaceExport(moduleProg, importDecl, namespaceSpecifier->Local()->Name(), nullptr,
                                      namespaceSpecifier, importDecl->IsTypeKind());
        }
        return;
    }

    if (specifier->IsImportSpecifier()) {
        auto *importSpecifier = specifier->AsImportSpecifier();
        auto exportedName = varbinder::NormalizeReExportName(importSpecifier->Local()->Name());
        auto importedName = varbinder::NormalizeReExportName(importSpecifier->Imported()->Name());
        if (varbinder::IsDefaultExportName(exportedName) && !varbinder::IsDefaultExportName(importedName)) {
            return;
        }
        store->AddNamedReExport(moduleProg, importDecl, exportedName, importedName, importSpecifier,
                                importDecl->IsTypeKind());
        return;
    }

    ES2PANDA_UNREACHABLE();
}

void MetadataDeserializationPhase::RegisterMetadataReExports(const ArenaVector<ir::AstNode *> &nodes,
                                                             parser::Program *moduleProg,
                                                             varbinder::ExportFactStore *store,
                                                             varbinder::ETSBinder *etsBinder)
{
    ES2PANDA_ASSERT(moduleProg != nullptr);
    ES2PANDA_ASSERT(store != nullptr);
    ES2PANDA_ASSERT(etsBinder != nullptr);
    const auto ctx = Context();
    for (auto *node : nodes) {
        if (!node->IsETSReExportDeclaration()) {
            continue;
        }
        node->SetParent(curProgram->Ast());
        auto *reExportDecl = node->AsETSReExportDeclaration();
        reExportDecl->SetProgram(moduleProg);
        etsBinder->AddReExportImport(reExportDecl);
        auto *importDecl = reExportDecl->GetETSImportDeclarations();
        ES2PANDA_ASSERT(importDecl != nullptr);
        auto *targetProg = ctx->parser->GetImportPathManager()->GatherImportInfo(curProgram, importDecl->Source());
        ES2PANDA_ASSERT(targetProg != nullptr);
        if (targetProg != curProgram && !targetProg->IsMetadataLoadedIfApplicable()) {
            PerformForProgram(targetProg);
        }
        store->AddImportTarget(moduleProg, importDecl, targetProg);
        store->AddEffectiveImportTarget(moduleProg, importDecl, targetProg);
        for (auto *specifier : importDecl->Specifiers()) {
            RegisterMetadataReExportSpecifier(store, moduleProg, importDecl, specifier);
        }
    }
}

void MetadataDeserializationPhase::RegisterMetadataLocalExports(const ArenaVector<ir::AstNode *> &nodes,
                                                                parser::Program *moduleProg,
                                                                varbinder::ExportFactStore *store) const
{
    ES2PANDA_ASSERT(moduleProg != nullptr);
    ES2PANDA_ASSERT(store != nullptr);

    store->RegisterProgramSurface(moduleProg);
    for (auto *node : nodes) {
        ES2PANDA_ASSERT(node != nullptr);

        auto *classDef = GetMetadataClassDefinition(node);
        if (classDef != nullptr) {
            RegisterMetadataClassExport(curProgram, moduleProg, store, classDef);
            continue;
        }

        if (node->IsTSInterfaceDeclaration()) {
            auto *interfaceDecl = node->AsTSInterfaceDeclaration();
            ES2PANDA_ASSERT(interfaceDecl->Parent() == moduleProg->Ast());
            RegisterMetadataInterfaceExport(curProgram, moduleProg, store, interfaceDecl);
            continue;
        }

        if (node->IsTSTypeAliasDeclaration()) {
            RegisterMetadataTypeAliasExport(moduleProg, store, node->AsTSTypeAliasDeclaration());
            continue;
        }

        if (node->IsAnnotationDeclaration()) {
            auto *annotationDecl = node->AsAnnotationDeclaration();
            ES2PANDA_ASSERT(annotationDecl->Parent() == moduleProg->Ast());
            RegisterMetadataAnnotationExport(moduleProg, store, annotationDecl);
            continue;
        }
    }
}

void MetadataDeserializationPhase::RegisterMetadataLocalExportSpecifier(
    const flatbuffers::Vector<flatbuffers::Offset<Metadata::LocalExportDecl>> *fbLocalExports,
    parser::Program *moduleProg, varbinder::ExportFactStore *store) const
{
    if (fbLocalExports == nullptr) {
        return;
    }
    for (const auto &fbLocalExportDecl : *fbLocalExports) {
        ES2PANDA_ASSERT(fbLocalExportDecl != nullptr);
        ES2PANDA_ASSERT(fbLocalExportDecl->exported_name() != nullptr);
        ES2PANDA_ASSERT(fbLocalExportDecl->local_name() != nullptr);
        ES2PANDA_ASSERT(moduleProg != nullptr);
        ES2PANDA_ASSERT(store != nullptr);

        const auto exportedName = util::StringView {fbLocalExportDecl->exported_name()->string_view()};
        const auto localName = util::StringView {fbLocalExportDecl->local_name()->string_view()};
        auto *const var = FindMetadataLocalVariable(moduleProg, localName);
        ES2PANDA_ASSERT(var != nullptr);
        ES2PANDA_ASSERT(var->Declaration() != nullptr);
        ES2PANDA_ASSERT(var->Declaration()->Node() != nullptr);
        if (fbLocalExportDecl->is_type() && !HasDeclarationNodeKind(var, &ir::AstNode::IsClassDefinition) &&
            !HasDeclarationNodeKind(var, &ir::AstNode::IsTSInterfaceDeclaration) &&
            !HasDeclarationNodeKind(var, &ir::AstNode::IsTSTypeAliasDeclaration) &&
            !HasDeclarationNodeKind(var, &ir::AstNode::IsTSEnumDeclaration)) {
            LOG(ERROR, ES2PANDA) << "Metadata local export '" << exportedName << "' for local '" << localName
                                 << "' in module '" << moduleProg->ModuleName()
                                 << "' is marked as type-only, but the local declaration is not a type declaration";
            ES2PANDA_UNREACHABLE();
        }

        auto *const node = var->Declaration()->Node();
        store->AddLocalExport(moduleProg, exportedName, localName, var, node);
        if (!store->AddPendingLocalExportAlias(moduleProg, exportedName, localName, node, node, node, true,
                                               fbLocalExportDecl->is_type(), varbinder::LocalExportKind::ALIAS)) {
            LOG(ERROR, ES2PANDA) << "Metadata local export alias '" << exportedName << "' refers to local '"
                                 << localName << "' in module '" << moduleProg->ModuleName()
                                 << "', but another pending alias with the same exported name already refers to a "
                                 << "different local";
            ES2PANDA_UNREACHABLE();
        }
    }
}

varbinder::Variable *MetadataDeserializationPhase::FindMetadataLocalVariable(
    parser::Program *program, util::StringView localName) const
{
    ES2PANDA_ASSERT(program != nullptr);
    ES2PANDA_ASSERT(program->GlobalScope() != nullptr);
    ES2PANDA_ASSERT(program->GlobalClassScope() != nullptr);

    struct Entry {
        const char *scope;
        varbinder::Variable *var;
    };
    auto const gcs = program->GlobalClassScope();
    std::vector<Entry> found;

    auto add = [&found](const char *scope, varbinder::Variable *var) {
        if (var != nullptr) {
            found.push_back({scope, var});
        }
    };

    add("StaticDeclScope", gcs->StaticDeclScope()->FindLocal(localName, varbinder::ResolveBindingOptions::ALL));
    add("StaticMethodScope", gcs->StaticMethodScope()->FindLocal(localName, varbinder::ResolveBindingOptions::ALL));
    add("GlobalScope", program->GlobalScope()->Find(localName, varbinder::ResolveBindingOptions::ALL).variable);
    add("StaticFieldScope", gcs->StaticFieldScope()->FindLocal(localName, varbinder::ResolveBindingOptions::ALL));
    add("TypeAliasScope", gcs->TypeAliasScope()->FindLocal(localName, varbinder::ResolveBindingOptions::ALL));

    if (found.size() != 1U) {
        LOG_METADATA("'" << localName << "' in module '" << program->ModuleName() << "': expected 1, got "
                         << found.size());
        for ([[maybe_unused]] const auto &e : found) {
            LOG_METADATA("  " << e.scope << " decl=" << static_cast<int>(e.var->Declaration()->Type()));
        }
    }
    ES2PANDA_ASSERT(found.size() == 1U);
    return found[0].var;
}

void MetadataDeserializationPhase::ProcessMetadata(MetadataByModules *metadata)
{
    auto resolveMetadataModuleProgram = [this](const Metadata::Decls *root, std::string_view moduleName) {
        auto *moduleProg = ResolveMetadataModuleProgram(root, moduleName);
        if (moduleProg->Is<util::ModuleKind::PACKAGE>()) {
            moduleProg->MaybeIteratePackage([&moduleProg, moduleName](parser::Program *fraction,
                                                                      bool isPackageFraction) {
                if (isPackageFraction && fraction->ModuleName() == moduleName) {
                    moduleProg = fraction;
                }
            });
        }
        ES2PANDA_ASSERT(moduleProg != nullptr);
        ES2PANDA_ASSERT(moduleProg->Ast() != nullptr);
        ES2PANDA_ASSERT(moduleProg->Ast()->Scope() != nullptr);
        return moduleProg;
    };

    // Pass 1: create every top-level class prototype from every metadata module before any member type is built.
    for (const auto &[moduleName, moduleMetadata] : *metadata) {
        if (moduleMetadata.empty()) {
            LOG_METADATA("skipped module \"" << moduleName << "\" (no metadata)");
            continue;
        }

        const auto root = Metadata::GetDecls(moduleMetadata.data());
        auto *moduleProg = resolveMetadataModuleProgram(root, moduleName);
        WithProgram(moduleProg, [this, &root, moduleProg] {
            LOG_METADATA("predeclaring metadata classes for " << curProgram->ModuleName());
            WithScope<void>(moduleProg->Ast()->Scope(), [this, &root] { PredeclareClasses(root); });
        });
    }

    // Pass 2: fill declarations after all class names are visible.
    for (const auto &[moduleName, moduleMetadata] : *metadata) {
        if (moduleMetadata.empty()) {
            continue;
        }

        LOG_METADATA("processing module \"" << moduleName << "\" (" << std::to_string(moduleMetadata.size())
                                            << " bytes)");

        const auto root = Metadata::GetDecls(moduleMetadata.data());
        auto *moduleProg = resolveMetadataModuleProgram(root, moduleName);

        ProcessMetadataModule(moduleProg, root);
    }

    LOG_METADATA("TOTAL SIZE: " << CalculateMetadataSize(metadata) << " bytes");
}

void MetadataDeserializationPhase::ProcessMetadataModule(parser::Program *moduleProg, const Metadata::Decls *root)
{
    auto *const etsBinder = Context()->GetChecker()->VarBinder()->AsETSBinder();
    auto *const store = &etsBinder->GetExportFactsStore();
    auto *const moduleStore = &FindMetadataETSBinder(moduleProg)->GetExportFactsStore();

    WithProgram(moduleProg, [this, root, moduleProg, moduleStore, etsBinder, store] {
        ES2PANDA_ASSERT(curProgram == moduleProg);
        LOG_METADATA("deserializing metadata module " << curProgram->ModuleName());
        varbinder::GlobalScopeContext gsc(etsBinder, curProgram, curProgram->GlobalScope());
        varbinder::RecordTableContext rtc(etsBinder, curProgram);

        auto const run = [this, root, moduleProg, moduleStore, etsBinder]()-> ArenaVector<ir::AstNode *> {
            return CreateDecls(root, false, moduleProg, moduleStore, etsBinder);
        };

        auto nodes = WithScope<ArenaVector<ir::AstNode *>>(curProgram->Ast()->Scope(), run);
        RegisterMetadataReExports(nodes, moduleProg, store, etsBinder);
        RegisterMetadataLocalExports(nodes, moduleProg, moduleStore);
        RegisterMetadataLocalExportSpecifier(root->local_exports(), moduleProg, moduleStore);
    });
}

bool MetadataDeserializationPhase::PerformForProgram(parser::Program *program)
{
    if (!Context()->config->options->IsReadMetadata() && !program->IsStdLib()) {
        return false;  // make phase failed due to the corresponding compilation option disabled
    }
    if (program->IsMetadataLoadedIfApplicable()) {
        return true;
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

        auto *checker = ctx->GetChecker();
        MetadataCheckerVarBinderContext checkerVarBinderCtx(checker, ctx->parserProgram->VarBinder());
        curProgram->PushChecker(checker);
        curProgram->SetAst(CreateModule());

        LOG_METADATA_NESTING_INC();
        SetupGlobalClass();
        ProcessMetadata(metadata);
        LOG_METADATA_NESTING_DEC();
    });

    return true;
}

}  // namespace ark::es2panda::compiler
