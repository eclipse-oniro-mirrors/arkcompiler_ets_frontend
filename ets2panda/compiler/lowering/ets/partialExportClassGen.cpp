/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "partialExportClassGen.h"

#include "checker/ETSchecker.h"
#include "checker/types/ets/etsObjectType.h"

namespace ark::es2panda::compiler {

static void GeneratePartialDeclForExported(const public_lib::Context *const ctx, ir::AstNode *const node)
{
    checker::ETSObjectType *type = nullptr;
    if (node->IsClassDeclaration()) {
        type = node->AsClassDeclaration()->Definition()->TsType()->AsETSObjectType();
    } else if (node->IsTSInterfaceDeclaration()) {
        type = node->AsTSInterfaceDeclaration()->TsType()->AsETSObjectType();
    } else {
        ES2PANDA_UNREACHABLE();
    }

    if (!type->IsPartial()) {
        ctx->GetChecker()->AsETSChecker()->CreatePartialType(type);
    }
}

static bool IsExportedPartialCandidate(const ir::AstNode *const ast)
{
    return ((ast->IsClassDeclaration() && !ast->AsClassDeclaration()->Definition()->IsModule()) ||
            ast->IsTSInterfaceDeclaration()) &&
           (ast->IsExported() || ast->IsDefaultExported());
}

static void CollectExportedPartialCandidates(const ArenaVector<ir::Statement *> &statements,
                                             std::vector<ir::AstNode *> *const candidates)
{
    for (auto *const node : statements) {
        if (node->IsETSModule() && node->AsETSModule()->IsNamespace()) {
            CollectExportedPartialCandidates(node->AsETSModule()->Statements(), candidates);
        } else if (IsExportedPartialCandidate(node)) {
            candidates->emplace_back(node);
        }
    }
}

static void CreatePartialDecls(public_lib::Context *ctx, parser::Program *program)
{
    std::vector<ir::AstNode *> exportedPartialCandidates {};
    CollectExportedPartialCandidates(program->Ast()->Statements(), &exportedPartialCandidates);

    auto *const savedProg = ctx->GetChecker()->VarBinder()->AsETSBinder()->Program();
    ctx->GetChecker()->VarBinder()->AsETSBinder()->SetProgram(program);
    for (auto *const ast : exportedPartialCandidates) {
        GeneratePartialDeclForExported(ctx, ast);
    }
    ctx->GetChecker()->VarBinder()->AsETSBinder()->SetProgram(savedProg);
}

bool PartialExportClassGen::PerformForProgram(parser::Program *program)
{
    CreatePartialDecls(Context(), program);
    return true;
}

}  // namespace ark::es2panda::compiler
