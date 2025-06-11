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
#include <iostream>
#include <string>

#include "libarkbase/os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

void createClassDeclaration(es2panda_Context *context, char *className, es2panda_AstNode *program)
{
    impl = GetImpl();
    size_t sizeOfStatements = 0;
    auto *statements = impl->BlockStatementStatements(context, program, &sizeOfStatements);
    es2panda_AstNode **newStatements =
        static_cast<es2panda_AstNode **>(impl->AllocMemory(context, sizeOfStatements + 1, sizeof(es2panda_AstNode *)));
    for (size_t i = 0; i < sizeOfStatements; i++) {
        newStatements[i] = statements[i];
    }
    auto *identifier = impl->CreateIdentifier1(context, className);
    auto *classDefinition = impl->CreateClassDefinition1(
        context, identifier, nullptr, 0, Es2pandaClassDefinitionModifiers::CLASS_DEFINITION_MODIFIERS_CLASS_DECL,
        Es2pandaModifierFlags::MODIFIER_FLAGS_FINAL);
    auto *classDeclaration = impl->CreateClassDeclaration(context, classDefinition);
    impl->AstNodeSetParent(context, classDefinition, classDeclaration);
    impl->AstNodeSetParent(context, identifier, classDefinition);
    newStatements[sizeOfStatements] = classDeclaration;
    impl->BlockStatementSetStatements(context, program, newStatements, sizeOfStatements + 1);
    impl->AstNodeSetParent(context, classDeclaration, program);
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
    auto context = impl->CreateContextFromFile(config, argv[argc - 1]);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);

    auto *programNode = impl->ProgramAst(context, impl->ContextProgram(context));

    std::cout << impl->AstNodeDumpJSONConst(context, programNode) << std::endl;

    std::string className = std::string("PluginClass");
    auto *memForName = static_cast<char *>(impl->AllocMemory(context, className.size() + 1, 1));
    std::copy_n(className.c_str(), className.size() + 1, memForName);
    createClassDeclaration(context, static_cast<char *>(memForName), programNode);

    std::cout << impl->AstNodeDumpJSONConst(context, programNode) << std::endl;

    impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", context);

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    std::cout << impl->AstNodeDumpJSONConst(context, programNode) << std::endl;

    impl->AstNodeRecheck(context, programNode);

    impl->ProceedToState(context, ES2PANDA_STATE_LOWERED);
    CheckForErrors("LOWERED", context);

    std::cout << impl->AstNodeDumpJSONConst(context, programNode) << std::endl;

    impl->ProceedToState(context, ES2PANDA_STATE_ASM_GENERATED);
    CheckForErrors("ASM", context);

    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", context);
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
