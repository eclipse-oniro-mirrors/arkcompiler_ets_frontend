/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "internalAPICheck.h"
#include "checker/types/signature.h"
#include "checker/types/ets/etsObjectType.h"
#include "ir/base/classDefinition.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/memberExpression.h"
#include "ir/ets/etsTypeReference.h"
#include "ir/ets/etsNewClassInstanceExpression.h"
#include "ir/statements/annotationDeclaration.h"
#include "ir/statements/annotationUsage.h"
#include "ir/statements/functionDeclaration.h"
#include "ir/statements/variableDeclaration.h"
#include "ir/statements/variableDeclarator.h"
#include "ir/ts/tsInterfaceDeclaration.h"

namespace ark::es2panda::compiler {

constexpr std::string_view ACCESS_RESTRICTION_ANNOTATION = "arkruntime.annotation.AccessRestriction";
constexpr std::string_view ACCESS_RESTRICTION_MODULES = "modules";

struct RestrictionInfo {
    std::string annotationName;
    std::vector<std::string> modules;
};

using RestrictionCache = std::unordered_map<ir::AnnotationDeclaration const *, std::optional<RestrictionInfo>>;

static bool NamespaceIsPrefixedWith(std::string_view internalName, std::string_view prefix)
{
    return internalName.rfind(prefix, 0) == 0 &&
           (internalName.length() == prefix.length() || internalName[prefix.length()] == '.');
}

static const ArenaVector<ir::AnnotationUsage *> *GetAnnotations(ir::AstNode const *declNode)
{
    if (declNode->IsClassDefinition()) {
        return &declNode->AsClassDefinition()->Annotations();
    }
    if (declNode->IsTSInterfaceDeclaration()) {
        return &declNode->AsTSInterfaceDeclaration()->Annotations();
    }
    if (declNode->IsAnnotationDeclaration()) {
        return &declNode->AsAnnotationDeclaration()->Annotations();
    }
    if (declNode->IsFunctionDeclaration()) {
        return &declNode->AsFunctionDeclaration()->Annotations();
    }
    if (declNode->IsVariableDeclaration()) {
        return &declNode->AsVariableDeclaration()->Annotations();
    }
    if (declNode->IsClassProperty()) {
        return &declNode->AsClassProperty()->Annotations();
    }
    if (declNode->IsScriptFunction()) {
        return &declNode->AsScriptFunction()->Annotations();
    }
    return nullptr;
}

static const ir::AnnotationDeclaration *ResolveAnnotationDeclaration(ir::AnnotationUsage const *anno)
{
    auto *baseName = anno->GetBaseName();
    if (baseName == nullptr) {
        return nullptr;
    }

    auto *baseVar = baseName->Variable();
    if (baseVar == nullptr) {
        auto *qualifiedName = baseName->Parent() != nullptr && baseName->Parent()->IsTSQualifiedName()
                                  ? baseName->Parent()->AsTSQualifiedName()
                                  : nullptr;
        if (qualifiedName == nullptr) {
            return nullptr;
        }

        auto *left = qualifiedName->Left();
        while (left != nullptr && left->IsTSQualifiedName()) {
            left = left->AsTSQualifiedName()->Left();
        }
        if (left == nullptr || !left->IsIdentifier()) {
            return nullptr;
        }

        auto *leftVar = left->AsIdentifier()->Variable();
        if (leftVar == nullptr || leftVar->TsType() == nullptr || !leftVar->TsType()->IsETSObjectType()) {
            return nullptr;
        }

        baseVar = leftVar->TsType()->AsETSObjectType()->GetProperty(baseName->Name(),
                                                                    checker::PropertySearchFlags::SEARCH_DECL);
        if (baseVar == nullptr) {
            return nullptr;
        }
    }

    auto *declNode = baseVar->Declaration() != nullptr ? baseVar->Declaration()->Node() : nullptr;
    if (declNode == nullptr || !declNode->IsAnnotationDeclaration()) {
        return nullptr;
    }

    return declNode->AsAnnotationDeclaration();
}

static bool IsAccessRestrictionAnnotation(ir::AnnotationDeclaration const *annoDecl)
{
    return annoDecl != nullptr && annoDecl->InternalName().Is(ACCESS_RESTRICTION_ANNOTATION);
}

static void CollectModulesFromValue(ir::Expression const *value, std::vector<std::string> &modules)
{
    if (value == nullptr || !value->IsArrayExpression()) {
        return;
    }

    for (auto *element : value->AsArrayExpression()->Elements()) {
        if (element != nullptr && element->IsStringLiteral()) {
            modules.emplace_back(element->AsStringLiteral()->Str().Mutf8());
        }
    }
}

static std::optional<RestrictionInfo> ParseRestrictionInfo(ir::AnnotationDeclaration const *declNode)
{
    auto const *annotations = GetAnnotations(declNode);
    if (annotations == nullptr) {
        return std::nullopt;
    }

    for (auto *anno : *annotations) {
        auto *restrictionDecl = ResolveAnnotationDeclaration(anno);
        if (!IsAccessRestrictionAnnotation(restrictionDecl)) {
            continue;
        }

        RestrictionInfo info {declNode->GetBaseName()->Name().Mutf8(), {}};
        for (auto *propNode : anno->Properties()) {
            auto *prop = propNode->AsClassProperty();
            if (prop == nullptr || prop->Id() == nullptr) {
                continue;
            }

            auto const propName = prop->Id()->Name();
            if (!propName.Is(ACCESS_RESTRICTION_MODULES) && propName != compiler::Signatures::ANNOTATION_KEY_VALUE) {
                continue;
            }

            CollectModulesFromValue(prop->Value(), info.modules);
        }

        return info.modules.empty() ? std::nullopt : std::optional<RestrictionInfo> {std::move(info)};
    }

    return std::nullopt;
}

static RestrictionInfo const *GetRestrictionInfo(ir::AnnotationDeclaration const *annoDecl, RestrictionCache &cache)
{
    auto const [it, inserted] = cache.emplace(annoDecl, std::nullopt);
    if (inserted) {
        it->second = ParseRestrictionInfo(annoDecl);
    }
    return it->second.has_value() ? &it->second.value() : nullptr;
}

static RestrictionInfo const *GetRestrictionInfo(ir::AstNode const *declNode, RestrictionCache &cache)
{
    auto const *annotations = GetAnnotations(declNode);
    if (annotations == nullptr) {
        return nullptr;
    }

    for (auto *anno : *annotations) {
        auto *annoDecl = ResolveAnnotationDeclaration(anno);
        if (annoDecl == nullptr) {
            continue;
        }

        auto *info = GetRestrictionInfo(annoDecl, cache);
        if (info != nullptr) {
            return info;
        }
    }

    return nullptr;
}

static bool IsAccessibleFromModule(RestrictionInfo const &info, std::string_view moduleName)
{
    return std::any_of(info.modules.begin(), info.modules.end(),
                       [moduleName](std::string const &prefix) { return NamespaceIsPrefixedWith(moduleName, prefix); });
}

static RestrictionInfo const *GetAppliedRestriction(ir::AstNode const *declNode, ir::AstNode const *useSite,
                                                    std::string_view moduleName, RestrictionCache &cache)
{
    auto *info = GetRestrictionInfo(declNode, cache);
    if (info == nullptr) {
        return nullptr;
    }

    auto *declProgram = declNode->Program();
    auto *useProgram = useSite != nullptr ? useSite->Program() : nullptr;
    if (declProgram != nullptr && useProgram != nullptr && declProgram == useProgram &&
        declNode->Start().Program() == useSite->Start().Program()) {
        return nullptr;
    }

    return IsAccessibleFromModule(*info, moduleName) ? nullptr : info;
}

static lexer::SourcePosition GetReportPosition(ir::AstNode const *useSite)
{
    if (useSite->IsETSNewClassInstanceExpression()) {
        auto *typeRef = useSite->AsETSNewClassInstanceExpression()->GetTypeRef();
        if (typeRef != nullptr) {
            return typeRef->Start();
        }
    }
    return useSite->Start();
}

static std::string GetRestrictedEntityName(ir::AstNode const *declNode)
{
    if (declNode->IsAnnotationDeclaration()) {
        return declNode->AsAnnotationDeclaration()->GetBaseName()->Name().Mutf8();
    }
    if (declNode->IsClassDefinition()) {
        return declNode->AsClassDefinition()->Ident()->Name().Mutf8();
    }
    if (declNode->IsTSInterfaceDeclaration()) {
        return declNode->AsTSInterfaceDeclaration()->Id()->Name().Mutf8();
    }
    if (declNode->IsClassProperty() && declNode->AsClassProperty()->Id() != nullptr) {
        return declNode->AsClassProperty()->Id()->Name().Mutf8();
    }
    if (declNode->IsVariableDeclaration()) {
        auto const &declarators = declNode->AsVariableDeclaration()->Declarators();
        if (!declarators.empty() && declarators[0]->Id() != nullptr && declarators[0]->Id()->IsIdentifier()) {
            return declarators[0]->Id()->AsIdentifier()->Name().Mutf8();
        }
    }
    if (declNode->IsScriptFunction()) {
        auto *scriptFunc = declNode->AsScriptFunction();
        if (scriptFunc->IsConstructor()) {
            return std::string {compiler::Signatures::CONSTRUCTOR_NAME};
        }
        if (scriptFunc->Id() != nullptr) {
            return scriptFunc->Id()->Name().Mutf8();
        }
        auto *method = scriptFunc->Parent() != nullptr && scriptFunc->Parent()->IsMethodDefinition()
                           ? scriptFunc->Parent()->AsMethodDefinition()
                           : nullptr;
        if (method != nullptr && method->Id() != nullptr && method->Id()->IsIdentifier()) {
            return method->Id()->AsIdentifier()->Name().Mutf8();
        }
    }
    return std::string {};
}

static void LogRestrictedUse(checker::ETSChecker *checker, ir::AstNode const *useSite, ir::AstNode const *declNode,
                             RestrictionInfo const &info)
{
    checker->LogError(diagnostic::ARKRUNTIME_INTERNAL_API_ACCESS,
                      {GetRestrictedEntityName(declNode), info.annotationName}, GetReportPosition(useSite));
}

static bool IsDeclarationNameReference(ir::AstNode const *node)
{
    auto *parent = node->Parent();
    if (parent == nullptr) {
        return false;
    }

    if (parent->IsClassDefinition()) {
        return parent->AsClassDefinition()->Ident() == node;
    }
    if (parent->IsAnnotationDeclaration()) {
        return parent->AsAnnotationDeclaration()->GetBaseName() == node;
    }
    if (parent->IsFunctionDeclaration()) {
        return parent->AsFunctionDeclaration()->Function() != nullptr &&
               parent->AsFunctionDeclaration()->Function()->Id() == node;
    }
    if (parent->IsClassProperty()) {
        return parent->AsClassProperty()->Id() == node;
    }
    if (parent->IsVariableDeclarator()) {
        return parent->AsVariableDeclarator()->Id() == node;
    }
    if (parent->IsMethodDefinition()) {
        return parent->AsMethodDefinition()->Id() == node;
    }
    if (parent->IsTSInterfaceDeclaration()) {
        return parent->AsTSInterfaceDeclaration()->Id() == node;
    }
    if (parent->IsScriptFunction()) {
        return parent->AsScriptFunction()->Id() == node;
    }
    return false;
}

static void CheckTypeReference(checker::ETSChecker *checker, std::string_view moduleName, ir::AstNode const *node,
                               checker::Type const *type, RestrictionCache &cache)
{
    if (type == nullptr || IsDeclarationNameReference(node)) {
        return;
    }

    if (node->Parent()->IsAnnotationUsage()) {
        auto *declNode = ResolveAnnotationDeclaration(node->Parent()->AsAnnotationUsage());
        auto *info = declNode != nullptr ? GetAppliedRestriction(declNode, node, moduleName, cache) : nullptr;
        if (info != nullptr) {
            LogRestrictedUse(checker, node, declNode, *info);
        }
        return;
    }

    if (type->IsETSObjectType()) {
        auto *declNode = type->AsETSObjectType()->GetDeclNode();
        auto *info = declNode != nullptr ? GetAppliedRestriction(declNode, node, moduleName, cache) : nullptr;
        if (info != nullptr) {
            LogRestrictedUse(checker, node, declNode, *info);
        }
    }
}

static void CheckResolvedVariable(checker::ETSChecker *checker, std::string_view moduleName, ir::AstNode const *useSite,
                                  varbinder::Variable const *var, RestrictionCache &cache)
{
    if (var == nullptr || var->Declaration() == nullptr || IsDeclarationNameReference(useSite)) {
        return;
    }

    auto *parent = useSite->Parent();
    if (parent != nullptr && parent->IsMemberExpression() && parent->AsMemberExpression()->Property() == useSite) {
        return;
    }
    if (parent != nullptr && parent->IsAnnotationUsage() && parent->AsAnnotationUsage()->GetBaseName() == useSite) {
        return;
    }

    auto *declNode = var->Declaration()->Node();
    auto *info = declNode != nullptr ? GetAppliedRestriction(declNode, useSite, moduleName, cache) : nullptr;
    if (info != nullptr) {
        LogRestrictedUse(checker, useSite, declNode, *info);
    }
}

static void CheckResolvedSignature(checker::ETSChecker *checker, std::string_view moduleName,
                                   ir::AstNode const *useSite, checker::Signature const *signature,
                                   RestrictionCache &cache)
{
    if (signature == nullptr || !signature->HasFunction()) {
        return;
    }

    auto *funcDecl = signature->Function();
    auto *info = GetAppliedRestriction(funcDecl, useSite, moduleName, cache);
    if (info != nullptr) {
        LogRestrictedUse(checker, useSite, funcDecl, *info);
    }
}

static void EnforceChecks(public_lib::Context *ctx, parser::Program *program)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto moduleName = std::string {program->ModuleName()};
    if (moduleName.empty()) {
        moduleName = program->RelativeFilePath(ctx);
    }

    RestrictionCache cache;
    program->Ast()->IterateRecursively([checker, moduleName, &cache](ir::AstNode *node) {
        if (node->IsIdentifier()) {
            auto *ident = node->AsIdentifier();
            if (ident->Variable() != nullptr) {
                CheckResolvedVariable(checker, moduleName, node, ident->Variable(), cache);
                CheckTypeReference(checker, moduleName, node, ident->Variable()->TsType(), cache);
            }
        } else if (node->IsETSTypeReference()) {
            CheckTypeReference(checker, moduleName, node, node->AsETSTypeReference()->TsType(), cache);
        } else if (node->IsMemberExpression()) {
            auto *memberExpr = node->AsMemberExpression();
            if (memberExpr->PropVar() != nullptr) {
                CheckResolvedVariable(checker, moduleName, node, memberExpr->PropVar(), cache);
            }
        } else if (node->IsCallExpression()) {
            CheckResolvedSignature(checker, moduleName, node, node->AsCallExpression()->Signature(), cache);
        } else if (node->IsETSNewClassInstanceExpression()) {
            CheckResolvedSignature(checker, moduleName, node, node->AsETSNewClassInstanceExpression()->Signature(),
                                   cache);
        }
    });
}

bool InternalAPICheck::PerformForProgram(parser::Program *program)
{
    EnforceChecks(Context(), program);
    return true;
}

}  // namespace ark::es2panda::compiler
