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

#include "formatting/rules_map.h"
#include <cstdint>
#include "generated/tokenType.h"

namespace ark::es2panda::lsp {

RulesMapCache &RulesMapCache::Instance()
{
    static RulesMapCache cache;
    return cache;
}

RulesMap &RulesMapCache::GetRulesMap()
{
    static RulesMap rulesMap = CreateRulesMap(GetAllRules());
    return rulesMap;
}

int RulesMapCache::GetRuleBucketIndex(lexer::TokenType left, lexer::TokenType right)
{
    constexpr int MAP_ROW_LENGTH = static_cast<int>(LAST_TOKEN) + 1;
    return (static_cast<int>(left) * MAP_ROW_LENGTH) + static_cast<int>(right);
}

RuleAction RulesMapCache::GetRuleActionExclusion(RuleAction ruleAction)
{
    RuleAction mask = RuleAction::NONE;

    if ((ruleAction & RuleAction::STOP_PROCESSING_SPACE_ACTIONS) != RuleAction::NONE) {
        mask |= RuleAction::MODIFY_SPACE_ACTION;
    }
    if ((ruleAction & RuleAction::STOP_PROCESSING_TOKEN_ACTIONS) != RuleAction::NONE) {
        mask |= RuleAction::MODIFY_TOKEN_ACTION;
    }
    if ((ruleAction & RuleAction::MODIFY_SPACE_ACTION) != RuleAction::NONE) {
        mask |= RuleAction::MODIFY_SPACE_ACTION;
    }
    if ((ruleAction & RuleAction::MODIFY_TOKEN_ACTION) != RuleAction::NONE) {
        mask |= RuleAction::MODIFY_TOKEN_ACTION;
    }

    return mask;
}

int RulesMapCache::GetInsertionIndex(uint32_t bitmap, uint32_t shift)
{
    constexpr uint32_t MASK_BIT_SIZE = 5;
    constexpr uint32_t MASK = (1U << MASK_BIT_SIZE) - 1U;

    size_t index = 0;
    for (uint32_t pos = 0; pos <= shift; pos += MASK_BIT_SIZE) {
        index += bitmap & MASK;
        bitmap >>= MASK_BIT_SIZE;
    }

    return index;
}

int RulesMapCache::IncreaseInsertionIndex(uint32_t bitmap, uint32_t shift)
{
    constexpr uint32_t MASK_BIT_SIZE = 5;
    constexpr uint32_t MASK = (1U << MASK_BIT_SIZE) - 1U;

    uint32_t count = ((bitmap >> shift) & MASK) + 1;

    ES2PANDA_ASSERT((count & MASK) == count);
    return (bitmap & ~(MASK << shift)) | (count << shift);
}

void RulesMapCache::AddRule(std::vector<RuleSpec> &ruleSpecs, RuleSpec &ruleSpec, bool specificTokens,
                            unsigned int &bitmap)
{
    constexpr uint32_t MASK_BIT_SIZE = 5;
    constexpr int STOP_RULES_SPECIFIC = MASK_BIT_SIZE * 0;
    constexpr int STOP_RULES_ANY = MASK_BIT_SIZE * 1;
    constexpr int CONTEXT_RULES_SPECIFIC = MASK_BIT_SIZE * 2;
    constexpr int CONTEXT_RULES_ANY = MASK_BIT_SIZE * 3;
    constexpr int NO_CONTEXT_RULES_SPECIFIC = MASK_BIT_SIZE * 4;
    constexpr int NO_CONTEXT_RULES_ANY = MASK_BIT_SIZE * 5;

    int shift = 0;

    if ((ruleSpec.GetRule().GetRuleAction() & RuleAction::STOP_ACTION) != RuleAction::NONE) {
        shift = specificTokens ? STOP_RULES_SPECIFIC : STOP_RULES_ANY;
    } else if (!ruleSpec.GetRule().GetContext().empty()) {
        shift = specificTokens ? CONTEXT_RULES_SPECIFIC : CONTEXT_RULES_ANY;
    } else {
        shift = specificTokens ? NO_CONTEXT_RULES_SPECIFIC : NO_CONTEXT_RULES_ANY;
    }

    int insertAt = GetInsertionIndex(bitmap, shift);
    ruleSpecs.insert(ruleSpecs.begin() + insertAt, ruleSpec);
    bitmap = IncreaseInsertionIndex(bitmap, shift);
}

std::unordered_map<int, std::vector<RuleSpec>> RulesMapCache::BuildMap(const std::vector<RuleSpec> &ruleSpecs)
{
    constexpr int MAP_ROW_LENGTH = static_cast<int>(LAST_TOKEN) + 1;

    std::unordered_map<int, std::vector<RuleSpec>> map;
    std::unordered_map<int, unsigned int> bucketState;

    for (auto spec : ruleSpecs) {
        auto leftRange = spec.GetLeftTokenRange();
        std::vector<lexer::TokenType> leftTokens = leftRange.GetTokens();
        if (leftRange.GetIsSpecifier() && leftTokens.empty()) {
            leftTokens.reserve(MAP_ROW_LENGTH);
            for (int t = 0; t < MAP_ROW_LENGTH; ++t) {
                leftTokens.emplace_back(static_cast<lexer::TokenType>(t));
            }
        }

        auto rightRange = spec.GetRightTokenRange();
        std::vector<lexer::TokenType> rightTokens = rightRange.GetTokens();
        if (rightRange.GetIsSpecifier() && rightTokens.empty()) {
            rightTokens.reserve(MAP_ROW_LENGTH);
            for (int t = 0; t < MAP_ROW_LENGTH; ++t) {
                rightTokens.emplace_back(static_cast<lexer::TokenType>(t));
            }
        }

        bool specific = leftRange.GetIsSpecifier() && rightRange.GetIsSpecifier();

        for (auto l : leftTokens) {
            for (auto r : rightTokens) {
                int idx = GetRuleBucketIndex(l, r);
                auto &bucket = map[idx];
                unsigned int &bitmap = bucketState[idx];
                AddRule(bucket, spec, specific, bitmap);
            }
        }
    }
    return map;
}

RulesMap RulesMapCache::CreateRulesMap(const std::vector<RuleSpec> &ruleSpecs)
{
    auto buckets = BuildMap(ruleSpecs);
    return [buckets = std::move(buckets)](const FormattingContext &ctx) -> std::vector<RuleSpec> {
        lexer::TokenType leftKind = ctx.GetCurrentToken().Type();
        lexer::TokenType rightKind = ctx.GetNextToken().Type();
        int index = GetRuleBucketIndex(leftKind, rightKind);
        auto it = buckets.find(index);
        if (it == buckets.end()) {
            return {};
        }

        const auto &bucket = it->second;
        std::vector<RuleSpec> result;
        RuleAction mask = RuleAction::NONE;

        for (const auto &spec : bucket) {
            auto action = spec.GetRule().GetRuleAction();
            auto exclusion = GetRuleActionExclusion(mask);
            auto allowed = static_cast<RuleAction>(~exclusion);
            auto bitCheck = static_cast<RuleAction>(action & allowed);

            if (bitCheck == RuleAction::NONE) {
                continue;
            }

            bool allPass = true;
            for (const auto &pred : spec.GetRule().GetContext()) {
                bool ok = pred(const_cast<FormattingContext *>(&ctx));
                if (!ok) {
                    allPass = false;
                    break;
                }
            }
            if (!allPass) {
                continue;
            }

            result.push_back(spec);
            mask = static_cast<RuleAction>(mask | action);
        }

        return result;
    };
}

}  // namespace ark::es2panda::lsp
