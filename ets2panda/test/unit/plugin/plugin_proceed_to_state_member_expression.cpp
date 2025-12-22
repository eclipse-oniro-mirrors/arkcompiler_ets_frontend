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
 *   CreateContextFromFile
 *   CreateContextFromString
 *   CreateExpressionStatement
 *   CreateMemberExpression
 *   DestroyConfig
 *   IdentifierName
 *   IsClassDeclaration
 *   IsMemberExpression
 *   MemberExpressionPropVar
 *   ProceedToState
 *   ProgramAst
 *   UpdateMemberExpression
 *   VariableNameConst
 */

// NOLINTBEGIN
static std::string source = R"(
class A {
    name: string = "xtyi";
    age: int = 18;
    address: string = "china";
}
let a = new A();
a.name;
)";
static es2panda_Impl *impl = nullptr;
static es2panda_AstNode *classDef = nullptr;
static es2panda_AstNode *memberExpression = nullptr;
static es2panda_Context *context = nullptr;

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

static void FindMemberExpression(es2panda_AstNode *ast)
{
    if (!impl->IsMemberExpression(ast)) {
        impl->AstNodeIterateConst(context, ast, FindMemberExpression);
        return;
    }
    memberExpression = ast;
}

static es2panda_AstNode *CreateMemberExpression()
{
    es2panda_AstNode *instanceName = CreateIdentifierFromString(context, "a");
    es2panda_AstNode *memberName = CreateIdentifierFromString(context, "age");
    es2panda_AstNode *memberExpr = impl->CreateMemberExpression(context, instanceName, memberName,
                                                                MEMBER_EXPRESSION_KIND_PROPERTY_ACCESS, false, false);
    impl->AstNodeSetParent(context, instanceName, memberExpr);
    impl->AstNodeSetParent(context, memberName, memberExpr);
    return memberExpr;
}

static void CheckCreateMemberExpression(es2panda_AstNode *programNode)
{
    es2panda_AstNode *ageMemberExpr = CreateMemberExpression();
    es2panda_AstNode *ageMemberStat = impl->CreateExpressionStatement(context, ageMemberExpr);
    impl->AstNodeSetParent(context, ageMemberExpr, ageMemberStat);
    AppendStatementToProgram(context, programNode, ageMemberStat);
}

static int CheckMemberPropVar()
{
    es2panda_Variable *prop = impl->MemberExpressionPropVar(context, memberExpression);
    auto name = impl->VariableNameConst(context, prop);
    if (strcmp(name, "name") != 0) {
        return TEST_ERROR_CODE;
    }
    return 0;
}

static void CheckUpdateMemberExpression(es2panda_AstNode *programNode)
{
    es2panda_AstNode *instanceName = CreateIdentifierFromString(context, "a");
    es2panda_AstNode *memberName = CreateIdentifierFromString(context, "address");
    es2panda_AstNode *newMemberExpr = impl->UpdateMemberExpression(
        context, memberExpression, instanceName, memberName, MEMBER_EXPRESSION_KIND_PROPERTY_ACCESS, false, false);
    impl->AstNodeSetParent(context, instanceName, newMemberExpr);
    impl->AstNodeSetParent(context, memberName, newMemberExpr);
    es2panda_AstNode *newMemberStat = impl->CreateExpressionStatement(context, newMemberExpr);
    impl->AstNodeSetParent(context, newMemberExpr, newMemberStat);
    AppendStatementToProgram(context, programNode, newMemberStat);
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
    impl->AstNodeIterateConst(context, programNode, FindMemberExpression);

    CheckCreateMemberExpression(programNode);
    CheckUpdateMemberExpression(programNode);

    std::cout << "PROGRAM NODE: \n" << impl->AstNodeDumpEtsSrcConst(context, programNode) << std::endl;

    impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", context);

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    int code = CheckMemberPropVar();
    if (code != 0) {
        std::cerr << "FAILED TO CHECK MEMBER PROP VAR" << std::endl;
        return code;
    }

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
