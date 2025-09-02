/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RULE_H
#define RULE_H

#include <functional>
#include <vector>
#include "generated/tokenType.h"
#include "lexer/lexer.h"
#include "formatting_context.h"

namespace ark::es2panda::lsp {

using ContextPredicate = std::function<bool(FormattingContext *)>;

enum class RuleAction : uint16_t {
    NONE = 0U,
    STOP_PROCESSING_SPACE_ACTIONS = 1U << 0U,
    STOP_PROCESSING_TOKEN_ACTIONS = 1U << 1U,
    INSERT_SPACE = 1U << 2U,
    INSERT_NEWLINE = 1U << 3U,
    DELETE_SPACE = 1U << 4U,
    DELETE_TOKEN = 1U << 5U,
    INSERT_TRAILING_SEMICOLON = 1U << 6U
};

enum class RuleFlags { NONE, CAN_DELETE_NEWLINES };

struct Rule {
public:
    explicit Rule(std::vector<ContextPredicate> cb, RuleAction action, RuleFlags flag)
        : context_(std::move(cb)), action_(action), flags_(flag)
    {
    }

    std::vector<ContextPredicate> &GetContext()
    {
        return context_;
    }

    RuleAction GetRuleAction()
    {
        return action_;
    }

    RuleFlags GetRuleFlags()
    {
        return flags_;
    }

private:
    std::vector<ContextPredicate> context_;
    RuleAction action_;
    RuleFlags flags_;
};

struct TokenRange {
public:
    explicit TokenRange(std::vector<lexer::TokenType> tokens, bool isSpecific)
        : tokens_(std::move(tokens)), isSpecific_(isSpecific)
    {
    }

    std::vector<lexer::TokenType> &GetTokens()
    {
        return tokens_;
    }

    bool GetIsSpecifier()
    {
        return isSpecific_;
    }

private:
    std::vector<lexer::TokenType> tokens_;
    bool isSpecific_;
};

}  // namespace ark::es2panda::lsp

#endif