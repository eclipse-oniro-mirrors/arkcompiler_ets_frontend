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
#include "abc2program/abc2program_driver.h"
#include "test/utils/checker_test.h"
#include "flatbuffers/flatbuffers.h"
#include "schemaMetadataGenerated.h"
#include "util/generateBin.h"

namespace ark::es2panda::compiler::test {

using namespace Metadata;

static std::string GetExecDir()
{
    auto es2pandaPath = std::string(::test::utils::PandaExecutablePathGetter::Get()[0]);
    const size_t lastSlashIdx = es2pandaPath.rfind('/');
    ASSERT(lastSlashIdx != std::string::npos);
    return es2pandaPath.substr(0, lastSlashIdx + 1);
}

const static std::string WORKING_DIR = GetExecDir();

class MetadataTestAbc : public testing::Test {
public:
    MetadataTestAbc() = default;

    ~MetadataTestAbc() override = default;

    static void SetUpTestCase()
    {
        if (!ScopedAllocatorsManager::IsInitialized()) {
            ScopedAllocatorsManager::Initialize();
        }
    }

    static void AssertClassPresented(const pandasm::Program *program, const std::string &expectedClassName)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);

        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() == expectedClassName) {
                return;
            }
        }

        FAIL() << "Class " + expectedClassName + " not found in the metadata";
    }

private:
    NO_COPY_SEMANTIC(MetadataTestAbc);
    NO_MOVE_SEMANTIC(MetadataTestAbc);
};

TEST_F(MetadataTestAbc, metadata_abc2program)
{
    const std::string sourceFilename = WORKING_DIR + "dummy.ets";

    std::string text = R"(
    export class MyClass {
        x: number = 11

        constructor() {}
        constructor(x: number) {}
        constructor(p1: number, p2: string, p3: string) {}
    })";
    std::ofstream etsFile(sourceFilename);
    etsFile << text << std::endl;
    etsFile.close();

    const std::string abcFilename = WORKING_DIR + "dummy.abc";
    auto diagnosticEngine = util::DiagnosticEngine();
    auto options = std::make_unique<util::Options>(WORKING_DIR, diagnosticEngine);
    constexpr auto argsNumber = 4;
    const auto outputParam = "--output=" + abcFilename;
    std::array<const char *const, argsNumber> args = {
        {"--extension=ets", outputParam.c_str(), sourceFilename.c_str(), "--emit-metadata"}};
    options->Parse(Span(args.begin(), argsNumber));

    Compiler compiler(options->GetExtension(), options->GetThread(), {});
    SourceFile input(sourceFilename, text);
    auto programs = compiler.Compile(input, *options.get(), diagnosticEngine);
    auto report = [&diagnosticEngine](const diagnostic::DiagnosticKind &kind,
                                      const util::DiagnosticMessageParams &params) {
        diagnosticEngine.LogDiagnostic(kind, params);
    };

    ASSERT_EQ(util::GenerateBinaryFile(programs[abcFilename].get(), abcFilename, *options, report), 0)
        << "Generating program `dummy.abc` failed";

    abc2program::Abc2ProgramDriver driver;
    ASSERT_EQ(driver.Compile(abcFilename), true) << "`dummy.abc` is failed to load by abc2program";
    auto prog = &(driver.GetProgram());

    AssertClassPresented(prog, "MyClass");
}

TEST_F(MetadataTestAbc, metadata_disabled)
{
    const std::string sourceFilename = WORKING_DIR + "dummy.ets";

    std::string text = R"(
    export class MyClass {
        x: number = 11

        constructor() {}
        constructor(x: number) {}
        constructor(p1: number, p2: string, p3: string) {}
    })";
    std::ofstream etsFile(sourceFilename);
    etsFile << text << std::endl;
    etsFile.close();

    const std::string abcFilename = WORKING_DIR + "dummy.abc";
    const auto outputParam = "--output=" + abcFilename;
    auto diagnosticEngine = util::DiagnosticEngine();
    auto options = std::make_unique<util::Options>(WORKING_DIR, diagnosticEngine);
    constexpr auto argsNumber = 3;
    std::array<const char *const, argsNumber> args = {{"--extension=ets", outputParam.c_str(), sourceFilename.c_str()}};
    options->Parse(Span(args.begin(), argsNumber));

    Compiler compiler(options->GetExtension(), options->GetThread(), {});
    SourceFile input(sourceFilename, text);
    auto programs = compiler.Compile(input, *options.get(), diagnosticEngine);
    auto report = [&diagnosticEngine](const diagnostic::DiagnosticKind &kind,
                                      const util::DiagnosticMessageParams &params) {
        diagnosticEngine.LogDiagnostic(kind, params);
    };

    ASSERT_EQ(util::GenerateBinaryFile(programs[abcFilename].get(), abcFilename, *options, report), 0);

    abc2program::Abc2ProgramDriver driver;
    ASSERT_EQ(driver.Compile(abcFilename), true) << "`dummy.abc` is failed to load by abc2program";
    auto prog = &(driver.GetProgram());
    ASSERT_EQ(GetRoot(prog->metadata.data()), nullptr)
        << "Metadata shouldn't be emitted because it's disabled explicitly";
}

}  // namespace ark::es2panda::compiler::test