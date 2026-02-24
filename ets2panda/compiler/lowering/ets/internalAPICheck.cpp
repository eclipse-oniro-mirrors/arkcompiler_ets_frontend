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
#include "util/internalAPIWhitelist.h"
#include "checker/ETSchecker.h"
#include "generated/diagnostic.h"
#include "ir/base/classDefinition.h"
#include "ir/expressions/identifier.h"
#include "ir/ets/etsImportDeclaration.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/statements/annotationDeclaration.h"
#include "ir/statements/annotationUsage.h"
#include "parser/program/program.h"
#include "varbinder/ETSBinder.h"
#include "util/ustring.h"

namespace ark::es2panda::compiler {

static bool HasSomeAnnotationSet(ir::AstNode const *declNode, std::vector<std::string_view> const &listedAnnotations)
{
    ArenaVector<ir::AnnotationUsage *> const *annotations =
        declNode->IsClassDefinition()          ? &declNode->AsClassDefinition()->Annotations()
        : declNode->IsTSInterfaceDeclaration() ? &declNode->AsTSInterfaceDeclaration()->Annotations()
        : declNode->IsAnnotationDeclaration()  ? &declNode->AsAnnotationDeclaration()->Annotations()
                                               : nullptr;
    if (annotations == nullptr) {
        return false;
    }
    for (auto *anno : *annotations) {
        if (anno->GetBaseName()->Variable() == nullptr) {
            continue;
        }
        auto node = anno->GetBaseName()->Variable()->Declaration()->Node();
        // This is the only reliable way to check if the annotation is exactly the same
        // especially because the checker::Type for annotations is not provided
        if (node->IsAnnotationDeclaration() &&
            std::any_of(listedAnnotations.begin(), listedAnnotations.end(),
                        [node](auto e) { return node->AsAnnotationDeclaration()->InternalName().Is(e); })) {
            return true;
        }
    }
    return false;
}

static void CheckTypeReference(checker::ETSChecker *checker, ir::AstNode const *node, checker::Type const *type,
                               std::vector<std::string_view> const &listedAnnotations)
{
    if (type == nullptr) {
        return;
    }
    if (node->Parent()->IsAnnotationUsage()) {
        // This might appear as a total nonsense, yet it is a single way to work with annotations themselves
        // The type for annotation is not produced, sometimes it is a TypeError,
        // and it is expected according to the implementation.
        auto var = node->Parent()->AsAnnotationUsage()->GetBaseName()->Variable();
        if (var == nullptr) {
            return;
        }
        auto declNode = var->Declaration()->Node();
        if (declNode != nullptr && HasSomeAnnotationSet(declNode, listedAnnotations)) {
            checker->LogError(diagnostic::ARKRUNTIME_INTERNAL_API_ACCESS, {var->Name()}, node->Start());
        }
    } else if (type->IsETSObjectType()) {
        auto declNode = type->AsETSObjectType()->GetDeclNode();
        if (declNode != nullptr && HasSomeAnnotationSet(declNode, listedAnnotations)) {
            checker->LogError(diagnostic::ARKRUNTIME_INTERNAL_API_ACCESS, {type}, node->Start());
        }
    }
}

static void EnforceChecks(public_lib::Context *ctx, parser::Program *program,
                          std::vector<std::string_view> const &listedAnnotations)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    program->Ast()->IterateRecursively([checker, &listedAnnotations](ir::AstNode *node) {
        if (node->IsIdentifier()) {
            auto *ident = node->AsIdentifier();
            if (ident->Variable() != nullptr) {
                CheckTypeReference(checker, node, ident->Variable()->TsType(), listedAnnotations);
            }
        } else if (node->IsETSTypeReference()) {
            auto *typeRef = node->AsETSTypeReference();
            CheckTypeReference(checker, node, typeRef->TsType(), listedAnnotations);
        }
    });
}

bool InternalAPICheck::PerformForProgram(parser::Program *program)
{
    auto restricted = util::ComputeRestrictedAPIAnnotationsAt(program->ModuleName());
    if (!restricted.empty()) {
        EnforceChecks(Context(), program, restricted);
    }

    return true;
}

}  // namespace ark::es2panda::compiler
