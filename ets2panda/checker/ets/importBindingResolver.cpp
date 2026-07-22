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

#include "checker/types/ets/etsObjectType.h"
#include "generated/diagnostic.h"
#include "ir/ets/etsFunctionType.h"
#include "ir/ets/etsParameterExpression.h"
#include "ir/ets/etsTypeReference.h"
#include "ir/ets/etsTypeReferencePart.h"
#include "ir/ets/etsUnionType.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/module/importDeclaration.h"
#include "ir/module/importDefaultSpecifier.h"
#include "ir/module/importNamespaceSpecifier.h"
#include "ir/module/importSpecifier.h"
#include "ir/statements/annotationDeclaration.h"
#include "ir/statements/annotationUsage.h"
#include "parser/program/program.h"
#include "util/es2pandaMacros.h"
#include "util/helpers.h"
#include "varbinder/ETSBinder.h"

namespace ark::es2panda::checker {

static const parser::Program *DeclarationProgram(const varbinder::Variable *var)
{
    if (var == nullptr || var->Declaration() == nullptr || var->Declaration()->Node() == nullptr) {
        return nullptr;
    }
    return var->Declaration()->Node()->Start().Program();
}

static bool IsDynamicStaticInteropProgram(const parser::Program *program)
{
    return program != nullptr && program->IsDeclForDynamicStaticInterop();
}

static parser::Program *ImportTargetProgram(ETSChecker *checker, varbinder::LocalVariable *localVar)
{
    auto *bindingInfo = localVar == nullptr ? nullptr : localVar->ImportBinding();
    if (bindingInfo == nullptr || bindingInfo->importDecl == nullptr) {
        return nullptr;
    }

    return checker->VarBinder()->AsETSBinder()->GetExternalProgram(bindingInfo->importDecl);
}

static bool IsDynamicStaticInteropImportTarget(ETSChecker *checker, varbinder::LocalVariable *localVar)
{
    return IsDynamicStaticInteropProgram(ImportTargetProgram(checker, localVar));
}

static bool IsDynamicStaticInteropImportTarget(ETSChecker *checker, varbinder::LocalVariable *localVar,
                                               const ResolvedImportResult &result)
{
    if (result.status != ImportResolutionStatus::RESOLVED_VARIABLE) {
        return false;
    }
    if (IsDynamicStaticInteropProgram(result.entry.originProgram) ||
        IsDynamicStaticInteropProgram(DeclarationProgram(result.entry.variable))) {
        return true;
    }

    return IsDynamicStaticInteropProgram(ImportTargetProgram(checker, localVar));
}

static bool IsCallCallee(const ir::Identifier *ident)
{
    return ident->Parent() != nullptr && ident->Parent()->IsCallExpression() &&
           ident->Parent()->AsCallExpression()->Callee() == ident;
}

static void ValidateImportCallIdentifier(ETSChecker *checker, ir::Identifier *ident, Type *type)
{
    if (type == nullptr || !type->IsETSObjectType()) {
        return;
    }

    checker->ResolveDeclaredMembersOfObject(type);
    auto searchFlag = PropertySearchFlags::SEARCH_IN_INTERFACES | PropertySearchFlags::SEARCH_IN_BASE |
                      PropertySearchFlags::SEARCH_STATIC_METHOD;
    auto *objectType = type->AsETSObjectType();
    if (objectType->GetProperty(compiler::Signatures::STATIC_INSTANTIATE_METHOD, searchFlag) != nullptr ||
        objectType->GetProperty(compiler::Signatures::STATIC_INVOKE_METHOD, searchFlag) != nullptr) {
        checker->ValidateCallExpressionIdentifier(ident, type);
    }
}

static bool ShouldSkipValueImportMaterialization(ETSChecker *checker, varbinder::LocalVariable *localVar,
                                                 const ResolvedImportResult &result)
{
    return IsDynamicStaticInteropImportTarget(checker, localVar, result);
}

static Type *MaterializeResolvedImportIdentifier(ETSChecker *checker, ir::Identifier *ident,
                                                 varbinder::LocalVariable *localVar, const ResolvedImportResult &result)
{
    if (ShouldSkipValueImportMaterialization(checker, localVar, result)) {
        // Dynamic-static interop imports still need callable-class validation
        // so imported components can resolve through $_invoke/$_instantiate.
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *type = checker->ResolveImportBindingType(localVar, ident);
        if (IsCallCallee(ident)) {
            ValidateImportCallIdentifier(checker, ident, type);
        }
        return ident->SetTsType(type);
    }

    auto *target = checker->ResolveEffectiveVariable(localVar);
    if (target == nullptr) {
        return nullptr;
    }

    ident->SetVariable(target);
    if (!IsCallCallee(ident)) {
        return ident->SetTsType(checker->GetTypeOfVariable(target));
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *type = checker->ResolveImportBindingType(localVar, ident);
    ValidateImportCallIdentifier(checker, ident, type);
    return ident->SetTsType(type);
}

static bool IsNamespaceImportValueUse(ETSChecker *checker, ir::Identifier *ident, varbinder::LocalVariable *localVar,
                                      Type *type)
{
    auto *bindingInfo = localVar->ImportBinding();
    return bindingInfo != nullptr && bindingInfo->kind == varbinder::ImportBindingKind::NAMESPACE &&
           (checker->IsCallArgument(ident) || checker->IsNamespaceObjectValueUse(ident, type));
}

Type *ETSChecker::MaterializeImportIdentifier(ir::Identifier *ident, varbinder::LocalVariable *localVar)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto result = ResolveImportBinding(localVar, {false, true});
    if (result.status == ImportResolutionStatus::RESOLVED_VARIABLE) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *type = MaterializeResolvedImportIdentifier(this, ident, localVar, result);
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return type != nullptr ? type : ident->SetTsType(ResolveImportBindingType(localVar, ident));
    }

    if (result.status == ImportResolutionStatus::RESOLVED_SURFACE) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *type = GetImportSurfaceObjectType(result.surface, ident, localVar);
        if (IsCallCallee(ident)) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            ValidateCallExpressionIdentifier(ident, type);
            return ident->SetTsType(GlobalTypeError());
        }
        if (IsNamespaceImportValueUse(this, ident, localVar, type)) {
            LogError(diagnostic::NAMESPACE_AS_OBJ, {ident->Name()}, ident->Start());
            return ident->SetTsType(GlobalTypeError());
        }
        return ident->SetTsType(type);
    }

    if (result.status == ImportResolutionStatus::NOT_FOUND || result.status == ImportResolutionStatus::INVALID) {
        LogError(diagnostic::UNRESOLVED_REF, {ident->Name()}, ident->Start());
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return ident->SetTsType(ResolveImportBindingType(localVar, ident));
}

static bool IsImportDeclarationIdentifier(const ir::Identifier *ident)
{
    auto *parent = ident == nullptr ? nullptr : ident->Parent();
    return parent != nullptr &&
           (parent->IsImportSpecifier() || parent->IsImportDefaultSpecifier() || parent->IsImportNamespaceSpecifier());
}

Type *ETSChecker::MaterializePrecheckedImportIdentifier(ir::Identifier *ident)
{
    auto *var = ident == nullptr ? nullptr : ident->Variable();
    if (IsImportDeclarationIdentifier(ident) || var == nullptr || !var->IsLocalVariable() ||
        !var->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
        return nullptr;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return MaterializeImportIdentifier(ident, var->AsLocalVariable());
}

static bool IsImportBindingVariable(varbinder::Variable *var)
{
    return var != nullptr && var->IsLocalVariable() && var->HasFlag(varbinder::VariableFlags::IMPORT_BINDING);
}

static ir::AnnotationDeclaration *AsAnnotationDeclaration(varbinder::Variable *var)
{
    if (var == nullptr || var->Declaration() == nullptr || var->Declaration()->Node() == nullptr ||
        !var->Declaration()->Node()->IsAnnotationDeclaration()) {
        return nullptr;
    }
    return var->Declaration()->Node()->AsAnnotationDeclaration();
}

ir::AnnotationDeclaration *ETSChecker::MaterializeAnnotationUsage(ir::AnnotationUsage *annotation,
                                                                  [[maybe_unused]] AnnotationUseKind kind)
{
    auto *baseName = annotation == nullptr ? nullptr : annotation->GetBaseName();
    if (baseName == nullptr || baseName->IsErrorPlaceHolder()) {
        return nullptr;
    }

    // USER and META annotation uses consume imports the same way. META-specific validation, namely requiring a
    // standard annotation on @interface declarations, runs in CheckStandardAnnotation() after this resolves the name.
    ES2PANDA_ASSERT(kind == AnnotationUseKind::USER || kind == AnnotationUseKind::META);

    auto *sourceVar = baseName->Variable();
    if (IsImportBindingVariable(sourceVar)) {
        auto *bindingInfo = sourceVar->AsLocalVariable()->ImportBinding();
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto result = ResolveImportBinding(sourceVar->AsLocalVariable(), {false, true});
        varbinder::Variable *target = nullptr;
        if (result.status == ImportResolutionStatus::RESOLVED_VARIABLE) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            target = ResolveEffectiveVariable(sourceVar);
        }
        if (auto *decl = AsAnnotationDeclaration(target); decl != nullptr) {
            if (bindingInfo != nullptr && bindingInfo->isTypeOnly && !IsAnyError()) {
                LogError(diagnostic::IMPORT_TYPE_NOT_ALLOWED, {}, baseName->Start());
                return nullptr;
            }
            if (result.entry.isTypeOnlyUse && !IsAnyError()) {
                LogError(diagnostic::ANNOTATION_AS_TYPE, {}, baseName->Start());
                return nullptr;
            }
            baseName->SetVariable(target);
            return decl;
        }
        return nullptr;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *resolved = ResolveEffectiveVariable(baseName->Variable());
    if (auto *decl = AsAnnotationDeclaration(resolved); decl != nullptr) {
        baseName->SetVariable(resolved);
        return decl;
    }
    return AsAnnotationDeclaration(baseName->Variable());
}

void ETSChecker::MaterializeAnnotationUsageBaseName(ir::AnnotationUsage *annotation)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    (void)MaterializeAnnotationUsage(annotation, AnnotationUseKind::USER);
}

static bool IsAnnotationVariable(varbinder::Variable *var)
{
    return var != nullptr && var->Declaration() != nullptr && var->Declaration()->Node() != nullptr &&
           var->Declaration()->Node()->IsAnnotationDeclaration();
}

enum class ImportConflictDeclarationKind { VARIABLE, FUNCTION, CLASS, NAMESPACE, INTERFACE, ANNOTATION };

static ImportConflictDeclarationKind DeclarationKind(varbinder::Variable *var)
{
    if (var == nullptr || var->Declaration() == nullptr || var->Declaration()->Node() == nullptr) {
        return ImportConflictDeclarationKind::VARIABLE;
    }

    auto *node = var->Declaration()->Node();
    if (node->IsClassDefinition()) {
        return node->AsClassDefinition()->IsNamespaceTransformed() ? ImportConflictDeclarationKind::NAMESPACE
                                                                   : ImportConflictDeclarationKind::CLASS;
    }
    if (node->IsClassDeclaration()) {
        return ImportConflictDeclarationKind::CLASS;
    }
    if (node->IsTSInterfaceDeclaration()) {
        return ImportConflictDeclarationKind::INTERFACE;
    }
    if (node->IsAnnotationDeclaration()) {
        return ImportConflictDeclarationKind::ANNOTATION;
    }

    return var->Declaration()->IsFunctionDecl() ? ImportConflictDeclarationKind::FUNCTION
                                                : ImportConflictDeclarationKind::VARIABLE;
}

static const char *DeclarationKindName(ImportConflictDeclarationKind kind)
{
    switch (kind) {
        case ImportConflictDeclarationKind::FUNCTION:
            return "Function";
        case ImportConflictDeclarationKind::CLASS:
            return "Class";
        case ImportConflictDeclarationKind::NAMESPACE:
            return "Namespace";
        case ImportConflictDeclarationKind::INTERFACE:
            return "Interface";
        case ImportConflictDeclarationKind::ANNOTATION:
            return "Annotation";
        case ImportConflictDeclarationKind::VARIABLE:
            return "Variable";
        default:
            ES2PANDA_UNREACHABLE();
    }
}

static bool IsNamespaceImportBinding(varbinder::Variable *var)
{
    if (!IsImportBindingVariable(var)) {
        return false;
    }

    auto *bindingInfo = var->AsLocalVariable()->ImportBinding();
    return bindingInfo != nullptr && bindingInfo->kind == varbinder::ImportBindingKind::NAMESPACE;
}

static ir::Identifier *ImportSpecifierLocalIdentifier(ir::AstNode *spec)
{
    if (spec->IsImportSpecifier()) {
        return spec->AsImportSpecifier()->Local();
    }
    if (spec->IsImportDefaultSpecifier()) {
        return spec->AsImportDefaultSpecifier()->Local();
    }
    if (spec->IsImportNamespaceSpecifier()) {
        return spec->AsImportNamespaceSpecifier()->Local();
    }
    return nullptr;
}

static ir::Identifier *ImportSpecifierImportedIdentifier(ir::AstNode *spec)
{
    if (spec->IsImportSpecifier()) {
        return spec->AsImportSpecifier()->Imported();
    }
    if (spec->IsImportDefaultSpecifier()) {
        return spec->AsImportDefaultSpecifier()->Local();
    }
    return nullptr;
}

static void BindImportSpecifierDeclaration(ir::Identifier *ident, varbinder::Variable *target)
{
    if (ident != nullptr && target != nullptr) {
        ident->SetVariable(target);
    }
}

static void BindImportSpecifierDeclarations(ETSChecker *checker, ir::AstNode *spec, varbinder::LocalVariable *importVar,
                                            const ResolvedImportResult &result)
{
    if (result.status != ImportResolutionStatus::RESOLVED_VARIABLE ||
        ShouldSkipValueImportMaterialization(checker, importVar, result)) {
        return;
    }

    auto *target = checker->ResolveEffectiveVariable(result.entry.variable);
    target = target != nullptr ? target : result.entry.variable;
    auto *local = ImportSpecifierLocalIdentifier(spec);
    BindImportSpecifierDeclaration(local, target);

    auto *imported = ImportSpecifierImportedIdentifier(spec);
    if (imported != local) {
        BindImportSpecifierDeclaration(imported, target);
    }
}

static void MaterializeReExportSpecifier(ETSChecker *checker, const checker::ResolvedImportBindingResult &resolved)
{
    auto *origin = const_cast<ir::AstNode *>(resolved.reportOrigin);
    if (origin == nullptr || !origin->IsImportSpecifier() || resolved.entry.variable == nullptr) {
        return;
    }

    // Re-export import declarations are not checked as ordinary imports, but import-demand resolution can still
    // materialize their specifier identifiers for shared users such as symbol indexing.
    auto *importVar = util::Helpers::ImportSpecifierLocalVariable(origin);
    if (!IsImportBindingVariable(importVar)) {
        return;
    }

    BindImportSpecifierDeclarations(checker, origin, importVar->AsLocalVariable(),
                                    {ImportResolutionStatus::RESOLVED_VARIABLE, resolved.entry, {}});
}

static varbinder::Variable *FindImportConflictVariable(ETSChecker *checker, util::StringView localName,
                                                       varbinder::Variable *importVar, varbinder::Variable *target)
{
    auto isConflict = [importVar, target](varbinder::Variable *var) {
        return var != nullptr && var != importVar && var != target;
    };

    auto *topScope = checker->VarBinder() != nullptr ? checker->VarBinder()->TopScope() : nullptr;
    auto *current =
        topScope != nullptr ? topScope->FindLocal(localName, varbinder::ResolveBindingOptions::ALL) : nullptr;
    if (isConflict(current)) {
        return current;
    }

    auto *program = checker->VarBinder() != nullptr ? checker->VarBinder()->Program() : nullptr;
    auto *globalClassScope = program != nullptr ? program->GlobalClassScope() : nullptr;
    current = globalClassScope != nullptr
                  ? globalClassScope->FindLocal(localName, varbinder::ResolveBindingOptions::ALL)
                  : nullptr;
    return isConflict(current) ? current : nullptr;
}

static varbinder::Variable *ImportConflictVariable(ETSChecker *checker, util::StringView localName,
                                                   varbinder::Variable *importVar, varbinder::Variable *target)
{
    if (importVar != nullptr && importVar->IsLocalVariable()) {
        auto *bindingInfo = importVar->AsLocalVariable()->ImportBinding();
        if (bindingInfo != nullptr && bindingInfo->conflictingLocalVariable != nullptr) {
            return bindingInfo->conflictingLocalVariable;
        }
    }

    return FindImportConflictVariable(checker, localName, importVar, target);
}

static void LogImportConflict(ETSChecker *checker, varbinder::Variable *target, varbinder::Variable *current,
                              util::StringView localName, const lexer::SourcePosition &pos)
{
    auto targetKind = DeclarationKind(target);
    auto currentKind = DeclarationKind(current);
    if (targetKind == ImportConflictDeclarationKind::FUNCTION &&
        currentKind == ImportConflictDeclarationKind::FUNCTION) {
        checker->LogError(diagnostic::OVERLOADED_FUNCS_MUST_BE_IN_SAME_SCOPE, {}, pos);
        return;
    }

    auto *targetKindName = DeclarationKindName(targetKind);
    if (targetKind == ImportConflictDeclarationKind::NAMESPACE || targetKind == currentKind) {
        checker->LogError(diagnostic::REDEFINITION, {targetKindName, localName}, pos);
        return;
    }

    checker->LogError(diagnostic::REDEFINITION_DIFF_TYPE, {targetKindName, localName}, pos);
}

static void ValidateResolvedImportSpecifier(ETSChecker *checker, ir::ImportDeclaration *st, ir::AstNode *spec,
                                            const ResolvedImportResult &result)
{
    if (result.status != ImportResolutionStatus::RESOLVED_VARIABLE || result.entry.variable == nullptr) {
        return;
    }

    auto *local = ImportSpecifierLocalIdentifier(spec);
    if (local == nullptr) {
        return;
    }

    auto *target = checker->ResolveEffectiveVariable(result.entry.variable);
    if (target == nullptr) {
        target = result.entry.variable;
    }

    if (IsAnnotationVariable(target)) {
        if (st->IsTypeKind()) {
            checker->LogError(diagnostic::IMPORT_TYPE_NOT_ALLOWED, {}, spec->Start());
        }
        if (result.entry.isTypeOnlyUse) {
            checker->LogError(diagnostic::ANNOTATION_AS_TYPE, {}, spec->Start());
        }
        if (spec->IsImportSpecifier() && spec->AsImportSpecifier()->Imported()->Name() != local->Name()) {
            checker->LogError(diagnostic::IMPORT_RENAMES_ANNOTATION, {spec->AsImportSpecifier()->Imported()->Name()},
                              spec->Start());
        }
    }

    auto *importVar = util::Helpers::ImportSpecifierLocalVariable(spec);
    auto *current = ImportConflictVariable(checker, local->Name(), importVar, target);
    if (current == nullptr) {
        return;
    }

    if (IsNamespaceImportBinding(importVar) != IsNamespaceImportBinding(current)) {
        checker->LogError(diagnostic::REDEFINITION_DIFF_TYPE,
                          {DeclarationKindName(DeclarationKind(target)), local->Name()}, local->Start());
        return;
    }

    auto *currentTarget = checker->ResolveEffectiveVariable(current);
    if (currentTarget == nullptr) {
        currentTarget = current;
    }
    if (currentTarget == target) {
        return;
    }
    if (IsAnnotationVariable(target) && IsAnnotationVariable(currentTarget)) {
        return;
    }

    LogImportConflict(checker, target, currentTarget, local->Name(), local->Start());
}

void ETSChecker::ResolveAndMaterializeImportSpecifier(ir::ImportDeclaration *importDecl, ir::AstNode *specifier)
{
    auto *var = util::Helpers::ImportSpecifierLocalVariable(specifier);
    if (!IsImportBindingVariable(var)) {
        return;
    }

    if (IsDynamicStaticInteropImportTarget(this, var->AsLocalVariable())) {
        return;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto result = ResolveImportBinding(var->AsLocalVariable());
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    ValidateResolvedImportSpecifier(this, importDecl, specifier, result);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    BindImportSpecifierDeclarations(this, specifier, var->AsLocalVariable(), result);
}

static ResolvedImportResult InvalidImportResult()
{
    return {ImportResolutionStatus::INVALID, {}, {}};
}

static void PropagateResolvedImportVariable(ETSChecker *checker, varbinder::LocalVariable *localVar,
                                            varbinder::Variable *targetVar)
{
    auto *bindingInfo = localVar->ImportBinding();
    if (bindingInfo != nullptr) {
        bindingInfo->resolvedVariable = targetVar;
    }
    auto flagsToPropagate = static_cast<varbinder::VariableFlags>(
        targetVar->Flags() & (varbinder::VariableFlags::CLASS_OR_INTERFACE_OR_ENUM | varbinder::VariableFlags::METHOD |
                              varbinder::VariableFlags::NAMESPACE));
    localVar->AddFlag(flagsToPropagate);
    if (targetVar->IsLocalVariable() && targetVar->AsLocalVariable()->Declaration() != nullptr &&
        targetVar->AsLocalVariable()->Declaration()->Node() != nullptr &&
        targetVar->AsLocalVariable()->Declaration()->Node()->IsMethodDefinition()) {
        checker->BuildExportedFunctionSignature(targetVar);
    }
}

static ResolvedImportResult ResolvedImportVariable(ETSChecker *checker, varbinder::LocalVariable *localVar,
                                                   const checker::ResolvedImportBindingResult &resolved)
{
    if (resolved.entry.surface.program != nullptr) {
        return {ImportResolutionStatus::RESOLVED_SURFACE, resolved.entry, resolved.entry.surface};
    }
    if (resolved.entry.variable != nullptr) {
        PropagateResolvedImportVariable(checker, localVar, resolved.entry.variable);
        MaterializeReExportSpecifier(checker, resolved);
    }
    return {ImportResolutionStatus::RESOLVED_VARIABLE, resolved.entry, {}};
}

static void MarkUnresolvedImportBinding(ETSChecker *checker, varbinder::LocalVariable *localVar,
                                        const ImportBindingResolveOptions &options)
{
    if (options.reportDiagnostics) {
        localVar->SetTsType(checker->GlobalTypeError());
    }
}

ResolvedImportResult ETSChecker::ResolveImportBinding(varbinder::LocalVariable *localVar,
                                                      ImportBindingResolveOptions options)
{
    if (localVar == nullptr || !localVar->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
        return InvalidImportResult();
    }

    if (localVar->TsType() != nullptr && localVar->TsType()->IsTypeError()) {
        return InvalidImportResult();
    }

    auto *bindingInfo = localVar->ImportBinding();
    if (bindingInfo == nullptr) {
        return InvalidImportResult();
    }
    if (bindingInfo->resolvedVariable != nullptr) {
        ResolvedImportResult result {};
        result.status = ImportResolutionStatus::RESOLVED_VARIABLE;
        result.entry.variable = bindingInfo->resolvedVariable;
        return result;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    const auto resolved = exportClosureResolver_->ResolveImportBinding(bindingInfo, options);
    switch (resolved.status) {
        case ImportBindingResolutionStatus::RESOLVED_SURFACE:
            return {ImportResolutionStatus::RESOLVED_SURFACE, resolved.entry, resolved.surface};
        case ImportBindingResolutionStatus::RESOLVED_VARIABLE:
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            return ResolvedImportVariable(this, localVar, resolved);
        case ImportBindingResolutionStatus::AMBIGUOUS:
            MarkUnresolvedImportBinding(this, localVar, options);
            return {ImportResolutionStatus::AMBIGUOUS, {}, {}};
        case ImportBindingResolutionStatus::NOT_FOUND:
            MarkUnresolvedImportBinding(this, localVar, options);
            return {ImportResolutionStatus::NOT_FOUND, {}, {}};
        case ImportBindingResolutionStatus::INVALID:
            MarkUnresolvedImportBinding(this, localVar, options);
            return {ImportResolutionStatus::INVALID, {}, {}};
        default:
            ES2PANDA_UNREACHABLE();
    }
}

varbinder::Variable *ETSChecker::ResolveEffectiveVariable(varbinder::Variable *var)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return exportClosureResolver_->ResolveEffectiveImportVariable(var);
}

static bool IsImportedLocalReferenceNode(ir::AstNodeType nodeType)
{
    return nodeType == ir::AstNodeType::TS_INTERFACE_DECLARATION || nodeType == ir::AstNodeType::CLASS_DECLARATION ||
           nodeType == ir::AstNodeType::STRUCT_DECLARATION || nodeType == ir::AstNodeType::CLASS_DEFINITION ||
           nodeType == ir::AstNodeType::TS_TYPE_PARAMETER || nodeType == ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION ||
           nodeType == ir::AstNodeType::ANNOTATION_DECLARATION;
}

static bool IsTypeReferenceTarget(varbinder::Variable *target)
{
    if (target == nullptr) {
        return false;
    }
    if (target->IsLocalVariable() && target->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
        return true;
    }

    auto *declNode = target->Declaration() == nullptr ? nullptr : target->Declaration()->Node();
    return declNode != nullptr && IsImportedLocalReferenceNode(declNode->Type());
}

static Type *ResolveResolvedImportBindingType(ETSChecker *checker, varbinder::LocalVariable *localVar,
                                              varbinder::Variable *target, ir::Identifier *useSite)
{
    if (target == nullptr) {
        localVar->SetTsType(checker->GlobalTypeError());
        return checker->GlobalTypeError();
    }

    if (target->IsLocalVariable() && target->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return checker->ResolveImportBindingType(target->AsLocalVariable(), useSite);
    }

    auto *declNode = target->Declaration() == nullptr ? nullptr : target->Declaration()->Node();
    if (declNode != nullptr) {
        if (declNode->IsClassDefinition()) {
            return checker->GetTypeFromClassReference(target);
        }
        if (declNode->IsTSInterfaceDeclaration()) {
            return checker->GetTypeFromInterfaceReference(target);
        }
        if (target->IsLocalVariable() && IsImportedLocalReferenceNode(declNode->Type())) {
            return checker->ResolveReferencedType(target->AsLocalVariable(), useSite);
        }
        if (target->IsLocalVariable() && declNode->IsMethodDefinition()) {
            checker->BuildExportedFunctionSignature(target);
        }
    }

    return checker->GetTypeOfVariable(target);
}

Type *ETSChecker::ResolveImportBindingType(varbinder::LocalVariable *localVar, ir::Identifier *useSite)
{
    if (localVar == nullptr || !localVar->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
        return GlobalTypeError();
    }

    if (localVar->TsType() != nullptr && localVar->TsType()->IsTypeError()) {
        return localVar->TsType();
    }

    if (resolvingImportBindings_.find(localVar) != resolvingImportBindings_.end()) {
        localVar->SetTsType(GlobalTypeError());
        return GlobalTypeError();
    }

    resolvingImportBindings_.insert(localVar);
    auto finish = [this, localVar](Type *type) {
        resolvingImportBindings_.erase(localVar);
        if (type != nullptr) {
            localVar->SetTsType(type);
        }
        return type;
    };
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto result = ResolveImportBinding(localVar, {false, true});
    switch (result.status) {
        case ImportResolutionStatus::RESOLVED_SURFACE:
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            return finish(GetImportSurfaceObjectType(result.surface, useSite, localVar));
        case ImportResolutionStatus::RESOLVED_VARIABLE: {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            return finish(ResolveResolvedImportBindingType(this, localVar, result.entry.variable, useSite));
        }
        case ImportResolutionStatus::AMBIGUOUS:
            [[fallthrough]];
        case ImportResolutionStatus::NOT_FOUND:
            [[fallthrough]];
        case ImportResolutionStatus::INVALID:
            localVar->SetTsType(GlobalTypeError());
            return finish(GlobalTypeError());
        default:
            ES2PANDA_UNREACHABLE();
    }
}

static void BindResolvedImportReference(ETSChecker *checker, varbinder::LocalVariable *refVar, ir::Identifier *ident,
                                        const ResolvedImportResult &result, bool allowDynamicInteropTarget)
{
    if (!allowDynamicInteropTarget && IsDynamicStaticInteropImportTarget(checker, refVar, result)) {
        return;
    }

    auto *target = checker->ResolveEffectiveVariable(refVar);
    if (target != nullptr) {
        ident->SetVariable(target);
    }
}

static Type *TryHandleImportedTypeAliasReference(ETSChecker *checker, ir::Identifier *ident)
{
    auto *target = checker->ResolveEffectiveVariable(ident->Variable());
    auto *declNode = target == nullptr || target->Declaration() == nullptr ? nullptr : target->Declaration()->Node();
    if (declNode == nullptr || !target->Declaration()->IsTypeAliasDecl()) {
        return nullptr;
    }
    if (target->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
        return nullptr;
    }

    auto *parent = ident->Parent();
    if (parent == nullptr || !parent->IsETSTypeReferencePart()) {
        return nullptr;
    }

    ident->SetVariable(target);
    if (parent->AsETSTypeReferencePart()->TypeParams() == nullptr) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return checker->GetTypeFromTypeAliasReference(target);
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return checker->HandleTypeAlias(ident, parent->AsETSTypeReferencePart()->TypeParams(),
                                    declNode->AsTSTypeAliasDeclaration());
}

Type *ETSChecker::ResolveImportReferencedType(varbinder::LocalVariable *refVar, const ir::Expression *name)
{
    auto *ident = const_cast<ir::Expression *>(name)->AsIdentifier();
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    const bool isDynamicStaticInteropTarget = IsDynamicStaticInteropImportTarget(this, refVar);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto result = ResolveImportBinding(refVar, {!isDynamicStaticInteropTarget, true});
    if (result.status == ImportResolutionStatus::RESOLVED_VARIABLE) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *target = ResolveEffectiveVariable(refVar);
        if (!IsTypeReferenceTarget(target)) {
            LogError(diagnostic::TYPE_NOT_FOUND, {ident->Name()}, ident->Start());
            return ident->SetTsType(GlobalTypeError());
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        BindResolvedImportReference(this, refVar, ident, result, true);
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        if (auto *type = TryHandleImportedTypeAliasReference(this, ident); type != nullptr) {
            return ident->SetTsType(type);
        }
    } else if (result.status == ImportResolutionStatus::NOT_FOUND || result.status == ImportResolutionStatus::INVALID) {
        LogError(diagnostic::TYPE_NOT_FOUND, {ident->Name()}, ident->Start());
        if (isDynamicStaticInteropTarget) {
            return ident->SetTsType(GlobalTypeError());
        }
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return ident->SetTsType(ResolveImportBindingType(refVar, ident));
}

static void MaterializeImportTypeReference(ETSChecker *checker, ir::Identifier *ident)
{
    if (ident == nullptr || ident->Variable() == nullptr || !ident->Variable()->IsLocalVariable() ||
        !ident->Variable()->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
        return;
    }

    auto *refVar = ident->Variable()->AsLocalVariable();
    auto result = checker->ResolveImportBinding(refVar, {false, true});
    if (result.status == ImportResolutionStatus::RESOLVED_VARIABLE) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        if (!IsTypeReferenceTarget(checker->ResolveEffectiveVariable(refVar))) {
            return;
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        BindResolvedImportReference(checker, refVar, ident, result, true);
    }
}

static void MaterializeNamespaceTypeReference(ETSChecker *checker, ir::Expression *name)
{
    if (name != nullptr && name->IsTSQualifiedName()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        name->Check(checker);
    }
}

static void MaterializeImportTypeReferencePart(ETSChecker *checker, ir::ETSTypeReferencePart *part)
{
    if (part == nullptr) {
        return;
    }

    if (part->Name()->IsIdentifier()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        MaterializeImportTypeReference(checker, part->GetIdent());
    } else {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        MaterializeNamespaceTypeReference(checker, part->Name());
    }

    if (part->TypeParams() != nullptr) {
        for (auto *typeArg : part->TypeParams()->Params()) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            checker->MaterializeImportTypeReferences(typeArg);
        }
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    MaterializeImportTypeReferencePart(checker, part->Previous());
}

void ETSChecker::MaterializeImportTypeReferences(ir::TypeNode *typeNode)
{
    if (typeNode == nullptr) {
        return;
    }

    if (typeNode->IsETSTypeReference()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        MaterializeImportTypeReferencePart(this, typeNode->AsETSTypeReference()->Part());
        return;
    }

    if (typeNode->IsETSTypeReferencePart()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        MaterializeImportTypeReferencePart(this, typeNode->AsETSTypeReferencePart());
        return;
    }

    if (typeNode->IsETSUnionType()) {
        for (auto *const constituent : typeNode->AsETSUnionType()->Types()) {
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            MaterializeImportTypeReferences(constituent);
        }
        return;
    }

    if (typeNode->IsETSFunctionType()) {
        for (auto *const param : typeNode->AsETSFunctionType()->Params()) {
            if (param->IsETSParameterExpression()) {
                // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                MaterializeImportTypeReferences(param->AsETSParameterExpression()->TypeAnnotation());
            }
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        MaterializeImportTypeReferences(typeNode->AsETSFunctionType()->ReturnType());
    }
}

Type *ETSChecker::GetTypeFromTypeAnnotation(ir::TypeNode *typeNode)
{
    if (typeNode == nullptr) {
        return GlobalTypeError();
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    MaterializeImportTypeReferences(typeNode);
    return typeNode->GetType(this);
}

}  // namespace ark::es2panda::checker
