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

#ifndef RULES_MAP_H
#define RULES_MAP_H

#include <cstdint>
#include <vector>
#include "rules.h"

namespace ark::es2panda::lsp {

using RulesMap = std::function<std::vector<RuleSpec>(const FormattingContext &)>;
const lexer::TokenType LAST_TOKEN = lexer::TokenType::KEYW_YIELD;

class RulesMapCache {
public:
    static RulesMapCache &Instance();

    static RulesMap &GetRulesMap();

    ~RulesMapCache() = default;
    NO_COPY_SEMANTIC(RulesMapCache);
    NO_MOVE_SEMANTIC(RulesMapCache);

    RulesMapCache() = default;

    static RulesMap rulesMap_;

    static RulesMap CreateRulesMap(const std::vector<RuleSpec> &ruleSpec);

    static int GetInsertionIndex(uint32_t bitmap, uint32_t shift);
    static int IncreaseInsertionIndex(uint32_t bitmap, uint32_t shift);
    static void AddRule(std::vector<RuleSpec> &ruleSpecs, RuleSpec &ruleSpec, bool specificTokens,
                        unsigned int &bitmap);
    static RuleAction GetRuleActionExclusion(RuleAction ruleAction);
    static int GetRuleBucketIndex(lexer::TokenType left, lexer::TokenType right);
    static std::unordered_map<int, std::vector<RuleSpec>> BuildMap(const std::vector<RuleSpec> &ruleSpecs);
};

}  // namespace ark::es2panda::lsp

#endif
