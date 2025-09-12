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

#include "declGenPhase.h"

#include <fstream>
#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"
#include "ir/ets/etsPackageDeclaration.h"

namespace ark::es2panda::compiler {

constexpr std::string_view MODULE_DECLARATION_NAME {"ModuleDeclaration"};

bool DeclGenPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    if (!ctx->config->options->IsEmitDeclaration()) {
        return true;
    }

    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *phaseManager = ctx->phaseManager;
    auto *allocator = ctx->Allocator();

    // Arena cause we want declaration be life until codegen happens
    auto *declaration = allocator->New<ArenaString>(program->Ast()->DumpDecl(ctx), allocator->Adapter());
    ES2PANDA_ASSERT(declaration != nullptr);

    auto *const annoUsageIdent = checker->AllocNode<ir::Identifier>(MODULE_DECLARATION_NAME, checker->Allocator());
    annoUsageIdent->SetAnnotationUsage();

    auto flags = ir::ModifierFlags::ANNOTATION_USAGE;
    ArenaVector<ir::AstNode *> properties(checker->Allocator()->Adapter());
    auto *singleParamName =
        checker->AllocNode<ir::Identifier>(compiler::Signatures::ANNOTATION_KEY_VALUE, checker->Allocator());
    auto *declarationLiteral = checker->AllocNode<ir::StringLiteral>(declaration->c_str());
    auto *declarationProp = checker->AllocNode<ir::ClassProperty>(singleParamName, declarationLiteral, nullptr, flags,
                                                                  checker->Allocator(), false);
    properties.push_back(declarationProp);

    auto *annotationUsage = checker->AllocNode<ir::AnnotationUsage>(annoUsageIdent, std::move(properties));
    annotationUsage->AddModifier(flags);
    annotationUsage->SetParent(program->GlobalClass());

    program->GlobalClass()->EmplaceAnnotation(annotationUsage);
    Recheck(phaseManager, checker->VarBinder()->AsETSBinder(), checker, annotationUsage);

    return true;
}

}  // namespace ark::es2panda::compiler
