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

#include <iostream>
#include "util.h"

static es2panda_Impl *g_impl = nullptr;

static bool PerformAnnotationRecheck(es2panda_Context *context)
{
    auto *program = g_impl->ContextProgram(context);
    auto *programAst = g_impl->ProgramAst(context, program);

    std::cout << "Performing recheck..." << std::endl;
    g_impl->AstNodeRecheck(context, programAst);

    if (g_impl->IsAnyError(context)) {
        std::cerr << "Recheck failed with errors" << std::endl;
        return false;
    }

    if (g_impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        std::cerr << "Context in ERROR state after recheck" << std::endl;
        return false;
    }

    std::cout << "Recheck passed successfully" << std::endl;
    return true;
}

int main(int argc, char **argv)
{
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    testFunctions[ES2PANDA_STATE_CHECKED] = {PerformAnnotationRecheck};
    ProccedToStatePluginTestData data = {argc, argv, &g_impl, testFunctions, false, "", ES2PANDA_STATE_CHECKED};

    return RunAllStagesWithTestFunction(data);
}