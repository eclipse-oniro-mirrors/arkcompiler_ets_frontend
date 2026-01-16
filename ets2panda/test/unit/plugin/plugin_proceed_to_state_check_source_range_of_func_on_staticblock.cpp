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
static es2panda_Context *g_ctx = nullptr;
static std::string g_source = R"(
class AAAA {
    static num:AAAA = new AAAA()
}
)";

constexpr size_t Cctor_START_LINE = 1;
constexpr size_t Cctor_START_COL = 12;
constexpr size_t Cctor_END_LINE = 1;
constexpr size_t Cctor_END_COL = 12;
static es2panda_AstNode *classCctor = nullptr;

static void FindStaticBlock(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!g_impl->IsClassStaticBlock(ast)) {
        return;
    }
    auto *cctor = g_impl->ClassStaticBlockFunction(g_ctx, ast);
    if (cctor == nullptr) {
        return;
    } else {
        classCctor = cctor;
    }
}

static void FindCctorTest(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!g_impl->IsClassDeclaration(ast)) {
        return;
    }
    auto *ident = g_impl->ClassDefinitionIdent(g_ctx, g_impl->ClassDeclarationDefinition(g_ctx, ast));
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(g_impl->IdentifierName(g_ctx, ident));
    if (name != "AAAA") {
        return;
    }
    auto *classDef = g_impl->ClassDeclarationDefinition(g_ctx, ast);

    g_impl->AstNodeForEach(classDef, FindStaticBlock, g_ctx);
}

static bool CheckCctorSourceRange()
{
    auto start = g_impl->AstNodeStartConst(g_ctx, classCctor);
    auto end = g_impl->AstNodeEndConst(g_ctx, classCctor);

    auto res = Cctor_START_LINE == g_impl->SourcePositionLine(g_ctx, start);
    ASSERT(Cctor_START_LINE == g_impl->SourcePositionLine(g_ctx, start));

    res &= Cctor_START_COL == g_impl->SourcePositionCol(g_ctx, start);
    ASSERT(Cctor_START_COL == g_impl->SourcePositionCol(g_ctx, start));

    res &= Cctor_END_LINE == g_impl->SourcePositionLine(g_ctx, end);
    ASSERT(Cctor_END_LINE == g_impl->SourcePositionLine(g_ctx, end));

    res &= Cctor_END_COL == g_impl->SourcePositionCol(g_ctx, end);
    ASSERT(Cctor_END_COL == g_impl->SourcePositionCol(g_ctx, end));
    return res;
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

    auto *program = g_impl->ContextProgram(g_ctx);
    auto *ast = g_impl->ProgramAst(g_ctx, program);

    g_impl->ProceedToState(g_ctx, ES2PANDA_STATE_CHECKED);
    if (g_impl->ContextState(g_ctx) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }
    g_impl->AstNodeForEach(ast, FindCctorTest, g_ctx);
    if (!CheckCctorSourceRange()) {
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
