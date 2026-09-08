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

#include <algorithm>
#include <array>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "lsp/include/api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::Initializer;

// Node-info handler registrations in node_matchers.cpp must stay consistent with the
// IsNodeInfoSymbolKind gate applied by getNodeInfosByDefinitionData (api.cpp): kinds that
// the gate excludes must not have a registered handler, otherwise the entry is dead code.
// These four kinds are intentionally NOT part of the public NodeInfo protocol because each
// construct is reported through its canonical kind instead:
//   IDENTIFIER           -> generic token wrapper, never a symbol entry of its own
//   SCRIPT_FUNCTION      -> reported as FUNCTION_DECLARATION / METHOD_DEFINITION
//   VARIABLE_DECLARATION -> reported as the more precise VARIABLE_DECLARATOR
//   CLASS_DEFINITION     -> reported as CLASS_DECLARATION
constexpr std::array<ark::es2panda::ir::AstNodeType, 4> GATED_HANDLER_KINDS = {
    ark::es2panda::ir::AstNodeType::IDENTIFIER, ark::es2panda::ir::AstNodeType::SCRIPT_FUNCTION,
    ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATION, ark::es2panda::ir::AstNodeType::CLASS_DEFINITION};

class LspGetNodeInfoSymbolKindGateTests : public LSPAPITests {};

TEST_F(LspGetNodeInfoSymbolKindGateTests, DeclarationPositionsReportOnlyWhitelistedKinds)
{
    const std::string source = R"(function greet(): void {
    return;
}

let value = 1;

class Box {}
)";
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("node_info_symbol_kind_gate.ets", ES2PANDA_STATE_PARSED, source.c_str());
    LSPAPI const *lspApi = GetImpl();
    ASSERT_NE(lspApi, nullptr);

    const size_t greetPos = source.find("greet");
    const size_t valueDeclPos = source.find("value");
    const size_t boxPos = source.find("Box");
    ASSERT_NE(greetPos, std::string::npos);
    ASSERT_NE(valueDeclPos, std::string::npos);
    ASSERT_NE(boxPos, std::string::npos);

    // Function declaration name position: FUNCTION_DECLARATION only, no SCRIPT_FUNCTION entry.
    auto functionInfos = lspApi->getNodeInfosByDefinitionData(context, nullptr, greetPos);
    ASSERT_EQ(functionInfos.size(), 1U);
    EXPECT_EQ(functionInfos[0].name, "greet");
    EXPECT_EQ(functionInfos[0].kind, ark::es2panda::ir::AstNodeType::FUNCTION_DECLARATION);

    // Variable declaration name position: VARIABLE_DECLARATOR only, no VARIABLE_DECLARATION entry.
    auto variableInfos = lspApi->getNodeInfosByDefinitionData(context, nullptr, valueDeclPos);
    ASSERT_EQ(variableInfos.size(), 1U);
    EXPECT_EQ(variableInfos[0].name, "value");
    EXPECT_EQ(variableInfos[0].kind, ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATOR);

    // Class declaration name position: CLASS_DECLARATION only, no CLASS_DEFINITION entry.
    auto classInfos = lspApi->getNodeInfosByDefinitionData(context, nullptr, boxPos);
    ASSERT_EQ(classInfos.size(), 1U);
    EXPECT_EQ(classInfos[0].name, "Box");
    EXPECT_EQ(classInfos[0].kind, ark::es2panda::ir::AstNodeType::CLASS_DECLARATION);

    initializer.DestroyContext(context);

    for (const auto &infos : {functionInfos, variableInfos, classInfos}) {
        for (const auto &info : infos) {
            EXPECT_EQ(std::find(GATED_HANDLER_KINDS.begin(), GATED_HANDLER_KINDS.end(), info.kind),
                      GATED_HANDLER_KINDS.end())
                << "gated kind " << static_cast<int>(info.kind) << " leaked into public node infos";
        }
    }
}

TEST_F(LspGetNodeInfoSymbolKindGateTests, PlainIdentifierReferenceProducesNoIdentifierKindEntry)
{
    const std::string source = R"(let value = 1;

let total = value;
)";
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("node_info_identifier_ref_gate.ets", ES2PANDA_STATE_PARSED, source.c_str());
    LSPAPI const *lspApi = GetImpl();
    ASSERT_NE(lspApi, nullptr);

    const size_t firstValuePos = source.find("value");
    const size_t valueRefPos = source.find("value", firstValuePos + 1);
    ASSERT_NE(firstValuePos, std::string::npos);
    ASSERT_NE(valueRefPos, std::string::npos);

    // A bare identifier reference is resolved through its enclosing construct: the touching
    // token is the "value" identifier inside the "total" initializer, and the walk stops at
    // the enclosing VARIABLE_DECLARATOR. No {name, IDENTIFIER} entry may be produced.
    auto refInfos = lspApi->getNodeInfosByDefinitionData(context, nullptr, valueRefPos);
    ASSERT_EQ(refInfos.size(), 1U);
    EXPECT_EQ(refInfos[0].name, "total");
    EXPECT_EQ(refInfos[0].kind, ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATOR);

    initializer.DestroyContext(context);
}
}  // namespace
