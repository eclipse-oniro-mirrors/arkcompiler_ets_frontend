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

#include "common.h"
#include "util/arktsconfig.h"
#include "util/diagnosticEngine.h"

#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct ExtDeclParams {
    std::string config;
    std::string fileName;
    bool parseExpected;
    // module name -> expected 'IsExternalDeclPackage' result
    std::vector<std::pair<std::string, bool>> lookups;

    friend void PrintTo(const ExtDeclParams &params, std::ostream *os)
    {
        *os << "\nConfig: " << params.config << "\nFile name: " << params.fileName;
        *os << "\nParse expected: " << params.parseExpected;
    }
};

constexpr char CORRECT_EXT_DECL_CONFIG[] = "arktsconfig_ext_decl_correct.json";
constexpr char NO_EXT_DECL_CONFIG[] = "arktsconfig_ext_decl_absent.json";
constexpr char EMPTY_EXT_DECL_CONFIG[] = "arktsconfig_ext_decl_empty.json";
constexpr char STRING_EXT_DECL_CONFIG[] = "arktsconfig_ext_decl_string.json";
constexpr char PATHS_AND_EXT_DECL_CONFIG[] = "arktsconfig_paths_and_ext_decl.json";
constexpr char EXTENDS_BASE_CONFIG[] = "arktsconfig_ext_decl_base.json";
constexpr char EXTENDS_CHILD_CONFIG[] = "arktsconfig_ext_decl_child.json";

ExtDeclParams CorrectExtDeclPackages()
{
    return ExtDeclParams {R"({
        "compilerOptions": {
            "externalDeclPackages": {
                "libhar": { "paths": ["/libs/libhar"] },
                "libhar/sub": { "paths": ["/libs/libhar/sub"] },
                "@ohos/hilog": { "paths": ["/libs/hilog"] }
            }
        }
        })",
                          CORRECT_EXT_DECL_CONFIG,
                          true,
                          {{"libhar", true},
                           {"libhar.sub", true},
                           {"@ohos.hilog", true},
                           {"unknown", false},
                           {"libhar2", false},
                           {"libha", false},
                           {"libhar/sub/file", false},
                           {"", false}}};
}

ExtDeclParams AbsentExtDeclPackages()
{
    return ExtDeclParams {R"({
        "compilerOptions": {
            "baseUrl": "/libs"
        }
        })",
                          NO_EXT_DECL_CONFIG,
                          true,
                          {{"libhar", false}, {"@ohos.hilog", false}}};
}

ExtDeclParams EmptyExtDeclPackages()
{
    return ExtDeclParams {R"({
        "compilerOptions": {
            "externalDeclPackages": {}
        }
        })",
                          EMPTY_EXT_DECL_CONFIG,
                          true,
                          {{"libhar", false}, {"@ohos.hilog", false}}};
}

ExtDeclParams StringExtDeclPackages()
{
    // a non-object "externalDeclPackages" value is tolerated and ignored
    return ExtDeclParams {R"({
        "compilerOptions": {
            "externalDeclPackages": "libhar"
        }
        })",
                          STRING_EXT_DECL_CONFIG,
                          true,
                          {{"libhar", false}}};
}

ExtDeclParams PathsWithExtDeclPackages()
{
    return ExtDeclParams {R"({
        "compilerOptions": {
            "baseUrl": "/libs",
            "paths": {
                "libhar": ["/libs/libhar"],
                "otherlib": ["/libs/otherlib"]
            },
            "externalDeclPackages": {
                "libhar": { "paths": ["/libs/libhar"] }
            }
        }
        })",
                          PATHS_AND_EXT_DECL_CONFIG,
                          true,
                          {{"libhar", true}, {"otherlib", false}}};
}

class ArkTsConfigExternalDeclPackages : public ::testing::TestWithParam<ExtDeclParams> {};

TEST_P(ArkTsConfigExternalDeclPackages, CheckExternalDeclPackages)
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
    ASSERT_EQ(config.Parse(), param.parseExpected);

    for (const auto &[moduleName, expected] : param.lookups) {
        EXPECT_EQ(config.IsExternalDeclPackage(moduleName), expected) << "module name: " << moduleName;
    }
}

INSTANTIATE_TEST_SUITE_P(ArkTsConfigExternalDeclPackagesSuite, ArkTsConfigExternalDeclPackages,
                         ::testing::Values(CorrectExtDeclPackages(), AbsentExtDeclPackages(), EmptyExtDeclPackages(),
                                           StringExtDeclPackages(), PathsWithExtDeclPackages()));

// "externalDeclPackages" keys are inherited through "extends"
TEST(ArkTsConfigExternalDeclPackages, InheritExternalDeclPackages)
{
    const auto baseConfigPath = ark::es2panda::JoinPaths(::testing::TempDir(), EXTENDS_BASE_CONFIG);
    std::ofstream baseConfigFile {baseConfigPath};
    ASSERT_TRUE(baseConfigFile.is_open());
    baseConfigFile << R"({
        "compilerOptions": {
            "externalDeclPackages": {
                "libhar": { "paths": ["/libs/libhar"] }
            }
        }
        })";
    ASSERT_FALSE(baseConfigFile.fail());
    baseConfigFile.close();

    const auto childConfigPath = ark::es2panda::JoinPaths(::testing::TempDir(), EXTENDS_CHILD_CONFIG);
    std::ofstream childConfigFile {childConfigPath};
    ASSERT_TRUE(childConfigFile.is_open());
    childConfigFile << R"({
        "extends": ")";
    childConfigFile << EXTENDS_BASE_CONFIG << R"(",
        "compilerOptions": {
            "externalDeclPackages": {
                "@ohos/hilog": { "paths": ["/libs/hilog"] }
            }
        }
        })";
    ASSERT_FALSE(childConfigFile.fail());
    childConfigFile.close();

    ark::es2panda::util::DiagnosticEngine de;
    auto config = ark::es2panda::ArkTsConfig {childConfigPath, de};
    ASSERT_TRUE(config.Parse());

    // the key from the base config is inherited
    EXPECT_TRUE(config.IsExternalDeclPackage("libhar"));
    // the key from the child config is parsed as well
    EXPECT_TRUE(config.IsExternalDeclPackage("@ohos.hilog"));
    EXPECT_FALSE(config.IsExternalDeclPackage("unknown"));
}

}  // namespace
