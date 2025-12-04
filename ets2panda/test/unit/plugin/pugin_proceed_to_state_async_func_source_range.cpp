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

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN
static std::string g_source = R"(
function foo1() {
    let x1 = 1;
}

async function foo2() {
    let x2 = 1;
}
)";

constexpr size_t FUNC_START_LINE = 5;
constexpr size_t FUNC_END_LINE = 7;
constexpr size_t FUNC_START_COL = 1;
constexpr size_t FUNC_END_COL = 2;

constexpr size_t FUNC_X2_DECL_START_LINE = 6;
constexpr size_t FUNC_X2_DECL_END_LINE = 6;
constexpr size_t FUNC_X2_DECL_START_COL = 9;
constexpr size_t FUNC_X2_DECL_END_COL = 15;

static es2panda_Impl *impl = nullptr;
static es2panda_AstNode *fooScrFunc = nullptr;
static es2panda_AstNode *varDecl = nullptr;

static void FindMethodDef(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsMethodDefinition(ast)) {
        return;
    }
    auto *function = impl->MethodDefinitionFunction(ctx, ast);
    if (function == nullptr) {
        return;
    }
    auto *ident = impl->ScriptFunctionId(ctx, function);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(ctx, ident));
    if (name == "foo2") {
        fooScrFunc = ast;
    }
}

static void FindVariableDecl(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsVariableDeclarator(ast)) {
        return;
    }
    auto *ident = impl->VariableDeclaratorId(ctx, ast);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(ctx, ident));
    if (name == "x2") {
        varDecl = ast;
    }
}

static bool CheckAsyncFunctionSourceRange(es2panda_Context *context)
{
    auto *ast = impl->ProgramAst(context, impl->ContextProgram(context));
    impl->AstNodeForEach(ast, FindMethodDef, context);
    impl->AstNodeForEach(ast, FindVariableDecl, context);
    auto funcStart = impl->AstNodeStartConst(context, fooScrFunc);
    auto funcEnd = impl->AstNodeEndConst(context, fooScrFunc);
    auto declStart = impl->AstNodeStartConst(context, varDecl);
    auto declEnd = impl->AstNodeEndConst(context, varDecl);

    return FUNC_START_LINE == impl->SourcePositionLine(context, funcStart) &&
           FUNC_END_LINE == impl->SourcePositionLine(context, funcEnd) &&
           FUNC_START_COL == impl->SourcePositionCol(context, funcStart) &&
           FUNC_END_COL == impl->SourcePositionCol(context, funcEnd) &&
           FUNC_X2_DECL_START_LINE == impl->SourcePositionLine(context, declStart) &&
           FUNC_X2_DECL_END_LINE == impl->SourcePositionLine(context, declEnd) &&
           FUNC_X2_DECL_START_COL == impl->SourcePositionCol(context, declStart) &&
           FUNC_X2_DECL_END_COL == impl->SourcePositionCol(context, declEnd);
}

int main(int argc, char **argv)
{
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    testFunctions[ES2PANDA_STATE_LOWERED] = {CheckAsyncFunctionSourceRange};
    ProccedToStatePluginTestData data = {
        argc, argv, &impl, testFunctions, true, g_source, ES2PANDA_STATE_BIN_GENERATED};
    return RunAllStagesWithTestFunction(data);
}

// NOLINTEND
