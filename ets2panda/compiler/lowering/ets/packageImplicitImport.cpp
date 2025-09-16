/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "packageImplicitImport.h"
#include <generated/diagnostic.h>

namespace ark::es2panda::compiler {

static void MergeExternalFilesIntoCompiledProgram(parser::Program *const program,
                                                  const ArenaVector<parser::Program *> &packagePrograms)
{
    for (auto *const extProg : packagePrograms) {
        const auto extClassDecls = extProg->Ast()->Statements();
        for (auto *const stmt : extClassDecls) {
            if (stmt->IsETSPackageDeclaration()) {
                continue;
            }

            stmt->SetParent(program->Ast());

            // Because same package files must be in one folder, relative path references in an external
            // source's import declaration certainly will be the same (and can be resolved) from the global program too
            program->Ast()->AddStatement(stmt);
        }
    }
}

static void ValidateFolderContainOnlySamePackageFiles(const public_lib::Context *const ctx,
                                                      const parser::Program *const program)
{
    const auto throwErrorIfPackagesConflict = [&ctx](const parser::Program *const prog1,
                                                     const parser::Program *const prog2) {
        if ((prog1 == prog2) || !prog1->IsPackage() || !prog2->IsPackage()) {
            return;
        }

        if (prog1->ModuleName() != prog2->ModuleName() &&
            prog1->SourceFile().GetAbsoluteParentFolder() == prog2->SourceFile().GetAbsoluteParentFolder()) {
            // There exist 2 files in the same folder, with different package names
            //
            // Showing the full path would be more informative, but it also leaks it to the stdout, which is
            // not the best idea
            ctx->parser->LogError(diagnostic::DIFFERENT_PACKAGE_NAME,
                                  {prog1->FileName().Mutf8(), prog2->FileName().Mutf8()},
                                  lexer::SourcePosition(0, 0, prog1));
        }
    };

    for (const auto &srcIter : program->ExternalSources()) {
        // in the external sources, all programs for a record in the map is in the same module,
        // it's enough to check the first of them
        const auto *const extSrc = std::get<1>(srcIter).front();
        throwErrorIfPackagesConflict(program, extSrc);

        for (const auto &srcIterCmp : program->ExternalSources()) {
            const auto *const extSrcCpm = std::get<1>(srcIterCmp).front();
            throwErrorIfPackagesConflict(extSrc, extSrcCpm);
        }
    }
}

static void ValidateImportDeclarationsSourcePath(const public_lib::Context *const ctx,
                                                 const ArenaVector<parser::Program *> &packagePrograms,
                                                 const std::vector<const ir::Statement *> &importDeclarations)
{
    for (const auto *const stmt : importDeclarations) {
        const bool doesImportFromPackage =
            std::any_of(packagePrograms.cbegin(), packagePrograms.cend(), [&stmt](const parser::Program *const prog) {
                return prog->SourceFilePath() == stmt->AsETSImportDeclaration()->ResolvedSource();
            });
        if (doesImportFromPackage) {
            ctx->parser->LogError(diagnostic::PACKAGE_MODULE_IMPORT_OWN_PACKAGE, {}, stmt->Start());
        }
    }
}

static void ValidateNoImportComesFromSamePackage(const public_lib::Context *const ctx, parser::Program *const program,
                                                 ArenaVector<parser::Program *> packagePrograms)
{
    // Making sure that the variable is not a reference. We modify the local vector here, which must not have any side
    // effects to the original one. Don't change it.
    static_assert(!std::is_reference_v<decltype(packagePrograms)>);
    packagePrograms.emplace_back(program);

    for (const auto *const packageProg : packagePrograms) {
        // Filter out only import declarations
        std::vector<const ir::Statement *> importDeclarations {};
        const auto &progStatements = packageProg->Ast()->Statements();
        std::copy_if(progStatements.begin(), progStatements.end(), std::back_inserter(importDeclarations),
                     [](const ir::Statement *const stmt) { return stmt->IsETSImportDeclaration(); });

        // Validate if all import declaration refers to a path outside of the package module
        ValidateImportDeclarationsSourcePath(ctx, packagePrograms, importDeclarations);
    }
}

bool PackageImplicitImport::Perform(public_lib::Context *const ctx, parser::Program *const program)
{
    if (!program->IsPackage() || program->VarBinder()->IsGenStdLib()) {
        // Only run for package module files
        return true;
    }

    ValidateFolderContainOnlySamePackageFiles(ctx, program);

    auto &externalSources = program->ExternalSources();
    if (externalSources.count(program->ModuleName()) == 0) {
        // No other files in the package, return
        return true;
    }

    auto &packagePrograms = externalSources.at(program->ModuleName());

    // NOTE (mmartin): Very basic sorting of files in the package, to merge them in a prescribed order
    std::stable_sort(packagePrograms.begin(), packagePrograms.end(),
                     [](const parser::Program *const prog1, const parser::Program *const prog2) {
                         return prog1->FileName() < prog2->FileName();
                     });

    MergeExternalFilesIntoCompiledProgram(program, packagePrograms);
    ValidateNoImportComesFromSamePackage(ctx, program, packagePrograms);

    // All entities were merged into the main program from the external sources of the same package,
    // so we can delete all of them
    externalSources.erase(program->ModuleName());

    return true;
}

}  // namespace ark::es2panda::compiler
