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

#include "ir/astNode.h"
#include "lsp/include/api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include <gtest/gtest.h>

namespace {
using ark::es2panda::lsp::Initializer;

class LspGetNodeInfoScriptFunctionTests : public LSPAPITests {};

TEST_F(LspGetNodeInfoScriptFunctionTests, GetScriptFunctionInfo_Simple_TEST)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(function test() {}
)";
    es2panda_Context *contexts =
        initializer.CreateContext("ScriptFunctionInfo.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const size_t errorOffset = 20;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts, errorOffset);
    ASSERT_TRUE(result.empty());

    const size_t offset = 9;
    const size_t expectedSize = 3;
    result = lspApi->getNodeInfosByDefinitionData(contexts, offset);

    std::vector<NodeInfo> expectedResult = {{"test", ark::es2panda::ir::AstNodeType::FUNCTION_DECLARATION},
                                            {"test", ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION},
                                            {"test", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }

    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeInfoScriptFunctionTests, GetScriptFunctionInfo_Arrow_TEST)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(
function add(a: number, b: number): number { return a + b; }
)";
    es2panda_Context *contexts =
        initializer.CreateContext("ScriptFunctionInfo.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 10;
    const size_t expectedSize = 3;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts, offset);

    std::vector<NodeInfo> expectedResult = {{"add", ark::es2panda::ir::AstNodeType::FUNCTION_DECLARATION},
                                            {"add", ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION},
                                            {"add", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }

    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeInfoScriptFunctionTests, GetScriptFunctionInfo_Async_TEST)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(
async function fetchData(): Promise<string> {
    return "data";
}
)";
    es2panda_Context *contexts =
        initializer.CreateContext("ScriptFunctionInfo.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 16;
    const size_t expectedSize = 3;
    auto result = lspApi->getNodeInfosByDefinitionData(contexts, offset);

    std::vector<NodeInfo> expectedResult = {{"fetchData", ark::es2panda::ir::AstNodeType::FUNCTION_DECLARATION},
                                            {"fetchData", ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION},
                                            {"fetchData", ark::es2panda::ir::AstNodeType::IDENTIFIER}};
    ASSERT_EQ(result.size(), expectedSize);
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].name, expectedResult[i].name);
        ASSERT_EQ(result[i].kind, expectedResult[i].kind);
    }

    initializer.DestroyContext(contexts);
}
}  // namespace