/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <string_view>
#include <vector>

#include "util.h"
#include "public/es2panda_lib.h"

static es2panda_Impl *impl = nullptr;

// Regression test for a simultaneous-incremental recheck of a Partial<T> class.
// ETSChecker creates a synthetic %%partial class and constructor. The constructor
// ScriptFunction must inherit the source Program; otherwise recheck drops its
// saved FunctionScope when copying scopes for an unchanged external Program.
// The constructor is then not compiled, even though the object literal still
// calls it, and BIN generation fails while resolving the missing method.
// The test uses two real root files. The Partial<T> root remains unchanged while
// the other root is marked modified, so recheck must reuse scopes for the former
// and rebuild scopes only for the latter.
static bool IsOption(std::string_view arg, std::string_view option)
{
    return arg == option ||
           (arg.size() > option.size() && arg.compare(0, option.size(), option) == 0 && arg[option.size()] == '=');
}

static std::vector<const char *> BuildSimultaneousConfigArgs(int argc, char **argv)
{
    std::vector<const char *> args;
    // Keep the normal plugin-test arguments, but make this context a real
    // simultaneous-incremental context. The single-file output is invalid in
    // incremental mode, so replace it with the current directory.
    for (int i = 1; i < argc - 1; i++) {
        std::string_view arg(argv[i]);
        if (IsOption(arg, "--simultaneous") || IsOption(arg, "--incremental") || IsOption(arg, "--output")) {
            continue;
        }
        args.push_back(argv[i]);
    }
    args.push_back("--simultaneous=true");
    args.push_back("--incremental=true");
    args.push_back("--output=.");
    args.push_back(argv[argc - 1]);
    return args;
}

static std::string SiblingPath(const char *path, const char *fileName)
{
    std::string result(path);
    auto slash = result.find_last_of("/\\");
    result.resize(slash == std::string::npos ? 0 : slash + 1);
    result.append(fileName);
    return result;
}

static es2panda_Program *FindDirectProgram(es2panda_Context *context, const char *fileName)
{
    size_t sourceCount = 0;
    auto **sources = impl->ProgramDirectExternalSources(context, impl->ContextProgram(context), &sourceCount);
    for (size_t i = 0; i < sourceCount; i++) {
        size_t programCount = 0;
        auto **programs = impl->ExternalSourcePrograms(sources[i], &programCount);
        for (size_t j = 0; j < programCount; j++) {
            if (std::string_view(impl->ProgramFileNameWithExtensionConst(context, programs[j])) == fileName) {
                return programs[j];
            }
        }
    }
    return nullptr;
}

// Locate the synthetic partial-class constructor in the unchanged target AST.
// Its ScriptFunction must carry that target Program for scope merging to retain it.
struct PartialConstructorSearch {
    es2panda_Context *context;
    es2panda_AstNode *constructor = nullptr;
};

static void FindPartialConstructor(es2panda_AstNode *node, void *arg)
{
    auto *const search = static_cast<PartialConstructorSearch *>(arg);
    if (!impl->IsClassDefinition(node)) {
        return;
    }

    auto *const classId = impl->ClassDefinitionIdent(search->context, node);
    if (search->constructor != nullptr || classId == nullptr ||
        std::string_view(impl->IdentifierNameConst(search->context, classId)) != "%%partial-AC") {
        return;
    }

    size_t bodyCount = 0;
    auto **body = impl->ClassDefinitionBodyConst(search->context, node, &bodyCount);
    for (size_t i = 0; i < bodyCount; i++) {
        if (impl->IsMethodDefinition(body[i]) && impl->MethodDefinitionIsConstructorConst(search->context, body[i])) {
            search->constructor = impl->MethodDefinitionFunction(search->context, body[i]);
            return;
        }
    }
}

static bool PartialConstructorHasTargetProgram(es2panda_Context *context, es2panda_Program *partialTypeProgram)
{
    PartialConstructorSearch search {context};
    impl->AstNodeForEach(impl->ProgramAst(context, partialTypeProgram), FindPartialConstructor, &search);
    return search.constructor != nullptr &&
           impl->AstNodeProgramConst(context, search.constructor) == partialTypeProgram;
}

static bool RecheckPartialType(es2panda_Context *context)
{
    auto *partialTypeProgram = FindDirectProgram(context, "recheck_partial_type_target.ets");
    auto *modifiedProgram = FindDirectProgram(context, "recheck_partial_type_modified.ets");
    if (partialTypeProgram == nullptr || modifiedProgram == nullptr || partialTypeProgram == modifiedProgram) {
        return false;
    }

    // CHECKED has cleared the modified bit on both external roots. Preserve the
    // Partial<T> root so its saved scopes must be copied during recheck, and
    // mark only the other root as changed. Rewriting the same statement list is
    // intentional: the public setter marks its owning Program modified without
    // changing the test program's meaning.
    if (impl->ProgramIsProgramModifiedConst(context, partialTypeProgram) ||
        impl->ProgramIsProgramModifiedConst(context, modifiedProgram)) {
        return false;
    }
    auto *modifiedAst = impl->ProgramAst(context, modifiedProgram);
    size_t statementCount = 0;
    auto **statements = impl->BlockStatementStatements(context, modifiedAst, &statementCount);
    impl->BlockStatementSetStatements(context, modifiedAst, statements, statementCount);
    if (impl->ProgramIsProgramModifiedConst(context, partialTypeProgram) ||
        !impl->ProgramIsProgramModifiedConst(context, modifiedProgram)) {
        return false;
    }

    impl->AstNodeRecheck(context, impl->ProgramAst(context, impl->ContextProgram(context)));
    CheckForErrors("RECHECKED", context);
    // Assert the direct cause before BIN: without the fix the nested
    // ScriptFunction has no Program, its saved FunctionScope is dropped, and
    // the emitter later recurses while looking up the missing constructor.
    return impl->ContextState(context) != ES2PANDA_STATE_ERROR &&
           PartialConstructorHasTargetProgram(context, partialTypeProgram);
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }
    impl = GetImpl();
    if (impl == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }

    std::cout << "LOAD SUCCESS" << std::endl;
    auto configArgs = BuildSimultaneousConfigArgs(argc, argv);
    auto config = impl->CreateConfig(static_cast<int>(configArgs.size()), configArgs.data());
    auto modifiedPath = SiblingPath(argv[argc - 1], "recheck_partial_type_modified.ets");
    const char *fileNames[] = {argv[argc - 1], modifiedPath.c_str()};
    auto context = impl->CreateContextSimultaneousMode(config, 2, fileNames);
    if (context == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    for (auto state : {ES2PANDA_STATE_PARSED, ES2PANDA_STATE_BOUND, ES2PANDA_STATE_CHECKED}) {
        impl->ProceedToState(context, state);
        CheckForErrors(state == ES2PANDA_STATE_PARSED  ? "PARSE"
                       : state == ES2PANDA_STATE_BOUND ? "BOUND"
                                                       : "CHECKED",
                       context);
        if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
            impl->DestroyContext(context);
            impl->DestroyConfig(config);
            return PROCEED_ERROR_CODE;
        }
    }
    if (!RecheckPartialType(context)) {
        impl->DestroyContext(context);
        impl->DestroyConfig(config);
        return TEST_ERROR_CODE;
    }
    for (auto state : {ES2PANDA_STATE_LOWERED, ES2PANDA_STATE_ASM_GENERATED, ES2PANDA_STATE_BIN_GENERATED}) {
        impl->ProceedToState(context, state);
        CheckForErrors(state == ES2PANDA_STATE_LOWERED         ? "LOWERED"
                       : state == ES2PANDA_STATE_ASM_GENERATED ? "ASM"
                                                               : "BIN",
                       context);
        if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
            impl->DestroyContext(context);
            impl->DestroyConfig(config);
            return PROCEED_ERROR_CODE;
        }
    }
    impl->DestroyContext(context);
    impl->DestroyConfig(config);
    return 0;
}
