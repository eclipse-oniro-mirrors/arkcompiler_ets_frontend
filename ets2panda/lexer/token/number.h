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

#ifndef ES2PANDA_LEXER_TOKEN_NUMBER_H
#define ES2PANDA_LEXER_TOKEN_NUMBER_H

#include "util/diagnosticEngine.h"
#include "util/ustring.h"
#include "util/enumbitops.h"
#include <type_traits>

namespace ark::es2panda::lexer {

using ENUMBITOPS_OPERATORS;

enum class NumberFlags : uint32_t {
    NONE,
    BIGINT = 1U << 0U,
    DECIMAL_POINT = 1U << 1U,
    EXPONENT = 1U << 2U,
    ERROR = 1U << 3U,
};

}  // namespace ark::es2panda::lexer

template <>
struct enumbitops::IsAllowedType<ark::es2panda::lexer::NumberFlags> : std::true_type {
};

namespace ark::es2panda::lexer {

// NOLINTBEGIN(readability-identifier-naming)
// NOLINTBEGIN(fuchsia-multiple-inheritance)
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
// NOLINTEND(fuchsia-multiple-inheritance)

template <typename>
inline constexpr bool dependent_false_v = false;
// NOLINTEND(readability-identifier-naming)

class Number {
public:
    enum class TypeRank {
        // Keep this order
        INVALID,
        INT8,
        INT16,
        INT32,
        INT64,
        FLOAT,
        DOUBLE
    };

    Number() noexcept : num_(std::monostate()), flags_ {NumberFlags::ERROR} {};
    // NOLINTNEXTLINE(bugprone-exception-escape)
    Number(util::StringView str, NumberFlags flags) noexcept;
    Number(util::StringView str, double num) noexcept : str_(str), num_(num) {}

    template <typename T, typename Signed = std::enable_if_t<std::is_signed_v<T>, std::nullptr_t>>
    explicit Number(T num, [[maybe_unused]] Signed unused = nullptr) noexcept : num_ {num}
    {
    }
    template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>>
    explicit Number(T num) noexcept : Number(static_cast<std::make_signed_t<T>>(num))
    {
    }
    DEFAULT_COPY_SEMANTIC(Number);
    DEFAULT_MOVE_SEMANTIC(Number);
    ~Number() = default;

    template <typename T>
    bool Is() const
    {
        return std::holds_alternative<T>(num_);
    }

    bool IsByte() const noexcept
    {
        return Is<int8_t>();
    }

    bool IsShort() const noexcept
    {
        return Is<int16_t>();
    }

    bool IsInt() const noexcept
    {
        return Is<int32_t>();
    }

    bool IsLong() const noexcept
    {
        return Is<int64_t>();
    }

    bool IsFloat() const noexcept
    {
        return Is<float>();
    }

    bool IsDouble() const noexcept
    {
        return Is<double>();
    }

    bool IsInteger() const noexcept
    {
        return IsByte() || IsShort() || IsInt() || IsLong();
    }

    bool IsReal() const noexcept
    {
        return IsFloat() || IsDouble();
    }

    bool ConversionError() const
    {
        bool error = (flags_ & NumberFlags::ERROR) != 0;
        ES2PANDA_ASSERT(error == std::holds_alternative<std::monostate>(num_));
        return error;
    }

    // NOLINTBEGIN(readability-else-after-return)
    template <typename T>
    bool CanGetValue() const
    {
        if constexpr (std::is_same_v<T, int64_t>) {
            return Is<int64_t>() || Is<int32_t>() || Is<int16_t>() || Is<int8_t>();
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return Is<int32_t>() || Is<int16_t>() || Is<int8_t>() || CanBeNarrowedTo<int32_t>();
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return Is<int16_t>() || Is<int8_t>() || CanBeNarrowedTo<int16_t>();
        } else if constexpr (std::is_same_v<T, int8_t>) {
            return Is<int8_t>() || CanBeNarrowedTo<int8_t>();
        } else if constexpr (std::is_same_v<T, double>) {
            return true;
        } else if constexpr (std::is_same_v<T, float>) {
            return IsInteger() || Is<float>() || CanBeNarrowedTo<float>();
        } else {
            static_assert(dependent_false_v<T>, "Invalid value type was requested for Number.");
            return false;
        }
    }

    template <typename T>
    auto GetValue() const
    {
        ES2PANDA_ASSERT(CanGetValue<T>());
        return std::visit(overloaded {[](auto value) { return static_cast<T>(value); },
                                      []([[maybe_unused]] std::monostate value) -> T { ES2PANDA_UNREACHABLE(); }},
                          num_);
    }

    template <typename TargetType>
    TargetType GetValueAndCastTo() const
    {
        return std::visit(
            overloaded {[](auto value) { return static_cast<TargetType>(value); },
                        []([[maybe_unused]] std::monostate value) -> TargetType { ES2PANDA_UNREACHABLE(); }},
            num_);
    }

    int8_t GetByte() const
    {
        return GetValue<int8_t>();
    }

    int16_t GetShort() const
    {
        return GetValue<int16_t>();
    }

    int32_t GetInt() const
    {
        return GetValue<int32_t>();
    }

    int64_t GetLong() const
    {
        return GetValue<int64_t>();
    }

    float GetFloat() const
    {
        return GetValue<float>();
    }

    double GetDouble() const
    {
        return GetValue<double>();
    }

    auto GetTypeRank() const
    {
        ES2PANDA_ASSERT(num_.index() != 0);
        return static_cast<TypeRank>(num_.index());
    }

    template <typename T>
    static constexpr auto ToTypeRank()
    {
        constexpr decltype(num_) V {T {}};
        return static_cast<TypeRank>(V.index());
    }

    template <typename T>
    bool TryNarrowTo()
    {
        if (CanGetValue<T>()) {
            num_ = GetValue<T>();
            return true;
        }
        return false;
    }

    void NarrowToLowest()
    {
        ES2PANDA_ASSERT(IsInteger());
        TryNarrowTo<int8_t>() || TryNarrowTo<int16_t>() || TryNarrowTo<int32_t>();
    }

    const util::StringView &Str() const
    {
        return str_;
    }

    void SetStr(util::StringView str)
    {
        str_ = str;
    }

    bool IsZero() const
    {
        return std::visit(overloaded {[]([[maybe_unused]] std::monostate value) -> bool { ES2PANDA_UNREACHABLE(); },
                                      [](auto &value) { return value == 0; }},
                          num_);
    }

    // NOLINTEND(readability-else-after-return)

    template <typename RT>
    void SetValue(RT &&value)
    {
        using T = typename std::remove_cv_t<typename std::remove_reference_t<RT>>;

        if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, int32_t> || std::is_same_v<T, int16_t> ||
                      std::is_same_v<T, int8_t> || std::is_same_v<T, double> || std::is_same_v<T, float>) {
            num_ = std::forward<RT>(value);
        } else {
            static_assert(dependent_false_v<T>, "Invalid value type was requested for Number.");
        }
    }

private:
    template <typename Dst, typename Src>
    static bool InRange(Src value)
    {
        static_assert(std::is_arithmetic_v<Dst> && std::is_arithmetic_v<Src>);
        if constexpr (std::is_same_v<Src, float>) {
            return false;
        }
        if constexpr (std::is_same_v<Src, double> && std::is_same_v<Dst, float>) {
            const auto min = static_cast<Src>(std::numeric_limits<Dst>::lowest());
            const auto max = static_cast<Src>(std::numeric_limits<Dst>::max());
            return (min <= value) && (value <= max);
        }
        if constexpr (std::is_integral_v<Src> && std::is_integral_v<Dst>) {
            static_assert(ToTypeRank<Dst>() < ToTypeRank<Src>());
            static_assert(std::is_signed_v<Dst>);
            static_assert(std::is_signed_v<Src>);
            const auto min = static_cast<Src>(std::numeric_limits<Dst>::lowest());
            const auto max = static_cast<Src>(std::numeric_limits<Dst>::max());
            return (min <= value) && (value <= max);
        }
        return false;
    }

    template <typename T>
    bool CanBeNarrowedTo() const
    {
        auto canBeNarrowed = [](auto value) {
            if constexpr (ToTypeRank<decltype(value)>() > ToTypeRank<T>()) {
                return InRange<T>(value);
            }
            return false;
        };
        auto unreachable = []([[maybe_unused]] std::monostate value) -> bool { ES2PANDA_UNREACHABLE(); };
        return std::visit(overloaded {canBeNarrowed, unreachable}, num_);
    }

private:
    util::StringView str_ {};
    std::variant<std::monostate, int8_t, int16_t, int32_t, int64_t, float, double> num_;
    NumberFlags flags_ {};
};
}  // namespace ark::es2panda::lexer

#endif
