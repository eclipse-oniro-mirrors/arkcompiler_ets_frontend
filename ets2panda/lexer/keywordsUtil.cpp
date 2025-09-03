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

#include "keywordsUtil.h"

#include "generated/keywords.h"
#include "lexer/lexer.h"
#include "unicode/uchar.h"
#include "util/enumbitops.h"

namespace ark::es2panda::lexer {
enum class AsciiFlags : uint32_t {
    NONE = 0U,
    ID_START = 1U << 0U,
    ID_CONTINUE = 1U << 1U,
};

constexpr AsciiFlags operator|(AsciiFlags a, AsciiFlags b)
{
    using Utype = std::underlying_type_t<AsciiFlags>;
    return static_cast<AsciiFlags>(static_cast<Utype>(a) | static_cast<Utype>(b));
}

inline std::underlying_type_t<AsciiFlags> operator&(AsciiFlags a, AsciiFlags b)
{
    using Utype = std::underlying_type_t<AsciiFlags>;
    /* NOLINTNEXTLINE(hicpp-signed-bitwise) */
    return static_cast<Utype>(static_cast<Utype>(a) & static_cast<Utype>(b));
}

constexpr std::array<AsciiFlags, 128> ASCII_FLAGS = {{
    AsciiFlags::NONE,                               /* NUL */
    AsciiFlags::NONE,                               /* SOH */
    AsciiFlags::NONE,                               /* STX */
    AsciiFlags::NONE,                               /* ETX */
    AsciiFlags::NONE,                               /* EOT */
    AsciiFlags::NONE,                               /* ENQ */
    AsciiFlags::NONE,                               /* ACK */
    AsciiFlags::NONE,                               /* BEL */
    AsciiFlags::NONE,                               /* BS */
    AsciiFlags::NONE,                               /* TAB */
    AsciiFlags::NONE,                               /* LF */
    AsciiFlags::NONE,                               /* VT */
    AsciiFlags::NONE,                               /* FF */
    AsciiFlags::NONE,                               /* CR */
    AsciiFlags::NONE,                               /* SO */
    AsciiFlags::NONE,                               /* SI */
    AsciiFlags::NONE,                               /* DLE */
    AsciiFlags::NONE,                               /* DC1 */
    AsciiFlags::NONE,                               /* DC2 */
    AsciiFlags::NONE,                               /* DC3 */
    AsciiFlags::NONE,                               /* DC4 */
    AsciiFlags::NONE,                               /* NAK */
    AsciiFlags::NONE,                               /* SYN */
    AsciiFlags::NONE,                               /* ETB */
    AsciiFlags::NONE,                               /* CAN */
    AsciiFlags::NONE,                               /* EM */
    AsciiFlags::NONE,                               /* SUB */
    AsciiFlags::NONE,                               /* ESC */
    AsciiFlags::NONE,                               /* FS */
    AsciiFlags::NONE,                               /* GS */
    AsciiFlags::NONE,                               /* RS */
    AsciiFlags::NONE,                               /* US */
    AsciiFlags::NONE,                               /* Space */
    AsciiFlags::NONE,                               /* ! */
    AsciiFlags::NONE,                               /* " */
    AsciiFlags::NONE,                               /* # */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* $ */
    AsciiFlags::NONE,                               /* % */
    AsciiFlags::NONE,                               /* & */
    AsciiFlags::NONE,                               /* ' */
    AsciiFlags::NONE,                               /* ( */
    AsciiFlags::NONE,                               /* ) */
    AsciiFlags::NONE,                               /* * */
    AsciiFlags::NONE,                               /* + */
    AsciiFlags::NONE,                               /* , */
    AsciiFlags::NONE,                               /* - */
    AsciiFlags::NONE,                               /* . */
    AsciiFlags::NONE,                               /* / */
    AsciiFlags::ID_CONTINUE,                        /* 0 */
    AsciiFlags::ID_CONTINUE,                        /* 1 */
    AsciiFlags::ID_CONTINUE,                        /* 2 */
    AsciiFlags::ID_CONTINUE,                        /* 3 */
    AsciiFlags::ID_CONTINUE,                        /* 4 */
    AsciiFlags::ID_CONTINUE,                        /* 5 */
    AsciiFlags::ID_CONTINUE,                        /* 6 */
    AsciiFlags::ID_CONTINUE,                        /* 7 */
    AsciiFlags::ID_CONTINUE,                        /* 8 */
    AsciiFlags::ID_CONTINUE,                        /* 9 */
    AsciiFlags::NONE,                               /* : */
    AsciiFlags::NONE,                               /* ; */
    AsciiFlags::NONE,                               /* < */
    AsciiFlags::NONE,                               /* = */
    AsciiFlags::NONE,                               /* > */
    AsciiFlags::NONE,                               /* ? */
    AsciiFlags::NONE,                               /* @ */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* A */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* B */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* C */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* D */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* E */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* F */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* G */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* H */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* I */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* J */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* K */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* L */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* M */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* N */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* O */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* P */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* Q */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* R */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* S */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* T */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* U */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* V */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* W */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* X */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* Y */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* Z */
    AsciiFlags::NONE,                               /* [ */
    AsciiFlags::NONE,                               /* \ */
    AsciiFlags::NONE,                               /* ] */
    AsciiFlags::NONE,                               /* ^ */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* _ */
    AsciiFlags::NONE,                               /* ` */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* a */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* b */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* c */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* d */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* e */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* f */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* g */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* h */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* i */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* j */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* k */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* l */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* m */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* n */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* o */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* p */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* q */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* r */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* s */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* t */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* u */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* v */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* w */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* x */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* y */
    AsciiFlags::ID_START | AsciiFlags::ID_CONTINUE, /* z */
    AsciiFlags::NONE,                               /* { */
    AsciiFlags::NONE,                               /* | */
    AsciiFlags::NONE,                               /* } */
    AsciiFlags::NONE,                               /* ~ */
    AsciiFlags::NONE                                /* DEL */
}};

bool KeywordsUtil::IsIdentifierStart(char32_t cp)
{
    if (cp < LEX_ASCII_MAX_BITS) {
        return (ASCII_FLAGS[cp] & AsciiFlags::ID_START) != 0;
    }

    auto uchar = static_cast<UChar32>(cp);
    return u_hasBinaryProperty(uchar, UCHAR_ID_START);
}

bool KeywordsUtil::IsIdentifierPart(char32_t cp)
{
    if (cp < LEX_ASCII_MAX_BITS) {
        return (ASCII_FLAGS[cp] & AsciiFlags::ID_CONTINUE) != 0;
    }

    // u_isIDPart or Other_ID_Continue characters or ZWJ/ZWNJ.
    auto uchar = static_cast<UChar32>(cp);
    return (u_hasBinaryProperty(uchar, UCHAR_ID_CONTINUE) || cp == LEX_CHAR_ZWNJ || cp == LEX_CHAR_ZWJ);
}

void KeywordsUtil::ScanIdentifierStart(const Keywords *kws, char32_t cp)
{
    if (!KeywordsUtil::IsIdentifierStart(cp)) {
        lexer_->LogError(diagnostic::UNEXPECTED_TOKEN_ID);
    }

    cp_ = cp;
    const auto map = kws->KeywordMap(cp);
    ScanIdContinueMaybeKeyword(kws, map);
}

void KeywordsUtil::ScanIdContinue()
{
    util::UString ident(lexer_->Allocator());
    size_t startPos = lexer_->GetToken().Start().index;

    if (HasEscape()) {
        ident.Append(cp_);
        startPos = Iterator().Index();
    }

    auto escapeEnd = startPos;

    while (true) {
        if (Iterator().Peek() == LEX_CHAR_BACKSLASH) {
            ident.Append(lexer_->SourceView(escapeEnd, Iterator().Index()));

            auto cp = ScanUnicodeEscapeSequence();
            if (!IsIdentifierPart(cp)) {
                lexer_->LogError(diagnostic::INVALID_IDENTIFIER_PART);
            }

            escapeEnd = Iterator().Index();
            ident.Append(cp);
            continue;
        }

        size_t cpSize {};
        auto cp = Iterator().PeekCp(&cpSize);
        if (!IsIdentifierPart(cp) &&
            (cp != LEX_CHAR_PERCENT || (Flags() & NextTokenFlags::CHAR_PERCENT_ALLOWED) == 0)) {
            break;
        }

        Iterator().Forward(cpSize);
    }

    lexer_->GetToken().type_ = TokenType::LITERAL_IDENT;
    lexer_->GetToken().keywordType_ = TokenType::LITERAL_IDENT;

    if (HasEscape()) {
        ident.Append(lexer_->SourceView(escapeEnd, Iterator().Index()));
        lexer_->GetToken().src_ = ident.View();
    } else {
        lexer_->GetToken().src_ = lexer_->SourceView(startPos, Iterator().Index());
    }
}

void KeywordsUtil::ScanIdContinueMaybeKeyword(const Keywords *kws, Span<const KeywordString> map)
{
    ScanIdContinue();

    if (!HasEscape() || map.empty()) {
        return;
    }

    const auto &str = lexer_->GetToken().Ident().Utf8();

    size_t start = 0;
    size_t end = map.size();
    size_t middle = end / 2;

    while (true) {
        const auto &kw = map[middle];

        int relation = str.compare(kw.Str());
        if (relation == 0) {
            kws->HandlePotentialEscapedKeyword(kw);
            return;
        }

        if (relation > 0) {
            start = middle + 1;
        } else {
            end = middle;
        }

        middle = (start + end) / 2U;

        if (start >= end) {
            return;
        }
    }
}

char32_t KeywordsUtil::ScanUnicodeEscapeSequence()
{
    ES2PANDA_ASSERT(Iterator().Peek() == LEX_CHAR_BACKSLASH);

    lexer_->GetToken().flags_ |= lexer::TokenFlags::HAS_ESCAPE;

    Iterator().Forward(1);

    if (Iterator().Peek() != LEX_CHAR_LOWERCASE_U) {
        return util::StringView::Iterator::INVALID_CP;
    }

    return lexer_->ScanUnicodeEscapeSequence();
}
}  // namespace ark::es2panda::lexer
