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

#include "get_definition_at_position.h"

#include <utility>

#include "checker/exportClosureResolver.h"
#include "compiler/lowering/util.h"
#include "internal_api.h"
#include "ir/astNode.h"
#include "ir/ets/etsReExportDeclaration.h"
#include "ir/module/importSpecifier.h"
#include "public/public.h"
#include "varbinder/ETSBinder.h"
#include "varbinder/declaration.h"
#include "varbinder/exportFacts.h"
#include "varbinder/variable.h"

namespace ark::es2panda::lsp {

static std::string GetDefinitionFilePathFromImportNamespaceAlias(const ir::AstNode *declNode)
{
    if (declNode == nullptr || !declNode->IsImportNamespaceSpecifier()) {
        return {};
    }

    const auto *parent = declNode->Parent();
    if (parent == nullptr) {
        return {};
    }

    const ir::ETSImportDeclaration *importDecl = nullptr;
    if (parent->IsETSImportDeclaration()) {
        importDecl = parent->AsETSImportDeclaration();
    } else if (parent->IsETSReExportDeclaration()) {
        importDecl = parent->AsETSReExportDeclaration()->GetETSImportDeclarations();
    }
    if (importDecl == nullptr) {
        return {};
    }

    return GetImportFilePath(importDecl->ImportInfo());
}

static std::string GetDefinitionFilePathFromImportSurface(es2panda_Context *context, size_t position,
                                                          checker::ExportClosureResolver *resolver)
{
    auto *node = GetTouchingTokenRightMatch(context, position);
    if (node == nullptr || !node->IsIdentifier()) {
        return {};
    }
    auto *identifier = node->AsIdentifier();
    auto *parent = identifier->Parent();
    auto *variable = parent != nullptr && parent->IsImportSpecifier() ? parent->AsImportSpecifier()->Local()->Variable()
                                                                      : identifier->Variable();
    if (variable == nullptr || !variable->IsLocalVariable()) {
        return {};
    }
    auto *bindingInfo = variable->AsLocalVariable()->ImportBinding();
    if (bindingInfo == nullptr) {
        return {};
    }
    auto resolved = resolver->ResolveImportBinding(bindingInfo, {false, false});
    if (resolved.status != checker::ImportBindingResolutionStatus::RESOLVED_SURFACE ||
        resolved.surface.program == nullptr) {
        return {};
    }
    return std::string(resolved.surface.program->SourceFilePath());
}

static ir::AstNode *FindDefinitionIdentifier(ir::AstNode *declaration, util::StringView referencedName,
                                             const varbinder::Variable *resolvedVariable)
{
    if (resolvedVariable != nullptr) {
        auto *target = declaration->FindChild([resolvedVariable](ir::AstNode *childNode) {
            return childNode->IsIdentifier() && childNode->AsIdentifier()->Variable() == resolvedVariable;
        });
        if (target != nullptr) {
            return target;
        }
    }

    auto *target = declaration->FindChild([referencedName](ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == referencedName;
    });
    if (target != nullptr) {
        return target;
    }

    auto declarationName = compiler::GetNameOfDeclaration(declaration);
    if (!declarationName.has_value()) {
        return nullptr;
    }
    return declaration->FindChild([&declarationName](ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name().Utf8() == declarationName.value();
    });
}

static const varbinder::ImportBindingInfo *GetClickedNamedImportBinding(es2panda_Context *context, size_t position)
{
    auto *node = GetTouchingTokenRightMatch(context, position);
    if (node == nullptr || !node->IsIdentifier() || node->Parent() == nullptr || !node->Parent()->IsImportSpecifier()) {
        return nullptr;
    }

    auto *specifier = node->Parent()->AsImportSpecifier();
    if (node != specifier->Imported() && node != specifier->Local()) {
        return nullptr;
    }
    auto *importDecl = specifier->Parent();
    if (importDecl == nullptr || !importDecl->IsETSImportDeclaration()) {
        return nullptr;
    }

    auto *variable = specifier->Local()->Variable();
    if (variable == nullptr || !variable->IsLocalVariable() ||
        !variable->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
        return nullptr;
    }
    auto *bindingInfo = variable->AsLocalVariable()->ImportBinding();
    if (bindingInfo == nullptr || bindingInfo->kind != varbinder::ImportBindingKind::NAMED ||
        bindingInfo->origin != specifier || bindingInfo->importDecl != importDecl->AsETSImportDeclaration() ||
        bindingInfo->importedName != specifier->Imported()->Name() ||
        bindingInfo->localName != specifier->Local()->Name()) {
        return nullptr;
    }
    return bindingInfo;
}

static ir::Identifier *GetInvalidTypeOnlyExportTarget(const varbinder::ExportFact &fact, parser::Program *targetProgram,
                                                      util::StringView importedName)
{
    if (!fact.isInvalid || !fact.isExplicitTypeOnly || !fact.isLocalAlias || fact.sourceProgram != targetProgram ||
        fact.exportedName != importedName || fact.variable == nullptr || fact.variable->Declaration() == nullptr) {
        return nullptr;
    }

    auto *node = fact.variable->Declaration()->Node();
    if (node == nullptr || node->Program() != targetProgram) {
        return nullptr;
    }
    auto *identifier = node->IsIdentifier() ? node : node->FindChild([&fact](ir::AstNode *child) {
        return child->IsIdentifier() && child->AsIdentifier()->Name() == fact.localName &&
               child->AsIdentifier()->Variable() == fact.variable;
    });
    if (identifier == nullptr || !identifier->IsIdentifier() || identifier->AsIdentifier()->Name() != fact.localName ||
        identifier->AsIdentifier()->Variable() != fact.variable) {
        return nullptr;
    }
    return identifier->AsIdentifier();
}

static ir::Identifier *FindInvalidTypeOnlyImportDefinition(es2panda_Context *context, size_t position,
                                                           checker::ExportClosureResolver *resolver)
{
    auto *bindingInfo = GetClickedNamedImportBinding(context, position);
    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    auto *program = ctx->parserProgram;
    auto *varBinder = program == nullptr ? nullptr : program->VarBinder();
    if (bindingInfo == nullptr || varBinder == nullptr || !varBinder->IsETSBinder()) {
        return nullptr;
    }

    auto resolved = resolver->ResolveImportBinding(bindingInfo, {false, false});
    if (resolved.status != checker::ImportBindingResolutionStatus::NOT_FOUND) {
        return nullptr;
    }

    const auto &store = varBinder->AsETSBinder()->GetExportFactsStore();
    const auto *surface = store.FindImportTarget(program, bindingInfo->importDecl);
    if (surface == nullptr || surface->kind != varbinder::ExportSurfaceKind::Program || surface->program == nullptr) {
        return nullptr;
    }

    ir::Identifier *target = nullptr;
    for (const auto &fact : store.GetExportFacts(surface->program).locals) {
        auto *candidate = GetInvalidTypeOnlyExportTarget(fact, surface->program, bindingInfo->importedName);
        if (candidate == nullptr) {
            continue;
        }
        if (target != nullptr && target != candidate) {
            return nullptr;
        }
        target = candidate;
    }
    return target;
}

static std::string GetDefinitionFileName(ir::AstNode *node)
{
    while (node != nullptr) {
        if (node->Range().start.Program() != nullptr) {
            return std::string(node->Range().start.Program()->SourceFile().GetAbsolutePath().Utf8());
        }
        if (node->IsETSModule()) {
            return std::string(node->AsETSModule()->Program()->SourceFilePath());
        }
        node = node->Parent();
    }
    return {};
}

DefinitionTarget GetDefinitionTargetAtPosition(es2panda_Context *context, size_t position)
{
    auto importFilePath = GetImportFilePath(context, position);
    if (!importFilePath.empty()) {
        return {std::move(importFilePath), {}, 0, 0};
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    auto *resolver = ctx->GetChecker()->AsETSChecker()->GetNavigationExportClosureResolver();
    auto importSurfaceFilePath = GetDefinitionFilePathFromImportSurface(context, position, resolver);
    if (!importSurfaceFilePath.empty()) {
        return {std::move(importSurfaceFilePath), {}, 0, 0};
    }

    varbinder::Variable *resolvedVariable = nullptr;
    auto declInfo = GetDefinitionAtPositionImpl(context, position, resolver, &resolvedVariable);
    ir::AstNode *targetNode = nullptr;
    if (declInfo.first != nullptr) {
        auto importNamespaceAliasFilePath = GetDefinitionFilePathFromImportNamespaceAlias(declInfo.first);
        if (!importNamespaceAliasFilePath.empty()) {
            return {std::move(importNamespaceAliasFilePath), {}, 0, 0};
        }
        targetNode = FindDefinitionIdentifier(declInfo.first, declInfo.second, resolvedVariable);
    }
    if (targetNode == nullptr) {
        targetNode = FindInvalidTypeOnlyImportDefinition(context, position, resolver);
    }
    if (targetNode == nullptr) {
        return {};
    }

    auto fileName = GetDefinitionFileName(targetNode);
    std::string targetSource = targetNode->Range().start.Program() != nullptr
                                   ? std::string(targetNode->Range().start.Program()->SourceCode())
                                   : std::string(ctx->parserProgram->SourceCode());
    return {std::move(fileName), std::move(targetSource), targetNode->Start().index,
            targetNode->End().index - targetNode->Start().index};
}

}  // namespace ark::es2panda::lsp
