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

#include "arktsconfig_test.h"

namespace ark::es2panda::util::test {
namespace {

struct InvalidConfig {
    const char *name;
    const char *json;
    const diagnostic::DiagnosticKind &error;
    const char *field;
};

class InvalidArkTsConfigTest : public ArkTsConfigTest, public testing::WithParamInterface<InvalidConfig> {};

TEST_P(InvalidArkTsConfigTest, ReportsTheOffendingField)
{
    const auto &input = GetParam();
    Write("arktsconfig.json", input.json);
    ArkTsConfig config(Path("arktsconfig.json").string(), diagnostics_);
    EXPECT_FALSE(config.Parse());
    const auto &errors = diagnostics_.GetDiagnosticStorage(input.error.Type());
    ASSERT_EQ(errors.size(), 1U);
    ASSERT_EQ(errors.front()->GetId(), input.error.Id());
    if (input.field != nullptr) {
        EXPECT_NE(errors.front()->Message().find(input.field), std::string::npos);
    }
}

const InvalidConfig INVALID_CONFIGS[] = {
    {"MalformedJson", "{", diagnostic::INVALID_JSON, nullptr},
    {"ExtendsIsArray", R"({"extends":["base"]})", diagnostic::INVALID_JSON_TYPE, "extends"},
    {"CannotLocateBase", R"({"extends":"absent-ark-module-suite/base.json"})", diagnostic::UNRESOLVABLE_CONFIG_PATH,
     "absent-ark-module-suite/base.json"},
    {"FilesIsString", R"({"files":"Main.ats"})", diagnostic::INVALID_JSON_TYPE, "files"},
    {"FilesEmpty", R"({"files":[]})", diagnostic::EMPTY_LIST, "files"},
    {"IncludeIsObject", R"({"include":{}})", diagnostic::INVALID_JSON_TYPE, "include"},
    {"IncludeEmpty", R"({"include":[]})", diagnostic::EMPTY_LIST, "include"},
    {"ExcludeIsString", R"({"exclude":"out"})", diagnostic::INVALID_JSON_TYPE, "exclude"},
    {"ExcludeEmpty", R"({"exclude":[]})", diagnostic::EMPTY_LIST, "exclude"},
    {"PathSubstitutionIsString", R"({"compilerOptions":{"paths":{"sdk":"src"}}})", diagnostic::INVALID_VALUE, "sdk"},
    {"PathSubstitutionEmpty", R"({"compilerOptions":{"paths":{"sdk":[]}}})", diagnostic::EMPTY_ARRAY_SUBSTITOTIONS,
     "sdk"},
    {"MockIsString", R"({"compilerOptions":{"mock":{"api":"double.ats"}}})", diagnostic::INVALID_VALUE, "api"},
    {"MockLacksSource", R"({"compilerOptions":{"mock":{"api":{}}}})", diagnostic::INVALID_VALUE, "mock"},
    {"MockSourceIsNumber", R"({"compilerOptions":{"mock":{"api":{"source":42}}}})", diagnostic::INVALID_VALUE, "api"},
    {"InternalPathIsNumber", R"({"compilerOptions":{"internalPaths":["ok",7]}})", diagnostic::INVALID_JSON_TYPE,
     "internalPaths"},
    {"DependencyIsArray", R"({"compilerOptions":{"dependencies":{"lib":[]}}})", diagnostic::INVALID_VALUE, "lib"},
    {"UnknownLanguage", R"({"compilerOptions":{"dependencies":{"lib":{"language":"unknown"}}}})",
     diagnostic::INVALID_LANGUAGE, "lib"},
    {"AtsDependenciesIsArray", R"({"compilerOptions":{"atsDependencies":[]}})", diagnostic::INVALID_JSON_TYPE,
     "atsDependencies"},
    {"AtsRecordIsString", R"({"compilerOptions":{"atsDependencies":{"lib":"src"}}})", diagnostic::INVALID_VALUE, "lib"},
    {"PackageNameEmpty", R"({"compilerOptions":{"atsDependencies":{"":{"moduleRoot":"."}}}})",
     diagnostic::INVALID_VALUE, "atsDependencies"},
    {"PackageNameEndsInSlash", R"({"compilerOptions":{"atsDependencies":{"lib/":{"moduleRoot":"lib"}}}})",
     diagnostic::INVALID_VALUE, "lib/"},
    {"ModuleRootMissing", R"({"compilerOptions":{"atsDependencies":{"lib":{}}}})", diagnostic::INVALID_VALUE,
     "moduleRoot"},
    {"ModuleRootEmpty", R"({"compilerOptions":{"atsDependencies":{"lib":{"moduleRoot":""}}}})",
     diagnostic::INVALID_VALUE, "moduleRoot"},
    {"ModuleRootIsArray", R"({"compilerOptions":{"atsDependencies":{"lib":{"moduleRoot":[]}}}})",
     diagnostic::INVALID_VALUE, "moduleRoot"},
    {"EntryFileEmpty", R"({"compilerOptions":{"atsDependencies":{"lib":{"moduleRoot":"lib","entryFile":""}}}})",
     diagnostic::INVALID_VALUE, "entryFile"},
    {"EntryFileIsArray", R"({"compilerOptions":{"atsDependencies":{"lib":{"moduleRoot":"lib","entryFile":[]}}}})",
     diagnostic::INVALID_VALUE, "entryFile"},
    {"SearchPathEmpty", R"({"compilerOptions":{"atsDependencies":{"lib":{"moduleRoot":"lib","searchPath":[]}}}})",
     diagnostic::INVALID_VALUE, "searchPath"},
    {"SearchPathIsString", R"({"compilerOptions":{"atsDependencies":{"lib":{"moduleRoot":"lib","searchPath":"src"}}}})",
     diagnostic::INVALID_VALUE, "searchPath"},
    {"SearchRootEmpty",
     R"({"compilerOptions":{"atsDependencies":{"lib":{"moduleRoot":"lib","searchPath":["src",""]}}}})",
     diagnostic::INVALID_VALUE, "searchPath"},
    {"SearchRootIsNumber", R"({"compilerOptions":{"atsDependencies":{"lib":{"moduleRoot":"lib","searchPath":[123]}}}})",
     diagnostic::INVALID_VALUE, "searchPath"},
};

INSTANTIATE_TEST_SUITE_P(Configuration, InvalidArkTsConfigTest, testing::ValuesIn(INVALID_CONFIGS),
                         [](const auto &info) { return info.param.name; });

TEST_F(ArkTsConfigTest, MissingConfigDiagnosticsIdentifyTheRequestedFile)
{
    const auto missing = Path("absent.json").string();
    ArkTsConfig config(missing, diagnostics_);
    ASSERT_FALSE(config.Parse());
    const auto &errors = diagnostics_.GetDiagnosticStorage(diagnostic::FAILED_TO_OPEN_FILE.Type());
    ASSERT_EQ(errors.size(), 2U);
    EXPECT_EQ(errors[0]->GetId(), diagnostic::FAILED_TO_OPEN_FILE.Id());
    EXPECT_EQ(errors[1]->GetId(), diagnostic::UNRESOLVABLE_CONFIG_PATH.Id());
    for (const auto &error : errors) {
        EXPECT_NE(error->Message().find(missing), std::string::npos);
    }
}

TEST_F(ArkTsConfigTest, InvalidBasePreservesCauseAndNamesTheBaseInTheChild)
{
    Write("base.json", R"({"files":[]})");
    Write("app.json", R"({"extends":"base.json"})");
    ArkTsConfig config(Path("app.json").string(), diagnostics_);
    ASSERT_FALSE(config.Parse());
    const auto &errors = diagnostics_.GetDiagnosticStorage(diagnostic::WRONG_BASE_CONFIG.Type());
    ASSERT_EQ(errors.size(), 2U);
    EXPECT_EQ(errors[0]->GetId(), diagnostic::EMPTY_LIST.Id());
    EXPECT_EQ(errors[1]->GetId(), diagnostic::WRONG_BASE_CONFIG.Id());
    EXPECT_NE(errors[1]->Message().find("base.json"), std::string::npos);
}

TEST_F(ArkTsConfigTest, ExtendsCycleReportsTheCycleAndEachAffectedBase)
{
    Write("base.json", R"({"extends":"./app.json"})");
    Write("app.json", R"({"extends":"base.json"})");
    ArkTsConfig config(Path("app.json").string(), diagnostics_);
    ASSERT_FALSE(config.Parse());
    const auto &errors = diagnostics_.GetDiagnosticStorage(diagnostic::CYCLIC_IMPORT.Type());
    ASSERT_EQ(errors.size(), 2U);
    EXPECT_EQ(errors[0]->GetId(), diagnostic::CYCLIC_IMPORT.Id());
    EXPECT_EQ(errors[1]->GetId(), diagnostic::WRONG_BASE_CONFIG.Id());
}

TEST_F(ArkTsConfigTest, RootDirMustBeADirectoryEvenIfTheFileExists)
{
    Write("regular.ats");
    Write("app.json", R"({"compilerOptions":{"rootDir":)" + JsonPath(Path("regular.ats")) + "}}");
    ArkTsConfig config(Path("app.json").string(), diagnostics_);
    ASSERT_FALSE(config.Parse());
    const auto &errors = diagnostics_.GetDiagnosticStorage(diagnostic::NOT_A_DIR.Type());
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front()->GetId(), diagnostic::NOT_A_DIR.Id());
    EXPECT_NE(errors.front()->Message().find("regular.ats"), std::string::npos);
}

TEST_F(ArkTsConfigTest, AbsolutePackageKeysAreDiagnosedUsingTheHostPathSyntax)
{
    const auto key = Path("external").string();
    Write("config.json", Project(R"("dependencies":{)" + JsonPath(Path("external")) + R"(:{"path":"lib.abc"}})"));
    ArkTsConfig config(Path("config.json").string(), diagnostics_);
    config.Parse();
    const auto &errors = diagnostics_.GetDiagnosticStorage(diagnostic::DEPENDENCIES_ABSOLUTE.Type());
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front()->GetId(), diagnostic::DEPENDENCIES_ABSOLUTE.Id());
    EXPECT_NE(errors.front()->Message().find(key), std::string::npos);
    diagnostics_.ClearDiagnostics();

    Write("config.json", Project(R"("atsDependencies":{)" + JsonPath(Path("external")) + R"(:{"moduleRoot":"pkg"}})"));
    ArkTsConfig atsConfig(Path("config.json").string(), diagnostics_);
    EXPECT_FALSE(atsConfig.Parse());
    const auto &atsErrors = diagnostics_.GetDiagnosticStorage(diagnostic::INVALID_VALUE.Type());
    ASSERT_EQ(atsErrors.size(), 1U);
    EXPECT_EQ(atsErrors.front()->GetId(), diagnostic::INVALID_VALUE.Id());
    EXPECT_NE(atsErrors.front()->Message().find(key), std::string::npos);
}

TEST_F(ArkTsConfigTest, StaticDependencyRequiresADeclarationOrArchivePath)
{
    Write("config.json", Project(R"("dependencies":{"static":{"mainFile":"Entry"}})"));
    ArkTsConfig config(Path("config.json").string(), diagnostics_);
    EXPECT_FALSE(config.Parse());
    const auto &errors = diagnostics_.GetDiagnosticStorage(diagnostic::INVALID_PATH.Type());
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front()->GetId(), diagnostic::INVALID_PATH.Id());
}

TEST_F(ArkTsConfigTest, DynamicDependencyWithoutOhmUrlRetainsItsIdentityAndEmitsWarning)
{
    ASSERT_NO_FATAL_FAILURE(Parse(Project(R"("dependencies":{"foreign":{"language":"js","alias":["legacy"]}})")));
    const auto &warnings = diagnostics_.GetDiagnosticStorage(diagnostic::NO_OHMURL.Type());
    ASSERT_EQ(warnings.size(), 1U);
    EXPECT_EQ(warnings.front()->GetId(), diagnostic::NO_OHMURL.Id());
    EXPECT_NE(warnings.front()->Message().find("foreign"), std::string::npos);
    EXPECT_TRUE(config_->Dependencies().at("foreign").OhmUrl().empty());
    EXPECT_EQ(config_->ResolvePath("legacy", true, nullptr), "foreign");
}

TEST_F(ArkTsConfigTest, UnsupportedInteropLanguageHasItsOwnDiagnostic)
{
    Write("config.json", Project(R"("dependencies":{"foreign":{"language":"ts","ohmUrl":"foreign"}})"));
    ArkTsConfig config(Path("config.json").string(), diagnostics_);
    EXPECT_FALSE(config.Parse());
    const auto &errors = diagnostics_.GetDiagnosticStorage(diagnostic::UNSUPPORTED_LANGUAGE_FOR_INTEROP.Type());
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front()->GetId(), diagnostic::UNSUPPORTED_LANGUAGE_FOR_INTEROP.Id());
    EXPECT_NE(errors.front()->Message().find("'ts'"), std::string::npos);
}

}  // namespace
}  // namespace ark::es2panda::util::test
