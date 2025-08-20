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

class LspGetNodeScriptFunctionTests : public LSPAPITests {};

TEST_F(LspGetNodeScriptFunctionTests, GetScriptFunction_Simple_TEST)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(
function test() {}
)";
    es2panda_Context *contexts =
        initializer.CreateContext("ScriptFunctionSimple.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "test";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeScriptFunctionTests, GetScriptFunction_Arrow_TEST)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(
function add(a: number, b: number): number { return a + b; }
)";
    es2panda_Context *contexts =
        initializer.CreateContext("ScriptFunctionArrow.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "add";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeScriptFunctionTests, GetScriptFunction_Async_TEST)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(
async function fetchData(): Promise<string> {
    return "data";
}
)";
    es2panda_Context *contexts =
        initializer.CreateContext("ScriptFunctionAsync.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "fetchData";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeScriptFunctionTests, GetScriptFunction_ClassMethod_TEST)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(
class MyClass {
    myMethod(): void {
        console.log("method");
    }
}
)";
    es2panda_Context *contexts =
        initializer.CreateContext("ScriptFunctionClassMethod.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "myMethod";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeScriptFunctionTests, GetScriptFunction_Getter_TEST)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(
class MyClass {
    get value(): number {
        return 42;
    }
}
)";
    es2panda_Context *contexts =
        initializer.CreateContext("ScriptFunctionGetter.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());

    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "value";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeScriptFunctionTests, GetScriptFunction_Setter_TEST)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(
class MyClass {
    private _value: number = 0;
    
    set value(v: number) {
        this._value = v;
    }
}
)";
    es2panda_Context *contexts =
        initializer.CreateContext("ScriptFunctionSetter.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "value";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeScriptFunctionTests, GetScriptFunction_StaticMethod_TEST)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(
class MyClass {
    static staticMethod(): void {
        console.log("static method");
    }
}
)";
    es2panda_Context *contexts =
        initializer.CreateContext("ScriptFunctionStaticMethod.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "staticMethod";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_NE(extractedText.find(nodeName), std::string::npos);
    ASSERT_EQ(nodeName, extractedText);
    initializer.DestroyContext(contexts);
}

TEST_F(LspGetNodeScriptFunctionTests, GetScriptFunction_NotFound_TEST)
{
    Initializer initializer = Initializer();
    const std::string sourceCode = R"(
class MyClass {
    static staticMethod(): void {
        console.log("static method");
    }
}
)";
    es2panda_Context *contexts =
        initializer.CreateContext("ScriptFunctionNotFound.ets", ES2PANDA_STATE_PARSED, sourceCode.c_str());
    LSPAPI const *lspApi = GetImpl();
    const std::string nodeName = "nonexistent";
    std::vector<NodeInfo> nodeInfos;
    nodeInfos.emplace_back(NodeInfo {nodeName, ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION});
    std::vector<NodeInfo *> nodeInfoPtrs;
    nodeInfoPtrs.push_back(&nodeInfos[0]);

    auto res = lspApi->getDefinitionDataFromNode(contexts, nodeInfoPtrs);
    std::string extractedText(sourceCode.substr(res.start, res.length));
    ASSERT_EQ(extractedText.find(nodeName), std::string::npos);
    initializer.DestroyContext(contexts);
}
}  // namespace