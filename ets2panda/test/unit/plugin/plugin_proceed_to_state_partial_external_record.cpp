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

#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "util.h"
#include "public/es2panda_lib.h"

static es2panda_Impl *impl = nullptr;

using StateStep = std::pair<es2panda_ContextState, const char *>;

// Partial<T> external-record regression (see patial_external_record_bug.md): the consumer never
// calls %%get-j, so its foreign declaration reaches the abc only with the GenInterfaceRecord
// overload-dependency fix. BIN succeeds either way, so assert the name string in the abc (the
// path comes from FormOutputPathForFile, not a hardcoded name).
static bool AbcContains(const std::string &fileName, std::string_view needle)
{
    std::ifstream in(fileName, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return contents.find(needle) != std::string::npos;
}

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

static bool RecheckUserProgram(es2panda_Context *context)
{
    auto *userProgram = FindDirectProgram(context, "partial_external_record_user.ets");
    if (userProgram == nullptr || impl->ProgramIsProgramModifiedConst(context, userProgram)) {
        return false;
    }
    auto *userAst = impl->ProgramAst(context, userProgram);
    size_t statementCount = 0;
    auto **statements = impl->BlockStatementStatements(context, userAst, &statementCount);
    impl->BlockStatementSetStatements(context, userAst, statements, statementCount);
    if (!impl->ProgramIsProgramModifiedConst(context, userProgram)) {
        return false;
    }
    impl->AstNodeRecheck(context, impl->ProgramAst(context, impl->ContextProgram(context)));
    CheckForErrors("RECHECKED", context);
    return impl->ContextState(context) != ES2PANDA_STATE_ERROR;
}

static bool ProceedStates(es2panda_Context *context, const std::vector<StateStep> &steps)
{
    for (const auto &[state, name] : steps) {
        impl->ProceedToState(context, state);
        CheckForErrors(name, context);
        if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
            return false;
        }
    }
    return true;
}

// FormOutputPathForFile requires the context to be in the PARSED state; capture the
// compiler-derived abc path for the consumer program while that holds. The returned string is
// allocated from the context allocator, so copy it out immediately.
static bool CaptureUserAbcPath(es2panda_Context *context, const std::string &userPath, std::string *out)
{
    char *abcPath = impl->FormOutputPathForFile(context, userPath.c_str());
    if (abcPath == nullptr || *abcPath == '\0') {
        std::cerr << "FAIL: FormOutputPathForFile returned no path for " << userPath << std::endl;
        return false;
    }
    *out = abcPath;
    return true;
}

// Without the fix, the uncalled %%get-j accessor has no foreign method item in the consumer
// abc; with the fix it must be declared.
static bool CheckUncalledAccessorDeclared(const std::string &userAbcPath)
{
    if (AbcContains(userAbcPath, "%%get-j")) {
        return true;
    }
    std::cerr << "FAIL: uncalled %%partial-I.%%get-j accessor is missing from " << userAbcPath << std::endl;
    return false;
}

static int RunScenario(es2panda_Context *context, const std::string &userPath, std::string *userAbcPath)
{
    if (!ProceedStates(context, {{ES2PANDA_STATE_PARSED, "PARSE"}})) {
        return PROCEED_ERROR_CODE;
    }
    if (!CaptureUserAbcPath(context, userPath, userAbcPath)) {
        return TEST_ERROR_CODE;
    }
    if (!ProceedStates(context, {{ES2PANDA_STATE_BOUND, "BOUND"}, {ES2PANDA_STATE_CHECKED, "CHECKED"}})) {
        return PROCEED_ERROR_CODE;
    }
    if (!RecheckUserProgram(context)) {
        return TEST_ERROR_CODE;
    }
    const std::vector<StateStep> emitSteps = {{ES2PANDA_STATE_LOWERED, "LOWERED"},
                                              {ES2PANDA_STATE_ASM_GENERATED, "ASM"},
                                              {ES2PANDA_STATE_BIN_GENERATED, "BIN"}};
    if (!ProceedStates(context, emitSteps)) {
        return PROCEED_ERROR_CODE;
    }
    return 0;
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
    auto userPath = SiblingPath(argv[argc - 1], "partial_external_record_user.ets");
    const char *fileNames[] = {argv[argc - 1], userPath.c_str()};
    auto context = impl->CreateContextSimultaneousMode(config, 2, fileNames);
    if (context == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    std::string userAbcPath;
    int result = RunScenario(context, userPath, &userAbcPath);
    if (result == 0 && !CheckUncalledAccessorDeclared(userAbcPath)) {
        result = TEST_ERROR_CODE;
    }

    impl->DestroyContext(context);
    impl->DestroyConfig(config);
    return result;
}
