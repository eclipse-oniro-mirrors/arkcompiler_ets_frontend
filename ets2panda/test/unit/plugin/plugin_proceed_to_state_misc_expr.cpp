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
 *   AllocMemory
 *   AstNodeDumpEtsSrcConst
 *   AstNodeSetParent
 *   ContextProgram
 *   CreateAssignmentExpression
 *   CreateBinaryExpression
 *   CreateCallExpression
 *   CreateConfig
 *   CreateContextFromFile
 *   CreateContextFromString
 *   CreateExpressionStatement
 *   CreateNumberLiteral
 *   CreateUnaryExpression
 *   DestroyConfig
 *   IsAssignmentExpression
 *   IsExpressionStatement
 *   ProceedToState
 *   ProgramAst
 */

// NOLINTBEGIN
static std::string source = R"(
function add(a: number, b: number) {
    return a + b;
}
let x = 1;
)";

static es2panda_Impl *impl = nullptr;
static es2panda_Context *context = nullptr;

static void CheckFuncCall(es2panda_AstNode *programNode)
{
    es2panda_AstNode *callee = CreateIdentifierFromString(context, "add");
    es2panda_AstNode *arg1 = impl->CreateNumberLiteral(context, 1);
    es2panda_AstNode *arg2 = impl->CreateNumberLiteral(context, 2);
    es2panda_AstNode **callArgs =
        static_cast<es2panda_AstNode **>(impl->AllocMemory(context, 2, sizeof(es2panda_AstNode *)));
    callArgs[0] = arg1;
    callArgs[1] = arg2;
    es2panda_AstNode *callExpr = impl->CreateCallExpression(context, callee, callArgs, 2, nullptr, false, false);
    es2panda_AstNode *callStat = impl->CreateExpressionStatement(context, callExpr);
    impl->AstNodeSetParent(context, callee, callExpr);
    impl->AstNodeSetParent(context, arg1, callExpr);
    impl->AstNodeSetParent(context, arg2, callExpr);
    impl->AstNodeSetParent(context, callExpr, callStat);

    AppendStatementToProgram(context, programNode, callStat);
}

static void CheckAssignmentExpression(es2panda_AstNode *programNode)
{
    es2panda_AstNode *identifier = CreateIdentifierFromString(context, "x");
    es2panda_AstNode *numberLiteral = impl->CreateNumberLiteral(context, 2);

    es2panda_AstNode *assignmentExpr =
        impl->CreateAssignmentExpression(context, identifier, numberLiteral, TOKEN_TYPE_PUNCTUATOR_SUBSTITUTION);
    ASSERT(impl->IsAssignmentExpression(assignmentExpr));
    es2panda_AstNode *assignmentStat = impl->CreateExpressionStatement(context, assignmentExpr);

    impl->AstNodeSetParent(context, identifier, assignmentExpr);
    impl->AstNodeSetParent(context, numberLiteral, assignmentExpr);
    impl->AstNodeSetParent(context, assignmentExpr, assignmentStat);

    AppendStatementToProgram(context, programNode, assignmentStat);
}

static void CheckCreateBinaryExpression(es2panda_AstNode *programNode)
{
    es2panda_AstNode *identifier = CreateIdentifierFromString(context, "x");
    es2panda_AstNode *numberLiteral = impl->CreateNumberLiteral(context, 10);

    es2panda_AstNode *assignmentExpr =
        impl->CreateBinaryExpression(context, identifier, numberLiteral, TOKEN_TYPE_PUNCTUATOR_PLUS);
    es2panda_AstNode *assignmentStat = impl->CreateExpressionStatement(context, assignmentExpr);
    ASSERT(impl->IsExpressionStatement(assignmentStat));
    impl->AstNodeSetParent(context, identifier, assignmentExpr);
    impl->AstNodeSetParent(context, numberLiteral, assignmentExpr);
    impl->AstNodeSetParent(context, assignmentExpr, assignmentStat);

    AppendStatementToProgram(context, programNode, assignmentStat);
}

static void CheckCreateUnaryExpression(es2panda_AstNode *programNode)
{
    es2panda_AstNode *identifier = CreateIdentifierFromString(context, "x");
    es2panda_AstNode *unaryExpr = impl->CreateUnaryExpression(context, identifier, TOKEN_TYPE_PUNCTUATOR_MINUS);
    es2panda_AstNode *unaryStat = impl->CreateExpressionStatement(context, unaryExpr);

    impl->AstNodeSetParent(context, identifier, unaryExpr);
    impl->AstNodeSetParent(context, unaryExpr, unaryStat);

    AppendStatementToProgram(context, programNode, unaryStat);
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
    std::cout << "PROGRAM NODE: \n" << impl->AstNodeDumpEtsSrcConst(context, programNode) << std::endl << std::endl;

    CheckFuncCall(programNode);
    CheckAssignmentExpression(programNode);
    CheckCreateBinaryExpression(programNode);
    CheckCreateUnaryExpression(programNode);
    std::cout << "PROGRAM NODE: \n" << impl->AstNodeDumpEtsSrcConst(context, programNode) << std::endl << std::endl;

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
