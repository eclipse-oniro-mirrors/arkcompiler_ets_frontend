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
function func1() {
    return 1;
}

function func2(num: number) {
    let x1 = 1;
    let y1 = num;
}

function func2(str: string, num: number) {
    let x2 = str;
    let y2 = num;
}

function func2(bool: boolean, str: string, num: number) {
    let x3 = bool;
    let y3 = str;
    let z3 = num;
}
)";

constexpr size_t FUNC1_START_LINE = 1;
constexpr size_t FUNC1_END_LINE = 3;
constexpr size_t FUNC2_V1_START_LINE = 0;
constexpr size_t FUNC2_V1_END_LINE = 20;
constexpr size_t FUNC2_V2_START_LINE = 10;
constexpr size_t FUNC2_V2_END_LINE = 13;
constexpr size_t FUNC2_V3_START_LINE = 15;
constexpr size_t FUNC2_V3_END_LINE = 19;

constexpr size_t FUNC1_START_COL = 1;
constexpr size_t FUNC1_END_COL = 2;
constexpr size_t FUNC2_V1_START_COL = 1;
constexpr size_t FUNC2_V1_END_COL = 1;
constexpr size_t FUNC2_V2_START_COL = 1;
constexpr size_t FUNC2_V2_END_COL = 2;
constexpr size_t FUNC2_V3_START_COL = 1;
constexpr size_t FUNC2_V3_END_COL = 2;

constexpr size_t FUNC2_V1_X1_DECL_START_LINE = 6;
constexpr size_t FUNC2_V1_X1_DECL_END_LINE = 6;
constexpr size_t FUNC2_V1_X1_DECL_START_COL = 9;
constexpr size_t FUNC2_V1_X1_DECL_END_COL = 15;
constexpr size_t FUNC2_V1_Y1_DECL_START_LINE = 7;
constexpr size_t FUNC2_V1_Y1_DECL_END_LINE = 7;
constexpr size_t FUNC2_V1_Y1_DECL_START_COL = 9;
constexpr size_t FUNC2_V1_Y1_DECL_END_COL = 17;
constexpr size_t FUNC2_V2_X2_DECL_START_LINE = 11;
constexpr size_t FUNC2_V2_X2_DECL_END_LINE = 11;
constexpr size_t FUNC2_V2_X2_DECL_START_COL = 9;
constexpr size_t FUNC2_V2_X2_DECL_END_COL = 17;
constexpr size_t FUNC2_V2_Y2_DECL_START_LINE = 12;
constexpr size_t FUNC2_V2_Y2_DECL_END_LINE = 12;
constexpr size_t FUNC2_V2_Y2_DECL_START_COL = 9;
constexpr size_t FUNC2_V2_Y2_DECL_END_COL = 17;
constexpr size_t FUNC2_V3_X3_DECL_START_LINE = 16;
constexpr size_t FUNC2_V3_X3_DECL_END_LINE = 16;
constexpr size_t FUNC2_V3_X3_DECL_START_COL = 9;
constexpr size_t FUNC2_V3_X3_DECL_END_COL = 18;
constexpr size_t FUNC2_V3_Y3_DECL_START_LINE = 17;
constexpr size_t FUNC2_V3_Y3_DECL_END_LINE = 17;
constexpr size_t FUNC2_V3_Y3_DECL_START_COL = 9;
constexpr size_t FUNC2_V3_Y3_DECL_END_COL = 17;
constexpr size_t FUNC2_V3_Z3_DECL_START_LINE = 18;
constexpr size_t FUNC2_V3_Z3_DECL_END_LINE = 18;
constexpr size_t FUNC2_V3_Z3_DECL_START_COL = 9;
constexpr size_t FUNC2_V3_Z3_DECL_END_COL = 17;

constexpr size_t FUNC2_V1_PARAM_COUNT = 1;
constexpr size_t FUNC2_V2_PARAM_COUNT = 2;
constexpr size_t FUNC2_V3_PARAM_COUNT = 3;

static std::map<std::string, size_t> startLineMap = {
    {"func1", FUNC1_START_LINE},         {"func2_v1", FUNC2_V1_START_LINE},   {"func2_v2", FUNC2_V2_START_LINE},
    {"func2_v3", FUNC2_V3_START_LINE},   {"x1", FUNC2_V1_X1_DECL_START_LINE}, {"y1", FUNC2_V1_Y1_DECL_START_LINE},
    {"x2", FUNC2_V2_X2_DECL_START_LINE}, {"y2", FUNC2_V2_Y2_DECL_START_LINE}, {"x3", FUNC2_V3_X3_DECL_START_LINE},
    {"y3", FUNC2_V3_Y3_DECL_START_LINE}, {"z3", FUNC2_V3_Z3_DECL_START_LINE}};
static std::map<std::string, size_t> endLineMap = {
    {"func1", FUNC1_END_LINE},         {"func2_v1", FUNC2_V1_END_LINE},   {"func2_v2", FUNC2_V2_END_LINE},
    {"func2_v3", FUNC2_V3_END_LINE},   {"x1", FUNC2_V1_X1_DECL_END_LINE}, {"y1", FUNC2_V1_Y1_DECL_END_LINE},
    {"x2", FUNC2_V2_X2_DECL_END_LINE}, {"y2", FUNC2_V2_Y2_DECL_END_LINE}, {"x3", FUNC2_V3_X3_DECL_END_LINE},
    {"y3", FUNC2_V3_Y3_DECL_END_LINE}, {"z3", FUNC2_V3_Z3_DECL_END_LINE}};
static std::map<std::string, size_t> startColMap = {
    {"func1", FUNC1_START_COL},         {"func2_v1", FUNC2_V1_START_COL},   {"func2_v2", FUNC2_V2_START_COL},
    {"func2_v3", FUNC2_V3_START_COL},   {"x1", FUNC2_V1_X1_DECL_START_COL}, {"y1", FUNC2_V1_Y1_DECL_START_COL},
    {"x2", FUNC2_V2_X2_DECL_START_COL}, {"y2", FUNC2_V2_Y2_DECL_START_COL}, {"x3", FUNC2_V3_X3_DECL_START_COL},
    {"y3", FUNC2_V3_Y3_DECL_START_COL}, {"z3", FUNC2_V3_Z3_DECL_START_COL}};
static std::map<std::string, size_t> endColMap = {
    {"func1", FUNC1_END_COL},         {"func2_v1", FUNC2_V1_END_COL},   {"func2_v2", FUNC2_V2_END_COL},
    {"func2_v3", FUNC2_V3_END_COL},   {"x1", FUNC2_V1_X1_DECL_END_COL}, {"y1", FUNC2_V1_Y1_DECL_END_COL},
    {"x2", FUNC2_V2_X2_DECL_END_COL}, {"y2", FUNC2_V2_Y2_DECL_END_COL}, {"x3", FUNC2_V3_X3_DECL_END_COL},
    {"y3", FUNC2_V3_Y3_DECL_END_COL}, {"z3", FUNC2_V3_Z3_DECL_END_COL}};

static es2panda_Impl *impl = nullptr;
static std::map<std::string, es2panda_AstNode *> methodMap = {
    {"func1", nullptr}, {"func2_v1", nullptr}, {"func2_v2", nullptr}, {"func2_v3", nullptr}};
static std::map<std::string, es2panda_AstNode *> variableDeclMap = {{"x1", nullptr}, {"y1", nullptr}, {"x2", nullptr},
                                                                    {"y2", nullptr}, {"x3", nullptr}, {"y3", nullptr},
                                                                    {"z3", nullptr}};

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
    size_t paramCount = 0;
    impl->ScriptFunctionParams(ctx, function, &paramCount);

    if (name == "func1") {
        methodMap["func1"] = ast;
    } else if (name == "func2") {
        if (paramCount == FUNC2_V1_PARAM_COUNT) {
            methodMap["func2_v1"] = ast;
        } else if (paramCount == FUNC2_V2_PARAM_COUNT) {
            methodMap["func2_v2"] = ast;
        } else if (paramCount == FUNC2_V3_PARAM_COUNT) {
            methodMap["func2_v3"] = ast;
        }
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
    if (variableDeclMap.find(name) != variableDeclMap.end()) {
        variableDeclMap[name] = ast;
    }
}

static bool CheckLineAndCol(es2panda_AstNode *ast, std::string name, es2panda_Context *context)
{
    auto start = impl->AstNodeStartConst(context, ast);
    auto end = impl->AstNodeEndConst(context, ast);
    auto res = startLineMap[name] == impl->SourcePositionLine(context, start);
    ASSERT(startLineMap[name] == impl->SourcePositionLine(context, start));

    res &= startColMap[name] == impl->SourcePositionCol(context, start);
    ASSERT(startColMap[name] == impl->SourcePositionCol(context, start));

    res &= endLineMap[name] == impl->SourcePositionLine(context, end);
    ASSERT(endLineMap[name] == impl->SourcePositionLine(context, end));

    res &= endColMap[name] == impl->SourcePositionCol(context, end);
    ASSERT(endColMap[name] == impl->SourcePositionCol(context, end));
    return res;
}

static bool CheckAllFunctions(es2panda_Context *context)
{
    bool allPassed = true;
    for (const auto &[name, targetAst] : methodMap) {
        allPassed &= CheckLineAndCol(targetAst, name, context);
    }
    for (const auto &[name, targetAst] : variableDeclMap) {
        allPassed &= CheckLineAndCol(targetAst, name, context);
    }
    return allPassed;
}

static bool CheckOverloadSourceRange(es2panda_Context *context)
{
    auto *ast = impl->ProgramAst(context, impl->ContextProgram(context));
    impl->AstNodeForEach(ast, FindMethodDef, context);
    impl->AstNodeForEach(ast, FindVariableDecl, context);
    return CheckAllFunctions(context);
}

int main(int argc, char **argv)
{
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    testFunctions[ES2PANDA_STATE_LOWERED] = {CheckOverloadSourceRange};
    ProccedToStatePluginTestData data = {
        argc, argv, &impl, testFunctions, true, g_source, ES2PANDA_STATE_BIN_GENERATED};
    return RunAllStagesWithTestFunction(data);
}

// NOLINTEND
