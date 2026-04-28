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
#include "test/utils/metadata_test.h"
#include "flatbuffers/flatbuffers.h"
#include "schemaMetadataGenerated.h"
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

TEST_F(MetadataTestImport, metadata_not_supported)
{
    const auto testDataDir = std::string(TEST_DATA_PATH) + "import/" + test_info_->name();

    CompileLibToImport(testDataDir + "/lib.ets", workingDir + "lib.abc");
    // Reading of metadata is disabled by default in `RunCheckerWithMetadata`
    ASSERT_NE(RunCheckerWithMetadata(testDataDir + "/main.ets"), nullptr);

    ASSERT_EQ(GetAnyError().GetId(), diagnostic::UNSUPPORTED_IMPORT_WITH_METADATA.Id())
        << "`UNSUPPORTED_IMPORT_WITH_METADATA` error should be reported as reading of metadata is not enabled";
}

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

}  // namespace ark::es2panda::compiler::test