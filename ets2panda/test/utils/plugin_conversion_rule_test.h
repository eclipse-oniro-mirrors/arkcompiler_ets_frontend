/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_TEST_UTILS_PLUGIN_CONVERSION_RULE_TEST_H
#define ES2PANDA_TEST_UTILS_PLUGIN_CONVERSION_RULE_TEST_H

#include <string>
#include <filesystem>

#include <gtest/gtest.h>

#include "libarkbase/macros.h"

namespace test::utils {

class PluginConversionRuleTest : public testing::Test {
public:
    PluginConversionRuleTest();

    ~PluginConversionRuleTest() override = default;

    static std::string RemoveWhitespace(const std::string &str);

    bool HasMatched(const std::string &searchStr) const;

    NO_COPY_SEMANTIC(PluginConversionRuleTest);
    NO_MOVE_SEMANTIC(PluginConversionRuleTest);

private:
    void ReadStringFromFile(const std::string &filePath);

    std::string cApiImplFileStr_;
};
}  // namespace test::utils

#endif  // ES2PANDA_TEST_UTILS_PLUGIN_CONVERSION_RULE_TEST_H
