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

#include "checkerPhase.h"
#include "checker/checker.h"
#include "checker/ETSchecker.h"
#include "evaluate/scopedDebugInfoPlugin.h"

namespace ark::es2panda::compiler {

static void CreateDebuggerEvaluationPlugin(public_lib::Context *ctx)
{
    // Sometimes evaluation mode might work without project context.
    // In this case, users might omit context files.
    const auto &options = *ctx->config->options;
    if (options.IsDebuggerEval() && !options.GetDebuggerEvalPandaFiles().empty()) {
        auto *plugin = ctx->Allocator()->New<evaluate::ScopedDebugInfoPlugin>(ctx);
        ctx->GetChecker()->AsETSChecker()->SetDebugInfoPlugin(plugin);
    }
}

void CheckerPhase::Setup()
{
    Context()->GetChecker()->Initialize(Context()->parserProgram->VarBinder());
    if (Context()->GetChecker()->IsETSChecker()) {
        CreateDebuggerEvaluationPlugin(Context());
    }

    // for ast-cache using
    // NOTE(dkofanov): If present, the whole cache should be restored at once, at program-restoration. To be moved.
    if (Context()->parserProgram->VarBinder()->Extension() != ScriptExtension::ETS) {
        return;
    }
    Context()->GetChecker()->AsETSChecker()->ReputCheckerData();
}

static void MarkStatementsNoCleanup(parser::Program *program)
{
    for (auto stmt : program->Ast()->Statements()) {
        stmt->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);
    }
}

bool CheckerPhase::Perform()
{
    Context()->parserProgram->GetExternalSources()->Visit([](auto *extProg) {
        if (!extProg->IsASTLowered()) {
            MarkStatementsNoCleanup(extProg);
        }
    });
    MarkStatementsNoCleanup(Context()->parserProgram);

    if (Context()->parserProgram->Extension() == ScriptExtension::ETS) {
        try {
            Context()->GetChecker()->StartChecker(Context()->parserProgram->VarBinder(), *Context()->config->options);
        } catch (std::exception &e) {
            // nothing to do - just to avoid program crash
        }
        return true;
    }

    return Context()->GetChecker()->StartChecker(Context()->parserProgram->VarBinder(), *Context()->config->options);
}
}  // namespace ark::es2panda::compiler
