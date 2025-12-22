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
#include "formatting_context.h"

namespace ark::es2panda::lsp {

using ContextPredicate = std::function<bool(FormattingContext *)>;

enum class RuleAction : uint32_t {
    NONE = 0U,
    STOP_PROCESSING_SPACE_ACTIONS = 1U << 0U,
    STOP_PROCESSING_TOKEN_ACTIONS = 1U << 1U,
    INSERT_SPACE = 1U << 2U,
    INSERT_NEWLINE = 1U << 3U,
    DELETE_SPACE = 1U << 4U,
    DELETE_TOKEN = 1U << 5U,
    INSERT_TRAILING_SEMICOLON = 1U << 6U,

    STOP_ACTION = STOP_PROCESSING_SPACE_ACTIONS | STOP_PROCESSING_TOKEN_ACTIONS,
    MODIFY_SPACE_ACTION = INSERT_SPACE | INSERT_NEWLINE | DELETE_SPACE,
    MODIFY_TOKEN_ACTION = DELETE_TOKEN | INSERT_TRAILING_SEMICOLON
};

inline RuleAction operator&(RuleAction lhs, RuleAction rhs)
{
    return static_cast<RuleAction>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

inline RuleAction operator|(RuleAction lhs, RuleAction rhs)
{
    return static_cast<RuleAction>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline RuleAction &operator|=(RuleAction &lhs, RuleAction rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline RuleAction operator~(RuleAction rhs)
{
    return static_cast<RuleAction>(~static_cast<uint32_t>(rhs));
}

enum class RuleFlags { NONE, CAN_DELETE_NEWLINES };

struct Rule {
public:
    explicit Rule(std::vector<ContextPredicate> cb, RuleAction action, RuleFlags flag)
        : context_(std::move(cb)), action_(action), flags_(flag)
    {
    }

    const std::vector<ContextPredicate> &GetContext() const
    {
        return context_;
    }

    RuleAction GetRuleAction() const
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

    void SetSpecific(bool isSpecific)
    {
        isSpecific_ = isSpecific;
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
