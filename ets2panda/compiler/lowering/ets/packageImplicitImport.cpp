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

#include "packageImplicitImport.h"
#include <generated/diagnostic.h>

namespace ark::es2panda::compiler {

static void MergeExternalFilesIntoCompiledProgram(parser::PackageProgram *const package)
{
    ES2PANDA_ASSERT(package->Ast() != nullptr);
    ES2PANDA_ASSERT(package->Ast()->Statements().empty());
    for (auto *const extProg : package->GetUnmergedPackagePrograms()) {
        for (auto *const stmt : extProg->Ast()->Statements()) {
            if (stmt->IsETSPackageDeclaration()) {
                continue;
            }

            stmt->SetParent(package->Ast());

            // Because same package files must be in one folder, relative path references in an external
            // source's import declaration certainly will be the same (and can be resolved) from the global program too
            package->Ast()->AddStatement(stmt);
        }
    }
    package->GetUnmergedPackagePrograms().resize(0);
}

static void ValidateImportDeclarationsSourcePath(const public_lib::Context *const ctx,
                                                 const parser::PackageProgram *package,
                                                 const std::vector<const ir::Statement *> &importDeclarations)
{
    for (const auto *const stmt : importDeclarations) {
        auto *importManager = ctx->parser->GetImportPathManager();
        auto *referencedProg = importManager->SearchResolved(stmt->AsETSImportDeclaration()->ImportMetadata());
        const bool doesImportFromPackage =
            (referencedProg != nullptr) && (referencedProg->ModuleName() == package->ModuleName());
        if (doesImportFromPackage) {
            ctx->parser->LogError(diagnostic::PACKAGE_MODULE_IMPORT_OWN_PACKAGE, {}, stmt->Start());
        }
    }
}

static void ValidateNoImportComesFromSamePackage(const public_lib::Context *const ctx, parser::PackageProgram *package)
{
    ES2PANDA_ASSERT(package->GetUnmergedPackagePrograms().size() == 0);  // Was just merged.

    {
        // Filter out only import declarations
        std::vector<const ir::Statement *> importDeclarations {};
        const auto &progStatements = package->Ast()->Statements();
        std::copy_if(progStatements.begin(), progStatements.end(), std::back_inserter(importDeclarations),
                     [](const ir::Statement *const stmt) { return stmt->IsETSImportDeclaration(); });

        // Validate if all import declaration refers to a path outside of the package module
        ValidateImportDeclarationsSourcePath(ctx, package, importDeclarations);
    }
}

// Why only "main" package program is being merged?
// NOTE (DZ) Now ModuleName = FileName != PackageName and the package name from `package` directive is not preserved
//           anywhere. Thus ValidateFolderContainOnlySamePackageFiles() method is invalid and senseless.
bool PackageImplicitImport::Perform()
{
    if (!Context()->parserProgram->Is<util::ModuleKind::PACKAGE>() || Context()->config->options->IsGenStdlib()) {
        // Only run for package module files
        return true;
    }

    auto *package = Context()->parserProgram->As<util::ModuleKind::PACKAGE>();
    auto &packagePrograms = package->GetUnmergedPackagePrograms();
    // NOTE (mmartin): Very basic sorting of files in the package, to merge them in a prescribed order
    std::stable_sort(packagePrograms.begin(), packagePrograms.end(),
                     [](const parser::Program *const prog1, const parser::Program *const prog2) {
                         return prog1->FileName() < prog2->FileName();
                     });

    MergeExternalFilesIntoCompiledProgram(package);
    ValidateNoImportComesFromSamePackage(Context(), package);

    return true;
}

}  // namespace ark::es2panda::compiler
