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

RulesMap RulesMapCache::CreateRulesMap(const std::vector<RuleSpec> &ruleSpec)
{
    return [ruleSpec]([[maybe_unused]] const FormattingContext &ctx) { return ruleSpec; };
}

}  // namespace ark::es2panda::lsp
