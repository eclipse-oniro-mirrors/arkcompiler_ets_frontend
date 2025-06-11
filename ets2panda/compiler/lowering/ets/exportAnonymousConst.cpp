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
#include "libarkbase/macros.h"

namespace ark::es2panda::compiler {

static ir::AstNode *CreateAnonymousVariableDecl(public_lib::Context *ctx, ir::ExportNamedDeclaration *exportDecl)
{
    [[maybe_unused]] const size_t exportDefaultMaxSize = 1;
    auto *parser = ctx->parser->AsETSParser();
    auto *allocator = ctx->allocator;

    auto *anonymousIdentifier =
        allocator->New<ir::Identifier>(exportDecl->Specifiers().front()->Exported()->Name(), allocator);
    ES2PANDA_ASSERT(exportDecl->Specifiers().size() == exportDefaultMaxSize);
    auto *anonymousConstant = parser->CreateFormattedExpression(
        "const @@I1 = @@E2;", anonymousIdentifier, exportDecl->Specifiers().front()->GetConstantExpression());

    auto *anonymousVariableDecl = anonymousConstant->AsBlockExpression()->Statements().front()->AsVariableDeclaration();
    anonymousVariableDecl->AddModifier(ir::ModifierFlags::CONST | ir::ModifierFlags::STATIC |
                                       ir::ModifierFlags::PUBLIC);

    return anonymousVariableDecl;
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
    auto iteratorForFind =
        std::find_if(module->Statements().begin(), module->Statements().end(), isExportAnonymousConst);
    if (iteratorForFind == module->Statements().end()) {
        return;
    }
    auto &stmt = module->StatementsForUpdates();
    auto iterator = std::find_if(stmt.begin(), stmt.end(), isExportAnonymousConst);

    [[maybe_unused]] const size_t exportDefaultMaxSize = 1;
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
    auto exportNamedDeclarationhasDefault = [](ir::AstNode *ast) {
        if (!ast->IsExportNamedDeclaration()) {
            return false;
        }
        return std::any_of(ast->AsExportNamedDeclaration()->Specifiers().begin(),
                           ast->AsExportNamedDeclaration()->Specifiers().end(),
                           [](auto *specific) { return specific->IsDefault(); });
    };

    auto module = program->Ast();
    auto iteratorConst =
        std::find_if(module->Statements().begin(), module->Statements().end(), exportNamedDeclarationhasDefault);
    if (iteratorConst == module->Statements().end()) {
        return;
    }

    auto &stmt = module->StatementsForUpdates();

    auto iterator = std::find_if(stmt.begin(), stmt.end(), exportNamedDeclarationhasDefault);

    auto *allocator = ctx->allocator;
    auto *exportNamedDeclaration = (*iterator)->AsExportNamedDeclaration();
    auto oldSpecifiers = exportNamedDeclaration->Specifiers();
    ArenaVector<ir::ExportSpecifier *> newSpecifiers(allocator->Adapter());
    ArenaVector<ir::ExportNamedDeclaration *> exportDefaulNamedDeclarations(allocator->Adapter());

    for (auto *specifier : oldSpecifiers) {
        if (specifier->IsDefault()) {
            ArenaVector<ir::ExportSpecifier *> exports(allocator->Adapter());
            exports.emplace_back(specifier);
            auto *exportDefaulNamedDecl = allocator->New<ir::ExportNamedDeclaration>(
                allocator, static_cast<ir::StringLiteral *>(nullptr), std::move(exports));
            ES2PANDA_ASSERT(exportDefaulNamedDecl);
            exportDefaulNamedDecl->AddModifier(ir::ModifierFlags::DEFAULT_EXPORT);
            exportDefaulNamedDeclarations.push_back(exportDefaulNamedDecl);
            continue;
        }
        newSpecifiers.push_back(specifier);
    }

    stmt.insert(iterator, exportDefaulNamedDeclarations.front());
    exportNamedDeclaration->ReplaceSpecifiers(newSpecifiers);
    exportNamedDeclaration->ClearModifier(ir::ModifierFlags::DEFAULT_EXPORT);
}

bool ExportAnonymousConstPhase::PerformForModule(public_lib::Context *const ctx, parser::Program *const program)
{
    HandleExportDefaultInExportNamedDecl(ctx, program);
    HandleAnonymousConst(ctx, program);
    return true;
}

}  // namespace ark::es2panda::compiler
