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

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include "util.h"
#include "public/es2panda_lib.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

static auto source = std::string("function main() { \nlet a = 5;\n arktest.assertEQ(a, 5);\n  }");
constexpr int NUM_OF_CHANGE_IDENTIFIERS = 2;

static es2panda_AstNode *mainScriptFunc = nullptr;
static std::vector<es2panda_AstNode *> changeIdentifiers {};
static es2panda_Context *ctx = nullptr;

static void FindChangeIdentifiers(es2panda_AstNode *ast)
{
    if (impl->IsIdentifier(ast) && std::string(impl->IdentifierName(ctx, ast)) == "a") {
        changeIdentifiers.emplace_back(ast);
    }
    impl->AstNodeIterateConst(ctx, ast, FindChangeIdentifiers);
}

static void FindMainDef(es2panda_AstNode *ast)
{
    if (!impl->IsMethodDefinition(ast)) {
        impl->AstNodeIterateConst(ctx, ast, FindMainDef);
        return;
    }
    auto scriptFunc = impl->MethodDefinitionFunction(ctx, ast);
    if (scriptFunc == nullptr ||
        std::string("main") != impl->IdentifierName(ctx, impl->ScriptFunctionId(ctx, scriptFunc))) {
        impl->AstNodeIterateConst(ctx, ast, FindMainDef);
        return;
    }
    mainScriptFunc = scriptFunc;
}

static bool ChangeAst(es2panda_Context *context)
{
    auto Ast = impl->ProgramAst(context, impl->ContextProgram(context));
    ctx = context;
    impl->AstNodeIterateConst(context, Ast, FindMainDef);
    if (mainScriptFunc == nullptr) {
        return false;
    }
    impl->AstNodeIterateConst(context, mainScriptFunc, FindChangeIdentifiers);
    if (changeIdentifiers.size() != NUM_OF_CHANGE_IDENTIFIERS) {
        return false;
    }

    std::string className = std::string("b");
    auto *memForName = static_cast<char *>(impl->AllocMemory(context, className.size() + 1, 1));
    std::copy_n(className.c_str(), className.size() + 1, memForName);

    for (auto ident : changeIdentifiers) {
        impl->IdentifierSetName(context, ident, memForName);
    }
    std::cout << impl->AstNodeDumpEtsSrcConst(context, Ast) << std::endl;
    impl->AstNodeRecheck(context, Ast);
    CheckForErrors("RECHECK", context);
    return true;
}

int main(int argc, char **argv)
{
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    testFunctions[ES2PANDA_STATE_CHECKED] = {ChangeAst};
    ProccedToStatePluginTestData data = {argc, argv, &impl, testFunctions, true, source};
    return RunAllStagesWithTestFunction(data);
}

// NOLINTEND
