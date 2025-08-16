/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "dynamicImport.h"
#include <algorithm>
#include <string>
#include "checker/ETSchecker.h"
#include "ir/astNode.h"

#include "compiler/lowering/util.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"

namespace ark::es2panda::compiler {

using AstNodePtr = ir::AstNode *;
static constexpr std::string_view LAZY_IMPORT_OBJECT_SUFFIX = "%%lazyImportObject-";
static constexpr std::string_view FIELD_NAME = "value";

static checker::Type *CreateModuleObjectType(public_lib::Context *ctx, ir::ETSImportDeclaration *importDecl);

ir::ClassDeclaration *GetOrCreateLazyImportObjectClass(ark::ArenaAllocator *allocator,
                                                       ir::ETSImportDeclaration *importDecl, parser::Program *program)
{
    auto checker = program->Checker()->AsETSChecker();
    auto globalClass = program->GlobalClass();
    auto varbinder = checker->VarBinder()->AsETSBinder();
    auto sourceProgram = checker->SelectEntryOrExternalProgram(varbinder, importDecl->DeclPath());

    const std::string classNameStr = std::string(LAZY_IMPORT_OBJECT_SUFFIX) + sourceProgram->ModuleName().Mutf8();
    const util::UString className(classNameStr, allocator);
    const auto nameView = className.View().Mutf8();

    auto &classBody = globalClass->BodyForUpdate();
    auto classIt = std::find_if(classBody.begin(), classBody.end(), [&nameView](ir::AstNode *node) {
        if (!node->IsClassDeclaration()) {
            return false;
        }
        return node->AsClassDeclaration()->Definition()->Ident()->Name().Mutf8() == nameView;
    });
    if (classIt != classBody.end()) {
        return (*classIt)->AsClassDeclaration();
    }

    auto *ident = allocator->New<ir::Identifier>(className.View(), allocator);
    auto *classDef = util::NodeAllocator::ForceSetParent<ir::ClassDefinition>(
        allocator, allocator, ident, ir::ClassDefinitionModifiers::CLASS_DECL, ir::ModifierFlags::ABSTRACT,
        Language(Language::Id::ETS));

    classDef->SetLazyImportObjectClass();
    return util::NodeAllocator::ForceSetParent<ir::ClassDeclaration>(allocator, classDef, allocator);
}

static void AddImportInitializationStatement(public_lib::Context *ctx, ir::ETSImportDeclaration *import,
                                             ArenaVector<ir::Statement *> *statements, util::StringView className)
{
    auto allocator = ctx->GetChecker()->AsETSChecker()->ProgramAllocator();
    const auto builtin = compiler::Signatures::Dynamic::LoadModuleBuiltin(import->Language());
    auto [builtinClassName, builtinMethodName] = util::Helpers::SplitSignature(builtin);

    auto *classId = allocator->New<ir::Identifier>(builtinClassName, allocator);
    auto *methodId = allocator->New<ir::Identifier>(builtinMethodName, allocator);
    auto *callee = util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
        allocator, classId, methodId, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);

    util::StringView ohmUrl = util::UString(import->OhmUrl(), allocator).View();
    if (ohmUrl.Empty()) {
        ohmUrl = import->ResolvedSource();
        if (ark::os::file::File::IsRegularFile(ohmUrl.Mutf8())) {
            ohmUrl = util::UString(ark::os::RemoveExtension(ohmUrl.Mutf8()), allocator).View();
        }
    }

    ArenaVector<ir::Expression *> callParams(allocator->Adapter());
    callParams.push_back(allocator->New<ir::StringLiteral>(ohmUrl));

    auto *loadCall = util::NodeAllocator::ForceSetParent<ir::CallExpression>(allocator, callee, std::move(callParams),
                                                                             nullptr, false);
    auto *moduleClassId = allocator->New<ir::Identifier>(className, allocator);
    auto *fieldId = allocator->New<ir::Identifier>(FIELD_NAME, allocator);
    auto *property = util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
        allocator, moduleClassId, fieldId, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);

    moduleClassId->SetParent(property);
    fieldId->SetParent(property);

    auto *initializer = util::NodeAllocator::ForceSetParent<ir::AssignmentExpression>(
        allocator, property, loadCall, lexer::TokenType::PUNCTUATOR_SUBSTITUTION);

    statements->push_back(util::NodeAllocator::ForceSetParent<ir::ExpressionStatement>(allocator, initializer));
}

static void ImportNamespaceObjectTypeAddReExportType(public_lib::Context *ctx, varbinder::ETSBinder *varbinder,
                                                     ir::ETSImportDeclaration *importDecl,
                                                     checker::ETSObjectType *lastObjectType)
{
    for (auto item : varbinder->AsETSBinder()->ReExportImports()) {
        if (importDecl->DeclPath() != item->GetProgramPath().Mutf8()) {
            continue;
        }
        auto *reExportType = CreateModuleObjectType(ctx, item->GetETSImportDeclarations());
        if (reExportType->IsTypeError()) {
            continue;
        }
        ES2PANDA_ASSERT(lastObjectType != nullptr);
        lastObjectType->AddReExports(reExportType->AsETSObjectType());
        for (auto node : importDecl->Specifiers()) {
            if (node->IsImportSpecifier()) {
                auto specifier = node->AsImportSpecifier();
                lastObjectType->AddReExportAlias(specifier->Imported()->Name(), specifier->Local()->Name());
            }
        }
    }
}

static void SetPropertiesForModuleObject(public_lib::Context *ctx, checker::ETSObjectType *moduleObjType,
                                         const util::StringView &importPath, ir::ETSImportDeclaration *importDecl)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto varbinder = static_cast<varbinder::ETSBinder *>(checker->VarBinder()->AsETSBinder());
    parser::Program *program =
        checker->SelectEntryOrExternalProgram(static_cast<varbinder::ETSBinder *>(varbinder), importPath);
    // Check imported properties before assigning them to module object
    ES2PANDA_ASSERT(program != nullptr);
    if (!program->IsASTChecked()) {
        // NOTE: helps to avoid endless loop in case of recursive imports that uses all bindings
        program->SetASTChecked();
        program->Ast()->Check(checker);
    }

    checker->BindingsModuleObjectAddProperty<checker::PropertyType::INSTANCE_DECL>(
        moduleObjType, importDecl, program->GlobalClassScope()->StaticFieldScope()->Bindings(), importPath);

    checker->BindingsModuleObjectAddProperty<checker::PropertyType::INSTANCE_METHOD>(
        moduleObjType, importDecl, program->GlobalClassScope()->StaticMethodScope()->Bindings(), importPath);

    checker->BindingsModuleObjectAddProperty<checker::PropertyType::INSTANCE_DECL>(
        moduleObjType, importDecl, program->GlobalClassScope()->StaticDeclScope()->Bindings(), importPath);

    checker->BindingsModuleObjectAddProperty<checker::PropertyType::INSTANCE_DECL>(
        moduleObjType, importDecl, program->GlobalClassScope()->InstanceDeclScope()->Bindings(), importPath);

    checker->BindingsModuleObjectAddProperty<checker::PropertyType::INSTANCE_DECL>(
        moduleObjType, importDecl, program->GlobalClassScope()->TypeAliasScope()->Bindings(), importPath);
}

static checker::Type *CreateModuleObjectType(public_lib::Context *ctx, ir::ETSImportDeclaration *importDecl)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto varbinder = static_cast<varbinder::ETSBinder *>(checker->VarBinder()->AsETSBinder());
    auto allocator = checker->ProgramAllocator();

    const auto importPath = importDecl->DeclPath() == util::ImportPathManager::DUMMY_PATH ? importDecl->ResolvedSource()
                                                                                          : importDecl->DeclPath();
    auto program = checker->SelectEntryOrExternalProgram(varbinder, importPath);
    if (program == nullptr) {
        return checker->GlobalTypeError();
    }

    const auto moduleName = program->ModuleName();
    const auto internalNameStr = std::string(moduleName.Mutf8())
                                     .append(compiler::Signatures::METHOD_SEPARATOR)
                                     .append(compiler::Signatures::ETS_GLOBAL);
    const util::UString internalName(internalNameStr, allocator);

    auto *moduleObjectType = allocator->New<checker::ETSObjectType>(
        allocator, moduleName, internalName.View(),
        std::make_tuple(program->GlobalClass(), checker::ETSObjectFlags::CLASS, checker->Relation()));

    auto *rootDecl = allocator->New<varbinder::ClassDecl>(moduleName);
    auto *rootVar = allocator->New<varbinder::LocalVariable>(rootDecl, varbinder::VariableFlags::NONE);
    rootVar->SetTsType(moduleObjectType);
    ImportNamespaceObjectTypeAddReExportType(ctx, program->VarBinder()->AsETSBinder(), importDecl, moduleObjectType);
    SetPropertiesForModuleObject(ctx, moduleObjectType, importPath, nullptr);
    moduleObjectType->AddObjectFlag(checker::ETSObjectFlags::LAZY_IMPORT_OBJECT);

    return moduleObjectType;
}

static void FillVarMapForImportSpecifiers(const ArenaVector<ir::AstNode *> &specifiers, ir::ClassDefinition *classDef,
                                          ArenaUnorderedMap<varbinder::Variable *, ir::ClassDefinition *> &varMap)
{
    for (auto specifier : specifiers) {
        if (specifier->IsImportSpecifier()) {
            auto *var = specifier->AsImportSpecifier()->Imported()->Variable();
            var->AddFlag(varbinder::VariableFlags::DYNAMIC);
            varMap.insert({var, classDef});
        } else if (specifier->IsImportNamespaceSpecifier()) {
            auto *var = specifier->AsImportNamespaceSpecifier()->Local()->Variable();
            var->AddFlag(varbinder::VariableFlags::DYNAMIC);
            varMap.insert({var, classDef});
        }
    }
}

static void BuildLazyImportObject(public_lib::Context *ctx, ir::ETSImportDeclaration *importDecl,
                                  parser::Program *program,
                                  ArenaUnorderedMap<util::StringView, checker::ETSObjectType *> &moduleMap,
                                  ArenaUnorderedMap<varbinder::Variable *, ir::ClassDefinition *> &varMap)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto varBinder = checker->VarBinder()->AsETSBinder();
    auto allocator = checker->ProgramAllocator();

    auto declProgram = checker->SelectEntryOrExternalProgram(varBinder, importDecl->DeclPath());
    if (!declProgram->IsDeclForDynamicStaticInterop()) {
        return;
    }

    auto *classDecl = GetOrCreateLazyImportObjectClass(allocator, importDecl, program);
    FillVarMapForImportSpecifiers(importDecl->Specifiers(), classDecl->Definition(), varMap);

    const auto className = classDecl->Definition()->Ident()->Name();
    auto found = moduleMap.find(className);
    if (declProgram->IsASTChecked() && found != moduleMap.end()) {
        checker->SetPropertiesForModuleObject(found->second, importDecl->DeclPath(), nullptr);
        return;
    }

    auto *objType = CreateModuleObjectType(ctx, importDecl)->AsETSObjectType();
    moduleMap.insert({className, objType});

    auto moduleType = checker->CreateGradualType(objType, Language::Id::JS);

    auto parser = ctx->parser->AsETSParser();
    auto *typeAnnotation = allocator->New<ir::OpaqueTypeNode>(moduleType, allocator);
    auto *classProp = parser->CreateFormattedClassFieldDefinition(std::string {FIELD_NAME} + ": @@T1", typeAnnotation)
                          ->AsClassProperty();
    typeAnnotation->SetParent(classProp);
    classProp->AddModifier(ir::ModifierFlags::CONST | ir::ModifierFlags::STATIC);

    classDecl->Definition()->EmplaceBody(classProp);
    classProp->SetParent(classDecl->Definition());

    auto initializer = checker->CreateClassStaticInitializer(
        [ctx, importDecl, className](ArenaVector<ir::Statement *> *statements,
                                     [[maybe_unused]] ArenaVector<ir::Expression *> *params) {
            AddImportInitializationStatement(ctx, importDecl, statements, className);
        });

    classDecl->Definition()->EmplaceBody(initializer);
    initializer->SetParent(classDecl->Definition());

    program->GlobalClass()->EmplaceBody(classDecl);
    classDecl->SetParent(program->GlobalClass());

    auto lexScope = varbinder::LexicalScope<varbinder::Scope>::Enter(varBinder, program->GlobalClassScope());
    InitScopesPhaseETS::RunExternalNode(classDecl, varBinder);
    varBinder->ResolveReferencesForScopeWithContext(classDecl, varBinder->TopScope());
    classDecl->Check(checker);
}

static ir::MemberExpression *CreateTripleMemberExpr(ArenaAllocator *allocator, const util::StringView &left,
                                                    const util::StringView &middle, const util::StringView &right)
{
    auto *leftId = allocator->New<ir::Identifier>(left, allocator);
    auto *middleId = allocator->New<ir::Identifier>(middle, allocator);
    auto *expr = util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
        allocator, leftId, middleId, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    auto *rightId = allocator->New<ir::Identifier>(right, allocator);
    return util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
        allocator, expr, rightId, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
}

static AstNodePtr TransformIdentifier(ir::Identifier *ident, public_lib::Context *ctx,
                                      const ArenaUnorderedMap<varbinder::Variable *, ir::ClassDefinition *> &varMap)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto varBinder = checker->VarBinder()->AsETSBinder();
    auto allocator = checker->ProgramAllocator();
    if (ident->Variable()->Declaration() != nullptr && ident->Variable()->Declaration()->Node() != nullptr &&
        ident->Variable()->Declaration()->Node()->IsImportNamespaceSpecifier()) {
        return ident;
    }

    const auto parent = ident->Parent();
    auto isTransformedNode =
        (parent->IsMemberExpression() && parent->AsMemberExpression()->ObjType() != nullptr &&
         parent->AsMemberExpression()->ObjType()->HasObjectFlag(checker::ETSObjectFlags::LAZY_IMPORT_OBJECT));
    if (parent->IsImportSpecifier() || parent->IsImportNamespaceSpecifier() || parent->IsScriptFunction() ||
        parent->IsMethodDefinition() || isTransformedNode) {
        return ident;
    }

    auto varIt = varMap.find(ident->Variable());
    if (varIt == varMap.end()) {
        return ident;
    }

    auto *memberExpr =
        CreateTripleMemberExpr(allocator, varIt->second->Ident()->Name(), FIELD_NAME, ident->Variable()->Name());
    memberExpr->SetParent(parent);
    // Ensure that it will not be incorrectly converted to ArrowType.
    if (parent->IsCallExpression() && parent->AsCallExpression()->Callee() == ident) {
        parent->AsCallExpression()->SetCallee(memberExpr);
    }
    CheckLoweredNode(varBinder, checker, memberExpr);
    return memberExpr;
}

AstNodePtr TransformTsQualifiedName(ir::TSQualifiedName *qualifiedName, public_lib::Context *ctx,
                                    const ArenaUnorderedMap<varbinder::Variable *, ir::ClassDefinition *> &varMap)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto varBinder = checker->VarBinder()->AsETSBinder();
    auto allocator = checker->ProgramAllocator();
    if (!qualifiedName->Left()->IsIdentifier()) {
        return qualifiedName;
    }
    auto *moduleId = qualifiedName->Left()->AsIdentifier();
    if (moduleId->Variable() != nullptr && moduleId->Variable()->Declaration() != nullptr &&
        !moduleId->Variable()->Declaration()->Node()->IsImportNamespaceSpecifier()) {
        return qualifiedName;
    }

    auto varIt = varMap.find(moduleId->Variable());
    if (varIt == varMap.end()) {
        return qualifiedName;
    }
    const auto parent = qualifiedName->Parent();
    auto *newIdent = allocator->New<ir::Identifier>(qualifiedName->Right()->AsIdentifier()->Name(), allocator);
    auto *memberExpr = CreateTripleMemberExpr(allocator, varIt->second->Ident()->Name(), FIELD_NAME, newIdent->Name());
    memberExpr->SetParent(parent);
    // Ensure that it will not be incorrectly converted to ArrowType.
    if (parent->IsCallExpression() && parent->AsCallExpression()->Callee() == qualifiedName) {
        parent->AsCallExpression()->SetCallee(memberExpr);
    }
    CheckLoweredNode(varBinder, checker, memberExpr);
    return memberExpr;
}

AstNodePtr TransformMemberExpression(ir::MemberExpression *memberExpr, public_lib::Context *ctx,
                                     const ArenaUnorderedMap<varbinder::Variable *, ir::ClassDefinition *> &varMap)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto varBinder = checker->VarBinder()->AsETSBinder();
    auto allocator = checker->ProgramAllocator();
    if (!memberExpr->Object()->IsIdentifier() || !memberExpr->Property()->IsIdentifier()) {
        return memberExpr;
    }
    auto *moduleId = memberExpr->Object()->AsIdentifier();
    if (moduleId->Variable()->Declaration() != nullptr &&
        !moduleId->Variable()->Declaration()->Node()->IsImportNamespaceSpecifier()) {
        return memberExpr;
    }
    auto varIt = varMap.find(moduleId->Variable());
    if (varIt == varMap.end()) {
        return memberExpr;
    }
    const auto parent = memberExpr->Parent();
    auto *newIdent = allocator->New<ir::Identifier>(memberExpr->Property()->AsIdentifier()->Name(), allocator);
    auto *res = CreateTripleMemberExpr(allocator, varIt->second->Ident()->Name(), FIELD_NAME, newIdent->Name());
    res->SetParent(parent);

    // Ensure that it will not be incorrectly converted to ArrowType.
    if (parent->IsCallExpression() && parent->AsCallExpression()->Callee() == memberExpr) {
        parent->AsCallExpression()->SetCallee(res);
    }
    CheckLoweredNode(varBinder, checker, res);

    return res;
}

bool DynamicImport::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    auto dynamicImports = program->VarBinder()->AsETSBinder()->DynamicImports();
    if (dynamicImports.empty()) {
        return true;
    }
    auto allocator = ctx->GetChecker()->ProgramAllocator();
    ArenaUnorderedMap<varbinder::Variable *, ir::ClassDefinition *> varMap {allocator->Adapter()};
    ArenaUnorderedMap<util::StringView, checker::ETSObjectType *> moduleMap {allocator->Adapter()};

    for (auto *importDecl : dynamicImports) {
        BuildLazyImportObject(ctx, importDecl, program, moduleMap, varMap);
    }

    program->Ast()->TransformChildrenRecursively(
        [ctx, &varMap](ir::AstNode *node) -> AstNodePtr {
            if (node->IsIdentifier() && node->AsIdentifier()->Variable() != nullptr) {
                return TransformIdentifier(node->AsIdentifier(), ctx, varMap);
            }
            if (node->IsTSQualifiedName()) {
                return TransformTsQualifiedName(node->AsTSQualifiedName(), ctx, varMap);
            }
            if (node->IsMemberExpression() && node->AsMemberExpression()->PropVar() != nullptr) {
                return TransformMemberExpression(node->AsMemberExpression(), ctx, varMap);
            }
            return node;
        },
        Name());

    return true;
}
}  // namespace ark::es2panda::compiler