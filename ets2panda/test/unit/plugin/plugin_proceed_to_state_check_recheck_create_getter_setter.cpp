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
#include <cstring>
#include <string>
#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN
static std::string source = R"(
type Xxx = number
class AA {
  CCC: Xxx
}
)";
static const std::string Value = "value";
static const std::string scriptName = "Xxx";

static es2panda_Impl *impl = nullptr;
es2panda_Context *context = nullptr;
es2panda_AstNode *propA = nullptr;
es2panda_AstNode *propB = nullptr;
es2panda_AstNode *funcCall = nullptr;
es2panda_AstNode *lambdaExpression = nullptr;
es2panda_AstNode *scriptFuncOfLambda = nullptr;

static char *createIdent(std::string identName)
{
    auto *memForIdent = static_cast<char *>(impl->AllocMemory(context, identName.size() + 1, 1));
    std::copy_n(identName.c_str(), identName.size() + 1, memForIdent);
    return memForIdent;
}

static void AddGetterSetterToBody(es2panda_AstNode *ast, [[maybe_unused]] void *ctx)
{
    if (!impl->IsClassProperty(ast)) {
        return;
    }
    auto classDef = impl->AstNodeParent(context, ast);

    // Create Script function
    auto clonedTypeAnno1 = impl->AstNodeClone(context, impl->ClassPropertyTypeAnnotationConst(context, ast), nullptr);

    auto paramIdent = impl->CreateIdentifier2(context, createIdent(Value), clonedTypeAnno1);
    impl->AstNodeSetParent(context, clonedTypeAnno1, paramIdent);
    auto parameterExpr = impl->CreateETSParameterExpression(context, paramIdent, false);
    impl->AstNodeSetParent(context, paramIdent, parameterExpr);

    es2panda_AstNode *params[1];
    params[0] = parameterExpr;
    auto scriptFuncFlags = SCRIPT_FUNCTION_FLAGS_METHOD | SCRIPT_FUNCTION_FLAGS_SETTER;
    auto modifierFlags = MODIFIER_FLAGS_PUBLIC;

    auto body = impl->CreateBlockStatement(context, nullptr, 0);

    auto scriptFunc = impl->CreateScriptFunction(
        context, body, impl->CreateFunctionSignature(context, nullptr, params, 1, nullptr, false), scriptFuncFlags,
        modifierFlags);

    impl->AstNodeSetParent(context, body, scriptFunc);
    auto scriptFuncName = impl->CreateIdentifier1(context, createIdent(scriptName));
    impl->ScriptFunctionSetIdent(context, scriptFunc, scriptFuncName);

    // Create Method Definition
    auto methodDefIdent = impl->CreateIdentifier1(context, createIdent(scriptName));
    auto funcExprIdent = impl->CreateIdentifier1(context, createIdent(scriptName));
    auto funcExpr = impl->CreateFunctionExpression1(context, funcExprIdent, scriptFunc);
    auto methodDef = impl->CreateMethodDefinition(context, METHOD_DEFINITION_KIND_SET, methodDefIdent, funcExpr,
                                                  MODIFIER_FLAGS_PUBLIC, false);
    impl->AstNodeSetParent(context, scriptFunc, funcExpr);
    impl->AstNodeSetParent(context, funcExpr, methodDef);

    impl->ClassDefinitionEmplaceBody(context, classDef, methodDef);
    impl->AstNodeSetParent(context, methodDef, classDef);
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
    context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }
    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    auto *program = impl->ContextProgram(context);
    es2panda_AstNode *programNode = impl->ProgramAst(context, program);

    impl->AstNodeForEach(programNode, AddGetterSetterToBody, context);
    impl->AstNodeRecheck(context, programNode);
    CheckForErrors("RECHECKED", context);

    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    impl->DestroyConfig(config);
    return 0;
}

// NOLINTEND
