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

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "assembler/assembly-program.h"
#include "es2panda.h"
#include "util/diagnosticEngine.h"
#include "util/eheap.h"
#include "util/options.h"

using namespace ark;
using namespace ark::es2panda;

namespace {

std::string g_arktsconfig;

const char *LIB_SOURCE = "export function helper(): int { return 42; }\n";
const char *MAIN_SOURCE = "import { helper } from './lib'\nfunction main(): int { return helper(); }\n";

bool WriteFile(const std::string &path, const char *content)
{
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) {
        return false;
    }
    f << content;
    return true;
}

bool FileNonEmpty(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        return false;
    }
    return f.seekg(0, std::ios::end).tellg() > 0;
}

std::string ReadSource(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

std::string MakeTestDir()
{
    std::string dir = "/tmp/es2panda_program_sink_" + std::to_string(::getpid());
    std::string remove = "rm -rf " + dir;
    (void)system(remove.c_str());
    mkdir(dir.c_str(), S_IRWXU);
    WriteFile(dir + "/lib.ets", LIB_SOURCE);
    WriteFile(dir + "/main.ets", MAIN_SOURCE);
    WriteFile(dir + "/arktsconfig.json", R"({"compilerOptions": {"paths": {}}})");
    return dir;
}

void RemoveTestDir(const std::string &dir)
{
    std::string remove = "rm -rf " + dir;
    (void)system(remove.c_str());
}

class TestDir {
public:
    TestDir() : dir_(MakeTestDir()) {}

    ~TestDir()
    {
        RemoveTestDir(dir_);
    }

    const std::string &Path() const
    {
        return dir_;
    }

    // Number of non-empty files in the directory.
    size_t CountFiles() const
    {
        size_t count = 0;
        std::string cmd = "find " + dir_ + " -maxdepth 1 -type f -size +0 | wc -l";
        FILE *pipe = popen(cmd.c_str(), "r");
        if (pipe == nullptr) {
            return 0;
        }
        char buffer[32];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            count = static_cast<size_t>(atoi(buffer));
        }
        pclose(pipe);
        return count;
    }

private:
    std::string dir_;
};

bool CompileMain(const std::string &dir, const std::string &arktsconfig, es2panda::ProgramSink *sink,
                 util::DiagnosticEngine *de,
                 std::unordered_map<std::string, std::unique_ptr<pandasm::Program>> *programs)
{
    std::string sourcePath = dir + "/main.ets";
    std::vector<std::string> argStorage = {"es2panda",
                                           "--extension=ets",
                                           "--simultaneous",
                                           "--incremental",
                                           "--opt-level=0",
                                           "--thread=0",
                                           "--arktsconfig=" + arktsconfig,
                                           "--output=" + dir,
                                           sourcePath};
    std::vector<const char *> args;
    for (const auto &a : argStorage) {
        args.push_back(a.c_str());
    }

    util::Options options("es2panda", *de);
    if (!options.Parse(ark::Span<const char *const>(args.data(), args.size()))) {
        de->FlushDiagnostic();
        return false;
    }

    std::string source = ReadSource(sourcePath);
    std::string output = options.GetOutput();
    es2panda::SourceFile input(sourcePath, source, options.IsModule(), output);
    es2panda::Compiler compiler(ScriptExtension::ETS, options.GetThread());
    *programs =
        sink == nullptr ? compiler.Compile(input, options, *de) : compiler.Compile(input, options, *de, 0, *sink);
    return true;
}

int RunWithSink()
{
    TestDir dir;
    util::DiagnosticEngine de;
    std::unordered_map<std::string, std::unique_ptr<pandasm::Program>> programs;

    std::vector<std::string> abcPaths;
    es2panda::ProgramSink sink = [&abcPaths](pandasm::Program *program, std::string_view abcPath) {
        (void)program;
        abcPaths.emplace_back(abcPath);
        return WriteFile(std::string(abcPath), "written by sink");
    };
    if (!CompileMain(dir.Path(), g_arktsconfig, &sink, &de, &programs)) {
        std::cerr << "FAIL [sink]: compilation failed" << std::endl;
        return 1;
    }
    if (!programs.empty()) {
        std::cerr << "FAIL [sink]: program map must be empty in SIMULTANEOUS_INCREMENTAL mode" << std::endl;
        return 1;
    }
    if (abcPaths.size() != 1U) {
        // The loop iterates over the 'direct' external sources: lib.ets (the main
        // source itself is not delivered through the sink in this mode).
        std::cerr << "FAIL [sink]: expected 1 sink invocation for the imported file, got " << abcPaths.size()
                  << std::endl;
        return 1;
    }
    for (const auto &path : abcPaths) {
        if (path.empty() || !FileNonEmpty(path)) {
            std::cerr << "FAIL [sink]: no file written at reported path: '" << path << "'" << std::endl;
            return 1;
        }
    }
    std::cout << "PASS [sink-per-file]" << std::endl;
    return 0;
}

int RunWithoutSink()
{
    TestDir dir;
    util::DiagnosticEngine de;
    std::unordered_map<std::string, std::unique_ptr<pandasm::Program>> programs;

    if (!CompileMain(dir.Path(), g_arktsconfig, nullptr, &de, &programs)) {
        std::cerr << "FAIL [no-sink]: compilation failed" << std::endl;
        return 1;
    }
    if (de.IsAnyError()) {
        std::cerr << "FAIL [no-sink]: compilation must finish without errors" << std::endl;
        de.FlushDiagnostic();
        return 1;
    }
    if (dir.CountFiles() != 3U) {  // lib.ets, main.ets and arktsconfig.json prepared above
        std::cerr << "FAIL [no-sink]: no files must be written without a sink, got " << dir.CountFiles() << std::endl;
        return 1;
    }
    std::cout << "PASS [no-sink-no-output]" << std::endl;
    return 0;
}

int RunInterruptingSink()
{
    TestDir dir;
    util::DiagnosticEngine de;
    std::unordered_map<std::string, std::unique_ptr<pandasm::Program>> programs;

    size_t calls = 0;
    es2panda::ProgramSink sink = [&calls](pandasm::Program *program, std::string_view abcPath) {
        (void)program;
        (void)abcPath;
        calls++;
        return false;  // stop processing of the remaining files
    };

    if (!CompileMain(dir.Path(), g_arktsconfig, &sink, &de, &programs)) {
        std::cerr << "FAIL [interrupt]: compilation failed" << std::endl;
        return 1;
    }
    if (calls != 1U) {
        std::cerr << "FAIL [interrupt]: sink must not be called again after returning 'false', got " << calls
                  << std::endl;
        return 1;
    }
    std::cout << "PASS [interrupt-stops-remaining]" << std::endl;
    return 0;
}

}  // namespace

int main(int argc, char **argv)
{
    // The compiler allocates through the scoped allocators machinery; it must
    // be initialized for the process (the aot entry does the same).
    ScopedAllocatorsManager::Initialize();

    constexpr std::string_view arktsconfigFlag = "--arktsconfig=";
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], arktsconfigFlag.data(), arktsconfigFlag.length()) == 0) {
            g_arktsconfig = std::string(argv[i] + arktsconfigFlag.length());
            break;
        }
    }
    if (g_arktsconfig.empty()) {
        // Fall back to the config generated next to the test binary.
        g_arktsconfig = "arktsconfig.json";
    }

    int rc = RunWithSink();
    rc |= RunWithoutSink();
    rc |= RunInterruptingSink();
    ScopedAllocatorsManager::Finalize();
    if (rc != 0) {
        return 1;
    }
    std::cout << "ALL DONE" << std::endl;
    return 0;
}
