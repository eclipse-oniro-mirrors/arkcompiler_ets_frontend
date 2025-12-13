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

#include "lsp_api_test.h"
#include "lsp/include/api.h"

namespace {
using ark::es2panda::lsp::Initializer;
TEST_F(LSPAPITests, GetTokenNative1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("token_native_1.ets", ES2PANDA_STATE_CHECKED,
                                                      "class Calc {\n  native add(arg1: int, arg2:int): int;\n}");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    size_t const offset = 24;
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getTokenTypes(ctx, offset);
    ASSERT_EQ(result.type.find("native") != std::string::npos, true);
    initializer.DestroyContext(ctx);
}
}  // namespace
