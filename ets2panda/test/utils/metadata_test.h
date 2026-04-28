/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_TEST_UTILS_METADATA_TEST_H
#define ES2PANDA_TEST_UTILS_METADATA_TEST_H

#include "parser/program/ImportCache.h"
#include "checker_test.h"
#include "libarkbase/utils/span.h"
#include "util/generateBin.h"

namespace test::utils {

using namespace ark;
using namespace ark::es2panda;

class MetadataTest : public CheckerTest {
public:
    MetadataTest() = default;

    ~MetadataTest() override = default;

    static void SetUpTestCase()
    {
        if (!ScopedAllocatorsManager::IsInitialized()) {
            ScopedAllocatorsManager::Initialize();
        }
    }

    void SetUp() override
    {
        parser::ImportCache<parser::CacheType::METADATA>::ActivateCache();
        EnableMetadataEmitting();
        SetWorkingDirWithConfig();
    }

protected:
    const static std::string execDir;
    const static std::string workingDir;
    const static std::string configName;

    template <bool METADATA_EMITTING_ENABLED = true, bool METADATA_READING_ENABLED = true>
    static void Compile(const std::string &sourceFilePath, const std::string &abcPath)
    {
        const auto filename = fs::path(sourceFilePath).filename();
        std::ifstream file(sourceFilePath);
        std::string sourceCode((std::istreambuf_iterator(file)), std::istreambuf_iterator<char>());

        auto diagnosticEngine = util::DiagnosticEngine();
        auto options = std::make_unique<util::Options>(workingDir, diagnosticEngine);
        const auto outputParam = "--output=" + abcPath;
        std::vector args = {{"--extension=ets", outputParam.c_str(), sourceFilePath.c_str()}};
        if constexpr (METADATA_EMITTING_ENABLED) {
            args.emplace_back("--emit-metadata");
        }
        if constexpr (METADATA_READING_ENABLED) {
            args.emplace_back("--read-metadata");
        }
        EXPECT_EQ(options->Parse(Span(&(*args.cbegin()), args.size())), true)
            << "Compilation options are failed to parse";

        Compiler compiler(options->GetExtension(), options->GetThread(), {});
        auto programs = compiler.Compile(SourceFile(filename.c_str(), sourceCode), *options.get(), diagnosticEngine);
        auto report = [&diagnosticEngine](const diagnostic::DiagnosticKind &kind,
                                          const util::DiagnosticMessageParams &params) {
            diagnosticEngine.LogDiagnostic(kind, params);
        };

        EXPECT_EQ(util::GenerateBinaryFile(programs[abcPath].get(), abcPath, *options, report), 0)
            << "Generating program " << abcPath << " failed";
    }

    static void CompileLibToImport(const std::string &sourceFilePath, const std::string &abcFilename)
    {
        Compile(sourceFilePath, abcFilename);
        AddDependencyToConfig("lib", abcFilename);
    }

    std::unique_ptr<pandasm::Program> RunCheckerWithMetadata(const std::string &sourceFilePath)
    {
        const auto fileIfStream = std::ifstream(sourceFilePath);
        EXPECT_EQ(fileIfStream.good(), true) << "Source file " << sourceFilePath << " not found.";
        std::ostringstream sourceFileStream;
        sourceFileStream << fileIfStream.rdbuf();
        parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
        auto program = RunCheckerWithCustomFunc(fs::path(sourceFilePath).filename().string(), sourceFileStream.str(),
                                                []([[maybe_unused]] ir::AstNode *ast) {});
        EXPECT_NE(program, nullptr) << "Couldn't compile a program for " << sourceFilePath;
        return program;
    }

private:
    static std::string GetExecDir()
    {
        const auto es2pandaPath = std::string(PandaExecutablePathGetter::Get()[0]);
        const size_t lastSlashIdx = es2pandaPath.rfind('/');
        ASSERT(lastSlashIdx != std::string::npos);
        return es2pandaPath.substr(0, lastSlashIdx + 1);
    }

    void SetWorkingDirWithConfig()
    {
        fs::create_directory(workingDir);
        SetWorkingDir(workingDir);

        auto newConfigPath = workingDir + configName;
        fs::copy_file(execDir + configName, newConfigPath, fs::copy_options::overwrite_existing);
        std::fstream configFile(newConfigPath, std::ios::in | std::ios::out);
        std::ostringstream rawConfig;
        rawConfig << configFile.rdbuf();

        auto config = std::regex_replace(rawConfig.str(), std::regex(R"(\.\/plugins\/ets)"), "../plugins/ets");
        configFile.seekp(0, std::ios::beg);
        configFile << config << std::endl;
        configFile.close();
    }

    static void AddDependencyToConfig(const std::string &name, const std::string &path)
    {
        std::fstream configFile(workingDir + configName, std::ios::in | std::ios::out);
        std::ostringstream rawConfig;
        rawConfig << configFile.rdbuf();

        auto regex = std::regex(R"("dependencies": \{)");
        auto replacement = R"("dependencies": {")" + name + R"(": { "path": ")" + path + "\" },";
        auto config = std::regex_replace(rawConfig.str(), regex, replacement);
        configFile.seekp(0, std::ios::beg);
        configFile << config << std::endl;
        configFile.close();
    }

    NO_COPY_SEMANTIC(MetadataTest);
    NO_MOVE_SEMANTIC(MetadataTest);
};

}  // namespace test::utils

#endif  // ES2PANDA_TEST_UTILS_METADATA_TEST_H