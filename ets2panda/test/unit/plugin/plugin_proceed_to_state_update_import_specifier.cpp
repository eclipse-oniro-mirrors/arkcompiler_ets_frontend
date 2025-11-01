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

// Test case: run plugin in .ts mode should not crash with Segfault.

#include <iostream>
#include <algorithm>
#include <cstring>

#include "public/es2panda_lib.h"
#include "util.h"

/*
 * Found unique words after 'impl->' and before '(':
 * - AllocMemory
 * - AstNodeDumpEtsSrcConst
 * - AstNodeDumpJSONConst
 * - AstNodeSetParent
 * - BlockStatementSetStatements
 * - BlockStatementStatements
 * - ContextProgram
 * - ContextState
 * - CreateConfig
 * - CreateContextFromString
 * - CreateIdentifier
 * - DestroyConfig
 * - DestroyContext
 * - IdentifierSetName
 * - ImportDeclarationSource
 * - ProceedToState
 * - ProgramAst
 * - UpdateImportDeclaration
 * - UpdateImportSpecifier
 *
 * Total unique words found: 19
 */

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;
static std::vector<std::string> importDeclarationIdentifiers {};
static es2panda_AstNode *program = nullptr;

es2panda_AstNode *parNode;
es2panda_Context *newCtx;

static std::string originalSourceCode = R"(
import { PI as PI } from "std/math/consts"
)";

void UpdateImportSpecifier(es2panda_Context *context)
{
    size_t sizeOfStatements = 0;
    auto *statements = impl->BlockStatementStatements(context, program, &sizeOfStatements);
    auto *importNode = statements[0];

    const char *identifierName = "E";
    auto *memForName = static_cast<char *>(impl->AllocMemory(context, strlen(identifierName) + 1, 1));
    std::copy_n(identifierName, strlen(identifierName) + 1, memForName);

    auto newLocal = impl->CreateIdentifier(context);
    auto newImported = impl->CreateIdentifier(context);
    impl->IdentifierSetName(context, newLocal, memForName);
    impl->IdentifierSetName(context, newImported, memForName);
    auto newSpecifier = impl->UpdateImportSpecifier(context, importNode, newLocal, newImported);

    impl->AstNodeSetParent(context, newLocal, newSpecifier);
    impl->AstNodeSetParent(context, newImported, newSpecifier);
    auto source = impl->ImportDeclarationSource(context, importNode);
    auto newImportNode = impl->UpdateImportDeclaration(context, importNode, source, &newSpecifier, 1,
                                                       Es2pandaImportKinds::IMPORT_KINDS_ALL);

    statements[0] = newImportNode;
    impl->BlockStatementSetStatements(context, program, statements, sizeOfStatements);
    impl->AstNodeSetParent(context, newImportNode, program);
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

    auto config = impl->CreateConfig(argc - 1, argv + 1);
    auto context = impl->CreateContextFromString(config, originalSourceCode.c_str(), argv[argc - 1]);

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    program = impl->ProgramAst(context, impl->ContextProgram(context));
    UpdateImportSpecifier(context);

    const char *src = impl->AstNodeDumpEtsSrcConst(context, program);
    const char *expected = "import { E as E } from \"std/math/consts\"";
    if (strstr(src, expected) == nullptr) {
        return 1;
    }

    // Note: This testcase uses wrong C-api interface, thus recheck doesn't work. Need to be fixed

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