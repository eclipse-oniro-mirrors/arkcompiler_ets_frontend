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
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ctime>
#include <string>

#include "find_rename_locations.h"
#include "find_references.h"
#include "public/public.h"

namespace ark::es2panda::lsp {

std::set<RenameLocation> FindRenameLocations(CancellationToken *tkn,
                                             const std::vector<es2panda_Context *> &fileContexts,
                                             es2panda_Context *context, size_t position)
{
    auto references = FindReferences(tkn, fileContexts, context, position);
    std::set<RenameLocation> res;

    for (auto ref : references) {
        auto fileIt = std::find_if(fileContexts.begin(), fileContexts.end(), [&ref](es2panda_Context *fileContext) {
            auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(fileContext);
            return ctx->sourceFile->filePath == ref.filePath;
        });
        if (fileIt == fileContexts.end()) {
            std::cout << "Error: Could not find " << ref.filePath << " in list!\n";
            continue;
        }
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(*fileIt);
        std::string source = std::string {ctx->sourceFile->source};
        // Get prefix and suffix texts
        std::string prefix;
        {
            auto end = source.begin() + ref.start;
            auto beg = end;
            while (beg > source.begin() && *(beg - 1) != '\n') {
                --beg;
            }
            prefix = std::string {beg, end};
        }
        // Suffix
        std::string suffix;
        {
            auto beg = source.begin() + ref.end;
            auto end = beg;
            while (end < source.end() && *end != '\n') {
                ++end;
            }
            suffix = std::string {beg, end};
        }
        res.insert(RenameLocation {ref.filePath, ref.start, ref.end, ref.line, prefix, suffix});
    }

    return res;
}

std::set<RenameLocation> FindRenameLocations(const std::vector<es2panda_Context *> &fileContexts,
                                             es2panda_Context *context, size_t position)
{
    time_t tmp = 0;
    CancellationToken cancellationToken {tmp, nullptr};
    return FindRenameLocations(&cancellationToken, fileContexts, context, position);
}
}  // namespace ark::es2panda::lsp