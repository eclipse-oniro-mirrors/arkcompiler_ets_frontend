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

#include "common.h"
#include "util/arktsconfig.h"
#include "util/diagnosticEngine.h"

#include <fstream>
namespace {

constexpr char EMPTY_INCLUDE_CONFIG[] = "arktsconfig_include_empty.json";
constexpr char STRING_INCLUDE_CONFIG[] = "arktsconfig_include_string.json";
constexpr char NULL_INCLUDE_CONFIG[] = "arktsconfig_include_something.json";
constexpr char CORRECT_INCLUDE_CONFIG[] = "arktsconfig_include_correct.json";
common::Params EmptyIncludeNeg()
{
    return common::Params {R"({
        "include": []
        })",
                           EMPTY_INCLUDE_CONFIG, false};
}

common::Params IncludeStringNeg()
{
    return common::Params {R"({
        "include": "abc"
        })",
                           STRING_INCLUDE_CONFIG, false};
}

common::Params IncludeSomethingNeg()
{
    return common::Params {R"({
        "include": null
        })",
                           NULL_INCLUDE_CONFIG, false};
}

common::Params IncludeCorrect()
{
    return common::Params {R"({
        "include": [
                "foo",
                "bar"
            ]
        })",
                           CORRECT_INCLUDE_CONFIG, true};
}

class ArkTsConfigInclude : public ::testing::TestWithParam<common::Params> {};

TEST_P(ArkTsConfigInclude, CheckInclude)
{
    auto param = GetParam();
    const auto configPath = ark::es2panda::JoinPaths(::testing::TempDir(), param.fileName);
    std::ofstream configFile {configPath};
    ASSERT_TRUE(configFile.is_open());
    configFile << param.config;
    ASSERT_FALSE(configFile.fail());
    configFile.close();

    ark::es2panda::util::DiagnosticEngine de;
    auto config = ark::es2panda::ArkTsConfig {configPath, de};
    ASSERT_EQ(config.Parse(), param.expected);
}

INSTANTIATE_TEST_SUITE_P(ArkTsConfigIncludeSuite, ArkTsConfigInclude,
                         ::testing::Values(EmptyIncludeNeg(), IncludeStringNeg(), IncludeSomethingNeg(),
                                           IncludeCorrect()));

}  // namespace
