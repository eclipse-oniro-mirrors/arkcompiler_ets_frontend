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

#include "checker/ETSchecker.h"

#include "checker/checkerContext.h"
#include "generated/diagnostic.h"
#include "ir/module/importDefaultSpecifier.h"
#include "ir/module/importNamespaceSpecifier.h"
#include "ir/module/importSpecifier.h"
#include "parser/program/program.h"
#include "varbinder/ETSBinder.h"

namespace ark::es2panda::checker {

static std::pair<bool, util::StringView> FindSpecifierForModuleObject(ir::ETSImportDeclaration *importDecl,
                                                                      varbinder::Variable *binding)
{
    ES2PANDA_ASSERT(importDecl != nullptr);
    if ((!importDecl->Specifiers().empty()) && importDecl->Specifiers()[0]->IsImportNamespaceSpecifier()) {
        return std::make_pair(true, util::StringView());
    }

    for (auto item : importDecl->Specifiers()) {
        if (item->IsImportSpecifier() && (item->AsImportSpecifier()->Imported()->Variable() == binding)) {
            util::StringView alias {};
            auto specifier = item->AsImportSpecifier();
            if (specifier->Imported()->Name() != specifier->Local()->Name()) {
                alias = item->AsImportSpecifier()->Local()->Name();
            }
            return std::make_pair(true, alias);
        }
        if (item->IsImportDefaultSpecifier() && (item->AsImportDefaultSpecifier()->Local()->Variable() == binding)) {
            return std::make_pair(true, item->AsImportDefaultSpecifier()->Local()->Name());
        }
    }
    return std::make_pair(false, util::StringView());
}

void ETSChecker::BuildExportedFunctionSignature(varbinder::Variable *var)
{
    auto method = var->AsLocalVariable()->Declaration()->Node()->AsMethodDefinition();
    ES2PANDA_ASSERT(method->Parent()->IsClassDefinition() &&
                    method->Parent()->AsClassDefinition()->Ident()->Name().Is(compiler::Signatures::ETS_GLOBAL));
    auto classDef = method->Parent()->AsClassDefinition();
    if (classDef->TsType() == nullptr) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        BuildBasicClassProperties(classDef);
    }

    SavedCheckerContext scc(this, Context().Status(), classDef->TsType()->AsETSObjectType());
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto funcType = BuildMethodSignature(method);
    funcType->SetVariable(var);
    var->SetTsType(funcType);
    method->SetTsType(funcType);
}

static checker::Type *CreateNamespaceMemberType(ETSChecker *checker, util::StringView memberName, checker::Type *type,
                                                varbinder::VariableFlags exportFlags)
{
    if (type == nullptr || !type->IsETSFunctionType() || (exportFlags & varbinder::VariableFlags::TYPE_ALIAS) != 0U) {
        return type;
    }

    ArenaVector<checker::Signature *> signatures {checker->ProgramAllocator()->Adapter()};
    for (auto *signature : type->AsETSFunctionType()->CallSignaturesOfMethodOrArrow()) {
        auto *copy = signature->Copy(checker->ProgramAllocator(), checker->Relation(), checker->GetGlobalTypesHolder());
        signatures.push_back(copy);
    }
    return checker->CreateETSMethodType(memberName, std::move(signatures));
}

static checker::Type *ResolveNamespaceMemberType(ETSChecker *checker, varbinder::Variable *var)
{
    if (var == nullptr || var->Declaration() == nullptr || var->Declaration()->Node() == nullptr) {
        return checker->GetTypeOfVariable(var);
    }

    auto *declNode = var->Declaration()->Node();
    if (declNode->IsClassDefinition()) {
        auto *type = checker->GetTypeFromClassReference(var);
        if (type->IsETSObjectType()) {
            checker->ETSObjectTypeDeclNode(checker, type->AsETSObjectType());
        }
        return type;
    }

    if (declNode->IsTSInterfaceDeclaration()) {
        return checker->GetTypeFromInterfaceReference(var);
    }

    return checker->GetTypeOfVariable(var);
}

static varbinder::VariableFlags NamespaceMemberFlagsForExport(const varbinder::Variable *var)
{
    if (var == nullptr) {
        return varbinder::VariableFlags::NONE;
    }

    auto flags = static_cast<varbinder::VariableFlags>(
        var->Flags() & (varbinder::VariableFlags::CLASS_OR_INTERFACE_OR_ENUM | varbinder::VariableFlags::TYPE_ALIAS |
                        varbinder::VariableFlags::NAMESPACE | varbinder::VariableFlags::ANNOTATIONDECL |
                        varbinder::VariableFlags::ANNOTATIONUSAGE));
    if ((flags & (varbinder::VariableFlags::ANNOTATIONDECL | varbinder::VariableFlags::ANNOTATIONUSAGE)) != 0U) {
        flags |= varbinder::VariableFlags::TYPE_ALIAS;
    }
    if (var->Declaration() != nullptr && var->Declaration()->IsFunctionDecl()) {
        flags |= varbinder::VariableFlags::METHOD;
    }
    return flags;
}

static bool ShouldBindSyntheticMemberToSource(const varbinder::Variable *source)
{
    return source != nullptr && source->Declaration() != nullptr && source->Declaration()->Node() != nullptr &&
           !source->Declaration()->Node()->IsImportSpecifier() &&
           !source->Declaration()->Node()->IsImportNamespaceSpecifier() &&
           !source->Declaration()->Node()->IsImportDefaultSpecifier();
}

static ir::AstNode *NamespaceMemberDeclNode(checker::ETSObjectType *namespaceObject, const varbinder::Variable *source)
{
    return ShouldBindSyntheticMemberToSource(source) ? const_cast<ir::AstNode *>(source->Declaration()->Node())
                                                     : namespaceObject->GetDeclNode();
}

struct SyntheticNamespaceMemberArgs {
    ETSChecker *checker;
    checker::ETSObjectType *namespaceObject;
    util::StringView memberName;
    checker::Type *type;
    varbinder::VariableFlags exportFlags;
    const varbinder::Variable *source;
};

static varbinder::LocalVariable *CreateSyntheticNamespaceMember(const SyntheticNamespaceMemberArgs &args)
{
    auto *checker = args.checker;
    auto *decl = checker->ProgramAllocator()->New<varbinder::LetDecl>(args.memberName);
    auto flags = varbinder::VariableFlags::PUBLIC | varbinder::VariableFlags::READONLY |
                 varbinder::VariableFlags::INITIALIZED | varbinder::VariableFlags::SYNTHETIC | args.exportFlags;
    auto *var = checker->ProgramAllocator()->New<varbinder::LocalVariable>(decl, flags);
    decl->BindNode(NamespaceMemberDeclNode(args.namespaceObject, args.source));
    var->SetTsType(CreateNamespaceMemberType(checker, args.memberName, args.type, args.exportFlags));
    return var;
}

static bool CanBindNamespaceMemberToExportedVariable(const varbinder::Variable *var)
{
    if (var == nullptr || !var->IsLocalVariable() || var->Declaration() == nullptr ||
        var->Declaration()->Node() == nullptr || var->HasFlag(varbinder::VariableFlags::TYPE_ALIAS)) {
        return false;
    }

    switch (var->Declaration()->Node()->Type()) {
        case ir::AstNodeType::CLASS_PROPERTY:
        case ir::AstNodeType::METHOD_DEFINITION:
        case ir::AstNodeType::CLASS_DEFINITION:
            return true;
        default:
            return false;
    }
}

template <checker::PropertyType TYPE>
static void AddBindingModuleObjectProperty(ETSChecker *checker, checker::ETSObjectType *moduleObjType,
                                           ir::ETSImportDeclaration *importDecl, varbinder::Variable *var,
                                           const util::StringView &importPath)
{
    auto [foundInSpecifiers, aliasedName] = FindSpecifierForModuleObject(importDecl, var);
    auto node = var->AsLocalVariable()->Declaration()->Node();
    const auto isFromDynamicDefaultImport =
        node->IsDefaultExported() && var->HasFlag(varbinder::VariableFlags::DYNAMIC);
    const auto isReExportDefault = node->IsDefaultExported() && !importDecl->Specifiers().empty() &&
                                   !importDecl->Specifiers()[0]->IsImportNamespaceSpecifier();
    if (!node->IsValidInCurrentPhase() ||
        (!(node->IsExported() || isFromDynamicDefaultImport || node->HasExportAlias() || isReExportDefault) ||
         !foundInSpecifiers)) {
        return;
    }

    if (node->IsMethodDefinition()) {
        checker->BuildExportedFunctionSignature(var);
    }
    if (!aliasedName.Empty()) {
        moduleObjType->AddReExportAlias(var->Declaration()->Name(), aliasedName);
    }

    auto propNames = checker->FindPropNameForNamespaceImport(var->AsLocalVariable()->Name(), importPath);
    for (auto &propName : propNames) {
        if (CanBindNamespaceMemberToExportedVariable(var) && propName == var->AsLocalVariable()->Name()) {
            moduleObjType->AddProperty<TYPE>(var->AsLocalVariable(), propName);
            continue;
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *member =
            CreateSyntheticNamespaceMember({checker, moduleObjType, propName, ResolveNamespaceMemberType(checker, var),
                                            NamespaceMemberFlagsForExport(var), var});
        moduleObjType->AddProperty<TYPE>(member, propName);
    }
}

template <checker::PropertyType TYPE>
void ETSChecker::BindingsModuleObjectAddProperty(checker::ETSObjectType *moduleObjType,
                                                 ir::ETSImportDeclaration *importDecl,
                                                 const varbinder::Scope::VariableMap &bindings,
                                                 const util::StringView &importPath)
{
    ES2PANDA_ASSERT(importDecl != nullptr);
    for (const auto &binding : bindings) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        AddBindingModuleObjectProperty<TYPE>(this, moduleObjType, importDecl, binding.second, importPath);
    }
}

template void ETSChecker::BindingsModuleObjectAddProperty<PropertyType::INSTANCE_DECL>(
    ETSObjectType *, ir::ETSImportDeclaration *, const varbinder::Scope::VariableMap &, const util::StringView &);
template void ETSChecker::BindingsModuleObjectAddProperty<PropertyType::INSTANCE_METHOD>(
    ETSObjectType *, ir::ETSImportDeclaration *, const varbinder::Scope::VariableMap &, const util::StringView &);

std::vector<util::StringView> ETSChecker::FindPropNameForNamespaceImport(const util::StringView &originalName,
                                                                         const util::StringView &importPath)
{
    std::vector<util::StringView> results;
    const auto &store = VarBinder()->AsETSBinder()->GetExportFactsStore();
    if (const auto *surface = store.FindSurfaceByResolvedSource(importPath); surface != nullptr) {
        for (const auto &fact : VarBinder()->AsETSBinder()->GetExportFacts(surface->program).locals) {
            if (fact.localName == originalName && fact.variable != nullptr) {
                results.push_back(fact.exportedName);
            }
        }
    }

    if (results.empty()) {
        results.push_back(originalName);
    }
    return results;
}

void ETSChecker::SetPropertiesForModuleObject(checker::ETSObjectType *moduleObjType, const util::StringView &importPath,
                                              ir::ETSImportDeclaration *importDecl)
{
    parser::Program *program = VarBinder()->AsETSBinder()->GetExternalProgram(importDecl);
    ES2PANDA_ASSERT(program != nullptr);
    if (!program->IsASTChecked()) {
        varbinder::RecordTableContext recordTableCtx(VarBinder()->AsETSBinder(), program);
        checker::SavedCheckerContext savedContext(this, Context().Status(), Context().ContainingClass());
        if (!VarBinder()->AsETSBinder()->GetGlobalRecordTable()->IsExternal()) {
            RemoveStatus(CheckerStatus::IN_EXTERNAL);
        }
        auto savedProgram = Program();
        auto topScopeCtx = varbinder::TopScopeContext(VarBinder(), program->GlobalScope());
        VarBinder()->AsETSBinder()->SetProgram(program);
        program->SetASTChecked();
        program->Ast()->Check(this);
        VarBinder()->AsETSBinder()->SetProgram(savedProgram);
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    BindingsModuleObjectAddProperty<checker::PropertyType::STATIC_FIELD>(
        moduleObjType, importDecl, program->GlobalClassScope()->StaticFieldScope()->Bindings(), importPath);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    BindingsModuleObjectAddProperty<checker::PropertyType::STATIC_METHOD>(
        moduleObjType, importDecl, program->GlobalClassScope()->StaticMethodScope()->Bindings(), importPath);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    BindingsModuleObjectAddProperty<checker::PropertyType::STATIC_DECL>(
        moduleObjType, importDecl, program->GlobalClassScope()->StaticDeclScope()->Bindings(), importPath);

    BindingsModuleObjectAddProperty<checker::PropertyType::STATIC_DECL>(
        moduleObjType, importDecl, program->GlobalClassScope()->InstanceDeclScope()->Bindings(), importPath);

    BindingsModuleObjectAddProperty<checker::PropertyType::STATIC_DECL>(
        moduleObjType, importDecl, program->GlobalClassScope()->TypeAliasScope()->Bindings(), importPath);
}

void ETSChecker::SetrModuleObjectTsType(ir::Identifier *local, checker::ETSObjectType *moduleObjType)
{
    auto *etsBinder = static_cast<varbinder::ETSBinder *>(VarBinder());

    for (auto [bindingName, var] : etsBinder->TopScope()->Bindings()) {
        if (bindingName.Is(local->Name().Mutf8())) {
            var->SetTsType(moduleObjType);
        }
    }
}

Type *ETSChecker::GetImportSurfaceObjectType(varbinder::ExportSurfaceId surface, ir::Identifier *ident,
                                             varbinder::LocalVariable *localVar)
{
    if (surface.program == nullptr) {
        if (localVar != nullptr) {
            localVar->SetTsType(GlobalTypeError());
        }
        return ident->SetTsType(GlobalTypeError());
    }

    auto const moduleName =
        surface.moduleName.Empty() ? util::StringView {surface.program->ModuleName()} : surface.moduleName;
    auto const internalName = util::UString(std::string(moduleName)
                                                .append(compiler::Signatures::METHOD_SEPARATOR)
                                                .append(compiler::Signatures::ETS_GLOBAL),
                                            ProgramAllocator())
                                  .View();

    ir::AstNode *declNode = ident;
    if (localVar != nullptr && localVar->Declaration() != nullptr && localVar->Declaration()->Node() != nullptr &&
        localVar->Declaration()->Node()->IsImportNamespaceSpecifier()) {
        declNode = localVar->Declaration()->Node()->AsImportNamespaceSpecifier()->Local();
    }

    auto *moduleObjectType =
        ProgramAllocator()->New<ETSObjectType>(ProgramAllocator(), moduleName, internalName,
                                               std::make_tuple(declNode, checker::ETSObjectFlags::CLASS, Relation()));
    moduleObjectType->SetSuperType(GlobalETSObjectType());
    moduleObjectType->SetExportSurface(surface);
    ident->SetTsType(moduleObjectType);
    if (localVar != nullptr) {
        localVar->SetTsType(moduleObjectType);
    } else {
        SetrModuleObjectTsType(ident, moduleObjectType);
    }
    return moduleObjectType;
}

Type *ETSChecker::GetImportNamespaceObjectType(ir::ETSImportDeclaration *importDecl, ir::Identifier *ident)
{
    auto surface = exportClosureResolver_->GetImportedSurface(importDecl);
    if (surface.program == nullptr) {
        LogError(diagnostic::IMPORT_NOT_FOUND_2, {importDecl->ResolvedSource()}, ident->Start());
        return ident->SetTsType(GlobalTypeError());
    }

    auto *var = ident->Variable();
    auto *localVar = var != nullptr && var->IsLocalVariable() ? var->AsLocalVariable() : nullptr;
    return GetImportSurfaceObjectType(surface, ident, localVar);
}

static checker::PropertyType PropertyTypeForNamespaceExport(const varbinder::Variable *var)
{
    if (var == nullptr || var->Declaration() == nullptr) {
        return checker::PropertyType::STATIC_DECL;
    }

    if (var->Declaration()->IsFunctionDecl()) {
        return checker::PropertyType::STATIC_METHOD;
    }

    auto *node = var->Declaration()->Node();
    if (node != nullptr && node->IsClassProperty() && node->AsClassProperty()->IsTopLevelLexicalDecl()) {
        return checker::PropertyType::STATIC_FIELD;
    }

    return checker::PropertyType::STATIC_DECL;
}

static bool MaterializeNestedNamespaceMember(ETSChecker *checker, checker::ETSObjectType *namespaceObject,
                                             util::StringView memberName, const checker::ResolvedExportResult *resolved)
{
    auto *nestedType = checker->ProgramAllocator()->New<ETSObjectType>(
        checker->ProgramAllocator(), resolved->entry.surface.moduleName, resolved->entry.surface.moduleName,
        std::make_tuple(namespaceObject->GetDeclNode(), checker::ETSObjectFlags::CLASS, checker->Relation()));
    nestedType->SetSuperType(checker->GlobalETSObjectType());
    nestedType->SetExportSurface(resolved->entry.surface);
    auto *var = CreateSyntheticNamespaceMember(
        {checker, namespaceObject, memberName, nestedType, varbinder::VariableFlags::NAMESPACE, nullptr});
    namespaceObject->AddProperty<checker::PropertyType::STATIC_DECL>(var, memberName);
    return true;
}

static bool MaterializeVariableNamespaceMember(ETSChecker *checker, checker::ETSObjectType *namespaceObject,
                                               util::StringView memberName, varbinder::Variable *var)
{
    if (var == nullptr || !var->IsLocalVariable()) {
        return false;
    }

    if (var->AsLocalVariable()->Declaration()->Node()->IsMethodDefinition()) {
        checker->BuildExportedFunctionSignature(var);
    }

    if (var->HasFlag(varbinder::VariableFlags::TYPE_ALIAS)) {
        namespaceObject->AddProperty<checker::PropertyType::STATIC_DECL>(var->AsLocalVariable(), memberName);
        return true;
    }

    const auto propertyType = PropertyTypeForNamespaceExport(var);
    if (CanBindNamespaceMemberToExportedVariable(var)) {
        auto *member = var->AsLocalVariable();
        switch (propertyType) {
            case checker::PropertyType::STATIC_METHOD:
                namespaceObject->AddProperty<checker::PropertyType::STATIC_METHOD>(member, memberName);
                break;
            case checker::PropertyType::STATIC_FIELD:
                namespaceObject->AddProperty<checker::PropertyType::STATIC_FIELD>(member, memberName);
                break;
            case checker::PropertyType::STATIC_DECL:
                namespaceObject->AddProperty<checker::PropertyType::STATIC_DECL>(member, memberName);
                break;
            default:
                ES2PANDA_UNREACHABLE();
        }
        return true;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *member =
        CreateSyntheticNamespaceMember({checker, namespaceObject, memberName, ResolveNamespaceMemberType(checker, var),
                                        NamespaceMemberFlagsForExport(var), var});
    switch (propertyType) {
        case checker::PropertyType::STATIC_METHOD:
            namespaceObject->AddProperty<checker::PropertyType::STATIC_METHOD>(member, memberName);
            break;
        case checker::PropertyType::STATIC_FIELD:
            namespaceObject->AddProperty<checker::PropertyType::STATIC_FIELD>(member, memberName);
            break;
        case checker::PropertyType::STATIC_DECL:
            namespaceObject->AddProperty<checker::PropertyType::STATIC_DECL>(member, memberName);
            break;
        default:
            ES2PANDA_UNREACHABLE();
    }
    return true;
}

static void MaterializeErrorNamespaceMember(ETSChecker *checker, checker::ETSObjectType *namespaceObject,
                                            util::StringView memberName)
{
    auto *member = CreateSyntheticNamespaceMember(
        {checker, namespaceObject, memberName, checker->GlobalTypeError(), varbinder::VariableFlags::NONE, nullptr});
    namespaceObject->AddProperty<checker::PropertyType::STATIC_DECL>(member, memberName);
}

bool ETSChecker::MaterializeNamespaceMember(checker::ETSObjectType *namespaceObject, util::StringView memberName,
                                            const lexer::SourcePosition &pos)
{
    if (namespaceObject == nullptr || !namespaceObject->HasExportSurface()) {
        return false;
    }

    if (memberName.Is("default")) {
        LogError(diagnostic::DEFAULT_EXPORT_DIRECT_IMPORTED, {}, pos);
        return true;
    }

    auto exportSurface = namespaceObject->ExportSurface();
    auto *resolver = exportClosureResolver_;
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    const auto *resolved = resolver->ResolveExportNameWithoutAmbiguousDiagnostic(exportSurface, memberName);
    auto namespaceExportMember = checker::ResolvedExportResult {};
    if (resolved == nullptr || resolved->status == ExportResolutionStatus::NOT_FOUND) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        namespaceExportMember = resolver->ResolveNamespaceExportMember(exportSurface, memberName);
        if (namespaceExportMember.status != ExportResolutionStatus::NOT_FOUND) {
            resolved = &namespaceExportMember;
        }
    }

    if (resolved == nullptr) {
        return false;
    }

    auto materialized = checker::ResolvedExportResult {};
    if (resolved->status == ExportResolutionStatus::RESOLVED) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        materialized = resolver->SelectMaterializedSurfaceEntry(exportSurface, memberName, *resolved);
        resolved = &materialized;
    }

    switch (resolved->status) {
        case ExportResolutionStatus::RESOLVED: {
            if (resolved->entry.surface.program != nullptr) {
                return MaterializeNestedNamespaceMember(this, namespaceObject, memberName, resolved);
            }

            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            return MaterializeVariableNamespaceMember(this, namespaceObject, memberName, resolved->entry.variable);
        }
        case ExportResolutionStatus::AMBIGUOUS:
            LogError(diagnostic::AMBIGUOUS_REFERENCE, {memberName}, pos);
            MaterializeErrorNamespaceMember(this, namespaceObject, memberName);
            return true;
        case ExportResolutionStatus::NOT_FOUND:
            return false;
        case ExportResolutionStatus::INVALID:
            LogError(diagnostic::CYCLIC_EXPORT, pos);
            MaterializeErrorNamespaceMember(this, namespaceObject, memberName);
            return true;
        default:
            ES2PANDA_UNREACHABLE();
    }
}

}  // namespace ark::es2panda::checker
