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

#ifndef RULES_H
#define RULES_H

#include <vector>
#include "rule.h"

namespace ark::es2panda::lsp {

struct RuleSpec {
public:
    explicit RuleSpec(Rule rule, TokenRange leftTokenRange, TokenRange rightTokenRange)
        : rule_(std::move(rule)),
          leftTokenRange_(std::move(leftTokenRange)),
          rightTokenRange_(std::move(rightTokenRange))
    {
    }

    Rule &GetRule()
    {
        return rule_;
    }

    const Rule &GetRule() const
    {
        return rule_;
    }

    TokenRange &GetLeftTokenRange()
    {
        return leftTokenRange_;
    }

    const TokenRange &GetLeftTokenRange() const
    {
        return leftTokenRange_;
    }

    TokenRange &GetRightTokenRange()
    {
        return rightTokenRange_;
    }

    const TokenRange &GetRightTokenRange() const
    {
        return rightTokenRange_;
    }

private:
    Rule rule_;
    TokenRange leftTokenRange_;
    TokenRange rightTokenRange_;
};

std::vector<RuleSpec> GetAllRules();

}  // namespace ark::es2panda::lsp

#endif
