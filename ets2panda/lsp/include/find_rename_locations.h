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
#ifndef FIND_RENAME_LOCATIONS_H
#define FIND_RENAME_LOCATIONS_H

#include <cstddef>
#include <string>
#include <set>

#include "es2panda.h"
#include "cancellation_token.h"

namespace ark::es2panda::lsp {

// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct RenameLocation {
    std::string fileName;
    size_t start = 0;
    size_t end = 0;
    size_t line = 0;
    std::string prefixText;
    std::string suffixText;

    bool operator<(const RenameLocation &other) const
    {
        return std::tie(fileName, start, end, line, prefixText, suffixText) <
               std::tie(other.fileName, other.start, other.end, other.line, other.prefixText, other.suffixText);
    }
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

std::set<RenameLocation> FindRenameLocations(CancellationToken *tkn,
                                             const std::vector<es2panda_Context *> &fileContexts,
                                             es2panda_Context *context, size_t position);

std::set<RenameLocation> FindRenameLocations(const std::vector<es2panda_Context *> &fileContexts,
                                             es2panda_Context *context, size_t position);

}  // namespace ark::es2panda::lsp

#endif  // FIND_RENAME_LOCATIONS_H