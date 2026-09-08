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

#include "util/base64.h"

#include <array>
#include <string>

namespace ark::es2panda::util {

namespace {

constexpr const char *BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

constexpr int BASE64_DECODE_INVALID = -1;
constexpr int BASE64_ALPHABET_SIZE = 64;
constexpr size_t BASE64_DECODE_TABLE_SIZE = 256;

const std::array<int, BASE64_DECODE_TABLE_SIZE> &GetDecodeTable()
{
    static const auto table = []() {
        std::array<int, BASE64_DECODE_TABLE_SIZE> t {};
        t.fill(BASE64_DECODE_INVALID);
        for (int i = 0; i < BASE64_ALPHABET_SIZE; i++) {
            t[static_cast<unsigned char>(BASE64_CHARS[i])] = i;
        }
        return t;
    }();
    return table;
}
constexpr size_t BASE64_INPUT_GROUP_SIZE = 3;
constexpr size_t BASE64_OUTPUT_GROUP_SIZE = 4;
constexpr size_t BASE64_BITS_PER_CHAR = 6;
constexpr int BASE64_CHAR_MASK = 0x3F;
constexpr size_t BASE64_BYTE0_SHIFT = 16;
constexpr size_t BASE64_BYTE1_SHIFT = 8;
constexpr size_t BASE64_SEXTET0_SHIFT = 18;
constexpr size_t BASE64_SEXTET1_SHIFT = 12;
constexpr int BASE64_BYTE_MASK = 0xFF;
constexpr size_t BASE64_BYTE1_OFF = 1;
constexpr size_t BASE64_BYTE2_OFF = 2;
constexpr int BASE64_MAX_PAD_FOR_BYTE2 = 2;
constexpr int BASE64_MAX_PAD_FOR_BYTE3 = 1;

}  // namespace

std::string Base64Encode(const std::string &input)
{
    std::string result;
    size_t len = input.length();
    result.reserve(((len + BASE64_INPUT_GROUP_SIZE - 1) / BASE64_INPUT_GROUP_SIZE) * BASE64_OUTPUT_GROUP_SIZE);
    for (size_t i = 0; i < len; i += BASE64_INPUT_GROUP_SIZE) {
        unsigned int val = (static_cast<unsigned char>(input[i]) << BASE64_BYTE0_SHIFT);
        if (i + BASE64_BYTE1_OFF < len) {
            val |= (static_cast<unsigned char>(input[i + BASE64_BYTE1_OFF]) << BASE64_BYTE1_SHIFT);
        }
        if (i + BASE64_BYTE2_OFF < len) {
            val |= static_cast<unsigned char>(input[i + BASE64_BYTE2_OFF]);
        }
        result += BASE64_CHARS[(val >> BASE64_SEXTET0_SHIFT) & BASE64_CHAR_MASK];
        result += BASE64_CHARS[(val >> BASE64_SEXTET1_SHIFT) & BASE64_CHAR_MASK];
        result += (i + BASE64_BYTE1_OFF < len) ? BASE64_CHARS[(val >> BASE64_BITS_PER_CHAR) & BASE64_CHAR_MASK] : '=';
        result += (i + BASE64_BYTE2_OFF < len) ? BASE64_CHARS[val & BASE64_CHAR_MASK] : '=';
    }
    return result;
}

std::string Base64Decode(const std::string &input)
{
    std::string result;
    size_t len = input.length();
    if (len % BASE64_OUTPUT_GROUP_SIZE != 0) {
        return result;
    }
    result.reserve((len / BASE64_OUTPUT_GROUP_SIZE) * BASE64_INPUT_GROUP_SIZE);
    for (size_t i = 0; i < len; i += BASE64_OUTPUT_GROUP_SIZE) {
        unsigned int val = 0;
        int padding = 0;
        for (size_t j = 0; j < BASE64_OUTPUT_GROUP_SIZE; j++) {
            char c = input[i + j];
            if (c == '=') {
                padding++;
                val <<= BASE64_BITS_PER_CHAR;
                continue;
            }
            int idx = GetDecodeTable()[static_cast<unsigned char>(c)];
            if (idx == BASE64_DECODE_INVALID) {
                return std::string();
            }
            val = (val << BASE64_BITS_PER_CHAR) | static_cast<unsigned int>(idx);
        }
        result += static_cast<char>((val >> BASE64_BYTE0_SHIFT) & BASE64_BYTE_MASK);
        if (padding < BASE64_MAX_PAD_FOR_BYTE2) {
            result += static_cast<char>((val >> BASE64_BYTE1_SHIFT) & BASE64_BYTE_MASK);
        }
        if (padding < BASE64_MAX_PAD_FOR_BYTE3) {
            result += static_cast<char>(val & BASE64_BYTE_MASK);
        }
    }
    return result;
}

}  // namespace ark::es2panda::util
