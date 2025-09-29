/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "cfgBuilderPhase.h"
#include "compiler/core/CFG.h"
#include "util/options.h"

namespace ark::es2panda::compiler {

static void DumpCFG(parser::Program *program)
{
    compiler::CFG *cfg = program->GetCFG();

    program->Ast()->IterateRecursively([&](ir::AstNode *node) -> void {
        if (node->IsScriptFunction()) {
            cfg->Build(node->AsScriptFunction());
        }
    });

    cfg->MergeEmptyBlocks();

    std::string filename = program->AbsoluteName().Mutf8();
    cfg->DumpDot((filename + ".dot").c_str());
}

bool CFGBuilderPhase::PerformForProgram(parser::Program *program)
{
    auto ctx = Context();
    if (!ctx->config->options->IsDumpCfg() || ctx->config->options->IsGenStdlib()) {
        return true;
    }

    if (program->IsASTLowered()) {
        return true;
    }

    DumpCFG(program);

    return true;
}

}  // namespace ark::es2panda::compiler
