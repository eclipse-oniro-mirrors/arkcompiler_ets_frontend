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

#include "lsp/include/formatting/formatting_context.h"
#include "lsp/include/formatting/formatting_settings.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>
#include "ir/astNode.h"

namespace {

class LSPFormattingContextTests : public LSPAPITests {};

TEST_F(LSPFormattingContextTests, FormattingContextConstructorTest)
{
    const std::string sourceCode = "let x = 10;";

    ark::es2panda::lsp::FormattingContext context(sourceCode);

    EXPECT_EQ(context.GetSourceText(), sourceCode);
}

TEST_F(LSPFormattingContextTests, FormattingContextTokenSettersTest)
{
    const std::string sourceCode = "let x = 10;";

    ark::es2panda::lsp::FormattingContext context(sourceCode);

    ark::es2panda::lexer::Token prevToken;
    prevToken.SetTokenType(ark::es2panda::lexer::TokenType::KEYW_LET);

    ark::es2panda::lexer::Token currentToken;
    currentToken.SetTokenType(ark::es2panda::lexer::TokenType::LITERAL_IDENT);

    ark::es2panda::lexer::Token nextToken;
    nextToken.SetTokenType(ark::es2panda::lexer::TokenType::PUNCTUATOR_SUBSTITUTION);

    context.SetPreviousToken(prevToken);
    context.SetCurrentToken(currentToken);
    context.SetNextToken(nextToken);

    EXPECT_EQ(context.GetPreviousToken().Type(), ark::es2panda::lexer::TokenType::KEYW_LET);
    EXPECT_EQ(context.GetCurrentToken().Type(), ark::es2panda::lexer::TokenType::LITERAL_IDENT);
    EXPECT_EQ(context.GetNextToken().Type(), ark::es2panda::lexer::TokenType::PUNCTUATOR_SUBSTITUTION);
}

TEST_F(LSPFormattingContextTests, FormattingContextParentNodeTest)
{
    const std::string sourceCode = "let x = 10;";

    ark::es2panda::lsp::FormattingContext context(sourceCode);

    ark::es2panda::ir::AstNode *mockParent = nullptr;
    ark::es2panda::ir::AstNode *mockNextParent = nullptr;

    context.SetCurrentTokenParent(mockParent);
    context.SetNextTokenParent(mockNextParent);

    EXPECT_EQ(context.GetCurrentTokenParent(), mockParent);
    EXPECT_EQ(context.GetNextTokenParent(), mockNextParent);
}

TEST_F(LSPFormattingContextTests, FormattingContextBlockIsOnOneLineTest)
{
    const std::string sourceCode = "{ let x = 10; }";

    ark::es2panda::lsp::FormattingContext context(sourceCode);

    context.SetCurrentTokenParent(nullptr);
    EXPECT_TRUE(context.ContextNodeBlockIsOnOneLine());
}

TEST_F(LSPFormattingContextTests, FormatCodeSettingsTest)
{
    const size_t indentSize = 2U;
    const size_t tabSize = 4U;
    const std::string newlineChar = "\r\n";

    ark::es2panda::lsp::FormatCodeSettings settings;

    settings.SetIndentSize(indentSize);
    settings.SetTabSize(tabSize);
    settings.SetNewLineCharacter(newlineChar);
    settings.SetConvertTabsToSpaces(true);
    settings.SetInsertSpaceAfterCommaDelimiter(true);
    settings.SetInsertSpaceAfterSemicolonInForStatements(true);
    settings.SetInsertSpaceBeforeAndAfterBinaryOperators(true);

    EXPECT_EQ(settings.GetIndentSize(), indentSize);
    EXPECT_EQ(settings.GetTabSize(), tabSize);
    EXPECT_EQ(settings.GetNewLineCharacter(), newlineChar);
    EXPECT_TRUE(settings.GetConvertTabsToSpaces());
    EXPECT_TRUE(settings.GetInsertSpaceAfterCommaDelimiter());
    EXPECT_TRUE(settings.GetInsertSpaceAfterSemicolonInForStatements());
    EXPECT_TRUE(settings.GetInsertSpaceBeforeAndAfterBinaryOperators());
}

TEST_F(LSPFormattingContextTests, FormatCodeSettingsDefaultValuesTest)
{
    ark::es2panda::lsp::FormatCodeSettings settings;

    EXPECT_EQ(settings.GetIndentSize(), 4U);
    EXPECT_EQ(settings.GetTabSize(), 4U);
    EXPECT_EQ(settings.GetNewLineCharacter(), "\n");
    EXPECT_TRUE(settings.GetConvertTabsToSpaces());
    EXPECT_TRUE(settings.GetInsertSpaceAfterCommaDelimiter());
    EXPECT_TRUE(settings.GetInsertSpaceAfterSemicolonInForStatements());
    EXPECT_TRUE(settings.GetInsertSpaceBeforeAndAfterBinaryOperators());
}

}  // namespace