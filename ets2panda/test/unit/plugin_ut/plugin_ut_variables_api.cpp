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
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iterator>
#include <ostream>
#include <string>
#include "test/unit/plugin/util.h"
#include "public/es2panda_lib.h"

// NOLINTBEGIN
/*
 * Covered C API List:
 *   AstNodeDumpEtsSrcConst
 *   AstNodeDumpJSONConst
 *   AstNodeForEach
 *   AstNodeIterateConst
 *   AstNodeRebind
 *   AstNodeSetParent
 *   BlockStatementSetStatements
 *   ContextErrorMessage
 *   ContextProgram
 *   ContextState
 *   CreateConfig
 *   CreateContextFromString
 *   CreateIdentifier1
 *   CreateVariableDeclarator1
 *   DestroyConfig
 *   DestroyContext
 *   FunctionDeclarationFunction
 *   IdentifierName
 *   IsFunctionDeclaration
 *   IsVariableDeclaration
 *   IsVariableDeclarator
 *   ProceedToState
 *   ProgramAst
 *   ScriptFunctionBody
 *   ScriptFunctionId
 *   UpdateVariableDeclaration
 *   VariableDeclarationDeclaratorsConst
 *   VariableDeclaratorId
 *   VariableDeclaratorIdConst
 *   VariableDeclaratorInit
 */

static std::string expectResStr(R"(
{
  "type": "Program",
  "statements": [
    {
      "type": "FunctionDeclaration",
      "function": {
        "type": "ScriptFunction",
        "id": {
          "type": "Identifier",
          "name": "main"
        },
        "generator": false,
        "async": false,
        "expression": false,
        "params": [],
        "body": {
          "type": "BlockStatement",
          "statements": [
            {
              "type": "VariableDeclaration",
              "declarations": [
                {
                  "type": "VariableDeclarator",
                  "id": {
                    "type": "Identifier",
                    "name": "newVar"
                  },
                  "init": {
                    "type": "NumberLiteral",
                    "value": 5
                  }
                }
              ],
              "kind": "let"
            }
          ]
        }
      }
    }
  ]
}
)");
static es2panda_Impl *g_impl = nullptr;
static auto source = std::string("function main() { \nlet a = 5;\n }");

static es2panda_AstNode *letStatement = nullptr;
static es2panda_AstNode *mainScriptFunc = nullptr;
static es2panda_Context *g_ctx = nullptr;

es2panda_AstNode *parNode = nullptr;
es2panda_Context *newCtx = nullptr;

static void FindLet(es2panda_AstNode *ast)
{
    if (!g_impl->IsVariableDeclaration(ast)) {
        g_impl->AstNodeIterateConst(g_ctx, ast, FindLet);
        return;
    }
    letStatement = ast;
}

static void FindMainDef(es2panda_AstNode *ast)
{
    if (!g_impl->IsFunctionDeclaration(ast)) {
        g_impl->AstNodeIterateConst(g_ctx, ast, FindMainDef);
        return;
    }
    auto scriptFunc = g_impl->FunctionDeclarationFunction(g_ctx, ast);
    if (scriptFunc == nullptr ||
        std::string("main") != g_impl->IdentifierName(g_ctx, g_impl->ScriptFunctionId(g_ctx, scriptFunc))) {
        return;
    }
    mainScriptFunc = scriptFunc;
}

static void changeParent(es2panda_AstNode *child)
{
    g_impl->AstNodeSetParent(newCtx, child, parNode);
}

static void SetRightParent(es2panda_AstNode *node, void *arg)
{
    es2panda_Context *context = static_cast<es2panda_Context *>(arg);
    newCtx = context;
    parNode = node;
    g_impl->AstNodeIterateConst(context, node, changeParent);
}

/*
 *   update variableDeclaration name
 */
static bool UpdateAst(es2panda_Context *context, es2panda_AstNode *ast)
{
    auto mainFuncBody = g_impl->ScriptFunctionBody(context, mainScriptFunc);
    size_t n = 0;
    es2panda_AstNode *varIdent = g_impl->CreateIdentifier1(context, const_cast<char *>("newVar"));
    auto declarator = g_impl->CreateVariableDeclarator1(
        context, Es2pandaVariableDeclaratorFlag::VARIABLE_DECLARATOR_FLAG_LET, varIdent,
        g_impl->VariableDeclaratorInit(context,
                                       g_impl->VariableDeclarationDeclaratorsConst(context, letStatement, &n)[0]));

    auto updateDeclaration = g_impl->UpdateVariableDeclaration(
        context, letStatement, Es2pandaVariableDeclarationKind::VARIABLE_DECLARATION_KIND_LET, &declarator, 1);
    es2panda_AstNode *newMainStatements[1] = {updateDeclaration};
    g_impl->BlockStatementSetStatements(context, mainFuncBody, newMainStatements, 1U);
    g_impl->AstNodeForEach(ast, SetRightParent, context);

    const char *errMessage = g_impl->ContextErrorMessage(context);
    if (!errMessage) {
        std::cout << "The errMessage is nullptr." << std::endl;
        return false;
    }

    g_impl->AstNodeIterateConst(context, ast, FindLet);
    size_t declaratorLen = 0;
    es2panda_AstNode **declarators = g_impl->VariableDeclarationDeclaratorsConst(context, letStatement, &declaratorLen);
    if (declaratorLen != 1) {
        std::cout << "Fail to get declartors." << std::endl;
        return false;
    }

    char *astStr = g_impl->AstNodeDumpJSONConst(context, ast);
    expectResStr = expectResStr.erase(0, 1);
    expectResStr = expectResStr.erase(expectResStr.length() - 1, 1);
    if (strcmp(expectResStr.c_str(), astStr) != 0) {
        std::cout << "Dumping ast has wrong result." << std::endl;
        return false;
    }
    es2panda_AstNode *varDeclInit = g_impl->VariableDeclaratorInit(context, declarators[0]);
    if (!varDeclInit) {
        std::cout << "The varDeclInit is nullptr." << std::endl;
        return false;
    }
    std::string res1(g_impl->AstNodeDumpEtsSrcConst(context, varDeclInit));
    if (res1.find("5") == std::string::npos) {
        std::cout << "Fail to find given variabledeclaration init value." << std::endl;
        return false;
    }
    std::string res2(g_impl->AstNodeDumpEtsSrcConst(context, letStatement));
    return (res2.find("newVar") != std::string::npos);
}

/*
 *   "function main() { \nlet a = 5;\n }"
 *   is changed to
 *   "function main() { \nlet newVar = 5;\n }"
 */
static bool Handle(es2panda_Context *context, [[maybe_unused]] es2panda_AstNode *ast)
{
    g_impl->AstNodeIterateConst(context, ast, FindLet);
    g_impl->AstNodeIterateConst(context, ast, FindMainDef);
    if (mainScriptFunc == nullptr || letStatement == nullptr) {
        return false;
    }

    size_t declaratorLen = 0;
    es2panda_AstNode **declarators = g_impl->VariableDeclarationDeclaratorsConst(context, letStatement, &declaratorLen);
    if (declaratorLen != 1) {
        std::cout << "Fail to get declartors." << std::endl;
        return false;
    }

    es2panda_AstNode *varDeclNode = declarators[0];
    if (!varDeclNode) {
        std::cout << "The declaratorsElement is nullptr. " << std::endl;
        return false;
    }

    if (!g_impl->IsVariableDeclarator(varDeclNode)) {
        std::cout << "The varDeclNode is not variable declaration." << std::endl;
        return false;
    }

    const es2panda_AstNode *varDeclIDConst = g_impl->VariableDeclaratorIdConst(context, varDeclNode);
    if (!varDeclIDConst) {
        std::cout << "The varDeclIDConst is nullptr." << std::endl;
        return false;
    }
    es2panda_AstNode *varDeclID = g_impl->VariableDeclaratorId(context, varDeclNode);
    if (!varDeclID) {
        std::cout << "The varDeclID is nullptr." << std::endl;
        return false;
    }

    if (strcmp(g_impl->IdentifierName(context, varDeclID), const_cast<char *>("a")) != 0) {
        std::cout << "The varDeclNode is not the expected node." << std::endl;
        return false;
    }

    return UpdateAst(context, ast);
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }

    g_impl = GetImpl();
    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = g_impl->CreateConfig(argc - 1, args);
    g_ctx = g_impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (g_ctx == nullptr) {
        std::cerr << "Fail to create context." << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }
    int retCode = Test(g_ctx, g_impl, ES2PANDA_STATE_PARSED, Handle);
    if (retCode != 0) {
        std::cerr << "Fail to execute on AfterParse phase" << std::endl;
        g_impl->DestroyConfig(config);
        return retCode;
    }

    g_impl->DestroyConfig(config);

    return 0;
}
// NOLINTEND
