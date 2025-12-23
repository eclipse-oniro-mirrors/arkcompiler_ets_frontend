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

#include "lsp/include/api.h"
#include "internal_api.h"
#include "references.h"
#include "ir/astNode.h"
#include "find_safe_delete_location.h"

namespace ark::es2panda::lsp {

std::vector<SafeDeleteLocation> FindSafeDeleteLocationImpl(es2panda_Context *ctx,
                                                           const std::tuple<std::string, std::string> &declInfo)
{
    std::vector<SafeDeleteLocation> locations;
    std::unordered_set<std::string> seen;
    auto references = GetReferencesAtPositionImpl(ctx, declInfo);
    auto decl = ResolveInfo(declInfo);
    if (decl.fileName.empty()) {
        return locations;
    }
    SafeDeleteLocation declLoc;
    declLoc.uri = decl.fileName;
    declLoc.start = decl.start;
    declLoc.length = decl.length;
    locations.reserve(references.referenceInfos.size() + 1);

    locations.push_back(declLoc);
    for (const auto &ref : references.referenceInfos) {
        std::string key = ref.fileName + ":" + std::to_string(ref.start) + ":" + std::to_string(ref.length);
        if (seen.insert(key).second) {
            SafeDeleteLocation loc;
            loc.uri = ref.fileName;
            loc.start = ref.start;
            loc.length = ref.length;
            locations.push_back(loc);
        }
    }

    return locations;
}

}  // namespace ark::es2panda::lsp
