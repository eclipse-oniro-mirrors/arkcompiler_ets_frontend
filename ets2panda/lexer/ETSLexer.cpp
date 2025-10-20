/*
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

#include "ETSLexer.h"
#include "generated/keywords.h"

namespace ark::es2panda::lexer {
// NOLINTNEXTLINE(google-default-arguments)
void ETSLexer::NextToken(NextTokenFlags flags)
{
    flags |= DefaultNextTokenFlags();
    ETSKeywords kws(this, static_cast<NextTokenFlags>(flags & ~NextTokenFlags::KEYWORD_TO_IDENT));
    Lexer::NextToken(&kws);
}

void ETSLexer::ScanHashMark()
{
    LogError(diagnostic::ERROR_ARKTS_NO_PRIVATE_IDENTIFIERS);
}

bool ETSLexer::ScanCharLiteral()
{
    //  Note: for character literal on call iterator is pointed to the opening single quote (') character!
    //  Otherwise it's another token.
    bool status = true;

    if (Iterator().Peek() != LEX_CHAR_SINGLE_QUOTE) {
        return false;
    }

    GetToken().type_ = TokenType::LITERAL_CHAR;

    Iterator().Forward(1);
    char32_t cp = Iterator().PeekCp();

    switch (cp) {
        case LEX_CHAR_SINGLE_QUOTE:
        case util::StringView::Iterator::INVALID_CP: {
            LogError(diagnostic::INVALID_CHAR);
            status = false;
            break;
        }
        case LEX_CHAR_BACKSLASH: {
            GetToken().flags_ |= TokenFlags::HAS_ESCAPE;

            Iterator().Forward(1);
            cp = ScanUnicodeCharacter();
            break;
        }
        default: {
            Iterator().SkipCp();
            break;
        }
    }

    if (CheckUtf16Compatible(cp)) {
        GetToken().c16_ = cp;
    }

    if (Iterator().Peek() != LEX_CHAR_SINGLE_QUOTE) {
        LogError(diagnostic::UNSUPPORTED_CHAR_LIT);
        return false;
    }

    Iterator().Forward(1);
    return status;
}

void ETSLexer::CheckNumberLiteralEnd()
{
    if (Iterator().Peek() == LEX_CHAR_LOWERCASE_F) {
        GetToken().flags_ |= TokenFlags::NUMBER_FLOAT;
        GetToken().src_ = SourceView(GetToken().Start().index, Iterator().Index());
        Iterator().Forward(1);
        const auto nextCp = Iterator().PeekCp();
        if (KeywordsUtil::IsIdentifierStart(nextCp) || IsDecimalDigit(nextCp)) {
            LogError(diagnostic::INVALID_NUMERIC_LIT);
            Iterator().Forward(1);  // Error processing
        }
    } else {
        Lexer::CheckNumberLiteralEnd();
    }
}

bool ETSLexer::CheckUtf16Compatible(char32_t cp) const
{
    if (cp >= util::StringView::Constants::CELESTIAL_OFFSET) {
        // lexer_invalid_characters.ets
        LogError(diagnostic::UNSUPPORTED_CHAR_LIT);
        return false;
    }
    return true;
}

void ETSLexer::ScanAsteriskPunctuator()
{
    GetToken().type_ = TokenType::PUNCTUATOR_MULTIPLY;

    switch (Iterator().Peek()) {
        case LEX_CHAR_EQUALS: {
            GetToken().type_ = TokenType::PUNCTUATOR_MULTIPLY_EQUAL;
            Iterator().Forward(1);
            break;
        }
        default: {
            break;
        }
    }
}

void ETSLexer::ConvertNumber(NumberFlags const flags)
{
    GetToken().number_ = lexer::Number(GetToken().src_, flags);
    if (GetToken().number_.ConversionError()) {
        LogError(diagnostic::INVALID_NUM);
    }
}

void ETSLexer::ScanEqualsPunctuator()
{
    GetToken().type_ = TokenType::PUNCTUATOR_SUBSTITUTION;

    switch (Iterator().Peek()) {
        case LEX_CHAR_EQUALS: {
            GetToken().type_ = TokenType::PUNCTUATOR_EQUAL;
            Iterator().Forward(1);

            if (Iterator().Peek() == LEX_CHAR_EQUALS) {
                GetToken().type_ = TokenType::PUNCTUATOR_STRICT_EQUAL;
                Iterator().Forward(1);
            }
            break;
        }
        case LEX_CHAR_GREATER_THAN: {
            GetToken().type_ = TokenType::PUNCTUATOR_ARROW;
            Iterator().Forward(1);
            break;
        }
        default: {
            break;
        }
    }
}

void ETSLexer::ScanExclamationPunctuator()
{
    GetToken().type_ = TokenType::PUNCTUATOR_EXCLAMATION_MARK;

    switch (Iterator().Peek()) {
        case LEX_CHAR_EQUALS: {
            GetToken().type_ = TokenType::PUNCTUATOR_NOT_EQUAL;
            Iterator().Forward(1);

            if (Iterator().Peek() == LEX_CHAR_EQUALS) {
                GetToken().type_ = TokenType::PUNCTUATOR_NOT_STRICT_EQUAL;
                Iterator().Forward(1);
            }
            break;
        }
        default: {
            break;
        }
    }
}

}  // namespace ark::es2panda::lexer
