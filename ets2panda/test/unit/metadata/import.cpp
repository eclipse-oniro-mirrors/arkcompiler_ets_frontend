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

#include <string>
#include <vector>
#include <regex>
#include "assembly-function.h"
#include "assembly-parser.h"
#include "assembly-program.h"
#include "test/utils/checker_test.h"
#include "flatbuffers/flatbuffers.h"
#include "schemaMetadataGenerated.h"
#include "util/generateBin.h"
#include "parser/program/ImportCache.h"

namespace ark::es2panda::compiler::test {

using namespace Metadata;

static std::string GetExecDir()
{
    auto es2pandaPath = std::string(::test::utils::PandaExecutablePathGetter::Get()[0]);
    const size_t lastSlashIdx = es2pandaPath.rfind('/');
    ASSERT(lastSlashIdx != std::string::npos);
    return es2pandaPath.substr(0, lastSlashIdx + 1);
}

const static std::string EXEC_DIR = GetExecDir();
const static std::string WORKING_DIR = EXEC_DIR + "metadata_test/";
const static std::string CONFIG_NAME = "arktsconfig.json";

class MetadataTestImport : public ::test::utils::CheckerTest {
public:
    MetadataTestImport() = default;

    ~MetadataTestImport() override = default;

    static void SetUpTestCase()
    {
        if (!ScopedAllocatorsManager::IsInitialized()) {
            ScopedAllocatorsManager::Initialize();
        }
    }

    void SetWorkingDirWithConfig()
    {
        fs::create_directory(WORKING_DIR);
        SetWorkingDir(WORKING_DIR);

        auto newConfigPath = WORKING_DIR + CONFIG_NAME;
        fs::copy_file(EXEC_DIR + CONFIG_NAME, newConfigPath, fs::copy_options::overwrite_existing);
        std::fstream configFile(newConfigPath, std::ios::in | std::ios::out);

        std::ostringstream rawConfig;
        rawConfig << configFile.rdbuf();
        std::string config = rawConfig.str();
        config = std::regex_replace(config, std::regex(R"(\.\/plugins\/ets)"), "../plugins/ets");
        configFile.seekp(0, std::ios::beg);
        configFile << config << std::endl;
        configFile.close();
    }

    static void AppendDependency(std::string name, std::string path)
    {
        std::fstream configFile(WORKING_DIR + CONFIG_NAME, std::ios::in | std::ios::out);

        std::ostringstream rawConfig;
        rawConfig << configFile.rdbuf();
        std::string config = rawConfig.str();
        config = std::regex_replace(config, std::regex(R"("dependencies": \{)"),
                                    "\"dependencies\": {\"" + name + "\": { \"path\": \"" + path + "\" },");
        configFile.seekp(0, std::ios::beg);
        configFile << config << std::endl;
        configFile.close();
    }

    static void AssertClassPresented(const parser::MetadataCacheType &metadata, const std::string &expectedClassName)
    {
        const auto root = GetRoot(metadata->data());

        ASSERT_NE(root, nullptr);

        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() == expectedClassName) {
                return;
            }
        }

        FAIL() << "Class " + expectedClassName + " not found in the metadata";
    }

private:
    NO_COPY_SEMANTIC(MetadataTestImport);
    NO_MOVE_SEMANTIC(MetadataTestImport);
};

TEST_F(MetadataTestImport, metadata_check_metadata_based_program)
{
    parser::ImportCache<parser::CacheType::METADATA>::ActivateCache();
    SetWorkingDirWithConfig();
    EnableMetadataEmitting();

    std::string libText = R"(
    export class MyClass {
        x: number = 11

        constructor() {}
        constructor(x: number) {}
        constructor(p1: number, p2: string, p3: string) {}
    })";
    const std::string sourceFilename = WORKING_DIR + "lib.ets";
    const std::string abcFilename = WORKING_DIR + "lib.abc";
    std::ofstream libFile(sourceFilename);
    libFile << libText << std::endl;
    libFile.close();

    auto diagnosticEngine = util::DiagnosticEngine();
    auto options = std::make_unique<util::Options>(WORKING_DIR, diagnosticEngine);
    constexpr auto argsNumber = 4;
    const auto outputParam = "--output=" + abcFilename;
    std::array<const char *const, argsNumber> args = {
        {"--extension=ets", outputParam.c_str(), sourceFilename.c_str(), "--emit-metadata"}};
    options->Parse(Span(args.begin(), argsNumber));

    Compiler compiler(options->GetExtension(), options->GetThread(), {});
    auto libPrograms = compiler.Compile(SourceFile(sourceFilename, libText), *options.get(), diagnosticEngine);
    auto report = [&diagnosticEngine](const diagnostic::DiagnosticKind &kind,
                                      const util::DiagnosticMessageParams &params) {
        diagnosticEngine.LogDiagnostic(kind, params);
    };

    ASSERT_EQ(util::GenerateBinaryFile(libPrograms[abcFilename].get(), abcFilename, *options, report), 0)
        << "Failed to generate a program with metadata emitted";

    AppendDependency("lib", abcFilename);

    std::string mainText = R"(
    import { MyClass } from "lib"

    export class MyClass2 {
        x: number = 1
    })";
    const std::string mainSourceFilename = WORKING_DIR + "main.ets";

    ASSERT_NE(RunCheckerWithCustomFunc(mainSourceFilename, mainText, []([[maybe_unused]] ir::AstNode *ast) {}),
              nullptr);

    auto directDeps = Program()->GetExternalSources()->Direct();

    ASSERT_GT(directDeps.size(), 0) << "There are no external dependencies in the program with metadata-based import";
    ASSERT_NE(directDeps.find("lib."), directDeps.end())
        << "There are no `lib` dependency provided in the program with metadata-based import";
    ASSERT_EQ(GetAnyError().GetId(), diagnostic::UNSUPPORTED_IMPORT_WITH_METADATA.Id())
        << "`UNSUPPORTED_IMPORT_WITH_METADATA` error should be reported as metadata-based imports are not supported "
           "yet";

    auto metadata = directDeps["lib."]->GetImportInfo().DataFor<parser::CacheType::METADATA>();
    AssertClassPresented(metadata, "MyClass");
}

}  // namespace ark::es2panda::compiler::test