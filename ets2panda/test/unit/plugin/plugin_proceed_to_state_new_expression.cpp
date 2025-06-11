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

#include <algorithm>
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
 *   ClassDefinitionIdent
 *   ContextProgram
 *   CreateConfig
 *   CreateContextFromString
 *   CreateETSNewClassInstanceExpression
 *   CreateETSTypeReference
 *   CreateETSTypeReferencePart1
 *   DestroyConfig
 *   IdentifierName
 *   IsClassDeclaration
 *   IsVariableDeclaration
 *   ProceedToState
 *   ProgramAst
 *   VariableDeclarationDeclaratorsConst
 *   VariableDeclaratorSetInit
 */

// NOLINTBEGIN
static std::string source = R"(
class A {
    name: string = "a";
}

class B {
    name: string = "b";
}

let a = new B();
)";
static es2panda_Impl *impl = nullptr;
static es2panda_Context *context = nullptr;
static es2panda_AstNode *classDef = nullptr;
static es2panda_AstNode *variableDeclaration = nullptr;

static void FindClass(es2panda_AstNode *node)
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
        classDef = definition;
    }
}

static void FindVariableDeclaration(es2panda_AstNode *ast)
{
    if (!impl->IsVariableDeclaration(ast)) {
        impl->AstNodeIterateConst(context, ast, FindVariableDeclaration);
        return;
    }
    variableDeclaration = ast;
}

static es2panda_AstNode *CreateNewExpr()
{
    es2panda_AstNode *className = CreateIdentifierFromString(context, "A");
    es2panda_AstNode *part = impl->CreateETSTypeReferencePart1(context, className);
    impl->AstNodeSetParent(context, className, part);
    es2panda_AstNode *typeRef = impl->CreateETSTypeReference(context, part);
    impl->AstNodeSetParent(context, part, typeRef);
    es2panda_AstNode *newExpr = impl->CreateETSNewClassInstanceExpression(context, typeRef, nullptr, 0);
    impl->AstNodeSetParent(context, typeRef, newExpr);
    return newExpr;
}

bool Find(es2panda_AstNode *ast)
{
    if (impl->IsETSTypeReference(ast)) {
        auto part = impl->ETSTypeReferencePart(context, ast);
        auto newTypeReference = impl->UpdateETSTypeReference(context, ast, part);
        auto property = impl->AstNodeParent(context, ast);
        impl->ClassPropertySetTypeAnnotation(context, property, newTypeReference);
        impl->AstNodeSetParent(context, newTypeReference, property);
        impl->AstNodeSetParent(context, part, newTypeReference);
        std::string str(impl->AstNodeDumpEtsSrcConst(context, property));
        if (str.find("public name: string = \"a\"") != std::string::npos) {
            return true;
        }
    }
    return false;
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

    if (!impl->AstNodeIsAnyChildConst(context, programNode, Find)) {
        return TEST_ERROR_CODE;
    }

    impl->AstNodeIterateConst(context, programNode, FindClass);
    impl->AstNodeIterateConst(context, programNode, FindVariableDeclaration);

    es2panda_AstNode *newExpr = CreateNewExpr();

    size_t len = 0;
    es2panda_AstNode **declarators = impl->VariableDeclarationDeclaratorsConst(context, variableDeclaration, &len);
    if (declarators == nullptr) {
        std::cerr << "FAILED TO FIND DECLARATORS" << std::endl;
        return TEST_ERROR_CODE;
    }
    for (size_t i = 0; i < len; i++) {
        es2panda_AstNode *declarator = declarators[i];
        impl->VariableDeclaratorSetInit(context, declarator, newExpr);
        impl->AstNodeSetParent(context, newExpr, declarator);
    }

    std::cout << "PROGRAM NODE: \n" << impl->AstNodeDumpEtsSrcConst(context, programNode) << std::endl;

    impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", context);

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    impl->AstNodeRecheck(context, programNode);

    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", context);
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
