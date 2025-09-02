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

#ifndef ES2PANDA_JSDOC_HELPER_H
#define ES2PANDA_JSDOC_HELPER_H

#include "parserImpl.h"
#include "parser/program/program.h"
#include "lexer/token/letters.h"
#include "ir/astNode.h"
#include "ir/base/classDefinition.h"
#include "ir/ets/etsModule.h"

namespace ark::es2panda::parser {
using UStringView = util::StringView;
class JsdocHelper {
public:
    explicit JsdocHelper(const ir::AstNode *inputNode)
    {
        auto root = inputNode;
        while (root->Parent() != nullptr) {
            root = root->Parent();
        }
        root_ = root;
        program_ = root_->AsETSModule()->Program();
        sourceCode_ = program_->SourceCode();
        iter_ = util::StringView::Iterator(sourceCode_);
        InitNode(inputNode);
    }

    NO_COPY_SEMANTIC(JsdocHelper);
    DEFAULT_MOVE_SEMANTIC(JsdocHelper);

    ~JsdocHelper() = default;

    util::StringView GetJsdocBackward();
    util::StringView GetLicenseStringFromStart();

    const ir::AstNode *Node()
    {
        return node_;
    }

    util::StringView::Iterator &Iterator()
    {
        return iter_;
    }

    util::StringView SourceView(size_t begin, size_t end) const
    {
        return sourceCode_.Substr(begin, end);
    }

    void BackwardAndSkipSpace(size_t offset)
    {
        Iterator().Backward(offset);
        SkipWhiteSpacesBackward();
    }

    void Backward(size_t offset)
    {
        Iterator().Backward(offset);
    }

    void Forward(size_t offset)
    {
        Iterator().Forward(offset);
    }

    char32_t PeekBackWard() const
    {
        return iter_.Peek();
    }

private:
    void InitNode(const ir::AstNode *input)
    {
        if (input->IsClassDefinition()) {
            node_ = input->Parent();
        } else {
            node_ = input;
        }
    }

    bool SkipWhiteSpacesBackwardHelper(const char32_t &cp)
    {
        if (cp < lexer::LEX_ASCII_MAX_BITS) {
            return false;
        }

        size_t cpSize {};

        char32_t ch = Iterator().PeekCp(&cpSize);
        switch (ch) {
            case lexer::LEX_CHAR_LS:
            case lexer::LEX_CHAR_PS:
            case lexer::LEX_CHAR_NBSP:
            case lexer::LEX_CHAR_ZWNBSP:
            case lexer::LEX_CHAR_OGHAM:
            case lexer::LEX_CHAR_NARROW_NO_BREAK_SP:
            case lexer::LEX_CHAR_MATHEMATICAL_SP:
            case lexer::LEX_CHAR_IDEOGRAPHIC_SP:
                Iterator().Backward(cpSize);
                return true;
            default:
                if (ch >= lexer::LEX_CHAR_ENQUAD && ch <= lexer::LEX_CHAR_ZERO_WIDTH_SP) {
                    Iterator().Backward(cpSize);
                    return true;
                }
                return false;
        }
    }

    void SkipWhiteSpacesBackward()
    {
        bool skipContinue = true;
        while (skipContinue) {
            auto cp = Iterator().Peek();
            switch (cp) {
                case lexer::LEX_CHAR_CR:
                case lexer::LEX_CHAR_LF:
                case lexer::LEX_CHAR_VT:
                case lexer::LEX_CHAR_FF:
                case lexer::LEX_CHAR_SP:
                case lexer::LEX_CHAR_TAB:
                case lexer::LEX_CHAR_NEXT_LINE:
                    Iterator().Backward(1);
                    continue;
                default:
                    skipContinue = SkipWhiteSpacesBackwardHelper(cp);
            }
        }
    }

    void SkipCpBackward()
    {
        if (iter_.Index() == 0) {
            return;
        }
        Backward(1U);

        char32_t cu0 = static_cast<uint8_t>(iter_.Peek());
        if (cu0 < UStringView::Constants::UTF8_1BYTE_LIMIT) {
            return;
        }

        if ((cu0 & UStringView::Constants::UTF8_3BYTE_HEADER) == UStringView::Constants::UTF8_2BYTE_HEADER) {
            Backward(1U);
            return;
        }

        if ((cu0 & UStringView::Constants::UTF8_4BYTE_HEADER) == UStringView::Constants::UTF8_3BYTE_HEADER) {
            Backward(2U);
            return;
        }

        if (((cu0 & UStringView::Constants::UTF8_DECODE_4BYTE_MASK) == UStringView::Constants::UTF8_4BYTE_HEADER) &&
            (cu0 <= UStringView::Constants::UTF8_DECODE_4BYTE_LIMIT)) {
            Backward(3U);
        }
    }

    bool BackWardUntilJsdocStart();

    const ir::AstNode *root_ {};
    const parser::Program *program_ {};
    util::StringView sourceCode_ {};
    util::StringView::Iterator iter_ {nullptr};
    const ir::AstNode *node_ {};
};
}  // namespace ark::es2panda::parser

#endif
