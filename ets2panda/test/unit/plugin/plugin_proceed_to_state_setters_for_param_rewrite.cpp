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
#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN
static std::string source = R"(

type FunctionalType = (arg: int) => void

function TestBody() {
    const f: FunctionalType = (arg: int) => {
    }

    f(3)
}
)";

static es2panda_Impl *impl = nullptr;
static es2panda_Config *config = nullptr;
static es2panda_Context *context = nullptr;
static int hasErrors = 0;
static es2panda_AstNode *testBody = nullptr;

void FindTestBody(es2panda_AstNode *node, [[maybe_unused]] void *arg)
{
    if (impl->IsScriptFunction(node)) {
        auto *id = impl->ScriptFunctionId(context, node);
        if (!id) {
            return;
        }
        if (std::string(impl->IdentifierName(context, id)) == "TestBody") {
            testBody = impl->ScriptFunctionBody(context, node);
        }
    }
}

void RewriteETSFunctionTypeParams(es2panda_AstNode *node, [[maybe_unused]] void *arg)
{
    if (impl->IsETSFunctionType(node)) {
        size_t paramCount = 0;

        auto *type1 = impl->CreateETSPrimitiveType(context, Es2pandaPrimitiveType::PRIMITIVE_TYPE_DOUBLE);
        std::string param1Name = "x";
        auto *memForParam1Name = static_cast<char *>(impl->AllocMemory(context, param1Name.size() + 1, 1));
        std::copy_n(param1Name.c_str(), param1Name.size() + 1, memForParam1Name);
        auto *ident1 = impl->CreateIdentifier2(context, memForParam1Name, type1);
        auto *param1 = impl->CreateETSParameterExpression(context, ident1, false);
        paramCount++;

        auto *type2 = impl->CreateETSPrimitiveType(context, Es2pandaPrimitiveType::PRIMITIVE_TYPE_BOOLEAN);
        std::string param2Name = "y";
        auto *memForParam2Name = static_cast<char *>(impl->AllocMemory(context, param2Name.size() + 1, 1));
        std::copy_n(param2Name.c_str(), param2Name.size() + 1, memForParam2Name);
        auto *ident2 = impl->CreateIdentifier2(context, memForParam2Name, type2);
        auto *param2 = impl->CreateETSParameterExpression(context, ident2, false);
        paramCount++;

        es2panda_AstNode *paramList[] = {param1, param2};
        impl->ETSFunctionTypeIrSetParams(context, node, paramList, paramCount);

        if (impl->AstNodeParent(context, param1) != node) {
            hasErrors = 1;
        }
        if (impl->AstNodeParent(context, param2) != node) {
            hasErrors = 1;
        }
    }
}

void RewriteScriptFunctionParams(es2panda_AstNode *node, [[maybe_unused]] void *arg)
{
    if (impl->IsScriptFunction(node)) {
        size_t paramCount = 0;

        auto *type1 = impl->CreateETSPrimitiveType(context, Es2pandaPrimitiveType::PRIMITIVE_TYPE_DOUBLE);
        std::string param1Name = "x";
        auto *memForParam1Name = static_cast<char *>(impl->AllocMemory(context, param1Name.size() + 1, 1));
        std::copy_n(param1Name.c_str(), param1Name.size() + 1, memForParam1Name);
        auto *ident1 = impl->CreateIdentifier2(context, memForParam1Name, type1);
        auto *param1 = impl->CreateETSParameterExpression(context, ident1, false);
        paramCount++;

        auto *type2 = impl->CreateETSPrimitiveType(context, Es2pandaPrimitiveType::PRIMITIVE_TYPE_BOOLEAN);
        std::string param2Name = "y";
        auto *memForParam2Name = static_cast<char *>(impl->AllocMemory(context, param2Name.size() + 1, 1));
        std::copy_n(param2Name.c_str(), param2Name.size() + 1, memForParam2Name);
        auto *ident2 = impl->CreateIdentifier2(context, memForParam2Name, type2);
        auto *param2 = impl->CreateETSParameterExpression(context, ident2, false);
        paramCount++;

        es2panda_AstNode *paramList[] = {param1, param2};
        impl->ScriptFunctionSetParams(context, node, paramList, paramCount);

        if (impl->AstNodeParent(context, param1) != node) {
            hasErrors = 1;
        }
        if (impl->AstNodeParent(context, param2) != node) {
            hasErrors = 1;
        }
    }
}

void RewriteCallExpressionArguments(es2panda_AstNode *node, [[maybe_unused]] void *arg)
{
    if (impl->IsCallExpression(node)) {
        size_t argCount = 0;

        auto *arg1 = impl->CreateNumberLiteral2(context, 3.14f);
        argCount++;

        auto *arg2 = impl->CreateBooleanLiteral(context, true);
        argCount++;

        es2panda_AstNode *argList[] = {arg1, arg2};
        impl->CallExpressionSetArguments(context, node, argList, argCount);

        if (impl->AstNodeParent(context, arg1) != node) {
            hasErrors = 1;
        }
        if (impl->AstNodeParent(context, arg2) != node) {
            hasErrors = 1;
        }
    }
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

    const char **args = const_cast<const char **>(&(argv[1]));
    config = impl->CreateConfig(argc - 1, args);
    context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    auto *program = impl->ContextProgram(context);
    es2panda_AstNode *ast = impl->ProgramAst(context, program);

    impl->AstNodeForEach(ast, FindTestBody, nullptr);
    if (testBody == nullptr) {
        return PROCEED_ERROR_CODE;
    }

    impl->AstNodeForEach(ast, RewriteETSFunctionTypeParams, nullptr);
    impl->AstNodeForEach(testBody, RewriteScriptFunctionParams, nullptr);
    impl->AstNodeForEach(testBody, RewriteCallExpressionArguments, nullptr);
    if (hasErrors) {
        return PROCEED_ERROR_CODE;
    }

    impl->AstNodeRecheck(context, ast);
    CheckForErrors("RECHECKED", context);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", context);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    impl->DestroyContext(context);
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND