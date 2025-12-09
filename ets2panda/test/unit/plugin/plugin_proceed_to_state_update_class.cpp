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

#include <asm-generic/errno-base.h>
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <cstring>

#include "libarkbase/os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

/*
 * Covered C API List:
 *   AstNodeDumpEtsSrcConst
 *   AstNodeIterateConst
 *   AstNodeSetParent
 *   ClassDeclarationDefinition
 *   ClassDefinitionAddProperties
 *   ClassDefinitionBody
 *   ClassDefinitionIdent
 *   ClassElementKey
 *   ContextProgram
 *   CreateBlockStatement
 *   CreateClassDefinition
 *   CreateConfig
 *   CreateContextFromString
 *   CreateETSPrimitiveType
 *   CreateFunctionExpression
 *   CreateFunctionSignature
 *   CreateScriptFunction
 *   DestroyConfig
 *   IdentifierName
 *   IsClassDeclaration
 *   ProceedToState
 *   ProgramAst
 *   ScriptFunctionSetIdent
 *   UpdateClassDeclaration
 *   UpdateMethodDefinition
 */

// NOLINTBEGIN
static std::string source = R"(
class A {
    x: int = 10;
    bar() {}
}
)";

static es2panda_Impl *impl = nullptr;
es2panda_Context *context = nullptr;
static es2panda_AstNode *classDecl = nullptr;

void FindClass(es2panda_AstNode *node)
{
    if (!impl->IsClassDeclaration(node)) {
        return;
    }
    es2panda_AstNode *definition = impl->ClassDeclarationDefinition(context, node);
    if (definition == nullptr) {
        return;
    }
    es2panda_AstNode *identifier = impl->ClassDefinitionIdent(context, definition);
    if (identifier == nullptr) {
        return;
    }
    if (strcmp(impl->IdentifierName(context, identifier), "A") == 0) {
        classDecl = node;
    }
}

static es2panda_AstNode *FindBarMethod()
{
    es2panda_AstNode *classDef = impl->ClassDeclarationDefinition(context, classDecl);
    size_t bodySize = 0;
    es2panda_AstNode **body = impl->ClassDefinitionBody(context, classDef, &bodySize);
    for (size_t i = 0; i < bodySize; i++) {
        es2panda_AstNode *methodDef = body[i];
        es2panda_AstNode *key = impl->ClassElementKey(context, methodDef);
        if (strcmp(impl->IdentifierName(context, key), "bar") == 0) {
            return methodDef;
        }
    }
    return nullptr;
}

static es2panda_FunctionSignature *CreateFuncSignature()
{
    es2panda_AstNode *voidType = impl->CreateETSPrimitiveType(context, PRIMITIVE_TYPE_VOID);
    es2panda_FunctionSignature *sig = impl->CreateFunctionSignature(context, nullptr, nullptr, 0, voidType, false);
    return sig;
}

static es2panda_AstNode *CreateScriptFunction()
{
    es2panda_FunctionSignature *sig = CreateFuncSignature();
    es2panda_AstNode *body = impl->CreateBlockStatement(context, nullptr, 0);
    es2panda_AstNode *func = impl->CreateScriptFunction(context, body, sig, 0, 0);
    impl->AstNodeSetParent(context, body, func);

    return func;
}

static es2panda_AstNode *CreateFooMethod()
{
    es2panda_AstNode *name = CreateIdentifierFromString(context, "foo");
    es2panda_AstNode *func = CreateScriptFunction();
    impl->ScriptFunctionSetIdent(context, func, name);
    impl->AstNodeSetParent(context, name, func);

    auto value = impl->CreateFunctionExpression(context, func);
    impl->AstNodeSetParent(context, func, value);

    es2panda_AstNode *barMethodDef = FindBarMethod();

    auto fooMethodDef = impl->UpdateMethodDefinition(context, barMethodDef, METHOD_DEFINITION_KIND_METHOD, name, value,
                                                     MODIFIER_FLAGS_PUBLIC, false);
    impl->AstNodeSetParent(context, name, fooMethodDef);
    impl->AstNodeSetParent(context, value, fooMethodDef);

    return fooMethodDef;
}

static void AddFooMethodToExistsClass()
{
    es2panda_AstNode *classDef = impl->ClassDeclarationDefinition(context, classDecl);
    es2panda_AstNode *fooMethodDef = CreateFooMethod();
    impl->ClassDefinitionAddProperties(context, classDef, &fooMethodDef, 1);
}

static es2panda_AstNode *CreateClassDecl()
{
    // update class declaration
    es2panda_AstNode *fooMethodDef = CreateFooMethod();

    es2panda_AstNode *identifier = CreateIdentifierFromString(context, "B");
    es2panda_AstNode *classDefinition =
        impl->CreateClassDefinition(context, identifier, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &fooMethodDef,
                                    1, CLASS_DEFINITION_MODIFIERS_CLASS_DECL, MODIFIER_FLAGS_FINAL);
    impl->AstNodeSetParent(context, fooMethodDef, classDefinition);
    impl->AstNodeSetParent(context, identifier, classDefinition);
    es2panda_AstNode *newClassDecl = impl->UpdateClassDeclaration(context, classDecl, classDefinition);
    impl->AstNodeSetParent(context, classDefinition, newClassDecl);
    return newClassDecl;
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
    context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);

    es2panda_AstNode *programNode = impl->ProgramAst(context, impl->ContextProgram(context));

    // find class definition
    impl->AstNodeIterateConst(context, programNode, FindClass);

    AddFooMethodToExistsClass();
    es2panda_AstNode *newClassDecl = CreateClassDecl();
    AppendStatementToProgram(context, programNode, newClassDecl);

    // dump src after change
    std::cout << "SRC DUMP AFTER UPDATE CLASS:" << std::endl;
    std::cout << impl->AstNodeDumpEtsSrcConst(context, programNode) << std::endl;

    impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", context);

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    impl->AstNodeRecheck(context, programNode);

    impl->ProceedToState(context, ES2PANDA_STATE_LOWERED);
    CheckForErrors("LOWERED", context);

    impl->ProceedToState(context, ES2PANDA_STATE_ASM_GENERATED);
    CheckForErrors("ASM", context);

    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", context);
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
