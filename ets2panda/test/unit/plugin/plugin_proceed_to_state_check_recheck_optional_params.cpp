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

static es2panda_Impl *impl = nullptr;
static std::string g_source = R"(
function foo(a: number, b?: string, f?: () => string) {
}

foo(1) {
  return "Test"
}
)";

static bool CheckRecheckOptionalParams(es2panda_Context *context)
{
    auto *program = impl->ContextProgram(context);
    es2panda_AstNode *programNode = impl->ProgramAst(context, program);

    impl->AstNodeRecheck(context, programNode);
    CheckForErrors("RECHECKED", context);

    return impl->ContextState(context) != ES2PANDA_STATE_ERROR;
}

int main(int argc, char **argv)
{
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    testFunctions[ES2PANDA_STATE_CHECKED] = {CheckRecheckOptionalParams};
    ProccedToStatePluginTestData data = {argc, argv, &impl, testFunctions, true, g_source, ES2PANDA_STATE_CHECKED};
    return RunAllStagesWithTestFunction(data);
}
