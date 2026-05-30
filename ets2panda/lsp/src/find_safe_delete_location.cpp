/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
#include "find_safe_delete_location.h"
#include "lsp/include/lsp_utils.h"
#include "lsp/include/symbol_reference_index.h"
#include "public/public.h"

namespace ark::es2panda::lsp {

std::vector<SafeDeleteLocation> FindSafeDeleteLocationImpl(es2panda_Context *ctx, size_t position)
{
    std::vector<SafeDeleteLocation> locations;
    if (ctx == nullptr) {
        return locations;
    }

    auto *context = reinterpret_cast<public_lib::Context *>(ctx);
    std::string source = std::string(context->parserProgram->SourceCode());
    size_t byteOffset = CodePointOffsetToByteOffset(source, position);
    auto references = GetReferencesAtPositionFromIndex(ctx, byteOffset);
    std::unordered_set<std::string> seen;
    auto appendLocation = [&locations, &seen](const ReferenceInfo &ref) {
        if (ref.fileName.empty()) {
            return;
        }
        std::string key = ref.fileName + ":" + std::to_string(ref.start) + ":" + std::to_string(ref.length);
        if (!seen.insert(key).second) {
            return;
        }
        SafeDeleteLocation loc;
        loc.uri = ref.fileName;
        loc.start = ref.start;
        loc.length = ref.length;
        locations.push_back(std::move(loc));
    };

    if (!references.definitionInfo.fileName.empty()) {
        auto defFileSource = GetIndexedFileSource(references.definitionInfo.fileName);
        if (!defFileSource.empty()) {
            size_t startCharOffset = ByteOffsetToCodePointOffset(defFileSource, references.definitionInfo.start);
            size_t lengthChar = ByteOffsetToCodePointOffset(defFileSource, references.definitionInfo.start +
                                                                               references.definitionInfo.length) -
                                startCharOffset;
            appendLocation(ReferenceInfo(references.definitionInfo.fileName, startCharOffset, lengthChar));
        }
    }
    for (const auto &ref : references.referenceInfos) {
        auto fileSource = GetIndexedFileSource(ref.fileName);
        if (fileSource.empty()) {
            continue;
        }
        size_t startCharOffset = ByteOffsetToCodePointOffset(fileSource, ref.start);
        size_t lengthChar = ByteOffsetToCodePointOffset(fileSource, ref.start + ref.length) - startCharOffset;
        appendLocation(ReferenceInfo(ref.fileName, startCharOffset, lengthChar));
    }
    return locations;
}

}  // namespace ark::es2panda::lsp
