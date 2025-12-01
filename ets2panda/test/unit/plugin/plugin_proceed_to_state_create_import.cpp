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

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include "libarkbase/macros.h"
#include "util.h"
#include "generated/signatures.h"
#include "parser/program/program.h"
#include "public/es2panda_lib.h"
#include "ir/ets/etsImportDeclaration.h"
#include "ir/statements/expressionStatement.h"

// NOLINTBEGIN

static es2panda_Impl *g_impl = nullptr;
static es2panda_Context *g_ctx = nullptr;

static es2panda_AstNode *CreateImportDecl(es2panda_Context *context, es2panda_Program *program, const char *importIdStr,
                                          const char *importAliasStr, const char *importPathStr)
{
    auto *importPath = g_impl->CreateStringLiteral1(context, const_cast<char *>(importPathStr));

    std::vector<es2panda_AstNode *> specifiersArray;
    auto *importId = g_impl->CreateIdentifier1(context, const_cast<char *>(importIdStr));
    auto *importAlias = g_impl->CreateIdentifier1(context, const_cast<char *>(importAliasStr));
    auto *importSpecifier = g_impl->CreateImportSpecifier(context, importId, importAlias);
    g_impl->AstNodeSetParent(context, importId, importSpecifier);
    g_impl->AstNodeSetParent(context, importAlias, importSpecifier);
    specifiersArray.push_back(importSpecifier);

    auto *importDecl =
        g_impl->ETSParserBuildImportDeclaration(context, Es2pandaImportKinds::IMPORT_KINDS_ALL, specifiersArray.data(),
                                                1, importPath, program, Es2pandaImportFlags::IMPORT_FLAGS_NONE);
    return importDecl;
}

void InsertStatementInFunctionBody(es2panda_Context *context, es2panda_AstNode *func)
{
    auto *blockStmt = g_impl->ScriptFunctionBody(context, func);
    size_t newStmtNum = 1;
    auto **statement = g_impl->ETSParserCreateStatements(context, const_cast<char *>("let a0 = A0;"), &newStmtNum);
    PrependStatementToProgram(context, blockStmt, statement[0]);
}

es2panda_AstNode *GetTargetFunc(es2panda_Context *context, es2panda_AstNode *ast)
{
    static constexpr size_t BLK_STMT_IDX = 3;
    static constexpr size_t CLASS_DEF_IDX = 2;
    size_t blockStmtNum = 0;
    auto **blockStmt = g_impl->BlockStatementStatements(context, ast, &blockStmtNum);
    auto *etsGlobal = blockStmt[BLK_STMT_IDX];
    auto *classDef = g_impl->ClassDeclarationDefinition(context, etsGlobal);
    size_t blockStmtNum2 = 0;
    auto *classBody = g_impl->ClassDefinitionBody(context, classDef, &blockStmtNum2);
    auto *fooMethod = classBody[CLASS_DEF_IDX];
    auto *fooFunc = g_impl->MethodDefinitionFunction(context, fooMethod);

    return fooFunc;
}

bool TestInsertImportAfterParse(es2panda_Context *context, [[maybe_unused]] es2panda_Config *config,
                                es2panda_Program *program)
{
    size_t externalSourceCnt {0};
    g_impl->ProgramExternalSources(context, program, &externalSourceCnt);
    std::cout << "ExternalProgram Count:" << externalSourceCnt << std::endl;
    auto *importDeclAfterParse = CreateImportDecl(context, program, "B", "B", "./export2");

    g_impl->InsertETSImportDeclarationAndParse(context, program, importDeclAfterParse);
    auto *importDeclString = g_impl->AstNodeDumpEtsSrcConst(context, importDeclAfterParse);
    std::cout << importDeclString << std::endl;
    size_t externalSourceCntAfterInsert {0};
    g_impl->ProgramExternalSources(context, g_impl->ContextProgram(context), &externalSourceCntAfterInsert);
    std::cout << "ExternalProgram Count:" << externalSourceCntAfterInsert << std::endl;

    if (externalSourceCntAfterInsert != externalSourceCnt + 1) {
        std::cout << "Insert ETSImportDeclaration Failure." << std::endl;
        g_impl->DestroyContext(context);
        g_impl->DestroyConfig(config);
        return false;
    }

    return true;
}

bool CheckFirstUseStatic(es2panda_Program *program)
{
    auto *parserProgram = reinterpret_cast<ark::es2panda::parser::Program *>(program);
    auto &stmts = parserProgram->Ast()->StatementsForUpdates();
    if (stmts.empty()) {
        return false;
    }
    auto *firstStmt = stmts.front();
    if (!firstStmt->IsExpressionStatement()) {
        return false;
    }
    auto *expr = firstStmt->AsExpressionStatement()->GetExpression();
    if (!expr->IsStringLiteral()) {
        return false;
    }
    auto *strLiteral = expr->AsStringLiteral();
    if (strLiteral->Str() != ark::es2panda::compiler::Signatures::STATIC_PROGRAM_FLAG) {
        return false;
    }
    return true;
}

void InsertImportInHeaderAfterParse(es2panda_Context *context, [[maybe_unused]] es2panda_Config *config,
                                    es2panda_Program *program)
{
    size_t mainProgExternalSourceCnt {0};
    auto **externalSources = g_impl->ProgramExternalSources(context, program, &mainProgExternalSourceCnt);
    if (mainProgExternalSourceCnt == 0) {
        std::cout << "No ExternalSource in main program." << std::endl;
        return;
    }

    es2panda_AstNode *importDeclInHeaderAfterParse {nullptr};
    size_t prevHeaderExternalSourceCnt {0};
    es2panda_Program *headerProgram {nullptr};
    for (size_t i = 0; i < mainProgExternalSourceCnt; ++i) {
        auto *externalSource = externalSources[i];
        std::string sourceName = g_impl->ExternalSourceName(externalSource);
        size_t externalSourceProgramCnt {0};
        auto **programs = g_impl->ExternalSourcePrograms(externalSource, &externalSourceProgramCnt);
        if (sourceName.compare("export2") == 0) {
            headerProgram = programs[0];
            g_impl->ProgramExternalSources(context, headerProgram, &prevHeaderExternalSourceCnt);
            importDeclInHeaderAfterParse = CreateImportDecl(context, headerProgram, "hello", "hello", "./export3");
            g_impl->InsertETSImportDeclarationAndParse(context, headerProgram, importDeclInHeaderAfterParse);
            std::cout << g_impl->AstNodeDumpEtsSrcConst(context, g_impl->ProgramAst(context, headerProgram))
                      << std::endl;
            auto *importDeclString = g_impl->AstNodeDumpEtsSrcConst(context, importDeclInHeaderAfterParse);
            std::cout << importDeclString << std::endl;
            break;
        }
    }
}

bool Find(es2panda_AstNode *ast)
{
    if (g_impl->IsETSImportDeclaration(ast)) {
        size_t len = 0;
        auto specifiers = g_impl->ImportDeclarationSpecifiersConst(g_ctx, ast, &len);
        auto source = g_impl->ImportDeclarationSource(g_ctx, ast);
        auto importDeclaration =
            g_impl->UpdateETSImportDeclaration(g_ctx, ast, source, specifiers, len, IMPORT_KINDS_ALL);
        auto declPath = g_impl->ETSImportDeclarationDeclPathConst(g_ctx, ast);
        auto ohmUrl = g_impl->ETSImportDeclarationOhmUrlConst(g_ctx, ast);
        auto resolvedSource = g_impl->ETSImportDeclarationResolvedSourceConst(g_ctx, ast);
        g_impl->ETSImportDeclarationSetImportMetadata(g_ctx, importDeclaration, IMPORT_FLAGS_NONE, ID_ETS,
                                                      resolvedSource, declPath, ohmUrl);
        for (size_t i = 0; i < len; i++) {
            g_impl->AstNodeSetParent(g_ctx, specifiers[i], importDeclaration);
        }
        g_impl->AstNodeSetParent(g_ctx, source, importDeclaration);
        auto parent = g_impl->AstNodeParent(g_ctx, ast);
        if (g_impl->AstNodeIsProgramConst(g_ctx, parent)) {
            size_t sizeOfStatements = 0;
            auto *statements = g_impl->BlockStatementStatements(g_ctx, parent, &sizeOfStatements);
            statements[1] = importDeclaration;
            g_impl->BlockStatementSetStatements(g_ctx, parent, statements, sizeOfStatements);
            g_impl->AstNodeSetParent(g_ctx, importDeclaration, parent);
            std::string str(g_impl->AstNodeDumpEtsSrcConst(g_ctx, parent));
            if (str.find("import { A as A } from \"./export\"") != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

void DestroyConfigAndContext(es2panda_Context *context, es2panda_Config *config)
{
    g_impl->DestroyContext(context);
    g_impl->DestroyConfig(config);
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

    std::cout << "LOAD SUCCESS" << std::endl;
    const char **args = const_cast<const char **>(std::next(argv));
    auto config = g_impl->CreateConfig(argc - 1, args);
    auto source = std::string(
        "\"use static\";import {A} from \"./export\" ;function foo() {let b:B = new B();let a:A = new A()}");

    auto context = g_impl->CreateContextFromString(config, source.data(), *(std::next(argv, argc - 1)));
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }
    g_ctx = context;
    g_impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);

    auto *program = g_impl->ContextProgram(context);
    auto ast = g_impl->ProgramAst(context, program);
    if (!g_impl->AstNodeIsAnyChildConst(context, ast, Find)) {
        return TEST_ERROR_CODE;
    }

    if (!TestInsertImportAfterParse(context, config, program) || !CheckFirstUseStatic(program)) {
        DestroyConfigAndContext(context, config);
        return 1;
    }

    InsertImportInHeaderAfterParse(context, config, program);

    g_impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", context);

    g_impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    auto *importDeclAfterCheck = CreateImportDecl(context, program, "A0", "A0", "./export2");
    g_impl->InsertETSImportDeclarationAndParse(context, program, importDeclAfterCheck);

    auto *tagetFunc = GetTargetFunc(context, ast);
    InsertStatementInFunctionBody(context, tagetFunc);

    g_impl->AstNodeRecheck(context, ast);

    g_impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", context);
    if (g_impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        DestroyConfigAndContext(context, config);
        return PROCEED_ERROR_CODE;
    }

    DestroyConfigAndContext(context, config);
    return 0;
}

// NOLINTEND
