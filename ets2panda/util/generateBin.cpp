/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "generateBin.h"
#include "bytecode_optimizer/bytecodeopt_options.h"
#include "bytecode_optimizer/optimize_bytecode.h"
#include "compiler/compiler_logger.h"
#include "compiler/compiler_options.h"
#include "util/options.h"
#include "util/perfMetrics.h"

namespace ark::es2panda::util {

[[maybe_unused]] static void InitializeLogging(const util::Options &options)
{
    ark::Logger::ComponentMask componentMask;
    componentMask.set(ark::Logger::Component::ASSEMBLER);
    componentMask.set(ark::Logger::Component::COMPILER);
    componentMask.set(ark::Logger::Component::BYTECODE_OPTIMIZER);

    if (!Logger::IsInitialized()) {
        Logger::InitializeStdLogging(options.LogLevel(), componentMask);
    } else {
        Logger::EnableComponent(componentMask);
    }
}

#ifdef PANDA_WITH_BYTECODE_OPTIMIZER
static bool OptimizeBytecode(ark::pandasm::Program *prog, const std::string &output, const util::Options &options,
                             const ReporterFun &reporter)
{
    InitializeLogging(options);
    ark::pandasm::AsmEmitter::PandaFileToPandaAsmMaps maps {};

    // NOTE(mshimenkov): AsmEmitter is called mainly to fill PandaFileToPandaAsmMaps map that is used in bytecode
    // optimizer later
    if (!ark::pandasm::AsmEmitter::Emit(output, *prog, nullptr, &maps, true)) {
        reporter(diagnostic::EMIT_FAILED, {ark::pandasm::AsmEmitter::GetLastError()});
        return false;
    }

    ark::bytecodeopt::g_options.SetOptLevel(options.GetOptLevel());
    // Set default value instead of maximum set in ark::bytecodeopt::SetCompilerOptions()
    ark::compiler::CompilerLogger::Init({"all"});
    ark::compiler::g_options.SetCompilerMaxBytecodeSize(ark::compiler::g_options.GetCompilerMaxBytecodeSize());
    return ark::bytecodeopt::OptimizeBytecode(prog, &maps, output, options.IsDynamic(), true);
}
#endif

static void DumpStatistics(const std::map<std::string, size_t> &stat)
{
    size_t totalSize = 0;
    std::cout << "Panda file size statistic:" << std::endl;
    constexpr std::array<std::string_view, 2> INFO_STATS = {"instructions_number", "codesize"};

    for (const auto &[name, size] : stat) {
        if (find(INFO_STATS.begin(), INFO_STATS.end(), name) != INFO_STATS.end()) {
            continue;
        }
        std::cout << name << " section: " << size << std::endl;
        totalSize += size;
    }

    for (const auto &name : INFO_STATS) {
        std::cout << name << ": " << stat.at(std::string(name)) << std::endl;
    }

    std::cout << "total: " << totalSize << std::endl;
}

static int EmitBytecodeToBinaryFile(ark::pandasm::Program *prog, const std::string &output,
                                    const util::Options &options, const ReporterFun &reporter)
{
    if (options.IsDumpAssembly()) {
        es2panda::Compiler::DumpAsm(prog);
    }

    if (!ark::pandasm::AsmEmitter::AssignProfileInfo(prog)) {
        reporter(diagnostic::ASSIGN_PROFILE_INFO_FAILED, {});
        return 1;
    }

    std::map<std::string, size_t> stat;
    std::map<std::string, size_t> *statp = nullptr;
    if ((options.GetOptLevel() != 0) && options.IsDumpSizeStat()) {
        statp = &stat;
    }

    if (!ark::pandasm::AsmEmitter::Emit(output, *prog, statp, nullptr, true)) {
        reporter(diagnostic::EMIT_FAILED, {ark::pandasm::AsmEmitter::GetLastError()});
        return 1;
    }

    if (options.IsDumpSizeStat()) {
        DumpStatistics(stat);
    }

    return 0;
}

int GenerateBinaryFiles(std::unordered_map<std::string, std::unique_ptr<ark::pandasm::Program>> &progs,
                        const util::Options &options, const ReporterFun &reporter)
{
    for (auto &[abcFile, prog] : progs) {
        auto progParentDir = ark::os::GetParentDir(abcFile);
        fs::create_directories(progParentDir);

        if (GenerateBinaryFile(prog.get(), abcFile, options, reporter)) {
            return 1;
        }
    }

    return 0;
}

int GenerateBinaryFile(ark::pandasm::Program *prog, const std::string &output, const util::Options &options,
                       const ReporterFun &reporter)
{
    ES2PANDA_PERF_SCOPE("@GenerateBinaryFile");

#ifdef PANDA_WITH_BYTECODE_OPTIMIZER
    {
        ES2PANDA_PERF_SCOPE("@GenerateBinaryFile/OptimizeBytecode");
        if ((options.GetOptLevel() != 0)) {
            // Bytecode optimizer may fail
            // Ignore the result
            OptimizeBytecode(prog, output, options, reporter);
        }
    }
#endif

    return EmitBytecodeToBinaryFile(prog, output, options, reporter);
}
}  // namespace ark::es2panda::util
//
