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

#include "compiler/lowering/ets/topLevelStmts/topLevelStmts.h"

#include "compiler/lowering/ets/topLevelStmts/globalClassHandler.h"

namespace ark::es2panda::compiler {

static bool CheckSourceConsistency(parser::Program *program)
{
    if (!program->Is<util::ModuleKind::PACKAGE>()) {
        return true;
    }
    auto packageFractions = program->As<util::ModuleKind::PACKAGE>()->GetUnmergedPackagePrograms();
    if (std::all_of(packageFractions.begin(), packageFractions.end(),
                    [](auto p) { return p->GetModuleKind() != util::ModuleKind::PACKAGE; })) {
        return true;
    }
    std::stringstream ss;
    ss << "Package \"" << program->ModuleName() << "\" has incosistent fractions:";
    std::for_each(packageFractions.begin(), packageFractions.end(), [&ss](parser::Program *p) {
        ss << std::endl << "  at " << p->GetImportMetadata().ResolvedSource();
        if (p->GetModuleKind() == util::ModuleKind::PACKAGE) {
            ss << " (ok)";
        } else {
            ss << " (fail)";
        }
    });
    std::cerr << ss.str() << std::endl;
    return false;
}

static bool CheckProgramSourcesConsistency(parser::Program *globalProgram)
{
    bool success = true;
    globalProgram->GetExternalSources()->Visit(
        [&success](auto *extProg) { success &= CheckSourceConsistency(extProg); });
    // NOTE(dkofanov): direct to be removed.
    for (auto const &[_, program] : globalProgram->GetExternalSources()->Direct()) {
        (void)_;
        success &= CheckSourceConsistency(program);
    }
    return success;
}

static void AddExportModifierForInterface(ir::AstNode *const ast)
{
    auto body = ast->AsClassDeclaration()->Definition()->BodyForUpdate();
    for (auto it : body) {
        if (it->IsTSInterfaceDeclaration()) {
            it->AsTSInterfaceDeclaration()->AddModifier(ir::ModifierFlags::EXPORT);
        }
    }
}

static void DeclareNamespaceExportAdjust(parser::Program *program)
{
    program->Ast()->Iterate([](ir::AstNode *const ast) {
        if (ast->IsClassDeclaration() && ast->AsClassDeclaration()->Definition()->IsNamespaceTransformed() &&
            ast->AsClassDeclaration()->Definition()->IsDeclare() && (ast->IsExported() || ast->IsDefaultExported())) {
            AddExportModifierForInterface(ast);
        }
    });
}

static void CheckFileHeaderFlag(parser::Program *program)
{
    auto &statements = program->Ast()->StatementsForUpdates();
    if (statements.empty()) {
        return;
    }

    if (!statements.front()->IsExpressionStatement()) {
        return;
    }

    // If further processing based on "use static" is required later, such as throwing a warning or modifying the node,
    // perform the operation here.
    auto *expansion = statements.front()->AsExpressionStatement()->GetExpression();
    if (expansion->IsStringLiteral() && expansion->AsStringLiteral()->Str() == Signatures::STATIC_PROGRAM_FLAG) {
        statements.erase(statements.begin());
        return;
    }
}

bool TopLevelStatements::Perform()
{
    auto ctx = Context();
    parser::Program *globalProgram = ctx->parserProgram;
    CheckFileHeaderFlag(globalProgram);
    auto importsHandler = ImportExportDecls(ctx);
    importsHandler.IntroduceStdlibImportProgram();
    if (!CheckProgramSourcesConsistency(globalProgram)) {
        // NOTE(dkofanov): Should be already handled during 'ImportPathManager' routine, kept just in case.
        ES2PANDA_UNREACHABLE();
    }

    GlobalClassHandler globalClassIntroducer(ctx);
    // NOTE(dkofanov): Change 'Visit<false>' to 'Visit' when packages are merged:
    globalProgram->GetExternalSources()->Visit<false>([&importsHandler, &globalClassIntroducer](auto *extProgram) {
        if (extProgram->IsASTLowered()) {
            return;
        }
        importsHandler.HandleGlobalStmts(extProgram);
        globalClassIntroducer.SetupGlobalClass(extProgram);
        extProgram->MaybeIteratePackage([](parser::Program *prog) { DeclareNamespaceExportAdjust(prog); });
    });

    importsHandler.HandleGlobalStmts(globalProgram);
    globalClassIntroducer.SetupGlobalClass(globalProgram);
    DeclareNamespaceExportAdjust(globalProgram);

    return true;
}

}  // namespace ark::es2panda::compiler
