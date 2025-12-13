/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef FORMATTING_CONTEXT_H
#define FORMATTING_CONTEXT_H

#include "formatting_settings.h"
#include "ir/astNode.h"
#include "lexer/token/token.h"
#include <string>

namespace ark::es2panda::lsp {

class FormattingContext {
public:
    explicit FormattingContext(const std::string &sourceText);

    void SetCurrentToken(const lexer::Token &token);
    void SetPreviousToken(const lexer::Token &token);
    void SetNextToken(const lexer::Token &token);
    void SetCurrentTokenParent(ir::AstNode *node);
    void SetNextTokenParent(ir::AstNode *node);
    void SetOptions(const FormatCodeSettings &options);

    const lexer::Token &GetCurrentToken() const;
    const lexer::Token &GetPreviousToken() const;
    const lexer::Token &GetNextToken() const;
    ir::AstNode *GetCurrentTokenParent() const;
    ir::AstNode *GetNextTokenParent() const;
    const std::string &GetSourceText() const;
    const lexer::SourceRange &GetCurrentTokenSpan() const;
    const FormatCodeSettings &GetOptions() const;

    bool ContextNodeBlockIsOnOneLine() const;
    bool TokensAreOnSameLine() const;

private:
    bool BlockIsOnOneLine(ir::AstNode *node) const;
    const std::string &sourceText_;

    lexer::Token currentToken_;
    lexer::Token prevToken_;
    lexer::Token nextToken_;
    ir::AstNode *currentTokenParent_ {nullptr};
    ir::AstNode *nextTokenParent_ {nullptr};
    lexer::SourceRange currentTokenSpan_;
    FormatCodeSettings options_;
};

}  // namespace ark::es2panda::lsp

#endif