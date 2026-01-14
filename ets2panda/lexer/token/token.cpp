/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "token.h"

namespace ark::es2panda::lexer {

#include "generated/token.inl"

bool Token::IsAccessability() const
{
    return (type_ == TokenType::LITERAL_IDENT &&
            (keywordType_ == TokenType::KEYW_PUBLIC || keywordType_ == TokenType::KEYW_PROTECTED ||
             keywordType_ == TokenType::KEYW_PRIVATE) &&
            ((flags_ & TokenFlags::HAS_ESCAPE) == 0));
}

bool Token::IsAsyncModifier() const
{
    return (type_ == TokenType::LITERAL_IDENT && keywordType_ == TokenType::KEYW_ASYNC &&
            (flags_ & TokenFlags::HAS_ESCAPE) == 0);
}

bool Token::IsForInOf() const
{
    return (keywordType_ == TokenType::KEYW_IN || keywordType_ == TokenType::KEYW_OF);
}

bool Token::IsStaticModifier() const
{
    return (type_ == TokenType::LITERAL_IDENT && keywordType_ == TokenType::KEYW_STATIC &&
            (flags_ & TokenFlags::HAS_ESCAPE) == 0);
}

bool Token::IsDeclareModifier() const
{
    return (type_ == TokenType::LITERAL_IDENT && keywordType_ == TokenType::KEYW_DECLARE &&
            (flags_ & TokenFlags::HAS_ESCAPE) == 0);
}

bool Token::IsReadonlyModifier() const
{
    return (type_ == TokenType::LITERAL_IDENT && keywordType_ == TokenType::KEYW_READONLY &&
            (flags_ & TokenFlags::HAS_ESCAPE) == 0);
}

bool Token::IsUpdate() const
{
    return (type_ == TokenType::PUNCTUATOR_MINUS_MINUS || type_ == TokenType::PUNCTUATOR_PLUS_PLUS);
}

bool Token::IsPropNameLiteral() const noexcept
{
    return (type_ == TokenType::LITERAL_STRING || type_ == TokenType::LITERAL_NUMBER ||
            type_ == TokenType::LITERAL_TRUE || type_ == TokenType::LITERAL_FALSE);
}

bool Token::IsLiteral() const noexcept
{
    return (type_ == TokenType::LITERAL_IDENT || IsPropNameLiteral() || type_ == TokenType::LITERAL_CHAR ||
            type_ == TokenType::LITERAL_REGEXP || type_ == TokenType::LITERAL_NULL);
}

std::string_view Token::ToString() const noexcept
{
    if (!IsLiteral()) {
        return TokenToString(type_);
    }

    auto const str = Ident().Utf8();
    if (!str.empty() && (str[0U] == '\n')) {
        return "end of stream";
    }

    return str;
}

bool Token::IsKeyword() const
{
    return (type_ >= TokenType::FIRST_KEYW);
}

bool Token::IsUpdateToken(TokenType type)
{
    return (type == TokenType::PUNCTUATOR_MINUS_MINUS || type == TokenType::PUNCTUATOR_PLUS_PLUS);
}

bool Token::IsPunctuatorToken(TokenType type)
{
    return type >= TokenType::FIRST_PUNCTUATOR && type < TokenType::FIRST_KEYW;
}

bool Token::IsTsParamToken(TokenType type)
{
    return (type == TokenType::PUNCTUATOR_COLON || type == TokenType::PUNCTUATOR_QUESTION_MARK);
}

}  // namespace ark::es2panda::lexer
