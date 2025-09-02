/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "lsp/include/code_fix_provider.h"
#include "gtest/gtest.h"

namespace {

TEST(RefactorProviderRegistrationTest, RegistersConvertFunctionRefactor)
{
    const auto &provider = ark::es2panda::lsp::CodeFixProvider::Instance();
    const auto &errors = provider.GetErrorCodeToFixes();
    const auto &fixIdToRegistration = provider.GetFixIdToRegistration();
    EXPECT_FALSE(errors.empty());
    EXPECT_FALSE(fixIdToRegistration.empty());
    EXPECT_TRUE(errors.size() >= fixIdToRegistration.size());
}
}  // namespace
