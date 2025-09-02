/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
namespace {

// NOTE(morlovsky): change to non-sideeffect ones (without opening the file)
// This will help with this nonsense of file opening on every parse
common::Params EmptyIncludeNeg()
{
    return common::Params {
        R"({
        "include": []
        })",
        ark::es2panda::JoinPaths(common::CurrentSourceFileDir(), "arktsconfig_include_empty.json"), false};
}

common::Params IncludeStringNeg()
{
    return common::Params {
        R"({
        "include": "abc"
        })",
        ark::es2panda::JoinPaths(common::CurrentSourceFileDir(), "arktsconfig_include_string.json"), false};
}

common::Params IncludeSomethingNeg()
{
    return common::Params {
        R"({
        "include": null
        })",
        ark::es2panda::JoinPaths(common::CurrentSourceFileDir(), "arktsconfig_include_something.json"), false};
}

common::Params IncludeCorrect()
{
    return common::Params {
        R"({
        "include": [
                "foo",
                "bar"
            ]
        })",
        ark::es2panda::JoinPaths(common::CurrentSourceFileDir(), "arktsconfig_include_correct.json"), true};
}

class ArkTsConfigInclude : public ::testing::TestWithParam<common::Params> {};

TEST_P(ArkTsConfigInclude, CheckInclude)
{
    auto param = GetParam();
    ark::es2panda::util::DiagnosticEngine de;
    auto config = ark::es2panda::ArkTsConfig {*param.path, de};
    ASSERT_EQ(config.Parse(), param.expected);
}

INSTANTIATE_TEST_SUITE_P(ArkTsConfigIncludeSuite, ArkTsConfigInclude,
                         ::testing::Values(EmptyIncludeNeg(), IncludeStringNeg(), IncludeSomethingNeg(),
                                           IncludeCorrect()));

}  // namespace
