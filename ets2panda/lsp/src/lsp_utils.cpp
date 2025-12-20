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
        unsigned char c = static_cast<unsigned char>(content[byteOffset]);
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
        unsigned char c = static_cast<unsigned char>(content[bytes]);
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

}  // namespace ark::es2panda::lsp