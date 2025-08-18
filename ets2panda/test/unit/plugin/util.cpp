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

#include "util.h"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <ostream>
#include <string>
#include "macros.h"

static es2panda_Impl *g_implPtr = nullptr;

es2panda_Impl *GetImpl()
{
    if (g_implPtr != nullptr) {
        return g_implPtr;
    }

    std::string soName = ark::os::library_loader::DYNAMIC_LIBRARY_PREFIX + std::string("es2panda-public") +
                         ark::os::library_loader::DYNAMIC_LIBRARY_SUFFIX;
    auto libraryRes = ark::os::library_loader::Load(soName);
    if (!libraryRes.HasValue()) {
        std::cout << "Error in load lib" << std::endl;
        return nullptr;
    }

    auto library = std::move(libraryRes.Value());
    auto getImpl = ark::os::library_loader::ResolveSymbol(library, "es2panda_GetImpl");
    if (!getImpl.HasValue()) {
        std::cout << "Error in load func get g_implPtr" << std::endl;
        return nullptr;
    }

    auto getImplFunc = reinterpret_cast<const es2panda_Impl *(*)(int)>(getImpl.Value());
    if (getImplFunc != nullptr) {
        g_implPtr = const_cast<es2panda_Impl *>(getImplFunc(ES2PANDA_LIB_VERSION));
        return g_implPtr;
    }
    return nullptr;
}

void CheckForErrors(const std::string &stateName, es2panda_Context *context)
{
    if (g_implPtr->ContextState(context) == ES2PANDA_STATE_ERROR) {
        std::cout << "PROCEED TO " << stateName << " ERROR" << std::endl;
        std::cout << g_implPtr->ContextErrorMessage(context) << std::endl;
    } else {
        std::cout << "PROCEED TO " << stateName << " SUCCESS" << std::endl;
    }
}

es2panda_AstNode *CreateIdentifierFromString(es2panda_Context *context, const std::string_view &name)
{
    auto impl = GetImpl();
    auto *memForName = static_cast<char *>(impl->AllocMemory(context, name.size() + 1, 1));
    std::copy_n(name.data(), name.size() + 1, memForName);
    auto *identifier = impl->CreateIdentifier1(context, memForName);
    return identifier;
}

void AppendStatementToProgram(es2panda_Context *context, es2panda_AstNode *program, es2panda_AstNode *newStatement)
{
    auto impl = GetImpl();
    size_t sizeOfStatements = 0;
    auto *statements = impl->BlockStatementStatements(context, program, &sizeOfStatements);
    auto **newStatements =
        static_cast<es2panda_AstNode **>(impl->AllocMemory(context, sizeOfStatements + 1, sizeof(es2panda_AstNode *)));
    std::copy_n(statements, sizeOfStatements, newStatements);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    newStatements[sizeOfStatements] = newStatement;
    impl->BlockStatementSetStatements(context, program, newStatements, sizeOfStatements + 1);
    impl->AstNodeSetParent(context, newStatement, program);
}

void PrependStatementToProgram(es2panda_Context *context, es2panda_AstNode *program, es2panda_AstNode *newStatement)
{
    auto impl = GetImpl();
    size_t sizeOfStatements = 0;
    auto *statements = impl->BlockStatementStatements(context, program, &sizeOfStatements);
    auto **newStatements =
        static_cast<es2panda_AstNode **>(impl->AllocMemory(context, sizeOfStatements + 1, sizeof(es2panda_AstNode *)));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::copy_n(statements, sizeOfStatements, &newStatements[1]);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    newStatements[0] = newStatement;
    impl->BlockStatementSetStatements(context, program, newStatements, sizeOfStatements + 1);
    impl->AstNodeSetParent(context, newStatement, program);
}

static const char *GetPhaseName(es2panda_ContextState state)
{
    switch (state) {
        case ES2PANDA_STATE_NEW:
            return "NEW";
        case ES2PANDA_STATE_PARSED:
            return "PARSE";
        case ES2PANDA_STATE_BOUND:
            return "BOUND";
        case ES2PANDA_STATE_CHECKED:
            return "CHECKED";
        case ES2PANDA_STATE_LOWERED:
            return "LOWERED";
        case ES2PANDA_STATE_ASM_GENERATED:
            return "ASM";
        case ES2PANDA_STATE_BIN_GENERATED:
            return "BIN";
        case ES2PANDA_STATE_ERROR:
            return "ERROR";
        default:
            return "NON_VALID_STATE";
    }
}

static bool IsAllowedStage(es2panda_ContextState state)
{
    switch (state) {
        case ES2PANDA_STATE_NEW:
            return false;
        case ES2PANDA_STATE_ERROR:
            return false;
        default:
            return true;
    }
}

static int DestroyTest(es2panda_Context *context, es2panda_Config *config, const int exitCode)
{
    g_implPtr->DestroyContext(context);
    g_implPtr->DestroyConfig(config);
    return exitCode;
}

int RunAllStagesWithTestFunction(ProccedToStatePluginTestData &data)
{
    if (data.argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    *data.impl = GetImpl();
    std::cout << "LOAD SUCCESS" << std::endl;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char **args = const_cast<const char **>(&(data.argv[1]));
    auto config = g_implPtr->CreateConfig(data.argc - 1, args);
    es2panda_Context *context = nullptr;
    if (data.fromSource) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        context = g_implPtr->CreateContextFromString(config, data.source.data(), data.argv[data.argc - 1]);
    } else {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        context = g_implPtr->CreateContextFromFile(config, data.argv[data.argc - 1]);
    }
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    for (auto [testStage, _] : data.testFunctions) {
        if (!IsAllowedStage(testStage)) {
            return DestroyTest(context, config, TEST_ERROR_CODE);
        }
    }

    for (auto state = ES2PANDA_STATE_PARSED; state <= ES2PANDA_STATE_BIN_GENERATED;
         state = static_cast<es2panda_ContextState>(state + 1)) {
        if (!IsAllowedStage(state)) {
            continue;
        }
        g_implPtr->ProceedToState(context, state);
        CheckForErrors(GetPhaseName(state), context);
        for (const auto &testFunc : data.testFunctions[state]) {
            if (!testFunc(context)) {
                return DestroyTest(context, config, TEST_ERROR_CODE);
            }
        }
        if (state == data.exitAfterState) {
            break;
        }
    }

    int result = g_implPtr->ContextState(context) == ES2PANDA_STATE_ERROR ? PROCEED_ERROR_CODE : 0;
    return DestroyTest(context, config, result);
}

int Test(es2panda_Context *context, es2panda_Impl *impl, int stage,
         const std::function<bool(es2panda_Context *, es2panda_AstNode *)> &handle)
{
    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);
    es2panda_AstNode *ast = impl->ProgramAst(context, impl->ContextProgram(context));
    if (ast == nullptr) {
        return TEST_ERROR_CODE;
    }
    switch (stage) {
        case ES2PANDA_STATE_PARSED: {
            break;
        }
        case ES2PANDA_STATE_BOUND: {
            impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
            CheckForErrors("BOUND", context);
            break;
        }
        case ES2PANDA_STATE_CHECKED: {
            impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
            CheckForErrors("CHECKED", context);
            break;
        }
        case ES2PANDA_STATE_LOWERED: {
            impl->ProceedToState(context, ES2PANDA_STATE_LOWERED);
            CheckForErrors("LOWERED", context);
            break;
        }
        default: {
            UNREACHABLE();
        }
    }
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }
    if (!handle(context, ast)) {
        return TEST_ERROR_CODE;
    }
    if (stage == ES2PANDA_STATE_BOUND) {
        impl->AstNodeRebind(context, ast);
    } else if (stage == ES2PANDA_STATE_CHECKED || stage == ES2PANDA_STATE_LOWERED) {
        impl->AstNodeRecheck(context, ast);
    }
    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", context);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }
    impl->DestroyContext(context);
    return 0;
}
