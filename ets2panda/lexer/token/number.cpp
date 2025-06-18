/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "number.h"
#include "lexer/lexer.h"

namespace ark::es2panda::lexer {
// CC-OFFNXT(huge_depth[C++], C_RULE_ID_FUNCTION_NESTING_LEVEL, G.FUN.01-CPP, G.FUD.05) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,bugprone-exception-escape)
Number::Number(util::StringView str, NumberFlags flags) noexcept : str_(str), flags_(flags)
{
    Lexer::ConversionResult res {};

    const auto s = str.Utf8();
    const bool hasFloatSuffix = !s.empty() && s.back() == 'f';
    const bool hasPointOrExp =
        (flags & (NumberFlags::DECIMAL_POINT | NumberFlags::EXPONENT)) != std::underlying_type_t<TokenFlags>(0);

    if (!hasPointOrExp && !hasFloatSuffix) {
        const int64_t temp = Lexer::StrToNumeric(&std::strtoll, s.data(), res, 10);

        if (res == Lexer::ConversionResult::SUCCESS) {
            if (temp <= std::numeric_limits<int32_t>::max() && temp >= std::numeric_limits<int32_t>::min()) {
                num_ = static_cast<int32_t>(temp);
            } else {
                num_ = temp;
            }
        }
    } else {
        if (hasFloatSuffix) {
            if (!hasPointOrExp) {
                flags_ |= NumberFlags::ERROR;
            } else {
                // NOTE(dkofanov): floats should be parsed via 'strtof', however there are problems with subnormal
                // values.
                num_ = Lexer::StrToNumeric<double, float>(&std::strtod, s.data(), res);
            }
        } else {
            num_ = Lexer::StrToNumeric(&std::strtod, s.data(), res);
        }
    }
    if (res != Lexer::ConversionResult::SUCCESS) {
        num_ = std::monostate {};
        flags_ |= NumberFlags::ERROR;
    }
}
}  // namespace ark::es2panda::lexer
