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

#include "formatting/rules.h"
#include <vector>
#include "formatting/formatting_context.h"
#include "ir/astNode.h"

namespace ark::es2panda::lsp {

static bool IsConditionalOperatorContext(FormattingContext *ctx)
{
    auto *parent = ctx->GetCurrentTokenParent();
    return parent != nullptr && (parent->IsConditionalExpression() || parent->IsTSConditionalType());
}

std::vector<RuleSpec> GetAllRules()
{
    std::vector<RuleSpec> rules;

    auto createTokenRange = [](const std::vector<lexer::TokenType> &tokens) {
        return std::vector<TokenRange> {TokenRange(tokens, true)};
    };
    auto anyTokenRange = createTokenRange({});

    {
        std::vector<ContextPredicate> p = {[](FormattingContext *ctx) { return ctx->TokensAreOnSameLine(); },
                                           IsConditionalOperatorContext};
        Rule rule(p, RuleAction::INSERT_SPACE, RuleFlags::NONE);
        auto left = createTokenRange({lexer::TokenType::PUNCTUATOR_QUESTION_MARK});
        auto right = anyTokenRange;
        rules.emplace_back(rule, left, right);
    }

    return rules;
}

}  // namespace ark::es2panda::lsp