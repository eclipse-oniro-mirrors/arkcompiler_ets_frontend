/**
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

#include "numberLiteral.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "util/dtoa_helper.h"

namespace ark::es2panda::ir {

inline constexpr size_t BUF_SIZE = 128;
inline constexpr int TWO = 2;
inline constexpr int SIX = 6;
inline constexpr int TWENTY_ONE = 21;
inline constexpr size_t TEN = 10;

void NumberLiteral::TransformChildren([[maybe_unused]] const NodeTransformer &cb,
                                      [[maybe_unused]] std::string_view const transformationName)
{
}

void NumberLiteral::Iterate([[maybe_unused]] const NodeTraverser &cb) const {}

void NumberLiteral::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "NumberLiteral"}, {"value", number_}});
}

void NumberLiteral::Dump(ir::SrcDumper *dumper) const
{
    if (std::string(number_.Str()).empty() || (parent_ != nullptr && parent_->IsTSEnumMember())) {
        if (number_.IsInt()) {
            dumper->Add(number_.GetInt());
            return;
        }

        if (number_.IsLong()) {
            dumper->Add(number_.GetLong());
            return;
        }

        if (number_.IsFloat()) {
            dumper->Add(number_.GetFloat());
            return;
        }

        if (number_.IsDouble()) {
            dumper->Add(number_.GetDouble());
            return;
        }

        if (number_.IsShort()) {
            dumper->Add(number_.GetShort());
            return;
        }

        if (number_.IsByte()) {
            dumper->Add(number_.GetByte());
            return;
        }
    }
    dumper->Add(std::string(number_.Str()));
}

void NumberLiteral::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void NumberLiteral::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *NumberLiteral::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType NumberLiteral::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

NumberLiteral *NumberLiteral::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = allocator->New<NumberLiteral>(number_);
    ES2PANDA_ASSERT(clone != nullptr);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    clone->SetRange(Range());
    clone->SetFolded(IsFolded());
    return clone;
}

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
template <typename FpType, std::enable_if_t<std::is_floating_point_v<FpType>, bool> = true>
static FpType StrToFp(char *str, char **strEnd)
{
    if constexpr (std::is_same_v<FpType, double>) {
        return std::strtod(str, strEnd);
    } else {
        return std::strtof(str, strEnd);
    }
}

static int ParseExponent(const char *from, const char *end)
{
    int result = 0;
    bool negative = false;

    if (*from == '-') {
        negative = true;
        ++from;
    } else if (*from == '+') {
        ++from;
    }

    // Parse the digits of the exponent
    while (from != end && (std::isdigit(*from) != 0)) {
        result = result * TEN + (*from - '0');
        ++from;
    }

    if (negative) {
        result = -result;
    }

    return result;
}

template <typename FpType, std::enable_if_t<std::is_floating_point_v<FpType>, bool> = true>
void GetBase(FpType d, int digits, int *decpt, Span<char> buf)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    auto ret = snprintf_s(buf.begin(), buf.size(), buf.size() - 1, "%.*e", digits - 1, d);
    if (ret == -1) {
        ES2PANDA_UNREACHABLE();
    }
    char *end = buf.begin() + ret;
    ES2PANDA_ASSERT(*end == 0);
    const size_t positive = (digits > 1) ? 1 : 0;
    char *ePos = buf.begin() + digits + positive;
    ES2PANDA_ASSERT(*ePos == 'e');
    char *signPos = ePos + 1;
    char *from = signPos + 1;
    // exponent
    *decpt = ParseExponent(from, end);

    if (*signPos == '-') {
        *decpt *= -1;
    }
    ++*decpt;
}

template <typename FpType>
static int GetBaseBinarySearch(FpType d, int *decpt, Span<char> buf)
{
    // find the minimum amount of digits
    int minDigits = 1;
    int maxDigits = std::is_same_v<FpType, double> ? ark::es2panda::util::DOUBLE_MAX_PRECISION
                                                   : ark::es2panda::util::FLOAT_MAX_PRECISION;
    int digits;

    while (minDigits < maxDigits) {
        digits = (minDigits + maxDigits) / TWO;
        GetBase(d, digits, decpt, buf);

        bool same = StrToFp<FpType>(buf.begin(), nullptr) == d;
        if (same) {
            // no need to keep the trailing zeros
            while (digits >= 2 && buf[digits] == '0') {  // 2 means ignore the integer and point
                digits--;
            }
            maxDigits = digits;
        } else {
            minDigits = digits + 1;
        }
    }
    digits = maxDigits;
    GetBase(d, digits, decpt, buf);
    return digits;
}

template <typename IntegerType>
static void IntegerToString(IntegerType number, std::string &resStr)
{
    std::ostringstream oss;
    oss << number;
    resStr = oss.str();
}

template <typename FpType>
[[maybe_unused]] static bool RecheckGetMinimumDigits(FpType d, Span<char> buf)
{
    ES2PANDA_ASSERT(StrToFp<FpType>(buf.begin(), nullptr) == d);
    std::string str(buf.begin());
    auto pos = str.find('e');
    std::copy(str.begin() + pos, str.end(), str.begin() + pos - 1);
    str[str.size() - 1] = '\0';
    return StrToFp<FpType>(str.data(), nullptr) != d;
}

// result is written starting with buf[1]
template <typename FpType>
static int GetMinimumDigits(FpType d, int *decpt, Span<char> buf)
{
    if (std::is_same_v<FpType, double>) {
        util::DtoaHelper helper(buf.begin() + 1);
        helper.Dtoa(d);
        *decpt = helper.GetPoint();
        return helper.GetDigits();
    }
    // Note(daizihan): Can use GetBaseFast as in runtime implementation, but need charconv supported by compiler.
    int digits = GetBaseBinarySearch(d, decpt, buf);
    ES2PANDA_ASSERT(RecheckGetMinimumDigits(d, buf));
    ES2PANDA_ASSERT(digits == 1 || buf[1] == '.');
    buf[1] = buf[0];
    return digits;
}

template <typename FpType>
static Span<char> FpToStringDecimalRadixMainCase(FpType number, bool negative, Span<char> buffer)
{
    auto bufferStart = buffer.begin() + 2U;
    ES2PANDA_ASSERT(number > 0);
    int n = 0;
    int k = GetMinimumDigits(number, &n, buffer.SubSpan(1));
    auto bufferEnd = bufferStart + k;

    if (0 < n && n <= TWENTY_ONE) {  // NOLINT(readability-magic-numbers)
        if (k <= n) {
            // 6. If k ≤ n ≤ 21, return the String consisting of the code units of the k digits of the decimal
            // representation of s (in order, with no leading zeroes), followed by n−k occurrences of the code unit
            // 0x0030 (DIGIT ZERO).
            std::fill_n(bufferEnd, n - k, '0');
            bufferEnd += n - k;
        } else {
            // 7. If 0 < n ≤ 21, return the String consisting of the code units of the most significant n digits of the
            // decimal representation of s, followed by the code unit 0x002E (FULL STOP), followed by the code units of
            // the remaining k−n digits of the decimal representation of s.
            auto fracStart = bufferStart + n;
            if (memmove_s(fracStart + 1, buffer.end() - (fracStart + 1), fracStart, k - n) != EOK) {
                ES2PANDA_UNREACHABLE();
            }
            *fracStart = '.';
            bufferEnd++;
        }
    } else if (-SIX < n && n <= 0) {  // NOLINT(readability-magic-numbers)
        // 8. If −6 < n ≤ 0, return the String consisting of the code unit 0x0030 (DIGIT ZERO), followed by the code
        // unit 0x002E (FULL STOP), followed by −n occurrences of the code unit 0x0030 (DIGIT ZERO), followed by the
        // code units of the k digits of the decimal representation of s.
        size_t length = -n + 2U;
        auto fracStart = bufferStart + length;
        if (memmove_s(fracStart, buffer.end() - fracStart, bufferStart, k) != EOK) {
            ES2PANDA_UNREACHABLE();
        }
        std::fill_n(bufferStart, length, '0');
        bufferStart[1] = '.';
        bufferEnd += length;
    } else {
        if (k == 1) {
            // 9. Otherwise, if k = 1, return the String consisting of the code unit of the single digit of s
            ES2PANDA_ASSERT(bufferEnd == bufferStart + 1);
        } else {
            *(bufferStart - 1) = *bufferStart;
            *(bufferStart--) = '.';
        }
        // followed by code unit 0x0065 (LATIN SMALL LETTER E), followed by the code unit 0x002B (PLUS SIGN) or the code
        // unit 0x002D (HYPHEN-MINUS) according to whether n−1 is positive or negative, followed by the code units of
        // the decimal representation of the integer abs(n−1) (with no leading zeroes).
        *(bufferEnd++) = 'e';
        if (n >= 1) {
            *(bufferEnd++) = '+';
        }
        std::string result = std::to_string(n - 1);
        if (bufferEnd + result.size() <= buffer.end()) {
            bufferEnd = std::copy(result.begin(), result.end(), bufferEnd);
        } else {
            ES2PANDA_UNREACHABLE();
        }
    }
    if (negative) {
        *--bufferStart = '-';
    }
    return {bufferStart, bufferEnd};
}

template <typename FpType>
static char *SmallFpToString(FpType number, bool negative, char *buffer)
{
    using SignedInt = typename ark::helpers::TypeHelperT<sizeof(FpType) * CHAR_BIT, true>;
    if (negative) {
        *(buffer++) = '-';
    }
    *(buffer++) = '0';
    *(buffer++) = '.';
    SignedInt power = TEN;
    SignedInt s = 0;
    int maxDigits = std::is_same_v<FpType, double> ? ark::es2panda::util::DOUBLE_MAX_PRECISION
                                                   : ark::es2panda::util::FLOAT_MAX_PRECISION;
    int digits = maxDigits;
    for (int k = 1; k <= maxDigits; ++k) {
        s = static_cast<SignedInt>(number * power);
        if (k == maxDigits || s / static_cast<FpType>(power) == number) {  // s * (10 ** -k)
            digits = k;
            break;
        }
        power *= TEN;
    }
    for (int k = digits - 1; k >= 0; k--) {
        auto digit = std::abs(s) % TEN;
        s /= TEN;
        *(buffer + k) = '0' + digit;
    }
    return buffer + digits;
}

template <typename FpType>
static const char *FpNonFiniteToString(FpType number)
{
    ES2PANDA_ASSERT(std::isnan(number) || !std::isfinite(number));
    if (std::isnan(number)) {
        return "NaN";
    }
    return std::signbit(number) ? "-Infinity" : "Infinity";
}

template <typename FpType>
static void FpToString(FpType number, std::string &resStr)
{
    static constexpr FpType MIN_BOUND = 0.1;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    std::array<char, BUF_SIZE + 2U> buffer;
    if (INT_MIN < number && number < static_cast<FpType>(INT_MAX)) {
        if (auto intVal = static_cast<int32_t>(number); number == static_cast<double>(intVal)) {
            IntegerToString(intVal, resStr);
            return;
        }
    }

    // isfinite checks if the number is normal, subnormal or zero, but not infinite or NaN.
    if (!std::isfinite(number)) {
        auto *str = FpNonFiniteToString(number);
        resStr.assign(str);
        return;
    }

    bool negative = false;
    if (number < 0) {
        negative = true;
        number = -number;
    }

    if (!std::is_same_v<FpType, double> && MIN_BOUND <= number && number < 1) {
        // Fast path. In this case, n==0, just need to calculate k and s.
        auto bufferEnd = SmallFpToString(number, negative, buffer.begin());
        resStr.assign(buffer.data(), bufferEnd - buffer.data());
        return;
    }

    auto newBuffer = FpToStringDecimalRadixMainCase(number, negative, Span(buffer));
    resStr.assign(newBuffer.begin(), newBuffer.size());
}
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

std::string NumberLiteral::ToString() const
{
    std::string result {};
    if (number_.IsInt()) {
        IntegerToString(number_.GetInt(), result);
    }

    if (number_.IsLong()) {
        IntegerToString(number_.GetLong(), result);
    }

    if (number_.IsDouble()) {
        FpToString(number_.GetDouble(), result);
    }

    if (number_.IsFloat()) {
        FpToString(number_.GetFloat(), result);
    }

    if (number_.IsShort()) {
        IntegerToString(number_.GetShort(), result);
    }

    if (number_.IsByte()) {
        IntegerToString(static_cast<int32_t>(number_.GetByte()), result);
    }

    ES2PANDA_ASSERT(!result.empty());
    return result;
}
}  // namespace ark::es2panda::ir
