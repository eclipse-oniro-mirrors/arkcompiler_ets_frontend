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
#include <regex>
#include <string>
#include <vector>
#include "assembly-function.h"
#include "assembly-parser.h"
#include "test/utils/metadata_test.h"
#include "flatbuffers/flatbuffers.h"
#include "schemaMetadataGenerated.h"
#include "util/perfMetrics.h"
#include "utils/assertions.h"

namespace ark::es2panda::compiler::test {

using namespace Metadata;
using namespace metadata_test;

class MetadataTestImport : public ::test::utils::MetadataTest {
public:
    MetadataTestImport() = default;
    ~MetadataTestImport() override = default;

protected:
    /*
    void CompileLibWithMalformedMetadata(const std::string &sourceFilePath, const std::string &abcFilename)
    {
        CompileWithMutator(sourceFilePath, abcFilename, [](pandasm::Program *program) {
            ASSERT_FALSE(program->metadata.empty()) << "No metadata was emitted for the imported library";
            auto &moduleMetadata = program->metadata.begin()->second.begin()->second;
            ASSERT_GE(moduleMetadata.size(), sizeof(uint32_t)) << "Metadata blob is too small to corrupt";
            std::fill_n(moduleMetadata.begin(), sizeof(uint32_t), static_cast<uint8_t>(0xFF));
        });
        AddDependency("lib", abcFilename);
    }
    */

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

TEST_F(MetadataTestImport, from_stdlib)
{
    const auto testDataDir = std::string(TEST_DATA_PATH) + "import/" + test_info_->name();
    Compile(testDataDir + "/main.ets", workingDir + "main.abc");
    util::DumpPerfMetrics();
}
/*
TEST_F(MetadataTestImport, invalid_flatbuffer_falls_back_to_etscache)
{
    const auto testDataDir = std::string(TEST_DATA_PATH) + "import/" + test_info_->name();
    CompileLibWithMalformedMetadata(testDataDir + "/lib.ets", workingDir + "lib.abc");
    Compile(testDataDir + "/main.ets", workingDir + "main.abc");
}
*/
}  // namespace ark::es2panda::compiler::test
