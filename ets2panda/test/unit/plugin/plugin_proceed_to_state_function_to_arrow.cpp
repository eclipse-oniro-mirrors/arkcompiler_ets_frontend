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

#include <iostream>
#include <ostream>
#include <string>
#include <cstring>
#include <vector>
#include "util.h"
#include "public/es2panda_lib.h"
#include "libarkbase/os/library_loader.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;
static es2panda_Context *ctx = nullptr;

static std::string source = R"(
// Arrow function
const add = (a: number, b: number) => {
    return a + b;
};

// One-line arrow function
const multiply = (x: number, y: number) => x * y;

// Arrow function for string processing
const greet = (name: string) => {
    return "Hello, " + name;
};

console.log(add(5, 3));
console.log(multiply(4, 7));
console.log(greet("World"));
)";

static es2panda_AstNode *CreateFunctionBody(es2panda_AstNode *scriptFunc);
static es2panda_FunctionSignature *CreateFunctionSignature(es2panda_AstNode *scriptFunc);

struct ArrowFunctionTransform {
    es2panda_AstNode *varDecl;
    es2panda_AstNode *arrowFunc;
    std::string funcName;
};

static std::vector<ArrowFunctionTransform> arrowsToTransform;

void FindArrowFunctions(es2panda_AstNode *node, void *arg)
{
    (void)arg;

    if (impl->IsExportNamedDeclaration(node) || !impl->IsVariableDeclaration(node)) {
        return;
    }

    size_t declaratorsCount = 0;
    auto **declarators = impl->VariableDeclarationDeclaratorsConst(ctx, node, &declaratorsCount);

    for (size_t i = 0; i < declaratorsCount; i++) {
        auto *varDeclarator = declarators[i];

        auto *init = impl->VariableDeclaratorInit(ctx, varDeclarator);
        if (init == nullptr || !impl->IsArrowFunctionExpression(init)) {
            continue;
        }

        auto *id = impl->VariableDeclaratorId(ctx, varDeclarator);
        if (!impl->IsIdentifier(id)) {
            continue;
        }

        const char *funcName = impl->IdentifierName(ctx, id);
        arrowsToTransform.push_back({node, init, funcName});
    }
}

static es2panda_AstNode *CreateFunctionBody(es2panda_AstNode *scriptFunc)
{
    auto *body = impl->ScriptFunctionBody(ctx, scriptFunc);
    if (!body) {
        return nullptr;
    }

    auto *blockStmt = impl->CreateBlockStatement(ctx, nullptr, 0);
    auto *returnStmt = impl->CreateReturnStatement(ctx);
    if (!blockStmt || !returnStmt) {
        return nullptr;
    }

    if (impl->IsBlockStatement(body)) {
        size_t stmtCount = 0;
        auto **stmts = impl->BlockStatementStatements(ctx, body, &stmtCount);
        if (stmts && stmtCount > 0 && impl->IsReturnStatement(stmts[0])) {
            auto *returnArg = impl->ReturnStatementArgument(ctx, stmts[0]);
            impl->ReturnStatementSetArgument(ctx, returnStmt, returnArg);
        }
    } else if (impl->IsExpression(body)) {
        impl->ReturnStatementSetArgument(ctx, returnStmt, body);
    }

    auto **stmts = static_cast<es2panda_AstNode **>(impl->AllocMemory(ctx, 1, sizeof(es2panda_AstNode *)));
    stmts[0] = returnStmt;
    impl->BlockStatementSetStatements(ctx, blockStmt, stmts, 1);
    return blockStmt;
}

static es2panda_FunctionSignature *CreateFunctionSignature(es2panda_AstNode *scriptFunc)
{
    size_t paramCount = 0;
    auto **params = impl->ScriptFunctionParams(ctx, scriptFunc, &paramCount);
    return impl->CreateFunctionSignature(ctx, nullptr, params, paramCount, nullptr, false);
}

static void SetParentRelations(es2panda_AstNode *funcDecl, es2panda_AstNode *funcId, es2panda_AstNode *blockStmt,
                               es2panda_AstNode **params, size_t paramCount)
{
    impl->AstNodeSetParent(ctx, funcId, funcDecl);
    impl->AstNodeSetParent(ctx, blockStmt, funcDecl);
    for (size_t i = 0; i < paramCount; i++) {
        if (params[i])
            impl->AstNodeSetParent(ctx, params[i], funcDecl);
    }
}

es2panda_AstNode *CreateFunctionDeclaration(const char *name)
{
    auto *funcId = impl->CreateIdentifier(ctx);
    if (!funcId) {
        return nullptr;
    }
    impl->IdentifierSetName(ctx, funcId, const_cast<char *>(name));

    es2panda_AstNode *arrowFunc = nullptr;
    for (const auto &transform : arrowsToTransform) {
        if (transform.funcName == name) {
            arrowFunc = transform.arrowFunc;
            break;
        }
    }
    if (!arrowFunc) {
        return nullptr;
    }

    auto *scriptFunc = impl->ArrowFunctionExpressionFunction(ctx, arrowFunc);
    auto *blockStmt = CreateFunctionBody(scriptFunc);
    auto *signature = CreateFunctionSignature(scriptFunc);
    if (!signature) {
        return nullptr;
    }

    auto *newScriptFunc =
        impl->CreateScriptFunction(ctx, blockStmt, reinterpret_cast<es2panda_FunctionSignature *>(signature), 0, 0);
    if (!newScriptFunc) {
        return nullptr;
    }
    impl->ScriptFunctionSetIdent(ctx, newScriptFunc, funcId);

    auto *funcDecl = impl->CreateFunctionDeclaration(ctx, newScriptFunc, nullptr, 0, false);
    if (!funcDecl || !impl->IsFunctionDeclaration(funcDecl)) {
        return nullptr;
    }

    size_t paramCount = 0;
    auto **params = impl->ScriptFunctionParams(ctx, scriptFunc, &paramCount);
    SetParentRelations(funcDecl, funcId, blockStmt, params, paramCount);

    return funcDecl;
}

void ApplyArrowToFunctionTransformations(es2panda_AstNode *programNode)
{
    if (arrowsToTransform.empty()) {
        return;
    }

    size_t statementsCount = 0;
    auto **statements = impl->BlockStatementStatements(ctx, programNode, &statementsCount);

    size_t newStatementsCount = statementsCount + arrowsToTransform.size();

    auto **newStatements =
        static_cast<es2panda_AstNode **>(impl->AllocMemory(ctx, newStatementsCount, sizeof(es2panda_AstNode *)));
    if (newStatements == nullptr) {
        return;
    }

    size_t funcIndex = 0;
    for (const auto &transform : arrowsToTransform) {
        auto *funcDecl = CreateFunctionDeclaration(transform.funcName.c_str());
        if (funcDecl == nullptr) {
            continue;
        }
        newStatements[funcIndex++] = funcDecl;
    }

    for (size_t i = 0; i < statementsCount; i++) {
        bool skip = false;
        for (const auto &transform : arrowsToTransform) {
            if (statements[i] == transform.varDecl) {
                skip = true;
                break;
            }
        }

        if (!skip) {
            newStatements[funcIndex++] = statements[i];
        }
    }

    for (size_t i = 0; i < funcIndex; i++) {
        impl->AstNodeSetParent(ctx, newStatements[i], programNode);
    }

    impl->BlockStatementSetStatements(ctx, programNode, newStatements, funcIndex);
}

static bool TransformAndValidateCode(es2panda_Context *context, es2panda_AstNode *programNode,
                                     const char **transformedCode)
{
    std::cout << "========================= Source Code =========================" << std::endl;
    std::cout << impl->AstNodeDumpEtsSrcConst(context, programNode) << std::endl;

    arrowsToTransform.clear();
    impl->AstNodeForEach(programNode, FindArrowFunctions, context);
    ApplyArrowToFunctionTransformations(programNode);

    *transformedCode = impl->AstNodeDumpEtsSrcConst(context, programNode);
    std::cout << "========================= Conversion Result =========================" << std::endl;
    std::cout << *transformedCode << std::endl;

    std::string result(*transformedCode);
    if (result.find("function") == std::string::npos) {
        std::cerr << "Error: Function declaration conversion failed." << std::endl;
        return false;
    }
    if (result.find("=>") != std::string::npos) {
        std::cerr << "Error: Arrow function not fully converted." << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    impl = GetImpl();
    std::cout << "LOAD SUCCESS" << std::endl;

    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    auto context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context != nullptr) {
        std::cout << "CREATE CONTEXT SUCCESS" << std::endl;
    }
    ctx = context;
    impl->ProceedToState(ctx, ES2PANDA_STATE_PARSED);
    auto *programNode = impl->ProgramAst(ctx, impl->ContextProgram(ctx));

    const char *transformedCode = nullptr;
    if (!TransformAndValidateCode(ctx, programNode, &transformedCode)) {
        impl->DestroyContext(ctx);
        impl->DestroyConfig(config);
        return 1;
    }

    auto newCtx = impl->CreateContextFromString(config, transformedCode, argv[argc - 1]);
    if (newCtx == nullptr) {
        impl->DestroyContext(ctx);
        impl->DestroyConfig(config);
        return 1;
    }

    impl->ProceedToState(newCtx, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", newCtx);

    impl->ProceedToState(newCtx, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", newCtx);

    impl->AstNodeRecheck(newCtx, impl->ProgramAst(newCtx, impl->ContextProgram(newCtx)));

    impl->ProceedToState(newCtx, ES2PANDA_STATE_LOWERED);
    CheckForErrors("LOWERED", newCtx);

    impl->ProceedToState(newCtx, ES2PANDA_STATE_ASM_GENERATED);
    CheckForErrors("ASM", newCtx);

    impl->ProceedToState(newCtx, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", newCtx);

    impl->DestroyContext(ctx);
    impl->DestroyContext(newCtx);
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
