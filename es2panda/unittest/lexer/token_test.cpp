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

#include <lexer/token/token.h>
#include <lexer/token/tokenType.h>
#include <lexer/token/letters.h>
#include <lexer/lexer.h>
#include <memory>
#include <parser/context/parserContext.h>
#include <parser/program/program.h>
#include <es2panda.h>
#include <gtest/gtest.h>
#include <mem/pool_manager.h>

namespace panda::es2panda::lexer {

using mem::MemConfig;

class MemManager {
public:
    explicit MemManager()
    {
        constexpr auto COMPILER_SIZE = 8192_MB;
        MemConfig::Initialize(0, 0, COMPILER_SIZE, 0);
        PoolManager::Initialize(PoolType::MMAP);
    }

    NO_COPY_SEMANTIC(MemManager);
    NO_MOVE_SEMANTIC(MemManager);

    ~MemManager()
    {
        PoolManager::Finalize();
        MemConfig::Finalize();
    }
};

class TokenTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mm_ = std::make_unique<MemManager>();
        program_ = std::make_unique<parser::Program>(es2panda::ScriptExtension::JS);
        parserContext_ = std::make_unique<parser::ParserContext>(program_.get());
    }

    void TearDown() override
    {
        tsParserContext_.reset();
        tsProgram_.reset();
        parserContext_.reset();
        program_.reset();
        mm_.reset();
    }

    Lexer CreateLexer(const std::string &source)
    {
        program_->SetSource(source, "test.js", false);
        return Lexer(parserContext_.get());
    }

    Lexer CreateLexerTS(const std::string &source)
    {
        if (!tsProgram_) {
            tsProgram_ = std::make_unique<parser::Program>(es2panda::ScriptExtension::TS);
            tsParserContext_ = std::make_unique<parser::ParserContext>(tsProgram_.get());
        }
        tsProgram_->SetSource(source, "test.ts", false);
        return Lexer(tsParserContext_.get());
    }

    std::unique_ptr<MemManager> mm_;
    std::unique_ptr<parser::Program> program_;
    std::unique_ptr<parser::ParserContext> parserContext_;
    std::unique_ptr<parser::Program> tsProgram_;
    std::unique_ptr<parser::ParserContext> tsParserContext_;
};

// Test Token basic type operations
TEST_F(TokenTest, TestTokenType)
{
    Token token;
    EXPECT_EQ(token.Type(), TokenType::EOS);
    
    token.SetTokenType(TokenType::LITERAL_IDENT);
    EXPECT_EQ(token.Type(), TokenType::LITERAL_IDENT);
}

// Test Token flags
TEST_F(TokenTest, TestTokenFlags)
{
    Token token;
    EXPECT_EQ(token.Flags(), TokenFlags::NONE);
    EXPECT_FALSE(token.NewLine());
    EXPECT_FALSE(token.EscapeError());
    EXPECT_FALSE(token.IsTaggedTemplate());
}

// Test IsUpdate
TEST_F(TokenTest, TestIsUpdate)
{
    const TokenType trueCases[] = {
        TokenType::PUNCTUATOR_PLUS_PLUS,
        TokenType::PUNCTUATOR_MINUS_MINUS,
    };

    for (auto type : trueCases) {
        Token token;
        token.SetTokenType(type);
        EXPECT_TRUE(token.IsUpdate());
    }

    Token token;
    token.SetTokenType(TokenType::PUNCTUATOR_PLUS);
    EXPECT_FALSE(token.IsUpdate());
}

// Test IsUnary
TEST_F(TokenTest, TestIsUnary)
{
    const TokenType trueCases[] = {
        TokenType::PUNCTUATOR_MINUS,
        TokenType::PUNCTUATOR_PLUS,
        TokenType::PUNCTUATOR_TILDE,
        TokenType::PUNCTUATOR_EXCLAMATION_MARK,
        TokenType::PUNCTUATOR_PLUS_PLUS,
        TokenType::PUNCTUATOR_MINUS_MINUS,
        TokenType::KEYW_TYPEOF,
        TokenType::KEYW_VOID,
        TokenType::KEYW_DELETE,
        TokenType::KEYW_AWAIT,
    };

    for (auto type : trueCases) {
        Token token;
        token.SetTokenType(type);
        EXPECT_TRUE(token.IsUnary());
    }

    const TokenType falseCases[] = {
        TokenType::PUNCTUATOR_MULTIPLY,
        TokenType::PUNCTUATOR_DIVIDE,
        TokenType::LITERAL_IDENT,
    };

    for (auto type : falseCases) {
        Token token;
        token.SetTokenType(type);
        EXPECT_FALSE(token.IsUnary());
    }
}

// Test IsPropNameLiteral
TEST_F(TokenTest, TestIsPropNameLiteral)
{
    const TokenType trueCases[] = {
        TokenType::LITERAL_STRING,
        TokenType::LITERAL_NUMBER,
        TokenType::LITERAL_TRUE,
        TokenType::LITERAL_FALSE,
    };

    for (auto type : trueCases) {
        Token token;
        token.SetTokenType(type);
        EXPECT_TRUE(token.IsPropNameLiteral());
    }

    Token token;
    token.SetTokenType(TokenType::LITERAL_IDENT);
    EXPECT_FALSE(token.IsPropNameLiteral());
}

// Test IsBooleanOrNullLiteral
TEST_F(TokenTest, TestIsBooleanOrNullLiteral)
{
    const TokenType trueCases[] = {
        TokenType::LITERAL_NULL,
        TokenType::LITERAL_TRUE,
        TokenType::LITERAL_FALSE,
    };

    for (auto type : trueCases) {
        Token token;
        token.SetTokenType(type);
        EXPECT_TRUE(token.IsBooleanOrNullLiteral());
    }

    Token token;
    token.SetTokenType(TokenType::LITERAL_STRING);
    EXPECT_FALSE(token.IsBooleanOrNullLiteral());
}

// Test IsKeyword
TEST_F(TokenTest, TestIsKeyword)
{
    const TokenType trueCases[] = {
        TokenType::KEYW_IF,
        TokenType::KEYW_FOR,
        TokenType::KEYW_IN,
        TokenType::KEYW_INSTANCEOF,
    };

    for (auto type : trueCases) {
        Token token;
        token.SetTokenType(type);
        EXPECT_TRUE(token.IsKeyword());
    }

    Token token;
    token.SetTokenType(TokenType::LITERAL_IDENT);
    EXPECT_FALSE(token.IsKeyword());
}

// Test IsBinaryToken
TEST_F(TokenTest, TestIsBinaryToken)
{
    const TokenType trueCases[] = {
        TokenType::PUNCTUATOR_NULLISH_COALESCING,
        TokenType::PUNCTUATOR_LOGICAL_OR,
        TokenType::PUNCTUATOR_LOGICAL_AND,
        TokenType::PUNCTUATOR_BITWISE_OR,
        TokenType::PUNCTUATOR_BITWISE_XOR,
        TokenType::PUNCTUATOR_BITWISE_AND,
        TokenType::PUNCTUATOR_EQUAL,
        TokenType::PUNCTUATOR_STRICT_EQUAL,
        TokenType::PUNCTUATOR_LESS_THAN,
        TokenType::PUNCTUATOR_GREATER_THAN,
        TokenType::PUNCTUATOR_PLUS,
        TokenType::PUNCTUATOR_MINUS,
        TokenType::PUNCTUATOR_MULTIPLY,
        TokenType::PUNCTUATOR_DIVIDE,
        TokenType::PUNCTUATOR_MOD,
        TokenType::PUNCTUATOR_EXPONENTIATION,
    };

    for (auto type : trueCases) {
        EXPECT_TRUE(Token::IsBinaryToken(type));
    }

    EXPECT_FALSE(Token::IsBinaryToken(TokenType::LITERAL_IDENT));
    EXPECT_FALSE(Token::IsBinaryToken(TokenType::PUNCTUATOR_PLUS_PLUS));
}

// Test IsBinaryLvalueToken
TEST_F(TokenTest, TestIsBinaryLvalueToken)
{
    const TokenType trueCases[] = {
        TokenType::PUNCTUATOR_SUBSTITUTION,
        TokenType::PUNCTUATOR_PLUS_EQUAL,
        TokenType::PUNCTUATOR_MINUS_EQUAL,
        TokenType::PUNCTUATOR_MULTIPLY_EQUAL,
        TokenType::PUNCTUATOR_DIVIDE_EQUAL,
        TokenType::PUNCTUATOR_MOD_EQUAL,
        TokenType::PUNCTUATOR_EXPONENTIATION_EQUAL,
    };

    for (auto type : trueCases) {
        EXPECT_TRUE(Token::IsBinaryLvalueToken(type));
    }

    EXPECT_FALSE(Token::IsBinaryLvalueToken(TokenType::PUNCTUATOR_PLUS));
    EXPECT_FALSE(Token::IsBinaryLvalueToken(TokenType::LITERAL_IDENT));
}

// Test IsUpdateToken
TEST_F(TokenTest, TestIsUpdateToken)
{
    const TokenType trueCases[] = {
        TokenType::PUNCTUATOR_PLUS_PLUS,
        TokenType::PUNCTUATOR_MINUS_MINUS,
    };

    for (auto type : trueCases) {
        EXPECT_TRUE(Token::IsUpdateToken(type));
    }

    const TokenType falseCases[] = {
        TokenType::PUNCTUATOR_PLUS,
        TokenType::PUNCTUATOR_MINUS,
        TokenType::LITERAL_IDENT,
    };

    for (auto type : falseCases) {
        EXPECT_FALSE(Token::IsUpdateToken(type));
    }
}

// Test IsPunctuatorToken
TEST_F(TokenTest, TestIsPunctuatorToken)
{
    const TokenType trueCases[] = {
        TokenType::PUNCTUATOR_NULLISH_COALESCING,
        TokenType::PUNCTUATOR_LOGICAL_OR,
        TokenType::PUNCTUATOR_LOGICAL_AND,
        TokenType::PUNCTUATOR_ARROW,
    };

    for (auto type : trueCases) {
        EXPECT_TRUE(Token::IsPunctuatorToken(type));
    }

    EXPECT_FALSE(Token::IsPunctuatorToken(TokenType::LITERAL_IDENT));
    EXPECT_FALSE(Token::IsPunctuatorToken(TokenType::KEYW_IF));
}

// Test IsTsParamToken
TEST_F(TokenTest, TestIsTsParamToken)
{
    // Colon should always be a TS param token
    EXPECT_TRUE(Token::IsTsParamToken(TokenType::PUNCTUATOR_COLON, 'a'));
    EXPECT_TRUE(Token::IsTsParamToken(TokenType::PUNCTUATOR_COLON, ','));

    // Question mark followed by comma, colon, or right paren
    EXPECT_TRUE(Token::IsTsParamToken(TokenType::PUNCTUATOR_QUESTION_MARK, ','));
    EXPECT_TRUE(Token::IsTsParamToken(TokenType::PUNCTUATOR_QUESTION_MARK, ':'));
    EXPECT_TRUE(Token::IsTsParamToken(TokenType::PUNCTUATOR_QUESTION_MARK, ')'));

    // Question mark followed by other characters
    EXPECT_FALSE(Token::IsTsParamToken(TokenType::PUNCTUATOR_QUESTION_MARK, 'a'));
    EXPECT_FALSE(Token::IsTsParamToken(TokenType::PUNCTUATOR_QUESTION_MARK, '+'));

    // Other token types
    EXPECT_FALSE(Token::IsTsParamToken(TokenType::LITERAL_IDENT, ','));
    EXPECT_FALSE(Token::IsTsParamToken(TokenType::PUNCTUATOR_PLUS, ','));
}

// Test Token location information
TEST_F(TokenTest, TestTokenLocation)
{
    Token token;
    auto start = token.Start();
    auto end = token.End();
    auto loc = token.Loc();
    
    (void)start;
    (void)end;
    (void)loc;
}

// Test Token ident operations
TEST_F(TokenTest, TestTokenIdent)
{
    Token token;
    util::StringView ident("test");
    token.SetIdent(ident);
    
    EXPECT_EQ(token.Ident().Utf8(), "test");
}

// Test Token keyword type
TEST_F(TokenTest, TestTokenKeywordType)
{
    Token token;
    EXPECT_EQ(token.KeywordType(), TokenType::EOS);
}

// Test IsAccessability with all branches
TEST_F(TokenTest, TestIsAccessabilityAllBranches)
{
    auto lexer1 = CreateLexerTS("public");
    lexer1.NextToken();
    auto token1 = lexer1.GetToken();
    EXPECT_TRUE(token1.IsAccessability());

    auto lexer2 = CreateLexerTS("protected");
    lexer2.NextToken();
    auto token2 = lexer2.GetToken();
    EXPECT_TRUE(token2.IsAccessability());

    auto lexer3 = CreateLexerTS("private");
    lexer3.NextToken();
    auto token3 = lexer3.GetToken();
    EXPECT_TRUE(token3.IsAccessability());

    // Test with escape that forms a non-keyword identifier (should be false)
    auto lexer4 = CreateLexerTS("\\u0071ublic");  // \u0071 is 'q', so this is "qublic" (not a keyword)
    lexer4.NextToken();
    auto token4 = lexer4.GetToken();
    EXPECT_FALSE(token4.IsAccessability());

    // Test non-identifier type (should be false)
    Token token5;
    token5.SetTokenType(TokenType::KEYW_PUBLIC);
    EXPECT_FALSE(token5.IsAccessability());

    // Test identifier with different keywordType (should be false)
    auto lexer6 = CreateLexer("async");
    lexer6.NextToken();
    auto token6 = lexer6.GetToken();
    EXPECT_FALSE(token6.IsAccessability());
}

// Test IsAsyncModifier with all branches
TEST_F(TokenTest, TestIsAsyncModifierAllBranches)
{
    // Test async keyword (should be true)
    auto lexer1 = CreateLexer("async");
    lexer1.NextToken();
    auto token1 = lexer1.GetToken();
    EXPECT_TRUE(token1.IsAsyncModifier());

    // Test with escape that forms a non-keyword identifier (should be false)
    auto lexer2 = CreateLexer("\\u0062sync");  // \u0062 is 'b', so this is "bsync" (not a keyword)
    lexer2.NextToken();
    auto token2 = lexer2.GetToken();
    EXPECT_FALSE(token2.IsAsyncModifier());

    // Test non-identifier type (should be false)
    Token token3;
    token3.SetTokenType(TokenType::KEYW_ASYNC);
    EXPECT_FALSE(token3.IsAsyncModifier());

    // Test identifier with different keywordType (should be false)
    auto lexer4 = CreateLexer("if");
    lexer4.NextToken();
    auto token4 = lexer4.GetToken();
    EXPECT_FALSE(token4.IsAsyncModifier());
}

// Test IsStaticModifier with all branches
TEST_F(TokenTest, TestIsStaticModifierAllBranches)
{
    // Test static keyword (should be true) - use TS mode to avoid strict mode error
    auto lexer1 = CreateLexerTS("static");
    lexer1.NextToken();
    auto token1 = lexer1.GetToken();
    EXPECT_TRUE(token1.IsStaticModifier());

    // Test with escape that forms a non-keyword identifier (should be false)
    auto lexer2 = CreateLexerTS("\\u0074tatic");  // \u0074 is 't', so this is "ttatic" (not a keyword)
    lexer2.NextToken();
    auto token2 = lexer2.GetToken();
    EXPECT_FALSE(token2.IsStaticModifier());

    // Test non-identifier type (should be false)
    Token token3;
    token3.SetTokenType(TokenType::KEYW_STATIC);
    EXPECT_FALSE(token3.IsStaticModifier());

    // Test identifier with different keywordType (should be false)
    auto lexer4 = CreateLexer("async");
    lexer4.NextToken();
    auto token4 = lexer4.GetToken();
    EXPECT_FALSE(token4.IsStaticModifier());
}

// Test IsDeclareModifier with all branches
TEST_F(TokenTest, TestIsDeclareModifierAllBranches)
{
    // Test declare keyword (should be true)
    auto lexer1 = CreateLexer("declare");
    lexer1.NextToken();
    auto token1 = lexer1.GetToken();
    EXPECT_TRUE(token1.IsDeclareModifier());

    // Test with escape that forms a non-keyword identifier (should be false)
    auto lexer2 = CreateLexer("\\u0065eclare");  // \u0065 is 'e', so this is "edeclare" (not a keyword)
    lexer2.NextToken();
    auto token2 = lexer2.GetToken();
    EXPECT_FALSE(token2.IsDeclareModifier());

    // Test non-identifier type (should be false)
    Token token3;
    token3.SetTokenType(TokenType::KEYW_DECLARE);
    EXPECT_FALSE(token3.IsDeclareModifier());

    // Test identifier with different keywordType (should be false)
    auto lexer4 = CreateLexer("async");
    lexer4.NextToken();
    auto token4 = lexer4.GetToken();
    EXPECT_FALSE(token4.IsDeclareModifier());
}

// Test IsReadonlyModifier with all branches
TEST_F(TokenTest, TestIsReadonlyModifierAllBranches)
{
    // Test readonly keyword (should be true)
    auto lexer1 = CreateLexer("readonly");
    lexer1.NextToken();
    auto token1 = lexer1.GetToken();
    EXPECT_TRUE(token1.IsReadonlyModifier());

    // Test with escape that forms a non-keyword identifier (should be false)
    auto lexer2 = CreateLexer("\\u0073eadonly");  // \u0073 is 's', so this is "seadonly" (not a keyword)
    lexer2.NextToken();
    auto token2 = lexer2.GetToken();
    EXPECT_FALSE(token2.IsReadonlyModifier());

    // Test non-identifier type (should be false)
    Token token3;
    token3.SetTokenType(TokenType::KEYW_READONLY);
    EXPECT_FALSE(token3.IsReadonlyModifier());

    // Test identifier with different keywordType (should be false)
    auto lexer4 = CreateLexer("async");
    lexer4.NextToken();
    auto token4 = lexer4.GetToken();
    EXPECT_FALSE(token4.IsReadonlyModifier());
}

// Test IsAccessorModifier with all branches
TEST_F(TokenTest, TestIsAccessorModifierAllBranches)
{
    // Test accessor keyword (should be true)
    auto lexer1 = CreateLexer("accessor");
    lexer1.NextToken();
    auto token1 = lexer1.GetToken();
    EXPECT_TRUE(token1.IsAccessorModifier());

    // Test with escape that forms a non-keyword identifier (should be false)
    auto lexer2 = CreateLexer("\\u0062ccessor");  // \u0062 is 'b', so this is "bccessor" (not a keyword)
    lexer2.NextToken();
    auto token2 = lexer2.GetToken();
    EXPECT_FALSE(token2.IsAccessorModifier());

    // Test non-identifier type (should be false)
    Token token3;
    token3.SetTokenType(TokenType::KEYW_ACCESSOR);
    EXPECT_FALSE(token3.IsAccessorModifier());

    // Test identifier with different keywordType (should be false)
    auto lexer4 = CreateLexer("async");
    lexer4.NextToken();
    auto token4 = lexer4.GetToken();
    EXPECT_FALSE(token4.IsAccessorModifier());
}

// Test IsReservedTypeName with all switch cases
TEST_F(TokenTest, TestIsReservedTypeNameAllCases)
{
    // Test all reserved type names (TypeScript type keywords)
    // Use TS mode to properly parse TypeScript type keywords
    struct TestCase {
        std::string keyword;
        bool expected;
    };

    std::vector<TestCase> testCases = {
        {"any", true},
        {"unknown", true},
        {"never", true},
        {"number", true},
        {"bigint", true},
        {"boolean", true},
        {"string", true},
        {"void", true},
        {"object", true},
        {"async", false},
        {"if", false},
    };

    for (const auto &testCase : testCases) {
        // Use TS mode for TypeScript type keywords
        auto lexer = CreateLexerTS(testCase.keyword);
        lexer.NextToken();
        auto token = lexer.GetToken();
        EXPECT_EQ(token.IsReservedTypeName(), testCase.expected)
            << "Failed for keyword: " << testCase.keyword;
    }

    // Test default case (non-keyword identifier)
    auto lexer = CreateLexer("identifier");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_FALSE(token.IsReservedTypeName());
}

// Test IsJsStrictReservedWord with all switch cases
TEST_F(TokenTest, TestIsJsStrictReservedWordAllCases)
{
    // Test all JS strict reserved words
    // Note: Some keywords are strict mode reserved words in JS and require TS mode
    struct TestCase {
        std::string keyword;
        bool expected;
        bool useTS;
    };

    std::vector<TestCase> testCases = {
        // These keywords work in JS mode
        {"arguments", true, false},  // KEYW_ARGUMENTS
        {"eval", true, false},       // KEYW_EVAL
        {"let", true, false},        // KEYW_LET
        // These keywords are strict mode reserved words - use TS mode
        // Note: CheckFutureReservedKeyword allows these in TS mode when keywordType <= KEYW_INTERFACE
        {"static", true, true},      // KEYW_STATIC (175 <= 182)
        {"private", true, true},     // KEYW_PRIVATE (178 <= 182)
        {"protected", true, true},   // KEYW_PROTECTED (179 <= 182)
        {"public", true, true},       // KEYW_PUBLIC (180 <= 182)
        {"implements", true, true},  // KEYW_IMPLEMENTS (181 <= 182)
        {"interface", true, true},   // KEYW_INTERFACE (182 <= 182)
        // These are not strict reserved words (test default case)
        {"async", false, false},  // Not a strict reserved word
        {"if", false, false},     // Not a strict reserved word
        {"function", false, false}, // Not a strict reserved word
        {"class", false, false},   // Not a strict reserved word
    };

    for (const auto &testCase : testCases) {
        if (testCase.useTS) {
            auto lexer = CreateLexerTS(testCase.keyword);
            lexer.NextToken();
            auto token = lexer.GetToken();
            EXPECT_EQ(token.IsJsStrictReservedWord(), testCase.expected)
                << "Failed for keyword: " << testCase.keyword;
        } else {
            auto lexer = CreateLexer(testCase.keyword);
            lexer.NextToken();
            auto token = lexer.GetToken();
            EXPECT_EQ(token.IsJsStrictReservedWord(), testCase.expected)
                << "Failed for keyword: " << testCase.keyword;
        }
    }

    // Test default case (non-keyword identifier)
    auto lexer = CreateLexer("identifier");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_FALSE(token.IsJsStrictReservedWord());
}

// Test IsKeyword with edge cases
TEST_F(TokenTest, TestIsKeywordEdgeCases)
{
    // Test FIRST_KEYW boundary (KEYW_ANY is FIRST_KEYW)
    Token token1;
    token1.SetTokenType(TokenType::KEYW_ANY);
    EXPECT_TRUE(token1.IsKeyword());

    // Test KEYW_IN (special case - not >= FIRST_KEYW but should return true)
    Token token2;
    token2.SetTokenType(TokenType::KEYW_IN);
    EXPECT_TRUE(token2.IsKeyword());

    // Test KEYW_INSTANCEOF (special case - not >= FIRST_KEYW but should return true)
    Token token3;
    token3.SetTokenType(TokenType::KEYW_INSTANCEOF);
    EXPECT_TRUE(token3.IsKeyword());

    // Test keyword after FIRST_KEYW
    Token token4;
    token4.SetTokenType(TokenType::KEYW_IF);
    EXPECT_TRUE(token4.IsKeyword());

    // Test non-keyword (LITERAL_IDENT)
    Token token5;
    token5.SetTokenType(TokenType::LITERAL_IDENT);
    EXPECT_FALSE(token5.IsKeyword());

    // Test non-keyword (PUNCTUATOR)
    Token token6;
    token6.SetTokenType(TokenType::PUNCTUATOR_PLUS);
    EXPECT_FALSE(token6.IsKeyword());
}
}  // namespace panda::es2panda::lexer
