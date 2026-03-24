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

#include <cstring>
#include "public/es2panda_lib.h"
#include "util.h"

static es2panda_Impl *g_impl = nullptr;
static const char *const G_SOURCE = "export function foo(x?: int): void {}";
static std::string *g_dumpedSource = nullptr;

int main(int argc, char **argv)
{
    auto dumpTest = [](es2panda_Context *context) -> bool {
        const char *dumpedSourceInArenaMemory =
            g_impl->AstNodeDumpEtsSrcConst(context, g_impl->ProgramAst(context, g_impl->ContextProgram(context)));
        g_dumpedSource = new std::string(dumpedSourceInArenaMemory);
        return g_dumpedSource->find(G_SOURCE) != std::string::npos;
    };

    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions1;
    testFunctions1[ES2PANDA_STATE_CHECKED] = {dumpTest};
    ProccedToStatePluginTestData data1 = {argc, argv, &g_impl, testFunctions1, true, G_SOURCE, ES2PANDA_STATE_CHECKED};
    int result = RunAllStagesWithTestFunction(data1);
    if (result != 0) {
        delete g_dumpedSource;
        return result;
    }

    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions2;
    ProccedToStatePluginTestData data2 = {argc, argv, &g_impl, testFunctions2, true, *g_dumpedSource};
    result = RunAllStagesWithTestFunction(data2);
    delete g_dumpedSource;
    g_dumpedSource = nullptr;
    return result;
}
