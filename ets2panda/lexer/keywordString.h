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

#ifndef ES2PANDA_PARSER_CORE_KEYWORD_STRING_H
#define ES2PANDA_PARSER_CORE_KEYWORD_STRING_H

#include "util/es2pandaMacros.h"
#include "util/ustring.h"
#include "generated/tokenType.h"

namespace ark::es2panda::lexer {
struct KeywordString {
    constexpr KeywordString(std::string_view s, TokenType t) : KeywordString(s, t, t) {}
    constexpr KeywordString(std::string_view s, TokenType t, TokenType k) : str_(s), tokenType_(t), keywordType_(k) {}

    explicit KeywordString(std::string_view &s) : str_(s) {}

    TokenType GetTokenType() const
    {
        return tokenType_;
    }

    TokenType GetKeywordType() const
    {
        return keywordType_;
    }

    std::string_view Str() const
    {
        return str_;
    }

private:
    std::string_view str_;
    TokenType tokenType_ {};
    TokenType keywordType_ {};
};
}  // namespace ark::es2panda::lexer

#endif
