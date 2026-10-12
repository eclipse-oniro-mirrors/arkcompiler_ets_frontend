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

#include <gtest/gtest.h>
#include <string>
#include "lsp_api_test.h"
#include "lsp/include/classifier.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::ir::AstNodeType;
using ark::es2panda::lsp::AstNodeTypeToClassificationType;
using ark::es2panda::lsp::ClassificationType;
using ark::es2panda::lsp::ClassificationTypeToString;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::InitLexer;

class LspClassifierInternalsTests : public LSPAPITests {};

// =============================================================================
// P2E-CL-01: InitLexer_ReturnsLexer
// =============================================================================
// First direct InitLexer coverage; classifier_test.cpp exercises it only via the
// full classification pipeline.
TEST_F(LspClassifierInternalsTests, InitLexer_ReturnsLexer)
{
    const std::string source = "let x = 1;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("cl01.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *publicCtx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ark::es2panda::parser::ParserContext parserContext(publicCtx->parserProgram,
                                                       ark::es2panda::parser::ParserStatus::NO_OPTS);
    auto lexer = InitLexer(ctx, &parserContext);
    ASSERT_NE(lexer, nullptr);

    // The lexer should be initialized and ready to tokenize
    EXPECT_NO_FATAL_FAILURE(lexer->NextToken());

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-CL-02: GetClassificationType_ReturnsType
// =============================================================================
// BLOCKED_DESIGN_MAPPING
// classifier.h:50 declares GetClassificationType with 1 parameter
// (const lexer::Token &token).  classifier.cpp:61 defines it with 2 parameters
// (const lexer::Token &token, ir::AstNode *astNode).  The signature mismatch
// means the linker would look for a 1-param symbol that does not exist.
// NEXT ACTION: Align the header declaration with the implementation.

// =============================================================================
// P2E-CL-03: AstNodeTypeToClassificationType_ReturnsType
// =============================================================================
// classifier_test.cpp asserts these mappings only through pipeline output names;
// here the switch tables are pinned directly.
TEST_F(LspClassifierInternalsTests, AstNodeTypeToClassificationType_ReturnsType)
{
    // CLASS_DEFINITION → CLASS_NAME
    EXPECT_EQ(AstNodeTypeToClassificationType(AstNodeType::CLASS_DEFINITION), ClassificationType::CLASS_NAME);

    // TS_TYPE_PARAMETER → TYPE_PARAMETER_NAME
    EXPECT_EQ(AstNodeTypeToClassificationType(AstNodeType::TS_TYPE_PARAMETER), ClassificationType::TYPE_PARAMETER_NAME);

    // TS_INTERFACE_DECLARATION → INTERFACE_NAME
    EXPECT_EQ(AstNodeTypeToClassificationType(AstNodeType::TS_INTERFACE_DECLARATION),
              ClassificationType::INTERFACE_NAME);

    // TS_ENUM_DECLARATION → ENUM_NAME
    EXPECT_EQ(AstNodeTypeToClassificationType(AstNodeType::TS_ENUM_DECLARATION), ClassificationType::ENUM_NAME);

    // ETS_PARAMETER_EXPRESSION → PARAMETER_NAME
    EXPECT_EQ(AstNodeTypeToClassificationType(AstNodeType::ETS_PARAMETER_EXPRESSION),
              ClassificationType::PARAMETER_NAME);

    // TS_TYPE_ALIAS_DECLARATION → TYPE_ALIAS_NAME
    EXPECT_EQ(AstNodeTypeToClassificationType(AstNodeType::TS_TYPE_ALIAS_DECLARATION),
              ClassificationType::TYPE_ALIAS_NAME);

    // Default case: unknown type → IDENTIFIER
    EXPECT_EQ(AstNodeTypeToClassificationType(AstNodeType::BINARY_EXPRESSION), ClassificationType::IDENTIFIER);
}

// =============================================================================
// P2E-CL-04: ClassificationTypeToString_ReturnsString
// =============================================================================
TEST_F(LspClassifierInternalsTests, ClassificationTypeToString_ReturnsString)
{
    // Verify each classification type returns a non-empty string
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::IDENTIFIER), "identifier");
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::KEYWORD), "keyword");
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::NUMERIC_LITERAL), "number");
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::STRING_LITERAL), "string");
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::BOOLEAN_LITERAL), "boolean");
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::NULL_LITERAL), "null");
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::PUNCTUATION), "punctuation");
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::CLASS_NAME), "class name");
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::ENUM_NAME), "enum name");
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::INTERFACE_NAME), "interface name");
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::TYPE_PARAMETER_NAME), "type parameter name");
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::TYPE_ALIAS_NAME), "type alias name");
    EXPECT_STREQ(ClassificationTypeToString(ClassificationType::PARAMETER_NAME), "parameter name");
}

}  // namespace
