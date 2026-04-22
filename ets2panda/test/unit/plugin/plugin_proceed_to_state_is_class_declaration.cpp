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

#include <map>
#include <cstring>
#include <algorithm>
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *g_impl = nullptr;
static es2panda_Context *g_ctx = nullptr;

static std::string g_source = R"(
enum Colors { Red, Blue = 3 };
let foo = () => { return 0; };
foo();
type t = () => void;
type T = int | double;
class A {
  x: int = 1;
}
class User {
    data = 'string';
}
const obj: User = { data: 'string' }
export const myVariable: int = 42;
)";

std::map<std::string, bool> g_results = {
    {"ArrowFunctionExpression", false}, {"CallExpression", false},   {"ClassDeclaration", false},
    {"ReturnStatement", false},         {"ETSTypeReference", false}, {"ETSUnionType", false},
    {"ETSFunctionType", false},         {"ClassProperty", false},    {"ObjectExpression", false},
    {"ETSParameterExpression", false},
};

static void Find(es2panda_AstNode *ast)
{
    if (g_impl->IsArrowFunctionExpression(ast)) {
        g_results["ArrowFunctionExpression"] = true;
    } else if (g_impl->IsCallExpression(ast)) {
        g_results["CallExpression"] = true;
    } else if (g_impl->IsClassDeclaration(ast)) {
        g_results["ClassDeclaration"] = true;
    } else if (g_impl->IsReturnStatement(ast)) {
        g_results["ReturnStatement"] = true;
    } else if (g_impl->IsETSTypeReference(ast)) {
        g_results["ETSTypeReference"] = true;
    } else if (g_impl->IsETSUnionType(ast)) {
        g_results["ETSUnionType"] = true;
    } else if (g_impl->IsETSFunctionType(ast)) {
        g_results["ETSFunctionType"] = true;
    } else if (g_impl->IsClassProperty(ast)) {
        g_results["ClassProperty"] = true;
    } else if (g_impl->IsObjectExpression(ast)) {
        g_results["ObjectExpression"] = true;
    } else if (g_impl->IsETSParameterExpression(ast)) {
        g_results["ETSParameterExpression"] = true;
    }

    g_impl->AstNodeIterateConst(g_ctx, ast, Find);
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return 1;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    g_impl = GetImpl();
    std::cout << "LOAD SUCCESS" << std::endl;
    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = g_impl->CreateConfig(argc - 1, args);
    g_ctx = g_impl->CreateContextFromString(config, g_source.data(), argv[argc - 1]);
    if (g_ctx == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    g_impl->ProceedToState(g_ctx, ES2PANDA_STATE_CHECKED);
    if (g_impl->ContextState(g_ctx) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    auto ast = g_impl->ProgramAst(g_ctx, g_impl->ContextProgram(g_ctx));
    g_impl->AstNodeIterateConst(g_ctx, ast, Find);

    for (const auto &[_, second] : g_results) {
        if (!second) {
            return TEST_ERROR_CODE;
        }
    }

    g_impl->AstNodeRecheck(g_ctx, ast);

    g_impl->ProceedToState(g_ctx, ES2PANDA_STATE_BIN_GENERATED);
    if (g_impl->ContextState(g_ctx) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }
    g_impl->DestroyContext(g_ctx);
    g_impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
