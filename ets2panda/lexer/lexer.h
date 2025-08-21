/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_PARSER_CORE_LEXER_H
#define ES2PANDA_PARSER_CORE_LEXER_H

#include <limits>
#include <type_traits>
#include "lexer/regexp/regexp.h"
#include "lexer/token/letters.h"
#include "lexer/token/token.h"
#include "util/enumbitops.h"

namespace ark::es2panda::parser {
class ParserContext;
class ETSNolintParser;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::lexer {
class Keywords;
class KeywordsUtil;

using ENUMBITOPS_OPERATORS;

enum class NextTokenFlags : uint32_t {
    NONE = 0U,
    KEYWORD_TO_IDENT = 1U << 0U,
    NUMERIC_SEPARATOR_ALLOWED = 1U << 1U,
    BIGINT_ALLOWED = 1U << 2U,
    UNARY_MINUS = 1U << 3U,
};

class LexerPosition {
public:
    explicit LexerPosition(const util::StringView &source);
    DEFAULT_COPY_SEMANTIC(LexerPosition);
    DEFAULT_MOVE_SEMANTIC(LexerPosition);
    ~LexerPosition() = default;

    util::StringView::Iterator &Iterator()
    {
        return iterator_;
    }

    const util::StringView::Iterator &Iterator() const
    {
        return iterator_;
    }

    size_t Line() const
    {
        return line_;
    }

    Token &GetToken()
    {
        return token_;
    }

    const Token &GetToken() const
    {
        return token_;
    }

    size_t &NextTokenLine()
    {
        return nextTokenLine_;
    }

    bool operator==(const LexerPosition &other) const
    {
        return iterator_.Save() == other.iterator_.Save();
    }

    bool operator!=(const LexerPosition &other) const
    {
        return !(*this == other);
    }

private:
    friend class Lexer;

    Token token_ {};
    util::StringView::Iterator iterator_;
    size_t line_ {};
    size_t nextTokenLine_ {};
};

class LexerTemplateString {
public:
    explicit LexerTemplateString(ArenaAllocator *allocator) : str(allocator) {}
    DEFAULT_COPY_SEMANTIC(LexerTemplateString);
    DEFAULT_MOVE_SEMANTIC(LexerTemplateString);
    ~LexerTemplateString() = default;

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    util::UString str;
    size_t end {};
    bool scanExpression {};
    bool validSequence {true};
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

class TemplateLiteralParserContext;

class Lexer {
public:
    explicit Lexer(const parser::ParserContext *parserContext, util::DiagnosticEngine &diagnosticEngine,
                   bool startLexer = true);
    NO_COPY_SEMANTIC(Lexer);
    NO_MOVE_SEMANTIC(Lexer);
    virtual ~Lexer() = default;

    // NOLINTNEXTLINE(google-default-arguments)
    virtual void NextToken(NextTokenFlags flags = NextTokenFlags::NONE);
    virtual void ScanAsteriskPunctuator();

    Token &GetToken();
    const Token &GetToken() const;
    size_t Line() const;
    const parser::Program *GetProgram() const;

    bool TryEatTokenType(lexer::TokenType type)
    {
        auto token = GetToken();
        if (token.Type() == type) {
            NextToken();
            return true;
        }
        return false;
    }

    bool TryEatTokenFromKeywordType(lexer::TokenType type)
    {
        auto token = GetToken();
        if (token.KeywordType() == type) {
            NextToken();
            return true;
        }
        return false;
    }

    util::DiagnosticEngine &DiagnosticEngine()
    {
        return diagnosticEngine_;
    }

    std::optional<Token> TryEatTokenKeyword(lexer::TokenType type)
    {
        auto token = GetToken();
        if (token.KeywordType() == type) {
            NextToken();
            return token;
        }
        return std::nullopt;
    }

    LexerPosition Save() const;
    void Rewind(const LexerPosition &pos);
    void Reset(size_t offset);
    void BackwardToken(TokenType type, size_t offset);
    void ForwardToken(TokenType type, size_t offset);

    char32_t Lookahead();
    bool CheckArrow();

    RegExp ScanRegExp();

    void HandleNewlineHelper(util::UString *str, size_t *escapeEnd);
    bool HandleBackslashHelper(util::UString *str, size_t *escapeEnd);
    bool HandleDollarSignHelper(const char32_t &end);
    bool HandleDoubleQuoteHelper(const char32_t &end, const char32_t &cp);
    void PrepareStringTokenHelper();
    void FinalizeTokenHelper(util::UString *str, const size_t &startPos, size_t escapeEnd, bool finalize = true);
    template <char32_t END>
    void ScanString();

    void ResetTokenEnd();
    bool CheckOctalDigit(char32_t const nextCp);
    std::tuple<bool, bool, LexerTemplateString> ScanTemplateStringCpHelper(char32_t cp,
                                                                           LexerTemplateString templateStr);
    LexerTemplateString ScanTemplateString();
    util::StringView ScanMultilineString();
    void ScanTemplateStringEnd();
    void PushTemplateContext(TemplateLiteralParserContext *ctx);
    void LogUnexpectedStrictModeReservedKeyword() const
    {
        LogError(diagnostic::UNEXPECTED_STRICT_MODE_RESERVED_KEYWORD);
    }

    enum class ConversionResult : uint8_t {
        SUCCESS,
        INVALID_ARGUMENT,
        OUT_OF_RANGE,
    };

    template <typename Tret, typename Ret = Tret, typename... Base>
    static Ret StrToNumeric(Tret (*converter)(const char *, char **, Base...), const char *str,
                            ConversionResult &result, Base... base) noexcept
    {
        Ret ret {};
        char *endPtr;
        // NOLINTBEGIN(cppcoreguidelines-special-member-functions)
        struct SaveErrno {
            explicit SaveErrno() : errno_(errno)
            {
                errno = 0;
            }
            ~SaveErrno()
            {
                if (errno == 0) {
                    errno = errno_;
                }
            }

        private:
            decltype(errno) errno_;
        } const savedErrno;
        // NOLINTEND(cppcoreguidelines-special-member-functions)

        const Tret tmp = converter(str, &endPtr, base...);

        bool outOfRange = false;
        if constexpr (!std::is_same_v<Ret, Tret>) {
            auto min = std::numeric_limits<Ret>::lowest();
            auto max = std::numeric_limits<Ret>::max();
            if constexpr (std::is_same_v<Ret, float>) {
                const double delta = 0.0000001e38;
                outOfRange = (tmp < (min - delta)) || (tmp > (max + delta));
            } else {
                outOfRange = (tmp < min) || (tmp > max);
            }
        }

        if constexpr (std::is_floating_point_v<Tret>) {
            outOfRange |= (tmp == std::numeric_limits<Tret>::infinity());
        } else {
            outOfRange |= (errno == ERANGE);
        }

        if (endPtr == str) {
            result = ConversionResult::INVALID_ARGUMENT;
        } else if (outOfRange) {
            result = ConversionResult::OUT_OF_RANGE;
        } else {
            result = ConversionResult::SUCCESS;
            ret = tmp;
        }

        return ret;
    }

    util::StringView SourceView(size_t begin, size_t end) const;

    lexer::SourcePosition GetPositionForDiagnostic() const
    {
        return GetToken().Start();
    }

protected:
    void NextToken(Keywords *kws);
    ArenaAllocator *Allocator();
    bool IsLineTerminatorOrEos() const;
    bool ScanRegExpPattern();
    RegExpFlags ScanRegExpFlags();

    void LogUnexpectedToken(lexer::TokenType const tokenType) const;

    void LogError(const diagnostic::DiagnosticKind &diagnostic,
                  const util::DiagnosticMessageParams &diagnosticParams = {}) const;
    void LogError(const diagnostic::DiagnosticKind &diagnostic, const util::DiagnosticMessageParams &diagnosticParams,
                  const lexer::SourcePosition &pos) const;

    void SetTokenStart();
    void SetTokenEnd();

    inline util::StringView::Iterator &Iterator()
    {
        return pos_.iterator_;
    }

    inline const util::StringView::Iterator &Iterator() const
    {
        return pos_.iterator_;
    }

    util::StringView SourceView(const util::StringView::Iterator &begin, const util::StringView::Iterator &end) const;

    bool SkipWhiteSpacesHelperSlash(char32_t *cp);
    bool SkipWhiteSpacesHelperDefault(const char32_t &cp);
    void SkipWhiteSpaces();
    void SkipSingleLineComment();

    bool ScanPunctuator();
    void ScanQuestionPunctuator();
    void ScanLessThanPunctuator();
    void ScanGreaterThanPunctuator();
    virtual void ScanEqualsPunctuator();
    virtual void ScanExclamationPunctuator();
    void ScanAmpersandPunctuator();
    void ScanVLinePunctuator();
    void ScanCircumflexPunctuator();
    void ScanPlusPunctuator();
    void ScanMinusPunctuator();
    void ScanSlashPunctuator();
    void ScanPercentPunctuator();
    void ScanDotPunctuator(KeywordsUtil &kwu);
    void ScanColonPunctuator();
    virtual bool ScanDollarPunctuator();
    void ScanAtPunctuator();

    virtual void SkipMultiLineComment();
    virtual void ScanHashMark();
    virtual void ScanBackTick();

    virtual bool ScanCharLiteral()
    {
        return false;
    }

    char32_t ScanUnicodeEscapeSequence();
    template <int N, bool IN_AS = false>
    char32_t ScanHexEscape();
    char32_t ScanUnicodeCodePointEscape();

    bool ScanStringUnicodePart(util::UString *str);
    char32_t ScanUnicodeCharacterHelper(size_t cpSize, char32_t cp);
    char32_t ScanUnicodeCharacter();

    void ScanDecimalNumbers();

    virtual void ScanNumberLeadingZero(bool const leadingMinus)
    {
        ScanNumberLeadingZeroImpl<double>(leadingMinus);
    }

    template <typename RadixType, typename RadixLimit = RadixType>
    bool ScanNumberLeadingZeroImpl(bool const leadingMinus);
    void ScanNumberLeadingZeroImplNonAllowedCases();
    template <bool RANGE_CHECK(char32_t), int RADIX, typename RadixType, typename RadixLimit>
    bool ScanNumberRadix(bool leadingMinus, bool allowNumericSeparator = true);
    void ScanNumber(bool const leadingMinus = false, bool allowBigInt = true);
    std::optional<std::size_t> ScanCharLex(bool parseExponent, bool &allowBigInt, NumberFlags &flags);
    std::optional<std::size_t> ScanSignOfNumber() noexcept;
    virtual void ConvertNumber(NumberFlags flags);
    void ScanDecimalLiteral();
    void ScanDecimalDigits(bool allowNumericSeparator);
    virtual void CheckNumberLiteralEnd();
    virtual void CheckNumberLiteralEndForIdentifier();
    void CheckOctal();

    inline static uint32_t HexValue(char32_t ch);
    inline static bool IsDecimalDigit(uint32_t cp);
    inline static bool IsHexDigit(char32_t ch);
    inline static bool IsBinaryDigit(char32_t ch);
    inline static bool IsOctalDigit(char32_t ch);

    friend class KeywordsUtil;
    friend class TemplateLiteralParserContext;
    friend class parser::ETSNolintParser;

    LexerPosition &Pos();
    const LexerPosition &Pos() const;

private:
    TemplateLiteralParserContext *tlCtx_ {};
    ArenaAllocator *allocator_;
    Keywords *kws_ {};
    const parser::ParserContext *parserContext_;
    util::StringView source_;
    LexerPosition pos_;
    util::DiagnosticEngine &diagnosticEngine_;
};

class TemplateLiteralParserContext {
public:
    explicit TemplateLiteralParserContext(Lexer *lexer) : lexer_(lexer), prev_(lexer_->tlCtx_) {}
    NO_MOVE_SEMANTIC(TemplateLiteralParserContext);
    NO_COPY_SEMANTIC(TemplateLiteralParserContext);

    ~TemplateLiteralParserContext()
    {
        lexer_->tlCtx_ = prev_;
    }

    void ConsumeLeftBrace()
    {
        braceDepth_++;
    }

    bool ConsumeRightBrace()
    {
        braceDepth_--;

        return braceDepth_ == 0;
    }

private:
    Lexer *lexer_;
    TemplateLiteralParserContext *prev_ {};
    size_t braceDepth_ {1};
};

template <char32_t END>
// CC-OFFNXT(huge_method,G.FUN.01) big switch-case, solid logic
void Lexer::ScanString()
{
    util::UString str(Allocator());
    PrepareStringTokenHelper();
    const auto startPos = Iterator().Index();
    auto escapeEnd = startPos;
    bool isFinalizedStr = true;

    do {
        const char32_t cp = Iterator().Peek();
        switch (cp) {
            case util::StringView::Iterator::INVALID_CP: {
                LogError(diagnostic::UNTERMINATED_STRING);
                isFinalizedStr = false;
                break;
            }
            case LEX_CHAR_CR:
            case LEX_CHAR_LF: {
                if constexpr (END != LEX_CHAR_BACK_TICK) {
                    LogError(diagnostic::NEWLINE_NOT_ALLOWED_IN_STRING);
                    break;
                }
                HandleNewlineHelper(&str, &escapeEnd);
                continue;
            }
            case LEX_CHAR_BACKSLASH: {
                isFinalizedStr &= HandleBackslashHelper(&str, &escapeEnd);
                continue;
            }
            case LEX_CHAR_BACK_TICK:
            case LEX_CHAR_SINGLE_QUOTE:
            case LEX_CHAR_DOUBLE_QUOTE: {
                if (!HandleDoubleQuoteHelper(END, cp)) {
                    break;
                }
                continue;
            }
            case LEX_CHAR_DOLLAR_SIGN: {
                if (HandleDollarSignHelper(END)) {
                    break;
                }
                continue;
            }
            default: {
                Iterator().SkipCp();
                continue;
            }
        }

        FinalizeTokenHelper(&str, startPos, escapeEnd, isFinalizedStr);
        break;
    } while (true);

    // NOLINTNEXTLINE(readability-braces-around-statements,bugprone-suspicious-semicolon)
    if constexpr (END != LEX_CHAR_BACK_TICK) {
        Iterator().Forward(1);
    }
}

template <int N, bool IN_AS>
char32_t Lexer::ScanHexEscape()
{
    char32_t code = 0;

    for (size_t i = 0; i < N; ++i) {
        const auto cp = Iterator().Peek();
        if (IN_AS && cp == LEX_CHAR_BACK_TICK) {
            break;
        }

        Iterator().Forward(1);

        if (!IsHexDigit(cp)) {
            LogError(diagnostic::INVALID_UNICODE_ESCAPE);
            return UNICODE_INVALID_CP;
        }

        constexpr auto MULTIPLIER = 16;
        code = code * MULTIPLIER + HexValue(cp);
    }

    return code;
}

template <typename RadixType, typename RadixLimit>
bool Lexer::ScanNumberLeadingZeroImpl(bool const leadingMinus)
{
    GetToken().type_ = TokenType::LITERAL_NUMBER;
    GetToken().keywordType_ = TokenType::LITERAL_NUMBER;

    switch (Iterator().Peek()) {
        case LEX_CHAR_LOWERCASE_X:
        case LEX_CHAR_UPPERCASE_X: {
            Iterator().Forward(1);
            constexpr auto RADIX = 16;
            if (!ScanNumberRadix<IsHexDigit, RADIX, RadixType, RadixLimit>(leadingMinus)) {
                return false;
            }
            CheckNumberLiteralEnd();
            return true;
        }
        case LEX_CHAR_LOWERCASE_B:
        case LEX_CHAR_UPPERCASE_B: {
            Iterator().Forward(1);
            constexpr auto RADIX = 2;
            if (!ScanNumberRadix<IsBinaryDigit, RADIX, RadixType, RadixLimit>(leadingMinus)) {
                return false;
            }
            CheckNumberLiteralEnd();
            return true;
        }
        case LEX_CHAR_LOWERCASE_O:
        case LEX_CHAR_UPPERCASE_O: {
            Iterator().Forward(1);
            constexpr auto RADIX = 8;
            if (!ScanNumberRadix<IsOctalDigit, RADIX, RadixType, RadixLimit>(leadingMinus)) {
                return false;
            }
            CheckOctal();
            CheckNumberLiteralEnd();
            return true;
        }
        default: {
            ScanNumberLeadingZeroImplNonAllowedCases();
            break;
        }
    }

    ScanNumber(leadingMinus);
    return true;
}

template <int RADIX, typename RadixType, typename RadixLimit>
bool ScanTooLargeNumber([[maybe_unused]] RadixType const number, [[maybe_unused]] std::uint32_t const digit)
{
    // NOTE (DZ): probably more sophisticates check will be required for general usage
    if constexpr (std::is_integral_v<RadixLimit>) {
        if (static_cast<RadixType>(std::numeric_limits<RadixLimit>::max()) / RADIX < number ||
            static_cast<RadixType>(std::numeric_limits<RadixLimit>::max()) - number * RADIX < digit) {
            return false;
        }
    }
    return true;
}

template <bool RANGE_CHECK(char32_t), int RADIX, typename RadixType, typename RadixLimit>
bool Lexer::ScanNumberRadix(bool leadingMinus, bool allowNumericSeparator)
{
    RadixType number {};

    auto cp = Iterator().Peek();
    if (!RANGE_CHECK(cp)) {
        LogError(diagnostic::INVALID_DIGIT);
    }

    bool allowNumericOnNext = true;

    do {
        cp = Iterator().Peek();
        if (RANGE_CHECK(cp)) {
            auto const digit = HexValue(cp);
            if (!ScanTooLargeNumber<RADIX, RadixType, RadixLimit>(number, digit)) {
                GetToken().number_ = lexer::Number();
                return false;
            }
            number = number * RADIX + digit;

            Iterator().Forward(1);
            allowNumericOnNext = true;
            continue;
        }

        if (cp == LEX_CHAR_UNDERSCORE) {
            if (!allowNumericSeparator || !allowNumericOnNext) {
                LogError(diagnostic::INVALID_NUMERIC_SEP);
            }

            GetToken().flags_ |= TokenFlags::NUMBER_HAS_UNDERSCORE;
            Iterator().Forward(1);
            allowNumericOnNext = false;
            continue;
        }

        if (!allowNumericOnNext) {
            Iterator().Backward(1);
            LogError(diagnostic::INVALID_NUMERIC_SEP_AT_END_OF_NUM);
        }

        break;
    } while (true);

    if (leadingMinus) {
        if constexpr (std::is_integral_v<RadixType>) {
            number = ~number + static_cast<RadixType>(1);
        } else {
            number = -number;
        }
    }

    GetToken().number_ = lexer::Number(number);
    GetToken().number_.SetStr(SourceView(GetToken().Start().index, Iterator().Index()));
    return true;
}

inline uint32_t Lexer::HexValue(char32_t ch)
{
    constexpr uint32_t HEX_MASK = 0xF;
    constexpr uint32_t DEC_OFFSET = 10;
    return ch < LEX_CHAR_UPPERCASE_A ? ch - LEX_CHAR_0 : ((ch - LEX_CHAR_UPPERCASE_A + DEC_OFFSET) & HEX_MASK);
}

inline bool Lexer::IsDecimalDigit(uint32_t cp)
{
    return (cp >= LEX_CHAR_0 && cp <= LEX_CHAR_9);
}

inline bool Lexer::IsHexDigit(char32_t ch)
{
    return ch < LEX_ASCII_MAX_BITS && (std::isxdigit(static_cast<unsigned char>(ch)) != 0);
}

inline bool Lexer::IsBinaryDigit(char32_t ch)
{
    return ch == LEX_CHAR_0 || ch == LEX_CHAR_1;
}

inline bool Lexer::IsOctalDigit(char32_t ch)
{
    return (ch >= LEX_CHAR_0 && ch <= LEX_CHAR_7);
}
}  // namespace ark::es2panda::lexer

template <>
struct enumbitops::IsAllowedType<ark::es2panda::lexer::NextTokenFlags> : std::true_type {
};

#endif
