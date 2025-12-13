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

#ifndef ES2PANDA_PARSER_CORE_KEYWORDS_UTIL_H
#define ES2PANDA_PARSER_CORE_KEYWORDS_UTIL_H

#include "lexer/keywordString.h"
#include "lexer/lexer.h"
#include "lexer/token/letters.h"
#include "generated/tokenType.h"
#include "util/es2pandaMacros.h"
#include "util/ustring.h"
#include "libarkbase/utils/span.h"

namespace ark::es2panda::lexer {
class Lexer;
class Keywords;

class KeywordsUtil {
public:
    explicit KeywordsUtil(Lexer *lexer, lexer::NextTokenFlags flags) : lexer_(lexer), flags_(flags) {}
    explicit KeywordsUtil(Lexer *lexer, lexer::NextTokenFlags flags, char32_t cp)
        : lexer_(lexer), flags_(flags), cp_(cp)
    {
    }
    NO_COPY_SEMANTIC(KeywordsUtil);
    DEFAULT_MOVE_SEMANTIC(KeywordsUtil);
    ~KeywordsUtil() = default;

    inline bool HasEscape() const
    {
        return (lexer_->GetToken().flags_ & lexer::TokenFlags::HAS_ESCAPE) != 0;
    }

    const parser::ParserContext *GetParserContext() const
    {
        return lexer_->parserContext_;
    }

    void LogError(const diagnostic::DiagnosticKind &diagnostic,
                  const util::DiagnosticMessageParams &diagnosticParams = {}) const
    {
        lexer_->LogError(diagnostic, diagnosticParams);
    }

    void LogUnexpectedStrictModeReservedKeyword() const
    {
        lexer_->LogUnexpectedStrictModeReservedKeyword();
    }

    inline NextTokenFlags Flags() const
    {
        return flags_;
    }

    inline bool KeywordToIdent() const
    {
        return (flags_ & NextTokenFlags::KEYWORD_TO_IDENT) != 0;
    }

    inline void CheckEscapedKeyword() const
    {
        if (HasEscape()) {
            LogEscapedKeyword();
        }
    }

    inline void LogEscapedKeyword() const
    {
        LogError(diagnostic::ESCAPE_SEQUENCES_IN_KEYWORD);
    }

    inline void SetKeyword(KeywordString kws) const
    {
        lexer_->GetToken().src_ = util::StringView(kws.Str());
        lexer_->GetToken().type_ = kws.GetTokenType();
        lexer_->GetToken().keywordType_ = kws.GetKeywordType();
    }

    inline util::StringView::Iterator &Iterator()
    {
        return lexer_->Iterator();
    }

    void ScanIdentifierStart(const Keywords *kws, char32_t cp);
    void ScanIdContinue();

    void ScanIdContinueMaybeKeyword(const Keywords *kws, Span<const KeywordString> map);
    char32_t ScanUnicodeEscapeSequence();

    static bool IsIdentifierStart(char32_t cp);
    static bool IsIdentifierPart(char32_t cp);

private:
    Lexer *lexer_;
    NextTokenFlags flags_ {};
    char32_t cp_ {util::StringView::Iterator::INVALID_CP};
};

}  // namespace ark::es2panda::lexer

#endif
