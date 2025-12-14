/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

class LexerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mm_ = std::make_unique<MemManager>();
        program_ = std::make_unique<parser::Program>(es2panda::ScriptExtension::JS);
        parserContext_ = std::make_unique<parser::ParserContext>(program_.get());
    }

    void TearDown() override
    {
        parserContext_.reset();
        program_.reset();
        mm_.reset();
    }

    Lexer CreateLexer(const std::string &source)
    {
        program_->SetSource(source, "test.js", false);
        return Lexer(parserContext_.get());
    }

    std::unique_ptr<MemManager> mm_;
    std::unique_ptr<parser::Program> program_;
    std::unique_ptr<parser::ParserContext> parserContext_;
};

// Test basic identifier tokenization
TEST_F(LexerTest, TestIdentifier)
{
    auto lexer = CreateLexer("identifier");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(token.Ident().Utf8(), "identifier");
}

// Test numeric literals
TEST_F(LexerTest, TestBasicNumericLiterals)
{
    struct TestCase {
        std::string input;
        double expected;
    };

    std::vector<TestCase> testCases = {
        {"123", 123.0}, // Decimal number
        {"123.456", 123.456}, // Decimal number with point
        {"0xFF", 255.0}, // Hexadecimal number
        {"0b1010", 10.0}, // Binary number
        {"0o755", 493.0}, // Octal number
    };

    for (const auto &testCase : testCases) {
        auto lexer = CreateLexer(testCase.input);
        lexer.NextToken();
        auto token = lexer.GetToken();
        EXPECT_EQ(token.Type(), TokenType::LITERAL_NUMBER);
        EXPECT_EQ(token.Number(), testCase.expected);
    }
}

TEST_F(LexerTest, TestBigInt)
{
    auto lexer = CreateLexer("123n");
    lexer.NextToken(LexerNextTokenFlags::BIGINT_ALLOWED);
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_NUMBER);
    EXPECT_TRUE(token.Flags() & TokenFlags::NUMBER_BIGINT);
}

TEST_F(LexerTest, TestNumericSeparator)
{
    const double expectedNumber = 1000000.0;
    auto lexer = CreateLexer("1_000_000");
    lexer.NextToken(LexerNextTokenFlags::NUMERIC_SEPARATOR_ALLOWED);
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_NUMBER);
    EXPECT_EQ(token.Number(), expectedNumber);
    EXPECT_TRUE(token.Flags() & TokenFlags::NUMBER_HAS_UNDERSCORE);
}

// Test numeric separator edge cases
TEST_F(LexerTest, TestNumericSeparatorAtStart)
{
    auto lexer = CreateLexer("_123");
    lexer.NextToken();
    // Should be treated as identifier starting with underscore
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::LITERAL_IDENT);
}

// Test exponential notation
TEST_F(LexerTest, TestExponentialNotation)
{
    const double expectedNumber = 100000.0;
    auto lexer = CreateLexer("1e5");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_NUMBER);
    EXPECT_DOUBLE_EQ(token.Number(), expectedNumber);
}

// Test string literals
TEST_F(LexerTest, TestDoubleQuotedString)
{
    auto lexer = CreateLexer("\"hello world\"");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_STRING);
    EXPECT_EQ(token.String().Utf8(), "hello world");
}

TEST_F(LexerTest, TestSingleQuotedString)
{
    auto lexer = CreateLexer("'hello world'");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_STRING);
    EXPECT_EQ(token.String().Utf8(), "hello world");
}

TEST_F(LexerTest, TestStringWithEscape)
{
    auto lexer = CreateLexer("\"hello\\nworld\"");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_STRING);
    EXPECT_TRUE(token.Flags() & TokenFlags::HAS_ESCAPE);
}

// Test punctuators
TEST_F(LexerTest, TestPunctuators)
{
    struct TestCase {
        std::string input;
        TokenType expected;
    };

    std::vector<TestCase> testCases = {
        {"+", TokenType::PUNCTUATOR_PLUS},
        {"-", TokenType::PUNCTUATOR_MINUS},
        {"*", TokenType::PUNCTUATOR_MULTIPLY},
        {"/", TokenType::PUNCTUATOR_DIVIDE},
        {"%", TokenType::PUNCTUATOR_MOD},
        {"=", TokenType::PUNCTUATOR_SUBSTITUTION},
        {"==", TokenType::PUNCTUATOR_EQUAL},
        {"===", TokenType::PUNCTUATOR_STRICT_EQUAL},
        {"!", TokenType::PUNCTUATOR_EXCLAMATION_MARK},
        {"!=", TokenType::PUNCTUATOR_NOT_EQUAL},
        {"!==", TokenType::PUNCTUATOR_NOT_STRICT_EQUAL},
        {"<", TokenType::PUNCTUATOR_LESS_THAN},
        {"<=", TokenType::PUNCTUATOR_LESS_THAN_EQUAL},
        {">", TokenType::PUNCTUATOR_GREATER_THAN},
        {">=", TokenType::PUNCTUATOR_GREATER_THAN_EQUAL},
        {"&&", TokenType::PUNCTUATOR_LOGICAL_AND},
        {"||", TokenType::PUNCTUATOR_LOGICAL_OR},
        {"??", TokenType::PUNCTUATOR_NULLISH_COALESCING},
        {"++", TokenType::PUNCTUATOR_PLUS_PLUS},
        {"--", TokenType::PUNCTUATOR_MINUS_MINUS},
        {"(", TokenType::PUNCTUATOR_LEFT_PARENTHESIS},
        {")", TokenType::PUNCTUATOR_RIGHT_PARENTHESIS},
        {"{", TokenType::PUNCTUATOR_LEFT_BRACE},
        {"}", TokenType::PUNCTUATOR_RIGHT_BRACE},
        {"[", TokenType::PUNCTUATOR_LEFT_SQUARE_BRACKET},
        {"]", TokenType::PUNCTUATOR_RIGHT_SQUARE_BRACKET},
        {".", TokenType::PUNCTUATOR_PERIOD},
        {",", TokenType::PUNCTUATOR_COMMA},
        {":", TokenType::PUNCTUATOR_COLON},
        {";", TokenType::PUNCTUATOR_SEMI_COLON},
        {"?", TokenType::PUNCTUATOR_QUESTION_MARK},
        {"=>", TokenType::PUNCTUATOR_ARROW},
    };

    for (const auto &testCase : testCases) {
        auto lexer = CreateLexer(testCase.input);
        lexer.NextToken();
        auto token = lexer.GetToken();
        EXPECT_EQ(token.Type(), testCase.expected) << "Failed for input: " << testCase.input;
    }
}

// Test keywords, except undefined
TEST_F(LexerTest, TestKeywords)
{
    struct TestCase {
        std::string input;
        TokenType expected;
    };

    std::vector<TestCase> testCases = {
        {"if", TokenType::KEYW_IF},
        {"else", TokenType::KEYW_ELSE},
        {"for", TokenType::KEYW_FOR},
        {"while", TokenType::KEYW_WHILE},
        {"function", TokenType::KEYW_FUNCTION},
        {"return", TokenType::KEYW_RETURN},
        {"var", TokenType::KEYW_VAR},
        {"let", TokenType::KEYW_LET},
        {"const", TokenType::KEYW_CONST},
        {"true", TokenType::LITERAL_TRUE},
        {"false", TokenType::LITERAL_FALSE},
        {"null", TokenType::LITERAL_NULL},
        {"class", TokenType::KEYW_CLASS},
        {"extends", TokenType::KEYW_EXTENDS},
        {"super", TokenType::KEYW_SUPER},
        {"this", TokenType::KEYW_THIS},
        {"new", TokenType::KEYW_NEW},
        {"typeof", TokenType::KEYW_TYPEOF},
        {"instanceof", TokenType::KEYW_INSTANCEOF},
        {"in", TokenType::KEYW_IN},
        {"break", TokenType::KEYW_BREAK},
        {"continue", TokenType::KEYW_CONTINUE},
        {"switch", TokenType::KEYW_SWITCH},
        {"case", TokenType::KEYW_CASE},
        {"default", TokenType::KEYW_DEFAULT},
        {"try", TokenType::KEYW_TRY},
        {"catch", TokenType::KEYW_CATCH},
        {"finally", TokenType::KEYW_FINALLY},
        {"throw", TokenType::KEYW_THROW},
    };

    for (const auto &testCase : testCases) {
        auto lexer = CreateLexer(testCase.input);
        lexer.NextToken();
        auto token = lexer.GetToken();
        EXPECT_EQ(token.Type(), testCase.expected) << "Failed for keyword: " << testCase.input;
    }
}

// Test undefined
TEST_F(LexerTest, TestUndefined)
{
    auto lexer = CreateLexer("undefined");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(token.KeywordType(), TokenType::KEYW_UNDEFINED);
}

// Test arrow function detection
TEST_F(LexerTest, TestCheckArrow)
{
    auto lexer = CreateLexer("=>");
    EXPECT_TRUE(lexer.CheckArrow());
}

// Test unicode escape sequences
TEST_F(LexerTest, TestUnicodeEscape)
{
    auto lexer = CreateLexer("\"\\u0041\"");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_STRING);
    EXPECT_TRUE(token.Flags() & TokenFlags::HAS_ESCAPE);
}

// Test comments-1
TEST_F(LexerTest, TestSingleLineComment)
{
    auto lexer = CreateLexer("// This is a comment\nidentifier");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(token.Ident().Utf8(), "identifier");
}

// Test comments-2
TEST_F(LexerTest, TestMultiLineComment)
{
    auto lexer = CreateLexer("/* This is a\nmulti-line comment */identifier");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(token.Ident().Utf8(), "identifier");
}

// Test whitespace handling
TEST_F(LexerTest, TestWhitespace)
{
    auto lexer = CreateLexer("   \t\n   identifier");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(token.Ident().Utf8(), "identifier");
}

// Test line counting
TEST_F(LexerTest, TestLineCounting)
{
    const size_t expectedLine1 = 0;
    const size_t expectedLine2 = 1;
    const size_t expectedLine3 = 2;
    auto lexer = CreateLexer("line1\nline2\nline3");
    lexer.NextToken();
    EXPECT_EQ(lexer.Line(), expectedLine1);
    
    lexer.NextToken(); // Skip to line2
    EXPECT_EQ(lexer.Line(), expectedLine2);
    
    lexer.NextToken(); // Skip to line3
    EXPECT_EQ(lexer.Line(), expectedLine3);
}

// Test position save and restore
TEST_F(LexerTest, TestPositionSaveAndRestore)
{
    auto lexer = CreateLexer("token1 token2 token3");
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Ident().Utf8(), "token1");
    
    auto pos = lexer.Save();
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Ident().Utf8(), "token2");
    
    lexer.Rewind(pos);
    EXPECT_EQ(lexer.GetToken().Ident().Utf8(), "token1");
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Ident().Utf8(), "token2");
}

// Test lookahead
TEST_F(LexerTest, TestLookahead)
{
    auto lexer = CreateLexer("abc");
    char32_t next = lexer.Lookahead();
    EXPECT_EQ(next, 'a');
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Ident().Utf8(), "abc");
}

// Test token sequence
TEST_F(LexerTest, TestTokenSequence)
{
    const std::string expectedVarName = "x";
    const double expectedValue = 42.0;
    auto lexer = CreateLexer("var x = 42;");
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::KEYW_VAR);
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(lexer.GetToken().Ident().Utf8(), expectedVarName);
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::PUNCTUATOR_SUBSTITUTION);
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::LITERAL_NUMBER);
    EXPECT_EQ(lexer.GetToken().Number(), expectedValue);
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::PUNCTUATOR_SEMI_COLON);
}

// Test EOS (End of Source)
TEST_F(LexerTest, TestEndOfSource)
{
    auto lexer = CreateLexer("token");
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::LITERAL_IDENT);
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::EOS);
}

// Test exponential notation negative
TEST_F(LexerTest, TestExponentialNotationNegative)
{
    const double expectedNumber = 0.00001;
    auto lexer = CreateLexer("1e-5");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_NUMBER);
    EXPECT_DOUBLE_EQ(token.Number(), expectedNumber);
}

// Test regexp scanning
TEST_F(LexerTest, TestRegExp)
{
    auto lexer = CreateLexer("/pattern/g");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::PUNCTUATOR_DIVIDE);

    lexer.ResetTokenEnd();
    auto regexp = lexer.ScanRegExp();
    EXPECT_EQ(regexp.patternStr, "pattern");
    EXPECT_EQ(regexp.flagsStr, "g");
}

// Test keyword to identifier conversion flag
TEST_F(LexerTest, TestKeywordToIdentFlag)
{
    auto lexer = CreateLexer("if");
    lexer.NextToken(LexerNextTokenFlags::KEYWORD_TO_IDENT);
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(token.Ident().Utf8(), "if");
}

// Test multiple tokens in sequence
TEST_F(LexerTest, TestMultipleTokens)
{
    const std::string expectedVarName = "x";
    const double expectedValueNumber0 = 10.0;
    const double expectedValueNumber1 = 20.0;
    auto lexer = CreateLexer("let x = 10 + 20;");
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::KEYW_LET);
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(lexer.GetToken().Ident().Utf8(), expectedVarName);
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::PUNCTUATOR_SUBSTITUTION);
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::LITERAL_NUMBER);
    EXPECT_EQ(lexer.GetToken().Number(), expectedValueNumber0);
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::PUNCTUATOR_PLUS);
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::LITERAL_NUMBER);
    EXPECT_EQ(lexer.GetToken().Number(), expectedValueNumber1);
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Type(), TokenType::PUNCTUATOR_SEMI_COLON);
}

// Test backward and forward token operations
TEST_F(LexerTest, TestBackwardForwardToken)
{
    std::string token1 = "token1";
    std::string token2 = "token2";
    auto lexer = CreateLexer(token1 + " " + token2);
    
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Ident().Utf8(), token1);
    
    lexer.BackwardToken(TokenType::LITERAL_IDENT, token1.length());
    EXPECT_EQ(lexer.GetToken().Ident().Utf8(), token1);
    
    lexer.ForwardToken(TokenType::LITERAL_IDENT, token1.length());
    lexer.NextToken();
    EXPECT_EQ(lexer.GetToken().Ident().Utf8(), token2);
}

// Test template strings
TEST_F(LexerTest, TestTemplateString)
{
    auto lexer = CreateLexer("`hello world`");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::PUNCTUATOR_BACK_TICK);

    lexer.NextToken();
    token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(token.Ident().Utf8(), "hello");

    lexer.NextToken();
    token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(token.Ident().Utf8(), "world");

    lexer.NextToken();
    token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::PUNCTUATOR_BACK_TICK);
}

TEST_F(LexerTest, TestTemplateStringWithExpression)
{
    auto lexer = CreateLexer("`hello ${name}`");
    lexer.NextToken();
    auto token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::PUNCTUATOR_BACK_TICK);

    lexer.NextToken();
    token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(token.Ident().Utf8(), "hello");

    lexer.NextToken();
    token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(token.Ident().Utf8(), "$");

    lexer.NextToken();
    token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::PUNCTUATOR_LEFT_BRACE);

    lexer.NextToken();
    token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::LITERAL_IDENT);
    EXPECT_EQ(token.Ident().Utf8(), "name");

    lexer.NextToken();
    token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::PUNCTUATOR_RIGHT_BRACE);

    lexer.NextToken();
    token = lexer.GetToken();
    EXPECT_EQ(token.Type(), TokenType::PUNCTUATOR_BACK_TICK);
}

// Test error handling - unterminated string
TEST_F(LexerTest, TestUnterminatedString)
{
    auto lexer = CreateLexer("\"unterminated");
    EXPECT_THROW(lexer.NextToken(), es2panda::Error);
}

// Test error handling - invalid number
TEST_F(LexerTest, TestInvalidNumber)
{
    auto lexer = CreateLexer("123abc");
    EXPECT_THROW(lexer.NextToken(), es2panda::Error);
}

// Test error handling - invalid octal digit
TEST_F(LexerTest, TestInvalidOctalDigit)
{
    auto lexer = CreateLexer("0o89");
    EXPECT_THROW(lexer.NextToken(), es2panda::Error);
}

// Test error handling - newline in string (non-template)
TEST_F(LexerTest, TestNewlineInString)
{
    auto lexer = CreateLexer("\"hello\nworld\"");
    EXPECT_THROW(lexer.NextToken(), es2panda::Error);
}

}  // namespace panda::es2panda::lexer
