/*
 * Copyright (c) 2021 - 2025 Huawei Device Co., Ltd.
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

#include "lexer/keywordsUtil.h"
#include "parser/context/parserContext.h"
#include "generated/keywords.h"

namespace ark::es2panda::lexer {

KeywordString JSKeywords::Handle_as([[maybe_unused]] const KeywordsUtil &util, std::string_view src,
                                    TokenType tokenType)
{
    return {src, TokenType::LITERAL_IDENT, tokenType};
}

KeywordString JSKeywords::Handle_await(const KeywordsUtil &util, std::string_view src, TokenType tokenType)
{
    const auto *parserContext = util.GetParserContext();

    if (!parserContext->IsAsync() && !parserContext->IsModule()) {
        return {src, TokenType::LITERAL_IDENT, tokenType};
    }

    util.CheckEscapedKeyword();

    return {src, tokenType};
}

KeywordString JSKeywords::Handle_yield(const KeywordsUtil &util, std::string_view src, TokenType tokenType)
{
    const auto *parserContext = util.GetParserContext();

    if (!parserContext->IsGenerator()) {
        util.LogUnexpectedStrictModeReservedKeyword();
    }

    util.CheckEscapedKeyword();

    return {src, tokenType};
}

KeywordString TSKeywords::Handle_as(const KeywordsUtil &util, std::string_view src, TokenType tokenType)
{
    return JSKeywords::Handle_as(util, src, tokenType);
}

KeywordString TSKeywords::Handle_await(const KeywordsUtil &util, std::string_view src, TokenType tokenType)
{
    return JSKeywords::Handle_await(util, src, tokenType);
}

KeywordString TSKeywords::Handle_yield(const KeywordsUtil &util, std::string_view src, TokenType tokenType)
{
    return JSKeywords::Handle_yield(util, src, tokenType);
}

KeywordString ASKeywords::Handle_as(const KeywordsUtil &util, std::string_view src, TokenType tokenType)
{
    return JSKeywords::Handle_as(util, src, tokenType);
}

}  // namespace ark::es2panda::lexer
