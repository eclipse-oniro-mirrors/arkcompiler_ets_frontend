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

#include "lsp_utils.h"
#include <algorithm>
#include <string>

namespace ark::es2panda::lsp {

enum Utf8LeadByte {
    UTF8_2BYTE_LEAD = 0xC0,  // 110xxxxx
    UTF8_3BYTE_LEAD = 0xE0,  // 1110xxxx
    UTF8_4BYTE_LEAD = 0xF0   // 11110xxx
};

constexpr size_t UTF8_1BYTE_LEN = 1;
constexpr size_t UTF8_2BYTE_LEN = 2;
constexpr size_t UTF8_3BYTE_LEN = 3;
constexpr size_t UTF8_4BYTE_LEN = 4;

size_t CodePointOffsetToByteOffset(const std::string &content, size_t charOffset)
{
    size_t byteOffset = 0;
    size_t chars = 0;
    while (byteOffset < content.size() && chars < charOffset) {
        auto c = static_cast<unsigned char>(content[byteOffset]);
        size_t charLen = UTF8_1BYTE_LEN;
        if (c >= UTF8_4BYTE_LEAD) {
            charLen = UTF8_4BYTE_LEN;
        } else if (c >= UTF8_3BYTE_LEAD) {
            charLen = UTF8_3BYTE_LEN;
        } else if (c >= UTF8_2BYTE_LEAD) {
            charLen = UTF8_2BYTE_LEN;
        }
        byteOffset += charLen;
        ++chars;
    }
    return byteOffset;
}

size_t ByteOffsetToCodePointOffset(const std::string &content, size_t byteOffset)
{
    size_t chars = 0;
    size_t bytes = 0;
    while (bytes < content.size() && bytes < byteOffset) {
        auto c = static_cast<unsigned char>(content[bytes]);
        size_t charLen = UTF8_1BYTE_LEN;
        if (c >= UTF8_4BYTE_LEAD) {
            charLen = UTF8_4BYTE_LEN;
        } else if (c >= UTF8_3BYTE_LEAD) {
            charLen = UTF8_3BYTE_LEN;
        } else if (c >= UTF8_2BYTE_LEAD) {
            charLen = UTF8_2BYTE_LEN;
        }
        if (bytes + charLen > byteOffset) {
            break;
        }
        ++chars;
        bytes += charLen;
    }
    return chars;
}

std::string ApplyRefactorTextChangesToSource(const std::string &source, const std::vector<TextChange> &textChanges)
{
    if (textChanges.empty()) {
        return source;
    }

    std::vector<const TextChange *> ordered;
    ordered.reserve(textChanges.size());
    for (const auto &change : textChanges) {
        ordered.push_back(&change);
    }
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const TextChange *lhs, const TextChange *rhs) { return lhs->span.start < rhs->span.start; });

    std::string result;
    result.reserve(source.size());
    size_t cursor = 0;
    for (const auto *change : ordered) {
        if (change->span.start > source.size()) {
            continue;
        }
        const size_t originalStart = change->span.start;
        const size_t start = std::max(originalStart, cursor);
        const size_t remaining = source.size() - originalStart;
        const size_t end = originalStart + std::min(change->span.length, remaining);
        if (cursor < start) {
            result.append(source, cursor, start - cursor);
        }
        result.append(change->newText);
        cursor = std::max(cursor, end);
    }
    if (cursor < source.size()) {
        result.append(source, cursor, source.size() - cursor);
    }
    return result;
}

std::string GetRefactoredSourceForRenameLocation(const std::string &source,
                                                 const std::vector<FileTextChanges> &fileTextChanges,
                                                 const std::optional<std::string> &renameFileName)
{
    if (!renameFileName.has_value()) {
        return source;
    }
    for (const auto &fileChange : fileTextChanges) {
        if (fileChange.fileName == renameFileName.value()) {
            return ApplyRefactorTextChangesToSource(source, fileChange.textChanges);
        }
    }
    return source;
}

}  // namespace ark::es2panda::lsp
