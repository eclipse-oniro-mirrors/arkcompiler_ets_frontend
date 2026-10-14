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

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "libarkbase/os/filesystem.h"
#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static constexpr std::string_view ARKTSCONFIG_OPTION = "--arktsconfig";
static constexpr std::string_view ARKTSCONFIG_PREFIX = "--arktsconfig=";
static constexpr std::string_view OUTPUT_PREFIX = "--output=";

static constexpr mode_t DIR_MODE = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

static es2panda_Impl *impl = nullptr;

static bool ReadFile(const std::string &path, std::string &out)
{
    FILE *f = fopen(path.c_str(), "r");
    if (f == nullptr) {
        return false;
    }
    char buf[4096];
    size_t n;
    out.clear();
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        out.append(buf, n);
    }
    bool ok = ferror(f) == 0;
    if (fclose(f) != 0) {
        ok = false;
    }
    return ok;
}

static std::string FindArktsConfig(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg.compare(0, ARKTSCONFIG_PREFIX.size(), ARKTSCONFIG_PREFIX) == 0) {
            return std::string(arg.substr(ARKTSCONFIG_PREFIX.size()));
        }
        if (arg == ARKTSCONFIG_OPTION && i + 1 < argc) {
            return std::string(argv[i + 1]);
        }
    }
    return "";
}

static std::vector<std::string> BuildConfigArgs(int argc, char **argv, const std::string &configPath)
{
    std::vector<std::string> args;
    args.emplace_back(ark::os::GetAbsolutePath(argv[1]));
    for (int i = 2; i < argc - 1; ++i) {
        std::string_view arg = argv[i];
        if (arg.compare(0, OUTPUT_PREFIX.size(), OUTPUT_PREFIX) == 0 ||
            arg.compare(0, ARKTSCONFIG_PREFIX.size(), ARKTSCONFIG_PREFIX) == 0) {
            continue;
        }
        args.emplace_back(argv[i]);
    }
    args.emplace_back(std::string(ARKTSCONFIG_PREFIX) + ark::os::GetAbsolutePath(configPath));
    args.emplace_back("--simultaneous");
    args.emplace_back("--opt-level=0");
    return args;
}

struct TestPaths {
    std::string fooPath;
    std::string mainPagePath;
    std::string expectedPath;
    std::string configPath;
};

static bool ResolveTestPaths(int argc, char **argv, TestPaths &paths)
{
    std::string testDataDir = ark::os::GetAbsolutePath(ark::os::GetParentDir(argv[argc - 1])) + "/simult_declgen";
    paths.fooPath = testDataDir + "/Foo.ets";
    paths.mainPagePath = testDataDir + "/MainPage.ets";
    paths.expectedPath = testDataDir + "/expected/MainPage-expected.txt";
    paths.configPath = FindArktsConfig(argc, argv);
    return !paths.configPath.empty();
}

static int DestroyContextAndReturn(es2panda_Context *context, int code)
{
    impl->DestroyContext(context);
    return code;
}

static int CompareAndReport(const std::string &actual, const std::string &expected, const std::string &expectedPath,
                            const std::string &outputPath)
{
    if (actual == expected) {
        std::cout << "PASS: relative import path preserved in declaration" << std::endl;
        return 0;
    }
    std::cerr << "FAIL: generated declaration does not match expected output." << std::endl;
    std::cerr << std::endl;
    std::cerr << "This indicates that relative import paths (e.g. \"./Foo\") are" << std::endl;
    std::cerr << "incorrectly converted to package-qualified paths (e.g. \"har2/Foo\")" << std::endl;
    std::cerr << "in simultaneous compilation mode. See PackageQualifiedImportSource()" << std::endl;
    std::cerr << "in ir/srcDump.cpp." << std::endl;
    std::cerr << std::endl;
    std::cerr << "--- Expected (from " << expectedPath << ") ---" << std::endl;
    std::cerr << expected << std::endl;
    std::cerr << "--- Actual (from " << outputPath << ") ---" << std::endl;
    std::cerr << actual << std::endl;
    std::cerr << "----------------------------------------" << std::endl;
    return TEST_ERROR_CODE;
}

static int GenerateAndCompare(es2panda_Config *config, const TestPaths &paths)
{
    const char *fileNames[] = {paths.fooPath.c_str(), paths.mainPagePath.c_str()};
    auto *context = impl->CreateContextSimultaneousMode(config, 2, fileNames);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        std::cerr << "COMPILATION FAILED" << std::endl;
        return DestroyContextAndReturn(context, PROCEED_ERROR_CODE);
    }

    std::string buildDir = ark::os::GetParentDir(paths.configPath);
    if (chdir(buildDir.c_str()) != 0) {
        std::cerr << "Failed to chdir to build dir: " << buildDir << std::endl;
        return DestroyContextAndReturn(context, TEST_ERROR_CODE);
    }

    if (mkdir("har2", DIR_MODE) != 0 && errno != EEXIST) {
        std::cerr << "Failed to create output dir: har2" << std::endl;
        return DestroyContextAndReturn(context, TEST_ERROR_CODE);
    }

    std::string outputPath = "har2/MainPage.etscache";
    int result = impl->GenerateStaticDeclarationsFromContext(context, outputPath.c_str());
    if (result != 0) {
        std::cerr << "FAILED TO GENERATE DECLARATIONS (result=" << result << ")" << std::endl;
        return DestroyContextAndReturn(context, TEST_ERROR_CODE);
    }

    std::string actualContent;
    if (!ReadFile(outputPath, actualContent)) {
        std::cerr << "Failed to read generated declaration: " << outputPath << std::endl;
        return DestroyContextAndReturn(context, TEST_ERROR_CODE);
    }

    std::string expectedContent;
    if (!ReadFile(paths.expectedPath, expectedContent)) {
        std::cerr << "Failed to read expected file: " << paths.expectedPath << std::endl;
        return DestroyContextAndReturn(context, TEST_ERROR_CODE);
    }

    impl->DestroyContext(context);
    return CompareAndReport(actualContent, expectedContent, paths.expectedPath, outputPath);
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

    TestPaths paths;
    if (!ResolveTestPaths(argc, argv, paths)) {
        std::cerr << "No --arktsconfig found in argv" << std::endl;
        return TEST_ERROR_CODE;
    }

    auto stringArgs = BuildConfigArgs(argc, argv, paths.configPath);
    std::vector<const char *> args;
    args.reserve(stringArgs.size());
    for (const auto &arg : stringArgs) {
        args.push_back(arg.c_str());
    }
    auto *config = impl->CreateConfig(static_cast<int>(args.size()), args.data());
    if (config == nullptr) {
        std::cerr << "FAILED TO CREATE CONFIG" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    int result = GenerateAndCompare(config, paths);
    impl->DestroyConfig(config);
    return result;
}

// NOLINTEND
