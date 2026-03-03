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

#include <cstddef>
#include <cstring>
#include <algorithm>
#include <string>
#include "util.h"

// NOLINTBEGIN

static std::string strInit {""};

static es2panda_Impl *g_impl = nullptr;
static es2panda_Context *g_ctx = nullptr;
static std::string g_source = R"(
let a = new number[5](0)
let b = new number[5][5]
let c = new number[5](0)
let d = new number[5][5]
let e = new number[5][5]
let f = new number[5][5]
)";
static int g_count = 6;

void FindA(es2panda_AstNode *ast, es2panda_AstNode *declarator, char *name)
{
    if (g_impl->IsETSNewArrayInstanceExpression(ast) && strcmp(name, "a") == 0) {
        auto className = CreateIdentifierFromString(g_ctx, "string");
        auto part = g_impl->CreateETSTypeReferencePart1(g_ctx, className);
        auto typeReference = g_impl->CreateETSTypeReference(g_ctx, part);
        g_impl->AstNodeSetParent(g_ctx, part, typeReference);
        g_impl->AstNodeSetParent(g_ctx, className, part);

        auto dimension = g_impl->ETSNewArrayInstanceExpressionDimension(g_ctx, ast);
        auto initializer = g_impl->CreateStringLiteral1(g_ctx, strInit.data());
        auto expression = g_impl->CreateETSNewArrayInstanceExpression(g_ctx, typeReference, dimension, initializer);
        g_impl->VariableDeclaratorSetInit(g_ctx, declarator, expression);
        g_impl->AstNodeSetParent(g_ctx, expression, declarator);
        g_impl->AstNodeSetParent(g_ctx, dimension, expression);
        g_impl->AstNodeSetParent(g_ctx, typeReference, expression);
        auto str = g_impl->AstNodeDumpEtsSrcConst(g_ctx, declarator);
        if (strcmp(str, "a = new string[5](\"\")") == 0) {
            g_count--;
        }
    }
}

void FindB(es2panda_AstNode *ast, es2panda_AstNode *declarator, char *name)
{
    if (g_impl->IsETSNewMultiDimArrayInstanceExpression(ast) && strcmp(name, "b") == 0) {
        auto className = CreateIdentifierFromString(g_ctx, "string");
        auto part = g_impl->CreateETSTypeReferencePart1(g_ctx, className);
        auto typeReference = g_impl->CreateETSTypeReference(g_ctx, part);
        g_impl->AstNodeSetParent(g_ctx, part, typeReference);
        g_impl->AstNodeSetParent(g_ctx, className, part);

        size_t dimensionsLen = 0;
        auto dimensions = g_impl->ETSNewMultiDimArrayInstanceExpressionDimensions(g_ctx, ast, &dimensionsLen);
        auto expression =
            g_impl->CreateETSNewMultiDimArrayInstanceExpression(g_ctx, typeReference, dimensions, dimensionsLen);
        g_impl->VariableDeclaratorSetInit(g_ctx, declarator, expression);
        g_impl->AstNodeSetParent(g_ctx, expression, declarator);
        for (size_t i = 0; i < dimensionsLen; i++) {
            g_impl->AstNodeSetParent(g_ctx, dimensions[i], expression);
        }
        g_impl->AstNodeSetParent(g_ctx, typeReference, expression);
        auto str = g_impl->AstNodeDumpEtsSrcConst(g_ctx, declarator);
        if (strcmp(str, "b = new string[5][5]") == 0) {
            g_count--;
        }
    }
}

void FindC(es2panda_AstNode *ast, es2panda_AstNode *declarator, char *name)
{
    if (g_impl->IsETSNewArrayInstanceExpression(ast) && strcmp(name, "c") == 0) {
        auto className = CreateIdentifierFromString(g_ctx, "string");
        auto part = g_impl->CreateETSTypeReferencePart1(g_ctx, className);
        auto typeReference = g_impl->CreateETSTypeReference(g_ctx, part);
        g_impl->AstNodeSetParent(g_ctx, part, typeReference);
        g_impl->AstNodeSetParent(g_ctx, className, part);

        auto dimension = g_impl->ETSNewArrayInstanceExpressionDimension(g_ctx, ast);
        auto initializer = g_impl->CreateStringLiteral1(g_ctx, strInit.data());
        auto expression =
            g_impl->UpdateETSNewArrayInstanceExpression(g_ctx, ast, typeReference, dimension, initializer);
        g_impl->VariableDeclaratorSetInit(g_ctx, declarator, expression);
        g_impl->AstNodeSetParent(g_ctx, expression, declarator);
        g_impl->AstNodeSetParent(g_ctx, dimension, expression);
        g_impl->AstNodeSetParent(g_ctx, typeReference, expression);
        auto str = g_impl->AstNodeDumpEtsSrcConst(g_ctx, declarator);
        if (strcmp(str, "c = new string[5](\"\")") == 0) {
            g_count--;
        }
    }
}

void FindD(es2panda_AstNode *ast, es2panda_AstNode *declarator, char *name)
{
    if (g_impl->IsETSNewMultiDimArrayInstanceExpression(ast) && strcmp(name, "d") == 0) {
        auto className = CreateIdentifierFromString(g_ctx, "string");
        auto part = g_impl->CreateETSTypeReferencePart1(g_ctx, className);
        auto typeReference = g_impl->CreateETSTypeReference(g_ctx, part);
        g_impl->AstNodeSetParent(g_ctx, part, typeReference);
        g_impl->AstNodeSetParent(g_ctx, className, part);

        size_t dimensionsLen = 0;
        auto dimensions = g_impl->ETSNewMultiDimArrayInstanceExpressionDimensions(g_ctx, ast, &dimensionsLen);
        auto expression =
            g_impl->UpdateETSNewMultiDimArrayInstanceExpression(g_ctx, ast, typeReference, dimensions, dimensionsLen);
        g_impl->VariableDeclaratorSetInit(g_ctx, declarator, expression);
        g_impl->AstNodeSetParent(g_ctx, expression, declarator);
        for (size_t i = 0; i < dimensionsLen; i++) {
            g_impl->AstNodeSetParent(g_ctx, dimensions[i], expression);
        }
        g_impl->AstNodeSetParent(g_ctx, typeReference, expression);
        auto str = g_impl->AstNodeDumpEtsSrcConst(g_ctx, declarator);
        if (strcmp(str, "d = new string[5][5]") == 0) {
            g_count--;
        }
    }
}

void FindE(es2panda_AstNode *ast, es2panda_AstNode *declarator, char *name)
{
    if (g_impl->IsETSNewMultiDimArrayInstanceExpression(ast) && strcmp(name, "e") == 0) {
        auto typeReference = g_impl->ETSNewMultiDimArrayInstanceExpressionTypeReference(g_ctx, ast);
        size_t dimensionsLen = 0;
        auto dimensions = g_impl->ETSNewMultiDimArrayInstanceExpressionDimensions(g_ctx, ast, &dimensionsLen);
        auto expression = g_impl->CreateETSNewMultiDimArrayInstanceExpression1(g_ctx, ast);
        g_impl->VariableDeclaratorSetInit(g_ctx, declarator, expression);
        g_impl->AstNodeSetParent(g_ctx, expression, declarator);
        for (size_t i = 0; i < dimensionsLen; i++) {
            g_impl->AstNodeSetParent(g_ctx, dimensions[i], expression);
        }
        g_impl->AstNodeSetParent(g_ctx, typeReference, expression);
        auto str = g_impl->AstNodeDumpEtsSrcConst(g_ctx, declarator);
        if (strcmp(str, "e = new number[5][5]") == 0) {
            g_count--;
        }
    }
}

void FindF(es2panda_AstNode *ast, es2panda_AstNode *declarator, char *name)
{
    if (g_impl->IsETSNewMultiDimArrayInstanceExpression(ast) && strcmp(name, "f") == 0) {
        auto typeReference = g_impl->ETSNewMultiDimArrayInstanceExpressionTypeReference(g_ctx, ast);
        size_t dimensionsLen = 0;
        auto dimensions = g_impl->ETSNewMultiDimArrayInstanceExpressionDimensions(g_ctx, ast, &dimensionsLen);
        auto expression = g_impl->UpdateETSNewMultiDimArrayInstanceExpression1(g_ctx, ast, ast);
        g_impl->VariableDeclaratorSetInit(g_ctx, declarator, expression);
        g_impl->AstNodeSetParent(g_ctx, expression, declarator);
        for (size_t i = 0; i < dimensionsLen; i++) {
            g_impl->AstNodeSetParent(g_ctx, dimensions[i], expression);
        }
        g_impl->AstNodeSetParent(g_ctx, typeReference, expression);
        auto str = g_impl->AstNodeDumpEtsSrcConst(g_ctx, declarator);
        if (strcmp(str, "f = new number[5][5]") == 0) {
            g_count--;
        }
    }
}

bool Find(es2panda_AstNode *ast)
{
    auto declarator = g_impl->AstNodeParent(g_ctx, ast);
    if (g_impl->IsVariableDeclarator(declarator)) {
        auto name = g_impl->IdentifierName(g_ctx, g_impl->VariableDeclaratorId(g_ctx, declarator));
        FindA(ast, declarator, name);
        FindB(ast, declarator, name);
        FindC(ast, declarator, name);
        FindD(ast, declarator, name);
        FindE(ast, declarator, name);
        FindF(ast, declarator, name);
    }
    return g_count == 0;
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
    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = g_impl->CreateConfig(argc - 1, args);
    g_ctx = g_impl->CreateContextFromString(config, g_source.data(), argv[argc - 1]);
    if (g_ctx == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    g_impl->ProceedToState(g_ctx, ES2PANDA_STATE_PARSED);
    if (g_impl->ContextState(g_ctx) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    auto ast = g_impl->ProgramAst(g_ctx, g_impl->ContextProgram(g_ctx));
    if (!g_impl->AstNodeIsAnyChildConst(g_ctx, ast, Find)) {
        return TEST_ERROR_CODE;
    }

    g_impl->ProceedToState(g_ctx, ES2PANDA_STATE_BIN_GENERATED);
    if (g_impl->ContextState(g_ctx) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    g_impl->DestroyContext(g_ctx);
    g_impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
