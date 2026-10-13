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

#include <algorithm>
#include <cstdint>
#include <regex>
#include <string>
#include <vector>
#include "abc2program/abc2program_driver.h"
#include "assembly-function.h"
#include "assembly-parser.h"
#include "test/utils/metadata_test.h"
#include "flatbuffers/flatbuffers.h"
#include "schemaMetadataGenerated.h"
#include "util/importPathManager.h"
#include "util/perfMetrics.h"
#include "utils/assertions.h"

namespace ark::es2panda::compiler::test {

using namespace Metadata;
using namespace metadata_test;

class MetadataTestImport : public ::test::utils::MetadataTest {
public:
    MetadataTestImport() = default;
    ~MetadataTestImport() override = default;

private:
    NO_COPY_SEMANTIC(MetadataTestImport);
    NO_MOVE_SEMANTIC(MetadataTestImport);
};

TEST_F(MetadataTestImport, basic_calls)
{
    const auto testDataDir = std::string(TEST_DATA_PATH) + "import/" + test_info_->name();
    CompileLibToImport(testDataDir + "/lib.ets", workingDir + "lib.abc");
    Compile(testDataDir + "/main.ets", workingDir + "main.abc");
}

TEST_F(MetadataTestImport, complex_calls)
{
    const auto testDataDir = std::string(TEST_DATA_PATH) + "import/" + test_info_->name();
    CompileLibToImport(testDataDir + "/lib.ets", workingDir + "lib.abc");
    Compile(testDataDir + "/main.ets", workingDir + "main.abc");
}

TEST_F(MetadataTestImport, generic_alias_arguments)
{
    const auto testDataDir = std::string(TEST_DATA_PATH) + "import/" + test_info_->name();
    CompileLibToImport(testDataDir + "/lib.ets", workingDir + "lib.abc");
    Compile(testDataDir + "/main.ets", workingDir + "main.abc");
}

static bool ContainsClassDecl(const Metadata::Decls *decls, const std::string &className)
{
    if (decls == nullptr) {
        return false;
    }
    for (const auto classDecl : *decls->classes()) {
        if (classDecl->name()->str() == className) {
            return true;
        }
    }
    return false;
}

static bool MetadataContainsClassDecl(const pandasm::Program &program, const std::string &className)
{
    for (const auto &packageMetadata : program.metadata) {
        for (const auto &moduleMetadata : packageMetadata.second) {
            if (ContainsClassDecl(GetDecls(moduleMetadata.second.data()), className)) {
                return true;
            }
        }
    }
    return false;
}

// Regression test: compiling a module that source-imports another module must not crash metadata
// serialization. Metadata is emitted for the compiled module only: source dependencies are lazily
// checked here and must be serialized by their own compilation. Only a non-enum symbol is imported
// on purpose: using the enum itself would force its lazy type resolution and mask the issue.
TEST_F(MetadataTestImport, enum_in_source_imported_module)
{
    const auto testDataDir = std::string(TEST_DATA_PATH) + "import/" + test_info_->name();
    AddPathToConfig("enum_lib", testDataDir + "/lib.ets");
    const auto mainAbcPath = workingDir + "main.abc";
    Compile(testDataDir + "/main.ets", mainAbcPath);

    abc2program::Abc2ProgramDriver driver;
    ASSERT_EQ(driver.Compile(mainAbcPath), true) << mainAbcPath << " is failed to load by abc2program";

    const auto &program = driver.GetProgram();
    const bool ownExportFound = MetadataContainsClassDecl(program, "Consumer");
    const bool importedEnumFound = MetadataContainsClassDecl(program, "MyEnum");
    ASSERT_TRUE(ownExportFound) << "Export of the compiled module is not found in the emitted metadata";
    ASSERT_FALSE(importedEnumFound) << "Imported source module must be serialized by its own compilation";
}

TEST_F(MetadataTestImport, alias_to_imported_type)
{
    const auto testDataDir = std::string(TEST_DATA_PATH) + "import/" + test_info_->name();
    CompileLibToImport(testDataDir + "/widget.ets", workingDir + "widget.abc", "widget");
    CompileLibToImport(testDataDir + "/alias.ets", workingDir + "alias.abc", "alias");
    Compile(testDataDir + "/main.ets", workingDir + "main.abc");
}

TEST_F(MetadataTestImport, this_return_type)
{
    const auto testDataDir = std::string(TEST_DATA_PATH) + "import/" + test_info_->name();
    CompileLibToImport(testDataDir + "/lib.ets", workingDir + "lib.abc");
    Compile(testDataDir + "/main.ets", workingDir + "main.abc");
}

TEST_F(MetadataTestImport, cached_program_materializes_members)
{
    const auto testDataDir = std::string(TEST_DATA_PATH) + "import/" + test_info_->name();
    CompileLibToImport(testDataDir + "/lib.ets", workingDir + "lib.abc");

    SetExternalContext();
    ASSERT_NE(RunCheckerWithMetadata(testDataDir + "/main.ets"), nullptr);

    bool classMaterialized = false;
    bool interfaceMaterialized = false;
    Program()->GetExternalPrograms()->Visit([&classMaterialized, &interfaceMaterialized](auto *program) {
        program->Ast()->IterateRecursively([&classMaterialized, &interfaceMaterialized](ir::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("CachedClass")) {
                classMaterialized = !node->AsClassDefinition()->Body().empty();
                return;
            }

            if (node->IsTSInterfaceDeclaration() &&
                node->AsTSInterfaceDeclaration()->Id()->Name().Is("CachedInterface")) {
                interfaceMaterialized = !node->AsTSInterfaceDeclaration()->Body()->Body().empty();
            }
        });
    });

    EXPECT_TRUE(classMaterialized);
    EXPECT_TRUE(interfaceMaterialized);
}

TEST_F(MetadataTestImport, from_stdlib)
{
    const auto testDataDir = std::string(TEST_DATA_PATH) + "import/" + test_info_->name();
    Compile(testDataDir + "/main.ets", workingDir + "main.abc");
    util::DumpPerfMetrics();
}

TEST_F(MetadataTestImport, invalid_flatbuffer_is_rejected)
{
    flatbuffers::FlatBufferBuilder builder;
    FinishDeclsBuffer(builder, CreateDecls(builder));

    panda_file::MetadataByModules metadata;
    metadata["lib"].assign(builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize());
    ASSERT_TRUE(util::VerifyMetadataModules(metadata));

    auto &moduleMetadata = metadata.begin()->second;
    ASSERT_GE(moduleMetadata.size(), sizeof(uint32_t));
    std::fill_n(moduleMetadata.begin(), sizeof(uint32_t), static_cast<uint8_t>(0xFF));
    ASSERT_FALSE(util::VerifyMetadataModules(metadata));
}
}  // namespace ark::es2panda::compiler::test
