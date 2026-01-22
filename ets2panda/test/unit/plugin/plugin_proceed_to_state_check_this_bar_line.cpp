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

#include <cstddef>
#include <cstring>
#include <algorithm>
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *g_impl = nullptr;
static std::string g_source = R"(function foo(x: ()=>void) {
}
class A {
    bar():void {}
    f1() {
        foo(()=> {
          this.bar();
        })
    }
}
)";

constexpr size_t THIS_IN_NEW_START_LINE = 6;
static es2panda_AstNode *thisExprInNew = nullptr;

static void FindThisInNewCall(es2panda_AstNode *ast, [[maybe_unused]] void *context)
{
    if (!g_impl->IsThisExpression(ast)) {
        return;
    }

    thisExprInNew = ast;
}

static void FindCctorTest(es2panda_AstNode *ast, [[maybe_unused]] void *context)
{
    auto g_ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!g_impl->IsClassDeclaration(ast)) {
        return;
    }
    auto *ident = g_impl->ClassDefinitionIdent(g_ctx, g_impl->ClassDeclarationDefinition(g_ctx, ast));
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(g_impl->IdentifierName(g_ctx, ident));
    if (name != "A") {
        return;
    }
    auto *classDef = g_impl->ClassDeclarationDefinition(g_ctx, ast);

    g_impl->AstNodeForEach(classDef, FindThisInNewCall, g_ctx);
}

static bool CheckThisInNewLine(es2panda_Context *context)
{
    auto g_ctx = reinterpret_cast<es2panda_Context *>(context);
    if (thisExprInNew == nullptr) {
        return false;
    }
    auto start = g_impl->AstNodeStartConst(g_ctx, thisExprInNew);
    auto res = THIS_IN_NEW_START_LINE == g_impl->SourcePositionLine(context, start);
    ASSERT(THIS_IN_NEW_START_LINE == g_impl->SourcePositionLine(context, start));

    return res;
}

static bool CheckSourceRange(es2panda_Context *context)
{
    auto *ast = g_impl->ProgramAst(context, g_impl->ContextProgram(context));
    g_impl->AstNodeForEach(ast, FindCctorTest, context);

    return CheckThisInNewLine(context);
}

int main(int argc, char **argv)
{
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    testFunctions[ES2PANDA_STATE_LOWERED] = {CheckSourceRange};
    ProccedToStatePluginTestData data = {
        argc, argv, &g_impl, testFunctions, true, g_source, ES2PANDA_STATE_BIN_GENERATED};
    return RunAllStagesWithTestFunction(data);
}

// NOLINTEND
