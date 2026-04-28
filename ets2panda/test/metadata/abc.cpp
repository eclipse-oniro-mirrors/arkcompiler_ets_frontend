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
#include "test/utils/metadata_test.h"
#include "flatbuffers/flatbuffers.h"
#include "schemaMetadataGenerated.h"
#include "utils/assertions.h"

namespace ark::es2panda::compiler::test {

using namespace Metadata;
using namespace metadata_test;

class MetadataTestAbc : public ::test::utils::MetadataTest {
public:
    MetadataTestAbc() = default;
    ~MetadataTestAbc() override = default;

private:
    NO_COPY_SEMANTIC(MetadataTestAbc);
    NO_MOVE_SEMANTIC(MetadataTestAbc);
};

TEST_F(MetadataTestAbc, abc2program)
{
    const auto abcPath = workingDir + "app.abc";
    Compile(std::string(TEST_DATA_PATH) + "abc/common.ets", abcPath);

    abc2program::Abc2ProgramDriver driver;
    ASSERT_EQ(driver.Compile(abcPath), true) << abcPath << " is failed to load by abc2program";
    MetadataAssertions::AssertClassPresented(&(driver.GetProgram()), "MyClass");
}

TEST_F(MetadataTestAbc, metadata_disabled)
{
    const auto abcPath = workingDir + "app.abc";
    Compile<false>(std::string(TEST_DATA_PATH) + "abc/common.ets", abcPath);

    abc2program::Abc2ProgramDriver driver;
    ASSERT_EQ(driver.Compile(abcPath), true) << abcPath << " is failed to load by abc2program";
    ASSERT_EQ(GetRoot(driver.GetProgram().metadata.data()), nullptr)
        << "Metadata shouldn't be emitted because it's disabled explicitly";
}

}  // namespace ark::es2panda::compiler::test