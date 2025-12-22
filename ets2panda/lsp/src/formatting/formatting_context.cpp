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

#include "formatting/formatting_context.h"
#include "ir/astNode.h"
#include "ir/statements/blockStatement.h"

namespace ark::es2panda::lsp {

FormattingContext::FormattingContext(const std::string &sourceText) : sourceText_(sourceText) {}

void FormattingContext::SetCurrentToken(const lexer::Token &token)
{
    currentToken_ = token;
    currentTokenSpan_ = token.Loc();
}

void FormattingContext::SetPreviousToken(const lexer::Token &token)
{
    prevToken_ = token;
}

void FormattingContext::SetNextToken(const lexer::Token &token)
{
    nextToken_ = token;
}

void FormattingContext::SetCurrentTokenParent(ir::AstNode *node)
{
    currentTokenParent_ = node;
}

void FormattingContext::SetNextTokenParent(ir::AstNode *node)
{
    nextTokenParent_ = node;
}

void FormattingContext::SetOptions(const FormatCodeSettings &options)
{
    options_ = options;
}

const lexer::Token &FormattingContext::GetCurrentToken() const
{
    return currentToken_;
}

const lexer::Token &FormattingContext::GetPreviousToken() const
{
    return prevToken_;
}

const lexer::Token &FormattingContext::GetNextToken() const
{
    return nextToken_;
}

ir::AstNode *FormattingContext::GetCurrentTokenParent() const
{
    return currentTokenParent_;
}

ir::AstNode *FormattingContext::GetNextTokenParent() const
{
    return nextTokenParent_;
}

const std::string &FormattingContext::GetSourceText() const
{
    return sourceText_;
}

const lexer::SourceRange &FormattingContext::GetCurrentTokenSpan() const
{
    return currentTokenSpan_;
}

const FormatCodeSettings &FormattingContext::GetOptions() const
{
    return options_;
}

bool FormattingContext::ContextNodeBlockIsOnOneLine() const
{
    if (currentTokenParent_ == nullptr) {
        return true;
    }
    return BlockIsOnOneLine(currentTokenParent_);
}

bool FormattingContext::TokensAreOnSameLine() const
{
    return prevToken_.Loc().end.line == currentToken_.Loc().start.line;
}

bool FormattingContext::BlockIsOnOneLine(ir::AstNode *node) const
{
    if (node->IsBlockStatement()) {
        auto block = node->AsBlockStatement();
        if (!block->Statements().empty()) {
            return block->Start().line == block->Statements().back()->End().line;
        }
    }
    return node->Start().line == node->End().line;
}

}  // namespace ark::es2panda::lsp