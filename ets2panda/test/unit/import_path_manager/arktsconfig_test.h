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

#ifndef ES2PANDA_TEST_UNIT_MODULE_ARKTSCONFIG_TEST_H
#define ES2PANDA_TEST_UNIT_MODULE_ARKTSCONFIG_TEST_H

#include "module_project_test.h"
#include "util/diagnosticEngine.h"

namespace ark::es2panda::util::test {

class ArkTsConfigTest : public ModuleProjectTest {
protected:
    void Parse(std::string_view json, std::string_view file = "arktsconfig.json")
    {
        Write(file, json);
        config_ = std::make_unique<ArkTsConfig>(Path(file).string(), diagnostics_);
        ASSERT_TRUE(config_->Parse());
        EXPECT_FALSE(diagnostics_.IsAnyError());
    }

    std::string Project(std::string_view options, std::string_view fields = {}) const
    {
        std::string result = R"({"compilerOptions":{"rootDir":)" + JsonPath(root_);
        if (!options.empty()) {
            result += ",";
            result += options;
        }
        result += "}";
        if (!fields.empty()) {
            result += ",";
            result += fields;
        }
        return result + "}";
    }

    DiagnosticEngine diagnostics_;
    std::unique_ptr<ArkTsConfig> config_;
};

}  // namespace ark::es2panda::util::test

#endif  // ES2PANDA_TEST_UNIT_MODULE_ARKTSCONFIG_TEST_H
