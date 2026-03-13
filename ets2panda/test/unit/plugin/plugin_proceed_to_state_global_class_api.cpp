/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "util.h"
#include "public/es2panda_lib.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;
static size_t g_count_global = 0;
static std::string g_source = R"(
class A {
    public x: int = 0;
}

class B {
    public x: int = 1;
}

function supportedLocalesOf(options: A | B): int {
    return options.x;
}

)";

static void FindAllGlobalClassDefinitions(es2panda_AstNode *ast, void *context)
{
    [[maybe_unused]] auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (impl->IsClassDefinition(ast) && impl->ClassDefinitionIsGlobalConst(ctx, ast)) {
        ++g_count_global;
    }
}

static bool CheckRecheckInterfaceImplements(es2panda_Context *context)
{
    auto *program = impl->ContextProgram(context);
    es2panda_AstNode *ast = impl->ProgramAst(context, program);

    impl->AstNodeForEach(ast, FindAllGlobalClassDefinitions, context);

    // There sould be only 1 global class
    return impl->ContextState(context) != ES2PANDA_STATE_ERROR && g_count_global == 1;
}

int main(int argc, char **argv)
{
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    testFunctions[ES2PANDA_STATE_LOWERED] = {CheckRecheckInterfaceImplements};
    ProccedToStatePluginTestData data = {argc, argv, &impl, testFunctions, true, g_source, ES2PANDA_STATE_LOWERED};
    return RunAllStagesWithTestFunction(data);
}

// NOLINTEND