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

// Regression test for a simultaneous-incremental recheck of a rest-tuple function.
// RestTupleConstructionPhase creates a synthetic wrapper for the expanded tuple
// arguments. The wrapper must retain the source Program in its range; otherwise
// recheck drops its saved FunctionScope when copying scopes for an unchanged
// external Program. The wrapper is then not compiled, even though main still
// calls it, and BIN generation fails while resolving the missing method.
// The test uses two real root files. The rest-tuple root remains unchanged while
// the other root is marked modified, so the recheck must reuse scopes for the
// former and rebuild scopes only for the latter.
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

static bool RecheckRestTuple(es2panda_Context *context)
{
    auto *restTupleProgram = FindDirectProgram(context, "recheck_rest_tuple_target.ets");
    auto *modifiedProgram = FindDirectProgram(context, "recheck_rest_tuple_modified.ets");
    if (restTupleProgram == nullptr || modifiedProgram == nullptr || restTupleProgram == modifiedProgram) {
        return false;
    }

    // CHECKED has cleared the modified bit on both external roots. Preserve the
    // rest-tuple root so its saved scopes must be copied during recheck, and
    // mark only the other root as changed. Rewriting the same statement list is
    // intentional: the public setter marks its owning Program modified without
    // changing the test program's meaning.
    if (impl->ProgramIsProgramModifiedConst(context, restTupleProgram) ||
        impl->ProgramIsProgramModifiedConst(context, modifiedProgram)) {
        return false;
    }
    auto *modifiedAst = impl->ProgramAst(context, modifiedProgram);
    size_t statementCount = 0;
    auto **statements = impl->BlockStatementStatements(context, modifiedAst, &statementCount);
    impl->BlockStatementSetStatements(context, modifiedAst, statements, statementCount);
    if (impl->ProgramIsProgramModifiedConst(context, restTupleProgram) ||
        !impl->ProgramIsProgramModifiedConst(context, modifiedProgram)) {
        return false;
    }

    impl->AstNodeRecheck(context, impl->ProgramAst(context, impl->ContextProgram(context)));
    CheckForErrors("RECHECKED", context);
    return impl->ContextState(context) != ES2PANDA_STATE_ERROR;
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
    auto modifiedPath = SiblingPath(argv[argc - 1], "recheck_rest_tuple_modified.ets");
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
    if (!RecheckRestTuple(context)) {
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
