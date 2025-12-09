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

#include "util.h"
#include "public/es2panda_lib.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;
static std::string g_source = R"(
namespace NS1 {
    export namespace NS2 {
        export enum E {EE};
        export const x = () => {};
    }
}
export class A {
    public static readonly SOME_FIELD = NS1.NS2.E.EE;
}
)";

static std::string g_expected = "SOME_FIELD: NS1.NS2.E;\n";
static es2panda_AstNode *g_target_field = nullptr;

static void FindClassSomeFieldProperty(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsClassProperty(ast)) {
        return;
    }
    auto *ident = impl->ClassElementId(ctx, ast);
    if (ident == nullptr) {
        return;
    }

    auto name = std::string(impl->IdentifierName(ctx, ident));
    if (name == "SOME_FIELD") {
        g_target_field = ast;
    }
}

static bool CheckDeclDumpInferredTypeNamespaces(es2panda_Context *context)
{
    auto *ast = impl->ProgramAst(context, impl->ContextProgram(context));
    impl->AstNodeDumpDeclConst(context, ast);
    impl->AstNodeForEach(ast, FindClassSomeFieldProperty, context);
    std::string decl = impl->AstNodeDumpDeclConst(context, g_target_field);
    return decl.find(g_expected) != std::string::npos;
}

int main(int argc, char **argv)
{
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    testFunctions[ES2PANDA_STATE_CHECKED] = {CheckDeclDumpInferredTypeNamespaces};
    ProccedToStatePluginTestData data = {argc, argv, &impl, testFunctions, true, g_source, ES2PANDA_STATE_CHECKED};
    return RunAllStagesWithTestFunction(data);
}

// NOLINTEND
