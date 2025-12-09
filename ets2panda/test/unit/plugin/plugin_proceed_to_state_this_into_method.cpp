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
#include <string_view>

#include "libarkbase/os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

/*
 * Covered C API List:
 *   AllocMemory
 *   AstNodeDumpEtsSrcConst
 *   AstNodeIterateConst
 *   AstNodeSetParent
 *   BlockStatementSetStatements
 *   ClassElementKey
 *   ClassElementValue
 *   ContextProgram
 *   CreateAssignmentExpression
 *   CreateBlockStatement
 *   CreateBooleanLiteral
 *   CreateConfig
 *   CreateContextFromString
 *   CreateExpressionStatement
 *   CreateIfStatement
 *   CreateMemberExpression
 *   CreateNumberLiteral
 *   CreateThisExpression
 *   DestroyConfig
 *   FunctionExpressionFunction
 *   IdentifierName
 *   IsMethodDefinition
 *   ProceedToState
 *   ProgramAst
 *   ScriptFunctionBody
 *   UpdateMethodDefinition
 */

// NOLINTBEGIN
static std::string source = R"(
class A {
    x: int = 1;
    public foo() {
    }
}
let a = new A();
a.foo();
arktest.assertEQ(a.x, 2);
)";

static es2panda_Impl *impl = nullptr;
static es2panda_Context *context = nullptr;
static es2panda_AstNode *fooMethodDefinition = nullptr;

static es2panda_AstNode *CreateThisExpression()
{
    es2panda_AstNode *thisExpr = impl->CreateThisExpression(context);
    return thisExpr;
}

static es2panda_AstNode *CreateMemberExpression()
{
    es2panda_AstNode *thisExpr = CreateThisExpression();
    es2panda_AstNode *memberName = CreateIdentifierFromString(context, "x");
    es2panda_AstNode *memberExpr = impl->CreateMemberExpression(context, thisExpr, memberName,
                                                                MEMBER_EXPRESSION_KIND_PROPERTY_ACCESS, false, false);
    impl->AstNodeSetParent(context, thisExpr, memberExpr);
    impl->AstNodeSetParent(context, memberName, memberExpr);
    return memberExpr;
}

static es2panda_AstNode *CreateAssignmentExpression()
{
    es2panda_AstNode *memberExpr = CreateMemberExpression();
    es2panda_AstNode *num2 = impl->CreateNumberLiteral(context, 2);

    es2panda_AstNode *assignmentExpr =
        impl->CreateAssignmentExpression(context, memberExpr, num2, TOKEN_TYPE_PUNCTUATOR_SUBSTITUTION);

    impl->AstNodeSetParent(context, memberExpr, assignmentExpr);
    impl->AstNodeSetParent(context, num2, assignmentExpr);

    return assignmentExpr;
}

static es2panda_AstNode *CreateAssignmentStatement()
{
    es2panda_AstNode *assignExpr = CreateAssignmentExpression();
    es2panda_AstNode *assignStat = impl->CreateExpressionStatement(context, assignExpr);
    impl->AstNodeSetParent(context, assignExpr, assignStat);
    return assignStat;
}

static es2panda_AstNode *CreateBlockStatement()
{
    es2panda_AstNode *assignStat = CreateAssignmentStatement();
    es2panda_AstNode **newStatements =
        static_cast<es2panda_AstNode **>(impl->AllocMemory(context, 1, sizeof(es2panda_AstNode *)));
    newStatements[0] = assignStat;
    es2panda_AstNode *blockStmt = impl->CreateBlockStatement(context, newStatements, 1);
    impl->AstNodeSetParent(context, assignStat, blockStmt);
    return blockStmt;
}

static es2panda_AstNode *CreateIfStatement()
{
    es2panda_AstNode *boolLit = impl->CreateBooleanLiteral(context, true);
    es2panda_AstNode *blockStmt = CreateBlockStatement();
    es2panda_AstNode *ifStmt = impl->CreateIfStatement(context, boolLit, blockStmt, nullptr);

    impl->AstNodeSetParent(context, boolLit, ifStmt);
    impl->AstNodeSetParent(context, blockStmt, ifStmt);
    return ifStmt;
}

static void AddThisToMethod()
{
    es2panda_AstNode *ifStat = CreateIfStatement();

    es2panda_AstNode *methodValue = impl->ClassElementValue(context, fooMethodDefinition);
    es2panda_AstNode *func = impl->FunctionExpressionFunction(context, methodValue);
    es2panda_AstNode *body = impl->ScriptFunctionBody(context, func);
    es2panda_AstNode **newStatements =
        static_cast<es2panda_AstNode **>(impl->AllocMemory(context, 1, sizeof(es2panda_AstNode *)));
    newStatements[0] = ifStat;
    impl->BlockStatementSetStatements(context, body, newStatements, 1);
    impl->AstNodeSetParent(context, ifStat, body);
}

static void FindMethod(es2panda_AstNode *node)
{
    if (!impl->IsMethodDefinition(node)) {
        impl->AstNodeIterateConst(context, node, FindMethod);
        return;
    }
    es2panda_AstNode *methodKey = impl->ClassElementKey(context, node);
    auto name = impl->IdentifierName(context, methodKey);
    if (strcmp(name, "foo") != 0) {
        impl->AstNodeIterateConst(context, node, FindMethod);
        return;
    }
    fooMethodDefinition = node;
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

    FindMethod(programNode);

    AddThisToMethod();

    std::cout << "PROGRAM NODE: \n" << impl->AstNodeDumpEtsSrcConst(context, programNode) << std::endl;

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
