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

#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

bool DeclGenPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    if (!ctx->config->options->IsEmitDeclaration()) {
        return true;
    }

    checker_ = ctx->GetChecker()->AsETSChecker();
    phaseManager_ = ctx->phaseManager;

    DumpDeclaration(program);
    CreateModuleDeclarationAnnotation(program);

    return true;
}

void DeclGenPhase::DumpDeclaration(parser::Program *program)
{
    declaration_ = program->Ast()->DumpDecl();
    declaration_.erase(0, declaration_.find_first_not_of('\n'));
    declaration_.erase(declaration_.find_last_not_of('\n'), declaration_.size() - 1);
}

void DeclGenPhase::CreateModuleDeclarationAnnotation(parser::Program *program)
{
    auto *const annoUsageIdent = checker_->AllocNode<ir::Identifier>(MODULE_DECLARATION_NAME, checker_->Allocator());
    annoUsageIdent->SetAnnotationUsage();

    auto flags = ir::ModifierFlags::ANNOTATION_USAGE;
    ArenaVector<ir::AstNode *> properties(checker_->Allocator()->Adapter());
    auto *singleParamName =
        checker_->AllocNode<ir::Identifier>(compiler::Signatures::ANNOTATION_KEY_VALUE, checker_->Allocator());
    auto *declarationLiteral = checker_->AllocNode<ir::StringLiteral>(declaration_.c_str());
    auto *declarationProp = checker_->AllocNode<ir::ClassProperty>(singleParamName, declarationLiteral, nullptr, flags,
                                                                   checker_->Allocator(), false);
    properties.push_back(declarationProp);

    auto *annotationUsage = checker_->AllocNode<ir::AnnotationUsage>(annoUsageIdent, std::move(properties));
    annotationUsage->AddModifier(flags);
    annotationUsage->SetParent(program->GlobalClass());

    program->GlobalClass()->EmplaceAnnotation(annotationUsage);
    Recheck(phaseManager_, checker_->VarBinder()->AsETSBinder(), checker_, annotationUsage);
}

}  // namespace ark::es2panda::compiler
