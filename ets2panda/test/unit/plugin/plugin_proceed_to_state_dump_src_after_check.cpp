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

#include "libarkbase/os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

static std::string g_source = R"(
interface A {
  xsx : ()=>void;
}
)";

static es2panda_AstNode *mySetter = nullptr;
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
    if (name == "xsx") {
        mySetter = ast;
    }
}

static es2panda_AstNode *myParam = nullptr;
static void FindParamExpr(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!impl->IsETSParameterExpression(ast)) {
        return;
    }
    auto *ident = impl->ETSParameterExpressionIdent(ctx, ast);
    if (ident == nullptr) {
        return;
    }
    auto name = std::string(impl->IdentifierName(ctx, ident));
    if (name == "xsx") {
        myParam = ast;
    }
}

static es2panda_Context *context = nullptr;
static es2panda_AstNode *newAnno = nullptr;
void RewriteScriptFunctionParams(es2panda_AstNode *node, [[maybe_unused]] void *arg)
{
    if (impl->IsScriptFunction(node)) {
        auto *type1 = impl->ETSParameterExpressionTypeAnnotation(context, myParam);
        auto signature = impl->CreateFunctionSignature(
            context, nullptr, nullptr, 0, impl->CreateETSPrimitiveType(context, PRIMITIVE_TYPE_DOUBLE), false);
        newAnno = impl->UpdateETSFunctionTypeIr(context, type1, signature, SCRIPT_FUNCTION_FLAGS_NONE);
        impl->ETSParameterExpressionSetTypeAnnotation(context, myParam, newAnno);
        es2panda_AstNode *paramList[] = {myParam};
        impl->ScriptFunctionSetParams(context, node, paramList, 1);
    }
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    impl = GetImpl();
    if (impl == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }

    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    context = impl->CreateContextFromString(config, g_source.data(), argv[argc - 1]);
    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    auto *program = impl->ContextProgram(context);
    auto *entryAst = impl->ProgramAst(context, program);
    impl->AstNodeForEach(entryAst, FindMethodDef, context);
    impl->AstNodeForEach(mySetter, FindParamExpr, context);
    std::cout << impl->AstNodeDumpEtsSrcConst(context, mySetter) << std::endl;
    impl->AstNodeForEach(mySetter, RewriteScriptFunctionParams, nullptr);
    std::cout << impl->AstNodeDumpEtsSrcConst(context, newAnno) << std::endl;
    std::cout << impl->AstNodeDumpEtsSrcConst(context, mySetter) << std::endl;
    return 0;
}

// NOLINTEND
