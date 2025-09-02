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

#ifndef FIND_REFERENCES_H
#define FIND_REFERENCES_H

#include <cstddef>
#include <string>
#include <tuple>
#include <vector>

#include "es2panda.h"
#include "cancellation_token.h"

namespace ark::es2panda::lsp {

// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct ReferencedNode {
    std::string filePath;
    size_t start = 0;
    size_t end = 0;
    size_t line = 0;
    bool isDefinition = false;

    bool operator<(const ReferencedNode &other) const
    {
        return std::tie(filePath, start, end, line, isDefinition) <
               std::tie(other.filePath, other.start, other.end, other.line, other.isDefinition);
    }
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

// Returns a map of file path and reference position list
std::set<ReferencedNode> FindReferences(CancellationToken *tkn, const std::vector<SourceFile> &files,
                                        const SourceFile &file, size_t position);
std::set<ReferencedNode> FindReferences(CancellationToken *tkn, const std::vector<es2panda_Context *> &fileContexts,
                                        es2panda_Context *context, size_t position);
}  // namespace ark::es2panda::lsp

#endif