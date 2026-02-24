/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "internalAPIWhitelist.h"

#include <algorithm>
#include <string>
#include <unordered_set>

namespace ark::es2panda::util {

using RestrictionSet = std::pair<std::string_view, std::unordered_set<std::string_view>>;

static bool NamespaceIsPrefixedWith(std::string_view internalName, std::string_view prefix)
{
    return internalName.rfind(prefix, 0) == 0 &&
           (internalName.length() == prefix.length() || internalName[prefix.length()] == '.');
}

static bool IsRestricted(std::string_view namespaceName, RestrictionSet const &restriction)
{
    auto const &prefixSet = restriction.second;
    return !std::any_of(prefixSet.begin(), prefixSet.end(), [namespaceName](std::string_view prefix) {
        return NamespaceIsPrefixedWith(namespaceName, prefix);
    });
}

std::vector<std::string_view> ComputeRestrictedAPIAnnotationsAt(std::string_view namespaceName)
{
    std::vector<std::string_view> restricted;

    auto const addRestriction = [&restricted, namespaceName](RestrictionSet const &restriction) {
        if (IsRestricted(namespaceName, restriction)) {
            restricted.push_back(restriction.first);
        }
    };

    static const RestrictionSet ARKRUNTIME_INTERNAL_API_RESTRICTIONS =
        // It is strictly forbidden to modify this list
        // CC-OFFNXT(G.FMT.03-CPP) project code style
        {"arkruntime.annotation.InternalAPI",
         {
             "arkruntime",
             "std",
             "escompat",
         }};
    addRestriction(ARKRUNTIME_INTERNAL_API_RESTRICTIONS);

    static const RestrictionSet ARKRUNTIME_BYTECODE_API_RESTRICTIONS =
        // It is strictly forbidden to modify this list
        // CC-OFFNXT(G.FMT.03-CPP) project code style
        {"arkruntime.annotation.BytecodeAPI",
         {
             "arkruntime",
         }};
    addRestriction(ARKRUNTIME_BYTECODE_API_RESTRICTIONS);

    return restricted;
}

}  // namespace ark::es2panda::util
