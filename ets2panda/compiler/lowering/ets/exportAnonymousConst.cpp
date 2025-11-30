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

#include "compiler/lowering/ets/exportAnonymousConst.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

static ir::AstNode *CreateAnonymousVariableDecl(public_lib::Context *ctx, ir::ExportNamedDeclaration *exportDecl)
{
    [[maybe_unused]] const size_t exportDefaultMaxSize = 1;
    auto *parser = ctx->parser->AsETSParser();
    auto *allocator = ctx->allocator;

    auto *anonymousIdentifier =
        allocator->New<ir::Identifier>(exportDecl->Specifiers().front()->Exported()->Name(), allocator);
    anonymousIdentifier->SetRange(exportDecl->Specifiers().front()->GetConstantExpression()->Range());
    ES2PANDA_ASSERT(exportDecl->Specifiers().size() == exportDefaultMaxSize);
    auto *anonymousConstant = parser->CreateFormattedExpression(
        "const @@I1 = @@E2;", anonymousIdentifier, exportDecl->Specifiers().front()->GetConstantExpression());

    auto *anonymousVariableDecl = anonymousConstant->AsBlockExpression()->Statements().front()->AsVariableDeclaration();
    anonymousVariableDecl->AddModifier(ir::ModifierFlags::CONST | ir::ModifierFlags::STATIC |
                                       ir::ModifierFlags::PUBLIC);

    return anonymousVariableDecl;
}

bool IsDefaultExport(ark::es2panda::ir::Statement *ast)
{
    if (ast->IsExportNamedDeclaration()) {
        return std::any_of(ast->AsExportNamedDeclaration()->Specifiers().begin(),
                           ast->AsExportNamedDeclaration()->Specifiers().end(),
                           [](auto *specific) { return specific->IsDefault() || specific->IsDefaultExported(); });
    }
    if (ast->IsETSReExportDeclaration()) {
        return std::any_of(
            ast->AsETSReExportDeclaration()->GetETSImportDeclarations()->Specifiers().begin(),
            ast->AsETSReExportDeclaration()->GetETSImportDeclarations()->Specifiers().end(), [](auto *specific) {
                return specific->IsImportSpecifier() && specific->AsImportSpecifier()->Local()->Name() ==
                                                            compiler::Signatures::REEXPORT_DEFAULT_ANONYMOUSLY;
            });
    }

    return (ast->AsStatement()->Modifiers() & ir::ModifierFlags::DEFAULT_EXPORT) != 0U;
}

static void HandleAnonymousConst(public_lib::Context *const ctx, parser::Program *const program)
{
    /* The Single Export Directive can directly export anonymous constant variables
     * export default new A()
     * ----- After Parser -----
     * export default genName
     * ----- perform the following conversion this phase -----
     * const genName = new A()
     * export default genName
     */
    auto isExportAnonymousConst = [](ir::AstNode *ast) {
        if (!ast->IsExportNamedDeclaration()) {
            return false;
        }
        return std::any_of(
            ast->AsExportNamedDeclaration()->Specifiers().begin(), ast->AsExportNamedDeclaration()->Specifiers().end(),
            [](auto *specific) { return specific->IsDefault() && specific->GetConstantExpression() != nullptr; });
    };
    auto module = program->Ast();
    const size_t exportDefaultMaxSize = 1;
    std::vector<ark::es2panda::ir::Statement *> defaultExportStatements;
    std::copy_if(module->Statements().begin(), module->Statements().end(), std::back_inserter(defaultExportStatements),
                 IsDefaultExport);
    if (defaultExportStatements.size() > exportDefaultMaxSize) {
        lexer::SourcePosition multiplePos = defaultExportStatements.back()->AsStatement()->Start();
        ctx->GetChecker()->AsETSChecker()->LogError(diagnostic::MULTIPLE_DEFAULT_EXPORTS, multiplePos);
        return;
    }
    auto iteratorForFind =
        std::find_if(module->Statements().begin(), module->Statements().end(), isExportAnonymousConst);
    if (iteratorForFind == module->Statements().end()) {
        return;
    }
    auto &stmt = module->StatementsForUpdates();
    auto iterator = std::find_if(stmt.begin(), stmt.end(), isExportAnonymousConst);

    if ((*iterator)->AsExportNamedDeclaration()->Specifiers().size() != exportDefaultMaxSize) {
        ctx->GetChecker()->AsETSChecker()->LogError(diagnostic::MULTIPLE_DEFAULT_EXPORTS,
                                                    (*iterator)->AsExportNamedDeclaration()->Start());
        return;
    }

    auto *anonymousVariableDecl =
        CreateAnonymousVariableDecl(ctx, (*iterator)->AsExportNamedDeclaration())->AsStatement();
    stmt.insert(iterator, anonymousVariableDecl);
}

static void HandleExportDefaultInExportNamedDecl(public_lib::Context *const ctx, parser::Program *const program)
{
    /* Create a separate ExportNamedDeclaration for export default and add the Export_default flag
     * export {new A() as default,B as B, C as C}
     * ----- perform the following conversion this phase -----
     * export default new A()
     * export {B as B, C as C}
     */
    auto const exportNamedDeclarationHasDefault = [](ir::AstNode *ast) -> bool {
        if (!ast->IsExportNamedDeclaration()) {
            return false;
        }
        auto const &specifiers = ast->AsExportNamedDeclaration()->Specifiers();
        return std::any_of(specifiers.cbegin(), specifiers.cend(),
                           [](auto *specifier) { return specifier->IsDefault(); });
    };

    auto *const module = program->Ast();
    if (auto const &statements = module->Statements();
        std::find_if(statements.cbegin(), statements.end(), exportNamedDeclarationHasDefault) == statements.cend()) {
        return;
    }

    auto *allocator = ctx->allocator;
    auto &stmt = module->StatementsForUpdates();

    auto iterator = std::find_if(stmt.begin(), stmt.end(), exportNamedDeclarationHasDefault);
    auto *const exportNamedDeclaration = (*iterator)->AsExportNamedDeclaration();

    auto &specifiers = exportNamedDeclaration->Specifiers();
    auto specifier = specifiers.begin();

    while (specifier != specifiers.end()) {
        if ((*specifier)->IsDefault()) {
            ArenaVector<ir::ExportSpecifier *> exports(allocator->Adapter());
            exports.emplace_back(*specifier);

            ir::ExportNamedDeclaration *exportDefaultNamedDeclaration = allocator->New<ir::ExportNamedDeclaration>(
                allocator, static_cast<ir::StringLiteral *>(nullptr), std::move(exports));
            exportDefaultNamedDeclaration->AddModifier(ir::ModifierFlags::DEFAULT_EXPORT);
            exportDefaultNamedDeclaration->SetParent(exportNamedDeclaration->Parent());
            exportDefaultNamedDeclaration->SetRange(exportNamedDeclaration->Range());

            iterator = std::next(stmt.insert(iterator, exportDefaultNamedDeclaration));
            specifier = specifiers.erase(specifier);
        } else {
            ++specifier;
        }
    }

    if (!specifiers.empty()) {
        exportNamedDeclaration->ClearModifier(ir::ModifierFlags::DEFAULT_EXPORT);
    } else {
        stmt.erase(iterator);
    }
}

bool ExportAnonymousConstPhase::PerformForModule(public_lib::Context *const ctx, parser::Program *const program)
{
    HandleExportDefaultInExportNamedDecl(ctx, program);
    HandleAnonymousConst(ctx, program);
    return true;
}

}  // namespace ark::es2panda::compiler
