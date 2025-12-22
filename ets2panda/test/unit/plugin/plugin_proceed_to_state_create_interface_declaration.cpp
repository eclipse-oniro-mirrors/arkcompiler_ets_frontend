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

#include "libarkbase/os/library_loader.h"
#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;
static es2panda_Context *ctx = nullptr;

static std::string source = R"(
class Demo {
    run(arg: string): void {}
}
)";

static es2panda_AstNode *CreateMethodNode(es2panda_Context *context, const char *funcName)
{
    es2panda_AstNode *key = impl->CreateIdentifier1(context, const_cast<char *>(funcName));
    es2panda_AstNode *body = impl->CreateBlockStatement(context, nullptr, 0);
    auto *scriptFuncId = impl->CreateIdentifier1(context, const_cast<char *>(funcName));

    es2panda_AstNode *scriptFunction = impl->CreateScriptFunction(
        context, body,
        impl->CreateFunctionSignature(context, nullptr, nullptr, 0,
                                      impl->CreateETSPrimitiveType(context, PRIMITIVE_TYPE_VOID), false),
        SCRIPT_FUNCTION_FLAGS_NONE, MODIFIER_FLAGS_NONE);

    impl->ScriptFunctionSetIdent(context, scriptFunction, scriptFuncId);
    impl->AstNodeSetParent(context, body, scriptFunction);
    impl->AstNodeSetParent(context, scriptFuncId, scriptFunction);

    es2panda_AstNode *value = impl->CreateFunctionExpression(context, scriptFunction);
    impl->AstNodeSetParent(context, scriptFunction, value);

    es2panda_AstNode *func =
        impl->CreateMethodDefinition(context, METHOD_DEFINITION_KIND_METHOD, key, value, MODIFIER_FLAGS_NONE, false);
    impl->AstNodeSetParent(context, key, func);
    impl->AstNodeSetParent(context, value, func);
    return func;
}

static es2panda_AstNode *CreateInterfaceNode(es2panda_Context *context, const char *interfaceName,
                                             es2panda_AstNode *method)
{
    auto *interfaceId = impl->CreateIdentifier1(context, const_cast<char *>(interfaceName));
    es2panda_AstNode **interfaceMembers =
        static_cast<es2panda_AstNode **>(impl->AllocMemory(context, 1, sizeof(es2panda_AstNode *)));
    interfaceMembers[0] = method;

    es2panda_AstNode *interfaceBody = impl->CreateTSInterfaceBody(context, interfaceMembers, 1);
    impl->AstNodeSetParent(context, method, interfaceBody);

    es2panda_AstNode *interfaceDecl =
        impl->CreateTSInterfaceDeclaration(context, nullptr, 0, interfaceId, nullptr, interfaceBody, false, false);
    impl->AstNodeSetParent(context, interfaceBody, interfaceDecl);
    impl->AstNodeSetParent(context, interfaceId, interfaceDecl);
    return interfaceDecl;
}

static bool CreateInterfaceAndModifyAST(es2panda_Context *context, es2panda_AstNode *ast)
{
    std::cout << "========================= Source Code =========================" << std::endl;
    std::cout << impl->AstNodeDumpEtsSrcConst(context, ast) << std::endl << std::endl;

    es2panda_AstNode *method = CreateMethodNode(context, "inter");
    if (method == nullptr)
        return false;

    es2panda_AstNode *interfaceDecl = CreateInterfaceNode(context, "MyInterface", method);
    AppendStatementToProgram(context, ast, interfaceDecl);

    std::cout << "========================= Conversion Result =========================" << std::endl;
    std::cout << impl->AstNodeDumpEtsSrcConst(context, ast) << std::endl;
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

    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);

    auto context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    ctx = context;

    impl->ProceedToState(ctx, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", ctx);

    auto program = impl->ContextProgram(context);
    auto ast = impl->ProgramAst(context, program);
    if (!CreateInterfaceAndModifyAST(ctx, ast)) {
        std::cout << "Interface creation failure." << std::endl;
        impl->DestroyContext(ctx);
        impl->DestroyConfig(config);
        return 1;
    }

    impl->ProceedToState(ctx, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", ctx);

    impl->ProceedToState(ctx, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", ctx);

    impl->AstNodeRecheck(context, ast);

    impl->ProceedToState(ctx, ES2PANDA_STATE_LOWERED);
    CheckForErrors("LOWERED", ctx);

    impl->ProceedToState(ctx, ES2PANDA_STATE_ASM_GENERATED);
    CheckForErrors("ASM", ctx);

    impl->ProceedToState(ctx, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", ctx);

    impl->DestroyContext(ctx);
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
