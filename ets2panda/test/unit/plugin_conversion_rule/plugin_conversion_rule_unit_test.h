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

#ifndef PANDA_PLUGIN_CONVERSION_RULE_UNIT_TEST_H
#define PANDA_PLUGIN_CONVERSION_RULE_UNIT_TEST_H

#include <gtest/gtest.h>

#include "libarkbase/macros.h"

#include "test/utils/plugin_conversion_rule_test.h"

namespace ark::es2panda {

class PluginConversionRuleUnitTest : public ::test::utils::PluginConversionRuleTest {
public:
    PluginConversionRuleUnitTest() = default;

    ~PluginConversionRuleUnitTest() override = default;

private:
    NO_COPY_SEMANTIC(PluginConversionRuleUnitTest);
    NO_MOVE_SEMANTIC(PluginConversionRuleUnitTest);
};

}  // namespace ark::es2panda

#endif  // PANDA_PLUGIN_CONVERSION_RULE_UNIT_TEST_H